/*
 * Copyright (c) 2015-2016 François Tigeot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_DMA_MAPPING_H_
#define _LINUX_DMA_MAPPING_H_

#include <linux/string.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/dma-direction.h>
#include <linux/scatterlist.h>

static inline dma_addr_t
dma_map_page(struct device *dev, struct vm_page *page,
    unsigned long offset, size_t size, enum dma_data_direction direction)
{
	return VM_PAGE_TO_PHYS(page) + offset;
}

static inline void dma_unmap_page(struct device *dev, dma_addr_t addr,
				  size_t size, enum dma_data_direction dir)
{
}

static inline int
dma_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return 0;
}

#endif	/* _LINUX_DMA-MAPPING_H_ */
