/**************************************************************************//**
@File		gro.c

@Description	This file contains the AIOP SW TCP GRO API implementation

		Copyright 2013 Freescale Semiconductor, Inc.
*//***************************************************************************/

#include "net/fsl_net.h"
#include "dplib/fsl_ldpaa.h"
#include "dplib/fsl_cdma.h"
#include "dplib/fsl_parser.h"
#include "dplib/fsl_ste.h"
#include "dplib/fsl_tman.h"
#include "gro.h"
#include "general.h"
#include "fdma.h"
#include "checksum.h"
#include "cdma.h"


/* New Aggregation */
int32_t tcp_gro_aggregate_seg(
		uint64_t tcp_gro_context_addr,
		struct tcp_gro_context_params *params,
		uint32_t flags)
{
	struct tcp_gro_context gro_ctx;
	struct tcphdr *tcp;
	int32_t status;
	int32_t sr_status;
	uint32_t ecn;
	uint16_t seg_size;
	uint8_t data_offset;

	/* If segment FD contain errors (FD[err] != 0) return the frame to
	 * the user. */
	if (LDPAA_FD_GET_ERR(HWC_FD_ADDRESS))
		return -EBADFD;


	seg_size = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS);

	/* read GRO context*/
	cdma_read_with_mutex(tcp_gro_context_addr,
			CDMA_PREDMA_MUTEX_WRITE_LOCK,
			(void *)(&gro_ctx),
			(uint16_t)sizeof(struct tcp_gro_context));

	/* add segment to an existing aggregation */
	if (gro_ctx.metadata.seg_num != 0) {
		status = tcp_gro_add_seg_to_aggregation(
				tcp_gro_context_addr, params, &gro_ctx);
		/* write entire gro context back to DDR + release
		 * mutex */
		cdma_write_with_mutex(
			tcp_gro_context_addr,
			CDMA_POSTDMA_MUTEX_RM_BIT,
			(void *)&gro_ctx,
			(uint16_t)
				sizeof(struct tcp_gro_context));
		return status;
	}

	/* read segment sizes address */
	if (flags & TCP_GRO_METADATA_SEGMENT_SIZES) {
		cdma_read(&(gro_ctx.metadata.seg_sizes_addr),
				params->metadata_addr,
				(uint16_t)METADATA_MEMBER1_SIZE);
		cdma_write(gro_ctx.metadata.seg_sizes_addr,
				&seg_size, (uint16_t)sizeof(seg_size));
		gro_ctx.metadata.seg_sizes_addr += (uint16_t)sizeof(seg_size);
	}

	/* set metadata values */
	gro_ctx.metadata.seg_num = 1;
	gro_ctx.metadata.max_seg_size = seg_size;

	/* New aggregation - Initialize GRO Context */
	tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();

	/* Flush Aggregation */
	if ((tcp->flags & NET_HDR_FLD_TCP_FLAGS_PSH) ||
	    (params->limits.seg_num_limit <= 1)	||
	    (params->limits.packet_size_limit <= seg_size))
		return TCP_GRO_SEG_AGG_DONE;

	/* Aggregate */
	/* create timer for the aggregation */

	sr_status = tman_create_timer(params->timeout_params.tmi_id,
			(uint32_t)(
				(params->timeout_params.granularity <<
						GRO_GRAN_OFFSET) |
						TMAN_CREATE_TIMER_ONE_SHOT),
			params->limits.timeout_limit,
			tcp_gro_context_addr,
			0,
			&tcp_gro_timeout_callback,
			&(gro_ctx.timer_handle));

	/* no more timers available */
	if (sr_status == -ENAVAIL){
		cdma_mutex_lock_release(tcp_gro_context_addr);
		return sr_status;
	}

	/* initialize gro context fields */
	gro_ctx.params = *params;
	gro_ctx.flags = flags;
	gro_ctx.last_ack = tcp->acknowledgment_number;
	data_offset = (tcp->data_offset_reserved &
			NET_HDR_FLD_TCP_DATA_OFFSET_MASK) >>
			(NET_HDR_FLD_TCP_DATA_OFFSET_OFFSET -
			 NET_HDR_FLD_TCP_DATA_OFFSET_SHIFT_VALUE);

	gro_ctx.agg_headers_size = (uint16_t)
			(PARSER_GET_L4_OFFSET_DEFAULT() + data_offset);
	gro_ctx.next_seq = tcp->sequence_number + seg_size -
			gro_ctx.agg_headers_size;
	/* in case there is an option it must be a timestamp option */
	if (data_offset > TCP_HDR_LENGTH) {
		/* Timestamp option is optimized with to starting nops */
		if (*((uint8_t *)((uint8_t *)tcp + TCP_HDR_LENGTH)) ==
				TIMESTAMP_NOP_VAL)
			gro_ctx.timestamp =
					((struct tcphdr_gro_opt *)tcp)->tsval;
		else /* Timestamp option is not optimized */
			gro_ctx.timestamp = ((struct tcphdr_gro *)tcp)->tsval;
		gro_ctx.internal_flags |= TCP_GRO_HAS_TIMESTAMP;
	}

	/* set ECN flags */
	ecn = *((uint32_t *)(PARSER_GET_OUTER_IP_POINTER_DEFAULT()));
	if (PARSER_IS_OUTER_IPV6_DEFAULT())
		ecn >>= TCP_GRO_IPV6_ECN_OFFSET;
	ecn = ecn & TCP_GRO_ECN_MASK;
	gro_ctx.internal_flags |= ecn;

	/* store aggregated frame */
	sr_status = fdma_store_frame_data(PRC_GET_FRAME_HANDLE(),
			*(uint8_t *)HWC_SPID_ADDRESS,
			&(gro_ctx.agg_fd_isolation_attributes));

	if (sr_status == -ENOMEM){

		cdma_mutex_lock_release(tcp_gro_context_addr);
		return sr_status;
	}


	/* copy default FD to gro context */
	gro_ctx.agg_fd = *((struct ldpaa_fd *)HWC_FD_ADDRESS);
	/* write gro context back to DDR + release mutex */
	cdma_write_with_mutex(tcp_gro_context_addr,
			CDMA_POSTDMA_MUTEX_RM_BIT,
			(void *)&gro_ctx,
			(uint16_t)sizeof(struct tcp_gro_context));

	/* update statistics */
	ste_inc_counter(gro_ctx.params.stats_addr +
		GRO_STAT_SEG_NUM_CNTR_OFFSET,
		1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);

	return TCP_GRO_SEG_AGG_NOT_DONE_NEW_AGG;
}

/* Add segment to an existing aggregation */
int32_t tcp_gro_add_seg_to_aggregation(
		uint64_t tcp_gro_context_addr,
		struct tcp_gro_context_params *params,
		struct tcp_gro_context *gro_ctx)
{
	struct tcphdr *tcp;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct fdma_concatenate_frames_params concat_params;
	uint32_t timestamp;
	uint32_t ecn;
	int32_t  sr_status;
	uint16_t headers_size;
	uint16_t seg_size;
	uint16_t aggregated_size;
	uint8_t  data_offset;

	tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();

	/* Check for termination conditions due to the current segment.
	 * In case one of the following conditions is met, close the aggregation
	 * and start a new aggregation with the current segment */
	/* 1. Segment sequence number is not the expected sequence number  */
	if (gro_ctx->next_seq != tcp->sequence_number) {
		/* update statistics */
		if (gro_ctx->flags & TCP_GRO_EXTENDED_STATS_EN)
			ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_UNEXPECTED_SEQ_NUM_CNTR_OFFSET, 1,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
		return tcp_gro_close_aggregation_and_open_new_aggregation(
				tcp_gro_context_addr, params, gro_ctx);
	}

	/* 2. IP ECN value of the new packet is different from previously
	 * coalesced packet.
	 * 3. Aggregated timestamp value is different than the new segment
	 * timestamp value.
	 * 4. Segment ACK number is less than the ACK number of the previously
	 * coalesced segment.
	 * 5. PHS flag is set for the aggregation from the previous segment */
	ecn = *((uint32_t *)PARSER_GET_OUTER_IP_POINTER_DEFAULT());
	if (PARSER_IS_OUTER_IPV6_DEFAULT())
		ecn >>= TCP_GRO_IPV6_ECN_OFFSET;
	ecn &= TCP_GRO_ECN_MASK;
	data_offset = (tcp->data_offset_reserved &
			NET_HDR_FLD_TCP_DATA_OFFSET_MASK) >>
			(NET_HDR_FLD_TCP_DATA_OFFSET_OFFSET -
			 NET_HDR_FLD_TCP_DATA_OFFSET_SHIFT_VALUE);
	timestamp = 0;
	if (data_offset > TCP_HDR_LENGTH) {
		/* Timestamp option is optimized with to starting nops */
		if (*((uint8_t *)((uint8_t *)tcp + TCP_HDR_LENGTH)) ==
				TIMESTAMP_NOP_VAL)
			timestamp = ((struct tcphdr_gro_opt *)tcp)->tsval;
		else /* Timestamp option is not optimized */
			timestamp = ((struct tcphdr_gro *)tcp)->tsval;
	}

	if (((gro_ctx->internal_flags & TCP_GRO_ECN_MASK) != ecn)	||
		(gro_ctx->timestamp != timestamp)			||
		(gro_ctx->last_ack > tcp->acknowledgment_number)	||
		(gro_ctx->internal_flags & TCP_GRO_FLUSH_AGG_SET))
		return tcp_gro_close_aggregation_and_open_new_aggregation(
				tcp_gro_context_addr, params, gro_ctx);

	/* Check for termination condition due to the current segment.
	 * In case one of the following conditions is met, add segment to
	 * aggregation and close the aggregation. */
	if (tcp->flags & NET_HDR_FLD_TCP_FLAGS_PSH)
		return tcp_gro_add_seg_and_close_aggregation(gro_ctx);

	/* calculate data offset */
	headers_size = (uint16_t)(PARSER_GET_L4_OFFSET_DEFAULT() + data_offset);

	seg_size = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS);
	aggregated_size = (uint16_t)(LDPAA_FD_GET_LENGTH(&(gro_ctx->agg_fd))) +
			seg_size - headers_size;
	/* check whether aggregation limits are met */
	/* check aggregated packet size limit */
	if (aggregated_size > gro_ctx->params.limits.packet_size_limit)
		return tcp_gro_close_aggregation_and_open_new_aggregation(
				tcp_gro_context_addr, params, gro_ctx);
	else if (aggregated_size == gro_ctx->params.limits.packet_size_limit) {
		/* update statistics */
		if (gro_ctx->flags & TCP_GRO_EXTENDED_STATS_EN)
			ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_AGG_MAX_PACKET_SIZE_CNTR_OFFSET, 1,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
		return tcp_gro_add_seg_and_close_aggregation(gro_ctx);
	}
	/* check segment number limit */
	if ((gro_ctx->metadata.seg_num + 1) ==
			gro_ctx->params.limits.seg_num_limit){
		/* update statistics */
		if (gro_ctx->flags & TCP_GRO_EXTENDED_STATS_EN)
			ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_AGG_MAX_SEG_NUM_CNTR_OFFSET, 1,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
		return tcp_gro_add_seg_and_close_aggregation(gro_ctx);
	}

	/* segment can be aggregated */

	concat_params.frame1 = 0;
	/* present aggregated frame */
	fdma_present_frame_without_segments(&(gro_ctx->agg_fd),
			FDMA_INIT_NO_FLAGS, 0,
			(uint8_t *)(&(concat_params.frame1)) + sizeof(uint8_t));
	/* concatenate frames and store aggregated packet */
	concat_params.frame2 = (uint16_t)PRC_GET_FRAME_HANDLE();
	concat_params.trim = (uint8_t)headers_size;
	concat_params.spid = *((uint8_t *)HWC_SPID_ADDRESS);
	concat_params.flags = FDMA_CONCAT_PCA_BIT;
	/*concat_params.flags = FDMA_CONCAT_NO_FLAGS;*/
	sr_status = fdma_concatenate_frames(&concat_params);
	/* Report to the user that due to a concatenation failure (due to buffer
	 * pool depletion) the aggregation was discarded. */
	if (sr_status != SUCCESS){
		fdma_discard_frame(concat_params.frame1, FDMA_DIS_NO_FLAGS);
		/* update statistics */
		ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_AGG_DISCARDED_SEG_NUM_CNTR_OFFSET,
				gro_ctx->metadata.seg_num,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
		/* zero gro context fields */
		gro_ctx->metadata.seg_num = 0;
		gro_ctx->internal_flags = 0;
		gro_ctx->timestamp = 0;
		/* Clear gross running sum in parse results */
		pr->gross_running_sum = 0;
		return TCP_GRO_AGG_DISCARDED;
	}
	/* update gro context fields */
	gro_ctx->last_ack = tcp->acknowledgment_number;
	gro_ctx->next_seq = gro_ctx->next_seq + seg_size - headers_size;
	gro_ctx->last_seg_fields = *((struct tcp_gro_last_seg_header_fields *)
			(&(tcp->acknowledgment_number)));
	gro_ctx->metadata.seg_num++;
	if (gro_ctx->metadata.max_seg_size < seg_size)
		gro_ctx->metadata.max_seg_size = seg_size;

	/* write metadata segment size to external memory */
	if (gro_ctx->flags & TCP_GRO_METADATA_SEGMENT_SIZES) {
		cdma_write(gro_ctx->metadata.seg_sizes_addr,
				&seg_size, (uint16_t)sizeof(seg_size));
		gro_ctx->metadata.seg_sizes_addr += (uint16_t)sizeof(seg_size);
	}
	/* update statistics */
	ste_inc_counter(gro_ctx->params.stats_addr +
			GRO_STAT_SEG_NUM_CNTR_OFFSET,
			1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);

	return TCP_GRO_SEG_AGG_NOT_DONE;
}

/* Add segment to aggregation and close aggregation. */
int32_t tcp_gro_add_seg_and_close_aggregation(
		struct tcp_gro_context *gro_ctx)
{
	struct tcphdr *tcp;
	struct ipv4hdr *ipv4;
	struct ipv6hdr *ipv6;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct fdma_concatenate_frames_params concat_params;
	int32_t sr_status;
	uint16_t seg_size, headers_size, ip_length;
	uint16_t outer_ip_offset;
	uint8_t  data_offset;

	/* delete the timer for this aggregation */
	tman_delete_timer(gro_ctx->timer_handle,
			TMAN_TIMER_DELETE_MODE_WO_EXPIRATION);

	gro_ctx->timer_handle = TCP_GRO_INVALID_TMAN_HANDLE;

	tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();
	seg_size = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS);

	/* update gro context fields */
	gro_ctx->last_seg_fields = *((struct tcp_gro_last_seg_header_fields *)
				(&(tcp->acknowledgment_number)));
	gro_ctx->metadata.seg_num++;
	if (gro_ctx->metadata.max_seg_size < seg_size)
		gro_ctx->metadata.max_seg_size = seg_size;

	concat_params.frame1 = 0;
	/* present aggregated frame */
	fdma_present_frame_without_segments(&(gro_ctx->agg_fd),
			FDMA_INIT_NO_FLAGS, 0,
			(uint8_t *)(&(concat_params.frame1)) + sizeof(uint8_t));

	/* concatenate frames and store aggregated packet */
	data_offset = (tcp->data_offset_reserved &
			NET_HDR_FLD_TCP_DATA_OFFSET_MASK) >>
			(NET_HDR_FLD_TCP_DATA_OFFSET_OFFSET -
			 NET_HDR_FLD_TCP_DATA_OFFSET_SHIFT_VALUE);
	headers_size = (uint16_t)(PARSER_GET_L4_OFFSET_DEFAULT() + data_offset);
	concat_params.frame2 = (uint16_t)PRC_GET_FRAME_HANDLE();
	concat_params.trim = (uint8_t)headers_size;
	concat_params.spid = *((uint8_t *)HWC_SPID_ADDRESS);
	concat_params.flags = FDMA_CONCAT_PCA_BIT;
	/*concat_params.flags = FDMA_CONCAT_NO_FLAGS;*/
	sr_status = fdma_concatenate_frames(&concat_params);
	/* Report to the user that due to a concatenation failure (due to buffer
	 * pool depletion) the aggregation was discarded. */
	if (sr_status != SUCCESS){
		fdma_discard_frame(concat_params.frame1, FDMA_DIS_NO_FLAGS);
		/* update statistics */
		ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_AGG_DISCARDED_SEG_NUM_CNTR_OFFSET,
				gro_ctx->metadata.seg_num,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);

		/* zero gro context fields */
		gro_ctx->metadata.seg_num = 0;
		gro_ctx->internal_flags = 0;
		gro_ctx->timestamp = 0;
		/* Clear gross running sum in parse results */
		pr->gross_running_sum = 0;
		return TCP_GRO_AGG_DISCARDED;
	}
	/* store aggregated frame*/
	/*sr_status = fdma_store_frame_data((uint8_t)(concat_params.frame1),
				*((uint8_t *)HWC_SPID_ADDRESS),
				&(gro_ctx->agg_fd_isolation_attributes));
*/
	/* copy aggregated FD to default FD */
	*((struct ldpaa_fd *)HWC_FD_ADDRESS) = gro_ctx->agg_fd;

	/* present default FD (the aggregated FD) */
	fdma_present_default_frame();

	/* run parser if headers were changed */
	/*if (gro_ctx->agg_headers_size != headers_size) {
		sr_status = parse_result_generate_default(PARSER_NO_FLAGS);
		tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();
	}*/

	/* update IP length + checksum */
	outer_ip_offset = (uint16_t)PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	ip_length = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS) -
			outer_ip_offset;
	if (PARSER_IS_OUTER_IPV4_DEFAULT()) {
		ipv4 = (struct ipv4hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		if (gro_ctx->flags & TCP_GRO_CALCULATE_IP_CHECKSUM)
			cksum_update_uint32(&(ipv4->hdr_cksum),
					ipv4->total_length, ip_length);
		ipv4->total_length = ip_length;
	} else {
		ipv6 = (struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		ipv6->payload_length = ip_length - IPV6_HDR_LENGTH;
	}

	/* update TCP fields from last segment  */
	*((struct tcp_gro_last_seg_header_fields *)
		(&(tcp->acknowledgment_number))) = gro_ctx->last_seg_fields;

	/* Save headers changes to FDMA */
	/* Zero TCP checksum before calculating new TCP checksum. */
	if (gro_ctx->flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp->checksum = 0;
	fdma_modify_default_segment_data(outer_ip_offset, (uint16_t)
	   (PARSER_GET_L4_OFFSET_DEFAULT() + TCP_HDR_LENGTH - outer_ip_offset));
	/* calculate tcp header checksum */
	if (gro_ctx->flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp_gro_calc_tcp_header_cksum();

	/* write metadata segment size to external memory */
	if (gro_ctx->flags & TCP_GRO_METADATA_SEGMENT_SIZES) {
		cdma_write(gro_ctx->metadata.seg_sizes_addr,
				&seg_size, (uint16_t)sizeof(seg_size));
	}
	/* write metadata to external memory */
	cdma_write((gro_ctx->params.metadata_addr + METADATA_MEMBER1_SIZE),
			&(gro_ctx->metadata.seg_num),
			(uint16_t)(METADATA_MEMBER2_SIZE +
					METADATA_MEMBER3_SIZE));

	/* update statistics */
	ste_inc_and_acc_counters(gro_ctx->params.stats_addr +
			GRO_STAT_AGG_NUM_CNTR_OFFSET, 1,
			STE_MODE_COMPOUND_32_BIT_CNTR_SIZE |
			STE_MODE_COMPOUND_32_BIT_ACC_SIZE |
			STE_MODE_COMPOUND_CNTR_SATURATE |
			STE_MODE_COMPOUND_ACC_SATURATE);

	/* zero gro context fields */
	gro_ctx->metadata.seg_num = 0;
	gro_ctx->internal_flags = 0;
	gro_ctx->timestamp = 0;

	/* Clear gross running sum in parse results */
	pr->gross_running_sum = 0;

	return TCP_GRO_SEG_AGG_DONE;
}

/* Close an existing aggregation and start a new aggregation with the new
 * segment. */
int32_t tcp_gro_close_aggregation_and_open_new_aggregation(
		uint64_t tcp_gro_context_addr,
		struct tcp_gro_context_params *params,
		struct tcp_gro_context *gro_ctx)
{
	struct tcphdr *tcp;
	uint8_t data_offset;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	uint16_t headers_size, outer_ip_offset, ip_length, seg_size;
	/*uint16_t agg_checksum, new_agg_checksum;*/
	struct ipv4hdr *ipv4;
	struct ipv6hdr *ipv6;
	struct ldpaa_fd tmp_fd;
	int32_t sr_status;
	uint32_t old_agg_timestamp;

	seg_size = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS);
	tcp = (struct tcphdr *)(PARSER_GET_L4_POINTER_DEFAULT());

	/* initialize gro_context parameters */
	old_agg_timestamp = gro_ctx->internal_flags & TCP_GRO_HAS_TIMESTAMP;
	gro_ctx->internal_flags = 0;
	data_offset = (tcp->data_offset_reserved &
			NET_HDR_FLD_TCP_DATA_OFFSET_MASK) >>
			(NET_HDR_FLD_TCP_DATA_OFFSET_OFFSET -
			NET_HDR_FLD_TCP_DATA_OFFSET_SHIFT_VALUE);
	headers_size = (uint16_t)(PARSER_GET_L4_OFFSET_DEFAULT() + data_offset);
	/* Flush the segment when possible */
	if ((tcp->flags & NET_HDR_FLD_TCP_FLAGS_PSH) ||
		(params->limits.seg_num_limit <= 1)  ||
		(params->limits.packet_size_limit <= seg_size)) {
		gro_ctx->internal_flags |= TCP_GRO_FLUSH_AGG_SET;
	} else {
		gro_ctx->last_ack = tcp->acknowledgment_number;
		gro_ctx->next_seq = tcp->sequence_number +
				(uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS) -
				headers_size;
		/* save timestamp if exist */
		if (data_offset > TCP_HDR_LENGTH) {
			/* Timestamp option is optimized with to starting nops*/
			if (*((uint8_t *)((uint8_t *)tcp + TCP_HDR_LENGTH)) ==
					TIMESTAMP_NOP_VAL)
				gro_ctx->timestamp =
					((struct tcphdr_gro_opt *)tcp)->tsval;
			else /* Timestamp option is not optimized */
				gro_ctx->timestamp =
					((struct tcphdr_gro *)tcp)->tsval;
			gro_ctx->internal_flags |= TCP_GRO_HAS_TIMESTAMP;
		} else {
			gro_ctx->timestamp = 0;
		}

		/* Set ECN flags */
		if (PARSER_IS_OUTER_IPV4_DEFAULT()) {
			/* IPv4 */
			ipv4 = (struct ipv4hdr *)
					PARSER_GET_OUTER_IP_POINTER_DEFAULT();
			gro_ctx->internal_flags |= (*((uint32_t *)ipv4) &
					TCP_GRO_ECN_MASK);
		} else {
			/* IPv6 */
			ipv6 = (struct ipv6hdr *)
					PARSER_GET_OUTER_IP_POINTER_DEFAULT();
			gro_ctx->internal_flags |= (((*((uint32_t *)ipv6)) >>
					TCP_GRO_IPV6_ECN_OFFSET) &
					TCP_GRO_ECN_MASK);
		}
	}

	/* store segment frame */
	sr_status = fdma_store_default_frame_data();
	/* Segment cannot be stored due to buffer pool depletion.
	 * Discard the frame and report the user. */
	if (sr_status != SUCCESS){
		fdma_discard_default_frame(FDMA_DIS_NO_FLAGS);
		gro_ctx->internal_flags |= TCP_GRO_DISCARD_SEG_SET ;
	}

	/* replace between default_FD and agg_FD (segment <--> agg frame) */
	tmp_fd = *((struct ldpaa_fd *)HWC_FD_ADDRESS);
	*((struct ldpaa_fd *)HWC_FD_ADDRESS) = gro_ctx->agg_fd;
	gro_ctx->agg_fd = tmp_fd;

	/* present frame (default_FD(agg_FD)) +  present header */
	fdma_present_default_frame();

	/* run parser if headers were changed */
	if ((gro_ctx->agg_headers_size != headers_size) ||
	    (old_agg_timestamp !=
		(gro_ctx->internal_flags & TCP_GRO_HAS_TIMESTAMP))) {
		parse_result_generate_default(PARSER_NO_FLAGS);
		tcp = (struct tcphdr *)(PARSER_GET_L4_POINTER_DEFAULT());
	}

	/* update last segment header fields */
	if (gro_ctx->metadata.seg_num > 1)
		*((struct tcp_gro_last_seg_header_fields *)
				(&(tcp->acknowledgment_number))) =
						gro_ctx->last_seg_fields;

	/* write metadata to external memory */
	cdma_write((gro_ctx->params.metadata_addr + METADATA_MEMBER1_SIZE),
			&(gro_ctx->metadata.seg_num),
			(uint16_t)(METADATA_MEMBER2_SIZE +
					METADATA_MEMBER3_SIZE));

	/* update IP length + checksum */
	outer_ip_offset = (uint16_t)PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	ip_length = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS) -
			outer_ip_offset;
	if (PARSER_IS_OUTER_IPV4_DEFAULT()) {
		/* IPv4 */
		ipv4 = (struct ipv4hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		if (gro_ctx->flags & TCP_GRO_CALCULATE_IP_CHECKSUM)
			cksum_update_uint32(&(ipv4->hdr_cksum),
					ipv4->total_length, ip_length);
			ipv4->total_length = ip_length;
	} else {
		/* IPv6 */
		ipv6 = (struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		ipv6->payload_length = ip_length - IPV6_HDR_LENGTH;
	}

	/* Zero TCP checksum before calculating new TCP checksum. */
	if (gro_ctx->flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp->checksum = 0;
	/* Save headers changes to FDMA */
	fdma_modify_default_segment_data(outer_ip_offset, (uint16_t)
			(PARSER_GET_L4_OFFSET_DEFAULT() +
					TCP_HDR_LENGTH - outer_ip_offset));

	/* update TCP checksum for the aggregated frame */
	if (gro_ctx->flags & TCP_GRO_CALCULATE_TCP_CHECKSUM) {
		tcp_gro_calc_tcp_header_cksum();
	}

	if (gro_ctx->internal_flags &
			(TCP_GRO_FLUSH_AGG_SET | TCP_GRO_DISCARD_SEG_SET)) {
		/* update statistics */
		ste_inc_counter(gro_ctx->params.stats_addr +
			GRO_STAT_AGG_NUM_CNTR_OFFSET,
			1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
		/* delete the timer since the new segment will be flushed as is
		 * without additional segments. */
		tman_delete_timer(gro_ctx->timer_handle,
				TMAN_TIMER_DELETE_MODE_WO_EXPIRATION);

		gro_ctx->timer_handle = TCP_GRO_INVALID_TMAN_HANDLE;
		gro_ctx->timestamp = 0;
		/* Clear gross running sum in parse results */
		pr->gross_running_sum = 0;
		/* If the segment requires a flush but it was already discarded,
		 * report the discard since a flush is not required anymore. */
		/* Report to the user the segment was discarded. */
		if (gro_ctx->internal_flags & TCP_GRO_DISCARD_SEG_SET){
			ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_AGG_DISCARDED_SEG_NUM_CNTR_OFFSET, 1,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
			gro_ctx->metadata.seg_num = 0;
			gro_ctx->internal_flags = 0;
			return TCP_GRO_SEG_AGG_DONE | TCP_GRO_SEG_DISCARDED;
		} else { /* Report to the user a flush is required. */
			gro_ctx->metadata.seg_num = 1;
			return TCP_GRO_SEG_AGG_DONE | TCP_GRO_FLUSH_REQUIRED;
		}
	}

	/* Done handling aggregated packet.
	 * Finalize new aggregation. */

	/* recharge timer for the new aggregation */
	sr_status = tman_recharge_timer(gro_ctx->timer_handle);

	if (sr_status != SUCCESS) {
		sr_status = tman_create_timer(
				params->timeout_params.tmi_id,
				(uint32_t)((params->timeout_params.granularity
						<< GRO_GRAN_OFFSET) |
						TMAN_CREATE_TIMER_ONE_SHOT),
				params->limits.timeout_limit,
				tcp_gro_context_addr,
				0,
				&tcp_gro_timeout_callback,
				&(gro_ctx->timer_handle));

		/* No more timers available.
		 * Report the user this segment should be flushed due to
		 * timer unavailability. */
		if (sr_status == -ENAVAIL){

			/* update statistics */
			ste_inc_counter(gro_ctx->params.stats_addr +
				GRO_STAT_AGG_NUM_CNTR_OFFSET, 1,
				STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);

			gro_ctx->timer_handle = TCP_GRO_INVALID_TMAN_HANDLE;
			gro_ctx->metadata.seg_num = 1;
			/* Clear gross running sum in parse results */
			pr->gross_running_sum = 0;
			return TCP_GRO_SEG_AGG_DONE | TCP_GRO_FLUSH_REQUIRED |
					TCP_GRO_TIMER_UNAVAIL;
		}
	}
	/* update statistics */
	ste_inc_and_acc_counters(params->stats_addr +
			GRO_STAT_AGG_NUM_CNTR_OFFSET, 1,
			STE_MODE_COMPOUND_32_BIT_CNTR_SIZE |
			STE_MODE_COMPOUND_32_BIT_ACC_SIZE |
			STE_MODE_COMPOUND_CNTR_SATURATE |
			STE_MODE_COMPOUND_ACC_SATURATE);

	/* initialize gro context fields */
	gro_ctx->params = *params;
	gro_ctx->metadata.seg_num = 1;
	gro_ctx->metadata.max_seg_size = seg_size;

	/* update seg size */
	if (gro_ctx->flags & TCP_GRO_METADATA_SEGMENT_SIZES) {
		cdma_read(&(gro_ctx->metadata.seg_sizes_addr),
				params->metadata_addr ,
				(uint16_t)METADATA_MEMBER1_SIZE);
		cdma_write(gro_ctx->metadata.seg_sizes_addr,
				&(gro_ctx->metadata.max_seg_size),
				(uint16_t)
					sizeof(gro_ctx->metadata.max_seg_size));
		gro_ctx->metadata.seg_sizes_addr += (uint16_t)
				sizeof(gro_ctx->metadata.max_seg_size);
	}

	/* Clear gross running sum in parse results */
	pr->gross_running_sum = 0;

	return TCP_GRO_SEG_AGG_DONE_AGG_OPEN_NEW_AGG;
}

int32_t tcp_gro_flush_aggregation(
		uint64_t tcp_gro_context_addr)
{
	struct tcp_gro_context gro_ctx;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	struct tcphdr *tcp;
	struct ipv4hdr *ipv4;
	struct ipv6hdr *ipv6;
	uint16_t ip_length, outer_ip_offset;
	uint8_t single_seg;

	/* read GRO context*/
	cdma_read_with_mutex(tcp_gro_context_addr,
			CDMA_PREDMA_MUTEX_WRITE_LOCK,
			(void *)(&gro_ctx),
			(uint16_t)sizeof(struct tcp_gro_context));
	/* no aggregation */
	if (gro_ctx.metadata.seg_num == 0) {
		cdma_mutex_lock_release(tcp_gro_context_addr);
		return TCP_GRO_FLUSH_NO_AGG;
	}

	/* delete the timer for this aggregation */
	tman_delete_timer(gro_ctx.timer_handle,
			TMAN_TIMER_DELETE_MODE_WO_EXPIRATION);

	gro_ctx.timer_handle = TCP_GRO_INVALID_TMAN_HANDLE;

	single_seg = (gro_ctx.metadata.seg_num == 1) ? 1 : 0;

	/* prepare presentation context fields for frame presentation. */
	PRC_SET_SEGMENT_ADDRESS((uint32_t)TLS_SECTION_END_ADDR +
				DEFAULT_SEGMENT_HEADOOM_SIZE);
	PRC_SET_SEGMENT_LENGTH(DEFAULT_SEGMENT_SIZE);
	PRC_SET_SEGMENT_OFFSET(0);
	PRC_RESET_SR_BIT();
	PRC_RESET_NDS_BIT();
	PRC_SET_ASA_SIZE(0);
	PRC_SET_PTA_ADDRESS(PRC_PTA_NOT_LOADED_ADDRESS);
	set_default_amq_attributes(&(gro_ctx.agg_fd_isolation_attributes));

	if (gro_ctx.internal_flags & TCP_GRO_FLUSH_AGG_SET) {
		/* reset gro context fields */
		gro_ctx.metadata.seg_num = 0;
		gro_ctx.internal_flags = 0;
		gro_ctx.timestamp = 0;
		/* write gro context back to DDR + release mutex */
		cdma_write_with_mutex(tcp_gro_context_addr,
				CDMA_POSTDMA_MUTEX_RM_BIT,
				(void *)&gro_ctx,
				(uint16_t)sizeof(struct tcp_gro_context));
		/* Copy aggregated FD to default FD location and prepare
		 * aggregated FD parameters in Presentation Context */
		*((struct ldpaa_fd *)HWC_FD_ADDRESS) = gro_ctx.agg_fd;
		fdma_present_default_frame();
		/* Clear gross running sum in parse results */
		pr->gross_running_sum = 0;

		return TCP_GRO_FLUSH_AGG_DONE;
	}

	/* write metadata to external memory */
	cdma_write((gro_ctx.params.metadata_addr + METADATA_MEMBER1_SIZE),
		&(gro_ctx.metadata.seg_num),
		(uint16_t)(METADATA_MEMBER2_SIZE +
				METADATA_MEMBER3_SIZE));
	/* reset gro context fields */
	gro_ctx.metadata.seg_num = 0;
	gro_ctx.internal_flags = 0;
	gro_ctx.timestamp = 0;

	/* Copy aggregated FD to default FD location and prepare aggregated FD
	 * parameters in Presentation Context */
	*((struct ldpaa_fd *)HWC_FD_ADDRESS) = gro_ctx.agg_fd;
	fdma_present_default_frame();

	/* run parser since we don't know which scenario preceded
	 * the flush call */
	parse_result_generate_default(PARSER_NO_FLAGS);

	/* update IP length + checksum */
	outer_ip_offset = (uint16_t)PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	ip_length = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS) -
			outer_ip_offset;
	if (PARSER_IS_OUTER_IPV4_DEFAULT()) {
		ipv4 = (struct ipv4hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		if (gro_ctx.flags & TCP_GRO_CALCULATE_IP_CHECKSUM)
			cksum_update_uint32(&(ipv4->hdr_cksum),
					ipv4->total_length, ip_length);
		ipv4->total_length = ip_length;
	} else {
		ipv6 = (struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		ipv6->payload_length = ip_length - IPV6_HDR_LENGTH;
	}

	/* update TCP length + checksum */
	/* update last segment header fields */
	tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();
	if (!single_seg)
		*((struct tcp_gro_last_seg_header_fields *)
			(&(tcp->acknowledgment_number))) =
					gro_ctx.last_seg_fields;

	/* Zero TCP checksum before calculating new TCP checksum. */
	if (gro_ctx.flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp->checksum = 0;
	/* Save headers changes to FDMA */
	fdma_modify_default_segment_data(outer_ip_offset, (uint16_t)
	   (PARSER_GET_L4_OFFSET_DEFAULT() + TCP_HDR_LENGTH - outer_ip_offset));

	if (gro_ctx.flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp_gro_calc_tcp_header_cksum();

	/* write gro context back to DDR + release mutex */
	cdma_write_with_mutex(tcp_gro_context_addr,
				CDMA_POSTDMA_MUTEX_RM_BIT,
				(void *)&gro_ctx,
				(uint16_t)sizeof(struct tcp_gro_context));

	/* update statistics */
	ste_inc_counter(gro_ctx.params.stats_addr + GRO_STAT_AGG_NUM_CNTR_OFFSET
			, 1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
	if (gro_ctx.flags & TCP_GRO_EXTENDED_STATS_EN)
		ste_inc_counter(gro_ctx.params.stats_addr +
			GRO_STAT_AGG_FLUSH_REQUEST_NUM_CNTR_OFFSET,
			1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);

	/* Clear gross running sum in parse results */
	pr->gross_running_sum = 0;

	return TCP_GRO_FLUSH_AGG_DONE;
}

/* TCP GRO timeout callback */
void tcp_gro_timeout_callback(uint64_t tcp_gro_context_addr, uint16_t opaque2)
{
	struct tcp_gro_context gro_ctx;
	struct tcphdr *tcp;
	struct ipv4hdr *ipv4;
	struct ipv6hdr *ipv6;
	uint16_t ip_length, outer_ip_offset;
	uint8_t single_seg;
	uint32_t timer_handle;

	opaque2 = 0;
	/* read GRO context*/
	cdma_read_with_mutex(tcp_gro_context_addr,
			CDMA_PREDMA_MUTEX_WRITE_LOCK,
			(void *)(&gro_ctx),
			(uint16_t)sizeof(struct tcp_gro_context));

	timer_handle = TMAN_GET_TIMER_HANDLE(HWC_FD_ADDRESS);
	if (gro_ctx.timer_handle !=
		(timer_handle & TIMER_HANDLE_MASK)) {
		cdma_mutex_lock_release(tcp_gro_context_addr);
		return;
	}

	/* confirm timer expiration */
	tman_timer_completion_confirmation(gro_ctx.timer_handle);

	/* no aggregation */
	if (gro_ctx.metadata.seg_num == 0) {
		cdma_mutex_lock_release(tcp_gro_context_addr);
		return;
	}

	single_seg = (gro_ctx.metadata.seg_num == 1) ? 1 : 0;

	/* write metadata to external memory */
	cdma_write((gro_ctx.params.metadata_addr + METADATA_MEMBER1_SIZE),
			&(gro_ctx.metadata.seg_num),
			(uint16_t)(METADATA_MEMBER2_SIZE +
					METADATA_MEMBER3_SIZE));

	/* reset gro context fields */
	gro_ctx.metadata.seg_num = 0;
	gro_ctx.internal_flags = 0;
	gro_ctx.timestamp = 0;

	/* Copy aggregated FD to default FD location and prepare aggregated FD
	 * parameters in Presentation Context */
	*((struct ldpaa_fd *)HWC_FD_ADDRESS) = gro_ctx.agg_fd;
	PRC_SET_SEGMENT_ADDRESS((uint32_t)TLS_SECTION_END_ADDR +
			DEFAULT_SEGMENT_HEADOOM_SIZE);
	PRC_SET_SEGMENT_LENGTH(DEFAULT_SEGMENT_SIZE);
	PRC_SET_SEGMENT_OFFSET(0);
	PRC_RESET_SR_BIT();
	PRC_RESET_NDS_BIT();
	PRC_SET_ASA_SIZE(0);
	PRC_SET_PTA_ADDRESS(PRC_PTA_NOT_LOADED_ADDRESS);
	set_default_amq_attributes(&(gro_ctx.agg_fd_isolation_attributes));
	fdma_present_default_frame();

	/* run parser */
	parse_result_generate_default(PARSER_NO_FLAGS);

	/* update IP length + checksum */
	outer_ip_offset = (uint16_t)PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	ip_length = (uint16_t)LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS) -
			outer_ip_offset;
	if (PARSER_IS_OUTER_IPV4_DEFAULT()) {
		ipv4 = (struct ipv4hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		if (gro_ctx.flags & TCP_GRO_CALCULATE_IP_CHECKSUM)
			cksum_update_uint32(&(ipv4->hdr_cksum),
					ipv4->total_length, ip_length);
		ipv4->total_length = ip_length;
	} else {
		ipv6 = (struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
		ipv6->payload_length = ip_length - IPV6_HDR_LENGTH;
	}

	/* update TCP length + checksum */
	/* update last segment header fields */
	tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();
	if (!single_seg)
		*((struct tcp_gro_last_seg_header_fields *)
			(&(tcp->acknowledgment_number))) =
				gro_ctx.last_seg_fields;

	/* Zero TCP checksum before calculating new TCP checksum. */
	if (gro_ctx.flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp->checksum = 0;
	/* Save headers changes to FDMA */
	fdma_modify_default_segment_data(outer_ip_offset, (uint16_t)
	   (PARSER_GET_L4_OFFSET_DEFAULT() + TCP_HDR_LENGTH - outer_ip_offset));

	if (gro_ctx.flags & TCP_GRO_CALCULATE_TCP_CHECKSUM)
		tcp_gro_calc_tcp_header_cksum();

	/* write gro context back to DDR + release mutex */
	cdma_write_with_mutex(tcp_gro_context_addr,
				CDMA_POSTDMA_MUTEX_RM_BIT,
				(void *)&gro_ctx,
				(uint16_t)sizeof(struct tcp_gro_context));

	/* update statistics */
	ste_inc_counter(gro_ctx.params.stats_addr + GRO_STAT_AGG_NUM_CNTR_OFFSET
			, 1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
	if (gro_ctx.flags & TCP_GRO_EXTENDED_STATS_EN)
		ste_inc_counter(gro_ctx.params.stats_addr +
			GRO_STAT_AGG_TIMEOUT_CNTR_OFFSET,
			1, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);

	/* call user callback function*/
	gro_ctx.params.timeout_params.gro_timeout_cb(
			gro_ctx.params.timeout_params.gro_timeout_cb_arg);
}

void tcp_gro_calc_tcp_header_cksum()
{
	uint8_t tcp_header_length;
	uint16_t tmp_checksum, tcp_offset, pseudo_tcp_length, ipsrc_offset;
	struct tcphdr *tcp;
	struct ipv4hdr *ipv4;
	struct ipv6hdr *ipv6;

	tcp = (struct tcphdr *)PARSER_GET_L4_POINTER_DEFAULT();
	ipv4 = (struct ipv4hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();
	ipv6 = (struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT();

	/* offset to TCP header */
	tcp_offset = (uint16_t)(PARSER_GET_L4_OFFSET_DEFAULT());
	/* TCP header length */
	tcp_header_length = (tcp->data_offset_reserved &
				NET_HDR_FLD_TCP_DATA_OFFSET_MASK) >>
				(NET_HDR_FLD_TCP_DATA_OFFSET_OFFSET -
				NET_HDR_FLD_TCP_DATA_OFFSET_SHIFT_VALUE);

	if (PARSER_IS_OUTER_IPV4_DEFAULT()) {
		/* calculate IP source address offset  */
		ipsrc_offset = (uint16_t)(tcp_offset - IPV4_HDR_ADD_LENGTH);
		/* calculate TCP length for the pseudo header  */
		pseudo_tcp_length = ipv4->total_length - IPV4_HDR_LENGTH;

	} else {
		/* calculate IP source address offset  */
		ipsrc_offset = (uint16_t)(tcp_offset - IPV6_HDR_ADD_LENGTH);
		/* calculate TCP length for the pseudo header  */
		pseudo_tcp_length = ipv6->payload_length;
	}

	/* calculate IPsrc + IPdst + TCP header + Data checksum  */
	fdma_calculate_default_frame_checksum(
			ipsrc_offset, 0xffff, &(tmp_checksum));
	/* Add TCP length */
	tmp_checksum = cksum_ones_complement_sum16(tmp_checksum,
			pseudo_tcp_length);
	/* Add TCP Protocol number for the pseudo header and finalize TCP
	 * checksum. */
	tcp->checksum = (uint16_t)~cksum_ones_complement_sum16(
			tmp_checksum, TCP_PROTOCOL);

	/* Modify default segment (TCP checksum = 0) */
	fdma_modify_default_segment_data(
			tcp_offset +
			(uint16_t)offsetof(struct tcphdr, checksum),
			(uint16_t)(sizeof(tcp->checksum)));
}
