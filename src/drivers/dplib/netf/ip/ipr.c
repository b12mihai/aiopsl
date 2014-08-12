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
@File		ipr.c

@Description	This file contains the AIOP SW IP Reassembly implementation.

*//***************************************************************************/


#include "general.h"
#include "system.h"
#include "dplib/fsl_parser.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_osm.h"
#include "dplib/fsl_table.h"
#include "dplib/fsl_keygen.h"
#include "dplib/fsl_ste.h"
#include "net/fsl_net.h"
#include "kernel/fsl_spinlock.h"
#include "fdma.h"
#include "checksum.h"
#include "ipr.h"
#include "cdma.h"
#include "ip.h"
#include "fsl_platform.h"

#ifdef AIOP_VERIF
#include "slab_stub.h"
#else
#include "slab.h"
#endif

__SHRAM struct  ipr_global_parameters ipr_global_parameters1;

int ipr_init(void)
{
	struct kcr_builder kb;
	int    status;
	uint8_t  ipr_key_id;

	/* For IPv4 */
	keygen_kcr_builder_init(&kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_IPSRC_1_FECID,
			NULL , &kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_IPDST_1_FECID,
				NULL , &kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_PTYPE_1_FECID,
					NULL , &kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_IPID_1_FECID,
					NULL , &kb);
	status = keygen_kcr_create(KEYGEN_ACCEL_ID_CTLU,
			  kb.kcr,
			  &ipr_key_id);
	if (status < 0) {
		/* todo  Fatal */
	}
	ipr_global_parameters1.ipr_key_id_ipv4 = ipr_key_id;
	/* For IPv6 */
	keygen_kcr_builder_init(&kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_IPSRC_1_FECID,
			NULL , &kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_IPDST_1_FECID,
				NULL , &kb);
	keygen_kcr_builder_add_protocol_specific_field(KEYGEN_KCR_IPID_1_FECID,
					NULL , &kb);
	keygen_kcr_create(KEYGEN_ACCEL_ID_CTLU,
			  kb.kcr,
			  &ipr_key_id);
	ipr_global_parameters1.ipr_key_id_ipv6 = ipr_key_id;

	if (status < 0) {
		/* todo  Fatal */
	}
	return 0;
}

int ipr_create_instance(struct ipr_params *ipr_params_ptr,
			    ipr_instance_handle_t *ipr_instance_ptr)
{
	struct ipr_instance	      ipr_instance;
	struct ipr_instance_extension ipr_instance_ext;
	struct table_create_params tbl_params;
	int32_t err;
	uint32_t max_open_frames, aggregate_open_frames, table_location;
	uint16_t table_location_attr;
	uint16_t bpid;
	int table_ipv4_valid = 0;
	int num_filled_buffs, status;

	aggregate_open_frames = ipr_params_ptr->max_open_frames_ipv4 +
				ipr_params_ptr->max_open_frames_ipv6 + 1;
	/* call ARENA function for allocating buffers needed to IPR
	 * processing (create_slab ) */
	status = slab_find_and_reserve_bpid(aggregate_open_frames,
					IPR_CONTEXT_SIZE,
					8,
					MEM_PART_DP_DDR,
					&num_filled_buffs,
					&bpid);

	if (status < 0)
		return status;

	if(num_filled_buffs != aggregate_open_frames)
		return IPR_MAX_BUFFERS_REACHED;

	err = cdma_acquire_context_memory(bpid,
					  ipr_instance_ptr);
	if (err)
		return err;

	ipr_instance.bpid = bpid;
	ipr_instance.flags = ipr_params_ptr->flags;

	/* For IPv4 */
	max_open_frames = ipr_params_ptr->max_open_frames_ipv4;
	/* Initialize instance parameters */
	ipr_instance.table_id_ipv4 = 0;
	ipr_instance.table_id_ipv6 = 0;
	if (max_open_frames) {
		ipr_instance.flags |= IPV4_VALID;
		ipr_instance_ext.max_open_frames_ipv4 = max_open_frames;
		tbl_params.committed_rules = max_open_frames;
		tbl_params.max_rules = max_open_frames;
		/* IPv4 src, IPv4 dst, prot, ID */
		tbl_params.key_size = 11;
		table_location = ipr_params_ptr->flags & 0x0C000000;
		if (table_location == IPR_MODE_TABLE_LOCATION_INT)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_INT;
		else if (table_location == IPR_MODE_TABLE_LOCATION_PEB)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_PEB;
		else if (table_location == IPR_MODE_TABLE_LOCATION_EXT1)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_EXT1;
		else if (table_location == IPR_MODE_TABLE_LOCATION_EXT2)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_EXT2;
		tbl_params.attributes = TABLE_ATTRIBUTE_TYPE_EM | \
				table_location_attr | \
				TABLE_ATTRIBUTE_MR_NO_MISS;
		err = table_create(TABLE_ACCEL_ID_CTLU, &tbl_params,
				&ipr_instance.table_id_ipv4);
		if (err != TABLE_STATUS_SUCCESS) {
			/* todo SR error case */
			cdma_release_context_memory(*ipr_instance_ptr);
			return err;
		}
		table_ipv4_valid = 1;
	}
	/* For IPv6 */
	max_open_frames = ipr_params_ptr->max_open_frames_ipv6;
	if (max_open_frames) {
		ipr_instance.flags |= IPV6_VALID;
		ipr_instance_ext.max_open_frames_ipv6 = max_open_frames;
		tbl_params.committed_rules = max_open_frames;
		tbl_params.max_rules = max_open_frames;
		/* IPv6 src, IPv6 dst, ID (4 bytes) */
		tbl_params.key_size = 36;
		table_location = ipr_params_ptr->flags & 0x0C000000;
		if (table_location == IPR_MODE_TABLE_LOCATION_INT)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_INT;
		else if (table_location == IPR_MODE_TABLE_LOCATION_PEB)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_PEB;
		else if (table_location == IPR_MODE_TABLE_LOCATION_EXT1)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_EXT1;
		else if (table_location == IPR_MODE_TABLE_LOCATION_EXT2)
			table_location_attr = TABLE_ATTRIBUTE_LOCATION_EXT2;
		tbl_params.attributes = TABLE_ATTRIBUTE_TYPE_EM | \
				table_location_attr | \
				TABLE_ATTRIBUTE_MR_NO_MISS;
		err = table_create(TABLE_ACCEL_ID_CTLU, &tbl_params,
				&ipr_instance.table_id_ipv6);
		if (err != TABLE_STATUS_SUCCESS) {
			/* todo SR error case */
			cdma_release_context_memory(*ipr_instance_ptr);
			if (table_ipv4_valid)
				table_delete(TABLE_ACCEL_ID_CTLU,
					     ipr_instance.table_id_ipv4);
			return err;
		}
	}

	/* Initialize instance parameters */
	ipr_instance.extended_stats_addr = ipr_params_ptr->extended_stats_addr;
	ipr_instance.max_reass_frm_size = ipr_params_ptr->max_reass_frm_size;
	ipr_instance.min_frag_size_ipv4 = ipr_params_ptr->min_frag_size_ipv4;
	ipr_instance.min_frag_size_ipv6 = ipr_params_ptr->min_frag_size_ipv6;
	ipr_instance.timeout_value_ipv4 = ipr_params_ptr->timeout_value_ipv4;
	ipr_instance.timeout_value_ipv6 = ipr_params_ptr->timeout_value_ipv6;
	ipr_instance.ipv4_timeout_cb = ipr_params_ptr->ipv4_timeout_cb;
	ipr_instance.ipv6_timeout_cb = ipr_params_ptr->ipv6_timeout_cb;
	ipr_instance.cb_timeout_ipv4_arg = ipr_params_ptr->cb_timeout_ipv4_arg;
	ipr_instance.cb_timeout_ipv6_arg = ipr_params_ptr->cb_timeout_ipv6_arg;
	ipr_instance.tmi_id = ipr_params_ptr->tmi_id;

	/* Write ipr instance data structure */
	cdma_write(*ipr_instance_ptr, &ipr_instance, IPR_INSTANCE_SIZE);

	/* Initialization of ipr instance extension parameters */
	ipr_instance_ext.confirm_delete_cb = 0;
	ipr_instance_ext.delete_arg	   = 0;
	ipr_instance_ext.num_of_open_reass_frames_ipv4 = 0;
	ipr_instance_ext.num_of_open_reass_frames_ipv6 = 0;
	ipr_instance_ext.ipv4_reass_frm_cntr = 0;
	ipr_instance_ext.ipv6_reass_frm_cntr = 0;

	/* Write ipr instance extension data structure */
	cdma_write((*ipr_instance_ptr)+sizeof(struct ipr_instance),
		   &ipr_instance_ext,
		   sizeof(struct ipr_instance_extension));

	return IPR_CREATE_INSTANCE_SUCCESS;
}

int ipr_delete_instance(ipr_instance_handle_t ipr_instance_ptr,
			    ipr_del_cb_t *confirm_delete_cb,
			    ipr_del_arg_t delete_arg)
{
	struct ipr_instance	  ipr_instance;

	/* todo callback function */
	UNUSED(confirm_delete_cb);
	UNUSED(delete_arg);

	ste_barrier();

	cdma_read(&ipr_instance, ipr_instance_ptr, IPR_INSTANCE_SIZE);

	/* todo
	ipr_instance_extension_ptr = ((uint64_t)ipr_instance_ptr) +
				     sizeof(struct ipr_instance);
	cdma_read(&ipr_instance_ext,
		  ipr_instance_extension_ptr,
		  IPR_INSTANCE_SIZE);
	*/

	/* todo SR error case */
	cdma_release_context_memory(ipr_instance_ptr);
	/* error case */
	if (ipr_instance.flags & IPV4_VALID)
		table_delete(TABLE_ACCEL_ID_CTLU, ipr_instance.table_id_ipv4);
	if (ipr_instance.flags & IPV6_VALID)
		table_delete(TABLE_ACCEL_ID_CTLU, ipr_instance.table_id_ipv6);

	return SUCCESS;
}

int ipr_reassemble(ipr_instance_handle_t instance_handle)
{
	uint16_t iphdr_offset;
	uint32_t status_insert_to_LL;
	uint32_t osm_status;
	uint32_t frame_is_ipv4;
	uint64_t rfdc_ext_addr;
	int32_t  sr_status;
	uint32_t status;
	uint16_t timeout_value;
	uint8_t	 keysize;
	uint8_t  ipv6_key[36];
	void	*iphdr_ptr;
	struct ipr_rfdc rfdc;
	struct ipr_instance instance_params;
	struct scope_status_params scope_status;
	struct table_lookup_result lookup_result;
	/* todo rule should be aligned to 16 bytes */
	struct table_rule rule;
	struct	presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;

	iphdr_offset = (uint16_t)PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	iphdr_ptr = (void *)(iphdr_offset + PRC_GET_SEGMENT_ADDRESS());

	/* Get OSM status (ordering scope mode and levels) */
	osm_get_scope(&scope_status);

	if (scope_status.scope_mode == EXCLUSIVE) {
		if (PARSER_IS_OUTER_IP_FRAGMENT_DEFAULT()) {
			osm_status = BYPASS_OSM;
		} else {
			/* regular frame */
			return IPR_REASSEMBLY_REGULAR;
		}
	} else {
	    /* move to exclusive */
	    osm_scope_transition_to_exclusive_with_increment_scope_id();
	    if (PARSER_IS_OUTER_IP_FRAGMENT_DEFAULT()) {
		/* Fragment */
		    if (scope_status.scope_level <= 2) {
			/* create nested exclusive for the fragments of
			 * the same flow*/
			 osm_scope_enter_to_exclusive_with_increment_scope_id();
		} else {
			/* can't create 2 nested */
			osm_status = BYPASS_OSM | START_CONCURRENT;
			}
		} else {
			/* regular frame */
			osm_scope_relinquish_exclusivity();
			return IPR_REASSEMBLY_REGULAR;
		}
	}

	/* read instance parameters */
	cdma_read(&instance_params,
		  instance_handle,
		  IPR_INSTANCE_SIZE);

	if (check_for_frag_error() == NO_ERROR) {
		/* Good fragment */
		if (PARSER_IS_OUTER_IPV4_DEFAULT())
			frame_is_ipv4 = 1;
		else
			frame_is_ipv4 = 0;
		if (frame_is_ipv4) {
			sr_status = table_lookup_by_keyid_default_frame(
				TABLE_ACCEL_ID_CTLU,
				instance_params.table_id_ipv4,
				ipr_global_parameters1.ipr_key_id_ipv4,
				&lookup_result);
		} else {
			sr_status = table_lookup_by_keyid_default_frame(
					TABLE_ACCEL_ID_CTLU,
					instance_params.table_id_ipv6,
					ipr_global_parameters1.ipr_key_id_ipv6,
					&lookup_result);
		}
		if (sr_status == TABLE_STATUS_SUCCESS) {
			/* Hit */
			rfdc_ext_addr = lookup_result.opaque0_or_reference;
			if (osm_status == NO_BYPASS_OSM) {
				/* create nested per reassembled frame */
				osm_scope_enter_to_exclusive_with_new_scope_id(
						  (uint32_t)rfdc_ext_addr);
			}
			/* read and lock RFDC */
			cdma_read_with_mutex(rfdc_ext_addr,
					  CDMA_PREDMA_MUTEX_WRITE_LOCK,
					  &rfdc,
					  RFDC_SIZE);

			if (!(rfdc.status & RFDC_VALID)) {
				move_to_correct_ordering_scope2(
							   osm_status);
				/* CDMA write, unlock, dec ref_cnt and release
				 * if ref_cnt=0 */
				cdma_access_context_memory(
				rfdc_ext_addr,
				CDMA_ACCESS_CONTEXT_MEM_DEC_REFCOUNT_AND_REL |
				CDMA_ACCESS_CONTEXT_MEM_RM_BIT,
				NULL,
				&rfdc,
				CDMA_ACCESS_CONTEXT_MEM_DMA_WRITE
				| RFDC_SIZE,
				(uint32_t *)REF_COUNT_ADDR_DUMMY);

			   /* Early Time out */
			   return IPR_ERROR;
			}
		} else if (sr_status == TABLE_STATUS_MISS) {
			/* Miss */
			cdma_acquire_context_memory(
					instance_params.bpid,
					&rfdc_ext_addr);
			/* Lock RFDC + increment reference count*/
			cdma_access_context_memory(
				rfdc_ext_addr,
				CDMA_ACCESS_CONTEXT_MEM_MUTEX_WRITE_LOCK |
				CDMA_ACCESS_CONTEXT_MEM_INC_REFCOUNT,
				0,
				(void *)0,
				0,
				(uint32_t *)REF_COUNT_ADDR_DUMMY);

			/* Reset RFDC + Link List */
			/*cdma_ws_memory_init((void *)&rfdc,
					SIZE_TO_INIT,
					0);  */
			/* Add entry to TLU table */
			/* Generate key */
			rule.options = 0;
			rule.result.type = TABLE_RESULT_TYPE_REFERENCE;
			rule.result.op0_rptr_clp.reference_pointer =
								 rfdc_ext_addr;

			if (frame_is_ipv4) {
				keygen_gen_key(
					KEYGEN_ACCEL_ID_CTLU,
					ipr_global_parameters1.ipr_key_id_ipv4,
					0,
					&rule.key_desc,
					&keysize);
				table_rule_create(TABLE_ACCEL_ID_CTLU,
				      instance_params.table_id_ipv4,
				      &rule,
				      keysize);
				/* store key in RDFC */
				rfdc.ipv4_key[0] =
					      *(uint64_t *)rule.key_desc.em.key;
				rfdc.ipv4_key[1] =
					  *(uint64_t *)(rule.key_desc.em.key+8);

				/* Increment no of IPv4 open frames in instance
					data structure */
				ste_inc_counter(
					 instance_handle+
					 sizeof(struct ipr_instance)+
					 offsetof(struct ipr_instance_extension,
					 num_of_open_reass_frames_ipv4),
					 1,
					 STE_MODE_32_BIT_CNTR_SIZE);

				rfdc.status = RFDC_VALID | IPV4_FRAME;

			} else {
			    /* IPv6 */
			    keygen_gen_key(
					 KEYGEN_ACCEL_ID_CTLU,
					 ipr_global_parameters1.ipr_key_id_ipv6,
					 0,
					 &rule.key_desc,
					 &keysize);
			    table_rule_create(TABLE_ACCEL_ID_CTLU,
					      instance_params.table_id_ipv6,
					      &rule,
					      keysize);
			    /* write key in RDFC Extension */
			    cdma_write(rfdc_ext_addr+RFDC_SIZE+
					    offsetof(struct extended_ipr_rfdc,
					    ipv6_key),
					&rule.key_desc.em.key,
					RFDC_EXTENSION_TRUNCATED_SIZE);

			    /* Increment no of IPv6 open frames in instance
			       data structure */
			     ste_inc_counter(
					 instance_handle+
					 sizeof(struct ipr_instance)+
					 offsetof(struct ipr_instance_extension,
					 num_of_open_reass_frames_ipv6),
					 1,
					 STE_MODE_32_BIT_CNTR_SIZE);

				rfdc.status = RFDC_VALID | IPV6_FRAME;

		    }

		    /* todo release struct rule  or call function for
		     * gen+add rule */
			rfdc.instance_handle		= instance_handle;
			rfdc.expected_total_length	= 0;
			rfdc.index_to_out_of_order	= 0;
			rfdc.next_index			= 0;
			rfdc.current_total_length	= 0;
			rfdc.first_frag_index		= 0;
			rfdc.num_of_frags		= 0;
			/* todo check if necessary */
			rfdc.biggest_payload		= 0;
			rfdc.current_running_sum	= 0;
			rfdc.last_frag_index		= 0;
			rfdc.total_in_order_payload	= 0;
			get_default_amq_attributes(&rfdc.isolation_bits);

		    /* create Timer in TMAN */

		    if(frame_is_ipv4)
			    timeout_value = instance_params.timeout_value_ipv4;
		    else
			    timeout_value = instance_params.timeout_value_ipv6;

/*		    tman_create_timer(instance_params.tmi_id,
				      IPR_TIMEOUT_FLAGS,
				      timeout_value,
				      (tman_arg_8B_t) rfdc_ext_addr,
				      (tman_arg_2B_t) NULL,
				      (tman_cb_t) ipr_time_out,
				      &rfdc.timer_handle);
*/
		     if (osm_status == NO_BYPASS_OSM) {
			/* create nested per reassembled frame */
			osm_scope_enter_to_exclusive_with_new_scope_id(
					       (uint32_t)rfdc_ext_addr);
			}

	} else {
		/* TLU lookup SR error */
		return IPR_ERROR;
	}

	status_insert_to_LL = ipr_insert_to_link_list(&rfdc, rfdc_ext_addr,
						      iphdr_ptr, frame_is_ipv4);
	switch (status_insert_to_LL) {
	case FRAG_OK_REASS_NOT_COMPL:
		move_to_correct_ordering_scope2(osm_status);
		if(frame_is_ipv4) {
			if (instance_params.flags & IPR_MODE_IPV4_TO_TYPE) {
			/* recharge timer in case of time out
			 * between fragments */
//				tman_recharge_timer(rfdc.timer_handle);
			}
		} else if (instance_params.flags & IPR_MODE_IPV6_TO_TYPE) {
			/* recharge timer in case of time out
			 * between fragments */
//				tman_recharge_timer(rfdc.timer_handle);
		}
		/* Write and release updated 64 first bytes
		 * of RFDC */
		cdma_write_release_lock_and_decrement(
				       rfdc_ext_addr,
				       &rfdc,
				       RFDC_SIZE);
		return IPR_REASSEMBLY_NOT_COMPLETED;
	case LAST_FRAG_IN_ORDER:
		closing_in_order(rfdc_ext_addr, rfdc.num_of_frags);
		break;
	case LAST_FRAG_OUT_OF_ORDER:
		closing_with_reordering(&rfdc, rfdc_ext_addr);
		break;
	case FRAG_ERROR:
		/* duplicate or overlap fragment */
		return IPR_MALFORMED_FRAG;
		break;
	}
	/* Only successfully reassembled frames continue
	   from here */
	/* default frame is now the full reassembled frame */

	if (frame_is_ipv4) {
		table_rule_delete(TABLE_ACCEL_ID_CTLU,
				  instance_params.table_id_ipv4,
				  (union table_key_desc *)&rfdc.ipv4_key,
				  IPV4_KEY_SIZE,
				  NULL);
	} else {
		cdma_read(&ipv6_key,
			  rfdc_ext_addr+RFDC_SIZE+
				  offsetof(struct extended_ipr_rfdc,ipv6_key),
			  IPV6_KEY_SIZE);
		table_rule_delete(TABLE_ACCEL_ID_CTLU,
				  instance_params.table_id_ipv6,
				  (union table_key_desc *)&ipv6_key,
				  IPV6_KEY_SIZE,
				  NULL);
	}

	/* Open segment for reassembled frame */
	/* Retrieve original seg length,seg addr and seg offset from RFDC */
	prc->seg_address = rfdc.seg_addr;
	prc->seg_length  = rfdc.seg_length;
	prc->seg_offset  = rfdc.seg_offset;
	fdma_present_default_frame_default_segment();
	/* FD length is still not updated */

	/* Parser is not re-run here, and iphdr offset will be retrieved
	   from RFDC*/

	if (frame_is_ipv4)
		status = ipv4_header_update_and_l4_validation(&rfdc);
	else
		status = ipv6_header_update_and_l4_validation(&rfdc);

	/* Write and release updated 64 first bytes of RFDC */
	/* CDMA write, unlock, dec ref_cnt and release if
	 * ref_cnt=0 */
	cdma_access_context_memory(
				  rfdc_ext_addr,
				  CDMA_ACCESS_CONTEXT_MEM_DEC_REFCOUNT_AND_REL |
				  CDMA_ACCESS_CONTEXT_MEM_RM_BIT,
				  NULL,
				  &rfdc,
				  CDMA_ACCESS_CONTEXT_MEM_DMA_WRITE |
				  RFDC_SIZE,
				  (uint32_t *)REF_COUNT_ADDR_DUMMY);


	move_to_correct_ordering_scope2(osm_status);

	if (frame_is_ipv4) {
		/* Decrement no of IPv4 open frames in instance data structure*/
		ste_dec_counter(instance_handle + sizeof(struct ipr_instance)+
					offsetof(struct ipr_instance_extension,
					num_of_open_reass_frames_ipv4),
				1,
				STE_MODE_32_BIT_CNTR_SIZE);
		if (status == SUCCESS) {
			/* L4 checksum is valid */
			/* Increment no of frames in instance data structure */
			ste_inc_counter(instance_handle +
					sizeof(struct ipr_instance)+
					offsetof(struct ipr_instance_extension,
					ipv4_reass_frm_cntr),
					1,
					STE_MODE_32_BIT_CNTR_SIZE);
			return IPR_REASSEMBLY_SUCCESS;
		}
	} else { /* IPv6 */
		/* Decrement no of IPv6 open frames in instance data structure*/
		ste_dec_counter(instance_handle + sizeof(struct ipr_instance)+
					offsetof(struct ipr_instance_extension,
					num_of_open_reass_frames_ipv6),
				1,
				STE_MODE_32_BIT_CNTR_SIZE);
		if (status == SUCCESS) {
			ste_inc_counter(instance_handle +
					sizeof(struct ipr_instance)+
					offsetof(struct ipr_instance_extension,
					ipv6_reass_frm_cntr),
					1,
					STE_MODE_32_BIT_CNTR_SIZE);

			return IPR_REASSEMBLY_SUCCESS;
		}
	}

	/* L4 checksum is not valid */
	return IPR_ERROR;

	} else {
		/* Error fragment */
		move_to_correct_ordering_scope1(osm_status);
		return IPR_MALFORMED_FRAG;
	}
}

uint32_t ipr_insert_to_link_list(struct ipr_rfdc *rfdc_ptr,
				 uint64_t rfdc_ext_addr,
				 void *iphdr_ptr,
				 uint32_t frame_is_ipv4)
{

	uint8_t			current_index;
	uint16_t		frag_offset_shifted;
	uint16_t		current_frag_size;
	uint16_t		expected_frag_offset;
	uint16_t		ip_header_size;
	uint16_t		ipv6fraghdr_offset;
	uint32_t		last_fragment;
	uint64_t		ext_addr;
	/* todo reuse ext_addr for current_element_ext_addr */
	uint64_t		current_element_ext_addr;
	struct link_list_element	current_element;
	struct ipv4hdr			*ipv4hdr_ptr;
	struct ipv6hdr			*ipv6hdr_ptr;
	struct ipv6fraghdr		*ipv6fraghdr_ptr;
	struct	parse_result	*pr =
				  (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct	presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;

	if (frame_is_ipv4) {
		ipv4hdr_ptr = (struct ipv4hdr *) iphdr_ptr;
		frag_offset_shifted =
		    (ipv4hdr_ptr->flags_and_offset & FRAG_OFFSET_IPV4_MASK)<<3;

		ip_header_size = (uint16_t)
			  ((ipv4hdr_ptr->vsn_and_ihl & IPV4_HDR_IHL_MASK)<<2);
		current_frag_size = ipv4hdr_ptr->total_length - ip_header_size;
		last_fragment = !(ipv4hdr_ptr->flags_and_offset &
				IPV4_HDR_M_FLAG_MASK);
	} else {
		ipv6hdr_ptr = (struct ipv6hdr *) iphdr_ptr;
		ipv6fraghdr_offset =
				PARSER_GET_IPV6_FRAG_HEADER_OFFSET_DEFAULT();
		ipv6fraghdr_ptr = (struct ipv6fraghdr *)
			       (PRC_GET_SEGMENT_ADDRESS() + ipv6fraghdr_offset);
		frag_offset_shifted = ipv6fraghdr_ptr->offset_and_flags &
							  FRAG_OFFSET_IPV6_MASK;

		ip_header_size = ((uint16_t)((uint32_t)ipv6fraghdr_ptr -
						(uint32_t)ipv6hdr_ptr)) + 8;
		current_frag_size = ipv6hdr_ptr->payload_length -
				ip_header_size + IPV6_FIXED_HEADER_SIZE;
		last_fragment = !(ipv6fraghdr_ptr->offset_and_flags &
				IPV6_HDR_M_FLAG_MASK);
	}

	if (frag_offset_shifted != 0) {
		/* Not first frag */
		/* Save header to be removed in FD[FRC] */
		((struct ldpaa_fd *)HWC_FD_ADDRESS)->frc =
			    (uint32_t) (PARSER_GET_OUTER_IP_OFFSET_DEFAULT() +
					    ip_header_size);

		/* Add current frag's running sum for
		 * L4 checksum check */
		if (pr->gross_running_sum == 0) {
			fdma_calculate_default_frame_checksum(
							0,
							0xffff,
						        &pr->gross_running_sum);
			parse_result_generate_default(0);
		}

		rfdc_ptr->current_running_sum = cksum_ones_complement_sum16(
						  rfdc_ptr->current_running_sum,
						  pr->running_sum);
	} else {
		/* First fragment (frag_offset == 0) */
		rfdc_ptr->status |= FIRST_ARRIVED;
		/* Save PRC params for presentation of the reassembled frame */
		rfdc_ptr->seg_addr   = prc->seg_address;
		rfdc_ptr->seg_length = prc->seg_length;
		rfdc_ptr->seg_offset = prc->seg_offset;

		if (pr->gross_running_sum == 0)
			fdma_calculate_default_frame_checksum(
							0,
							0xffff,
							&pr->gross_running_sum);
		/* Set 1rst frag's running sum for L4 checksum check */
		rfdc_ptr->current_running_sum = cksum_ones_complement_sum16(
				  	          rfdc_ptr->current_running_sum,
				  	          pr->gross_running_sum);
		/* Get IP offset */
		rfdc_ptr->iphdr_offset = PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
		if (!frame_is_ipv4)
			rfdc_ptr->ipv6_fraghdr_offset = ipv6fraghdr_offset;
	}

	if (!(rfdc_ptr->status & OUT_OF_ORDER)) {
		/* In order handling */
		expected_frag_offset = rfdc_ptr->current_total_length;
		if (frag_offset_shifted == expected_frag_offset) {

			rfdc_ptr->num_of_frags++;
			rfdc_ptr->current_total_length += current_frag_size;

			if (frame_is_ipv4)
				check_remove_padding();
			/* Close current frame before storing FD */
			fdma_store_default_frame_data();

			/* Write FD in external buffer */
			ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
						rfdc_ptr->next_index*FD_SIZE;
			cdma_write(ext_addr,
				   (void *)HWC_FD_ADDRESS,
				   FD_SIZE);

			if (last_fragment) {
				return LAST_FRAG_IN_ORDER;
			} else {
				/* Non closing fragment */
				rfdc_ptr->next_index++;
				return FRAG_OK_REASS_NOT_COMPL;
				}
		} else if (frag_offset_shifted < expected_frag_offset) {
				/* Malformed Error */
				return FRAG_ERROR;
		} else {
			/* New out of order */
			current_index = rfdc_ptr->next_index;
			if (current_index != 0) {
				rfdc_ptr->status |= OUT_OF_ORDER |
						  ORDER_AND_OOO;
				rfdc_ptr->index_to_out_of_order = current_index;
				rfdc_ptr->first_frag_index = current_index;
				rfdc_ptr->total_in_order_payload =
						rfdc_ptr->current_total_length;
		} else {
			rfdc_ptr->status |= OUT_OF_ORDER;
		}
		rfdc_ptr->last_frag_index  = current_index;
		rfdc_ptr->num_of_frags++;
		rfdc_ptr->current_total_length +=
					    current_frag_size;
		rfdc_ptr->next_index = current_index + 1;
		rfdc_ptr->biggest_payload = frag_offset_shifted +
					 current_frag_size;

		if (last_fragment)
			rfdc_ptr->expected_total_length = frag_offset_shifted +
							  current_frag_size;
		current_element.next_index  = 0;
		current_element.prev_index  = 0;
		current_element.frag_offset = frag_offset_shifted;
		current_element.frag_length = current_frag_size;
		/* Write my element of link list */
		current_element_ext_addr = rfdc_ext_addr +
			START_OF_LINK_LIST +
			current_index*LINK_LIST_ELEMENT_SIZE;
		cdma_write(current_element_ext_addr,
				   (void *)&current_element,
				   LINK_LIST_ELEMENT_SIZE);

		if (frame_is_ipv4)
			check_remove_padding();
		/* Close current frame before storing FD */
		fdma_store_default_frame_data();

		/* Write FD in external buffer */
		ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
			   current_index*FD_SIZE;
		cdma_write(ext_addr,
			       (void *)HWC_FD_ADDRESS,
			       FD_SIZE);

		return FRAG_OK_REASS_NOT_COMPL;
	    }
		} else {
			/* Out of order handling */
			return out_of_order(rfdc_ptr, rfdc_ext_addr,
					last_fragment, current_frag_size,
					frag_offset_shifted);
	}
	/* to be removed */
	return SUCCESS;
}

uint32_t closing_in_order(uint64_t rfdc_ext_addr, uint8_t num_of_frags)
{
	struct		ldpaa_fd fds_to_concatenate[2] \
			     __attribute__((aligned(sizeof(struct ldpaa_fd))));
	uint64_t	fds_to_fetch_addr;
	struct		fdma_concatenate_frames_params concatenate_params;

	/* Bring into workspace 2 FDs to be concatenated */
	fds_to_fetch_addr = rfdc_ext_addr + START_OF_FDS_LIST;
	cdma_read((void *)fds_to_concatenate,
			  fds_to_fetch_addr,
			  FD_SIZE*2);
	/* Copy 1rst FD to default frame FD's place */
	*((struct ldpaa_fd *)(HWC_FD_ADDRESS)) = fds_to_concatenate[0];

	/* Open 1rst frame and get frame handle */
	fdma_present_default_frame_without_segments();

	/* Open 2nd frame and get frame handle */
	/* reset frame2 field because handle is 2 bytes in concatenate
	   vs 1 byte in present*/
	concatenate_params.frame2 = 0;
	fdma_present_frame_without_segments(
		   fds_to_concatenate+1,
		   FDMA_INIT_NO_FLAGS,
		   0,
		   (uint8_t *)(&(concatenate_params.frame2)) + sizeof(uint8_t));

	concatenate_params.flags  = FDMA_CONCAT_SF_BIT;
	concatenate_params.spid   = *((uint8_t *) HWC_SPID_ADDRESS);
	concatenate_params.frame1 = (uint16_t) PRC_GET_FRAME_HANDLE();
	/* Take header size to be removed from 2nd FD[FRC] */
	concatenate_params.trim   = (uint8_t)fds_to_concatenate[1].frc;

	fdma_concatenate_frames(&concatenate_params);

	num_of_frags -= 2;
	while (num_of_frags >= 2) {
		/* Bring into workspace 2 FDs to be concatenated */
		fds_to_fetch_addr += 2*FD_SIZE;
		cdma_read((void *)fds_to_concatenate,
			  fds_to_fetch_addr,
			  2*FD_SIZE);
		/* Open frame and get frame handle */
		/* reset frame2 field because handle is 2 bytes in concatenate
		   vs 1 byte in present*/
		concatenate_params.frame2 = 0;
		fdma_present_frame_without_segments(
				fds_to_concatenate,
				FDMA_INIT_NO_FLAGS,
				0,
				(uint8_t *)(&(concatenate_params.frame2)) +
				sizeof(uint8_t));

		/* Take header size to be removed from FD[FRC] */
		concatenate_params.trim  = (uint8_t)fds_to_concatenate[0].frc;

		fdma_concatenate_frames(&concatenate_params);

		/* Open frame and get frame handle */
		fdma_present_frame_without_segments(
				fds_to_concatenate+1,
				FDMA_INIT_NO_FLAGS,
				0,
				(uint8_t *)(&(concatenate_params.frame2)) +
				sizeof(uint8_t));

		/* Take header size to be removed from FD[FRC] */
		concatenate_params.trim  = (uint8_t)fds_to_concatenate[1].frc;

		fdma_concatenate_frames(&concatenate_params);

		num_of_frags -= 2;
	}
	if (num_of_frags == 1) {
		/* Handle last even fragment */
		/* Bring into workspace last FD to be concatenated */
		fds_to_fetch_addr += 2*FD_SIZE;
		cdma_read((void *)fds_to_concatenate,
			  fds_to_fetch_addr,
			  FD_SIZE);

		/* reset frame2 field because handle is 2 bytes in concatenate
		   vs 1 byte in present*/
		concatenate_params.frame2 = 0;

		fdma_present_frame_without_segments(
				fds_to_concatenate,
				FDMA_INIT_NO_FLAGS,
				0,
				(uint8_t *)(&(concatenate_params.frame2)) +
				sizeof(uint8_t));

		/* Take header size to be removed from FD[FRC] */
		concatenate_params.trim = (uint8_t)fds_to_concatenate[0].frc;

		fdma_concatenate_frames(&concatenate_params);

	}
	return SUCCESS;
}

uint32_t ipv4_header_update_and_l4_validation(struct ipr_rfdc *rfdc_ptr)
{
	uint16_t	ipv4hdr_offset;
	uint16_t	new_total_length;
	uint16_t	ip_hdr_cksum;
	uint16_t	old_ip_checksum;
	uint16_t	new_flags_and_offset;
	uint16_t	ip_header_size;
	struct ipv4hdr  *ipv4hdr_ptr;
	struct parse_result *pr;
	struct	presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;

	ipv4hdr_offset = rfdc_ptr->iphdr_offset;
	ipv4hdr_ptr = (struct ipv4hdr *)
			(ipv4hdr_offset + PRC_GET_SEGMENT_ADDRESS());

	/* update IP checksum */
	ip_header_size = (uint16_t)
			((ipv4hdr_ptr->vsn_and_ihl & IPV4_HDR_IHL_MASK)<<2);
	new_total_length = rfdc_ptr->current_total_length + ip_header_size;
	old_ip_checksum = ipv4hdr_ptr->hdr_cksum;
	ip_hdr_cksum = old_ip_checksum;
	ip_hdr_cksum = cksum_accumulative_update_uint32(ip_hdr_cksum,
					 ipv4hdr_ptr->total_length,
					 new_total_length);
	ipv4hdr_ptr->total_length = new_total_length;

	new_flags_and_offset = ipv4hdr_ptr->flags_and_offset & RESET_MF_BIT;
	ip_hdr_cksum = cksum_accumulative_update_uint32(
				 ip_hdr_cksum,
				 ipv4hdr_ptr->flags_and_offset,
				 new_flags_and_offset);
	ipv4hdr_ptr->flags_and_offset = new_flags_and_offset;

	ipv4hdr_ptr->hdr_cksum = ip_hdr_cksum;

	/* update FDMA with total length and IP header checksum*/
	fdma_modify_default_segment_data(ipv4hdr_offset+2, 10);

	/* Updated FD[length] */
	LDPAA_FD_SET_LENGTH(HWC_FD_ADDRESS, new_total_length + ipv4hdr_offset+
					   prc->seg_offset);

	/* Update Gross running sum of the reassembled frame */
	pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	pr->gross_running_sum = rfdc_ptr->current_running_sum;

	/* Run Parser and check L4 checksum if needed */
	if (parse_result_generate_default(PARSER_VALIDATE_L4_CHECKSUM)){
		/* error in L4 checksum */
		return IPR_ERROR;
	}
	return SUCCESS;
}

uint32_t ipv6_header_update_and_l4_validation(struct ipr_rfdc *rfdc_ptr)
{
	uint16_t		ipv6hdr_offset;
	uint16_t		ipv6fraghdr_offset;
	uint16_t		ipv6_frag_extension_size;
	uint16_t		checksum;
	uint16_t		gross_running_sum;
	uint16_t		size;
	uint16_t		ipv6_payload_length;
	struct ipv6hdr		*ipv6hdr_ptr;
	struct ipv6fraghdr	*ipv6fraghdr_ptr;
	struct	parse_result 	*pr =
				   (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct	presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;

	ipv6hdr_offset = rfdc_ptr->iphdr_offset;
	ipv6hdr_ptr = (struct ipv6hdr *)
				(ipv6hdr_offset + PRC_GET_SEGMENT_ADDRESS());

	ipv6fraghdr_offset = rfdc_ptr->ipv6_fraghdr_offset;

	ipv6fraghdr_ptr = (struct ipv6fraghdr *) (PRC_GET_SEGMENT_ADDRESS() +
						ipv6fraghdr_offset);

	ipv6_frag_extension_size = (uint16_t)((uint32_t)ipv6fraghdr_ptr -
				(uint32_t)ipv6hdr_ptr) - IPV6_FIXED_HEADER_SIZE;

	fdma_calculate_default_frame_checksum(
				ipv6hdr_offset,
				ipv6fraghdr_offset-ipv6hdr_offset+8,
				&checksum);

	ipv6_payload_length = rfdc_ptr->current_total_length +
				ipv6_frag_extension_size;
	ipv6hdr_ptr->payload_length = ipv6_payload_length;
	/* Move next header of fragment header to previous extension header */
	*(uint8_t *)(ipv6_last_header(ipv6hdr_ptr, LAST_HEADER_BEFORE_FRAG) &
			0x7FFFFFFF) = ipv6fraghdr_ptr->next_header;

	/* Remove fragment header extension and update FDMA cache */
	size = ipv6fraghdr_offset-ipv6hdr_offset;

	fdma_replace_default_segment_data(ipv6hdr_offset,
					  size+8,
					  (void *)ipv6hdr_ptr,
					  size,
					  (void *)prc->seg_address,
					  prc->seg_length,
					  FDMA_REPLACE_SA_REPRESENT_BIT);

	/* Updated FD[length] */
	LDPAA_FD_SET_LENGTH(HWC_FD_ADDRESS, ipv6_payload_length +
					    ipv6fraghdr_offset +
					    prc->seg_offset);

	/* Update Gross running sum of the reassembled frame */
	/* Subtract old IPv6 header (fixed until frag header) */
	gross_running_sum = cksum_ones_complement_sum16(
					rfdc_ptr->current_running_sum,
					(uint16_t)~checksum);

	/* Calculate checksum of new IPv6 header */
	fdma_calculate_default_frame_checksum(
			ipv6hdr_offset,
			size,
			&checksum);

	/* Add new fields */
	gross_running_sum = cksum_ones_complement_sum16(gross_running_sum,
							checksum);

	pr->gross_running_sum = gross_running_sum;

	if(parse_result_generate_default(PARSER_VALIDATE_L4_CHECKSUM)) {
			/* error in L4 checksum */
//			return IPR_ERROR;
		}
	return SUCCESS;
}


uint32_t closing_with_reordering(struct ipr_rfdc *rfdc_ptr,
				 uint64_t rfdc_ext_addr)
{
	uint8_t				num_of_frags;
	uint8_t				current_index;
	uint8_t				octet_index;
	uint64_t			temp_ext_addr;
	struct link_list_element	link_list[8];
	struct				ldpaa_fd fds_to_concatenate[2] \
			      __attribute__((aligned(sizeof(struct ldpaa_fd))));
	struct		fdma_concatenate_frames_params concatenate_params;

	concatenate_params.flags = FDMA_CONCAT_SF_BIT;

	if (rfdc_ptr->status & ORDER_AND_OOO) {
		if (rfdc_ptr->index_to_out_of_order == 1) {
			temp_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST;
			cdma_read((void *)HWC_FD_ADDRESS,
					  temp_ext_addr,
					  FD_SIZE);
			/* Copy 1rst FD to default frame FD's place */
			/* *((struct ldpaa_fd *)(HWC_FD_ADDRESS)) =
			 * fds_to_concatenate[0];*/

			/* Open 1rst frame and get frame handle */
			fdma_present_default_frame_without_segments();
			concatenate_params.frame1 =
					    (uint16_t) PRC_GET_FRAME_HANDLE();
			current_index = rfdc_ptr->first_frag_index;
			temp_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
							current_index*FD_SIZE;

			cdma_read((void *)fds_to_concatenate,
					  temp_ext_addr,
					  FD_SIZE);
			/* Open frame and get frame handle */
			/* reset frame2 field because handle is 2 bytes in
			 * concatenate vs 1 byte in present*/
			concatenate_params.frame2 = 0;

			fdma_present_frame_without_segments(
				    fds_to_concatenate,
				    FDMA_INIT_NO_FLAGS,
				    0,
				    (uint8_t *)(&(concatenate_params.frame2)) +
				    sizeof(uint8_t));

			/* Take header size to be removed from FD[FRC] */
			concatenate_params.trim  =
					(uint8_t)fds_to_concatenate[0].frc;
			fdma_concatenate_frames(&concatenate_params);

			num_of_frags = rfdc_ptr->num_of_frags - 2;
			octet_index = 255; /* invalid value */

		} else {
		current_index = rfdc_ptr->index_to_out_of_order;
		closing_in_order(rfdc_ext_addr, current_index);
		num_of_frags = rfdc_ptr->num_of_frags - current_index - 1;
		current_index = rfdc_ptr->first_frag_index;

		temp_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
						current_index*FD_SIZE;

		cdma_read((void *)fds_to_concatenate,
				  temp_ext_addr,
				  FD_SIZE);
		/* Open frame and get frame handle */
		/* reset frame2 field because handle is 2 bytes in concatenate
		   vs 1 byte in present*/
		concatenate_params.frame2 = 0;

		concatenate_params.frame1 = (uint16_t) PRC_GET_FRAME_HANDLE();

		fdma_present_frame_without_segments(
				fds_to_concatenate,
				FDMA_INIT_NO_FLAGS,
				0,
				(uint8_t *)(&(concatenate_params.frame2)) +
				sizeof(uint8_t));

		/* Take header size to be removed from FD[FRC] */
		concatenate_params.trim  = (uint8_t)fds_to_concatenate[0].frc;
		fdma_concatenate_frames(&concatenate_params);
		octet_index = 255; /* invalid value */
		}
	} else {
		num_of_frags  = rfdc_ptr->num_of_frags;
		current_index = rfdc_ptr->first_frag_index;

		/* Bring 8 elements of LL */
		octet_index = current_index >> 3;
		temp_ext_addr = rfdc_ext_addr + START_OF_LINK_LIST +
				octet_index * 8 * LINK_LIST_ELEMENT_SIZE;
		cdma_read(link_list,
			  temp_ext_addr,
			  8*LINK_LIST_ELEMENT_SIZE);
		temp_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
						current_index*FD_SIZE;
		cdma_read(fds_to_concatenate,
				  temp_ext_addr,
				  FD_SIZE);
		current_index =
		      link_list[current_index&OCTET_LINK_LIST_MASK].next_index;
		temp_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
						current_index*FD_SIZE;
		cdma_read(fds_to_concatenate+1,
				  temp_ext_addr,
				  FD_SIZE);
		/* Copy 1rst FD to default frame FD's place */
		*((struct ldpaa_fd *)(HWC_FD_ADDRESS)) = fds_to_concatenate[0];

		/* Open 1rst frame and get frame handle */
		fdma_present_default_frame_without_segments();

		/* Open 2nd frame and get frame handle */
		/* reset frame2 field because handle is 2 bytes in concatenate
		   vs 1 byte in present*/
		concatenate_params.frame2 = 0;
		fdma_present_frame_without_segments(
			   fds_to_concatenate+1,
			   FDMA_INIT_NO_FLAGS,
			   0,
			   (uint8_t *)(&(concatenate_params.frame2)) +
			   sizeof(uint8_t));

		concatenate_params.spid   = *((uint8_t *) HWC_SPID_ADDRESS);
		concatenate_params.frame1 = (uint16_t) PRC_GET_FRAME_HANDLE();
		/* Take header size to be removed from 2nd FD[FRC] */
		concatenate_params.trim   = (uint8_t)fds_to_concatenate[1].frc;

		fdma_concatenate_frames(&concatenate_params);

		num_of_frags -= 2;
	}
	while (num_of_frags != 0) {
		if ((current_index >> 3) == octet_index)
			current_index =
		       link_list[current_index&OCTET_LINK_LIST_MASK].next_index;
		else {
			/* Bring 8 elements of LL */
			octet_index = current_index >> 3;
			temp_ext_addr = rfdc_ext_addr + START_OF_LINK_LIST +
					octet_index * 8 *
					LINK_LIST_ELEMENT_SIZE;
			cdma_read(link_list,
				  temp_ext_addr,
				  8*LINK_LIST_ELEMENT_SIZE);

			current_index =
		       link_list[current_index&OCTET_LINK_LIST_MASK].next_index;
		}

		temp_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
						current_index*FD_SIZE;

		cdma_read((void *)fds_to_concatenate,
				  temp_ext_addr,
				  FD_SIZE);
		/* Open frame and get frame handle */
		/* reset frame2 field because handle is 2 bytes in concatenate
		   vs 1 byte in present*/
		/* todo move this reset to be done once and not each
		 * iteration */
		concatenate_params.frame2 = 0;
		fdma_present_frame_without_segments(
				fds_to_concatenate,
				FDMA_INIT_NO_FLAGS,
				0,
				(uint8_t *)(&(concatenate_params.frame2)) +
				sizeof(uint8_t));

		/* Take header size to be removed from FD[FRC] */
		concatenate_params.trim  = (uint8_t)fds_to_concatenate[0].frc;

		fdma_concatenate_frames(&concatenate_params);

		num_of_frags -= 1;
	}

	return SUCCESS;
}

uint32_t check_for_frag_error()
{
	return SUCCESS;
}

void ipr_time_out(uint64_t rfdc_ext_addr, uint16_t opaque_not_used)
{
	UNUSED(opaque_not_used);
	UNUSED(rfdc_ext_addr);

}

void move_to_correct_ordering_scope2(uint32_t osm_status)
{
	if (osm_status == 0) {
		/* return to original ordering scope that entered
		 * the ipr_reassemble function */
		osm_scope_exit();
		osm_scope_exit();
	} else if (osm_status & START_CONCURRENT) {
		osm_scope_transition_to_concurrent_with_increment_scope_id();
	}
}

void check_remove_padding()
{
	uint8_t			delta;
	uint16_t		ipv4hdr_offset;
	struct ipv4hdr		*ipv4hdr_ptr;
	void			*tail_frame_ptr;
	struct fdma_delete_segment_data_params params;
	struct fdma_present_segment_params *present_params_ptr;

	ipv4hdr_offset = (uint16_t)PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	ipv4hdr_ptr = (struct ipv4hdr *)
		  (ipv4hdr_offset + PRC_GET_SEGMENT_ADDRESS());

	delta = (uint8_t) (LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS) -
			(ipv4hdr_ptr->total_length+ipv4hdr_offset));

	if (delta != 0) {

		present_params_ptr = (struct fdma_present_segment_params *)
					(&params);
		present_params_ptr->flags 	 = FDMA_PRES_SR_BIT;
		present_params_ptr->frame_handle =
					       (uint8_t) PRC_GET_FRAME_HANDLE();
		present_params_ptr->offset 	 = delta;
		present_params_ptr->ws_dst 	 = &tail_frame_ptr;
		present_params_ptr->present_size = delta;
		fdma_present_frame_segment(present_params_ptr);

		params.seg_handle	  = present_params_ptr->seg_handle;
		params.delete_target_size = delta;
		params.flags 		  = FDMA_REPLACE_SA_CLOSE_BIT,
		params.frame_handle	  = (uint8_t) PRC_GET_FRAME_HANDLE(),
		params.to_offset	  = 0;

		fdma_delete_segment_data(&params);
	}
	return;
}


uint32_t out_of_order(struct ipr_rfdc *rfdc_ptr, uint64_t rfdc_ext_addr,
		  uint32_t last_fragment, uint16_t current_frag_size,
		  uint16_t frag_offset_shifted)
{
	uint8_t				current_index;
	uint8_t				temp_frag_index;
	uint8_t				first_frag_index;
	uint8_t				octet_index;
	uint8_t				new_frag_index;
	uint8_t				current_index_in_octet;
	uint8_t				temp_index_in_octet;
	uint8_t				new_frag_index_in_octet;
	uint16_t			temp_total_payload;
	uint64_t			current_element_ext_addr;
	uint64_t			temp_element_ext_addr;
	uint64_t			new_frag_ext_addr;
	uint64_t			octet_ext_addr;
	struct link_list_element	*temp_element_ptr;
	struct link_list_element	*current_element_ptr;
	struct link_list_element	link_list[8];


	if (frag_offset_shifted < rfdc_ptr->total_in_order_payload) {
		/* overlap or duplicate */
		/* return IPR_ERROR; */
	}
	current_index = rfdc_ptr->next_index;
	current_element_ext_addr =  rfdc_ext_addr + START_OF_LINK_LIST +
					current_index*LINK_LIST_ELEMENT_SIZE;
	first_frag_index = rfdc_ptr->first_frag_index;
	temp_frag_index = rfdc_ptr->last_frag_index;
	temp_element_ext_addr = rfdc_ext_addr + START_OF_LINK_LIST +
				LINK_LIST_ELEMENT_SIZE*temp_frag_index;
	temp_total_payload = rfdc_ptr->biggest_payload;
	if (frag_offset_shifted >= temp_total_payload) {
		/* Bigger than last */
		temp_element_ptr = link_list;
		cdma_read((void *)temp_element_ptr,
			  temp_element_ext_addr,
			  LINK_LIST_ELEMENT_SIZE);
		if (LAST_FRAG_ARRIVED()) {
			/* Error */
			/* return IPR_ERROR; */
		}
		if(last_fragment)
			rfdc_ptr->expected_total_length = frag_offset_shifted +
							  current_frag_size;

		rfdc_ptr->biggest_payload = frag_offset_shifted +
							current_frag_size;
		current_element_ptr = link_list + 1;
		rfdc_ptr->last_frag_index	 = current_index;
		temp_element_ptr->next_index	 = current_index;
		current_element_ptr->prev_index  = temp_frag_index;
		current_element_ptr->frag_offset = frag_offset_shifted;
		current_element_ptr->frag_length = current_frag_size;
		/* not required */
		current_element_ptr->next_index	  = 0;
		cdma_write(temp_element_ext_addr,
				   temp_element_ptr,
				   LINK_LIST_ELEMENT_SIZE);
		cdma_write(current_element_ext_addr,
				   current_element_ptr,
				   LINK_LIST_ELEMENT_SIZE);
	} else {
		/* Smaller than last */
		if(last_fragment) {
			/* Current fragment is smaller than last but is marked
			 * as last : Error */
		}
		/* Bring 8 elements of the Link List */
		octet_index = temp_frag_index >> 3;
		octet_ext_addr = rfdc_ext_addr + START_OF_LINK_LIST +
			LINK_LIST_ELEMENT_SIZE * 8 * octet_index;
		cdma_read(link_list,
			  octet_ext_addr,
			  8*LINK_LIST_ELEMENT_SIZE);
		temp_index_in_octet = temp_frag_index & OCTET_LINK_LIST_MASK;
		temp_element_ptr = link_list + temp_index_in_octet;
		if ((frag_offset_shifted + current_frag_size) >
					temp_element_ptr->frag_offset) {
			/* Overlap */
			return 1;
		}
		do {
			if (temp_frag_index == first_frag_index) {
				rfdc_ptr->first_frag_index	= current_index;
				temp_element_ptr->prev_index	= current_index;
				current_index_in_octet = current_index &
							 OCTET_LINK_LIST_MASK;
				if ((current_index >> 3) == octet_index) {
				 link_list[current_index_in_octet].frag_length =
							    current_frag_size;
				 link_list[current_index_in_octet].frag_offset =
							frag_offset_shifted;
				 link_list[current_index_in_octet].next_index =
								temp_frag_index;
					/* not required */
				  link_list[current_index_in_octet].prev_index =
									    0;

				cdma_write(octet_ext_addr,
					   link_list,
					   8*LINK_LIST_ELEMENT_SIZE);
				} else {
					if (temp_index_in_octet == 0)
						current_element_ptr =
							   temp_element_ptr+1;
					else
						current_element_ptr =
							  temp_element_ptr-1;

					current_element_ptr->frag_length =
							     current_frag_size;
					current_element_ptr->frag_offset =
							frag_offset_shifted;
					current_element_ptr->next_index =
							      temp_frag_index;
				    /* not required */
					current_element_ptr->prev_index  = 0;

				    cdma_write(current_element_ext_addr,
						   current_element_ptr,
						   LINK_LIST_ELEMENT_SIZE);
				    temp_element_ext_addr = rfdc_ext_addr +
						    START_OF_LINK_LIST +
						    LINK_LIST_ELEMENT_SIZE *
						    temp_frag_index;
				    cdma_write(temp_element_ext_addr,
						   temp_element_ptr,
						   LINK_LIST_ELEMENT_SIZE);
				}
				rfdc_ptr->current_total_length +=
							     current_frag_size;
				rfdc_ptr->num_of_frags++;

				if (PARSER_IS_OUTER_IPV4_DEFAULT())
					check_remove_padding();
				/* Close current frame before storing FD */
				fdma_store_default_frame_data();

				/* Write FD in external buffer */
				current_element_ext_addr = rfdc_ext_addr +
							   START_OF_FDS_LIST +
							  current_index*FD_SIZE;
				cdma_write(current_element_ext_addr,
					       (void *)HWC_FD_ADDRESS,
					       FD_SIZE);

				if (rfdc_ptr->current_total_length ==
					rfdc_ptr->expected_total_length) {
					return LAST_FRAG_OUT_OF_ORDER;
				} else {
				       rfdc_ptr->next_index = current_index + 1;
				       return FRAG_OK_REASS_NOT_COMPL;
				}
			}
			temp_frag_index = temp_element_ptr->prev_index;
			if ((temp_frag_index>>3) != octet_index) {
				/* Bring 8 elements of the Link List */
				/* todo check if compiler add a clock
				   for next line */
				octet_index = temp_frag_index >> 3;
				octet_ext_addr = rfdc_ext_addr +
						START_OF_LINK_LIST +
						LINK_LIST_ELEMENT_SIZE * 8 *
						octet_index;
				cdma_read(link_list,
					  octet_ext_addr,
					  8*LINK_LIST_ELEMENT_SIZE);
			}
			temp_index_in_octet = temp_frag_index &
					      OCTET_LINK_LIST_MASK;
			temp_element_ptr = link_list + temp_index_in_octet;
		} while ((frag_offset_shifted + current_frag_size) <=
					temp_element_ptr->frag_offset);
		temp_total_payload = temp_element_ptr->frag_offset +
					temp_element_ptr->frag_length;
		if (frag_offset_shifted >= temp_total_payload) {
			current_index_in_octet = current_index &
						OCTET_LINK_LIST_MASK;
			if ((current_index >> 3) == octet_index) {
			new_frag_index = temp_element_ptr->next_index;
			link_list[current_index_in_octet].frag_length =
							current_frag_size;
			link_list[current_index_in_octet].frag_offset =
							frag_offset_shifted;
			link_list[current_index_in_octet].next_index =
								new_frag_index;
			link_list[current_index_in_octet].prev_index =
								temp_frag_index;

			link_list[temp_index_in_octet].next_index =
								current_index;

			if ((new_frag_index>>3) == octet_index) {
				new_frag_index_in_octet =
					new_frag_index&OCTET_LINK_LIST_MASK;
				link_list[new_frag_index_in_octet].prev_index =
								  current_index;
				cdma_write(octet_ext_addr,
					   link_list,
					   8*LINK_LIST_ELEMENT_SIZE);

			} else {
			    cdma_write(octet_ext_addr,
					   link_list,
					   8*LINK_LIST_ELEMENT_SIZE);
			    new_frag_ext_addr = rfdc_ext_addr +
					    START_OF_LINK_LIST +
					LINK_LIST_ELEMENT_SIZE * new_frag_index;
			    cdma_read(link_list,
					  new_frag_ext_addr,
					  LINK_LIST_ELEMENT_SIZE);
			    link_list[0].prev_index = current_index;
			    cdma_write(new_frag_ext_addr,
					  link_list,
					  LINK_LIST_ELEMENT_SIZE);
			  }
			   } else {
			new_frag_index = temp_element_ptr->next_index;
			link_list[temp_index_in_octet].next_index =
								current_index;

			if ((new_frag_index>>3) == octet_index) {

				new_frag_index_in_octet = new_frag_index &
							OCTET_LINK_LIST_MASK;
				link_list[new_frag_index_in_octet].prev_index =
								  current_index;
				/* update temp and new_frag elements */
				cdma_write(octet_ext_addr,
					   link_list,
					   8*LINK_LIST_ELEMENT_SIZE);

			} else {
				/* update temp element */
				cdma_write(octet_ext_addr,
					   link_list,
					   8*LINK_LIST_ELEMENT_SIZE);
			    new_frag_ext_addr = rfdc_ext_addr +
					    START_OF_LINK_LIST +
					    LINK_LIST_ELEMENT_SIZE *
					    new_frag_index;
			    cdma_read(link_list,
				     new_frag_ext_addr,
				     LINK_LIST_ELEMENT_SIZE);
			    link_list[0].prev_index = current_index;
			    cdma_write(new_frag_ext_addr,
					  link_list,
					  LINK_LIST_ELEMENT_SIZE);
			}

			link_list[0].frag_length = current_frag_size;
			link_list[0].frag_offset = frag_offset_shifted;
			link_list[0].next_index  = new_frag_index;
			link_list[0].prev_index  = temp_frag_index;

			cdma_write(current_element_ext_addr,
					   link_list,
					   LINK_LIST_ELEMENT_SIZE);
			   }
		} else {
			/* Error */
		}
	}

	if (PARSER_IS_OUTER_IPV4_DEFAULT())
		check_remove_padding();
	/* Close current frame before storing FD */
	fdma_store_default_frame_data();

	/* Write FD in external buffer */
	current_element_ext_addr = rfdc_ext_addr + START_OF_FDS_LIST +
					   current_index*FD_SIZE;
	cdma_write(current_element_ext_addr,
			   (void *)HWC_FD_ADDRESS,
			   FD_SIZE);

	rfdc_ptr->current_total_length += current_frag_size;
	rfdc_ptr->num_of_frags++;

	if (rfdc_ptr->current_total_length == rfdc_ptr->expected_total_length) {
		return LAST_FRAG_OUT_OF_ORDER;
	} else {
		rfdc_ptr->next_index++;
		return FRAG_OK_REASS_NOT_COMPL;
	}
}

void ipr_modify_max_reass_frm_size(ipr_instance_handle_t ipr_instance,
					  uint16_t max_reass_frm_size)
{
	cdma_write(ipr_instance+offsetof(struct ipr_instance,
		    max_reass_frm_size),
		   &max_reass_frm_size,
		   sizeof(max_reass_frm_size));
	return;
}

void ipr_modify_min_frag_size_ipv4(ipr_instance_handle_t ipr_instance,
				      uint16_t min_frag_size)
{
	cdma_write(ipr_instance+
		   offsetof(struct ipr_instance,min_frag_size_ipv4),
		   &min_frag_size,
		   sizeof(min_frag_size));
	return;
}

void ipr_modify_min_frag_size_ipv6(ipr_instance_handle_t ipr_instance,
				      uint16_t min_frag_size)
{
	cdma_write(ipr_instance+
		   offsetof(struct ipr_instance,min_frag_size_ipv6),
		   &min_frag_size,
		   sizeof(min_frag_size));
	return;
}

void ipr_modify_timeout_value_ipv4(ipr_instance_handle_t ipr_instance,
				      uint16_t reasm_timeout_value_ipv4)
{
	cdma_write(ipr_instance+
		   offsetof(struct ipr_instance, timeout_value_ipv4),
		   &reasm_timeout_value_ipv4,
		   sizeof(reasm_timeout_value_ipv4));
	return;
}

void ipr_modify_timeout_value_ipv6(ipr_instance_handle_t ipr_instance,
				      uint16_t reasm_timeout_value_ipv6)
{

	cdma_write(ipr_instance+
		   offsetof(struct ipr_instance, timeout_value_ipv6),
		   &reasm_timeout_value_ipv6,
		   sizeof(reasm_timeout_value_ipv6));
	return;
}

void ipr_get_reass_frm_cntr(ipr_instance_handle_t ipr_instance,
				uint32_t flags, uint32_t *reass_frm_cntr)
{

	if (flags & IPR_STATS_IP_VERSION)
		cdma_read(reass_frm_cntr,
			  ipr_instance+sizeof(struct ipr_instance)+
			  offsetof(struct ipr_instance_extension,
			  ipv4_reass_frm_cntr),
			  sizeof(*reass_frm_cntr));
	else
		cdma_read(reass_frm_cntr,
			  ipr_instance+sizeof(struct ipr_instance)+
			  offsetof(struct ipr_instance_extension,
				   ipv6_reass_frm_cntr),
			  sizeof(*reass_frm_cntr));
	return;
}
