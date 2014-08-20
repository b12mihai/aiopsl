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

#include "common/types.h"
#include "common/fsl_stdio.h"
#include "common/fsl_string.h"
#include "dpni/drv.h"
#include "fsl_fdma.h"
#include "general.h"
#include "fsl_time.h"
#include "fsl_ip.h"
#include "fsl_cdma.h"
#include "fsl_slab.h"
#include "platform.h"
#include "fsl_io.h"
#include "aiop_common.h"
#include "kernel/fsl_spinlock.h"
#include "dplib/fsl_parser.h"

int app_init(void);
void app_free(void);

#define APP_NI_GET(ARG)   ((uint16_t)((ARG) & 0x0000FFFF))
/**< Get NI from callback argument, it's demo specific macro */
#define APP_FLOW_GET(ARG) (((uint16_t)(((ARG) & 0xFFFF0000) >> 16)
/**< Get flow id from callback argument, it's demo specific macro */


#define MAX_NUM_OF_CORES	16
#define MAX_NUM_OF_TASKS	16

extern int slab_init();
extern int malloc_test();
extern int slab_test();
extern int random_init();
extern int random_test();
extern int memory_test();

extern __SHRAM struct slab *slab_peb;
extern __SHRAM struct slab *slab_ddr;
extern __SHRAM int num_of_cores;
extern __SHRAM int num_of_tasks;
extern __SHRAM uint32_t rnd_seed[MAX_NUM_OF_CORES][MAX_NUM_OF_TASKS];
extern __TASK uint32_t	seed_32bit;
__SHRAM uint8_t dpni_lock; /*lock to change dpni_ctr and dpni_broadcast_flag safely */
__SHRAM uint8_t dpni_ctr; /*counts number of packets received before removing broadcast address*/
__SHRAM uint8_t dpni_broadcast_flag; /*flag if packet with broadcast mac destination received during the test*/
__SHRAM uint8_t packet_number;
__SHRAM uint8_t packet_lock;
__SHRAM uint8_t time_lock;
__SHRAM time_t global_time;

__SHRAM int test_error;
__SHRAM uint8_t test_error_lock;

static void app_process_packet_flow0 (dpni_drv_app_arg_t arg)
{
	int      err = 0, ni, i, j;
	int core_id;
	char *eth_ptr;
	uint64_t time_ms_since_epoch = 0;
	uint32_t time_ms = 0;
	time_t local_time;
	uint8_t local_packet_number;
	int local_test_error = 0;

	lock_spinlock(&packet_lock);
	local_packet_number = packet_number;
	packet_number++;
	unlock_spinlock(&packet_lock);
	core_id = (int)core_get_id();
	err = slab_test();
	if (err) {
		fsl_os_print("ERROR = %d: slab_test failed  in runtime phase \n", err);
		local_test_error |= err;
	} else {
		fsl_os_print("Slab test passed for packet number %d, on core %d\n", local_packet_number, core_id);
	}

	err = malloc_test();

	if (err) {
		fsl_os_print("ERROR = %d: malloc_test failed in runtime phase \n", err);
		local_test_error |= err;
	} else {
		fsl_os_print("Malloc test passed for packet number %d, on core %d\n", local_packet_number, core_id);
	}

	err = memory_test();

	if (err) {
		fsl_os_print("ERROR = %d: memory_test failed in runtime phase \n", err);
		local_test_error |= err;
	} else {
		fsl_os_print("Memory  test passed for packet number %d, on core %d\n", local_packet_number, core_id);
	}

	/*Random Test*/

	err = random_test();
	if (err) {
		fsl_os_print("ERROR = %d: random_test failed in runtime phase \n", err);
		if(seed_32bit != 0)/*TODO remove this condition after moving to sim f118*/
			local_test_error |= err;
	}
	else{
		fsl_os_print("seed %x\n",seed_32bit);
		fsl_os_print("Random test passed for packet number %d, on core %d\n", local_packet_number, core_id);
	}


	/*DPNI test*/
	if(dpni_ctr == 5) /*disable mac after 3 injected packets, one of first 3 packets is broadcast*/
	{
		lock_spinlock(&dpni_lock);
		if(dpni_ctr == 5)
		{
			for(ni = 0; ni < dpni_get_num_of_ni(); ni ++)
			{
				err = dpni_drv_remove_mac_addr((uint16_t)ni,((uint8_t []){0xff,0xff,0xff,0xff,0xff,0xff}));
				if(err != 0) {
					fsl_os_print("dpni_drv_remove_mac_addr error FF:FF:FF:FF:FF:FF for ni %d\n",ni);
					local_test_error |= err;
				}
				else {
					fsl_os_print("dpni_drv_remove_mac_addr FF:FF:FF:FF:FF:FF for ni %d succeeded\n",ni);
				}
			}
			dpni_ctr ++; /*increase counter so the function will be called only once*/
		}
		unlock_spinlock(&dpni_lock);
	}
	else{
		dpni_ctr ++;
	}

	eth_ptr = (char *)PARSER_GET_ETH_POINTER_DEFAULT();

	for(i = 0; i < NET_HDR_FLD_ETH_ADDR_SIZE; i++) /*check if destination mac is broadcast*/
		if(*eth_ptr++ != 0xff)
			break;
	if(i == NET_HDR_FLD_ETH_ADDR_SIZE) /*check if all the destination MAC was broadcast FF:FF:FF:FF:FF:FF*/
	{
		lock_spinlock(&dpni_lock);
		dpni_broadcast_flag = 1;
		unlock_spinlock(&dpni_lock);
	}

	if(dpni_ctr == 10)
		if(dpni_broadcast_flag == 0) {
			fsl_os_print("dpni error - broadcast packets didn't received\n");
			local_test_error |= 0x01;
		}
		else {
			fsl_os_print("dpni success - broadcast packets received during the test\n");
		}



	err = fsl_get_time_ms(&time_ms);
	err |= fsl_get_time_since_epoch_ms(&time_ms_since_epoch);

	if(err){
		fsl_os_print("ERROR = %d: fsl_os_gettimeofday failed  in runtime phase \n", err);
		local_test_error |= err;
	}else {

		fsl_os_print("time ms is: %d milliseconds \n",time_ms);
		fsl_os_print("time since epoch is: %ll milliseconds\n",time_ms_since_epoch);


		local_time = time_ms_since_epoch;
		lock_spinlock(&time_lock);
		if(local_time >= global_time)
		{
			fsl_os_print("time test passed for packet number %d, on core %d\n", local_packet_number, core_id);
			global_time = local_time;
		}
		else
		{
			fsl_os_print("ERROR = %d: time test failed in runtime phase \n", err);
			local_test_error |= 0x01;
		}
		unlock_spinlock(&time_lock);
	}



	local_test_error |= dpni_drv_send(APP_NI_GET(arg));

	lock_spinlock(&test_error_lock);
	test_error |= local_test_error; /*mark if error occured during one of the tests*/
	unlock_spinlock(&test_error_lock);

	if(local_packet_number == 38 ){ /*40 packets (0 - 39) with one broadcast after the broadcast is dissabled */
		if (test_error == 0)
		{
			int not_active_task = 0;
			fsl_os_print("No errors were found during injection of 40 packets\n");
			fsl_os_print("1 packet was sent with removed MAC address\n");
			fsl_os_print("Only 39 (0-38) packets should be received\n");
			fsl_os_print("Test executed with %d cores and %d tasks per core\n", num_of_cores, num_of_tasks);
			fsl_os_print("Cores/Tasks processed packets during the test:\n");
			fsl_os_print("CORE/TASK ");
			for(i = 0; i < num_of_tasks; i++)
				fsl_os_print("  %d ",i);

			for(i = 0; i < num_of_cores; i++)
			{
				if(i < 10)
					fsl_os_print("\nCore  %d:  ", i);
				else
					fsl_os_print("\nCore %d:  ", i);
				for(j = 0; j < num_of_tasks; j++)
				{
					if(rnd_seed[i][j] == 0)
					{
						not_active_task ++;

						if(j < 10)
							fsl_os_print("  X ");
						else
							fsl_os_print("   X ");
					}
					else
					{
						if(j <10)
							fsl_os_print("  V ");
						else
							fsl_os_print("   V ");
					}
				}
			}

			if(not_active_task > 0){
				fsl_os_print("\nWARNING: Not all the tasks were active during the test!\n");

			}

			fsl_os_print("\nARENA Test Finished SUCCESSFULLY\n");
		}
		else {
			fsl_os_print("ARENA Test Finished with ERRORS\n");
		}
	}
}

int app_init(void)
{
	int        err  = 0;
	uint32_t   ni   = 0;
	dma_addr_t buff = 0;


	fsl_os_print("Running AIOP arena app_init()\n");

	for (ni = 0; ni < dpni_get_num_of_ni(); ni++)
	{
		/* Every ni will have 1 flow */
		uint32_t flow_id = 0;

		err = dpni_drv_add_mac_addr((uint16_t)ni, ((uint8_t []){0x02, 0x00 ,0xc0 ,0x0a8 ,0x0b ,0xfe }));

		if (err){
			fsl_os_print("dpni_drv_add_mac_addr failed %d\n", err);
		}
		else{
			fsl_os_print("dpni_drv_add_mac_addr succeeded in boot\n");
			fsl_os_print("MAC 02:00:C0:A8:0B:FE added for ni %d\n",ni);

		}
		err = dpni_drv_register_rx_cb((uint16_t)ni/*ni_id*/,
		                              (uint16_t)flow_id/*flow_id*/,
		                              app_process_packet_flow0, /* callback for flow_id*/
		                              (ni | (flow_id << 16)) /*arg, nic number*/);

		if (err)
			return err;
	}

	err = slab_init();
	if (err) {
		fsl_os_print("ERROR = %d: slab_init failed  in init phase()\n", err);
		test_error |= err;
	}
	else
		fsl_os_print("slab_init  succeeded  in init phase()\n", err);
	err = memory_test();

	if (err) {
		fsl_os_print("ERROR = %d: memory_test failed in init phase()\n", err);
		test_error |= err;
	}
	else
		fsl_os_print("memory_test succeeded  in init phase()\n", err);

	err = malloc_test();
	if (err) {
		fsl_os_print("ERROR = %d: malloc_test failed in init phase()\n", err);
		test_error |= err;
	}
	else
		fsl_os_print("malloc_test succeeded  in init phase()\n", err);
	err = random_init();
	if (err) {
		fsl_os_print("ERROR = %d: random_test failed in init phase()\n", err);
		test_error |= err;
	} else {
		fsl_os_print("random_test passed in init phase()\n");
	}

	fsl_os_print("To start test inject packets: \"arena_test_40.pcap\"\n");
	return 0;
}

void app_free(void)
{
	int err = 0;
	/* TODO - complete!*/
	err = slab_free(&slab_ddr);
	err = slab_free(&slab_peb);
}
