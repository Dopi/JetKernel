/** @file wlan_fw.c
  * @brief This file contains the initialization for FW
  * and HW 
  * 
  *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2007
  */
/********************************************************
Change log:
	09/28/05: Add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	01/11/06: Conditionalize new scan/join functions.
	          Cleanup association response handler initialization.
	01/06/05: Add FW file read
	05/08/06: Remove the 2nd GET_HW_SPEC command and TempAddr/PermanentAddr
	06/30/06: replaced MODULE_PARM(name, type) with module_param(name, type, perm)

********************************************************/

#include	"include.h"
#include <linux/vmalloc.h>
#include <linux/firmware.h>

/********************************************************
		Local Variables
********************************************************/

extern const char *helper_name;
extern const char *fw_name;

#if 0
module_param(helper_name, charp, 0);
module_param(fw_name, charp, 0);
#endif

#ifdef MFG_CMD_SUPPORT
int mfgmode = 0;
module_param(mfgmode, int, 0);
#endif

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function downloads firmware image, gets
 *  HW spec from firmware and set basic parameters to
 *  firmware.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_setup_station_hw(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *adapter = priv->adapter;
    u8 *ptr = NULL;
    u32 len = 0;

    //HostCmd_DS_SDIO_INT_CONFIG sdio_int_cfg;

    ENTER();

    sbi_disable_host_int(priv);

#if 0
    if ((intmode == INTMODE_GPIO) && (gpiopin == 0)) {
        PRINTM(MSG, "Invalid gpio pin#\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
#endif

    adapter->fmimage = NULL;
    adapter->fmimage_len = 0;
    adapter->helper = NULL;
    adapter->helper_len = 0;
	
#if 0
    if (helper_name != NULL) {
        if (fw_read(helper_name, &ptr, &len) != WLAN_STATUS_FAILURE) {
            adapter->helper = ptr;
            adapter->helper_len = len;
            PRINTM(INFO, "helper read success, len=%x\n", len);
        } else {
            PRINTM(MSG, "helper %s read fail.\n", helper_name);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
    }
#else
	const struct firmware *fw;

	if (!helper_name) {
		ret = request_firmware(&fw, helper_name, NULL);
		if (ret) {
            		PRINTM(MSG, "helper %s read fail.\n", helper_name);
            		ret = WLAN_STATUS_FAILURE;
            		goto done;
		} else {
		            adapter->helper = fw->data;
		            adapter->helper_len = fw->size;
		            PRINTM(INFO, "helper read success, len=%x\n", fw->size);
		}
	}
#endif

#if 0
    if (fw_name != NULL) {
        if (fw_read(fw_name, &ptr, &len) != WLAN_STATUS_FAILURE) {
            adapter->fmimage = ptr;
            adapter->fmimage_len = len;
            PRINTM(INFO, "fw read success, len=%x\n", len);
        } else {
            PRINTM(MSG, "fw %s read fail.\n", fw_name);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
    }
#else
	if (!fw_name) {
		ret = request_firmware(&fw, fw_name, NULL);
		if (ret) {
			PRINTM(MSG, "fw %s read fail.\n", fw_name);
			ret = WLAN_STATUS_FAILURE;
			goto done;
		} else {
			adapter->fmimage = fw->data;
			adapter->fmimage_len = fw->size;
			PRINTM(INFO, "fw read success, len=%x\n", fw->size);
		}
	}
#endif
    /* Download the helper */
    ret = sbi_prog_helper(priv);

    if (ret) {
        PRINTM(INFO, "Bootloader in invalid state!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    /* Download the main firmware via the helper firmware */
    if (sbi_prog_firmware_w_helper(priv)) {
        PRINTM(INFO, "Wlan FW download failed!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* check if the fimware is downloaded successfully or not */
    if (sbi_verify_fw_download(priv)) {
        PRINTM(INFO, "FW failed to be active in time!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
#define RF_REG_OFFSET 0x07
#define RF_REG_VALUE  0xc8

    sbi_enable_host_int(priv);

#define INT_RASING_EDGE		0
#define INT_FALLING_EDGE	1

#define DELAY_1_US		1
#if 0
    if (intmode == INTMODE_GPIO) {
        /* This command should be issued first */
        sdio_int_cfg.Action = HostCmd_ACT_GEN_SET;
        sdio_int_cfg.Gpio_pin = gpiopin;
        sdio_int_cfg.Gpio_int_edge = INT_FALLING_EDGE;
        sdio_int_cfg.Gpio_pulse_width = DELAY_1_US;
        ret = PrepareAndSendCommand(priv, HostCmd_CMD_SDIO_GPIO_INT_CONFIG,
                                    0, HostCmd_OPTION_WAITFORRSP,
                                    0, &sdio_int_cfg);
    }
#endif
#ifdef MFG_CMD_SUPPORT
    if (mfgmode == 0) {
#endif

        /*
         * Read MAC address from HW
         */
        memset(adapter->CurrentAddr, 0xff, MRVDRV_ETH_ADDR_LEN);

        ret = PrepareAndSendCommand(priv, HostCmd_CMD_GET_HW_SPEC,
                                    0, HostCmd_OPTION_WAITFORRSP, 0, NULL);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_MAC_CONTROL,
                                    0, HostCmd_OPTION_WAITFORRSP, 0,
                                    &adapter->CurrentPacketFilter);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_FW_WAKE_METHOD,
                                    HostCmd_ACT_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0,
                                    &priv->adapter->fwWakeupMethod);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
#ifdef MFG_CMD_SUPPORT
    }
#endif

#ifdef MFG_CMD_SUPPORT
    if (mfgmode == 0) {
#endif
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                                    HostCmd_ACT_GEN_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, NULL);
        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        priv->adapter->DataRate = 0;
        ret = PrepareAndSendCommand(priv,
                                    HostCmd_CMD_802_11_RF_TX_POWER,
                                    HostCmd_ACT_GEN_GET,
                                    HostCmd_OPTION_WAITFORRSP, 0, NULL);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
#ifdef MFG_CMD_SUPPORT
    }
#endif

    ret = WLAN_STATUS_SUCCESS;
  done:
    if (adapter->helper != NULL) {
        fw_buffer_free(adapter->helper);
    }
    if (adapter->fmimage != NULL) {
        fw_buffer_free(adapter->fmimage);
    }

    LEAVE();

    return (ret);
}

/** 
 *  @brief This function initializes timers.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
static void
init_sync_objects(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    InitializeTimer(&Adapter->MrvDrvCommandTimer,
                    MrvDrvCommandTimerFunction, priv);
    Adapter->CommandTimerIsSet = FALSE;

#ifdef REASSOCIATION
    /* Initialize the timer for the reassociation */
    InitializeTimer(&Adapter->MrvDrvTimer, MrvDrvReassocTimerFunction, priv);
    Adapter->ReassocTimerIsSet = FALSE;
#endif /* REASSOCIATION */

    return;
}

/** 
 *  @brief This function allocates buffer for the member of adapter
 *  structure like command buffer and BSSID list.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_allocate_adapter(wlan_private * priv)
{
    u32 ulBufSize;
    wlan_adapter *Adapter = priv->adapter;

    BSSDescriptor_t *pTempScanTable;

    /* Allocate buffer to store the BSSID list */
    ulBufSize = sizeof(BSSDescriptor_t) * MRVDRV_MAX_BSSID_LIST;
    if (!(pTempScanTable = kmalloc(ulBufSize, GFP_KERNEL))) {
        return WLAN_STATUS_FAILURE;
    }

    Adapter->ScanTable = pTempScanTable;
    memset(Adapter->ScanTable, 0, ulBufSize);

    if (!(Adapter->bgScanConfig =
          kmalloc(sizeof(HostCmd_DS_802_11_BG_SCAN_CONFIG), GFP_KERNEL))) {
        return WLAN_STATUS_FAILURE;
    }
    Adapter->bgScanConfigSize = sizeof(HostCmd_DS_802_11_BG_SCAN_CONFIG);
    memset(Adapter->bgScanConfig, 0, Adapter->bgScanConfigSize);

    spin_lock_init(&Adapter->QueueSpinLock);

    /* Allocate the command buffers */
    if (AllocateCmdBuffer(priv) != WLAN_STATUS_SUCCESS) {
        return WLAN_STATUS_FAILURE;
    }

    memset(&Adapter->PSConfirmSleep, 0, sizeof(PS_CMD_ConfirmSleep));
    Adapter->PSConfirmSleep.SeqNum = wlan_cpu_to_le16(++Adapter->SeqNum);
    Adapter->PSConfirmSleep.Command =
        wlan_cpu_to_le16(HostCmd_CMD_802_11_PS_MODE);
    Adapter->PSConfirmSleep.Size =
        wlan_cpu_to_le16(sizeof(PS_CMD_ConfirmSleep));
    Adapter->PSConfirmSleep.Result = 0;
    Adapter->PSConfirmSleep.Action =
        wlan_cpu_to_le16(HostCmd_SubCmd_Sleep_Confirmed);

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function initializes the adapter structure
 *  and set default value to the member of adapter.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
static void
wlan_init_adapter(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int i;

    Adapter->ScanProbes = 0;

    Adapter->bcn_avg_factor = DEFAULT_BCN_AVG_FACTOR;
    Adapter->data_avg_factor = DEFAULT_DATA_AVG_FACTOR;

    /* ATIM params */
    Adapter->AtimWindow = 0;
    Adapter->ATIMEnabled = FALSE;

    Adapter->MediaConnectStatus = WlanMediaStateDisconnected;

    memset(Adapter->CurrentAddr, 0xff, MRVDRV_ETH_ADDR_LEN);

    /* Status variables */
    Adapter->HardwareStatus = WlanHardwareStatusInitializing;

    /* scan type */
    Adapter->ScanType = HostCmd_SCAN_TYPE_ACTIVE;

    /* scan mode */
    Adapter->ScanMode = HostCmd_BSS_TYPE_ANY;

    /* scan time */
    Adapter->SpecificScanTime = MRVDRV_SPECIFIC_SCAN_CHAN_TIME;
    Adapter->ActiveScanTime = MRVDRV_ACTIVE_SCAN_CHAN_TIME;
    Adapter->PassiveScanTime = MRVDRV_PASSIVE_SCAN_CHAN_TIME;

    /* 802.11 specific */
    Adapter->SecInfo.WEPStatus = Wlan802_11WEPDisabled;
    for (i = 0; i < sizeof(Adapter->WepKey) / sizeof(Adapter->WepKey[0]); i++)
        memset(&Adapter->WepKey[i], 0, sizeof(MRVL_WEP_KEY));
    Adapter->CurrentWepKeyIndex = 0;
    Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
    Adapter->SecInfo.EncryptionMode = CIPHER_NONE;
    Adapter->AdhocAESEnabled = FALSE;
    Adapter->InfrastructureMode = Wlan802_11Infrastructure;

    Adapter->NumInScanTable = 0;
    Adapter->pAttemptedBSSDesc = NULL;
#ifdef REASSOCIATION
    OS_INIT_SEMAPHORE(&Adapter->ReassocSem);
#endif
    Adapter->pBeaconBufEnd = Adapter->beaconBuffer;

    Adapter->HisRegCpy |= HIS_TxDnLdRdy;

    memset(&Adapter->CurBssParams, 0, sizeof(Adapter->CurBssParams));

    /* PnP and power profile */
    Adapter->SurpriseRemoved = FALSE;

    Adapter->CurrentPacketFilter =
        HostCmd_ACT_MAC_RTS_CTS_ENABLE |
        HostCmd_ACT_MAC_RX_ON | HostCmd_ACT_MAC_TX_ON;

    Adapter->RadioOn = RADIO_ON;
#ifdef REASSOCIATION
#if (WIRELESS_EXT >= 18)
    Adapter->Reassoc_on = FALSE;
#else
    Adapter->Reassoc_on = TRUE;
#endif
#endif /* REASSOCIATION */
    Adapter->TxAntenna = RF_ANTENNA_2;
    Adapter->RxAntenna = RF_ANTENNA_AUTO;

    Adapter->HWRateDropMode = HW_TABLE_RATE_DROP;
    Adapter->Is_DataRate_Auto = TRUE;
    Adapter->BeaconPeriod = MRVDRV_BEACON_INTERVAL;

    Adapter->AdhocChannel = DEFAULT_AD_HOC_CHANNEL;

    Adapter->PSMode = Wlan802_11PowerModeCAM;
    Adapter->MultipleDtim = MRVDRV_DEFAULT_MULTIPLE_DTIM;

    Adapter->ListenInterval = MRVDRV_DEFAULT_LISTEN_INTERVAL;

    Adapter->PSState = PS_STATE_FULL_POWER;

    Adapter->NeedToWakeup = FALSE;
    Adapter->LocalListenInterval = 0;   /* default value in firmware will be used */
    Adapter->fwWakeupMethod = WAKEUP_FW_UNCHANGED;

    Adapter->IsDeepSleep = FALSE;
    Adapter->IsAutoDeepSleepEnabled = FALSE;

    Adapter->bWakeupDevRequired = FALSE;

    Adapter->WakeupTries = 0;
    Adapter->bHostSleepConfigured = FALSE;

    Adapter->HSCfg.conditions = HOST_SLEEP_CFG_CANCEL;
    Adapter->HSCfg.gpio = 0;
    Adapter->HSCfg.gap = 0;

    Adapter->DataRate = 0;      // Initially indicate the rate as auto 

    Adapter->adhoc_grate_enabled = FALSE;

    Adapter->IntCounter = Adapter->IntCounterSaved = 0;

    INIT_LIST_HEAD((struct list_head *) &Adapter->RxSkbQ);

    Adapter->gen_null_pkg = TRUE;       /*Enable NULL Pkg generation */

    init_waitqueue_head(&Adapter->cmd_EncKey);

    spin_lock_init(&Adapter->CurrentTxLock);

    Adapter->CurrentTxSkb = NULL;
    Adapter->PktTxCtrl = 0;

    return;
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief This function initializes firmware
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_init_fw(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* Allocate adapter structure */
    if ((ret = wlan_allocate_adapter(priv)) != WLAN_STATUS_SUCCESS) {
        goto done;
    }

    /* init adapter structure */
    wlan_init_adapter(priv);

    /* init timer etc. */
    init_sync_objects(priv);

    /* download fimrware etc. */
    if ((ret = wlan_setup_station_hw(priv)) != WLAN_STATUS_SUCCESS) {
        Adapter->HardwareStatus = WlanHardwareStatusNotReady;
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    /* init 802.11d */
    wlan_init_11d(priv);

    Adapter->HardwareStatus = WlanHardwareStatusReady;
    ret = WLAN_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function frees the structure of adapter
 *    
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   n/a
 */
void
wlan_free_adapter(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!Adapter) {
        PRINTM(INFO, "Why double free adapter?:)\n");
        return;
    }

    PRINTM(INFO, "Free Command buffer\n");
    FreeCmdBuffer(priv);

    PRINTM(INFO, "Free CommandTimer\n");
    if (Adapter->CommandTimerIsSet) {
        CancelTimer(&Adapter->MrvDrvCommandTimer);
        Adapter->CommandTimerIsSet = FALSE;
    }
    FreeTimer(&Adapter->MrvDrvCommandTimer);
#ifdef REASSOCIATION
    PRINTM(INFO, "Free MrvDrvTimer\n");
    if (Adapter->ReassocTimerIsSet) {
        CancelTimer(&Adapter->MrvDrvTimer);
        Adapter->ReassocTimerIsSet = FALSE;
    }
    FreeTimer(&Adapter->MrvDrvTimer);
#endif /* REASSOCIATION */

    if (Adapter->bgScanConfig) {
        kfree(Adapter->bgScanConfig);
        Adapter->bgScanConfig = NULL;
    }

    OS_FREE_LOCK(&Adapter->CurrentTxLock);
    OS_FREE_LOCK(&Adapter->QueueSpinLock);

    PRINTM(INFO, "Free ScanTable\n");
    if (Adapter->ScanTable) {
        kfree(Adapter->ScanTable);
        Adapter->ScanTable = NULL;
    }

    PRINTM(INFO, "Free Adapter\n");

    /* Free the adapter object itself */
    kfree(Adapter);
    priv->adapter = NULL;
    LEAVE();
}
