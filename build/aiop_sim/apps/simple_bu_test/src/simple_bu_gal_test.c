/*
 * Copyright 2014-2015 Freescale Semiconductor, Inc.
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

#include "common/types.h"
#include "common/fsl_stdio.h"
#include "fsl_dpni_drv.h"
#include "fsl_ip.h"
#include "platform.h"
#include "fsl_io.h"
#include "fsl_parser.h"
#include "general.h"
#include "fsl_dbg.h"
/*#include "fsl_cmdif_server.h"*/
#include "dplib/fsl_cdma.h"
#include "dplib/fsl_fdma.h"
#include "dplib/fsl_ip.h"
#include "dplib/fsl_l4.h"
#include "ls2085_aiop/fsl_platform.h"
#include "fsl_tman.h"
#include "fsl_slab.h"
#include "fsl_malloc.h"
#include "fsl_frame_operations.h"
#include "fsl_ipr.h"
#include "ipr.h"
#include "fsl_l2.h"
#include "fsl_table.h"
#include "fsl_keygen.h"
#include "fsl_ste.h"
#include "simple_bu_test.h"


void test_tmi_create();
void test_fdma_discard_fd();
void test_fdma_modify_default_segment_data();
void test_ste();

extern struct  ipr_global_parameters ipr_global_parameters1;
extern __PROFILE_SRAM struct storage_profile storage_profile[SP_NUM_OF_STORAGE_PROFILES];

uint8_t snic_tmi_id_gal;
uint64_t snic_tmi_mem_base_addr_gal;


#define APP_NI_GET(ARG)   ((uint16_t)((ARG) & 0x0000FFFF))
/**< Get NI from callback argument, it's demo specific macro */
#define APP_FLOW_GET(ARG) (((uint16_t)(((ARG) & 0xFFFF0000) >> 16)
/**< Get flow id from callback argument, it's demo specific macro */

#define FRAME_SIZE	124
#define	COPY_SIZE	16
#define	MODIFY_SIZE	16
#define	INC_VAL		7
#define	DEC_VAL		4
#define SP_BDI_MASK     0x00080000
#define SP_BP_PBS_MASK  0x3FFF

int simple_bu_gal_test(void)
{
	int        err  = 0;
	uint8_t prpid;
	int i;
	
	fsl_os_print("****************************\n");
	fsl_os_print("Running simple bring-up test\n");
	fsl_os_print("****************************\n");
	fsl_os_print("****************************\n");
	
	parser_init(&prpid);

	default_task_params.parser_profile_id = prpid;
	default_task_params.parser_starting_hxs = 0;

	// run create_frame on default frame
	{
		struct ldpaa_fd *fd = (struct ldpaa_fd *)HWC_FD_ADDRESS;
		uint8_t frame_data[FRAME_SIZE] = {0x00,0x00,0x01,0x00,0x00,0x01,0x00,\
				0x10,0x94,0x00,0x00,0x02,0x08,0x00,0x45,0x00,\
				0x00,0x6e,0x00,0x00,0x00,0x00,0xff,0x11,0x3a,\
				0x26,0xc0,0x55,0x01,0x02,0xc0,0x00,0x00,0x01,\
				0x04,0x00,0x04,0x00,0x00,0x5a,0xff,0xff,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf5,0xd0,\
				0x64,0x51,0xac,0x9f,0x69,0xd4,0xd3,0xf7,0x39,\
				0x6e,0x20,0x0e,0x97,0xb7,0xe9,0xe4,0x56,0x3a};
		uint8_t frame_data_read[FRAME_SIZE+4] = {0x00,0x00,0x01,0x00,0x00,0x01,0x00,\
				0x10,0x94,0x00,0x00,0x02,0x81,0x00,0xaa,0xbb,0x08,0x00,0x45,0x00,\
				0x00,0x6e,0x00,0x00,0x00,0x00,0xff,0x11,0x3a,\
				0x26,0xc0,0x55,0x01,0x02,0xc0,0x00,0x00,0x01,\
				0x04,0x00,0x04,0x00,0x00,0x5a,0xff,0xff,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf5,0xd0,\
				0x64,0x51,0xac,0x9f,0x69,0xd4,0xd3,0xf7,0x39,\
				0x6e,0x20,0x0e,0x97,0xb7,0xe9,0xe4,0x56,0x3a};

		uint8_t frame_handle;
		uint32_t vlan = 0x8100aabb;
		int parse_status;
		uint8_t *frame_presented;
		uint8_t *seg_addr;
		struct fdma_amq amq;
		uint16_t icid, flags = 0;
		uint8_t tmp;
		uint32_t frame_length;
		/* setting SPID = 0 */
		*((uint8_t *)HWC_SPID_ADDRESS) = 0;
		icid = (uint16_t)(storage_profile[0].ip_secific_sp_info >> 48);
		icid = ((icid << 8) & 0xff00) | ((icid >> 8) & 0xff);
		tmp = (uint8_t)(storage_profile[0].ip_secific_sp_info >> 40);
		if (tmp & 0x08)
			flags |= FDMA_ICID_CONTEXT_BDI;
		if (tmp & 0x04)
			flags |= FDMA_ICID_CONTEXT_PL;
		if (storage_profile[0].mode_bits2 & sp1_mode_bits2_VA_MASK)
			flags |= FDMA_ICID_CONTEXT_VA;
		amq.icid = icid;
		amq.flags = flags;
		set_default_amq_attributes(&amq);
		*(uint32_t *)(&storage_profile[0].pbs2) = *(uint32_t *)(&storage_profile[0].pbs1);

		for (i=0; i<8 ; i++)
			fsl_os_print("storage profile arg %d: 0x%x \n", i, *((uint32_t *)(&(storage_profile[0]))+i));
		
		
		err = create_frame(fd, frame_data, FRAME_SIZE, &frame_handle);
		if (err)
			fsl_os_print("ERROR: create frame failed!\n");

		
		fsl_os_print("parse result before create frame - \n");
		
		fsl_os_print("ethernet offset %d %x\n", 
					PARSER_IS_ETH_MAC_DEFAULT(), PARSER_GET_ETH_OFFSET_DEFAULT());
		
		fsl_os_print("vlan offset %d %x\n",
					PARSER_IS_ONE_VLAN_DEFAULT(), PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT());
		
		fsl_os_print("ipv4 offset %d %x\n", 
					PARSER_IS_IP_DEFAULT(), PARSER_GET_OUTER_IP_OFFSET_DEFAULT());
		
		fsl_os_print("udp offset %d %x\n", 
					PARSER_IS_UDP_DEFAULT(), PARSER_GET_L4_OFFSET_DEFAULT());

		for (i=0; i<16 ; i++)
			fsl_os_print("parse results arg %d: 0x%x \n", i, *((uint32_t *)(0x80)+i));
		
		//test_fdma_modify_default_segment_data();
		
		fdma_close_default_segment();
		err = fdma_present_default_frame_segment(FDMA_PRES_NO_FLAGS, (void *)0x180, 0, 256);
		if (err)
			fsl_os_print("STATUS: fdma present default segment returned status is %d\n", err);
		l2_push_and_set_vlan(vlan);
		
		frame_length = PRC_GET_SEGMENT_LENGTH();
		seg_addr = (uint8_t *)PRC_GET_SEGMENT_ADDRESS();
		
		/*fsl_os_print("frame length is 0x%x\n", frame_length);
		for (i=0; i<frame_length ; i++)
			fsl_os_print("frame read byte %d is %x\n", i, seg_addr[i]);*/

		parse_result_generate(PARSER_ETH_STARTING_HXS, 0, PARSER_NO_FLAGS);
		
		fsl_os_print("parse result after create frame - \n");
		
		fsl_os_print("ethernet offset %d %x\n", 
					PARSER_IS_ETH_MAC_DEFAULT(), PARSER_GET_ETH_OFFSET_DEFAULT());
		
		fsl_os_print("vlan offset %d %x\n",
					PARSER_IS_ONE_VLAN_DEFAULT(), PARSER_GET_FIRST_VLAN_TCI_OFFSET_DEFAULT());
		
		fsl_os_print("ipv4 offset %d %x\n", 
					PARSER_IS_IP_DEFAULT(), PARSER_GET_OUTER_IP_OFFSET_DEFAULT());
		
		fsl_os_print("udp offset %d %x\n", 
					PARSER_IS_UDP_DEFAULT(), PARSER_GET_L4_OFFSET_DEFAULT());
		fsl_os_print(" FD length (by SW) is : 0x%x \n", LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS));

		LDPAA_FD_SET_LENGTH(HWC_FD_ADDRESS, 0);
		fsl_os_print(" FD length (after SW zeroing it) is : 0x%x \n", LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS));
		err = fdma_store_default_frame_data();
		if (err){
			fsl_os_print("ERROR: fdma store default frame returned error is %d\n", err);
			return err;
		}
		
		//test_fdma_discard_fd();
		
		test_tmi_create();
		
		for (i=0; i<8 ; i++)
			fsl_os_print("FD content arg %d is %x\n", i, *((uint32_t *)(0x60 + i*4)));
		
		fsl_os_print(" FD length is : 0x%x \n", LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS));
		fsl_os_print(" FD address LSB is : 0x%x \n", LDPAA_FD_GET_ADDR(HWC_FD_ADDRESS));
		fsl_os_print(" FD address MSB is : 0x%x \n", ((uint64_t)LDPAA_FD_GET_ADDR(HWC_FD_ADDRESS)) >> 32);

		err = fdma_present_default_frame();
		if (err < 0)
			fsl_os_print("ERROR: fdma present default frame returned error is %d\n", err);
		else
			if (err)
				fsl_os_print("STATUS: fdma present default frame returned status is %d\n", err);
		parse_status = parse_result_generate_default(PARSER_NO_FLAGS);
		if (parse_status)
		{
			fsl_os_print("ERROR: parser failed for simple BU test!\n");
			fdma_discard_default_frame(FDMA_DIS_NO_FLAGS);
		}
		/* check frame presented */
		frame_presented = (uint8_t *)PRC_GET_SEGMENT_ADDRESS();
		frame_length = LDPAA_FD_GET_LENGTH(HWC_FD_ADDRESS);
		
		for (i=0; i<(FRAME_SIZE+4); i++)
			if (*(frame_presented+i) != frame_data_read[i])
				err = -EINVAL;
		if (err)
		{
			fsl_os_print("Simple BU ERROR: frame data after HM is not correct\n");
			fsl_os_print("frame length is 0x%x\n", frame_length);
			//for (i=0; i<frame_length ; i++)
			//	fsl_os_print("frame read byte %d is %x\n", i, frame_data_read[i]);
			fsl_os_print("actual frame length is 0x%x\n", frame_length);
			//for (i=0; i<frame_length ; i++)
			//	fsl_os_print("actual frame read byte %d is %x\n", i, frame_presented[i]);
			fdma_discard_default_frame(FDMA_DIS_NO_FLAGS);
			return err;
		}
		else {
			fsl_os_print("**************************************************\n");
			fsl_os_print("Simple BU Test: fdma frame after HM is correct !!!\n");
			fsl_os_print("**************************************************\n");
		}
		
		test_fdma_copy_data();
		
		fdma_discard_default_frame(FDMA_DIS_NO_FLAGS);
	}
	
	//test_ste();
	
	
	fsl_os_print("Simple bring-up test completed successfully\n");
	return 0;
}

/*  Test fdma_copy_data  */
void test_fdma_copy_data()
{
	int err, i;
	void *src_addr;
	void *dst_addr;
	uint16_t copy_size;
	uint32_t flags;
	uint8_t copy_data[COPY_SIZE] = {0x0a, 0x45, 0x77, 0x11, 0x39, 0x97, 0x63, 0x87, 
				 0x33, 0x22, 0x54, 0x18, 0x84, 0xfa, 0xcb, 0xed};
	
	/* ws->ws */
	//fsl_os_print("FDMA Copy WS->WS \n");
	//src_addr = (void *)(copy_data);
	//dst_addr = (void *)0x180;
	//copy_size = COPY_SIZE;
	//flags = 0;
	
	/* ws->sram */
	//fsl_os_print("FDMA Copy WS->SRAM \n");
	//src_addr = (void *)copy_data;
	//dst_addr = (void *)(&(storage_profile[0]));
	//copy_size = COPY_SIZE;
	//flags = FDMA_COPY_DM_BIT;
	
	/* sram->ws */
	fsl_os_print("FDMA Copy SRAM->WS \n");
	src_addr = (void *)(&(storage_profile[0]));
	dst_addr = (void *)0x180;
	copy_size = sizeof(struct storage_profile);
	flags = FDMA_COPY_SM_BIT;
	
	/* sram->sram */
	//fsl_os_print("FDMA Copy SRAM->SRAM \n");
	//src_addr = (void *)(&(storage_profile[0]));
	//dst_addr = (void *)((uint32_t)(&(storage_profile[0]) + sizeof(struct storage_profile)*4));
	//copy_size = sizeof(struct storage_profile);
	//flags = FDMA_COPY_SM_BIT | FDMA_COPY_DM_BIT;
	
	
	fdma_copy_data(copy_size, flags, src_addr, dst_addr);
	fsl_os_print("Simple BU: fdma_copy_data params:\n size = %d, flags = %d, src_address = 0x%x, dst_address = 0x%x \n",
			sizeof(struct storage_profile), FDMA_COPY_SM_BIT, src_addr, dst_addr);
	fsl_os_print("copy command HW arg1 = 0x%x\n", *(uint32_t *) 0x20);
	fsl_os_print("copy command HW arg2 = 0x%x\n", *(uint32_t *) 0x24);
	fsl_os_print("copy command HW arg3 = 0x%x\n", *(uint32_t *) 0x28);
	fsl_os_print("copy command HW arg4 = 0x%x\n", *(uint32_t *) 0x2C);
	err = 0;
	for (i=0; i<copy_size; i+=4)
		if (*((uint32_t *)((uint32_t)src_addr+i)) != *((uint32_t *)((uint32_t)dst_addr+i))) {
			err = -EINVAL;
			fsl_os_print("arg %d: 0x%x != 0x%x\n", i, *((uint32_t *)((uint32_t)src_addr+i)),
					*((uint32_t *)((uint32_t)dst_addr+i)));
		}
	if (err)
	{
		fsl_os_print("Simple BU ERROR: fdma_copy_data FAILED\n");
		for (i=0; i<copy_size; i+=4) {
			fsl_os_print("Copy src arg %d = 0x%x\n", i/4, *((uint32_t *)((uint32_t)src_addr+i)));
			fsl_os_print("Copy dst arg %d = 0x%x\n", i/4, *((uint32_t *)((uint32_t)dst_addr+i)));
		}
	}
	else
		fsl_os_print("Simple BU ERROR: fdma_copy_data PASSED\n");
}

void test_tmi_create()
{
	uint64_t time;
	fsl_os_get_mem( 4*64, MEM_PART_DP_DDR, 64, 
			&snic_tmi_mem_base_addr_gal);
	tman_get_timestamp(&time);
	fsl_os_print("Timestamp = 0x%x\n", time);
	tman_get_timestamp(&time);
	fsl_os_print("Timestamp = 0x%x\n", time);
	tman_get_timestamp(&time);
	fsl_os_print("Timestamp = 0x%x\n", time);
	/* todo tmi delete in snic_free */
	tman_create_tmi(snic_tmi_mem_base_addr_gal , 4, 
			&snic_tmi_id_gal);
}

void test_fdma_discard_fd()
{
	int err  = 0;
	
	err = fdma_discard_fd((struct ldpaa_fd *)HWC_FD_ADDRESS, FDMA_DIS_NO_FLAGS);
	if (err)
		fsl_os_print("*** ERROR: fdma_discard_fd FAILED! ***\n");
	else
		fsl_os_print("\n*** fdma_discard_fd PASSED! ***\n\n");
}

void test_fdma_modify_default_segment_data()
{
	int err  = 0;
	uint8_t i;
	uint8_t modify_data[MODIFY_SIZE] = {0x0a, 0x45, 0x77, 0x11, 0x39, 0x97, 0x63, 0x87, 
					 0x33, 0x22, 0x54, 0x18, 0x84, 0xfa, 0xcb, 0xed};
	uint8_t *seg_addr = (uint8_t *)PRC_GET_SEGMENT_ADDRESS();
	
	for (i=0; i<MODIFY_SIZE; i++) 
		seg_addr[i] = modify_data[i];
	
	fdma_modify_default_segment_data(0, MODIFY_SIZE);
	fdma_close_default_segment();
	for (i=0; i<MODIFY_SIZE; i++) {
		seg_addr[i] = 0;
		fsl_os_print("0 data byte %d: %d \n", i, seg_addr[i]);
	}
	err = fdma_present_default_frame_segment(FDMA_PRES_NO_FLAGS, (void *)seg_addr, 0, 256);
	if (err)
		fsl_os_print("STATUS: fdma present default segment returned status is %d\n", err);
	err = 0;
	for (i=0; i<MODIFY_SIZE; i++)
		if (seg_addr[i] != modify_data[i]){
			err = -EIO;
			fsl_os_print("byte %d: 0x%x != 0x%x\n", i, seg_addr[i], modify_data[i]);
		}
	
	if (err)
		fsl_os_print("*** ERROR: fdma_modify_default_segment_data FAILED !!! \n");
	else{
		fsl_os_print("*** fdma_modify_default_segment_data PASSED !!! \n\n");
		for (i=0; i<MODIFY_SIZE; i++) {
			fsl_os_print("Good data byte %d: %d \n", i, seg_addr[i]);
		};
	}
}

void test_ste()
{
	int err  = 0;
	uint64_t ext_addr;
	uint16_t icid, bpid;
	uint32_t flags;
	struct storage_profile *sp;
	uint32_t data[COPY_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint32_t read_data[COPY_SIZE] = {1,2,3,4,5,6,7,8,9,10};
	uint32_t counter = 0;
	uint8_t i;
	
	sp = &storage_profile[*((uint8_t *)HWC_SPID_ADDRESS)];
	icid = LH_SWAP(0, (uint16_t *)&(sp->ip_secific_sp_info)) & ADC_ICID_MASK;
	flags = (LW_SWAP(0, (uint32_t *)&(sp->ip_secific_sp_info)) & SP_BDI_MASK) ? FDMA_ACQUIRE_BDI_BIT : 0;
	bpid = LH_SWAP(0, &(sp->bpid1)) & SP_BP_PBS_MASK;
	err = fdma_acquire_buffer(icid, flags, bpid, &ext_addr);
	if (err){
		fsl_os_print("*** ERROR: fdma_acquire_buffer FAILED !!! \n");
		return;
	}
	
	fsl_os_print("Simple BU : fdma_acquire_buffer parameters: icid = %d, "
			"flags = %x, bpid = %d, fd_addr = %I64d\n",
			icid, flags, bpid, ext_addr);
	/* zero external address */
	fdma_dma_data(COPY_SIZE*sizeof(uint16_t), icid, (void *)data, ext_addr, FDMA_DMA_DA_WS_TO_SYS_BIT);
	/* read external address */
	fdma_dma_data(COPY_SIZE*sizeof(uint16_t), icid, (void *)read_data, ext_addr, FDMA_DMA_DA_SYS_TO_WS_BIT);
	for (i=0; i<MODIFY_SIZE; i++)
		if (data[i] != read_data[i]){
			err = -EIO;
			fsl_os_print("byte %d: 0x%x != 0x%x\n", i, data[i], read_data[i]);
		}
	if (err)
		fsl_os_print("*** ERROR: fdma_dma_data FAILED !!! \n");
	
	ste_inc_counter(ext_addr, INC_VAL, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
	fdma_dma_data(sizeof(uint32_t), icid, &counter, ext_addr, FDMA_DMA_DA_SYS_TO_WS_BIT);
	if (counter != INC_VAL){
		fsl_os_print("*** ERROR: ste_inc_counter FAILED !!! \n");
		fsl_os_print("*** ERROR: counter =  %d \n", counter);
	}
	else{
		fsl_os_print("*** ste_inc_counter PASSED !!! \n");
		fsl_os_print("*** counter =  %d \n", counter);
	}
	ste_dec_counter(ext_addr, DEC_VAL, STE_MODE_SATURATE | STE_MODE_32_BIT_CNTR_SIZE);
	fdma_dma_data(sizeof(uint32_t), icid, &counter, ext_addr, FDMA_DMA_DA_SYS_TO_WS_BIT);
	if (counter != (INC_VAL-DEC_VAL)){
		fsl_os_print("*** ERROR: ste_dec_counter FAILED !!! \n");
		fsl_os_print("*** ERROR: counter =  %d \n", counter);
	}
	else{
		fsl_os_print("*** ste_dec_counter PASSED !!! \n");
		fsl_os_print("*** counter =  %d \n", counter);
	}
}
