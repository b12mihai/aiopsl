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
@File          aiop_verification_ste.h

@Description   This file contains the AIOP STE SW Verification Structures
*//***************************************************************************/


#ifndef __AIOP_VERIFICATION_STE_H_
#define __AIOP_VERIFICATION_STE_H_

#include "dplib/fsl_ldpaa.h"


/**************************************************************************//**
 @addtogroup		AIOP_Service_Routines_Verification

 @{
*//***************************************************************************/

/**************************************************************************//**
 @Group		AIOP_STE_SRs_Verification

 @Description	AIOP STE Verification structures definitions.

 @{
*//***************************************************************************/

#define STE_VERIF_ACCEL_ID	0xFF
	/**< STE accelerator ID For verification purposes*/

/*! \enum e_ste_cmd_type defines the statistics engine CMDTYPE field.*/
enum e_ste_verif_cmd_type {
	STE_CMDTYPE_SET_4B = 0,
	STE_CMDTYPE_SET_8B,
	STE_CMDTYPE_ADD,
	STE_CMDTYPE_DEC,
	STE_CMDTYPE_INC_ADD,
	STE_CMDTYPE_INC_SUB,
	STE_CMDTYPE_DEC_ADD,
	STE_CMDTYPE_DEC_SUB,
	STE_CMDTYPE_BARRIER,
	STE_CMDTYPE_READ_ERRORS,
	STE_CMDTYPE_CLEAR_ERRORS
};

/* STE Commands Structure identifiers */
#define STE_SET_4B_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_SET_4B)

#define STE_SET_8B_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_SET_8B)

#define STE_ADD_CMD_STR			((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_ADD)

#define STE_SUB_CMD_STR			((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_SUB)

#define STE_ADD_CMD_STR			((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_ADD)

#define STE_DEC_CMD_STR			((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_DEC)

#define STE_DEC_8B_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_DEC_8B)

#define STE_INC_ACC_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_INC_ADD)

#define STE_INC_SUB_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_INC_SUB)

#define STE_DEC_ACC_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_DEC_ADD)

#define STE_DEC_SUB_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_DEC_SUB)

#define STE_BARRIER_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_BARRIER)

#define STE_READ_ERRORS_CMD_STR		((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_READ_ERRORS)

#define STE_CLEAR_ERRORS_CMD_STR	((STE_MODULE << 16) | \
		(uint32_t)STE_CMDTYPE_CLEAR_ERRORS)

/**************************************************************************//**
@Description	STE set 4 byte counter Command structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_set_4byte_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint8_t		pad[4];
	uint64_t	counter_addr;
	uint32_t	value;
	uint8_t		pad2[4];
};


/**************************************************************************//**
@Description	STE set 8 byte counter Command structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_set_8byte_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint8_t		pad[4];
	uint64_t	counter_addr;
	uint64_t	value;
};


/**************************************************************************//**
@Description	STE add/dec from counter Command structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_add_dec_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint8_t		pad2[4];
	uint64_t	counter_addr;
	uint32_t	value;
	uint32_t	flags;
};


/**************************************************************************//**
@Description	STE compound Command structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_compound_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint8_t		pad[4];
	uint64_t	counter_addr;
	uint32_t	acc_value;
	uint32_t	flags;
};

/**************************************************************************//**
@Description	STE barrier Command structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_barrier_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
};

/**************************************************************************//**
@Description	STE Get error params Commands structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_read_error_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint32_t	status_reg;
	uint32_t	capture_attributes;
	uint32_t	acc_value;
	uint32_t	counter_msb;
	uint32_t	counter_lsb;
};

/**************************************************************************//**
@Description	STE clear errors Commands structure.

		Includes information needed for STE Command verification.
*//***************************************************************************/
struct ste_clear_error_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
};

uint16_t aiop_verification_ste(uint32_t asa_seg_addr);

/** @} */ /* end of AIOP_STE_SRs_Verification */

/** @} */ /* end of AIOP_Service_Routines_Verification */


#endif /* __AIOP_VERIFICATION_STE_H_ */
