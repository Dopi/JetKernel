/*
 * Copyright (c) 2000-2005 Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write the Free Software Foundation,
 * Inc.,  51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef __XFS_MOUNT_H__
#define	__XFS_MOUNT_H__

typedef struct xfs_trans_reservations {
	uint	tr_write;	/* extent alloc trans */
	uint	tr_itruncate;	/* truncate trans */
	uint	tr_rename;	/* rename trans */
	uint	tr_link;	/* link trans */
	uint	tr_remove;	/* unlink trans */
	uint	tr_symlink;	/* symlink trans */
	uint	tr_create;	/* create trans */
	uint	tr_mkdir;	/* mkdir trans */
	uint	tr_ifree;	/* inode free trans */
	uint	tr_ichange;	/* inode update trans */
	uint	tr_growdata;	/* fs data section grow trans */
	uint	tr_swrite;	/* sync write inode trans */
	uint	tr_addafork;	/* cvt inode to attributed trans */
	uint	tr_writeid;	/* write setuid/setgid file */
	uint	tr_attrinval;	/* attr fork buffer invalidation */
	uint	tr_attrset;	/* set/create an attribute */
	uint	tr_attrrm;	/* remove an attribute */
	uint	tr_clearagi;	/* clear bad agi unlinked ino bucket */
	uint	tr_growrtalloc;	/* grow realtime allocations */
	uint	tr_growrtzero;	/* grow realtime zeroing */
	uint	tr_growrtfree;	/* grow realtime freeing */
} xfs_trans_reservations_t;

#ifndef __KERNEL__

#define xfs_daddr_to_agno(mp,d) \
	((xfs_agnumber_t)(XFS_BB_TO_FSBT(mp, d) / (mp)->m_sb.sb_agblocks))
#define xfs_daddr_to_agbno(mp,d) \
	((xfs_agblock_t)(XFS_BB_TO_FSBT(mp, d) % (mp)->m_sb.sb_agblocks))

#else /* __KERNEL__ */

#include "xfs_sync.h"

struct cred;
struct log;
struct xfs_mount_args;
struct xfs_inode;
struct xfs_bmbt_irec;
struct xfs_bmap_free;
struct xfs_extdelta;
struct xfs_swapext;
struct xfs_mru_cache;
struct xfs_nameops;
struct xfs_ail;

/*
 * Prototypes and functions for the Data Migration subsystem.
 */

typedef int	(*xfs_send_data_t)(int, struct xfs_inode *,
			xfs_off_t, size_t, int, int *);
typedef int	(*xfs_send_mmap_t)(struct vm_area_struct *, uint);
typedef int	(*xfs_send_destroy_t)(struct xfs_inode *, dm_right_t);
typedef int	(*xfs_send_namesp_t)(dm_eventtype_t, struct xfs_mount *,
			struct xfs_inode *, dm_right_t,
			struct xfs_inode *, dm_right_t,
			const char *, const char *, mode_t, int, int);
typedef int	(*xfs_send_mount_t)(struct xfs_mount *, dm_right_t,
			char *, char *);
typedef void	(*xfs_send_unmount_t)(struct xfs_mount *, struct xfs_inode *,
			dm_right_t, mode_t, int, int);

typedef struct xfs_dmops {
	xfs_send_data_t		xfs_send_data;
	xfs_send_mmap_t		xfs_send_mmap;
	xfs_send_destroy_t	xfs_send_destroy;
	xfs_send_namesp_t	xfs_send_namesp;
	xfs_send_mount_t	xfs_send_mount;
	xfs_send_unmount_t	xfs_send_unmount;
} xfs_dmops_t;

#define XFS_SEND_DATA(mp, ev,ip,off,len,fl,lock) \
	(*(mp)->m_dm_ops->xfs_send_data)(ev,ip,off,len,fl,lock)
#define XFS_SEND_MMAP(mp, vma,fl) \
	(*(mp)->m_dm_ops->xfs_send_mmap)(vma,fl)
#define XFS_SEND_DESTROY(mp, ip,right) \
	(*(mp)->m_dm_ops->xfs_send_destroy)(ip,right)
#define XFS_SEND_NAMESP(mp, ev,b1,r1,b2,r2,n1,n2,mode,rval,fl) \
	(*(mp)->m_dm_ops->xfs_send_namesp)(ev,NULL,b1,r1,b2,r2,n1,n2,mode,rval,fl)
#define XFS_SEND_PREUNMOUNT(mp,b1,r1,b2,r2,n1,n2,mode,rval,fl) \
	(*(mp)->m_dm_ops->xfs_send_namesp)(DM_EVENT_PREUNMOUNT,mp,b1,r1,b2,r2,n1,n2,mode,rval,fl)
#define XFS_SEND_MOUNT(mp,right,path,name) \
	(*(mp)->m_dm_ops->xfs_send_mount)(mp,right,path,name)
#define XFS_SEND_UNMOUNT(mp, ip,right,mode,rval,fl) \
	(*(mp)->m_dm_ops->xfs_send_unmount)(mp,ip,right,mode,rval,fl)


/*
 * Prototypes and functions for the Quota Management subsystem.
 */

struct xfs_dquot;
struct xfs_dqtrxops;
struct xfs_quotainfo;

typedef int	(*xfs_qminit_t)(struct xfs_mount *, uint *, uint *);
typedef int	(*xfs_qmmount_t)(struct xfs_mount *, uint, uint);
typedef void	(*xfs_qmunmount_t)(struct xfs_mount *);
typedef void	(*xfs_qmdone_t)(struct xfs_mount *);
typedef void	(*xfs_dqrele_t)(struct xfs_dquot *);
typedef int	(*xfs_dqattach_t)(struct xfs_inode *, uint);
typedef void	(*xfs_dqdetach_t)(struct xfs_inode *);
typedef int	(*xfs_dqpurgeall_t)(struct xfs_mount *, uint);
typedef int	(*xfs_dqvopalloc_t)(struct xfs_mount *,
			struct xfs_inode *, uid_t, gid_t, prid_t, uint,
			struct xfs_dquot **, struct xfs_dquot **);
typedef void	(*xfs_dqvopcreate_t)(struct xfs_trans *, struct xfs_inode *,
			struct xfs_dquot *, struct xfs_dquot *);
typedef int	(*xfs_dqvoprename_t)(struct xfs_inode **);
typedef struct xfs_dquot * (*xfs_dqvopchown_t)(
			struct xfs_trans *, struct xfs_inode *,
			struct xfs_dquot **, struct xfs_dquot *);
typedef int	(*xfs_dqvopchownresv_t)(struct xfs_trans *, struct xfs_inode *,
			struct xfs_dquot *, struct xfs_dquot *, uint);
typedef void	(*xfs_dqstatvfs_t)(struct xfs_inode *, struct kstatfs *);
typedef int	(*xfs_dqsync_t)(struct xfs_mount *, int flags);
typedef int	(*xfs_quotactl_t)(struct xfs_mount *, int, int, xfs_caddr_t);

typedef struct xfs_qmops {
	xfs_qminit_t		xfs_qminit;
	xfs_qmdone_t		xfs_qmdone;
	xfs_qmmount_t		xfs_qmmount;
	xfs_qmunmount_t		xfs_qmunmount;
	xfs_dqrele_t		xfs_dqrele;
	xfs_dqattach_t		xfs_dqattach;
	xfs_dqdetach_t		xfs_dqdetach;
	xfs_dqpurgeall_t	xfs_dqpurgeall;
	xfs_dqvopalloc_t	xfs_dqvopalloc;
	xfs_dqvopcreate_t	xfs_dqvopcreate;
	xfs_dqvoprename_t	xfs_dqvoprename;
	xfs_dqvopchown_t	xfs_dqvopchown;
	xfs_dqvopchownresv_t	xfs_dqvopchownresv;
	xfs_dqstatvfs_t		xfs_dqstatvfs;
	xfs_dqsync_t		xfs_dqsync;
	xfs_quotactl_t		xfs_quotactl;
	struct xfs_dqtrxops	*xfs_dqtrxops;
} xfs_qmops_t;

#define XFS_QM_INIT(mp, mnt, fl) \
	(*(mp)->m_qm_ops->xfs_qminit)(mp, mnt, fl)
#define XFS_QM_MOUNT(mp, mnt, fl) \
	(*(mp)->m_qm_ops->xfs_qmmount)(mp, mnt, fl)
#define XFS_QM_UNMOUNT(mp) \
	(*(mp)->m_qm_ops->xfs_qmunmount)(mp)
#define XFS_QM_DONE(mp) \
	(*(mp)->m_qm_ops->xfs_qmdone)(mp)
#define XFS_QM_DQRELE(mp, dq) \
	(*(mp)->m_qm_ops->xfs_dqrele)(dq)
#define XFS_QM_DQATTACH(mp, ip, fl) \
	(*(mp)->m_qm_ops->xfs_dqattach)(ip, fl)
#define XFS_QM_DQDETACH(mp, ip) \
	(*(mp)->m_qm_ops->xfs_dqdetach)(ip)
#define XFS_QM_DQPURGEALL(mp, fl) \
	(*(mp)->m_qm_ops->xfs_dqpurgeall)(mp, fl)
#define XFS_QM_DQVOPALLOC(mp, ip, uid, gid, prid, fl, dq1, dq2) \
	(*(mp)->m_qm_ops->xfs_dqvopalloc)(mp, ip, uid, gid, prid, fl, dq1, dq2)
#define XFS_QM_DQVOPCREATE(mp, tp, ip, dq1, dq2) \
	(*(mp)->m_qm_ops->xfs_dqvopcreate)(tp, ip, dq1, dq2)
#define XFS_QM_DQVOPRENAME(mp, ip) \
	(*(mp)->m_qm_ops->xfs_dqvoprename)(ip)
#define XFS_QM_DQVOPCHOWN(mp, tp, ip, dqp, dq) \
	(*(mp)->m_qm_ops->xfs_dqvopchown)(tp, ip, dqp, dq)
#define XFS_QM_DQVOPCHOWNRESV(mp, tp, ip, dq1, dq2, fl) \
	(*(mp)->m_qm_ops->xfs_dqvopchownresv)(tp, ip, dq1, dq2, fl)
#define XFS_QM_DQSTATVFS(ip, statp) \
	(*(ip)->i_mount->m_qm_ops->xfs_dqstatvfs)(ip, statp)
#define XFS_QM_DQSYNC(mp, flags) \
	(*(mp)->m_qm_ops->xfs_dqsync)(mp, flags)
#define XFS_QM_QUOTACTL(mp, cmd, id, addr) \
	(*(mp)->m_qm_ops->xfs_quotactl)(mp, cmd, id, addr)

#ifdef HAVE_PERCPU_SB

/*
 * Valid per-cpu incore superblock counters. Note that if you add new counters,
 * you may need to define new counter disabled bit field descriptors as there
 * are more possible fields in the superblock that can fit in a bitfield on a
 * 32 bit platform. The XFS_SBS_* values for the current current counters just
 * fit.
 */
typedef struct xfs_icsb_cnts {
	uint64_t	icsb_fdblocks;
	uint64_t	icsb_ifree;
	uint64_t	icsb_icount;
	unsigned long	icsb_flags;
} xfs_icsb_cnts_t;

#define XFS_ICSB_FLAG_LOCK	(1 << 0)	/* counter lock bit */

#define XFS_ICSB_LAZY_COUNT	(1 << 1)	/* accuracy not needed */

extern int	xfs_icsb_init_counters(struct xfs_mount *);
extern void	xfs_icsb_reinit_counters(struct xfs_mount *);
extern void	xfs_icsb_destroy_counters(struct xfs_mount *);
extern void	xfs_icsb_sync_counters(struct xfs_mount *, int);
extern void	xfs_icsb_sync_counters_locked(struct xfs_mount *, int);

#else
#define xfs_icsb_init_counters(mp)		(0)
#define xfs_icsb_destroy_counters(mp)		do { } while (0)
#define xfs_icsb_reinit_counters(mp)		do { } while (0)
#define xfs_icsb_sync_counters(mp, flags)	do { } while (0)
#define xfs_icsb_sync_counters_locked(mp, flags) do { } while (0)
#endif

typedef struct xfs_mount {
	struct super_block	*m_super;
	xfs_tid_t		m_tid;		/* next unused tid for fs */
	struct xfs_ail		*m_ail;		/* fs active log item list */
	xfs_sb_t		m_sb;		/* copy of fs superblock */
	spinlock_t		m_sb_lock;	/* sb counter lock */
	struct xfs_buf		*m_sb_bp;	/* buffer for superblock */
	char			*m_fsname;	/* filesystem name */
	int			m_fsname_len;	/* strlen of fs name */
	char			*m_rtname;	/* realtime device name */
	char			*m_logname;	/* external log device name */
	int			m_bsize;	/* fs logical block size */
	xfs_agnumber_t		m_agfrotor;	/* last ag where space found */
	xfs_agnumber_t		m_agirotor;	/* last ag dir inode alloced */
	spinlock_t		m_agirotor_lock;/* .. and lock protecting it */
	xfs_agnumber_t		m_maxagi;	/* highest inode alloc group */
	uint			m_readio_log;	/* min read size log bytes */
	uint			m_readio_blocks; /* min read size blocks */
	uint			m_writeio_log;	/* min write size log bytes */
	uint			m_writeio_blocks; /* min write size blocks */
	struct log		*m_log;		/* log specific stuff */
	int			m_logbufs;	/* number of log buffers */
	int			m_logbsize;	/* size of each log buffer */
	uint			m_rsumlevels;	/* rt summary levels */
	uint			m_rsumsize;	/* size of rt summary, bytes */
	struct xfs_inode	*m_rbmip;	/* pointer to bitmap inode */
	struct xfs_inode	*m_rsumip;	/* pointer to summary inode */
	struct xfs_inode	*m_rootip;	/* pointer to root directory */
	struct xfs_quotainfo	*m_quotainfo;	/* disk quota information */
	xfs_buftarg_t		*m_ddev_targp;	/* saves taking the address */
	xfs_buftarg_t		*m_logdev_targp;/* ptr to log device */
	xfs_buftarg_t		*m_rtdev_targp;	/* ptr to rt device */
	__uint8_t		m_blkbit_log;	/* blocklog + NBBY */
	__uint8_t		m_blkbb_log;	/* blocklog - BBSHIFT */
	__uint8_t		m_agno_log;	/* log #ag's */
	__uint8_t		m_agino_log;	/* #bits for agino in inum */
	__uint16_t		m_inode_cluster_size;/* min inode buf size */
	uint			m_blockmask;	/* sb_blocksize-1 */
	uint			m_blockwsize;	/* sb_blocksize in words */
	uint			m_blockwmask;	/* blockwsize-1 */
	uint			m_alloc_mxr[2];	/* max alloc btree records */
	uint			m_alloc_mnr[2];	/* min alloc btree records */
	uint			m_bmap_dmxr[2];	/* max bmap btree records */
	uint			m_bmap_dmnr[2];	/* min bmap btree records */
	uint			m_inobt_mxr[2];	/* max inobt btree records */
	uint			m_inobt_mnr[2];	/* min inobt btree records */
	uint			m_ag_maxlevels;	/* XFS_AG_MAXLEVELS */
	uint			m_bm_maxlevels[2]; /* XFS_BM_MAXLEVELS */
	uint			m_in_maxlevels;	/* XFS_IN_MAXLEVELS */
	struct xfs_perag	*m_perag;	/* per-ag accounting info */
	struct rw_semaphore	m_peraglock;	/* lock for m_perag (pointer) */
	struct mutex		m_growlock;	/* growfs mutex */
	int			m_fixedfsid[2];	/* unchanged for life of FS */
	uint			m_dmevmask;	/* DMI events for this FS */
	__uint64_t		m_flags;	/* global mount flags */
	uint			m_attroffset;	/* inode attribute offset */
	uint			m_dir_node_ents; /* #entries in a dir danode */
	uint			m_attr_node_ents; /* #entries in attr danode */
	int			m_ialloc_inos;	/* inodes in inode allocation */
	int			m_ialloc_blks;	/* blocks in inode allocation */
	int			m_litino;	/* size of inode union area */
	int			m_inoalign_mask;/* mask sb_inoalignmt if used */
	uint			m_qflags;	/* quota status flags */
	xfs_trans_reservations_t m_reservations;/* precomputed res values */
	__uint64_t		m_maxicount;	/* maximum inode count */
	__uint64_t		m_maxioffset;	/* maximum inode offset */
	__uint64_t		m_resblks;	/* total reserved blocks */
	__uint64_t		m_resblks_avail;/* available reserved blocks */
#if XFS_BIG_INUMS
	xfs_ino_t		m_inoadd;	/* add value for ino64_offset */
#endif
	int			m_dalign;	/* stripe unit */
	int			m_swidth;	/* stripe width */
	int			m_sinoalign;	/* stripe unit inode alignment */
	int			m_attr_magicpct;/* 37% of the blocksize */
	int			m_dir_magicpct;	/* 37% of the dir blocksize */
	__uint8_t		m_sectbb_log;	/* sectlog - BBSHIFT */
	const struct xfs_nameops *m_dirnameops;	/* vector of dir name ops */
	int			m_dirblksize;	/* directory block sz--bytes */
	int			m_dirblkfsbs;	/* directory block sz--fsbs */
	xfs_dablk_t		m_dirdatablk;	/* blockno of dir data v2 */
	xfs_dablk_t		m_dirleafblk;	/* blockno of dir non-data v2 */
	xfs_dablk_t		m_dirfreeblk;	/* blockno of dirfreeindex v2 */
	uint			m_chsize;	/* size of next field */
	struct xfs_chash	*m_chash;	/* fs private inode per-cluster
						 * hash table */
	struct xfs_dmops	*m_dm_ops;	/* vector of DMI ops */
	struct xfs_qmops	*m_qm_ops;	/* vector of XQM ops */
	atomic_t		m_active_trans;	/* number trans frozen */
#ifdef HAVE_PERCPU_SB
	xfs_icsb_cnts_t		*m_sb_cnts;	/* per-cpu superblock counters */
	unsigned long		m_icsb_counters; /* disabled per-cpu counters */
	struct notifier_block	m_icsb_notifier; /* hotplug cpu notifier */
	struct mutex		m_icsb_mutex;	/* balancer sync lock */
#endif
	struct xfs_mru_cache	*m_filestream;  /* per-mount filestream data */
	struct task_struct	*m_sync_task;	/* generalised sync thread */
	bhv_vfs_sync_work_t	m_sync_work;	/* work item for VFS_SYNC */
	struct list_head	m_sync_list;	/* sync thread work item list */
	spinlock_t		m_sync_lock;	/* work item list lock */
	int			m_sync_seq;	/* sync thread generation no. */
	wait_queue_head_t	m_wait_single_sync_task;
	__int64_t		m_update_flags;	/* sb flags we need to update
						   on the next remount,rw */
} xfs_mount_t;

/*
 * Flags for m_flags.
 */
#define XFS_MOUNT_WSYNC		(1ULL << 0)	/* for nfs - all metadata ops
						   must be synchronous except
						   for space allocations */
#define XFS_MOUNT_INO64		(1ULL << 1)
#define XFS_MOUNT_DMAPI		(1ULL << 2)	/* dmapi is enabled */
#define XFS_MOUNT_WAS_CLEAN	(1ULL << 3)
#define XFS_MOUNT_FS_SHUTDOWN	(1ULL << 4)	/* atomic stop of all filesystem
						   operations, typically for
						   disk errors in metadata */
#define XFS_MOUNT_RETERR	(1ULL << 6)     /* return alignment errors to
						   user */
#define XFS_MOUNT_NOALIGN	(1ULL << 7)	/* turn off stripe alignment
						   allocations */
#define XFS_MOUNT_ATTR2		(1ULL << 8)	/* allow use of attr2 format */
#define XFS_MOUNT_GRPID		(1ULL << 9)	/* group-ID assigned from directory */
#define XFS_MOUNT_NORECOVERY	(1ULL << 10)	/* no recovery - dirty fs */
#define XFS_MOUNT_DFLT_IOSIZE	(1ULL << 12)	/* set default i/o size */
#define XFS_MOUNT_OSYNCISOSYNC	(1ULL << 13)	/* o_sync is REALLY o_sync */
						/* osyncisdsync is now default*/
#define XFS_MOUNT_32BITINODES	(1ULL << 14)	/* do not create inodes above
						 * 32 bits in size */
#define XFS_MOUNT_SMALL_INUMS	(1ULL << 15)	/* users wants 32bit inodes */
#define XFS_MOUNT_NOUUID	(1ULL << 16)	/* ignore uuid during mount */
#define XFS_MOUNT_BARRIER	(1ULL << 17)
#define XFS_MOUNT_IKEEP		(1ULL << 18)	/* keep empty inode clusters*/
#define XFS_MOUNT_SWALLOC	(1ULL << 19)	/* turn on stripe width
						 * allocation */
#define XFS_MOUNT_RDONLY	(1ULL << 20)	/* read-only fs */
#define XFS_MOUNT_DIRSYNC	(1ULL << 21)	/* synchronous directory ops */
#define XFS_MOUNT_COMPAT_IOSIZE	(1ULL << 22)	/* don't report large preferred
						 * I/O size in stat() */
#define XFS_MOUNT_NO_PERCPU_SB	(1ULL << 23)	/* don't use per-cpu superblock
						   counters */
#define XFS_MOUNT_FILESTREAMS	(1ULL << 24)	/* enable the filestreams
						   allocator */
#define XFS_MOUNT_NOATTR2	(1ULL << 25)	/* disable use of attr2 format */


/*
 * Default minimum read and write sizes.
 */
#define XFS_READIO_LOG_LARGE	16
#define XFS_WRITEIO_LOG_LARGE	16

/*
 * Max and min values for mount-option defined I/O
 * preallocation sizes.
 */
#define XFS_MAX_IO_LOG		30	/* 1G */
#define XFS_MIN_IO_LOG		PAGE_SHIFT

/*
 * Synchronous read and write sizes.  This should be
 * better for NFSv2 wsync filesystems.
 */
#define	XFS_WSYNC_READIO_LOG	15	/* 32K */
#define	XFS_WSYNC_WRITEIO_LOG	14	/* 16K */

/*
 * Allow large block sizes to be reported to userspace programs if the
 * "largeio" mount option is used.
 *
 * If compatibility mode is specified, simply return the basic unit of caching
 * so that we don't get inefficient read/modify/write I/O from user apps.
 * Otherwise....
 *
 * If the underlying volume is a stripe, then return the stripe width in bytes
 * as the recommended I/O size. It is not a stripe and we've set a default
 * buffered I/O size, return that, otherwise return the compat default.
 */
static inline unsigned long
xfs_preferred_iosize(xfs_mount_t *mp)
{
	if (mp->m_flags & XFS_MOUNT_COMPAT_IOSIZE)
		return PAGE_CACHE_SIZE;
	return (mp->m_swidth ?
		(mp->m_swidth << mp->m_sb.sb_blocklog) :
		((mp->m_flags & XFS_MOUNT_DFLT_IOSIZE) ?
			(1 << (int)MAX(mp->m_readio_log, mp->m_writeio_log)) :
			PAGE_CACHE_SIZE));
}

#define XFS_MAXIOFFSET(mp)	((mp)->m_maxioffset)

#define XFS_LAST_UNMOUNT_WAS_CLEAN(mp)	\
				((mp)->m_flags & XFS_MOUNT_WAS_CLEAN)
#define XFS_FORCED_SHUTDOWN(mp)	((mp)->m_flags & XFS_MOUNT_FS_SHUTDOWN)
void xfs_do_force_shutdown(struct xfs_mount *mp, int flags, char *fname,
		int lnnum);
#define xfs_force_shutdown(m,f)	\
	xfs_do_force_shutdown(m, f, __FILE__, __LINE__)

#define SHUTDOWN_META_IO_ERROR	0x0001	/* write attempt to metadata failed */
#define SHUTDOWN_LOG_IO_ERROR	0x0002	/* write attempt to the log failed */
#define SHUTDOWN_FORCE_UMOUNT	0x0004	/* shutdown from a forced unmount */
#define SHUTDOWN_CORRUPT_INCORE	0x0008	/* corrupt in-memory data structures */
#define SHUTDOWN_REMOTE_REQ	0x0010	/* shutdown came from remote cell */
#define SHUTDOWN_DEVICE_REQ	0x0020	/* failed all paths to the device */

#define xfs_test_for_freeze(mp)		((mp)->m_super->s_frozen)
#define xfs_wait_for_freeze(mp,l)	vfs_check_frozen((mp)->m_super, (l))

/*
 * Flags for xfs_mountfs
 */
#define XFS_MFSI_QUIET		0x40	/* Be silent if mount errors found */

static inline xfs_agnumber_t
xfs_daddr_to_agno(struct xfs_mount *mp, xfs_daddr_t d)
{
	xfs_daddr_t ld = XFS_BB_TO_FSBT(mp, d);
	do_div(ld, mp->m_sb.sb_agblocks);
	return (xfs_agnumber_t) ld;
}

static inline xfs_agblock_t
xfs_daddr_to_agbno(struct xfs_mount *mp, xfs_daddr_t d)
{
	xfs_daddr_t ld = XFS_BB_TO_FSBT(mp, d);
	return (xfs_agblock_t) do_div(ld, mp->m_sb.sb_agblocks);
}

/*
 * perag get/put wrappers for eventual ref counting
 */
static inline xfs_perag_t *
xfs_get_perag(struct xfs_mount *mp, xfs_ino_t ino)
{
	return &mp->m_perag[XFS_INO_TO_AGNO(mp, ino)];
}

static inline void
xfs_put_perag(struct xfs_mount *mp, xfs_perag_t *pag)
{
	/* nothing to see here, move along */
}

/*
 * Per-cpu superblock locking functions
 */
#ifdef HAVE_PERCPU_SB
STATIC_INLINE void
xfs_icsb_lock(xfs_mount_t *mp)
{
	mutex_lock(&mp->m_icsb_mutex);
}

STATIC_INLINE void
xfs_icsb_unlock(xfs_mount_t *mp)
{
	mutex_unlock(&mp->m_icsb_mutex);
}
#else
#define xfs_icsb_lock(mp)
#define xfs_icsb_unlock(mp)
#endif

/*
 * This structure is for use by the xfs_mod_incore_sb_batch() routine.
 * xfs_growfs can specify a few fields which are more than int limit
 */
typedef struct xfs_mod_sb {
	xfs_sb_field_t	msb_field;	/* Field to modify, see below */
	int64_t		msb_delta;	/* Change to make to specified field */
} xfs_mod_sb_t;

#define	XFS_MOUNT_ILOCK(mp)	mutex_lock(&((mp)->m_ilock))
#define	XFS_MOUNT_IUNLOCK(mp)	mutex_unlock(&((mp)->m_ilock))

extern int	xfs_log_sbcount(xfs_mount_t *, uint);
extern int	xfs_mountfs(xfs_mount_t *mp);
extern void	xfs_mountfs_check_barriers(xfs_mount_t *mp);

extern void	xfs_unmountfs(xfs_mount_t *);
extern int	xfs_unmountfs_writesb(xfs_mount_t *);
extern int	xfs_mod_incore_sb(xfs_mount_t *, xfs_sb_field_t, int64_t, int);
extern int	xfs_mod_incore_sb_unlocked(xfs_mount_t *, xfs_sb_field_t,
			int64_t, int);
extern int	xfs_mod_incore_sb_batch(xfs_mount_t *, xfs_mod_sb_t *,
			uint, int);
extern int	xfs_mount_log_sb(xfs_mount_t *, __int64_t);
extern struct xfs_buf *xfs_getsb(xfs_mount_t *, int);
extern int	xfs_readsb(xfs_mount_t *, int);
extern void	xfs_freesb(xfs_mount_t *);
extern int	xfs_fs_writable(xfs_mount_t *);
extern int	xfs_sb_validate_fsb_count(struct xfs_sb *, __uint64_t);

extern int	xfs_dmops_get(struct xfs_mount *);
extern void	xfs_dmops_put(struct xfs_mount *);
extern int	xfs_qmops_get(struct xfs_mount *);
extern void	xfs_qmops_put(struct xfs_mount *);

extern struct xfs_dmops xfs_dmcore_xfs;

#endif	/* __KERNEL__ */

extern void	xfs_mod_sb(struct xfs_trans *, __int64_t);
extern xfs_agnumber_t	xfs_initialize_perag(struct xfs_mount *, xfs_agnumber_t);
extern void	xfs_sb_from_disk(struct xfs_sb *, struct xfs_dsb *);
extern void	xfs_sb_to_disk(struct xfs_dsb *, struct xfs_sb *, __int64_t);

#endif	/* __XFS_MOUNT_H__ */
