/** @file wlan_tx.c
  * @brief This file contains the handling of TX in wlan
  * driver.
  * 
  *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
  */
/********************************************************
Change log:
	09/28/05: Add Doxygen format comments
	12/13/05: Add Proprietary periodic sleep support
	01/05/06: Add kernel 2.6.x support	
	04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
********************************************************/

#include	"include.h"

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function processes a single packet and sends
 *  to IF layer
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param skb     A pointer to skb which includes TX packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
SendSinglePacket(wlan_private * priv, struct sk_buff *skb)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    TxPD LocalTxPD;
    TxPD *pLocalTxPD = &LocalTxPD;
    u8 *ptr = Adapter->TmpTxBuf;

    ENTER();

    if (!skb->len || (skb->len > MRVDRV_ETH_TX_PACKET_BUFFER_SIZE)) {
        PRINTM(ERROR, "Tx Error: Bad skb length %d : %d\n",
               skb->len, MRVDRV_ETH_TX_PACKET_BUFFER_SIZE);
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    memset(pLocalTxPD, 0, sizeof(TxPD));

    pLocalTxPD->TxPacketLength = skb->len;

    if (Adapter->wmm.enabled) {
        /* 
         * original skb->priority has been overwritten 
         * by wmm_map_and_add_skb()
         */
        pLocalTxPD->Priority = (u8) skb->priority;

        pLocalTxPD->PktDelay_2ms = wmm_compute_driver_packet_delay(skb);
    }

    if (pLocalTxPD->Priority < NELEMENTS(Adapter->wmm.userPriPktTxCtrl)) {
        /* 
         * Set the priority specific TxControl field, setting of 0 will
         *   cause the default value to be used later in this function
         */
        pLocalTxPD->TxControl
            = Adapter->wmm.userPriPktTxCtrl[pLocalTxPD->Priority];
    }

    if (Adapter->PSState != PS_STATE_FULL_POWER) {
        if (TRUE == CheckLastPacketIndication(priv)) {
            Adapter->TxLockFlag = TRUE;
            pLocalTxPD->Flags = MRVDRV_TxPD_POWER_MGMT_LAST_PACKET;
        }
    }

    /* offset of actual data */
    pLocalTxPD->TxPacketLocation = sizeof(TxPD);

    if (pLocalTxPD->TxControl == 0) {
        /* TxCtrl set by user or default */
        pLocalTxPD->TxControl = Adapter->PktTxCtrl;
    }

    endian_convert_TxPD(pLocalTxPD);

    memcpy((u8 *) pLocalTxPD->TxDestAddr, skb->data, MRVDRV_ETH_ADDR_LEN);

    ptr += SDIO_HEADER_LEN;
    memcpy(ptr, pLocalTxPD, sizeof(TxPD));

    ptr += sizeof(TxPD);

    memcpy(ptr, skb->data, skb->len);

    ret = sbi_host_to_card(priv, MVMS_DAT, Adapter->TmpTxBuf,
                           skb->len + sizeof(TxPD));
    if (ret) {
        PRINTM(ERROR,
               "SendSinglePacket Error: sbi_host_to_card failed: 0x%X\n",
               ret);
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }

    PRINTM(DATA, "Data => FW\n");
    DBG_HEXDUMP(DAT_D, "Tx", ptr - sizeof(TxPD),
                MIN(skb->len + sizeof(TxPD), MAX_DATA_DUMP_LEN));

    wmm_process_fw_iface_tx_xfer_start(priv);

  done:
    if (!ret) {
        priv->stats.tx_packets++;
        priv->stats.tx_bytes += skb->len;
    } else {
        priv->stats.tx_dropped++;
        priv->stats.tx_errors++;
    }

    /* need to be freed in all cases */
    os_free_tx_packet(priv);

    LEAVE();
    return ret;
}

/********************************************************
		Global functions
********************************************************/

/** 
 *  @brief This function checks the conditions and sends packet to IF
 *  layer if everything is ok.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
void
wlan_process_tx(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    OS_INTERRUPT_SAVE_AREA;

    if (priv->wlan_dev.dnld_sent) {
        PRINTM(MSG, "TX Error: dnld_sent = %d, not sending\n",
               priv->wlan_dev.dnld_sent);
        goto done;
    }

    SendSinglePacket(priv, Adapter->CurrentTxSkb);
    OS_INT_DISABLE;
    priv->adapter->HisRegCpy &= ~HIS_TxDnLdRdy;
    OS_INT_RESTORE;

  done:
    LEAVE();
}

/** 
 *  @brief This function queues the packet received from
 *  kernel/upper layer and wake up the main thread to handle it.
 *  
 *  @param priv    A pointer to wlan_private structure
  * @param skb     A pointer to skb which includes TX packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_tx_packet(wlan_private * priv, struct sk_buff *skb)
{
    ulong flags;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    HEXDUMP("TX Data", skb->data, MIN(skb->len, 100));

    spin_lock_irqsave(&Adapter->CurrentTxLock, flags);

    wmm_map_and_add_skb(priv, skb);
    wake_up_interruptible(&priv->MainThread.waitQ);
    spin_unlock_irqrestore(&Adapter->CurrentTxLock, flags);

    LEAVE();

    return ret;
}

/** 
 *  @brief This function tells firmware to send a NULL data packet.
 *  
 *  @param priv     A pointer to wlan_private structure
 *  @param flags    Trasnit Pkt Flags
 *  @return 	    n/a
 */
int
SendNullPacket(wlan_private * priv, u8 flags)
{
    wlan_adapter *Adapter = priv->adapter;
    TxPD txpd = { 0 };
    int ret = WLAN_STATUS_SUCCESS;
    u8 *ptr = Adapter->TmpTxBuf;

    ENTER();

    if (Adapter->SurpriseRemoved == TRUE) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    memset(&txpd, 0, sizeof(TxPD));

    txpd.TxControl = Adapter->PktTxCtrl;
    txpd.Flags = flags;
    txpd.Priority = WMM_HIGHEST_PRIORITY;
    txpd.TxPacketLocation = sizeof(TxPD);

    endian_convert_TxPD(&txpd);

    ptr += SDIO_HEADER_LEN;
    memcpy(ptr, &txpd, sizeof(TxPD));

    ret = sbi_host_to_card(priv, MVMS_DAT, Adapter->TmpTxBuf, sizeof(TxPD));

    if (ret != 0) {
        PRINTM(ERROR, "TX Error: SendNullPacket failed!\n");
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }
    PRINTM(DATA, "Null data => FW\n");
    DBG_HEXDUMP(DAT_D, "Tx", ptr, sizeof(TxPD));

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function check if we need send last packet indication.
 *  
 *  @param priv     A pointer to wlan_private structure
 *
 *  @return 	   TRUE or FALSE
 */
BOOLEAN
CheckLastPacketIndication(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    BOOLEAN ret = FALSE;
    BOOLEAN prop_ps = TRUE;

    ENTER();

    if (Adapter->sleep_period.period == 0 || Adapter->gen_null_pkg == FALSE     /* for UPSD certification tests */
        ) {
        LEAVE();
        return ret;
    }

    if (wmm_lists_empty(priv)) {
        if (((Adapter->CurBssParams.wmm_uapsd_enabled == TRUE)
             && (Adapter->wmm.qosinfo != 0)) || prop_ps) {
            ret = TRUE;
        }
    }

    LEAVE();
    return ret;
}
