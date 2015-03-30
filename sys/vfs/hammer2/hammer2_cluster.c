/*
 * Copyright (c) 2013-2015 The DragonFly Project.  All rights reserved.
 *
 * This code is derived from software contributed to The DragonFly Project
 * by Matthew Dillon <dillon@dragonflybsd.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of The DragonFly Project nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific, prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * The cluster module collects multiple chains representing the same
 * information from different nodes into a single entity.  It allows direct
 * access to media data as long as it is not blockref array data (which
 * will obviously have to be different at each node).
 *
 * This module also handles I/O dispatch, status rollup, and various
 * mastership arrangements including quorum operations.  It effectively
 * presents one topology to the vnops layer.
 *
 * Many of the API calls mimic chain API calls but operate on clusters
 * instead of chains.  Please see hammer2_chain.c for more complete code
 * documentation of the API functions.
 *
 * WARNING! This module is *extremely* complex.  It must issue asynchronous
 *	    locks and I/O, do quorum and/or master-slave processing, and
 *	    it must operate properly even if some nodes are broken (which
 *	    can also mean indefinite locks).
 *
 *				CLUSTER OPERATIONS
 *
 * Cluster operations can be broken down into three pieces:
 *
 * (1) Chain locking and data retrieval.
 *		hammer2_cluster_lock()
 *		hammer2_cluster_parent()
 *
 *	- Most complex functions, quorum management on transaction ids.
 *
 *	- Locking and data accesses must be internally asynchronous.
 *
 *	- Validate and manage cache coherency primitives (cache state
 *	  is stored in chain topologies but must be validated by these
 *	  functions).
 *
 * (2) Lookups and Scans
 *		hammer2_cluster_lookup()
 *		hammer2_cluster_next()
 *
 *	- Depend on locking & data retrieval functions, but still complex.
 *
 *	- Must do quorum management on transaction ids.
 *
 *	- Lookup and Iteration ops Must be internally asynchronous.
 *
 * (3) Modifying Operations
 *		hammer2_cluster_create()
 *		hammer2_cluster_rename()
 *		hammer2_cluster_delete()
 *		hammer2_cluster_modify()
 *		hammer2_cluster_modsync()
 *
 *	- Can usually punt on failures, operation continues unless quorum
 *	  is lost.  If quorum is lost, must wait for resynchronization
 *	  (depending on the management mode).
 *
 *	- Must disconnect node on failures (also not flush), remount, and
 *	  resynchronize.
 *
 *	- Network links (via kdmsg) are relatively easy to issue as the
 *	  complex underworkings of hammer2_chain.c don't have to messed
 *	  with (the protocol is at a higher level than block-level).
 *
 *	- Multiple local disk nodes (i.e. block devices) are another matter.
 *	  Chain operations have to be dispatched to per-node threads (xN)
 *	  because we can't asynchronize potentially very complex chain
 *	  operations in hammer2_chain.c (it would be a huge mess).
 *
 *	  (these threads are also used to terminate incoming kdmsg ops from
 *	  other machines).
 *
 *	- Single-node filesystems do not use threads and will simply call
 *	  hammer2_chain.c functions directly.  This short-cut is handled
 *	  at the base of each cluster function.
 */
#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/lock.h>
#include <sys/uuid.h>

#include "hammer2.h"

/*
 * Returns TRUE if any chain in the cluster needs to be resized.
 */
int
hammer2_cluster_need_resize(hammer2_cluster_t *cluster, int bytes)
{
	hammer2_chain_t *chain;
	int i;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain && chain->bytes != bytes)
			return 1;
	}
	return 0;
}

uint8_t
hammer2_cluster_type(hammer2_cluster_t *cluster)
{
	return(cluster->focus->bref.type);
}

int
hammer2_cluster_modified(hammer2_cluster_t *cluster)
{
	return((cluster->focus->flags & HAMMER2_CHAIN_MODIFIED) != 0);
}

/*
 * Return a bref representative of the cluster.  Any data offset is removed
 * (since it would only be applicable to a particular chain in the cluster).
 *
 * However, the radix portion of data_off is used for many purposes and will
 * be retained.
 */
void
hammer2_cluster_bref(hammer2_cluster_t *cluster, hammer2_blockref_t *bref)
{
	*bref = cluster->focus->bref;
	bref->data_off &= HAMMER2_OFF_MASK_RADIX;
}

/*
 * Return non-zero if the chain representing an inode has been flagged
 * as having been unlinked.  Allows the vnode reclaim to avoid loading
 * the inode data from disk e.g. when unmount or recycling old, clean
 * vnodes.
 */
int
hammer2_cluster_isunlinked(hammer2_cluster_t *cluster)
{
	hammer2_chain_t *chain;
	int flags;
	int i;

	flags = 0;
	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain)
			flags |= chain->flags;
	}
	return (flags & HAMMER2_CHAIN_UNLINKED);
}

void
hammer2_cluster_set_chainflags(hammer2_cluster_t *cluster, uint32_t flags)
{
	hammer2_chain_t *chain;
	int i;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain)
			atomic_set_int(&chain->flags, flags);
	}
}

void
hammer2_cluster_clr_chainflags(hammer2_cluster_t *cluster, uint32_t flags)
{
	hammer2_chain_t *chain;
	int i;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain)
			atomic_clear_int(&chain->flags, flags);
	}
}

void
hammer2_cluster_setflush(hammer2_trans_t *trans, hammer2_cluster_t *cluster)
{
	hammer2_chain_t *chain;
	int i;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain)
			hammer2_chain_setflush(trans, chain);
	}
}

void
hammer2_cluster_setmethod_check(hammer2_trans_t *trans,
				hammer2_cluster_t *cluster,
				int check_algo)
{
	hammer2_chain_t *chain;
	int i;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			KKASSERT(chain->flags & HAMMER2_CHAIN_MODIFIED);
			chain->bref.methods &= ~HAMMER2_ENC_CHECK(-1);
			chain->bref.methods |= HAMMER2_ENC_CHECK(check_algo);
		}
	}
}

/*
 * Create a cluster with one ref from the specified chain.  The chain
 * is not further referenced.  The caller typically supplies a locked
 * chain and transfers ownership to the cluster.
 *
 * The returned cluster will be focused on the chain (strictly speaking,
 * the focus should be NULL if the chain is not locked but we do not check
 * for this condition).
 */
hammer2_cluster_t *
hammer2_cluster_from_chain(hammer2_chain_t *chain)
{
	hammer2_cluster_t *cluster;

	cluster = kmalloc(sizeof(*cluster), M_HAMMER2, M_WAITOK | M_ZERO);
	cluster->array[0].chain = chain;
	cluster->nchains = 1;
	cluster->focus = chain;
	cluster->pmp = chain->pmp;
	cluster->refs = 1;
	cluster->flags = HAMMER2_CLUSTER_LOCKED;

	return cluster;
}

#if 0
/*
 * Allocates a cluster and its underlying chain structures.  The underlying
 * chains will be locked.  The cluster and underlying chains will have one
 * ref and will be focused on the first chain.
 *
 * XXX focus on first chain.
 */
hammer2_cluster_t *
hammer2_cluster_alloc(hammer2_pfs_t *pmp,
		      hammer2_trans_t *trans, hammer2_blockref_t *bref)
{
	hammer2_cluster_t *cluster;
	hammer2_cluster_t *rcluster;
	hammer2_chain_t *chain;
	hammer2_chain_t *rchain;
#if 0
	u_int bytes = 1U << (int)(bref->data_off & HAMMER2_OFF_MASK_RADIX);
#endif
	int i;

	KKASSERT(pmp != NULL);

	/*
	 * Construct the appropriate system structure.
	 */
	switch(bref->type) {
	case HAMMER2_BREF_TYPE_INODE:
	case HAMMER2_BREF_TYPE_INDIRECT:
	case HAMMER2_BREF_TYPE_FREEMAP_NODE:
	case HAMMER2_BREF_TYPE_DATA:
	case HAMMER2_BREF_TYPE_FREEMAP_LEAF:
		/*
		 * Chain's are really only associated with the hmp but we
		 * maintain a pmp association for per-mount memory tracking
		 * purposes.  The pmp can be NULL.
		 */
		break;
	case HAMMER2_BREF_TYPE_VOLUME:
	case HAMMER2_BREF_TYPE_FREEMAP:
		chain = NULL;
		panic("hammer2_cluster_alloc volume type illegal for op");
	default:
		chain = NULL;
		panic("hammer2_cluster_alloc: unrecognized blockref type: %d",
		      bref->type);
	}

	cluster = kmalloc(sizeof(*cluster), M_HAMMER2, M_WAITOK | M_ZERO);
	cluster->refs = 1;
	cluster->flags = HAMMER2_CLUSTER_LOCKED;

	rcluster = &pmp->iroot->cluster;
	for (i = 0; i < rcluster->nchains; ++i) {
		rchain = rcluster->array[i].chain;
		chain = hammer2_chain_alloc(rchain->hmp, pmp, trans, bref);
#if 0
		chain->hmp = rchain->hmp;
		chain->bref = *bref;
		chain->bytes = bytes;
		chain->refs = 1;
		chain->flags |= HAMMER2_CHAIN_ALLOCATED;
#endif

		/*
		 * NOTE: When loading a chain from backing store or creating a
		 *	 snapshot, trans will be NULL and the caller is
		 *	 responsible for setting these fields.
		 */
		cluster->array[i].chain = chain;
	}
	cluster->nchains = i;
	cluster->pmp = pmp;
	cluster->focus = cluster->array[0].chain;

	return (cluster);
}
#endif

/*
 * Add a reference to a cluster.
 *
 * We must also ref the underlying chains in order to allow ref/unlock
 * sequences to later re-lock.
 */
void
hammer2_cluster_ref(hammer2_cluster_t *cluster)
{
	hammer2_chain_t *chain;
	int i;

	atomic_add_int(&cluster->refs, 1);
	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain)
			hammer2_chain_ref(chain);
	}
}

/*
 * Drop the caller's reference to the cluster.  When the ref count drops to
 * zero this function frees the cluster and drops all underlying chains.
 *
 * In-progress read I/Os are typically detached from the cluster once the
 * first one returns (the remaining stay attached to the DIOs but are then
 * ignored and drop naturally).
 */
void
hammer2_cluster_drop(hammer2_cluster_t *cluster)
{
	hammer2_chain_t *chain;
	int i;

	KKASSERT(cluster->refs > 0);
	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			hammer2_chain_drop(chain);
			if (cluster->refs == 1)
				cluster->array[i].chain = NULL;
		}
	}
	if (atomic_fetchadd_int(&cluster->refs, -1) == 1) {
		cluster->focus = NULL;		/* safety */
		kfree(cluster, M_HAMMER2);
		/* cluster is invalid */
	}
}

void
hammer2_cluster_wait(hammer2_cluster_t *cluster)
{
	tsleep(cluster->focus, 0, "h2clcw", 1);
}

/*
 * Lock and ref a cluster.  This adds a ref to the cluster and its chains
 * and then locks them.
 *
 * The act of locking a cluster sets its focus if not already set.
 *
 * The chains making up the cluster may be narrowed down based on quorum
 * acceptability, and if RESOLVE_RDONLY is specified the chains can be
 * narrowed down to a single chain as long as the entire subtopology is known
 * to be intact.  So, for example, we can narrow a read-only op to a single
 * fast SLAVE but if we focus a CACHE chain we must still retain at least
 * a SLAVE to ensure that the subtopology can be accessed.
 *
 * RESOLVE_RDONLY operations are effectively as-of so the quorum does not need
 * to be maintained once the topology is validated as-of the top level of
 * the operation.
 */
int
hammer2_cluster_lock(hammer2_cluster_t *cluster, int how)
{
	hammer2_chain_t *chain;
	hammer2_chain_t *tmp;
	int i;
	int error;

	/* cannot be on inode-embedded cluster template, must be on copy */
	KKASSERT((cluster->flags & HAMMER2_CLUSTER_INODE) == 0);
	if (cluster->flags & HAMMER2_CLUSTER_LOCKED) {
		kprintf("hammer2_cluster_lock: cluster %p already locked!\n",
			cluster);
	}
	atomic_set_int(&cluster->flags, HAMMER2_CLUSTER_LOCKED);

	if ((how & HAMMER2_RESOLVE_NOREF) == 0)
		atomic_add_int(&cluster->refs, 1);

	error = 0;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			error = hammer2_chain_lock(chain, how);
			if (error) {
				while (--i >= 0) {
					tmp = cluster->array[i].chain;
					hammer2_chain_unlock(tmp);
				}
				atomic_add_int(&cluster->refs, -1);
				break;
			}
			if (cluster->focus == NULL)
				cluster->focus = chain;
		}
	}
	return error;
}

#if 0
/*
 * Replace the contents of dst with src, adding a reference to src's chains
 * but not adding any additional locks.
 *
 * dst is assumed to already have a ref and any chains present in dst are
 * assumed to be locked and will be unlocked.
 *
 * If the chains in src are locked, only one of (src) or (dst) should be
 * considered locked by the caller after return, not both.
 */
void
hammer2_cluster_replace(hammer2_cluster_t *dst, hammer2_cluster_t *src)
{
	hammer2_chain_t *chain;
	hammer2_chain_t *tmp;
	int i;

	KKASSERT(dst->refs == 1);
	dst->focus = NULL;

	for (i = 0; i < src->nchains; ++i) {
		chain = src->array[i].chain;
		if (chain) {
			hammer2_chain_ref(chain);
			if (i < dst->nchains &&
			    (tmp = dst->array[i].chain) != NULL) {
				hammer2_chain_unlock(tmp);
			}
			dst->array[i].chain = chain;
			if (dst->focus == NULL)
				dst->focus = chain;
		}
	}
	while (i < dst->nchains) {
		chain = dst->array[i].chain;
		if (chain) {
			hammer2_chain_unlock(chain);
			dst->array[i].chain = NULL;
		}
		++i;
	}
	dst->nchains = src->nchains;
}

/*
 * Replace the contents of the locked destination with the contents of the
 * locked source.  The destination must have one ref.
 *
 * Returns with the destination still with one ref and the copied chains
 * with an additional lock (representing their state on the destination).
 * The original chains associated with the destination are unlocked.
 *
 * From the point of view of the caller, both src and dst are locked on
 * call and remain locked on return.
 *
 * XXX adjust flag state
 */
void
hammer2_cluster_replace_locked(hammer2_cluster_t *dst, hammer2_cluster_t *src)
{
	hammer2_chain_t *chain;
	hammer2_chain_t *tmp;
	int i;

	KKASSERT(dst->refs == 1);

	dst->focus = NULL;
	for (i = 0; i < src->nchains; ++i) {
		chain = src->array[i].chain;
		if (chain) {
			hammer2_chain_lock(chain, 0);
			if (i < dst->nchains &&
			    (tmp = dst->array[i].chain) != NULL) {
				hammer2_chain_unlock(tmp);
			}
			dst->array[i].chain = chain;
		}
	}
	while (i < dst->nchains) {
		chain = dst->array[i].chain;
		if (chain) {
			hammer2_chain_unlock(chain);
			dst->array[i].chain = NULL;
		}
		++i;
	}
	dst->nchains = src->nchains;
	dst->flags = src->flags;
	dst->focus = src->focus;
}
#endif

/*
 * Copy a cluster, returned a ref'd cluster.  All underlying chains
 * are also ref'd, but not locked.  The cluster focus is not set because
 * the cluster is not yet locked (and the originating cluster does not
 * have to be locked either).
 */
hammer2_cluster_t *
hammer2_cluster_copy(hammer2_cluster_t *ocluster)
{
	hammer2_pfs_t *pmp = ocluster->pmp;
	hammer2_cluster_t *ncluster;
	hammer2_chain_t *chain;
	int i;

	ncluster = kmalloc(sizeof(*ncluster), M_HAMMER2, M_WAITOK | M_ZERO);
	ncluster->pmp = pmp;
	ncluster->nchains = ocluster->nchains;
	ncluster->refs = 1;
	ncluster->flags = 0;	/* cluster not locked */

	for (i = 0; i < ocluster->nchains; ++i) {
		chain = ocluster->array[i].chain;
		ncluster->array[i].chain = chain;
		if (chain)
			hammer2_chain_ref(chain);
	}
	return (ncluster);
}

/*
 * Unlock and deref a cluster.  The cluster is destroyed if this is the
 * last ref.
 */
void
hammer2_cluster_unlock(hammer2_cluster_t *cluster)
{
	hammer2_chain_t *chain;
	int i;

	if ((cluster->flags & HAMMER2_CLUSTER_LOCKED) == 0) {
		kprintf("hammer2_cluster_unlock: cluster %p not locked\n",
			cluster);
	}
	/* KKASSERT(cluster->flags & HAMMER2_CLUSTER_LOCKED); */
	KKASSERT(cluster->refs > 0);
	atomic_clear_int(&cluster->flags, HAMMER2_CLUSTER_LOCKED);

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			hammer2_chain_unlock(chain);
			if (cluster->refs == 1)
				cluster->array[i].chain = NULL;	/* safety */
		}
	}
	if (atomic_fetchadd_int(&cluster->refs, -1) == 1) {
		cluster->focus = NULL;
		kfree(cluster, M_HAMMER2);
		/* cluster = NULL; safety */
	}
}

/*
 * Resize the cluster's physical storage allocation in-place.  This may
 * replace the cluster's chains.
 */
void
hammer2_cluster_resize(hammer2_trans_t *trans, hammer2_inode_t *ip,
		       hammer2_cluster_t *cparent, hammer2_cluster_t *cluster,
		       int nradix, int flags)
{
	hammer2_chain_t *chain;
	int i;

	KKASSERT(cparent->pmp == cluster->pmp);		/* can be NULL */
	KKASSERT(cparent->nchains == cluster->nchains);

	cluster->focus = NULL;
	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			KKASSERT(cparent->array[i].chain);
			hammer2_chain_resize(trans, ip,
					     cparent->array[i].chain, chain,
					     nradix, flags);
			if (cluster->focus == NULL)
				cluster->focus = chain;
		}
	}
}

/*
 * Set an inode's cluster modified, marking the related chains RW and
 * duplicating them if necessary.
 *
 * The passed-in chain is a localized copy of the chain previously acquired
 * when the inode was locked (and possilby replaced in the mean time), and
 * must also be updated.  In fact, we update it first and then synchronize
 * the inode's cluster cache.
 */
hammer2_inode_data_t *
hammer2_cluster_modify_ip(hammer2_trans_t *trans, hammer2_inode_t *ip,
			  hammer2_cluster_t *cluster, int flags)
{
	atomic_set_int(&ip->flags, HAMMER2_INODE_MODIFIED);
	hammer2_cluster_modify(trans, cluster, flags);

	hammer2_inode_repoint(ip, NULL, cluster);
	if (ip->vp)
		vsetisdirty(ip->vp);
	return (&hammer2_cluster_wdata(cluster)->ipdata);
}

/*
 * Adjust the cluster's chains to allow modification and adjust the
 * focus.  Data will be accessible on return.
 */
void
hammer2_cluster_modify(hammer2_trans_t *trans, hammer2_cluster_t *cluster,
		       int flags)
{
	hammer2_chain_t *chain;
	int i;

	cluster->focus = NULL;
	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			hammer2_chain_modify(trans, chain, flags);
			if (cluster->focus == NULL)
				cluster->focus = chain;
		}
	}
}

/*
 * Synchronize modifications from the focus to other chains in a cluster.
 * Convenient because nominal API users can just modify the contents of the
 * focus (at least for non-blockref data).
 *
 * Nominal front-end operations only edit non-block-table data in a single
 * chain.  This code copies such modifications to the other chains in the
 * cluster.  Blocktable modifications are handled on a chain-by-chain basis
 * by both the frontend and the backend and will explode in fireworks if
 * blindly copied.
 */
void
hammer2_cluster_modsync(hammer2_cluster_t *cluster)
{
	hammer2_chain_t *focus;
	hammer2_chain_t *scan;
	const hammer2_inode_data_t *ripdata;
	hammer2_inode_data_t *wipdata;
	int i;

	focus = cluster->focus;
	KKASSERT(focus->flags & HAMMER2_CHAIN_MODIFIED);

	for (i = 0; i < cluster->nchains; ++i) {
		scan = cluster->array[i].chain;
		if (scan == NULL || scan == focus)
			continue;
		KKASSERT(scan->flags & HAMMER2_CHAIN_MODIFIED);
		KKASSERT(focus->bytes == scan->bytes &&
			 focus->bref.type == scan->bref.type);
		switch(focus->bref.type) {
		case HAMMER2_BREF_TYPE_INODE:
			ripdata = &focus->data->ipdata;
			wipdata = &scan->data->ipdata;
			if ((ripdata->op_flags &
			    HAMMER2_OPFLAG_DIRECTDATA) == 0) {
				bcopy(ripdata, wipdata,
				      offsetof(hammer2_inode_data_t, u));
				break;
			}
			/* fall through to full copy */
		case HAMMER2_BREF_TYPE_DATA:
			bcopy(focus->data, scan->data, focus->bytes);
			break;
		case HAMMER2_BREF_TYPE_FREEMAP_NODE:
		case HAMMER2_BREF_TYPE_FREEMAP_LEAF:
		case HAMMER2_BREF_TYPE_FREEMAP:
		case HAMMER2_BREF_TYPE_VOLUME:
			panic("hammer2_cluster_modsync: illegal node type");
			/* NOT REACHED */
			break;
		default:
			panic("hammer2_cluster_modsync: unknown node type");
			break;
		}
	}
}

/*
 * Lookup initialization/completion API
 */
hammer2_cluster_t *
hammer2_cluster_lookup_init(hammer2_cluster_t *cparent, int flags)
{
	hammer2_cluster_t *cluster;
	int i;

	cluster = kmalloc(sizeof(*cluster), M_HAMMER2, M_WAITOK | M_ZERO);
	cluster->pmp = cparent->pmp;			/* can be NULL */
	cluster->flags = 0;	/* cluster not locked (yet) */
	/* cluster->focus = NULL; already null */

	for (i = 0; i < cparent->nchains; ++i) {
		cluster->array[i].chain = cparent->array[i].chain;
		if (cluster->focus == NULL)
			cluster->focus = cluster->array[i].chain;
	}
	cluster->nchains = cparent->nchains;

	/*
	 * Independently lock (this will also give cluster 1 ref)
	 */
	if (flags & HAMMER2_LOOKUP_SHARED) {
		hammer2_cluster_lock(cluster, HAMMER2_RESOLVE_ALWAYS |
					      HAMMER2_RESOLVE_SHARED);
	} else {
		hammer2_cluster_lock(cluster, HAMMER2_RESOLVE_ALWAYS);
	}
	return (cluster);
}

void
hammer2_cluster_lookup_done(hammer2_cluster_t *cparent)
{
	if (cparent)
		hammer2_cluster_unlock(cparent);
}

/*
 * Locate first match or overlap under parent, return a new cluster
 */
hammer2_cluster_t *
hammer2_cluster_lookup(hammer2_cluster_t *cparent, hammer2_key_t *key_nextp,
		     hammer2_key_t key_beg, hammer2_key_t key_end, int flags)
{
	hammer2_pfs_t *pmp;
	hammer2_cluster_t *cluster;
	hammer2_chain_t *chain;
	hammer2_key_t key_accum;
	hammer2_key_t key_next;
	hammer2_key_t bref_key;
	int null_count;
	int bref_keybits;
	int i;
	uint8_t bref_type;
	u_int bytes;

	pmp = cparent->pmp;				/* can be NULL */
	key_accum = *key_nextp;
	null_count = 0;
	bref_type = 0;
	bref_key = 0;
	bref_keybits = 0;
	bytes = 0;

	cluster = kmalloc(sizeof(*cluster), M_HAMMER2, M_WAITOK | M_ZERO);
	cluster->pmp = pmp;				/* can be NULL */
	cluster->refs = 1;
	/* cluster->focus = NULL; already null */
	if ((flags & HAMMER2_LOOKUP_NOLOCK) == 0)
		cluster->flags |= HAMMER2_CLUSTER_LOCKED;

	for (i = 0; i < cparent->nchains; ++i) {
		key_next = *key_nextp;
		if (cparent->array[i].chain == NULL) {
			++null_count;
			continue;
		}
		chain = hammer2_chain_lookup(&cparent->array[i].chain,
					     &key_next,
					     key_beg, key_end,
					     &cparent->array[i].cache_index,
					     flags);
		cluster->array[i].chain = chain;
		if (chain == NULL) {
			++null_count;
		} else {
			int ddflag = (chain->bref.type ==
				      HAMMER2_BREF_TYPE_INODE);

			/*
			 * Set default focus.
			 */
			if (cluster->focus == NULL) {
				bref_type = chain->bref.type;
				bref_key = chain->bref.key;
				bref_keybits = chain->bref.keybits;
				bytes = chain->bytes;
				cluster->ddflag = ddflag;
				cluster->focus = chain;
			}

			/*
			 * Override default focus to follow the parent.
			 */
			if (cparent->focus == cparent->array[i].chain)
				cluster->focus = chain;

			KKASSERT(bref_type == chain->bref.type);
			KKASSERT(bref_key == chain->bref.key);
			KKASSERT(bref_keybits == chain->bref.keybits);
			KKASSERT(bytes == chain->bytes);
			KKASSERT(cluster->ddflag == ddflag);
		}
		if (key_accum > key_next)
			key_accum = key_next;
	}
	*key_nextp = key_accum;
	cluster->nchains = i;

	if (null_count == i) {
		hammer2_cluster_drop(cluster);
		cluster = NULL;
	}

	return (cluster);
}

/*
 * Locate next match or overlap under parent, replace cluster
 */
hammer2_cluster_t *
hammer2_cluster_next(hammer2_cluster_t *cparent, hammer2_cluster_t *cluster,
		     hammer2_key_t *key_nextp,
		     hammer2_key_t key_beg, hammer2_key_t key_end, int flags)
{
	hammer2_chain_t *chain;
	hammer2_key_t key_accum;
	hammer2_key_t key_next;
	hammer2_key_t bref_key;
	int null_count;
	int bref_keybits;
	int i;
	uint8_t bref_type;
	u_int bytes;

	key_accum = *key_nextp;
	null_count = 0;
	cluster->focus = NULL;
	cparent->focus = NULL;

	bref_type = 0;
	bref_key = 0;
	bref_keybits = 0;
	bytes = 0;
	cluster->ddflag = 0;

	for (i = 0; i < cparent->nchains; ++i) {
		key_next = *key_nextp;
		chain = cluster->array[i].chain;
		if (chain == NULL) {
			++null_count;
			continue;
		}
		if (cparent->array[i].chain == NULL) {
			if (flags & HAMMER2_LOOKUP_NOLOCK)
				hammer2_chain_drop(chain);
			else
				hammer2_chain_unlock(chain);
			++null_count;
			continue;
		}
		chain = hammer2_chain_next(&cparent->array[i].chain, chain,
					   &key_next, key_beg, key_end,
					   &cparent->array[i].cache_index,
					   flags);
		cluster->array[i].chain = chain;
		if (chain == NULL) {
			++null_count;
		} else {
			int ddflag = (chain->bref.type ==
				      HAMMER2_BREF_TYPE_INODE);
			if (cluster->focus == NULL) {
				bref_type = chain->bref.type;
				bref_key = chain->bref.key;
				bref_keybits = chain->bref.keybits;
				bytes = chain->bytes;
				cluster->ddflag = ddflag;
				cluster->focus = chain;
			}

			/*
			 * Override default focus to follow the parent.
			 */
			if (cparent->focus == cparent->array[i].chain)
				cluster->focus = chain;

			KKASSERT(bref_type == chain->bref.type);
			KKASSERT(bref_key == chain->bref.key);
			KKASSERT(bref_keybits == chain->bref.keybits);
			KKASSERT(bytes == chain->bytes);
			KKASSERT(cluster->ddflag == ddflag);
		}
		if (key_accum > key_next)
			key_accum = key_next;
	}
	cluster->nchains = i;

	if (null_count == i) {
		hammer2_cluster_drop(cluster);
		cluster = NULL;
	}
	return(cluster);
}

#if 0
/*
 * XXX initial NULL cluster needs reworking (pass **clusterp ?)
 *
 * The raw scan function is similar to lookup/next but does not seek to a key.
 * Blockrefs are iterated via first_chain = (parent, NULL) and
 * next_chain = (parent, chain).
 *
 * The passed-in parent must be locked and its data resolved.  The returned
 * chain will be locked.  Pass chain == NULL to acquire the first sub-chain
 * under parent and then iterate with the passed-in chain (which this
 * function will unlock).
 */
hammer2_cluster_t *
hammer2_cluster_scan(hammer2_cluster_t *cparent, hammer2_cluster_t *cluster,
		     int flags)
{
	hammer2_chain_t *chain;
	int null_count;
	int i;

	null_count = 0;

	for (i = 0; i < cparent->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain == NULL) {
			++null_count;
			continue;
		}
		if (cparent->array[i].chain == NULL) {
			if (flags & HAMMER2_LOOKUP_NOLOCK)
				hammer2_chain_drop(chain);
			else
				hammer2_chain_unlock(chain);
			++null_count;
			continue;
		}

		chain = hammer2_chain_scan(cparent->array[i].chain, chain,
					   &cparent->array[i].cache_index,
					   flags);
		cluster->array[i].chain = chain;
		if (chain == NULL)
			++null_count;
	}

	if (null_count == i) {
		hammer2_cluster_drop(cluster);
		cluster = NULL;
	}
	return(cluster);
}

#endif

/*
 * Create a new cluster using the specified key
 */
int
hammer2_cluster_create(hammer2_trans_t *trans, hammer2_cluster_t *cparent,
		     hammer2_cluster_t **clusterp,
		     hammer2_key_t key, int keybits,
		     int type, size_t bytes, int flags)
{
	hammer2_cluster_t *cluster;
	hammer2_pfs_t *pmp;
	int error;
	int i;

	pmp = trans->pmp;				/* can be NULL */

	if ((cluster = *clusterp) == NULL) {
		cluster = kmalloc(sizeof(*cluster), M_HAMMER2,
				  M_WAITOK | M_ZERO);
		cluster->pmp = pmp;			/* can be NULL */
		cluster->refs = 1;
		cluster->flags = HAMMER2_CLUSTER_LOCKED;
	}
	cluster->focus = NULL;

	/*
	 * NOTE: cluster->array[] entries can initially be NULL.  If
	 *	 *clusterp is supplied, skip NULL entries, otherwise
	 *	 create new chains.
	 */
	for (i = 0; i < cparent->nchains; ++i) {
		if (*clusterp && cluster->array[i].chain == NULL) {
			continue;
		}
		error = hammer2_chain_create(trans, &cparent->array[i].chain,
					     &cluster->array[i].chain, pmp,
					     key, keybits,
					     type, bytes, flags);
		KKASSERT(error == 0);
		if (cluster->focus == NULL)
			cluster->focus = cluster->array[i].chain;
		if (cparent->focus == cparent->array[i].chain)
			cluster->focus = cluster->array[i].chain;
	}
	cluster->nchains = i;
	*clusterp = cluster;

	return error;
}

/*
 * Rename a cluster to a new parent.
 *
 * WARNING! Unlike hammer2_chain_rename(), only the key and keybits fields
 *	    are used from a passed-in non-NULL bref pointer.  All other fields
 *	    are extracted from the original chain for each chain in the
 *	    iteration.
 */
void
hammer2_cluster_rename(hammer2_trans_t *trans, hammer2_blockref_t *bref,
		       hammer2_cluster_t *cparent, hammer2_cluster_t *cluster,
		       int flags)
{
	hammer2_chain_t *chain;
	hammer2_blockref_t xbref;
	int i;

	cluster->focus = NULL;
	cparent->focus = NULL;

	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain) {
			if (bref) {
				xbref = chain->bref;
				xbref.key = bref->key;
				xbref.keybits = bref->keybits;
				hammer2_chain_rename(trans, &xbref,
						     &cparent->array[i].chain,
						     chain, flags);
			} else {
				hammer2_chain_rename(trans, NULL,
						     &cparent->array[i].chain,
						     chain, flags);
			}
			cluster->array[i].chain = chain;
			if (cluster->focus == NULL)
				cluster->focus = chain;
			if (cparent->focus == NULL)
				cparent->focus = cparent->array[i].chain;
		} else {
			if (cparent->focus == NULL)
				cparent->focus = cparent->array[i].chain;
		}
	}
}

/*
 * Mark a cluster deleted
 */
void
hammer2_cluster_delete(hammer2_trans_t *trans, hammer2_cluster_t *cparent,
		       hammer2_cluster_t *cluster, int flags)
{
	hammer2_chain_t *chain;
	hammer2_chain_t *parent;
	int i;

	if (cparent == NULL) {
		kprintf("cparent is NULL\n");
		return;
	}

	for (i = 0; i < cluster->nchains; ++i) {
		parent = (i < cparent->nchains) ?
			 cparent->array[i].chain : NULL;
		chain = cluster->array[i].chain;
		if (chain == NULL)
			continue;
		if (chain->parent != parent) {
			kprintf("hammer2_cluster_delete: parent "
				"mismatch chain=%p parent=%p against=%p\n",
				chain, chain->parent, parent);
		} else {
			hammer2_chain_delete(trans, parent, chain, flags);
		}
	}
}

/*
 * Create a snapshot of the specified {parent, ochain} with the specified
 * label.  The originating hammer2_inode must be exclusively locked for
 * safety.
 *
 * The ioctl code has already synced the filesystem.
 */
int
hammer2_cluster_snapshot(hammer2_trans_t *trans, hammer2_cluster_t *ocluster,
		       hammer2_ioc_pfs_t *pfs)
{
	hammer2_dev_t *hmp;
	hammer2_cluster_t *ncluster;
	const hammer2_inode_data_t *ripdata;
	hammer2_inode_data_t *wipdata;
	hammer2_chain_t *nchain;
	hammer2_inode_t *nip;
	size_t name_len;
	hammer2_key_t lhc;
	struct vattr vat;
#if 0
	uuid_t opfs_clid;
#endif
	int error;
	int i;

	kprintf("snapshot %s\n", pfs->name);

	name_len = strlen(pfs->name);
	lhc = hammer2_dirhash(pfs->name, name_len);

	/*
	 * Get the clid
	 */
	ripdata = &hammer2_cluster_rdata(ocluster)->ipdata;
#if 0
	opfs_clid = ripdata->pfs_clid;
#endif
	hmp = ocluster->focus->hmp;	/* XXX find synchronized local disk */

	/*
	 * Create the snapshot directory under the super-root
	 *
	 * Set PFS type, generate a unique filesystem id, and generate
	 * a cluster id.  Use the same clid when snapshotting a PFS root,
	 * which theoretically allows the snapshot to be used as part of
	 * the same cluster (perhaps as a cache).
	 *
	 * Copy the (flushed) blockref array.  Theoretically we could use
	 * chain_duplicate() but it becomes difficult to disentangle
	 * the shared core so for now just brute-force it.
	 */
	VATTR_NULL(&vat);
	vat.va_type = VDIR;
	vat.va_mode = 0755;
	ncluster = NULL;
	nip = hammer2_inode_create(trans, hmp->spmp->iroot, &vat,
				   proc0.p_ucred, pfs->name, name_len,
				   &ncluster,
				   HAMMER2_INSERT_PFSROOT, &error);

	if (nip) {
		wipdata = hammer2_cluster_modify_ip(trans, nip, ncluster, 0);
		wipdata->pfs_type = HAMMER2_PFSTYPE_SNAPSHOT;
		wipdata->op_flags |= HAMMER2_OPFLAG_PFSROOT;
		kern_uuidgen(&wipdata->pfs_fsid, 1);

		/*
		 * Give the snapshot its own private cluster.  As a snapshot
		 * no further synchronization with the original cluster will
		 * be done.
		 */
#if 0
		if (ocluster->focus->flags & HAMMER2_CHAIN_PFSBOUNDARY)
			wipdata->pfs_clid = opfs_clid;
		else
			kern_uuidgen(&wipdata->pfs_clid, 1);
#endif
		kern_uuidgen(&wipdata->pfs_clid, 1);

		for (i = 0; i < ncluster->nchains; ++i) {
			nchain = ncluster->array[i].chain;
			if (nchain)
				nchain->bref.flags |= HAMMER2_BREF_FLAG_PFSROOT;
		}
#if 0
		/* XXX can't set this unless we do an explicit flush, which
		   we also need a pmp assigned to do, else the flush code
		   won't flush ncluster because it thinks it is crossing a
		   flush boundary */
		hammer2_cluster_set_chainflags(ncluster,
					       HAMMER2_CHAIN_PFSBOUNDARY);
#endif

		/* XXX hack blockset copy */
		/* XXX doesn't work with real cluster */
		KKASSERT(ocluster->nchains == 1);
		wipdata->u.blockset = ripdata->u.blockset;
		hammer2_cluster_modsync(ncluster);
		for (i = 0; i < ncluster->nchains; ++i) {
			nchain = ncluster->array[i].chain;
			if (nchain)
				hammer2_flush(trans, nchain);
		}
		hammer2_inode_unlock_ex(nip, ncluster);
	}
	return (error);
}

/*
 * Return locked parent cluster given a locked child.  The child remains
 * locked on return.  The new parent's focus follows the child's focus
 * and the parent is always resolved.
 */
hammer2_cluster_t *
hammer2_cluster_parent(hammer2_cluster_t *cluster)
{
	hammer2_cluster_t *cparent;
	int i;

	cparent = hammer2_cluster_copy(cluster);

	for (i = 0; i < cparent->nchains; ++i) {
		hammer2_chain_t *chain;
		hammer2_chain_t *rchain;

		/*
		 * Calculate parent for each element.  Old chain has an extra
		 * ref for cparent but the lock remains with cluster.
		 */
		chain = cparent->array[i].chain;
		if (chain == NULL)
			continue;
		while ((rchain = chain->parent) != NULL) {
			hammer2_chain_ref(rchain);
			hammer2_chain_unlock(chain);
			hammer2_chain_lock(rchain, HAMMER2_RESOLVE_ALWAYS);
			hammer2_chain_lock(chain, HAMMER2_RESOLVE_ALWAYS);
			hammer2_chain_drop(rchain);
			if (chain->parent == rchain)
				break;
			hammer2_chain_unlock(rchain);
		}
		if (cluster->focus == chain)
			cparent->focus = rchain;
		cparent->array[i].chain = rchain;
		hammer2_chain_drop(chain);
	}
	cparent->flags |= HAMMER2_CLUSTER_LOCKED;

	return cparent;
}

/************************************************************************
 *			        CLUSTER I/O 				*
 ************************************************************************
 *
 *
 * WARNING! blockref[] array data is not universal.  These functions should
 *	    only be used to access universal data.
 *
 * NOTE!    The rdata call will wait for at least one of the chain I/Os to
 *	    complete if necessary.  The I/O's should have already been
 *	    initiated by the cluster_lock/chain_lock operation.
 *
 *	    The cluster must already be in a modified state before wdata
 *	    is called.  The data will already be available for this case.
 */
const hammer2_media_data_t *
hammer2_cluster_rdata(hammer2_cluster_t *cluster)
{
	return(cluster->focus->data);
}

hammer2_media_data_t *
hammer2_cluster_wdata(hammer2_cluster_t *cluster)
{
	KKASSERT(hammer2_cluster_modified(cluster));
	return(cluster->focus->data);
}

/*
 * Load cluster data asynchronously with callback.
 *
 * The callback is made for the first validated data found, or NULL
 * if no valid data is available.
 *
 * NOTE! The cluster structure is either unique or serialized (e.g. embedded
 *	 in the inode with an exclusive lock held), the chain structure may be
 *	 shared.
 */
void
hammer2_cluster_load_async(hammer2_cluster_t *cluster,
			   void (*callback)(hammer2_iocb_t *iocb), void *ptr)
{
	hammer2_chain_t *chain;
	hammer2_iocb_t *iocb;
	hammer2_dev_t *hmp;
	hammer2_blockref_t *bref;
	int i;

	/*
	 * Try to find a chain whos data is already resolved.  If none can
	 * be found, start with the first chain.
	 */
	chain = NULL;
	for (i = 0; i < cluster->nchains; ++i) {
		chain = cluster->array[i].chain;
		if (chain && chain->data)
			break;
	}
	if (i == cluster->nchains) {
		chain = cluster->array[0].chain;
		i = 0;
	}

	iocb = &cluster->iocb;
	iocb->callback = callback;
	iocb->dio = NULL;		/* for already-validated case */
	iocb->cluster = cluster;
	iocb->chain = chain;
	iocb->ptr = ptr;
	iocb->lbase = (off_t)i;
	iocb->flags = 0;
	iocb->error = 0;

	/*
	 * Data already validated
	 */
	if (chain->data) {
		callback(iocb);
		return;
	}

	/*
	 * We must resolve to a device buffer, either by issuing I/O or
	 * by creating a zero-fill element.  We do not mark the buffer
	 * dirty when creating a zero-fill element (the hammer2_chain_modify()
	 * API must still be used to do that).
	 *
	 * The device buffer is variable-sized in powers of 2 down
	 * to HAMMER2_MIN_ALLOC (typically 1K).  A 64K physical storage
	 * chunk always contains buffers of the same size. (XXX)
	 *
	 * The minimum physical IO size may be larger than the variable
	 * block size.
	 *
	 * XXX TODO - handle HAMMER2_CHAIN_INITIAL for case where chain->bytes
	 *	      matches hammer2_devblksize()?  Or does the freemap's
	 *	      pre-zeroing handle the case for us?
	 */
	bref = &chain->bref;
	hmp = chain->hmp;

#if 0
	/* handled by callback? <- TODO XXX even needed for loads? */
	/*
	 * The getblk() optimization for a 100% overwrite can only be used
	 * if the physical block size matches the request.
	 */
	if ((chain->flags & HAMMER2_CHAIN_INITIAL) &&
	    chain->bytes == hammer2_devblksize(chain->bytes)) {
		error = hammer2_io_new(hmp, bref->data_off, chain->bytes, &dio);
		KKASSERT(error == 0);
		iocb->dio = dio;
		callback(iocb);
		return;
	}
#endif

	/*
	 * Otherwise issue a read
	 */
	hammer2_adjreadcounter(&chain->bref, chain->bytes);
	hammer2_io_getblk(hmp, bref->data_off, chain->bytes, iocb);
}

/************************************************************************
 *			    NODE FAILURES 				*
 ************************************************************************
 *
 * A node failure can occur for numerous reasons.
 *
 *	- A read I/O may fail
 *	- A write I/O may fail
 *	- An unexpected chain might be found (or be missing)
 *	- A node might disconnect temporarily and reconnect later
 *	  (for example, a USB stick could get pulled, or a node might
 *	  be programmatically disconnected).
 *	- A node might run out of space during a modifying operation.
 *
 * When a read failure or an unexpected chain state is found, the chain and
 * parent chain at the failure point for the nodes involved (the nodes
 * which we determine to be in error) are flagged as failed and removed
 * from the cluster.  The node itself is allowed to remain active.  The
 * highest common point (usually a parent chain) is queued to the
 * resynchronization thread for action.
 *
 * When a write I/O fails or a node runs out of space, we first adjust
 * as if a read failure occurs but we further disable flushes on the
 * ENTIRE node.  Concurrent modifying transactions are allowed to complete
 * but any new modifying transactions will automatically remove the node
 * from consideration in all related cluster structures and not generate
 * any new modified chains.  The ROOT chain for the failed node(s) is queued
 * to the resynchronization thread for action.
 *
 * A temporary disconnect is handled as if a write failure occurred.
 *
 * Any of these failures might or might not stall related high level VNOPS,
 * depending on what has failed, what nodes remain, the type of cluster,
 * and the operating state of the cluster.
 *
 *			    FLUSH ON WRITE-DISABLED NODES
 *
 * A flush on a write-disabled node is not allowed to write anything because
 * we cannot safely update the mirror_tid anywhere on the failed node.  The
 * synchronization thread uses mirror_tid to calculate incremental resyncs.
 * Dirty meta-data related to the failed node is thrown away.
 *
 * Dirty buffer cache buffers and inodes are only thrown away if they can be
 * retired... that is, if the filesystem still has enough nodes to complete
 * the operation.
 */

/************************************************************************
 *			SYNCHRONIZATION THREAD				*
 ************************************************************************
 *
 * This thread is responsible for [re]synchronizing the cluster representing
 * a PFS.  Any out-of-sync or failed node starts this thread on a
 * node-by-node basis when the failure is detected.
 *
 * Clusters needing resynchronization are queued at the highest point
 * where the parent on the failed node is still valid, or a special
 * incremental scan from the ROOT is queued if no parent exists.  This
 * thread is also responsible for waiting for reconnections of the failed
 * node if the cause was due to a disconnect, and waiting for space to be
 * freed up if the cause was due to running out of space.
 *
 * If the cause is due to a node running out of space, this thread will also
 * remove older (unlocked) snapshots to make new space, recover space, and
 * then start resynchronization.
 *
 * Each resynchronization pass virtually snapshots the PFS on the good nodes
 * and synchronizes using that snapshot against the target node.  This
 * ensures a consistent chain topology and also avoids interference between
 * the resynchronization thread and frontend operations.
 *
 * Since these are per-node threads it is possible to resynchronize several
 * nodes at once.
 */
