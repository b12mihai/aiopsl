/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
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

/**************************************************************************//**
@File          apps.h

@Description   This file contains the AIOP SL user defined setup.

Please refer to the apps.h of app_process_packet for the full documentation
of all the macros.
*//***************************************************************************/

#ifndef __APPS_H
#define __APPS_H

#include "apps_arch.h"
#include "system.h"

/* For better performances enable all core's tasks */
#define APP_INIT_TASKS_PER_CORE		16
#define APP_INIT_APP_MAX_NUM		10

#define APP_MEM_DP_DDR_SIZE		ARCH_DP_DDR_SIZE
#define APP_MEM_PEB_SIZE		(512 * KILOBYTE)
#define APP_MEM_SYS_DDR1_SIZE		(32 * MEGABYTE)

#define APP_CTLU_SYS_DDR_NUM_ENTRIES	2048 
#define APP_CTLU_DP_DDR_NUM_ENTRIES	ARCH_CTLU_DP_DDR_NUM_ENTRIES
#define APP_CTLU_PEB_NUM_ENTRIES	2048 

#define APP_MFLU_SYS_DDR_NUM_ENTRIES	2048
#define APP_MFLU_DP_DDR_NUM_ENTRIES	ARCH_MFLU_DP_DDR_NUM_ENTRIES
#define APP_MFLU_PEB_NUM_ENTRIES	2048

/* For better performances, the number of buffer should be at least equal with
 * the number of AIOP tasks */
#define APP_DPNI_NUM_BUFS_IN_POOL	(16 * APP_INIT_TASKS_PER_CORE)
#define APP_DPNI_BUF_SIZE_IN_POOL	2048
#define APP_DPNI_BUF_ALIGN_IN_POOL	64
#define APP_DPNI_SPID_COUNT		8

int app_early_init(void);
int app_init(void);
void app_free(void);

extern struct platform_app_params g_app_params;

/*#define IPSEC_DEBUG_PRINT_SP*/

#ifdef IPSEC_DEBUG_PRINT_SP
	extern __PROFILE_SRAM struct storage_profile
				storage_profile[SP_NUM_OF_STORAGE_PROFILES];
#endif

#endif /* __APPS_H */
