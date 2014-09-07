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
#include "inc/fsl_gen.h"
#include "fsl_errors.h"
#include "common/fsl_string.h"
#include "fsl_malloc.h"
#include "general.h"
#include "sys.h"
#include "fsl_dbg.h"
#include "dplib/fsl_dprc.h"
#include "dplib/fsl_dpci.h"
#include "fsl_mc_init.h"
#include "ls2085_aiop/fsl_platform.h"

int mc_obj_init();
void mc_obj_free();

#define DPCI_LOW_PR  1
#define MC_DPCI_NUM 1
#define MC_DPCI_ID  0

static int aiop_container_init()
{
	void *p_vaddr;
	int err = 0;
	int container_id;
	struct mc_dprc *dprc = fsl_os_xmalloc(sizeof(struct mc_dprc),
					   MEM_PART_SH_RAM,
					   1);
	if (dprc == NULL) {
		pr_err("No memory for AIOP Root Container \n");
		return -ENOMEM;
	}
	memset(dprc, 0, sizeof(struct mc_dprc));

	/* TODO: replace hard-coded portal address 1 with configured value */
	/* TODO : layout file must contain portal ID 1 in order to work. */
	/* TODO : in this call, can 3rd argument be zero? */
	/* Get virtual address of MC portal */
	p_vaddr = \
	UINT_TO_PTR(sys_get_memory_mapped_module_base(FSL_OS_MOD_MC_PORTAL,
					 (uint32_t)1, E_MAPPED_MEM_TYPE_MC_PORTAL));

	/* Open root container in order to create and query for devices */
	dprc->io.regs = p_vaddr;
	if ((err = dprc_get_container_id(&dprc->io, &container_id)) != 0) {
		pr_err("Failed to get AIOP root container ID.\n");
		return err;
	}
	if ((err = dprc_open(&dprc->io, container_id, &dprc->token)) != 0) {
		pr_err("Failed to open AIOP root container DP-RC%d.\n",
		container_id);
		return err;
	}

	err = sys_add_handle(dprc, FSL_OS_MOD_AIOP_RC, 1, 0);
	return err;
}

static void aiop_container_free()
{
	void *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);

	sys_remove_handle(FSL_OS_MOD_AIOP_RC, 0);

	if (dprc != NULL)
		fsl_os_xfree(dprc);
}

static int dpci_tbl_create(struct mc_dpci_obj **_dpci_tbl, int dpci_count)
{
	uint32_t size = 0;
	struct   mc_dpci_obj *dpci_tbl = NULL;
	int      err = 0;

	size = sizeof(struct mc_dpci_obj);
	dpci_tbl = fsl_os_xmalloc(size, MEM_PART_SH_RAM, 1);
	*_dpci_tbl = dpci_tbl;
	if (dpci_tbl == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl, 0, size);

	size = sizeof(struct dpci_attr) * dpci_count;
	dpci_tbl->attr = fsl_os_xmalloc(size, MEM_PART_SH_RAM, 1);
	if (dpci_tbl->attr == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->attr, 0, size);

	size = sizeof(uint16_t) * dpci_count;
	dpci_tbl->token = fsl_os_xmalloc(size, MEM_PART_SH_RAM, 1);
	if (dpci_tbl->token == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->token, 0, size);

	size = sizeof(uint16_t) * dpci_count;
	dpci_tbl->icid = fsl_os_xmalloc(size, MEM_PART_SH_RAM, 1);
	if (dpci_tbl->icid == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->icid, 0, size);

	size = sizeof(uint32_t) * dpci_count;
	dpci_tbl->dma_flags = fsl_os_xmalloc(size, MEM_PART_SH_RAM, 1);
	if (dpci_tbl->dma_flags == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->dma_flags, 0, size);

	size = sizeof(uint32_t) * dpci_count;
	dpci_tbl->enq_flags = fsl_os_xmalloc(size, MEM_PART_SH_RAM, 1);
	if (dpci_tbl->enq_flags == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->enq_flags, 0, size);

	err = sys_add_handle(dpci_tbl,
			     FSL_OS_MOD_DPCI_TBL,
			     1,
			     0);
	if (err != 0) {
		pr_err("FSL_OS_MOD_DPCI_TBL sys_add_handle failed\n");
		return err;
	}

	return err;
}

static int dpci_tbl_add(struct dprc_obj_desc *dev_desc, int ind,
			struct mc_dpci_obj *dpci_tbl, struct mc_dprc *dprc)
{
	uint16_t dpci = 0;
	struct   dpci_dest_cfg dest_cfg;
	int      err = 0;
	uint8_t  p   = 0;

	if (dev_desc == NULL)
		return -EINVAL;

	pr_debug(" Found DPCI device\n");
	pr_debug("***********\n");
	pr_debug("vendor - %x\n", dev_desc->vendor);
	pr_debug("type - %s\n", dev_desc->type);
	pr_debug("id - %d\n", dev_desc->id);
	pr_debug("region_count - %d\n", dev_desc->region_count);
	pr_debug("state - %d\n", dev_desc->state);
	pr_debug("ver_major - %d\n", dev_desc->ver_major);
	pr_debug("ver_minor - %d\n", dev_desc->ver_minor);
	pr_debug("irq_count - %d\n\n", dev_desc->irq_count);

	memset(&dest_cfg, 0, sizeof(struct dpci_dest_cfg));

	err |= dpci_open(&dprc->io, dev_desc->id, &dpci);
	/* Set priorities 0 and 1
	 * 0 is high priority
	 * 1 is low priority
	 * Making sure that low priority is at index 0*/
	dest_cfg.type = DPCI_DEST_NONE;
	for (p = 0; p <= DPCI_LOW_PR; p++) {
		dest_cfg.priority = DPCI_LOW_PR - p;
		err |= dpci_set_rx_queue(&dprc->io,
		                         dpci,
					 p,
					 &dest_cfg,
					 (ind << 1) | p);
	}
	err |= dpci_enable(&dprc->io, dpci);
	err |= dpci_get_attributes(&dprc->io,
	                           dpci,
				   &dpci_tbl->attr[ind]);

	dpci_tbl->token[ind] = dpci;
	return 0;
}

static int dpci_for_mc_add(struct mc_dpci_obj *dpci_tbl, struct mc_dprc *dprc, int ind)
{
	struct dpci_cfg dpci_cfg;
	uint16_t dpci;
	struct dpci_dest_cfg dest_cfg;
	struct dprc_endpoint endpoint1 ;
	struct dprc_endpoint endpoint2;
	uint8_t p = 0;
	int     err = 0;
	int     link_up = 0;

	dpci_cfg.num_of_priorities = 2;

	err |= dpci_create(&dprc->io, &dpci_cfg, &dpci);
	/* Set priorities 0 and 1
	 * 0 is high priority
	 * 1 is low priority
	 * Making sure that low priority is at index 0*/
	dest_cfg.type = DPCI_DEST_NONE;
	for (p = 0; p <= DPCI_LOW_PR; p++) {
		dest_cfg.priority = DPCI_LOW_PR - p;
		err |= dpci_set_rx_queue(&dprc->io,
		                         dpci,
					 p,
					 &dest_cfg,
					 (ind << 1) | p);
	}

	/* Get attributes just for dpci id,
	 * fqids are not there yet */
	err |= dpci_get_attributes(&dprc->io,
	                           dpci,
				   &dpci_tbl->attr[ind]);

	/* Connect to dpci 0 that belongs to MC */
	memset(&endpoint1, 0, sizeof(struct dprc_endpoint));
	memset(&endpoint2, 0, sizeof(struct dprc_endpoint));
	endpoint1.id = MC_DPCI_ID;
	endpoint1.interface_id = 0;
	strcpy(endpoint1.type, "dpci");

	endpoint2.id = dpci_tbl->attr[ind].id;
	endpoint2.interface_id = 0;
	strcpy(endpoint2.type, "dpci");

	err |= dpci_enable(&dprc->io, dpci);
	err |= dprc_connect(&dprc->io, dprc->token, &endpoint1, &endpoint2);
	err |= dpci_get_attributes(&dprc->io, dpci, &dpci_tbl->attr[ind]);
	err |= dpci_get_link_state(&dprc->io, dpci, &link_up);
	if (!link_up) {
		pr_err("MC<->AIOP DPCI link is down !\n");
	}

	dpci_tbl->token[ind] = dpci;
	return err;
}

static int dpci_tbl_fill(struct mc_dpci_obj *dpci_tbl, struct mc_dprc *dprc,
			 int dpci_count, int dev_count)
{
	int ind = 0;
	int i   = 0;
	int err = 0;
	struct dprc_obj_desc dev_desc;

	if (dpci_tbl == NULL) {
		pr_err("No DPCI objects in AIOP root container \n");
		return -EINVAL;
	}

	while ((i < dev_count) && (ind < dpci_count)) {
		dprc_get_obj(&dprc->io, dprc->token, i, &dev_desc);
		if (strcmp(dev_desc.type, "dpci") == 0) {
			err = dpci_tbl_add(&dev_desc, ind, dpci_tbl, dprc);
			if (err) {
				pr_err("Failed dpci_tbl_add \n");
				dpci_tbl->count = ind;
				return -ENODEV;
			}
			ind++;
		}
		i++;
	}

	err = dpci_for_mc_add(dpci_tbl, dprc, ind);
	if (err) {
		pr_err("Failed to create and link AIOP<->MC DPCI \n");
		dpci_tbl->count = ind;
	} else {
		dpci_tbl->count = ind + MC_DPCI_NUM;
	}
	return err;
}

static int dpci_discovery()
{
	struct dprc_obj_desc dev_desc;
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	struct mc_dpci_obj *dpci_tbl = NULL;
	int dev_count  = 0;
	int dpci_count = 0;
	int err        = 0;
	int i          = 0;

	if (dprc == NULL) {
		pr_err("No AIOP root container \n");
		return -ENODEV;
	}

	if ((err = dprc_get_obj_count(&dprc->io, dprc->token, &dev_count)) != 0) {
	pr_err("Failed to get device count for RC auth_d = %d\n",
		   dprc->token);
	return err;
	}

	/* First count how many DPCI objects we have */
	for (i = 0; i < dev_count; i++) {
		dprc_get_obj(&dprc->io, dprc->token, i, &dev_desc);
		if (strcmp(dev_desc.type, "dpci") == 0) {
			dpci_count++;
		}
	}

	if (dpci_count > 0) {
		err = dpci_tbl_create(&dpci_tbl, dpci_count + MC_DPCI_NUM);
		if (err != 0) {
			pr_err("Failed dpci_tbl_create() \n");
			return err;
		}
	}

	err = dpci_tbl_fill(dpci_tbl, dprc, dpci_count, dev_count);
	return err;
}

static void dpci_discovery_free()
{
	void *dpci_tbl = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	sys_remove_handle(FSL_OS_MOD_DPCI_TBL, 0);

	if (dpci_tbl != NULL)
		fsl_os_xfree(dpci_tbl);
}

int mc_obj_init()
{
	int err = 0;

#ifndef AIOP_STANDALONE
	err |= aiop_container_init();
	err |= dpci_discovery(); /* must be after aiop_container_init */
#endif
	return err;

}

void mc_obj_free()
{
#ifndef AIOP_STANDALONE
	aiop_container_free();
	dpci_discovery_free();
	/* TODO DPCI close ???
	 * TODO DPRC close */
#endif
}
