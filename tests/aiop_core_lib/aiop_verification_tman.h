/**************************************************************************//**
@File          aiop_verification_tman.h

@Description   This file contains the AIOP TMAN SW Verification Structures
*//***************************************************************************/


#ifndef __AIOP_VERIFICATION_TMAN_H_
#define __AIOP_VERIFICATION_TMAN_H_

#include "dplib/fsl_ldpaa.h"
#include "dplib/fsl_tman.h"


/**************************************************************************//**
 @addtogroup		AIOP_Service_Routines_Verification

 @{
*//***************************************************************************/


/**************************************************************************//**
 @Group		AIOP_TMAN_SRs_Verification

 @Description	AIOP TMAN Verification structures definitions.

 @{
*//***************************************************************************/

/*! \enum e_tman_cmd_type Defines the TMAN CMDTYPE field.*/
enum e_tman_cmd_type {
	TMAN_CMDTYPE_TMI_CREATE = 0,
	TMAN_CMDTYPE_TMI_DELETE,
	TMAN_CMDTYPE_TMI_QUERY,
	TMAN_CMDTYPE_TIMER_CREATE,
	TMAN_CMDTYPE_TIMER_DELETE,
	TMAN_CMDTYPE_TIMER_INC_DURATION,
	TMAN_CMDTYPE_TIMER_RECHARGE,
	TMAN_CMDTYPE_TIMER_QUERY,
	TMAN_CMDTYPE_COMPLETION_CONF,
	TMAN_CMDTYPE_GET_TS
};
/* TMAN Commands Structure identifiers */
#define TMAN_TMI_CREATE_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TMI_CREATE)

#define TMAN_TMI_DELETE_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TMI_DELETE)

#define TMAN_TMI_QUERY_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TMI_QUERY)

#define TMAN_TIMER_CREATE_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TIMER_CREATE)

#define TMAN_TIMER_DELETE_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TIMER_DELETE)

#define TMAN_TIMER_INC_DURATION_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TIMER_INC_DURATION)

#define TMAN_TIMER_RECHARGE_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TIMER_RECHARGE)

#define TMAN_TIMER_QUERY_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_TIMER_QUERY)

#define TMAN_TIMER_COMPLETION_CONF_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_COMPLETION_CONF)

#define TMAN_GET_TS_CMD_STR	((TMAN_ACCEL_ID << 16) | \
		(uint32_t)TMAN_CMDTYPE_GET_TS)

/**************************************************************************//**
@Description	TMAN TMI Create Command structure.

		Includes information needed for TMAN TMI Create Command
		verification.
*//***************************************************************************/
struct tman_tmi_create_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	int32_t		status;
	uint64_t	tmi_mem_base_addr;
	uint32_t	max_num_of_timers;
	uint8_t		tmi_id;
	uint8_t		pad[3];
};

/**************************************************************************//**
@Description	TMAN TMI delete Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_tmi_delete_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint32_t	mode_bits;
	tman_arg_8B_t	conf_opaque_data1;
	int32_t		status;
	tman_cb_t	tman_confirm_cb;
	tman_arg_2B_t	conf_opaque_data2;
	uint8_t		tmi_id;
	uint8_t		pad[5];
};

/**************************************************************************//**
@Description	TMAN TMI query Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_tmi_query_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint8_t		pad[4];
	struct tman_tmi_params tmi_params;
	int32_t		status;
	uint8_t		tmi_id;
	uint8_t		pad2[3];
};

/**************************************************************************//**
@Description	TMAN timer create Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_timer_create_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint32_t	mode_bits;
	uint64_t	opaque_data1;
	int32_t		status;
	uint32_t	timer_handle;
	tman_cb_t	tman_timer_cb;
	uint16_t	opaque_data2;
	uint16_t	duration;
	uint8_t		tmi_id;
	uint8_t		pad[7];
};

/**************************************************************************//**
@Description	TMAN timer delete Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_timer_delete_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	int32_t		status;
	uint32_t	mode_bits;
	uint32_t	timer_handle;
};

/**************************************************************************//**
@Description	TMAN timer increase duration Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_timer_increase_duration_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	int32_t		status;
	uint32_t	timer_handle;
	uint16_t	duration;
	uint8_t		pad[2];
};

/**************************************************************************//**
@Description	TMAN timer recharge Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_timer_recharge_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	int32_t		status;
	uint32_t	timer_handle;
};

/**************************************************************************//**
@Description	TMAN timer query Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_timer_query_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	int32_t		status;
	uint32_t	timer_handle;
	enum e_tman_query_timer state;
	uint8_t		pad[3];
};

/**************************************************************************//**
@Description	TMAN timer completion confirmation Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_timer_comp_conf_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint32_t	timer_handle;
};

/**************************************************************************//**
@Description	TMAN get TS Command structure.

		Includes information needed for TMAN Command verification.
*//***************************************************************************/
struct tman_get_ts_command {
	uint32_t	opcode;
		/**< Command structure identifier. */
	uint8_t		pad[4];
	uint64_t	timestamp;
};

uint16_t aiop_verification_tman(uint32_t asa_seg_addr);
void verif_tman_callback(uint64_t opaque1, uint16_t opaque2);

/** @} */ /* end of AIOP_TMAN_SRs_Verification */

/** @} */ /* end of AIOP_Service_Routines_Verification */



#endif /* __AIOP_VERIFICATION_TMAN_H_ */
