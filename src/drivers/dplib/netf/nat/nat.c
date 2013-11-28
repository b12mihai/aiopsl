/**************************************************************************//**
@File		nat.c

@Description	This file contains the NAT header modification API
		implementation.

		Copyright 2013 Freescale Semiconductor, Inc.
*//***************************************************************************/

#include "general.h"
#include "dplib/fsl_parser.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_nat.h"
#include "dplib/fsl_l4.h"
#include "dplib/fsl_cdma.h"

#include "header_modification.h"


int32_t nat_ipv4(uint8_t flags, uint32_t ip_src_addr,
		uint32_t ip_dst_addr, uint16_t l4_src_port,
		uint16_t l4_dst_port, int16_t tcp_seq_num_delta,
		int16_t tcp_ack_num_delta)
{
	uint8_t l4_offset, ipv4_offset, modify_size;
	uint32_t old_header;
	struct tcphdr *tcp_ptr;
	struct ipv4hdr *ipv4_ptr;
	if (~PARSER_IS_L4_DEFAULT() || ~PARSER_IS_IP_DEFAULT())
		return NO_L4_IP_FOUND_ERROR;

	l4_offset = (uint8_t)(PARSER_GET_L4_OFFSET_DEFAULT());
	tcp_ptr = (struct tcphdr *)(l4_offset + PRC_GET_SEGMENT_ADDRESS());
	ipv4_offset = (uint8_t)(PARSER_GET_OUTER_IP_OFFSET_DEFAULT());
	ipv4_ptr = (struct ipv4hdr *)(ipv4_offset + PRC_GET_SEGMENT_ADDRESS());

	modify_size = l4_offset - ipv4_offset + TCP_NO_OPTION_SIZE;

	PARSER_CLEAR_RUNNING_SUM();

	if (flags & NAT_MODIFY_MODE_L4_CHECKSUM) {
		if (flags & NAT_MODIFY_MODE_IPSRC) {
			old_header = ipv4_ptr->src_addr;
			ipv4_ptr->src_addr = ip_src_addr;
			cksum_update_uint32(&ipv4_ptr->hdr_cksum,
					old_header,
					ipv4_ptr->src_addr);
			if (PARSER_IS_TCP_DEFAULT())
				cksum_update_uint32(&tcp_ptr->checksum,
							old_header,
							ipv4_ptr->src_addr);
			else /* In case UDP header */
				cksum_update_uint32(
					(uint16_t *)((uint16_t*)tcp_ptr+3),
					old_header,
					ipv4_ptr->src_addr);
		}
		if (flags & NAT_MODIFY_MODE_IPDST) {
			old_header = ipv4_ptr->dst_addr;
			ipv4_ptr->dst_addr = ip_dst_addr;
			cksum_update_uint32(&ipv4_ptr->hdr_cksum,
					old_header,
					ipv4_ptr->dst_addr);
			if (PARSER_IS_TCP_DEFAULT())
				cksum_update_uint32(&tcp_ptr->checksum,
							old_header,
							ipv4_ptr->dst_addr);
			else /* In case UDP header */
				cksum_update_uint32(
					(uint16_t *)((uint16_t*)tcp_ptr+3),
					old_header,
					ipv4_ptr->dst_addr);
		}

		old_header = *(uint32_t *)tcp_ptr;
		if (flags & NAT_MODIFY_MODE_L4SRC)
			tcp_ptr->src_port = l4_src_port;

		if (flags & NAT_MODIFY_MODE_L4DST)
			tcp_ptr->dst_port = l4_dst_port;

		if (flags & (NAT_MODIFY_MODE_L4SRC |
				NAT_MODIFY_MODE_L4DST))
			cksum_update_uint32(&tcp_ptr->checksum,
						old_header,
						*(uint32_t *)tcp_ptr);

		if (flags & NAT_MODIFY_MODE_TCP_SEQNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				fdma_modify_default_segment_data(ipv4_offset,
						modify_size);
				return NO_TCP_FOUND_ERROR;
			}
			old_header = tcp_ptr->sequence_number;
			/* todo need to verify if int16 is ok in the
			 * bellow addition */
			tcp_ptr->sequence_number += (int32_t)tcp_seq_num_delta;
			cksum_update_uint32(&tcp_ptr->checksum,
					old_header,
					tcp_ptr->sequence_number);
		}
		if (flags & NAT_MODIFY_MODE_TCP_ACKNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				fdma_modify_default_segment_data(ipv4_offset,
						modify_size);
				return NO_TCP_FOUND_ERROR;
			}
			old_header = tcp_ptr->acknowledgment_number;
			tcp_ptr->acknowledgment_number +=
					(int32_t)tcp_ack_num_delta;
			cksum_update_uint32(&tcp_ptr->checksum,
					old_header,
					tcp_ptr->acknowledgment_number);
		}

	} else {
		if (flags & NAT_MODIFY_MODE_IPSRC) {
			old_header = ipv4_ptr->src_addr;
			ipv4_ptr->src_addr = ip_src_addr;
			cksum_update_uint32(&ipv4_ptr->hdr_cksum,
					old_header,
					ipv4_ptr->src_addr);
		}
		if (flags & NAT_MODIFY_MODE_IPDST) {
			old_header = ipv4_ptr->dst_addr;
			ipv4_ptr->dst_addr = ip_dst_addr;
			cksum_update_uint32(&ipv4_ptr->hdr_cksum,
					old_header,
					ipv4_ptr->dst_addr);
		}

		if (flags & NAT_MODIFY_MODE_L4SRC)
			tcp_ptr->src_port = l4_src_port;

		if (flags & NAT_MODIFY_MODE_L4DST)
			tcp_ptr->dst_port = l4_dst_port;

		if (flags & NAT_MODIFY_MODE_TCP_SEQNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				fdma_modify_default_segment_data(ipv4_offset,
						modify_size);
				return NO_TCP_FOUND_ERROR;
			}
			/* todo need to verify if int16 is ok in the
			 * bellow addition */
			tcp_ptr->sequence_number += (int32_t)tcp_seq_num_delta;
		}
		if (flags & NAT_MODIFY_MODE_TCP_ACKNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				fdma_modify_default_segment_data(ipv4_offset,
						modify_size);
				return NO_TCP_FOUND_ERROR;
			}
			tcp_ptr->acknowledgment_number +=
					(int32_t)tcp_ack_num_delta;
		}
	}
	/* Modify the segment */
	fdma_modify_default_segment_data(ipv4_offset, modify_size);

	return SUCCESS;

}

int32_t nat_ipv6(uint8_t flags, uint32_t *ip_src_addr,
		uint32_t *ip_dst_addr, uint16_t l4_src_port,
		uint16_t l4_dst_port, int16_t tcp_seq_num_delta,
		int16_t tcp_ack_num_delta)
{
	uint8_t l4_offset, ipv6_offset, i, modify_offset;
	uint32_t old_header;
	struct tcphdr *tcp_ptr;
	struct ipv6hdr *ipv6_ptr;
	if (~PARSER_IS_L4_DEFAULT() || ~PARSER_IS_IP_DEFAULT())
		return NO_L4_IP_FOUND_ERROR;

	l4_offset = (uint8_t)(PARSER_GET_L4_OFFSET_DEFAULT());
	tcp_ptr = (struct tcphdr *)(l4_offset + PRC_GET_SEGMENT_ADDRESS());
	ipv6_offset = (uint8_t)(PARSER_GET_OUTER_IP_OFFSET_DEFAULT());
	ipv6_ptr = (struct ipv6hdr *)(ipv6_offset + PRC_GET_SEGMENT_ADDRESS());

	PARSER_CLEAR_RUNNING_SUM();

	if (flags & NAT_MODIFY_MODE_L4_CHECKSUM) {
		if (flags & NAT_MODIFY_MODE_IPSRC) {
			for(i=0; i<4; i++) {
				old_header = ipv6_ptr->src_addr[i];
				ipv6_ptr->src_addr[i] = ip_src_addr[i];
				if (PARSER_IS_TCP_DEFAULT())
					cksum_update_uint32(&tcp_ptr->checksum,
							old_header,
							ipv6_ptr->src_addr[i]);
				else /* In case UDP header */
					cksum_update_uint32(
					    (uint16_t*)((uint16_t*)tcp_ptr+3),
					    old_header,
					    ipv6_ptr->src_addr[i]);
			}
		}
		if (flags & NAT_MODIFY_MODE_IPDST) {
			for(i=0; i<4; i++) {
				old_header = ipv6_ptr->dst_addr[i];
				ipv6_ptr->dst_addr[i] = ip_dst_addr[i];
				if (PARSER_IS_TCP_DEFAULT())
					cksum_update_uint32(&tcp_ptr->checksum,
							old_header,
							ipv6_ptr->dst_addr[i]);
				else /* In case UDP header */
					cksum_update_uint32(
					    (uint16_t*)((uint16_t*)tcp_ptr+3),
					    old_header,
					    ipv6_ptr->dst_addr[i]);
			}

		}

		old_header = *(uint32_t *)tcp_ptr;
		if (flags & NAT_MODIFY_MODE_L4SRC) {
			tcp_ptr->src_port = l4_src_port;
		}
		if (flags & NAT_MODIFY_MODE_L4DST) {
			tcp_ptr->dst_port = l4_dst_port;
		}
		if (flags & (NAT_MODIFY_MODE_L4SRC |
				NAT_MODIFY_MODE_L4DST)) {
			if (PARSER_IS_TCP_DEFAULT())
				cksum_update_uint32(&tcp_ptr->checksum,
							old_header,
							*(uint32_t *)tcp_ptr);
			else /* In case UDP header */
				cksum_update_uint32(
					(uint16_t*)((uint16_t*)tcp_ptr+3),
					old_header,
					*(uint32_t *)tcp_ptr);
		}
		if (flags & NAT_MODIFY_MODE_TCP_SEQNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				/*from the IPv6 SRC addr */
				modify_offset = ipv6_offset + 8;
				fdma_modify_default_segment_data(modify_offset,
						IPV6_ADDR_SIZE);
				return NO_TCP_FOUND_ERROR;
			}
			old_header = tcp_ptr->sequence_number;
			/* todo need to verify if int16 is ok in the
			 * bellow addition */
			tcp_ptr->sequence_number += (int32_t)tcp_seq_num_delta;
			cksum_update_uint32(&tcp_ptr->checksum,
					old_header,
					tcp_ptr->sequence_number);
		}
		if (flags & NAT_MODIFY_MODE_TCP_ACKNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				/*from the IPv6 SRC addr */
				modify_offset = ipv6_offset + 8;
				fdma_modify_default_segment_data(modify_offset,
						IPV6_ADDR_SIZE);
				return NO_TCP_FOUND_ERROR;
			}
			old_header = tcp_ptr->acknowledgment_number;
			tcp_ptr->acknowledgment_number +=
					(int32_t)tcp_ack_num_delta;
			cksum_update_uint32(&tcp_ptr->checksum,
					old_header,
					tcp_ptr->acknowledgment_number);
		}

	} else {
		if (flags & NAT_MODIFY_MODE_IPSRC) {
			for(i=0; i<4; i++) {
				ipv6_ptr->src_addr[i] = ip_src_addr[i];
			}
		}
		if (flags & NAT_MODIFY_MODE_IPDST) {
			for(i=0; i<4; i++) {
				ipv6_ptr->dst_addr[i] = ip_dst_addr[i];
			}
		}

		if (flags & NAT_MODIFY_MODE_L4SRC) {
			tcp_ptr->src_port = l4_src_port;
		}
		if (flags & NAT_MODIFY_MODE_L4DST) {
			tcp_ptr->dst_port = l4_dst_port;
		}

		if (flags & NAT_MODIFY_MODE_TCP_SEQNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				/*from the IPv6 SRC addr */
				modify_offset = ipv6_offset + 8;
				fdma_modify_default_segment_data(modify_offset,
						IPV6_ADDR_SIZE);
				return NO_TCP_FOUND_ERROR;
			}
			/* todo need to verify if int16 is ok in the
			 * bellow addition */
			tcp_ptr->sequence_number += (int32_t)tcp_seq_num_delta;
		}
		if (flags & NAT_MODIFY_MODE_TCP_ACKNUM) {
			if (~PARSER_IS_TCP_DEFAULT()) {
				/*from the IPv6 SRC addr */
				modify_offset = ipv6_offset + 8;
				fdma_modify_default_segment_data(modify_offset,
						IPV6_ADDR_SIZE);
				return NO_TCP_FOUND_ERROR;
			}
			tcp_ptr->acknowledgment_number +=
					(int32_t)tcp_ack_num_delta;
		}
	}
	/*from the IPv6 SRC addr */
	modify_offset = ipv6_offset + 8;
	/* Modify the IPv6 header */
	fdma_modify_default_segment_data(modify_offset, IPV6_ADDR_SIZE);

	/* Modify the segment TCP/UDP header */
	fdma_modify_default_segment_data(l4_offset, TCP_NO_OPTION_SIZE);

	return SUCCESS;

}
