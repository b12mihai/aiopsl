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
@File		l2.c

@Description	This file contains the layer 2 header modification API
		implementation.

*//***************************************************************************/

#include "dplib/fsl_parser.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_l2.h"
#include "dplib/fsl_cdma.h"
#include "dplib/fsl_dpni_drv.h"
#include "net/fsl_net.h"
#include "header_modification.h"
#include "general.h"


void l2_header_remove(void)
{
	uint8_t  first_offset, last_offset;
	uint16_t size_to_be_removed;
	uint32_t fdma_flags;
	struct   parse_result *pr =
				(struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct   presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;


	/* If eth_offset is always 0 can remove following line*/
	first_offset = PARSER_GET_ETH_OFFSET_DEFAULT();

	if (PARSER_IS_ONE_MPLS_DEFAULT())
		last_offset = PARSER_GET_LAST_MPLS_OFFSET_DEFAULT() + 4;
	else
		last_offset = PARSER_GET_LAST_ETYPE_OFFSET_DEFAULT() + 2;

	size_to_be_removed = (uint16_t) (last_offset - first_offset);

	fdma_flags = (uint32_t)(FDMA_REPLACE_SA_REPRESENT_BIT);
	if ((prc->seg_length - size_to_be_removed) >= 128) {
		fdma_delete_default_segment_data((uint16_t)first_offset,
						 size_to_be_removed,
						 fdma_flags);
	} else {
		fdma_replace_default_segment_data((uint16_t)first_offset,
						  size_to_be_removed,
						  NULL,
						  0,
						  (void *)prc->seg_address,
						  128,
						  fdma_flags);
	}
	/* Re-run parser */
	parse_result_generate_default(0);

	/* Mark running sum as invalid */
	pr->gross_running_sum = 0;

	return;
}

int l2_vlan_header_remove()
{
	uint32_t fdma_flags;
	uint8_t  first_offset, last_offset;
	uint16_t size_to_be_removed;
	struct   parse_result *pr =
				(struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct   presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;


	if (PARSER_IS_ONE_VLAN_DEFAULT()) {

		first_offset = PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT() - 2;
		last_offset = PARSER_GET_LAST_VLAN_TCI_OFFSET_DEFAULT();
		size_to_be_removed = (uint16_t)(last_offset - first_offset + 2);

		fdma_flags = FDMA_REPLACE_SA_REPRESENT_BIT;
		/* Remove all VLAN headers */
		if ((prc->seg_length - size_to_be_removed) >= 128) {
			fdma_delete_default_segment_data((uint16_t)first_offset,
							 size_to_be_removed,
							 fdma_flags);
		} else {
			fdma_replace_default_segment_data(
						       (uint16_t)first_offset,
						       size_to_be_removed,
						       NULL,
						       0,
						       (void *)prc->seg_address,
						       128,
						       fdma_flags);

		}
		/* Re-run parser */
		parse_result_generate_default(0);
		/* Mark running sum as invalid */
		pr->gross_running_sum = 0;

		return SUCCESS;
	} else {
		return NO_VLAN_ERROR;
	}
}

void l2_set_dl_src(uint8_t *src_addr)
{
	uint16_t eth_smac_offset;
	uint32_t *eth_smac_ptr;

	eth_smac_offset = (uint16_t)(PARSER_GET_ETH_OFFSET_DEFAULT()+6);
	eth_smac_ptr = (uint32_t *)(eth_smac_offset +
			PRC_GET_SEGMENT_ADDRESS());

	*eth_smac_ptr = *((uint32_t *)src_addr);
	eth_smac_ptr += 1;
	*(uint16_t *)eth_smac_ptr = *((uint16_t *)(src_addr + 4));

	/* No need for parser offset update */
	/* Reset parser running sum */
	(((struct parse_result *)HWC_PARSE_RES_ADDRESS)->gross_running_sum) = 0;

	/* todo need to check error from fdma */
	/* Modify the segment */
	fdma_modify_default_segment_data(eth_smac_offset, 6);
}


void l2_set_dl_dst(uint8_t *dst_addr)
{
	uint16_t eth_dmac_offset;
	uint32_t *eth_dmac_ptr;

	eth_dmac_offset = (uint16_t)(PARSER_GET_ETH_OFFSET_DEFAULT());
	eth_dmac_ptr = (uint32_t *)(eth_dmac_offset +
			PRC_GET_SEGMENT_ADDRESS());

	*eth_dmac_ptr = *((uint32_t *)dst_addr);
	eth_dmac_ptr += 1;
	*(uint16_t *)eth_dmac_ptr = *((uint16_t *)(dst_addr + 4));

	/* No need for parser offset update */
	/* Reset parser running sum */
	(((struct parse_result *)HWC_PARSE_RES_ADDRESS)->gross_running_sum) = 0;

	/* todo need to check error from fdma */
	/* Modify the segment */
	fdma_modify_default_segment_data(eth_dmac_offset, 6);
}

int l2_set_vlan_vid(uint16_t vlan_vid)
{
	uint32_t *vlan_ptr, constructed_vlan;
	uint8_t vlan_offset;

	vlan_offset = (uint8_t)(PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT() - 2);

	if (!PARSER_IS_ONE_VLAN_DEFAULT())
		return NO_VLAN_ERROR;
	/* Optimization: whole vlan to remove 2 add cycles */
	vlan_ptr = (uint32_t *)(vlan_offset + PRC_GET_SEGMENT_ADDRESS());

	/* todo make sure that compiler C43 compiler issue is solved
	 * (1 cycle less - e_rlwinm r8,r8,0,0,19) */
	/* Optimization: using constructed_vlan variable remove 1 cycle of
	 * store to stack */
	constructed_vlan = (vlan_vid & VLAN_VID_MASK) |\
			(*vlan_ptr & ~VLAN_VID_MASK);

	*vlan_ptr = constructed_vlan;

	/* No need for parser offset update */
	/* Reset parser running sum */
	(((struct parse_result *)HWC_PARSE_RES_ADDRESS)->gross_running_sum) = 0;

	/* todo need to check error from fdma */
	/* Modify the segment */
	fdma_modify_default_segment_data(vlan_offset, 4);
	return SUCCESS;
}


int l2_set_vlan_pcp(uint8_t vlan_pcp)
{
	uint32_t *vlan_ptr, constructed_vlan;
	uint8_t vlan_offset;

	vlan_offset = (uint8_t)(PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT() - 2);

	if (!PARSER_IS_ONE_VLAN_DEFAULT())
		return NO_VLAN_ERROR;
	/* Optimization: whole vlan to remove 2 add cycles */
	vlan_ptr = (uint32_t *)(vlan_offset + PRC_GET_SEGMENT_ADDRESS());

	/* todo make sure that compiler C43 compiler issue is solved
	 * (1 cycle less - e_rlwinm r8,r8,0,19,15) */
	/* Optimization: using constructed_vlan variable remove 1 cycle of
	 * store to stack */
	/* Optimization: using VLAN_PCP_MASK to remove 1 or cycle */
	constructed_vlan = ((vlan_pcp << VLAN_PCP_SHIFT) & VLAN_PCP_MASK) |\
			(*vlan_ptr & ~VLAN_PCP_MASK);

	*vlan_ptr = constructed_vlan;

	/* No need for parser offset update */
	/* Reset parser running sum */
	(((struct parse_result *)HWC_PARSE_RES_ADDRESS)->gross_running_sum) = 0;

	/* todo need to check error from fdma */
	/* Modify the segment */
	fdma_modify_default_segment_data(vlan_offset, 4);
	return SUCCESS;
}

void l2_push_vlan(uint16_t ethertype)
{
	uint32_t inserted_vlan = 0;
	uint32_t *inserted_vlan_ptr;
	struct   parse_result *pr =
				(struct parse_result *)HWC_PARSE_RES_ADDRESS;

	inserted_vlan_ptr = &inserted_vlan;
	*((uint16_t *)inserted_vlan_ptr) = ethertype;

	fdma_insert_default_segment_data(12,
					inserted_vlan_ptr,
					4,
					FDMA_REPLACE_SA_REPRESENT_BIT);

	/* Re-run parser */
	parse_result_generate_default(0);
	/* Mark running sum as invalid */
	pr->gross_running_sum = 0;
}

void l2_push_and_set_vlan(uint32_t vlan_tag)
{
	struct   parse_result *pr =
				(struct parse_result *)HWC_PARSE_RES_ADDRESS;

	fdma_insert_default_segment_data(12,
					&vlan_tag,
					4,
					FDMA_REPLACE_SA_REPRESENT_BIT);

	/* Re-run parser */
	parse_result_generate_default(0);
	/* Mark running sum as invalid */
	pr->gross_running_sum = 0;
}

int l2_pop_vlan()
{
	uint32_t fdma_flags;
	uint16_t vlan_offset;
	struct   parse_result *pr =
				(struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct   presentation_context *prc =
				(struct presentation_context *) HWC_PRC_ADDRESS;

	if (PARSER_IS_ONE_VLAN_DEFAULT()) {
		vlan_offset = (uint16_t)
			      (PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT()) - 2;

		fdma_flags = FDMA_REPLACE_SA_REPRESENT_BIT;
		/* Remove all VLAN headers */
		if (prc->seg_length >= 132) {
			fdma_delete_default_segment_data(vlan_offset,
							 4,
							 fdma_flags);
		} else {
			fdma_replace_default_segment_data(
						      vlan_offset,
						      4,
						      NULL,
						      0,
						      (void *)prc->seg_address,
						      128,
						      fdma_flags);
		}

		/* Re-run parser */
		parse_result_generate_default(0);
		/* Mark running sum as invalid */
		pr->gross_running_sum = 0;

		return SUCCESS;

	} else {
		return NO_VLAN_ERROR;
	}
}


void l2_arp_response()
{
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	uint8_t local_hw_addr[NET_HDR_FLD_ETH_ADDR_SIZE];
	uint8_t *ethhdr = PARSER_GET_ETH_POINTER_DEFAULT();
	struct arphdr *arp_hdr = (struct arphdr *)
			PARSER_GET_ARP_POINTER_DEFAULT();
	uint32_t temp_ip;

	/* get local HW address */
	dpni_drv_get_primary_mac_addr(
			(uint16_t)dpni_get_receive_niid(), local_hw_addr);
	/* set ETH destination address */
	*((uint32_t *)(&ethhdr[0])) = *((uint32_t *)(arp_hdr->src_hw_addr));
	*((uint16_t *)(&ethhdr[4])) = *((uint16_t *)(arp_hdr->src_hw_addr + 4));
	/* set ETH source address */
	*((uint32_t *)(&ethhdr[6])) = *((uint32_t *)local_hw_addr);
	*((uint16_t *)(&ethhdr[10])) = *((uint16_t *)(local_hw_addr+4));

	/* set ARP HW destination address */
	*((uint32_t *)(arp_hdr->dst_hw_addr)) =
			*((uint32_t *)(arp_hdr->src_hw_addr));
	*((uint16_t *)(arp_hdr->dst_hw_addr + 4)) =
				*((uint16_t *)(arp_hdr->src_hw_addr + 4));
	/* set ARP ETH source address */
	*((uint32_t *)(arp_hdr->src_hw_addr)) = *((uint32_t *)local_hw_addr);
	*((uint16_t *)(arp_hdr->src_hw_addr + 4)) =
			*((uint16_t *)(local_hw_addr + 4));

	/* switch ARP IP addresses */
	temp_ip = arp_hdr->src_pro_addr;
	arp_hdr->src_pro_addr = arp_hdr->dst_pro_addr;
	arp_hdr->dst_pro_addr = temp_ip;

	/* set ARP operation to ARP Reply */
	arp_hdr->operation = ARP_REPLY_OP;

	fdma_modify_default_segment_data(PARSER_GET_ETH_OFFSET_DEFAULT(),
			(ARPHDR_ETH_HDR_LEN + ARP_HDR_LEN));

	/* Mark running sum as invalid */
	pr->gross_running_sum = 0;
}

void l2_set_hw_src_dst(uint8_t *target_hw_addr)
{
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	uint8_t local_hw_addr[NET_HDR_FLD_ETH_ADDR_SIZE];
	uint8_t *ethhdr = PARSER_GET_ETH_POINTER_DEFAULT();

	/* get local HW address */
	dpni_drv_get_primary_mac_addr(
			(uint16_t)dpni_get_receive_niid(), local_hw_addr);
	/* set ETH destination address */
	*((uint32_t *)(&ethhdr[0])) = *((uint32_t *)(target_hw_addr));
	*((uint16_t *)(&ethhdr[4])) = *((uint16_t *)(target_hw_addr + 4));
	/* set ETH source address */
	*((uint32_t *)(&ethhdr[6])) = *((uint32_t *)local_hw_addr);
	*((uint16_t *)(&ethhdr[10])) = *((uint16_t *)(local_hw_addr+4));

	fdma_modify_default_segment_data(PARSER_GET_ETH_OFFSET_DEFAULT(),
		(NET_HDR_FLD_ETH_ADDR_SIZE + NET_HDR_FLD_ETH_ADDR_SIZE));

	/* Mark running sum as invalid */
	pr->gross_running_sum = 0;
}


