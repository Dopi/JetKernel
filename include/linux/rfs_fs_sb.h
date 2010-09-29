/*
 *---------------------------------------------------------------------------*
 *                                                                           *
 *          COPYRIGHT 2003-2009 SAMSUNG ELECTRONICS CO., LTD.                *
 *                          ALL RIGHTS RESERVED                              *
 *                                                                           *
 *   Permission is hereby granted to licensees of Samsung Electronics        *
 *   Co., Ltd. products to use or abstract this computer program only in     *
 *   accordance with the terms of the NAND FLASH MEMORY SOFTWARE LICENSE     *
 *   AGREEMENT for the sole purpose of implementing a product based on       *
 *   Samsung Electronics Co., Ltd. products. No other rights to reproduce,   *
 *   use, or disseminate this computer program, whether in part or in        *
 *   whole, are granted.                                                     *
 *                                                                           *
 *   Samsung Electronics Co., Ltd. makes no representation or warranties     *
 *   with respect to the performance of this computer program, and           *
 *   specifically disclaims any responsibility for any damages,              *
 *   special or consequential, connected with the use of this program.       *
 *                                                                           *
 *---------------------------------------------------------------------------*
*/
/**
 *  @version 	RFS_1.3.1_b072_RTM
 *  @file	include/linux/rfs_fs_sb.h
 *  @brief	header file for RFS superblock
 *
 *
 */
     
#ifndef _LINUX_RFS_FS_SB
#define _LINUX_RFS_FS_SB

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

/*
 * RFS file system superblock data in memory(in-core)
 */

/* rfs mount options */
struct rfs_mount_info {
        char 	*codepage;
	char	*str_fcache;
        __u32   isvfat;
        __u32   opts;
};

/* rfs private data structure of sb */
struct rfs_sb_info {
	__u32	fat_bits;		/* FAT bits (12, 16 or 32) */
	__u32	blks_per_clu;
	__u32	blks_per_clu_bits;
	__u32	cluster_size;
	__u32	cluster_bits;
	__u32	num_clusters;

	loff_t	fat_start_addr;		/* start address of first FAT table */

	loff_t	root_start_addr;	/* start address of root directory */
	loff_t	root_end_addr;		/* end address of root directory */

	sector_t data_start;		/* start block of data area */

	__u32	root_clu;		/* root dir cluster, FAT16 = 0 */
	__u32	search_ptr;		/* cluster search pointer */
	__u32   num_used_clusters;	/* the number of used clusters */

	struct inode *root_inode;

	/* for FAT table */
	void   *fat_mutex;
	
	struct rfs_mount_info options;

	/* RFS internal FAT cache */
	struct list_head fcache_lru_list;
	struct rfs_fcache *fcache_array;
	unsigned int fcache_size;	/* numof block for fcache */

	struct nls_table *nls_disk;

	/* fields for log */
	void *log_info;			/* private for log structure */

	/* CONFIG_RFS_RDONLY_MOUNT */
        __u32   use_log;	

	/* chunk list for map destroy */
	struct list_head free_chunks;
	unsigned int nr_free_chunk;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 0)
	struct task_struct *sleep_proc;
	struct timer_list timer;
#endif

	unsigned long highest_d_ino;

#ifdef RFS_CLUSTER_CHANGE_NOTIFY
	int cluster_usage_changed;
#endif
};

/* get super block info */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
static inline struct rfs_sb_info *RFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}
#else
#define RFS_SB(s) 	(&((s)->u.rfs_sb))
#endif

#endif
