/*
*    spi-dev.c - spi-bus driver, char device interface
*
*	Copyright (C) 2006 Samsung Electronics Co. Ltd.
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <asm/dma.h>
#include <mach/hardware.h>
#include <mach/s3c-dma.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include "spi-dev.h"
#include <asm/uaccess.h>

#undef debug
#ifdef debug
#define CRDEBUG	printk("%s :: %d\n",__FUNCTION__,__LINE__)
#else
#define CRDEBUG
#endif

static struct spi_dev *spi_dev_array[SPI_MINORS];
static spinlock_t (spi_dev_array_lock) = SPIN_LOCK_UNLOCKED;

static struct spi_dev *attach_to_spi_dev_array(struct spi_dev *spi_dev)
{
	CRDEBUG;
	spin_lock(&spi_dev_array_lock);
	if (spi_dev_array[spi_dev->minor]) {
		spin_unlock(&spi_dev_array_lock);
		dev_err(&spi_dev->dev, "spi-dev already has a device assigned to this adapter\n");
		goto error;
	}
	spi_dev_array[spi_dev->minor] = spi_dev;
	spin_unlock(&spi_dev_array_lock);
	return spi_dev;
error:
	return ERR_PTR(-ENODEV);
}

static void return_spi_dev(struct spi_dev *spi_dev)
{
	CRDEBUG;
	spin_lock(&spi_dev_array_lock);
	spi_dev_array[spi_dev->minor] = NULL;
	spin_unlock(&spi_dev_array_lock);
}

int spi_attach_spidev(struct spi_dev *spidev)
{
	struct spi_dev *spi_dev;

	CRDEBUG;
	spi_dev = attach_to_spi_dev_array(spidev);
	if (IS_ERR(spi_dev))
		return PTR_ERR(spi_dev);

	dev_dbg(&spi_dev->dev, "Registered as minor %d\n", spi_dev->minor);

	return 0;
}

int spi_detach_spidev(struct spi_dev *spi_dev)
{
	CRDEBUG;
	return_spi_dev(spi_dev);

	dev_dbg(&spi_dev->dev, "Adapter unregistered\n");
	return 0;
}

struct spi_dev *spi_dev_get_by_minor(unsigned index)
{
	struct spi_dev *spi_dev;

	CRDEBUG;
	spin_lock(&spi_dev_array_lock);
	spi_dev = spi_dev_array[index];
	spin_unlock(&spi_dev_array_lock);
	return spi_dev;
}

int spi_master_recv(struct spi_dev *spi_dev, char *rbuf ,int count)
{
	struct spi_msg msg;
	int ret;

	CRDEBUG;
	if (spi_dev->algo->master_xfer) {
		msg.flags = spi_dev->flags;
		msg.len = count;
		msg.wbuf = NULL;
		msg.rbuf = rbuf;

		dev_dbg(&spi_dev->dev, "master_recv: reading %d bytes.\n",
			count);

		down(&spi_dev->bus_lock);
		ret = spi_dev->algo->master_xfer(spi_dev, &msg, 1);
		up(&spi_dev->bus_lock);

		dev_dbg(&spi_dev->dev, "master_recv: return:%d (count:%d)\n",
			ret, count);

		/* if everything went ok (i.e. 1 msg transmitted), return #bytes
	 	* transmitted, else error code.
	 	*/
		return (ret == 1 )? count : ret;
	} else {
		dev_err(&spi_dev->dev, "SPI level transfers not supported\n");
		return -ENOSYS;
	}
}

int spi_master_send(struct spi_dev *spi_dev, const char *wbuf, int count)
{
	int ret;
	struct spi_msg msg;

	CRDEBUG;
	if (spi_dev->algo->master_xfer) {
		msg.flags = spi_dev->flags;
		msg.len = count;
		msg.wbuf = (char *)wbuf;
		msg.rbuf = NULL;

		dev_dbg(&spi_dev->dev, "master_send: writing %d bytes.\n",
			count);
		down(&spi_dev->bus_lock);
		ret = spi_dev->algo->master_xfer(spi_dev, &msg, 1);
		up(&spi_dev->bus_lock);

		/* if everything went ok (i.e. 1 msg transmitted), return #bytes
		 * transmitted, else error code.
		 */
		return (ret == 1 )? count : ret;
	} else {
		dev_err(&spi_dev->dev, "SPI level transfers not supported\n");
		return -ENOSYS;
	}
}

static ssize_t spidev_read (struct file *file, char __user *buf, size_t count,
                            loff_t *offset)
{
	char *tmp;
	int ret;
	struct spi_dev *spi_dev = (struct spi_dev *)file->private_data;
#ifdef CONFIG_WORD_TRANSIZE
	count = count * 4;
#endif
	CRDEBUG;
	if (count > BUFFER_SIZE)
		count = BUFFER_SIZE;

	if(spi_dev->flags & SPI_M_DMA_MODE){
		tmp = dma_alloc_coherent(NULL, BUFFER_SIZE,
			  &spi_dev->dmabuf, GFP_KERNEL | GFP_DMA);
	}else{
		tmp = kmalloc(count,GFP_KERNEL);
	}
	if (tmp==NULL)
		return -ENOMEM;

	pr_debug("%s: tmp=0x%x  dmabuf=0x%x\n",
		__FUNCTION__,*tmp,spi_dev->dmabuf);
	pr_debug("spi-dev: spi-%d reading %zd bytes.\n",
		iminor(file->f_dentry->d_inode), count);

	ret = spi_master_recv(spi_dev,tmp,count);
	if (ret >= 0)
		ret = copy_to_user(buf,tmp,count)?-EFAULT:ret;
	if(spi_dev->flags & SPI_M_DMA_MODE){
		dma_free_coherent(NULL,BUFFER_SIZE,tmp,spi_dev->dmabuf);
	}else{
		kfree(tmp);
	}
	return ret;
}

static ssize_t spidev_write (struct file *file, const char __user *buf, size_t count,
                             loff_t *offset)
{
	int ret;
	char *tmp;
	struct spi_dev *spi_dev = (struct spi_dev *)file->private_data;
#ifdef CONFIG_WORD_TRANSIZE
	count = count * 4;
#endif
	CRDEBUG;
	if (count > BUFFER_SIZE)
		count = BUFFER_SIZE;
	if(spi_dev->flags & SPI_M_DMA_MODE){
		tmp = dma_alloc_coherent(NULL, BUFFER_SIZE,
			  &spi_dev->dmabuf, GFP_KERNEL | GFP_DMA);
	}else{
		tmp = kmalloc(count,GFP_KERNEL);
	}

	if (tmp==NULL)
		return -ENOMEM;
	pr_debug("%s: tmp=0x%x  dmabuf=0x%x\n",
		__FUNCTION__,*tmp,spi_dev->dmabuf);

	if (copy_from_user(tmp, buf, count)) {
		if(spi_dev->flags & SPI_M_DMA_MODE){
			dma_free_coherent(NULL,BUFFER_SIZE,tmp,spi_dev->dmabuf);
		}else{
			kfree(tmp);
		}
		return -EFAULT;
	}

	pr_debug("spi-dev: spi-%d writing %zd bytes.\n",
		iminor(file->f_dentry->d_inode), count);

	ret = spi_master_send(spi_dev, tmp, count);
	if(spi_dev->flags & SPI_M_DMA_MODE){
		dma_free_coherent(NULL,BUFFER_SIZE,tmp,spi_dev->dmabuf);
	}else{
		kfree(tmp);
	}
	return ret;
}

int spidev_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
                  unsigned long arg)
{
	struct spi_dev *spi_dev = (struct spi_dev *)file->private_data;

	CRDEBUG;
	dev_dbg(&spi_dev->dev, "spi-%d ioctl, cmd: 0x%x, arg: %lx.\n",
		iminor(inode),cmd, arg);

	switch ( cmd ) {
		case SET_SPI_FLAGS:
			spi_dev->flags = (unsigned int) arg;
			break;
		case SET_SPI_RETRIES:
			spi_dev->retries = arg;
			break;
		case SET_SPI_TIMEOUT:
			spi_dev->timeout = arg;
			break;
		default:
			printk("Invalid ioctl option\n");
	}
	return 0;
}

static int spidev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct spi_dev *spi_dev;

	CRDEBUG;

	spi_dev = spi_dev_get_by_minor(minor);
	if (!spi_dev)
		return -ENODEV;

	/* registered with adapter, passed as client to user */
	file->private_data = spi_dev;

	return 0;
}

int spi_master_close(struct spi_dev *spi_dev)
{
	int ret;
	CRDEBUG;
	ret = spi_dev->algo->close(spi_dev);
	return 0;
}
static int spidev_release(struct inode *inode, struct file *file)
{
	struct spi_dev *spi_dev = (struct spi_dev *)file->private_data;
	int ret;
	CRDEBUG;

	ret = spi_master_close(spi_dev);
	file->private_data = NULL;

	return 0;
}

static struct file_operations spidev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.read		= spidev_read,
	.write		= spidev_write,
	.ioctl		= spidev_ioctl,
	.open		= spidev_open,
	.release	= spidev_release,
};

static int spi_dev_init(void)
{
	int res;

	printk(KERN_INFO "spi /dev entries driver\n");

#if(SPI_CHANNEL==0)
	res = register_chrdev(SPI_MAJOR, "spi0", &spidev_fops);
#elif(SPI_CHANNEL==1)
	res = register_chrdev(SPI_MAJOR, "spi1", &spidev_fops);
#else
	res = register_chrdev(SPI_MAJOR, "spi2", &spidev_fops);
#endif

	if (res)
		goto out;

	return 0;

out:
	printk(KERN_ERR "%s: Driver Initialisation failed\n", __FILE__);
	return res;
}

static void spi_dev_exit(void)
{
	CRDEBUG;
	unregister_chrdev(SPI_MAJOR,"spi");
}

MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("spi /dev entries driver");
MODULE_LICENSE("GPL");

module_init(spi_dev_init);
module_exit(spi_dev_exit);

EXPORT_SYMBOL(spi_attach_spidev);
EXPORT_SYMBOL(spi_detach_spidev);
