/**

 @File          fsl_soc_spec.h

 @Description   LS2085A external definitions and structures.
*//***************************************************************************/
#ifndef __FSL_SOC_SPEC_H
#define __FSL_SOC_SPEC_H

#include "common/types.h"
#if defined(AIOP)
#include "fsl_soc_aiop_spec.h"
#elif defined(MC)
#include "fsl_soc_mc_spec.h"
#elif defined(GPP)
#include "fsl_soc_gpp_spec.h"
#else
#error "SoC subsystem not defined!"
#endif


/**************************************************************************//**
 @Group         ls2085a_g LS2085A Application Programming Interface

 @Description   LS2085A Chip functions,definitions and enums.

 @{
*//***************************************************************************/

#define FSL_OS_NUM_MODULES  FSL_OS_MOD_DUMMY_LAST

/* Offsets relative to CCSR base */

#define SOC_PERIPH_OFF_DUART1           0x021C0500
#define SOC_PERIPH_OFF_DUART2           0x021C0600
#define SOC_PERIPH_OFF_DUART3           0x021d0500
#define SOC_PERIPH_OFF_DUART4           0x021d0600
#define SOC_PERIPH_OFF_QBMAN            0x08180000
#define SOC_PERIPH_OFF_EIOP             0x08b90000
#define SOC_PERIPH_OFF_MC               0x08340000
#define SOC_PERIPH_OFF_SEC_GEN          0x08000000
#define SOC_PERIPH_OFF_EIOP_IFPS        0x08800000



/* Offset of MC portals  relative to MC area base */
#define PERIPH_OFF_MC_PORTALS_AREA  0x0000000
#define SOC_PERIPH_MC_PORTAL_ALIGNMENT  0x10000 /* Alignment of a MC portal in SoC */



#define SOC_PERIPH_OFF_PORTALS_MC(_prtl) \
    (PERIPH_OFF_MC_PORTALS_AREA + SOC_PERIPH_MC_PORTAL_ALIGNMENT * (_prtl))



/**************************************************************************//**
 @Group         ls2085a_init_g LS2085A Initialization Unit

 @Description   LS2085A initialization unit API functions, definitions and enums

 @{
*//***************************************************************************/

/** @} */ /* end of ls2085a_init_g group */
/** @} */ /* end of ls2085a_g group */



/*****************************************************************************
 INTEGRATION-SPECIFIC MODULE CODES
******************************************************************************/
#define MODULE_UNKNOWN          0x00000000
#define MODULE_SLAB             0x00010000
#define MODULE_SLOB             0x00020000
#define MODULE_SOC_PLATFORM     0x00030000
#define MODULE_PIC              0x00040000
#define MODULE_DUART            0x00050000
#define MODULE_DPSW             0x00060000
#define MODULE_DPRC             0x00070000
#define MODULE_LINKMAN          0x00080000
#define MODULE_DPDMUX			0x00090000
#define MODULE_DPMAC			0x000a0000
#define MODULE_DPCI             0x000b0000
#define MODULE_DPSECI           0x000c0000




#endif /* __FSL_SOC_SPEC_H */
