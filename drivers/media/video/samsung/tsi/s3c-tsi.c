/* linux/drivers/media/video/samsung/s3c-tsi.c
 *
 * Driver file for Samsung Transport Stream Interface
 *
 *  Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <asm/io.h>
#include <asm/page.h>
#include <mach/irqs.h>
#include <mach/gpio.h>
#include <plat/map.h>
#include <plat/regs-clock.h>
#include <plat/regs-tsi.h>
#include <plat/gpio-cfg.h>

#define TSI_BUF_SIZE	(256*1024)

enum filter_mode 
{
 OFF,
 ON	  	
};

enum pid_filter_mode
{
 	BYPASS=0,
	 FILTERING
};

enum data_byte_order
{
	MSB2LSB=0,
	LSB2MSB

};
typedef struct
{
  struct list_head list;
  dma_addr_t addr;
  void *buf;
  u32 len;
}tsi_pkt; 


typedef struct 
{
	enum filter_mode flt_mode;
	enum pid_filter_mode pid_flt_mode;
	enum data_byte_order byte_order;
	u16  burst_len;
	u8  sync_detect;
	u8  byte_swap;			
	u16 pad_pattern;
	u16 num_packet;
}s3c_tsi_conf;


typedef struct  
{
	spinlock_t tsi_lock;
	struct clk *tsi_clk;
	struct resource *tsi_mem;
	//struct resource *tsi_irq;
	void __iomem 	*tsi_base;
	int tsi_irq;
	int running;
	int new_pkt;
	dma_addr_t 	tsi_buf_phy;
	void *		tsi_buf_virt;
	u32		tsi_buf_size;
	s3c_tsi_conf *tsi_conf;
	struct list_head free_list;
	struct list_head full_list;
	struct list_head partial_list;
	wait_queue_head_t read_wq;
}tsi_dev;
tsi_dev *tsi_priv;

static struct platform_device *s3c_tsi_dev;


//#define DRIVER_LOGIC_CHK
#ifdef DRIVER_LOGIC_CHK
static struct timer_list tsi_timer;
#endif //driver_logic_chk


/* debug macro */
#define TSI_DEBUG(fmt, ...)                                     \
        do {                                                    \
                printk(                               \
                        "%s: " fmt, __func__, ##__VA_ARGS__);   \
        } while(0)


#define TSI_WARN(fmt, ...)                                      \
        do {                                                    \
                printk(KERN_WARNING                             \
                        fmt, ##__VA_ARGS__);                    \
        } while (0)

#define TSI_ERROR(fmt, ...)                                     \
        do {                                                    \
                printk(KERN_ERR                                 \
                        "%s: " fmt, __func__, ##__VA_ARGS__);   \
        } while (0)



#ifdef CONFIG_VIDEO_TSI_DEBUG
#define tsi_dbg(fmt, ...)               TSI_DEBUG(fmt, ##__VA_ARGS__)
#else
#define tsi_dbg(fmt, ...)
#endif
        
#define tsi_warn(fmt, ...)              TSI_WARN(fmt, ##__VA_ARGS__)
#define tsi_err(fmt, ...)               TSI_ERROR(fmt, ##__VA_ARGS__)

#define tsi_list_dbg(fmt,...)		TSI_DEBUG(fmt,##__VA_ARGS__)


#ifdef CONFIG_TSI_LIST_DEBUG
void list_debug(struct list_head *head)
{
	int i;
	tsi_pkt *pkt; 
       //tsi_list_dbg("DEBUGGING FREE LIST\n");
        i=1;

        list_for_each_entry(pkt,head,list){

                tsi_list_dbg(" node %d node_addr %x physical add %p virt add %p size %d\n ",i,pkt,pkt->addr,pkt->buf,pkt->len);
                i++;

                }



}

#endif //LIST_DEBUG

/*This should be done in platform*/
void s3c_tsi_set_gpio(void)
{
	int i;

	for(i=2;i< 7; i++)
	{
		s3c_gpio_cfgpin(S5PC11X_GPJ0(i), S3C_GPIO_SFN(5));
		s3c_gpio_setpull(S5PC11X_GPJ0(i), S3C_GPIO_PULL_NONE);
	}
}


void s3c_tsi_reset(tsi_dev *tsi)
{
	u32 tscon;
	tscon = readl((tsi->tsi_base + S3C_TS_CON));	
	tscon |= S3C_TSI_SWRESET  ;
	writel(tscon,(tsi->tsi_base + S3C_TS_CON)); 
}

void s3c_tsi_set_timeout(u32 count,tsi_dev *tsi)
{
	writel(count,(tsi->tsi_base + S3C_TS_CNT ));
}

tsi_pkt *tsi_get_pkt(tsi_dev *tsi, struct list_head *head)
{
	unsigned long flags;
	tsi_pkt *pkt;
	spin_lock_irqsave(&tsi->tsi_lock,flags);

	if(list_empty(head)){
		tsi_err("TSI %p list is null\n",head);
	spin_unlock_irqrestore(&tsi->tsi_lock,flags);
		return NULL;	
		}
	pkt = list_first_entry(head,tsi_pkt,list);
	spin_unlock_irqrestore(&tsi->tsi_lock,flags);

	return pkt;
}

void s3c_tsi_set_dest_addr(dma_addr_t addr,u32 reg)
{
	 writel(addr,reg);
}

void s3c_tsi_set_sync_mode(u8 mode,u32 reg)
{
	writel(mode,reg);
}

void s3c_tsi_set_clock(u8 enable,u32 reg)
{
	u32 val = 0;
	if(enable)
		val |= 0x1;
	writel(val,reg);
}

void tsi_enable_interrupts(tsi_dev *tsi)
{
	u32 mask;
	// Enable all the interrupts...
	mask = 0xFF;
	 writel(mask,(tsi->tsi_base + S3C_TS_INTMASK));	

}

void tsi_disable_interrupts(tsi_dev *tsi)
{
	
	 writel(0,(tsi->tsi_base + S3C_TS_INTMASK));	
}

static int s3c_tsi_start(tsi_dev *tsi)
{
	unsigned long flags;
	u32 pkt_size;
	tsi_pkt *pkt1;
	pkt1 =	tsi_get_pkt(tsi,&tsi->free_list);
	if(pkt1 == NULL){
		tsi_err("Failed to start TSI--No buffers avaialble\n");
		return -1;
	}
	pkt_size = pkt1->len;
	writel(pkt_size,(tsi->tsi_base+S3C_TS_SIZE));
	s3c_tsi_set_dest_addr(pkt1->addr,(u32)(tsi->tsi_base+S3C_TS_BASE));
		
	spin_lock_irqsave(&tsi->tsi_lock,flags);
	list_move_tail(&pkt1->list,&tsi->partial_list);
	spin_unlock_irqrestore(&tsi->tsi_lock,flags);
	//start the clock
	s3c_tsi_set_clock(TSI_CLK_START,(u32)(tsi->tsi_base+S3C_TS_CLKCON));
	//set the next buffer immediatly
	pkt1 =	tsi_get_pkt(tsi,&tsi->free_list);
	if(pkt1 == NULL){
		tsi_err("Failed to start TSI--No buffers avaialble\n");
		return -1;
	}
	s3c_tsi_set_dest_addr(pkt1->addr,(u32)(tsi->tsi_base+S3C_TS_BASE));
	spin_lock_irqsave(&tsi->tsi_lock,flags);
	list_move_tail(&pkt1->list,&tsi->partial_list);
	spin_unlock_irqrestore(&tsi->tsi_lock,flags);
	tsi_enable_interrupts(tsi);	

#ifdef CONFIG_TSI_LIST_DEBUG1
	tsi_list_dbg("Debugging Partial list \n");
	list_debug(&tsi->partial_list);
	tsi_list_dbg("Debugging free list \n");
	list_debug(&tsi->free_list);
#endif
	return 0;
}

static int s3c_tsi_stop(tsi_dev *tsi)
{
	unsigned long flags;
	tsi_pkt *pkt;
	struct list_head *full = &tsi->full_list;
	struct list_head *partial = &tsi->partial_list;
	
	spin_lock_irqsave(&tsi->tsi_lock,flags);
	#ifdef DRIVER_LOGIC_CHK
	del_timer(&tsi_timer);
	#endif

	tsi_disable_interrupts(tsi);	
	s3c_tsi_set_clock(TSI_CLK_STOP,(u32)(tsi->tsi_base+S3C_TS_CLKCON));
	//move all the packets from partial and full list to free list
	while(!list_empty(full)){
	pkt = list_entry(full->next,tsi_pkt,list);
	list_move_tail(&pkt->list,&tsi->free_list);		

	}
	while(!list_empty(partial)){
	pkt = list_entry(partial->next,tsi_pkt,list);
	list_move_tail(&pkt->list,&tsi->free_list);		

	}
	tsi->running = 0;
	tsi_priv->new_pkt = 0;
	spin_unlock_irqrestore(&tsi->tsi_lock,flags);
	
	return 0;
}






void s3c_tsi_setup(tsi_dev *tsi)
{
	u32 tscon;
	s3c_tsi_conf *conf = tsi->tsi_conf;
	s3c_tsi_reset(tsi);
	s3c_tsi_set_timeout(TS_TIMEOUT_CNT_MAX,tsi);	
//set the tscon bits 
	tscon = readl((tsi->tsi_base+S3C_TS_CON));	

	tscon &= ~(S3C_TSI_SWRESET_MASK|S3C_TSI_CLKFILTER_MASK|S3C_TSI_BURST_LEN_MASK | S3C_TSI_INT_FIFO_FULL_INT_ENA_MASK |
		S3C_TSI_SYNC_MISMATCH_INT_MASK |  S3C_TSI_PSUF_INT_MASK|S3C_TSI_PSOF_INT_MASK |S3C_TSI_TS_CLK_TIME_OUT_INT_MASK | 
		  S3C_TSI_TS_ERROR_MASK | S3C_TSI_PID_FILTER_MASK |S3C_TSI_ERROR_ACTIVE_MASK  |S3C_TSI_DATA_BYTE_ORDER_MASK |
		S3C_TSI_TS_VALID_ACTIVE_MASK |S3C_TSI_SYNC_ACTIVE_MASK |S3C_TSI_CLK_INVERT_MASK    );

	tscon |= (conf->flt_mode << S3C_TSI_CLKFILTER_SHIFT); 	
	tscon |= (conf->pid_flt_mode << S3C_TSI_PID_FILTER_SHIFT );
	tscon |= (conf->byte_order << S3C_TSI_DATA_BYTE_ORDER_SHIFT );
	tscon |= (conf->burst_len << S3C_TSI_BURST_LEN_SHIFT);	
	tscon |= (conf->pad_pattern << S3C_TSI_PAD_PATTERN_SHIFT);

	tscon |= (S3C_TSI_OUT_BUF_FULL_INT_ENA | S3C_TSI_INT_FIFO_FULL_INT_ENA);
	tscon |= (S3C_TSI_SYNC_MISMATCH_INT_SKIP | S3C_TSI_PSUF_INT_SKIP| S3C_TSI_PSOF_INT_SKIP);
	tscon |= (S3C_TSI_TS_ERROR_SKIP_PKT_INT);	
	/*These values are bd dependent?*/
	tscon |= (S3C_TSI_TS_VALID_ACTIVE_HIGH | S3C_TSI_CLK_INVERT_HIGH | S3C_TSI_TS_VALID_ACTIVE_HIGH);	
	tsi_dbg("Tscon is %x\n",tscon);	
	writel(tscon,(tsi->tsi_base+S3C_TS_CON));
	
	s3c_tsi_set_sync_mode(conf->sync_detect,(u32)(tsi->tsi_base+S3C_TS_SYNC));	
}

void s3c_tsi_rx_int(tsi_dev *tsi)
{
	tsi_pkt *pkt;
 //deque the pcket from partial list to full list
 //incase the free list is empty, stop the tsi..

	//	if(list_empty(&tsi->partial_list))
		

	pkt = tsi_get_pkt(tsi,&tsi->partial_list);
		if(pkt == NULL){
				//this situation should not come.. stop_tsi
			tsi_err("TSI..Receive interrupt without buffer\n");
			s3c_tsi_stop(tsi); 
			return;
		}
	
	tsi_dbg("moving %p node %x phy %p virt to full list\n",pkt,pkt->addr,pkt->buf);
	
	list_move_tail(&pkt->list,&tsi->full_list);

	pkt = tsi_get_pkt(tsi,&tsi->free_list);
	if(pkt == NULL){
				//this situation should not come.. stop_tsi
			tsi_err("TSI..No more free bufs..stopping channel\n");
			s3c_tsi_stop(tsi); 
			return;
		}
	list_move_tail(&pkt->list,&tsi->partial_list);

#ifdef CONFIG_TSI_LIST_DEBUG
	tsi_list_dbg("Debugging Full list \n");
	list_debug(&tsi->full_list);
	tsi_list_dbg("Debugging Partial list \n");
	list_debug(&tsi->partial_list);
#endif
	tsi->new_pkt = 1;
	wake_up(&tsi->read_wq);
}


static irqreturn_t s3c_tsi_irq(int irq, void *dev_id)
{
	u32 intpnd;
	tsi_dev *tsi = platform_get_drvdata(( struct platform_device *)dev_id);
	intpnd = readl(tsi->tsi_base + S3C_TS_INT );
	tsi_dbg("INTPND is %x\n",intpnd);	
	writel(intpnd,(tsi->tsi_base+S3C_TS_INT));

	if(intpnd & S3C_TSI_OUT_BUF_FULL)	
		s3c_tsi_rx_int(tsi);		
	return IRQ_HANDLED;
}

static int s3c_tsi_release(struct inode *inode, struct file *file)
{
	int ret = 0;
	tsi_dev *tsi = file->private_data;
	tsi_dbg("TSI_RELEASE  \n");	
	if(tsi->running)
	{
		tsi_dbg("TSI_RELEASE stopping \n");	
		tsi->running = 0;
		ret = s3c_tsi_stop(tsi);
		tsi_dbg("TSI_RELEASE LIST cleaned \n");	
	}

	
#ifdef CONFIG_TSI_LIST_DEBUG
	tsi_list_dbg("Debugging Full list \n");
	list_debug(&tsi->full_list);
	tsi_list_dbg("Debugging Partial list \n");
	list_debug(&tsi->partial_list);
#endif

	return ret;
}

int s3c_tsi_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

static ssize_t s3c_tsi_read(struct file *file, char *buf, size_t count, loff_t *pos)
{
	unsigned long flags;
	int ret = 0;
	u32 len = 0,pkt_size = 0;
	tsi_pkt *pkt;
	tsi_dev *tsi = file->private_data;
	struct list_head *full = &tsi->full_list;

#ifdef CONFIG_TSI_LIST_DEBUG
	tsi_list_dbg("Debugging Full list \n");
	tsi_dbg("count is %d\n",count);
	list_debug(&tsi->full_list);
#endif
	while(count > 0){
//deque packet from full list	
		pkt = tsi_get_pkt(tsi,full);	
			if(pkt == NULL) {
				
				ret = wait_event_interruptible(tsi->read_wq,tsi->new_pkt);
				
				if(ret < 0)
				{
					tsi_dbg("woken up from signal..returning\n");
					return ret;
				}
			//	if(ret == 0)
					tsi_dbg("woken up proprt\n");
				pkt = tsi_get_pkt(tsi,full);    
				
			}
		pkt_size = pkt->len * 4;
		if(pkt_size > count)
			pkt_size = count;

		if (copy_to_user((buf+len), pkt->buf, pkt_size)) {
					tsi_dbg("copy user fail\n");
                                ret = -EFAULT;
                                break;
                        }

		len += pkt_size;		
 		count -= pkt_size;
		tsi_dbg("len is%d count %d pkt_size %d\n",len,count,pkt_size);
		ret = len;
		spin_lock_irqsave(&tsi->tsi_lock,flags);
		list_move(&pkt->list,&tsi->free_list);
		spin_unlock_irqrestore(&tsi->tsi_lock,flags);
		
		if(list_empty(full))
			tsi->new_pkt =0;

	}

#ifdef CONFIG_TSI_LIST_DEBUG1
	tsi_list_dbg("Debugging Free list \n");
	list_debug(&tsi->free_list);
#endif

	return ret;
}

#define TSI_TRIGGER 0xAABB
#define TSI_STOP     0xAACC	

static int s3c_tsi_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	tsi_dev *tsi = platform_get_drvdata(s3c_tsi_dev);
//currently only two ioctl for tigger and stop are provided..
	tsi_dbg("TSI cmd is %x\n",cmd);
	switch(cmd)
	{
		case TSI_TRIGGER:
			if(tsi->running)
				return -EBUSY;
			tsi->running = 1;
		ret = s3c_tsi_start(tsi);
	
	#ifdef DRIVER_LOGIC_CHK
		 tsi_timer.expires = jiffies + HZ/100;
	        add_timer(&tsi_timer);
	#endif //ifdef DRIVER_LOGIC_CHK
		break;

		case TSI_STOP:
			tsi->running = 0;	
			ret =	s3c_tsi_stop(tsi);	

		break;
		default:
		break;



	}

 	return ret;
}


static int s3c_tsi_open(struct inode *inode, struct file *file)
{
	tsi_dev *s3c_tsi = platform_get_drvdata(s3c_tsi_dev);
	file->private_data = s3c_tsi;
	return 0;
}

static struct file_operations tsi_fops = {
	owner:		THIS_MODULE,
	open:		s3c_tsi_open,
	release:	s3c_tsi_release,
	ioctl:		s3c_tsi_ioctl,
	read:		s3c_tsi_read,
	mmap:		s3c_tsi_mmap,
};


static struct miscdevice s3c_tsi_miscdev = {
	minor:		MISC_DYNAMIC_MINOR,
	name:		"s3c-tsi",
	fops:		&tsi_fops
};

static int tsi_setup_bufs(tsi_dev *dev, struct list_head *head)
{
	tsi_pkt *pkt;
	u32 tsi_virt,tsi_size,buf_size;
	u16 num_buf;
	dma_addr_t tsi_phy;
	int i;

	tsi_phy = dev->tsi_buf_phy;
	tsi_virt =(u32) dev->tsi_buf_virt;
	tsi_size = dev->tsi_buf_size;
	//num_pkt*47*4
	buf_size = dev->tsi_conf->num_packet * TS_PKT_SIZE; //TSI generates interrupt after filling this many bytes
	num_buf = (tsi_size / buf_size);

 	for(i=0;i<num_buf;i++){
		pkt = kmalloc(sizeof(tsi_pkt),GFP_KERNEL);	
		if(!pkt)
			return list_empty(head) ? -ENOMEM : 0 ;
		pkt->addr = (tsi_phy + i*4*buf_size);
		pkt->buf = (void *)(tsi_virt + i*4*buf_size);
		pkt->len = buf_size;
		list_add_tail(&pkt->list,head);	
	}

	tsi_dbg("total nodes calulated %d buf_size %d\n", num_buf,buf_size);
#ifdef CONFIG_TSI_LIST_DEBUG1
	list_debug(head);
#endif //LIST_DEBUG

return 0;	

}

#ifdef DRIVER_LOGIC_CHK
void tsi_timer_function( u32 dev)
{
	tsi_dev *tsi = (tsi_dev *)(dev);
	s3c_tsi_rx_int(tsi);
	tsi_timer.expires = jiffies + HZ/100;
        add_timer(&tsi_timer);


}

#endif



static int s3c_tsi_probe(struct platform_device *pdev)
{
	struct resource 	*res;
	static int		size;
	static int		ret;
	s3c_tsi_conf *conf;
	dma_addr_t 		map_dma;
	struct device *dev = &pdev->dev;
	
	tsi_priv = kmalloc(sizeof( tsi_dev  ),GFP_KERNEL);
	if(tsi_priv == NULL)
		{
			printk("NO Memory for tsi allocation\n");
			return -ENOMEM;
		}
	
	conf = kmalloc(sizeof(s3c_tsi_conf),GFP_KERNEL);
	if(conf == NULL)
		{


			printk("NO Memory for tsi conf allocation\n");
			kfree(tsi_priv);
			return -ENOMEM;

		}
		//Initialise the dafault conf parameters..this should be obtained from the platform data and ioctl
//move this to platform later
	conf->flt_mode    = OFF; 
	conf->pid_flt_mode = BYPASS;
	conf->byte_order = MSB2LSB; 
	conf->sync_detect = S3C_TSI_SYNC_DET_MODE_TS_SYNC8;
        conf->burst_len = 0; 
	conf->byte_swap = 1; //little endian
	conf->pad_pattern = 0;  //this might vary from bd to bd
	conf->num_packet = TS_NUM_PKT;  //this might vary from bd to bd

	tsi_priv->tsi_conf = conf;
	tsi_priv->tsi_buf_size = TSI_BUF_SIZE;


	tsi_priv->tsi_clk = clk_get(NULL,"tsi");
	if(tsi_priv->tsi_clk == NULL)
		{
			printk(KERN_ERR "Failed to get TSI clock\n");
			return -ENOENT;

		}
	clk_enable(tsi_priv->tsi_clk);
	
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res == NULL) {
		tsi_err("failed to get memory region resouce\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;
	tsi_priv->tsi_mem = request_mem_region(res->start, size, pdev->name);

	if (tsi_priv->tsi_mem  == NULL) {
		tsi_err("failed to get memory region\n");
		return -ENOENT;
	}

	ret = platform_get_irq(pdev,0);

	if (ret == 0) {
		tsi_err("failed to get irq resource\n");
		ret = -ENOENT;
		goto err_res;
	}

	tsi_priv->tsi_irq = ret;
	ret = request_irq(  tsi_priv->tsi_irq , (void*)s3c_tsi_irq, 0, pdev->name, pdev);

	if (ret != 0) {
		tsi_err("failed to install irq (%d)\n", ret);
		goto err_res;
	}

	tsi_priv->tsi_base = ioremap(tsi_priv->tsi_mem->start, size);

	if (tsi_priv->tsi_base == 0) {
		tsi_err("failed to ioremap() region\n");
		ret = -EINVAL;
		goto err_irq;
	}

	INIT_LIST_HEAD(&tsi_priv->free_list);
	INIT_LIST_HEAD(&tsi_priv->full_list);
	INIT_LIST_HEAD(&tsi_priv->partial_list);
	spin_lock_init(&tsi_priv->tsi_lock);
	init_waitqueue_head(&tsi_priv->read_wq);
	tsi_priv->new_pkt = 0;
	tsi_priv->running = 0;
//get the dma coherent mem
	tsi_priv->tsi_buf_virt = dma_alloc_coherent(dev,tsi_priv->tsi_buf_size,&map_dma,GFP_KERNEL); 
	if(tsi_priv->tsi_buf_virt == NULL){
		tsi_err("Failed to claim TSI memory\n");
		ret = -ENOMEM;
		goto err_map;
		}
	
	tsi_dbg("TSI dev dma mem phy %x virt %p\n", map_dma,tsi_priv->tsi_buf_virt);

	tsi_priv->tsi_buf_phy = map_dma;

	ret = tsi_setup_bufs(tsi_priv,&tsi_priv->free_list);
	if(ret)	{
		tsi_err("TSI failed to setup pkt list");
		goto err_clk;
	}
	platform_set_drvdata(pdev, tsi_priv);
	s3c_tsi_set_gpio();
	s3c_tsi_setup(tsi_priv);
	s3c_tsi_dev= pdev;
	ret = misc_register(&s3c_tsi_miscdev);
	if(ret){
		tsi_err("Unable to register the s3c-tsi driver\n");
		goto err_clk;
	}
#ifdef DRIVER_LOGIC_CHK
	init_timer(&tsi_timer);
        tsi_timer.function=tsi_timer_function;
        tsi_timer.data = (unsigned long) tsi_priv;
	
 	//s3c_tsi_start(tsi_priv);
 	//s3c_tsi_rx_int(tsi_priv);
#endif
	
	return 0;

err_clk:
	  clk_disable(tsi_priv->tsi_clk);

err_map:
	iounmap(tsi_priv->tsi_base);
err_irq:
	free_irq(tsi_priv->tsi_irq,pdev);
err_res:
	release_resource(tsi_priv->tsi_mem);
	kfree(tsi_priv);
	
	return ret;
}

static void tsi_free_packets(tsi_dev *tsi)
{
	tsi_pkt *pkt;
	struct list_head *head = &(tsi->free_list);

	  while (!list_empty(head)) {
                pkt = list_entry(head->next, tsi_pkt, list);
                list_del(&pkt->list);
		kfree(pkt);
        }


}



static int s3c_tsi_remove(struct platform_device *dev)
{
	
	tsi_dev *tsi = platform_get_drvdata(( struct platform_device *)dev);
	if(tsi->running)
		s3c_tsi_stop(tsi);
//free allocated memory and nodes
	tsi_free_packets(tsi);
	free_irq(tsi->tsi_irq,dev);
	dma_free_coherent(&dev->dev,tsi->tsi_buf_size,tsi->tsi_buf_virt,tsi->tsi_buf_phy);
	kfree(tsi);	
	return 0;

}



static struct platform_driver s3c_tsi_driver = {
	.probe		= s3c_tsi_probe,
	.remove		= s3c_tsi_remove,
	.shutdown	= NULL,
	.suspend	= NULL,
	.resume		= NULL,
	.driver		= {
			.owner	= THIS_MODULE,
			.name	= "s3c-tsi",
	},
};



const char banner[] __initdata =  "TSI Driver Version 1.0\n";

static int __init s3c_tsi_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_tsi_driver);
}




static void __exit s3c_tsi_exit(void)
{

	platform_driver_unregister(&s3c_tsi_driver);
}



module_init(s3c_tsi_init);
module_exit(s3c_tsi_exit);

MODULE_AUTHOR("Samsung");
MODULE_DESCRIPTION("S3C TSI Device Driver");
MODULE_LICENSE("GPL");
