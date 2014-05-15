/**************************************************************************//**
@File		fsl_ipsec.h

@Description	This file contains the AIOP IPSec Functional Module API.
*//***************************************************************************/

#ifndef __FSL_IPSEC_H
#define __FSL_IPSEC_H

#include "common/types.h"

/**************************************************************************//**
 @Group		NETF NETF (Network Libraries)

 @Description	AIOP Accelerator APIs

 @{
*//***************************************************************************/
/**************************************************************************//**
@Group	FSL_IPSEC IPSEC

@Description	Freescale AIOP IPsec Functional Module API

@{
*//***************************************************************************/

/**************************************************************************//**
 @Group		IPSEC_ENUM IPsec Enumerations

 @Description	IPsec Enumerations

 @{
*//***************************************************************************/

/**************************************************************************//**
 @enum ipsec_direction

 @Description   IPSEC Inbound/Outbound (Decap/Encap) Direction

*************************************************************************/
enum ipsec_direction {
	IPSEC_DIRECTION_INBOUND = 1, 	/*!< Inbound Direction */
	IPSEC_DIRECTION_OUTBOUND = 2 	/*!< Outbound Direction */
};

/**************************************************************************//**
 @enum ipsec_status_codes

 @Description	AIOP IPsec Functional Module return codes.

 @{
*//***************************************************************************/
enum ipsec_status_codes {
	/** Success. */
	IPSEC_SUCCESS = 0,
	/** Reached Soft Lifetime Kilobyte Limit */
	IPSEC_KILOBYTE_LIMIT_SOFT = 0x2,
	/** Reached Hard Lifetime Kilobyte Limit */
	IPSEC_KILOBYTE_LIMIT_HARD = 0x4,
	/** Reached Soft Lifetime Packet Limit */
	IPSEC_PACKET_LIMIT_SOFT = 0x8,
	/** Reached Hard Lifetime Packet Limit */
	IPSEC_PACKET_LIMIT_HARD = 0x10,
	/** Reached Soft Lifetime Seconds Limit */
	IPSEC_SEC_LIMIT_SOFT = 0x20,
	/** Reached Hard Lifetime Seconds Limit */
	IPSEC_SEC_LIMIT_HARD = 0x40,
	
	/** Sequence Number overflow */
	IPSEC_SEC_NUM_OVERFLOW = 0x100,
	/** Anti Replay Check: Late packet */
	IPSEC_AR_LATE_PACKET = 0x200,
	/** Anti Replay Check: Replay packet */
	IPSEC_AR_REPLAY_PACKET = 0x400,
	/** ICV comparison failed */
	IPSEC_ICV_COMPARE_FAIL = 0x800,
};

/* @} end of enum ipsec_status_codes */

/* @} end of IPSEC_ENUM */

/**************************************************************************//**
@Group	FSL_IPSEC_MACROS IPsec Macros

@Description	Freescale IPsec Macros

@{
*//***************************************************************************/

/**************************************************************************//**
@Description	IPSec handle Type definition

*//***************************************************************************/
typedef uint64_t ipsec_handle_t;
typedef uint64_t ipsec_instance_handle_t;

/**************************************************************************//**
@Description	Lifetime callback function type definition

*//***************************************************************************/
typedef void (ipsec_lifetime_callback_t) (
		uint64_t callback_arg, /* callback argument (SA address) */
		uint8_t expiry_type /* Hard or Soft */
		);


/**************************************************************************//**
@Description	ipsec general flags

		Use for ipsec_descriptor_params.flags
*//***************************************************************************/

/** IPsec transport mode (default = tunnel mode) */
#define IPSEC_FLG_TRANSPORT_MODE		0x00000001

/** Enable Transport mode ESP pad check (default = no check)
 * Valid for transport mode only.
 * In tunnel mode pad check is always done*/
#define IPSEC_FLG_TRANSPORT_PAD_CHECK	0x00000002

/** NAT UDP Encapsulation enable. (IPv4 only) */
#define IPSEC_ENC_OPTS_NAT_EN		0x00000010
/** NAT UDP checksum enable. (IPv4 only) */
#define IPSEC_ENC_OPTS_NUC_EN		0x00000020

/** Lifetime KiloByte Counter Enable */
#define IPSEC_FLG_LIFETIME_KB_CNTR_EN	0x00000100
/** Lifetime Packet counter Enable */
#define IPSEC_FLG_LIFETIME_PKT_CNTR_EN	0x00000200
/** Lifetime Seconds counter Enable */
#define IPSEC_FLG_LIFETIME_SEC_CNTR_EN	0x00000400

/** Preserve the ASA (Accelerator Specific Annotation) */
#define IPSEC_FLG_PRESERVE_ASA		0x00020000

/**************************************************************************//**
@Description	General IPSec ESP encap/decap options

		Use for ipsec_encap/decap_params.options
*//***************************************************************************/
#define IPSEC_OPTS_ESP_ESN	0x10   /**< extended sequence included */

#define IPSEC_OPTS_ESP_IPVSN	0x02   /**< process an IPv6 header
 	 	 	 	 	 Valid for transport mode only */

/**************************************************************************//**
@Description	IPSec ESP Encapsulation options

		Use for ipsec_encap_params.options
*//***************************************************************************/
/** Perform computations to update header checksum for IPv4 header.
 * Not valid for tunnel mode */
#define IPSEC_ENC_OPTS_UPDATE_CSUM	0x0080

/** Copy TOS field (IPv4) or Traffic-Class field (IPv6) from outer
 * IP header to inner IP header. Not valid for tunnel mode */
#define IPSEC_ENC_OPTS_DIFFSERV		0x0040

/** Generate random initial vector before starting encapsulation
 * If set, the IV comes from an internal random generator */
#define IPSEC_ENC_OPTS_IVSRC		0x0020

/** Add Output IP header to the frame
* Relevant for tunnel mode only */
#define IPSEC_ENC_OPTS_ADD_IPHDR	0x000c /* Add IP header */

/** Copy the DF bit from the inner IP header to the outer IP header. */
#define IPSEC_ENC_OPTS_DFC 		0x0400

/** Decrement TTL field (IPv4) or Hop-Limit field (IPv6) within inner
 * IP header */
#define IPSEC_ENC_OPTS_DTTL 		0x0200

/* Sequence Number Rollover control.
 * This control permits a Sequence Number Rollover
 * If not set, a Sequence Number Rollover causes an error */
#define IPSEC_ENC_OPTS_SNR_EN  	0x0100	


/**************************************************************************//**
@Description	IPSec ESP Decapsulation options

		Use for ipsec_decap_params.options
*//***************************************************************************/

/** Anti-replay window size. Use one of the following options */
#define IPSEC_DEC_OPTS_ARSNONE	0x0000   /**< no anti-replay window */
#define IPSEC_DEC_OPTS_ARS32	0x0040   /**< 32-entry anti-replay window */
#define IPSEC_DEC_OPTS_ARS128	0x0080   /**< 128-entry anti-replay window */
#define IPSEC_DEC_OPTS_ARS64	0x00c0   /**< 64-entry anti-replay window */

/** Perform checksum verification to IPv4 header in Transport mode.
 * Transport mode only. Not valid for tunnel mode */
#define IPSEC_DEC_OPTS_VERIFY_CSUM 	0x0020 /** validate IP header checksum */

/** Enable Tunnel ECN according to RFC 6040
 * Valid for Tunnel mode only. Not valid for transport mode */
#define IPSEC_DEC_OPTS_TECN		0x0020

/** Decrement TTL field (IPv4) or Hop-Limit field (IPv6) within inner
 * IP header */
#define IPSEC_DEC_OPTS_DTTL	0x0200

/** DiffServ Copy
 * Copy the IPv4 TOS or IPv6 Traffic Class byte from the outer IP header
 * to the inner IP header. */
#define IPSEC_DEC_OPTS_DSC	0x0100

/**************************************************************************//**
@Description	IPSec Cipher Algorithms

 Use for the ipsec_descriptor_params.cipherdata.algtype field

*//***************************************************************************/
#define IPSEC_CIPHER_DES_IV64			0x0100
#define IPSEC_CIPHER_DES			0x0200
#define IPSEC_CIPHER_3DES			0x0300
#define IPSEC_CIPHER_NULL			0x0B00
#define IPSEC_CIPHER_AES_CBC			0x0c00
#define IPSEC_CIPHER_AES_CTR			0x0d00
#define IPSEC_CIPHER_AES_XTS			0x1600
#define IPSEC_CIPHER_AES_CCM8			0x0e00
#define IPSEC_CIPHER_AES_CCM12			0x0f00
#define IPSEC_CIPHER_AES_CCM16			0x1000
#define IPSEC_CIPHER_AES_GCM8			0x1200
#define IPSEC_CIPHER_AES_GCM12			0x1300
#define IPSEC_CIPHER_AES_GCM16			0x1400
#define IPSEC_CIPHER_AES_NULL_WITH_GMAC		0x1500

/**************************************************************************//**
@Description	IPSec Authentication Algorithms

 Use for the ipsec_descriptor_params.authdata.algtype field

*//***************************************************************************/
#define IPSEC_AUTH_HMAC_NULL			0x0000
#define IPSEC_AUTH_HMAC_MD5_96		 	0x0001
#define IPSEC_AUTH_HMAC_SHA1_96			0x0002
#define IPSEC_AUTH_AES_XCBC_MAC_96		0x0005
#define IPSEC_AUTH_HMAC_MD5_128			0x0006
#define IPSEC_AUTH_HMAC_SHA1_160		0x0007
#define IPSEC_AUTH_AES_CMAC_96			0x0008
#define IPSEC_AUTH_HMAC_SHA2_256_128		0x000c
#define IPSEC_AUTH_HMAC_SHA2_384_192		0x000d
#define IPSEC_AUTH_HMAC_SHA2_512_256		0x000e

/**************************************************************************//**
@Description	IPSec Key Encryption Flags

 To be set to the alg_info.key_enc_flags field

*//***************************************************************************/

#define IPSEC_KEY_ENC			0x00400000
	/**< ENC: Encrypted - Key is encrypted either with the KEK, or
	 * 	with the TDKEK if this descriptor is trusted */
#define IPSEC_KEY_NWB			0x00200000
	/**< NWB: No Write Back - Do not allow key to be FIFO STOREd */
#define IPSEC_KEY_EKT			0x00100000
	/**< EKT: Enhanced Encryption of Key */
#define IPSEC_KEY_TK			0x00008000
	/**< TK: Encrypted with Trusted Key */

/** @} */ /* end of FSL_IPSEC_MACROS */

/**************************************************************************//**
@Group		FSL_IPSEC_STRUCTS IPsec Structures

@Description	Freescale IPsec Structures

@{
*//***************************************************************************/

/**************************************************************************//**
 * @Description   IV field for IPsec CBC encapsulation
*//***************************************************************************/
struct ipsec_encap_cbc_params {
	uint32_t iv[4];
};

/**************************************************************************//**
 * @Description   Nonce and IV fields for IPsec CTR encapsulation
*//***************************************************************************/
struct ipsec_encap_ctr_params {
	uint32_t ctr_nonce;
	uint32_t iv[2];
};

/**************************************************************************//**
 * @Description   Salt and IV fields for IPsec CCM encapsulation
*//***************************************************************************/
struct ipsec_encap_ccm_params {
	uint32_t salt; /**< lower 24 bits are used */
	uint32_t iv[2];
};

/**************************************************************************//**
 * @Description   Salt and IV fields for IPsec GCM encapsulation
*//***************************************************************************/
struct ipsec_encap_gcm_params {
	uint32_t salt; /**< lower 24 bits are used */
	uint32_t iv[2];
};

/**************************************************************************//**
 * @Description   Container for encapsulation parameters
*//***************************************************************************/
struct ipsec_encap_params {
	uint8_t ip_nh; /**< Next header value used for transport mode */
	uint16_t options; /**< Options */
	uint32_t seq_num_ext_hi; /**< Extended sequence number */
	uint32_t seq_num;	/**< Initial sequence number */
	uint32_t spi; 	/**< Security Parameter Index */
	uint16_t ip_hdr_len; /**< IP header length */
	uint32_t *outer_hdr; /**< optional IP and UDP Header content */
	union {
		struct ipsec_encap_cbc_params cbc;
		struct ipsec_encap_ctr_params ctr;
		struct ipsec_encap_ccm_params ccm;
		struct ipsec_encap_gcm_params gcm;
	};
};

/**************************************************************************//**
 * @Description   Salt and counter fields for IPsec CTR decapsulation
*//***************************************************************************/
struct ipsec_decap_ctr_params {
	uint32_t salt;
	uint32_t ctr_initial;
};

/**************************************************************************//**
 * @Description   Salt, counter and flag fields for IPsec CCM decapsulation
*//***************************************************************************/
struct ipsec_decap_ccm_params {
	uint32_t salt;
};

/**************************************************************************//**
 * @Description   Salt field for IPsec GCM decapsulation
*//***************************************************************************/
struct ipsec_decap_gcm_params {
	uint32_t salt;
};

/**************************************************************************//**
 * @Description   Container for decapsulation parameters
*//***************************************************************************/
struct ipsec_decap_params {
	uint16_t options; /**< Options */
	uint32_t seq_num_ext_hi; /**< Extended sequence number */
	uint32_t seq_num; /**< Sequence number */
	union {
		struct ipsec_decap_ctr_params ctr;
		struct ipsec_decap_ccm_params ccm;
		struct ipsec_decap_gcm_params gcm;
	};
};

/**************************************************************************//**
 * @Description   Container for IPsec algorithm details
*//***************************************************************************/
struct alg_info {
	uint32_t algtype;  /**< Algorithm selector. */
	uint64_t key;      /**< Address where algorithm key resides
	 	 	 No alignment requirements */
	uint32_t keylen;   /**< Length of the provided key, in bytes */
	uint32_t key_enc_flags; /**< Key encryption flags
				ENC, EKT, TK, NWB */
};

/**************************************************************************//**
 @Description   IPsec Descriptor Parameters
*//***************************************************************************/
struct ipsec_descriptor_params {
	
	//enum ipsec_direction direction; 	/**< Descriptor direction */
	int32_t direction; 	/**< Descriptor direction */

	uint32_t flags; /**< Miscellaneous control flags */
	
	union {
		struct ipsec_encap_params encparams;
		struct ipsec_decap_params decparams;
	}; /**< Enc/Dec Parameters */

	struct alg_info cipherdata; /**< cipher algorithm information */
	struct alg_info authdata; /**< authentication algorithm information */
			
	/** Lifetime Limits */
	/** Set to NULL to disable specific limits check */
	uint64_t soft_kilobytes_limit;	/**< Soft Kilobytes limit, in bytes. */
	uint64_t hard_kilobytes_limit; 	/**< Hard Kilobytes limit, in bytes. */
	uint64_t soft_packet_limit; 	/**< Soft Packet count limit. */
	uint64_t hard_packet_limit;	/**< Hard Packet count limit. */
	uint32_t soft_seconds_limit;	/**< Soft Seconds limit. */
	uint32_t hard_seconds_limit; 	/**< Hard Second limit. */

	/** Callback function.
	 * Invoked when the Soft or Hard Seconds timer reaches the limit value
	 * Set to NULL to disable this option */
        ipsec_lifetime_callback_t *lifetime_callback;

        uint64_t  callback_arg; /**< argument for callback function
        			(SA address) */

	uint16_t spid; /**< Storage Profile ID of the SEC output frame */
};

/** @} */ /* end of FSL_IPSEC_STRUCTS */

/**************************************************************************//**
@Group		FSL_IPSEC_Functions IPsec Functions

@Description	Freescale AIOP IPsec Functions

@{
*//***************************************************************************/

/**************************************************************************//**
@Function	ipsec_add_sa_descriptor

@Description	This function performs add SA for encapsulation:
		creating the IPsec flow context and the Shared Descriptor.

		Implicit Input: BPID in the SRAM (internal usage).
		
@Param[in]	params - pointer to descriptor parameters

@Param[out]	ipsec_handle - IPsec handle to the descriptor database
		
@Return		Status

*//****************************************************************************/
int32_t ipsec_add_sa_descriptor(
		struct ipsec_descriptor_params *params,
		ipsec_handle_t *ipsec_handle);


/**************************************************************************//**
@Function	ipsec_del_sa_descriptor

@Description	This function performs buffer deallocation of the IPsec handler.

		Implicit Input: BPID in the SRAM (internal usage).

@Param[in]	ipsec_handle - descriptor handle.

@Return		Status

*//****************************************************************************/
int32_t ipsec_del_sa_descriptor(ipsec_handle_t ipsec_handle);

/**************************************************************************//**
@Function	ipsec_get_lifetime_stats

@Description	This function returns the SA lifetime counters:
		kilobyte, packets and seconds.

@Param[in]	ipsec_handle - IPsec handle.
@Param[out]	kilobytes - number of bytes processed by this SA.
@Param[out]	packets - number of packets processed by this SA.
@Param[out]	sec - number of seconds passed since this SA was created.

@Return		Status

*//****************************************************************************/
int32_t ipsec_get_lifetime_stats(
		ipsec_handle_t ipsec_handle,
		uint64_t *kilobytes,
		uint64_t *packets,
		uint32_t *sec);

/**************************************************************************//**
@Function	ipsec_decr_lifetime_counters

@Description	This function decrements the SA lifetime counters:
		kilobytes and packets.

@Param[in]	ipsec_handle - IPsec handle.
@Param[in]	kilobytes_decr_val - number of bytes to decrement from
		the kilobytes counter of this SA.
@Param[in]	packets_decr_val - number of packets to decrement from
		the packets counter of this SA.

@Return		Status

*//****************************************************************************/
int32_t ipsec_decr_lifetime_counters(
		ipsec_handle_t ipsec_handle,
		uint32_t kilobytes_decr_val,
		uint32_t packets_decr_val
		);

/**************************************************************************//**
@Function	ipsec_get_seq_num

@Description	This function returns the following information:
		- Sequence number.
		- Extended sequence number (if exists).
		- Anti-replay bitmap (scorecard) (if exists).

@Param[in]	ipsec_handle - IPsec handle.
@Param[out]	sequence_number - Sequence number.
@Param[out]	extended_sequence_number - Extended sequence number.
@Param[out]	anti_replay_bitmap - Anti-replay bitmap. 4 words.
		* For 32-entry only the first 32 bit word is valid.
		* For 64-entry only the first two 32 bit words are valid.
		* For 128-entry all four words are valid.

@Return		Status

*//****************************************************************************/
int32_t ipsec_get_seq_num(
		ipsec_handle_t ipsec_handle,
		uint32_t *sequence_number,
		uint32_t *extended_sequence_number,
		uint32_t anti_replay_bitmap[4]);

/**************************************************************************//**
@Function	ipsec_frame_decrypt

@Description	This function performs the decryption and the required IPsec
		protocol changes (according to RFC4303). This function also
		handles UDP encapsulated IPsec packets according to RFC3948.
		Both Tunnel and Transport modes are supported.
		Tunneling of IPv4/IPv6 packets (either with (up to 64 Bytes) or
		without Ethernet L2) within IPv4/IPv6 is supported.
		The function also updates the decrypted frame parser result and
		checks the inner UDP checksum (if available).

@Param[in]	ipsec_handle - IPsec handle.
@Param[out]	dec_status - decryption operation return status,
		including indication of kilobyte/packet lifetime limit crossing

@Return		General status

@Cautions	User should note the following:
		 - In this function the task yields.
		 - This function preserves the Order Scope mode of the task. If
		the Order Scope is of mode concurrent, the Order Scope ID is
		incremented by 1.
		 - It is assumed that IPv6 ESP extension is the last IPv6
		extension in the packet.
		 - This function does not support input frames which are IPv6
		jumbograms.
*//****************************************************************************/
int32_t ipsec_frame_decrypt(
		ipsec_handle_t ipsec_handle,
		uint32_t *dec_status
		);

/**************************************************************************//**
@Function	ipsec_frame_encrypt

@Description	This function performs the encryption and the required IPsec
		protocol changes (according to RFC4303).  This function
		enables UDP encapsulation of the IPsec packets according
		to RFC3948.
		Both Tunnel and Transport modes are supported.
		Tunneling of IPv4/IPv6 packets (either with (up to 64 Bytes) or
		without Ethernet L2) within IPv4/IPv6 is supported.
		The function also updates the encrypted frame parser result.

@Param[in]	ipsec_handle - IPsec handle.
@Param[out]	enc_status - encryption operation return status,
		including indication of kilobyte/packet lifetime limit crossing

@Return		General status

@Cautions	User should note the following:
		 - In this function the task yields.
		 - This function preserves the Order Scope mode of the task. If
		the Order Scope is of mode concurrent, the Order Scope ID is
		incremented by 1.
		 - This function does not support encrypted frames which are
		IPv6 jumbograms.
*//****************************************************************************/
int32_t ipsec_frame_encrypt(
		ipsec_handle_t ipsec_handle,
		uint32_t *enc_status
		);

/** @} */ /* end of FSL_IPSEC_Functions */
/** @} */ /* end of FSL_IPSEC */
/** @} */ /* end of NETF */

#endif /* __FSL_IPSEC_H */
