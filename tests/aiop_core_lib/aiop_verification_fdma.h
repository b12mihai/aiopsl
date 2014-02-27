/**************************************************************************//**
@File		aiop_verification_fdma.h

@Description	This file contains the AIOP FDMA SW Verification Structures.

		Copyright 2013 Freescale Semiconductor, Inc.
*//***************************************************************************/


#ifndef __AIOP_VERIFICATION_FDMA_H_
#define __AIOP_VERIFICATION_FDMA_H_

#include "dplib/fsl_ldpaa.h"
#include "general.h"
#include "fdma.h"

/* FDMA Command IDs (Extended commands relative to the commands defined at
 * fdma.h) */
	/** FDMA Initial Presentation explicit command code */
#define FDMA_INIT_EXP_CMD		0x00001001
	/** FDMA Read ASA segment command code */
#define FDMA_READ_ASA_CMD		0x00001002
	/** FDMA Read PTA segment command code */
#define FDMA_READ_PTA_CMD		0x00002002
	/** FDMA Present Data segment explicit command code */
#define FDMA_PRESENT_EXP_CMD		0x00003002
	/** FDMA Store default working frame command code */
#define FDMA_STORE_DEFAULT_WF_CMD	0x00001010
	/** FDMA Store working frame command code */
#define FDMA_STORE_WF_CMD		0x00002010
	/** FDMA Enqueue working frame command code */
#define FDMA_ENQUEUE_WF_EXP_CMD		0x00001011
	/** FDMA Discard default frame command code */
#define FDMA_ENQUEUE_FRAME_EXP_CMD	0x00001012
	/** FDMA Discard default frame command code */
#define FDMA_DISCARD_DEFAULT_WF_CMD	0x00001013
	/** FDMA Discard frame command code */
#define FDMA_DISCARD_WF_CMD		0x00002013
	/** FDMA Modify working frame segment command code */
#define FDMA_MODIFY_CMD			0x00006019
	/** FDMA Insert working frame segment command code */
#define FDMA_INSERT_DATA_CMD		0x00001019
	/** FDMA Delete working frame segment command code */
#define FDMA_DELETE_DATA_CMD		0x00002019
	/** FDMA Close working frame segment command code */
#define FDMA_CLOSE_SEG_CMD		0x00003019
	/** FDMA Replace ASA segment command code */
#define FDMA_REPLACE_ASA_CMD		0x00004019
	/** FDMA Replace PTA segment command code */
#define FDMA_REPLACE_PTA_CMD		0x00005019
	/** FDMA explicit Insert working frame segment command code */
#define FDMA_INSERT_EXP_DATA_CMD	0x00007019
	/** FDMA Create Frame command code */
#define FDMA_CREATE_FRAME_CMD		0x00000100

/* FDMA Commands Structure identifiers */
	/** FDMA Initial frame presentation Command Structure identifier */
#define FDMA_INIT_CMD_STR	((FPDMA_ACCEL_ID << 16) | FDMA_INIT_CMD)
	/** FDMA Initial frame presentation explicit Command Structure
	 * identifier */
#define FDMA_INIT_EXP_CMD_STR	((FPDMA_ACCEL_ID << 16) | FDMA_INIT_EXP_CMD)
	/** FDMA Present Data segment Command Structure identifier */
#define FDMA_PRESENT_CMD_STR	((FPDMA_ACCEL_ID << 16) | FDMA_PRESENT_CMD)
	/** FDMA Read ASA segment Command Structure identifier */
#define FDMA_READ_ASA_CMD_STR	((FPDMA_ACCEL_ID << 16) | FDMA_READ_ASA_CMD)
	/** FDMA Read PTA segment Command Structure identifier */
#define FDMA_READ_PTA_CMD_STR	((FPDMA_ACCEL_ID << 16) | FDMA_READ_PTA_CMD)
	/** FDMA Present Data segment explicit Command Structure identifier */
#define FDMA_PRESENT_EXP_CMD_STR ((FPDMA_ACCEL_ID << 16) | FDMA_PRESENT_EXP_CMD)
	/** FDMA Extend presentation Command Structure identifier */
#define FDMA_EXTEND_CMD_STR	((FPDMA_ACCEL_ID << 16) | FDMA_EXTEND_CMD)
	/** FDMA Store working frame Command Structure identifier */
#define FDMA_STORE_DEFAULT_WF_CMD_STR	((FODMA_ACCEL_ID << 16) | \
		FDMA_STORE_DEFAULT_WF_CMD)
	/** FDMA Store working frame Command Structure identifier */
#define FDMA_STORE_WF_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_STORE_WF_CMD)
	/** FDMA Enqueue working frame Command Structure identifier */
#define FDMA_ENQUEUE_WF_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_ENQUEUE_WF_CMD)
	/** FDMA Enqueue working frame explicit Command Structure identifier */
#define FDMA_ENQUEUE_WF_EXP_CMD_STR ((FODMA_ACCEL_ID << 16) | 		\
		FDMA_ENQUEUE_WF_EXP_CMD)
	/** FDMA Enqueue FD Command Structure identifier */
#define FDMA_ENQUEUE_FRAME_CMD_STR	((FODMA_ACCEL_ID << 16) |	\
		FDMA_ENQUEUE_FRAME_CMD)
	/** FDMA Enqueue FD explicit Command Structure identifier */
#define FDMA_ENQUEUE_FRAME_EXP_CMD_STR	((FODMA_ACCEL_ID << 16) |	\
		FDMA_ENQUEUE_FRAME_EXP_CMD)
	/** FDMA Discard default frame Command Structure identifier */
#define FDMA_DISCARD_DEFAULT_WF_CMD_STR	((FODMA_ACCEL_ID << 16) |	\
		FDMA_DISCARD_DEFAULT_WF_CMD)
	/** FDMA Discard working frame Command Structure identifier */
#define FDMA_DISCARD_WF_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_DISCARD_WF_CMD)
	/** FDMA Terminate Task Command Structure identifier */
#define FDMA_TERMINATE_TASK_CMD_STR	((FODMA_ACCEL_ID << 16) |	\
		FDMA_TERMINATE_TASK_CMD)
	/** FDMA Replicate frame Command Structure identifier */
#define FDMA_REPLICATE_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_REPLICATE_CMD)
	/** FDMA Concatenate frames Command Structure identifier */
#define FDMA_CONCAT_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_CONCAT_CMD)
	/** FDMA Split working frame Command Structure identifier */
#define FDMA_SPLIT_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_SPLIT_CMD)
	/** FDMA Trim working frame segment Command Structure identifier */
#define FDMA_TRIM_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_TRIM_CMD)
	/** FDMA Modify working frame segment Command Structure identifier */
#define FDMA_MODIFY_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_MODIFY_CMD)
	/** FDMA Replace working frame segment Command Structure identifier */
#define FDMA_REPLACE_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_REPLACE_CMD)
	/** FDMA Insert working frame segment Command Structure identifier */
#define FDMA_INSERT_DATA_CMD_STR ((FODMA_ACCEL_ID << 16) | FDMA_INSERT_DATA_CMD)
	/** FDMA Delete working frame segment Command Structure identifier */
#define FDMA_DELETE_DATA_CMD_STR ((FODMA_ACCEL_ID << 16) | FDMA_DELETE_DATA_CMD)
	/** FDMA Delete working frame segment Command Structure identifier */
#define FDMA_CLOSE_SEG_CMD_STR ((FODMA_ACCEL_ID << 16) | FDMA_CLOSE_SEG_CMD)
	/** FDMA Replace working frame ASA segment Command Structure
	 * identifier */
#define FDMA_REPLACE_ASA_CMD_STR ((FODMA_ACCEL_ID << 16) | FDMA_REPLACE_ASA_CMD)
	/** FDMA Replace working frame PTA segment Command Structure
	 * identifier*/
#define FDMA_REPLACE_PTA_CMD_STR ((FODMA_ACCEL_ID << 16) | FDMA_REPLACE_PTA_CMD)
/** FDMA Insert explicit working frame segment Command Structure identifier */
#define FDMA_INSERT_EXP_DATA_CMD_STR ((FODMA_ACCEL_ID << 16) | 		\
		FDMA_INSERT_EXP_DATA_CMD)
	/** FDMA Checksum working frame command Structure identifier */
#define FDMA_CKS_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_CKS_CMD)
	/** FDMA Copy data command Structure identifier */
#define FDMA_COPY_CMD_STR	((FODMA_ACCEL_ID << 16) | FDMA_COPY_CMD)
	/** FDMA Acquire buffer command Structure identifier */
#define FDMA_ACQUIRE_BUFFER_CMD_STR 	((FODMA_ACCEL_ID << 16) | 	\
		FDMA_ACQUIRE_BUFFER_CMD)
	/** FDMA Release buffer command Structure identifier */
#define FDMA_RELEASE_BUFFER_CMD_STR	((FODMA_ACCEL_ID << 16) | 	\
		FDMA_RELEASE_BUFFER_CMD)
	/** FDMA Create Frame command Structure identifier */
#define FDMA_CREATE_FRAME_CMD_STR ((FODMA_ACCEL_ID << 16) | 		\
		FDMA_CREATE_FRAME_CMD)

/** \addtogroup AIOP_Service_Routines_Verification
 *  @{
 */


/**************************************************************************//**
 @Group		AIOP_FDMA_SRs_Verification

 @Description	AIOP FDMA Verification structures definitions.

 @{
*//***************************************************************************/


/**************************************************************************//**
@Description	FDMA Initial frame presentation Command structure.

		Includes information needed for FDMA Initial frame presentation
		command verification.
		This command works on the default frame (expect a FD at address
		0x60), and present a data segment only if present_size > 0.

*//***************************************************************************/
struct fdma_init_command {
		/** FDMA Initial frame presentation command structure
		 * identifier. */
	uint32_t opcode;
		/** Pointer to the address within the workspace to present the
		 * opened frame segment. */
	uint32_t seg_address;
		/** 64B-aligned location in the workspace to store the 64B
		 * PTA field (0xFFC0 for no PTA presentation). */
	uint32_t pta_address;
		/** 64B-aligned location in the workspace to store the ASA. */
	uint32_t asa_address;
		/** location within the presented frame to start presenting
		 * the segment from. */
	uint16_t seg_offset;
		/** Number of frame bytes to present and create an open
		 * segment for. */
	uint16_t present_size;
		/** The first ASA 64B quantity to present. */
	uint8_t asa_offset;
		/** Number (maximum) of 64B ASA quantities to present (0 for no
		 * ASA presentation). */
	uint8_t asa_size;
		/** Reference within the frame to present from:
		 * - 0: start of the frame.
		 * - 1: end of the frame. */
	uint8_t SR;
		/** Command returned status. */
	int8_t  status;
};


/**************************************************************************//**
@Description	FDMA Initial frame presentation (explicit) Command structure.

		Includes information needed for FDMA Initial frame presentation
		explicit command verification.

*//***************************************************************************/
struct fdma_init_exp_command {
		/** FDMA Initial frame presentation explicit command structure
		 * identifier. */
	uint32_t opcode;
		/** Pointer to the location within the workspace to present the
		 * opened frame segment. */
	uint32_t seg_dst;
		/** Pointer to the 64B-aligned location in the workspace to
		 * store the 64B PTA field  (0xFFC0 for no PTA presentation). */
	uint32_t pta_dst;
		/** Pointer to the 64B-aligned location in the workspace to
		 * store the ASA. */
	uint32_t asa_dst;
		/** Pointer to the location within the workspace of the FD that
		 * is to be presented. */
	uint32_t fd_src;
		/** location within the presented frame to start presenting
		 * the segment from. */
	uint16_t present_size;
		/** Number of frame bytes to present and create an open
		 * segment for. */
	uint16_t seg_offset;
		/** Command returned number of bytes actually presented
		 * (the segment actual size). */
	uint16_t seg_length;
		/**
		* bits<0> : Bypass Datapath Isolation. Frame AMQ attribute.
		* bits<1-15> : Isolation Context ID. Frame AMQ attribute.
		* Used only in case AS field is set. */
	uint16_t bdi_icid;
		/** The first ASA 64B quantity to present. */
	uint8_t asa_offset;
		/** Number (maximum) of 64B ASA quantities to present (0 for no
		 * ASA presentation). */
	uint8_t asa_size;
		/** No Data Segment:
		 * - 0: Present data segment.
		 * - 1: Don't present data segment. */
	uint8_t NDS;
		/** Reference within the frame to present from:
		 * - 0: start of the frame.
		 * - 1: end of the frame. */
	uint8_t SR;
		/** AMQ attributes (PL, VA, BDI, ICID) Source.
		 * If set - supplied AMQ attributes are used.
		 * If reset - task default AMQ attributes (From Additional Dequeue
		 * Context) are used. */
	uint8_t AS;
		/** Virtual Address. Frame AMQ attribute.
		 * Used only in case AS field is set. */
	uint8_t VA;
		/** Privilege Level. Frame AMQ attribute.
		 * Used only in case AS field is set. */
	uint8_t PL;
		/** Command returned handle of the working frame. */
	uint8_t frame_handle;
		/** Command returned handle of the presented segment. */
	uint8_t seg_handle;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[2];
};


/**************************************************************************//**
@Description	FDMA Present segment Command structure.

		Includes information needed for FDMA Present segment
		command verification.

*//***************************************************************************/
struct fdma_present_command {
		/** FDMA Present segment command structure identifier. */
	uint32_t opcode;
		/** Pointer to the location within the workspace to present the
		 * frame segment data. */
	uint32_t ws_dst;
		/** location within the presented frame to start presenting
		 * from. Relative to SR flag. */
	uint16_t offset;
		/** Number of frame bytes to present (Must be greater than 0)*/
	uint16_t present_size;
		/** Command returned segment length. */
	uint16_t seg_length;
		/** Command returned segment handle. */
	uint8_t seg_handle;
		/** Reference within the frame to present from:
		 * - 0: start of the frame.
		 * - 1: end of the frame. */
	uint8_t SR;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[7];
};

/**************************************************************************//**
@Description	FDMA Read ASA segment Command structure.

		Includes information needed for FDMA to read ASA
		command verification.

*//***************************************************************************/
struct fdma_read_asa_command {
		/** FDMA Read ASA segment command structure identifier. */
	uint32_t opcode;
		/** Pointer to the location within the workspace to present the
		 * ASA segment data. */
	uint32_t ws_dst;
		/** Location within the ASA to start presenting from.
		 * Must be within the bound of the frame. Specified in 64B
		 * units. Relative to SR flag. */
	uint16_t offset;
		/** Number of frame bytes to present (Must be greater
		 * than 0). Contains the number of 64B quantities to present
		 * because the Frame ASAL field is specified in 64B units.*/
	uint16_t present_size;
		/** Command returned number of bytes actually
		 * presented (the segment size) given in 64B units */
	uint16_t seg_length;
		/** Command returned status. */
	int8_t  status;
};

/**************************************************************************//**
@Description	FDMA Read PTA segment Command structure.

		Includes information needed for FDMA to read PTA
		command verification.

*//***************************************************************************/
struct fdma_read_pta_command {
		/** FDMA Read PTA segment command structure identifier. */
	uint32_t opcode;
		/** Pointer to the location within the workspace to present the
		 * PTA segment data. */
	uint32_t ws_dst;
		/** Command returned number of bytes actually presented:
		- 0x0 = No PTA presented. PTV1=0 and PTV2=0 in the working
		frame.
		- 0x1 = 32B of PTA presented (PTV1=1 and PTV2=0 in the working
		frame).
		- 0x2 = 32B of PTA presented (PTV1=0 and PTV2=1 in the working
		frame).
		- 0x3 = 64B of PTA presented (PTV1=1 and PTV2=1 in the working
		frame). */
	uint16_t seg_length;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[5];
};

/**************************************************************************//**
@Description	FDMA Present segment explicit Command structure.

		Includes information needed for FDMA Present segment explicit
		command verification.

*//***************************************************************************/
struct fdma_present_exp_command {
		/** FDMA Present segment explicit command structure identifier. */
	uint32_t opcode;
		/** Pointer to the location within the workspace to present the
		 * frame segment data. */
	uint32_t ws_dst;
		/** location within the presented frame to start presenting
		 * from. Relative to SR flag. */
	uint16_t offset;
		/** Number of frame bytes to present (Must be greater than 0)*/
	uint16_t present_size;
		/** Command returned segment length. */
	uint16_t seg_length;
		/** frame handle. */
	uint8_t frame_handle;
		/** Command returned segment handle. */
	uint8_t seg_handle;
		/** Reference within the frame to present from:
		 * - 0: start of the frame.
		 * - 1: end of the frame. */
	uint8_t SR;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[6];
};

/**************************************************************************//**
@Description	FDMA Extend presentation Command structure.

		Includes information needed for FDMA Extend presentation
		command verification.

*//***************************************************************************/
struct fdma_extend_command {
		/** FDMA Extend presentation command structure identifier. */
	uint32_t opcode;
		/** Pointer to the address within the workspace to present the
		* additional frame segment data*/
	uint32_t ws_dst;
		/** Number of additional frame bytes to present and add to
		* the open segment. 0 results in no operation.*/
	uint16_t extend_size;
		/** Command returned segment length. */
	uint16_t seg_length;
		/** Indicates the type of segment to present:
		* - 0: Data segment.
		* - 1: ASA segment. */
	uint8_t ST;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[2];
};

/**************************************************************************//**
@Description	FDMA Store Default Working Frame Command structure.

		Includes information needed for FDMA Store Default Working Frame
		command verification.

*//***************************************************************************/
struct fdma_store_default_frame_command {
		/** FDMA Store Default Working Frame command structure
		 * identifier. */
	uint32_t opcode;
		/** Storage profile used to store frame data if additional
		 * buffers are required. */
	uint8_t	spid;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[2];
};

/**************************************************************************//**
@Description	FDMA Store Working Frame Command structure.

		Includes information needed for FDMA Store Working Frame
		command verification.

*//***************************************************************************/
struct fdma_store_frame_command {
		/** FDMA Store Working Frame command structure
		 * identifier. */
	uint32_t opcode;
		/** Command returned ICID of the Stored frame. */
	uint16_t icid;
		/** Frame handle to store. */
	uint8_t frame_handle;
		/** Storage profile used to store frame data if additional
		 * buffers are required*/
	uint8_t	spid;
		/** Command returned BDI flag of the Stored frame. */
	uint8_t	BDI;
		/** Command returned Bypass the Memory Translation of the Stored
		 * frame flag. */
	uint8_t	BMT;
		/** Command returned Privilege Level of the Stored frame flag.*/
	uint8_t	PL;
		/** Command returned Virtual Address of the Stored frame flag.*/
	uint8_t	VA;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[3];
};

/**************************************************************************//**
@Description	FDMA Enqueue Working Frame Command structure.

		Includes information needed for FDMA Enqueue Working Frame
		command verification.

*//***************************************************************************/
struct fdma_enqueue_wf_command {
		/** FDMA Enqueue working frame command structure
		* identifier. */
	uint32_t opcode;
		/** Queueing destination for the enqueue
		 * (enqueue_id_sel = 0,16bit) or Frame Queue ID for the enqueue
		 * (enqueue_id_sel = 1,24bit).*/
	uint32_t qd_fqid;
		/** Distribution hash value passed to QMan for distribution
		 * purpose on the enqueue. */
	uint16_t hash_value;
		/** Queueing Destination Priority. */
	uint8_t	qd_priority;
		/** Storage profile used to store frame data if additional
		* buffers are required*/
	uint8_t	 spid;
		/** Enqueue Priority source
		* - 0: use QD_PRI provided with DMA Command
		* - 1: use QD_PRI from h/w context. This is the value
		* found in the WQID field from ADC. */
	uint8_t	PS;
		/** Terminate Control:
		* - 0: Return after enqueue.
		* - 1: Terminate: this command will trigger the Terminate task
		* command right after the enqueue. If the enqueue failed, the
		* frame will be discarded.
		* - 2: Conditional Terminate: trigger the Terminate task
		* command only if the enqueue succeeded. If the enqueue
		* failed, the frame handle is not released and the command
		* returns with an error code.
		* - 3: reserved */
	uint8_t	TC;
		/** Enqueue ID selection:
		* - 0 = queueing destination(16bit)
		* - 1 = fqid (24bit). */
	uint8_t	EIS;
		/** Command returned status. */
	int8_t  status;
};

/**************************************************************************//**
@Description	FDMA Enqueue Working Frame explicit Command structure.

		Includes information needed for FDMA Enqueue Working Frame
		command verification.

*//***************************************************************************/
struct fdma_enqueue_wf_exp_command {
		/** FDMA Enqueue working frame explicit command structure
		* identifier. */
	uint32_t opcode;
		/** Queueing destination for the enqueue
		 * (enqueue_id_sel = 0,16bit) or Frame Queue ID for the enqueue
		 * (enqueue_id_sel = 1,24bit).*/
	uint32_t qd_fqid;
		/** Distribution hash value passed to QMan for distribution
		 * purpose on the enqueue. */
	uint16_t hash_value;
		/** Working Frame handle to enqueue. */
	uint8_t	frame_handle;
		/** Queueing Destination Priority. */
	uint8_t	qd_priority;
		/** Storage profile used to store frame data if additional
		* buffers are required*/
	uint8_t	 spid;
		/** Enqueue Priority source
		* - 0: use QD_PRI provided with DMA Command
		* - 1: use QD_PRI from h/w context. This is the value
		* found in the WQID field from ADC. */
	uint8_t	PS;
		/** Terminate Control:
		* - 0: Return after enqueue.
		* - 1: Terminate: this command will trigger the Terminate task
		* command right after the enqueue. If the enqueue failed, the
		* frame will be discarded.
		* - 2: Conditional Terminate: trigger the Terminate task
		* command only if the enqueue succeeded. If the enqueue
		* failed, the frame handle is not released and the command
		* returns with an error code.
		* - 3: reserved */
	uint8_t	TC;
		/** Enqueue ID selection:
		* - 0 = queueing destination(16bit)
		* - 1 = fqid (24bit). */
	uint8_t	EIS;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[7];
};

/**************************************************************************//**
@Description	FDMA Enqueue Frame Command structure.

		Includes information needed for FDMA Enqueue Frame
		command verification (the frame must be closed, i.e. - no WF
		(frame is not presented)).

*//***************************************************************************/
struct fdma_enqueue_frame_command {
		/** FDMA Enqueue frame command structure identifier. */
	uint32_t opcode;
		/**< Queueing destination for the enqueue
		 * (enqueue_id_sel = 0,16bit) or Frame Queue ID for the enqueue
		 * (enqueue_id_sel = 1,24bit).*/
	uint32_t qd_fqid;
		/** ICID of the FD to enqueue. */
	uint16_t icid;
		/** Distribution hash value passed to QMan for distribution
		 * purpose on the enqueue. */
	uint16_t hash_value;
		/** Queueing Destination Priority. */
	uint8_t	qd_priority;
		/** Enqueue Priority source
		* - 0: use QD_PRI provided with DMA Command
		* - 1: use QD_PRI from h/w context. This is the value
		* found in the WQID field from ADC. */
	uint8_t	PS;
		/** Terminate Control:
		* - 0: Return after enqueue.
		* - 1: Terminate: this command will trigger the Terminate task
		* command right after the enqueue. If the enqueue failed, the
		* frame will be discarded.
		* - 2: Conditional Terminate: trigger the Terminate task
		* command only if the enqueue succeeded. If the enqueue
		* failed, the frame handle is not released and the command
		* returns with an error code.
		* - 3: reserved */
	uint8_t	TC;
		/** Enqueue ID selection:
		* - 0 = queueing destination(16bit)
		* - 1 = fqid (24bit). */
	uint8_t	EIS;
		/** Virtual Address. */
	uint8_t	VA;
		/** Privilege Level. */
	uint8_t	PL;
		/** Bypass DPAA resource Isolation:
		* - 0: Isolation is enabled for this command. The FQID ID
		* specified is virtual within the specified ICID.
		* - 1: Isolation is not enabled for this command. The FQID ID
		* specified is a real (not virtual) pool ID. */
	uint8_t	BDI;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[4];
};

/**************************************************************************//**
@Description	FDMA Enqueue Frame explicit Command structure.

		Includes information needed for FDMA Enqueue Frame explicit
		command verification (the frame must be closed, i.e. - no WF
		(frame is not presented)).

*//***************************************************************************/
struct fdma_enqueue_frame_exp_command {
		/** FDMA Enqueue frame explicit command structure identifier. */
	uint32_t opcode;
		/**< Queueing destination for the enqueue
		 * (enqueue_id_sel = 0,16bit) or Frame Queue ID for the enqueue
		 * (enqueue_id_sel = 1,24bit).*/
	uint32_t qd_fqid;
		/** Frame Descriptor to enqueue. */
	struct ldpaa_fd fd;
		/** ICID of the FD to enqueue. */
	uint16_t icid;
		/** Distribution hash value passed to QMan for distribution
		 * purpose on the enqueue. */
	uint16_t hash_value;
		/** Queueing Destination Priority. */
	uint8_t	qd_priority;
		/** Enqueue Priority source
		* - 0: use QD_PRI provided with DMA Command
		* - 1: use QD_PRI from h/w context. This is the value
		* found in the WQID field from ADC. */
	uint8_t	PS;
		/** Terminate Control:
		* - 0: Return after enqueue.
		* - 1: Terminate: this command will trigger the Terminate task
		* command right after the enqueue. If the enqueue failed, the
		* frame will be discarded.
		* - 2: Conditional Terminate: trigger the Terminate task
		* command only if the enqueue succeeded. If the enqueue
		* failed, the frame handle is not released and the command
		* returns with an error code.
		* - 3: reserved */
	uint8_t	TC;
		/** Enqueue ID selection:
		* - 0 = queueing destination(16bit)
		* - 1 = fqid (24bit). */
	uint8_t	EIS;
		/** Virtual Address. */
	uint8_t	VA;
		/** Privilege Level. */
	uint8_t	PL;
		/** Bypass DPAA resource Isolation:
		* - 0: Isolation is enabled for this command. The FQID ID
		* specified is virtual within the specified ICID.
		* - 1: Isolation is not enabled for this command. The FQID ID
		* specified is a real (not virtual) pool ID. */
	uint8_t	BDI;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[4];
};

/**************************************************************************//**
@Description	FDMA Discard Default Working Frame Segment Command structure.

		Includes information needed for FDMA Discard Working Frame
		command verification.

*//***************************************************************************/
struct fdma_discard_default_wf_command {
		/** FDMA Discard Default Working Frame command structure
		* identifier. */
	uint32_t opcode;
		/* Frame Source. Currently not supported since only FS = 0 is
		* supported in rev1.
		* - 0: discard working frame (using FRAME_HANDLE)
		* - 1: discard Frame (using FD at FD_ADDRESS).
	uint8_t	 FS; */
		/** Control:
		* - 0: Return after discard
		* - 1: Trigger the Terminate task command right after
		* the discard. */
	uint8_t	 TC;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[2];
};

/**************************************************************************//**
@Description	FDMA Discard Working Frame Segment Command structure.

		Includes information needed for FDMA Discard Working Frame
		command verification.

*//***************************************************************************/
struct fdma_discard_wf_command {
		/** FDMA Discard Working Frame command structure
		* identifier. */
	uint32_t opcode;
		/** Frame handle to discard. */
	uint16_t frame;
		/* Frame Source. Currently not supported since only FS = 0 is
		* supported in rev1.
		* - 0: discard working frame (using FRAME_HANDLE)
		* - 1: discard Frame (using FD at FD_ADDRESS).
	uint8_t	 FS; */
		/** Control:
		* - 0: Return after discard
		* - 1: Trigger the Terminate task command right after
		* the discard. */
	uint8_t	 TC;
		/** Command returned status. */
	int8_t	status;
};

/**************************************************************************//**
@Description	FDMA Discard Frame Segment Command structure.

		Includes information needed for FDMA Discard Frame command
		verification.

*//***************************************************************************/
struct fdma_terminate_task_command {
		/** FDMA Terminate task command structure
		 * identifier. */
	uint32_t opcode;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[3];
};

/**************************************************************************//**
@Description	Replicate frame parameters structure.

*//***************************************************************************/
struct fdma_replicate_frames_command {
		/** FDMA replicate frame command structure
		 * identifier. */
	uint32_t opcode;
		/** A pointer to the location within the workspace of the
		 * destination FD. */
	uint32_t fd_dst;
		/** Queueing destination for the enqueue
		 * (enqueue_id_sel = 0,16bit) or Frame Queue ID for the enqueue
		 * (enqueue_id_sel = 1,24bit).*/
	uint32_t qd_fqid;
		/** Distribution hash value passed to QMan for distribution
		 * purpose on the enqueue. */
	uint16_t hash_value;
		/** Queueing Destination Priority. */
	uint8_t	qd_priority;
		/** Handle of the source frame. */
	uint8_t	frame_handle1;
		/** Storage Profile used to store frame data of the
		 * destination frame if \ref FDMA_REPLIC_ENQ_BIT is selected */
	uint8_t	spid;
		/** Enqueue Priority source
		* - 0: use QD_PRI provided with DMA Command
		* - 1: use QD_PRI from h/w context. This is the value
		* found in the WQID field from ADC. */
	uint8_t	PS;
		/** Enqueue ID selection:
		* - 0 = queueing destination(16bit)
		* - 1 = fqid (24bit). */
	uint8_t	EIS;
		/** Enqueue the replicated frame to the provided Queueing
		 * Destination.
		 * Release destination frame handle is implicit when enqueueing.
		 * - 0: replicate only
		 * - 1: replicate and enqueue */
	uint8_t	ENQ;
		/** The source frame resources are released after the
		 * replication.
		 * Release source frame handle is implicit when discarding.
		 * - 0: keep source frame
		 * - 1: discard source frame and release frame handle */
	uint8_t	DSF;
		/** Frame annotation copy option:
		 * - 0: do not copy annotations.
		 * - 1: copy ASA.
		 * - 2: copy PTA.
		 * - 3: copy ASA + PTA. */
	uint8_t CFA;
		/** Returned parameter:
		 * The Handle of the replicated frame (when no
		 * enqueue was selected). */
	uint8_t frame_handle2;
		/** Command returned status. */
	int8_t	status;
};

/**************************************************************************//**
@Description	Concatenate frames parameters structure.

*//***************************************************************************/
struct fdma_concatenate_frames_command {
		/** FDMA Concatenate frames command structure
		 * identifier. */
	uint32_t opcode;
		/** The handle of working frame 1. */
	uint16_t frame1;
		/** The handle of working frame 2. */
	uint16_t frame2;
		/** Storage Profile used to store frame data if additional
		 * buffers are required when optionally closing the concatenated
		 *  working frame (PCA is set) */
	uint8_t  spid;
		/** Trim a number of bytes from the beginning of frame 2 before
		 * the concatenation. A size of zero disables the trim. */
	uint8_t  trim;
		/** Set SF bit on SGE of the original frames. Note, this will
		 * force the usage of SG format on the concatenated frame.
		 * - 0: Do not set SF bit.
		 * - 1: Set SF bit. */
	uint8_t SF;
		/** Post Concatenate Action:
		 * - 0: keep resulting working frame 1 open
		 * - 1: close resulting working frame 1 using provided storage
		 * profile, update FD1 */
	uint8_t PCA;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[3];
};

/**************************************************************************//**
@Description	Split frame parameters structure.

*//***************************************************************************/
struct fdma_split_frame_command {
		/** FDMA Split frame command structure
		 * identifier. */
	uint32_t opcode;
		/** A pointer to the location within the workspace to present
		 * the split frame segment. */
	uint32_t seg_dst;
		/** A pointer to the location within the workspace of the split
		 * FD. */
	uint32_t fd_dst;
		/** location within the presented split frame to start
		 * presenting the segment from. */
	uint16_t seg_offset;
		/** Number of frame bytes to present and create an open segment
		 * for (in the split frame). */
	uint16_t present_size;
		/** SM = 0: Split size, number of bytes to split from the head
		 * of the input frame and move to the output frame.
		 * SM = 1: Backward offset from the SGE marked with the SF bit,
		 * used by the FDMA to modify the start offset in the structure
		 * to recover frame data that is currently in the structure but
		 * not part of the frame.
		 * A value of 0 will have no effect on start offset. */
	uint16_t split_size_sf;
		/** Returned parameter:
		 * The number of bytes actually presented from the split frame
		 * (the segment actual size). */
	uint16_t seg_length;
		/** The handle of the source working frame. */
	uint8_t  source_frame_handle;
		/** Returned parameter:
		 * A pointer to the handle of the split working frame. */
	uint8_t  split_frame_handle;
		/** Returned parameter:
		 * A pointer to the handle of the presented segment (in the
		 * split frame). */
	uint8_t  seg_handle;
		/** Storage Profile used to store frame data if additional
		 * buffers are required when optionally closing the split
		 * working frame */
	uint8_t  spid;
		/** Frame annotation copy option:
		 * - 0: do not copy annotations.
		 * - 1: copy ASA.
		 * - 2: copy PTA.
		 * - 3: copy ASA + PTA.
		 * */
	uint8_t CFA;
		/** Post split action:
		 * - 0: do not present segment from the split frame, keep split
		 * working frame open.
		 * - 1: present segment from the split frame, keep split working
		 * frame open.
		 * - 2: do not present, close split working frame. */
	uint8_t PSA;
		/** Frame split mode:
		 * - 0: Split is performed at the split_size_sf value.
		 * - 1: Split is performed based on Scatter Gather Entry SF
		 * bit */
	uint8_t SM;
		/** Reference within the frame to present from:
		- 0: start of the frame.
		- 1: end of the frame. */
	uint8_t SR;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[3];
};


/**************************************************************************//**
@Description	FDMA Trim Working Frame Segment Command structure.

		Includes information needed for FDMA Replace Working Frame
		Segment command verification.

*//***************************************************************************/
struct fdma_trim_command {
		/** FDMA Trim Working Frame Segment command structure
		* identifier. */
	uint32_t opcode;
		/** Offset from the previously presented segment representing
		 * the new start of the segment (head trim). */
	uint16_t offset;
		/** New segment size in bytes. */
	uint16_t size;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[7];
};

/**************************************************************************//**
@Description	FDMA Modify Working Frame Segment Command structure.

		Includes information needed for FDMA Modify Working Frame
		Segment command verification. This command updates the FDMA that
		certain data in the presented segment was modified. The updated
		data is located in the same place the old data was located at in
		the segment presentation in workspace.

*//***************************************************************************/
struct fdma_modify_command {
		/** FDMA Modify Working Frame Segment command structure
		* identifier. */
	uint32_t opcode;
		/** Offset from the previously presented segment representing
		 * the start point of the replacement. */
	uint16_t offset;
		/** Replaced size. */
	uint16_t size;
		/** Replacing Data. If size > 24 then the replacing data will
		 * be wrapped. */
	uint8_t	data[24];
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[7];
};

/**************************************************************************//**
@Description	FDMA Replace Working Frame Segment Command structure.

		Includes information needed for FDMA Replace Working Frame
		Segment command verification.

*//***************************************************************************/
struct fdma_replace_command {
		/** FDMA Replace Working Frame Segment command structure
		* identifier. */
	uint32_t opcode;
		/** Pointer to the workspace start location of the replacement
		* segment data. */
	uint32_t from_ws_src;
		/** Pointer to the workspace start location of represented
		* segment (relevant if (flags == \ref
		* FDMA_REPLACE_SA_REPRESENT_BIT)). */
	uint32_t ws_dst_rs;
		/** Offset from the previously presented segment representing
		 * the start point of the replacement. */
	uint16_t to_offset;
		/** Working Frame replaced size. */
	uint16_t to_size;
		/** Replacing segment size. */
	uint16_t from_size;
		/** Number of frame bytes to represent. Must be greater than 0.
		 *  Relevant if SA field is set. */
	uint16_t size_rs;
		/** Command returned segment length. (relevant if
		(flags == \ref FDMA_REPLACE_SA_REPRESENT_BIT))*/
	uint16_t seg_length_rs;
		/** Segment Action.
		* - 0: keep segment open
		* - 1: represent segment
		* - 2: close segment */
	uint8_t	SA;
		/** Command returned status. */
	int8_t	status;
};

/**************************************************************************//**
@Description	FDMA Insert data to Working Frame Segment Command structure.

		Includes information needed for FDMA Insert Working Frame
		Segment command verification.

*//***************************************************************************/
struct fdma_insert_segment_data_command {
		/** FDMA Insert data to Working Frame Segment command structure
		* identifier. */
	uint32_t opcode;
		/** Pointer to the workspace start location of the replacement
		* segment data. */
	uint32_t from_ws_src;
		/** Offset from the previously presented segment representing
		* the start point of the replacement. */
	uint16_t to_offset;
		/** Inserted segment data size. */
	uint16_t insert_size;
		/** Command returned segment length. (relevant if
		 * (flags == \ref FDMA_REPLACE_SA_REPRESENT_BIT))*/
	uint16_t seg_length_rs;
		/** Segment Action.
		* - 0: keep segment open
		* - 1: represent segment
		* - 2: close segment */
	uint8_t	SA;
		/** Command returned status. */
	int8_t	status;
};

/**************************************************************************//**
@Description	FDMA Insert explicit data to Working Frame Segment Command structure.

		Includes information needed for FDMA Insert Working Frame
		Segment command verification.

*//***************************************************************************/
struct fdma_insert_segment_data_exp_command {
		/** FDMA Insert data to Working Frame Segment explicit command
		 * structure identifier. */
	uint32_t opcode;
		/** Pointer to the workspace start location of the replacement
		* segment data. */
	uint32_t from_ws_src;
		/**< pointer to the location in workspace for the represented
		 * frame segment (relevant if \ref FDMA_REPLACE_SA_REPRESENT_BIT
		 *  flag is set). */
	uint32_t ws_dst_rs;
		/** Offset from the previously presented segment representing
		* the start point of the replacement. */
	uint16_t to_offset;
		/** Inserted segment data size. */
	uint16_t insert_size;
	/**< Number of frame bytes to represent. Must be greater than 0.
	 * Relevant if SA field is set. */
	uint16_t size_rs;
		/** Command returned segment length.
		 * Relevant if SA_REPRESENT_BIT))*/
	uint16_t seg_length_rs;
		/**< Working frame handle to which the data is being inserted.*/
	uint8_t	 frame_handle;
		/**< Data segment handle (related to the working frame handle)
		 * from which the data is being inserted. */
	uint8_t  seg_handle;
		/** Segment Action.
		* - 0: keep segment open
		* - 1: represent segment
		* - 2: close segment */
	uint8_t	SA;
		/** Command returned status. */
	int8_t	status;
};

/**************************************************************************//**
@Description	FDMA Delete data from Working Frame Segment Command structure.

		Includes information needed for FDMA Delete data from Working
		Frame Segment command verification.

*//***************************************************************************/
struct fdma_delete_segment_data_command {
		/** FDMA Delete data from Working Frame Segment command
		 * structure identifier. */
	uint32_t opcode;
		/** Offset from the previously presented segment representing
		* the start point of the replacement. */
	uint16_t to_offset;
		/** Deleted segment data size. */
	uint16_t delete_target_size;
		/** Command returned segment length.
		 * Relevant if SA field is set. */
	uint16_t seg_length_rs;
		/** Segment Action.
		 * - 0: keep segment open
		 * - 1: represent segment
		 * - 2: close segment */
	uint8_t	SA;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[4];
};

/**************************************************************************//**
@Description	FDMA Close Working Frame default Segment Command structure.

		Includes information needed for FDMA Close Working
		Frame Segment command verification.

*//***************************************************************************/
struct fdma_close_segment_command {
		/** FDMA Close Working Frame Segment command structure
		 * identifier. */
	uint32_t opcode;
		/** Command returned status. */
	int8_t	status;
		/** 64-bit alignment. */
	uint8_t	pad[3];
};

/**************************************************************************//**
@Description	FDMA Replace Working Frame ASA Segment Command structure.

		Includes information needed for FDMA Replace Working Frame
		ASA Segment command verification.

*//***************************************************************************/
struct fdma_replace_asa_command {
		/** FDMA Replace Working Frame ASA Segment command structure
		* identifier. */
	uint32_t opcode;
		/** Pointer to the workspace start location of the replacement
		* segment data. */
	uint32_t from_ws_src;
		/** Pointer to the workspace start location of represented
		* segment (relevant if (flags == \ref
		* FDMA_REPLACE_SA_REPRESENT_BIT)). */
	uint32_t ws_dst_rs;
		/** Offset from the beginning of the ASA data representing the
		 * start point of the replacement specified in
		 * 64B quantities. */
	uint16_t to_offset;
		/** The number of 64B quantities that are to be replaced
		 * within the ASA segment. */
	uint16_t to_size;
		/** The number of 64B units that will replace the
		 * specified portion of the ASA segment. */
	uint16_t from_size;
		/** Number of frame bytes to represent in 64B portions.
		 * Must be greater than 0.
		 * (relevant if (flags==\ref FDMA_REPLACE_SA_REPRESENT_BIT)). */
	uint16_t size_rs;
		/** Command returned segment length in 64 bytes units.
		 * (relevant if (flags == \ref FDMA_REPLACE_SA_REPRESENT_BIT)).
		 * */
	uint16_t seg_length_rs;
		/** Segment Action.
		* - 0: keep segment open
		* - 1: represent segment
		* */
	uint8_t	SA;
		/** Command returned status. */
	int8_t	status;
};

/**************************************************************************//**
@Description	FDMA Replace Working Frame PTA Segment Command structure.

		Includes information needed for FDMA Replace Working Frame
		PTA Segment command verification.

*//***************************************************************************/
struct fdma_replace_pta_command {
		/** FDMA Write Replace Frame PTA Segment command structure
		* identifier. */
	uint32_t opcode;
		/** Pointer to the workspace start location of the replacement
		* segment data. */
	uint32_t from_ws_src;
		/** Pointer to the workspace start location of represented
		* segment (relevant if (flags == \ref
		* FDMA_REPLACE_SA_REPRESENT_BIT)). */
	uint32_t ws_dst_rs;
		/**
		 * - 0x1: 32B of PTA will be read from WS_START_ADDRESS and
		 * become the frame PTA. PTV1=1 and PTV2=0 afterwards.
		 * - 0x2: 32B of PTA will be read from WS_START_ADDRESS and
		 * become the frame PTA. PTV1=0 and PTV2=1 afterwards.
		 * - 0x3: 64B of PTA will be read from WS_START_ADDRESS and
		 * become the frame PTA. PTV1=1 and PTV2=1 afterwards.
		 * - All other values are reserved when this command is invoked.
		 * */
	uint16_t size;
		/** Command returned segment length. (relevant if
		 * (flags == \ref FDMA_REPLACE_SA_REPRESENT_BIT)):
		 * - 0x0 = No PTA presented. PTV1=0 and PTV2=0 in the working
		 * frame.
		 * - 0x1: 32B of PTA presented. PTV1=1 and PTV2=0 in the working
		 * frame.
		 * - 0x2: 32B of PTA presented. PTV1=0 and PTV2=1 in the working
		 * frame.
		 * - 0x3: 64B of PTA presented. PTV1=1 and PTV2=1 in the working
		 * frame.*/
	uint16_t seg_length;
		/** Segment Action.
		* - 0: keep segment open
		* - 1: represent segment
		* */
	uint8_t	SA;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[6];
};

/**************************************************************************//**
@Description	FDMA Checksum Working Frame Command structure.

		Includes information needed for FDMA Checksum Working Frame
		command verification.

*//***************************************************************************/
struct fdma_checksum_command {
		/** FDMA Checksum Working Frame command structure
		 * identifier. */
	uint32_t opcode;
		/** Number of bytes offset in the frame from which to start
		 * calculation of checksum. */
	uint16_t offset;
		/** Number of bytes to do calculation of checksum.
		 * Use 0xffff to calculate check until the end of the frame. */
	uint16_t size;
		/** Command returned Ones complement sum over the specified
		 * range of the working frame. */
	uint16_t checksum;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[5];
};

/**************************************************************************//**
@Description	FDMA Copy Data Command structure.

		Includes information needed for FDMA Copy data command
		verification.

*//***************************************************************************/
struct fdma_copy_command {
		/** FDMA Copy Data command structure identifier. */
	uint32_t opcode;
		/** A pointer to the location in the workspace/AIOP Shared
		 * memory of the source data. */
	uint32_t src;
		/** A pointer to the location in the workspace/AIOP Shared
		 * memory to store the copied data. */
	uint32_t dst;
		/** Number of bytes to copy from source to destination. */
	uint16_t copy_size;
		/** Source Memory:
		- 0: Workspace.
		- 1: AIOP Shared Memory. */
	uint8_t SM;
		/** Destination Memory:
		- 0: Workspace.
		- 1: AIOP Shared Memory. */
	uint8_t DM;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[7];
};

/**************************************************************************//**
@Description	FDMA Acquire Buffer Command structure.

		Includes information needed for FDMA Acquire buffer command
		verification.

*//***************************************************************************/
struct fdma_acquire_buffer_command {
		/** FDMA Acquire buffer command structure identifier. */
	uint32_t opcode;
		/** A pointer to the location in the workspace where to return
		 * the acquired 64 bit buffer address. */
	uint32_t dst;
		/** Buffer Pool ICID. */
	uint16_t icid;
		/** Buffer pool ID used for the Acquire Buffer. */
	uint16_t bpid;
		/** Bypass DPAA resource Isolation:
		 * If reset - Isolation is enabled for this command.
		 * The pool ID specified is virtual within the specified ICID.
		 * If set - Isolation is not enabled for this command.
		 * The pool ID specified is a real (not virtual) pool ID. */
	uint8_t bdi;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[2];
};

/**************************************************************************//**
@Description	FDMA Release Buffer Command structure.

		Includes information needed for FDMA Release buffer command
		verification.

*//***************************************************************************/
struct fdma_release_buffer_command {
		/** FDMA Release buffer command structure identifier. */
	uint32_t opcode;
		/** Buffer Pool ICID. */
	uint16_t icid;
		/** Buffer pool ID used for the Acquire Buffer. */
	uint16_t bpid;
		/** Buffer address to be released. */
	uint64_t addr;
		/** Bypass DPAA resource Isolation:
		 * If reset - Isolation is enabled for this command.
		 * The pool ID specified is virtual within the specified ICID.
		 * If set - Isolation is not enabled for this command.
		 * The pool ID specified is a real (not virtual) pool ID. */
	uint8_t bdi;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[6];
};

/**************************************************************************//**
@Description	FDMA Create Frame Command structure.

		Includes information needed for Create Frame data command
		verification.

*//***************************************************************************/
struct fdma_create_frame_command {
		/** Create Frame command structure identifier. */
	uint32_t opcode;
		/** A pointer to the location in workspace of the data to be
		 * inserted to the frame.
		 * The data MUST be located in workspace prior to calling this
		 * command. */
	uint32_t data;
		/** Data size to be inserted to the frame. */
	uint16_t size;
		/** Command returned status. */
	int8_t  status;
		/** 64-bit alignment. */
	uint8_t	pad[5];
		/** Command returned Frame Descriptor for the created frame.
		 * The command updates the FD in workspace, and when the ASA is
		 * written back to the frame, the updated FD will be written to
		 * the frame as well.
		 * The FD address in workspace must be aligned to 32 bytes. */
	struct ldpaa_fd fd;
};


/** @} */ /* end of AIOP_FDMA_SRs_Verification */

/** @}*/ /* end of AIOP_Service_Routines_Verification */


uint16_t aiop_verification_fdma(uint32_t asa_seg_addr);


#endif /* __AIOP_VERIFICATION_FDMA_H_ */
