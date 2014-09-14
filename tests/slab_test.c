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

#include "common/types.h"
#include "common/fsl_stdio.h"
#include "common/fsl_string.h"
#include "fsl_fdma.h"
#include "general.h"
#include "fsl_ip.h"
#include "fsl_cdma.h"
#include "slab.h"
#include "platform.h"
#include "ls2085_aiop/fsl_platform.h"
#include "fsl_io.h"

__SHRAM struct slab *slab_peb = 0;
__SHRAM struct slab *slab_ddr = 0;

extern __SHRAM struct slab_virtual_pools_main_desc g_slab_virtual_pools;
int app_test_slab_init();
int app_test_slab_overload_test();
int slab_init();
int slab_test();
int app_test_slab(struct slab *slab, int num_times);


static int slab_callback_test(uint64_t context_address){

	fsl_os_print("slab_release: callback function\n");
	fsl_os_print("release context address - 0x%x%x\n", (uint32_t)( context_address >> 32),(uint32_t)context_address);
	return 0;
}

int slab_init()
{
	int err = 0;
	struct slab_debug_info slab_info;
	/* Test for slab initialization, ignore it unless it fails */
	err = app_test_slab_init();
	if (err) {
		fsl_os_print("ERROR = %d: app_test_slab_init()\n", err);
		return err;
	}

	err = app_test_slab_overload_test();
	if (err) {
		fsl_os_print("ERROR = %d: app_test_slab_overload_test()\n", err);
		return err;
	}

	/* DDR SLAB creation */
	err = slab_create(10, 0, 256, 0, 0, 4, MEM_PART_DP_DDR, 0,
			          NULL, &slab_ddr);
	if (err) return err;

	err = slab_debug_info_get(slab_ddr, &slab_info);
	if (err) {
		return err;
	} else {
		if ((slab_info.committed_buffs != slab_info.max_buffs) ||
		    (slab_info.committed_buffs == 0))
			return -ENODEV;
	}

	/* PEB SLAB creation */
	err = slab_create(5, 0, 100, 0, 0, 4, MEM_PART_PEB, 0, NULL, &slab_peb);
	if (err) return err;

	err = slab_debug_info_get(slab_peb, &slab_info);
	if(!err)
		if ((slab_info.committed_buffs != slab_info.max_buffs) ||
			(slab_info.committed_buffs == 0))
				return -ENODEV;
	return err;
}

int app_test_slab_overload_test()
{
	int        err = 0;
	int 	i;
	dma_addr_t buff = 0;
	struct slab *my_slab[2000];

	for (i = 0; i < 2000 ; i++)
	{
		err = slab_create(1, 0, 256, 0, 0, 4, MEM_PART_DP_DDR, 0,
				  &slab_callback_test, &(my_slab[i]));
		if (err) return err;

		else
			fsl_os_print("Slab cluster and pool id are: cluster %d, pool ID %d\n",
			             SLAB_CLUSTER_ID_GET(SLAB_VP_POOL_GET(my_slab[i])),
			             SLAB_POOL_ID_GET(SLAB_VP_POOL_GET(my_slab[i])));
	}


	for(i = 0; i < 100; i++ ){
		if(g_slab_virtual_pools.clusters_count[i] > 0)
			fsl_os_print("Number of pools in cluster %d: %d\n", i, g_slab_virtual_pools.clusters_count[i]);
	}
	for (i = 1999; i >= 0 ; i--)
	{
		err = slab_acquire(my_slab[i], &buff);
		if (err) return err;

		if (slab_refcount_decr(buff) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
			err = slab_release(my_slab[i], buff);
			if (err) return err;
		}
		else
			return -ENODEV;

		err = slab_free(&(my_slab[i]));
		if (err) return err;

		/* Must fail because my_slab was freed  */
		err = slab_acquire(my_slab[i], &buff);
		if (!err) return -EEXIST;
	}

	for(i = 0; i < 100; i++ ){
			if(g_slab_virtual_pools.clusters_count[i] > 0)
				fsl_os_print("Number of pools in cluster %d: %d\n", i, g_slab_virtual_pools.clusters_count[i]);
	}
	return 0;
}


int app_test_slab_init()
{
	int        err = 0;
	dma_addr_t buff = 0;
	struct slab *my_slab;

	err = slab_create(5, 0, 256, 0, 0, 4, MEM_PART_DP_DDR, 0,
	                  &slab_callback_test, &my_slab);
	if (err) return err;

	err = slab_acquire(my_slab, &buff);
	if (err) return err;

	if (slab_refcount_decr(buff) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
		err = slab_release(my_slab, buff);
		if (err) return err;
	}
	else
		return -ENODEV;

	err = slab_free(&my_slab);
	if (err) return err;

	/* Must fail because my_slab was freed  */
	err = slab_acquire(my_slab, &buff);
	if (!err) return -EEXIST;


	/* Reuse slab handle test  */
	err = slab_create(1, 0, 256, 0, 0, 4, MEM_PART_DP_DDR, 0,
	                  NULL, &my_slab);
	if (err) return err;

	err = slab_free(&my_slab);
	if (err) return err;

	return 0;
}

static int app_write_buff_and_release(struct slab *slab, uint64_t buff)
{
	uint64_t data1 = 0xAABBCCDD00001122;
	uint64_t data2 = 0;
	int      err = 0;

	cdma_write(buff, &data1, 8);
	cdma_read(&data2, buff, 8);


	if (data1 != data2) {
		if (slab_refcount_decr(buff) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
			err = slab_release(slab, buff);
			if (err) return err;
		}
		return -EPERM;
	}

	if (slab_refcount_decr(buff) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
		err = slab_release(slab, buff);
		if (err) return err;
	}

	return 0;
}

int app_test_slab(struct slab *slab, int num_times)
{
	uint64_t buff[4] = {0, 0, 0, 0};
	int      err = 0, start = 1, end = 1;
	int      i = 0;
	struct slab *my_slab;
#pragma fn_ptr_candidates(slab_create)
	err = slab_create(5, 0, 256, 0, 0, 4, MEM_PART_PEB, 0,
	                  NULL, &my_slab);

	for (i = 0; i < num_times; i++) {

		err = slab_acquire(slab, &buff[0]);
		if (err || (buff[0] == NULL)) return -ENOMEM;
		err = slab_acquire(slab, &buff[1]);
		if (err || (buff[1] == NULL)) return -ENOMEM;
		err = slab_acquire(slab, &buff[2]);
		if (err || (buff[2] == NULL)) return -ENOMEM;

		slab_refcount_incr(buff[2]);
		if (buff[2] == NULL) return -ENOMEM;
		slab_refcount_incr(buff[2]);
		if (buff[2] == NULL) return -ENOMEM;
		slab_refcount_incr(buff[2]);
		if (buff[2] == NULL) return -ENOMEM;
		slab_refcount_incr(buff[2]);
		if (buff[2] == NULL) return -ENOMEM;



		err = slab_acquire(my_slab, &buff[3]);
		if (err || (buff[3] == NULL)) return -ENOMEM;

		err = app_write_buff_and_release(slab, buff[1]);
		if (err) return err;
		err = app_write_buff_and_release(slab, buff[2]);
		if (err) return err;
		err = app_write_buff_and_release(slab, buff[0]);
		if (err) return err;
		err = app_write_buff_and_release(my_slab, buff[3]);
		if (err) return err;

		if (slab_refcount_decr(buff[2]) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
			err = slab_release(slab, buff[2]);
			if (err) return err;
		}

		if (slab_refcount_decr(buff[2]) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
			err = slab_release(slab, buff[2]);
			if (err) return err;
		}
		if (slab_refcount_decr(buff[2]) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
			err = slab_release(slab, buff[2]);
			if (err) return err;
		}
		if (slab_refcount_decr(buff[2]) == SLAB_CDMA_REFCOUNT_DECREMENT_TO_ZERO){
			err = slab_release(slab, buff[2]);
			if (err) return err;
		}
	}

	err = slab_free(&my_slab);
	if (err) return err;

	err = slab_acquire(my_slab, &buff[3]);
	if(err == 0) return -EINVAL;
	err = slab_acquire(my_slab, &buff[2]);
	if(err == 0) return -EINVAL;

	return 0;
}

int slab_test()
{
	int err = 0;
	err |= app_test_slab(slab_peb, 4);
	if (err) {
		fsl_os_print("ERROR = %d: app_test_slab(slab_peb, 4)\n", err);
	}

	err |= app_test_slab(slab_ddr, 4);
	if (err) {
		fsl_os_print("ERROR = %d: app_test_slab(slab_ddr, 4)\n", err);
	}
	return err;
}
