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
@File		fsl_dpni_drv.h

@Description	Data Path Network Interface API
*//***************************************************************************/
#ifndef __FSL_DPNI_DRV_H
#define __FSL_DPNI_DRV_H

#include "common/types.h"
#include "dplib/fsl_dpni.h"
#include "dplib/fsl_ldpaa.h"
#include "dpni_drv.h"


/**************************************************************************//**
@Group		dpni_g DPNI

@Description	Contains initialization APIs and runtime control APIs for DPNI

@{
*//***************************************************************************/

/**************************************************************************//**
@Function	dpni_drv_register_rx_cb

@Description	Attaches a pointer to a call back function to a NI ID.

	The callback function will be called when the NI_ID receives a frame.

@Param[in]	ni_id  - The Network Interface ID
@Param[in]	flow_id - Flow ID, should be between 0 and
		#DPNI_DRV_MAX_NUM_FLOWS
@Param[in]	cb - Callback function for Network Interface specified flow_id
@Param[in]	arg - Argument that will be passed to callback function

@Return	OK on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_register_rx_cb(uint16_t        ni_id,
			uint16_t        flow_id,
			rx_cb_t		    *cb,
			dpni_drv_app_arg_t arg);

/**************************************************************************//**
@Function	dpni_drv_unregister_rx_cb

@Description	Unregisters a NI callback function by replacing it with a
		pointer to a discard callback.
		The discard callback function will be called when the NI_ID
		receives a frame

@Param[in]	ni_id - The Network Interface ID
@Param[in]	flow_id - Flow ID, should be between 0 and
		#DPNI_DRV_MAX_NUM_FLOWS

@Return	OK on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_unregister_rx_cb(uint16_t		ni_id,
                              uint16_t		flow_id);

/**************************************************************************//**
@Function	dpni_get_receive_niid

@Description	Get ID of NI on which the default packet arrived.

@Return	NI_IDs on which the default packet arrived.
*//***************************************************************************/
/* TODO : replace by macros/inline funcs */
int dpni_get_receive_niid(void);

/**************************************************************************//**
@Function	dpni_set_send_niid

@Description	Set the NI ID on which the packet should be sent.

@Param[in]	niid - The Network Interface ID

@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
/* TODO : replace by macros/inline funcs */
int dpni_set_send_niid(uint16_t niid);

/**************************************************************************//**
@Function	dpni_get_send_niid

@Description	Get ID of NI on which the default packet should be sent.

@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
/* TODO : replace by macros/inline funcs */
int dpni_get_send_niid(void);


/**************************************************************************//**
@Function	dpni_drv_get_primary_mac_address

@Description	Get Primary MAC address of NI.

@Param[in]	niid - The Network Interface ID

@Param[out]	mac_addr - stores primary MAC address of the supplied NI.

@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
/* TODO : replace by macros/inline funcs */
int dpni_drv_get_primary_mac_addr(uint16_t niid,
		uint8_t mac_addr[NET_HDR_FLD_ETH_ADDR_SIZE]);


/**************************************************************************//**
@Function	dpni_drv_add_mac_address

@Description	Adds unicast/multicast filter MAC address.

@Param[in]	ni_id - The Network Interface ID

@Param[in]	mac_addr - MAC address to be added to NI unicast/multicast
				filter.
@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_add_mac_addr(uint16_t ni_id,
          		const uint8_t mac_addr[NET_HDR_FLD_ETH_ADDR_SIZE]);

/**************************************************************************//**
@Function	dpni_drv_remove_mac_address

@Description	Removes unicast/multicast filter MAC address.

@Param[in]	ni_id - The Network Interface ID

@Param[in]	mac_addr - MAC address to be removed from NI
				unicast/multicast filter.

@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_remove_mac_addr(uint16_t ni_id,
          		const uint8_t mac_addr[NET_HDR_FLD_ETH_ADDR_SIZE]);

/**************************************************************************//**
@Function	dpni_drv_set_mfl

@Description	Set the maximum received frame length.

@Param[in]	ni_id - The Network Interface ID

@Param[in]	mfl - MFL length.

@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_set_mfl(uint16_t ni_id,
                          const uint16_t mfl);

/**************************************************************************//**
@Function	dpni_drv_get_mfl

@Description	Get the maximum received frame length.

@Param[in]	ni_id - The Network Interface ID

@Param[out]	*mfl - pointer to store MFL length.

@Return	0 on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_get_mfl(uint16_t ni_id,
                          uint16_t *mfl);

/**************************************************************************//**
@Function	dpni_drv_send

@Description	Network Interface send (AIOP enqueue) function.

@Param[in]	ni_id - The Network Interface ID
	Implicit: Queuing Destination Priority (qd_priority) in the TLS.

@Return	OK on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_send(uint16_t ni_id);

/**************************************************************************//**
@Function	dpni_drv_explicit_send

@Description	Network Interface explicit send (AIOP enqueue) function.

@Param[in]	ni_id - The Network Interface ID
	Implicit: Queuing Destination Priority (qd_priority) in the TLS.

@Param[in]	fd - pointer to explicit FD. The assumption is that user
		used fdma function to create an explicit FD as
		fdma_create_frame

@Return	OK on success; error code, otherwise.
		For error posix refer to
		\ref error_g
*//***************************************************************************/
int dpni_drv_explicit_send(uint16_t ni_id, struct ldpaa_fd *fd);

/** @} */ /* end of dpni_g DPNI group */
#endif /* __FSL_DPNI_DRV_H */
