/**************************************************************************//**
@File		aiop_verification_parser.c

@Description	This file contains the AIOP Parser SRs SW Verification

*//***************************************************************************/

//#include "dplib/fsl_parser.h"
#include "parser.h"
#include "system.h"

#include "aiop_verification.h"
#include "aiop_verification_parser.h"

extern __TASK struct aiop_default_task_params default_task_params;

void aiop_init_parser(uint8_t *prpid)
{
	uint8_t i;
	struct parse_profile_input verif_parse_profile1 __attribute__((aligned(16)));

	/* Init basic parse profile */
	verif_parse_profile1.parse_profile.eth_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.llc_snap_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.vlan_hxs_config.en_erm_soft_seq_start = 0x0;
	verif_parse_profile1.parse_profile.vlan_hxs_config.configured_tpid_1 = 0x0;
	verif_parse_profile1.parse_profile.vlan_hxs_config.configured_tpid_2 = 0x0;
	/* No MTU checking */
	verif_parse_profile1.parse_profile.pppoe_ppp_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.mpls_hxs_config.en_erm_soft_seq_start= 0x0;
	/* Frame Parsing advances to MPLS Default Next Parse (IP HXS) */
	verif_parse_profile1.parse_profile.mpls_hxs_config.lie_dnp =
					PARSER_PRP_MPLS_HXS_CONFIG_LIE;
	verif_parse_profile1.parse_profile.arp_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.ip_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.ipv4_hxs_config = 0x0;
	/* Routing header is ignored and the destination address from
	 * main header is used instead */
	verif_parse_profile1.parse_profile.ipv6_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.gre_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.minenc_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.other_l3_shell_hxs_config= 0x0;
	/* In short Packet, padding is removed from Checksum calculation */
	verif_parse_profile1.parse_profile.tcp_hxs_config = PARSER_PRP_TCP_UDP_HXS_CONFIG_SPPR;
	/* In short Packet, padding is removed from Checksum calculation */
	verif_parse_profile1.parse_profile.udp_hxs_config = PARSER_PRP_TCP_UDP_HXS_CONFIG_SPPR;
	verif_parse_profile1.parse_profile.ipsec_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.sctp_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.dccp_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.other_l4_shell_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.gtp_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.esp_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.l5_shell_hxs_config = 0x0;
	verif_parse_profile1.parse_profile.final_shell_hxs_config = 0x0;
	/* Assuming no soft examination parameters */
	for(i=0; i<16; i++)
		verif_parse_profile1.parse_profile.soft_examination_param_array[i] = 0x0;
	sys_prpid_pool_create();
	/* Create the parse_profile and get an id */
	parser_profile_create(&verif_parse_profile1, prpid);
	/* Update prpid in task defaults */
	default_task_params.parser_profile_id = *prpid;
}


uint16_t aiop_verification_parser(uint32_t asa_seg_addr)
{
	uint16_t str_size = STR_SIZE_ERR;
	uint32_t opcode;

	opcode  = *((uint32_t *) asa_seg_addr);

	switch (opcode) {
	case PARSER_GEN_INIT_GROSS_STR:
	{
		struct parser_init_gross_verif_command *pc =
						(struct parser_init_gross_verif_command *)
						asa_seg_addr;
		struct parse_result *pr =
						(struct parse_result *)HWC_PARSE_RES_ADDRESS;
		fdma_calculate_default_frame_checksum(0, 0xFFFF,
				&pr->gross_running_sum);

		str_size = sizeof(struct parser_init_gross_verif_command);
		break;
	}
	case PARSER_PRP_CREATE_STR:
	{
		struct parser_prp_create_verif_command *pc =
				(struct parser_prp_create_verif_command *)
				asa_seg_addr;

		pc->status =
			parser_profile_create(
			(struct parse_profile_input *)pc->parse_profile,
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

/*		pq->status = */
		   parser_profile_query(pq->prpid,
			(struct parse_profile_record *)pq->parse_profile);

		str_size = sizeof(struct parser_prp_query_verif_command);
		break;
	}
	case PARSER_PRP_REPLACE_STR:
	{
		struct parser_prp_replace_verif_command *pq =
				(struct parser_prp_replace_verif_command *)
				asa_seg_addr;

		parser_profile_replace(
			(struct parse_profile_input *)pq->parse_profile,
			pq->prpid);

		str_size = sizeof(struct parser_prp_replace_verif_command);
		break;
	}
	case PARSER_GEN_PARSE_RES_CHECKSUM_STR:
	{
		struct parser_gen_parser_res_checksum_verif_command *gpr =
			(struct parser_gen_parser_res_checksum_verif_command *)
			asa_seg_addr;
		gpr->status = parse_result_generate_checksum(
						(enum parser_starting_hxs_code)gpr->hxs,
							gpr->offset,
							&gpr->l3_checksum,
							&gpr->l4_checksum);
		str_size = sizeof(struct parser_gen_parser_res_checksum_verif_command);
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
		pipc->status = sys_prpid_pool_create();
		str_size =
			sizeof(struct parser_prp_id_pool_create_verif_command);
		break;
	}
	case PARSER_INIT_FOR_VERIF_STR:
	{
		struct parser_init_verif_command *str =
		(struct parser_init_verif_command *) asa_seg_addr;
		default_task_params.parser_starting_hxs =
				str->parser_starting_hxs;
		aiop_init_parser(&(str->prpid));
		str_size = sizeof(struct parser_init_verif_command);
		break;
	}
	case PARSER_MACROS_STR:
	{
		struct parser_macros_command *str =
		(struct parser_macros_command *) asa_seg_addr;

		/* Next header offset */
		((struct parse_result *)str->macros_struct)->nxt_hdr = PARSER_GET_NEXT_HEADER_DEFAULT();

		/* Frame Attribute Flags Extension */
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_extension = PARSER_IS_ROUTING_HDR_IN_2ND_IPV6_HDR_DEFAULT();

		/* Frame Attribute Flags 1 */
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_SHIM_SOFT_PARSING_ERROR_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_PARSING_ERROR_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ETH_MAC_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ETH_MAC_UNICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ETH_MAC_MULTICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ETH_MAC_BROADCAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_BPDU_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_FCOE_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_FCOE_INIT_PROTOCOL_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ETH_PARSING_ERROR_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_LLC_SNAP_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_UNKNOWN_LLC_OUI_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_LLC_SNAP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ONE_VLAN_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_MORE_THAN_ONE_VLAN_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_CFI_IN_VLAN_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_VLAN_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_PPPOE_PPP_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_PPPOE_PPP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ONE_MPLS_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_MORE_THAN_ONE_MPLS_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_MPLS_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ARP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_1 |= PARSER_IS_ARP_PARSING_ERROR_DEFAULT();

		/* Frame Attribute Flags 2 */
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_L2_UNKNOWN_PROTOCOL_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_L2_SOFT_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV4_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV4_UNICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV4_MULTICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV4_BROADCAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV4_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV4_UNICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV4_MULTICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV4_BROADCAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV6_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV6_UNICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IPV6_MULTICAST_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV6_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV6_UNICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IPV6_MULTICAST_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IP_OPTIONS_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IP_UNKNOWN_PROTOCOL_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IP_FRAGMENT_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IP_INIT_FRAGMENT_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_OUTER_IP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IP_OPTIONS_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IP_UNKNOWN_PROTOCOL_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IP_FRAGMENT_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IP_INIT_FRAGMENT_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_ICMP_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_IGMP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_ICMPV6_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_UDP_LITE_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_INNER_IP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_MIN_ENCAP_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_2 |= PARSER_IS_MIN_ENCAP_S_FLAG_DEFAULT();

		/* Frame Attribute Flags 3 */
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_MIN_ENCAP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_GRE_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_GRE_R_BIT_SET_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_GRE_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_L3_UNKOWN_PROTOCOL_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_L3_SOFT_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_UDP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_UDP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_TCP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_TCP_OPTIONS_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_TCP_CONTROLS_6_11_SET_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_TCP_CONTROLS_3_5_SET_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_TCP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_IPSEC_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_IPSEC_ESP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_IPSEC_AH_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_IPSEC_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_SCTP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_SCTP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_DCCP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_DCCP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_L4_UNKOWN_PROTOCOL_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_L4_SOFT_PARSING_ERROR_DEFAULT();
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_GTP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_GTP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_ESP_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_ESP_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_ISCSI_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_CAPWAP_CONTROL_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_CAPWAP_DATA_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_L5_SOFT_PARSING_ERROR_DEFAULT() ;
		((struct parse_result *)str->macros_struct)->frame_attribute_flags_3 |= PARSER_IS_ROUTING_HDR_IN_1ST_IPV6_HDR_DEFAULT();

		/* Offsets */
		((struct parse_result *)str->macros_struct)->shim_offset_1 = PARSER_GET_SHIM1_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->shim_offset_2 = PARSER_GET_SHIM2_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->ip_pid_offset = PARSER_GET_IP_PID_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->eth_offset = PARSER_GET_ETH_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->llc_snap_offset = PARSER_GET_LLC_SNAP_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->vlan_tci1_offset = PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->vlan_tcin_offset = PARSER_GET_LAST_VLAN_TCI_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->last_etype_offset = PARSER_GET_LAST_ETYPE_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->pppoe_offset = PARSER_GET_PPPOE_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->mpls_offset_1 = PARSER_GET_FIRST_MPLS_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->mpls_offset_n = PARSER_GET_LAST_MPLS_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->ip1_or_arp_offset = PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->ip1_or_arp_offset = PARSER_GET_ARP_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->ipn_or_minencapO_offset = PARSER_GET_INNER_IP_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->ipn_or_minencapO_offset = PARSER_GET_MINENCAP_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->gre_offset = PARSER_GET_GRE_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->l4_offset = PARSER_GET_L4_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->gtp_esp_ipsec_offset = PARSER_GET_L5_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->routing_hdr_offset1 = PARSER_GET_1ST_IPV6_ROUTING_HDR_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->routing_hdr_offset2 = PARSER_GET_2ND_IPV6_ROUTING_HDR_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->nxt_hdr_offset = PARSER_GET_NEXT_HEADER_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->ipv6_frag_offset = PARSER_GET_IPV6_FRAG_HEADER_OFFSET_DEFAULT();
		((struct parse_result *)str->macros_struct)->gross_running_sum = PARSER_GET_GROSS_RUNNING_SUM_CODE_DEFAULT();
		((struct parse_result *)str->macros_struct)->running_sum = PARSER_GET_RUNNING_SUM_DEFAULT();
		((struct parse_result *)str->macros_struct)->parse_error_code = PARSER_GET_PARSE_ERROR_CODE_DEFAULT();

		str_size = sizeof(struct parser_macros_command);
		break;
	}
	default:
	{
		return STR_SIZE_ERR;
	}
	}

	return str_size;
}

