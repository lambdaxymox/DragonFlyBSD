/*
 * Internal Header for the Direct Rendering Manager
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * Copyright (c) 2009-2010, Code Aurora Forum.
 * All rights reserved.
 *
 * Author: Rickard E. (Rik) Faith <faith@valinux.com>
 * Author: Gareth Hughes <gareth@valinux.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _DRM_P_H_
#define _DRM_P_H_

#if defined(_KERNEL) || defined(__KERNEL__)

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/ktr.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/sglist.h>
#include <sys/stat.h>
#include <sys/priv.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <sys/spinlock.h>
#include <sys/spinlock2.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/filio.h>
#include <sys/sysctl.h>
#include <sys/bus.h>
#include <sys/queue.h>
#include <sys/signalvar.h>
#include <sys/poll.h>
#include <linux/highmem.h>
#include <sys/sbuf.h>
#include <sys/taskqueue.h>
#include <sys/tree.h>
#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_extern.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page2.h>
#include <vm/vm_pager.h>
#include <vm/vm_param.h>
#include <machine/param.h>
#include <machine/pmap.h>
#ifdef __x86_64__
#include <machine/specialreg.h>
#endif
#include <machine/sysarch.h>
#include <sys/endian.h>
#include <sys/mman.h>
#include <sys/rman.h>
#include <sys/memrange.h>
#include <sys/mutex.h>

#include <linux/atomic.h>
#include <linux/bug.h>
#include <linux/dma-mapping.h>
#include <linux/capability.h>
#include <linux/err.h>
#include <linux/idr.h>
#include <linux/pci.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kref.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/timer.h>
#include <asm/io.h>
#include <linux/seq_file.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/uaccess.h>

#include <uapi_drm/drm.h>
#include <uapi_drm/drm_mode.h>

#include <drm/drm_agpsupport.h>
#include <drm/drm_crtc.h>
#include <drm/drm_global.h>
#include <drm/drm_hashtab.h>
#include <drm/drm_mem_util.h>
#include <drm/drm_mm.h>
#include <drm/drm_os_linux.h>
#include <uapi_drm/drm_sarea.h>
#include <drm/drm_vma_manager.h>

struct drm_file;
struct drm_device;
struct drm_agp_head;
struct drm_local_map;
struct drm_device_dma;
struct drm_dma_handle;
struct drm_gem_object;

struct device_node;
#ifdef CONFIG_VIDEOMODE_HELPERS
struct videomode;	/* XXX empty struct in videomode.h ? */
#endif
struct reservation_object;
struct dma_buf_attachment;

/*
 * 4 debug categories are defined:
 *
 * CORE: Used in the generic drm code: drm_ioctl.c, drm_mm.c, drm_memory.c, ...
 *	 This is the category used by the DRM_DEBUG() macro.
 *
 * DRIVER: Used in the vendor specific part of the driver: i915, radeon, ...
 *	   This is the category used by the DRM_DEBUG_DRIVER() macro.
 *
 * KMS: used in the modesetting code.
 *	This is the category used by the DRM_DEBUG_KMS() macro.
 *
 * PRIME: used in the prime code.
 *	  This is the category used by the DRM_DEBUG_PRIME() macro.
 *
 * ATOMIC: used in the atomic code.
 *	  This is the category used by the DRM_DEBUG_ATOMIC() macro.
 *
 * Enabling verbose debug messages is done through the drm.debug parameter,
 * each category being enabled by a bit.
 *
 * drm.debug=0x1 will enable CORE messages
 * drm.debug=0x2 will enable DRIVER messages
 * drm.debug=0x3 will enable CORE and DRIVER messages
 * ...
 * drm.debug=0xf will enable all messages
 *
 * An interesting feature is that it's possible to enable verbose logging at
 * run-time by echoing the debug value in its sysfs node:
 *   # echo 0xf > /sys/module/drm/parameters/debug
 */
#define DRM_UT_CORE 		0x01
#define DRM_UT_DRIVER		0x02
#define DRM_UT_KMS		0x04
#define DRM_UT_PRIME		0x08
#define DRM_UT_ATOMIC		0x10

void drm_ut_debug_printk(const char *function_name,
			 const char *format, ...);

void drm_err(const char *format, ...);

/***********************************************************************/
/** \name DRM template customization defaults */
/*@{*/

/* driver capabilities and requirements mask */
#define DRIVER_USE_AGP			0x1
#define DRIVER_REQUIRE_AGP		0x2
#define DRIVER_PCI_DMA			0x8
#define DRIVER_SG			0x10
#define DRIVER_HAVE_DMA			0x20
#define DRIVER_HAVE_IRQ			0x40
#define DRIVER_IRQ_SHARED		0x80
#define DRIVER_GEM			0x1000
#define DRIVER_MODESET			0x2000
#define DRIVER_PRIME			0x4000
#define DRIVER_RENDER			0x8000
#define DRIVER_ATOMIC			0x10000
#define DRIVER_KMS_LEGACY_CONTEXT	0x20000

#define DRM_MAGIC_HASH_ORDER  4  /**< Size of key hash table. Must be power of 2. */

/***********************************************************************/
/** \name Macros to make printk easier */
/*@{*/

/**
 * Error output.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */
#define DRM_ERROR(fmt, ...) \
	kprintf("error: [" DRM_NAME ":pid%d:%s] *ERROR* " fmt,		\
	    DRM_CURRENTPID, __func__ , ##__VA_ARGS__)

/**
 * Rate limited error output.  Like DRM_ERROR() but won't flood the log.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */

#define DRM_INFO(fmt, ...)  kprintf("info: [" DRM_NAME "] " fmt , ##__VA_ARGS__)

#define DRM_INFO_ONCE(fmt, ...)				\
	printk_once(KERN_INFO "[" DRM_NAME "] " fmt, ##__VA_ARGS__)

/**
 * Debug output.
 *
 * \param fmt printf() like format string.
 * \param arg arguments
 */

#define	DRM_DEBUGBITS_DEBUG		0x1
#define	DRM_DEBUGBITS_KMS		0x2
#define	DRM_DEBUGBITS_FAILED_IOCTL	0x4
#define	DRM_DEBUGBITS_VERBOSE		0x8

#define DRM_DEBUG(fmt, ...) do {					\
	if ((drm_debug & DRM_DEBUGBITS_DEBUG) != 0)		\
		kprintf("[" DRM_NAME ":pid%d:%s] " fmt, DRM_CURRENTPID,	\
			__func__ , ##__VA_ARGS__);			\
} while (0)

#define DRM_DEBUG_VERBOSE(fmt, ...) do {				\
	if ((drm_debug & DRM_DEBUGBITS_VERBOSE) != 0)		\
		kprintf("[" DRM_NAME ":pid%d:%s] " fmt, DRM_CURRENTPID,	\
			__func__ , ##__VA_ARGS__);			\
} while (0)

#define DRM_DEBUG_KMS(fmt, ...) do {					\
	if ((drm_debug & DRM_DEBUGBITS_KMS) != 0)			\
		kprintf("[" DRM_NAME ":KMS:pid%d:%s] " fmt, DRM_CURRENTPID,\
			__func__ , ##__VA_ARGS__);			\
} while (0)

#define DRM_DEBUG_DRIVER(fmt, ...) do {					\
	if ((drm_debug & DRM_DEBUGBITS_KMS) != 0)			\
		kprintf("[" DRM_NAME ":KMS:pid%d:%s] " fmt, DRM_CURRENTPID,\
			__func__ , ##__VA_ARGS__);			\
} while (0)

#define DRM_DEBUG_ATOMIC(fmt, args...)					\
	do {								\
		if (unlikely(drm_debug & DRM_UT_ATOMIC))		\
			drm_ut_debug_printk(__func__, fmt, ##args);	\
	} while (0)

/*@}*/

/***********************************************************************/
/** \name Internal types and structures */
/*@{*/

#define	DRM_GEM_MAPPING_MASK	(3ULL << 62)
#define	DRM_GEM_MAPPING_KEY	(2ULL << 62) /* Non-canonical address form */
#define	DRM_GEM_MAX_IDX		0x3fffff
#define	DRM_GEM_MAPPING_IDX(o)	(((o) >> 40) & DRM_GEM_MAX_IDX)
#define	DRM_GEM_MAPPING_OFF(i)	(((uint64_t)(i)) << 40)
#define	DRM_GEM_MAPPING_MAPOFF(o) \
    ((o) & ~(DRM_GEM_MAPPING_OFF(DRM_GEM_MAX_IDX) | DRM_GEM_MAPPING_KEY))

SYSCTL_DECL(_hw_drm);

#define DRM_MAX(a,b) ((a)>(b)?(a):(b))

#define DRM_IF_VERSION(maj, min) (maj << 16 | min)

#define DRM_DEV_MODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)
#define DRM_DEV_UID	UID_ROOT
#define DRM_DEV_GID	GID_WHEEL

#define DRM_CURPROC		curthread
#define DRM_STRUCTPROC		struct thread
#define DRM_CURRENTPID		(curproc != NULL ? curproc->p_pid : -1)
#define DRM_LOCK(dev)		lockmgr(&(dev)->struct_mutex, LK_EXCLUSIVE)
#define DRM_UNLOCK(dev)		lockmgr(&(dev)->struct_mutex, LK_RELEASE)
#define	DRM_LOCK_SLEEP(dev, chan, flags, msg, timeout)			\
    (lksleep((chan), &(dev)->struct_mutex, (flags), (msg), (timeout)))
#if defined(INVARIANTS)
#define	DRM_LOCK_ASSERT(dev)	KKASSERT(lockstatus(&(dev)->struct_mutex, curthread) != 0);
#define	DRM_UNLOCK_ASSERT(dev)	KKASSERT(lockstatus(&(dev)->struct_mutex, curthread) == 0);
#else
#define	DRM_LOCK_ASSERT(d)
#define	DRM_UNLOCK_ASSERT(d)
#endif

#define DRM_SYSCTL_HANDLER_ARGS	(SYSCTL_HANDLER_ARGS)

typedef void			irqreturn_t;
#define IRQ_HANDLED		/* nothing */
#define IRQ_NONE		/* nothing */

#define drm_get_device_from_kdev(_kdev) (_kdev->si_drv1)

#define DRM_MTRR_WC		MDF_WRITECOMBINE

/* Returns -errno to shared code */
#define DRM_WAIT_ON( ret, queue, timeout, condition )		\
for ( ret = 0 ; !ret && !(condition) ; ) {			\
	lwkt_serialize_enter(&dev->irq_lock);			\
	if (!(condition)) {					\
            tsleep_interlock(&(queue), PCATCH);			\
            lwkt_serialize_exit(&dev->irq_lock);		\
            ret = -tsleep(&(queue), PCATCH | PINTERLOCKED,	\
			  "drmwtq", (timeout));			\
        } else {                                                \
                lwkt_serialize_exit(&dev->irq_lock);            \
        }                                                 	\
}

int vm_phys_fictitious_reg_range(vm_paddr_t start, vm_paddr_t end,
    vm_memattr_t memattr);
void vm_phys_fictitious_unreg_range(vm_paddr_t start, vm_paddr_t end);
vm_page_t vm_phys_fictitious_to_vm_page(vm_paddr_t pa);


#define	dev_dbg(dev, fmt, ...) do {					\
	if ((drm_debug& DRM_DEBUGBITS_KMS) != 0) {			\
		device_printf((dev), "debug: " fmt, ## __VA_ARGS__);	\
	}								\
} while (0)

typedef struct drm_pci_id_list
{
	int vendor;
	int device;
	long driver_private;
	char *name;
} drm_pci_id_list_t;

/**
 * Ioctl function type.
 *
 * \param inode device inode.
 * \param file_priv DRM file private pointer.
 * \param cmd command.
 * \param arg argument.
 */
typedef int drm_ioctl_t(struct drm_device *dev, void *data,
			struct drm_file *file_priv);

typedef int drm_ioctl_compat_t(struct file *filp, unsigned int cmd,
			       unsigned long arg);

#define DRM_IOCTL_NR(n)                ((n) & 0xff)

#define DRM_AUTH	0x1
#define DRM_MASTER	0x2
#define DRM_ROOT_ONLY	0x4
#define DRM_CONTROL_ALLOW 0x8
#define DRM_UNLOCKED	0x10
#define DRM_RENDER_ALLOW 0x20

struct drm_ioctl_desc {
	unsigned int cmd;
	int flags;
	drm_ioctl_t *func;
	unsigned int cmd_drv;
	const char *name;
};

/**
 * Creates a driver or general drm_ioctl_desc array entry for the given
 * ioctl, for use by drm_ioctl().
 */
#define DRM_IOCTL_DEF_DRV(ioctl, _func, _flags)			\
	[DRM_IOCTL_NR(DRM_##ioctl)] = {.cmd = DRM_##ioctl, .func = _func, .flags = _flags, .cmd_drv = DRM_IOCTL_##ioctl}

typedef struct drm_magic_entry {
	struct list_head head;
	struct drm_hash_item hash_item;
	struct drm_file	       *priv;
	struct drm_magic_entry *next;
} drm_magic_entry_t;

typedef struct drm_magic_head {
	struct drm_magic_entry *head;
	struct drm_magic_entry *tail;
} drm_magic_head_t;

/** bufs is one longer than it has to be */
struct drm_waitlist {
	int count;			/**< Number of possible buffers */
	struct drm_buf **bufs;		/**< List of pointers to buffers */
	struct drm_buf **rp;			/**< Read pointer */
	struct drm_buf **wp;			/**< Write pointer */
	struct drm_buf **end;		/**< End pointer */
	struct spinlock *read_lock;
	struct spinlock *write_lock;
};

/* Event queued up for userspace to read */
struct drm_pending_event {
	struct drm_event *event;
	struct list_head link;
	struct drm_file *file_priv;
	pid_t pid; /* pid of requester, no guarantee it's valid by the time
		      we deliver the event, for tracing only */
	void (*destroy)(struct drm_pending_event *event);
};

/* initial implementaton using a linked list - todo hashtab */
struct drm_prime_file_private {
	struct list_head head;
#ifdef DUMBBELL_WIP
	struct mutex lock;
#endif /* DUMBBELL_WIP */
};

/** File private data */
struct drm_file {
	int authenticated;
	struct drm_device *dev;
	int		  master;

	/* true when the client has asked us to expose stereo 3D mode flags */
	bool stereo_allowed;
	/*
	 * true if client understands CRTC primary planes and cursor planes
	 * in the plane list
	 */
	unsigned universal_planes:1;
	/* true if client understands atomic properties */
	unsigned atomic:1;

	pid_t		  pid;
	uid_t		  uid;
	drm_magic_t	  magic;
	unsigned long	  ioctl_count;
	struct list_head lhead;
	struct kqinfo	  dkq;

	/** Mapping of mm object handles to object pointers. */
	struct idr object_idr;
	/** Lock for synchronization of access to object_idr. */
	struct lock table_lock;

	void		 *driver_priv;

	int		  is_master;
	struct drm_master *masterp;

	/**
	 * fbs - List of framebuffers associated with this file.
	 *
	 * Protected by fbs_lock. Note that the fbs list holds a reference on
	 * the fb object to prevent it from untimely disappearing.
	 */
	struct list_head fbs;
	struct lock fbs_lock;

	/** User-created blob properties; this retains a reference on the
	 *  property. */
	struct list_head blobs;

	wait_queue_head_t event_wait;
	struct list_head  event_list;
	int		  event_space;

	struct drm_prime_file_private prime;
};

/**
 * Lock data.
 */
struct drm_lock_data {
	struct drm_hw_lock *hw_lock;	/**< Hardware lock */
	/** Private of lock holder's file (NULL=kernel) */
	struct drm_file *file_priv;
	wait_queue_head_t lock_queue;	/**< Queue of blocked processes */
	unsigned long lock_time;	/**< Time of last lock in jiffies */
};

/**
 * GEM specific mm private for tracking GEM objects
 */
struct drm_gem_mm {
	struct drm_vma_offset_manager vma_manager;
	struct drm_mm offset_manager;	/**< Offset mgmt for buffer objects */
	struct drm_open_hash offset_hash; /**< User token hash table for maps */
	struct unrhdr *idxunr;
};

/**
 * struct drm_master - drm master structure
 *
 * @refcount: Refcount for this master object.
 * @minor: Link back to minor char device we are master for. Immutable.
 * @unique: Unique identifier: e.g. busid. Protected by drm_global_mutex.
 * @unique_len: Length of unique field. Protected by drm_global_mutex.
 * @unique_size: Amount allocated. Protected by drm_global_mutex.
 * @magiclist: Hash of used authentication tokens. Protected by struct_mutex.
 * @magicfree: List of used authentication tokens. Protected by struct_mutex.
 * @lock: DRI lock information.
 * @driver_priv: Pointer to driver-private information.
 */
struct drm_master {

	struct kref refcount; /* refcount for this master */

	struct list_head head; /**< each minor contains a list of masters */
	struct drm_minor *minor; /**< link back to minor we are a master for */

	char *unique;			/**< Unique identifier: e.g., busid */
	int unique_len;			/**< Length of unique field */
	int unique_size;		/**< amount allocated */

	int blocked;			/**< Blocked due to VC switch? */

	struct drm_open_hash magiclist;
	struct list_head magicfree;
	struct drm_lock_data lock;
	void *driver_priv;
};

/* Size of ringbuffer for vblank timestamps. Just double-buffer
 * in initial implementation.
 */
#define DRM_VBLANKTIME_RBSIZE 2

/* Flags and return codes for get_vblank_timestamp() driver function. */
#define DRM_CALLED_FROM_VBLIRQ 1
#define DRM_VBLANKTIME_SCANOUTPOS_METHOD (1 << 0)
#define DRM_VBLANKTIME_IN_VBLANK         (1 << 1)

/* get_scanout_position() return flags */
#define DRM_SCANOUTPOS_VALID        (1 << 0)
#define DRM_SCANOUTPOS_IN_VBLANK    (1 << 1)
#define DRM_SCANOUTPOS_ACCURATE     (1 << 2)

#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : (1ULL<<(n)) - 1)
#endif

#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))

/**
 * DRM driver structure. This structure represent the common code for
 * a family of cards. There will one drm_device for each card present
 * in this family
 */
struct drm_driver {
	int	(*load)(struct drm_device *, unsigned long flags);
	int	(*use_msi)(struct drm_device *, unsigned long flags);
	int	(*firstopen)(struct drm_device *);
	int	(*open)(struct drm_device *, struct drm_file *);
	void	(*preclose)(struct drm_device *, struct drm_file *file_priv);
	void	(*postclose)(struct drm_device *, struct drm_file *);
	void	(*lastclose)(struct drm_device *);
	int	(*unload)(struct drm_device *);
	void	(*reclaim_buffers_locked)(struct drm_device *,
					  struct drm_file *file_priv);
	int	(*dma_ioctl)(struct drm_device *dev, void *data,
			     struct drm_file *file_priv);
	void	(*dma_ready)(struct drm_device *);
	int	(*dma_quiescent)(struct drm_device *);
	int	(*dma_flush_block_and_flush)(struct drm_device *, int context,
					     enum drm_lock_flags flags);
	int	(*dma_flush_unblock)(struct drm_device *, int context,
				     enum drm_lock_flags flags);
	int	(*context_ctor)(struct drm_device *dev, int context);
	int	(*context_dtor)(struct drm_device *dev, int context);
	int	(*kernel_context_switch)(struct drm_device *dev, int old,
					 int new);
	int	(*kernel_context_switch_unlock)(struct drm_device *dev);
	void	(*irq_preinstall)(struct drm_device *dev);
	int	(*irq_postinstall)(struct drm_device *dev);
	void	(*irq_uninstall)(struct drm_device *dev);
	void	(*irq_handler)(void *arg);

	u32	(*get_vblank_counter)(struct drm_device *dev, int crtc);
	int	(*enable_vblank)(struct drm_device *dev, int crtc);
	void	(*disable_vblank)(struct drm_device *dev, int crtc);

	/**
	 * Called by \c drm_device_is_agp.  Typically used to determine if a
	 * card is really attached to AGP or not.
	 *
	 * \param dev  DRM device handle
	 *
	 * \returns
	 * One of three values is returned depending on whether or not the
	 * card is absolutely \b not AGP (return of 0), absolutely \b is AGP
	 * (return of 1), or may or may not be AGP (return of 2).
	 */
	int (*device_is_agp) (struct drm_device *dev);

	/**
	 * Called by vblank timestamping code.
	 *
	 * Return the current display scanout position from a crtc, and an
	 * optional accurate ktime_get timestamp of when position was measured.
	 *
	 * \param dev  DRM device.
	 * \param crtc Id of the crtc to query.
	 * \param flags Flags from the caller (DRM_CALLED_FROM_VBLIRQ or 0).
	 * \param *vpos Target location for current vertical scanout position.
	 * \param *hpos Target location for current horizontal scanout position.
	 * \param *stime Target location for timestamp taken immediately before
	 *               scanout position query. Can be NULL to skip timestamp.
	 * \param *etime Target location for timestamp taken immediately after
	 *               scanout position query. Can be NULL to skip timestamp.
	 *
	 * Returns vpos as a positive number while in active scanout area.
	 * Returns vpos as a negative number inside vblank, counting the number
	 * of scanlines to go until end of vblank, e.g., -1 means "one scanline
	 * until start of active scanout / end of vblank."
	 *
	 * \return Flags, or'ed together as follows:
	 *
	 * DRM_SCANOUTPOS_VALID = Query successful.
	 * DRM_SCANOUTPOS_INVBL = Inside vblank.
	 * DRM_SCANOUTPOS_ACCURATE = Returned position is accurate. A lack of
	 * this flag means that returned position may be offset by a constant
	 * but unknown small number of scanlines wrt. real scanout position.
	 *
	 */
	int (*get_scanout_position) (struct drm_device *dev, int crtc,
				     unsigned int flags,
				     int *vpos, int *hpos, ktime_t *stime,
				     ktime_t *etime);

	int	(*get_vblank_timestamp)(struct drm_device *dev, int crtc,
		    int *max_error, struct timeval *vblank_time,
		    unsigned flags);

	void	(*gem_free_object)(struct drm_gem_object *obj);
	int	(*gem_open_object)(struct drm_gem_object *, struct drm_file *);
	void	(*gem_close_object)(struct drm_gem_object *, struct drm_file *);

	struct cdev_pager_ops *gem_pager_ops;

	int	(*dumb_create)(struct drm_file *file_priv,
		    struct drm_device *dev, struct drm_mode_create_dumb *args);
	int	(*dumb_map_offset)(struct drm_file *file_priv,
		    struct drm_device *dev, uint32_t handle, uint64_t *offset);
	int	(*dumb_destroy)(struct drm_file *file_priv,
		    struct drm_device *dev, uint32_t handle);

	int	(*sysctl_init)(struct drm_device *dev,
		    struct sysctl_ctx_list *ctx, struct sysctl_oid *top);
	void	(*sysctl_cleanup)(struct drm_device *dev);

	drm_pci_id_list_t *id_entry;	/* PCI ID, name, and chipset private */

	int	major;
	int	minor;
	int	patchlevel;
	const char *name;		/* Simple driver name		   */
	const char *desc;		/* Longer driver name		   */
	const char *date;		/* Date of last major changes.	   */

	u32 driver_features;
	int dev_priv_size;
	const struct drm_ioctl_desc *ioctls;
	int num_ioctls;
};

/**
 * Info file list entry. This structure represents a debugfs or proc file to
 * be created by the drm core
 */
struct drm_info_list {
	const char *name; /** file name */
	int (*show)(struct seq_file*, void*); /** show callback */
	u32 driver_features; /**< Required driver features for this entry */
	void *data;
};

/**
 * debugfs node structure. This structure represents a debugfs file.
 */
struct drm_info_node {
	struct list_head list;
	struct drm_minor *minor;
	const struct drm_info_list *info_ent;
	struct dentry *dent;
};

/**
 * DRM minor structure. This structure represents a drm minor number.
 */
struct drm_minor {
	int index;			/**< Minor device number */
	int type;                       /**< Control or render */
	device_t kdev;			/**< OS device */
	struct drm_device *dev;

	struct drm_master *master; /* currently active master for this node */
	struct list_head master_list;
	struct drm_mode_group mode_group;
};

struct drm_pending_vblank_event {
	struct drm_pending_event base;
	unsigned int pipe;
	struct drm_event_vblank event;
};

struct drm_sysctl_info {
	struct sysctl_ctx_list ctx;
	char   name[2];
};

/* Length for the array of resource pointers for drm_get_resource_*. */
#define DRM_MAX_PCI_RESOURCE	6

struct drm_vblank_crtc {
	struct drm_device *dev;		/* pointer to the drm_device */
	wait_queue_head_t queue;	/**< VBLANK wait queue */
	struct timer_list disable_timer;		/* delayed disable timer */

	/* vblank counter, protected by dev->vblank_time_lock for writes */
	unsigned long count;
	/* vblank timestamps, protected by dev->vblank_time_lock for writes */
	struct timeval time[DRM_VBLANKTIME_RBSIZE];

	atomic_t refcount;		/* number of users of vblank interruptsper crtc */
	u32 last;			/* protected by dev->vbl_lock, used */
					/* for wraparound handling */
	u32 last_wait;			/* Last vblank seqno waited per CRTC */
	unsigned int inmodeset;		/* Display driver is setting mode */
	unsigned int pipe;		/* crtc index */
	bool enabled;			/* so we don't call enable more than
					   once per disable */
};

/**
 * DRM device structure. This structure represent a complete card that
 * may contain multiple heads.
 */
struct drm_device {
	drm_pci_id_list_t *id_entry;	/* PCI ID, name, and chipset private */

	char		  *unique;	/* Unique identifier: e.g., busid  */
	int		  unique_len;	/* Length of unique field	   */
	struct cdev	  *devnode;	/* Device number for mknod	   */
	int		  if_version;	/* Highest interface version set */

	int		  flags;	/* Flags to open(2)		   */

				/* Locks */
	struct spinlock	  dma_lock;	/* protects dev->dma */
	struct lwkt_serialize irq_lock;	/* protects irq condition checks */
	struct lock	  dev_lock;	/* protects everything else */

	/** \name Locks */
	/*@{ */
	struct lock struct_mutex;	/**< For others */
	struct lock master_mutex;	/**< For drm_minor::master */
	/*@} */

	/** \name Usage Counters */
	/*@{ */
	int open_count;			/**< Outstanding files open, protected by drm_global_mutex. */
	struct spinlock buf_lock;	/**< For drm_device::buf_use and a few other things. */
	int buf_use;			/**< Buffers in use -- cannot alloc */
	atomic_t buf_alloc;		/**< Buffer allocation in progress */
	/*@} */


	/** \name Performance counters */
	/*@{ */
	unsigned long     counters;
	enum drm_stat_type	types[15];
	atomic_t          counts[15];
	/*@} */

				/* Authentication */
	struct drm_open_hash magiclist;	/**< magic hash table */
	struct list_head magicfree;

	struct list_head filelist;

	/** \name Memory management */
	/*@{ */
	struct list_head maplist;	/**< Linked list of regions */
	int map_count;			/**< Number of mappable regions */
	struct drm_open_hash map_hash;	/**< User token hash table for maps */

	/** \name Context handle management */
	/*@{ */
	struct list_head ctxlist;	/**< Linked list of context handles */
	int ctx_count;			/**< Number of context handles */
	struct lock ctxlist_mutex;	/**< For ctxlist */

	struct idr ctx_idr;

	/*@} */

	struct drm_lock_data lock;	/* Information on hardware lock	   */

	/** \name DMA support */
	/*@{ */
	struct drm_device_dma *dma;		/**< Optional pointer for DMA support */
	/*@} */

	int		  irq;		/* Interrupt used by board	   */
	int		  irq_type;	/* IRQ type (MSI enabled or not) */
	int		  irqrid;	/* Interrupt used by board */
	struct resource   *irqr;	/* Resource for interrupt used by board	   */
	void		  *irqh;	/* Handle from bus_setup_intr      */

	/* Storage of resource pointers for drm_get_resource_* */
	struct resource   *pcir[DRM_MAX_PCI_RESOURCE];
	int		  pcirid[DRM_MAX_PCI_RESOURCE];

	int		  pci_domain;
	int		  pci_bus;
	int		  pci_slot;
	int		  pci_func;

	/** \name Context support */
	/*@{ */
	int irq_enabled;		/**< True if irq handler is enabled */
	__volatile__ long context_flag;	/**< Context swapping flag */
	__volatile__ long interrupt_flag; /**< Interruption handler flag */
	__volatile__ long dma_flag;	/**< DMA dispatch flag */
	wait_queue_head_t context_wait;	/**< Processes waiting on ctx switch */
	int last_checked;		/**< Last context checked for DMA */
	int last_context;		/**< Last current context */
	unsigned long last_switch;	/**< jiffies at last context switch */
	/*@} */

	/** \name VBLANK IRQ support */
	/*@{ */

	/*
	 * At load time, disabling the vblank interrupt won't be allowed since
	 * old clients may not call the modeset ioctl and therefore misbehave.
	 * Once the modeset ioctl *has* been called though, we can safely
	 * disable them when unused.
	 */
	bool vblank_disable_allowed;

	/*
	 * If true, vblank interrupt will be disabled immediately when the
	 * refcount drops to zero, as opposed to via the vblank disable
	 * timer.
	 * This can be set to true it the hardware has a working vblank
	 * counter and the driver uses drm_vblank_on() and drm_vblank_off()
	 * appropriately.
	 */
	bool vblank_disable_immediate;

	/* array of size num_crtcs */
	struct drm_vblank_crtc *vblank;

	struct lock vblank_time_lock;    /**< Protects vblank count and time updates during vblank enable/disable */
	struct lock vbl_lock;
	struct timer_list vblank_disable_timer;

	u32 max_vblank_count;           /**< size of vblank counter register */

	/**
	 * List of events
	 */
	struct list_head vblank_event_list;
	struct lock event_lock;

	/*@} */

	struct sigio      *buf_sigio;	/* Processes waiting for SIGIO     */

				/* Sysctl support */
	struct drm_sysctl_info *sysctl;


	struct drm_sg_mem *sg;	/**< Scatter gather memory */
	unsigned int num_crtcs;                  /**< Number of CRTCs on this device */

	unsigned long     *ctx_bitmap;
	void		  *dev_private;

	void		  *drm_ttm_bdev;

	/*@} */

	struct drm_agp_head *agp;	/**< AGP data */

	struct device *dev;             /**< Device structure */
	struct pci_dev *pdev;		/**< PCI device structure */

	struct drm_driver *driver;
	struct drm_local_map *agp_buffer_map;
	unsigned int agp_buffer_token;
	struct drm_minor *control;		/**< Control node for card */
	struct drm_minor *primary;		/**< render type primary screen head */

        struct drm_mode_config mode_config;	/**< Current mode config */

	/** \name GEM information */
	/*@{ */
	struct lock object_name_lock;
	struct idr object_name_idr;
	/*@} */
	void             *mm_private;

	void *sysctl_private;
	char busid_str[128];
	int modesetting;

	int switch_power_state;

	atomic_t unplugged; /* device has been unplugged or gone away */
};

#define DRM_SWITCH_POWER_ON 0
#define DRM_SWITCH_POWER_OFF 1
#define DRM_SWITCH_POWER_CHANGING 2
#define DRM_SWITCH_POWER_DYNAMIC_OFF 3

static __inline__ int drm_core_check_feature(struct drm_device *dev,
					     int feature)
{
	return ((dev->driver->driver_features & feature) ? 1 : 0);
}

static inline void drm_device_set_unplugged(struct drm_device *dev)
{
	smp_wmb();
	atomic_set(&dev->unplugged, 1);
}

static inline int drm_device_is_unplugged(struct drm_device *dev)
{
	int ret = atomic_read(&dev->unplugged);
	smp_rmb();
	return ret;
}

static inline bool drm_is_primary_client(const struct drm_file *file_priv)
{
	return 0 /* file_priv->minor->type == DRM_MINOR_LEGACY */;
}

/**
 * drm_is_master() - Check whether a DRM open-file is DRM-Master
 * @file: DRM open-file context
 *
 * This checks whether a DRM open-file context is owner of the master context
 * attached to it. If a file owns a master context, it's called DRM-Master.
 * Per DRM device, only one such file can be DRM-Master at a time.
 *
 * Returns: True if the file is DRM-Master, otherwise false.
 */
static inline bool drm_is_master(const struct drm_file *file)
{
#if 0
	return file->master && file->master == file->minor->master;
#else
	return true;
#endif
}

/******************************************************************/
/** \name Internal function definitions */
/*@{*/

#if __OS_HAS_AGP
static inline int drm_core_has_AGP(struct drm_device *dev)
{
	return drm_core_check_feature(dev, DRIVER_USE_AGP);
}
#else
#define drm_core_has_AGP(dev) (0)
#endif

extern unsigned int drm_rnodes;
extern unsigned int drm_universal_planes;

extern int drm_vblank_offdelay;
extern unsigned int drm_timestamp_precision;

				/* Driver support (drm_drv.h) */
extern int drm_ioctl_permit(u32 flags, struct drm_file *file_priv);
int	drm_probe(device_t kdev, drm_pci_id_list_t *idlist);
int	drm_attach(device_t kdev, drm_pci_id_list_t *idlist);
int	drm_create_cdevs(device_t kdev);
d_ioctl_t drm_ioctl;
extern bool drm_ioctl_flags(unsigned int nr, unsigned int *flags);

				/* Device support (drm_fops.h) */
d_open_t drm_open;
d_close_t drm_close;
d_read_t drm_read;
d_kqfilter_t drm_kqfilter;
int drm_release(device_t kdev);

d_mmap_t drm_mmap;
d_mmap_single_t drm_mmap_single;

void drm_cdevpriv_dtor(void *cd);

int drm_add_busid_modesetting(struct drm_device *dev,
    struct sysctl_ctx_list *ctx, struct sysctl_oid *top);

/* File operations helpers (drm_fops.c) */
extern int		drm_open_helper(struct cdev *kdev, int flags, int fmt,
					 DRM_STRUCTPROC *p,
					struct drm_device *dev,
					struct file *fp);
extern struct drm_file	*drm_find_file_by_proc(struct drm_device *dev,
					DRM_STRUCTPROC *p);

#ifdef DUMBBELL_WIP
extern int drm_gem_prime_handle_to_fd(struct drm_device *dev,
		struct drm_file *file_priv, uint32_t handle, uint32_t flags,
		int *prime_fd);
extern int drm_gem_prime_fd_to_handle(struct drm_device *dev,
		struct drm_file *file_priv, int prime_fd, uint32_t *handle);

extern int drm_prime_handle_to_fd_ioctl(struct drm_device *dev, void *data,
					struct drm_file *file_priv);
extern int drm_prime_fd_to_handle_ioctl(struct drm_device *dev, void *data,
					struct drm_file *file_priv);

extern int drm_prime_sg_to_page_addr_arrays(struct sg_table *sgt, vm_page_t *pages,
					    dma_addr_t *addrs, int max_pages);
#endif /* DUMBBELL_WIP */
extern struct sg_table *drm_prime_pages_to_sg(vm_page_t *pages, int nr_pages);
extern void drm_prime_gem_destroy(struct drm_gem_object *obj, struct sg_table *sg);

#ifdef DUMBBELL_WIP
void drm_prime_init_file_private(struct drm_prime_file_private *prime_fpriv);
void drm_prime_destroy_file_private(struct drm_prime_file_private *prime_fpriv);
int drm_prime_add_imported_buf_handle(struct drm_prime_file_private *prime_fpriv, struct dma_buf *dma_buf, uint32_t handle);
int drm_prime_lookup_imported_buf_handle(struct drm_prime_file_private *prime_fpriv, struct dma_buf *dma_buf, uint32_t *handle);
void drm_prime_remove_imported_buf_handle(struct drm_prime_file_private *prime_fpriv, struct dma_buf *dma_buf);

int drm_prime_add_dma_buf(struct drm_device *dev, struct drm_gem_object *obj);
int drm_prime_lookup_obj(struct drm_device *dev, struct dma_buf *buf,
			 struct drm_gem_object **obj);
#endif /* DUMBBELL_WIP */

int	drm_mtrr_add(unsigned long offset, size_t size, int flags);
int	drm_mtrr_del(int handle, unsigned long offset, size_t size, int flags);

/*
 * These are exported to drivers so that they can implement fencing using
 * DMA quiscent + idle. DMA quiescent usually requires the hardware lock.
 */

unsigned long drm_get_resource_start(struct drm_device *dev,
				     unsigned int resource);
unsigned long drm_get_resource_len(struct drm_device *dev,
				   unsigned int resource);

				/* IRQ support (drm_irq.h) */
extern int drm_irq_install(struct drm_device *dev, int irq);
extern int drm_irq_uninstall(struct drm_device *dev);

extern int drm_vblank_init(struct drm_device *dev, unsigned int num_crtcs);
extern int drm_wait_vblank(struct drm_device *dev, void *data,
			   struct drm_file *filp);
extern u32 drm_vblank_count(struct drm_device *dev, int pipe);
extern u32 drm_crtc_vblank_count(struct drm_crtc *crtc);
extern u32 drm_vblank_count_and_time(struct drm_device *dev, unsigned int pipe,
				     struct timeval *vblanktime);
extern void drm_send_vblank_event(struct drm_device *dev, unsigned int pipe,
				  struct drm_pending_vblank_event *e);
extern void drm_crtc_send_vblank_event(struct drm_crtc *crtc,
				       struct drm_pending_vblank_event *e);
extern bool drm_handle_vblank(struct drm_device *dev, unsigned int pipe);
extern bool drm_crtc_handle_vblank(struct drm_crtc *crtc);
extern int drm_vblank_get(struct drm_device *dev, unsigned int pipe);
extern void drm_vblank_put(struct drm_device *dev, unsigned int pipe);
extern int drm_crtc_vblank_get(struct drm_crtc *crtc);
extern void drm_crtc_vblank_put(struct drm_crtc *crtc);
extern void drm_wait_one_vblank(struct drm_device *dev, unsigned int pipe);
extern void drm_crtc_wait_one_vblank(struct drm_crtc *crtc);
extern void drm_vblank_off(struct drm_device *dev, unsigned int pipe);
extern void drm_vblank_on(struct drm_device *dev, unsigned int pipe);
extern void drm_crtc_vblank_off(struct drm_crtc *crtc);
extern void drm_crtc_vblank_reset(struct drm_crtc *crtc);
extern void drm_crtc_vblank_on(struct drm_crtc *crtc);
extern void drm_vblank_cleanup(struct drm_device *dev);

extern int drm_calc_vbltimestamp_from_scanoutpos(struct drm_device *dev,
						 unsigned int pipe, int *max_error,
						 struct timeval *vblank_time,
						 unsigned flags,
						 const struct drm_crtc *refcrtc,
						 const struct drm_display_mode *mode);
extern void drm_calc_timestamping_constants(struct drm_crtc *crtc,
					    const struct drm_display_mode *mode);

/**
 * drm_crtc_vblank_waitqueue - get vblank waitqueue for the CRTC
 * @crtc: which CRTC's vblank waitqueue to retrieve
 *
 * This function returns a pointer to the vblank waitqueue for the CRTC.
 * Drivers can use this to implement vblank waits using wait_event() & co.
 */
static inline wait_queue_head_t *drm_crtc_vblank_waitqueue(struct drm_crtc *crtc)
{
       return &crtc->dev->vblank[drm_crtc_index(crtc)].queue;
}

/* Modesetting support */
extern void drm_vblank_pre_modeset(struct drm_device *dev, unsigned int pipe);
extern void drm_vblank_post_modeset(struct drm_device *dev, unsigned int pipe);

/* sysctl support (drm_sysctl.h) */
extern int		drm_sysctl_init(struct drm_device *dev);
extern int		drm_sysctl_cleanup(struct drm_device *dev);

/* Locking IOCTL support (drm_drv.c) */
int	drm_lock(struct drm_device *dev, void *data,
		 struct drm_file *file_priv);
int	drm_unlock(struct drm_device *dev, void *data,
		   struct drm_file *file_priv);
int	drm_setversion(struct drm_device *dev, void *data,
		       struct drm_file *file_priv);

/*
 * These are exported to drivers so that they can implement fencing using
 * DMA quiscent + idle. DMA quiescent usually requires the hardware lock.
 */

extern int drm_i_have_hw_lock(struct drm_device *dev, struct drm_file *file_priv);

/* Misc. IOCTL support (drm_ioctl.c) */
int	drm_noop(struct drm_device *dev, void *data,
		 struct drm_file *file_priv);

/* Cache management (drm_cache.c) */
void drm_clflush_pages(struct vm_page *pages[], unsigned long num_pages);
void drm_clflush_sg(struct sg_table *st);
void drm_clflush_virt_range(void *addr, unsigned long length);

/* DMA support (drm_dma.c) */
int	drm_dma(struct drm_device *dev, void *data, struct drm_file *file_priv);

				/* Stub support (drm_stub.h) */

extern void drm_put_dev(struct drm_device *dev);
extern void drm_unplug_dev(struct drm_device *dev);
extern unsigned int drm_debug;
extern bool drm_atomic;

/* Scatter Gather Support (drm_scatter.c) */
int	drm_sg_alloc_ioctl(struct drm_device *dev, void *data,
			   struct drm_file *file_priv);
int	drm_sg_free(struct drm_device *dev, void *data,
		    struct drm_file *file_priv);

/* consistent PCI memory functions (drm_pci.c) */
extern struct drm_dma_handle *drm_pci_alloc(struct drm_device *dev, size_t size,
					    size_t align);
void	drm_pci_free(struct drm_device *dev, struct drm_dma_handle *dmah);

			       /* sysfs support (drm_sysfs.c) */
extern void drm_sysfs_hotplug_event(struct drm_device *dev);

int drm_gem_mmap_single(struct drm_device *dev, vm_ooffset_t *offset,
    vm_size_t size, struct vm_object **obj_res, int nprot);
void drm_gem_pager_dtr(void *obj);

struct ttm_bo_device;
int ttm_bo_mmap_single(struct ttm_bo_device *bdev, vm_ooffset_t *offset,
    vm_size_t size, struct vm_object **obj_res, int nprot);
struct ttm_buffer_object;
void ttm_bo_release_mmap(struct ttm_buffer_object *bo);

void drm_device_lock_mtx(struct drm_device *dev);
void drm_device_unlock_mtx(struct drm_device *dev);
int drm_device_sleep_mtx(struct drm_device *dev, void *chan, int flags,
    const char *msg, int timeout);
void drm_device_assert_mtx_locked(struct drm_device *dev);
void drm_device_assert_mtx_unlocked(struct drm_device *dev);

void drm_device_lock_struct(struct drm_device *dev);
void drm_device_unlock_struct(struct drm_device *dev);
int drm_device_sleep_struct(struct drm_device *dev, void *chan, int flags,
    const char *msg, int timeout);
void drm_device_assert_struct_locked(struct drm_device *dev);
void drm_device_assert_struct_unlocked(struct drm_device *dev);

void drm_compat_locking_init(struct drm_device *dev);
void drm_sleep_locking_init(struct drm_device *dev);

/* XXX glue logic, should be done in drm_pci_init(), pending drm update */
void drm_init_pdev(struct device *dev, struct pci_dev **pdev);
void drm_fini_pdev(struct pci_dev **pdev);

/* Inline replacements for drm_alloc and friends */
static __inline__ void *
drm_alloc(size_t size, struct malloc_type *area)
{
	return kmalloc(size, area, M_WAITOK | M_NULLOK);
}

static __inline__ void *
drm_calloc(size_t nmemb, size_t size, struct malloc_type *area)
{
	return kmalloc(size * nmemb, area, M_WAITOK | M_NULLOK | M_ZERO);
}

static __inline__ void
drm_free(void *pt, struct malloc_type *area)
{
	/* kfree is special!!! */
	if (pt != NULL)
		(kfree)(pt, area);
}

struct drm_device *drm_dev_alloc(struct drm_driver *driver,
				 struct device *parent);
void drm_dev_ref(struct drm_device *dev);
void drm_dev_unref(struct drm_device *dev);
int drm_dev_register(struct drm_device *dev, unsigned long flags);
void drm_dev_unregister(struct drm_device *dev);
int drm_dev_set_unique(struct drm_device *dev, const char *fmt, ...);

/* FreeBSD compatibility macros */
#define VM_OBJECT_WLOCK(object)		VM_OBJECT_LOCK(object)
#define VM_OBJECT_WUNLOCK(object)	VM_OBJECT_UNLOCK(object)

/* PCI section */
static __inline__ int drm_pci_device_is_agp(struct drm_device *dev)
{
	if (dev->driver->device_is_agp != NULL) {
		int err = (*dev->driver->device_is_agp) (dev);

		if (err != 2) {
			return err;
		}
	}

	return (pci_find_extcap(dev->dev, PCIY_AGP, NULL) == 0);
}

#define DRM_PCIE_SPEED_25 1
#define DRM_PCIE_SPEED_50 2
#define DRM_PCIE_SPEED_80 4

extern int drm_pcie_get_speed_cap_mask(struct drm_device *dev, u32 *speed_mask);

/* XXX bad */
#define	drm_can_sleep()	(HZ & 1)

#endif /* __KERNEL__ */
#endif /* _DRM_P_H_ */
