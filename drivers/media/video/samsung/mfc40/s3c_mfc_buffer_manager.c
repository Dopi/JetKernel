/*
 * drivers/media/video/samsung/mfc40/s3c_mfc_buffer_manager.c
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

#include <linux/types.h>
#include <linux/slab.h>
#include <plat/media.h>

#include "s3c_mfc_buffer_manager.h"
#include "s3c_mfc_errorno.h"
#include "s3c_mfc_logmsg.h"

s3c_mfc_alloc_mem_t *s3c_mfc_alloc_mem_head;
s3c_mfc_alloc_mem_t *s3c_mfc_alloc_mem_tail;
s3c_mfc_free_mem_t *s3c_mfc_free_mem_head;
s3c_mfc_free_mem_t *s3c_mfc_free_mem_tail;

extern dma_addr_t s3c_mfc_phys_data_buf;
extern unsigned char *s3c_mfc_virt_data_buf;


/* insert node ahead of s3c_mfc_alloc_mem_head */
static void s3c_mfc_insert_node_to_alloc_list(s3c_mfc_alloc_mem_t *node, int inst_no)
{
	mfc_debug("[%d]instance (cached_p_addr : 0x%08x uncached_p_addr : 0x%08x size:%d cacheflag : %d)\n",
			inst_no, node->cached_p_addr, node->uncached_p_addr, node->size, node->cache_flag);
	node->next = s3c_mfc_alloc_mem_head;
	node->prev = s3c_mfc_alloc_mem_head->prev;
	s3c_mfc_alloc_mem_head->prev->next = node;
	s3c_mfc_alloc_mem_head->prev = node;
	s3c_mfc_alloc_mem_head = node;
}

void s3c_mfc_print_list(void)
{
	s3c_mfc_alloc_mem_t *node1;
	s3c_mfc_free_mem_t *node2;
	int count = 0;
	unsigned int p_addr;

	for (node1 = s3c_mfc_alloc_mem_head; node1 != s3c_mfc_alloc_mem_tail; node1 = node1->next) {
		if(node1->cache_flag)
			p_addr = node1->cached_p_addr;
		else
			p_addr = (unsigned int)node1->uncached_p_addr;

 		printk("s3c_mfc_print_list [AllocList][%d] inst_no : %d p_addr : 0x%08x v_addr:0x%08x size:%d cacheflag : %d\n", 
				count++, node1->inst_no,  p_addr, (unsigned int)node1->v_addr, node1->size, node1->cache_flag);

	}

	count = 0;
	for (node2 = s3c_mfc_free_mem_head; node2 != s3c_mfc_free_mem_tail; node2 = node2->next) {
		printk("s3c_mfc_print_list [FreeList][%d] startAddr : 0x%08x size:%d\n", 
				count++, node2->start_addr , node2->size);
	}
}

int list_count()
{
	int count = 0;
	s3c_mfc_free_mem_t *node;

	node = s3c_mfc_free_mem_head;
	
	while (node != s3c_mfc_free_mem_tail) {	
		node = node->next;
		count++;
	}

	return count;
}

static void s3c_mfc_insert_first_node_to_free_list(s3c_mfc_free_mem_t *node,  int inst_no)
{
	mfc_debug("[%d]instance(startAddr : 0x%08x size:%d  cached flag : %d)\n",
			inst_no, node->start_addr, node->size, node->cache_flag);	

	node->next = s3c_mfc_free_mem_head;
	node->prev = s3c_mfc_free_mem_head->prev;
	s3c_mfc_free_mem_head->prev->next = node;
	s3c_mfc_free_mem_head->prev = node;
	s3c_mfc_free_mem_head = node;
	
}

/* insert node ahead of s3c_mfc_free_mem_head */
static void s3c_mfc_insert_node_to_free_list(s3c_mfc_free_mem_t *node,  int inst_no)
{
	s3c_mfc_free_mem_t *itr_node;
	
	mfc_debug("[%d]instance(startAddr : 0x%08x size:%d  cached flag : %d)\n",
			inst_no, node->start_addr, node->size, node->cache_flag);	

	itr_node = s3c_mfc_free_mem_head;
	
	while (itr_node != s3c_mfc_free_mem_tail) {
		
		if (itr_node->start_addr >= node->start_addr) {
			/* head */
			if (itr_node == s3c_mfc_free_mem_head) {
				node->next = s3c_mfc_free_mem_head;
				node->prev = s3c_mfc_free_mem_head->prev;
				s3c_mfc_free_mem_head->prev->next = node;
				s3c_mfc_free_mem_head->prev = node;
				s3c_mfc_free_mem_head = node;
				break;
			} else { /* mid */
				node->next = itr_node;
				node->prev = itr_node->prev;
				itr_node->prev->next = node;
				itr_node->prev = node;
				break;
			}
		
		}

		itr_node = itr_node->next;
	}

	/* tail */
	if (itr_node == s3c_mfc_free_mem_tail) {
		node->next = s3c_mfc_free_mem_tail;
		node->prev = s3c_mfc_free_mem_tail->prev;
		s3c_mfc_free_mem_tail->prev->next = node;
		s3c_mfc_free_mem_tail->prev = node;
	}
	
}

static void s3c_mfc_del_node_from_alloc_list(s3c_mfc_alloc_mem_t *node, int inst_no)
{
	mfc_debug("[%d]instance (uncached_p_addr : 0x%08x cached_p_addr : 0x%08x size:%d cacheflag : %d)\n",
			inst_no, node->uncached_p_addr, node->cached_p_addr, node->size, node->cache_flag);

	if(node == s3c_mfc_alloc_mem_tail){
		mfc_info("InValid node\n");
		return;
	}

	if(node == s3c_mfc_alloc_mem_head)
		s3c_mfc_alloc_mem_head = node->next;

	node->prev->next = node->next;
	node->next->prev = node->prev;

	kfree(node);
}



static void s3c_mfc_del_node_from_free_list( s3c_mfc_free_mem_t *node, int inst_no)
{
	mfc_debug("[%d]s3c_mfc_del_node_from_free_list(startAddr : 0x%08x size:%d)\n", 
						inst_no, node->start_addr, node->size);
	if(node == s3c_mfc_free_mem_tail){
		mfc_err("InValid node\n");
		return;
	}

	if(node == s3c_mfc_free_mem_head)
		s3c_mfc_free_mem_head = node->next;

	node->prev->next = node->next;
	node->next->prev = node->prev;

	kfree(node);
}

/* Remove Fragmentation in FreeMemList */
void s3c_mfc_merge_frag(int inst_no)
{
	s3c_mfc_free_mem_t *node1, *node2;

	node1 = s3c_mfc_free_mem_head;

	while (node1 != s3c_mfc_free_mem_tail) {
		node2 = s3c_mfc_free_mem_head;
		while (node2 != s3c_mfc_free_mem_tail) {
			if ((node1->start_addr + node1->size == node2->start_addr) && (node1->cache_flag == node2->cache_flag)) {
				node1->size += node2->size;
				mfc_debug("find merge area !! ( node1->start_addr + node1->size == node2->start_addr)\n");
				s3c_mfc_del_node_from_free_list(node2, inst_no);
				break;
			} else if((node1->start_addr == node2->start_addr + node2->size) && 
						(node1->cache_flag == node2->cache_flag) ) {
				mfc_debug("find merge area !! ( node1->start_addr == node2->start_addr + node2->size)\n");
				node1->start_addr = node2->start_addr;
				node1->size += node2->size;
				s3c_mfc_del_node_from_free_list(node2, inst_no);
				break;
			}
			node2 = node2->next;
		}
		node1 = node1->next;
	}
}

static unsigned int s3c_mfc_get_mem_area(int allocSize, int inst_no, char cache_flag)
{
	s3c_mfc_free_mem_t	*node, *match_node = NULL;
	unsigned int	allocAddr = 0;


	mfc_debug("request Size : %ld\n", allocSize);

	if (s3c_mfc_free_mem_head == s3c_mfc_free_mem_tail) {
		mfc_err("all memory is gone\n");
		return(allocAddr);
	}

	/* find best chunk of memory */
	for (node = s3c_mfc_free_mem_head; node != s3c_mfc_free_mem_tail; node = node->next) {
		if (match_node != NULL) {
			if (cache_flag) {
				if ((node->size >= allocSize) && (node->size < match_node->size) && (node->cache_flag))
					match_node = node;
			} else {
				if ((node->size >= allocSize) && (node->size < match_node->size) && (!node->cache_flag))
					match_node = node;
			}
		} else {
			if (cache_flag) {
				if ((node->size >= allocSize) && (node->cache_flag))
					match_node = node;
			} else {
				if ((node->size >= allocSize) && (!node->cache_flag))
					match_node = node;
			}
		}
	}

	if (match_node != NULL) {
		mfc_debug("match : startAddr(0x%08x) size(%ld) cache flag(%d)\n", 
			match_node->start_addr, match_node->size, match_node->cache_flag);
	}

	/* rearange FreeMemArea */
	if (match_node != NULL) {
		allocAddr = match_node->start_addr;
		match_node->start_addr += allocSize;
		match_node->size -= allocSize;

		if(match_node->size == 0)          /* delete match_node. */
			s3c_mfc_del_node_from_free_list(match_node, inst_no);

		return(allocAddr);
	} else {
		printk("there is no suitable chunk\n");
		return 0;
	}

	return(allocAddr);
}


int s3c_mfc_init_buffer_manager(void)
{
	s3c_mfc_free_mem_t	*free_node;
	s3c_mfc_alloc_mem_t	*alloc_node;

	/* init alloc list, if(s3c_mfc_alloc_mem_head == s3c_mfc_alloc_mem_tail) then, the list is NULL */
	alloc_node = (s3c_mfc_alloc_mem_t *)kmalloc(sizeof(s3c_mfc_alloc_mem_t), GFP_KERNEL);
	memset(alloc_node, 0x00, sizeof(s3c_mfc_alloc_mem_t));
	alloc_node->next = alloc_node;
	alloc_node->prev = alloc_node;
	s3c_mfc_alloc_mem_head = alloc_node;
	s3c_mfc_alloc_mem_tail = s3c_mfc_alloc_mem_head;

	/* init free list, if(s3c_mfc_free_mem_head == s3c_mfc_free_mem_tail) then, the list is NULL */
	free_node = (s3c_mfc_free_mem_t *)kmalloc(sizeof(s3c_mfc_free_mem_t), GFP_KERNEL);
	memset(free_node, 0x00, sizeof(s3c_mfc_free_mem_t));
	free_node->next = free_node;
	free_node->prev = free_node;
	s3c_mfc_free_mem_head = free_node;
	s3c_mfc_free_mem_tail = s3c_mfc_free_mem_head;

	/* init free head node */
	free_node = (s3c_mfc_free_mem_t *)kmalloc(sizeof(s3c_mfc_free_mem_t), GFP_KERNEL);
	memset(free_node, 0x00, sizeof(s3c_mfc_free_mem_t));
	free_node->start_addr = s3c_mfc_phys_data_buf;
	free_node->cache_flag = 0;
	free_node->size = s3c_get_media_memsize(S3C_MDEV_MFC);
	s3c_mfc_insert_first_node_to_free_list(free_node, -1);

	return 0;
}


/* Releae cacheable memory */
MFC_ERROR_CODE s3c_mfc_release_alloc_mem(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{		
	int ret;

	s3c_mfc_free_mem_t *free_node;
	s3c_mfc_alloc_mem_t *node;

	for(node = s3c_mfc_alloc_mem_head; node != s3c_mfc_alloc_mem_tail; node = node->next) {
		if(node->u_addr == (unsigned char *)args->mem_free.u_addr)
			break;
	}

	if (node == s3c_mfc_alloc_mem_tail) {
		mfc_err("invalid virtual address(0x%x)\r\n", args->mem_free.u_addr);
		ret = MFCINST_MEMORY_INVAILD_ADDR;
		goto out_releaseallocmem;
	}

	free_node = (s3c_mfc_free_mem_t	*)kmalloc(sizeof(s3c_mfc_free_mem_t), GFP_KERNEL);

	if(node->cache_flag) {
		free_node->start_addr = node->cached_p_addr;
		free_node->cache_flag = 1;
	} else {
		free_node->start_addr = node->uncached_p_addr;
		free_node->cache_flag = 0;
	}

	free_node->size = node->size;
	s3c_mfc_insert_node_to_free_list(free_node, MfcCtx->InstNo);

	/* Delete from AllocMem list */
	s3c_mfc_del_node_from_alloc_list(node, MfcCtx->InstNo);

	ret = MFCINST_RET_OK;

out_releaseallocmem:
	return ret;
}

MFC_ERROR_CODE s3c_mfc_get_phys_addr(s3c_mfc_inst_ctx *MfcCtx, s3c_mfc_args *args)
{
	int ret;
	s3c_mfc_alloc_mem_t *node;
	s3c_mfc_get_phys_addr_arg_t *codec_get_phy_addr_arg = (s3c_mfc_get_phys_addr_arg_t *)args;

	for(node = s3c_mfc_alloc_mem_head; node != s3c_mfc_alloc_mem_tail; node = node->next) {
		if(node->u_addr == (unsigned char *)codec_get_phy_addr_arg->u_addr)
			break;
	}

	if(node  == s3c_mfc_alloc_mem_tail){
		mfc_err("invalid virtual address(0x%x)\r\n", codec_get_phy_addr_arg->u_addr);
		ret = MFCINST_MEMORY_INVAILD_ADDR;
		goto out_getphysaddr;
	}

	if(node->cache_flag == MFC_MEM_CACHED)
		codec_get_phy_addr_arg->p_addr = node->cached_p_addr;
	else
		codec_get_phy_addr_arg->p_addr = node->uncached_p_addr;

	ret = MFCINST_RET_OK;

out_getphysaddr:
	return ret;

}

MFC_ERROR_CODE s3c_mfc_get_virt_addr(s3c_mfc_inst_ctx  *MfcCtx,  s3c_mfc_args *args)
{
	int ret;
	int inst_no = MfcCtx->InstNo;
	unsigned int p_startAddr;
	s3c_mfc_mem_alloc_arg_t *in_param;	
	s3c_mfc_alloc_mem_t *p_allocMem;
	

	in_param = (s3c_mfc_mem_alloc_arg_t *)args;

	/* if user request cachable area, allocate from reserved area */
	/* if user request uncachable area, allocate dynamically */
	p_startAddr = s3c_mfc_get_mem_area((int)in_param->buff_size, inst_no, in_param->cache_flag);
	mfc_debug("p_startAddr = 0x%X\n\r", p_startAddr);

	if (!p_startAddr) {
		mfc_debug("There is no more memory\n\r");
		in_param->out_addr = -1;
		ret = MFCINST_MEMORY_ALLOC_FAIL;
		goto out_getcodecviraddr;
	}

	p_allocMem = (s3c_mfc_alloc_mem_t *)kmalloc(sizeof(s3c_mfc_alloc_mem_t), GFP_KERNEL);
	memset(p_allocMem, 0x00, sizeof(s3c_mfc_alloc_mem_t));

	if (in_param->cache_flag == MFC_MEM_CACHED) {
		p_allocMem->cached_p_addr = p_startAddr;
		p_allocMem->v_addr = s3c_mfc_virt_data_buf + (p_allocMem->cached_p_addr - s3c_mfc_phys_data_buf);
		p_allocMem->u_addr = (unsigned char *)(in_param->cached_mapped_addr + 
				(p_allocMem->cached_p_addr - s3c_mfc_phys_data_buf));

		if (p_allocMem->v_addr == NULL) {
			mfc_debug("Mapping Failed [PA:0x%08x]\n\r", p_allocMem->cached_p_addr);
			ret = MFCINST_MEMORY_MAPPING_FAIL;
			goto out_getcodecviraddr;
		}
	} else {
		p_allocMem->uncached_p_addr = p_startAddr;
		p_allocMem->v_addr = s3c_mfc_virt_data_buf + (p_allocMem->uncached_p_addr - s3c_mfc_phys_data_buf);
		p_allocMem->u_addr = (unsigned char *)(in_param->non_cached_mapped_addr + 
				(p_allocMem->uncached_p_addr - s3c_mfc_phys_data_buf));
		mfc_debug("in_param->non_cached_mapped_addr = 0x%X, s3c_mfc_phys_data_buf = 0x%X, data buffer size = 0x%X\n", 
				in_param->non_cached_mapped_addr, s3c_mfc_phys_data_buf, s3c_get_media_memsize(S3C_MDEV_MFC));
		if (p_allocMem->v_addr == NULL) {
			mfc_debug("Mapping Failed [PA:0x%08x]\n\r", p_allocMem->uncached_p_addr);
			ret = MFCINST_MEMORY_MAPPING_FAIL;
			goto out_getcodecviraddr;
		}
	}

	in_param->out_addr = (unsigned int)p_allocMem->u_addr;
	mfc_debug("u_addr : 0x%x v_addr : 0x%x cached_p_addr : 0x%x, uncached_p_addr : 0x%x\n",
		p_allocMem->u_addr, p_allocMem->v_addr, p_allocMem->cached_p_addr, p_allocMem->uncached_p_addr);

	p_allocMem->size = (int)in_param->buff_size;
	p_allocMem->inst_no = inst_no;
	p_allocMem->cache_flag = in_param->cache_flag;

	s3c_mfc_insert_node_to_alloc_list(p_allocMem, inst_no);
	ret = MFCINST_RET_OK;

out_getcodecviraddr:	
	return ret;
}

