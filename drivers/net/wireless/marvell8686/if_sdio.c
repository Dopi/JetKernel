/** @file if_sdio.c
 *  @brief This file contains SDIO IF (interface) module
 *  related functions.
 *
 *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2007
 */
/****************************************************
Change log:
	10/14/05: add Doxygen format comments 
	01/05/06: add kernel 2.6.x support
	01/23/06: add fw downlaod
	06/06/06: add macro SD_BLOCK_SIZE_FW_DL for firmware download
		  add macro ALLOC_BUF_SIZE for cmd resp/Rx data skb buffer allocation
****************************************************/
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <linux/mmc/sdio_ids.h>

#include	"if_sdio.h"

//#undef PRINTM
//#define PRINTM(INFO, msg...) printk(msg)

/* define SD block size for firmware download */
#define SD_BLOCK_SIZE_FW_DL	32

/* define SD block size for data Tx/Rx */
#define SD_BLOCK_SIZE		320     /* To minimize the overhead of ethernet frame
                                           with 1514 bytes, 320 bytes block size is used */

#define ALLOC_BUF_SIZE		(((MAX(MRVDRV_ETH_RX_PACKET_BUFFER_SIZE, \
					MRVDRV_SIZE_OF_CMD_BUFFER) + SDIO_HEADER_LEN \
					+ SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE)

/* Max retry number of CMD53 write */
#define MAX_WRITE_IOMEM_RETRY	2

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

struct if_sdio_card {
	struct sdio_func        *func;
	wlan_private            *priv;
	int                     model;

	u8 int_cause;

	u8 chiprev;
	u8 async_int_mode;
	u8 block_size_512;
	card_capability info;
};

extern wlan_private *wlanpriv;
const char *helper_name;
const char *fw_name;
/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief This function adds the card
 *  
 *  @param card    A pointer to the card
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_add_card(void *card)
{
	struct if_sdio_card *sdio_card = (struct if_sdio_card*)card;
	sdio_card->priv = wlan_add_card(card);
	if (sdio_card->priv)
		return WLAN_STATUS_SUCCESS;
	else
		return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function removes the card
 *  
 *  @param card    A pointer to the card
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_remove_card(void *card)
{
	return wlan_remove_card(card);
}

/** 
 *  @brief This function reads scratch registers
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_read_scratch(wlan_private * priv, u16 * dat)
{
	int ret = WLAN_STATUS_SUCCESS;
	u8 scr0;
	u8 scr1;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
	
	scr0 = sdio_readb(card->func, CARD_OCR_0_REG, &ret);
	if (ret)
		return WLAN_STATUS_FAILURE;

	scr1 = sdio_readb(card->func, CARD_OCR_1_REG, &ret);
	PRINTM(INFO, "CARD_OCR_0_REG = 0x%x, CARD_OCR_1_REG = 0x%x\n", scr0,
	       scr1);
	if (ret)
		return WLAN_STATUS_FAILURE;

	*dat = (((u16) scr1) << 8) | scr0;

	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function polls the card status register.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param bits    	the bit mask
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_poll_card_status(wlan_private * priv, u8 bits)
{
	int tries;
	int rval;
	u8 cs;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
	
	for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
		cs = sdio_readb(card->func, CARD_STATUS_REG, &rval);
		if (rval == 0 && (cs & bits) == bits) {
			return WLAN_STATUS_SUCCESS;
		}

		mdelay(1);
	}

	PRINTM(WARN, "mv_sdio_poll_card_status: FAILED!:%d\n", rval);
	return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function programs the firmware image.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param firmware 	A pointer to the buffer of firmware image
 *  @param firmwarelen 	the length of firmware image
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_prog_firmware_image(wlan_private * priv,
			const u8 * firmware, int firmwarelen)
{
	int ret = WLAN_STATUS_SUCCESS;
	u16 firmwarestat;
	u8 *fwbuf = priv->adapter->TmpTxBuf;
	int fwblknow;
	u32 tx_len;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
#ifdef FW_DOWNLOAD_SPEED
	u32 tv1, tv2;
#endif

	ENTER();

	sdio_claim_host(card->func);
	ret = sdio_set_block_size(card->func, SD_BLOCK_SIZE_FW_DL);
	if (ret) {
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	if ((ret = mv_sdio_read_scratch(priv, &firmwarestat)) < 0) {
		PRINTM(INFO, "read scratch returned <0\n");
		goto done;
	}

	if (firmwarestat == FIRMWARE_READY) {
		PRINTM(INFO, "FW already downloaded!\n");
		ret = WLAN_STATUS_SUCCESS;
		goto done;
	}

	PRINTM(INFO, "Downloading helper image (%d bytes), block size %d bytes\n",
	       firmwarelen, SD_BLOCK_SIZE_FW_DL);

#ifdef FW_DOWNLOAD_SPEED
	tv1 = get_utimeofday();
#endif
	/* Perform firmware data transfer */
	tx_len =
	(FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE_FW_DL) - SDIO_HEADER_LEN;
	for (fwblknow = 0; fwblknow < firmwarelen; fwblknow += tx_len) {

		/* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
		ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
		if (ret < 0) {
			PRINTM(FATAL, "FW download died @ %d\n", fwblknow);
			goto done;
		}

		/* Set blocksize to transfer - checking for last block */
		if (firmwarelen - fwblknow < tx_len)
			tx_len = firmwarelen - fwblknow;

		fwbuf[0] = ((tx_len & 0x000000ff) >> 0);	/* Little-endian */
		fwbuf[1] = ((tx_len & 0x0000ff00) >> 8);
		fwbuf[2] = ((tx_len & 0x00ff0000) >> 16);
		fwbuf[3] = ((tx_len & 0xff000000) >> 24);

		/* Copy payload to buffer */
		memcpy(&fwbuf[SDIO_HEADER_LEN], &firmware[fwblknow], tx_len);

		PRINTM(INFO, ".");

		/* Send data */
		ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
				    fwbuf, FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE_FW_DL);

		if (ret) {
			PRINTM(FATAL, "IO error: transferring block @ %d\n", fwblknow);
			goto done;
		}
	}

#ifdef FW_DOWNLOAD_SPEED
	tv2 = get_utimeofday();
	PRINTM(INFO, "helper: %ld.%03ld.%03ld ", tv1 / 1000000,
	       (tv1 % 1000000) / 1000, tv1 % 1000);
	PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000,
	       (tv2 % 1000000) / 1000, tv2 % 1000);
	tv2 -= tv1;
	PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000,
	       (tv2 % 1000000) / 1000, tv2 % 1000);
#endif

	/* Write last EOF data */
	PRINTM(INFO, "\nTransferring EOF block\n");
	memset(fwbuf, 0x0, SD_BLOCK_SIZE_FW_DL);
	ret = sdio_writesb(card->func, priv->wlan_dev.ioport, fwbuf, SD_BLOCK_SIZE_FW_DL);

	if (ret) {
		PRINTM(FATAL, "IO error in writing EOF FW block\n");
		goto done;
	}

	ret = WLAN_STATUS_SUCCESS;

done:
	sdio_set_block_size(card->func, 0);
	sdio_release_host(card->func);
	return ret;
}

/** 
 *  @brief This function downloads firmware image to the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param firmware	A pointer to firmware image buffer
 *  @param firmwarelen	the length of firmware image
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sbi_download_wlan_fw_image(wlan_private * priv,
			   const u8 * firmware, int firmwarelen)
{
	u8 base0;
	u8 base1;
	int ret = WLAN_STATUS_SUCCESS;
	int offset;
	u8 *fwbuf = priv->adapter->TmpTxBuf;
	int timeout = 5000;
	u16 len;
	int txlen = 0;
	int tx_blocks = 0;
	int i = 0;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
#ifdef FW_DOWNLOAD_SPEED
	u32 tv1, tv2;
#endif

	ENTER();

	PRINTM(INFO, "Downloading FW image (%d bytes)\n", firmwarelen);

#ifdef FW_DOWNLOAD_SPEED
	tv1 = get_utimeofday();
#endif
	sdio_claim_host(card->func);

	ret = sdio_set_block_size(card->func, SD_BLOCK_SIZE_FW_DL);
	if (ret) {
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	/* Wait initially for the first non-zero value */
	do {
		base0 = sdio_readb(card->func, HOST_F1_RD_BASE_0, &ret);
		if (ret) {
			PRINTM(WARN, "Dev BASE0 register read failed:"
			       " base0=0x%04X(%d)\n", base0, base0);
			ret = WLAN_STATUS_FAILURE;
			goto done;
		}
		base1 = sdio_readb(card->func, HOST_F1_RD_BASE_1, &ret);
		if (ret) {
			PRINTM(WARN, "Dev BASE1 register read failed:"
			       " base1=0x%04X(%d)\n", base1, base1);
			ret = WLAN_STATUS_FAILURE;
			goto done;
		}
		len = (((u16) base1) << 8) | base0;
		mdelay(1);
	} while (!len && --timeout);

	if (!timeout) {
		PRINTM(MSG, "Helper downloading finished.\n");
		PRINTM(MSG, "Timeout for FW downloading!\n");
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	/* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
	ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
	if (ret < 0) {
		PRINTM(FATAL, "FW download died, helper not ready\n");
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	len &= ~B_BIT_0;

	/* Perform firmware data transfer */
	for (offset = 0; offset < firmwarelen; offset += txlen) {
		txlen = len;

		/* Set blocksize to transfer - checking for last block */
		if (firmwarelen - offset < txlen) {
			txlen = firmwarelen - offset;
		}
		/* PRINTM(INFO, "fw: offset=%d, txlen = 0x%04X(%d)\n", 
		   offset,txlen,txlen); */
		PRINTM(INFO, ".");

		tx_blocks = (txlen + SD_BLOCK_SIZE_FW_DL - 1) / SD_BLOCK_SIZE_FW_DL;

		/* Copy payload to buffer */
		memcpy(fwbuf, &firmware[offset], txlen);

		/* Send data */
		ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
				       fwbuf, tx_blocks * SD_BLOCK_SIZE_FW_DL);

		if (ret) {
			PRINTM(ERROR, "FW download, write iomem (%d) failed: %d\n", i,
			       ret);
			sdio_writeb(card->func, 0x04, CONFIGURATION_REG, &ret);
			if (ret) {
				PRINTM(ERROR, "write ioreg failed (FN1 CFG)\n");
			}
		}

		/* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
		ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
		if (ret < 0) {
			PRINTM(FATAL, "FW download with helper died @ %d\n", offset);
			goto done;
		}

		base0 = sdio_readb(card->func, HOST_F1_RD_BASE_0, &ret);
		if (ret) {
			PRINTM(WARN, "Dev BASE0 register read failed:"
			       " base0=0x%04X(%d)\n", base0, base0);
			ret = WLAN_STATUS_FAILURE;
			goto done;
		}
		base1 = sdio_readb(card->func, HOST_F1_RD_BASE_1, &ret);
		if (ret) {
			PRINTM(WARN, "Dev BASE1 register read failed:"
			       " base1=0x%04X(%d)\n", base1, base1);
			ret = WLAN_STATUS_FAILURE;
			goto done;
		}
		len = (((u16) base1) << 8) | base0;

		if (!len) {
			break;
		}

		if (len & B_BIT_0) {
			i++;
			if (i > MAX_WRITE_IOMEM_RETRY) {
				PRINTM(FATAL, "FW download failure, over max retry count\n");
				ret = WLAN_STATUS_FAILURE;
				goto done;
			}
			PRINTM(ERROR, "CRC error indicated by the helper:"
			       " len = 0x%04X, txlen = %d\n", len, txlen);
			len &= ~B_BIT_0;
			/* Setting this to 0 to resend from same offset */
			txlen = 0;
		}
		else
			i = 0;
	}
	PRINTM(INFO, "\nFW download over, size %d bytes\n", firmwarelen);

	ret = WLAN_STATUS_SUCCESS;
done:
	sdio_set_block_size(card->func, 0);
	sdio_release_host(card->func);
#ifdef FW_DOWNLOAD_SPEED
	tv2 = get_utimeofday();
	PRINTM(INFO, "FW: %ld.%03ld.%03ld ", tv1 / 1000000,
	       (tv1 % 1000000) / 1000, tv1 % 1000);
	PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000,
	       (tv2 % 1000000) / 1000, tv2 % 1000);
	tv2 -= tv1;
	PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000,
	       (tv2 % 1000000) / 1000, tv2 % 1000);
#endif
	LEAVE();
	return ret;
}

/** 
 *  @brief This function reads data from the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param type	   	A pointer to keep type as data or command
 *  @param nb		A pointer to keep the data/cmd length retured in buffer
 *  @param payload 	A pointer to the data/cmd buffer
 *  @param nb	   	the length of data/cmd buffer
 *  @param npayload	the length of data/cmd buffer
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_card_to_host(wlan_private * priv,
		     u32 * type, int *nb, u8 * payload, int npayload)
{
	int ret = WLAN_STATUS_SUCCESS;
	u16 buf_len = 0;
	int buf_block_len;
	int blksz;
	u32 *pevent;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	ENTER();

	if (!payload) {
		PRINTM(WARN, "payload NULL pointer received!\n");
		ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	/* Read the length of data to be transferred */
	ret = mv_sdio_read_scratch(priv, &buf_len);
	if (ret < 0) {
		PRINTM(ERROR, "card_to_host, read RX length failed\n");
		ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	if (buf_len <= SDIO_HEADER_LEN || buf_len > npayload) {
		PRINTM(ERROR, "card_to_host, invalid packet length: %d\n", buf_len);
		ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	ret = mv_sdio_poll_card_status(priv, CARD_IO_READY);
	if (ret < 0) {
		PRINTM(FATAL, "MV card status fail\n");
		goto exit;
	}

	/* Allocate buffer */
	blksz = SD_BLOCK_SIZE;
	buf_block_len = (buf_len + blksz - 1) / blksz;

	ret = sdio_set_block_size(card->func, blksz);
	if (ret) {
		ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	ret = sdio_readsb(card->func, payload, priv->wlan_dev.ioport, buf_block_len * blksz);

	if (ret) {
		PRINTM(ERROR, "card_to_host, read iomem failed: %d\n", ret);
		ret = WLAN_STATUS_FAILURE;
		goto exit;
	}
	*nb = buf_len;

	DBG_HEXDUMP(IF_D, "SDIO Blk Rd", payload, blksz * buf_block_len);

	*type = (payload[2] | (payload[3] << 8));
	if (*type == MVSD_EVENT) {
		pevent = (u32 *) & payload[4];
		priv->adapter->EventCause = MVSD_EVENT | (((u16) (*pevent)) << 3);
	}

exit:
	sdio_set_block_size(card->func, 0);
	LEAVE();
	return ret;
}

/** 
 *  @brief This function enables the host interrupts mask
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   WLAN_STATUS_SUCCESS
 */
static int
enable_host_int_mask(wlan_private * priv, u8 mask)
{
	int ret = WLAN_STATUS_SUCCESS;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	sdio_claim_host(card->func);
	/* Simply write the mask to the register */
	sdio_writeb(card->func, mask, HOST_INT_MASK_REG, &ret);
	sdio_release_host(card->func);

	if (ret) {
		PRINTM(WARN, "ret = %d\n", ret);
		ret = WLAN_STATUS_FAILURE;
	}

	priv->adapter->HisRegCpy = 1;

	return ret;
}

/**  @brief This function disables the host interrupts mask.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
disable_host_int_mask(wlan_private * priv, u8 mask)
{
	int ret = WLAN_STATUS_SUCCESS;
	u8 host_int_mask;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	sdio_claim_host(card->func);
	/* Read back the host_int_mask register */
	host_int_mask = sdio_readb(card->func, HOST_INT_MASK_REG, &ret);
	if (ret) {
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	/* Update with the mask and write back to the register */
	host_int_mask &= ~mask;
	sdio_writeb(card->func, host_int_mask, HOST_INT_MASK_REG, &ret);
	if (ret) {
		PRINTM(WARN, "Unable to diable the host interrupt!\n");
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

done:
	sdio_release_host(card->func);
	return ret;
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief This function handles the interrupt.
 *  
 *  @param irq 	   The irq of device.
 *  @param dev_id  A pointer to net_device structure
 *  @param fp	   A pointer to pt_regs structure
 *  @return 	   n/a
 */
static void sbi_interrupt(struct sdio_func *func)
{
	int ret = WLAN_STATUS_SUCCESS;
	u8 sdio_ireg = 0;
	u8 *cmdBuf;
	wlan_private *priv;
	wlan_dev_t *wlan_dev;
	struct sk_buff *skb;
	struct if_sdio_card* card = NULL;
	ENTER();

	priv = wlanpriv;
	wlan_dev = &priv->wlan_dev;
	card = (struct if_sdio_card*)wlan_dev->card;

	sdio_ireg = sdio_readb(func, HOST_INTSTATUS_REG, &ret);
	if (ret) {
		PRINTM(WARN, "sdio_read_ioreg: read int status register failed\n");
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	sdio_writeb(func,  (~sdio_ireg) & (DN_LD_HOST_INT_STATUS | UP_LD_HOST_INT_STATUS),
		    HOST_INTSTATUS_REG, &ret);
	if (ret) {
		PRINTM(WARN, "sdio_write_ioreg: clear int status register failed\n");
		ret = WLAN_STATUS_FAILURE;
		goto done;
	}

	if (sdio_ireg & DN_LD_HOST_INT_STATUS) {    /* tx_done INT */
		OS_INT_DISABLE;
		card->int_cause |= HIS_TxDnLdRdy;
		wlan_interrupt(wlan_dev->netdev);
		OS_INT_RESTORE;
		if (!priv->wlan_dev.dnld_sent) {	/* tx_done already received */
			PRINTM(INFO, "warning: tx_done already received:"
			       " dnld_sent=0x%x int status=0x%x\n",
			       priv->wlan_dev.dnld_sent, sdio_ireg);
		}
		else {
			wmm_process_fw_iface_tx_xfer_end(priv);
			priv->wlan_dev.dnld_sent = DNLD_RES_RECEIVED;
		}
	}

	if (sdio_ireg & UP_LD_HOST_INT_STATUS) {

		/* 
		 * DMA read data is by block alignment,so we need alloc extra block
		 * to avoid wrong memory access.
		 */
		if (!(skb = dev_alloc_skb(ALLOC_BUF_SIZE))) {
			PRINTM(WARN, "No free skb\n");
			priv->stats.rx_dropped++;
			ret = WLAN_STATUS_FAILURE;
			goto done;
		}

		/* 
		 * Transfer data from card
		 * skb->tail is passed as we are calling skb_put after we
		 * are reading the data
		 */
		if (mv_sdio_card_to_host(priv, &wlan_dev->upld_typ,
					 (int *) &wlan_dev->upld_len, skb->tail,
					 ALLOC_BUF_SIZE) < 0) {
			u8 cr = 0;

			PRINTM(ERROR, "Card to host failed: int status=0x%x\n",
			       sdio_ireg);
			cr = sdio_readb(func, CONFIGURATION_REG, &ret);
			if (ret) 
				PRINTM(ERROR, "read ioreg failed (FN1 CFG)\n");

			PRINTM(INFO, "Config Reg val = %d\n", cr);
			sdio_writeb(func, cr | 0x04, CONFIGURATION_REG, &ret);
			if (ret)
				PRINTM(ERROR, "write ioreg failed (FN1 CFG)\n");

			PRINTM(INFO, "write success\n");
			cr = sdio_readb(func, CONFIGURATION_REG, &ret);
			if (ret)
				PRINTM(ERROR, "read ioreg failed (FN1 CFG)\n");

			PRINTM(INFO, "Config reg val =%x\n", cr);
			ret = WLAN_STATUS_FAILURE;
			kfree_skb(skb);
			goto done;
		}

		OS_INT_DISABLE;
		switch (wlan_dev->upld_typ) {
		case MVSD_DAT:
			PRINTM(DATA, "Data <= FW\n");
			card->int_cause |= HIS_RxUpLdRdy;
			skb_put(skb, priv->wlan_dev.upld_len);
			skb_pull(skb, SDIO_HEADER_LEN);
			list_add_tail((struct list_head *) skb,
				      (struct list_head *) &priv->adapter->RxSkbQ);
			/* skb will be freed by kernel later */
			break;

		case MVSD_CMD:
			PRINTM(DATA, "CMD\n");
			card->int_cause |= HIS_CmdUpLdRdy;

			/* take care of CurCmd = NULL case */
			if (!priv->adapter->CurCmd) {
				cmdBuf = priv->wlan_dev.upld_buf;
			}
			else {
				cmdBuf = priv->adapter->CurCmd->BufVirtualAddr;
			}

			priv->wlan_dev.upld_len -= SDIO_HEADER_LEN;
			memcpy(cmdBuf, skb->data + SDIO_HEADER_LEN,
			       MIN(MRVDRV_SIZE_OF_CMD_BUFFER, priv->wlan_dev.upld_len));
			kfree_skb(skb);
			break;

		case MVSD_EVENT:
			/* event cause has been saved to priv->adapter->EventCause */
			kfree_skb(skb);
			card->int_cause |= HIS_CardEvent;
			break;

		default:
			PRINTM(ERROR, "SDIO unknown upld type = 0x%x\n",
			       wlan_dev->upld_typ);
			kfree_skb(skb);
			break;
		}
		wlan_interrupt(wlan_dev->netdev);
		OS_INT_RESTORE;
	}

	ret = WLAN_STATUS_SUCCESS;
done:
	LEAVE();
	return;
}

/**
 *  @brief This function reads the IO register.
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param func	   funcion number
 *  @param reg	   register to be read
 *  @param dat	   A pointer to variable that keeps returned value
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_read_ioreg(wlan_private * priv, u8 func, u32 reg, u8 * dat)
{
	int ret;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
	sdio_claim_host(card->func);
	if (func == 0)
		*dat = sdio_f0_readb(card->func, reg, &ret);
	else
		*dat = sdio_readb(card->func, reg, &ret);
	sdio_release_host(card->func);
	return ret;
}

/**
 *  @brief This function writes the IO register.
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param func	   funcion number
 *  @param reg	   register to be written
 *  @param dat	   the value to be written
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_write_ioreg(wlan_private * priv, u8 func, u32 reg, u8 dat)
{
	int ret;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
	sdio_claim_host(card->func);
	if (func == 0)
		sdio_f0_writeb(card->func, dat, reg, &ret);
	else
		sdio_writeb(card->func, dat, reg, &ret);
	sdio_release_host(card->func);
	return ret;
}

/** 
 *  @brief This function checks the interrupt status and handle it accordingly.
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param ireg    A pointer to variable that keeps returned value
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_get_int_status(wlan_private * priv, u8 * ireg)
{
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;
	OS_INT_DISABLE;
	*ireg = card->int_cause;
	card->int_cause = 0;
	OS_INT_RESTORE;
	return WLAN_STATUS_SUCCESS;

}

/**
 *  @brief This function is a dummy function.
 *  
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_card_to_host(wlan_private * priv, u32 type,
		 u32 * nb, u8 * payload, u16 npayload)
{
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_read_event_cause(wlan_private * priv)
{
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function disables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_disable_host_int(wlan_private * priv)
{
	return disable_host_int_mask(priv, HIM_DISABLE);
}

/** 
 *  @brief This function enables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_enable_host_int(wlan_private * priv)
{
	return enable_host_int_mask(priv, HIM_ENABLE);
}

/** 
 *  @brief This function de-registers the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_unregister_dev(wlan_private * priv)
{
	ENTER();

	if (priv->wlan_dev.card != NULL) {
		/* Release the SDIO IRQ */
		PRINTM(WARN, "Making the sdio dev card as NULL\n");
	}

	LEAVE();
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function registers the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_register_dev(wlan_private * priv)
{
	int ret = WLAN_STATUS_SUCCESS;
	u8 reg;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	ENTER();

	/* Initialize the private structure */
	strncpy(priv->wlan_dev.name, "sdio0", sizeof(priv->wlan_dev.name));
	priv->wlan_dev.ioport = 0;
	priv->wlan_dev.upld_rcv = 0;
	priv->wlan_dev.upld_typ = 0;
	priv->wlan_dev.upld_len = 0;

	sdio_claim_host(card->func);
	/* Read the IO port */
	reg = sdio_readb(card->func, IO_PORT_0_REG, &ret);
	if (ret) 
		goto failed;
	else
		priv->wlan_dev.ioport |= reg;

	reg = sdio_readb(card->func, IO_PORT_1_REG, &ret);
	if (ret) 
		goto failed;
	else
		priv->wlan_dev.ioport |= (reg << 8);

	reg = sdio_readb(card->func, IO_PORT_2_REG, &ret);
	if (ret) 
		goto failed;
	else
		priv->wlan_dev.ioport |= (reg << 16);
	sdio_release_host(card->func);

	PRINTM(INFO, "SDIO FUNC1 IO port: 0x%x\n", priv->wlan_dev.ioport);

	/* Disable host interrupt first. */
	if ((ret = disable_host_int_mask(priv, 0xff)) < 0) {
		PRINTM(WARN, "Warning: unable to disable host interrupt!\n");
	}

	priv->adapter->chip_rev = card->chiprev;
	priv->adapter->sdiomode = 4;

	return WLAN_STATUS_SUCCESS;

failed:
	sdio_release_host(card->func);
	priv->wlan_dev.card = NULL;

	return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function sends data to the card.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param type	   data or command
 *  @param payload A pointer to the data/cmd buffer
 *  @param nb	   the length of data/cmd
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_host_to_card(wlan_private * priv, u8 type, u8 * payload, u16 nb)
{
	int ret = WLAN_STATUS_SUCCESS;
	int buf_block_len;
	int blksz;
	int i = 0;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	ENTER();

	sdio_claim_host(card->func);
	priv->adapter->HisRegCpy = 0;

	/* Allocate buffer and copy payload */
	blksz = SD_BLOCK_SIZE;
	buf_block_len = (nb + SDIO_HEADER_LEN + blksz - 1) / blksz;

	/* This is SDIO specific header
	 *  length: byte[1][0], 
	 *  type: byte[3][2] (MVSD_DAT = 0, MVSD_CMD = 1, MVSD_EVENT = 3) 
	 */
	priv->adapter->TmpTxBuf[0] = (nb + SDIO_HEADER_LEN) & 0xff;
	priv->adapter->TmpTxBuf[1] = ((nb + SDIO_HEADER_LEN) >> 8) & 0xff;
	priv->adapter->TmpTxBuf[2] = type;
	priv->adapter->TmpTxBuf[3] = 0x0;

	if (payload != NULL &&
	    (nb > 0 &&
	     nb <= (sizeof(priv->adapter->TmpTxBuf) - SDIO_HEADER_LEN))) {
		if (type == MVMS_CMD)
			memcpy(&priv->adapter->TmpTxBuf[SDIO_HEADER_LEN], payload, nb);
	}
	else {
		PRINTM(WARN, "sbi_host_to_card(): Error: payload=%p, nb=%d\n",
		       payload, nb);
	}

	if (type == MVSD_DAT)
		priv->wlan_dev.dnld_sent = DNLD_DATA_SENT;
	else
		priv->wlan_dev.dnld_sent = DNLD_CMD_SENT;

	ret = sdio_set_block_size(card->func, blksz);
	if (ret) {
		ret = WLAN_STATUS_FAILURE;
		goto exit;
	}

	do {
		/* Transfer data to card */
		ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
				   priv->adapter->TmpTxBuf, blksz * buf_block_len);
		if (ret) {
			i++;

			PRINTM(ERROR, "host_to_card, write iomem (%d) failed: %d\n", i,
			       ret);
			sdio_writeb(card->func, 0x04, CONFIGURATION_REG, &ret);
			if (ret) {
				PRINTM(ERROR, "write ioreg failed (FN1 CFG)\n");
			}
			ret = WLAN_STATUS_FAILURE;
			if (i > MAX_WRITE_IOMEM_RETRY) {
				priv->wlan_dev.dnld_sent = DNLD_RES_RECEIVED;
				goto exit;
			}
		}
		else {
			DBG_HEXDUMP(IF_D, "SDIO Blk Wr", priv->adapter->TmpTxBuf,
				    blksz * buf_block_len);
		}
	} while (ret == WLAN_STATUS_FAILURE);

exit:
	sdio_set_block_size(card->func, 0);
	sdio_release_host(card->func);
	LEAVE();
	return ret;
}

/** 
 *  @brief This function reads CIS informaion.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_get_cis_info(wlan_private * priv)
{
	wlan_adapter *Adapter = priv->adapter;
	u8 tupledata[255];
	ENTER();

	/* TODO using sdio tuple data */

	/* Copy the CIS Table to Adapter */
	memset(Adapter->CisInfoBuf, 0x0, sizeof(Adapter->CisInfoBuf));
	memcpy(Adapter->CisInfoBuf, tupledata, sizeof(tupledata));
	Adapter->CisInfoLen = sizeof(tupledata);

	LEAVE();
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function probes the card.
 *  
 *  @param card_p  A pointer to the card
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_probe_card(void *card_p)
{
	struct if_sdio_card* card = (struct if_sdio_card*)card_p;
	int ret = WLAN_STATUS_SUCCESS;

	if (!card) {
		ret = -ENODEV;		//WLAN_STATUS_FAILURE;
		goto done;
	}

	/* Check for MANFID */
	PRINTM(INFO, "Marvell SDIO card detected!\n");

	sdio_claim_host(card->func);
	/* read Revision Register to get the hw revision number */
	card->chiprev = sdio_readb(card->func, CARD_REVISION_REG, &ret);
	if (ret) {
		PRINTM(FATAL, "cannot read CARD_REVISION_REG\n");
	}
	else {
		PRINTM(INFO, "revision=0x%x\n", card->chiprev);
		switch (card->chiprev) {
		default:
			card->block_size_512 = TRUE;
			card->async_int_mode = TRUE;
			break;
		}
	}

	ret = WLAN_STATUS_SUCCESS;
done:
	sdio_release_host(card->func);
	return ret;
}

/** 
 *  @brief This function calls sbi_download_wlan_fw_image to download
 *  firmware image to the card.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_prog_firmware_w_helper(wlan_private * priv)
{
	wlan_adapter *Adapter = priv->adapter;
	if (Adapter->fmimage != NULL) {
		return sbi_download_wlan_fw_image(priv,
						  Adapter->fmimage,
						  Adapter->fmimage_len);
	}
	else {
		PRINTM(MSG, "No external FW image\n");
		return WLAN_STATUS_FAILURE;
	}
}

/** 
 *  @brief This function programs helper image.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_prog_helper(wlan_private * priv)
{
	wlan_adapter *Adapter = priv->adapter;
	if (Adapter->helper != NULL) {
		return sbi_prog_firmware_image(priv,
					       Adapter->helper, Adapter->helper_len);
	}
	else {
		PRINTM(MSG, "No external helper image\n");
		return WLAN_STATUS_FAILURE;
	}
}

/** 
 *  @brief This function checks if the firmware is ready to accept
 *  command or not.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_verify_fw_download(wlan_private * priv)
{
	int ret = WLAN_STATUS_SUCCESS;
	u16 firmwarestat;
	int tries;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	sdio_claim_host(card->func);
	/* Wait for firmware initialization event */
	for (tries = 0; tries < MAX_FIRMWARE_POLL_TRIES; tries++) {
		if ((ret = mv_sdio_read_scratch(priv, &firmwarestat)) < 0)
			continue;

		if (firmwarestat == FIRMWARE_READY) {
			ret = WLAN_STATUS_SUCCESS;
			break;
		}
		else {
			mdelay(10);
			ret = WLAN_STATUS_FAILURE;
		}
	}

	if (ret < 0) {
		PRINTM(MSG, "Timeout waiting for FW to become active\n");
		goto done;
	}

	ret = WLAN_STATUS_SUCCESS;
done:
	sdio_release_host(card->func);
	return ret;
}

/** 
 *  @brief This function set bus clock on/off
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param option    TRUE--on , FALSE--off
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_set_bus_clock(wlan_private * priv, u8 option)
{
/*	if (option == TRUE)
		start_bus_clock(((mmc_card_t) ((priv->wlan_dev).card))->ctrlr);
	else
		stop_bus_clock_2(((mmc_card_t) ((priv->wlan_dev).card))->ctrlr); */
	return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function makes firmware exiting from deep sleep.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_exit_deep_sleep(wlan_private * priv)
{
	int ret = WLAN_STATUS_SUCCESS;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	sbi_set_bus_clock(priv, TRUE);

	sdio_claim_host(card->func);
	sdio_writeb(card->func, HOST_POWER_UP, CONFIGURATION_REG, &ret);
	sdio_release_host(card->func);

	return ret;
}

/** 
 *  @brief This function resets the setting of deep sleep.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_reset_deepsleep_wakeup(wlan_private * priv)
{

	int ret = WLAN_STATUS_SUCCESS;
	struct if_sdio_card* card = (struct if_sdio_card*)priv->wlan_dev.card;

	ENTER();

	sdio_claim_host(card->func);
	sdio_writeb(card->func, 0, CONFIGURATION_REG, &ret);
	sdio_release_host(card->func);

	LEAVE();

	return ret;
}

static const struct sdio_device_id if_sdio_ids[] = {
	{ SDIO_DEVICE(SDIO_VENDOR_ID_MARVELL, SDIO_DEVICE_ID_MARVELL_LIBERTAS)},
	{ /* end: all zeroes */},
};

MODULE_DEVICE_TABLE(sdio, if_sdio_ids);
struct if_sdio_model
{
	int model;
	const char *helper;
	const char *firmware;
};

static struct if_sdio_model if_sdio_models[] = {
	{
		/* 8686 */
		.model = 0x0B,
		.helper = "/lib/firmware/sd8686_helper.bin",
		.firmware = "/lib/firmware/sd8686.bin",
	},
};

static int if_sdio_probe(struct sdio_func *func,
			 const struct sdio_device_id *id)
{
	struct if_sdio_card *card;
	int ret, i;
	unsigned int model;

	card = kzalloc(sizeof(struct if_sdio_card), GFP_KERNEL);
	
	for (i = 0;i < func->card->num_info;i++) {
		if (sscanf(func->card->info[i],
			   "802.11 SDIO ID: %x", &model) == 1)
			break;
		if (sscanf(func->card->info[i],
			   "ID: %x", &model) == 1)
			break;
		if (!strcmp(func->card->info[i], "IBIS Wireless SDIO Card")) {
			model = 4;
			break;
		}
	}

	if (i == func->card->num_info) {
		printk("unable to identify card model\n");
		return -ENODEV;
	}
	
	card->func = func;
	card->model = model;

	for (i = 0;i < ARRAY_SIZE(if_sdio_models);i++) {
		if (card->model == if_sdio_models[i].model)
			break;
	}

	if (i == ARRAY_SIZE(if_sdio_models)) {
		printk("unknown card model 0x%x\n", card->model);
		ret = -ENODEV;
		goto free;
	}

	helper_name = if_sdio_models[i].helper;
	fw_name = if_sdio_models[i].firmware;

	sdio_claim_host(func);

	ret = sdio_enable_func(func);
	if (ret)
		goto release;

	sdio_writeb(func, 0x00, HOST_INT_MASK_REG, &ret);
	if (ret) {
		PRINTM(WARN, "Unable to diable the host interrupt!\n");
		goto reclaim;
	}

	ret = sdio_claim_irq(func, sbi_interrupt);
	if (ret)
		goto disable;

	sdio_release_host(func);

	sdio_set_drvdata(func, card);

	ret = sbi_add_card(card);
	if (ret)
		goto reclaim;

out:
	return ret;

reclaim:
	sdio_claim_host(func);
	sdio_release_irq(func);
disable:
	sdio_disable_func(func);
release:
	sdio_release_host(func);
free:
	kfree(card);
	goto out;
}

static void if_sdio_remove(struct sdio_func *func)
{
	struct if_sdio_card *card = sdio_get_drvdata(func);
	sbi_remove_card(card);
	sdio_claim_host(func);
	sdio_release_irq(func);
	sdio_disable_func(func);
	sdio_release_host(func);
	kfree(card);
	return;
}

static struct sdio_driver if_sdio_driver = {
	.name           = "sd8686_sdio",
	.id_table       = if_sdio_ids,
	.probe          = if_sdio_probe,
	.remove         = if_sdio_remove,
};

static int if_sdio_init_module(void)
{
	int ret = 0;

	printk(KERN_INFO "8686 sdio: sd 8686 driver\n");
	printk(KERN_INFO "8686 sdio: Copyright HHCN 2009\n");

	ret = sdio_register_driver(&if_sdio_driver);

	return ret;
}

static void if_sdio_exit_module(void)
{
	sdio_unregister_driver(&if_sdio_driver);
}

module_init(if_sdio_init_module);
module_exit(if_sdio_exit_module);

MODULE_DESCRIPTION("Marvell SD8686 SDIO WLAN Driver");
MODULE_AUTHOR("You Sheng");
MODULE_LICENSE("GPL");
