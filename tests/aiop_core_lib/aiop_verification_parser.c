/**************************************************************************//**
@File		aiop_verification_parser.c

@Description	This file contains the AIOP Parser SRs SW Verification

		Copyright 2013 Freescale Semiconductor, Inc.
*//***************************************************************************/

#include "dplib/fsl_parser.h"
#include "system.h"

#include "aiop_verification.h"
#include "aiop_verification_parser.h"

extern __TASK struct aiop_default_task_params default_task_params;

uint16_t aiop_verification_parser(uint32_t asa_seg_addr)
{
	uint16_t str_size = STR_SIZE_ERR;
	uint32_t opcode;
	

	opcode  = *((uint32_t *) asa_seg_addr);


	switch (opcode) {

	case PARSER_PRP_CREATE_STR:
	{
		struct parser_prp_create_verif_command *pc =
				(struct parser_prp_create_verif_command *)
				asa_seg_addr;

		pc->status = 
			parser_profile_create(
			(struct parse_profile_record *)pc->parse_profile,
			&pc->prpid);
		str_size = sizeof(struct parser_prp_create_verif_command);
		break;
	}
	case PARSER_PRP_DELETE_STR:
	{
		struct parser_prp_delete_verif_command *pd =
				(struct parser_prp_delete_verif_command *)
				asa_seg_addr;
		pd->status = parser_profile_delete(pd->prpid);
		str_size = sizeof(struct parser_prp_delete_verif_command);
		break;
	}
	case PARSER_PRP_QUERY_STR:
	{
		struct parser_prp_query_verif_command *pq =
				(struct parser_prp_query_verif_command *)
				asa_seg_addr;
		pq->status = 
		   parser_profile_query(pq->prpid,
			(struct parse_profile_record *)pq->parse_profile);
		str_size = sizeof(struct parser_prp_query_verif_command);
		break;
	}
	case PARSER_GEN_PARSE_RES_STR:
	{
		struct parser_gen_parser_res_verif_command *gpr =
			(struct parser_gen_parser_res_verif_command *)
			asa_seg_addr;
		default_task_params.parser_profile_id = gpr->prpid;
		gpr->status = parse_result_generate_default(gpr->flags);
		str_size = sizeof(struct parser_gen_parser_res_verif_command);
		break;
	}
	case PARSER_GEN_PARSE_RES_EXP_STR:
	{
		struct parser_gen_parser_res_exp_verif_command *gpre =
			(struct parser_gen_parser_res_exp_verif_command *)
			asa_seg_addr;
		gpre->status = parse_result_generate(
				(enum parser_starting_hxs_code)gpre->hxs,
							 gpre->offset,
							 gpre->flags);
		str_size =
			sizeof(struct parser_gen_parser_res_exp_verif_command);
		break;
	}
	case PARSER_PRP_ID_POOL_CREATE_STR:
	{
		struct parser_prp_id_pool_create_verif_command *pipc =
			(struct parser_prp_id_pool_create_verif_command*)
			asa_seg_addr;
		pipc->status = sys_ctlu_prpid_pool_create();
		str_size =
			sizeof(struct parser_prp_id_pool_create_verif_command);
		break;
	}
	default:
	{
		return STR_SIZE_ERR;
	}
	}

	return str_size;
}

