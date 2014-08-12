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

/**************************************************************************//**
@File          snic.h

@Description   This file contains the snic include file.

*//***************************************************************************/

#ifndef __SNIC_H
#define __SNIC_H

#include "common/types.h"
#include "dplib/fsl_ipr.h"
#include "dplib/fsl_snic_cmd.h"

/**************************************************************************//**
@Group		SNIC AIOP snic Internal

@Description	Freescale AIOP snic internal API

@{
*//***************************************************************************/

/**************************************************************************//**
@Group	SNIC_MACROS snic Macros

@Description	Freescale AIOP snic Macros

@{
*//***************************************************************************/

#define SNIC_PRPID	0
#define SNIC_HXS	0
#define SNIC_SPID	0

#define VLAN_VID_MASK	0x00000FFF
#define VLAN_PCP_MASK	0x0000E000
#define VLAN_PCP_SHIFT	13

#define MAX_SNIC_NO     16

/** @} */ /* end of SNIC_MACROS */

/**************************************************************************//**
@Group		SNIC_STRUCTS snic Structures

@Description	Freescale AIOP snic Structures

@{
*//***************************************************************************/

/**************************************************************************//**
@Description	AIOP snic parameters
*//***************************************************************************/

struct snic_params {
	/** IPR instance is per snic */
	ipr_instance_handle_t ipr_instance_val;
	/** snic general enable flags */
	uint32_t snic_enable_flags;
	/** IPF MTU */
	uint16_t snic_ipf_mtu;
	/** Queueing destination for the enqueue. */
	uint16_t qdid;
	/** valid */
	uint32_t valid;
	/** Storage profile ID */
	uint8_t spid;
	uint8_t res[3];
};

/** @} */ /* end of SNIC_STRUCTS */

void snic_process_packet(void);
int aiop_snic_init(void);
int snic_ipf(struct snic_params *snic);
int snic_ipr(struct snic_params *snic);
int snic_add_vlan(void);
/** @} */ /* end of SNIC */


#endif /* __SNIC_H */
