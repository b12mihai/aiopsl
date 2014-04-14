#include "common/types.h"
#include "common/fsl_stdio.h"
#include "common/fsl_string.h"
#include "fsl_fdma.h"
#include "general.h"
#include "fsl_cdma.h"
#include "common/fsl_slab.h"
#include "kernel/platform.h"
#include "common/fsl_aiop_cmdif.h"
#include "cmdif_srv.h"
#include "io.h"
#include "aiop_common.h"

int app_init(void);
void app_free(void);

//#define REFLECTOR_DEMO

#ifdef REFLECTOR_DEMO
extern uint32_t    sync_done;

/* Client STUB */
static int my_cmdif_open(struct cmdif_desc *cidesc,
		const char *module_name,
		uint8_t instance_id,
		cmdif_cb_t async_cb,
		void *async_ctx)
{
	struct   cmdif_srv *srv = sys_get_handle(FSL_OS_MOD_CMDIF_SRV, 0);
	UNUSED(cidesc);
	UNUSED(module_name);
	UNUSED(instance_id);
	UNUSED(async_cb);
	UNUSED(async_ctx);
	srv->inst_dev[0] = (void *)0x22222222;
	srv->sync_done[0] = (uint64_t)&sync_done;
	return 0;
}
#endif

static int open_cb(uint8_t instance_id, void **dev)
{
	UNUSED(instance_id);
	UNUSED(dev);
	fsl_os_print("open_cb\n");
	return 0;
}

static int close_cb(void *dev)
{
	UNUSED(dev);
	fsl_os_print("close_cb\n");
	return 0;
}

static int ctrl_cb(void *dev, uint16_t cmd, uint32_t size, uint8_t *data)
{
	UNUSED(dev);
	UNUSED(cmd);
	UNUSED(size);
	UNUSED(data);
	fsl_os_print("ctrl_cb\n");
	return 0;
}

/* This is temporal WA for stand alone demo only */
#define WRKS_REGS_GET \
	(sys_get_memory_mapped_module_base(FSL_OS_MOD_CMGW,            \
	                                   0,                          \
	                                   E_MAPPED_MEM_TYPE_GEN_REGS) \
	                                   + SOC_PERIPH_OFF_AIOP_WRKS);
static void epid_setup()
{
	struct aiop_ws_regs *wrks_addr = (struct aiop_ws_regs *)WRKS_REGS_GET;

	/* EPID = 0 is saved for cmdif, need to set it for stand alone demo */
	iowrite32(0, &wrks_addr->epas); 
	iowrite32(0x00fe0000, &wrks_addr->ep_pc);
}

int app_init(void)
{
	const char * module = "ABCABC";
	struct cmdif_module_ops ops;
	int err = 0;

	ops.open_cb  = open_cb;
	ops.close_cb = close_cb;
	ops.ctrl_cb  = ctrl_cb;

	err = cmdif_register_module(module, &ops);
	if (err) return err;
	err = cmdif_register_module(module, &ops);
	if (!err) return err;
	err = cmdif_register_module(module, &ops);
	if (!err) return err;
	err = cmdif_register_module("ABCDEF", &ops);
	if (err) return err;
	err = cmdif_register_module("TTTTTTTTTTT", &ops);
	if (err) return err;
	err = cmdif_unregister_module("ABCDEF");
	if (err) return err;
	err = cmdif_unregister_module("TTTTTTTTTTT");
	if (err) return err;

#ifdef REFLECTOR_DEMO
	err = my_cmdif_open(NULL, module, 0, NULL, NULL);
#else
	/* More complex demo that tests client server different commands */
	epid_setup();
#endif

	
	return err;
}

void app_free(void)
{

}