/*
 * drivers/media/video/samsung/mfc40/s3c-mfc.c
 *
 * C file for Samsung MFC (Multi Function Codec - FIMV) driver
 *
 * PyoungJae Jung, Jiun Yu, Copyright (c) 2009 Samsung Electronics
 * http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <asm/io.h>
#include <asm/uaccess.h>

#include <plat/media.h>

#include "s3c_mfc_interface.h"
#include "s3c_mfc_common.h"
#include "s3c_mfc_logmsg.h"
#include "s3c_mfc_opr.h"
#include "s3c_mfc_intr.h"
#include "s3c_mfc_memory.h"
#include "s3c_mfc_buffer_manager.h"

#include <plat/regs-clock.h>
#include <mach/map.h>

int isMFCRunning = 0;

static struct resource	*s3c_mfc_mem;
void __iomem		*s3c_mfc_sfr_virt_base;
volatile unsigned char	*s3c_mfc_virt_fw_buf= NULL;
unsigned int  		s3c_mfc_int_type = 0;

static struct mutex	s3c_mfc_mutex;

dma_addr_t		s3c_mfc_phys_data_buf;
unsigned char		*s3c_mfc_virt_data_buf;

DECLARE_WAIT_QUEUE_HEAD(s3c_mfc_wait_queue);

static int s3c_mfc_open(struct inode *inode, struct file *file)
{
	s3c_mfc_inst_ctx *MfcCtx;
	int ret;

	mutex_lock(&s3c_mfc_mutex);

	mfc_debug("MFC - Power on sequence -\n");

	writel(readl(S5P_NORMAL_CFG)|(1<<1),S5P_NORMAL_CFG);
//	writel(readl(S5P_MTC_STABLE)|(0<<1),S5P_MTC_STABLE);
	writel(readl(S5P_BLK_PWR_STAT)|(1<<1),S5P_BLK_PWR_STAT);

	mfc_debug("LP Mode or Normal Mode [1]:%x\n",readl(S5P_NORMAL_CFG));
	mfc_debug("Stabilization counter Domain MFC[7:4]:%x\n",readl(S5P_MTC_STABLE));
	mfc_debug("Power Status [1]:%x\n",readl(S5P_BLK_PWR_STAT));
	
	MfcCtx = (s3c_mfc_inst_ctx *) kmalloc(sizeof(s3c_mfc_inst_ctx), GFP_KERNEL);
	if (MfcCtx == NULL) {
		mfc_err("MFCINST_MEMORY_ALLOC_FAIL\n");
		ret = -ENOMEM;
		goto out_open;
	}
	memset(MfcCtx, 0, sizeof(s3c_mfc_inst_ctx));

	s3c_mfc_init_hw();

	MfcCtx->InstNo = s3c_mfc_get_inst_no();
	if (MfcCtx->InstNo < 0) {
		kfree(MfcCtx);
		mfc_err("MFCINST_INST_NUM_EXCEEDED\n");
		ret = -EPERM;
		goto out_open;
	}

	if (s3c_mfc_set_state(MfcCtx, MFCINST_STATE_OPENED) == 0) {
		mfc_err("MFCINST_ERR_STATE_INVALID\n");
		kfree(MfcCtx);
		ret = -ENODEV;
		goto out_open;
	}

	MfcCtx->extraDPB = MFC_MAX_EXTRA_DPB;
	MfcCtx->FrameType = MFC_RET_FRAME_NOT_SET;

	file->private_data = (s3c_mfc_inst_ctx *)MfcCtx;
	ret = 0;

out_open:
	mutex_unlock(&s3c_mfc_mutex);
	return ret;
}

static int s3c_mfc_release(struct inode *inode, struct file *file)
{
	s3c_mfc_inst_ctx *MfcCtx;
	int ret;

	mfc_debug("MFC Release..\n");
	mutex_lock(&s3c_mfc_mutex);

	MfcCtx = (s3c_mfc_inst_ctx *)file->private_data;
	if (MfcCtx == NULL) {
		mfc_err("MFCINST_ERR_INVALID_PARAM\n");
		ret = -EIO;
		goto out_release;
	}

	s3c_mfc_merge_frag(MfcCtx->InstNo);
	
	s3c_mfc_return_inst_no(MfcCtx->InstNo);
	kfree(MfcCtx);

	ret = 0;

out_release:

	mfc_debug("MFC - Power off sequence -\n");

	writel(readl(S5P_NORMAL_CFG)&~(1<<1),S5P_NORMAL_CFG);
//	writel(readl(S5P_MTC_STABLE)|(0<<1),S5P_MTC_STABLE);
	writel(readl(S5P_BLK_PWR_STAT)&~(1<<1),S5P_BLK_PWR_STAT);

	mfc_debug("LP Mode or Normal Mode [1]:%x\n",readl(S5P_NORMAL_CFG));
	mfc_debug("Stabilization counter Domain MFC[7:4]:%x\n",readl(S5P_MTC_STABLE));
	mfc_debug("Power Status [1]:%x\n",readl(S5P_BLK_PWR_STAT));
	
	mutex_unlock(&s3c_mfc_mutex);
	return ret;
}

static int s3c_mfc_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int 			ret, ex_ret;
	int 			frameBufSize;
	int			frame_size;
	s3c_mfc_inst_ctx	*MfcCtx = NULL;
	s3c_mfc_common_args	InParm;
	s3c_mfc_args		local_param;
	

	mutex_lock(&s3c_mfc_mutex);

	ret = copy_from_user(&InParm, (s3c_mfc_common_args *)arg, sizeof(s3c_mfc_common_args));
	if (ret < 0) {
		mfc_err("Inparm copy error\n");
		ret = -EIO;
		InParm.ret_code = MFCINST_ERR_INVALID_PARAM;
		goto out_ioctl;
	}

	MfcCtx = (s3c_mfc_inst_ctx *)file->private_data;
	mutex_unlock(&s3c_mfc_mutex);
	
	switch (cmd) {
	case IOCTL_MFC_ENC_INIT:
		mutex_lock(&s3c_mfc_mutex);
		if (!s3c_mfc_set_state(MfcCtx, MFCINST_STATE_ENC_INITIALIZE)) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.ret_code = s3c_mfc_init_encode(MfcCtx, &(InParm.args));
		mfc_debug("InParm->ret_code : %d\n", InParm.ret_code);
		ret = InParm.ret_code;
		mutex_unlock(&s3c_mfc_mutex);
		break;

	case IOCTL_MFC_ENC_EXE:
		mutex_lock(&s3c_mfc_mutex);
		if (!s3c_mfc_set_state(MfcCtx, MFCINST_STATE_ENC_EXE)) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.ret_code = s3c_mfc_exe_encode(MfcCtx, &(InParm.args));
		mfc_debug("InParm->ret_code : %d\n", InParm.ret_code);
		ret = InParm.ret_code;
		mutex_unlock(&s3c_mfc_mutex);
		break;

	case IOCTL_MFC_DEC_INIT:
		mutex_lock(&s3c_mfc_mutex);
		mfc_debug("IOCTL_MFC_DEC_INIT\n");
		if (!s3c_mfc_set_state(MfcCtx, MFCINST_STATE_DEC_INITIALIZE)) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		memset(&local_param, 0, sizeof(local_param));

		local_param.dec_init.in_codec_type = InParm.args.dec_super_init.in_codec_type;
		local_param.dec_init.in_strm_size = InParm.args.dec_super_init.in_strm_size;		
		local_param.dec_init.in_strm_buf = InParm.args.dec_super_init.in_strm_buf;
		local_param.dec_init.in_packed_PB = InParm.args.dec_super_init.in_packed_PB;

		/* MFC decode init */
		InParm.ret_code = s3c_mfc_init_decode(MfcCtx, &local_param);
		if (InParm.ret_code < 0) {
			ret = InParm.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.args.dec_super_init.out_img_width = local_param.dec_init.out_img_width;
		InParm.args.dec_super_init.out_img_height = local_param.dec_init.out_img_height;
		InParm.args.dec_super_init.out_buf_width = local_param.dec_init.out_buf_width;
		InParm.args.dec_super_init.out_buf_height = local_param.dec_init.out_buf_height;
		InParm.args.dec_super_init.out_dpb_cnt = local_param.dec_init.out_dpb_cnt;
		if (local_param.dec_init.out_dpb_cnt <=0 ) {
			mfc_err("MFC out_dpb_cnt error\n");
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		frame_size = (local_param.dec_init.out_buf_width * 
				local_param.dec_init.out_buf_height * 3) >> 1;
		frameBufSize = local_param.dec_init.out_dpb_cnt * frame_size;
		InParm.args.dec_super_init.out_frame_buf_size = frameBufSize;
		
		memset(&local_param, 0, sizeof(local_param));
		local_param.mem_alloc.buff_size = (frameBufSize + 63) / 64 * 64;
		local_param.mem_alloc.cached_mapped_addr = InParm.args.dec_super_init.in_cached_mapped_addr;
		local_param.mem_alloc.non_cached_mapped_addr = InParm.args.dec_super_init.in_non_cached_mapped_addr;
		local_param.mem_alloc.cache_flag = InParm.args.dec_super_init.in_cache_flag;

		/* mfc yuv buffer allocation */
		InParm.ret_code = s3c_mfc_get_virt_addr(MfcCtx, &local_param);
		if (InParm.ret_code < 0) {
			ret = InParm.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.args.dec_super_init.out_u_addr = local_param.mem_alloc.out_addr;

		memset(&local_param, 0, sizeof(local_param));

		/* get physical yuv buffer address */
		local_param.get_phys_addr.u_addr = InParm.args.dec_super_init.out_u_addr;
		InParm.ret_code = s3c_mfc_get_phys_addr(MfcCtx, &local_param);
		if (InParm.ret_code < 0) {
			ret = InParm.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.args.dec_super_init.out_p_addr = local_param.get_phys_addr.p_addr;

		if (!s3c_mfc_set_state(MfcCtx, MFCINST_STATE_DEC_SEQ_START)) {
			mfc_err("MFCINST_STATE_DEC_SEQ_START\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		memset(&local_param, 0, sizeof(local_param));

		local_param.dec_seq_start.in_codec_type = InParm.args.dec_super_init.in_codec_type;
		local_param.dec_seq_start.in_frm_buf = InParm.args.dec_super_init.out_p_addr;
		local_param.dec_seq_start.in_frm_size = frameBufSize;
		local_param.dec_seq_start.in_strm_buf = InParm.args.dec_super_init.in_strm_buf;
		local_param.dec_seq_start.in_strm_size = InParm.args.dec_super_init.in_strm_size;
		
		InParm.ret_code = s3c_mfc_start_decode_seq(MfcCtx, &local_param);
		if (InParm.ret_code < 0) {
			ret = InParm.ret_code;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}
		
		mutex_unlock(&s3c_mfc_mutex);
		
		/*
		 * !!! CAUTION !!!
		 * Don't release mutex at the end of IOCTL_MFC_DEC_INIT. and, 
		 * don't lock mutex at the begining of IOCTL_MFC_DEC_SEQ_START
		 * because IOCTL_MFC_DEC_INIT and IOCTL_MFC_DEC_SEQ_START 
		 * are atomic operation. 
		 * other operation is not allowed between them.
		 */

		break;

	case IOCTL_MFC_DEC_SEQ_START:
		mfc_debug("IOCTL_MFC_DEC_SEQ_START\n");
		if (!s3c_mfc_set_state(MfcCtx, MFCINST_STATE_DEC_SEQ_START)) {
			mfc_err("MFCINST_STATE_DEC_SEQ_START\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.ret_code = s3c_mfc_start_decode_seq(MfcCtx, &(InParm.args));
		mutex_unlock(&s3c_mfc_mutex);
		break;	

	case IOCTL_MFC_DEC_EXE:
		mutex_lock(&s3c_mfc_mutex);
		mfc_debug("IOCTL_MFC_DEC_EXE\n");
		if (!s3c_mfc_set_state(MfcCtx, MFCINST_STATE_DEC_EXE)) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.ret_code = s3c_mfc_exe_decode(MfcCtx, &(InParm.args));
		ret = InParm.ret_code;
		mutex_unlock(&s3c_mfc_mutex);
		break;

	case IOCTL_MFC_GET_CONFIG:
		mutex_lock(&s3c_mfc_mutex);
		if (MfcCtx->MfcState < MFCINST_STATE_DEC_SEQ_START) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
			mutex_unlock(&s3c_mfc_mutex);
			break;
		}

		InParm.ret_code = s3c_mfc_get_config(MfcCtx, &(InParm.args));
		ret = InParm.ret_code;
		mutex_unlock(&s3c_mfc_mutex);
		break;

	case IOCTL_MFC_SET_CONFIG:
		mutex_lock(&s3c_mfc_mutex);
		InParm.ret_code = s3c_mfc_set_config(MfcCtx, &(InParm.args));
		ret = InParm.ret_code;
		mutex_unlock(&s3c_mfc_mutex);
		break;

	case IOCTL_MFC_REQ_BUF:
	
		if (MfcCtx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;

			break;
		}

		InParm.args.mem_alloc.buff_size = (InParm.args.mem_alloc.buff_size + 63) / 64 * 64;
		InParm.ret_code = s3c_mfc_get_virt_addr(MfcCtx, &(InParm.args));
		ret = InParm.ret_code;

		break;

	case IOCTL_MFC_FREE_BUF:


		if (MfcCtx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;

			break;
		}
		InParm.ret_code = s3c_mfc_release_alloc_mem(MfcCtx, &(InParm.args));
		ret = InParm.ret_code;

		break;

	case IOCTL_MFC_GET_PHYS_ADDR:

		if (MfcCtx->MfcState < MFCINST_STATE_OPENED) {
			mfc_err("MFCINST_ERR_STATE_INVALID\n");
			InParm.ret_code = MFCINST_ERR_STATE_INVALID;
			ret = -EINVAL;
	
			break;
		}

		InParm.ret_code = s3c_mfc_get_phys_addr(MfcCtx, &(InParm.args));
		ret = InParm.ret_code;

		break;

	default:
		mfc_err("Requested ioctl command is not defined. (ioctl cmd=0x%08x)\n", cmd);
		InParm.ret_code  = MFCINST_ERR_INVALID_PARAM;
		ret = -EINVAL;

	}


out_ioctl:
	ex_ret = copy_to_user((s3c_mfc_common_args *)arg, &InParm, sizeof(s3c_mfc_common_args));
	if (ex_ret < 0) {
		mfc_err("Outparm copy to user error\n");
		ret = -EIO;
	}

	mfc_debug("---------------IOCTL return--------------------------%d\n", ret);
	return ret;
}

static int s3c_mfc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	unsigned long offset	= vma->vm_pgoff << PAGE_SHIFT;
	unsigned long size = 0;
	unsigned long pageFrameNo = 0;

	mfc_debug("vma->vm_end - vma->vm_start = %d\n", offset);

	pageFrameNo = __phys_to_pfn(s3c_mfc_phys_data_buf);
	vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	size = s3c_get_media_memsize(S3C_MDEV_MFC);

	if( remap_pfn_range(vma, vma->vm_start, pageFrameNo, size, vma->vm_page_prot) ) {
		mfc_err("mfc remap error\n");
		return -EAGAIN;
	}

	return 0;

}

static struct file_operations s3c_mfc_fops = {
	.owner		= THIS_MODULE,
	.open		= s3c_mfc_open,
	.release	= s3c_mfc_release,
	.ioctl		= s3c_mfc_ioctl,
	.mmap		= s3c_mfc_mmap
};


static struct miscdevice s3c_mfc_miscdev = {
	.minor		= 252,
	.name		= "s3c-mfc",
	.fops		= &s3c_mfc_fops,
};

static irqreturn_t s3c_mfc_irq(int irq, void *dev_id)
{
	unsigned int	intReason;

	intReason = readl(s3c_mfc_sfr_virt_base + S3C_FIMV_INT_STATUS) & 0x1FF;

	if (((intReason & MFC_INTR_FRAME_DONE) == MFC_INTR_FRAME_DONE)
			||((intReason & MFC_INTR_FW_DONE) == MFC_INTR_FW_DONE)
			||((intReason & MFC_INTR_DMA_DONE) == MFC_INTR_DMA_DONE)) {
		writel(1, s3c_mfc_sfr_virt_base + S3C_FIMV_INT_DONE_CLEAR);
		s3c_mfc_int_type = intReason;
		wake_up_interruptible(&s3c_mfc_wait_queue);
		mfc_debug("Interrupt !! : %d\n", intReason);
	} else
		mfc_err("Undefined interrupt : %d\n", intReason);

	writel(1, s3c_mfc_sfr_virt_base + S3C_FIMV_INT_DONE_CLEAR);

	return IRQ_HANDLED;
}


static int s3c_mfc_probe(struct platform_device *pdev)
{
	struct resource *res;
	size_t		size;
	int 		ret;


	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		ret = -ENOENT;
		goto probe_out;
	}

	size = (res->end - res->start) + 1;
	s3c_mfc_mem = request_mem_region(res->start, size, pdev->name);
	if (s3c_mfc_mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto probe_out;
	}

	s3c_mfc_sfr_virt_base = ioremap(s3c_mfc_mem->start, s3c_mfc_mem->end - s3c_mfc_mem->start + 1);
	if (s3c_mfc_sfr_virt_base == NULL) {
		dev_err(&pdev->dev, "failed to ioremap address region\n");
		ret = -ENOENT;
		goto probe_out;
	}


	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get irq resource\n");
		ret = -ENOENT;
		goto probe_out;
	}

	ret = request_irq(res->start, s3c_mfc_irq, IRQF_DISABLED, pdev->name, pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to install irq (%d)\n", ret);
		goto probe_out;
	}

	mutex_init(&s3c_mfc_mutex);

	/*
	 * buffer memory secure
	 */
	s3c_mfc_phys_data_buf = s3c_get_media_memory(S3C_MDEV_MFC);
	s3c_mfc_virt_data_buf = ioremap_nocache(s3c_mfc_phys_data_buf, s3c_get_media_memsize(S3C_MDEV_MFC));

	/*
	 * firmware load
	 */
	s3c_mfc_virt_fw_buf = kmalloc(MFC_FW_BUF_SIZE, GFP_DMA);
	if (s3c_mfc_virt_fw_buf == NULL) {
		mfc_err("firmware buffer allocation was failed\n");
		ret = -ENOENT;
		goto probe_out;
	}

	if (s3c_mfc_load_firmware() == FALSE){
		mfc_err("MFCINST_ERR_FW_INIT_FAIL\n");
		ret = -EPERM;
		goto probe_out;
	}

	s3c_mfc_init_inst_no();

	s3c_mfc_init_buffer_manager();

	ret = misc_register(&s3c_mfc_miscdev);

	mfc_debug("MFC - Power off sequence -\n");
	writel(readl(S5P_NORMAL_CFG)&~(1<<1),S5P_NORMAL_CFG);
//	writel(readl(S5P_MTC_STABLE)|(0<<1),S5P_MTC_STABLE);
	writel(readl(S5P_BLK_PWR_STAT)&~(1<<1),S5P_BLK_PWR_STAT);

	mfc_debug("LP Mode or Normal Mode [1]:%x\n",readl(S5P_NORMAL_CFG));
	mfc_debug("Stabilization counter Domain MFC[7:4]:%x\n",readl(S5P_MTC_STABLE));
	mfc_debug("Power Status [1]:%x\n",readl(S5P_BLK_PWR_STAT));

	return 0;

probe_out:
	dev_err(&pdev->dev, "not found (%d). \n", ret);
	return ret;
}

static int s3c_mfc_remove(struct platform_device *pdev)
{
	printk("Entered : %s\n",__FUNCTION__);

	kfree((void *)s3c_mfc_virt_fw_buf);

	iounmap(s3c_mfc_sfr_virt_base);
	iounmap(s3c_mfc_virt_data_buf);

	/* remove memory region */
	if (s3c_mfc_mem != NULL) {
		release_resource(s3c_mfc_mem);
		kfree(s3c_mfc_mem);
		s3c_mfc_mem = NULL;
	}

	free_irq(IRQ_MFC, pdev);

	mutex_destroy(&s3c_mfc_mutex);

	misc_deregister(&s3c_mfc_miscdev);

	return 0;
}

static int s3c_mfc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int s3c_mfc_resume(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver s3c_mfc_driver = {
	.probe		= s3c_mfc_probe,
	.remove		= s3c_mfc_remove,
	.shutdown	= NULL,
	.suspend	= s3c_mfc_suspend,
	.resume		= s3c_mfc_resume,

	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-mfc",
	},
};

static char banner[] __initdata = KERN_INFO "S5PC100 MFC Driver, (c) 2009 Samsung Electronics\n";

static int __init s3c_mfc_init(void)
{
	printk(banner);
	if (platform_driver_register(&s3c_mfc_driver) != 0) {
		printk(KERN_ERR "platform device registration failed.. \n");
		return -1;
	}

	return 0;
}

static void __exit s3c_mfc_exit(void)
{
	platform_driver_unregister( &s3c_mfc_driver);
	printk("S5PC100 MFC Driver exit.\n");
}

module_init( s3c_mfc_init );
module_exit( s3c_mfc_exit );

MODULE_AUTHOR("Jiun, Yu");
MODULE_AUTHOR("PyoungJae, Jung");
MODULE_DESCRIPTION("S3C MFC (Multi Function Codec - FIMV) Device Driver");
MODULE_LICENSE("GPL");

