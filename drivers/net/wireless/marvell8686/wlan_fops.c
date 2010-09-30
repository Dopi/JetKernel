/** @file wlan_fops.c
  * @brief This file contains the file read functions
  * 
  *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
  */
/********************************************************
Change log:
	01/06/06: Add Doxygen format comments

********************************************************/

#include	"include.h"

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/fcntl.h>
#include <linux/vmalloc.h>

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
 *  @brief This function opens/create a file in kernel mode.
 *  
 *  @param filename	Name of the file to be opened
 *  @param flags		File flags 
 *  @param mode		File permissions
 *  @return 		file pointer if successful or NULL if failed.
 */
static struct file *
wlan_fopen(const char *filename, unsigned int flags, int mode)
{
    int orgfsuid, orgfsgid;
    struct file *file_ret;
#if 0
    /* Save uid and gid used for filesystem access.  */

    orgfsuid = current->fsuid;
    orgfsgid = current->fsgid;

    /* Set user and group to 0 (root) */
    current->fsuid = 0;
    current->fsgid = 0;
#endif
    /* Open the file in kernel mode */
    file_ret = filp_open(filename, flags, mode);
#if 0
    /* Restore the uid and gid */
    current->fsuid = orgfsuid;
    current->fsgid = orgfsgid;
#endif
    /* Check if the file was opened successfully
       and return the file pointer of it was.  */
    return ((IS_ERR(file_ret)) ? NULL : file_ret);
}

/** 
 *  @brief This function closes a file in kernel mode.
 *  
 *  @param file_ptr     File pointer 
 *  @return 		WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_fclose(struct file *file_ptr)
{
    int orgfsuid, orgfsgid;
    int file_ret;

    if ((NULL == file_ptr) || (IS_ERR(file_ptr)))
        return -ENOENT;
#if 0
    /* Save uid and gid used for filesystem access.  */
    orgfsuid = current->fsuid;
    orgfsgid = current->fsgid;

    /* Set user and group to 0 (root) */
    current->fsuid = 0;
    current->fsgid = 0;
#endif
    /* Close the file in kernel mode (user_id = 0) */
    file_ret = filp_close(file_ptr, 0);
#if 0
    /* Restore the uid and gid */
    current->fsuid = orgfsuid;
    current->fsgid = orgfsgid;
#endif
    return (file_ret);
}

/** 
 *  @brief This function reads data from files in kernel mode.
 *  
 *  @param file_ptr     File pointer
 *  @param buf		Buffers to read data into
 *  @param len		Length of buffer
 *  @return 		number of characters read	
 */
static int
wlan_fread(struct file *file_ptr, char *buf, int len)
{
    int orgfsuid, orgfsgid;
    int file_ret;
    mm_segment_t orgfs;

    /* Check if the file pointer is valid */
    if ((NULL == file_ptr) || (IS_ERR(file_ptr)))
        return -ENOENT;

    /* Check for a valid file read function */
    if (file_ptr->f_op->read == NULL)
        return -ENOSYS;

    /* Check for access permissions */
    if (((file_ptr->f_flags & O_ACCMODE) & (O_RDONLY | O_RDWR)) == 0)
        return -EACCES;

    /* Check if there is a valid length */
    if (0 >= len)
        return -EINVAL;
#if 0
    /* Save uid and gid used for filesystem access.  */
    orgfsuid = current->fsuid;
    orgfsgid = current->fsgid;

    /* Set user and group to 0 (root) */
    current->fsuid = 0;
    current->fsgid = 0;
#endif
    /* Save FS register and set FS register to kernel
       space, needed for read and write to accept
       buffer in kernel space.  */
    orgfs = get_fs();

    /* Set the FS register to KERNEL mode.  */
    set_fs(KERNEL_DS);

    /* Read the actual data from the file */
    file_ret = file_ptr->f_op->read(file_ptr, buf, len, &file_ptr->f_pos);

    /* Restore the FS register */
    set_fs(orgfs);
#if 0
    /* Restore the uid and gid */
    current->fsuid = orgfsuid;
    current->fsgid = orgfsgid;
#endif
    return (file_ret);
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief This function free FW/Helper buffer.
 *  
 *  @param addr		Pointer to buffer storing FW/Helper
 *  @return 		None	
 */
void
fw_buffer_free(u8 * addr)
{
    vfree(addr);
}

/** 
 *  @brief This function reads FW/Helper.
 *  
 *  @param name		File name
 *  @param addr		Pointer to buffer storing FW/Helper
 *  @param len     	Pointer to length of FW/Helper
 *  @return 		WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
fw_read(const char *name, u8 ** addr, u32 * len)
{
    struct file *fp;
    int ret;
    u8 *ptr;

    fp = wlan_fopen(name, O_RDWR, 0);

    if (fp == NULL) {
        PRINTM(MSG, "Could not open file:%s\n", name);
        return WLAN_STATUS_FAILURE;
    }

    /*calculate file length */
    *len = fp->f_dentry->d_inode->i_size - fp->f_pos;

    ptr = (u8 *) vmalloc(*len + 1023);
    if (ptr == NULL) {
        PRINTM(MSG, "vmalloc failure\n");
        return WLAN_STATUS_FAILURE;
    }
    if (wlan_fread(fp, ptr, *len) > 0) {
        *addr = ptr;
        ret = WLAN_STATUS_SUCCESS;
    } else {
        fw_buffer_free(ptr);
        *addr = NULL;
        PRINTM(MSG, "fail to read the file %s \n", name);
        ret = WLAN_STATUS_FAILURE;
    }

    wlan_fclose(fp);
    return ret;
}
