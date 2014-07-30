/**************************************************************************//**
@File          tman.h

@Description   This file contains the AIOP SW TMAN definitions

		Copyright 2013 Freescale Semiconductor, Inc.
*//***************************************************************************/

#ifndef __AIOP_TMAN_H
#define __AIOP_TMAN_H

#include "general.h"
#include "fsl_errors.h"



/**************************************************************************//**
@Group		TMANReturnStatus TMAN functions return status

@Description	AIOP TMAN functions return status
@{
*//***************************************************************************/

/**************************************************************************//**
 @enum tman_tmi_create_status

 @Description	AIOP TMAN create TMI command status codes.

 @{
*//***************************************************************************/
enum tman_tmi_create_status {
	/** Success. */
	TMAN_TMI_CREATE_SUCCESS = 0,
	/** TMI Creation logic is currently busy with the previous TMI creation
	 * command. */
	TMAN_TMI_CREATE_BUSY = 0x81400040,
	/** All TMIs are used. A TMI must be deleted before a new one can
		be created. */
	TMAN_TMIID_DEPLETION_ERR = 0x814000FF,
	/** Bus error occurred when initializing the TMI timers memory*/
	TMAN_TMI_CREATE_BUS_ERR = 0x81400FFF
};

/* @} end of enum tman_tmi_create_status */

/**************************************************************************//**
 @enum tman_timer_create_status

 @Description	AIOP TMAN create timer command status codes.

 @{
*//***************************************************************************/
enum tman_timer_create_status {
	/** Success. */
	TMAN_TMR_CREATE_SUCCESS = 0,
	/** illegal Timer Create Fields Description[DURATION] */
	TMAN_ILLEGAL_DURATION_VAL_ERR = 0x81800010,
	/** The TMI timer list head is waiting for confirmation. In this case 
	 * the timer handle will contain the list head. */
	TMAN_TMR_CONF_WAIT_ERR = 0x81800040,
	/** A non active TMI was provided as an input */
	TMAN_TMR_TMI_NOT_ACTIVE = 0x81C00010,
	/** No more available timers in the TMI */
	TMAN_TMR_DEPLETION_ERR = 0x81C00020
};

/* @} end of enum tman_timer_create_status */

/**************************************************************************//**
 @enum tman_tmi_state_error

 @Description	AIOP TMAN TMI Engine state status codes.

 @{
*//***************************************************************************/
enum tman_tmi_state_ststus {
	/** A non active TMI was provided as an input */
	TMAN_TMI_NOT_ACTIVE = 0,
	/** TMI is active */
	TMAN_TMI_ACTIVE,
	/** TMI being purged */
	TMAN_TMI_PURGED,
	/** TMI Busy */
	TMAN_TMI_BUSY1 = 4,
	/** TMI Busy currently TMAN is busy creating the TMI */
	TMAN_TMI_BUSY2,
	/** TMI Failed to init due to Bus error */
	TMAN_TMI_BUS_ERR
};

/* @} end of enum tman_tmi_state_error */


/**************************************************************************//**
 @enum tman_timer_delete_status

 @Description	AIOP TMAN delete timer command status codes.

 @{
*//***************************************************************************/
enum tman_timer_delete_status {
	/** Success. */
	TMAN_DEL_TMR_DELETE_SUCCESS = 0,
	/** A non active timer was provided as an input */
	TMAN_DEL_TMR_NOT_ACTIVE_ERR = 0x81800050,
	/** The one shot timer has expired but it is pending a completion
	 * confirmation (done by calling the tman_timer_completion_confirmation
	 * function) */
	TMAN_DEL_CCP_WAIT_ERR = 0x81800051,
	/** The periodic timer has expired but it is pending a completion
	 * confirmation (done by calling the tman_timer_completion_confirmation
	 * function) */
	TMAN_DEL_PERIODIC_CCP_WAIT_ERR = 0x81800055,
	/** A delete command was already issued for this timer and the TMAN is
	 * in the process of deleting the timer. The timer will elapse in the
	 * future. */
	TMAN_DEL_TMR_DEL_ISSUED_ERR = 0x81800056,
	/** A delete command was already issued. The timer has already elapsed
	 * for the last time and it is pending a completion confirmation
	 * (done by calling the tman_timer_completion_confirmation function) */
	TMAN_DEL_TMR_DEL_ISSUED_CONF_ERR = 0x81800057,
};

/* @} end of enum tman_timer_delete_status */

/**************************************************************************//**
 @enum tman_timer_mod_status

 @Description	AIOP TMAN increase timer duration command status codes.

 @{
*//***************************************************************************/
enum tman_timer_mod_status {
	/** Success. */
	TMAN_MOD_TMR_SUCCESS = 0,
	/** illegal Timer Create Fields Description[DURATION] */
	TMAN_MOD_ILLEGAL_DURATION_VAL_ERR = 0x81400010,
	/** A non active timer was provided as an input */
	TMAN_MOD_TMR_NOT_ACTIVE_ERR = 0x81400060,
	/** The one shot timer has expired but it is pending a completion
	 * confirmation (done by calling the tman_timer_completion_confirmation
	 * function) */
	TMAN_MOD_CCP_WAIT_ERR = 0x81400061,
	/** The periodic timer has expired but it is pending a completion
	 * confirmation (done by calling the tman_timer_completion_confirmation
	 * function) */
	TMAN_MOD_PERIODIC_CCP_WAIT_ERR = 0x81400065,
	/** A delete command was already issued for this timer and the TMAN is
	 * in the process of deleting the timer. The timer will elapse in the
	 * future. */
	TMAN_MOD_TMR_DEL_ISSUED_ERR = 0x81400066,
	/** A delete command was already issued. The timer has already elapsed
	 * for the last time and it is pending a completion confirmation
	 * (done by calling the tman_timer_completion_confirmation function) */
	TMAN_MOD_TMR_DEL_ISSUED_CONF_ERR = 0x81400067,
};

/* @} end of enum tman_timer_mod_status */

/**************************************************************************//**
 @enum tman_timer_recharge_status

 @Description	AIOP TMAN timer recharge command status codes.

 @{
*//***************************************************************************/
enum tman_timer_recharge_status {
	/** Success. */
	TMAN_REC_TMR_SUCCESS = 0,
	/** A non active timer was provided as an input */
	TMAN_REC_TMR_NOT_ACTIVE_ERR = 0x81400070,
	/** The one shot timer has expired but it is pending a completion
	 * confirmation (done by calling the tman_timer_completion_confirmation
	 * function) */
	TMAN_REC_CCP_WAIT_ERR = 0x81400071,
	/** The periodic timer has expired but it is pending a completion
	 * confirmation (done by calling the tman_timer_completion_confirmation
	 * function) */
	TMAN_REC_PERIODIC_CCP_WAIT_ERR = 0x81400075,
	/** A delete command was already issued for this timer and the TMAN is
	 * in the process of deleting the timer. The timer will elapse in the
	 * future. */
	TMAN_REC_TMR_DEL_ISSUED_ERR = 0x81400076,
	/** A delete command was already issued. The timer has already elapsed
	 * for the last time and it is pending a completion confirmation
	 * (done by calling the tman_timer_completion_confirmation function) */
	TMAN_REC_TMR_DEL_ISSUED_CONF_ERR = 0x81400077,
	/** The timer is elapsing in this timer tick */
	TMAN_REC_TMR_CURR_ELAPSE = 0x81800020,
	/** Because of processing load the TMAN is lagging behind the wall
	 * clock. This causes the timer aimed to be recharged expiration date
	 * to currently being processed. */
	TMAN_REC_TMR_BUSY = 0x81800030
};

/* @} end of enum tman_timer_recharge_status */


/* @} end of group TMANReturnStatus */


/*! \enum e_tman_cmd_type Defines the TMAN CMDTYPE field.*/
enum e_tman_cmd_type {
	TMAN_CMDTYPE_TMI_CREATE = 0x1000,
	TMAN_CMDTYPE_TMI_DELETE = 0x1011,
	TMAN_CMDTYPE_TMI_DELETE_FORCE = 0x1012,
	TMAN_CMDTYPE_TMI_QUERY = 0x1023,
	TMAN_CMDTYPE_TIMER_CREATE = 0x2010,
	TMAN_CMDTYPE_TIMER_DELETE = 0x2001,
	TMAN_CMDTYPE_TIMER_DELETE_FORCE,
	TMAN_CMDTYPE_TIMER_DELETE_FORCE_WAIT,
	TMAN_CMDTYPE_TIMER_MODIFY,
	TMAN_CMDTYPE_TIMER_RECHARGE,
	TMAN_CMDTYPE_TIMER_QUERY
};

#define TMAN_QUERY_MAX_NT_MASK	0x00FFFFFF
#define TMAN_STATUS_MASK	0xF8000000
/** TMAN Peripheral base address */
#define TMAN_BASE_ADDRESS	0x02020000
/** TMCBCC- TMan Callback Completion Confirmation */
#define TMAN_TMCBCC_ADDRESS	(TMAN_BASE_ADDRESS+0x014)
/** TMTSTMP- TMan TMAN Timestamp register address */
#define TMAN_TMTSTMP_ADDRESS	(TMAN_BASE_ADDRESS+0x020)
/** TMSTATE- TMan TMAN State register base address */
#define TMAN_TMSTATE_ADDRESS	(TMAN_BASE_ADDRESS+0x2018)
/** TMan Dedicated EPID */
#define EPID_TIMER_EVENT_IDX	1
/** Offset to USER_OPAQUE1 in FD */
#define FD_OPAQUE1_OFFSET	0x1A
/** Offset to HASH in FD */
#define FD_HASH_OFFSET		0x1C
/** Number of command retries - for debug purposes */
#define TMAN_MAX_RETRIES	1000
/** The TMI deletion logic is currently busy with another TMI delete */
#define TMAN_TMI_DEL_TMP_ERR_MASK	0x00000020
/** The TMI deletion logic is currently busy with another TMI delete */
#define TMAN_TMI_CMD_ERR	0x81400000
/** If the delete was successful */
#define TMAN_TMI_DEL_SUCCESS		0x00000000
/** A mask that defines the query TMI command temporary error type */
#define TMAN_TMI_QUERY_TMP_ERR_MASK	0x00000020
/** If the TMI query was successful */
#define TMAN_TMI_QUERY_SUCCESS		0x00000000
/** The TMI state bits mask */
#define TMAN_TMI_STATE_MASK		0x0000000F
/** Fail status bit mask */
#define TMAN_FAIL_BIT_MASK		0x80000000
/** Timer commands temporary error 1 */
#define TMAN_TMR_TMP_ERR1	0x81800020
/** Timer commands temporary error 2 */
#define TMAN_TMR_TMP_ERR2	0x81800030
/** Timer commands temporary error 3 */
#define TMAN_TMR_TMP_ERR3	0x81800040
/** Timer query command state bits mask */
#define TMAN_TMR_QUERY_STATE_MASK	0x7
/** Timer query command success return status */
#define TMAN_TMR_QUERY_SUCCESS	0
/** Alignment that the TMAN requires for the input/output extension params */
#define TMAN_EXT_PARAM_ALIGNMENT	16

/**************************************************************************//**
@Description	TMI input extension params
*//***************************************************************************/
struct tman_tmi_input_extention {
		/** data to be associated with the confirmation task. */
	uint64_t	opaque_data1;
		/** Hash value for the confirmation task ScopeID */
	uint32_t	hash;
		/** data to be associated with the confirmation task.
		and EPID */
	uint32_t	opaque_data2_epid;
};


/**************************************************************************//**
@Function	tman_timer_callback

@Description	Callback function, called for every timer expiration and for
		TMI delete completion confirmation.
		This function will call the user function the argument
		specified in the tman_delete_tmi and tman_create_timer
		functions.

@Param		None.

@Return		None.
@Cautions	This is a none return function.

*//***************************************************************************/
void tman_timer_callback(void);


/**************************************************************************//**
@Function	tman_exception_handler

@Description	Function that distribute the TMAN errors

@Param[in]	filename - The file in which the error originated.
@Param[in]	line - The line in the file in which the error originated.
@Param[in]	status - The error status returned by the TMAN.

@Return		None.
@Cautions	This is a none return function.

*//***************************************************************************/
void tman_exception_handler(char *filename, uint32_t line, int32_t status);

#endif /* __AIOP_TMAN_H */
