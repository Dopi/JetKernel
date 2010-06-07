/*
 * Copyright (C) 2007 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */
#include <linux/sched.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>
#include <linux/blkdev.h>
#include <linux/sort.h>
#include <linux/rcupdate.h>
#include "compat.h"
#include "hash.h"
#include "crc32c.h"
#include "ctree.h"
#include "disk-io.h"
#include "print-tree.h"
#include "transaction.h"
#include "volumes.h"
#include "locking.h"
#include "ref-cache.h"

#define PENDING_EXTENT_INSERT 0
#define PENDING_EXTENT_DELETE 1
#define PENDING_BACKREF_UPDATE 2

struct pending_extent_op {
	int type;
	u64 bytenr;
	u64 num_bytes;
	u64 parent;
	u64 orig_parent;
	u64 generation;
	u64 orig_generation;
	int level;
	struct list_head list;
	int del;
};

static int finish_current_insert(struct btrfs_trans_handle *trans,
				 struct btrfs_root *extent_root, int all);
static int del_pending_extents(struct btrfs_trans_handle *trans,
			       struct btrfs_root *extent_root, int all);
static int pin_down_bytes(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root,
			  u64 bytenr, u64 num_bytes, int is_data);
static int update_block_group(struct btrfs_trans_handle *trans,
			      struct btrfs_root *root,
			      u64 bytenr, u64 num_bytes, int alloc,
			      int mark_free);

static int do_chunk_alloc(struct btrfs_trans_handle *trans,
			  struct btrfs_root *extent_root, u64 alloc_bytes,
			  u64 flags, int force);

static int block_group_bits(struct btrfs_block_group_cache *cache, u64 bits)
{
	return (cache->flags & bits) == bits;
}

/*
 * this adds the block group to the fs_info rb tree for the block group
 * cache
 */
static int btrfs_add_block_group_cache(struct btrfs_fs_info *info,
				struct btrfs_block_group_cache *block_group)
{
	struct rb_node **p;
	struct rb_node *parent = NULL;
	struct btrfs_block_group_cache *cache;

	spin_lock(&info->block_group_cache_lock);
	p = &info->block_group_cache_tree.rb_node;

	while (*p) {
		parent = *p;
		cache = rb_entry(parent, struct btrfs_block_group_cache,
				 cache_node);
		if (block_group->key.objectid < cache->key.objectid) {
			p = &(*p)->rb_left;
		} else if (block_group->key.objectid > cache->key.objectid) {
			p = &(*p)->rb_right;
		} else {
			spin_unlock(&info->block_group_cache_lock);
			return -EEXIST;
		}
	}

	rb_link_node(&block_group->cache_node, parent, p);
	rb_insert_color(&block_group->cache_node,
			&info->block_group_cache_tree);
	spin_unlock(&info->block_group_cache_lock);

	return 0;
}

/*
 * This will return the block group at or after bytenr if contains is 0, else
 * it will return the block group that contains the bytenr
 */
static struct btrfs_block_group_cache *
block_group_cache_tree_search(struct btrfs_fs_info *info, u64 bytenr,
			      int contains)
{
	struct btrfs_block_group_cache *cache, *ret = NULL;
	struct rb_node *n;
	u64 end, start;

	spin_lock(&info->block_group_cache_lock);
	n = info->block_group_cache_tree.rb_node;

	while (n) {
		cache = rb_entry(n, struct btrfs_block_group_cache,
				 cache_node);
		end = cache->key.objectid + cache->key.offset - 1;
		start = cache->key.objectid;

		if (bytenr < start) {
			if (!contains && (!ret || start < ret->key.objectid))
				ret = cache;
			n = n->rb_left;
		} else if (bytenr > start) {
			if (contains && bytenr <= end) {
				ret = cache;
				break;
			}
			n = n->rb_right;
		} else {
			ret = cache;
			break;
		}
	}
	if (ret)
		atomic_inc(&ret->count);
	spin_unlock(&info->block_group_cache_lock);

	return ret;
}

/*
 * this is only called by cache_block_group, since we could have freed extents
 * we need to check the pinned_extents for any extents that can't be used yet
 * since their free space will be released as soon as the transaction commits.
 */
static int add_new_free_space(struct btrfs_block_group_cache *block_group,
			      struct btrfs_fs_info *info, u64 start, u64 end)
{
	u64 extent_start, extent_end, size;
	int ret;

	mutex_lock(&info->pinned_mutex);
	while (start < end) {
		ret = find_first_extent_bit(&info->pinned_extents, start,
					    &extent_start, &extent_end,
					    EXTENT_DIRTY);
		if (ret)
			break;

		if (extent_start == start) {
			start = extent_end + 1;
		} else if (extent_start > start && extent_start < end) {
			size = extent_start - start;
			ret = btrfs_add_free_space(block_group, start,
						   size);
			BUG_ON(ret);
			start = extent_end + 1;
		} else {
			break;
		}
	}

	if (start < end) {
		size = end - start;
		ret = btrfs_add_free_space(block_group, start, size);
		BUG_ON(ret);
	}
	mutex_unlock(&info->pinned_mutex);

	return 0;
}

static int remove_sb_from_cache(struct btrfs_root *root,
				struct btrfs_block_group_cache *cache)
{
	u64 bytenr;
	u64 *logical;
	int stripe_len;
	int i, nr, ret;

	for (i = 0; i < BTRFS_SUPER_MIRROR_MAX; i++) {
		bytenr = btrfs_sb_offset(i);
		ret = btrfs_rmap_block(&root->fs_info->mapping_tree,
				       cache->key.objectid, bytenr, 0,
				       &logical, &nr, &stripe_len);
		BUG_ON(ret);
		while (nr--) {
			btrfs_remove_free_space(cache, logical[nr],
						stripe_len);
		}
		kfree(logical);
	}
	return 0;
}

static int cache_block_group(struct btrfs_root *root,
			     struct btrfs_block_group_cache *block_group)
{
	struct btrfs_path *path;
	int ret = 0;
	struct btrfs_key key;
	struct extent_buffer *leaf;
	int slot;
	u64 last;

	if (!block_group)
		return 0;

	root = root->fs_info->extent_root;

	if (block_group->cached)
		return 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 2;
	/*
	 * we get into deadlocks with paths held by callers of this function.
	 * since the alloc_mutex is protecting things right now, just
	 * skip the locking here
	 */
	path->skip_locking = 1;
	last = max_t(u64, block_group->key.objectid, BTRFS_SUPER_INFO_OFFSET);
	key.objectid = last;
	key.offset = 0;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		goto err;

	while (1) {
		leaf = path->nodes[0];
		slot = path->slots[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto err;
			if (ret == 0)
				continue;
			else
				break;
		}
		btrfs_item_key_to_cpu(leaf, &key, slot);
		if (key.objectid < block_group->key.objectid)
			goto next;

		if (key.objectid >= block_group->key.objectid +
		    block_group->key.offset)
			break;

		if (btrfs_key_type(&key) == BTRFS_EXTENT_ITEM_KEY) {
			add_new_free_space(block_group, root->fs_info, last,
					   key.objectid);

			last = key.objectid + key.offset;
		}
next:
		path->slots[0]++;
	}

	add_new_free_space(block_group, root->fs_info, last,
			   block_group->key.objectid +
			   block_group->key.offset);

	remove_sb_from_cache(root, block_group);
	block_group->cached = 1;
	ret = 0;
err:
	btrfs_free_path(path);
	return ret;
}

/*
 * return the block group that starts at or after bytenr
 */
static struct btrfs_block_group_cache *
btrfs_lookup_first_block_group(struct btrfs_fs_info *info, u64 bytenr)
{
	struct btrfs_block_group_cache *cache;

	cache = block_group_cache_tree_search(info, bytenr, 0);

	return cache;
}

/*
 * return the block group that contains teh given bytenr
 */
struct btrfs_block_group_cache *btrfs_lookup_block_group(
						 struct btrfs_fs_info *info,
						 u64 bytenr)
{
	struct btrfs_block_group_cache *cache;

	cache = block_group_cache_tree_search(info, bytenr, 1);

	return cache;
}

static inline void put_block_group(struct btrfs_block_group_cache *cache)
{
	if (atomic_dec_and_test(&cache->count))
		kfree(cache);
}

static struct btrfs_space_info *__find_space_info(struct btrfs_fs_info *info,
						  u64 flags)
{
	struct list_head *head = &info->space_info;
	struct btrfs_space_info *found;

	rcu_read_lock();
	list_for_each_entry_rcu(found, head, list) {
		if (found->flags == flags) {
			rcu_read_unlock();
			return found;
		}
	}
	rcu_read_unlock();
	return NULL;
}

/*
 * after adding space to the filesystem, we need to clear the full flags
 * on all the space infos.
 */
void btrfs_clear_space_info_full(struct btrfs_fs_info *info)
{
	struct list_head *head = &info->space_info;
	struct btrfs_space_info *found;

	rcu_read_lock();
	list_for_each_entry_rcu(found, head, list)
		found->full = 0;
	rcu_read_unlock();
}

static u64 div_factor(u64 num, int factor)
{
	if (factor == 10)
		return num;
	num *= factor;
	do_div(num, 10);
	return num;
}

u64 btrfs_find_block_group(struct btrfs_root *root,
			   u64 search_start, u64 search_hint, int owner)
{
	struct btrfs_block_group_cache *cache;
	u64 used;
	u64 last = max(search_hint, search_start);
	u64 group_start = 0;
	int full_search = 0;
	int factor = 9;
	int wrapped = 0;
again:
	while (1) {
		cache = btrfs_lookup_first_block_group(root->fs_info, last);
		if (!cache)
			break;

		spin_lock(&cache->lock);
		last = cache->key.objectid + cache->key.offset;
		used = btrfs_block_group_used(&cache->item);

		if ((full_search || !cache->ro) &&
		    block_group_bits(cache, BTRFS_BLOCK_GROUP_METADATA)) {
			if (used + cache->pinned + cache->reserved <
			    div_factor(cache->key.offset, factor)) {
				group_start = cache->key.objectid;
				spin_unlock(&cache->lock);
				put_block_group(cache);
				goto found;
			}
		}
		spin_unlock(&cache->lock);
		put_block_group(cache);
		cond_resched();
	}
	if (!wrapped) {
		last = search_start;
		wrapped = 1;
		goto again;
	}
	if (!full_search && factor < 10) {
		last = search_start;
		full_search = 1;
		factor = 10;
		goto again;
	}
found:
	return group_start;
}

/* simple helper to search for an existing extent at a given offset */
int btrfs_lookup_extent(struct btrfs_root *root, u64 start, u64 len)
{
	int ret;
	struct btrfs_key key;
	struct btrfs_path *path;

	path = btrfs_alloc_path();
	BUG_ON(!path);
	key.objectid = start;
	key.offset = len;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	ret = btrfs_search_slot(NULL, root->fs_info->extent_root, &key, path,
				0, 0);
	btrfs_free_path(path);
	return ret;
}

/*
 * Back reference rules.  Back refs have three main goals:
 *
 * 1) differentiate between all holders of references to an extent so that
 *    when a reference is dropped we can make sure it was a valid reference
 *    before freeing the extent.
 *
 * 2) Provide enough information to quickly find the holders of an extent
 *    if we notice a given block is corrupted or bad.
 *
 * 3) Make it easy to migrate blocks for FS shrinking or storage pool
 *    maintenance.  This is actually the same as #2, but with a slightly
 *    different use case.
 *
 * File extents can be referenced by:
 *
 * - multiple snapshots, subvolumes, or different generations in one subvol
 * - different files inside a single subvolume
 * - different offsets inside a file (bookend extents in file.c)
 *
 * The extent ref structure has fields for:
 *
 * - Objectid of the subvolume root
 * - Generation number of the tree holding the reference
 * - objectid of the file holding the reference
 * - number of references holding by parent node (alway 1 for tree blocks)
 *
 * Btree leaf may hold multiple references to a file extent. In most cases,
 * these references are from same file and the corresponding offsets inside
 * the file are close together.
 *
 * When a file extent is allocated the fields are filled in:
 *     (root_key.objectid, trans->transid, inode objectid, 1)
 *
 * When a leaf is cow'd new references are added for every file extent found
 * in the leaf.  It looks similar to the create case, but trans->transid will
 * be different when the block is cow'd.
 *
 *     (root_key.objectid, trans->transid, inode objectid,
 *      number of references in the leaf)
 *
 * When a file extent is removed either during snapshot deletion or
 * file truncation, we find the corresponding back reference and check
 * the following fields:
 *
 *     (btrfs_header_owner(leaf), btrfs_header_generation(leaf),
 *      inode objectid)
 *
 * Btree extents can be referenced by:
 *
 * - Different subvolumes
 * - Different generations of the same subvolume
 *
 * When a tree block is created, back references are inserted:
 *
 * (root->root_key.objectid, trans->transid, level, 1)
 *
 * When a tree block is cow'd, new back references are added for all the
 * blocks it points to. If the tree block isn't in reference counted root,
 * the old back references are removed. These new back references are of
 * the form (trans->transid will have increased since creation):
 *
 * (root->root_key.objectid, trans->transid, level, 1)
 *
 * When a backref is in deleting, the following fields are checked:
 *
 * if backref was for a tree root:
 *     (btrfs_header_owner(itself), btrfs_header_generation(itself), level)
 * else
 *     (btrfs_header_owner(parent), btrfs_header_generation(parent), level)
 *
 * Back Reference Key composing:
 *
 * The key objectid corresponds to the first byte in the extent, the key
 * type is set to BTRFS_EXTENT_REF_KEY, and the key offset is the first
 * byte of parent extent. If a extent is tree root, the key offset is set
 * to the key objectid.
 */

static noinline int lookup_extent_backref(struct btrfs_trans_handle *trans,
					  struct btrfs_root *root,
					  struct btrfs_path *path,
					  u64 bytenr, u64 parent,
					  u64 ref_root, u64 ref_generation,
					  u64 owner_objectid, int del)
{
	struct btrfs_key key;
	struct btrfs_extent_ref *ref;
	struct extent_buffer *leaf;
	u64 ref_objectid;
	int ret;

	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_REF_KEY;
	key.offset = parent;

	ret = btrfs_search_slot(trans, root, &key, path, del ? -1 : 0, 1);
	if (ret < 0)
		goto out;
	if (ret > 0) {
		ret = -ENOENT;
		goto out;
	}

	leaf = path->nodes[0];
	ref = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_extent_ref);
	ref_objectid = btrfs_ref_objectid(leaf, ref);
	if (btrfs_ref_root(leaf, ref) != ref_root ||
	    btrfs_ref_generation(leaf, ref) != ref_generation ||
	    (ref_objectid != owner_objectid &&
	     ref_objectid != BTRFS_MULTIPLE_OBJECTIDS)) {
		ret = -EIO;
		WARN_ON(1);
		goto out;
	}
	ret = 0;
out:
	return ret;
}

/*
 * updates all the backrefs that are pending on update_list for the
 * extent_root
 */
static noinline int update_backrefs(struct btrfs_trans_handle *trans,
				    struct btrfs_root *extent_root,
				    struct btrfs_path *path,
				    struct list_head *update_list)
{
	struct btrfs_key key;
	struct btrfs_extent_ref *ref;
	struct btrfs_fs_info *info = extent_root->fs_info;
	struct pending_extent_op *op;
	struct extent_buffer *leaf;
	int ret = 0;
	struct list_head *cur = update_list->next;
	u64 ref_objectid;
	u64 ref_root = extent_root->root_key.objectid;

	op = list_entry(cur, struct pending_extent_op, list);

search:
	key.objectid = op->bytenr;
	key.type = BTRFS_EXTENT_REF_KEY;
	key.offset = op->orig_parent;

	ret = btrfs_search_slot(trans, extent_root, &key, path, 0, 1);
	BUG_ON(ret);

	leaf = path->nodes[0];

loop:
	ref = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_extent_ref);

	ref_objectid = btrfs_ref_objectid(leaf, ref);

	if (btrfs_ref_root(leaf, ref) != ref_root ||
	    btrfs_ref_generation(leaf, ref) != op->orig_generation ||
	    (ref_objectid != op->level &&
	     ref_objectid != BTRFS_MULTIPLE_OBJECTIDS)) {
		printk(KERN_ERR "btrfs couldn't find %llu, parent %llu, "
		       "root %llu, owner %u\n",
		       (unsigned long long)op->bytenr,
		       (unsigned long long)op->orig_parent,
		       (unsigned long long)ref_root, op->level);
		btrfs_print_leaf(extent_root, leaf);
		BUG();
	}

	key.objectid = op->bytenr;
	key.offset = op->parent;
	key.type = BTRFS_EXTENT_REF_KEY;
	ret = btrfs_set_item_key_safe(trans, extent_root, path, &key);
	BUG_ON(ret);
	ref = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_extent_ref);
	btrfs_set_ref_generation(leaf, ref, op->generation);

	cur = cur->next;

	list_del_init(&op->list);
	unlock_extent(&info->extent_ins, op->bytenr,
		      op->bytenr + op->num_bytes - 1, GFP_NOFS);
	kfree(op);

	if (cur == update_list) {
		btrfs_mark_buffer_dirty(path->nodes[0]);
		btrfs_release_path(extent_root, path);
		goto out;
	}

	op = list_entry(cur, struct pending_extent_op, list);

	path->slots[0]++;
	while (path->slots[0] < btrfs_header_nritems(leaf)) {
		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		if (key.objectid == op->bytenr &&
		    key.type == BTRFS_EXTENT_REF_KEY)
			goto loop;
		path->slots[0]++;
	}

	btrfs_mark_buffer_dirty(path->nodes[0]);
	btrfs_release_path(extent_root, path);
	goto search;

out:
	return 0;
}

static noinline int insert_extents(struct btrfs_trans_handle *trans,
				   struct btrfs_root *extent_root,
				   struct btrfs_path *path,
				   struct list_head *insert_list, int nr)
{
	struct btrfs_key *keys;
	u32 *data_size;
	struct pending_extent_op *op;
	struct extent_buffer *leaf;
	struct list_head *cur = insert_list->next;
	struct btrfs_fs_info *info = extent_root->fs_info;
	u64 ref_root = extent_root->root_key.objectid;
	int i = 0, last = 0, ret;
	int total = nr * 2;

	if (!nr)
		return 0;

	keys = kzalloc(total * sizeof(struct btrfs_key), GFP_NOFS);
	if (!keys)
		return -ENOMEM;

	data_size = kzalloc(total * sizeof(u32), GFP_NOFS);
	if (!data_size) {
		kfree(keys);
		return -ENOMEM;
	}

	list_for_each_entry(op, insert_list, list) {
		keys[i].objectid = op->bytenr;
		keys[i].offset = op->num_bytes;
		keys[i].type = BTRFS_EXTENT_ITEM_KEY;
		data_size[i] = sizeof(struct btrfs_extent_item);
		i++;

		keys[i].objectid = op->bytenr;
		keys[i].offset = op->parent;
		keys[i].type = BTRFS_EXTENT_REF_KEY;
		data_size[i] = sizeof(struct btrfs_extent_ref);
		i++;
	}

	op = list_entry(cur, struct pending_extent_op, list);
	i = 0;
	while (i < total) {
		int c;
		ret = btrfs_insert_some_items(trans, extent_root, path,
					      keys+i, data_size+i, total-i);
		BUG_ON(ret < 0);

		if (last && ret > 1)
			BUG();

		leaf = path->nodes[0];
		for (c = 0; c < ret; c++) {
			int ref_first = keys[i].type == BTRFS_EXTENT_REF_KEY;

			/*
			 * if the first item we inserted was a backref, then
			 * the EXTENT_ITEM will be the odd c's, else it will
			 * be the even c's
			 */
			if ((ref_first && (c % 2)) ||
			    (!ref_first && !(c % 2))) {
				struct btrfs_extent_item *itm;

				itm = btrfs_item_ptr(leaf, path->slots[0] + c,
						     struct btrfs_extent_item);
				btrfs_set_extent_refs(path->nodes[0], itm, 1);
				op->del++;
			} else {
				struct btrfs_extent_ref *ref;

				ref = btrfs_item_ptr(leaf, path->slots[0] + c,
						     struct btrfs_extent_ref);
				btrfs_set_ref_root(leaf, ref, ref_root);
				btrfs_set_ref_generation(leaf, ref,
							 op->generation);
				btrfs_set_ref_objectid(leaf, ref, op->level);
				btrfs_set_ref_num_refs(leaf, ref, 1);
				op->del++;
			}

			/*
			 * using del to see when its ok to free up the
			 * pending_extent_op.  In the case where we insert the
			 * last item on the list in order to help do batching
			 * we need to not free the extent op until we actually
			 * insert the extent_item
			 */
			if (op->del == 2) {
				unlock_extent(&info->extent_ins, op->bytenr,
					      op->bytenr + op->num_bytes - 1,
					      GFP_NOFS);
				cur = cur->next;
				list_del_init(&op->list);
				kfree(op);
				if (cur != insert_list)
					op = list_entry(cur,
						struct pending_extent_op,
						list);
			}
		}
		btrfs_mark_buffer_dirty(leaf);
		btrfs_release_path(extent_root, path);

		/*
		 * Ok backref's and items usually go right next to eachother,
		 * but if we could only insert 1 item that means that we
		 * inserted on the end of a leaf, and we have no idea what may
		 * be on the next leaf so we just play it safe.  In order to
		 * try and help this case we insert the last thing on our
		 * insert list so hopefully it will end up being the last
		 * thing on the leaf and everything else will be before it,
		 * which will let us insert a whole bunch of items at the same
		 * time.
		 */
		if (ret == 1 && !last && (i + ret < total)) {
			/*
			 * last: where we will pick up the next time around
			 * i: our current key to insert, will be total - 1
			 * cur: the current op we are screwing with
			 * op: duh
			 */
			last = i + ret;
			i = total - 1;
			cur = insert_list->prev;
			op = list_entry(cur, struct pending_extent_op, list);
		} else if (last) {
			/*
			 * ok we successfully inserted the last item on the
			 * list, lets reset everything
			 *
			 * i: our current key to insert, so where we left off
			 *    last time
			 * last: done with this
			 * cur: the op we are messing with
			 * op: duh
			 * total: since we inserted the last key, we need to
			 *        decrement total so we dont overflow
			 */
			i = last;
			last = 0;
			total--;
			if (i < total) {
				cur = insert_list->next;
				op = list_entry(cur, struct pending_extent_op,
						list);
			}
		} else {
			i += ret;
		}

		cond_resched();
	}
	ret = 0;
	kfree(keys);
	kfree(data_size);
	return ret;
}

static noinline int insert_extent_backref(struct btrfs_trans_handle *trans,
					  struct btrfs_root *root,
					  struct btrfs_path *path,
					  u64 bytenr, u64 parent,
					  u64 ref_root, u64 ref_generation,
					  u64 owner_objectid)
{
	struct btrfs_key key;
	struct extent_buffer *leaf;
	struct btrfs_extent_ref *ref;
	u32 num_refs;
	int ret;

	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_REF_KEY;
	key.offset = parent;

	ret = btrfs_insert_empty_item(trans, root, path, &key, sizeof(*ref));
	if (ret == 0) {
		leaf = path->nodes[0];
		ref = btrfs_item_ptr(leaf, path->slots[0],
				     struct btrfs_extent_ref);
		btrfs_set_ref_root(leaf, ref, ref_root);
		btrfs_set_ref_generation(leaf, ref, ref_generation);
		btrfs_set_ref_objectid(leaf, ref, owner_objectid);
		btrfs_set_ref_num_refs(leaf, ref, 1);
	} else if (ret == -EEXIST) {
		u64 existing_owner;
		BUG_ON(owner_objectid < BTRFS_FIRST_FREE_OBJECTID);
		leaf = path->nodes[0];
		ref = btrfs_item_ptr(leaf, path->slots[0],
				     struct btrfs_extent_ref);
		if (btrfs_ref_root(leaf, ref) != ref_root ||
		    btrfs_ref_generation(leaf, ref) != ref_generation) {
			ret = -EIO;
			WARN_ON(1);
			goto out;
		}

		num_refs = btrfs_ref_num_refs(leaf, ref);
		BUG_ON(num_refs == 0);
		btrfs_set_ref_num_refs(leaf, ref, num_refs + 1);

		existing_owner = btrfs_ref_objectid(leaf, ref);
		if (existing_owner != owner_objectid &&
		    existing_owner != BTRFS_MULTIPLE_OBJECTIDS) {
			btrfs_set_ref_objectid(leaf, ref,
					BTRFS_MULTIPLE_OBJECTIDS);
		}
		ret = 0;
	} else {
		goto out;
	}
	btrfs_mark_buffer_dirty(path->nodes[0]);
out:
	btrfs_release_path(root, path);
	return ret;
}

static noinline int remove_extent_backref(struct btrfs_trans_handle *trans,
					  struct btrfs_root *root,
					  struct btrfs_path *path)
{
	struct extent_buffer *leaf;
	struct btrfs_extent_ref *ref;
	u32 num_refs;
	int ret = 0;

	leaf = path->nodes[0];
	ref = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_extent_ref);
	num_refs = btrfs_ref_num_refs(leaf, ref);
	BUG_ON(num_refs == 0);
	num_refs -= 1;
	if (num_refs == 0) {
		ret = btrfs_del_item(trans, root, path);
	} else {
		btrfs_set_ref_num_refs(leaf, ref, num_refs);
		btrfs_mark_buffer_dirty(leaf);
	}
	btrfs_release_path(root, path);
	return ret;
}

#ifdef BIO_RW_DISCARD
static void btrfs_issue_discard(struct block_device *bdev,
				u64 start, u64 len)
{
	blkdev_issue_discard(bdev, start >> 9, len >> 9, GFP_KERNEL);
}
#endif

static int btrfs_discard_extent(struct btrfs_root *root, u64 bytenr,
				u64 num_bytes)
{
#ifdef BIO_RW_DISCARD
	int ret;
	u64 map_length = num_bytes;
	struct btrfs_multi_bio *multi = NULL;

	/* Tell the block device(s) that the sectors can be discarded */
	ret = btrfs_map_block(&root->fs_info->mapping_tree, READ,
			      bytenr, &map_length, &multi, 0);
	if (!ret) {
		struct btrfs_bio_stripe *stripe = multi->stripes;
		int i;

		if (map_length > num_bytes)
			map_length = num_bytes;

		for (i = 0; i < multi->num_stripes; i++, stripe++) {
			btrfs_issue_discard(stripe->dev->bdev,
					    stripe->physical,
					    map_length);
		}
		kfree(multi);
	}

	return ret;
#else
	return 0;
#endif
}

static noinline int free_extents(struct btrfs_trans_handle *trans,
				 struct btrfs_root *extent_root,
				 struct list_head *del_list)
{
	struct btrfs_fs_info *info = extent_root->fs_info;
	struct btrfs_path *path;
	struct btrfs_key key, found_key;
	struct extent_buffer *leaf;
	struct list_head *cur;
	struct pending_extent_op *op;
	struct btrfs_extent_item *ei;
	int ret, num_to_del, extent_slot = 0, found_extent = 0;
	u32 refs;
	u64 bytes_freed = 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	path->reada = 1;

search:
	/* search for the backref for the current ref we want to delete */
	cur = del_list->next;
	op = list_entry(cur, struct pending_extent_op, list);
	ret = lookup_extent_backref(trans, extent_root, path, op->bytenr,
				    op->orig_parent,
				    extent_root->root_key.objectid,
				    op->orig_generation, op->level, 1);
	if (ret) {
		printk(KERN_ERR "btrfs unable to find backref byte nr %llu "
		       "root %llu gen %llu owner %u\n",
		       (unsigned long long)op->bytenr,
		       (unsigned long long)extent_root->root_key.objectid,
		       (unsigned long long)op->orig_generation, op->level);
		btrfs_print_leaf(extent_root, path->nodes[0]);
		WARN_ON(1);
		goto out;
	}

	extent_slot = path->slots[0];
	num_to_del = 1;
	found_extent = 0;

	/*
	 * if we aren't the first item on the leaf we can move back one and see
	 * if our ref is right next to our extent item
	 */
	if (likely(extent_slot)) {
		extent_slot--;
		btrfs_item_key_to_cpu(path->nodes[0], &found_key,
				      extent_slot);
		if (found_key.objectid == op->bytenr &&
		    found_key.type == BTRFS_EXTENT_ITEM_KEY &&
		    found_key.offset == op->num_bytes) {
			num_to_del++;
			found_extent = 1;
		}
	}

	/*
	 * if we didn't find the extent we need to delete the backref and then
	 * search for the extent item key so we can update its ref count
	 */
	if (!found_extent) {
		key.objectid = op->bytenr;
		key.type = BTRFS_EXTENT_ITEM_KEY;
		key.offset = op->num_bytes;

		ret = remove_extent_backref(trans, extent_root, path);
		BUG_ON(ret);
		btrfs_release_path(extent_root, path);
		ret = btrfs_search_slot(trans, extent_root, &key, path, -1, 1);
		BUG_ON(ret);
		extent_slot = path->slots[0];
	}

	/* this is where we update the ref count for the extent */
	leaf = path->nodes[0];
	ei = btrfs_item_ptr(leaf, extent_slot, struct btrfs_extent_item);
	refs = btrfs_extent_refs(leaf, ei);
	BUG_ON(refs == 0);
	refs--;
	btrfs_set_extent_refs(leaf, ei, refs);

	btrfs_mark_buffer_dirty(leaf);

	/*
	 * This extent needs deleting.  The reason cur_slot is extent_slot +
	 * num_to_del is because extent_slot points to the slot where the extent
	 * is, and if the backref was not right next to the extent we will be
	 * deleting at least 1 item, and will want to start searching at the
	 * slot directly next to extent_slot.  However if we did find the
	 * backref next to the extent item them we will be deleting at least 2
	 * items and will want to start searching directly after the ref slot
	 */
	if (!refs) {
		struct list_head *pos, *n, *end;
		int cur_slot = extent_slot+num_to_del;
		u64 super_used;
		u64 root_used;

		path->slots[0] = extent_slot;
		bytes_freed = op->num_bytes;

		mutex_lock(&info->pinned_mutex);
		ret = pin_down_bytes(trans, extent_root, op->bytenr,
				     op->num_bytes, op->level >=
				     BTRFS_FIRST_FREE_OBJECTID);
		mutex_unlock(&info->pinned_mutex);
		BUG_ON(ret < 0);
		op->del = ret;

		/*
		 * we need to see if we can delete multiple things at once, so
		 * start looping through the list of extents we are wanting to
		 * delete and see if their extent/backref's are right next to
		 * eachother and the extents only have 1 ref
		 */
		for (pos = cur->next; pos != del_list; pos = pos->next) {
			struct pending_extent_op *tmp;

			tmp = list_entry(pos, struct pending_extent_op, list);

			/* we only want to delete extent+ref at this stage */
			if (cur_slot >= btrfs_header_nritems(leaf) - 1)
				break;

			btrfs_item_key_to_cpu(leaf, &found_key, cur_slot);
			if (found_key.objectid != tmp->bytenr ||
			    found_key.type != BTRFS_EXTENT_ITEM_KEY ||
			    found_key.offset != tmp->num_bytes)
				break;

			/* check to make sure this extent only has one ref */
			ei = btrfs_item_ptr(leaf, cur_slot,
					    struct btrfs_extent_item);
			if (btrfs_extent_refs(leaf, ei) != 1)
				break;

			btrfs_item_key_to_cpu(leaf, &found_key, cur_slot+1);
			if (found_key.objectid != tmp->bytenr ||
			    found_key.type != BTRFS_EXTENT_REF_KEY ||
			    found_key.offset != tmp->orig_parent)
				break;

			/*
			 * the ref is right next to the extent, we can set the
			 * ref count to 0 since we will delete them both now
			 */
			btrfs_set_extent_refs(leaf, ei, 0);

			/* pin down the bytes for this extent */
			mutex_lock(&info->pinned_mutex);
			ret = pin_down_bytes(trans, extent_root, tmp->bytenr,
					     tmp->num_bytes, tmp->level >=
					     BTRFS_FIRST_FREE_OBJECTID);
			mutex_unlock(&info->pinned_mutex);
			BUG_ON(ret < 0);

			/*
			 * use the del field to tell if we need to go ahead and
			 * free up the extent when we delete the item or not.
			 */
			tmp->del = ret;
			bytes_freed += tmp->num_bytes;

			num_to_del += 2;
			cur_slot += 2;
		}
		end = pos;

		/* update the free space counters */
		spin_lock(&info->delalloc_lock);
		super_used = btrfs_super_bytes_used(&info->super_copy);
		btrfs_set_super_bytes_used(&info->super_copy,
					   super_used - bytes_freed);

		root_used = btrfs_root_used(&extent_root->root_item);
		btrfs_set_root_used(&extent_root->root_item,
				    root_used - bytes_freed);
		spin_unlock(&info->delalloc_lock);

		/* delete the items */
		ret = btrfs_del_items(trans, extent_root, path,
				      path->slots[0], num_to_del);
		BUG_ON(ret);

		/*
		 * loop through the extents we deleted and do the cleanup work
		 * on them
		 */
		for (pos = cur, n = pos->next; pos != end;
		     pos = n, n = pos->next) {
			struct pending_extent_op *tmp;
			tmp = list_entry(pos, struct pending_extent_op, list);

			/*
			 * remember tmp->del tells us wether or not we pinned
			 * down the extent
			 */
			ret = update_block_group(trans, extent_root,
						 tmp->bytenr, tmp->num_bytes, 0,
						 tmp->del);
			BUG_ON(ret);

			list_del_init(&tmp->list);
			unlock_extent(&info->extent_ins, tmp->bytenr,
				      tmp->bytenr + tmp->num_bytes - 1,
				      GFP_NOFS);
			kfree(tmp);
		}
	} else if (refs && found_extent) {
		/*
		 * the ref and extent were right next to eachother, but the
		 * extent still has a ref, so just free the backref and keep
		 * going
		 */
		ret = remove_extent_backref(trans, extent_root, path);
		BUG_ON(ret);

		list_del_init(&op->list);
		unlock_extent(&info->extent_ins, op->bytenr,
			      op->bytenr + op->num_bytes - 1, GFP_NOFS);
		kfree(op);
	} else {
		/*
		 * the extent has multiple refs and the backref we were looking
		 * for was not right next to it, so just unlock and go next,
		 * we're good to go
		 */
		list_del_init(&op->list);
		unlock_extent(&info->extent_ins, op->bytenr,
			      op->bytenr + op->num_bytes - 1, GFP_NOFS);
		kfree(op);
	}

	btrfs_release_path(extent_root, path);
	if (!list_empty(del_list))
		goto search;

out:
	btrfs_free_path(path);
	return ret;
}

static int __btrfs_update_extent_ref(struct btrfs_trans_handle *trans,
				     struct btrfs_root *root, u64 bytenr,
				     u64 orig_parent, u64 parent,
				     u64 orig_root, u64 ref_root,
				     u64 orig_generation, u64 ref_generation,
				     u64 owner_objectid)
{
	int ret;
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	struct btrfs_path *path;

	if (root == root->fs_info->extent_root) {
		struct pending_extent_op *extent_op;
		u64 num_bytes;

		BUG_ON(owner_objectid >= BTRFS_MAX_LEVEL);
		num_bytes = btrfs_level_size(root, (int)owner_objectid);
		mutex_lock(&root->fs_info->extent_ins_mutex);
		if (test_range_bit(&root->fs_info->extent_ins, bytenr,
				bytenr + num_bytes - 1, EXTENT_WRITEBACK, 0)) {
			u64 priv;
			ret = get_state_private(&root->fs_info->extent_ins,
						bytenr, &priv);
			BUG_ON(ret);
			extent_op = (struct pending_extent_op *)
							(unsigned long)priv;
			BUG_ON(extent_op->parent != orig_parent);
			BUG_ON(extent_op->generation != orig_generation);

			extent_op->parent = parent;
			extent_op->generation = ref_generation;
		} else {
			extent_op = kmalloc(sizeof(*extent_op), GFP_NOFS);
			BUG_ON(!extent_op);

			extent_op->type = PENDING_BACKREF_UPDATE;
			extent_op->bytenr = bytenr;
			extent_op->num_bytes = num_bytes;
			extent_op->parent = parent;
			extent_op->orig_parent = orig_parent;
			extent_op->generation = ref_generation;
			extent_op->orig_generation = orig_generation;
			extent_op->level = (int)owner_objectid;
			INIT_LIST_HEAD(&extent_op->list);
			extent_op->del = 0;

			set_extent_bits(&root->fs_info->extent_ins,
					bytenr, bytenr + num_bytes - 1,
					EXTENT_WRITEBACK, GFP_NOFS);
			set_state_private(&root->fs_info->extent_ins,
					  bytenr, (unsigned long)extent_op);
		}
		mutex_unlock(&root->fs_info->extent_ins_mutex);
		return 0;
	}

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	ret = lookup_extent_backref(trans, extent_root, path,
				    bytenr, orig_parent, orig_root,
				    orig_generation, owner_objectid, 1);
	if (ret)
		goto out;
	ret = remove_extent_backref(trans, extent_root, path);
	if (ret)
		goto out;
	ret = insert_extent_backref(trans, extent_root, path, bytenr,
				    parent, ref_root, ref_generation,
				    owner_objectid);
	BUG_ON(ret);
	finish_current_insert(trans, extent_root, 0);
	del_pending_extents(trans, extent_root, 0);
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_update_extent_ref(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root, u64 bytenr,
			    u64 orig_parent, u64 parent,
			    u64 ref_root, u64 ref_generation,
			    u64 owner_objectid)
{
	int ret;
	if (ref_root == BTRFS_TREE_LOG_OBJECTID &&
	    owner_objectid < BTRFS_FIRST_FREE_OBJECTID)
		return 0;
	ret = __btrfs_update_extent_ref(trans, root, bytenr, orig_parent,
					parent, ref_root, ref_root,
					ref_generation, ref_generation,
					owner_objectid);
	return ret;
}

static int __btrfs_inc_extent_ref(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root, u64 bytenr,
				  u64 orig_parent, u64 parent,
				  u64 orig_root, u64 ref_root,
				  u64 orig_generation, u64 ref_generation,
				  u64 owner_objectid)
{
	struct btrfs_path *path;
	int ret;
	struct btrfs_key key;
	struct extent_buffer *l;
	struct btrfs_extent_item *item;
	u32 refs;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 1;
	key.objectid = bytenr;
	key.type = BTRFS_EXTENT_ITEM_KEY;
	key.offset = (u64)-1;

	ret = btrfs_search_slot(trans, root->fs_info->extent_root, &key, path,
				0, 1);
	if (ret < 0)
		return ret;
	BUG_ON(ret == 0 || path->slots[0] == 0);

	path->slots[0]--;
	l = path->nodes[0];

	btrfs_item_key_to_cpu(l, &key, path->slots[0]);
	if (key.objectid != bytenr) {
		btrfs_print_leaf(root->fs_info->extent_root, path->nodes[0]);
		printk(KERN_ERR "btrfs wanted %llu found %llu\n",
		       (unsigned long long)bytenr,
		       (unsigned long long)key.objectid);
		BUG();
	}
	BUG_ON(key.type != BTRFS_EXTENT_ITEM_KEY);

	item = btrfs_item_ptr(l, path->slots[0], struct btrfs_extent_item);
	refs = btrfs_extent_refs(l, item);
	btrfs_set_extent_refs(l, item, refs + 1);
	btrfs_mark_buffer_dirty(path->nodes[0]);

	btrfs_release_path(root->fs_info->extent_root, path);

	path->reada = 1;
	ret = insert_extent_backref(trans, root->fs_info->extent_root,
				    path, bytenr, parent,
				    ref_root, ref_generation,
				    owner_objectid);
	BUG_ON(ret);
	finish_current_insert(trans, root->fs_info->extent_root, 0);
	del_pending_extents(trans, root->fs_info->extent_root, 0);

	btrfs_free_path(path);
	return 0;
}

int btrfs_inc_extent_ref(struct btrfs_trans_handle *trans,
			 struct btrfs_root *root,
			 u64 bytenr, u64 num_bytes, u64 parent,
			 u64 ref_root, u64 ref_generation,
			 u64 owner_objectid)
{
	int ret;
	if (ref_root == BTRFS_TREE_LOG_OBJECTID &&
	    owner_objectid < BTRFS_FIRST_FREE_OBJECTID)
		return 0;
	ret = __btrfs_inc_extent_ref(trans, root, bytenr, 0, parent,
				     0, ref_root, 0, ref_generation,
				     owner_objectid);
	return ret;
}

int btrfs_extent_post_op(struct btrfs_trans_handle *trans,
			 struct btrfs_root *root)
{
	u64 start;
	u64 end;
	int ret;

	while(1) {
		finish_current_insert(trans, root->fs_info->extent_root, 1);
		del_pending_extents(trans, root->fs_info->extent_root, 1);

		/* is there more work to do? */
		ret = find_first_extent_bit(&root->fs_info->pending_del,
					    0, &start, &end, EXTENT_WRITEBACK);
		if (!ret)
			continue;
		ret = find_first_extent_bit(&root->fs_info->extent_ins,
					    0, &start, &end, EXTENT_WRITEBACK);
		if (!ret)
			continue;
		break;
	}
	return 0;
}

int btrfs_lookup_extent_ref(struct btrfs_trans_handle *trans,
			    struct btrfs_root *root, u64 bytenr,
			    u64 num_bytes, u32 *refs)
{
	struct btrfs_path *path;
	int ret;
	struct btrfs_key key;
	struct extent_buffer *l;
	struct btrfs_extent_item *item;

	WARN_ON(num_bytes < root->sectorsize);
	path = btrfs_alloc_path();
	path->reada = 1;
	key.objectid = bytenr;
	key.offset = num_bytes;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	ret = btrfs_search_slot(trans, root->fs_info->extent_root, &key, path,
				0, 0);
	if (ret < 0)
		goto out;
	if (ret != 0) {
		btrfs_print_leaf(root, path->nodes[0]);
		printk(KERN_INFO "btrfs failed to find block number %llu\n",
		       (unsigned long long)bytenr);
		BUG();
	}
	l = path->nodes[0];
	item = btrfs_item_ptr(l, path->slots[0], struct btrfs_extent_item);
	*refs = btrfs_extent_refs(l, item);
out:
	btrfs_free_path(path);
	return 0;
}

int btrfs_cross_ref_exist(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root, u64 objectid, u64 bytenr)
{
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	struct btrfs_path *path;
	struct extent_buffer *leaf;
	struct btrfs_extent_ref *ref_item;
	struct btrfs_key key;
	struct btrfs_key found_key;
	u64 ref_root;
	u64 last_snapshot;
	u32 nritems;
	int ret;

	key.objectid = bytenr;
	key.offset = (u64)-1;
	key.type = BTRFS_EXTENT_ITEM_KEY;

	path = btrfs_alloc_path();
	ret = btrfs_search_slot(NULL, extent_root, &key, path, 0, 0);
	if (ret < 0)
		goto out;
	BUG_ON(ret == 0);

	ret = -ENOENT;
	if (path->slots[0] == 0)
		goto out;

	path->slots[0]--;
	leaf = path->nodes[0];
	btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);

	if (found_key.objectid != bytenr ||
	    found_key.type != BTRFS_EXTENT_ITEM_KEY)
		goto out;

	last_snapshot = btrfs_root_last_snapshot(&root->root_item);
	while (1) {
		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(extent_root, path);
			if (ret < 0)
				goto out;
			if (ret == 0)
				continue;
			break;
		}
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.objectid != bytenr)
			break;

		if (found_key.type != BTRFS_EXTENT_REF_KEY) {
			path->slots[0]++;
			continue;
		}

		ref_item = btrfs_item_ptr(leaf, path->slots[0],
					  struct btrfs_extent_ref);
		ref_root = btrfs_ref_root(leaf, ref_item);
		if ((ref_root != root->root_key.objectid &&
		     ref_root != BTRFS_TREE_LOG_OBJECTID) ||
		     objectid != btrfs_ref_objectid(leaf, ref_item)) {
			ret = 1;
			goto out;
		}
		if (btrfs_ref_generation(leaf, ref_item) <= last_snapshot) {
			ret = 1;
			goto out;
		}

		path->slots[0]++;
	}
	ret = 0;
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_cache_ref(struct btrfs_trans_handle *trans, struct btrfs_root *root,
		    struct extent_buffer *buf, u32 nr_extents)
{
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	u64 root_gen;
	u32 nritems;
	int i;
	int level;
	int ret = 0;
	int shared = 0;

	if (!root->ref_cows)
		return 0;

	if (root->root_key.objectid != BTRFS_TREE_RELOC_OBJECTID) {
		shared = 0;
		root_gen = root->root_key.offset;
	} else {
		shared = 1;
		root_gen = trans->transid - 1;
	}

	level = btrfs_header_level(buf);
	nritems = btrfs_header_nritems(buf);

	if (level == 0) {
		struct btrfs_leaf_ref *ref;
		struct btrfs_extent_info *info;

		ref = btrfs_alloc_leaf_ref(root, nr_extents);
		if (!ref) {
			ret = -ENOMEM;
			goto out;
		}

		ref->root_gen = root_gen;
		ref->bytenr = buf->start;
		ref->owner = btrfs_header_owner(buf);
		ref->generation = btrfs_header_generation(buf);
		ref->nritems = nr_extents;
		info = ref->extents;

		for (i = 0; nr_extents > 0 && i < nritems; i++) {
			u64 disk_bytenr;
			btrfs_item_key_to_cpu(buf, &key, i);
			if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
				continue;
			fi = btrfs_item_ptr(buf, i,
					    struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(buf, fi) ==
			    BTRFS_FILE_EXTENT_INLINE)
				continue;
			disk_bytenr = btrfs_file_extent_disk_bytenr(buf, fi);
			if (disk_bytenr == 0)
				continue;

			info->bytenr = disk_bytenr;
			info->num_bytes =
				btrfs_file_extent_disk_num_bytes(buf, fi);
			info->objectid = key.objectid;
			info->offset = key.offset;
			info++;
		}

		ret = btrfs_add_leaf_ref(root, ref, shared);
		if (ret == -EEXIST && shared) {
			struct btrfs_leaf_ref *old;
			old = btrfs_lookup_leaf_ref(root, ref->bytenr);
			BUG_ON(!old);
			btrfs_remove_leaf_ref(root, old);
			btrfs_free_leaf_ref(root, old);
			ret = btrfs_add_leaf_ref(root, ref, shared);
		}
		WARN_ON(ret);
		btrfs_free_leaf_ref(root, ref);
	}
out:
	return ret;
}

/* when a block goes through cow, we update the reference counts of
 * everything that block points to.  The internal pointers of the block
 * can be in just about any order, and it is likely to have clusters of
 * things that are close together and clusters of things that are not.
 *
 * To help reduce the seeks that come with updating all of these reference
 * counts, sort them by byte number before actual updates are done.
 *
 * struct refsort is used to match byte number to slot in the btree block.
 * we sort based on the byte number and then use the slot to actually
 * find the item.
 *
 * struct refsort is smaller than strcut btrfs_item and smaller than
 * struct btrfs_key_ptr.  Since we're currently limited to the page size
 * for a btree block, there's no way for a kmalloc of refsorts for a
 * single node to be bigger than a page.
 */
struct refsort {
	u64 bytenr;
	u32 slot;
};

/*
 * for passing into sort()
 */
static int refsort_cmp(const void *a_void, const void *b_void)
{
	const struct refsort *a = a_void;
	const struct refsort *b = b_void;

	if (a->bytenr < b->bytenr)
		return -1;
	if (a->bytenr > b->bytenr)
		return 1;
	return 0;
}


noinline int btrfs_inc_ref(struct btrfs_trans_handle *trans,
			   struct btrfs_root *root,
			   struct extent_buffer *orig_buf,
			   struct extent_buffer *buf, u32 *nr_extents)
{
	u64 bytenr;
	u64 ref_root;
	u64 orig_root;
	u64 ref_generation;
	u64 orig_generation;
	struct refsort *sorted;
	u32 nritems;
	u32 nr_file_extents = 0;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int i;
	int level;
	int ret = 0;
	int faili = 0;
	int refi = 0;
	int slot;
	int (*process_func)(struct btrfs_trans_handle *, struct btrfs_root *,
			    u64, u64, u64, u64, u64, u64, u64, u64);

	ref_root = btrfs_header_owner(buf);
	ref_generation = btrfs_header_generation(buf);
	orig_root = btrfs_header_owner(orig_buf);
	orig_generation = btrfs_header_generation(orig_buf);

	nritems = btrfs_header_nritems(buf);
	level = btrfs_header_level(buf);

	sorted = kmalloc(sizeof(struct refsort) * nritems, GFP_NOFS);
	BUG_ON(!sorted);

	if (root->ref_cows) {
		process_func = __btrfs_inc_extent_ref;
	} else {
		if (level == 0 &&
		    root->root_key.objectid != BTRFS_TREE_LOG_OBJECTID)
			goto out;
		if (level != 0 &&
		    root->root_key.objectid == BTRFS_TREE_LOG_OBJECTID)
			goto out;
		process_func = __btrfs_update_extent_ref;
	}

	/*
	 * we make two passes through the items.  In the first pass we
	 * only record the byte number and slot.  Then we sort based on
	 * byte number and do the actual work based on the sorted results
	 */
	for (i = 0; i < nritems; i++) {
		cond_resched();
		if (level == 0) {
			btrfs_item_key_to_cpu(buf, &key, i);
			if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
				continue;
			fi = btrfs_item_ptr(buf, i,
					    struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(buf, fi) ==
			    BTRFS_FILE_EXTENT_INLINE)
				continue;
			bytenr = btrfs_file_extent_disk_bytenr(buf, fi);
			if (bytenr == 0)
				continue;

			nr_file_extents++;
			sorted[refi].bytenr = bytenr;
			sorted[refi].slot = i;
			refi++;
		} else {
			bytenr = btrfs_node_blockptr(buf, i);
			sorted[refi].bytenr = bytenr;
			sorted[refi].slot = i;
			refi++;
		}
	}
	/*
	 * if refi == 0, we didn't actually put anything into the sorted
	 * array and we're done
	 */
	if (refi == 0)
		goto out;

	sort(sorted, refi, sizeof(struct refsort), refsort_cmp, NULL);

	for (i = 0; i < refi; i++) {
		cond_resched();
		slot = sorted[i].slot;
		bytenr = sorted[i].bytenr;

		if (level == 0) {
			btrfs_item_key_to_cpu(buf, &key, slot);

			ret = process_func(trans, root, bytenr,
					   orig_buf->start, buf->start,
					   orig_root, ref_root,
					   orig_generation, ref_generation,
					   key.objectid);

			if (ret) {
				faili = slot;
				WARN_ON(1);
				goto fail;
			}
		} else {
			ret = process_func(trans, root, bytenr,
					   orig_buf->start, buf->start,
					   orig_root, ref_root,
					   orig_generation, ref_generation,
					   level - 1);
			if (ret) {
				faili = slot;
				WARN_ON(1);
				goto fail;
			}
		}
	}
out:
	kfree(sorted);
	if (nr_extents) {
		if (level == 0)
			*nr_extents = nr_file_extents;
		else
			*nr_extents = nritems;
	}
	return 0;
fail:
	kfree(sorted);
	WARN_ON(1);
	return ret;
}

int btrfs_update_ref(struct btrfs_trans_handle *trans,
		     struct btrfs_root *root, struct extent_buffer *orig_buf,
		     struct extent_buffer *buf, int start_slot, int nr)

{
	u64 bytenr;
	u64 ref_root;
	u64 orig_root;
	u64 ref_generation;
	u64 orig_generation;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int i;
	int ret;
	int slot;
	int level;

	BUG_ON(start_slot < 0);
	BUG_ON(start_slot + nr > btrfs_header_nritems(buf));

	ref_root = btrfs_header_owner(buf);
	ref_generation = btrfs_header_generation(buf);
	orig_root = btrfs_header_owner(orig_buf);
	orig_generation = btrfs_header_generation(orig_buf);
	level = btrfs_header_level(buf);

	if (!root->ref_cows) {
		if (level == 0 &&
		    root->root_key.objectid != BTRFS_TREE_LOG_OBJECTID)
			return 0;
		if (level != 0 &&
		    root->root_key.objectid == BTRFS_TREE_LOG_OBJECTID)
			return 0;
	}

	for (i = 0, slot = start_slot; i < nr; i++, slot++) {
		cond_resched();
		if (level == 0) {
			btrfs_item_key_to_cpu(buf, &key, slot);
			if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
				continue;
			fi = btrfs_item_ptr(buf, slot,
					    struct btrfs_file_extent_item);
			if (btrfs_file_extent_type(buf, fi) ==
			    BTRFS_FILE_EXTENT_INLINE)
				continue;
			bytenr = btrfs_file_extent_disk_bytenr(buf, fi);
			if (bytenr == 0)
				continue;
			ret = __btrfs_update_extent_ref(trans, root, bytenr,
					    orig_buf->start, buf->start,
					    orig_root, ref_root,
					    orig_generation, ref_generation,
					    key.objectid);
			if (ret)
				goto fail;
		} else {
			bytenr = btrfs_node_blockptr(buf, slot);
			ret = __btrfs_update_extent_ref(trans, root, bytenr,
					    orig_buf->start, buf->start,
					    orig_root, ref_root,
					    orig_generation, ref_generation,
					    level - 1);
			if (ret)
				goto fail;
		}
	}
	return 0;
fail:
	WARN_ON(1);
	return -1;
}

static int write_one_cache_group(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 struct btrfs_path *path,
				 struct btrfs_block_group_cache *cache)
{
	int ret;
	int pending_ret;
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	unsigned long bi;
	struct extent_buffer *leaf;

	ret = btrfs_search_slot(trans, extent_root, &cache->key, path, 0, 1);
	if (ret < 0)
		goto fail;
	BUG_ON(ret);

	leaf = path->nodes[0];
	bi = btrfs_item_ptr_offset(leaf, path->slots[0]);
	write_extent_buffer(leaf, &cache->item, bi, sizeof(cache->item));
	btrfs_mark_buffer_dirty(leaf);
	btrfs_release_path(extent_root, path);
fail:
	finish_current_insert(trans, extent_root, 0);
	pending_ret = del_pending_extents(trans, extent_root, 0);
	if (ret)
		return ret;
	if (pending_ret)
		return pending_ret;
	return 0;

}

int btrfs_write_dirty_block_groups(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root)
{
	struct btrfs_block_group_cache *cache, *entry;
	struct rb_node *n;
	int err = 0;
	int werr = 0;
	struct btrfs_path *path;
	u64 last = 0;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	while (1) {
		cache = NULL;
		spin_lock(&root->fs_info->block_group_cache_lock);
		for (n = rb_first(&root->fs_info->block_group_cache_tree);
		     n; n = rb_next(n)) {
			entry = rb_entry(n, struct btrfs_block_group_cache,
					 cache_node);
			if (entry->dirty) {
				cache = entry;
				break;
			}
		}
		spin_unlock(&root->fs_info->block_group_cache_lock);

		if (!cache)
			break;

		cache->dirty = 0;
		last += cache->key.offset;

		err = write_one_cache_group(trans, root,
					    path, cache);
		/*
		 * if we fail to write the cache group, we want
		 * to keep it marked dirty in hopes that a later
		 * write will work
		 */
		if (err) {
			werr = err;
			continue;
		}
	}
	btrfs_free_path(path);
	return werr;
}

int btrfs_extent_readonly(struct btrfs_root *root, u64 bytenr)
{
	struct btrfs_block_group_cache *block_group;
	int readonly = 0;

	block_group = btrfs_lookup_block_group(root->fs_info, bytenr);
	if (!block_group || block_group->ro)
		readonly = 1;
	if (block_group)
		put_block_group(block_group);
	return readonly;
}

static int update_space_info(struct btrfs_fs_info *info, u64 flags,
			     u64 total_bytes, u64 bytes_used,
			     struct btrfs_space_info **space_info)
{
	struct btrfs_space_info *found;

	found = __find_space_info(info, flags);
	if (found) {
		spin_lock(&found->lock);
		found->total_bytes += total_bytes;
		found->bytes_used += bytes_used;
		found->full = 0;
		spin_unlock(&found->lock);
		*space_info = found;
		return 0;
	}
	found = kzalloc(sizeof(*found), GFP_NOFS);
	if (!found)
		return -ENOMEM;

	INIT_LIST_HEAD(&found->block_groups);
	init_rwsem(&found->groups_sem);
	spin_lock_init(&found->lock);
	found->flags = flags;
	found->total_bytes = total_bytes;
	found->bytes_used = bytes_used;
	found->bytes_pinned = 0;
	found->bytes_reserved = 0;
	found->bytes_readonly = 0;
	found->bytes_delalloc = 0;
	found->full = 0;
	found->force_alloc = 0;
	*space_info = found;
	list_add_rcu(&found->list, &info->space_info);
	return 0;
}

static void set_avail_alloc_bits(struct btrfs_fs_info *fs_info, u64 flags)
{
	u64 extra_flags = flags & (BTRFS_BLOCK_GROUP_RAID0 |
				   BTRFS_BLOCK_GROUP_RAID1 |
				   BTRFS_BLOCK_GROUP_RAID10 |
				   BTRFS_BLOCK_GROUP_DUP);
	if (extra_flags) {
		if (flags & BTRFS_BLOCK_GROUP_DATA)
			fs_info->avail_data_alloc_bits |= extra_flags;
		if (flags & BTRFS_BLOCK_GROUP_METADATA)
			fs_info->avail_metadata_alloc_bits |= extra_flags;
		if (flags & BTRFS_BLOCK_GROUP_SYSTEM)
			fs_info->avail_system_alloc_bits |= extra_flags;
	}
}

static void set_block_group_readonly(struct btrfs_block_group_cache *cache)
{
	spin_lock(&cache->space_info->lock);
	spin_lock(&cache->lock);
	if (!cache->ro) {
		cache->space_info->bytes_readonly += cache->key.offset -
					btrfs_block_group_used(&cache->item);
		cache->ro = 1;
	}
	spin_unlock(&cache->lock);
	spin_unlock(&cache->space_info->lock);
}

u64 btrfs_reduce_alloc_profile(struct btrfs_root *root, u64 flags)
{
	u64 num_devices = root->fs_info->fs_devices->rw_devices;

	if (num_devices == 1)
		flags &= ~(BTRFS_BLOCK_GROUP_RAID1 | BTRFS_BLOCK_GROUP_RAID0);
	if (num_devices < 4)
		flags &= ~BTRFS_BLOCK_GROUP_RAID10;

	if ((flags & BTRFS_BLOCK_GROUP_DUP) &&
	    (flags & (BTRFS_BLOCK_GROUP_RAID1 |
		      BTRFS_BLOCK_GROUP_RAID10))) {
		flags &= ~BTRFS_BLOCK_GROUP_DUP;
	}

	if ((flags & BTRFS_BLOCK_GROUP_RAID1) &&
	    (flags & BTRFS_BLOCK_GROUP_RAID10)) {
		flags &= ~BTRFS_BLOCK_GROUP_RAID1;
	}

	if ((flags & BTRFS_BLOCK_GROUP_RAID0) &&
	    ((flags & BTRFS_BLOCK_GROUP_RAID1) |
	     (flags & BTRFS_BLOCK_GROUP_RAID10) |
	     (flags & BTRFS_BLOCK_GROUP_DUP)))
		flags &= ~BTRFS_BLOCK_GROUP_RAID0;
	return flags;
}

static u64 btrfs_get_alloc_profile(struct btrfs_root *root, u64 data)
{
	struct btrfs_fs_info *info = root->fs_info;
	u64 alloc_profile;

	if (data) {
		alloc_profile = info->avail_data_alloc_bits &
			info->data_alloc_profile;
		data = BTRFS_BLOCK_GROUP_DATA | alloc_profile;
	} else if (root == root->fs_info->chunk_root) {
		alloc_profile = info->avail_system_alloc_bits &
			info->system_alloc_profile;
		data = BTRFS_BLOCK_GROUP_SYSTEM | alloc_profile;
	} else {
		alloc_profile = info->avail_metadata_alloc_bits &
			info->metadata_alloc_profile;
		data = BTRFS_BLOCK_GROUP_METADATA | alloc_profile;
	}

	return btrfs_reduce_alloc_profile(root, data);
}

void btrfs_set_inode_space_info(struct btrfs_root *root, struct inode *inode)
{
	u64 alloc_target;

	alloc_target = btrfs_get_alloc_profile(root, 1);
	BTRFS_I(inode)->space_info = __find_space_info(root->fs_info,
						       alloc_target);
}

/*
 * for now this just makes sure we have at least 5% of our metadata space free
 * for use.
 */
int btrfs_check_metadata_free_space(struct btrfs_root *root)
{
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_space_info *meta_sinfo;
	u64 alloc_target, thresh;
	int committed = 0, ret;

	/* get the space info for where the metadata will live */
	alloc_target = btrfs_get_alloc_profile(root, 0);
	meta_sinfo = __find_space_info(info, alloc_target);

again:
	spin_lock(&meta_sinfo->lock);
	if (!meta_sinfo->full)
		thresh = meta_sinfo->total_bytes * 80;
	else
		thresh = meta_sinfo->total_bytes * 95;

	do_div(thresh, 100);

	if (meta_sinfo->bytes_used + meta_sinfo->bytes_reserved +
	    meta_sinfo->bytes_pinned + meta_sinfo->bytes_readonly > thresh) {
		struct btrfs_trans_handle *trans;
		if (!meta_sinfo->full) {
			meta_sinfo->force_alloc = 1;
			spin_unlock(&meta_sinfo->lock);

			trans = btrfs_start_transaction(root, 1);
			if (!trans)
				return -ENOMEM;

			ret = do_chunk_alloc(trans, root->fs_info->extent_root,
					     2 * 1024 * 1024, alloc_target, 0);
			btrfs_end_transaction(trans, root);
			goto again;
		}
		spin_unlock(&meta_sinfo->lock);

		if (!committed) {
			committed = 1;
			trans = btrfs_join_transaction(root, 1);
			if (!trans)
				return -ENOMEM;
			ret = btrfs_commit_transaction(trans, root);
			if (ret)
				return ret;
			goto again;
		}
		return -ENOSPC;
	}
	spin_unlock(&meta_sinfo->lock);

	return 0;
}

/*
 * This will check the space that the inode allocates from to make sure we have
 * enough space for bytes.
 */
int btrfs_check_data_free_space(struct btrfs_root *root, struct inode *inode,
				u64 bytes)
{
	struct btrfs_space_info *data_sinfo;
	int ret = 0, committed = 0;

	/* make sure bytes are sectorsize aligned */
	bytes = (bytes + root->sectorsize - 1) & ~((u64)root->sectorsize - 1);

	data_sinfo = BTRFS_I(inode)->space_info;
again:
	/* make sure we have enough space to handle the data first */
	spin_lock(&data_sinfo->lock);
	if (data_sinfo->total_bytes - data_sinfo->bytes_used -
	    data_sinfo->bytes_delalloc - data_sinfo->bytes_reserved -
	    data_sinfo->bytes_pinned - data_sinfo->bytes_readonly -
	    data_sinfo->bytes_may_use < bytes) {
		struct btrfs_trans_handle *trans;

		/*
		 * if we don't have enough free bytes in this space then we need
		 * to alloc a new chunk.
		 */
		if (!data_sinfo->full) {
			u64 alloc_target;

			data_sinfo->force_alloc = 1;
			spin_unlock(&data_sinfo->lock);

			alloc_target = btrfs_get_alloc_profile(root, 1);
			trans = btrfs_start_transaction(root, 1);
			if (!trans)
				return -ENOMEM;

			ret = do_chunk_alloc(trans, root->fs_info->extent_root,
					     bytes + 2 * 1024 * 1024,
					     alloc_target, 0);
			btrfs_end_transaction(trans, root);
			if (ret)
				return ret;
			goto again;
		}
		spin_unlock(&data_sinfo->lock);

		/* commit the current transaction and try again */
		if (!committed) {
			committed = 1;
			trans = btrfs_join_transaction(root, 1);
			if (!trans)
				return -ENOMEM;
			ret = btrfs_commit_transaction(trans, root);
			if (ret)
				return ret;
			goto again;
		}

		printk(KERN_ERR "no space left, need %llu, %llu delalloc bytes"
		       ", %llu bytes_used, %llu bytes_reserved, "
		       "%llu bytes_pinned, %llu bytes_readonly, %llu may use"
		       "%llu total\n", bytes, data_sinfo->bytes_delalloc,
		       data_sinfo->bytes_used, data_sinfo->bytes_reserved,
		       data_sinfo->bytes_pinned, data_sinfo->bytes_readonly,
		       data_sinfo->bytes_may_use, data_sinfo->total_bytes);
		return -ENOSPC;
	}
	data_sinfo->bytes_may_use += bytes;
	BTRFS_I(inode)->reserved_bytes += bytes;
	spin_unlock(&data_sinfo->lock);

	return btrfs_check_metadata_free_space(root);
}

/*
 * if there was an error for whatever reason after calling
 * btrfs_check_data_free_space, call this so we can cleanup the counters.
 */
void btrfs_free_reserved_data_space(struct btrfs_root *root,
				    struct inode *inode, u64 bytes)
{
	struct btrfs_space_info *data_sinfo;

	/* make sure bytes are sectorsize aligned */
	bytes = (bytes + root->sectorsize - 1) & ~((u64)root->sectorsize - 1);

	data_sinfo = BTRFS_I(inode)->space_info;
	spin_lock(&data_sinfo->lock);
	data_sinfo->bytes_may_use -= bytes;
	BTRFS_I(inode)->reserved_bytes -= bytes;
	spin_unlock(&data_sinfo->lock);
}

/* called when we are adding a delalloc extent to the inode's io_tree */
void btrfs_delalloc_reserve_space(struct btrfs_root *root, struct inode *inode,
				  u64 bytes)
{
	struct btrfs_space_info *data_sinfo;

	/* get the space info for where this inode will be storing its data */
	data_sinfo = BTRFS_I(inode)->space_info;

	/* make sure we have enough space to handle the data first */
	spin_lock(&data_sinfo->lock);
	data_sinfo->bytes_delalloc += bytes;

	/*
	 * we are adding a delalloc extent without calling
	 * btrfs_check_data_free_space first.  This happens on a weird
	 * writepage condition, but shouldn't hurt our accounting
	 */
	if (unlikely(bytes > BTRFS_I(inode)->reserved_bytes)) {
		data_sinfo->bytes_may_use -= BTRFS_I(inode)->reserved_bytes;
		BTRFS_I(inode)->reserved_bytes = 0;
	} else {
		data_sinfo->bytes_may_use -= bytes;
		BTRFS_I(inode)->reserved_bytes -= bytes;
	}

	spin_unlock(&data_sinfo->lock);
}

/* called when we are clearing an delalloc extent from the inode's io_tree */
void btrfs_delalloc_free_space(struct btrfs_root *root, struct inode *inode,
			      u64 bytes)
{
	struct btrfs_space_info *info;

	info = BTRFS_I(inode)->space_info;

	spin_lock(&info->lock);
	info->bytes_delalloc -= bytes;
	spin_unlock(&info->lock);
}

static int do_chunk_alloc(struct btrfs_trans_handle *trans,
			  struct btrfs_root *extent_root, u64 alloc_bytes,
			  u64 flags, int force)
{
	struct btrfs_space_info *space_info;
	u64 thresh;
	int ret = 0;

	mutex_lock(&extent_root->fs_info->chunk_mutex);

	flags = btrfs_reduce_alloc_profile(extent_root, flags);

	space_info = __find_space_info(extent_root->fs_info, flags);
	if (!space_info) {
		ret = update_space_info(extent_root->fs_info, flags,
					0, 0, &space_info);
		BUG_ON(ret);
	}
	BUG_ON(!space_info);

	spin_lock(&space_info->lock);
	if (space_info->force_alloc) {
		force = 1;
		space_info->force_alloc = 0;
	}
	if (space_info->full) {
		spin_unlock(&space_info->lock);
		goto out;
	}

	thresh = space_info->total_bytes - space_info->bytes_readonly;
	thresh = div_factor(thresh, 6);
	if (!force &&
	   (space_info->bytes_used + space_info->bytes_pinned +
	    space_info->bytes_reserved + alloc_bytes) < thresh) {
		spin_unlock(&space_info->lock);
		goto out;
	}
	spin_unlock(&space_info->lock);

	ret = btrfs_alloc_chunk(trans, extent_root, flags);
	if (ret)
		space_info->full = 1;
out:
	mutex_unlock(&extent_root->fs_info->chunk_mutex);
	return ret;
}

static int update_block_group(struct btrfs_trans_handle *trans,
			      struct btrfs_root *root,
			      u64 bytenr, u64 num_bytes, int alloc,
			      int mark_free)
{
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *info = root->fs_info;
	u64 total = num_bytes;
	u64 old_val;
	u64 byte_in_group;

	while (total) {
		cache = btrfs_lookup_block_group(info, bytenr);
		if (!cache)
			return -1;
		byte_in_group = bytenr - cache->key.objectid;
		WARN_ON(byte_in_group > cache->key.offset);

		spin_lock(&cache->space_info->lock);
		spin_lock(&cache->lock);
		cache->dirty = 1;
		old_val = btrfs_block_group_used(&cache->item);
		num_bytes = min(total, cache->key.offset - byte_in_group);
		if (alloc) {
			old_val += num_bytes;
			cache->space_info->bytes_used += num_bytes;
			if (cache->ro)
				cache->space_info->bytes_readonly -= num_bytes;
			btrfs_set_block_group_used(&cache->item, old_val);
			spin_unlock(&cache->lock);
			spin_unlock(&cache->space_info->lock);
		} else {
			old_val -= num_bytes;
			cache->space_info->bytes_used -= num_bytes;
			if (cache->ro)
				cache->space_info->bytes_readonly += num_bytes;
			btrfs_set_block_group_used(&cache->item, old_val);
			spin_unlock(&cache->lock);
			spin_unlock(&cache->space_info->lock);
			if (mark_free) {
				int ret;

				ret = btrfs_discard_extent(root, bytenr,
							   num_bytes);
				WARN_ON(ret);

				ret = btrfs_add_free_space(cache, bytenr,
							   num_bytes);
				WARN_ON(ret);
			}
		}
		put_block_group(cache);
		total -= num_bytes;
		bytenr += num_bytes;
	}
	return 0;
}

static u64 first_logical_byte(struct btrfs_root *root, u64 search_start)
{
	struct btrfs_block_group_cache *cache;
	u64 bytenr;

	cache = btrfs_lookup_first_block_group(root->fs_info, search_start);
	if (!cache)
		return 0;

	bytenr = cache->key.objectid;
	put_block_group(cache);

	return bytenr;
}

int btrfs_update_pinned_extents(struct btrfs_root *root,
				u64 bytenr, u64 num, int pin)
{
	u64 len;
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *fs_info = root->fs_info;

	WARN_ON(!mutex_is_locked(&root->fs_info->pinned_mutex));
	if (pin) {
		set_extent_dirty(&fs_info->pinned_extents,
				bytenr, bytenr + num - 1, GFP_NOFS);
	} else {
		clear_extent_dirty(&fs_info->pinned_extents,
				bytenr, bytenr + num - 1, GFP_NOFS);
	}
	while (num > 0) {
		cache = btrfs_lookup_block_group(fs_info, bytenr);
		BUG_ON(!cache);
		len = min(num, cache->key.offset -
			  (bytenr - cache->key.objectid));
		if (pin) {
			spin_lock(&cache->space_info->lock);
			spin_lock(&cache->lock);
			cache->pinned += len;
			cache->space_info->bytes_pinned += len;
			spin_unlock(&cache->lock);
			spin_unlock(&cache->space_info->lock);
			fs_info->total_pinned += len;
		} else {
			spin_lock(&cache->space_info->lock);
			spin_lock(&cache->lock);
			cache->pinned -= len;
			cache->space_info->bytes_pinned -= len;
			spin_unlock(&cache->lock);
			spin_unlock(&cache->space_info->lock);
			fs_info->total_pinned -= len;
			if (cache->cached)
				btrfs_add_free_space(cache, bytenr, len);
		}
		put_block_group(cache);
		bytenr += len;
		num -= len;
	}
	return 0;
}

static int update_reserved_extents(struct btrfs_root *root,
				   u64 bytenr, u64 num, int reserve)
{
	u64 len;
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *fs_info = root->fs_info;

	while (num > 0) {
		cache = btrfs_lookup_block_group(fs_info, bytenr);
		BUG_ON(!cache);
		len = min(num, cache->key.offset -
			  (bytenr - cache->key.objectid));

		spin_lock(&cache->space_info->lock);
		spin_lock(&cache->lock);
		if (reserve) {
			cache->reserved += len;
			cache->space_info->bytes_reserved += len;
		} else {
			cache->reserved -= len;
			cache->space_info->bytes_reserved -= len;
		}
		spin_unlock(&cache->lock);
		spin_unlock(&cache->space_info->lock);
		put_block_group(cache);
		bytenr += len;
		num -= len;
	}
	return 0;
}

int btrfs_copy_pinned(struct btrfs_root *root, struct extent_io_tree *copy)
{
	u64 last = 0;
	u64 start;
	u64 end;
	struct extent_io_tree *pinned_extents = &root->fs_info->pinned_extents;
	int ret;

	mutex_lock(&root->fs_info->pinned_mutex);
	while (1) {
		ret = find_first_extent_bit(pinned_extents, last,
					    &start, &end, EXTENT_DIRTY);
		if (ret)
			break;
		set_extent_dirty(copy, start, end, GFP_NOFS);
		last = end + 1;
	}
	mutex_unlock(&root->fs_info->pinned_mutex);
	return 0;
}

int btrfs_finish_extent_commit(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root,
			       struct extent_io_tree *unpin)
{
	u64 start;
	u64 end;
	int ret;

	mutex_lock(&root->fs_info->pinned_mutex);
	while (1) {
		ret = find_first_extent_bit(unpin, 0, &start, &end,
					    EXTENT_DIRTY);
		if (ret)
			break;

		ret = btrfs_discard_extent(root, start, end + 1 - start);

		btrfs_update_pinned_extents(root, start, end + 1 - start, 0);
		clear_extent_dirty(unpin, start, end, GFP_NOFS);

		if (need_resched()) {
			mutex_unlock(&root->fs_info->pinned_mutex);
			cond_resched();
			mutex_lock(&root->fs_info->pinned_mutex);
		}
	}
	mutex_unlock(&root->fs_info->pinned_mutex);
	return ret;
}

static int finish_current_insert(struct btrfs_trans_handle *trans,
				 struct btrfs_root *extent_root, int all)
{
	u64 start;
	u64 end;
	u64 priv;
	u64 search = 0;
	struct btrfs_fs_info *info = extent_root->fs_info;
	struct btrfs_path *path;
	struct pending_extent_op *extent_op, *tmp;
	struct list_head insert_list, update_list;
	int ret;
	int num_inserts = 0, max_inserts, restart = 0;

	path = btrfs_alloc_path();
	INIT_LIST_HEAD(&insert_list);
	INIT_LIST_HEAD(&update_list);

	max_inserts = extent_root->leafsize /
		(2 * sizeof(struct btrfs_key) + 2 * sizeof(struct btrfs_item) +
		 sizeof(struct btrfs_extent_ref) +
		 sizeof(struct btrfs_extent_item));
again:
	mutex_lock(&info->extent_ins_mutex);
	while (1) {
		ret = find_first_extent_bit(&info->extent_ins, search, &start,
					    &end, EXTENT_WRITEBACK);
		if (ret) {
			if (restart && !num_inserts &&
			    list_empty(&update_list)) {
				restart = 0;
				search = 0;
				continue;
			}
			break;
		}

		ret = try_lock_extent(&info->extent_ins, start, end, GFP_NOFS);
		if (!ret) {
			if (all)
				restart = 1;
			search = end + 1;
			if (need_resched()) {
				mutex_unlock(&info->extent_ins_mutex);
				cond_resched();
				mutex_lock(&info->extent_ins_mutex);
			}
			continue;
		}

		ret = get_state_private(&info->extent_ins, start, &priv);
		BUG_ON(ret);
		extent_op = (struct pending_extent_op *)(unsigned long) priv;

		if (extent_op->type == PENDING_EXTENT_INSERT) {
			num_inserts++;
			list_add_tail(&extent_op->list, &insert_list);
			search = end + 1;
			if (num_inserts == max_inserts) {
				restart = 1;
				break;
			}
		} else if (extent_op->type == PENDING_BACKREF_UPDATE) {
			list_add_tail(&extent_op->list, &update_list);
			search = end + 1;
		} else {
			BUG();
		}
	}

	/*
	 * process the update list, clear the writeback bit for it, and if
	 * somebody marked this thing for deletion then just unlock it and be
	 * done, the free_extents will handle it
	 */
	list_for_each_entry_safe(extent_op, tmp, &update_list, list) {
		clear_extent_bits(&info->extent_ins, extent_op->bytenr,
				  extent_op->bytenr + extent_op->num_bytes - 1,
				  EXTENT_WRITEBACK, GFP_NOFS);
		if (extent_op->del) {
			list_del_init(&extent_op->list);
			unlock_extent(&info->extent_ins, extent_op->bytenr,
				      extent_op->bytenr + extent_op->num_bytes
				      - 1, GFP_NOFS);
			kfree(extent_op);
		}
	}
	mutex_unlock(&info->extent_ins_mutex);

	/*
	 * still have things left on the update list, go ahead an update
	 * everything
	 */
	if (!list_empty(&update_list)) {
		ret = update_backrefs(trans, extent_root, path, &update_list);
		BUG_ON(ret);

		/* we may have COW'ed new blocks, so lets start over */
		if (all)
			restart = 1;
	}

	/*
	 * if no inserts need to be done, but we skipped some extents and we
	 * need to make sure everything is cleaned then reset everything and
	 * go back to the beginning
	 */
	if (!num_inserts && restart) {
		search = 0;
		restart = 0;
		INIT_LIST_HEAD(&update_list);
		INIT_LIST_HEAD(&insert_list);
		goto again;
	} else if (!num_inserts) {
		goto out;
	}

	/*
	 * process the insert extents list.  Again if we are deleting this
	 * extent, then just unlock it, pin down the bytes if need be, and be
	 * done with it.  Saves us from having to actually insert the extent
	 * into the tree and then subsequently come along and delete it
	 */
	mutex_lock(&info->extent_ins_mutex);
	list_for_each_entry_safe(extent_op, tmp, &insert_list, list) {
		clear_extent_bits(&info->extent_ins, extent_op->bytenr,
				  extent_op->bytenr + extent_op->num_bytes - 1,
				  EXTENT_WRITEBACK, GFP_NOFS);
		if (extent_op->del) {
			u64 used;
			list_del_init(&extent_op->list);
			unlock_extent(&info->extent_ins, extent_op->bytenr,
				      extent_op->bytenr + extent_op->num_bytes
				      - 1, GFP_NOFS);

			mutex_lock(&extent_root->fs_info->pinned_mutex);
			ret = pin_down_bytes(trans, extent_root,
					     extent_op->bytenr,
					     extent_op->num_bytes, 0);
			mutex_unlock(&extent_root->fs_info->pinned_mutex);

			spin_lock(&info->delalloc_lock);
			used = btrfs_super_bytes_used(&info->super_copy);
			btrfs_set_super_bytes_used(&info->super_copy,
					used - extent_op->num_bytes);
			used = btrfs_root_used(&extent_root->root_item);
			btrfs_set_root_used(&extent_root->root_item,
					used - extent_op->num_bytes);
			spin_unlock(&info->delalloc_lock);

			ret = update_block_group(trans, extent_root,
						 extent_op->bytenr,
						 extent_op->num_bytes,
						 0, ret > 0);
			BUG_ON(ret);
			kfree(extent_op);
			num_inserts--;
		}
	}
	mutex_unlock(&info->extent_ins_mutex);

	ret = insert_extents(trans, extent_root, path, &insert_list,
			     num_inserts);
	BUG_ON(ret);

	/*
	 * if restart is set for whatever reason we need to go back and start
	 * searching through the pending list again.
	 *
	 * We just inserted some extents, which could have resulted in new
	 * blocks being allocated, which would result in new blocks needing
	 * updates, so if all is set we _must_ restart to get the updated
	 * blocks.
	 */
	if (restart || all) {
		INIT_LIST_HEAD(&insert_list);
		INIT_LIST_HEAD(&update_list);
		search = 0;
		restart = 0;
		num_inserts = 0;
		goto again;
	}
out:
	btrfs_free_path(path);
	return 0;
}

static int pin_down_bytes(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root,
			  u64 bytenr, u64 num_bytes, int is_data)
{
	int err = 0;
	struct extent_buffer *buf;

	if (is_data)
		goto pinit;

	buf = btrfs_find_tree_block(root, bytenr, num_bytes);
	if (!buf)
		goto pinit;

	/* we can reuse a block if it hasn't been written
	 * and it is from this transaction.  We can't
	 * reuse anything from the tree log root because
	 * it has tiny sub-transactions.
	 */
	if (btrfs_buffer_uptodate(buf, 0) &&
	    btrfs_try_tree_lock(buf)) {
		u64 header_owner = btrfs_header_owner(buf);
		u64 header_transid = btrfs_header_generation(buf);
		if (header_owner != BTRFS_TREE_LOG_OBJECTID &&
		    header_owner != BTRFS_TREE_RELOC_OBJECTID &&
		    header_transid == trans->transid &&
		    !btrfs_header_flag(buf, BTRFS_HEADER_FLAG_WRITTEN)) {
			clean_tree_block(NULL, root, buf);
			btrfs_tree_unlock(buf);
			free_extent_buffer(buf);
			return 1;
		}
		btrfs_tree_unlock(buf);
	}
	free_extent_buffer(buf);
pinit:
	btrfs_update_pinned_extents(root, bytenr, num_bytes, 1);

	BUG_ON(err < 0);
	return 0;
}

/*
 * remove an extent from the root, returns 0 on success
 */
static int __free_extent(struct btrfs_trans_handle *trans,
			 struct btrfs_root *root,
			 u64 bytenr, u64 num_bytes, u64 parent,
			 u64 root_objectid, u64 ref_generation,
			 u64 owner_objectid, int pin, int mark_free)
{
	struct btrfs_path *path;
	struct btrfs_key key;
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_root *extent_root = info->extent_root;
	struct extent_buffer *leaf;
	int ret;
	int extent_slot = 0;
	int found_extent = 0;
	int num_to_del = 1;
	struct btrfs_extent_item *ei;
	u32 refs;

	key.objectid = bytenr;
	btrfs_set_key_type(&key, BTRFS_EXTENT_ITEM_KEY);
	key.offset = num_bytes;
	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	path->reada = 1;
	ret = lookup_extent_backref(trans, extent_root, path,
				    bytenr, parent, root_objectid,
				    ref_generation, owner_objectid, 1);
	if (ret == 0) {
		struct btrfs_key found_key;
		extent_slot = path->slots[0];
		while (extent_slot > 0) {
			extent_slot--;
			btrfs_item_key_to_cpu(path->nodes[0], &found_key,
					      extent_slot);
			if (found_key.objectid != bytenr)
				break;
			if (found_key.type == BTRFS_EXTENT_ITEM_KEY &&
			    found_key.offset == num_bytes) {
				found_extent = 1;
				break;
			}
			if (path->slots[0] - extent_slot > 5)
				break;
		}
		if (!found_extent) {
			ret = remove_extent_backref(trans, extent_root, path);
			BUG_ON(ret);
			btrfs_release_path(extent_root, path);
			ret = btrfs_search_slot(trans, extent_root,
						&key, path, -1, 1);
			if (ret) {
				printk(KERN_ERR "umm, got %d back from search"
				       ", was looking for %llu\n", ret,
				       (unsigned long long)bytenr);
				btrfs_print_leaf(extent_root, path->nodes[0]);
			}
			BUG_ON(ret);
			extent_slot = path->slots[0];
		}
	} else {
		btrfs_print_leaf(extent_root, path->nodes[0]);
		WARN_ON(1);
		printk(KERN_ERR "btrfs unable to find ref byte nr %llu "
		       "root %llu gen %llu owner %llu\n",
		       (unsigned long long)bytenr,
		       (unsigned long long)root_objectid,
		       (unsigned long long)ref_generation,
		       (unsigned long long)owner_objectid);
	}

	leaf = path->nodes[0];
	ei = btrfs_item_ptr(leaf, extent_slot,
			    struct btrfs_extent_item);
	refs = btrfs_extent_refs(leaf, ei);
	BUG_ON(refs == 0);
	refs -= 1;
	btrfs_set_extent_refs(leaf, ei, refs);

	btrfs_mark_buffer_dirty(leaf);

	if (refs == 0 && found_extent && path->slots[0] == extent_slot + 1) {
		struct btrfs_extent_ref *ref;
		ref = btrfs_item_ptr(leaf, path->slots[0],
				     struct btrfs_extent_ref);
		BUG_ON(btrfs_ref_num_refs(leaf, ref) != 1);
		/* if the back ref and the extent are next to each other
		 * they get deleted below in one shot
		 */
		path->slots[0] = extent_slot;
		num_to_del = 2;
	} else if (found_extent) {
		/* otherwise delete the extent back ref */
		ret = remove_extent_backref(trans, extent_root, path);
		BUG_ON(ret);
		/* if refs are 0, we need to setup the path for deletion */
		if (refs == 0) {
			btrfs_release_path(extent_root, path);
			ret = btrfs_search_slot(trans, extent_root, &key, path,
						-1, 1);
			BUG_ON(ret);
		}
	}

	if (refs == 0) {
		u64 super_used;
		u64 root_used;

		if (pin) {
			mutex_lock(&root->fs_info->pinned_mutex);
			ret = pin_down_bytes(trans, root, bytenr, num_bytes,
				owner_objectid >= BTRFS_FIRST_FREE_OBJECTID);
			mutex_unlock(&root->fs_info->pinned_mutex);
			if (ret > 0)
				mark_free = 1;
			BUG_ON(ret < 0);
		}
		/* block accounting for super block */
		spin_lock(&info->delalloc_lock);
		super_used = btrfs_super_bytes_used(&info->super_copy);
		btrfs_set_super_bytes_used(&info->super_copy,
					   super_used - num_bytes);

		/* block accounting for root item */
		root_used = btrfs_root_used(&root->root_item);
		btrfs_set_root_used(&root->root_item,
					   root_used - num_bytes);
		spin_unlock(&info->delalloc_lock);
		ret = btrfs_del_items(trans, extent_root, path, path->slots[0],
				      num_to_del);
		BUG_ON(ret);
		btrfs_release_path(extent_root, path);

		if (owner_objectid >= BTRFS_FIRST_FREE_OBJECTID) {
			ret = btrfs_del_csums(trans, root, bytenr, num_bytes);
			BUG_ON(ret);
		}

		ret = update_block_group(trans, root, bytenr, num_bytes, 0,
					 mark_free);
		BUG_ON(ret);
	}
	btrfs_free_path(path);
	finish_current_insert(trans, extent_root, 0);
	return ret;
}

/*
 * find all the blocks marked as pending in the radix tree and remove
 * them from the extent map
 */
static int del_pending_extents(struct btrfs_trans_handle *trans,
			       struct btrfs_root *extent_root, int all)
{
	int ret;
	int err = 0;
	u64 start;
	u64 end;
	u64 priv;
	u64 search = 0;
	int nr = 0, skipped = 0;
	struct extent_io_tree *pending_del;
	struct extent_io_tree *extent_ins;
	struct pending_extent_op *extent_op;
	struct btrfs_fs_info *info = extent_root->fs_info;
	struct list_head delete_list;

	INIT_LIST_HEAD(&delete_list);
	extent_ins = &extent_root->fs_info->extent_ins;
	pending_del = &extent_root->fs_info->pending_del;

again:
	mutex_lock(&info->extent_ins_mutex);
	while (1) {
		ret = find_first_extent_bit(pending_del, search, &start, &end,
					    EXTENT_WRITEBACK);
		if (ret) {
			if (all && skipped && !nr) {
				search = 0;
				skipped = 0;
				continue;
			}
			mutex_unlock(&info->extent_ins_mutex);
			break;
		}

		ret = try_lock_extent(extent_ins, start, end, GFP_NOFS);
		if (!ret) {
			search = end+1;
			skipped = 1;

			if (need_resched()) {
				mutex_unlock(&info->extent_ins_mutex);
				cond_resched();
				mutex_lock(&info->extent_ins_mutex);
			}

			continue;
		}
		BUG_ON(ret < 0);

		ret = get_state_private(pending_del, start, &priv);
		BUG_ON(ret);
		extent_op = (struct pending_extent_op *)(unsigned long)priv;

		clear_extent_bits(pending_del, start, end, EXTENT_WRITEBACK,
				  GFP_NOFS);
		if (!test_range_bit(extent_ins, start, end,
				    EXTENT_WRITEBACK, 0)) {
			list_add_tail(&extent_op->list, &delete_list);
			nr++;
		} else {
			kfree(extent_op);

			ret = get_state_private(&info->extent_ins, start,
						&priv);
			BUG_ON(ret);
			extent_op = (struct pending_extent_op *)
						(unsigned long)priv;

			clear_extent_bits(&info->extent_ins, start, end,
					  EXTENT_WRITEBACK, GFP_NOFS);

			if (extent_op->type == PENDING_BACKREF_UPDATE) {
				list_add_tail(&extent_op->list, &delete_list);
				search = end + 1;
				nr++;
				continue;
			}

			mutex_lock(&extent_root->fs_info->pinned_mutex);
			ret = pin_down_bytes(trans, extent_root, start,
					     end + 1 - start, 0);
			mutex_unlock(&extent_root->fs_info->pinned_mutex);

			ret = update_block_group(trans, extent_root, start,
						end + 1 - start, 0, ret > 0);

			unlock_extent(extent_ins, start, end, GFP_NOFS);
			BUG_ON(ret);
			kfree(extent_op);
		}
		if (ret)
			err = ret;

		search = end + 1;

		if (need_resched()) {
			mutex_unlock(&info->extent_ins_mutex);
			cond_resched();
			mutex_lock(&info->extent_ins_mutex);
		}
	}

	if (nr) {
		ret = free_extents(trans, extent_root, &delete_list);
		BUG_ON(ret);
	}

	if (all && skipped) {
		INIT_LIST_HEAD(&delete_list);
		search = 0;
		nr = 0;
		goto again;
	}

	if (!err)
		finish_current_insert(trans, extent_root, 0);
	return err;
}

/*
 * remove an extent from the root, returns 0 on success
 */
static int __btrfs_free_extent(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root,
			       u64 bytenr, u64 num_bytes, u64 parent,
			       u64 root_objectid, u64 ref_generation,
			       u64 owner_objectid, int pin)
{
	struct btrfs_root *extent_root = root->fs_info->extent_root;
	int pending_ret;
	int ret;

	WARN_ON(num_bytes < root->sectorsize);
	if (root == extent_root) {
		struct pending_extent_op *extent_op = NULL;

		mutex_lock(&root->fs_info->extent_ins_mutex);
		if (test_range_bit(&root->fs_info->extent_ins, bytenr,
				bytenr + num_bytes - 1, EXTENT_WRITEBACK, 0)) {
			u64 priv;
			ret = get_state_private(&root->fs_info->extent_ins,
						bytenr, &priv);
			BUG_ON(ret);
			extent_op = (struct pending_extent_op *)
						(unsigned long)priv;

			extent_op->del = 1;
			if (extent_op->type == PENDING_EXTENT_INSERT) {
				mutex_unlock(&root->fs_info->extent_ins_mutex);
				return 0;
			}
		}

		if (extent_op) {
			ref_generation = extent_op->orig_generation;
			parent = extent_op->orig_parent;
		}

		extent_op = kmalloc(sizeof(*extent_op), GFP_NOFS);
		BUG_ON(!extent_op);

		extent_op->type = PENDING_EXTENT_DELETE;
		extent_op->bytenr = bytenr;
		extent_op->num_bytes = num_bytes;
		extent_op->parent = parent;
		extent_op->orig_parent = parent;
		extent_op->generation = ref_generation;
		extent_op->orig_generation = ref_generation;
		extent_op->level = (int)owner_objectid;
		INIT_LIST_HEAD(&extent_op->list);
		extent_op->del = 0;

		set_extent_bits(&root->fs_info->pending_del,
				bytenr, bytenr + num_bytes - 1,
				EXTENT_WRITEBACK, GFP_NOFS);
		set_state_private(&root->fs_info->pending_del,
				  bytenr, (unsigned long)extent_op);
		mutex_unlock(&root->fs_info->extent_ins_mutex);
		return 0;
	}
	/* if metadata always pin */
	if (owner_objectid < BTRFS_FIRST_FREE_OBJECTID) {
		if (root->root_key.objectid == BTRFS_TREE_LOG_OBJECTID) {
			mutex_lock(&root->fs_info->pinned_mutex);
			btrfs_update_pinned_extents(root, bytenr, num_bytes, 1);
			mutex_unlock(&root->fs_info->pinned_mutex);
			update_reserved_extents(root, bytenr, num_bytes, 0);
			return 0;
		}
		pin = 1;
	}

	/* if data pin when any transaction has committed this */
	if (ref_generation != trans->transid)
		pin = 1;

	ret = __free_extent(trans, root, bytenr, num_bytes, parent,
			    root_objectid, ref_generation,
			    owner_objectid, pin, pin == 0);

	finish_current_insert(trans, root->fs_info->extent_root, 0);
	pending_ret = del_pending_extents(trans, root->fs_info->extent_root, 0);
	return ret ? ret : pending_ret;
}

int btrfs_free_extent(struct btrfs_trans_handle *trans,
		      struct btrfs_root *root,
		      u64 bytenr, u64 num_bytes, u64 parent,
		      u64 root_objectid, u64 ref_generation,
		      u64 owner_objectid, int pin)
{
	int ret;

	ret = __btrfs_free_extent(trans, root, bytenr, num_bytes, parent,
				  root_objectid, ref_generation,
				  owner_objectid, pin);
	return ret;
}

static u64 stripe_align(struct btrfs_root *root, u64 val)
{
	u64 mask = ((u64)root->stripesize - 1);
	u64 ret = (val + mask) & ~mask;
	return ret;
}

/*
 * walks the btree of allocated extents and find a hole of a given size.
 * The key ins is changed to record the hole:
 * ins->objectid == block start
 * ins->flags = BTRFS_EXTENT_ITEM_KEY
 * ins->offset == number of blocks
 * Any available blocks before search_start are skipped.
 */
static noinline int find_free_extent(struct btrfs_trans_handle *trans,
				     struct btrfs_root *orig_root,
				     u64 num_bytes, u64 empty_size,
				     u64 search_start, u64 search_end,
				     u64 hint_byte, struct btrfs_key *ins,
				     u64 exclude_start, u64 exclude_nr,
				     int data)
{
	int ret = 0;
	struct btrfs_root *root = orig_root->fs_info->extent_root;
	u64 total_needed = num_bytes;
	u64 *last_ptr = NULL;
	u64 last_wanted = 0;
	struct btrfs_block_group_cache *block_group = NULL;
	int chunk_alloc_done = 0;
	int empty_cluster = 2 * 1024 * 1024;
	int allowed_chunk_alloc = 0;
	struct list_head *head = NULL, *cur = NULL;
	int loop = 0;
	int extra_loop = 0;
	struct btrfs_space_info *space_info;

	WARN_ON(num_bytes < root->sectorsize);
	btrfs_set_key_type(ins, BTRFS_EXTENT_ITEM_KEY);
	ins->objectid = 0;
	ins->offset = 0;

	if (orig_root->ref_cows || empty_size)
		allowed_chunk_alloc = 1;

	if (data & BTRFS_BLOCK_GROUP_METADATA) {
		last_ptr = &root->fs_info->last_alloc;
		if (!btrfs_test_opt(root, SSD))
			empty_cluster = 64 * 1024;
	}

	if ((data & BTRFS_BLOCK_GROUP_DATA) && btrfs_test_opt(root, SSD))
		last_ptr = &root->fs_info->last_data_alloc;

	if (last_ptr) {
		if (*last_ptr) {
			hint_byte = *last_ptr;
			last_wanted = *last_ptr;
		} else
			empty_size += empty_cluster;
	} else {
		empty_cluster = 0;
	}
	search_start = max(search_start, first_logical_byte(root, 0));
	search_start = max(search_start, hint_byte);

	if (last_wanted && search_start != last_wanted) {
		last_wanted = 0;
		empty_size += empty_cluster;
	}

	total_needed += empty_size;
	block_group = btrfs_lookup_block_group(root->fs_info, search_start);
	if (!block_group)
		block_group = btrfs_lookup_first_block_group(root->fs_info,
							     search_start);
	space_info = __find_space_info(root->fs_info, data);

	down_read(&space_info->groups_sem);
	while (1) {
		struct btrfs_free_space *free_space;
		/*
		 * the only way this happens if our hint points to a block
		 * group thats not of the proper type, while looping this
		 * should never happen
		 */
		if (empty_size)
			extra_loop = 1;

		if (!block_group)
			goto new_group_no_lock;

		if (unlikely(!block_group->cached)) {
			mutex_lock(&block_group->cache_mutex);
			ret = cache_block_group(root, block_group);
			mutex_unlock(&block_group->cache_mutex);
			if (ret)
				break;
		}

		mutex_lock(&block_group->alloc_mutex);
		if (unlikely(!block_group_bits(block_group, data)))
			goto new_group;

		if (unlikely(block_group->ro))
			goto new_group;

		free_space = btrfs_find_free_space(block_group, search_start,
						   total_needed);
		if (free_space) {
			u64 start = block_group->key.objectid;
			u64 end = block_group->key.objectid +
				block_group->key.offset;

			search_start = stripe_align(root, free_space->offset);

			/* move on to the next group */
			if (search_start + num_bytes >= search_end)
				goto new_group;

			/* move on to the next group */
			if (search_start + num_bytes > end)
				goto new_group;

			if (last_wanted && search_start != last_wanted) {
				total_needed += empty_cluster;
				empty_size += empty_cluster;
				last_wanted = 0;
				/*
				 * if search_start is still in this block group
				 * then we just re-search this block group
				 */
				if (search_start >= start &&
				    search_start < end) {
					mutex_unlock(&block_group->alloc_mutex);
					continue;
				}

				/* else we go to the next block group */
				goto new_group;
			}

			if (exclude_nr > 0 &&
			    (search_start + num_bytes > exclude_start &&
			     search_start < exclude_start + exclude_nr)) {
				search_start = exclude_start + exclude_nr;
				/*
				 * if search_start is still in this block group
				 * then we just re-search this block group
				 */
				if (search_start >= start &&
				    search_start < end) {
					mutex_unlock(&block_group->alloc_mutex);
					last_wanted = 0;
					continue;
				}

				/* else we go to the next block group */
				goto new_group;
			}

			ins->objectid = search_start;
			ins->offset = num_bytes;

			btrfs_remove_free_space_lock(block_group, search_start,
						     num_bytes);
			/* we are all good, lets return */
			mutex_unlock(&block_group->alloc_mutex);
			break;
		}
new_group:
		mutex_unlock(&block_group->alloc_mutex);
		put_block_group(block_group);
		block_group = NULL;
new_group_no_lock:
		/* don't try to compare new allocations against the
		 * last allocation any more
		 */
		last_wanted = 0;

		/*
		 * Here's how this works.
		 * loop == 0: we were searching a block group via a hint
		 *		and didn't find anything, so we start at
		 *		the head of the block groups and keep searching
		 * loop == 1: we're searching through all of the block groups
		 *		if we hit the head again we have searched
		 *		all of the block groups for this space and we
		 *		need to try and allocate, if we cant error out.
		 * loop == 2: we allocated more space and are looping through
		 *		all of the block groups again.
		 */
		if (loop == 0) {
			head = &space_info->block_groups;
			cur = head->next;
			loop++;
		} else if (loop == 1 && cur == head) {
			int keep_going;

			/* at this point we give up on the empty_size
			 * allocations and just try to allocate the min
			 * space.
			 *
			 * The extra_loop field was set if an empty_size
			 * allocation was attempted above, and if this
			 * is try we need to try the loop again without
			 * the additional empty_size.
			 */
			total_needed -= empty_size;
			empty_size = 0;
			keep_going = extra_loop;
			loop++;

			if (allowed_chunk_alloc && !chunk_alloc_done) {
				up_read(&space_info->groups_sem);
				ret = do_chunk_alloc(trans, root, num_bytes +
						     2 * 1024 * 1024, data, 1);
				down_read(&space_info->groups_sem);
				if (ret < 0)
					goto loop_check;
				head = &space_info->block_groups;
				/*
				 * we've allocated a new chunk, keep
				 * trying
				 */
				keep_going = 1;
				chunk_alloc_done = 1;
			} else if (!allowed_chunk_alloc) {
				space_info->force_alloc = 1;
			}
loop_check:
			if (keep_going) {
				cur = head->next;
				extra_loop = 0;
			} else {
				break;
			}
		} else if (cur == head) {
			break;
		}

		block_group = list_entry(cur, struct btrfs_block_group_cache,
					 list);
		atomic_inc(&block_group->count);

		search_start = block_group->key.objectid;
		cur = cur->next;
	}

	/* we found what we needed */
	if (ins->objectid) {
		if (!(data & BTRFS_BLOCK_GROUP_DATA))
			trans->block_group = block_group->key.objectid;

		if (last_ptr)
			*last_ptr = ins->objectid + ins->offset;
		ret = 0;
	} else if (!ret) {
		printk(KERN_ERR "btrfs searching for %llu bytes, "
		       "num_bytes %llu, loop %d, allowed_alloc %d\n",
		       (unsigned long long)total_needed,
		       (unsigned long long)num_bytes,
		       loop, allowed_chunk_alloc);
		ret = -ENOSPC;
	}
	if (block_group)
		put_block_group(block_group);

	up_read(&space_info->groups_sem);
	return ret;
}

static void dump_space_info(struct btrfs_space_info *info, u64 bytes)
{
	struct btrfs_block_group_cache *cache;

	printk(KERN_INFO "space_info has %llu free, is %sfull\n",
	       (unsigned long long)(info->total_bytes - info->bytes_used -
				    info->bytes_pinned - info->bytes_reserved),
	       (info->full) ? "" : "not ");
	printk(KERN_INFO "space_info total=%llu, pinned=%llu, delalloc=%llu,"
	       " may_use=%llu, used=%llu\n", info->total_bytes,
	       info->bytes_pinned, info->bytes_delalloc, info->bytes_may_use,
	       info->bytes_used);

	down_read(&info->groups_sem);
	list_for_each_entry(cache, &info->block_groups, list) {
		spin_lock(&cache->lock);
		printk(KERN_INFO "block group %llu has %llu bytes, %llu used "
		       "%llu pinned %llu reserved\n",
		       (unsigned long long)cache->key.objectid,
		       (unsigned long long)cache->key.offset,
		       (unsigned long long)btrfs_block_group_used(&cache->item),
		       (unsigned long long)cache->pinned,
		       (unsigned long long)cache->reserved);
		btrfs_dump_free_space(cache, bytes);
		spin_unlock(&cache->lock);
	}
	up_read(&info->groups_sem);
}

static int __btrfs_reserve_extent(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root,
				  u64 num_bytes, u64 min_alloc_size,
				  u64 empty_size, u64 hint_byte,
				  u64 search_end, struct btrfs_key *ins,
				  u64 data)
{
	int ret;
	u64 search_start = 0;
	struct btrfs_fs_info *info = root->fs_info;

	data = btrfs_get_alloc_profile(root, data);
again:
	/*
	 * the only place that sets empty_size is btrfs_realloc_node, which
	 * is not called recursively on allocations
	 */
	if (empty_size || root->ref_cows) {
		if (!(data & BTRFS_BLOCK_GROUP_METADATA)) {
			ret = do_chunk_alloc(trans, root->fs_info->extent_root,
				     2 * 1024 * 1024,
				     BTRFS_BLOCK_GROUP_METADATA |
				     (info->metadata_alloc_profile &
				      info->avail_metadata_alloc_bits), 0);
		}
		ret = do_chunk_alloc(trans, root->fs_info->extent_root,
				     num_bytes + 2 * 1024 * 1024, data, 0);
	}

	WARN_ON(num_bytes < root->sectorsize);
	ret = find_free_extent(trans, root, num_bytes, empty_size,
			       search_start, search_end, hint_byte, ins,
			       trans->alloc_exclude_start,
			       trans->alloc_exclude_nr, data);

	if (ret == -ENOSPC && num_bytes > min_alloc_size) {
		num_bytes = num_bytes >> 1;
		num_bytes = num_bytes & ~(root->sectorsize - 1);
		num_bytes = max(num_bytes, min_alloc_size);
		do_chunk_alloc(trans, root->fs_info->extent_root,
			       num_bytes, data, 1);
		goto again;
	}
	if (ret) {
		struct btrfs_space_info *sinfo;

		sinfo = __find_space_info(root->fs_info, data);
		printk(KERN_ERR "btrfs allocation failed flags %llu, "
		       "wanted %llu\n", (unsigned long long)data,
		       (unsigned long long)num_bytes);
		dump_space_info(sinfo, num_bytes);
		BUG();
	}

	return ret;
}

int btrfs_free_reserved_extent(struct btrfs_root *root, u64 start, u64 len)
{
	struct btrfs_block_group_cache *cache;
	int ret = 0;

	cache = btrfs_lookup_block_group(root->fs_info, start);
	if (!cache) {
		printk(KERN_ERR "Unable to find block group for %llu\n",
		       (unsigned long long)start);
		return -ENOSPC;
	}

	ret = btrfs_discard_extent(root, start, len);

	btrfs_add_free_space(cache, start, len);
	put_block_group(cache);
	update_reserved_extents(root, start, len, 0);

	return ret;
}

int btrfs_reserve_extent(struct btrfs_trans_handle *trans,
				  struct btrfs_root *root,
				  u64 num_bytes, u64 min_alloc_size,
				  u64 empty_size, u64 hint_byte,
				  u64 search_end, struct btrfs_key *ins,
				  u64 data)
{
	int ret;
	ret = __btrfs_reserve_extent(trans, root, num_bytes, min_alloc_size,
				     empty_size, hint_byte, search_end, ins,
				     data);
	update_reserved_extents(root, ins->objectid, ins->offset, 1);
	return ret;
}

static int __btrfs_alloc_reserved_extent(struct btrfs_trans_handle *trans,
					 struct btrfs_root *root, u64 parent,
					 u64 root_objectid, u64 ref_generation,
					 u64 owner, struct btrfs_key *ins)
{
	int ret;
	int pending_ret;
	u64 super_used;
	u64 root_used;
	u64 num_bytes = ins->offset;
	u32 sizes[2];
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_root *extent_root = info->extent_root;
	struct btrfs_extent_item *extent_item;
	struct btrfs_extent_ref *ref;
	struct btrfs_path *path;
	struct btrfs_key keys[2];

	if (parent == 0)
		parent = ins->objectid;

	/* block accounting for super block */
	spin_lock(&info->delalloc_lock);
	super_used = btrfs_super_bytes_used(&info->super_copy);
	btrfs_set_super_bytes_used(&info->super_copy, super_used + num_bytes);

	/* block accounting for root item */
	root_used = btrfs_root_used(&root->root_item);
	btrfs_set_root_used(&root->root_item, root_used + num_bytes);
	spin_unlock(&info->delalloc_lock);

	if (root == extent_root) {
		struct pending_extent_op *extent_op;

		extent_op = kmalloc(sizeof(*extent_op), GFP_NOFS);
		BUG_ON(!extent_op);

		extent_op->type = PENDING_EXTENT_INSERT;
		extent_op->bytenr = ins->objectid;
		extent_op->num_bytes = ins->offset;
		extent_op->parent = parent;
		extent_op->orig_parent = 0;
		extent_op->generation = ref_generation;
		extent_op->orig_generation = 0;
		extent_op->level = (int)owner;
		INIT_LIST_HEAD(&extent_op->list);
		extent_op->del = 0;

		mutex_lock(&root->fs_info->extent_ins_mutex);
		set_extent_bits(&root->fs_info->extent_ins, ins->objectid,
				ins->objectid + ins->offset - 1,
				EXTENT_WRITEBACK, GFP_NOFS);
		set_state_private(&root->fs_info->extent_ins,
				  ins->objectid, (unsigned long)extent_op);
		mutex_unlock(&root->fs_info->extent_ins_mutex);
		goto update_block;
	}

	memcpy(&keys[0], ins, sizeof(*ins));
	keys[1].objectid = ins->objectid;
	keys[1].type = BTRFS_EXTENT_REF_KEY;
	keys[1].offset = parent;
	sizes[0] = sizeof(*extent_item);
	sizes[1] = sizeof(*ref);

	path = btrfs_alloc_path();
	BUG_ON(!path);

	ret = btrfs_insert_empty_items(trans, extent_root, path, keys,
				       sizes, 2);
	BUG_ON(ret);

	extent_item = btrfs_item_ptr(path->nodes[0], path->slots[0],
				     struct btrfs_extent_item);
	btrfs_set_extent_refs(path->nodes[0], extent_item, 1);
	ref = btrfs_item_ptr(path->nodes[0], path->slots[0] + 1,
			     struct btrfs_extent_ref);

	btrfs_set_ref_root(path->nodes[0], ref, root_objectid);
	btrfs_set_ref_generation(path->nodes[0], ref, ref_generation);
	btrfs_set_ref_objectid(path->nodes[0], ref, owner);
	btrfs_set_ref_num_refs(path->nodes[0], ref, 1);

	btrfs_mark_buffer_dirty(path->nodes[0]);

	trans->alloc_exclude_start = 0;
	trans->alloc_exclude_nr = 0;
	btrfs_free_path(path);
	finish_current_insert(trans, extent_root, 0);
	pending_ret = del_pending_extents(trans, extent_root, 0);

	if (ret)
		goto out;
	if (pending_ret) {
		ret = pending_ret;
		goto out;
	}

update_block:
	ret = update_block_group(trans, root, ins->objectid,
				 ins->offset, 1, 0);
	if (ret) {
		printk(KERN_ERR "btrfs update block group failed for %llu "
		       "%llu\n", (unsigned long long)ins->objectid,
		       (unsigned long long)ins->offset);
		BUG();
	}
out:
	return ret;
}

int btrfs_alloc_reserved_extent(struct btrfs_trans_handle *trans,
				struct btrfs_root *root, u64 parent,
				u64 root_objectid, u64 ref_generation,
				u64 owner, struct btrfs_key *ins)
{
	int ret;

	if (root_objectid == BTRFS_TREE_LOG_OBJECTID)
		return 0;
	ret = __btrfs_alloc_reserved_extent(trans, root, parent, root_objectid,
					    ref_generation, owner, ins);
	update_reserved_extents(root, ins->objectid, ins->offset, 0);
	return ret;
}

/*
 * this is used by the tree logging recovery code.  It records that
 * an extent has been allocated and makes sure to clear the free
 * space cache bits as well
 */
int btrfs_alloc_logged_extent(struct btrfs_trans_handle *trans,
				struct btrfs_root *root, u64 parent,
				u64 root_objectid, u64 ref_generation,
				u64 owner, struct btrfs_key *ins)
{
	int ret;
	struct btrfs_block_group_cache *block_group;

	block_group = btrfs_lookup_block_group(root->fs_info, ins->objectid);
	mutex_lock(&block_group->cache_mutex);
	cache_block_group(root, block_group);
	mutex_unlock(&block_group->cache_mutex);

	ret = btrfs_remove_free_space(block_group, ins->objectid,
				      ins->offset);
	BUG_ON(ret);
	put_block_group(block_group);
	ret = __btrfs_alloc_reserved_extent(trans, root, parent, root_objectid,
					    ref_generation, owner, ins);
	return ret;
}

/*
 * finds a free extent and does all the dirty work required for allocation
 * returns the key for the extent through ins, and a tree buffer for
 * the first block of the extent through buf.
 *
 * returns 0 if everything worked, non-zero otherwise.
 */
int btrfs_alloc_extent(struct btrfs_trans_handle *trans,
		       struct btrfs_root *root,
		       u64 num_bytes, u64 parent, u64 min_alloc_size,
		       u64 root_objectid, u64 ref_generation,
		       u64 owner_objectid, u64 empty_size, u64 hint_byte,
		       u64 search_end, struct btrfs_key *ins, u64 data)
{
	int ret;

	ret = __btrfs_reserve_extent(trans, root, num_bytes,
				     min_alloc_size, empty_size, hint_byte,
				     search_end, ins, data);
	BUG_ON(ret);
	if (root_objectid != BTRFS_TREE_LOG_OBJECTID) {
		ret = __btrfs_alloc_reserved_extent(trans, root, parent,
					root_objectid, ref_generation,
					owner_objectid, ins);
		BUG_ON(ret);

	} else {
		update_reserved_extents(root, ins->objectid, ins->offset, 1);
	}
	return ret;
}

struct extent_buffer *btrfs_init_new_buffer(struct btrfs_trans_handle *trans,
					    struct btrfs_root *root,
					    u64 bytenr, u32 blocksize,
					    int level)
{
	struct extent_buffer *buf;

	buf = btrfs_find_create_tree_block(root, bytenr, blocksize);
	if (!buf)
		return ERR_PTR(-ENOMEM);
	btrfs_set_header_generation(buf, trans->transid);
	btrfs_set_buffer_lockdep_class(buf, level);
	btrfs_tree_lock(buf);
	clean_tree_block(trans, root, buf);

	btrfs_set_lock_blocking(buf);
	btrfs_set_buffer_uptodate(buf);

	if (root->root_key.objectid == BTRFS_TREE_LOG_OBJECTID) {
		set_extent_dirty(&root->dirty_log_pages, buf->start,
			 buf->start + buf->len - 1, GFP_NOFS);
	} else {
		set_extent_dirty(&trans->transaction->dirty_pages, buf->start,
			 buf->start + buf->len - 1, GFP_NOFS);
	}
	trans->blocks_used++;
	/* this returns a buffer locked for blocking */
	return buf;
}

/*
 * helper function to allocate a block for a given tree
 * returns the tree buffer or NULL.
 */
struct extent_buffer *btrfs_alloc_free_block(struct btrfs_trans_handle *trans,
					     struct btrfs_root *root,
					     u32 blocksize, u64 parent,
					     u64 root_objectid,
					     u64 ref_generation,
					     int level,
					     u64 hint,
					     u64 empty_size)
{
	struct btrfs_key ins;
	int ret;
	struct extent_buffer *buf;

	ret = btrfs_alloc_extent(trans, root, blocksize, parent, blocksize,
				 root_objectid, ref_generation, level,
				 empty_size, hint, (u64)-1, &ins, 0);
	if (ret) {
		BUG_ON(ret > 0);
		return ERR_PTR(ret);
	}

	buf = btrfs_init_new_buffer(trans, root, ins.objectid,
				    blocksize, level);
	return buf;
}

int btrfs_drop_leaf_ref(struct btrfs_trans_handle *trans,
			struct btrfs_root *root, struct extent_buffer *leaf)
{
	u64 leaf_owner;
	u64 leaf_generation;
	struct refsort *sorted;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int i;
	int nritems;
	int ret;
	int refi = 0;
	int slot;

	BUG_ON(!btrfs_is_leaf(leaf));
	nritems = btrfs_header_nritems(leaf);
	leaf_owner = btrfs_header_owner(leaf);
	leaf_generation = btrfs_header_generation(leaf);

	sorted = kmalloc(sizeof(*sorted) * nritems, GFP_NOFS);
	/* we do this loop twice.  The first time we build a list
	 * of the extents we have a reference on, then we sort the list
	 * by bytenr.  The second time around we actually do the
	 * extent freeing.
	 */
	for (i = 0; i < nritems; i++) {
		u64 disk_bytenr;
		cond_resched();

		btrfs_item_key_to_cpu(leaf, &key, i);

		/* only extents have references, skip everything else */
		if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
			continue;

		fi = btrfs_item_ptr(leaf, i, struct btrfs_file_extent_item);

		/* inline extents live in the btree, they don't have refs */
		if (btrfs_file_extent_type(leaf, fi) ==
		    BTRFS_FILE_EXTENT_INLINE)
			continue;

		disk_bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);

		/* holes don't have refs */
		if (disk_bytenr == 0)
			continue;

		sorted[refi].bytenr = disk_bytenr;
		sorted[refi].slot = i;
		refi++;
	}

	if (refi == 0)
		goto out;

	sort(sorted, refi, sizeof(struct refsort), refsort_cmp, NULL);

	for (i = 0; i < refi; i++) {
		u64 disk_bytenr;

		disk_bytenr = sorted[i].bytenr;
		slot = sorted[i].slot;

		cond_resched();

		btrfs_item_key_to_cpu(leaf, &key, slot);
		if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
			continue;

		fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);

		ret = __btrfs_free_extent(trans, root, disk_bytenr,
				btrfs_file_extent_disk_num_bytes(leaf, fi),
				leaf->start, leaf_owner, leaf_generation,
				key.objectid, 0);
		BUG_ON(ret);

		atomic_inc(&root->fs_info->throttle_gen);
		wake_up(&root->fs_info->transaction_throttle);
		cond_resched();
	}
out:
	kfree(sorted);
	return 0;
}

static noinline int cache_drop_leaf_ref(struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct btrfs_leaf_ref *ref)
{
	int i;
	int ret;
	struct btrfs_extent_info *info;
	struct refsort *sorted;

	if (ref->nritems == 0)
		return 0;

	sorted = kmalloc(sizeof(*sorted) * ref->nritems, GFP_NOFS);
	for (i = 0; i < ref->nritems; i++) {
		sorted[i].bytenr = ref->extents[i].bytenr;
		sorted[i].slot = i;
	}
	sort(sorted, ref->nritems, sizeof(struct refsort), refsort_cmp, NULL);

	/*
	 * the items in the ref were sorted when the ref was inserted
	 * into the ref cache, so this is already in order
	 */
	for (i = 0; i < ref->nritems; i++) {
		info = ref->extents + sorted[i].slot;
		ret = __btrfs_free_extent(trans, root, info->bytenr,
					  info->num_bytes, ref->bytenr,
					  ref->owner, ref->generation,
					  info->objectid, 0);

		atomic_inc(&root->fs_info->throttle_gen);
		wake_up(&root->fs_info->transaction_throttle);
		cond_resched();

		BUG_ON(ret);
		info++;
	}

	kfree(sorted);
	return 0;
}

static int drop_snap_lookup_refcount(struct btrfs_root *root, u64 start,
				     u64 len, u32 *refs)
{
	int ret;

	ret = btrfs_lookup_extent_ref(NULL, root, start, len, refs);
	BUG_ON(ret);

#if 0 /* some debugging code in case we see problems here */
	/* if the refs count is one, it won't get increased again.  But
	 * if the ref count is > 1, someone may be decreasing it at
	 * the same time we are.
	 */
	if (*refs != 1) {
		struct extent_buffer *eb = NULL;
		eb = btrfs_find_create_tree_block(root, start, len);
		if (eb)
			btrfs_tree_lock(eb);

		mutex_lock(&root->fs_info->alloc_mutex);
		ret = lookup_extent_ref(NULL, root, start, len, refs);
		BUG_ON(ret);
		mutex_unlock(&root->fs_info->alloc_mutex);

		if (eb) {
			btrfs_tree_unlock(eb);
			free_extent_buffer(eb);
		}
		if (*refs == 1) {
			printk(KERN_ERR "btrfs block %llu went down to one "
			       "during drop_snap\n", (unsigned long long)start);
		}

	}
#endif

	cond_resched();
	return ret;
}

/*
 * this is used while deleting old snapshots, and it drops the refs
 * on a whole subtree starting from a level 1 node.
 *
 * The idea is to sort all the leaf pointers, and then drop the
 * ref on all the leaves in order.  Most of the time the leaves
 * will have ref cache entries, so no leaf IOs will be required to
 * find the extents they have references on.
 *
 * For each leaf, any references it has are also dropped in order
 *
 * This ends up dropping the references in something close to optimal
 * order for reading and modifying the extent allocation tree.
 */
static noinline int drop_level_one_refs(struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct btrfs_path *path)
{
	u64 bytenr;
	u64 root_owner;
	u64 root_gen;
	struct extent_buffer *eb = path->nodes[1];
	struct extent_buffer *leaf;
	struct btrfs_leaf_ref *ref;
	struct refsort *sorted = NULL;
	int nritems = btrfs_header_nritems(eb);
	int ret;
	int i;
	int refi = 0;
	int slot = path->slots[1];
	u32 blocksize = btrfs_level_size(root, 0);
	u32 refs;

	if (nritems == 0)
		goto out;

	root_owner = btrfs_header_owner(eb);
	root_gen = btrfs_header_generation(eb);
	sorted = kmalloc(sizeof(*sorted) * nritems, GFP_NOFS);

	/*
	 * step one, sort all the leaf pointers so we don't scribble
	 * randomly into the extent allocation tree
	 */
	for (i = slot; i < nritems; i++) {
		sorted[refi].bytenr = btrfs_node_blockptr(eb, i);
		sorted[refi].slot = i;
		refi++;
	}

	/*
	 * nritems won't be zero, but if we're picking up drop_snapshot
	 * after a crash, slot might be > 0, so double check things
	 * just in case.
	 */
	if (refi == 0)
		goto out;

	sort(sorted, refi, sizeof(struct refsort), refsort_cmp, NULL);

	/*
	 * the first loop frees everything the leaves point to
	 */
	for (i = 0; i < refi; i++) {
		u64 ptr_gen;

		bytenr = sorted[i].bytenr;

		/*
		 * check the reference count on this leaf.  If it is > 1
		 * we just decrement it below and don't update any
		 * of the refs the leaf points to.
		 */
		ret = drop_snap_lookup_refcount(root, bytenr, blocksize, &refs);
		BUG_ON(ret);
		if (refs != 1)
			continue;

		ptr_gen = btrfs_node_ptr_generation(eb, sorted[i].slot);

		/*
		 * the leaf only had one reference, which means the
		 * only thing pointing to this leaf is the snapshot
		 * we're deleting.  It isn't possible for the reference
		 * count to increase again later
		 *
		 * The reference cache is checked for the leaf,
		 * and if found we'll be able to drop any refs held by
		 * the leaf without needing to read it in.
		 */
		ref = btrfs_lookup_leaf_ref(root, bytenr);
		if (ref && ref->generation != ptr_gen) {
			btrfs_free_leaf_ref(root, ref);
			ref = NULL;
		}
		if (ref) {
			ret = cache_drop_leaf_ref(trans, root, ref);
			BUG_ON(ret);
			btrfs_remove_leaf_ref(root, ref);
			btrfs_free_leaf_ref(root, ref);
		} else {
			/*
			 * the leaf wasn't in the reference cache, so
			 * we have to read it.
			 */
			leaf = read_tree_block(root, bytenr, blocksize,
					       ptr_gen);
			ret = btrfs_drop_leaf_ref(trans, root, leaf);
			BUG_ON(ret);
			free_extent_buffer(leaf);
		}
		atomic_inc(&root->fs_info->throttle_gen);
		wake_up(&root->fs_info->transaction_throttle);
		cond_resched();
	}

	/*
	 * run through the loop again to free the refs on the leaves.
	 * This is faster than doing it in the loop above because
	 * the leaves are likely to be clustered together.  We end up
	 * working in nice chunks on the extent allocation tree.
	 */
	for (i = 0; i < refi; i++) {
		bytenr = sorted[i].bytenr;
		ret = __btrfs_free_extent(trans, root, bytenr,
					blocksize, eb->start,
					root_owner, root_gen, 0, 1);
		BUG_ON(ret);

		atomic_inc(&root->fs_info->throttle_gen);
		wake_up(&root->fs_info->transaction_throttle);
		cond_resched();
	}
out:
	kfree(sorted);

	/*
	 * update the path to show we've processed the entire level 1
	 * node.  This will get saved into the root's drop_snapshot_progress
	 * field so these drops are not repeated again if this transaction
	 * commits.
	 */
	path->slots[1] = nritems;
	return 0;
}

/*
 * helper function for drop_snapshot, this walks down the tree dropping ref
 * counts as it goes.
 */
static noinline int walk_down_tree(struct btrfs_trans_handle *trans,
				   struct btrfs_root *root,
				   struct btrfs_path *path, int *level)
{
	u64 root_owner;
	u64 root_gen;
	u64 bytenr;
	u64 ptr_gen;
	struct extent_buffer *next;
	struct extent_buffer *cur;
	struct extent_buffer *parent;
	u32 blocksize;
	int ret;
	u32 refs;

	WARN_ON(*level < 0);
	WARN_ON(*level >= BTRFS_MAX_LEVEL);
	ret = drop_snap_lookup_refcount(root, path->nodes[*level]->start,
				path->nodes[*level]->len, &refs);
	BUG_ON(ret);
	if (refs > 1)
		goto out;

	/*
	 * walk down to the last node level and free all the leaves
	 */
	while (*level >= 0) {
		WARN_ON(*level < 0);
		WARN_ON(*level >= BTRFS_MAX_LEVEL);
		cur = path->nodes[*level];

		if (btrfs_header_level(cur) != *level)
			WARN_ON(1);

		if (path->slots[*level] >=
		    btrfs_header_nritems(cur))
			break;

		/* the new code goes down to level 1 and does all the
		 * leaves pointed to that node in bulk.  So, this check
		 * for level 0 will always be false.
		 *
		 * But, the disk format allows the drop_snapshot_progress
		 * field in the root to leave things in a state where
		 * a leaf will need cleaning up here.  If someone crashes
		 * with the old code and then boots with the new code,
		 * we might find a leaf here.
		 */
		if (*level == 0) {
			ret = btrfs_drop_leaf_ref(trans, root, cur);
			BUG_ON(ret);
			break;
		}

		/*
		 * once we get to level one, process the whole node
		 * at once, including everything below it.
		 */
		if (*level == 1) {
			ret = drop_level_one_refs(trans, root, path);
			BUG_ON(ret);
			break;
		}

		bytenr = btrfs_node_blockptr(cur, path->slots[*level]);
		ptr_gen = btrfs_node_ptr_generation(cur, path->slots[*level]);
		blocksize = btrfs_level_size(root, *level - 1);

		ret = drop_snap_lookup_refcount(root, bytenr, blocksize, &refs);
		BUG_ON(ret);

		/*
		 * if there is more than one reference, we don't need
		 * to read that node to drop any references it has.  We
		 * just drop the ref we hold on that node and move on to the
		 * next slot in this level.
		 */
		if (refs != 1) {
			parent = path->nodes[*level];
			root_owner = btrfs_header_owner(parent);
			root_gen = btrfs_header_generation(parent);
			path->slots[*level]++;

			ret = __btrfs_free_extent(trans, root, bytenr,
						blocksize, parent->start,
						root_owner, root_gen,
						*level - 1, 1);
			BUG_ON(ret);

			atomic_inc(&root->fs_info->throttle_gen);
			wake_up(&root->fs_info->transaction_throttle);
			cond_resched();

			continue;
		}

		/*
		 * we need to keep freeing things in the next level down.
		 * read the block and loop around to process it
		 */
		next = read_tree_block(root, bytenr, blocksize, ptr_gen);
		WARN_ON(*level <= 0);
		if (path->nodes[*level-1])
			free_extent_buffer(path->nodes[*level-1]);
		path->nodes[*level-1] = next;
		*level = btrfs_header_level(next);
		path->slots[*level] = 0;
		cond_resched();
	}
out:
	WARN_ON(*level < 0);
	WARN_ON(*level >= BTRFS_MAX_LEVEL);

	if (path->nodes[*level] == root->node) {
		parent = path->nodes[*level];
		bytenr = path->nodes[*level]->start;
	} else {
		parent = path->nodes[*level + 1];
		bytenr = btrfs_node_blockptr(parent, path->slots[*level + 1]);
	}

	blocksize = btrfs_level_size(root, *level);
	root_owner = btrfs_header_owner(parent);
	root_gen = btrfs_header_generation(parent);

	/*
	 * cleanup and free the reference on the last node
	 * we processed
	 */
	ret = __btrfs_free_extent(trans, root, bytenr, blocksize,
				  parent->start, root_owner, root_gen,
				  *level, 1);
	free_extent_buffer(path->nodes[*level]);
	path->nodes[*level] = NULL;

	*level += 1;
	BUG_ON(ret);

	cond_resched();
	return 0;
}

/*
 * helper function for drop_subtree, this function is similar to
 * walk_down_tree. The main difference is that it checks reference
 * counts while tree blocks are locked.
 */
static noinline int walk_down_subtree(struct btrfs_trans_handle *trans,
				      struct btrfs_root *root,
				      struct btrfs_path *path, int *level)
{
	struct extent_buffer *next;
	struct extent_buffer *cur;
	struct extent_buffer *parent;
	u64 bytenr;
	u64 ptr_gen;
	u32 blocksize;
	u32 refs;
	int ret;

	cur = path->nodes[*level];
	ret = btrfs_lookup_extent_ref(trans, root, cur->start, cur->len,
				      &refs);
	BUG_ON(ret);
	if (refs > 1)
		goto out;

	while (*level >= 0) {
		cur = path->nodes[*level];
		if (*level == 0) {
			ret = btrfs_drop_leaf_ref(trans, root, cur);
			BUG_ON(ret);
			clean_tree_block(trans, root, cur);
			break;
		}
		if (path->slots[*level] >= btrfs_header_nritems(cur)) {
			clean_tree_block(trans, root, cur);
			break;
		}

		bytenr = btrfs_node_blockptr(cur, path->slots[*level]);
		blocksize = btrfs_level_size(root, *level - 1);
		ptr_gen = btrfs_node_ptr_generation(cur, path->slots[*level]);

		next = read_tree_block(root, bytenr, blocksize, ptr_gen);
		btrfs_tree_lock(next);
		btrfs_set_lock_blocking(next);

		ret = btrfs_lookup_extent_ref(trans, root, bytenr, blocksize,
					      &refs);
		BUG_ON(ret);
		if (refs > 1) {
			parent = path->nodes[*level];
			ret = btrfs_free_extent(trans, root, bytenr,
					blocksize, parent->start,
					btrfs_header_owner(parent),
					btrfs_header_generation(parent),
					*level - 1, 1);
			BUG_ON(ret);
			path->slots[*level]++;
			btrfs_tree_unlock(next);
			free_extent_buffer(next);
			continue;
		}

		*level = btrfs_header_level(next);
		path->nodes[*level] = next;
		path->slots[*level] = 0;
		path->locks[*level] = 1;
		cond_resched();
	}
out:
	parent = path->nodes[*level + 1];
	bytenr = path->nodes[*level]->start;
	blocksize = path->nodes[*level]->len;

	ret = btrfs_free_extent(trans, root, bytenr, blocksize,
			parent->start, btrfs_header_owner(parent),
			btrfs_header_generation(parent), *level, 1);
	BUG_ON(ret);

	if (path->locks[*level]) {
		btrfs_tree_unlock(path->nodes[*level]);
		path->locks[*level] = 0;
	}
	free_extent_buffer(path->nodes[*level]);
	path->nodes[*level] = NULL;
	*level += 1;
	cond_resched();
	return 0;
}

/*
 * helper for dropping snapshots.  This walks back up the tree in the path
 * to find the first node higher up where we haven't yet gone through
 * all the slots
 */
static noinline int walk_up_tree(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 struct btrfs_path *path,
				 int *level, int max_level)
{
	u64 root_owner;
	u64 root_gen;
	struct btrfs_root_item *root_item = &root->root_item;
	int i;
	int slot;
	int ret;

	for (i = *level; i < max_level && path->nodes[i]; i++) {
		slot = path->slots[i];
		if (slot < btrfs_header_nritems(path->nodes[i]) - 1) {
			struct extent_buffer *node;
			struct btrfs_disk_key disk_key;

			/*
			 * there is more work to do in this level.
			 * Update the drop_progress marker to reflect
			 * the work we've done so far, and then bump
			 * the slot number
			 */
			node = path->nodes[i];
			path->slots[i]++;
			*level = i;
			WARN_ON(*level == 0);
			btrfs_node_key(node, &disk_key, path->slots[i]);
			memcpy(&root_item->drop_progress,
			       &disk_key, sizeof(disk_key));
			root_item->drop_level = i;
			return 0;
		} else {
			struct extent_buffer *parent;

			/*
			 * this whole node is done, free our reference
			 * on it and go up one level
			 */
			if (path->nodes[*level] == root->node)
				parent = path->nodes[*level];
			else
				parent = path->nodes[*level + 1];

			root_owner = btrfs_header_owner(parent);
			root_gen = btrfs_header_generation(parent);

			clean_tree_block(trans, root, path->nodes[*level]);
			ret = btrfs_free_extent(trans, root,
						path->nodes[*level]->start,
						path->nodes[*level]->len,
						parent->start, root_owner,
						root_gen, *level, 1);
			BUG_ON(ret);
			if (path->locks[*level]) {
				btrfs_tree_unlock(path->nodes[*level]);
				path->locks[*level] = 0;
			}
			free_extent_buffer(path->nodes[*level]);
			path->nodes[*level] = NULL;
			*level = i + 1;
		}
	}
	return 1;
}

/*
 * drop the reference count on the tree rooted at 'snap'.  This traverses
 * the tree freeing any blocks that have a ref count of zero after being
 * decremented.
 */
int btrfs_drop_snapshot(struct btrfs_trans_handle *trans, struct btrfs_root
			*root)
{
	int ret = 0;
	int wret;
	int level;
	struct btrfs_path *path;
	int i;
	int orig_level;
	struct btrfs_root_item *root_item = &root->root_item;

	WARN_ON(!mutex_is_locked(&root->fs_info->drop_mutex));
	path = btrfs_alloc_path();
	BUG_ON(!path);

	level = btrfs_header_level(root->node);
	orig_level = level;
	if (btrfs_disk_key_objectid(&root_item->drop_progress) == 0) {
		path->nodes[level] = root->node;
		extent_buffer_get(root->node);
		path->slots[level] = 0;
	} else {
		struct btrfs_key key;
		struct btrfs_disk_key found_key;
		struct extent_buffer *node;

		btrfs_disk_key_to_cpu(&key, &root_item->drop_progress);
		level = root_item->drop_level;
		path->lowest_level = level;
		wret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
		if (wret < 0) {
			ret = wret;
			goto out;
		}
		node = path->nodes[level];
		btrfs_node_key(node, &found_key, path->slots[level]);
		WARN_ON(memcmp(&found_key, &root_item->drop_progress,
			       sizeof(found_key)));
		/*
		 * unlock our path, this is safe because only this
		 * function is allowed to delete this snapshot
		 */
		for (i = 0; i < BTRFS_MAX_LEVEL; i++) {
			if (path->nodes[i] && path->locks[i]) {
				path->locks[i] = 0;
				btrfs_tree_unlock(path->nodes[i]);
			}
		}
	}
	while (1) {
		wret = walk_down_tree(trans, root, path, &level);
		if (wret > 0)
			break;
		if (wret < 0)
			ret = wret;

		wret = walk_up_tree(trans, root, path, &level,
				    BTRFS_MAX_LEVEL);
		if (wret > 0)
			break;
		if (wret < 0)
			ret = wret;
		if (trans->transaction->in_commit) {
			ret = -EAGAIN;
			break;
		}
		atomic_inc(&root->fs_info->throttle_gen);
		wake_up(&root->fs_info->transaction_throttle);
	}
	for (i = 0; i <= orig_level; i++) {
		if (path->nodes[i]) {
			free_extent_buffer(path->nodes[i]);
			path->nodes[i] = NULL;
		}
	}
out:
	btrfs_free_path(path);
	return ret;
}

int btrfs_drop_subtree(struct btrfs_trans_handle *trans,
			struct btrfs_root *root,
			struct extent_buffer *node,
			struct extent_buffer *parent)
{
	struct btrfs_path *path;
	int level;
	int parent_level;
	int ret = 0;
	int wret;

	path = btrfs_alloc_path();
	BUG_ON(!path);

	btrfs_assert_tree_locked(parent);
	parent_level = btrfs_header_level(parent);
	extent_buffer_get(parent);
	path->nodes[parent_level] = parent;
	path->slots[parent_level] = btrfs_header_nritems(parent);

	btrfs_assert_tree_locked(node);
	level = btrfs_header_level(node);
	extent_buffer_get(node);
	path->nodes[level] = node;
	path->slots[level] = 0;

	while (1) {
		wret = walk_down_subtree(trans, root, path, &level);
		if (wret < 0)
			ret = wret;
		if (wret != 0)
			break;

		wret = walk_up_tree(trans, root, path, &level, parent_level);
		if (wret < 0)
			ret = wret;
		if (wret != 0)
			break;
	}

	btrfs_free_path(path);
	return ret;
}

static unsigned long calc_ra(unsigned long start, unsigned long last,
			     unsigned long nr)
{
	return min(last, start + nr - 1);
}

static noinline int relocate_inode_pages(struct inode *inode, u64 start,
					 u64 len)
{
	u64 page_start;
	u64 page_end;
	unsigned long first_index;
	unsigned long last_index;
	unsigned long i;
	struct page *page;
	struct extent_io_tree *io_tree = &BTRFS_I(inode)->io_tree;
	struct file_ra_state *ra;
	struct btrfs_ordered_extent *ordered;
	unsigned int total_read = 0;
	unsigned int total_dirty = 0;
	int ret = 0;

	ra = kzalloc(sizeof(*ra), GFP_NOFS);

	mutex_lock(&inode->i_mutex);
	first_index = start >> PAGE_CACHE_SHIFT;
	last_index = (start + len - 1) >> PAGE_CACHE_SHIFT;

	/* make sure the dirty trick played by the caller work */
	ret = invalidate_inode_pages2_range(inode->i_mapping,
					    first_index, last_index);
	if (ret)
		goto out_unlock;

	file_ra_state_init(ra, inode->i_mapping);

	for (i = first_index ; i <= last_index; i++) {
		if (total_read % ra->ra_pages == 0) {
			btrfs_force_ra(inode->i_mapping, ra, NULL, i,
				       calc_ra(i, last_index, ra->ra_pages));
		}
		total_read++;
again:
		if (((u64)i << PAGE_CACHE_SHIFT) > i_size_read(inode))
			BUG_ON(1);
		page = grab_cache_page(inode->i_mapping, i);
		if (!page) {
			ret = -ENOMEM;
			goto out_unlock;
		}
		if (!PageUptodate(page)) {
			btrfs_readpage(NULL, page);
			lock_page(page);
			if (!PageUptodate(page)) {
				unlock_page(page);
				page_cache_release(page);
				ret = -EIO;
				goto out_unlock;
			}
		}
		wait_on_page_writeback(page);

		page_start = (u64)page->index << PAGE_CACHE_SHIFT;
		page_end = page_start + PAGE_CACHE_SIZE - 1;
		lock_extent(io_tree, page_start, page_end, GFP_NOFS);

		ordered = btrfs_lookup_ordered_extent(inode, page_start);
		if (ordered) {
			unlock_extent(io_tree, page_start, page_end, GFP_NOFS);
			unlock_page(page);
			page_cache_release(page);
			btrfs_start_ordered_extent(inode, ordered, 1);
			btrfs_put_ordered_extent(ordered);
			goto again;
		}
		set_page_extent_mapped(page);

		if (i == first_index)
			set_extent_bits(io_tree, page_start, page_end,
					EXTENT_BOUNDARY, GFP_NOFS);
		btrfs_set_extent_delalloc(inode, page_start, page_end);

		set_page_dirty(page);
		total_dirty++;

		unlock_extent(io_tree, page_start, page_end, GFP_NOFS);
		unlock_page(page);
		page_cache_release(page);
	}

out_unlock:
	kfree(ra);
	mutex_unlock(&inode->i_mutex);
	balance_dirty_pages_ratelimited_nr(inode->i_mapping, total_dirty);
	return ret;
}

static noinline int relocate_data_extent(struct inode *reloc_inode,
					 struct btrfs_key *extent_key,
					 u64 offset)
{
	struct btrfs_root *root = BTRFS_I(reloc_inode)->root;
	struct extent_map_tree *em_tree = &BTRFS_I(reloc_inode)->extent_tree;
	struct extent_map *em;
	u64 start = extent_key->objectid - offset;
	u64 end = start + extent_key->offset - 1;

	em = alloc_extent_map(GFP_NOFS);
	BUG_ON(!em || IS_ERR(em));

	em->start = start;
	em->len = extent_key->offset;
	em->block_len = extent_key->offset;
	em->block_start = extent_key->objectid;
	em->bdev = root->fs_info->fs_devices->latest_bdev;
	set_bit(EXTENT_FLAG_PINNED, &em->flags);

	/* setup extent map to cheat btrfs_readpage */
	lock_extent(&BTRFS_I(reloc_inode)->io_tree, start, end, GFP_NOFS);
	while (1) {
		int ret;
		spin_lock(&em_tree->lock);
		ret = add_extent_mapping(em_tree, em);
		spin_unlock(&em_tree->lock);
		if (ret != -EEXIST) {
			free_extent_map(em);
			break;
		}
		btrfs_drop_extent_cache(reloc_inode, start, end, 0);
	}
	unlock_extent(&BTRFS_I(reloc_inode)->io_tree, start, end, GFP_NOFS);

	return relocate_inode_pages(reloc_inode, start, extent_key->offset);
}

struct btrfs_ref_path {
	u64 extent_start;
	u64 nodes[BTRFS_MAX_LEVEL];
	u64 root_objectid;
	u64 root_generation;
	u64 owner_objectid;
	u32 num_refs;
	int lowest_level;
	int current_level;
	int shared_level;

	struct btrfs_key node_keys[BTRFS_MAX_LEVEL];
	u64 new_nodes[BTRFS_MAX_LEVEL];
};

struct disk_extent {
	u64 ram_bytes;
	u64 disk_bytenr;
	u64 disk_num_bytes;
	u64 offset;
	u64 num_bytes;
	u8 compression;
	u8 encryption;
	u16 other_encoding;
};

static int is_cowonly_root(u64 root_objectid)
{
	if (root_objectid == BTRFS_ROOT_TREE_OBJECTID ||
	    root_objectid == BTRFS_EXTENT_TREE_OBJECTID ||
	    root_objectid == BTRFS_CHUNK_TREE_OBJECTID ||
	    root_objectid == BTRFS_DEV_TREE_OBJECTID ||
	    root_objectid == BTRFS_TREE_LOG_OBJECTID ||
	    root_objectid == BTRFS_CSUM_TREE_OBJECTID)
		return 1;
	return 0;
}

static noinline int __next_ref_path(struct btrfs_trans_handle *trans,
				    struct btrfs_root *extent_root,
				    struct btrfs_ref_path *ref_path,
				    int first_time)
{
	struct extent_buffer *leaf;
	struct btrfs_path *path;
	struct btrfs_extent_ref *ref;
	struct btrfs_key key;
	struct btrfs_key found_key;
	u64 bytenr;
	u32 nritems;
	int level;
	int ret = 1;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	if (first_time) {
		ref_path->lowest_level = -1;
		ref_path->current_level = -1;
		ref_path->shared_level = -1;
		goto walk_up;
	}
walk_down:
	level = ref_path->current_level - 1;
	while (level >= -1) {
		u64 parent;
		if (level < ref_path->lowest_level)
			break;

		if (level >= 0)
			bytenr = ref_path->nodes[level];
		else
			bytenr = ref_path->extent_start;
		BUG_ON(bytenr == 0);

		parent = ref_path->nodes[level + 1];
		ref_path->nodes[level + 1] = 0;
		ref_path->current_level = level;
		BUG_ON(parent == 0);

		key.objectid = bytenr;
		key.offset = parent + 1;
		key.type = BTRFS_EXTENT_REF_KEY;

		ret = btrfs_search_slot(trans, extent_root, &key, path, 0, 0);
		if (ret < 0)
			goto out;
		BUG_ON(ret == 0);

		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(extent_root, path);
			if (ret < 0)
				goto out;
			if (ret > 0)
				goto next;
			leaf = path->nodes[0];
		}

		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.objectid == bytenr &&
		    found_key.type == BTRFS_EXTENT_REF_KEY) {
			if (level < ref_path->shared_level)
				ref_path->shared_level = level;
			goto found;
		}
next:
		level--;
		btrfs_release_path(extent_root, path);
		cond_resched();
	}
	/* reached lowest level */
	ret = 1;
	goto out;
walk_up:
	level = ref_path->current_level;
	while (level < BTRFS_MAX_LEVEL - 1) {
		u64 ref_objectid;

		if (level >= 0)
			bytenr = ref_path->nodes[level];
		else
			bytenr = ref_path->extent_start;

		BUG_ON(bytenr == 0);

		key.objectid = bytenr;
		key.offset = 0;
		key.type = BTRFS_EXTENT_REF_KEY;

		ret = btrfs_search_slot(trans, extent_root, &key, path, 0, 0);
		if (ret < 0)
			goto out;

		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(extent_root, path);
			if (ret < 0)
				goto out;
			if (ret > 0) {
				/* the extent was freed by someone */
				if (ref_path->lowest_level == level)
					goto out;
				btrfs_release_path(extent_root, path);
				goto walk_down;
			}
			leaf = path->nodes[0];
		}

		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.objectid != bytenr ||
				found_key.type != BTRFS_EXTENT_REF_KEY) {
			/* the extent was freed by someone */
			if (ref_path->lowest_level == level) {
				ret = 1;
				goto out;
			}
			btrfs_release_path(extent_root, path);
			goto walk_down;
		}
found:
		ref = btrfs_item_ptr(leaf, path->slots[0],
				struct btrfs_extent_ref);
		ref_objectid = btrfs_ref_objectid(leaf, ref);
		if (ref_objectid < BTRFS_FIRST_FREE_OBJECTID) {
			if (first_time) {
				level = (int)ref_objectid;
				BUG_ON(level >= BTRFS_MAX_LEVEL);
				ref_path->lowest_level = level;
				ref_path->current_level = level;
				ref_path->nodes[level] = bytenr;
			} else {
				WARN_ON(ref_objectid != level);
			}
		} else {
			WARN_ON(level != -1);
		}
		first_time = 0;

		if (ref_path->lowest_level == level) {
			ref_path->owner_objectid = ref_objectid;
			ref_path->num_refs = btrfs_ref_num_refs(leaf, ref);
		}

		/*
		 * the block is tree root or the block isn't in reference
		 * counted tree.
		 */
		if (found_key.objectid == found_key.offset ||
		    is_cowonly_root(btrfs_ref_root(leaf, ref))) {
			ref_path->root_objectid = btrfs_ref_root(leaf, ref);
			ref_path->root_generation =
				btrfs_ref_generation(leaf, ref);
			if (level < 0) {
				/* special reference from the tree log */
				ref_path->nodes[0] = found_key.offset;
				ref_path->current_level = 0;
			}
			ret = 0;
			goto out;
		}

		level++;
		BUG_ON(ref_path->nodes[level] != 0);
		ref_path->nodes[level] = found_key.offset;
		ref_path->current_level = level;

		/*
		 * the reference was created in the running transaction,
		 * no need to continue walking up.
		 */
		if (btrfs_ref_generation(leaf, ref) == trans->transid) {
			ref_path->root_objectid = btrfs_ref_root(leaf, ref);
			ref_path->root_generation =
				btrfs_ref_generation(leaf, ref);
			ret = 0;
			goto out;
		}

		btrfs_release_path(extent_root, path);
		cond_resched();
	}
	/* reached max tree level, but no tree root found. */
	BUG();
out:
	btrfs_free_path(path);
	return ret;
}

static int btrfs_first_ref_path(struct btrfs_trans_handle *trans,
				struct btrfs_root *extent_root,
				struct btrfs_ref_path *ref_path,
				u64 extent_start)
{
	memset(ref_path, 0, sizeof(*ref_path));
	ref_path->extent_start = extent_start;

	return __next_ref_path(trans, extent_root, ref_path, 1);
}

static int btrfs_next_ref_path(struct btrfs_trans_handle *trans,
			       struct btrfs_root *extent_root,
			       struct btrfs_ref_path *ref_path)
{
	return __next_ref_path(trans, extent_root, ref_path, 0);
}

static noinline int get_new_locations(struct inode *reloc_inode,
				      struct btrfs_key *extent_key,
				      u64 offset, int no_fragment,
				      struct disk_extent **extents,
				      int *nr_extents)
{
	struct btrfs_root *root = BTRFS_I(reloc_inode)->root;
	struct btrfs_path *path;
	struct btrfs_file_extent_item *fi;
	struct extent_buffer *leaf;
	struct disk_extent *exts = *extents;
	struct btrfs_key found_key;
	u64 cur_pos;
	u64 last_byte;
	u32 nritems;
	int nr = 0;
	int max = *nr_extents;
	int ret;

	WARN_ON(!no_fragment && *extents);
	if (!exts) {
		max = 1;
		exts = kmalloc(sizeof(*exts) * max, GFP_NOFS);
		if (!exts)
			return -ENOMEM;
	}

	path = btrfs_alloc_path();
	BUG_ON(!path);

	cur_pos = extent_key->objectid - offset;
	last_byte = extent_key->objectid + extent_key->offset;
	ret = btrfs_lookup_file_extent(NULL, root, path, reloc_inode->i_ino,
				       cur_pos, 0);
	if (ret < 0)
		goto out;
	if (ret > 0) {
		ret = -ENOENT;
		goto out;
	}

	while (1) {
		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto out;
			if (ret > 0)
				break;
			leaf = path->nodes[0];
		}

		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		if (found_key.offset != cur_pos ||
		    found_key.type != BTRFS_EXTENT_DATA_KEY ||
		    found_key.objectid != reloc_inode->i_ino)
			break;

		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		if (btrfs_file_extent_type(leaf, fi) !=
		    BTRFS_FILE_EXTENT_REG ||
		    btrfs_file_extent_disk_bytenr(leaf, fi) == 0)
			break;

		if (nr == max) {
			struct disk_extent *old = exts;
			max *= 2;
			exts = kzalloc(sizeof(*exts) * max, GFP_NOFS);
			memcpy(exts, old, sizeof(*exts) * nr);
			if (old != *extents)
				kfree(old);
		}

		exts[nr].disk_bytenr =
			btrfs_file_extent_disk_bytenr(leaf, fi);
		exts[nr].disk_num_bytes =
			btrfs_file_extent_disk_num_bytes(leaf, fi);
		exts[nr].offset = btrfs_file_extent_offset(leaf, fi);
		exts[nr].num_bytes = btrfs_file_extent_num_bytes(leaf, fi);
		exts[nr].ram_bytes = btrfs_file_extent_ram_bytes(leaf, fi);
		exts[nr].compression = btrfs_file_extent_compression(leaf, fi);
		exts[nr].encryption = btrfs_file_extent_encryption(leaf, fi);
		exts[nr].other_encoding = btrfs_file_extent_other_encoding(leaf,
									   fi);
		BUG_ON(exts[nr].offset > 0);
		BUG_ON(exts[nr].compression || exts[nr].encryption);
		BUG_ON(exts[nr].num_bytes != exts[nr].disk_num_bytes);

		cur_pos += exts[nr].num_bytes;
		nr++;

		if (cur_pos + offset >= last_byte)
			break;

		if (no_fragment) {
			ret = 1;
			goto out;
		}
		path->slots[0]++;
	}

	BUG_ON(cur_pos + offset > last_byte);
	if (cur_pos + offset < last_byte) {
		ret = -ENOENT;
		goto out;
	}
	ret = 0;
out:
	btrfs_free_path(path);
	if (ret) {
		if (exts != *extents)
			kfree(exts);
	} else {
		*extents = exts;
		*nr_extents = nr;
	}
	return ret;
}

static noinline int replace_one_extent(struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct btrfs_path *path,
					struct btrfs_key *extent_key,
					struct btrfs_key *leaf_key,
					struct btrfs_ref_path *ref_path,
					struct disk_extent *new_extents,
					int nr_extents)
{
	struct extent_buffer *leaf;
	struct btrfs_file_extent_item *fi;
	struct inode *inode = NULL;
	struct btrfs_key key;
	u64 lock_start = 0;
	u64 lock_end = 0;
	u64 num_bytes;
	u64 ext_offset;
	u64 search_end = (u64)-1;
	u32 nritems;
	int nr_scaned = 0;
	int extent_locked = 0;
	int extent_type;
	int ret;

	memcpy(&key, leaf_key, sizeof(key));
	if (ref_path->owner_objectid != BTRFS_MULTIPLE_OBJECTIDS) {
		if (key.objectid < ref_path->owner_objectid ||
		    (key.objectid == ref_path->owner_objectid &&
		     key.type < BTRFS_EXTENT_DATA_KEY)) {
			key.objectid = ref_path->owner_objectid;
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = 0;
		}
	}

	while (1) {
		ret = btrfs_search_slot(trans, root, &key, path, 0, 1);
		if (ret < 0)
			goto out;

		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
next:
		if (extent_locked && ret > 0) {
			/*
			 * the file extent item was modified by someone
			 * before the extent got locked.
			 */
			unlock_extent(&BTRFS_I(inode)->io_tree, lock_start,
				      lock_end, GFP_NOFS);
			extent_locked = 0;
		}

		if (path->slots[0] >= nritems) {
			if (++nr_scaned > 2)
				break;

			BUG_ON(extent_locked);
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto out;
			if (ret > 0)
				break;
			leaf = path->nodes[0];
			nritems = btrfs_header_nritems(leaf);
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);

		if (ref_path->owner_objectid != BTRFS_MULTIPLE_OBJECTIDS) {
			if ((key.objectid > ref_path->owner_objectid) ||
			    (key.objectid == ref_path->owner_objectid &&
			     key.type > BTRFS_EXTENT_DATA_KEY) ||
			    key.offset >= search_end)
				break;
		}

		if (inode && key.objectid != inode->i_ino) {
			BUG_ON(extent_locked);
			btrfs_release_path(root, path);
			mutex_unlock(&inode->i_mutex);
			iput(inode);
			inode = NULL;
			continue;
		}

		if (key.type != BTRFS_EXTENT_DATA_KEY) {
			path->slots[0]++;
			ret = 1;
			goto next;
		}
		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		extent_type = btrfs_file_extent_type(leaf, fi);
		if ((extent_type != BTRFS_FILE_EXTENT_REG &&
		     extent_type != BTRFS_FILE_EXTENT_PREALLOC) ||
		    (btrfs_file_extent_disk_bytenr(leaf, fi) !=
		     extent_key->objectid)) {
			path->slots[0]++;
			ret = 1;
			goto next;
		}

		num_bytes = btrfs_file_extent_num_bytes(leaf, fi);
		ext_offset = btrfs_file_extent_offset(leaf, fi);

		if (search_end == (u64)-1) {
			search_end = key.offset - ext_offset +
				btrfs_file_extent_ram_bytes(leaf, fi);
		}

		if (!extent_locked) {
			lock_start = key.offset;
			lock_end = lock_start + num_bytes - 1;
		} else {
			if (lock_start > key.offset ||
			    lock_end + 1 < key.offset + num_bytes) {
				unlock_extent(&BTRFS_I(inode)->io_tree,
					      lock_start, lock_end, GFP_NOFS);
				extent_locked = 0;
			}
		}

		if (!inode) {
			btrfs_release_path(root, path);

			inode = btrfs_iget_locked(root->fs_info->sb,
						  key.objectid, root);
			if (inode->i_state & I_NEW) {
				BTRFS_I(inode)->root = root;
				BTRFS_I(inode)->location.objectid =
					key.objectid;
				BTRFS_I(inode)->location.type =
					BTRFS_INODE_ITEM_KEY;
				BTRFS_I(inode)->location.offset = 0;
				btrfs_read_locked_inode(inode);
				unlock_new_inode(inode);
			}
			/*
			 * some code call btrfs_commit_transaction while
			 * holding the i_mutex, so we can't use mutex_lock
			 * here.
			 */
			if (is_bad_inode(inode) ||
			    !mutex_trylock(&inode->i_mutex)) {
				iput(inode);
				inode = NULL;
				key.offset = (u64)-1;
				goto skip;
			}
		}

		if (!extent_locked) {
			struct btrfs_ordered_extent *ordered;

			btrfs_release_path(root, path);

			lock_extent(&BTRFS_I(inode)->io_tree, lock_start,
				    lock_end, GFP_NOFS);
			ordered = btrfs_lookup_first_ordered_extent(inode,
								    lock_end);
			if (ordered &&
			    ordered->file_offset <= lock_end &&
			    ordered->file_offset + ordered->len > lock_start) {
				unlock_extent(&BTRFS_I(inode)->io_tree,
					      lock_start, lock_end, GFP_NOFS);
				btrfs_start_ordered_extent(inode, ordered, 1);
				btrfs_put_ordered_extent(ordered);
				key.offset += num_bytes;
				goto skip;
			}
			if (ordered)
				btrfs_put_ordered_extent(ordered);

			extent_locked = 1;
			continue;
		}

		if (nr_extents == 1) {
			/* update extent pointer in place */
			btrfs_set_file_extent_disk_bytenr(leaf, fi,
						new_extents[0].disk_bytenr);
			btrfs_set_file_extent_disk_num_bytes(leaf, fi,
						new_extents[0].disk_num_bytes);
			btrfs_mark_buffer_dirty(leaf);

			btrfs_drop_extent_cache(inode, key.offset,
						key.offset + num_bytes - 1, 0);

			ret = btrfs_inc_extent_ref(trans, root,
						new_extents[0].disk_bytenr,
						new_extents[0].disk_num_bytes,
						leaf->start,
						root->root_key.objectid,
						trans->transid,
						key.objectid);
			BUG_ON(ret);

			ret = btrfs_free_extent(trans, root,
						extent_key->objectid,
						extent_key->offset,
						leaf->start,
						btrfs_header_owner(leaf),
						btrfs_header_generation(leaf),
						key.objectid, 0);
			BUG_ON(ret);

			btrfs_release_path(root, path);
			key.offset += num_bytes;
		} else {
			BUG_ON(1);
#if 0
			u64 alloc_hint;
			u64 extent_len;
			int i;
			/*
			 * drop old extent pointer at first, then insert the
			 * new pointers one bye one
			 */
			btrfs_release_path(root, path);
			ret = btrfs_drop_extents(trans, root, inode, key.offset,
						 key.offset + num_bytes,
						 key.offset, &alloc_hint);
			BUG_ON(ret);

			for (i = 0; i < nr_extents; i++) {
				if (ext_offset >= new_extents[i].num_bytes) {
					ext_offset -= new_extents[i].num_bytes;
					continue;
				}
				extent_len = min(new_extents[i].num_bytes -
						 ext_offset, num_bytes);

				ret = btrfs_insert_empty_item(trans, root,
							      path, &key,
							      sizeof(*fi));
				BUG_ON(ret);

				leaf = path->nodes[0];
				fi = btrfs_item_ptr(leaf, path->slots[0],
						struct btrfs_file_extent_item);
				btrfs_set_file_extent_generation(leaf, fi,
							trans->transid);
				btrfs_set_file_extent_type(leaf, fi,
							BTRFS_FILE_EXTENT_REG);
				btrfs_set_file_extent_disk_bytenr(leaf, fi,
						new_extents[i].disk_bytenr);
				btrfs_set_file_extent_disk_num_bytes(leaf, fi,
						new_extents[i].disk_num_bytes);
				btrfs_set_file_extent_ram_bytes(leaf, fi,
						new_extents[i].ram_bytes);

				btrfs_set_file_extent_compression(leaf, fi,
						new_extents[i].compression);
				btrfs_set_file_extent_encryption(leaf, fi,
						new_extents[i].encryption);
				btrfs_set_file_extent_other_encoding(leaf, fi,
						new_extents[i].other_encoding);

				btrfs_set_file_extent_num_bytes(leaf, fi,
							extent_len);
				ext_offset += new_extents[i].offset;
				btrfs_set_file_extent_offset(leaf, fi,
							ext_offset);
				btrfs_mark_buffer_dirty(leaf);

				btrfs_drop_extent_cache(inode, key.offset,
						key.offset + extent_len - 1, 0);

				ret = btrfs_inc_extent_ref(trans, root,
						new_extents[i].disk_bytenr,
						new_extents[i].disk_num_bytes,
						leaf->start,
						root->root_key.objectid,
						trans->transid, key.objectid);
				BUG_ON(ret);
				btrfs_release_path(root, path);

				inode_add_bytes(inode, extent_len);

				ext_offset = 0;
				num_bytes -= extent_len;
				key.offset += extent_len;

				if (num_bytes == 0)
					break;
			}
			BUG_ON(i >= nr_extents);
#endif
		}

		if (extent_locked) {
			unlock_extent(&BTRFS_I(inode)->io_tree, lock_start,
				      lock_end, GFP_NOFS);
			extent_locked = 0;
		}
skip:
		if (ref_path->owner_objectid != BTRFS_MULTIPLE_OBJECTIDS &&
		    key.offset >= search_end)
			break;

		cond_resched();
	}
	ret = 0;
out:
	btrfs_release_path(root, path);
	if (inode) {
		mutex_unlock(&inode->i_mutex);
		if (extent_locked) {
			unlock_extent(&BTRFS_I(inode)->io_tree, lock_start,
				      lock_end, GFP_NOFS);
		}
		iput(inode);
	}
	return ret;
}

int btrfs_reloc_tree_cache_ref(struct btrfs_trans_handle *trans,
			       struct btrfs_root *root,
			       struct extent_buffer *buf, u64 orig_start)
{
	int level;
	int ret;

	BUG_ON(btrfs_header_generation(buf) != trans->transid);
	BUG_ON(root->root_key.objectid != BTRFS_TREE_RELOC_OBJECTID);

	level = btrfs_header_level(buf);
	if (level == 0) {
		struct btrfs_leaf_ref *ref;
		struct btrfs_leaf_ref *orig_ref;

		orig_ref = btrfs_lookup_leaf_ref(root, orig_start);
		if (!orig_ref)
			return -ENOENT;

		ref = btrfs_alloc_leaf_ref(root, orig_ref->nritems);
		if (!ref) {
			btrfs_free_leaf_ref(root, orig_ref);
			return -ENOMEM;
		}

		ref->nritems = orig_ref->nritems;
		memcpy(ref->extents, orig_ref->extents,
			sizeof(ref->extents[0]) * ref->nritems);

		btrfs_free_leaf_ref(root, orig_ref);

		ref->root_gen = trans->transid;
		ref->bytenr = buf->start;
		ref->owner = btrfs_header_owner(buf);
		ref->generation = btrfs_header_generation(buf);

		ret = btrfs_add_leaf_ref(root, ref, 0);
		WARN_ON(ret);
		btrfs_free_leaf_ref(root, ref);
	}
	return 0;
}

static noinline int invalidate_extent_cache(struct btrfs_root *root,
					struct extent_buffer *leaf,
					struct btrfs_block_group_cache *group,
					struct btrfs_root *target_root)
{
	struct btrfs_key key;
	struct inode *inode = NULL;
	struct btrfs_file_extent_item *fi;
	u64 num_bytes;
	u64 skip_objectid = 0;
	u32 nritems;
	u32 i;

	nritems = btrfs_header_nritems(leaf);
	for (i = 0; i < nritems; i++) {
		btrfs_item_key_to_cpu(leaf, &key, i);
		if (key.objectid == skip_objectid ||
		    key.type != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(leaf, i, struct btrfs_file_extent_item);
		if (btrfs_file_extent_type(leaf, fi) ==
		    BTRFS_FILE_EXTENT_INLINE)
			continue;
		if (btrfs_file_extent_disk_bytenr(leaf, fi) == 0)
			continue;
		if (!inode || inode->i_ino != key.objectid) {
			iput(inode);
			inode = btrfs_ilookup(target_root->fs_info->sb,
					      key.objectid, target_root, 1);
		}
		if (!inode) {
			skip_objectid = key.objectid;
			continue;
		}
		num_bytes = btrfs_file_extent_num_bytes(leaf, fi);

		lock_extent(&BTRFS_I(inode)->io_tree, key.offset,
			    key.offset + num_bytes - 1, GFP_NOFS);
		btrfs_drop_extent_cache(inode, key.offset,
					key.offset + num_bytes - 1, 1);
		unlock_extent(&BTRFS_I(inode)->io_tree, key.offset,
			      key.offset + num_bytes - 1, GFP_NOFS);
		cond_resched();
	}
	iput(inode);
	return 0;
}

static noinline int replace_extents_in_leaf(struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct extent_buffer *leaf,
					struct btrfs_block_group_cache *group,
					struct inode *reloc_inode)
{
	struct btrfs_key key;
	struct btrfs_key extent_key;
	struct btrfs_file_extent_item *fi;
	struct btrfs_leaf_ref *ref;
	struct disk_extent *new_extent;
	u64 bytenr;
	u64 num_bytes;
	u32 nritems;
	u32 i;
	int ext_index;
	int nr_extent;
	int ret;

	new_extent = kmalloc(sizeof(*new_extent), GFP_NOFS);
	BUG_ON(!new_extent);

	ref = btrfs_lookup_leaf_ref(root, leaf->start);
	BUG_ON(!ref);

	ext_index = -1;
	nritems = btrfs_header_nritems(leaf);
	for (i = 0; i < nritems; i++) {
		btrfs_item_key_to_cpu(leaf, &key, i);
		if (btrfs_key_type(&key) != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(leaf, i, struct btrfs_file_extent_item);
		if (btrfs_file_extent_type(leaf, fi) ==
		    BTRFS_FILE_EXTENT_INLINE)
			continue;
		bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
		num_bytes = btrfs_file_extent_disk_num_bytes(leaf, fi);
		if (bytenr == 0)
			continue;

		ext_index++;
		if (bytenr >= group->key.objectid + group->key.offset ||
		    bytenr + num_bytes <= group->key.objectid)
			continue;

		extent_key.objectid = bytenr;
		extent_key.offset = num_bytes;
		extent_key.type = BTRFS_EXTENT_ITEM_KEY;
		nr_extent = 1;
		ret = get_new_locations(reloc_inode, &extent_key,
					group->key.objectid, 1,
					&new_extent, &nr_extent);
		if (ret > 0)
			continue;
		BUG_ON(ret < 0);

		BUG_ON(ref->extents[ext_index].bytenr != bytenr);
		BUG_ON(ref->extents[ext_index].num_bytes != num_bytes);
		ref->extents[ext_index].bytenr = new_extent->disk_bytenr;
		ref->extents[ext_index].num_bytes = new_extent->disk_num_bytes;

		btrfs_set_file_extent_disk_bytenr(leaf, fi,
						new_extent->disk_bytenr);
		btrfs_set_file_extent_disk_num_bytes(leaf, fi,
						new_extent->disk_num_bytes);
		btrfs_mark_buffer_dirty(leaf);

		ret = btrfs_inc_extent_ref(trans, root,
					new_extent->disk_bytenr,
					new_extent->disk_num_bytes,
					leaf->start,
					root->root_key.objectid,
					trans->transid, key.objectid);
		BUG_ON(ret);
		ret = btrfs_free_extent(trans, root,
					bytenr, num_bytes, leaf->start,
					btrfs_header_owner(leaf),
					btrfs_header_generation(leaf),
					key.objectid, 0);
		BUG_ON(ret);
		cond_resched();
	}
	kfree(new_extent);
	BUG_ON(ext_index + 1 != ref->nritems);
	btrfs_free_leaf_ref(root, ref);
	return 0;
}

int btrfs_free_reloc_root(struct btrfs_trans_handle *trans,
			  struct btrfs_root *root)
{
	struct btrfs_root *reloc_root;
	int ret;

	if (root->reloc_root) {
		reloc_root = root->reloc_root;
		root->reloc_root = NULL;
		list_add(&reloc_root->dead_list,
			 &root->fs_info->dead_reloc_roots);

		btrfs_set_root_bytenr(&reloc_root->root_item,
				      reloc_root->node->start);
		btrfs_set_root_level(&root->root_item,
				     btrfs_header_level(reloc_root->node));
		memset(&reloc_root->root_item.drop_progress, 0,
			sizeof(struct btrfs_disk_key));
		reloc_root->root_item.drop_level = 0;

		ret = btrfs_update_root(trans, root->fs_info->tree_root,
					&reloc_root->root_key,
					&reloc_root->root_item);
		BUG_ON(ret);
	}
	return 0;
}

int btrfs_drop_dead_reloc_roots(struct btrfs_root *root)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_root *reloc_root;
	struct btrfs_root *prev_root = NULL;
	struct list_head dead_roots;
	int ret;
	unsigned long nr;

	INIT_LIST_HEAD(&dead_roots);
	list_splice_init(&root->fs_info->dead_reloc_roots, &dead_roots);

	while (!list_empty(&dead_roots)) {
		reloc_root = list_entry(dead_roots.prev,
					struct btrfs_root, dead_list);
		list_del_init(&reloc_root->dead_list);

		BUG_ON(reloc_root->commit_root != NULL);
		while (1) {
			trans = btrfs_join_transaction(root, 1);
			BUG_ON(!trans);

			mutex_lock(&root->fs_info->drop_mutex);
			ret = btrfs_drop_snapshot(trans, reloc_root);
			if (ret != -EAGAIN)
				break;
			mutex_unlock(&root->fs_info->drop_mutex);

			nr = trans->blocks_used;
			ret = btrfs_end_transaction(trans, root);
			BUG_ON(ret);
			btrfs_btree_balance_dirty(root, nr);
		}

		free_extent_buffer(reloc_root->node);

		ret = btrfs_del_root(trans, root->fs_info->tree_root,
				     &reloc_root->root_key);
		BUG_ON(ret);
		mutex_unlock(&root->fs_info->drop_mutex);

		nr = trans->blocks_used;
		ret = btrfs_end_transaction(trans, root);
		BUG_ON(ret);
		btrfs_btree_balance_dirty(root, nr);

		kfree(prev_root);
		prev_root = reloc_root;
	}
	if (prev_root) {
		btrfs_remove_leaf_refs(prev_root, (u64)-1, 0);
		kfree(prev_root);
	}
	return 0;
}

int btrfs_add_dead_reloc_root(struct btrfs_root *root)
{
	list_add(&root->dead_list, &root->fs_info->dead_reloc_roots);
	return 0;
}

int btrfs_cleanup_reloc_trees(struct btrfs_root *root)
{
	struct btrfs_root *reloc_root;
	struct btrfs_trans_handle *trans;
	struct btrfs_key location;
	int found;
	int ret;

	mutex_lock(&root->fs_info->tree_reloc_mutex);
	ret = btrfs_find_dead_roots(root, BTRFS_TREE_RELOC_OBJECTID, NULL);
	BUG_ON(ret);
	found = !list_empty(&root->fs_info->dead_reloc_roots);
	mutex_unlock(&root->fs_info->tree_reloc_mutex);

	if (found) {
		trans = btrfs_start_transaction(root, 1);
		BUG_ON(!trans);
		ret = btrfs_commit_transaction(trans, root);
		BUG_ON(ret);
	}

	location.objectid = BTRFS_DATA_RELOC_TREE_OBJECTID;
	location.offset = (u64)-1;
	location.type = BTRFS_ROOT_ITEM_KEY;

	reloc_root = btrfs_read_fs_root_no_name(root->fs_info, &location);
	BUG_ON(!reloc_root);
	btrfs_orphan_cleanup(reloc_root);
	return 0;
}

static noinline int init_reloc_tree(struct btrfs_trans_handle *trans,
				    struct btrfs_root *root)
{
	struct btrfs_root *reloc_root;
	struct extent_buffer *eb;
	struct btrfs_root_item *root_item;
	struct btrfs_key root_key;
	int ret;

	BUG_ON(!root->ref_cows);
	if (root->reloc_root)
		return 0;

	root_item = kmalloc(sizeof(*root_item), GFP_NOFS);
	BUG_ON(!root_item);

	ret = btrfs_copy_root(trans, root, root->commit_root,
			      &eb, BTRFS_TREE_RELOC_OBJECTID);
	BUG_ON(ret);

	root_key.objectid = BTRFS_TREE_RELOC_OBJECTID;
	root_key.offset = root->root_key.objectid;
	root_key.type = BTRFS_ROOT_ITEM_KEY;

	memcpy(root_item, &root->root_item, sizeof(root_item));
	btrfs_set_root_refs(root_item, 0);
	btrfs_set_root_bytenr(root_item, eb->start);
	btrfs_set_root_level(root_item, btrfs_header_level(eb));
	btrfs_set_root_generation(root_item, trans->transid);

	btrfs_tree_unlock(eb);
	free_extent_buffer(eb);

	ret = btrfs_insert_root(trans, root->fs_info->tree_root,
				&root_key, root_item);
	BUG_ON(ret);
	kfree(root_item);

	reloc_root = btrfs_read_fs_root_no_radix(root->fs_info->tree_root,
						 &root_key);
	BUG_ON(!reloc_root);
	reloc_root->last_trans = trans->transid;
	reloc_root->commit_root = NULL;
	reloc_root->ref_tree = &root->fs_info->reloc_ref_tree;

	root->reloc_root = reloc_root;
	return 0;
}

/*
 * Core function of space balance.
 *
 * The idea is using reloc trees to relocate tree blocks in reference
 * counted roots. There is one reloc tree for each subvol, and all
 * reloc trees share same root key objectid. Reloc trees are snapshots
 * of the latest committed roots of subvols (root->commit_root).
 *
 * To relocate a tree block referenced by a subvol, there are two steps.
 * COW the block through subvol's reloc tree, then update block pointer
 * in the subvol to point to the new block. Since all reloc trees share
 * same root key objectid, doing special handing for tree blocks owned
 * by them is easy. Once a tree block has been COWed in one reloc tree,
 * we can use the resulting new block directly when the same block is
 * required to COW again through other reloc trees. By this way, relocated
 * tree blocks are shared between reloc trees, so they are also shared
 * between subvols.
 */
static noinline int relocate_one_path(struct btrfs_trans_handle *trans,
				      struct btrfs_root *root,
				      struct btrfs_path *path,
				      struct btrfs_key *first_key,
				      struct btrfs_ref_path *ref_path,
				      struct btrfs_block_group_cache *group,
				      struct inode *reloc_inode)
{
	struct btrfs_root *reloc_root;
	struct extent_buffer *eb = NULL;
	struct btrfs_key *keys;
	u64 *nodes;
	int level;
	int shared_level;
	int lowest_level = 0;
	int ret;

	if (ref_path->owner_objectid < BTRFS_FIRST_FREE_OBJECTID)
		lowest_level = ref_path->owner_objectid;

	if (!root->ref_cows) {
		path->lowest_level = lowest_level;
		ret = btrfs_search_slot(trans, root, first_key, path, 0, 1);
		BUG_ON(ret < 0);
		path->lowest_level = 0;
		btrfs_release_path(root, path);
		return 0;
	}

	mutex_lock(&root->fs_info->tree_reloc_mutex);
	ret = init_reloc_tree(trans, root);
	BUG_ON(ret);
	reloc_root = root->reloc_root;

	shared_level = ref_path->shared_level;
	ref_path->shared_level = BTRFS_MAX_LEVEL - 1;

	keys = ref_path->node_keys;
	nodes = ref_path->new_nodes;
	memset(&keys[shared_level + 1], 0,
	       sizeof(*keys) * (BTRFS_MAX_LEVEL - shared_level - 1));
	memset(&nodes[shared_level + 1], 0,
	       sizeof(*nodes) * (BTRFS_MAX_LEVEL - shared_level - 1));

	if (nodes[lowest_level] == 0) {
		path->lowest_level = lowest_level;
		ret = btrfs_search_slot(trans, reloc_root, first_key, path,
					0, 1);
		BUG_ON(ret);
		for (level = lowest_level; level < BTRFS_MAX_LEVEL; level++) {
			eb = path->nodes[level];
			if (!eb || eb == reloc_root->node)
				break;
			nodes[level] = eb->start;
			if (level == 0)
				btrfs_item_key_to_cpu(eb, &keys[level], 0);
			else
				btrfs_node_key_to_cpu(eb, &keys[level], 0);
		}
		if (nodes[0] &&
		    ref_path->owner_objectid >= BTRFS_FIRST_FREE_OBJECTID) {
			eb = path->nodes[0];
			ret = replace_extents_in_leaf(trans, reloc_root, eb,
						      group, reloc_inode);
			BUG_ON(ret);
		}
		btrfs_release_path(reloc_root, path);
	} else {
		ret = btrfs_merge_path(trans, reloc_root, keys, nodes,
				       lowest_level);
		BUG_ON(ret);
	}

	/*
	 * replace tree blocks in the fs tree with tree blocks in
	 * the reloc tree.
	 */
	ret = btrfs_merge_path(trans, root, keys, nodes, lowest_level);
	BUG_ON(ret < 0);

	if (ref_path->owner_objectid >= BTRFS_FIRST_FREE_OBJECTID) {
		ret = btrfs_search_slot(trans, reloc_root, first_key, path,
					0, 0);
		BUG_ON(ret);
		extent_buffer_get(path->nodes[0]);
		eb = path->nodes[0];
		btrfs_release_path(reloc_root, path);
		ret = invalidate_extent_cache(reloc_root, eb, group, root);
		BUG_ON(ret);
		free_extent_buffer(eb);
	}

	mutex_unlock(&root->fs_info->tree_reloc_mutex);
	path->lowest_level = 0;
	return 0;
}

static noinline int relocate_tree_block(struct btrfs_trans_handle *trans,
					struct btrfs_root *root,
					struct btrfs_path *path,
					struct btrfs_key *first_key,
					struct btrfs_ref_path *ref_path)
{
	int ret;

	ret = relocate_one_path(trans, root, path, first_key,
				ref_path, NULL, NULL);
	BUG_ON(ret);

	if (root == root->fs_info->extent_root)
		btrfs_extent_post_op(trans, root);

	return 0;
}

static noinline int del_extent_zero(struct btrfs_trans_handle *trans,
				    struct btrfs_root *extent_root,
				    struct btrfs_path *path,
				    struct btrfs_key *extent_key)
{
	int ret;

	ret = btrfs_search_slot(trans, extent_root, extent_key, path, -1, 1);
	if (ret)
		goto out;
	ret = btrfs_del_item(trans, extent_root, path);
out:
	btrfs_release_path(extent_root, path);
	return ret;
}

static noinline struct btrfs_root *read_ref_root(struct btrfs_fs_info *fs_info,
						struct btrfs_ref_path *ref_path)
{
	struct btrfs_key root_key;

	root_key.objectid = ref_path->root_objectid;
	root_key.type = BTRFS_ROOT_ITEM_KEY;
	if (is_cowonly_root(ref_path->root_objectid))
		root_key.offset = 0;
	else
		root_key.offset = (u64)-1;

	return btrfs_read_fs_root_no_name(fs_info, &root_key);
}

static noinline int relocate_one_extent(struct btrfs_root *extent_root,
					struct btrfs_path *path,
					struct btrfs_key *extent_key,
					struct btrfs_block_group_cache *group,
					struct inode *reloc_inode, int pass)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_root *found_root;
	struct btrfs_ref_path *ref_path = NULL;
	struct disk_extent *new_extents = NULL;
	int nr_extents = 0;
	int loops;
	int ret;
	int level;
	struct btrfs_key first_key;
	u64 prev_block = 0;


	trans = btrfs_start_transaction(extent_root, 1);
	BUG_ON(!trans);

	if (extent_key->objectid == 0) {
		ret = del_extent_zero(trans, extent_root, path, extent_key);
		goto out;
	}

	ref_path = kmalloc(sizeof(*ref_path), GFP_NOFS);
	if (!ref_path) {
		ret = -ENOMEM;
		goto out;
	}

	for (loops = 0; ; loops++) {
		if (loops == 0) {
			ret = btrfs_first_ref_path(trans, extent_root, ref_path,
						   extent_key->objectid);
		} else {
			ret = btrfs_next_ref_path(trans, extent_root, ref_path);
		}
		if (ret < 0)
			goto out;
		if (ret > 0)
			break;

		if (ref_path->root_objectid == BTRFS_TREE_LOG_OBJECTID ||
		    ref_path->root_objectid == BTRFS_TREE_RELOC_OBJECTID)
			continue;

		found_root = read_ref_root(extent_root->fs_info, ref_path);
		BUG_ON(!found_root);
		/*
		 * for reference counted tree, only process reference paths
		 * rooted at the latest committed root.
		 */
		if (found_root->ref_cows &&
		    ref_path->root_generation != found_root->root_key.offset)
			continue;

		if (ref_path->owner_objectid >= BTRFS_FIRST_FREE_OBJECTID) {
			if (pass == 0) {
				/*
				 * copy data extents to new locations
				 */
				u64 group_start = group->key.objectid;
				ret = relocate_data_extent(reloc_inode,
							   extent_key,
							   group_start);
				if (ret < 0)
					goto out;
				break;
			}
			level = 0;
		} else {
			level = ref_path->owner_objectid;
		}

		if (prev_block != ref_path->nodes[level]) {
			struct extent_buffer *eb;
			u64 block_start = ref_path->nodes[level];
			u64 block_size = btrfs_level_size(found_root, level);

			eb = read_tree_block(found_root, block_start,
					     block_size, 0);
			btrfs_tree_lock(eb);
			BUG_ON(level != btrfs_header_level(eb));

			if (level == 0)
				btrfs_item_key_to_cpu(eb, &first_key, 0);
			else
				btrfs_node_key_to_cpu(eb, &first_key, 0);

			btrfs_tree_unlock(eb);
			free_extent_buffer(eb);
			prev_block = block_start;
		}

		mutex_lock(&extent_root->fs_info->trans_mutex);
		btrfs_record_root_in_trans(found_root);
		mutex_unlock(&extent_root->fs_info->trans_mutex);
		if (ref_path->owner_objectid >= BTRFS_FIRST_FREE_OBJECTID) {
			/*
			 * try to update data extent references while
			 * keeping metadata shared between snapshots.
			 */
			if (pass == 1) {
				ret = relocate_one_path(trans, found_root,
						path, &first_key, ref_path,
						group, reloc_inode);
				if (ret < 0)
					goto out;
				continue;
			}
			/*
			 * use fallback method to process the remaining
			 * references.
			 */
			if (!new_extents) {
				u64 group_start = group->key.objectid;
				new_extents = kmalloc(sizeof(*new_extents),
						      GFP_NOFS);
				nr_extents = 1;
				ret = get_new_locations(reloc_inode,
							extent_key,
							group_start, 1,
							&new_extents,
							&nr_extents);
				if (ret)
					goto out;
			}
			ret = replace_one_extent(trans, found_root,
						path, extent_key,
						&first_key, ref_path,
						new_extents, nr_extents);
		} else {
			ret = relocate_tree_block(trans, found_root, path,
						  &first_key, ref_path);
		}
		if (ret < 0)
			goto out;
	}
	ret = 0;
out:
	btrfs_end_transaction(trans, extent_root);
	kfree(new_extents);
	kfree(ref_path);
	return ret;
}

static u64 update_block_group_flags(struct btrfs_root *root, u64 flags)
{
	u64 num_devices;
	u64 stripped = BTRFS_BLOCK_GROUP_RAID0 |
		BTRFS_BLOCK_GROUP_RAID1 | BTRFS_BLOCK_GROUP_RAID10;

	num_devices = root->fs_info->fs_devices->rw_devices;
	if (num_devices == 1) {
		stripped |= BTRFS_BLOCK_GROUP_DUP;
		stripped = flags & ~stripped;

		/* turn raid0 into single device chunks */
		if (flags & BTRFS_BLOCK_GROUP_RAID0)
			return stripped;

		/* turn mirroring into duplication */
		if (flags & (BTRFS_BLOCK_GROUP_RAID1 |
			     BTRFS_BLOCK_GROUP_RAID10))
			return stripped | BTRFS_BLOCK_GROUP_DUP;
		return flags;
	} else {
		/* they already had raid on here, just return */
		if (flags & stripped)
			return flags;

		stripped |= BTRFS_BLOCK_GROUP_DUP;
		stripped = flags & ~stripped;

		/* switch duplicated blocks with raid1 */
		if (flags & BTRFS_BLOCK_GROUP_DUP)
			return stripped | BTRFS_BLOCK_GROUP_RAID1;

		/* turn single device chunks into raid0 */
		return stripped | BTRFS_BLOCK_GROUP_RAID0;
	}
	return flags;
}

static int __alloc_chunk_for_shrink(struct btrfs_root *root,
		     struct btrfs_block_group_cache *shrink_block_group,
		     int force)
{
	struct btrfs_trans_handle *trans;
	u64 new_alloc_flags;
	u64 calc;

	spin_lock(&shrink_block_group->lock);
	if (btrfs_block_group_used(&shrink_block_group->item) > 0) {
		spin_unlock(&shrink_block_group->lock);

		trans = btrfs_start_transaction(root, 1);
		spin_lock(&shrink_block_group->lock);

		new_alloc_flags = update_block_group_flags(root,
						   shrink_block_group->flags);
		if (new_alloc_flags != shrink_block_group->flags) {
			calc =
			     btrfs_block_group_used(&shrink_block_group->item);
		} else {
			calc = shrink_block_group->key.offset;
		}
		spin_unlock(&shrink_block_group->lock);

		do_chunk_alloc(trans, root->fs_info->extent_root,
			       calc + 2 * 1024 * 1024, new_alloc_flags, force);

		btrfs_end_transaction(trans, root);
	} else
		spin_unlock(&shrink_block_group->lock);
	return 0;
}

static int __insert_orphan_inode(struct btrfs_trans_handle *trans,
				 struct btrfs_root *root,
				 u64 objectid, u64 size)
{
	struct btrfs_path *path;
	struct btrfs_inode_item *item;
	struct extent_buffer *leaf;
	int ret;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	ret = btrfs_insert_empty_inode(trans, root, path, objectid);
	if (ret)
		goto out;

	leaf = path->nodes[0];
	item = btrfs_item_ptr(leaf, path->slots[0], struct btrfs_inode_item);
	memset_extent_buffer(leaf, 0, (unsigned long)item, sizeof(*item));
	btrfs_set_inode_generation(leaf, item, 1);
	btrfs_set_inode_size(leaf, item, size);
	btrfs_set_inode_mode(leaf, item, S_IFREG | 0600);
	btrfs_set_inode_flags(leaf, item, BTRFS_INODE_NOCOMPRESS);
	btrfs_mark_buffer_dirty(leaf);
	btrfs_release_path(root, path);
out:
	btrfs_free_path(path);
	return ret;
}

static noinline struct inode *create_reloc_inode(struct btrfs_fs_info *fs_info,
					struct btrfs_block_group_cache *group)
{
	struct inode *inode = NULL;
	struct btrfs_trans_handle *trans;
	struct btrfs_root *root;
	struct btrfs_key root_key;
	u64 objectid = BTRFS_FIRST_FREE_OBJECTID;
	int err = 0;

	root_key.objectid = BTRFS_DATA_RELOC_TREE_OBJECTID;
	root_key.type = BTRFS_ROOT_ITEM_KEY;
	root_key.offset = (u64)-1;
	root = btrfs_read_fs_root_no_name(fs_info, &root_key);
	if (IS_ERR(root))
		return ERR_CAST(root);

	trans = btrfs_start_transaction(root, 1);
	BUG_ON(!trans);

	err = btrfs_find_free_objectid(trans, root, objectid, &objectid);
	if (err)
		goto out;

	err = __insert_orphan_inode(trans, root, objectid, group->key.offset);
	BUG_ON(err);

	err = btrfs_insert_file_extent(trans, root, objectid, 0, 0, 0,
				       group->key.offset, 0, group->key.offset,
				       0, 0, 0);
	BUG_ON(err);

	inode = btrfs_iget_locked(root->fs_info->sb, objectid, root);
	if (inode->i_state & I_NEW) {
		BTRFS_I(inode)->root = root;
		BTRFS_I(inode)->location.objectid = objectid;
		BTRFS_I(inode)->location.type = BTRFS_INODE_ITEM_KEY;
		BTRFS_I(inode)->location.offset = 0;
		btrfs_read_locked_inode(inode);
		unlock_new_inode(inode);
		BUG_ON(is_bad_inode(inode));
	} else {
		BUG_ON(1);
	}
	BTRFS_I(inode)->index_cnt = group->key.objectid;

	err = btrfs_orphan_add(trans, inode);
out:
	btrfs_end_transaction(trans, root);
	if (err) {
		if (inode)
			iput(inode);
		inode = ERR_PTR(err);
	}
	return inode;
}

int btrfs_reloc_clone_csums(struct inode *inode, u64 file_pos, u64 len)
{

	struct btrfs_ordered_sum *sums;
	struct btrfs_sector_sum *sector_sum;
	struct btrfs_ordered_extent *ordered;
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct list_head list;
	size_t offset;
	int ret;
	u64 disk_bytenr;

	INIT_LIST_HEAD(&list);

	ordered = btrfs_lookup_ordered_extent(inode, file_pos);
	BUG_ON(ordered->file_offset != file_pos || ordered->len != len);

	disk_bytenr = file_pos + BTRFS_I(inode)->index_cnt;
	ret = btrfs_lookup_csums_range(root->fs_info->csum_root, disk_bytenr,
				       disk_bytenr + len - 1, &list);

	while (!list_empty(&list)) {
		sums = list_entry(list.next, struct btrfs_ordered_sum, list);
		list_del_init(&sums->list);

		sector_sum = sums->sums;
		sums->bytenr = ordered->start;

		offset = 0;
		while (offset < sums->len) {
			sector_sum->bytenr += ordered->start - disk_bytenr;
			sector_sum++;
			offset += root->sectorsize;
		}

		btrfs_add_ordered_sum(inode, ordered, sums);
	}
	btrfs_put_ordered_extent(ordered);
	return 0;
}

int btrfs_relocate_block_group(struct btrfs_root *root, u64 group_start)
{
	struct btrfs_trans_handle *trans;
	struct btrfs_path *path;
	struct btrfs_fs_info *info = root->fs_info;
	struct extent_buffer *leaf;
	struct inode *reloc_inode;
	struct btrfs_block_group_cache *block_group;
	struct btrfs_key key;
	u64 skipped;
	u64 cur_byte;
	u64 total_found;
	u32 nritems;
	int ret;
	int progress;
	int pass = 0;

	root = root->fs_info->extent_root;

	block_group = btrfs_lookup_block_group(info, group_start);
	BUG_ON(!block_group);

	printk(KERN_INFO "btrfs relocating block group %llu flags %llu\n",
	       (unsigned long long)block_group->key.objectid,
	       (unsigned long long)block_group->flags);

	path = btrfs_alloc_path();
	BUG_ON(!path);

	reloc_inode = create_reloc_inode(info, block_group);
	BUG_ON(IS_ERR(reloc_inode));

	__alloc_chunk_for_shrink(root, block_group, 1);
	set_block_group_readonly(block_group);

	btrfs_start_delalloc_inodes(info->tree_root);
	btrfs_wait_ordered_extents(info->tree_root, 0);
again:
	skipped = 0;
	total_found = 0;
	progress = 0;
	key.objectid = block_group->key.objectid;
	key.offset = 0;
	key.type = 0;
	cur_byte = key.objectid;

	trans = btrfs_start_transaction(info->tree_root, 1);
	btrfs_commit_transaction(trans, info->tree_root);

	mutex_lock(&root->fs_info->cleaner_mutex);
	btrfs_clean_old_snapshots(info->tree_root);
	btrfs_remove_leaf_refs(info->tree_root, (u64)-1, 1);
	mutex_unlock(&root->fs_info->cleaner_mutex);

	while (1) {
		ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
		if (ret < 0)
			goto out;
next:
		leaf = path->nodes[0];
		nritems = btrfs_header_nritems(leaf);
		if (path->slots[0] >= nritems) {
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				goto out;
			if (ret == 1) {
				ret = 0;
				break;
			}
			leaf = path->nodes[0];
			nritems = btrfs_header_nritems(leaf);
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);

		if (key.objectid >= block_group->key.objectid +
		    block_group->key.offset)
			break;

		if (progress && need_resched()) {
			btrfs_release_path(root, path);
			cond_resched();
			progress = 0;
			continue;
		}
		progress = 1;

		if (btrfs_key_type(&key) != BTRFS_EXTENT_ITEM_KEY ||
		    key.objectid + key.offset <= cur_byte) {
			path->slots[0]++;
			goto next;
		}

		total_found++;
		cur_byte = key.objectid + key.offset;
		btrfs_release_path(root, path);

		__alloc_chunk_for_shrink(root, block_group, 0);
		ret = relocate_one_extent(root, path, &key, block_group,
					  reloc_inode, pass);
		BUG_ON(ret < 0);
		if (ret > 0)
			skipped++;

		key.objectid = cur_byte;
		key.type = 0;
		key.offset = 0;
	}

	btrfs_release_path(root, path);

	if (pass == 0) {
		btrfs_wait_ordered_range(reloc_inode, 0, (u64)-1);
		invalidate_mapping_pages(reloc_inode->i_mapping, 0, -1);
	}

	if (total_found > 0) {
		printk(KERN_INFO "btrfs found %llu extents in pass %d\n",
		       (unsigned long long)total_found, pass);
		pass++;
		if (total_found == skipped && pass > 2) {
			iput(reloc_inode);
			reloc_inode = create_reloc_inode(info, block_group);
			pass = 0;
		}
		goto again;
	}

	/* delete reloc_inode */
	iput(reloc_inode);

	/* unpin extents in this range */
	trans = btrfs_start_transaction(info->tree_root, 1);
	btrfs_commit_transaction(trans, info->tree_root);

	spin_lock(&block_group->lock);
	WARN_ON(block_group->pinned > 0);
	WARN_ON(block_group->reserved > 0);
	WARN_ON(btrfs_block_group_used(&block_group->item) > 0);
	spin_unlock(&block_group->lock);
	put_block_group(block_group);
	ret = 0;
out:
	btrfs_free_path(path);
	return ret;
}

static int find_first_block_group(struct btrfs_root *root,
		struct btrfs_path *path, struct btrfs_key *key)
{
	int ret = 0;
	struct btrfs_key found_key;
	struct extent_buffer *leaf;
	int slot;

	ret = btrfs_search_slot(NULL, root, key, path, 0, 0);
	if (ret < 0)
		goto out;

	while (1) {
		slot = path->slots[0];
		leaf = path->nodes[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			ret = btrfs_next_leaf(root, path);
			if (ret == 0)
				continue;
			if (ret < 0)
				goto out;
			break;
		}
		btrfs_item_key_to_cpu(leaf, &found_key, slot);

		if (found_key.objectid >= key->objectid &&
		    found_key.type == BTRFS_BLOCK_GROUP_ITEM_KEY) {
			ret = 0;
			goto out;
		}
		path->slots[0]++;
	}
	ret = -ENOENT;
out:
	return ret;
}

int btrfs_free_block_groups(struct btrfs_fs_info *info)
{
	struct btrfs_block_group_cache *block_group;
	struct btrfs_space_info *space_info;
	struct rb_node *n;

	spin_lock(&info->block_group_cache_lock);
	while ((n = rb_last(&info->block_group_cache_tree)) != NULL) {
		block_group = rb_entry(n, struct btrfs_block_group_cache,
				       cache_node);
		rb_erase(&block_group->cache_node,
			 &info->block_group_cache_tree);
		spin_unlock(&info->block_group_cache_lock);

		btrfs_remove_free_space_cache(block_group);
		down_write(&block_group->space_info->groups_sem);
		list_del(&block_group->list);
		up_write(&block_group->space_info->groups_sem);

		WARN_ON(atomic_read(&block_group->count) != 1);
		kfree(block_group);

		spin_lock(&info->block_group_cache_lock);
	}
	spin_unlock(&info->block_group_cache_lock);

	/* now that all the block groups are freed, go through and
	 * free all the space_info structs.  This is only called during
	 * the final stages of unmount, and so we know nobody is
	 * using them.  We call synchronize_rcu() once before we start,
	 * just to be on the safe side.
	 */
	synchronize_rcu();

	while(!list_empty(&info->space_info)) {
		space_info = list_entry(info->space_info.next,
					struct btrfs_space_info,
					list);

		list_del(&space_info->list);
		kfree(space_info);
	}
	return 0;
}

int btrfs_read_block_groups(struct btrfs_root *root)
{
	struct btrfs_path *path;
	int ret;
	struct btrfs_block_group_cache *cache;
	struct btrfs_fs_info *info = root->fs_info;
	struct btrfs_space_info *space_info;
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct extent_buffer *leaf;

	root = info->extent_root;
	key.objectid = 0;
	key.offset = 0;
	btrfs_set_key_type(&key, BTRFS_BLOCK_GROUP_ITEM_KEY);
	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;

	while (1) {
		ret = find_first_block_group(root, path, &key);
		if (ret > 0) {
			ret = 0;
			goto error;
		}
		if (ret != 0)
			goto error;

		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &found_key, path->slots[0]);
		cache = kzalloc(sizeof(*cache), GFP_NOFS);
		if (!cache) {
			ret = -ENOMEM;
			break;
		}

		atomic_set(&cache->count, 1);
		spin_lock_init(&cache->lock);
		mutex_init(&cache->alloc_mutex);
		mutex_init(&cache->cache_mutex);
		INIT_LIST_HEAD(&cache->list);
		read_extent_buffer(leaf, &cache->item,
				   btrfs_item_ptr_offset(leaf, path->slots[0]),
				   sizeof(cache->item));
		memcpy(&cache->key, &found_key, sizeof(found_key));

		key.objectid = found_key.objectid + found_key.offset;
		btrfs_release_path(root, path);
		cache->flags = btrfs_block_group_flags(&cache->item);

		ret = update_space_info(info, cache->flags, found_key.offset,
					btrfs_block_group_used(&cache->item),
					&space_info);
		BUG_ON(ret);
		cache->space_info = space_info;
		down_write(&space_info->groups_sem);
		list_add_tail(&cache->list, &space_info->block_groups);
		up_write(&space_info->groups_sem);

		ret = btrfs_add_block_group_cache(root->fs_info, cache);
		BUG_ON(ret);

		set_avail_alloc_bits(root->fs_info, cache->flags);
		if (btrfs_chunk_readonly(root, cache->key.objectid))
			set_block_group_readonly(cache);
	}
	ret = 0;
error:
	btrfs_free_path(path);
	return ret;
}

int btrfs_make_block_group(struct btrfs_trans_handle *trans,
			   struct btrfs_root *root, u64 bytes_used,
			   u64 type, u64 chunk_objectid, u64 chunk_offset,
			   u64 size)
{
	int ret;
	struct btrfs_root *extent_root;
	struct btrfs_block_group_cache *cache;

	extent_root = root->fs_info->extent_root;

	root->fs_info->last_trans_new_blockgroup = trans->transid;

	cache = kzalloc(sizeof(*cache), GFP_NOFS);
	if (!cache)
		return -ENOMEM;

	cache->key.objectid = chunk_offset;
	cache->key.offset = size;
	cache->key.type = BTRFS_BLOCK_GROUP_ITEM_KEY;
	atomic_set(&cache->count, 1);
	spin_lock_init(&cache->lock);
	mutex_init(&cache->alloc_mutex);
	mutex_init(&cache->cache_mutex);
	INIT_LIST_HEAD(&cache->list);

	btrfs_set_block_group_used(&cache->item, bytes_used);
	btrfs_set_block_group_chunk_objectid(&cache->item, chunk_objectid);
	cache->flags = type;
	btrfs_set_block_group_flags(&cache->item, type);

	ret = update_space_info(root->fs_info, cache->flags, size, bytes_used,
				&cache->space_info);
	BUG_ON(ret);
	down_write(&cache->space_info->groups_sem);
	list_add_tail(&cache->list, &cache->space_info->block_groups);
	up_write(&cache->space_info->groups_sem);

	ret = btrfs_add_block_group_cache(root->fs_info, cache);
	BUG_ON(ret);

	ret = btrfs_insert_item(trans, extent_root, &cache->key, &cache->item,
				sizeof(cache->item));
	BUG_ON(ret);

	finish_current_insert(trans, extent_root, 0);
	ret = del_pending_extents(trans, extent_root, 0);
	BUG_ON(ret);
	set_avail_alloc_bits(extent_root->fs_info, type);

	return 0;
}

int btrfs_remove_block_group(struct btrfs_trans_handle *trans,
			     struct btrfs_root *root, u64 group_start)
{
	struct btrfs_path *path;
	struct btrfs_block_group_cache *block_group;
	struct btrfs_key key;
	int ret;

	root = root->fs_info->extent_root;

	block_group = btrfs_lookup_block_group(root->fs_info, group_start);
	BUG_ON(!block_group);
	BUG_ON(!block_group->ro);

	memcpy(&key, &block_group->key, sizeof(key));

	path = btrfs_alloc_path();
	BUG_ON(!path);

	spin_lock(&root->fs_info->block_group_cache_lock);
	rb_erase(&block_group->cache_node,
		 &root->fs_info->block_group_cache_tree);
	spin_unlock(&root->fs_info->block_group_cache_lock);
	btrfs_remove_free_space_cache(block_group);
	down_write(&block_group->space_info->groups_sem);
	list_del(&block_group->list);
	up_write(&block_group->space_info->groups_sem);

	spin_lock(&block_group->space_info->lock);
	block_group->space_info->total_bytes -= block_group->key.offset;
	block_group->space_info->bytes_readonly -= block_group->key.offset;
	spin_unlock(&block_group->space_info->lock);
	block_group->space_info->full = 0;

	put_block_group(block_group);
	put_block_group(block_group);

	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
	if (ret > 0)
		ret = -EIO;
	if (ret < 0)
		goto out;

	ret = btrfs_del_item(trans, root, path);
out:
	btrfs_free_path(path);
	return ret;
}
