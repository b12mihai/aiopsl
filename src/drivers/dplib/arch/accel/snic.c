/**************************************************************************//**
@File		snic.c

@Description	This file contains the AIOP snic source code.

		Copyright 2013 Freescale Semiconductor, Inc.
*//***************************************************************************/

#include "snic.h"
#include "system.h"
#include "net/fsl_net.h"
#include "common/fsl_stdio.h"
#include "common/fsl_string.h"
#include "fsl_errors.h"
#include "kernel/platform.h"
#include "fsl_io.h"
#include "aiop_common.h"
#include "fsl_parser.h"
#include "general.h"
#include "fsl_dbg.h"

#include "dplib/fsl_parser.h"
#include "dplib/fsl_l2.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_dplib_sys.h"

#include "general.h"
#include "osm.h"

#include "dplib/fsl_ipf.h"
#include "common/fsl_cmdif_server.h"

#define SNIC_CMD_READ(_param, _offset, _width, _type, _arg) \
	_arg = (_type)u64_dec(cmd_data->params[_param], _offset, _width);

#define SNIC_RSP_PREP(_param, _offset, _width, _type, _arg) \
	cmd_data->params[_param] |= u64_enc(_offset, _width, _arg);

/** This is where FQD CTX should reside */
#define FQD_CTX_GET \
	(((struct additional_dequeue_context *)HWC_ADC_ADDRESS)->fqd_ctx)
/** Get sNIC ID from dequeue context */
#define SNIC_ID_GET \
	(uint16_t)(LLLDW_SWAP((uint32_t)&FQD_CTX_GET, 0) & 0xFFFF)
/** Get sNIC modes from dequeue context */
#define SNIC_IS_INGRESS_GET \
	(uint32_t)(LLLDW_SWAP((uint32_t)&FQD_CTX_GET, 0) & 0x80000000)

extern __TASK struct aiop_default_task_params default_task_params;

__SHRAM struct snic_params snic_params[MAX_SNIC_NO];

__HOT_CODE void snic_process_packet(void)
{

	struct parse_result *pr;
	struct snic_params *snic;
	struct fdma_queueing_destination_params enqueue_params;
	int err;
	int32_t parse_status;

	pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;

	/* Need to save running-sum in parse-results LE-> BE */
	pr->gross_running_sum = LH_SWAP(HWC_FD_ADDRESS + FD_FLC_RUNNING_SUM, 0);

	osm_task_init();
	/* todo: spid=0?, prpid=0?, starting HXS=0?*/
	*((uint8_t *)HWC_SPID_ADDRESS) = SNIC_SPID;
	default_task_params.parser_profile_id = SNIC_PRPID;
	default_task_params.parser_starting_hxs = SNIC_HXS;

	parse_status = parse_result_generate_default(PARSER_NO_FLAGS);
	if (parse_status){
		fdma_discard_default_frame(FDMA_DIS_NO_FLAGS);
		fdma_terminate_task();
	}

	/* get sNIC ID */
	snic = snic_params + SNIC_ID_GET;

	if (SNIC_IS_INGRESS_GET) {
		/* snic uses only 1 QDID so we need to have different
		 * qd/priority for ingress than for egress */
		default_task_params.qd_priority = 8;
		/* For ingress may need to do IPR and then Remove Vlan */
		if (snic->snic_enable_flags & SNIC_IPR_EN)
			err = snic_ipr(snic);
		/*reach here if re-assembly success or regular or IPR disabled*/
		if (snic->snic_enable_flags & SNIC_VLAN_REMOVE_EN)
			l2_pop_vlan();

	}
	/* Egress*/
	else {
		default_task_params.qd_priority = ((*((uint8_t *)
				(HWC_ADC_ADDRESS +
				ADC_WQID_PRI_OFFSET)) & ADC_WQID_MASK) >> 4);
		/* For Egress may need to do add Vlan and then IPF */
		if (snic->snic_enable_flags & SNIC_VLAN_ADD_EN)
			snic_add_vlan();

		if (snic->snic_enable_flags & SNIC_IPF_EN)
			err = snic_ipf(snic);
	}

	/* for the enqueue set hash from TLS, an flags equal 0 meaning that \
	 * the qd_priority is taken from the TLS and that enqueue function \
	 * always returns*/
	enqueue_params.qdbin = 0;
	enqueue_params.qd = snic->qdid;
	enqueue_params.qd_priority = default_task_params.qd_priority;
	/* todo error cases */
	err = (int)fdma_store_and_enqueue_default_frame_qd(&enqueue_params, \
			FDMA_ENWF_NO_FLAGS);
	fdma_terminate_task();
}


/* Assuming IPF is the last iteration before enqueue (IPF after encryption)*/
int snic_ipf(struct snic_params *snic)
{
	uint16_t ip_offset;
	uint32_t total_length;
	struct ipv4hdr *ipv4_hdr;
	struct ipv6hdr *ipv6_hdr;
	struct fdma_amq amq;
	ipf_ctx_t ipf_context_addr
		__attribute__((aligned(sizeof(struct ldpaa_fd))));
	uint16_t icid;
	uint32_t flags = 0;
	uint8_t va_bdi;
	int32_t ipf_status;
	int err;
	struct fdma_queueing_destination_params enqueue_params;

	ip_offset = PARSER_GET_OUTER_IP_OFFSET_DEFAULT();
	ipv4_hdr = (struct ipv4hdr *)
			(ip_offset + PRC_GET_SEGMENT_ADDRESS());
	/* need to check frame size against MTU */
	if (PARSER_IS_OUTER_IPV6_DEFAULT())
	{
		ipv6_hdr = (struct ipv6hdr *)ipv4_hdr;
		total_length =
			(uint32_t)(ipv6_hdr->payload_length
					+ 40);
	}
	else
		total_length = (uint32_t)ipv4_hdr->total_length;

	if (total_length > snic->snic_ipf_mtu)
	{
		icid = LH_SWAP(HWC_ADC_ADDRESS +
				ADC_PL_ICID_OFFSET, 0);
		icid &= ADC_ICID_MASK;
		va_bdi = *((uint8_t *)(HWC_ADC_ADDRESS +
				ADC_FDSRC_VA_FCA_BDI_OFFSET));
		if (va_bdi & ADC_BDI_MASK)
			flags |= FDMA_ENF_BDI_BIT;
		amq.flags = (uint16_t)(flags >> 16);
		amq.icid = icid;

		ipf_context_init(0, snic->snic_ipf_mtu,
				ipf_context_addr);

		do {
			ipf_status =
			ipf_generate_frag(ipf_context_addr);
			if (ipf_status)
				err =
				(int)fdma_store_frame_data(1, 0,
						&amq);
			else
				err =
				(int)fdma_store_frame_data(0, 0,
						&amq);

			/* for the enqueue set hash from TLS,
			 * an flags equal 0 meaning that
			 * the qd_priority is taken from the
			 * TLS and that enqueue function
			 * always returns*/
			enqueue_params.qdbin = 0;
			enqueue_params.qd = snic->qdid;
			enqueue_params.qd_priority =
				default_task_params.qd_priority;
			/* todo error cases */

			err =
			(int)fdma_enqueue_default_fd_qd(icid,
				flags, &enqueue_params);

		} while (ipf_status);

		fdma_terminate_task();
		return 0;
	}
	else
		return 0;
}

int snic_ipr(struct snic_params *snic)
{
	int32_t reassemble_status;

	reassemble_status = ipr_reassemble(snic->ipr_instance_val);
	if (reassemble_status != IPR_REASSEMBLY_REGULAR &&
		reassemble_status != IPR_REASSEMBLY_SUCCESS)
	{
		/* todo: error cases*/
		fdma_terminate_task();
		return 0;
	}

	else
		return 0;
}

int snic_add_vlan(void)
{
	uint32_t vlan;
	struct presentation_context *presentation_context;
	uint32_t asa_seg_addr;	/* ASA Segment Address */

	/* Get ASA pointer */
	presentation_context =
		(struct presentation_context *) HWC_PRC_ADDRESS;
	asa_seg_addr = (uint32_t)(presentation_context->
			asapa_asaps & PRC_ASAPA_MASK);
	vlan = *((uint32_t *)(PTR_MOVE(asa_seg_addr, 0x50)));
	l2_push_and_set_vlan(vlan);
	return 0;
}

static int snic_open_cb(void *dev)
{
	/* TODO: */
	UNUSED(dev);
	return 0;
}


static int snic_close_cb(void *dev)
{
	/* TODO: */
	UNUSED(dev);
	return 0;
}

static int snic_ctrl_cb(void *dev, uint16_t cmd, uint16_t size, uint64_t data)
{
	ipr_instance_handle_t ipr_instance = 0;
	ipr_instance_handle_t *ipr_instance_ptr = &ipr_instance;
	uint16_t snic_id=0xFFFF, ipf_mtu, snic_flags, qdid;
	int i;
	struct snic_cmd_data *cmd_data = (struct snic_cmd_data *)PRC_GET_SEGMENT_ADDRESS();
	struct ipr_params ipr_params;
	uint32_t snic_ep_pc;

	UNUSED(dev);
	UNUSED(data);

	switch(cmd)
	{
	case SNIC_IPR_CREATE_INSTANCE:
		SNIC_IPR_CREATE_INSTANCE_CMD(SNIC_CMD_READ);

		ipr_create_instance(&ipr_params,
				ipr_instance_ptr);
		snic_params[snic_id].ipr_instance_val = ipr_instance;
		return 0;
	case SNIC_IPR_DELETE_INSTANCE:
		/* todo: parameters to ipr_delete_instance */
		SNIC_IPR_DELETE_INSTANCE_CMD(SNIC_CMD_READ);
		ipr_delete_instance(snic_params[snic_id].ipr_instance_val,
				NULL, NULL);
		return 0;
	case SNIC_SET_MTU:
		SNIC_CMD_MTU(SNIC_CMD_READ);
		snic_params[snic_id].snic_ipf_mtu = ipf_mtu;
		return 0;
	case SNIC_ENABLE_FLAGS:
		SNIC_ENABLE_FLAGS_CMD(SNIC_CMD_READ);
		snic_params[snic_id].snic_enable_flags = snic_flags;
		return 0;
	case SNIC_SET_QDID:
		SNIC_SET_QDID_CMD(SNIC_CMD_READ);
		snic_params[snic_id].qdid = qdid;
		return 0;
	case SNIC_REGISTER:
		snic_ep_pc = (uint32_t)snic_process_packet;
		for (i=0; i < MAX_SNIC_NO; i++)
		{
			if (!snic_params[i].valid)
			{
				snic_params[i].valid = TRUE;
				snic_id = (uint16_t)i;
				break;
			}
		}
		SNIC_REGISTER_CMD(SNIC_RSP_PREP);
		fdma_modify_default_segment_data(0, size);
		return 0;
	case SNIC_UNREGISTER:
		SNIC_UNREGISTER_CMD(SNIC_CMD_READ);
		memset(&snic_params[snic_id], 0, sizeof(struct snic_params));
		return 0;
	default:
		return -EINVAL;
	}
	return 0;
}

int aiop_snic_init(void)
{
	int status;
	struct cmdif_module_ops snic_cmd_ops;

	snic_cmd_ops.open_cb = (open_cb_t *)snic_open_cb;
	snic_cmd_ops.close_cb = (close_cb_t *)snic_close_cb;
	snic_cmd_ops.ctrl_cb = (ctrl_cb_t *)snic_ctrl_cb;
	pr_info("sNIC: register with cmdif module!\n");
	status = cmdif_register_module("sNIC", &snic_cmd_ops);
	if(status) {
		pr_info("sNIC:Failed to register with cmdif module!\n");
		return status;
	}
	memset(snic_params, 0, sizeof(snic_params));
	return 0;
}
