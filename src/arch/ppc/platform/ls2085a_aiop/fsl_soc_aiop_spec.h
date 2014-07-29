/**

 @File          fsl_soc_aiop_spec.h

 @Description   LS2085A external definitions and structures.
*//***************************************************************************/
#ifndef __FSL_SOC_AIOP_SPEC_H
#define __FSL_SOC_AIOP_SPEC_H

#include "common/types.h"


/**************************************************************************//**
 @Group         ls2085a_g LS2085A Application Programming Interface

 @Description   LS2085A Chip functions,definitions and enums.

 @{
*//***************************************************************************/

#define CORE_E200
#define CORE_E200_Z490

#define INTG_MAX_NUM_OF_CORES   16
#define INTG_THREADS_PER_CORE   1


/**************************************************************************//**
 @Description   Module types.
*//***************************************************************************/
enum fsl_os_module {
	FSL_OS_MOD_SOC = 0,

	FSL_OS_MOD_CMDIF_SRV, /**< AIOP server handle */
	FSL_OS_MOD_CMDIF_CL,  /**< AIOP client handle */
	FSL_OS_MOD_SLAB,
	FSL_OS_MOD_UART,
	FSL_OS_MOD_CMGW,
	FSL_OS_MOD_DPRC,
	FSL_OS_MOD_DPNI,
	FSL_OS_MOD_DPIO,
	FSL_OS_MOD_DPSP,
	FSL_OS_MOD_DPSW,

	FSL_OS_MOD_AIOP_TILE,

	FSL_OS_MOD_MC_PORTAL,
	FSL_OS_MOD_AIOP_RC,    /**< AIOP root container from DPL */
	FSL_OS_MOD_DPCI_TBL,   /**< AIOP DPCI table from DPL */

	FSL_OS_MOD_LAYOUT, /* TODO - review *//**< layout */

	FSL_OS_MOD_DUMMY_LAST
};

/** @} */ /* end of ls2085a_g group */


/* Offsets relative to CCSR base */
#define SOC_PERIPH_OFF_AIOP_WRKS        0x1d000
#define SOC_PERIPH_OFF_AIOP_TILE        0x00080000
#define SOC_PERIPH_OFF_AIOP_CMGW        0x0

#endif /* __FSL_SOC_AIOP_SPEC_H */