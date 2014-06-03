/**************************************************************************//**
@File		l4.c

@Description	This file contains the layer 4 header modification API
		implementation.

		Copyright 2013-2014 Freescale Semiconductor, Inc.
*//***************************************************************************/

#include "general.h"
#include "net/fsl_net.h"
#include "dplib/fsl_parser.h"
#include "parser.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_l4.h"
#include "dplib/fsl_cdma.h"
#include "checksum.h"
#include "header_modification.h"

extern __TASK struct aiop_default_task_params default_task_params;

int32_t l4_udp_header_modification(uint8_t flags,
		uint16_t udp_src_port, uint16_t udp_dst_port)
{
	uint8_t udp_offset;
	uint32_t old_header;
	struct udphdr *udp_ptr;
	udp_offset = (uint8_t)(PARSER_GET_L4_OFFSET_DEFAULT());
	udp_ptr = (struct udphdr *)(udp_offset + PRC_GET_SEGMENT_ADDRESS());
	old_header = *(uint32_t *)udp_ptr;
	if (!PARSER_IS_UDP_DEFAULT())
		return NO_UDP_FOUND_ERROR;
	PARSER_CLEAR_RUNNING_SUM();
	if (flags & L4_UDP_MODIFY_MODE_UDPSRC)
		udp_ptr->src_port = udp_src_port;
	if (flags & L4_UDP_MODIFY_MODE_UDPDST)
		udp_ptr->dst_port = udp_dst_port;
	if (flags & L4_UDP_MODIFY_MODE_L4_CHECKSUM) {
		if (udp_ptr->checksum == 0) {
			/* TODO checksum 0.6 API
			cksum_calc_udp_tcp_checksum(); */
			l4_udp_tcp_cksum_calc(
				L4_UDP_TCP_CKSUM_CALC_MODE_DONT_UPDATE_FDMA);
		} else {
			cksum_update_uint32(&udp_ptr->checksum,
					    old_header,
					    *(uint32_t *)udp_ptr);
		}
	}
	/* Modify the segment */
	fdma_modify_default_segment_data(udp_offset, 8);

	return SUCCESS;
}

int32_t l4_tcp_header_modification(uint8_t flags, uint16_t tcp_src_port,
		uint16_t tcp_dst_port, int16_t tcp_seq_num_delta,
		int16_t tcp_ack_num_delta, uint16_t tcp_mss)
{
	uint8_t tcp_offset, *options_ptr, modify_size, mss_found, *l5_ptr;
	uint8_t options_size;
	uint16_t old_mss;
	uint32_t old_header;
	struct tcphdr *tcp_ptr;
	tcp_offset = (uint8_t)(PARSER_GET_L4_OFFSET_DEFAULT());
	tcp_ptr = (struct tcphdr *)(tcp_offset + PRC_GET_SEGMENT_ADDRESS());
	modify_size = TCP_NO_OPTION_SIZE;
	if (!PARSER_IS_TCP_DEFAULT())
		return NO_TCP_FOUND_ERROR;
	PARSER_CLEAR_RUNNING_SUM();

	if (flags & L4_TCP_MODIFY_MODE_L4_CHECKSUM) {
		old_header = *(uint32_t *)tcp_ptr;
		if (flags & L4_TCP_MODIFY_MODE_TCPSRC)
			tcp_ptr->src_port = tcp_src_port;

		if (flags & L4_TCP_MODIFY_MODE_TCPDST)
			tcp_ptr->dst_port = tcp_dst_port;

		if (flags & (L4_TCP_MODIFY_MODE_TCPDST |
				L4_TCP_MODIFY_MODE_TCPSRC))
			cksum_update_uint32(&tcp_ptr->checksum,
				old_header,
				*(uint32_t *)tcp_ptr);

		if (flags & L4_TCP_MODIFY_MODE_SEQNUM) {
			old_header = tcp_ptr->sequence_number;
			/* todo need to verify if int16 is ok in the
			 * bellow addition */
			tcp_ptr->sequence_number += (int32_t)tcp_seq_num_delta;
			cksum_update_uint32(&tcp_ptr->checksum,
					old_header,
					tcp_ptr->sequence_number);
		}
		if (flags & L4_TCP_MODIFY_MODE_ACKNUM) {
			old_header = tcp_ptr->acknowledgment_number;
			tcp_ptr->acknowledgment_number +=
					(int32_t)tcp_ack_num_delta;
			cksum_update_uint32(&tcp_ptr->checksum,
					old_header,
					tcp_ptr->acknowledgment_number);
		}
		if (flags & L4_TCP_MODIFY_MODE_MSS) {
			if (!PARSER_IS_TCP_OPTIONS_DEFAULT()) {
				fdma_modify_default_segment_data(tcp_offset,
						modify_size);
				return NO_TCP_MSS_FOUND_ERROR;
			}
			options_size = (tcp_ptr->data_offset_reserved >>
					TCP_DATA_OFFSET_SHIFT);
			l5_ptr = (uint8_t *)tcp_ptr + options_size;
			options_ptr = (uint8_t *)tcp_ptr + TCP_NO_OPTION_SIZE;
			mss_found = 0;
			while (*options_ptr != 0 && !mss_found && (options_ptr <
					l5_ptr)) {
				if (*options_ptr == 2) {
					mss_found = 1;
					modify_size = options_size;
					old_mss = ((uint16_t *)options_ptr)[1];

					((uint16_t *)options_ptr)[1] = tcp_mss;

					cksum_update_uint32(&tcp_ptr->checksum,
						old_mss,
						((uint16_t *)options_ptr)[1]);
				}
				if (*options_ptr != 1)
					options_ptr += (uint8_t)options_ptr[1];
				else
					options_ptr++;

			}
			if (!mss_found) {
				fdma_modify_default_segment_data(tcp_offset,
						modify_size);
				return NO_TCP_MSS_FOUND_ERROR;
			}

		}
	} else {
		if (flags & L4_TCP_MODIFY_MODE_TCPSRC)
			tcp_ptr->src_port = tcp_src_port;

		if (flags & L4_TCP_MODIFY_MODE_TCPDST)
			tcp_ptr->dst_port = tcp_dst_port;

		if (flags & L4_TCP_MODIFY_MODE_SEQNUM)
			tcp_ptr->sequence_number += tcp_seq_num_delta;

		if (flags & L4_TCP_MODIFY_MODE_ACKNUM)
			tcp_ptr->acknowledgment_number += tcp_ack_num_delta;

		if (flags & L4_TCP_MODIFY_MODE_MSS) {
			if (!PARSER_IS_TCP_OPTIONS_DEFAULT()) {
				fdma_modify_default_segment_data(tcp_offset,
						modify_size);
				return NO_TCP_MSS_FOUND_ERROR;
			}

			options_size = (tcp_ptr->data_offset_reserved >>
					TCP_DATA_OFFSET_SHIFT);
			l5_ptr = (uint8_t *)tcp_ptr + options_size;
			options_ptr = (uint8_t *)tcp_ptr + TCP_NO_OPTION_SIZE;
			mss_found = 0;
			while (*options_ptr != 0 && !mss_found && (options_ptr <
					l5_ptr)) {
				if (*options_ptr == 2) {
					mss_found = 1;
					modify_size = options_size;
					((uint16_t *)options_ptr)[1] = tcp_mss;
				}
				if (*options_ptr != 1)
					options_ptr += (uint8_t)options_ptr[1];
				else
					options_ptr++;
			}
			if (!mss_found) {
				fdma_modify_default_segment_data(tcp_offset,
						modify_size);
				return NO_TCP_MSS_FOUND_ERROR;
			}
		}
	}
	/* Modify the segment */
	fdma_modify_default_segment_data(tcp_offset, modify_size);

	return SUCCESS;
}


int32_t l4_set_tp_src(uint16_t src_port)
{
	uint16_t l4_offset;
	struct   tcphdr *tcphdr_ptr;
	struct   udphdr *udphdr_ptr;
	struct   parse_result *pr =
				  (struct parse_result *)HWC_PARSE_RES_ADDRESS;

	l4_offset = PARSER_GET_L4_OFFSET_DEFAULT();

	if (PARSER_IS_TCP_DEFAULT()) {
		tcphdr_ptr = (struct tcphdr *) ((uint16_t)l4_offset
				+ PRC_GET_SEGMENT_ADDRESS());


		cksum_update_uint32(&tcphdr_ptr->checksum,
				    tcphdr_ptr->src_port,
				    src_port);

		tcphdr_ptr->src_port = src_port;

		/* update FDMA */
		fdma_modify_default_segment_data(l4_offset, 18);
		/* Invalidate gross running sum */
		pr->gross_running_sum = 0;

		return SUCCESS;
	} else if (PARSER_IS_UDP_DEFAULT()) {
		udphdr_ptr = (struct udphdr *) ((uint16_t)l4_offset
				+ PRC_GET_SEGMENT_ADDRESS());

		if (udphdr_ptr->checksum != 0) {
			cksum_update_uint32(&udphdr_ptr->checksum,
					    udphdr_ptr->src_port,
					    src_port);

		udphdr_ptr->src_port = src_port;

		/* update FDMA */
		fdma_modify_default_segment_data(l4_offset, 8);
		} else {
			udphdr_ptr->src_port = src_port;
			/* update FDMA */
			fdma_modify_default_segment_data(l4_offset, 2);
		}
		/* Invalidate gross running sum */
		pr->gross_running_sum = 0;


		return SUCCESS;
	} else {
		return NO_L4_FOUND_ERROR;
	}

}

int32_t l4_set_tp_dst(uint16_t dst_port)
{
	uint16_t l4_offset;
	struct   tcphdr *tcphdr_ptr;
	struct   udphdr *udphdr_ptr;
	struct   parse_result *pr =
				  (struct parse_result *)HWC_PARSE_RES_ADDRESS;


	l4_offset = PARSER_GET_L4_OFFSET_DEFAULT();

	if (PARSER_IS_TCP_DEFAULT()) {
		tcphdr_ptr = (struct tcphdr *) ((uint16_t)l4_offset
				+ PRC_GET_SEGMENT_ADDRESS());

		cksum_update_uint32(&tcphdr_ptr->checksum,
				    tcphdr_ptr->dst_port,
				    dst_port);

		tcphdr_ptr->dst_port = dst_port;

		/* update FDMA */
		fdma_modify_default_segment_data(l4_offset+2, 16);
		/* Invalidate gross running sum */
		pr->gross_running_sum = 0;

		return SUCCESS;
	} else if (PARSER_IS_UDP_DEFAULT()) {
		udphdr_ptr = (struct udphdr *) ((uint16_t)l4_offset
				+ PRC_GET_SEGMENT_ADDRESS());
		if (udphdr_ptr->checksum != 0) {
			cksum_update_uint32(&udphdr_ptr->checksum,
				    udphdr_ptr->dst_port,
				    dst_port);
			udphdr_ptr->dst_port = dst_port;

			/* update FDMA */
			fdma_modify_default_segment_data(l4_offset+2, 6);
			} else {
				udphdr_ptr->dst_port = dst_port;
				fdma_modify_default_segment_data(l4_offset+2,\
						2);
			}
		/* Invalidate gross running sum */
		pr->gross_running_sum = 0;


		return SUCCESS;
	} else {
		return NO_L4_FOUND_ERROR;
	}

}


int32_t l4_udp_tcp_cksum_calc(uint8_t flags)
{
	uint16_t	l3checksum_dummy;
	uint16_t	l4checksum;
	uint16_t	l4offset;
	int32_t		hw_status;
	struct tcphdr	*tcph;
	struct udphdr	*udph;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;


	/* Check if Gross Running Sum calculation is needed */
	if (!pr->gross_running_sum) {
		hw_status = fdma_calculate_default_frame_checksum(0, 0xFFFF,
					&pr->gross_running_sum);
		if (SUCCESS != hw_status)
			return hw_status;
	}

	/* Call parser */
	hw_status = parse_result_generate_checksum(
			(enum parser_starting_hxs_code)
			    default_task_params.parser_starting_hxs,
			0,
			&l3checksum_dummy,
			&l4checksum);
	if (PARSER_STATUS_PASS != hw_status)
		return hw_status;

	l4offset = PARSER_GET_L4_OFFSET_DEFAULT();

	if (PARSER_IS_TCP_DEFAULT()) {
		/* Point to the TCP header */
		tcph = (struct tcphdr *)(PRC_GET_SEGMENT_ADDRESS() + l4offset);

		/* Invalidate Parser Result Gross Running Sum field */
		pr->gross_running_sum = 0;

		/* Write checksum to TCP header */
		tcph->checksum = l4checksum;
	} /* TCP */
	else {
		/* Point to the UDP header */
		udph = (struct udphdr *)(PRC_GET_SEGMENT_ADDRESS() + l4offset);

		/* Invalidate Parser Result Gross Running Sum field */
		pr->gross_running_sum = 0;

		/* Write checksum to UDP header */
		udph->checksum = l4checksum;
	} /* UDP */

	/* Update FDMA */
	if (!(flags & L4_UDP_TCP_CKSUM_CALC_MODE_DONT_UPDATE_FDMA)) {
		hw_status = fdma_modify_default_segment_data(l4offset,
				sizeof((struct udphdr *)0)->checksum);
		if (hw_status != SUCCESS)
			return hw_status;
	}

	return SUCCESS;
}

