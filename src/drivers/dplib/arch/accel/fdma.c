/**************************************************************************//**
@File		fdma.c

@Description	This file contains the AIOP SW Frame Operations API
		implementation.

*//***************************************************************************/

#include "dplib/fsl_fdma.h"
#include "fdma.h"

int fdma_present_default_frame(void)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
		(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	int8_t  res1;
	uint32_t flags = 0;

	/* prepare command parameters */
	if (PRC_GET_NDS_BIT())
		flags |= FDMA_INIT_NDS_BIT;
	else
		arg2 = FDMA_INIT_CMD_ARG2((uint32_t)(prc->seg_address),
						prc->seg_offset);
	/* Todo - when compiler fixes bug C43 enable the following code
	 * instead of the 3 lines of code afterwards.
	if (prc->asapa_asaps & PRC_ASAPS_MASK)
		arg4 = FDMA_INIT_CMD_ARG4(prc->asapa_asaps & ~PRC_SR_MASK);
	else
		flags |= FDMA_INIT_NAS_BIT;*/
	if (PRC_GET_ASA_SIZE() == 0)
		flags |= FDMA_INIT_NAS_BIT;
	arg4 = FDMA_INIT_CMD_ARG4(prc->asapa_asaps &
			~(PRC_SR_MASK|PRC_NDS_MASK));


	if (PRC_GET_PTA_ADDRESS() == PRC_PTA_NOT_LOADED_ADDRESS)
		flags |= FDMA_INIT_NPS_BIT;

	arg1 = FDMA_INIT_CMD_ARG1(HWC_FD_ADDRESS,
			(flags | PRC_GET_SR_BIT_FDMA_CMD()));
	arg3 = FDMA_INIT_CMD_ARG3(prc->seg_length, prc->ptapa_asapo);


	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if ((res1 == FDMA_SUCCESS) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR)) {
		prc->seg_length = *((uint16_t *) (HWC_ACC_OUT_ADDRESS2));
		prc->handles =
			(((*((uint8_t *)
			(HWC_ACC_OUT_ADDRESS2 + FDMA_FRAME_HANDLE_OFFSET))) <<
			PRC_FRAME_HANDLE_BIT_OFFSET) &
			PRC_FRAME_HANDLE_MASK) |
			((*((uint8_t *)
			(HWC_ACC_OUT_ADDRESS2 + FDMA_SEG_HANDLE_OFFSET))) &
			PRC_SEGMENT_HANDLE_MASK);


		if ((res1 == FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR))
			prc->asapa_asaps = (prc->asapa_asaps & ~PRC_ASAPS_MASK)
				| (uint16_t)(LDPAA_FD_GET_ASAL(HWC_FD_ADDRESS));

#if NAS_NPS_ENABLE
		(flags & FDMA_INIT_NAS_BIT) ? PRC_SET_NAS_BIT() :
				PRC_RESET_NAS_BIT();
		(flags & FDMA_INIT_NPS_BIT) ? PRC_SET_NPS_BIT() :
				PRC_RESET_NPS_BIT();
#endif /*NAS_NPS_ENABLE*/

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else /* FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR or
			FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR*/
			return -EIO;
	}

	if (res1 == FDMA_FD_ERR)
		return -EBADFD;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_present_frame(
		struct fdma_present_frame_params *params)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
		(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	int8_t  res1;

	/* prepare command parameters */
	if (!(params->flags & FDMA_INIT_NDS_BIT))
		arg2 = FDMA_INIT_CMD_ARG2((uint32_t)(params->seg_dst),
					params->seg_offset);
	if (params->asa_size == 0)
		params->flags |= FDMA_INIT_NAS_BIT;

	if (params->flags & FDMA_INIT_AS_BIT)
		arg4 = FDMA_INIT_EXP_AMQ_CMD_ARG4(
				params->flags,
				params->icid,
				params->asa_dst,
				params->asa_size);
	else
		arg4 = FDMA_INIT_EXP_CMD_ARG4(params->asa_dst,
				params->asa_size);

	if ((uint32_t)(params->pta_dst) == PRC_PTA_NOT_LOADED_ADDRESS)
		params->flags |= FDMA_INIT_NPS_BIT;

	arg1 = FDMA_INIT_CMD_ARG1(((uint32_t)params->fd_src),
			(params->flags & ~FDMA_INIT_BDI_BIT));
	/*arg2 = FDMA_INIT_CMD_ARG2((uint32_t)(params->seg_address),
						params->seg_offset);*/
	arg3 = FDMA_INIT_EXP_CMD_ARG3(params->present_size, params->pta_dst,
				params->asa_offset);
	/*arg4 = FDMA_INIT_EXP_CMD_ARG4(params->asa_address,
					params->asa_size);*/


	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if ((int32_t)res1 == FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR)
		arg4 = (arg4 & ~PRC_ASAPS_MASK) |
			(uint32_t)(LDPAA_FD_GET_ASAL(HWC_FD_ADDRESS));

	if ((res1 == FDMA_SUCCESS) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR)) {

		params->frame_handle = *((uint8_t *)
			(HWC_ACC_OUT_ADDRESS2 + FDMA_FRAME_HANDLE_OFFSET));
		if (!(params->flags & FDMA_INIT_NDS_BIT)) {
			params->seg_length = *((uint16_t *)
					(HWC_ACC_OUT_ADDRESS2));
			params->seg_handle = *((uint8_t *)
					(HWC_ACC_OUT_ADDRESS2 +
					FDMA_SEG_HANDLE_OFFSET));
			PRC_RESET_NDS_BIT();
			/* Update Task Defaults */
			if (((uint32_t)params->fd_src) == HWC_FD_ADDRESS) {
				prc->seg_address = (uint16_t)
					((uint32_t)params->seg_dst);
				prc->seg_length = params->seg_length;
				prc->seg_offset = params->seg_offset;
				if (params->flags & FDMA_INIT_SR_BIT)
					PRC_SET_SR_BIT();
				else
					PRC_RESET_SR_BIT();
				prc->handles = ((params->frame_handle <<
						PRC_FRAME_HANDLE_BIT_OFFSET) &
						PRC_FRAME_HANDLE_MASK) |
						(params->seg_handle &
						PRC_SEGMENT_HANDLE_MASK);
				prc->ptapa_asapo = (uint16_t)arg3;
				prc->asapa_asaps =
					(prc->asapa_asaps & PRC_SR_MASK) |
					((uint16_t)arg4 & ~PRC_SR_MASK);
			}
		} else if (((uint32_t)params->fd_src) == HWC_FD_ADDRESS) {
			/* Todo - if NDS/NAS/NPS are added to prc update them */
			prc->handles = (params->frame_handle <<
				PRC_FRAME_HANDLE_BIT_OFFSET) &
				PRC_FRAME_HANDLE_MASK;
			prc->ptapa_asapo = (uint16_t)arg3;
			prc->asapa_asaps = (prc->asapa_asaps & PRC_SR_MASK) |
					((uint16_t)arg4 & ~PRC_SR_MASK);
		}


		if (params->flags & FDMA_INIT_NDS_BIT)
			PRC_SET_NDS_BIT();
#if NAS_NPS_ENABLE
		(params->flags & FDMA_INIT_NAS_BIT) ? PRC_SET_NAS_BIT() :
						PRC_RESET_NAS_BIT();
		(params->flags & FDMA_INIT_NPS_BIT) ? PRC_SET_NPS_BIT() :
						PRC_RESET_NPS_BIT();
#endif /*NAS_NPS_ENABLE*/

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else if (res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR)
			return -EIO;
		else /*FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR*/
			return -EIO;
	}

	if (res1 == FDMA_FD_ERR)
		return -EBADFD;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_present_default_frame_without_segments(void)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t  res1;

	/* prepare command parameters */
	arg1 = FDMA_INIT_CMD_ARG1((uint32_t)HWC_FD_ADDRESS, FDMA_INIT_NDS_BIT);

	/* store command parameters */
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;
	__stdw(PRC_PTA_NOT_LOADED_ADDRESS, 0, HWC_ACC_IN_ADDRESS3, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if (res1 == FDMA_SUCCESS) {
		PRC_SET_FRAME_HANDLE(*((uint8_t *)
			(HWC_ACC_OUT_ADDRESS2 + FDMA_FRAME_HANDLE_OFFSET)));
		PRC_SET_NDS_BIT();
		PRC_SET_ASA_SIZE(0);
		PRC_SET_PTA_ADDRESS(PRC_PTA_NOT_LOADED_ADDRESS);
#if NAS_NPS_ENABLE
		PRC_SET_NAS_BIT();
		PRC_SET_NPS_BIT();
#endif /*NAS_NPS_ENABLE*/

		return SUCCESS;
	}

	if (res1 == FDMA_FD_ERR)
		return -EBADFD;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_present_frame_without_segments(
		struct ldpaa_fd *fd,
		uint32_t flags,
		uint16_t icid,
		uint8_t *frame_handle)
{
	/* command parameters and results */
	uint32_t arg1, arg4;
	int8_t  res1;

	/* prepare command parameters */
	arg1 = FDMA_INIT_CMD_ARG1((uint32_t)fd, flags | FDMA_INIT_NDS_BIT);

	/* store command parameters */
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;

	if (flags & FDMA_INIT_AS_BIT)
		arg4 = FDMA_INIT_EXP_AMQ_CMD_ARG4(flags, icid, 0, 0);
	else
		arg4 = 0;

	__stdw(PRC_PTA_NOT_LOADED_ADDRESS, arg4, HWC_ACC_IN_ADDRESS3, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if (res1 == FDMA_SUCCESS) {
		*frame_handle = *((uint8_t *)
			(HWC_ACC_OUT_ADDRESS2 + FDMA_FRAME_HANDLE_OFFSET));
		if ((uint32_t)fd == HWC_FD_ADDRESS) {
			PRC_SET_FRAME_HANDLE(*frame_handle);
			PRC_SET_NDS_BIT();
			PRC_SET_ASA_SIZE(0);
			PRC_SET_PTA_ADDRESS(PRC_PTA_NOT_LOADED_ADDRESS);
#if NAS_NPS_ENABLE
			PRC_SET_NAS_BIT();
			PRC_SET_NPS_BIT();
#endif /*NAS_NPS_ENABLE*/
		}
		return SUCCESS;
	}

	if (res1 == FDMA_FD_ERR)
		return -EBADFD;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)res1;
}

int fdma_present_default_frame_segment(
		uint32_t flags,
		void	 *ws_dst,
		uint16_t offset,
		uint16_t present_size)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t  res1;

	/* This command may be invoked only on Data segment */
	/*if ((PRC_GET_SEGMENT_HANDLE() == FDMA_ASA_SEG_HANDLE) ||
	    (PRC_GET_SEGMENT_HANDLE() == FDMA_PTA_SEG_HANDLE))
		return FDMA_NO_DATA_SEGMENT_HANDLE;*/
	/* prepare command parameters */
	arg1 = FDMA_PRESENT_CMD_ARG1(PRC_GET_HANDLES(),
			(flags | FDMA_ST_DATA_SEGMENT_BIT));
	arg2 = FDMA_PRESENT_CMD_ARG2((uint32_t)ws_dst, offset);
	arg3 = FDMA_PRESENT_CMD_ARG3(present_size);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if ((res1 == FDMA_SUCCESS) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR)) {
		PRC_SET_SEGMENT_ADDRESS((uint16_t)(uint32_t)ws_dst);
		PRC_SET_SEGMENT_OFFSET(offset);
		PRC_SET_SEGMENT_LENGTH(*((uint16_t *)(HWC_ACC_OUT_ADDRESS2)));
		PRC_SET_SEGMENT_HANDLE(*((uint8_t *)(HWC_ACC_OUT_ADDRESS2 +
						FDMA_SEG_HANDLE_OFFSET)));
		PRC_RESET_NDS_BIT();
		if (flags & FDMA_PRES_SR_BIT)
			PRC_SET_SR_BIT();
		else
			PRC_RESET_SR_BIT();

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_present_default_frame_default_segment()
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t  res1;

	/* prepare command parameters */
	arg1 = FDMA_PRESENT_CMD_ARG1(PRC_GET_HANDLES(),
			(FDMA_ST_DATA_SEGMENT_BIT));
	arg2 = FDMA_PRESENT_CMD_ARG2((uint32_t)PRC_GET_SEGMENT_ADDRESS(),
			PRC_GET_SEGMENT_OFFSET());
	arg3 = FDMA_PRESENT_CMD_ARG3(PRC_GET_SEGMENT_LENGTH());
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if ((res1 == FDMA_SUCCESS) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR)) {
		PRC_SET_SEGMENT_LENGTH(*((uint16_t *)(HWC_ACC_OUT_ADDRESS2)));
		PRC_SET_SEGMENT_HANDLE(*((uint8_t *)(HWC_ACC_OUT_ADDRESS2 +
						FDMA_SEG_HANDLE_OFFSET)));
		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_present_frame_segment(
		struct fdma_present_segment_params *params)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t  res1;

	/* prepare command parameters */
	arg1 = FDMA_PRESENT_EXP_CMD_ARG1(params->frame_handle,
			(params->flags | FDMA_ST_DATA_SEGMENT_BIT));
	arg2 = FDMA_PRESENT_CMD_ARG2((uint32_t)params->ws_dst, params->offset);
	arg3 = FDMA_PRESENT_CMD_ARG3(params->present_size);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	params->seg_length = *((uint16_t *)(HWC_ACC_OUT_ADDRESS2));
	params->seg_handle = *((uint8_t *)(HWC_ACC_OUT_ADDRESS2 +
			FDMA_SEG_HANDLE_OFFSET));
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR)
		return -EIO;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_read_default_frame_asa(
		void	 *ws_dst,
		uint16_t offset,
		uint16_t present_size)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
		(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t  res1;

	/* prepare command parameters */
	arg1 = FDMA_PRESENT_CMD_ARG1(PRC_GET_HANDLES(),
			FDMA_ST_ASA_SEGMENT_BIT);
	arg2 = FDMA_PRESENT_CMD_ARG2((uint32_t)ws_dst, offset);
	arg3 = FDMA_PRESENT_CMD_ARG3(present_size);
	/* store command parameters */
	__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if ((res1 == FDMA_SUCCESS) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR)) {

		prc->asapa_asaps = (((uint16_t)prc->asapa_asaps) & PRC_SR_MASK)|
			((uint16_t)((uint32_t)ws_dst) & PRC_ASAPA_MASK) |
			((*((uint16_t *)HWC_ACC_OUT_ADDRESS2)) &
						PRC_ASAPS_MASK);
		PRC_SET_ASA_OFFSET(offset);
#if NAS_NPS_ENABLE
	PRC_RESET_NAS_BIT();
#endif /*NAS_NPS_ENABLE*/

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR)*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_read_default_frame_pta(
		void *ws_dst)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t  res1;

	if (((uint16_t)((uint32_t)ws_dst)) == PRC_PTA_NOT_LOADED_ADDRESS)
		fdma_handle_fatal_errors(FDMA_INVALID_PTA_ADDRESS);
	/* prepare command parameters */
	arg1 = FDMA_PRESENT_CMD_ARG1(PRC_GET_HANDLES(),
			FDMA_ST_PTA_SEGMENT_BIT);
	arg2 = FDMA_PRESENT_CMD_ARG2((uint16_t)((uint32_t)ws_dst), 0);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	if (res1 == FDMA_SUCCESS) {
		PRC_SET_PTA_ADDRESS((uint16_t)((uint32_t)ws_dst));
#if NAS_NPS_ENABLE
	PRC_RESET_NPS_BIT();
#endif /*NAS_NPS_ENABLE*/
		return SUCCESS;
	}

	if (res1 == FDMA_UNABLE_TO_PRESENT_PTA_ERR)
		return -EIO;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_extend_default_segment_presentation(
		uint16_t extend_size,
		void	 *ws_dst,
		uint32_t flags)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t  res1;
	/* prepare command parameters */
	if (flags & FDMA_ST_ASA_SEGMENT_BIT)
		arg1 = FDMA_EXTEND_CMD_ASA_ARG1(PRC_GET_HANDLES());
	else
		arg1 = FDMA_EXTEND_CMD_ARG1(PRC_GET_HANDLES());
	arg2 = FDMA_EXTEND_CMD_ARG2((uint32_t)ws_dst, extend_size);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	/* Update Task Defaults */
	if (((int32_t)res1) >= FDMA_SUCCESS) {
		if (flags & FDMA_ST_ASA_SEGMENT_BIT)
			PRC_SET_ASA_SIZE(*((uint16_t *)
					(HWC_ACC_OUT_ADDRESS2)));
		else
			PRC_SET_SEGMENT_LENGTH(*((uint16_t *)
					(HWC_ACC_OUT_ADDRESS2)));

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else if (res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR)
			return -EIO;
		else	/* FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_store_default_frame_data(void)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;
	/* storage profile ID */
	uint8_t spid = *((uint8_t *) HWC_SPID_ADDRESS);

	/* prepare command parameters */
	arg1 = FDMA_STORE_DEFAULT_CMD_ARG1(spid, PRC_GET_HANDLES());
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_store_frame_data(
		uint8_t frame_handle,
		uint8_t spid,
		struct fdma_amq *amq)
{
	/* command parameters and results */
	uint32_t arg1;
	uint16_t bdi_icid;
	int8_t res1;
	/* storage profile ID */

	/* prepare command parameters */
	arg1 = FDMA_STORE_CMD_ARG1(spid, frame_handle);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	bdi_icid = *((uint16_t *)
		(HWC_ACC_OUT_ADDRESS2 + FDMA_STORE_CMD_OUT_ICID_OFFSET));
	amq->icid = bdi_icid & ~(FDMA_ICID_CONTEXT_BDI);
	amq->flags =
		(((*((uint16_t *)HWC_ACC_OUT_ADDRESS2)) &
			(FDMA_ICID_CONTEXT_PL | FDMA_ICID_CONTEXT_VA)) |
		(uint16_t)(bdi_icid & FDMA_ICID_CONTEXT_BDI));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_store_and_enqueue_default_frame_fqid(
		uint32_t fqid,
		uint32_t flags)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;
	/* storage profile ID */
	uint8_t spid = *((uint8_t *) HWC_SPID_ADDRESS);

	/* prepare command parameters */
	flags |= FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_WF_ARG1(spid, PRC_GET_HANDLES(), flags);
	/* store command parameters */
	__stdw(arg1, fqid, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_store_and_enqueue_frame_fqid(
		uint8_t  frame_handle,
		uint32_t flags,
		uint32_t fqid,
		uint8_t  spid)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;

	/* prepare command parameters */
	flags |= FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_WF_EXP_ARG1(spid, frame_handle, flags);
	/* store command parameters */
	__stdw(arg1, fqid, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_store_and_enqueue_default_frame_qd(
		struct fdma_queueing_destination_params *qdp,
		uint32_t	flags)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t res1;
	/* storage profile ID */
	uint8_t spid = *((uint8_t *) HWC_SPID_ADDRESS);

	/* prepare command parameters */
	flags &= ~FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_WF_ARG1(spid, PRC_GET_HANDLES(), flags);
	arg2 = FDMA_ENQUEUE_WF_QD_ARG2(qdp->qd_priority, qdp->qd);
	arg3 = FDMA_ENQUEUE_WF_QD_ARG3(qdp->qdbin);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;
	/*__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);*/

	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_store_and_enqueue_frame_qd(
		uint8_t  frame_handle,
		uint32_t flags,
		struct fdma_queueing_destination_params *qdp,
		uint8_t spid)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t res1;

	/* prepare command parameters */
	flags &= ~FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_WF_EXP_ARG1(spid, frame_handle, flags);
	arg2 = FDMA_ENQUEUE_WF_QD_ARG2(qdp->qd_priority, qdp->qd);
	arg3 = FDMA_ENQUEUE_WF_QD_ARG3(qdp->qdbin);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;
	/*__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);*/

	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_enqueue_default_fd_fqid(
		uint16_t icid,
		uint32_t flags,
		uint32_t fqid)
{
	/* command parameters and results */
	uint32_t arg1, arg3;
	int8_t res1;
	/* storage profile ID */
	/* prepare command parameters */
	flags |= FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_FRAME_ARG1(flags);
	arg3 = FDMA_ENQUEUE_FRAME_ARG3(flags, icid);
	/* store command parameters */
	__stdw(arg1, fqid, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = arg3;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_enqueue_fd_fqid(
		struct ldpaa_fd *fd,
		uint32_t flags,
		uint32_t fqid,
		uint16_t icid)
{
	/* command parameters and results */
	uint32_t arg1, arg3;
	int8_t res1;
	/* storage profile ID */
	/* prepare command parameters */
	flags |= FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_FRAME_EXP_ARG1(flags, fd);
	arg3 = FDMA_ENQUEUE_FRAME_ARG3(flags, icid);
	/* store command parameters */
	__stdw(arg1, fqid, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = arg3;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_enqueue_default_fd_qd(
		uint16_t icid,
		uint32_t flags,
		struct fdma_queueing_destination_params *enqueue_params)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t res1;
	/* storage profile ID */
	/* prepare command parameters */
	flags &= ~FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_FRAME_ARG1(flags);
	arg2 = FDMA_ENQUEUE_WF_QD_ARG2(enqueue_params->qd_priority,
			enqueue_params->qd);
	arg3 = FDMA_ENQUEUE_FRAME_QD_ARG3(flags, icid,
			enqueue_params->qdbin);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = arg3;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_enqueue_fd_qd(
		struct ldpaa_fd *fd,
		uint32_t flags,
		struct fdma_queueing_destination_params *enqueue_params,
		uint16_t icid)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t res1;
	/* storage profile ID */
	/* prepare command parameters */
	flags &= ~FDMA_EN_EIS_BIT;
	arg1 = FDMA_ENQUEUE_FRAME_EXP_ARG1(flags, fd);
	arg2 = FDMA_ENQUEUE_WF_QD_ARG2(enqueue_params->qd_priority,
			enqueue_params->qd);
	arg3 = FDMA_ENQUEUE_FRAME_QD_ARG3(flags, icid,
			enqueue_params->qdbin);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = arg3;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

void fdma_discard_default_frame(uint32_t flags)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;
	/* prepare command parameters */
	/*if (flags & FDMA_DIS_FS_HANDLE_BIT)*/
		arg1 = FDMA_DISCARD_ARG1_WF(PRC_GET_HANDLES(), flags);
	/*else
		arg1 = FDMA_DISCARD_ARG1_FD(HWC_FD_ADDRESS, flags);*/
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}

void fdma_discard_frame(uint16_t frame, uint32_t flags)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;
	/* prepare command parameters */
#ifdef REV2
	/* Todo - Add ICID + flags support */
#endif /*REV2*/
	/*if (flags & FDMA_DIS_FS_HANDLE_BIT)*/
		arg1 = FDMA_DISCARD_ARG1_FRAME(frame, flags);
	/*else
		arg1 = FDMA_DISCARD_ARG1_FD(HWC_FD_ADDRESS, flags);*/
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}

int fdma_discard_fd(struct ldpaa_fd *fd, uint32_t flags)
{
	uint8_t frame_handle;
	int32_t status;

	status = fdma_present_frame_without_segments(fd, FDMA_INIT_NO_FLAGS,
			0, &frame_handle);
	if (status != SUCCESS)
		return status;

	fdma_discard_frame(frame_handle, flags);

	return SUCCESS;
}

void fdma_force_discard_fd(struct ldpaa_fd *fd)
{
	LDPAA_FD_SET_ERR(fd, 0);
	fdma_discard_fd(fd, FDMA_DIS_NO_FLAGS);
}

void fdma_terminate_task(void)
{
	/* command parameters and results */
	uint32_t arg1;
	/* prepare command parameters */
	arg1 = FDMA_TERM_TASK_CMD_ARG1();
	*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = arg1;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
}

int fdma_replicate_frame_fqid(
		uint8_t	frame_handle1,
		uint8_t	spid,
		uint32_t fqid,
		void *fd_dst,
		uint32_t flags,
		uint8_t *frame_handle2)
{
	/* command parameters and results */
	uint32_t arg1, arg3;
	int8_t  res1;

	flags |= FDMA_REPLIC_EIS_BIT;
	/* prepare command parameters */
	arg1 = FDMA_REPLIC_CMD_ARG1(spid, frame_handle1, flags);
	arg3 = FDMA_REPLIC_CMD_ARG3_FQID((uint32_t)fd_dst);
	/* store command parameters */
	__stdw(arg1, fqid, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	*frame_handle2 = *((uint8_t *) (FDMA_REPLIC_FRAME_HANDLE_OFFSET));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_replicate_frame_qd(
		uint8_t	frame_handle1,
		uint8_t	spid,
		struct fdma_queueing_destination_params *enqueue_params,
		void *fd_dst,
		uint32_t flags,
		uint8_t *frame_handle2)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t  res1;

	flags &= ~FDMA_REPLIC_EIS_BIT;
	/* prepare command parameters */
	arg1 = FDMA_REPLIC_CMD_ARG1(spid, frame_handle1, flags);
	arg2 = FDMA_REPLIC_CMD_ARG2_QD(enqueue_params->qd_priority,
			enqueue_params->qd);
	arg3 = FDMA_REPLIC_CMD_ARG3_QD((uint32_t)fd_dst,
			enqueue_params->qdbin);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *)(HWC_ACC_IN_ADDRESS3)) = arg3;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));
	*frame_handle2 = *((uint8_t *) (FDMA_REPLIC_FRAME_HANDLE_OFFSET));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_ENQUEUE_FAILED_ERR)
		return -EBUSY;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_concatenate_frames(
		struct fdma_concatenate_frames_params *params)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t  res1;
	uint16_t bdi_icid;

	/* prepare command parameters */
#ifdef REV2
	/* Todo - Add ICID + flags + FDs support */
#endif /*REV2*/
	arg1 = FDMA_CONCAT_CMD_ARG1(params->spid, params->trim, params->flags);
	arg2 = FDMA_CONCAT_CMD_ARG2(params->frame2, params->frame1);

	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if (params->flags & FDMA_CONCAT_PCA_BIT) {
		bdi_icid = *((uint16_t *)
		      (HWC_ACC_OUT_ADDRESS2 + FDMA_STORE_CMD_OUT_ICID_OFFSET));
		params->amq.icid = bdi_icid & ~(FDMA_ICID_CONTEXT_BDI);
		params->amq.flags =
			(((*((uint16_t *)HWC_ACC_OUT_ADDRESS2)) &
			(FDMA_ICID_CONTEXT_PL | FDMA_ICID_CONTEXT_VA)) |
			(uint16_t)(bdi_icid & FDMA_ICID_CONTEXT_BDI));
	}

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;
	else if (res1 == FDMA_FD_ERR)
		return -EBADFD;
	else
		fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}


int fdma_split_frame(
		struct fdma_split_frame_params *params)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
		(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	int8_t  res1;

	/* prepare command parameters */
#ifdef REV2
	if (((uint32_t)params->fd_dst) == HWC_FD_ADDRESS)
		arg1 = FDMA_SPLIT_CMD_ARG1(*((uint8_t *)HWC_SPID_ADDRESS),
				params->source_frame_handle, params->flags);
	else
		arg1 = FDMA_SPLIT_CMD_ARG1(params->spid,
				params->source_frame_handle, params->flags);
#endif /* REV2 */
	arg1 = FDMA_SPLIT_CMD_ARG1(params->spid,
				params->source_frame_handle, params->flags);

	arg2 = FDMA_SPLIT_CMD_ARG2((uint32_t)(params->seg_dst),
			params->seg_offset);
	arg3 = FDMA_SPLIT_CMD_ARG3(params->present_size);
	arg4 = FDMA_SPLIT_CMD_ARG4((uint32_t)(params->fd_dst),
			params->split_size_sf);

	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *) (FDMA_STATUS_ADDR));

	if ((res1 == FDMA_SUCCESS) ||
		(res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR) ||
		(res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)) {
		params->split_frame_handle = *((uint8_t *)
			(HWC_ACC_OUT_ADDRESS2 + FDMA_SEG_HANDLE_OFFSET));
		if (params->flags & FDMA_SPLIT_PSA_PRESENT_BIT) {
			params->seg_length = *((uint16_t *)
					(HWC_ACC_OUT_ADDRESS2));
			params->seg_handle = *((uint8_t *)
				(HWC_ACC_OUT_ADDRESS2 +
				FDMA_FRAME_HANDLE_OFFSET));
			/* Update Task Defaults */
			if (((uint32_t)params->fd_dst) == HWC_FD_ADDRESS) {
				prc->seg_address = (uint16_t)
				((uint32_t)params->seg_dst);
				prc->seg_length = params->seg_length;
				prc->seg_offset = params->seg_offset;
				prc->handles =
					((params->split_frame_handle << 4) &
						PRC_FRAME_HANDLE_MASK) |
					(params->seg_handle &
						PRC_SEGMENT_HANDLE_MASK);
				prc->ptapa_asapo = PRC_PTA_NOT_LOADED_ADDRESS;
				prc->asapa_asaps =
					(params->flags & FDMA_SPLIT_SR_BIT) ?
							PRC_SR_MASK : 0;
				if (!(params->flags & FDMA_SPLIT_SM_BIT))
					LDPAA_FD_SET_LENGTH(HWC_FD_ADDRESS,
							params->split_size_sf);
			}
		}
		/* Update Task Defaults */
		else if ((((uint32_t)params->fd_dst) == HWC_FD_ADDRESS) &&
			((params->flags & (FDMA_SPLIT_PSA_PRESENT_BIT)) == 0)) {
				prc->handles =
					((params->split_frame_handle << 4) &
					PRC_FRAME_HANDLE_MASK);
				prc->ptapa_asapo = PRC_PTA_NOT_LOADED_ADDRESS;
				prc->asapa_asaps = 0;
				if (!(params->flags & FDMA_SPLIT_SM_BIT))
					LDPAA_FD_SET_LENGTH(HWC_FD_ADDRESS,
						params->split_size_sf);
		}

		if ((((uint32_t)params->fd_dst) != HWC_FD_ADDRESS) &&
		    (params->source_frame_handle == PRC_GET_FRAME_HANDLE()) &&
		    !(params->flags & FDMA_SPLIT_SM_BIT))
			LDPAA_FD_UPDATE_LENGTH((uint32_t)params->fd_dst, 0,
					params->split_size_sf);

		if ((res1 == FDMA_SUCCESS))
			return SUCCESS;
		else if (res1 == FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR)
			return -EIO;
		else	/* FDMA_BUFFER_POOL_DEPLETION_ERR */
			return -ENOMEM;
	}

	if (res1 == FDMA_UNABLE_TO_SPLIT_ERR)
		return -EINVAL;
	else
		fdma_handle_fatal_errors(res1);

	return (int32_t)(res1);
}

void fdma_trim_default_segment_presentation(uint16_t offset, uint16_t size)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t res1;

	arg1 = FDMA_TRIM_CMD_ARG1(PRC_GET_HANDLES());
	arg2 = FDMA_TRIM_CMD_ARG2(offset, size);

	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));
	/* Update Task Defaults */
	if (res1 == FDMA_SUCCESS) {
		PRC_SET_SEGMENT_OFFSET(offset);
		PRC_SET_SEGMENT_LENGTH(size);
	} else {
		fdma_handle_fatal_errors((int32_t)res1);
	}
}

void fdma_modify_default_segment_data(
		uint16_t offset,
		uint16_t size)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((PRC_GET_SEGMENT_HANDLE() == FDMA_ASA_SEG_HANDLE) ||
	    (PRC_GET_SEGMENT_HANDLE() == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_REPLACE_CMD_ARG1(
			PRC_GET_HANDLES(), FDMA_REPLACE_NO_FLAGS);
	arg2 = FDMA_REPLACE_CMD_ARG2(offset, size);
	arg3 = FDMA_REPLACE_CMD_ARG3(
			(PRC_GET_SEGMENT_ADDRESS() + offset), size);
	/* store command parameters */
	__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}

#ifdef REV2
void fdma_modify_segment_data(
		uint8_t frame_handle,
		uint8_t seg_handle,
		uint16_t offset,
		uint16_t size,
		void	 *from_ws_src)
{
	/* command parameters and results */
	uint32_t arg1, arg2, arg3;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((seg_handle == FDMA_ASA_SEG_HANDLE) ||
	    (seg_handle == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_MODIFY_CMD_ARG1(
			frame_handle, seg_handle, FDMA_REPLACE_NO_FLAGS);
	arg2 = FDMA_REPLACE_CMD_ARG2(offset, size);
	arg3 = FDMA_REPLACE_CMD_ARG3(
			((uint32_t)from_ws_src), size);
	/* store command parameters */
	__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}
#endif /* REV2*/

int fdma_replace_default_segment_data(
		uint16_t to_offset,
		uint16_t to_size,
		void	 *from_ws_src,
		uint16_t from_size,
		void	 *ws_dst_rs,
		uint16_t size_rs,
		uint32_t flags)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((PRC_GET_SEGMENT_HANDLE() == FDMA_ASA_SEG_HANDLE) ||
	    (PRC_GET_SEGMENT_HANDLE() == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);

#ifdef REV2 /* Workaround for ticket TKT229796 */
	/* prepare command parameters */
	arg1 = FDMA_REPLACE_CMD_ARG1(prc->handles, flags);
	arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, to_size);
	arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, from_size);
	arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, size_rs);
	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
#else
	if (to_size < from_size) {
		/* Implement the workaround - First delete, that insert. */
		/* delete to_size bytes */
		/* prepare command parameters */
		arg1 = FDMA_REPLACE_CMD_ARG1(prc->handles,
				(flags & ~FDMA_REPLACE_SA_REPRESENT_BIT));
		arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, to_size);
		arg3 = FDMA_REPLACE_CMD_ARG3(0, 0);
		/* store command parameters */
		__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);
		/* call FDMA Accelerator */
		__e_hwacceli_(FODMA_ACCEL_ID);
		res1 = *((int8_t *)(FDMA_STATUS_ADDR));

		if ((int32_t)res1 < FDMA_SUCCESS)
			fdma_handle_fatal_errors((int32_t)res1);

		/* Insert from_size bytes*/
		arg1 = FDMA_REPLACE_CMD_ARG1(prc->handles, flags);
		arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, 0);
		arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, from_size);
		arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, size_rs);
		/* store command parameters */
		__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
		/* call FDMA Accelerator */
		__e_hwacceli_(FODMA_ACCEL_ID);
	} else {
		/* prepare command parameters */
		arg1 = FDMA_REPLACE_CMD_ARG1(prc->handles, flags);
		arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, to_size);
		arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, from_size);
		arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, size_rs);
		/* store command parameters */
		__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
		/* call FDMA Accelerator */
		__e_hwacceli_(FODMA_ACCEL_ID);
	}
#endif /* REV2 */

	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	/* Update Task Defaults */
	if ((int32_t)res1 >= FDMA_SUCCESS) {
		if (flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
			prc->seg_address = (uint16_t)(uint32_t)ws_dst_rs;
			prc->seg_length = *((uint16_t *)HWC_ACC_OUT_ADDRESS2);
		}
		/* FD fields should be updated with a swap load/store */
		if (from_size != to_size)
			LDPAA_FD_UPDATE_LENGTH(HWC_FD_ADDRESS,
					from_size, to_size);

		if (flags & FDMA_REPLACE_SA_CLOSE_BIT)
			PRC_SET_NDS_BIT();

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_insert_default_segment_data(
		uint16_t to_offset,
		void	 *from_ws_src,
		uint16_t insert_size,
		uint32_t flags)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4, seg_size_rs;
	void	 *ws_address_rs;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((PRC_GET_SEGMENT_HANDLE() == FDMA_ASA_SEG_HANDLE) ||
	    (PRC_GET_SEGMENT_HANDLE() == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_REPLACE_CMD_ARG1(prc->handles, flags);
	arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, 0);
	arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, insert_size);
	if (flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
		ws_address_rs = (void *) PRC_GET_SEGMENT_ADDRESS();
		seg_size_rs = PRC_GET_SEGMENT_LENGTH();
		if ((prc->seg_address - (uint32_t)TLS_SECTION_END_ADDR) >=
				insert_size){
			ws_address_rs = (void *)
				((uint32_t)ws_address_rs - insert_size);
			seg_size_rs = seg_size_rs + insert_size;
		}
		arg4 = FDMA_REPLACE_CMD_ARG4(ws_address_rs, seg_size_rs);
	}
	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	/* Update Task Defaults */
	if ((int32_t)res1 >= FDMA_SUCCESS) {
		if (flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
			prc->seg_address = (uint16_t)(uint32_t)ws_address_rs;
			prc->seg_length = *((uint16_t *)HWC_ACC_OUT_ADDRESS2);
		}
		/* FD fields should be updated with a swap load/store */
		LDPAA_FD_UPDATE_LENGTH(HWC_FD_ADDRESS, insert_size, 0);

		if (flags & FDMA_REPLACE_SA_CLOSE_BIT)
			PRC_SET_NDS_BIT();

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_insert_segment_data(
		struct fdma_insert_segment_data_params *params)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((params->seg_handle == FDMA_ASA_SEG_HANDLE) ||
	    (params->seg_handle == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_REPLACE_EXP_CMD_ARG1(params->seg_handle,
			params->frame_handle, params->flags);
	arg2 = FDMA_REPLACE_CMD_ARG2(params->to_offset, 0);
	arg3 = FDMA_REPLACE_CMD_ARG3(params->from_ws_src, params->insert_size);
	if (params->flags & FDMA_REPLACE_SA_REPRESENT_BIT)
		arg4 = FDMA_REPLACE_CMD_ARG4(params->ws_dst_rs,
				params->size_rs);
	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	/* Update Task Defaults */
	if ((int32_t)res1 >= FDMA_SUCCESS) {
		if (params->flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
			params->seg_length_rs = *((uint16_t *)
					(HWC_ACC_OUT_ADDRESS2));
			if (params->seg_handle == PRC_GET_SEGMENT_HANDLE()) {
				prc->seg_address = (uint16_t)(uint32_t)
							params->ws_dst_rs;
				prc->seg_length = *((uint16_t *)
							HWC_ACC_OUT_ADDRESS2);
			}
		}

		if (params->frame_handle == PRC_GET_FRAME_HANDLE())
			/* FD fields should be updated with a swap load/store */
			LDPAA_FD_UPDATE_LENGTH(HWC_FD_ADDRESS,
					params->insert_size, 0);

		if ((params->seg_handle == PRC_GET_SEGMENT_HANDLE()) &&
			(params->flags & FDMA_REPLACE_SA_CLOSE_BIT))
			PRC_SET_NDS_BIT();

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_delete_default_segment_data(
		uint16_t to_offset,
		uint16_t delete_target_size,
		uint32_t flags)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	void	 *ws_address_rs;
	uint16_t size_rs;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((PRC_GET_SEGMENT_HANDLE() == FDMA_ASA_SEG_HANDLE) ||
	    (PRC_GET_SEGMENT_HANDLE() == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_REPLACE_CMD_ARG1(prc->handles, flags);
	arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, delete_target_size);
	arg3 = FDMA_REPLACE_CMD_ARG3(0, 0);
	if (flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
		ws_address_rs = (void *)
			(PRC_GET_SEGMENT_ADDRESS() + delete_target_size);
		size_rs = PRC_GET_SEGMENT_LENGTH() - delete_target_size;
		arg4 = FDMA_REPLACE_CMD_ARG4(ws_address_rs, size_rs);
	}

	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	/* Update Task Defaults */
	if ((int32_t)res1 >= FDMA_SUCCESS) {
		if (flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
			prc->seg_address = (uint16_t)(uint32_t)ws_address_rs;
			prc->seg_length = *((uint16_t *)HWC_ACC_OUT_ADDRESS2);
		}
		/* FD fields should be updated with a swap load/store */
		LDPAA_FD_UPDATE_LENGTH(HWC_FD_ADDRESS, 0,
				delete_target_size);

		if (flags & FDMA_REPLACE_SA_CLOSE_BIT)
			PRC_SET_NDS_BIT();

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_delete_segment_data(
		struct fdma_delete_segment_data_params *params)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((params->seg_handle == FDMA_ASA_SEG_HANDLE) ||
	    (params->seg_handle == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_DELETE_CMD_ARG1(params->seg_handle, params->frame_handle,
			params->flags);
	arg2 = FDMA_REPLACE_CMD_ARG2(params->to_offset,
			params->delete_target_size);
	arg3 = FDMA_REPLACE_CMD_ARG3(0, 0);
	if (params->flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
		arg4 = FDMA_REPLACE_CMD_ARG4(params->ws_dst_rs,
				params->size_rs);
	}

	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	/* Update Task Defaults */
	if ((int32_t)res1 >= FDMA_SUCCESS) {
		if (params->flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
			params->seg_length_rs =
					*((uint16_t *)HWC_ACC_OUT_ADDRESS2);
			if (params->seg_handle == PRC_GET_SEGMENT_HANDLE()) {
				prc->seg_address =
					(uint16_t)((uint32_t)params->ws_dst_rs);
				prc->seg_length = *((uint16_t *)
							HWC_ACC_OUT_ADDRESS2);
			}
		}

		if (params->frame_handle == PRC_GET_FRAME_HANDLE()) {
			/* FD fields should be updated with a swap load/store */
			LDPAA_FD_UPDATE_LENGTH(HWC_FD_ADDRESS, 0,
					params->delete_target_size);

			if ((params->seg_handle == PRC_GET_SEGMENT_HANDLE()) &&
				(params->flags & FDMA_REPLACE_SA_CLOSE_BIT))
				PRC_SET_NDS_BIT();
		}

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}


void fdma_close_default_segment(void)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;
	uint8_t seg_handle;

	/* This command may be invoked only on Data segment */
	seg_handle = PRC_GET_SEGMENT_HANDLE();
	if ((seg_handle == FDMA_ASA_SEG_HANDLE) ||
	    (seg_handle == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_CLOSE_SEG_CMD_ARG1(PRC_GET_FRAME_HANDLE(), seg_handle);
	/* store command parameters */
	__stdw(arg1, 0, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = 0;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		PRC_SET_NDS_BIT();
	else
		fdma_handle_fatal_errors((int32_t)res1);
}

void fdma_close_segment(uint8_t frame_handle, uint8_t seg_handle)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;

	/* This command may be invoked only on Data segment */
	if ((seg_handle == FDMA_ASA_SEG_HANDLE) ||
	    (seg_handle == FDMA_PTA_SEG_HANDLE))
		fdma_handle_fatal_errors(FDMA_NO_DATA_SEGMENT_HANDLE);
	/* prepare command parameters */
	arg1 = FDMA_CLOSE_SEG_CMD_ARG1(frame_handle, seg_handle);
	/* store command parameters */
	__stdw(arg1, 0, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = 0;
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		PRC_SET_NDS_BIT();
	else
		fdma_handle_fatal_errors((int32_t)res1);
}

int fdma_replace_default_asa_segment_data(
		uint16_t to_offset,
		uint16_t to_size,
		void	 *from_ws_src,
		uint16_t from_size,
		void	 *ws_dst_rs,
		uint16_t size_rs,
		uint32_t flags)
{
	/* Presentation Context Pointer */
	struct presentation_context *prc =
			(struct presentation_context *) HWC_PRC_ADDRESS;
	/* command parameters and results */
	uint32_t arg1, arg2, arg3, arg4;
	/*uint16_t size_diff;*/
	int8_t res1;

#ifdef REV2 /* Workaround for ticket TKT229796 */
	/* prepare command parameters */
	arg1 = FDMA_REPLACE_PTA_ASA_CMD_ARG1(
			FDMA_ASA_SEG_HANDLE, prc->handles, flags);
	arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, to_size);
	arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, from_size);
	arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, size_rs);
	/* store command parameters */
	__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
#else
	if (to_size < from_size) {
		/* Implement the workaround - First delete, that insert. */
		/* delete to_size bytes */
		/* prepare command parameters */
		arg1 = FDMA_REPLACE_PTA_ASA_CMD_ARG1(
				FDMA_ASA_SEG_HANDLE, prc->handles,
				(flags & ~FDMA_REPLACE_SA_REPRESENT_BIT));
		arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, to_size);
		arg3 = FDMA_REPLACE_CMD_ARG3(0, 0);
		/* store command parameters */
		__stqw(arg1, arg2, arg3, 0, HWC_ACC_IN_ADDRESS, 0);
		/* call FDMA Accelerator */
		__e_hwacceli_(FODMA_ACCEL_ID);
		res1 = *((int8_t *)(FDMA_STATUS_ADDR));

		if ((int32_t)res1 < FDMA_SUCCESS)
			fdma_handle_fatal_errors((int32_t)res1);

		/* Insert from_size bytes*/
		arg1 = FDMA_REPLACE_PTA_ASA_CMD_ARG1(
				FDMA_ASA_SEG_HANDLE, prc->handles, flags);
		arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, 0);
		arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, from_size);
		arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, size_rs);
		/* store command parameters */
		__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
		/* call FDMA Accelerator */
		__e_hwacceli_(FODMA_ACCEL_ID);
	} else {
		/* prepare command parameters */
		arg1 = FDMA_REPLACE_PTA_ASA_CMD_ARG1(
				FDMA_ASA_SEG_HANDLE, prc->handles, flags);
		arg2 = FDMA_REPLACE_CMD_ARG2(to_offset, to_size);
		arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, from_size);
		arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, size_rs);
		/* store command parameters */
		__stqw(arg1, arg2, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
		/* call FDMA Accelerator */
		__e_hwacceli_(FODMA_ACCEL_ID);
	}
#endif /* REV2 */

	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	/* Update Task Defaults */
	if (((int32_t)res1 >= FDMA_SUCCESS) && (to_size != from_size)) {
		if (flags & FDMA_REPLACE_SA_REPRESENT_BIT) {
			prc->asapa_asaps =
				(((uint16_t)prc->asapa_asaps) & PRC_SR_MASK) |
				(((uint16_t)((uint32_t)ws_dst_rs)) &
							PRC_ASAPA_MASK) |
				((*((uint16_t *)HWC_ACC_OUT_ADDRESS2)) &
						PRC_ASAPS_MASK);
		}
	}

	/* Update FD ASA fields */
	if ((int32_t)res1 >= FDMA_SUCCESS)
		LDPAA_FD_SET_ASAL(HWC_FD_ADDRESS,
			LDPAA_FD_GET_ASAL(HWC_FD_ADDRESS) + (uint8_t)from_size -
							(uint8_t)to_size);

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR)
		return -EIO;

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

int fdma_replace_default_pta_segment_data(
		uint32_t flags,
		void	 *from_ws_src,
		void	 *ws_dst_rs,
		enum fdma_pta_size_type size_type)
{
	/* command parameters and results */
	uint32_t arg1, arg3, arg4;
	int8_t res1;

	/* prepare command parameters */
	if ((flags & FDMA_REPLACE_SA_REPRESENT_BIT) &&
		(((uint16_t)((uint32_t)ws_dst_rs)) ==
				PRC_PTA_NOT_LOADED_ADDRESS))
		fdma_handle_fatal_errors(FDMA_INVALID_PTA_ADDRESS);

	arg1 = FDMA_REPLACE_PTA_ASA_CMD_ARG1(
			FDMA_PTA_SEG_HANDLE, PRC_GET_HANDLES(), flags);
	arg3 = FDMA_REPLACE_CMD_ARG3(from_ws_src, size_type);
	arg4 = FDMA_REPLACE_CMD_ARG4(ws_dst_rs, 0);
	/* store command parameters */
	__stqw(arg1, 0, arg3, arg4, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (((int32_t)res1 >= FDMA_SUCCESS)) {
		/* in case of representing the PTA, update the PTA address in
		 * the prc */
		if (flags & FDMA_REPLACE_SA_REPRESENT_BIT)
			PRC_SET_PTA_ADDRESS(
				(((uint16_t)((uint32_t)ws_dst_rs))));
		/* Update FD PTA fields only in case PTA was modified */
		if (size_type & PTA_SIZE_PTV1) {
			LDPAA_FD_SET_PTV1(HWC_FD_ADDRESS, FD_PTV1_MASK);
			LDPAA_FD_SET_PTV2(HWC_FD_ADDRESS, 0);
		}
		if (size_type & PTA_SIZE_PTV2) {
			LDPAA_FD_SET_PTV2(HWC_FD_ADDRESS, FD_PTV2_MASK);
			LDPAA_FD_SET_PTV1(HWC_FD_ADDRESS, 0);
		}
		if (size_type & PTA_SIZE_PTV1_2) {
			LDPAA_FD_SET_PTA(HWC_FD_ADDRESS, FD_PTA_MASK);
			LDPAA_FD_SET_PTV1(HWC_FD_ADDRESS, FD_PTV1_MASK);
			LDPAA_FD_SET_PTV2(HWC_FD_ADDRESS, FD_PTV2_MASK);
		}

		if (res1 == FDMA_SUCCESS)
			return SUCCESS;
		else	/*FDMA_UNABLE_TO_PRESENT_PTA_ERR*/
			return -EIO;
	}

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

void fdma_calculate_default_frame_checksum(
		uint16_t offset,
		uint16_t size,
		uint16_t *checksum)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t res1;

	/* prepare command parameters */
	arg1 = FDMA_CKS_CMD_ARG1(PRC_GET_FRAME_HANDLE());
	arg2 = FDMA_CKS_CMD_ARG2(offset, size);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FODMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));
	*checksum = *((uint16_t *)(HWC_ACC_OUT_ADDRESS2+FDMA_CHECKSUM_OFFSET));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}

void fdma_copy_data(
		uint16_t copy_size,
		uint32_t flags,
		void *src,
		void *dst)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;

	/* prepare command parameters */
	arg1 = FDMA_COPY_CMD_ARG1(copy_size, flags);
	/* store command parameters */
	__stdw(arg1, (uint32_t)src, HWC_ACC_IN_ADDRESS, 0);
	*((uint32_t *) HWC_ACC_IN_ADDRESS3) = (uint32_t)dst;
	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}

void fdma_dma_data(
		uint16_t copy_size,
		uint16_t icid,
		void *loc_addr,
		uint64_t sys_addr,
		uint32_t flags)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t res1;

	/* prepare command parameters */
	arg1 = FDMA_DMA_CMD_ARG1(icid, flags);
	arg2 = FDMA_DMA_CMD_ARG2(copy_size, (uint32_t)loc_addr);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);
	__llstdw(sys_addr, HWC_ACC_IN_ADDRESS3, 0);
	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}


int fdma_acquire_buffer(
		uint16_t icid,
		uint32_t flags,
		uint16_t bpid,
		void *dst)
{
	/* command parameters and results */
	uint32_t arg1, arg2;
	int8_t res1;

	/* prepare command parameters */
	arg1 = FDMA_ACQUIRE_CMD_ARG1(icid, flags);
	arg2 = FDMA_ACQUIRE_CMD_ARG2(dst, bpid);
	/* store command parameters */
	__stdw(arg1, arg2, HWC_ACC_IN_ADDRESS, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 == FDMA_SUCCESS)
		return SUCCESS;
	else if (res1 == FDMA_BUFFER_POOL_DEPLETION_ERR)
		return -ENOMEM;

	fdma_handle_fatal_errors((int32_t)res1);

	return (int32_t)(res1);
}

void fdma_release_buffer(
		uint16_t icid,
		uint32_t flags,
		uint16_t bpid,
		uint64_t addr)
{
	/* command parameters and results */
	uint32_t arg1;
	int8_t res1;

	/* prepare command parameters */
	arg1 = FDMA_RELEASE_CMD_ARG1(icid, flags);
	/* store command parameters */
	__stdw(arg1, bpid, HWC_ACC_IN_ADDRESS, 0);
	__llstdw(addr, HWC_ACC_IN_ADDRESS3, 0);

	/* call FDMA Accelerator */
	__e_hwacceli_(FPDMA_ACCEL_ID);
	/* load command results */
	res1 = *((int8_t *)(FDMA_STATUS_ADDR));

	if (res1 != FDMA_SUCCESS)
		fdma_handle_fatal_errors((int32_t)res1);
}

/* Todo - enable inline when inline works correctly+move definition to .h file*/
/*inline*/ void get_default_amq_attributes(
		struct fdma_amq *amq)
{
	struct additional_dequeue_context *adc =
		(struct additional_dequeue_context *)HWC_ADC_ADDRESS;
	uint16_t adc_pl_icid = LH_SWAP(0, &(adc->pl_icid));

	amq->flags = 0;
	amq->icid = adc_pl_icid & ADC_ICID_MASK;
	if (adc->fdsrc_va_fca_bdi & ADC_BDI_MASK)
		amq->flags |= FDMA_ICID_CONTEXT_BDI;
	if (adc->fdsrc_va_fca_bdi & ADC_VA_MASK)
		amq->flags |= FDMA_ICID_CONTEXT_VA;
	if (adc_pl_icid & ADC_PL_MASK)
		amq->flags |= FDMA_ICID_CONTEXT_PL;
}

/* Todo - enable inline when inline works correctly+move definition to .h file*/
/*inline*/ void set_default_amq_attributes(
		struct fdma_amq *amq)
{
	struct additional_dequeue_context *adc =
			(struct additional_dequeue_context *)HWC_ADC_ADDRESS;
	uint16_t pl_icid = (amq->icid & ADC_ICID_MASK);
	uint8_t flags = 0;

	if (amq->flags & FDMA_ICID_CONTEXT_VA)
		flags |=  ADC_VA_MASK;
	if (amq->flags & FDMA_ICID_CONTEXT_BDI)
		flags |=  ADC_BDI_MASK;
	if (amq->flags & FDMA_ICID_CONTEXT_PL)
		pl_icid |= ADC_PL_MASK;
	STH_SWAP(pl_icid, 0, &(adc->pl_icid));
	adc->fdsrc_va_fca_bdi =
		(adc->fdsrc_va_fca_bdi & ~(ADC_BDI_MASK | ADC_VA_MASK)) | flags;
}

void fdma_handle_fatal_errors(int32_t status)
{
	/*TODO Fatal error*/
	switch (status) {
	case FDMA_UNABLE_TO_TRIM_ERR:
		handle_fatal_error((char *)FDMA_UNABLE_TO_TRIM_ERR);
		break;
	case FDMA_FRAME_STORE_ERR:
		handle_fatal_error((char *)FDMA_FRAME_STORE_ERR);
		break;
	case FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR:
		handle_fatal_error(
			(char *)FDMA_UNABLE_TO_PRESENT_FULL_SEGMENT_ERR);
		break;
	case FDMA_ASA_OFFSET_BEYOND_ASA_LENGTH_ERR:
		handle_fatal_error(
			(char *)FDMA_ASA_OFFSET_BEYOND_ASA_LENGTH_ERR);
		break;
	case FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR:
		handle_fatal_error((char *)FDMA_UNABLE_TO_PRESENT_FULL_ASA_ERR);
		break;
	case FDMA_UNABLE_TO_PRESENT_PTA_ERR:
		handle_fatal_error((char *)FDMA_UNABLE_TO_PRESENT_PTA_ERR);
		break;
	case FDMA_UNABLE_TO_EXECUTE_DUE_TO_RESERVED_FMT_ERR:
		handle_fatal_error(
			(char *)FDMA_UNABLE_TO_EXECUTE_DUE_TO_RESERVED_FMT_ERR);
		break;
	case FDMA_FD_ERR:
		handle_fatal_error((char *)FDMA_FD_ERR);
		break;
	case FDMA_FRAME_HANDLE_DEPLETION_ERR:
		handle_fatal_error((char *)FDMA_FRAME_HANDLE_DEPLETION_ERR);
		break;
	case FDMA_INVALID_FRAME_HANDLE_ERR:
		handle_fatal_error((char *)FDMA_INVALID_FRAME_HANDLE_ERR);
		break;
	case FDMA_SEGMENT_HANDLE_DEPLETION_ERR:
		handle_fatal_error((char *)FDMA_SEGMENT_HANDLE_DEPLETION_ERR);
		break;
	case FDMA_INVALID_SEGMENT_HANDLE_ERR:
		handle_fatal_error((char *)FDMA_INVALID_SEGMENT_HANDLE_ERR);
		break;
	case FDMA_INVALID_DMA_COMMAND_ARGS_ERR:
		handle_fatal_error((char *)FDMA_INVALID_DMA_COMMAND_ARGS_ERR);
		break;
	case FDMA_INVALID_DMA_COMMAND_ERR:
		handle_fatal_error((char *)FDMA_INVALID_DMA_COMMAND_ERR);
		break;
	case FDMA_INTERNAL_MEMORY_ECC_ERR:
		handle_fatal_error((char *)FDMA_INTERNAL_MEMORY_ECC_ERR);
		break;
	case FDMA_WORKSPACE_MEMORY_READ_ERR:
		handle_fatal_error((char *)FDMA_WORKSPACE_MEMORY_READ_ERR);
		break;
	case FDMA_WORKSPACE_MEMORY_WRITE_ERR:
		handle_fatal_error((char *)FDMA_WORKSPACE_MEMORY_WRITE_ERR);
		break;
	case FDMA_SYSTEM_MEMORY_READ_ERR:
		handle_fatal_error((char *)FDMA_SYSTEM_MEMORY_READ_ERR);
		break;
	case FDMA_SYSTEM_MEMORY_WRITE_ERR:
		handle_fatal_error((char *)FDMA_SYSTEM_MEMORY_WRITE_ERR);
		break;
	case FDMA_QMAN_ENQUEUE_ERR:
		handle_fatal_error((char *)FDMA_QMAN_ENQUEUE_ERR);
		break;
	case FDMA_FRAME_STRUCTURAL_ERR:
		handle_fatal_error((char *)FDMA_FRAME_STRUCTURAL_ERR);
		break;
	case FDMA_INTERNAL_ERR:
		handle_fatal_error((char *)FDMA_INTERNAL_ERR);
		break;
	case FDMA_SPID_ICID_ERR:
		handle_fatal_error((char *)FDMA_SPID_ICID_ERR);
		break;
	case FDMA_SRAM_MEMORY_READ_ERR:
		handle_fatal_error((char *)FDMA_SRAM_MEMORY_READ_ERR);
		break;
	case FDMA_PROFILE_SRAM_MEMORY_READ_ERR:
		handle_fatal_error((char *)FDMA_PROFILE_SRAM_MEMORY_READ_ERR);
		break;
	case FDMA_NO_DATA_SEGMENT_HANDLE:
		handle_fatal_error((char *)FDMA_NO_DATA_SEGMENT_HANDLE);
		break;
	case FDMA_INVALID_PTA_ADDRESS:
		handle_fatal_error((char *)FDMA_INVALID_PTA_ADDRESS);
		break;
	default:
		handle_fatal_error((char *)0);
	}
}

