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

/* Copyright 2008-2013 Freescale Semiconductor, Inc. */

#ifndef __DESC_MACSEC_H__
#define __DESC_MACSEC_H__

#include "flib/rta.h"
#include "common.h"

/**
 * DOC: MACsec Shared Descriptor Constructors
 *
 * Shared descriptors for MACsec protocol.
 */

/**
 * enum cipher_type_macsec - Type selectors for cipher types in MACSEC protocol
 * @MACSEC_CIPHER_TYPE_GCM: MACsec to use GCM as algorithm
 * @MACSEC_CIPHER_TYPE_GMAC: MACsec to use GMAC as algorithm
 */
enum cipher_type_macsec {
	MACSEC_CIPHER_TYPE_GCM,
	MACSEC_CIPHER_TYPE_GMAC
};

/*
 * IEEE 801.AE MacSEC Protocol Data Block
 */
#define MACSEC_PDBOPTS_FCS	0x01
#define MACSEC_PDBOPTS_AR	0x40	/* used in decap only */

struct macsec_encap_pdb {
	uint16_t aad_len;
	uint8_t rsvd;
	uint8_t options;
	uint32_t sci_hi;
	uint32_t sci_lo;
	uint16_t ethertype;
	uint8_t tci_an;
	uint8_t rsvd1;
	/* begin DECO writeback region */
	uint32_t pn;
	/* end DECO writeback region */
};

struct macsec_decap_pdb {
	uint16_t aad_len;
	uint8_t rsvd;
	uint8_t options;
	uint32_t sci_hi;
	uint32_t sci_lo;
	uint8_t rsvd1[3];
	/* begin DECO writeback region */
	uint8_t antireplay_len;
	uint32_t pn;
	uint32_t antireplay_scorecard_hi;
	uint32_t antireplay_scorecard_lo;
	/* end DECO writeback region */
};

/**
 * cnstr_shdsc_macsec_encap - MACsec(802.1AE) encapsulation
 * @descbuf: pointer to descriptor-under-construction buffer
 * @cipherdata: pointer to block cipher transform definitions
 * @sci: PDB Secure Channel Identifier
 * @ethertype: PDB EtherType
 * @tci_an: TAG Control Information and Association Number are treated as a
 *          single field of 8 bits in PDB.
 * @pn: PDB Packet Number
 *
 * Return: size of descriptor written in words
 */
static inline int cnstr_shdsc_macsec_encap(uint32_t *descbuf,
					   struct alginfo *cipherdata,
					   uint64_t sci, uint16_t ethertype,
					   uint8_t tci_an, uint32_t pn)
{
	struct program prg;
	struct program *p = &prg;
	struct macsec_encap_pdb pdb;
	uint32_t startidx;

	LABEL(keyjump);
	REFERENCE(pkeyjump);

	if ((cipherdata->algtype == MACSEC_CIPHER_TYPE_GMAC) &&
	    (rta_sec_era < RTA_SEC_ERA_5))
		pr_err("MACsec GMAC available only for Era 5 or above\n");

	memset(&pdb, 0x00, sizeof(struct macsec_encap_pdb));
	pdb.sci_hi = upper_32_bits(sci);
	pdb.sci_lo = lower_32_bits(sci);
	pdb.ethertype = ethertype;
	pdb.tci_an = tci_an;
	pdb.pn = pn;

	startidx = sizeof(struct macsec_encap_pdb) >> 2;

	PROGRAM_CNTXT_INIT(p, descbuf, 0);
	SHR_HDR(p, SHR_SERIAL, ++startidx, SC);
	{
		COPY_DATA(p, (uint8_t *)&pdb, sizeof(struct macsec_encap_pdb));
		pkeyjump = JUMP(p, keyjump, LOCAL_JUMP, ALL_TRUE,
				SHRD | SELF | BOTH);
		KEY(p, KEY1, cipherdata->key_enc_flags, cipherdata->key,
		    cipherdata->keylen, INLINE_KEY(cipherdata));
		SET_LABEL(p, keyjump);
		PROTOCOL(p, OP_TYPE_ENCAP_PROTOCOL, OP_PCLID_MACSEC,
			 OP_PCL_MACSEC);
	}
	PATCH_JUMP(p, pkeyjump, keyjump);
	return PROGRAM_FINALIZE(p);
}

/**
 * cnstr_shdsc_macsec_decap - MACsec(802.1AE) decapsulation
 * @descbuf: pointer to descriptor-under-construction buffer
 * @cipherdata: pointer to block cipher transform definitions
 * @sci: PDB Secure Channel Identifier
 * @pn: PDB Packet Number
 *
 * Return: size of descriptor written in words
 */
static inline int cnstr_shdsc_macsec_decap(uint32_t *descbuf,
					   struct alginfo *cipherdata,
					   uint64_t sci, uint32_t pn)
{
	struct program prg;
	struct program *p = &prg;
	struct macsec_decap_pdb pdb;
	uint32_t startidx;

	LABEL(keyjump);
	REFERENCE(pkeyjump);

	if ((cipherdata->algtype == MACSEC_CIPHER_TYPE_GMAC) &&
	    (rta_sec_era < RTA_SEC_ERA_5))
		pr_err("MACsec GMAC available only for Era 5 or above\n");

	memset(&pdb, 0x00, sizeof(struct macsec_decap_pdb));
	pdb.sci_hi = upper_32_bits(sci);
	pdb.sci_lo = lower_32_bits(sci);
	pdb.pn = pn;

	startidx = sizeof(struct macsec_decap_pdb) >> 2;

	PROGRAM_CNTXT_INIT(p, descbuf, 0);
	SHR_HDR(p, SHR_SERIAL, ++startidx, SC);
	{
		COPY_DATA(p, (uint8_t *)&pdb, sizeof(struct macsec_decap_pdb));
		pkeyjump = JUMP(p, keyjump, LOCAL_JUMP, ALL_TRUE,
				SHRD | SELF | BOTH);
		KEY(p, KEY1, cipherdata->key_enc_flags, cipherdata->key,
		    cipherdata->keylen, INLINE_KEY(cipherdata));
		SET_LABEL(p, keyjump);
		PROTOCOL(p, OP_TYPE_DECAP_PROTOCOL, OP_PCLID_MACSEC,
			 OP_PCL_MACSEC);
	}
	PATCH_JUMP(p, pkeyjump, keyjump);
	return PROGRAM_FINALIZE(p);
}

#endif /* __DESC_MACSEC_H__ */
