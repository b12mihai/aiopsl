/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Freescale Semiconductor nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************//*
@File          slab.h

@Description   This is slab internal header file which includes all the
		architecture specific implementation defines.

@Cautions      This file is private for AIOP.
*//***************************************************************************/
#ifndef __SLAB_H
#define __SLAB_H

#include "fsl_slab.h"
#include "platform.h"
#include "ls2085_aiop/fsl_platform.h"


#define SLAB_HW_HANDLE(SLAB) ((uint32_t)(SLAB)) /**< Casted HW handle */

/**************************************************************************//**
@Description   SLAB common internal macros
*//***************************************************************************/
#define SLAB_HW_POOL_SET      0x00000001
/**< Flag which indicates that this SLAB handle is HW pool */
#define SLAB_IS_HW_POOL(SLAB) (SLAB_HW_HANDLE(SLAB) & SLAB_HW_POOL_SET)
/**< Slab handle is HW pool */

/**************************************************************************//**
@Description   SLAB AIOP HW pool internal macros
*//***************************************************************************/
/*
 *  HW SLAB structure
 *
 * 31----------23--------------1--------0
 * | HW accel   |VP ID         |HW flg  |
 * -------------------------------------
 */
#define SLAB_VP_POOL_MASK      0x00FFFFFE
#define SLAB_VP_POOL_MAX       (SLAB_VP_POOL_MASK >> 1)
/**< Maximal number to be used as VP id */
#define SLAB_VP_POOL_SHIFT     1
#define SLAB_HW_ACCEL_MASK     0xFF000000
#define SLAB_VP_POOL_GET(SLAB) \
	((uint32_t)((SLAB_HW_HANDLE(SLAB) & SLAB_VP_POOL_MASK) >> 1))
/**< Returns slab's virtual pool id*/

#define SLAB_HW_META_OFFSET     8 /**< metadata offset in bytes */
#define SLAB_SIZE_GET(SIZE)     ((SIZE) - SLAB_HW_META_OFFSET)
/**< Real buffer size used by user */
#define SLAB_SIZE_SET(SIZE)     ((SIZE) + SLAB_HW_META_OFFSET)
/**< Buffer size that needs to be set for CDMA, including metadata */

#define SLAB_HW_POOL_CREATE(VP) \
((((VP) & (SLAB_VP_POOL_MASK >> SLAB_VP_POOL_SHIFT)) << SLAB_VP_POOL_SHIFT) \
	| SLAB_HW_POOL_SET)
/**< set slab's virtual pool id shifted to have space for slab hardware pool bit 0*/
/**************************************************************************//**
@Description   SLAB module defaults macros
*//***************************************************************************/
/** bpid, user required size, partition */

#define SLAB_BPIDS_ARR	\
	{ \
	{1,	4096,    MEM_PART_DP_DDR}, \
	{2,	2048,    MEM_PART_DP_DDR}, \
	{3,	1024,   MEM_PART_DP_DDR},  \
	{4,	512,   MEM_PART_DP_DDR},   \
	{5,	256,   MEM_PART_DP_DDR},   \
	{6,	4096,    MEM_PART_PEB},    \
	{7,	2048,    MEM_PART_PEB},    \
	{8,	1024,   MEM_PART_PEB},     \
	{9,	512,   MEM_PART_PEB},      \
	{10,	256,   MEM_PART_PEB}       \
	}


#define SLAB_FAST_MEMORY        MEM_PART_SH_RAM
#define SLAB_DDR_MEMORY         MEM_PART_DP_DDR
#define SLAB_DEFAULT_ALIGN      8
#define SLAB_MAX_NUM_VP         1000
#define SLAB_NUM_OF_BUFS_DPDDR  750
#define SLAB_NUM_OF_BUFS_PEB    20

/* Maximum number of BMAN pools used by the slab virtual pools */
#ifndef SLAB_MAX_BMAN_POOLS_NUM
	#define SLAB_MAX_BMAN_POOLS_NUM 64
#endif

/**************************************************************************//**
@Description   Information for every bpid
*//***************************************************************************/
struct slab_bpid_info {
	uint16_t bpid;
	/**< Bpid - slabs bman id */
	uint16_t size;
	/**< Size of memory the bman pool is taking  */
	e_memory_partition_id mem_pid;
	/**< Memory Partition Identifier */
};

/**************************************************************************//**
@Description   Information to be kept about every HW pool inside DDR
*//***************************************************************************/
struct slab_hw_pool_info {
	uint32_t flags;
	/**< Control flags */
	uint16_t buff_size;
	/**< Maximal buffer size including 8 bytes of CDMA metadata */
	uint16_t pool_id;
	/**< BMAN pool ID */
	uint16_t alignment;
	/**< Buffer alignment */
	uint16_t mem_pid;
	/**< Memory partition for buffers allocation */
	int32_t total_num_buffs;
	/**< Number of allocated buffers per pool */
};

/**************************************************************************//**
@Description   Information to be kept about SLAB module
*//***************************************************************************/
struct slab_module_info {
	uint32_t fdma_flags;
	/**< Flags to be used for FDMA release/acquire */
	uint32_t fdma_dma_flags;
	/**< Flags to be used for FDMA dma data,
	 * not including fdma_dma_data_access_options */
	struct  slab_hw_pool_info *hw_pools;
	/**< List of BMAN pools */
	uint16_t icid;
	/**< CDMA ICID to be used for FDMA release/acquire*/
	/* TODO uint8_t spinlock; */
	/**< Spinlock placed at SHRAM */
	uint8_t num_hw_pools;
	/**< Number of BMAN pools */
};

/* Virtual Pool structure */
struct slab_v_pool {
	int32_t max_bufs;
	/**< Number of MAX requested buffers per pool */
	int32_t committed_bufs;
	/**< Number of requested committed buffers per pool */
	int32_t allocated_bufs;
	/**< Number of allocated buffers per pool */
	uint8_t spinlock;
	/**< spinlock for locking the pool */
	uint8_t flags;
	/**< Flags to use when using the pool - unused  */
	uint16_t bman_array_index;
	/**< Index of bman pool that the buffers were taken from*/
};

/* BMAN Pool structure */
struct slab_bman_pool_desc {
	int32_t remaining;
	/**< Number of remaining buffers in the bman pool */
	uint8_t spinlock;
	/**< Spinlock for locking bman pool */
	uint8_t flags;
	/**< Flags to use when using the pool - unused  */
	uint16_t bman_pool_id;
	/**< Bman pool id - bpid  */
};

/* virtual root pool struct - holds all virtual pools data */
struct slab_virtual_pools_main_desc {
	struct slab_v_pool *virtual_pool_struct;
	/**< Pointer to virtual pools array*/
	slab_release_cb_t **callback_func;
	/**< Callback function to release virtual pool  */
	uint16_t num_of_virtual_pools;
	/**< Number of virtual pools pointed by this pool  */
	uint8_t flags;
	/**< Flags to use when using the pools - unused  */
	uint8_t global_spinlock;
	/**< Spinlock for locking the global virtual root pool */
};

/**************************************************************************//**
@Function      slab_module_init

@Description   Initialize SLAB module

		In AIOP during slab_module_init() we will call MC API in order
		to get all BPIDs

@Return        0 on success, error code otherwise.
 *//***************************************************************************/
int slab_module_init(void);

/**************************************************************************//**
@Function      slab_module_free

@Description   Frees SLAB module

		In addition to memory de-allocation it will return BPIDs to MC

@Return        None
 *//***************************************************************************/
void slab_module_free(void);

/**************************************************************************//**
@Function      slab_find_and_reserve_bpid

@Description   Finds and reserve buffers from buffer pool.

		This function is part of SLAB module therefore it should be
		called only after it has been initialized by slab_module_init()

@Param[in]     num_buffs         Number of buffers in new pool.
@Param[in]     buff_size         Size of buffers in pool.
@Param[in]     alignment         Requested alignment for data field (in bytes).
				 AIOP: HW pool supports up to 8 bytes alignment.
@Param[in]     mem_partition_id  Memory partition ID for buffer type.
				 AIOP: HW pool supports only PEB and DPAA DDR.
@Param[out]    num_reserved_buffs  Number of buffers that we succeeded to reserve.
@Param[out]    bpid              Id of pool that supply the requested buffers.

@Return        0       - on success,
	       -ENAVAIL - could not release into bpid
	       -ENOMEM  - not enough memory for mem_partition_id
 *//***************************************************************************/
int slab_find_and_reserve_bpid(uint32_t num_buffs,
			uint16_t buff_size,
			uint16_t alignment,
			uint8_t  mem_partition_id,
			int *num_reserved_buffs,
			uint16_t *bpid);

/**************************************************************************//**
@Function      slab_find_and_free_bpid

@Description   Finds and free buffer pool with new buffers

		This function is part of SLAB module therefore it should be
		called only after it has been initialized by slab_module_init()
		the function is for service layer to return buffers to bman pool.

@Param[in]    num_buffs        Number of buffers in new pool.
@Param[in]    bpid              Id of pool that was filled with new buffers.

@Return        0       - on success,
	       -ENAVAIL - bman pool not found
 *//***************************************************************************/
int slab_find_and_free_bpid(uint32_t num_buffs,
                            uint16_t *bpid);

#endif /* __SLAB_H */
