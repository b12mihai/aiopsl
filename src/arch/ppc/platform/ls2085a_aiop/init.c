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

#include "common/fsl_string.h"
#include "fsl_io_ccsr.h"
#include "dplib/fsl_dprc.h"
#include "dplib/fsl_dpni.h"
#include "fsl_malloc.h"
#include "kernel/fsl_spinlock.h"
#include "../drivers/dplib/arch/accel/fdma.h"  /* TODO: need to place fdma_release_buffer() in separate .h file */
#include "dplib/fsl_dpbp.h"
#include "sys.h"
#include "fsl_io_ccsr.h"
#include "slab.h"
#include "cmgw.h"
#include "fsl_mc_init.h"
#include "../drivers/dplib/dpni/drv.h"

extern t_system sys;

/*********************************************************************/
/*
 * This addition if for dynamic aiop load
 */
/* Place MC <-> AIOP structures at fixed address.
 * Don't create new macro for section because no one else should use it */
#pragma push
#pragma force_active on
#pragma section  RW ".aiop_init_data" ".aiop_init_data_bss"
__declspec(section ".aiop_init_data")   struct aiop_init_data  g_init_data;
#pragma pop

/* TODO set good default values
 * TODO Update and review structure */
struct aiop_init_data g_init_data =
{
 /* aiop_sl_init_info */
 {
  4,	/* aiop_rev_major     AIOP  */
  2,	/* aiop_rev_minor     AIOP  */
  0,	/* ddr_phys_addr      */
  0,	/* peb_phys_addr      */
  0,	/* sys_ddr1_phys_add  */
  0,	/* ddr_virt_addr      */
  0,	/* peb_virt_addr      */
  0,	/* sys_ddr1_virt_addr */
  2,	/* uart_port_id       MC */
  1,	/* mc_portal_id       MC */
  0,	/* mc_dpci_id         MC */
  128,	/* clock_period       MC */
  {0}	/* reserved           */
 },
 /* aiop_app_init_info */
 {
  0,	/* dp_ddr_size */
  0,	/* peb_size */
  0,	/* sys_ddr1_size */
  0,	/* sys_ddr1_ctlu_size */
  0,	/* sys_ddr2_ctlu_size */
  0,	/* dp_ddr_ctlu_size */
  0,	/* peb_ctlu_size */
  {0}	/* reserved */
 }
};


/* Address of end of memory_data section */
extern const uint8_t AIOP_INIT_DATA[];

/*********************************************************************/

extern int mc_obj_init();           extern void mc_obj_free();
extern int cmdif_client_init();     extern void cmdif_client_free();
extern int cmdif_srv_init(void);    extern void cmdif_srv_free(void);
extern int dpni_drv_init(void);     extern void dpni_drv_free(void);
extern int slab_module_init(void);  extern void slab_module_free(void);
extern int aiop_sl_init(void);      extern void aiop_sl_free(void);

extern void discard_rx_cb();
extern void tman_timer_callback(void);
extern void cmdif_cl_isr(void);
extern void cmdif_srv_isr(void);


extern void build_apps_array(struct sys_module_desc *apps);


#define MEMORY_INFO                                                                                           \
{   /* Region ID                Memory partition ID             Phys. Addr.    Virt. Addr.  Size            */\
    {PLTFRM_MEM_RGN_MC_PORTALS, MEM_PART_INVALID,               0x80c000000LL, 0x08000000, (64  * MEGABYTE) },\
    {PLTFRM_MEM_RGN_AIOP,       MEM_PART_INVALID,               0x02000000,    0x02000000, (384 * KILOBYTE) },\
    {PLTFRM_MEM_RGN_CCSR,       MEM_PART_INVALID,               0x08000000,    0x0c000000, (16 * MEGABYTE)   },\
    {PLTFRM_MEM_RGN_SHRAM,      MEM_PART_SH_RAM,                0x01010400,    0x01010400, (191 * KILOBYTE) },\
    {PLTFRM_MEM_RGN_DP_DDR,     MEM_PART_DP_DDR,                0x6018000000,    0x58000000, (128 * MEGABYTE) },\
    {PLTFRM_MEM_RGN_PEB,        MEM_PART_PEB,                   0x4c00200000,    0x80200000, (2 * MEGABYTE)   },\
}

#define GLOBAL_MODULES                     \
{                                          \
    {epid_drv_init,     epid_drv_free},    \
    {mc_obj_init,       mc_obj_free},      \
    {slab_module_init,  slab_module_free}, \
    {cmdif_client_init, cmdif_client_free}, /* must be before srv */\
    {cmdif_srv_init,    cmdif_srv_free},   \
    {aiop_sl_init,      aiop_sl_free},     \
    {dpni_drv_init,     dpni_drv_free}, /*must be after aiop_sl_init*/   \
    {NULL, NULL} /* never remove! */       \
}

#define MAX_NUM_OF_APPS		10

void fill_platform_parameters(struct platform_param *platform_param);
int global_init(void);
void global_free(void);
int global_post_init(void);
int tile_init(void);
int cluster_init(void);
int run_apps(void);
void core_ready_for_tasks(void);
void global_free(void);
int epid_drv_init(void);
void epid_drv_free(void);

#include "general.h"
/** Global task params */
extern __TASK struct aiop_default_task_params default_task_params;

void fill_platform_parameters(struct platform_param *platform_param)
{
    struct platform_memory_info mem_info[] = MEMORY_INFO;

    memset(platform_param, 0, sizeof(platform_param));

    platform_param->clock_in_freq_hz = 100000000; //TODO check value
    platform_param->l1_cache_mode = E_CACHE_MODE_INST_ONLY;
    platform_param->console_type = PLTFRM_CONSOLE_DUART;
    platform_param->console_id = (uint8_t)g_init_data.sl_data.uart_port_id;
    memcpy(platform_param->mem_info,
           mem_info,
           sizeof(struct platform_memory_info)*ARRAY_SIZE(mem_info));
}

int tile_init(void)
{
    struct aiop_tile_regs * aiop_regs = (struct aiop_tile_regs *)
	                      sys_get_handle(FSL_OS_MOD_AIOP_TILE, 1);
    uint32_t val;

    if(aiop_regs) {
        /* ws enable */
        val = ioread32_ccsr(&aiop_regs->ws_regs.cfg);
        val |= 0x3; /* AIOP_WS_ENABLE_ALL - Enable work scheduler to receive tasks from both QMan and TMan */
        iowrite32_ccsr(val, &aiop_regs->ws_regs.cfg);
    }
    else {
    	return -EFAULT;
    }

    return 0;
}

int cluster_init(void)
{
	return 0;
}

int global_init(void)
{
    struct sys_module_desc modules[] = GLOBAL_MODULES;
    int                    i;

    /* Verifying that MC saw the data at the beginning of special section
     * and at fixed address
     * TODO is it the right place to verify it ? Can't place it at sys_init()
     * because it's too generic. */
    ASSERT_COND((((uint8_t *)(&g_init_data.sl_data)) == AIOP_INIT_DATA) &&
                (AIOP_INIT_DATA == AIOP_INIT_DATA_FIXED_ADDR));

    for (i=0; i<ARRAY_SIZE(modules) ; i++)
        if (modules[i].init)
    	    modules[i].init();

    return 0;
}

void global_free(void)
{
	struct sys_module_desc modules[] = GLOBAL_MODULES;
	int i;

	for (i = (ARRAY_SIZE(modules) - 1); i >= 0; i--)
		if (modules[i].free)
			modules[i].free();
}

int global_post_init(void)
{
	return 0;
}

#if (STACK_OVERFLOW_DETECTION == 1)
static inline void config_runtime_stack_overflow_detection()
{
    switch(cmgw_get_ntasks())
    {
    case 0: /* 1 Task */
    	booke_set_spr_DAC2(0x8000);
    	break;
    case 1: /* 2 Tasks */
    	booke_set_spr_DAC2(0x4000);
    	break;
    case 2: /* 4 Tasks */
    	booke_set_spr_DAC2(0x2000);
    	break;
    case 3: /* 8 Tasks */
    	booke_set_spr_DAC2(0x1000);
    	break;
    case 4: /* 16 Tasks */
    	booke_set_spr_DAC2(0x800);
    	break;
    default:
    	//TODO complete
    	break;
    }
}
#endif /* STACK_OVERFLOW_DETECTION */

void core_ready_for_tasks(void)
{
    /*  finished boot sequence; now wait for event .... */
    pr_info("AIOP core %d completed boot sequence\n", core_get_id());

    sys_barrier();

    if(sys_is_master_core()) {
        pr_info("AIOP boot finished; ready for tasks...\n");
    }

    sys_barrier();

    sys.runtime_flag = 1;

    cmgw_update_core_boot_completion();

#if (STACK_OVERFLOW_DETECTION == 1)
    /*
     *  NOTE:
     *  Any access to the stack (read/write) following this line will cause
     *  a stack-overflow violation and an exception will occur.
     */
    config_runtime_stack_overflow_detection();
#endif

    /* CTSEN = 1, finished boot, Core Task Scheduler Enable */
    booke_set_CTSCSR0(booke_get_CTSCSR0() | CTSCSR_ENABLE);
    __e_hwacceli(YIELD_ACCEL_ID); /* Yield */
}


static void print_dev_desc(struct dprc_obj_desc* dev_desc)
{
	pr_debug(" device %d\n", dev_desc->id);
	pr_debug("***********\n");
	pr_debug("vendor - %x\n", dev_desc->vendor);

	if (strcmp(dev_desc->type, "dpni") == 0)
		pr_debug("type - DP_DEV_DPNI\n");
	else if (strcmp(dev_desc->type, "dprc") == 0)
		pr_debug("type - DP_DEV_DPRC\n");
	else if (strcmp(dev_desc->type, "dpio") == 0)
		pr_debug("type - DP_DEV_DPIO\n");
	pr_debug("id - %d\n", dev_desc->id);
	pr_debug("region_count - %d\n", dev_desc->region_count);
	pr_debug("ver_major - %d\n", dev_desc->ver_major);
	pr_debug("ver_minor - %d\n", dev_desc->ver_minor);
	pr_debug("irq_count - %d\n\n", dev_desc->irq_count);

}


/* TODO: Need to replace this temporary workaround with the actual function.
/*****************************************************************************/
static int fill_bpid(uint16_t num_buffs,
                              uint16_t buff_size,
                              uint16_t alignment,
                              uint8_t  mem_partition_id,
                              uint16_t bpid)
{
    int        i = 0;
    dma_addr_t addr  = 0;
    struct slab_module_info *slab_m = sys_get_unique_handle(FSL_OS_MOD_SLAB);
    uint16_t icid = 0;
    uint32_t flags = FDMA_RELEASE_NO_FLAGS;

    /* Use the same flags as for slab */
    if (slab_m) {
	    icid  = slab_m->icid;
	    flags = slab_m->fdma_flags;
    }

    for (i = 0; i < num_buffs; i++) {
        addr = fsl_os_virt_to_phys(fsl_os_xmalloc(buff_size,
                                                  mem_partition_id,
                                                  alignment));
        /* Here, we pass virtual BPID, therefore BDI = 0 */
        fdma_release_buffer(icid, flags, bpid, addr);
    }
    return 0;
}

int run_apps(void)
{
	struct sys_module_desc apps[MAX_NUM_OF_APPS];
	int i;
	int err = 0, tmp = 0;
	int dev_count;
	/* TODO: replace with memset */
	uint16_t dpbp = 0;
	struct dprc_obj_desc dev_desc;
	int dpbp_id = -1;
	struct dpbp_attr attr;
	uint8_t region_index = 0;
	struct dpni_pools_cfg pools_params;
	uint16_t buffer_size = 2048;
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);


	/* TODO - add initialization of global default DP-IO (i.e. call 'dpio_open', 'dpio_init');
	* This should be mapped to ALL cores of AIOP and to ALL the tasks */
	/* TODO - add initialization of global default DP-SP (i.e. call 'dpsp_open', 'dpsp_init');
	* This should be mapped to 3 buff-pools with sizes: 128B, 512B, 2KB;
	* all should be placed in PEB. */
	/* TODO - need to scan the bus in order to retrieve the AIOP "Device list" */
	/* TODO - iterate through the device-list:
	* call 'dpni_drv_probe(ni_id, mc_portal_id, dpio, dp-sp)' */

	if (dprc == NULL)
	{
		pr_err("Don't find AIOP root container \n");
		return -ENODEV;
	}
	/* TODO: replace the following dpbp_open&init with dpbp_create when available */


	if ((err = dprc_get_obj_count(&dprc->io, dprc->token,
	                              &dev_count)) != 0) {
		pr_err("Failed to get device count for AIOP RC auth_id = %d.\n",
		       dprc->token);
		return err;
	}

	for (i = 0; i < dev_count; i++) {
		dprc_get_obj(&dprc->io, dprc->token, i, &dev_desc);
		if (strcmp(dev_desc.type, "dpbp") == 0) {
			/* TODO: print conditionally based on log level */
			pr_info("Found First DPBP ID: %d, will be used for frame buffers\n",dev_desc.id);
			dpbp_id	= dev_desc.id;
			break;
		}
	}

	if(dpbp_id < 0){
		pr_err("DPBP not found in the container.\n");
		return -ENAVAIL;
	}

	if ((err = dpbp_open(&dprc->io, dpbp_id, &dpbp)) != 0) {
		pr_err("Failed to open DPBP-%d.\n", dpbp_id);
		return err;
	}

	if ((err = dpbp_enable(&dprc->io, dpbp)) != 0) {
		pr_err("Failed to enable DPBP-%d.\n", dpbp_id);
		return err;
	}

	if ((err = dpbp_get_attributes(&dprc->io, dpbp, &attr)) != 0) {
		pr_err("Failed to get attributes from DPBP-%d.\n", dpbp_id);
		return err;
	}

	/* TODO: number and size of buffers should not be hard-coded */
	if ((err = fill_bpid(100, buffer_size, 64, MEM_PART_PEB, attr.bpid)) != 0) {
		pr_err("Failed to fill DPBP-%d (BPID=%d) with buffer size %d.\n",
				dpbp_id, attr.bpid, buffer_size);
		return err;
	}

	/* Prepare parameters to attach to DPNI object */
	pools_params.num_dpbp = 1; /* for AIOP, can be up to 2 */
	pools_params.pools[0].dpbp_id = (uint16_t)dpbp_id; /*!< DPBPs object id */
	pools_params.pools[0].buffer_size = buffer_size;


	/* Enable all DPNI devices */
	for (i = 0; i < dev_count; i++) {
		dprc_get_obj(&dprc->io, dprc->token, i, &dev_desc);
		if (strcmp(dev_desc.type, "dpni") == 0) {
			/* TODO: print conditionally based on log level */
			print_dev_desc(&dev_desc);

			if ((err = dpni_drv_probe(dprc, (uint16_t)dev_desc.id, (uint16_t)i, &pools_params)) != 0) {
				pr_err("Failed to probe DPNI-%d.\n", i);
				return err;
			}
		}
	}


	/* At this stage, all the NIC of AIOP are up and running */

	memset(apps, 0, sizeof(apps));
	build_apps_array(apps);

	for (i=0; i<MAX_NUM_OF_APPS; i++) {
		if (apps[i].init)
			apps[i].init();
	}

	return 0;
}

static int cmdif_epid_setup(struct aiop_ws_regs *wrks_addr,
                            uint32_t epid,
                            void (*isr_cb)(void))
{
	uint32_t data = 0;
	int      err = 0;

	iowrite32_ccsr(epid, &wrks_addr->epas); /* EPID = 2 */
	iowrite32_ccsr(PTR_TO_UINT(isr_cb), &wrks_addr->ep_pc);

#ifdef AIOP_STANDALONE
	/* Default settings */
	iowrite32_ccsr(0x00600040, &wrks_addr->ep_fdpa);
	iowrite32_ccsr(0x010001c0, &wrks_addr->ep_spa);
	iowrite32_ccsr(0x00000000, &wrks_addr->ep_spo);
#endif

	/* no PTA presentation is required (even if there is a PTA)*/
	iowrite32_ccsr(0x0000ffc0, &wrks_addr->ep_ptapa);
	/* set epid ASA presentation size to 0 */
	iowrite32_ccsr(0x00000000, &wrks_addr->ep_asapa);
	/* Set mask for hash to 16 low bits OSRM = 5 */
	iowrite32_ccsr(0x11000005, &wrks_addr->ep_osc);
	data = ioread32_ccsr(&wrks_addr->ep_osc);
	if (data != 0x11000005)
		err |= -EINVAL;

	pr_info("CMDIF is setting EPID = %d\n", epid);
	pr_info("ep_pc = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_pc));
	pr_info("ep_fdpa = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_fdpa));
	pr_info("ep_ptapa = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_ptapa));
	pr_info("ep_asapa = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_asapa));
	pr_info("ep_spa = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_spa));
	pr_info("ep_spo = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_spo));
	pr_info("ep_osc = 0x%x \n", ioread32_ccsr(&wrks_addr->ep_osc));

	if (err) {
		pr_err("Failed to setup EPID %d\n", epid);
		/* No return err here in order to setup the rest of EPIDs */
	}
	return err;
}

int epid_drv_init(void)
{
	int i = 0;
	int err = 0;

	struct aiop_ws_regs *wrks_addr = (struct aiop_ws_regs *)
		(sys_get_memory_mapped_module_base(FSL_OS_MOD_CMGW,
		                                   0,
		                                   E_MAPPED_MEM_TYPE_GEN_REGS)
		                                   + SOC_PERIPH_OFF_AIOP_WRKS);

	/* CMDIF server epid initialization here*/
	err |= cmdif_epid_setup(wrks_addr, AIOP_EPID_CMDIF_SERVER, cmdif_srv_isr);

	/* TMAN epid initialization */
	iowrite32_ccsr(AIOP_EPID_TIMER_EVENT_IDX, &wrks_addr->epas); /* EPID = 1 */
	iowrite32_ccsr(PTR_TO_UINT(tman_timer_callback), &wrks_addr->ep_pc);
	iowrite32_ccsr(0x02000000, &wrks_addr->ep_spo); /* SET NDS bit */

	pr_info("TMAN is setting EPID = %d\n", AIOP_EPID_TIMER_EVENT_IDX);
	pr_info("ep_pc = 0x%x\n", ioread32_ccsr(&wrks_addr->ep_pc));
	pr_info("ep_fdpa = 0x%x\n", ioread32_ccsr(&wrks_addr->ep_fdpa));
	pr_info("ep_ptapa = 0x%x\n", ioread32_ccsr(&wrks_addr->ep_ptapa));
	pr_info("ep_asapa = 0x%x\n", ioread32_ccsr(&wrks_addr->ep_asapa));
	pr_info("ep_spa = 0x%x\n", ioread32_ccsr(&wrks_addr->ep_spa));
	pr_info("ep_spo = 0x%x\n", ioread32_ccsr(&wrks_addr->ep_spo));


	/* CMDIF interface client epid initialization here*/
	err |= cmdif_epid_setup(wrks_addr, AIOP_EPID_CMDIF_CLIENT, cmdif_cl_isr);

	/* Initialize EPID-table with discard_rx_cb for all NI's entries (EP_PC field) */
	for (i = AIOP_EPID_DPNI_START; i < AIOP_EPID_TABLE_SIZE; i++) {
		/* Prepare to write to entry i in EPID table - EPAS reg */
		iowrite32_ccsr((uint32_t)i, &wrks_addr->epas);

		iowrite32_ccsr(PTR_TO_UINT(discard_rx_cb), &wrks_addr->ep_pc);
	}

	return err;
}

void epid_drv_free(void)
{
}
