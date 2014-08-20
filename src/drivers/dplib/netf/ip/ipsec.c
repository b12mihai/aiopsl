/*
 * Copyright 2014 Freescale Semiconductor, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Freescale Semiconductor nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Freescale Semiconductor ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Freescale Semiconductor BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**************************************************************************//**
@File		ipsec.c

@Description	This file contains the AIOP IPSec implementation.
		
*//***************************************************************************/

#include "common/types.h"
#include "kernel/fsl_spinlock.h"
#include "dplib/fsl_cdma.h"
#include "dplib/fsl_parser.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_l2.h"
#include "dplib/fsl_tman.h"
#include "dplib/fsl_ste.h"
#include "dplib/fsl_osm.h"
#include "header_modification.h"

#include "cdma.h"
#include "osm.h"
#include "system.h"
#include "fsl_platform.h"
#include "fdma.h"

#ifdef AIOP_VERIF
#include "slab_stub.h"
#else
#include "slab.h"
#endif /* AIOP_VERIF */

#pragma push
	/* make all following functions go into .itext_vle */
#pragma section code_type ".itext_vle"

#include "dplib/fsl_ipsec.h"
#include "ipsec.h"

#include "rta.h"
#include "desc/ipsec.h"

/* SEC Era version for RTA */
enum rta_sec_era rta_sec_era = RTA_SEC_ERA_8;

/* Global parameters */
__SHRAM struct ipsec_global_instance_params ipsec_global_instance_params;

#ifdef AIOP_VERIF
__SHRAM uint64_t ipsec_debug_buf_addr; /* Global in Shared RAM */
__SHRAM uint32_t ipsec_debug_buf_size; /* Global in Shared RAM */
__SHRAM uint32_t ipsec_debug_buf_offset; /* Global in Shared RAM */
#endif

/**************************************************************************//**
*	ipsec_create_instance
*//****************************************************************************/
int ipsec_create_instance (
		uint32_t committed_sa_num,
		uint32_t max_sa_num,
		uint32_t instance_flags,
		uint8_t tmi_id,
		ipsec_instance_handle_t *instance_handle)
{
	int32_t return_val;
	
	// committed_sa_num for desc BPID size 512, alignment 64 B 
	// committed_sa_num for keys BPID
	// committed_sa_num for IPv6 outer header (TBD)
	// max num of tasks for ASA 
	
	int num_filled_buffs;
	
	struct ipsec_instance_params instance; 

	instance.sa_count = 0;
	instance.committed_sa_num = committed_sa_num;
	instance.max_sa_num = max_sa_num;
	instance.instance_flags = instance_flags;
	instance.tmi_id = tmi_id;

	/* Descriptor and Instance Buffers */
	return_val = slab_find_and_reserve_bpid(
			(committed_sa_num + 1), /* uint32_t num_buffs */
			IPSEC_SA_DESC_BUF_SIZE, /* uint16_t buff_size */
			1, /* uint16_t alignment = 1, i.e. no alignment requirements */ 
			IPSEC_MEM_PARTITION_ID, /* TODO: TMP. uint8_t  mem_partition_id */
            &num_filled_buffs, /* int *num_filled_buffs */
            &(instance.desc_bpid)); /* uint16_t *bpid */
	
	if (return_val) {
		// TODO: call future slab release function per BPID
		// for all previously requested buffers
		return -ENOMEM;
	}
	
	/* TODO: ASA buffers should be shared for all instances */
	/* ASA Buffers */
	
	/* Check if instances counter is zero */
	/* If yes allocate ASA buffers */
	lock_spinlock((uint8_t *)&ipsec_global_instance_params.spinlock);
		
	if (ipsec_global_instance_params.instance_count == 0) {
		ipsec_global_instance_params.instance_count++;
		unlock_spinlock((uint8_t *)&ipsec_global_instance_params.spinlock);
		
		return_val = slab_find_and_reserve_bpid(
				IPSEC_MAX_NUM_OF_TASKS, /* uint32_t num_buffs */
				IPSEC_MAX_ASA_SIZE, /* uint16_t buff_size */
				IPSEC_MAX_ASA_BUF_ALIGN, /* uint16_t alignment */
				IPSEC_MEM_PARTITION_ID, /* TODO: TMP. uint8_t  mem_partition_id */
	            &num_filled_buffs, /* int *num_filled_buffs */
	            &(instance.asa_bpid)); /* uint16_t *bpid */
		
		if (return_val) {
			// TODO: call future slab release function per BPID
			// for all previously requested buffers
			return -ENOMEM;
		}

	} else {
		ipsec_global_instance_params.instance_count++;
		unlock_spinlock((uint8_t *)&ipsec_global_instance_params.spinlock);
	}
	
	/* Allocate a buffer for the instance */
	return_val = (int32_t)cdma_acquire_context_memory(
		instance.desc_bpid,
		instance_handle); /* context_memory */ 
	
	if (return_val) {
		// TODO: return with correct error code 
		return IPSEC_ERROR;
	}
		
	/* Write the Instance to external memory */
	cdma_write(
			*instance_handle, /* ext_address */
			&instance, /* ws_src */
			(uint16_t)(sizeof(instance))); /* size */

	return IPSEC_SUCCESS; 
}


/**************************************************************************//**
*	ipsec_delete_instance
*//****************************************************************************/
int ipsec_delete_instance(ipsec_instance_handle_t instance_handle)
{
	int32_t return_val;
	uint32_t sa_count;

	cdma_read(
			&sa_count, /* void *ws_dst */
			instance_handle, /* uint64_t ext_address */
			sizeof(sa_count) /* uint16_t size */
			);
	
	/* Check if all SAs were deleted */
	if (sa_count == 0) {
		
		/* Release the instance buffer */ 
		return_val = cdma_refcount_decrement_and_release(instance_handle);
		/* TODO: check for CDMA errors. Mind reference count zero status */
		
		/* TODO: return "committed + 1" buffers back to the slab */
		
		/* Check if instances counter is zero */
		/* If yes return ASA buffers to the slab */
		lock_spinlock((uint8_t *)&ipsec_global_instance_params.spinlock);
		
		/* Error if instance counter is already zero */
		if (ipsec_global_instance_params.instance_count == 0) {
			/* EPERM = 1, Operation not permitted */
			return -EPERM; /* TODO: what is the correct error code? */
		}
				
		ipsec_global_instance_params.instance_count--;
		
		/* Check if this is the last instance */
		if (ipsec_global_instance_params.instance_count == 0) {
			unlock_spinlock((uint8_t *)&ipsec_global_instance_params.spinlock);
			
			/* TODO: return IPSEC_MAX_NUM_OF_TASKS buffers back to the slab */
		
		} else {
			unlock_spinlock((uint8_t *)&ipsec_global_instance_params.spinlock);
		}
		
		return IPSEC_SUCCESS;
	} else {
		/* TODO: handle a case of instance delete before SAs full delete */
		
		/* EPERM = 1, Operation not permitted */
		return -EPERM; /* TODO: what is the correct error code? */
	}
}

/**************************************************************************//**
*	ipsec_get_buffer
*//****************************************************************************/
int ipsec_get_buffer(ipsec_instance_handle_t instance_handle,
		ipsec_handle_t *ipsec_handle)
{
	int return_val;
	struct ipsec_instance_params instance; 
	int num_filled_buffs;

	cdma_read_with_mutex(
			instance_handle, /* uint64_t ext_address */
			CDMA_PREDMA_MUTEX_WRITE_LOCK, /* uint32_t flags */
			&instance, /* void *ws_dst */
			sizeof(instance) /* uint16_t size */	
	);

	if (instance.sa_count < instance.committed_sa_num) {
		instance.sa_count++;
		/* Write and release lock */
		cdma_write_with_mutex(
				instance_handle, /* uint64_t ext_address */
				CDMA_POSTDMA_MUTEX_RM_BIT, /* uint32_t flags */
				&instance.sa_count, /* void *ws_dst */
				sizeof(instance.sa_count) /* uint16_t size */	
		);
		
		return_val = (int)cdma_acquire_context_memory(
				instance.desc_bpid,
				ipsec_handle); /* context_memory */

		/* Check if CDMA allocation failed */
		if (return_val) goto get_buffer_alloc_err;
		
	} else if (instance.sa_count < instance.max_sa_num) {
		instance.sa_count++;
		/* Write and release lock */
		cdma_write_with_mutex(
				instance_handle, /* uint64_t ext_address */
				CDMA_POSTDMA_MUTEX_RM_BIT, /* uint32_t flags */
				&instance.sa_count, /* void *ws_dst */
				sizeof(instance.sa_count) /* uint16_t size */	
		);
		/* Descriptor and Instance Buffers */
		return_val = slab_find_and_reserve_bpid(
				1, /* uint32_t num_buffs */
				IPSEC_SA_DESC_BUF_SIZE, /* uint16_t buff_size */
				IPSEC_SA_DESC_BUF_ALIGN, /* uint16_t alignment */
				IPSEC_MEM_PARTITION_ID, /* TODO: TMP. uint8_t  mem_partition_id */
	            &num_filled_buffs, /* int *num_filled_buffs */
	            &(instance.desc_bpid)); /* uint16_t *bpid */

		/* Check if Slab has no buffers */
		if (return_val) goto get_buffer_alloc_err;

		return_val = (int)cdma_acquire_context_memory(
				instance.desc_bpid,
				ipsec_handle); /* context_memory */
		
		/* Check if CDMA allocation failed */
		if (return_val) goto get_buffer_alloc_err;

	} else {
		/* Release lock */
		cdma_mutex_lock_release(instance_handle);
		return -ENOMEM;
	}
	
	return IPSEC_SUCCESS; 

get_buffer_alloc_err:
	cdma_read_with_mutex(
			instance_handle, /* uint64_t ext_address */
			CDMA_PREDMA_MUTEX_WRITE_LOCK, /* uint32_t flags */
			&instance.sa_count, /* void *ws_dst */
			sizeof(instance.sa_count) /* uint16_t size */	
	);
	
	instance.sa_count--;
	
	cdma_write_with_mutex(
			instance_handle, /* uint64_t ext_address */
			CDMA_POSTDMA_MUTEX_RM_BIT, /* uint32_t flags */
			&instance.sa_count, /* void *ws_dst */
			sizeof(instance.sa_count) /* uint16_t size */	
	);
	
	return -ENOMEM;
} /* End of ipsec_get_buffer */

/**************************************************************************//**
*	ipsec_release_buffer
*//****************************************************************************/
int ipsec_release_buffer(ipsec_instance_handle_t instance_handle,
		ipsec_handle_t ipsec_handle)
{
	int32_t return_val;
	struct ipsec_instance_params instance; 

	cdma_read_with_mutex(
			instance_handle, /* uint64_t ext_address */
			CDMA_PREDMA_MUTEX_WRITE_LOCK, /* uint32_t flags */
			&instance, /* void *ws_dst */
			sizeof(instance) /* uint16_t size */	
	);

	if (instance.sa_count > 0) {
		/* Release the buffer */ 
		return_val = cdma_refcount_decrement_and_release(ipsec_handle); 
		/* TODO: check for CDMA errors. Mind reference count zero status */
				
		/* If buffer taken from 'max' quanta, need to return to slab */
		if (instance.sa_count > instance.committed_sa_num) {
		
			/* TODO: return one buffer back to the slab */
		}
		
		instance.sa_count--;
		
		/* Write (just the counter ) and release lock */
		cdma_write_with_mutex(
				instance_handle, /* uint64_t ext_address */
				CDMA_POSTDMA_MUTEX_RM_BIT, /* uint32_t flags */
				&instance.sa_count, /* void *ws_dst */
				sizeof(instance.sa_count) /* uint16_t size */	
		);
		return return_val;
	} else {
		/* Release lock */
		cdma_mutex_lock_release(instance_handle);
		/* EPERM = 1, Operation not permitted */
		return -EPERM; /* TODO: what is the correct error code? */
	}
} /* End of ipsec_release_buffer */	
		

/**************************************************************************//**
@Function		ipsec_generate_encap_sd 

@Description	Generate SEC Shared Descriptor for Encapsulation
*//***************************************************************************/
int ipsec_generate_encap_sd(
		uint64_t sd_addr, /* Flow Context Address in external memory */
		struct ipsec_descriptor_params *params,
		int *sd_size) /* Shared descriptor Length */
{
	
	uint8_t cipher_type = 0;
	uint8_t pdb_options = 0;
	
	uint32_t ws_shared_desc[64]; /* Temporary Workspace Shared Descriptor */
	uint32_t inl_mask = 0;
	unsigned data_len[3];
	int err;

	struct ipsec_encap_pdb pdb;

	struct alginfo rta_auth_alginfo;
	struct alginfo rta_cipher_alginfo;

	/* Build PDB fields for the RTA */
	
	/* Check which method is it according to the key */
	switch (params->cipherdata.algtype) {
		case IPSEC_CIPHER_AES_CBC:
		case IPSEC_CIPHER_DES_IV64:
		case IPSEC_CIPHER_DES:
		case IPSEC_CIPHER_3DES:
		case IPSEC_CIPHER_AES_XTS: // TODO: check if this is correct
		case IPSEC_CIPHER_NULL: /* No usage of IV for null encryption */
			cipher_type = CIPHER_TYPE_CBC;
			break;
		case IPSEC_CIPHER_AES_CTR:
			cipher_type = CIPHER_TYPE_CTR;
			break;
		case IPSEC_CIPHER_AES_CCM8:
		case IPSEC_CIPHER_AES_CCM12:
		case IPSEC_CIPHER_AES_CCM16:
			cipher_type = CIPHER_TYPE_CCM;
			break;
		case IPSEC_CIPHER_AES_GCM8:
		case IPSEC_CIPHER_AES_GCM12:
		case IPSEC_CIPHER_AES_GCM16:
		case IPSEC_CIPHER_AES_NULL_WITH_GMAC:
			cipher_type = CIPHER_TYPE_GCM;
			break;
		default:
			cipher_type = CIPHER_TYPE_CBC; // TODO: check if this is correct
	}
	
	switch (cipher_type) {
		case CIPHER_TYPE_CBC:
			/* uint32_t iv[4] */
			pdb.cbc.iv[0] = params->encparams.cbc.iv[0];
			pdb.cbc.iv[1] = params->encparams.cbc.iv[1];
			pdb.cbc.iv[2] = params->encparams.cbc.iv[2];
			pdb.cbc.iv[3] = params->encparams.cbc.iv[3];
			break;
		case CIPHER_TYPE_CTR:
			/*	uint32_t ctr_nonce; */
			/*	uint32_t ctr_initial; */
			/*	uint32_t iv[2]; */
			pdb.ctr.ctr_nonce = params->encparams.ctr.ctr_nonce;
			pdb.ctr.ctr_initial = 1;
			pdb.ctr.iv[0] = params->encparams.ctr.iv[0];
			pdb.ctr.iv[1] = params->encparams.ctr.iv[1];
			break;
		case CIPHER_TYPE_CCM:
			/*	uint32_t salt; lower 24 bits */
			/*	uint8_t b0_flags; */
			/*	uint8_t ctr_flags; */
			/*	uint16_t ctr_initial; */
			/*	uint32_t iv[2]; */
			pdb.ccm.salt = params->encparams.ccm.salt;
			pdb.ccm.b0_flags = 0;
			pdb.ccm.ctr_flags = 0;
			pdb.ccm.ctr_initial = 0;
			pdb.ccm.iv[0] = params->encparams.ccm.iv[0];
			pdb.ccm.iv[1] = params->encparams.ccm.iv[1];
			break;
		case CIPHER_TYPE_GCM:
			/*	uint32_t salt; lower 24 bits */
			/*	uint32_t rsvd1; */
			/*	uint32_t iv[2]; */
			pdb.gcm.salt = params->encparams.gcm.salt;
			pdb.gcm.rsvd1 = 0;
			pdb.gcm.iv[0] = params->encparams.gcm.iv[0];
			pdb.gcm.iv[1] = params->encparams.gcm.iv[1];
			break;
		default:
			pdb.cbc.iv[0] = 0;
			pdb.cbc.iv[1] = 0;
			pdb.cbc.iv[2] = 0;
			pdb.cbc.iv[3] = 0;
	}
	
	/* Tunnel Mode Parameters */
	if (params->flags & IPSEC_FLG_TUNNEL_MODE) {
		/* NAT and NUC Options for tunnel mode encapsulation */
		/* Bit 1 : NAT Enable RFC 3948 UDP-encapsulated-ESP */
		/* Bit 0 : NUC Enable NAT UDP Checksum */
		if (params->flags & IPSEC_ENC_OPTS_NAT_EN)
				pdb_options = IPSEC_ENC_PDB_OPTIONS_NAT; 
		if (params->flags & IPSEC_ENC_OPTS_NUC_EN)
				pdb_options |= IPSEC_ENC_PDB_OPTIONS_NUC;
		
		/* outer header from PDB */
		pdb_options |= IPSEC_ENC_PDB_OPTIONS_OIHI_PDB;
	} else {
	/* Transport Mode Parameters */

	}
	
	pdb.hmo = 
		(uint8_t)(((params->encparams.options) & IPSEC_ENC_PDB_HMO_MASK)>>8);
	pdb.options = 
		(uint8_t)((((params->encparams.options) & IPSEC_PDB_OPTIONS_MASK)) |
		pdb_options
		);

	/* Transport mode Next Header value share the same stack location with
	 * Tunnel mode reserved bits at the RTA API.
	 * Since NH comes from DPOVERD it can be init to 0 in both cases
	 * 
	 	union {
			uint8_t ip_nh;	- next header for legacy mode 
			uint8_t rsvd;	- reserved for new mode
		};
	 */  
	pdb.rsvd = 0;
				
	/* Transport mode Next Header value share the same stack location with
	 * Tunnel mode reserved bits at the RTA API.
	 * Since NH comes from DPOVERD it can be init to 0 in both cases
	 * 
	 	 union {
			uint8_t ip_nh_offset;	- next header offset for legacy mode
			uint8_t aoipho; - actual outer IP header offset for new mode 
		};
	*/
	pdb.aoipho = 0;

	pdb.seq_num_ext_hi = params->encparams.seq_num_ext_hi;
	pdb.seq_num = params->encparams.seq_num;
	
	pdb.spi = params->encparams.spi;
		
	pdb.rsvd2 = 0;

	pdb.ip_hdr_len = params->encparams.ip_hdr_len;

	rta_auth_alginfo.algtype = params->authdata.algtype;
	rta_auth_alginfo.keylen = params->authdata.keylen;
	rta_auth_alginfo.key = params->authdata.key;
	rta_auth_alginfo.key_enc_flags  = params->authdata.key_enc_flags;

	rta_cipher_alginfo.algtype = params->cipherdata.algtype;
	rta_cipher_alginfo.keylen = params->cipherdata.keylen;
	rta_cipher_alginfo.key = params->cipherdata.key;
	rta_cipher_alginfo.key_enc_flags  = params->cipherdata.key_enc_flags;
	
	
	/* Lengths of items to be inlined in descriptor; order is important.
	 * Note: For now we assume that inl_mask[0] = 1, i.e. that the
	 * Outer IP Header can be inlined. Demo should be modified to
	        * accommodate for the reference case.
	 * Job descriptor maximum length is hard-coded to 5 * CAAM_CMD_SZ +
	 * 3 * CAAM_PTR_SZ, and pointer size considered extended.
	*/
	data_len[0] = pdb.ip_hdr_len; /* Outer IP header length */
	data_len[1] = params->authdata.keylen;
	data_len[2] = params->cipherdata.keylen;
	
	err = rta_inline_query(IPSEC_NEW_ENC_BASE_DESC_LEN, 5 * 4 + 3 * 8,
				       data_len, &inl_mask, 3);
	if (err < 0)
		return err;
	
	/* ^^^^^^^^^^^^^^  debug ^^^^^^^^^^^^^^^^^*/
	//if (inl_mask & (1 << 1))
	//	pr_warn("ipsec.c: encryption auth descriptor use RTA_PARAM_IMM_DMA\n");
	//else
	//	pr_warn("ipsec.c: encryption auth descriptor use RTA_PARAM_PTR\n");
	//if (inl_mask & (1 << 2))
	//	pr_warn("ipsec.c: encryption cipher descriptor use RTA_PARAM_IMM_DMA\n");
	//else
	//	pr_warn("ipsec.c: encryption cipher descriptor use RTA_PARAM_PTR\n");
	/* ^^^^^^^^^^^^^^^^^  End debug ^^^^^^^^^^^^^^^^^^^*/
	
	if (inl_mask & (1 << 1))
		rta_auth_alginfo.key_type = (enum rta_data_type)RTA_PARAM_IMM_DMA;

	else
		rta_auth_alginfo.key_type = (enum rta_data_type)RTA_PARAM_PTR;

	if (inl_mask & (1 << 2))
		rta_cipher_alginfo.key_type = (enum rta_data_type)RTA_PARAM_IMM_DMA;
	else
		rta_cipher_alginfo.key_type = (enum rta_data_type)RTA_PARAM_PTR;
	
	/* Call RTA function to build an encap descriptor */
	if (params->flags & IPSEC_FLG_TUNNEL_MODE) {
		/* Tunnel mode, SEC "new thread" */	
		*sd_size = cnstr_shdsc_ipsec_new_encap(
			(uint32_t *)(ws_shared_desc), /* uint32_t *descbuf */
			IPSEC_SEC_POINTER_SIZE, /* unsigned short ps */
			&pdb, /* PDB */
			(uint8_t *)params->encparams.outer_hdr, /* uint8_t *opt_ip_hdr */
			//(struct alginfo *)(&(params->cipherdata)),
			//(struct alginfo *)(&(params->authdata)) 
			(struct alginfo *)(&rta_cipher_alginfo),
			(struct alginfo *)(&rta_auth_alginfo)
		);
	} else {
		/* Transport mode, SEC legacy new thread */
		*sd_size = cnstr_shdsc_ipsec_encap(
			(uint32_t *)(ws_shared_desc), /* uint32_t *descbuf */
			IPSEC_SEC_POINTER_SIZE, /* unsigned short ps */
			&pdb, /* PDB */
			//(struct alginfo *)(&(params->cipherdata)),
			//(struct alginfo *)(&(params->authdata)) 
			(struct alginfo *)(&rta_cipher_alginfo),
			(struct alginfo *)(&rta_auth_alginfo)
		);
	}	
	
	/* ^^^^^^^^^^^^^^  debug ^^^^^^^^^^^^^^^^^*/
	//pr_warn("ipsec.c: encryption shared descriptor:\n");
	//for (inl_mask = 0; inl_mask < (*sd_size); inl_mask ++) {
	//	pr_warn("Desc %d : 0x%x\n", inl_mask, ws_shared_desc[inl_mask]);
	//}
	/* ^^^^^^^^^^^^^^^^^  End debug ^^^^^^^^^^^^^^^^^^^*/
	
	/* Write the descriptor to external memory */
	cdma_write(
			sd_addr, /* ext_address */
			ws_shared_desc, /* ws_src */
			(uint16_t)((*sd_size)<<2)); /* sd_size is in 32-bit words */
	
	return IPSEC_SUCCESS;

} /* End of ipsec_generate_encap_sd */

/**************************************************************************//**
@Function		ipsec_generate_decap_sd 

@Description	Generate SEC Shared Descriptor for Encapsulation
*//***************************************************************************/
int ipsec_generate_decap_sd(
		uint64_t sd_addr, /* Flow Context Address in external memory */
		struct ipsec_descriptor_params *params,
		int *sd_size) /* Shared descriptor Length */
{
	
	uint8_t cipher_type = 0;
	

	uint32_t ws_shared_desc[64]; /* Temporary Workspace Shared Descriptor */
	uint32_t inl_mask = 0;
	unsigned data_len[2];
	int err;

	struct ipsec_decap_pdb pdb;

	struct alginfo rta_auth_alginfo;
	struct alginfo rta_cipher_alginfo;

	/* Build PDB fields for the RTA */
	
	/* Check which method is it according to the key */
	switch (params->cipherdata.algtype) {
		case IPSEC_CIPHER_AES_CBC:
		case IPSEC_CIPHER_DES_IV64:
		case IPSEC_CIPHER_DES:
		case IPSEC_CIPHER_3DES:
		case IPSEC_CIPHER_AES_XTS: // TODO: check if this is correct
		case IPSEC_CIPHER_NULL: /* No usage of IV for null encryption */
			cipher_type = CIPHER_TYPE_CBC;
			break;
		case IPSEC_CIPHER_AES_CTR:
			cipher_type = CIPHER_TYPE_CTR;
			break;
		case IPSEC_CIPHER_AES_CCM8:
		case IPSEC_CIPHER_AES_CCM12:
		case IPSEC_CIPHER_AES_CCM16:
			cipher_type = CIPHER_TYPE_CCM;
			break;
		case IPSEC_CIPHER_AES_GCM8:
		case IPSEC_CIPHER_AES_GCM12:
		case IPSEC_CIPHER_AES_GCM16:
		case IPSEC_CIPHER_AES_NULL_WITH_GMAC:
			cipher_type = CIPHER_TYPE_GCM;
			break;
		default:
			cipher_type = CIPHER_TYPE_CBC; // TODO: check if this is correct
	}
		/*----------------------------------*/
		/* 	ipsec_generate_decap_sd			*/
		/*----------------------------------*/
	
	switch (cipher_type) {
		case CIPHER_TYPE_CBC:
			/* uint32_t rsvd[2]; */
			pdb.cbc.rsvd[0] = 0;
			pdb.cbc.rsvd[1] = 0;
            break;
		case CIPHER_TYPE_CTR:
			/* uint32_t salt; */
			/* uint32_t ctr_initial; */
			//pdb.ctr.salt = params->decparams.ctr.salt;
			pdb.ctr.salt = params->decparams.ctr.ctr_nonce; // TODO: need to fix RTA to "nonce" instead of "salt" 
			pdb.ctr.ctr_initial = 1;
			break;
		case CIPHER_TYPE_CCM:
			/* uint32_t salt; */
			/* uint8_t iv_flags; */
			/* uint8_t ctr_flags; */
			/* uint16_t ctr_initial; */
			pdb.ccm.salt = params->decparams.ccm.salt;
			pdb.ccm.iv_flags = 0;
			pdb.ccm.ctr_flags = 0;
			pdb.ccm.ctr_initial = 0;
			break;
		case CIPHER_TYPE_GCM:
			/* uint32_t salt; */
			/* uint32_t resvd; */
			pdb.gcm.salt = params->decparams.gcm.salt;
			pdb.gcm.resvd = 0;
			break;
		default:
			pdb.cbc.rsvd[0] = 0;
			pdb.cbc.rsvd[1] = 0;
	}
	
			/*----------------------------------*/
			/* 	ipsec_generate_decap_sd			*/
			/*----------------------------------*/
	
	/* uint16_t ip_hdr_len : 
	 * 		HMO (upper nibble)
	 * 		IP header length (lower 3 nibbles) is not relevant for tunnel
	 * 		and will be set by DPOVRD for transport */
	pdb.ip_hdr_len = 
			((params->decparams.options) & IPSEC_DEC_PDB_HMO_MASK);

	pdb.options = 
		(uint8_t)(((params->decparams.options) & IPSEC_PDB_OPTIONS_MASK));
	
	if (params->flags & IPSEC_FLG_TUNNEL_MODE) {
		pdb.options |= IPSEC_DEC_OPTS_ETU;
	} else {
		/* Transport mode */
		/* If ESP pad checking is not required output frame is only the PDU */
		if (!(params->flags & IPSEC_FLG_TRANSPORT_PAD_CHECK)) {
			pdb.options |= (IPSEC_DEC_PDB_OPTIONS_AOFL | 
					IPSEC_DEC_PDB_OPTIONS_OUTFMT);
		}
	}
	/*
	3 	OUT_FMT 	Output Frame format:
		0 - All Input Frame fields copied to Output Frame
		1 - Output Frame is just the decapsulated PDU
	2 	AOFL 	Adjust Output Frame Length
		0 - Don't adjust output frame length 
		  output frame length reflects output frame actually written to memory,
		  including the padding, Pad Length, and Next Header fields.
		1 - Adjust output frame length 
		subtract the length of the padding, the Pad Length, and the Next Header
		byte from the output frame length reported to the frame consumer.
		If outFMT==0, this bit is reserved and must be zero.
	*/
	
	/* Transport mode Next Header value share the same stack location with
	 * Tunnel mode reserved bits at the RTA API.
	 * Since NH comes from DPOVERD it can be init to 0 in both cases
	 * 
	 	 union {
			uint8_t ip_nh_offset;	- next header offset for legacy mode
			uint8_t aoipho; - actual outer IP header offset for new mode 
		};
	*/
	pdb.aoipho = 0; /* Will be set by DPOVRD */

	pdb.seq_num_ext_hi = params->decparams.seq_num_ext_hi;
	pdb.seq_num = params->decparams.seq_num;
	
	/* uint32_t anti_replay[4]; */
	pdb.anti_replay[0] = 0;
	pdb.anti_replay[1] = 0;
	pdb.anti_replay[2] = 0;
	pdb.anti_replay[3] = 0;

	rta_auth_alginfo.algtype = params->authdata.algtype;
	rta_auth_alginfo.keylen = params->authdata.keylen;
	rta_auth_alginfo.key = params->authdata.key;
	rta_auth_alginfo.key_enc_flags  = params->authdata.key_enc_flags;

	rta_cipher_alginfo.algtype = params->cipherdata.algtype;
	rta_cipher_alginfo.keylen = params->cipherdata.keylen;
	rta_cipher_alginfo.key = params->cipherdata.key;
	rta_cipher_alginfo.key_enc_flags  = params->cipherdata.key_enc_flags;
	
	/*
	 * Lengths of items to be inlined in descriptor; order is important.
	 * Job descriptor maximum length is hard-coded to 5 * CAAM_CMD_SZ +
	 * 3 * CAAM_PTR_SZ, and pointer size considered extended.
	*/
	data_len[0] = params->authdata.keylen;
	data_len[1] = params->cipherdata.keylen;
	
	err = rta_inline_query(IPSEC_NEW_DEC_BASE_DESC_LEN, 5 * 4 + 3 * 8,
			       data_len, &inl_mask, 2);
	if (err < 0)
		return err;
	
	if (inl_mask & (1 << 0))
		rta_auth_alginfo.key_type = (enum rta_data_type)RTA_PARAM_IMM_DMA;

	else
		rta_auth_alginfo.key_type = (enum rta_data_type)RTA_PARAM_PTR;
	
	if (inl_mask & (1 << 1))
		rta_cipher_alginfo.key_type = (enum rta_data_type)RTA_PARAM_IMM_DMA;

	else
		rta_cipher_alginfo.key_type = (enum rta_data_type)RTA_PARAM_PTR;
	
	/* Call RTA function to build an encap descriptor */
	if (params->flags & IPSEC_FLG_TUNNEL_MODE) {
		/* Tunnel mode, SEC "new thread" */	
		*sd_size = cnstr_shdsc_ipsec_new_decap(
			(uint32_t *)(ws_shared_desc), /* uint32_t *descbuf */
			IPSEC_SEC_POINTER_SIZE, /* unsigned short ps */
			&pdb, /* struct ipsec_encap_pdb *pdb */
			//(struct alginfo *)(&(params->cipherdata)),
			//(struct alginfo *)(&(params->authdata)) 
			(struct alginfo *)(&rta_cipher_alginfo),
			(struct alginfo *)(&rta_auth_alginfo)
		);
	} else {
		/* Transport mode, SEC legacy new thread */
		*sd_size = cnstr_shdsc_ipsec_decap(
			(uint32_t *)(ws_shared_desc), /* uint32_t *descbuf */
			IPSEC_SEC_POINTER_SIZE, /* unsigned short ps */
			&pdb, /* struct ipsec_encap_pdb *pdb */
			//(struct alginfo *)(&(params->cipherdata)),
			//(struct alginfo *)(&(params->authdata))
			(struct alginfo *)(&rta_cipher_alginfo),
			(struct alginfo *)(&rta_auth_alginfo)
		);
	}	
	
	/* Write the descriptor to external memory */
	cdma_write(
			sd_addr, /* ext_address */
			ws_shared_desc, /* ws_src */
			(uint16_t)((*sd_size)<<2)); /* sd_size is in 32-bit words */

	return IPSEC_SUCCESS;
} /* End of ipsec_generate_decap_sd */

/**************************************************************************//**
@Function		ipsec_generate_flc 

@Description	Generate SEC Flow Context Descriptor
*//***************************************************************************/
void ipsec_generate_flc(
		uint64_t flc_address, /* Flow Context Address in external memory */
		uint16_t spid, /* Storage Profile ID of the SEC output frame */
		int sd_size) /* Shared descriptor Length  in words*/
{
	
	struct ipsec_flow_context flow_context;

	extern struct storage_profile storage_profile;
	int i;
	
	struct storage_profile *sp_addr = &storage_profile;
	uint8_t *sp_byte;
	
	sp_addr += spid;
	sp_byte = (uint8_t *)sp_addr;
	
	/* Word 0 */
	flow_context.word0_sdid = 0; //TODO: how to get this value? 
	flow_context.word0_res = 0; 

	/* Word 1 */
	/* 5-0 SDL = Shared Descriptor length, 7-6 reserved */
	/* SDL is encoded in terms of 32-bit descriptor command words */ 
	flow_context.word1_sdl = (uint8_t)(sd_size & 0x000000FF);
	
	flow_context.word1_bits_15_8 = 0; /* 11-8 CRID, 14-12 reserved, 15 CRJD */
	flow_context.word1_bits23_16 = 0; /* 16	EWS,17 DAC,18-20?, 23-21 reserved */
	flow_context.word1_bits31_24 = 0; /* 24 RSC (not used for AIOP), 
		25 RBMT (not used for AIOP), 31-26 reserved */
	// TODO: check regarding EWS in buffer reuse mode
	
	/* word 2  RFLC[31-0] */
	flow_context.word2_rflc_31_0 = 0; /* Not used for AIOP */

	/* word 3  RFLC[63-32] */
	flow_context.word3_rflc_63_32 = 0; /* Not used for AIOP */

	/* word 4 */ /* Not used, should be NULL */
	flow_context.word4_iicid = 0; /* 15-0  IICID */
	flow_context.word4_oicid = 0; /* 31-16 OICID */
	
	/* word 5 */ 	
	flow_context.word5_7_0 = 0; /* 23-0 OFQID, not used for AIOP */
	flow_context.word5_15_8 = 0;
	flow_context.word5_23_16 = 0;
	/* 31-30 ICR = 2. AIOP is a trusted user - no need for any restrictions. */
	flow_context.word5_31_24 = 0x40;
						/* 24 OSC : not used for AIOP */
						/* 25 OBMT : not used for AIOP */
						/* 29-26 reserved */
						/* 31-30 ICR */
	/* word 6 */
	flow_context.word6_oflc_31_0 = 0; /* Not used for AIOP */
	
	/* word 7 */
	flow_context.word7_oflc_63_32 = 0; /* Not used for AIOP */
	
	/* Storage profile format:
	* 0x00 IP-Specific Storage Profile Information 
	* 0x08 Frame Format and Data Placement Controls 
	* 0x10 Buffer Pool 2, Buffer Pool 1 Attributes and Controls 
	* 0x18 Buffer Pool 4, Buffer Pool 3 Attributes and Controls
	* 
	* Only The data from offset 0x08 and 0x10 is copied to SEC flow context 
	*/
	/* Copy the standard Storage Profile to Flow Context words 8-15 */
	/* No need to for the first 8 bytes, so start from 8 */
	for (i = 8; i < 32; i++) {
		*((uint8_t *)((uint8_t *)flow_context.storage_profile + i - 8)) = 
				*(sp_byte + i); 
	}
	
	/* Write the Flow Context to external memory with CDMA */
	cdma_write(
			flc_address, /* ext_address */
			&flow_context, /* ws_src */
			IPSEC_FLOW_CONTEXT_SIZE); /* uint16_t size */
	
} /* End of ipsec_generate_flc */


/**************************************************************************//**
@Function		ipsec_generate_sa_params 

@Description	Generate and store the functional module internal parameter
*//***************************************************************************/
void ipsec_generate_sa_params(
		struct ipsec_descriptor_params *params, 
		ipsec_handle_t desc_addr, /* Parameters area */
		ipsec_instance_handle_t instance_handle)
{
	
	struct ipsec_sa_params sap;
	
	sap.sap1.instance_handle = instance_handle; 
	
	/* Descriptor Part #1 */
	sap.sap1.flags = params->flags; // TMP 
		/* 	transport mode, UDP encap, pad check, counters enable, 
					outer IP version, etc. 4B */
	
	/* Add inbound/outbound indication to the flags field */
	/* Inbound indication is 0, so no action */
	if (params->direction == IPSEC_DIRECTION_OUTBOUND) {
		sap.sap1.flags |= IPSEC_FLG_DIR_OUTBOUND;
	}
	
	/* Add IPv6/IPv4 indication to the flags field */
	if ((params->decparams.options) & IPSEC_PDB_OPTIONS_MASK & 
			IPSEC_OPTS_ESP_IPVSN) {
		sap.sap1.flags |= IPSEC_FLG_IPV6;
	}
	
	if (params->flags & IPSEC_FLG_TUNNEL_MODE) {
		if ((*(params->encparams.outer_hdr) & IPSEC_IP_VERSION_MASK) == 
				IPSEC_IP_VERSION_IPV6) {
			sap.sap1.flags |= IPSEC_FLG_OUTER_HEADER_IPV6;
		}
	}
	
	sap.sap1.status = 0; /* 	lifetime expiry, semaphores	*/

	/* UDP Encap for transport mode */
	sap.sap1.udp_src_port = 0; /* UDP source for transport mode. TMP */
	sap.sap1.udp_dst_port = 0; /* UDP destination for transport mode. TMP */

	/* Extended sequence number enable */
	sap.sap1.esn = (uint8_t)(((params->encparams.options) & 
					IPSEC_PDB_OPTIONS_MASK & IPSEC_ESN_MASK));

	sap.sap1.anti_replay_size = /* none/32/64/128 */ 
			(uint8_t)(((params->encparams.options) & 
					IPSEC_PDB_OPTIONS_MASK & IPSEC_ARS_MASK));
		
		/* new/reuse (for ASA copy). TMP */
	sap.sap1.sec_buffer_mode = IPSEC_SEC_NEW_BUFFER_MODE; 

	sap.sap1.output_spid = (uint8_t)(params->spid);

	sap.sap1.soft_byte_limit = params->soft_kilobytes_limit; 
	sap.sap1.soft_packet_limit = params->soft_packet_limit; 
	sap.sap1.hard_byte_limit = params->hard_kilobytes_limit; 
	sap.sap1.hard_packet_limit = params->hard_packet_limit; 
		
	sap.sap1.byte_counter = 0; /* Encrypted/decrypted bytes counter */
	sap.sap1.packet_counter = 0; /*	Packets counter */

	/* Set valid flag */
	sap.sap1.valid = 1; /* descriptor valid. */
	
	/* Descriptor Part #2 */
	sap.sap2.sec_callback_func = (uint32_t)params->lifetime_callback;
	sap.sap2.sec_callback_arg = params->callback_arg;
		
	// TODO: init one-shot timers according to:
	// soft_seconds_limit; 
	// hard_seconds_limit; 
	sap.sap2.soft_tmr_handle = NULL; /* Soft seconds timer handle, TMP */
	sap.sap2.hard_tmr_handle = NULL; /* Hard seconds timer handle, TMP */

	/* Get timestamp from TMAN */
	tman_get_timestamp(&(sap.sap1.timestamp));
	
	/* Store to external memory with CDMA */
	cdma_write(
			desc_addr, /* uint64_t ext_address */
			&sap, /* void *ws_src */
			(uint16_t)(sizeof(sap)) /* uint16_t size */
			);
	
} /* End of ipsec_generate_sa_params */

/**************************************************************************//**
*	ipsec_add_sa_descriptor
*//****************************************************************************/

/*                 SA Descriptor Structure
 * ------------------------------------------------------
 * |  ipsec_sa_params                 | 128 bytes       |
 * ------------------------------------------------------
 * | sec_flow_context                 | 64 bytes        |
 * -----------------------------------------------------
 * | sec_shared_descriptor            | Up to 256 bytes |
 * ------------------------------------------------------
 * | Replacement Job Descriptor (TBD) |                 |
 * ------------------------------------------------------
 * 
 * ipsec_sa_params - Parameters used by the IPsec functional module	128 bytes
 * sec_flow_context	- SEC Flow Context. 64 bytes
 * 			Should be 64-byte aligned for optimal performance.	
 * sec_shared_descriptor - Shared descriptor. Up to 256 bytes
 * Replacement Job Descriptor (RJD) for Peer Gateway Adaptation 
 * (Outer IP change)	TBD 
*/

int ipsec_add_sa_descriptor(
		struct ipsec_descriptor_params *params,
		ipsec_instance_handle_t instance_handle,
		ipsec_handle_t *ipsec_handle)
{

	int return_val;
	int sd_size; /* shared descriptor size, set by the RTA */
	ipsec_handle_t desc_addr;
	
	/* Create a shared descriptor */

	return_val = ipsec_get_buffer(instance_handle,
			ipsec_handle);
	
	/* Check for allocation error */
	if (return_val) {
		// TODO: decrement SA counter
		return return_val;
	}
		
	desc_addr = IPSEC_DESC_ADDR(*ipsec_handle);
	
	/* Build a shared descriptor with the RTA library */
	/* Then store it in the memory with CDMA */
	if (params->direction == IPSEC_DIRECTION_INBOUND) {
		return_val = 
			ipsec_generate_decap_sd(IPSEC_SD_ADDR(desc_addr),params, &sd_size);
	} else {
		return_val = 
			ipsec_generate_encap_sd(IPSEC_SD_ADDR(desc_addr),params, &sd_size);
	}
	
	/* Check for IPsec descriptor generation error */
	if (return_val) {
		// TODO: free the buffer, decrement SA counter?, fix error value
		return return_val;
	}
	
	/* Generate the SEC Flow Context descriptor and write to memory with CDMA */
	ipsec_generate_flc(
			IPSEC_FLC_ADDR(desc_addr), 
				/* Flow Context Address in external memory */
			params->spid, /* Storage Profile ID of the SEC output frame */
			sd_size); /* Shared descriptor size in words */
	
	/*	Prepare descriptor parameters:
	 * Kilobytes and packets lifetime limits.
	 * Modes indicators and other flags */
	/* Store the descriptor parameters to memory (CDMA write). */
	ipsec_generate_sa_params(
			params,
			desc_addr, /* Parameters area (start of buffer) */
			instance_handle);
	
	/* Create one-shot TMAN timers for the soft and hard seconds lifetime 
	 * limits, with callback to internal function 
	 * (including the descriptor handle and soft/hard indication arguments). */
	
	/* Success, handle returned. */
	return IPSEC_SUCCESS;
	
} /* End of ipsec_add_sa_descriptor */

/**************************************************************************//**
*	ipsec_del_sa_descriptor
*//****************************************************************************/
int ipsec_del_sa_descriptor(
		ipsec_handle_t ipsec_handle)
{

	int return_val;
	ipsec_instance_handle_t instance_handle;
	ipsec_handle_t desc_addr;

	// TODO Delete the timers; take care of callbacks in the middle of operation.
	
	desc_addr = IPSEC_DESC_ADDR(ipsec_handle);

	/* Flush all the counter updates that are pending in the 
	 * statistics engine request queue. */
	ste_barrier();

	/* Read the instance handle from params area */
	cdma_read(
			&instance_handle, /* void *ws_dst */
			//(ipsec_handle + (offsetof(struct ipsec_sa_params_part1,
			//(desc_addr + (offsetof(struct ipsec_sa_params_part1,
			//		 instance_handle))), /* uint64_t ext_address */
			IPSEC_INSTANCE_HANDLE_ADDR(desc_addr),		 
			sizeof(instance_handle) /* uint16_t size */
			);
	
	/* Release the buffer */ 
	return_val = ipsec_release_buffer(instance_handle, ipsec_handle);
	
	// TODO: 
	// 1. Check that all frames are closed (reference count)
	// 2. Add timer delay for tasks that are in an interim state
	// (called by the application but did npt enter the SL yet)
	// If there were open frames do another ste_barrier();
	
	if (return_val != CDMA_REFCOUNT_DECREMENT_TO_ZERO) { /* error */
		return IPSEC_ERROR; /* Trying to delete before all frames done */
	} else { /* success */
		//atomic_incr32((int32_t *)(&(global_params.sa_count)), 1);
		return IPSEC_SUCCESS; 
	}
	
} /* End of ipsec_del_sa_descriptor */

/**************************************************************************//**
* ipsec_frame_encrypt
*//****************************************************************************/
int ipsec_frame_encrypt(
		ipsec_handle_t ipsec_handle,
		uint32_t *enc_status
		)
{
	int return_val;
	uint8_t eth_header[40]; /* Ethernet header place holder, 40 bytes */ 
	uint8_t eth_length = 0; /* Ethernet header length and indicator */ 
	uint64_t orig_flc;
	uint32_t orig_frc;
	uint8_t *eth_pointer_default;
	uint32_t byte_count;
	uint16_t checksum;
	uint8_t dont_encrypt = 0;
	int i;
	ipsec_handle_t desc_addr;

	struct ipsec_sa_params_part1 sap1; /* Parameters to read from ext buffer */
	struct scope_status_params scope_status;
	struct dpovrd_general dpovrd;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;

	/* Increment the reference counter */
	cdma_refcount_increment(ipsec_handle);
	
	*enc_status = 0; /* Initialize */
	
	/* 	Outbound frame encryption and encapsulation (ipsec_frame_encrypt) 
	 * � Simplified Flow */
	
	desc_addr = IPSEC_DESC_ADDR(ipsec_handle);

	/* 	2.	Read relevant descriptor fields with CDMA. */
	cdma_read(
			&sap1, /* void *ws_dst */
			//ipsec_handle, /* uint64_t ext_address */
			desc_addr, /* uint64_t ext_address */
			sizeof(sap1) /* uint16_t size */
			);

	/*---------------------*/
	/* ipsec_frame_encrypt */
	/*---------------------*/
	
	/* 	3.	Check that hard kilobyte/packet/seconds lifetime limits have 
	 * not expired. If expired, return with error and go to END */
	/* The seconds lifetime status is checked in the params[status] 
	 * and the kilobyte/packet status is checked from the params[counters].
	 * This is done to avoid doing mutex lock for kilobyte/packet status */
	
	/* Seconds Lifetime */
	if (sap1.flags & IPSEC_FLG_LIFETIME_SEC_CNTR_EN) {
		if (sap1.status & IPSEC_STATUS_SOFT_SEC_EXPIRED) {
			*enc_status |= IPSEC_STATUS_SOFT_SEC_EXPIRED;
		}
		if (sap1.status & IPSEC_STATUS_HARD_SEC_EXPIRED) {
			*enc_status |= IPSEC_STATUS_HARD_SEC_EXPIRED;
			dont_encrypt = 1;
		}
	}
	
	/* KB lifetime counters */
	if (sap1.flags & IPSEC_FLG_LIFETIME_KB_CNTR_EN) {
		if (sap1.byte_counter >= sap1.soft_byte_limit) {
			*enc_status |= IPSEC_STATUS_SOFT_KB_EXPIRED;
		}
		if (sap1.byte_counter >= sap1.hard_byte_limit) {
			*enc_status |= IPSEC_STATUS_HARD_KB_EXPIRED;
			dont_encrypt = 1;
		}
	}
	
	/* Packets lifetime counters*/
	if (sap1.flags & IPSEC_FLG_LIFETIME_PKT_CNTR_EN) {

		if (sap1.packet_counter >= sap1.soft_packet_limit) {
			*enc_status |= IPSEC_STATUS_SOFT_PACKET_EXPIRED;
		}
		if (sap1.packet_counter >= sap1.hard_packet_limit) {
			*enc_status |= IPSEC_STATUS_HARD_PACKET_EXPIRED;
			dont_encrypt = 1;
		}
	}
	
	if (dont_encrypt) {
		return_val = IPSEC_ERROR; // TODO: TMP
		goto encrypt_end;
	}
	
		/*---------------------*/
		/* ipsec_frame_encrypt */
		/*---------------------*/
	
	if (sap1.flags & IPSEC_FLG_TUNNEL_MODE) {
		/* Tunnel Mode */
		/* Clear FD[FRC], so DPOVRD takes no action */
		dpovrd.tunnel_encap.word = 0; 
	} else {
		/* For Transport mode set DPOVRD */
		/* 31 OVRD, 30-28 Reserved, 27-24 ECN (Not relevant for transport mode)
		 * 23-16 IP Header Length in bytes, 
		* of the portion of the IP header that is not encrypted.
		* 15-8 NH_OFFSET - location of the next header within the IP header.
		* 7-0 Next Header */
		dpovrd.transport_encap.ovrd = IPSEC_DPOVRD_OVRD_TRANSPORT;
		
		/* Header Length according to IPv6/IPv4 */
		if (sap1.flags & IPSEC_FLG_IPV6) { /* IPv6 header */
			/* Get the NH_OFFSET for the last header to encapsulate*/
			dpovrd.transport_encap.nh_offset = 
				ipsec_get_ipv6_nh_offset(
					(struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT(),
					&(dpovrd.transport_encap.ip_hdr_len));
		} else { /* IPv4 */
			/* IPv4 Header Length in Bytes */
			dpovrd.transport_encap.ip_hdr_len = ((uint8_t)
				((*((uint8_t *)PARSER_GET_OUTER_IP_POINTER_DEFAULT())) & 
											IPV4_HDR_IHL_MASK)) << 2;
			/* If transport/IPv4 for any non-zero value of NH_OFFSET 
			 * (typically set to 01h), the N byte comes from byte 9 of 
			 * the IP header */
			dpovrd.transport_encap.nh_offset = 0x1;
		}
		
		/* Set the Next Header to ESP (the same for IPv4 and IPv6) */
		dpovrd.transport_encap.next_hdr = IPSEC_IP_NEXT_HEADER_ESP;
	}
	
	/*---------------------*/
	/* ipsec_frame_encrypt */
	/*---------------------*/
	
	/* 	4.	Identify if L2 header exist in the frame: */
	/* Check if Ethernet/802.3 MAC header exist and remove it */
	if (PARSER_IS_ETH_MAC_DEFAULT()) { /* Check if Ethernet header exist */
		
		/* For tunnel mode, update the Ethertype field according to the 
		 * outer header (IPv4/Ipv6), since after SEC encryption
		 * the parser results are not valid any more */
		if (sap1.flags & IPSEC_FLG_TUNNEL_MODE) {
			/* Update the Ethertype according to the outher IP header */
			if (sap1.flags & IPSEC_FLG_OUTER_HEADER_IPV6) {
				*((uint16_t *)PARSER_GET_LAST_ETYPE_POINTER_DEFAULT()) =
						IPSEC_ETHERTYPE_IPV6;
			} else {
				*((uint16_t *)PARSER_GET_LAST_ETYPE_POINTER_DEFAULT()) =
						IPSEC_ETHERTYPE_IPV4;
			}
		}

		/* Save Ethernet header. Note: no swap */
		/* up to 6 VLANs x 4 bytes + 14 regular bytes */
		
		/* Ethernet header length and indicator */ 
		eth_length = (uint8_t)(
						(uint8_t *)PARSER_GET_OUTER_IP_OFFSET_DEFAULT() - 
								(uint8_t *)PARSER_GET_ETH_OFFSET_DEFAULT()); 

		eth_pointer_default = (uint8_t *)PARSER_GET_ETH_POINTER_DEFAULT();
	
		for (i = 0 ; i < eth_length; i++) {
			eth_header[i] = *(eth_pointer_default + i);
		}
			
		/* Remove L2 Header */	
		/* Note: The gross running sum of the frame becomes invalid 
		 * after calling this function.
		 */
		l2_header_remove();
	}

			/*---------------------*/
			/* ipsec_frame_encrypt */
			/*---------------------*/
	
	/* 	5.	Save original FD[FLC], FD[FRC] (to stack) */
	orig_flc = LDPAA_FD_GET_FLC(HWC_FD_ADDRESS);
	orig_frc = LDPAA_FD_GET_FRC(HWC_FD_ADDRESS);
	
	/* Update FD[FRC] for DPOBERD */
	//LDPAA_FD_SET_FRC(HWC_FD_ADDRESS, 0);
	LDPAA_FD_SET_FRC(HWC_FD_ADDRESS, *((uint32_t *)(&dpovrd)));

	/* 	6.	Update the FD[FLC] with the flow context buffer address. */
	LDPAA_FD_SET_FLC(HWC_FD_ADDRESS, IPSEC_FLC_ADDR(desc_addr));	
	
	/* 	7.	FDMA store default frame command 
	 * (for closing the frame, updating the other FD fields) */
	return_val = fdma_store_default_frame_data();
	// TODO: check FDMA return status

	/* 	8.	Prepare AAP parameters in the Workspace memory. */
	/* 	8.1.	Use accelerator macros for storing parameters */
	/* 
	* 3 USE_FLC_SP Use Flow Context Storage Profile = 1 
	* The Storage Profile (SP) is embedded as part of the 
	* flow context pointed to by the Flow Context field in the Frame Descriptor 
	* (FD) for this acceleration operation.
	* 
	* 8 OS_EX Ordering Scope Exclusive Phase.
	* 0 Indicates that the accelerator call is not made during the 
	* exclusive phase of an Ordering Scope.
	* 1 Indicates that the accelerator call is made during the 
	* exclusive phase of an Ordering Scope.
	*/
	
	/* Get OSM status (ordering scope mode and levels) */
	osm_get_scope(&scope_status);

	/* If in Concurrent ordering scope, move to Exclusive 
	 * (increment scope ID). */ 
	if (scope_status.scope_mode == IPSEC_OSM_CONCURRENT) {
	    /* Move to exclusive */
	    osm_scope_transition_to_exclusive_with_increment_scope_id();
		/* Set OS_EX so AAP will do relinquish */
		*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = 
				(IPSEC_AAP_USE_FLC_SP | IPSEC_AAP_OS_EX);
	} else {
		/* Call AAP without relinquish */
		*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = IPSEC_AAP_USE_FLC_SP;
	}
	
	/* 	9.	Call the AAP */
	__e_hwacceli(AAP_SEC_ACCEL_ID);
	
	/* 	10.	SEC Doing Encryption */

			/*---------------------*/
			/* ipsec_frame_encrypt */
			/*---------------------*/
	
	/* Check if started in concurrent mode */
	if (scope_status.scope_mode == IPSEC_OSM_CONCURRENT) {
		/* The AAP already did OSM relinquished, so just register that */
		REGISTER_OSM_CONCURRENT;
	}

	/* Update the SPID of the new frame (SEC output) in the HW Context*/
	*((uint8_t *)HWC_SPID_ADDRESS) = sap1.output_spid;
	
	/* Update the default segment length for the new frame  in 
	 * the presentation context */
	PRC_SET_SEGMENT_LENGTH(DEFAULT_SEGMENT_SIZE);
	
	/* 	11.	FDMA present default frame command (open frame) */
	return_val = fdma_present_default_frame();
	// TODO: check for FDMA error
	
	/* 	12.	Read the SEC return status from the FD[FRC]. Use swap macro. */
	//*enc_status = LDPAA_FD_GET_FRC(HWC_FD_ADDRESS);
	// TODO: which errors can happen in encryption?
	switch (LDPAA_FD_GET_FRC(HWC_FD_ADDRESS)) {
		case SEC_NO_ERROR:
			break;
		case SEC_SEQ_NUM_OVERFLOW: /** Sequence Number overflow */
			*enc_status |= IPSEC_SEQ_NUM_OVERFLOW;
			return_val = -1;
			break;
		case SEC_AR_LATE_PACKET:	/** Anti Replay Check: Late packet */
			*enc_status |= IPSEC_AR_LATE_PACKET;
			return_val = -1;
			break;
		case SEC_AR_REPLAY_PACKET:	/** Anti Replay Check: Replay packet */
			*enc_status |= IPSEC_AR_REPLAY_PACKET;
			return_val = -1;
			break;
		case SEC_ICV_COMPARE_FAIL:	/** ICV comparison failed */
			*enc_status |= IPSEC_ICV_COMPARE_FAIL;	
			return_val = -1;
			break;
		default:
			*enc_status |= IPSEC_GEN_ENCR_ERR;	
			return_val = -1;
	}
	
	/* 	13.	If encryption/encapsulation failed go to END (see below) */
	// TODO: check results
		
	/* 	14.	Get new running sum and byte count (encrypted/encapsulated frame) 
	 * from the FD[FLC] */
	/* The least significant 6 bytes of the 8-byte FLC in the enqueued FD 
	 * contain a 2-byte checksum and 4-byte encrypted/decrypted byte count.
	 * FLC[63:0] = { 16�b0, checksum[15:0], byte_count[31:0] } */
	//checksum = LH_SWAP(HWC_FD_ADDRESS + FD_FLC_DS_AS_CS_OFFSET + 2);
	
	/** Load 2 bytes with endian swap.
	 * The address loaded from memory is calculated as: _displ + _base.
	 * _displ - a word aligned constant value between 0-1020.
	 * _base - a variable containing the base address.
	 * If 'base' is a literal 0, the base address is considered as 0. */
	checksum = LH_SWAP(HWC_FD_ADDRESS + FD_FLC_DS_AS_CS_OFFSET + 2, 0);

	
	//byte_count = LW_SWAP(HWC_FD_ADDRESS + FD_FLC_DS_AS_CS_OFFSET + 4);
	byte_count = LW_SWAP(HWC_FD_ADDRESS + FD_FLC_DS_AS_CS_OFFSET + 4, 0);

	/* 	15.	Update the gross running checksum in the Workspace parser results.*/
	// TODO: is it needed for encryption?
	
	/* 	16.	If L2 header existed in the original frame, add it back: */
	if (eth_length) {
		//TODO: debug info
#ifdef AIOP_VERIF
		if (ipsec_debug_buf_addr != NULL) {
			/* Write the debug info to external memory */
			cdma_write(
				(ipsec_debug_buf_addr + ipsec_debug_buf_offset), /* ext_address */
				&eth_header, /* ws_src */
				40); /* size */
			if (ipsec_debug_buf_offset <= (ipsec_debug_buf_size-64)) {
				ipsec_debug_buf_offset += 64;
			}
		}
#endif
		
		/* Note: The Ethertype was already updated before removing the 
		 * L2 header */
		return_val = fdma_insert_default_segment_data(
				0, /* uint16_t to_offset */
				eth_header, /* void	 *from_ws_src */
				eth_length, /* uint16_t insert_size */
				FDMA_REPLACE_SA_REPRESENT_BIT 
					/* uint32_t flags */
				);
		
		/* TODO: Re-run parser ??? */
		//		parse_result_generate_default(0);
		
		/* TODO: Update running sum ??? */
		//		pr->gross_running_sum = 0;
	}
	
	/* In transport mode, optionally add UDP encapsulation */
	if ((!(sap1.flags & IPSEC_FLG_TUNNEL_MODE)) &&
			(sap1.flags & IPSEC_ENC_OPTS_NAT_EN)) {
		// TODO, including checksum updates
	} 

		/*---------------------*/
		/* ipsec_frame_encrypt */
		/*---------------------*/
	
	/* 	Set the gross running to 0 (invalidate) */
	pr->gross_running_sum = 0;
	
	/* 	Run parser and check for errors. */
	return_val = parse_result_generate_default(PARSER_NO_FLAGS);
	// TODO: check results (TBD)
	
	/* 	17.	Restore the original FD[FLC], FD[FRC] (from stack). 
	 * No need for additional FDMA command. */
	LDPAA_FD_SET_FLC(HWC_FD_ADDRESS, orig_flc);	
	LDPAA_FD_SET_FRC(HWC_FD_ADDRESS, orig_frc)	
	
	/* 	18.	Handle lifetime counters */
		/* 	18.1.	Read lifetime counters (CDMA) */
		/* 	18.2.	Add byte-count from SEC and one packet count. */
		/* 	18.4.	Update the kilobytes and/or packets lifetime counters 
		 * (STE increment + accumulate). */
	
	if (sap1.flags & 
			(IPSEC_FLG_LIFETIME_KB_CNTR_EN | IPSEC_FLG_LIFETIME_PKT_CNTR_EN)) {
		ste_inc_and_acc_counters(
			//IPSEC_PACKET_COUNTER_ADDR,/* uint64_t counter_addr */
			IPSEC_PACKET_COUNTER_ADDR(desc_addr), /* uint64_t counter_addr */
			byte_count,	/* uint32_t acc_value */
			/* uint32_t flags */
			(STE_MODE_COMPOUND_64_BIT_CNTR_SIZE |  
			STE_MODE_COMPOUND_64_BIT_ACC_SIZE |
			STE_MODE_COMPOUND_CNTR_SATURATE |
			STE_MODE_COMPOUND_ACC_SATURATE));
	} else if (sap1.flags & IPSEC_FLG_LIFETIME_KB_CNTR_EN) {
		ste_inc_counter(
				//IPSEC_KB_COUNTER_ADDR,
				IPSEC_KB_COUNTER_ADDR(desc_addr),
				byte_count,
				(STE_MODE_SATURATE | STE_MODE_64_BIT_CNTR_SIZE));
	} else if (sap1.flags & IPSEC_FLG_LIFETIME_PKT_CNTR_EN) {
		ste_inc_counter(
				//IPSEC_PACKET_COUNTER_ADDR,
				IPSEC_PACKET_COUNTER_ADDR(desc_addr),
				1,
				(STE_MODE_SATURATE | STE_MODE_64_BIT_CNTR_SIZE));
	}
	
	return_val = IPSEC_SUCCESS;	

encrypt_end:
	
	/* 	19.	END */
		
	/* 	19.1. Update the encryption status (enc_status) and return status. */

	/* Decrement the reference counter */
	return_val = cdma_refcount_decrement(ipsec_handle);
	// TODO: check CDMA return status
	
	/* 	19.3.	Return */
	return return_val;
} /* End of ipsec_frame_encrypt */

/**************************************************************************//**
* ipsec_frame_decrypt
*//****************************************************************************/
int ipsec_frame_decrypt(
		ipsec_handle_t ipsec_handle,
		uint32_t *dec_status
		)
{
	int return_val;
	uint8_t eth_header[40]; /* Ethernet header place holder, 40 bytes */ 
	uint8_t eth_length = 0; /* Ethernet header length and indicator */ 
	uint64_t orig_flc; /* Original FLC */
	//uint64_t return_flc; /* SEC returned FLC */
	uint32_t orig_frc;
	uint16_t outer_material_length;
	//uint16_t running_sum;
	uint8_t *eth_pointer_default;
	uint32_t byte_count;
	uint16_t checksum;
	uint8_t dont_decrypt = 0;
	int i;
	ipsec_handle_t desc_addr;

	struct ipsec_sa_params_part1 sap1; /* Parameters to read from ext buffer */
	struct scope_status_params scope_status;

	struct dpovrd_general dpovrd;
	struct parse_result *pr = (struct parse_result *)HWC_PARSE_RES_ADDRESS;
	
	/* Increment the reference counter */
	cdma_refcount_increment(ipsec_handle);
	
	*dec_status = 0; /* Initialize */
	
	/* 	Inbound frame decryption and decapsulation */
	
	desc_addr = IPSEC_DESC_ADDR(ipsec_handle);

	/* 	2.	Read relevant descriptor fields with CDMA. */
	cdma_read(
			&sap1, /* void *ws_dst */
			//ipsec_handle, /* uint64_t ext_address */
			desc_addr, /* uint64_t ext_address */
			sizeof(sap1) /* uint16_t size */
			);

	/*---------------------*/
	/* ipsec_frame_decrypt */
	/*---------------------*/
	
	/* 	3.	Check that hard kilobyte/packet/seconds lifetime limits 
	 * have expired. If expired, return with error. go to END */
	// TODO
	/* The seconds lifetime status is checked in the params[status] 
	 * and the kilobyte/packet status is checked from the params[counters].
	 * This is done to avoid doing mutex lock for kilobyte/packet status */
	/* Seconds Lifetime */
	if (sap1.flags & IPSEC_FLG_LIFETIME_SEC_CNTR_EN) {
		if (sap1.status & IPSEC_STATUS_SOFT_SEC_EXPIRED) {
			*dec_status |= IPSEC_STATUS_SOFT_SEC_EXPIRED;
		}
		if (sap1.status & IPSEC_STATUS_HARD_SEC_EXPIRED) {
			*dec_status |= IPSEC_STATUS_HARD_SEC_EXPIRED;
			dont_decrypt = 1;
		}
	}
	
	/* KB lifetime counters */
	if (sap1.flags & IPSEC_FLG_LIFETIME_KB_CNTR_EN) {
		if (sap1.byte_counter >= sap1.soft_byte_limit) {
			*dec_status |= IPSEC_STATUS_SOFT_KB_EXPIRED;
		}
		if (sap1.byte_counter >= sap1.hard_byte_limit) {
			*dec_status |= IPSEC_STATUS_HARD_KB_EXPIRED;
			dont_decrypt = 1;
		}
	}
	
	/* Packets lifetime counters*/
	if (sap1.flags & IPSEC_FLG_LIFETIME_PKT_CNTR_EN) {

		if (sap1.packet_counter >= sap1.soft_packet_limit) {
			*dec_status |= IPSEC_STATUS_SOFT_PACKET_EXPIRED;
		}
		if (sap1.packet_counter >= sap1.hard_packet_limit) {
			*dec_status |= IPSEC_STATUS_HARD_PACKET_EXPIRED;
			dont_decrypt = 1;
		}
	}
	
	if (dont_decrypt) {
		return_val = IPSEC_ERROR; // TODO: TMP
		goto decrypt_end;
	}
	
			/*---------------------*/
			/* ipsec_frame_decrypt */
			/*---------------------*/

	/* 	4.	Identify if L2 header exist in the frame, 
	 * and if yes get the L2 header length. */
	if (PARSER_IS_ETH_MAC_DEFAULT()) { /* Check if Ethernet header exist */
		/* Note: For tunnel mode decryption there is no need to update 
		 * the Ethertype field, since SEC HW is doing it */
		
		/* Ethernet header length and indicator */ 
		eth_length = (uint8_t)
				((uint8_t *)PARSER_GET_OUTER_IP_OFFSET_DEFAULT() - 
								(uint8_t *)PARSER_GET_ETH_OFFSET_DEFAULT()); 
	}
	
	/* Prepare DPOVRD Parameters */
	/* For transport mode: IP header length, Next header offset */
	/* For tunnel mode: 
	 * IP header length, Actual Outer IP Header Offset (AOIPHO), including L2 */
	if (sap1.flags & IPSEC_FLG_TUNNEL_MODE) {
		/* Prepare DPOVRD */
		/* 31 OVRD
		 * 30-20 Reserved
		 * 19-12 AOIPHO: Actual Outer IP Header Offset. 
		 * 		AOIPHO indicates the number of bytes of material in the 
		 * 		Input Frame prior to the actual Outer IP Header.
		 * 11-0 Outer IP Header Material Length 
		 * 		Length for the Outer IP Header Material (in bytes). 
		 * 		This field indicates the total length of the material that 
		 * 		includes the Outer IP Header, up to but not including the 
		 * 		ESP Header.
		*/
		outer_material_length = (uint16_t)
			((uint32_t)((uint8_t *)PARSER_GET_L5_OFFSET_DEFAULT()) - 
				(uint32_t)((uint8_t *)PARSER_GET_OUTER_IP_OFFSET_DEFAULT()) +
				eth_length); 
				
		dpovrd.tunnel_decap.word = 
				IPSEC_DPOVRD_OVRD |
				(eth_length<<12) | /* AOIPHO */
				outer_material_length; /* Outer IP Header Material Length */

		/*---------------------*/
		/* ipsec_frame_decrypt */
		/*---------------------*/

	} else { /* Transport Mode */
		/* For Transport mode set DPOVRD */
		/* 31 OVRD, 30-28 Reserved, 27-24 ECN (Not relevant for transport mode)
		 * 23-16 IP Header Length in bytes, 
		* of the portion of the IP header that is not encrypted.
		* 15-8 NH_OFFSET - location of the next header within the IP header.
		* 7-0 Reserved */
		dpovrd.transport_decap.ovrd = IPSEC_DPOVRD_OVRD_TRANSPORT;

		if (sap1.flags & IPSEC_FLG_IPV6) { /* IPv6 header */
			/* Get the NH_OFFSET for the last header before ESP */
			dpovrd.transport_decap.nh_offset = 
				ipsec_get_ipv6_nh_offset(
					(struct ipv6hdr *)PARSER_GET_OUTER_IP_POINTER_DEFAULT(),
					&(dpovrd.transport_decap.ip_hdr_len));
			
		} else { /* IPv4 */
			/* If transport/IPv4 for any non-zero value of NH_OFFSET 
			 * (typically set to 01h), the N byte comes from byte 9 of 
			 * the IP header */
			dpovrd.transport_decap.nh_offset = 0x1;
			
			/* Header Length up to ESP */
			dpovrd.transport_decap.ip_hdr_len = (uint8_t)
				((uint32_t)((uint8_t *)PARSER_GET_L5_OFFSET_DEFAULT()) - 
					(uint32_t)
						((uint8_t *)PARSER_GET_OUTER_IP_OFFSET_DEFAULT())); 
		}
		
		dpovrd.transport_decap.reserved = 0;

		/* 	If L2 header exist in the frame, save it and remove from frame */
		if (eth_length) {
		/* Save Ethernet header. Note: no swap */
		/* up to 6 VLANs x 4 bytes + 14 regular bytes */
			eth_pointer_default = (uint8_t *)PARSER_GET_ETH_POINTER_DEFAULT();
		
			for (i = 0 ; i < eth_length; i++) {
				eth_header[i] = *(eth_pointer_default + i);
			}
	
			/* Remove L2 Header */	
			/* Note: The gross running sum of the frame becomes invalid 
			 * after calling this function. */ 
			 l2_header_remove();
			
			// TODO: 
			/* For decryption in transport mode it is required to update 
			  * the running sum. */
		}
	}

	/*---------------------*/
	/* ipsec_frame_decrypt */
	/*---------------------*/
	
	/* 	5.	Save original FD[FLC], FD[FRC] (to stack) */
	orig_flc = LDPAA_FD_GET_FLC(HWC_FD_ADDRESS);
	orig_frc = LDPAA_FD_GET_FRC(HWC_FD_ADDRESS);
	
	/* 	6.	Update the FD[FLC] with the flow context buffer address. */
	LDPAA_FD_SET_FLC(HWC_FD_ADDRESS, IPSEC_FLC_ADDR(desc_addr));	
	
	/* 7.	Update the FD[FRC] with SEC DPOVRD parameters */
	LDPAA_FD_SET_FRC(HWC_FD_ADDRESS, *((uint32_t *)(&dpovrd)));

	/* 	8.	FDMA store default frame command 
	 * (for closing the frame, updating the other FD fields) */
	return_val = fdma_store_default_frame_data();
	
	/* 	9.	Prepare AAP parameters in the Workspace memory. */
	/* 3 USE_FLC_SP Use Flow Context Storage Profile = 1 */ 
	/* 8 OS_EX Ordering Scope Exclusive Phase.
	* 0 Indicates that the accelerator call is not made during the 
	* exclusive phase of an Ordering Scope.
	* 1 Indicates that the accelerator call is made during the 
	* exclusive phase of an Ordering Scope.
	*/
	
	/* Get OSM status (ordering scope mode and levels) */
	osm_get_scope(&scope_status);

	/* If in Concurrent ordering scope, move to Exclusive 
	 * (increment scope ID). */ 
	if (scope_status.scope_mode == IPSEC_OSM_CONCURRENT) {
	    /* Move to exclusive */
	    osm_scope_transition_to_exclusive_with_increment_scope_id();
		/* Set OS_EX so AAP will do relinquish */
		*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = 
				(IPSEC_AAP_USE_FLC_SP | IPSEC_AAP_OS_EX);
	} else {
		/* Call AAP without relinquish */
		*((uint32_t *)(HWC_ACC_IN_ADDRESS)) = IPSEC_AAP_USE_FLC_SP;
	}
		
	/* 	10.	Call the AAP */
	__e_hwacceli(AAP_SEC_ACCEL_ID);

	/* 	11.	SEC Doing Decryption */

	/* Check if started in concurrent mode */
	if (scope_status.scope_mode == IPSEC_OSM_CONCURRENT) {
		/* The AAP already did OSM relinquished, so just register that */
		REGISTER_OSM_CONCURRENT;
	}

	/* Update the SPID of the new frame (SEC output) in the HW Context*/
	*((uint8_t *)HWC_SPID_ADDRESS) = sap1.output_spid;
	
	/* Update the default segment length for the new frame  in 
	 * the presentation context */
	PRC_SET_SEGMENT_LENGTH(DEFAULT_SEGMENT_SIZE);
	
	/* 	12.	FDMA present default frame command */ 
	return_val = fdma_present_default_frame();

	/* 	13.	Read the SEC return status from the FD[FRC]. Use swap macro. */
	// TODO: which errors can happen in decryption?
	switch (LDPAA_FD_GET_FRC(HWC_FD_ADDRESS)) {
		case SEC_NO_ERROR:
			break;
		case SEC_SEQ_NUM_OVERFLOW: /** Sequence Number overflow */
			*dec_status |= IPSEC_SEQ_NUM_OVERFLOW;
			return_val = -1;
			break;
		case SEC_AR_LATE_PACKET:	/** Anti Replay Check: Late packet */
			*dec_status |= IPSEC_AR_LATE_PACKET;
			return_val = -1;
			break;
		case SEC_AR_REPLAY_PACKET:	/** Anti Replay Check: Replay packet */
			*dec_status |= IPSEC_AR_REPLAY_PACKET;
			return_val = -1;
			break;
		case SEC_ICV_COMPARE_FAIL:	/** ICV comparison failed */
			*dec_status |= IPSEC_ICV_COMPARE_FAIL;	
			return_val = -1;
			break;
		default:
			*dec_status |= IPSEC_GEN_ENCR_ERR;	
			return_val = -1;
	}

	/* 	14.	If encryption/encapsulation failed go to END (see below) */
	// TODO: check results
		
	/* 	15.	Get new running sum and byte count (encrypted/encapsulated frame) 
	 * from the FD[FLC] */
	
	/* From Martin Dorr 27-Mar-2014: 
	 * A 32-bit byte count is stored in the LS portion of the FLC in LE format.
	 * A 2-byte checksum is stored starting at offset 4 relative to the 
	 * beginning of the FLC.
	 * FLC[63:0] = { 16�b0, checksum[15:0], byte_count[31:0] }
	*/
	checksum = LH_SWAP(HWC_FD_ADDRESS + FD_FLC_DS_AS_CS_OFFSET + 2, 0);
	byte_count = LW_SWAP(HWC_FD_ADDRESS + FD_FLC_DS_AS_CS_OFFSET + 4, 0);
	
	/* 	16.	Update the gross running checksum in the Workspace parser results.*/
	pr->gross_running_sum = 0;
	// TODO: currently setting to 0 (invalid), so parser will call
	// FDMA to recalculate the gross running sum
	// Later need to manipulate the checksum returned from SEC
	
	// TODO: special handling of running sum in transport mode
	// since L2 header was removed
	
	/* In Transport mode, if L2 header existed in the original frame, 
	 * add it back */
	if ((!(sap1.flags & IPSEC_FLG_TUNNEL_MODE)) && eth_length) {
		/* Note: The Ethertype was already updated before removing the 
		 * L2 header */
		return_val = fdma_insert_default_segment_data(
				0, /* uint16_t to_offset */
				eth_header, /* void	 *from_ws_src */
				eth_length, /* uint16_t insert_size */
				FDMA_REPLACE_SA_REPRESENT_BIT 
					/* uint32_t flags */
				);
		
	}
	
			/*---------------------*/
			/* ipsec_frame_decrypt */
			/*---------------------*/

	/* 	17.	Run parser and check for errors. */
	return_val = parse_result_generate_default(PARSER_VALIDATE_L3_L4_CHECKSUM);
	// TODO: mask out some parser error bits, in case there is no L4 etc.
	// TODO: special handling in case of fragments
	
	/* 	18.	If validity check failed, go to END, return with error. */
	// TODO
	
	/* 	19.	Restore the original FD[FLC], FD[FRC] (from stack) */
	LDPAA_FD_SET_FLC(HWC_FD_ADDRESS, orig_flc);	
	LDPAA_FD_SET_FRC(HWC_FD_ADDRESS, orig_frc)	

	/* 	20.	Handle lifetime counters */
	/* 	20.1.	Read lifetime counters (CDMA) */
	/* 	20.2.	Add byte-count from SEC and one packet count. */
	/* 	20.3.	Calculate locally if lifetime counters crossed the limits. 
	 * If yes set flag in the descriptor statistics (CDMA write). */
	/* 	20.4.	Update the kilobytes and/or packets lifetime counters 
	 * (STE increment + accumulate). */
	if (sap1.flags & 
			(IPSEC_FLG_LIFETIME_KB_CNTR_EN | IPSEC_FLG_LIFETIME_PKT_CNTR_EN)) {
		ste_inc_and_acc_counters(
			//IPSEC_PACKET_COUNTER_ADDR,/* uint64_t counter_addr */
			IPSEC_PACKET_COUNTER_ADDR(desc_addr), /* uint64_t counter_addr */
			byte_count,	/* uint32_t acc_value */
			/* uint32_t flags */
			(STE_MODE_COMPOUND_64_BIT_CNTR_SIZE |  
			STE_MODE_COMPOUND_64_BIT_ACC_SIZE |
			STE_MODE_COMPOUND_CNTR_SATURATE |
			STE_MODE_COMPOUND_ACC_SATURATE));
	} else if (sap1.flags & IPSEC_FLG_LIFETIME_KB_CNTR_EN) {
		ste_inc_counter(
				//IPSEC_KB_COUNTER_ADDR,
				IPSEC_KB_COUNTER_ADDR(desc_addr),
				byte_count,
				(STE_MODE_SATURATE | STE_MODE_64_BIT_CNTR_SIZE));
	} else if (sap1.flags & IPSEC_FLG_LIFETIME_PKT_CNTR_EN) {
		ste_inc_counter(
				//IPSEC_PACKET_COUNTER_ADDR,
				IPSEC_PACKET_COUNTER_ADDR(desc_addr),
				1,
				(STE_MODE_SATURATE | STE_MODE_64_BIT_CNTR_SIZE));
	}
	
	return_val = IPSEC_SUCCESS;	

decrypt_end:
	
	/* 	21.	END */
	/* 	21.1. Update the encryption status (enc_status) and return status. */
	/* 	21.2. If started as Concurrent ordering scope, 
	 *  move from Exclusive to Concurrent  
	 *  (AAP does that, only register through OSM functions). */
	
	/* Decrement the reference counter */
	return_val = cdma_refcount_decrement(ipsec_handle);
	// TODO: check CDMA return status
	
	/* Return */
	return return_val;
} /* End of ipsec_frame_decrypt */

/**************************************************************************//**
	ipsec_get_lifetime_stats
*//****************************************************************************/
int ipsec_get_lifetime_stats(
		ipsec_handle_t ipsec_handle,
		uint64_t *kilobytes,
		uint64_t *packets,
		uint32_t *sec)
{
	
	int return_val;
	uint64_t current_timestamp;
	ipsec_handle_t desc_addr;

	/* Note: this struct must be equal to the head of ipsec_sa_params_part1 */
	struct counters_and_timestamp {
		uint64_t packet_counter; /*	Packets counter, 8B */
		uint64_t byte_counter; /* Encrypted/decrypted bytes counter, 8B */
		uint64_t timestamp; /* TMAN timestamp in micro-seconds, 8 Bytes */
	} ctrs;
	
	/* Increment the reference counter */
	cdma_refcount_increment(ipsec_handle);
	// TODO: check CDMA return status

	desc_addr = IPSEC_DESC_ADDR(ipsec_handle);

	/* Flush all the counter updates that are pending in the 
	 * statistics engine request queue. */
	ste_barrier();

	/* 	Read relevant descriptor fields with CDMA. */
	cdma_read(
			&ctrs, /* void *ws_dst */
			desc_addr, /* uint64_t ext_address */
			sizeof(ctrs) /* uint16_t size */
			);
	
	*packets = ctrs.packet_counter;
	*kilobytes =  ctrs.byte_counter;
	
	/* Get current timestamp from TMAN (in micro-seconds)*/
	tman_get_timestamp(&current_timestamp);

	/* Calculate elapsed time in seconds */
	/* Do shift 20, since 2^20 = 1,048,576 */
	if (current_timestamp >= ctrs.timestamp) { /* No roll-over */
		*sec = (uint32_t)((current_timestamp - ctrs.timestamp)>>20);
	} else { /* Roll-over */
		*sec = (uint32_t)(
				(current_timestamp + 
						(IPSEC_MAX_TIMESTAMP - ctrs.timestamp) + 1)>>20);
	}

	/* Decrement the reference counter */
	return_val = cdma_refcount_decrement(ipsec_handle);
	// TODO: check CDMA return status
	
	return IPSEC_SUCCESS;
	
} /* End of ipsec_get_lifetime_stats */

/**************************************************************************//**
	ipsec_decr_lifetime_counters
*//****************************************************************************/
int ipsec_decr_lifetime_counters(
		ipsec_handle_t ipsec_handle,
		uint32_t kilobytes_decr_val,
		uint32_t packets_decr_val
		)
{
	/* Note: there is no check of counters enable, nor current value.
	 * Assuming that it is only called appropriately by the upper layer */
	int return_val;
	ipsec_handle_t desc_addr;

	/* Increment the reference counter */
	cdma_refcount_increment(ipsec_handle);
	
	desc_addr = IPSEC_DESC_ADDR(ipsec_handle);

	/* Flush all the counter updates that are pending in the 
	 * statistics engine request queue. */
	ste_barrier();
	
	if (kilobytes_decr_val) {
		ste_dec_counter(
				//IPSEC_KB_COUNTER_ADDR,
				IPSEC_KB_COUNTER_ADDR(desc_addr),
				kilobytes_decr_val,
				(STE_MODE_SATURATE | STE_MODE_64_BIT_CNTR_SIZE));
	}
	
	if (packets_decr_val) {
		ste_dec_counter(
				IPSEC_PACKET_COUNTER_ADDR(desc_addr),
				packets_decr_val,
				(STE_MODE_SATURATE | STE_MODE_64_BIT_CNTR_SIZE));
	}	
	
	/* Decrement the reference counter */
	return_val = cdma_refcount_decrement(ipsec_handle);
	// TODO: check CDMA return status
	
	return IPSEC_SUCCESS;	
} /* End of ipsec_decr_lifetime_counters */

/**************************************************************************//**
	ipsec_get_seq_num
*//****************************************************************************/
int ipsec_get_seq_num(
		ipsec_handle_t ipsec_handle,
		uint32_t *sequence_number,
		uint32_t *extended_sequence_number,
		uint32_t anti_replay_bitmap[4])
{
	
	int return_val;
	ipsec_handle_t desc_addr;
	uint32_t params_flags;

	union {
		struct ipsec_encap_pdb encap_pdb;
		struct ipsec_decap_pdb decap_pdb;
	} pdb;
	
	/* Increment the reference counter */
	cdma_refcount_increment(ipsec_handle);
	
	desc_addr = IPSEC_DESC_ADDR(ipsec_handle);

	/* Read he descriptor flags to identify the direction */
	cdma_read(
			&params_flags, /* void *ws_dst */
			IPSEC_FLAGS_ADDR(desc_addr), /* uint64_t ext_address */
			sizeof(params_flags) /* uint16_t size */
	);
	
	/* Outbound (encapsulation) PDB format */
	if (params_flags & IPSEC_FLG_DIR_OUTBOUND) {
		/* 	Read the PDB from the descriptor with CDMA. */
		cdma_read(
			&pdb.encap_pdb, /* void *ws_dst */
			IPSEC_PDB_ADDR(desc_addr), /* uint64_t ext_address */
			sizeof(pdb.encap_pdb) /* uint16_t size */
		);
		
		/* Return swapped values (little to big endian conversion) */
		*extended_sequence_number = LW_SWAP(0,&(pdb.encap_pdb.seq_num_ext_hi));
		*sequence_number = LW_SWAP(0,&(pdb.encap_pdb.seq_num));
		
		/* No anti-replay bitmap for encap, so just return zero */
		anti_replay_bitmap[0] = 0x0;
		anti_replay_bitmap[1] = 0x0;
		anti_replay_bitmap[2] = 0x0;
		anti_replay_bitmap[3] = 0x0;
	} else {
	/* Inbound (decapsulation) PDB format */
				
		/* 	Read the PDB from the descriptor with CDMA. */
		cdma_read(
			&(pdb.decap_pdb), /* void *ws_dst */
			IPSEC_PDB_ADDR(desc_addr), /* uint64_t ext_address */
			sizeof(pdb.decap_pdb) /* uint16_t size */
		);
	
		/* Return swapped values (little to big endian conversion) */
		*extended_sequence_number = LW_SWAP(0,&(pdb.decap_pdb.seq_num_ext_hi));
		*sequence_number = LW_SWAP(0,&(pdb.decap_pdb.seq_num));

		switch (pdb.decap_pdb.options & IPSEC_DECAP_PDB_ARS_MASK) {
			case IPSEC_DEC_OPTS_ARSNONE:
				anti_replay_bitmap[0] = 0x0;
				anti_replay_bitmap[1] = 0x0;
				anti_replay_bitmap[2] = 0x0;
				anti_replay_bitmap[3] = 0x0;
				break;
			case IPSEC_DEC_OPTS_ARS32:
				anti_replay_bitmap[0] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[0]));
				anti_replay_bitmap[1] = 0x0;
				anti_replay_bitmap[2] = 0x0;
				anti_replay_bitmap[3] = 0x0;
				break;
			case IPSEC_DEC_OPTS_ARS64:
				anti_replay_bitmap[0] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[0]));
				anti_replay_bitmap[1] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[1]));
				anti_replay_bitmap[2] = 0x0;
				anti_replay_bitmap[3] = 0x0;
				break;		
			case IPSEC_DEC_OPTS_ARS128:	
				anti_replay_bitmap[0] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[0]));
				anti_replay_bitmap[1] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[1]));
				anti_replay_bitmap[2] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[2]));
				anti_replay_bitmap[3] = LW_SWAP(
						0,&(pdb.decap_pdb.anti_replay[3]));
				break;
			default:
				anti_replay_bitmap[0] = 0x0;
				anti_replay_bitmap[1] = 0x0;
				anti_replay_bitmap[2] = 0x0;
				anti_replay_bitmap[3] = 0x0;	
		}
	}
	
	/* Derement the reference counter */
	return_val = cdma_refcount_decrement(ipsec_handle);
	// TODO: check CDMA return status
	
	return IPSEC_SUCCESS;	

} /* End of ipsec_get_seq_num */

/**************************************************************************//**
	ipsec_get_ipv6_nh_offset
	
	The Destination header creates 2 different options for IPv6 extensions order 

	1.	IPv6 header � Destination � Routing � Fragment � Destination 
	The first destination header is for intermediate destinations, 
	and the second one is for the last destination.
	This option can occur only when Routing header is present and 
	the first destination placed before Routing. 
	The second Destination header is optional 
 
	2.	IPv6 header � Fragment - Destination 
	The destination header is for the last destination.
	Routing header is not present, or 
	Destination is placed after Routing header.
	
*//****************************************************************************/
uint8_t ipsec_get_ipv6_nh_offset(struct ipv6hdr *ipv6_hdr, uint8_t *length)
{
	uint32_t current_hdr_ptr;
	uint16_t current_hdr_size;
	uint8_t current_ver;
	uint8_t next_hdr;
	uint8_t dst_ext;
	uint8_t nh_offset = 0; /* default value for no extensions */
	uint8_t header_after_dest;
	
	/* Destination extension can appear only once on fragment request */
	dst_ext = IPV6_EXT_DESTINATION;

	/* Copy initial IPv6 header */
	current_hdr_ptr = (uint32_t)ipv6_hdr;
	current_hdr_size = IPV6_HDR_LENGTH;
	next_hdr = ipv6_hdr->next_header;
	
	/* IP Header Length for SEC encapsulation, including IP header and
	 * extensions before ESP */
	*length = IPV6_HDR_LENGTH;
	
	/* Skip to next extension header until extension isn't ipv6 header
	 * or until extension is the fragment position (depend on flag) */
	while ((next_hdr == IPV6_EXT_HOP_BY_HOP) ||
		(next_hdr == IPV6_EXT_ROUTING) || (next_hdr == dst_ext) ||
		(next_hdr == IPV6_EXT_FRAGMENT)) {

		current_ver = next_hdr;
		current_hdr_ptr += current_hdr_size;
		next_hdr = *((uint8_t *)(current_hdr_ptr));
		current_hdr_size = *((uint8_t *)(current_hdr_ptr + 1));

		/* Calculate current extension size  */
		switch (current_ver) {

		case IPV6_EXT_DESTINATION:
		{
			/* If the next header is not Routing, this should be
			 * the starting point for ESP encapsulation  */
			if (next_hdr != IPV6_EXT_ROUTING) {
				/* Don't add to NH_OFFSET/length and Exit from the while loop */
				dst_ext = 0;
			} else {
				/* Next header is Routing */
				nh_offset += current_hdr_size; /* in 8 bytes multiples */
				current_hdr_size = ((current_hdr_size + 1) << 3);
				*length += current_hdr_size;
			}
			break;
		}
		case IPV6_EXT_FRAGMENT:
		{
			/* Increment NH_OFFSET only if next header is extension */
			if ((next_hdr == IPV6_EXT_ROUTING) ||
				(next_hdr == IPV6_EXT_HOP_BY_HOP)) {
				/* in 8 bytes multiples */
				nh_offset += IPV6_FRAGMENT_HEADER_LENGTH>>3; 
			} else if (next_hdr == IPV6_EXT_DESTINATION) {
				/* Get the header after the following destination */
				header_after_dest = 
					*((uint8_t *)(current_hdr_ptr + 
							IPV6_FRAGMENT_HEADER_LENGTH));
				/* Increment NH_OFFSET only if this is not the last ext. header
				 * before Destination */
				if (header_after_dest == IPV6_EXT_ROUTING) {
					nh_offset += IPV6_FRAGMENT_HEADER_LENGTH>>3; 
				}
			}

			current_hdr_size = IPV6_FRAGMENT_HEADER_LENGTH;
			*length += current_hdr_size;
			break;
		}

		/* Routing, Hop By Hop */
		default:
		{
			/* Increment NH_OFFSET only if next header is extension */
			if ((next_hdr == IPV6_EXT_ROUTING) ||
				(next_hdr == IPV6_EXT_HOP_BY_HOP) ||	
				(next_hdr == IPV6_EXT_FRAGMENT)) {
				/* in 8 bytes multiples */
				nh_offset += current_hdr_size; 
			} else if (next_hdr == IPV6_EXT_DESTINATION) {
				header_after_dest = 
					*((uint8_t *)(current_hdr_ptr + 
							((current_hdr_size + 1)<<3)));
				/* Increment NH_OFFSET only if this is not the last ext. header
				 * before Destination */
				if (header_after_dest == IPV6_EXT_ROUTING) {
					nh_offset += IPV6_FRAGMENT_HEADER_LENGTH>>3; 
				}
			}
			
			current_hdr_size = ((current_hdr_size + 1) << 3);
			*length += current_hdr_size;
			break;
		}
		}
	}

	/* Return NH_OFFSET as expected by the SEC */
	if (nh_offset) {
		/* NH_OFFSET in 8 bytes multiples for IP header + Extensions */
		return (nh_offset + (IPV6_HDR_LENGTH>>3));
	} else {
		return 0x1; /* NH_OFFSET in case of no Extensions */
	}
} /* End of ipsec_get_ipv6_nh_offset */


/**************************************************************************/

#pragma pop 

/** @} */ /* end of FSL_IPSEC_Functions */

/** @} */ /* end of FSL_IPSEC */
/** @} */ /* end of NETF */


