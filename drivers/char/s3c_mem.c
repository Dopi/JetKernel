/*
 * drivers/char/s3c_mem.c
 *
 * Revision 1.0
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 *	    S3C MEM driver
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/errno.h> 	/* error codes */
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/dma-mapping.h>
#include <linux/version.h>
#include <linux/unistd.h>
#include <linux/mman.h>

#include <asm/div64.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/dma.h>
#include <mach/map.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <plat/dma.h>

#include "s3c_mem.h"

static int flag = 0;

static unsigned int physical_address;

int s3c_mem_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	void * virt_addr;
	struct mm_struct *mm = current->mm;
	struct s3c_mem_alloc param;

	DEBUG("s3c_mem ioctl, cmd = 0x%x\n", cmd);

	switch (cmd) {
		case S3C_MEM_ALLOC:
			mutex_lock(&mem_alloc_lock);
			if(copy_from_user(&param, (struct s3c_mem_alloc *)arg, sizeof(struct s3c_mem_alloc))){
				mutex_unlock(&mem_alloc_lock);			
				return -EFAULT;
			}
			flag = MEM_ALLOC;
			param.vir_addr = do_mmap(file, 0, param.size, PROT_READ|PROT_WRITE, MAP_SHARED, 0);
			DEBUG("param.vir_addr = %08x, %d\n", param.vir_addr, __LINE__);
			if(param.vir_addr == -EINVAL) {
				printk("S3C_MEM_ALLOC FAILED\n");
				flag = 0;
				mutex_unlock(&mem_alloc_lock);			
				return -EFAULT;
			}
			param.phy_addr = physical_address;
			DEBUG("KERNEL MALLOC : param.phy_addr = 0x%X \t size = %d \t param.vir_addr = 0x%X, %d\n", param.phy_addr, param.size, param.vir_addr, __LINE__);

			if(copy_to_user((struct s3c_mem_alloc *)arg, &param, sizeof(struct s3c_mem_alloc))){
				flag = 0;
				mutex_unlock(&mem_alloc_lock);
				return -EFAULT;		
			}
			flag = 0;
			mutex_unlock(&mem_alloc_lock);
			
			break;

		case S3C_MEM_FREE:	
			mutex_lock(&mem_free_lock);
			if(copy_from_user(&param, (struct s3c_mem_alloc *)arg, sizeof(struct s3c_mem_alloc))){
				mutex_unlock(&mem_free_lock);
				return -EFAULT;
			}

			DEBUG("KERNEL FREE : param.phy_addr = 0x%X \t size = %d \t param.vir_addr = 0x%X, %d\n", param.phy_addr, param.size, param.vir_addr, __LINE__);

			if (do_munmap(mm, param.vir_addr, param.size) < 0) {
				printk("do_munmap() failed !!\n");
				mutex_unlock(&mem_free_lock);
				return -EINVAL;
			}
			virt_addr = phys_to_virt(param.phy_addr);

			kfree(virt_addr);
			param.size = 0;
			DEBUG("do_munmap() succeed !!\n");

			if(copy_to_user((struct s3c_mem_alloc *)arg, &param, sizeof(struct s3c_mem_alloc))){
				mutex_unlock(&mem_free_lock);
				return -EFAULT;
			}
			
			mutex_unlock(&mem_free_lock);
			
			break;

		case S3C_MEM_SHARE_ALLOC:		
			mutex_lock(&mem_share_alloc_lock);
			if(copy_from_user(&param, (struct s3c_mem_alloc *)arg, sizeof(struct s3c_mem_alloc))){
				mutex_unlock(&mem_share_alloc_lock);
				return -EFAULT;
			}
			flag = MEM_ALLOC_SHARE;

			physical_address = param.phy_addr;
			DEBUG("param.phy_addr = %08x, %d\n", physical_address, __LINE__);

			param.vir_addr = do_mmap(file, 0, param.size, PROT_READ|PROT_WRITE, MAP_SHARED, 0);
			DEBUG("param.vir_addr = %08x, %d\n", param.vir_addr, __LINE__);

			if(param.vir_addr == -EINVAL) {
				printk("S3C_MEM_SHARE_ALLOC FAILED\n");
				flag = 0;
				mutex_unlock(&mem_share_alloc_lock);
				return -EFAULT;
			}

			DEBUG("MALLOC_SHARE : param.phy_addr = 0x%X \t size = %d \t param.vir_addr = 0x%X, %d\n", param.phy_addr, param.size, param.vir_addr, __LINE__);

			if(copy_to_user((struct s3c_mem_alloc *)arg, &param, sizeof(struct s3c_mem_alloc))){
				flag = 0;
				mutex_unlock(&mem_share_alloc_lock);
				return -EFAULT;		
			}

			flag = 0;
			
			mutex_unlock(&mem_share_alloc_lock);
			
			break;

		case S3C_MEM_SHARE_FREE:	
			mutex_lock(&mem_share_free_lock);
			if(copy_from_user(&param, (struct s3c_mem_alloc *)arg, sizeof(struct s3c_mem_alloc))){
				mutex_unlock(&mem_share_free_lock);
				return -EFAULT;		
			}

			DEBUG("MEM_SHARE_FREE : param.phy_addr = 0x%X \t size = %d \t param.vir_addr = 0x%X, %d\n", param.phy_addr, param.size, param.vir_addr, __LINE__);

			if (do_munmap(mm, param.vir_addr, param.size) < 0) {
				printk("do_munmap() failed - MEM_SHARE_FREE!!\n");
				mutex_unlock(&mem_share_free_lock);
				return -EINVAL;
			}

			param.vir_addr = 0;
			DEBUG("do_munmap() succeed !! - MEM_SHARE_FREE\n");

			if(copy_to_user((struct s3c_mem_alloc *)arg, &param, sizeof(struct s3c_mem_alloc))){
				mutex_unlock(&mem_share_free_lock);
				return -EFAULT;		
			}

			mutex_unlock(&mem_share_free_lock);
			
			break;
			
		default:
			DEBUG("s3c_mem_ioctl() : default !!\n");
			return -EINVAL;
	}
	
	return 0;
}

int s3c_mem_mmap(struct file* filp, struct vm_area_struct *vma)
{
	unsigned long pageFrameNo, size, phys_addr;
	void * virt_addr;

	size = vma->vm_end - vma->vm_start;

	switch (flag) { 
	case MEM_ALLOC :
		virt_addr = kmalloc(size, GFP_DMA|GFP_ATOMIC);

		if (virt_addr == NULL) {
			printk("kmalloc() failed !\n");
			return -EINVAL;
		}
		DEBUG("MMAP_KMALLOC : virt addr = 0x%08x, size = %d, %d\n", virt_addr, size, __LINE__);
		phys_addr = virt_to_phys(virt_addr);
		physical_address = (unsigned int)phys_addr;

		//DEBUG("MMAP_KMALLOC : phys addr = 0x%p\n", phys_addr);
		pageFrameNo = __phys_to_pfn(phys_addr);
		//DEBUG("MMAP_KMALLOC : PFN = 0x%x\n", pageFrameNo);
		break;

	case MEM_ALLOC_SHARE :
		DEBUG("MMAP_KMALLOC_SHARE : phys addr = 0x%08x, %d\n", physical_address, __LINE__);
		
		// page frame number of the address for the physical_address to be shared.
		pageFrameNo = __phys_to_pfn(physical_address);
		//DEBUG("MMAP_KMALLOC_SHARE: PFN = 0x%x\n", pageFrameNo);
		DEBUG("MMAP_KMALLOC_SHARE : vma->end = 0x%08x, vma->start = 0x%08x, size = %d, %d\n", vma->vm_end, vma->vm_start, size, __LINE__);
		break;

	default :
		break;
	}
	
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;

	if (remap_pfn_range(vma, vma->vm_start, pageFrameNo, size, vma->vm_page_prot)) {
		printk("s3c_mem_mmap() : remap_pfn_range() failed !\n");
		return -EINVAL;
	}

	return 0;
}

EXPORT_SYMBOL(s3c_mem_ioctl);
EXPORT_SYMBOL(s3c_mem_mmap);
