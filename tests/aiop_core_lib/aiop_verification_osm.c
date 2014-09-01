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
@File		aiop_verification_osm.c

@Description	This file contains the AIOP OSM SRs SW Verification.

*//***************************************************************************/

#include "dplib/fsl_osm.h"

#include "osm.h"

#include "aiop_verification.h"
#include "aiop_verification_osm.h"


uint16_t aiop_verification_osm(uint32_t asa_seg_addr)
{
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	uint16_t str_size = STR_SIZE_ERR;
	uint32_t opcode;
	*(uint32_t *)OSM_REG_OERR  = ENABLE_ERR_REG;

	opcode  = *((uint32_t *) asa_seg_addr);

	switch (opcode) {
		
	case OSM_SCOPE_TRANS_TO_EX_INC_SCOPE_ID_STR:
	{
		struct osm_scope_tran_to_ex_inc_scope_id_verif_command *str =
			(struct osm_scope_tran_to_ex_inc_scope_id_verif_command *) 
																asa_seg_addr;
		osm_scope_transition_to_exclusive_with_increment_scope_id();
		str->status = OSM_SUCCESS;
	
		str_size = (uint16_t)
			sizeof(struct osm_scope_tran_to_ex_inc_scope_id_verif_command);
		break;
	}
	
	case OSM_SCOPE_TRANS_TO_EX_NEW_SCOPE_ID_STR:
	{
		struct osm_scope_tran_to_ex_new_scope_id_verif_command *str =
			(struct osm_scope_tran_to_ex_new_scope_id_verif_command *) 
																asa_seg_addr;
	
		osm_scope_transition_to_exclusive_with_new_scope_id(str->scope_id);
		str->status = OSM_SUCCESS;
		
		str_size = (uint16_t)
			sizeof(struct osm_scope_tran_to_ex_new_scope_id_verif_command);	
		break;
	}

	case OSM_SCOPE_TRANS_TO_CON_INC_SCOPE_ID_STR:
	{
		struct osm_scope_tran_to_con_inc_scope_id_verif_command *str =
			(struct osm_scope_tran_to_con_inc_scope_id_verif_command *) 
																asa_seg_addr;

		osm_scope_transition_to_concurrent_with_increment_scope_id();
		str->status = OSM_SUCCESS;
		
		str_size = (uint16_t)
			sizeof(struct osm_scope_tran_to_con_inc_scope_id_verif_command);
		break;
	}
	
	case OSM_SCOPE_TRANS_TO_CON_NEW_SCOPE_ID_STR:
	{
		struct osm_scope_tran_to_con_new_scope_id_verif_command *str =
			(struct osm_scope_tran_to_con_new_scope_id_verif_command *) 
																asa_seg_addr;

		osm_scope_transition_to_concurrent_with_new_scope_id(str->scope_id);
		str->status = OSM_SUCCESS;
		
		str_size = (uint16_t)
			sizeof(struct osm_scope_tran_to_con_new_scope_id_verif_command);
		break;
	}

	case OSM_SCOPE_RELINQUISH_EX_STR:
	{
		struct osm_scope_relinquish_ex_verif_command *str =
			(struct osm_scope_relinquish_ex_verif_command *) asa_seg_addr;
		
		osm_scope_relinquish_exclusivity();
		
		str_size = (uint16_t)
				sizeof(struct osm_scope_relinquish_ex_verif_command);
		break;
	}
	
	case OSM_SCOPE_ENTER_TO_EX_INC_SCOPE_ID_STR:
	{
		struct osm_scope_enter_to_ex_inc_scope_id_verif_command *str =
			(struct osm_scope_enter_to_ex_inc_scope_id_verif_command *)
															asa_seg_addr;

		osm_scope_enter_to_exclusive_with_increment_scope_id();
		str->status = OSM_SUCCESS;
		
		str_size = (uint16_t)
			sizeof(struct osm_scope_enter_to_ex_inc_scope_id_verif_command);
		break;
	}
	
	case OSM_SCOPE_ENTER_TO_EX_NEW_SCOPE_ID_STR:
	{
		struct osm_scope_enter_to_ex_new_scope_id_verif_command *str =
			(struct osm_scope_enter_to_ex_new_scope_id_verif_command *) 
															asa_seg_addr;

		osm_scope_enter_to_exclusive_with_new_scope_id(str->child_scope_id);
		str->status = OSM_SUCCESS;
		
		str_size = (uint16_t)
			sizeof(struct osm_scope_enter_to_ex_new_scope_id_verif_command);
		break;
	}
	
	case OSM_SCOPE_ENTER_STR:
	{
		struct osm_scope_enter_verif_command *str =
			(struct osm_scope_enter_verif_command *) asa_seg_addr;

		osm_scope_enter(str->scope_enter_flags,str->child_scope_id);
		str->status = OSM_SUCCESS;
		
		str_size = (uint16_t)sizeof(struct osm_scope_enter_verif_command);
		break;
	}
	
	case OSM_SCOPE_EXIT_STR:
	{
		struct osm_scope_exit_verif_command *str =
			(struct osm_scope_exit_verif_command *) asa_seg_addr;
		
		osm_scope_exit();
		
		str_size = (uint16_t)sizeof(struct osm_scope_exit_verif_command);
		break;
	}
	
	case OSM_GET_SCOPE_STR:
	{
		struct osm_get_scope_verif_command *str =
			(struct osm_get_scope_verif_command *) asa_seg_addr;
		
		/* initialize TASK_ID (=0) in ORTAR to enable OSM registers */
		*(uint32_t *)OSM_REG_ORTAR = 0;

		osm_get_scope((struct scope_status_params *)str->scope_status);

		((struct osm_registers *)str->scope_status)->ortdr0 = OSM_REG_ORTDR0();
		((struct osm_registers *)str->scope_status)->ortdr1 = OSM_REG_ORTDR1();
		((struct osm_registers *)str->scope_status)->ortdr2 = OSM_REG_ORTDR2();
		((struct osm_registers *)str->scope_status)->ortdr3 = OSM_REG_ORTDR3();
		((struct osm_registers *)str->scope_status)->ortdr4 = OSM_REG_ORTDR4();
		((struct osm_registers *)str->scope_status)->ortdr5 = OSM_REG_ORTDR5();
		((struct osm_registers *)str->scope_status)->ortdr6 = OSM_REG_ORTDR6();
		((struct osm_registers *)str->scope_status)->ortdr7 = OSM_REG_ORTDR7();

		((struct osm_error_reg *)str->err_scope_status)->oedr = OSM_REG_OEDR();
		((struct osm_error_reg *)str->err_scope_status)->oecr0 = OSM_REG_OECR0();
		((struct osm_error_reg *)str->err_scope_status)->oecr1 = OSM_REG_OECR1();
		((struct osm_error_reg *)str->err_scope_status)->oecr2 = OSM_REG_OECR2();
		((struct osm_error_reg *)str->err_scope_status)->oecr3 = OSM_REG_OECR3();
		((struct osm_error_reg *)str->err_scope_status)->oecr4 = OSM_REG_OECR4();
		((struct osm_error_reg *)str->err_scope_status)->oecr5 = OSM_REG_OECR5();
		((struct osm_error_reg *)str->err_scope_status)->oecr6 = OSM_REG_OECR6();
		((struct osm_error_reg *)str->err_scope_status)->oecr7 = OSM_REG_OECR7();

		str_size = (uint16_t)sizeof(struct osm_get_scope_verif_command);
		break;
	}
	
	default:
	{
		return STR_SIZE_ERR;
	}
	}

	return str_size;
}



