/**************************************************************************//**
@File		ipsec.h

@Description	This file contains the AIOP IPSec 
		internal functions and definitions.
		
*//***************************************************************************/

#ifndef __AIOP_IPSEC_H
#define __AIOP_IPSEC_H

#include "common/types.h"


/**************************************************************************//**
 @Group		NETF NETF (Network Libraries)

 @Description	AIOP Accelerator APIs

 @{
*//***************************************************************************/
/**************************************************************************//**
@Group	FSL_IPSEC IPSEC

@Description	Freescale AIOP IPsec API

@{
*//***************************************************************************/

/**************************************************************************//**
 @Group		IPSEC_ENUM IPsec Enumerations

 @Description	IPsec Enumerations

 @{
*//***************************************************************************/

enum ipsec_cipher_type {
	CIPHER_TYPE_CBC = 0, 	/* CBC */
	CIPHER_TYPE_CTR, /* CTR */
	CIPHER_TYPE_CCM, /* CCM */
	CIPHER_TYPE_GCM /* GCM */
};


/* @} end of IPSEC_ENUM */

/**************************************************************************//**
@Group	FSL_IPSEC_MACROS IPsec Macros

@Description	Freescale IPsec Macros

@{
*//***************************************************************************/

/* Inbound/Outbound Flag (most significant bit) */
#define IPSEC_FLG_DIR_INBOUND  0x00000000 
#define IPSEC_FLG_DIR_OUTBOUND 0x80000000 

/* PS Pointer Size. This bit determines the size of address pointers */
#define IPSEC_SEC_POINTER_SIZE 1 /* 1 - SEC Pointers require two 32-bit words */ 

#define IPSEC_PROFILE_SRAM_ADDR 0x00030000 /* hard wired address */
#define IPSEC_STORAGE_PROFILE_SIZE_SHIFT 5 /* 32 bytes */
#define IPSEC_INTERNAL_PARMS_SIZE 128 /* 128 bytes */
#define IPSEC_FLOW_CONTEXT_SIZE 64 /* 64 bytes */
#define IPSEC_ENC_PDB_HMO_MASK 0xFF00
#define IPSEC_DEC_PDB_HMO_MASK 0xF000
#define IPSEC_PDB_OPTIONS_MASK 0x00FF

#define IPSEC_DECAP_PDB_ARS_MASK 0xC0 /* PDB options[ARS], bits 7:6 */

#define IPSEC_DEC_OPTS_ETU 	0x0001 /** ETU: EtherType Update 
* If set this has two implications: 
* - The first PDB:AOIPHO-2 bytes of the Outer IP Header Material 
* 	are copied to the output frame.validate IP header checksum 
* - The EtherType field is updated */

/* OPTIONS[3:2] - OIHI: Outer IP Header Included 
 * 00 : No Outer IP Header provided
 * 01 : First PDB:Opt IP Hdr Len bytes of Input frame is the
 * 		Outer IP Header Material to be included in Output Frame
 * 10 : PDB contains address to Outer IP Header Material to be
 * 		included in Output Frame (length is PDB:Opt IP Hdr Len  bytes)
 * 11 : PDB contains Outer IP Header Material to be included in Output Frame 
 * 		(length is PDB:Opt IP Hdr Len bytes)
*/
#define IPSEC_ENC_PDB_OPTIONS_OIHI_PDB 0x0C

/* NAT and NUC Options for tunnel mode encapsulation */
/* Bit 1 : NAT Enable RFC 3948 UDP-encapsulated-ESP */
/* Bit 0 : NUC Enable NAT UDP Checksum */
#define IPSEC_ENC_PDB_OPTIONS_NAT 0x02
#define IPSEC_ENC_PDB_OPTIONS_NUC 0x01

/* 28 (HMO 4 out of 7:0) Sequence Number Rollover control. 
 * 0 : Sequence Number Rollover causes an error
 * 1 : Sequence Number Rollover permitted
*/
#define IPSEC_ENC_PDB_HMO_SNR 0x10


#define IPSEC_ARS_MASK	0x00c0   /* anti-replay window option mask */
#define IPSEC_ESN_MASK 0x10 /* Extended sequence number option mask */
#define IPSEC_SEC_NEW_BUFFER_MODE 0
#define IPSEC_SEC_REUSE_BUFFER_MODE 1

#define IPSEC_SA_DESC_BUF_SIZE 512 /* SA descriptor buffer size */
#define IPSEC_SA_DESC_BUF_ALIGN 64 /* SA descriptor alignment */
#define IPSEC_KEY_BUF_SIZE 512 /* Key buffer size */
#define IPSEC_MAX_NUM_OF_TASKS 256 /* Total maximum number of tasks in AIOP */
#define IPSEC_MEM_PARTITION_ID MEM_PART_DP_DDR
					/* Memory partition ID */
#define IPSEC_MAX_ASA_SIZE 960 /* Maximum ASA size (960 bytes) */
#define IPSEC_MAX_ASA_BUF_ALIGN 8 /* ASA buffer alignment */

/**< Align a given address - equivalent to ceil(ADDRESS,ALIGNMENT) */
#define IPSEC_ALIGN_64(ADDRESS, ALIGNMENT)           \
        ((((uint64_t)(ADDRESS)) + ((uint64_t)(ALIGNMENT)) - 1) & \
        								(~(((uint64_t)(ALIGNMENT)) - 1)))

#define IPSEC_DESC_ALIGN(ADDRESS) \
	IPSEC_ALIGN_64((ADDRESS), IPSEC_SA_DESC_BUF_ALIGN)

/* Aligned Descriptor Address (parameters area start) */
#define IPSEC_DESC_ADDR(ADDRESS) IPSEC_DESC_ALIGN(ADDRESS)

/* Flow Context Address */
#define IPSEC_FLC_ADDR(ADDRESS) ((ADDRESS) + IPSEC_INTERNAL_PARMS_SIZE)

/* Shared Descriptor Address */
#define IPSEC_SD_ADDR(ADDRESS) \
	((ADDRESS) + IPSEC_INTERNAL_PARMS_SIZE + IPSEC_FLOW_CONTEXT_SIZE)

/*
* Big-endian systems are systems in which the most significant byte of the word 
* is stored in the smallest address given and the least significant byte 
* is stored in the largest. 
* In contrast, little endian systems are those in which the 
* least significant byte is stored in the smallest address.
*/
/* Little Endian
 Register:
* +---------------------------------------------------------------+
* |   A   |   B   |   C   |   D   |   E   |   F   |   G   |   H   |
* |  MSB  |       |       |       |       |       |       |  LSB  |
* +---------------------------------------------------------------+
* Bytes address
* |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
* +---------------------------------------------------------------+

Big Endian
 Register:
* +---------------------------------------------------------------+
* |   H   |   G   |   F   |   E   |   D   |   C   |   B   |   A   |
* |  LSB  |       |       |       |       |       |       |  MSB  |
* +---------------------------------------------------------------+
* Bytes address
* |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
* +---------------------------------------------------------------+
*/

/* FLC[63:0] = { 16�b0, checksum[15:0], byte_count[31:0] } */
/* FLC Little Endian Format 
* +---------------------------------------------------------------+
* |   x   |   x   |  CS1  |  CS0  |  BC3  |  BC2  |  BC1  |  BC0  |
* |       |       |  MSB  |  LSB  |  MSB  |       |       |  LSB  |
* +---------------------------------------------------------------+
* Byte address
* |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
* +---------------------------------------------------------------+

* FLC Big Endian Format 
* +---------------------------------------------------------------+
* |  BC0  |  BC1  |  BC2  |  BC3  |  CS1  |  CS0  |   x   |   x   |
* |  LSB  |       |  MSB  |  MSB  |  LSB  |  MSB  |       |       |
* +---------------------------------------------------------------+
* Byte address
* |   7   |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
* +---------------------------------------------------------------+
*/
/* FLC Checksum Little Endian Mask */
#define IPSEC_RETURN_FLC_CHECKSUM_LE_MASK 0x0000FFFF00000000
/* FLC Checksum Big Endian Mask */
#define IPSEC_RETURN_FLC_CHECKSUM_BE_MASK 0x00000000FFFF0000
/* FLC Byte Count Big Endian Mask */
#define IPSEC_RETURN_FLC_BCNT_BE_MASK 0xFFFFFFFF00000000

/* FLC Checksum Big Endian Shift */
#define IPSEC_RETURN_FLC_CHECKSUM_BE_SHIFT 16
/* FLC Byte Count Big Endian Shift */
#define IPSEC_RETURN_FLC_BCNT_BE_SHIFT 32

/* AAP Command Fields */
#define  IPSEC_AAP_USE_FLC_SP 0x10000000
#define  IPSEC_AAP_OS_EX 0x00800000

// TODO: temporary, need to update general.h
/** AAP SEC accelerator ID  (according to Archdef 7.5)*/
#define AAP_SEC_ACCEL_ID	0x03

/* DPOVRD OVRD */
#define IPSEC_DPOVRD_OVRD 0x80000000

// TMP, removed from the external API
/** Frames do not include a L2 Header */
#define IPSEC_FLG_NO_L2_HEADER		0x00000010

/* Output IP header source options. Use one of the following options. 
 * Relevant for tunnel mode only */
#define IPSEC_ENC_OPTS_IPHDR_SRC_NONE	0x00 /* IP header not included */
#define IPSEC_ENC_OPTS_IPHDR_SRC_FRAME 	0x04 /* IP header from input frame */
#define IPSEC_ENC_OPTS_IPHDR_SRC_ADDR	0x08 /* IP header ref from parameters */
#define IPSEC_ENC_OPTS_IPHDR_SRC_PARAM	0x0c /* IP header from parameters */

/**************************************************************************//**
@Description	IPSec ESP Encapsulation HMO field  

		Use for ipsec_encap_params.hmo
*//***************************************************************************/

// TMP, removed from the external API
/* SNR: Sequence Number Rollover control 
 * If not set, a Sequence Number Rollover causes an error
 * if set, Sequence Number Rollover is permitted*/
#define IPSEC_HMO_ENCAP_SNR	0x01                             


/**************************************************************************//**
@Description	IPSec ESP Decapsulation HMO field  

		Use for ipsec_decap_params.hmo
*//***************************************************************************/

// TMP, removed from the external API
/* ODF: the DF bit in the IPv4 header in the output frame is replaced 
 * with the DFV value as shown below. 
 * Note: * Must not be set for IPv6 */
#define IPSEC_HMO_DECAP_ODF	0x08

/* DFV -- DF bit Value */
#define IPSEC_HMO_DECAP_DFV	0x04

/**************************************************************************//**
@Description	SEC Returned Status  

		Use for ipsec_decap_params.hmo
*//***************************************************************************/
/* SEC Job termination status/error word 
* bits 31-28      bits 3-0 / bits 7-0 
* (Source of      (ERRID)  / (Error Code)
*  the status 
*  code)
* -----------     ---------
* 2h (CCB)	    Ah - ICV check failed
* -----------     ---------
* 4h (DECO)		83h - Anti-replay LATE error
*				84h - Anti-replay REPLAY error
*				85h - Sequence number overflow
*/
/** No Error */
#define	SEC_NO_ERROR 0x00000000

/** ICV comparison failed */
#define	SEC_ICV_COMPARE_FAIL 0x2000000A

/** Anti Replay Check: Late packet */
#define	SEC_AR_LATE_PACKET 0x40000083
/** Anti Replay Check: Replay packet */
#define	SEC_AR_REPLAY_PACKET 0x40000084
/** Sequence Number overflow */
#define	SEC_SEQ_NUM_OVERFLOW 0x40000085

/* OSM temporary defines */
/* TODO: should move to general or OSM include file */
#define IPSEC_OSM_CONCURRENT			0
#define IPSEC_OSM_EXCLUSIVE				1

// TMP, removed from the external API
/**************************************************************************//**
 * @struct    ipsec_storage_params
 * @ingroup   ipsec_storage_params
 * @details   Container for IPsec descriptor storage parameters
*//***************************************************************************/
struct ipsec_storage_params {
	uint16_t sdid; /** Security domain ID */
	uint16_t spid; /** Storage profile ID of SEC output frame */
	uint8_t rife; /** Return input frame on error */
	uint8_t crid; /** Critical resource ID */
};

/* Global Parameters structure */
struct ipsec_global_params {
	uint32_t sa_count; /* SA (descriptors) counter. Initialized to max number */
	uint16_t asa_bpid; /* Buffer pool ID for ASA copy */
	uint16_t desc_bpid; /* Buffer pool ID for the SA descriptor */
	uint8_t tmi; /* Timer Instance ID  */
	uint8_t spinlock; /* Spinlock indicator, for SA counter  */
};

/* Instance Parameters structure */
struct ipsec_instance_params {
	uint32_t sa_count; /* SA (descriptors) counter. Initialized to max number */
	uint32_t committed_sa_num; /* Committed SAs (descriptors) */
	uint32_t max_sa_num; /* Maximum SAs (descriptors) */
	uint32_t instance_flags; /* Flags place holder */
	uint16_t asa_bpid; /* Buffer pool ID for ASA copy */
	uint16_t desc_bpid; /* Buffer pool ID for the SA descriptor */
	uint8_t tmi_id; /* TMAN Instance ID  */
};

/* Instance Parameters structure */
struct ipsec_global_instance_params {
	uint32_t instance_count; /* instances counter */
	uint32_t spinlock; /* spinlock */
};

/* Note: For ste_inc_and_acc_counters function, the accumulator memory address 
 * should be counter_addr + sizeof(counter) 
 * In this case "accumulator" = byte counter, "counter" = packets counter*/

#define IPSEC_PACKET_COUNTER_ADDR(ADDRESS) \
	(ADDRESS + (offsetof(struct ipsec_sa_params_part1, packet_counter)))

#define IPSEC_KB_COUNTER_ADDR(ADDRESS) \
	(ADDRESS + (offsetof(struct ipsec_sa_params_part1, byte_counter)))

#define IPSEC_FLAGS_ADDR(ADDRESS) \
	(ADDRESS + (offsetof(struct ipsec_sa_params_part1, flags)))

#define IPSEC_STATUS_ADDR(ADDRESS) \
	(ADDRESS + (offsetof(struct ipsec_sa_params_part1, status)))

#define IPSEC_INSTANCE_HANDLE_ADDR(ADDRESS) \
	(ADDRESS + (offsetof(struct ipsec_sa_params_part1, instance_handle)))


/* Shared descriptor address */
#define IPSEC_SHARED_DESC_ADDR(ADDRESS) (ADDRESS + \
	IPSEC_INTERNAL_PARMS_SIZE + IPSEC_FLOW_CONTEXT_SIZE)

/* PDB address */
#define IPSEC_PDB_ADDR(ADDRESS) (IPSEC_SHARED_DESC_ADDR(ADDRESS) + 4) 
	
#define IPSEC_MAX_TIMESTAMP 0xFFFFFFFFFFFFFFFF

/* SA Descriptor Parameter for Internal Usage */ 
/* Part 1 */
struct ipsec_sa_params_part1 {
	/* Required at Add descriptor and enc/dec */
	/* 6x8 = 48 bytes */
	uint64_t packet_counter; /*	Packets counter, 8B */
	uint64_t byte_counter; /* Encrypted/decrypted bytes counter, 8B */
	uint64_t timestamp; /* TMAN timestamp in micro-seconds, 8 Bytes */

	uint64_t soft_byte_limit; /* soft byte count limit,	8 Bytes */
	uint64_t soft_packet_limit; /* soft packet limit, 8B */
	uint64_t hard_byte_limit; /* hard byte count limit, 8B */
	uint64_t hard_packet_limit; /* hard packet limit, 8B */

	
	/* Always required, except timer callback */
	/* 2x4 + 2x2 + 4x1 = 8 + 4 + 4 = 16 bytes */
	uint32_t flags; /* 	transport mode, UDP encap, pad check, counters enable, 
					outer IP version, etc. 4B */
	uint32_t status; /* lifetime expire, semaphores	4-8B */

	ipsec_instance_handle_t instance_handle; /* Instance handle 8B */

	uint16_t udp_src_port; /* UDP source for transport mode. 2B */
	uint16_t udp_dst_port; /* UDP destination for transport mode. 2B */
	
	uint8_t valid; /* descriptor valid. 1B */
	uint8_t esn; /* Extended sequence number enabled. 1B */
	
	uint8_t anti_replay_size; /* none/32/64/128	1B */
	uint8_t sec_buffer_mode; /* new/reuse (for ASA copy). 1B */

	uint8_t output_spid; /* SEC output buffer SPID */
	
	/* Total size = */
	/* 8*8 (64) + 2*4 (8) + 2*2 (4) + 5*1 (5) = 81 bytes */
};
/* Part 2 */
struct ipsec_sa_params_part2 {
	
	/* Required at Add descriptor and timer callback */
	/* 2x8 + 3x4 = 16+12 = 28 bytes */
	uint64_t sec_callback_arg; /* SA handle used as argument for the 
								application callback function. 8B */
	uint32_t sec_callback_func; /* Application callback function, 
	 	 	 	 	to call upon a seconds lifetime expiry. 4B */
	
	uint32_t soft_tmr_handle; /* Soft seconds timer handle, 4B */
	uint32_t hard_tmr_handle; /* Hard seconds timer handle, 4B */
	
	/* Total size = 8 + 3*4 (12) = 20 bytes */
};

/* Total size for part 1 + part 2 = 64+28 = 92 bytes */
/* Remaining = 128 - 92 = 36 bytes */
struct ipsec_sa_params {
		struct ipsec_sa_params_part1 sap1;
		struct ipsec_sa_params_part2 sap2;
};

/* SEC Flow Context Descriptor */ 
struct sec_flow_context {
	/* TODO: add actual fields.
	 * Note: little endian */
	uint32_t flow_context_word[16];
};

/* SEC Shared Descriptor Place Holder*/ 
struct sec_shared_descriptor {
	/* TODO: add actual fields.
	 * Note: little endian */
	uint8_t sd[256];
};

/* DPOVRD for Tunnel Encap mode */
struct dpovrd_tunnel_encap {
	uint8_t reserved; /* 7-0 Reserved */
	uint8_t aoipho; /* 13-8 AOIPHO */
					/* 14 Reserved */
					/* 15 OIMIF */
	uint16_t outer_material_length; /* 27-16 Outer IP Header Material Length */
									/* 30-28 Reserved */
									/* 31 OVRD */
};

/* DPOVRD for Tunnel Decap mode */
struct dpovrd_tunnel_decap {
	uint32_t word;
	/* 31 OVRD
	 * 30-20 Reserved
	 * 19-12 AOIPHO
	 * 11-0 Outer IP Header Material Length */
};

struct dpovrd_general {
	union {
		struct dpovrd_tunnel_decap tunnel_decap;
	};
};

/* SEC Flow Context Descriptor */
struct ipsec_flow_context {
	/* word 0 */
	uint16_t word0_sdid; /* 11-0  SDID */
	uint16_t word0_res; /* 31-12 reserved */
	
	/* word 1 */
	uint8_t word1_sdl; 	/* 5-0 SDL 	*/
						/* 7-6 reserved */
	
	uint8_t word1_bits_15_8; 	/* 11-8 CRID */
								/* 14-12 reserved */
								/* 15 CRJD */
	
	uint8_t word1_bits23_16; /* 16	EWS */
								/* 17 DAC */
								/* 18,19,20 ? */
								/* 23-21 reserved */

	uint8_t word1_bits31_24; /* 24 RSC */
							/* 25 RBMT */
	 	 	 	 	 	 	 /* 31-26 reserved */
	
	/* word 2  RFLC[31-0] */
	uint32_t word2_rflc_31_0;

	/* word 3  RFLC[63-32] */
	uint32_t word3_rflc_63_32;

	/* word 4 */
	uint16_t word4_iicid; /* 15-0  IICID */
	uint16_t word4_oicid; /* 31-16 OICID */
	
	/* word 5 */
	uint8_t word5_7_0; /* 23-0 OFQID */
	uint8_t word5_15_8;
	uint8_t word5_23_16;
	
	uint8_t word5_31_24; 
						/* 24 OSC */
						/* 25 OBMT */
						/* 29-26 reserved 	*/
						/* 31-30 ICR */
	
	//uint32_t word5_31_0; 
	
	/* word 6 */
	uint32_t word6_oflc_31_0;

	/* word 7 */
	uint32_t word7_oflc_63_32;
	 
	/* words 8-15 are a copy of the standard storage profile */
	uint64_t storage_profile[4];
};


/** @} */ /* end of FSL_IPSEC_STRUCTS */


/**************************************************************************//**
@Group		FSL_IPSEC_Functions IPsec Functions

@Description	Freescale AIOP IPsec Functions

@{
*//***************************************************************************/

/**************************************************************************//**
@Function		ipsec_generate_flc 

@Description	Generate SEC Flow Context Descriptor
*//***************************************************************************/
void ipsec_generate_flc(
		uint64_t flc_address, /* Flow Context Address in external memory */
		uint16_t spid, /* Storage Profile ID of the SEC output frame */
		uint32_t sd_size /* Shared descriptor Length */
);

/**************************************************************************//**
@Function		ipsec_generate_sd 

@Description	Generate SEC Shared Descriptor for Encapsulation
*//***************************************************************************/
void ipsec_generate_encap_sd(
		uint64_t sd_addr, /* Flow Context Address in external memory */
		struct ipsec_descriptor_params *params,
		uint32_t *sd_size /* Shared descriptor Length */
);

/**************************************************************************//**
@Function		ipsec_generate_sd 

@Description	Generate SEC Shared Descriptor for Decapsulation
*//***************************************************************************/
void ipsec_generate_decap_sd(
		uint64_t sd_addr, /* Flow Context Address in external memory */
		struct ipsec_descriptor_params *params,
		uint32_t *sd_size /* Shared descriptor Length */
);

/**************************************************************************//**
@Function		ipsec_generate_sa_params 

@Description	Generate and store the functional module internal parameter
*//***************************************************************************/
void ipsec_generate_sa_params(
		struct ipsec_descriptor_params *params,
		ipsec_handle_t ipsec_handle, /* Parameters area (start of buffer) */
		ipsec_instance_handle_t instance_handle);

/**************************************************************************//**
*	ipsec_get_buffer
*	
*	@Description	Allocates a buffer for the IPsec parameters	according 
*			to the instance parameters and increments the instance counters 
*	
*//****************************************************************************/
int ipsec_get_buffer(ipsec_instance_handle_t instance_handle,
		ipsec_handle_t *ipsec_handle
	);

/**************************************************************************//**
*	ipsec_release_buffer
*	@Description	release a buffer and decrements the instance counters 
*		
*//****************************************************************************/
int ipsec_release_buffer(ipsec_instance_handle_t instance_handle,
		ipsec_handle_t ipsec_handle
	);

/**************************************************************************//**
******************************************************************************/

/** @} */ /* end of FSL_IPSEC_Functions */

/** @} */ /* end of FSL_IPSEC */
/** @} */ /* end of NETF */


#endif /* __AIOP_IPSEC_H */
