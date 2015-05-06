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
#include "sys.h"
#include "inc/fsl_gen.h"
#include "fsl_errors.h"
#include "fsl_sys.h"
#include "fsl_dprc.h"
#include "fsl_dpci.h"
#include "fsl_dpci_event.h"
#include "fsl_dpci_drv.h"
#include "fsl_dpci_mng.h"
#include "fsl_mc_init.h"
#include "fsl_dbg.h"
#include "cmdif_client.h"
#include "cmdif_srv.h"
#include "fsl_fdma.h"
#include "fsl_cdma.h"
#include "fsl_icontext.h"
#include "fsl_spinlock.h"
#include "fsl_malloc.h"

/*************************************************************************/
#define MC_DPCI_NUM 		1
#define DPCI_DYNAMIC_MAX	32
#define DPCI_LOW_PR		1

#define CMDIF_Q_OPTIONS (DPCI_QUEUE_OPT_USER_CTX | DPCI_QUEUE_OPT_DEST)

#define CMDIF_FQD_CTX_GET \
	(((struct additional_dequeue_context *)HWC_ADC_ADDRESS)->fqd_ctx)

#define CMDIF_RX_CTX_GET \
	(LLLDW_SWAP((uint32_t)&CMDIF_FQD_CTX_GET, 0))

#define AMQ_BDI_SET(_offset, _width, _type, _arg) \
	(amq_bdi |= u32_enc((_offset), (_width), (_arg)))

#define AMQ_BDI_GET(_offset, _width, _type, _arg) \
	(*(_arg) = (_type)u32_dec(dt->ic[ind], (_offset), (_width)))

#define USER_CTX_SET(_offset, _width, _type, _arg) \
	(queue_cfg.user_ctx |= u64_enc((_offset), (_width), (_arg)))

#define USER_CTX_GET(_offset, _width, _type, _arg) \
	(*(_arg) = (_type)u64_dec(rx_ctx, (_offset), (_width)))

#define CMDIF_DPCI_FQID(_OP, DPCI, FQID) \
do { \
	_OP(32,		32,	uint32_t,	DPCI); \
	_OP(0,		32,	uint32_t,	FQID); \
} while (0)


#define CMDIF_ICID_AMQ_BDI(_OP, ICID, AMQ_BDI) \
do { \
	_OP(16,		16,	uint16_t,	ICID); \
	_OP(0,		16,	uint16_t,	AMQ_BDI); \
} while (0)

#define MEM_SET(_ADDR, _SIZE, _VAL) \
	do { \
		for (i = 0; i < (_SIZE); i++) \
			((uint8_t *)_ADDR)[i] = _VAL; \
	} while (0)

#define DPCI_DT_LOCK_R_TAKE \
	do { \
		cdma_mutex_lock_take((uint64_t)(dt), CDMA_MUTEX_READ_LOCK); \
	} while(0)

#define DPCI_DT_LOCK_W_TAKE \
	do { \
		cdma_mutex_lock_take((uint64_t)(dt), CDMA_MUTEX_WRITE_LOCK); \
	} while(0)

#define DPCI_DT_LOCK_RELEASE \
	do { \
		cdma_mutex_lock_release((uint64_t)(dt)); \
	} while(0)

int dpci_amq_bdi_init(uint32_t dpci_id);
int dpci_rx_ctx_init(uint32_t dpci_id, uint32_t id);
int dpci_drv_init();
void dpci_drv_free();


extern struct aiop_init_info g_init_data;


__COLD_CODE static void dpci_tbl_dump()
{
	int i;
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);

	fsl_os_print("----------DPCI table----------\n");
	for (i = 0; i < dt->count; i++) {
		fsl_os_print("ID = 0x%x\t PEER ID = 0x%x\t IC = 0x%x\t\n",
		             dt->dpci_id[i], dt->dpci_id_peer[i], dt->ic[i]);
	}
}

int dpci_mng_find(uint32_t dpci_id, uint32_t *ic)
{
	int i;
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);
	ASSERT_COND(dpci_id != DPCI_FQID_NOT_VALID);
	
	for (i = 0; i < dt->count; i++) {
		if (dt->dpci_id[i] == dpci_id) {
			if (ic != NULL)
				*ic = dt->ic[i];
			return i;
		}
	}

	return -ENOENT;
}

int dpci_mng_peer_find(uint32_t dpci_id, uint32_t *ic)
{
	int i;
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);
	ASSERT_COND(dpci_id != DPCI_FQID_NOT_VALID);
	
	for (i = 0; i < dt->count; i++) {
		if (dt->dpci_id_peer[i] == dpci_id) {
			if (ic != NULL)
				*ic = dt->ic[i];
			return i;
		}
	}

	return -ENOENT;
}

static int dpci_entry_get()
{
	int i;
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);

	for (i = 0; i < dt->count; i++)
		if (dt->dpci_id[i] == DPCI_FQID_NOT_VALID)
			return i;

	if (dt->count < dt->max) {
		i = dt->count;
		atomic_incr32(&dt->count, 1);
		return i;
	}
	
	return -ENOENT;
}

static void dpci_entry_delete(int ind)
{
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);

	dt->ic[ind] = DPCI_FQID_NOT_VALID;
	dt->dpci_id[ind] = DPCI_FQID_NOT_VALID;
	dt->dpci_id_peer[ind] = DPCI_FQID_NOT_VALID;
	atomic_decr32(&dt->count, 1);
}

__COLD_CODE static int dpci_get_peer_id(uint32_t dpci_id, uint32_t *dpci_id_peer)
{
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	int err = 0;
	uint16_t token;
	struct dpci_peer_attr peer_attr;
	uint8_t i;

	ASSERT_COND(dprc);

	/* memset */
	MEM_SET(&peer_attr, sizeof(peer_attr), 0);

	err = dpci_open(&dprc->io, (int)dpci_id, &token);
	if (err) {
		pr_err("Failed dpci_open dpci id = %d\n", dpci_id);
		return err;
	}
	err = dpci_get_peer_attributes(&dprc->io, token, &peer_attr);
	if (err) {
		pr_err("Failed to get peer_id dpci id = %d\n", dpci_id);
		dpci_close(&dprc->io, token);
		return err;
	}

	if (peer_attr.peer_id == -1) {
		err = dpci_close(&dprc->io, token);
		return -EINVAL;
	}

	*dpci_id_peer = (uint32_t)peer_attr.peer_id;

	err = dpci_close(&dprc->io, token);
	return err;
}

static inline void amq_bits_update(uint32_t id)
{
	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
		sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);
	uint32_t amq_bdi = 0;
	uint16_t amq_bdi_temp = 0;
	uint16_t pl_icid = PL_ICID_GET;

	ASSERT_COND(dt);
	
	ADD_AMQ_FLAGS(amq_bdi_temp, pl_icid);
	if (BDI_GET != 0)
		amq_bdi_temp |= CMDIF_BDI_BIT;

	CMDIF_ICID_AMQ_BDI(AMQ_BDI_SET, ICID_GET(pl_icid), amq_bdi_temp);

	/*
	 * TODO
	 * NOTE : only dpci_peer_id can be updated but not dpci_id.
	 * Maybe it should not update peer id at all ?? 
	 * It should be updated only in dpci_drv_added() !!!
	 * TODO
	 * Check if amq bits updated and update only if they are 0xffffffff
	 */
	//err = dpci_get_peer_id(dt->dpci_id[id], &(dt->dpci_id_peer[id]));
	//ASSERT_COND(!err);

	/* Must be written last */
	dt->ic[id] = amq_bdi;
	/* Don't update AIOP to AIOP DPCI 2 entries because it 
	 * shouldn't change anyway */
	dpci_tbl_dump();
}


/* To be called upon connected event, assign even */
__COLD_CODE int dpci_amq_bdi_init(uint32_t dpci_id)
{
	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
		sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);
	int ind = -1;
	uint32_t amq_bdi = 0;
	uint32_t dpci_id_peer = DPCI_FQID_NOT_VALID;
	int err = 0;

	ASSERT_COND(dt);

	CMDIF_ICID_AMQ_BDI(AMQ_BDI_SET, ICONTEXT_INVALID, ICONTEXT_INVALID);

	ind = dpci_mng_find(dpci_id, NULL);
	if (ind >= 0) {
		/* Updated DPCI peer if possible */
		ASSERT_COND(dt->dpci_id[ind] == dpci_id);
		err = dpci_get_peer_id(dpci_id, &dpci_id_peer);
		if (!err)
			dt->dpci_id_peer[ind] = dpci_id_peer;
		else
			dt->dpci_id_peer[ind] = DPCI_FQID_NOT_VALID;

		dt->ic[ind] = amq_bdi;
	} else {		
		ind = dpci_entry_get();
		if (ind >= 0) {
			/* Adding new dpci_id */
			dt->ic[ind] = amq_bdi;
			dt->dpci_id[ind] = dpci_id;
		} else {
			pr_err("Not enough entries\n");
			return -ENOMEM;
		}

		/* Updated DPCI peer if possible */
		err = dpci_get_peer_id(dpci_id, &dpci_id_peer);
		if ((dpci_id_peer != DPCI_FQID_NOT_VALID) && (!err)) {
			if (ind >= 0)
				dt->dpci_id_peer[ind] = dpci_id_peer;
			else
				dt->dpci_id_peer[ind] = DPCI_FQID_NOT_VALID;
			/* AIOP DPCI to AIOP DPCI case 2 entries must be 
			 * updated 1->2 and 2->1 */
			err = dpci_mng_find(dpci_id_peer, NULL);
			if (err >= 0) {
				/* If here then 2 AIOP DPCIs are connected */
				dt->dpci_id_peer[err] = dpci_id; 
				dt->ic[err] = amq_bdi;
				pr_debug("AIOP DPCI %d to AIOP DPCI %d\n", 
				         dpci_id, dpci_id_peer);
			}
		}
	}

	return ind;
}

__COLD_CODE int dpci_rx_ctx_init(uint32_t dpci_id, uint32_t id)
{
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	int err = 0;
	uint16_t token;
	struct dpci_rx_queue_cfg queue_cfg;
	struct dpci_rx_queue_attr rx_attr;
	struct dpci_attr attr;
	uint8_t i;

	ASSERT_COND(dprc);

	/* memset */
	MEM_SET(&queue_cfg, sizeof(queue_cfg), 0);
	MEM_SET(&rx_attr, sizeof(rx_attr), 0);
	MEM_SET(&attr, sizeof(attr), 0);

	err = dpci_open(&dprc->io, (int)dpci_id, &token);
	if (err) {
		pr_err("Failed dpci_open dpci id = %d\n", dpci_id);
		return err;
	}
	err = dpci_get_attributes(&dprc->io, token, &attr);
	if (err) {
		dpci_close(&dprc->io, token);
		return err;
	}

	queue_cfg.dest_cfg.dest_type = DPCI_DEST_NONE;
	queue_cfg.options = CMDIF_Q_OPTIONS;
	for (i = 0; i < attr.num_of_priorities; i++) {
		queue_cfg.dest_cfg.priority = DPCI_LOW_PR - i;
		queue_cfg.user_ctx = 0;
		CMDIF_DPCI_FQID(USER_CTX_SET, id, DPCI_FQID_NOT_VALID);
		err = dpci_set_rx_queue(&dprc->io, token, i,
		                         &queue_cfg);
		ASSERT_COND(!err);
	}

	err = dpci_close(&dprc->io, token);
	return err;
}

__COLD_CODE static int rx_ctx_set(uint32_t id)
{
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	int err = 0;
	uint16_t token;
	struct dpci_rx_queue_cfg queue_cfg;
	struct dpci_attr attr;
	struct dpci_tx_queue_attr tx_attr;
	struct dpci_rx_queue_attr rx_attr;
	uint8_t i;
	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
		sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dprc && dt);

	/* memset */
	MEM_SET(&queue_cfg, sizeof(queue_cfg), 0);
	MEM_SET(&rx_attr, sizeof(rx_attr), 0);
	MEM_SET(&tx_attr, sizeof(tx_attr), 0);
	MEM_SET(&attr, sizeof(attr), 0);

	err = dpci_open(&dprc->io, (int)dt->dpci_id[id], &token);
	if (err)
		return err;

	err = dpci_get_attributes(&dprc->io, token, &attr);
	if (err) {
		dpci_close(&dprc->io, token);
		return err;
	}

	queue_cfg.dest_cfg.dest_type = DPCI_DEST_NONE;
	queue_cfg.options = CMDIF_Q_OPTIONS;
	for (i = 0; i < attr.num_of_priorities; i++) {

		err = dpci_get_tx_queue(&dprc->io, token, i, &tx_attr);
		ASSERT_COND(!err);
		ASSERT_COND(tx_attr.fqid != DPCI_FQID_NOT_VALID);
		queue_cfg.dest_cfg.priority = DPCI_LOW_PR - i;
		queue_cfg.user_ctx = 0;
		CMDIF_DPCI_FQID(USER_CTX_SET, id, tx_attr.fqid);
		err = dpci_set_rx_queue(&dprc->io, token, i,
		                         &queue_cfg);
		ASSERT_COND(!err);
	}

	err = dpci_close(&dprc->io, token);

	return err;
}

__COLD_CODE static int tx_get(uint32_t dpci_id, uint32_t *tx)
{

	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	int err = 0;
	uint16_t token;
	struct dpci_attr attr;
	struct dpci_tx_queue_attr tx_attr;
	int i;
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(tx);
	ASSERT_COND(dt);
	ASSERT_COND(dprc);

	/* memset */
	MEM_SET(&tx_attr, sizeof(tx_attr), 0);
	MEM_SET(&attr, sizeof(attr), 0);

	/* dpci id may belong to peer */
	i = dpci_mng_find(dpci_id, NULL);
	if (i < 0) {
		pr_err("Not found DPCI id or it's peer %d\n", dpci_id);
		return -ENAVAIL;
	}

	err = dpci_open(&dprc->io, (int)dt->dpci_id[i], &token);
	if (err)
		return err;

	err = dpci_get_attributes(&dprc->io, token, &attr);
	if (err) {
		dpci_close(&dprc->io, token);
		return err;
	}

	for (i = 0; i < attr.num_of_priorities; i++) {
		err = dpci_get_tx_queue(&dprc->io, token, (uint8_t)i, &tx_attr);
		ASSERT_COND(!err);
		ASSERT_COND(tx_attr.fqid != DPCI_FQID_NOT_VALID);
		if (tx != NULL)
			tx[i] = tx_attr.fqid;
	}

	err = dpci_close(&dprc->io, token);

	return err;
}


/*************************************************************************/

/*
 * New DPCI was added or the state of the DPCI has changed
 * The dpci_id must belong to AIOP side
 */
__COLD_CODE int dpci_event_assign(uint32_t dpci_id)
{
	int err = 0;
	int ind = 0;
	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
			sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);
	
	DPCI_DT_LOCK_W_TAKE;

	ind = dpci_amq_bdi_init(dpci_id);
	if (ind >= 0) {
		err = dpci_rx_ctx_init(dpci_id, (uint32_t)ind);
		/* Set rx ctx if peer is already connected */
		if (dt->dpci_id_peer[ind] != DPCI_FQID_NOT_VALID)
			err = rx_ctx_set((uint32_t)ind);
		/* 
		 * TODO what if not connected DPCI is added ? Can it be ?
		 */
	}

	DPCI_DT_LOCK_RELEASE;

	if (dt->mc_dpci_id != dpci_id) {
		/* TODO call EVM here 
		 * TODO keep mc_dpci_id internally if this memory is reused */
	}
	
	dpci_tbl_dump();
	
	return err;
}

/*
 * The DPCI was removed from AIOP container
 * The dpci_id must belong to AIOP side
 */
__COLD_CODE int dpci_event_unassign(uint32_t dpci_id)
{

	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
		sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);
	int ind = -1;
	int err = 0;

	ASSERT_COND(dt);

	DPCI_DT_LOCK_W_TAKE;

	ind = dpci_mng_find(dpci_id, NULL);
	if (ind >= 0) {
		ASSERT_COND(dt->dpci_id[ind] == dpci_id);
		dpci_entry_delete(ind);
		err = 0;
	} else {
		err = -ENOENT;
	}

	DPCI_DT_LOCK_RELEASE;
	
	dpci_tbl_dump();

	return err;
}


/*
 * The DPCI user context and AMQ bits are updated
 * This function is to be called only inside the open command and before
 * the AMQ bits had been changed to AIOP AMQ bits
 */
__COLD_CODE int dpci_event_update(uint32_t ind)
{
	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
		sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	/* Read lock because many can update same entry with the same values
	 * New values can be set only inside dpci_drv_removed() dpci_drv_added()
	 * */
	
	/*
	 * TODO
	 * Is it possible that DPCI will be removed in the middle of the task ?
	 * If yes than we need read lock on dpci_mng_find() + dpci_drv_icid_get()
	 * NOTE : only dpci_peer_id can be updated but not dpci_id.
	 * Maybe it should not update peer id at all ?? 
	 * It should be updated only in dpci_drv_added() !!!
	 * Event connected should be before link up, once there is command the 
	 * index can't be changed to other dpci_id and dpci_peer_id. 
	 * If it changes then there should be removed/disconnected event, 
	 */

	DPCI_DT_LOCK_R_TAKE;

	amq_bits_update(ind);
	/* TODO err = rx_ctx_set(ind);
	 * rx_ctx_set(ind) moved to added event
	 * Need to check if this is the right thing
	 * Can't get tx fqid from GPP inside command, maybe for AIOP it is authorized
	 * with a different ICID
	 */

	DPCI_DT_LOCK_RELEASE;
	return 0;
}


__HOT_CODE void dpci_mng_user_ctx_get(uint32_t *id, uint32_t *fqid)
{
	uint64_t rx_ctx = CMDIF_RX_CTX_GET;
	uint32_t _id;
	uint32_t _fqid;

	CMDIF_DPCI_FQID(USER_CTX_GET, (&_id), (&_fqid));

	if (id)
		*id = _id;
	if (fqid)
		*fqid = _fqid;
}

__HOT_CODE void dpci_mng_icid_get(uint32_t ind, uint16_t *icid, uint16_t *amq_bdi)
{
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	ASSERT_COND(dt);

	DPCI_DT_LOCK_R_TAKE;

	CMDIF_ICID_AMQ_BDI(AMQ_BDI_GET, icid, amq_bdi);

	DPCI_DT_LOCK_RELEASE;
}

__COLD_CODE int dpci_mng_tx_get(uint32_t dpci_id, uint32_t *tx)
{
	int err;
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	DPCI_DT_LOCK_R_TAKE;

	err = tx_get(dpci_id, tx);

	
	DPCI_DT_LOCK_RELEASE;

	return err;
}

__COLD_CODE int dpci_drv_enable(uint32_t dpci_id)
{
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	int err = 0;
	uint16_t token = 0xffff;
	
	ASSERT_COND(dprc && dt);
	
	err = dpci_open(&dprc->io, (int)dpci_id, &token);
	if (err) {
		dpci_close(&dprc->io, token);
		return err;
	}
	
	err = dpci_enable(&dprc->io, token);
	if (err) {
		pr_err("DPCI enable failed\n");
		dpci_close(&dprc->io, token);
		return err;
	}
	
	/*
	 * TODO maybe need to keep enable bit in dt table ???
	 */
	dpci_close(&dprc->io, token);
	return err;
}

__COLD_CODE int dpci_drv_disable(uint32_t dpci_id)
{
	struct dpci_mng_tbl *dt = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	int err = 0;
	uint16_t token = 0xffff;
	
	ASSERT_COND(dprc && dt);
	
	err = dpci_open(&dprc->io, (int)dpci_id, &token);
	if (err) {
		dpci_close(&dprc->io, token);
		return err;
	}
	
	err = dpci_disable(&dprc->io, token);
	if (err) {
		pr_err("DPCI disable failed\n");
		dpci_close(&dprc->io, token);
		return err;
	}
	
	/*
	 * TODO maybe need to keep enable bit in dt table ???
	 * TODO draining !!!!!
	 */
	dpci_close(&dprc->io, token);
	return err;
}

__COLD_CODE static int dpci_tbl_create(struct dpci_mng_tbl **_dpci_tbl, int dpci_count)
{
	uint32_t size = 0;
	struct   dpci_mng_tbl *dpci_tbl = NULL;
	int      err = 0;

	size = sizeof(struct dpci_mng_tbl);
	dpci_tbl = fsl_malloc(size, 1);
	*_dpci_tbl = dpci_tbl;
	if (dpci_tbl == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl, 0, size);

	size = sizeof(uint32_t) * dpci_count;
	dpci_tbl->dpci_id = fsl_malloc(size,1);
	if (dpci_tbl->dpci_id == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->dpci_id, 0xff, size);

	size = sizeof(uint32_t) * dpci_count;
	dpci_tbl->dpci_id_peer = fsl_malloc(size,1);
	if (dpci_tbl->dpci_id_peer == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->dpci_id_peer, 0xff, size);

	size = sizeof(uint32_t) * dpci_count;
	dpci_tbl->ic = fsl_malloc(size,1);
	if (dpci_tbl->ic == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->ic, 0xff, size);

	size = sizeof(uint16_t) * dpci_count;
	dpci_tbl->token = fsl_malloc(size,1);
	if (dpci_tbl->token == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->token, 0xff, size);
	
	size = sizeof(uint8_t) * dpci_count;
	dpci_tbl->state = fsl_malloc(size,1);
	if (dpci_tbl->state == NULL) {
		pr_err("No memory for %d DPCIs\n", dpci_count);
		return -ENOMEM;
	}
	memset(dpci_tbl->state, 0, size);
	
	dpci_tbl->count = 0;
	dpci_tbl->max = dpci_count;

	err = sys_add_handle(dpci_tbl,
			FSL_OS_MOD_DPCI_TBL,
			1,
			0);
	if (err != 0) {
		pr_err("FSL_OS_MOD_DPCI_TBL sys_add_handle failed\n");
		return err;
	}

	return err;
}

__COLD_CODE static int dpci_for_mc_add(struct mc_dprc *dprc)
{
	struct dpci_cfg dpci_cfg;
	uint16_t dpci = 0;
	struct dpci_rx_queue_cfg queue_cfg;
	struct dprc_endpoint endpoint1 ;
	struct dprc_endpoint endpoint2;
	struct dpci_attr attr;
	uint8_t p = 0;
	int     err = 0;
	int     link_up = 0;
	struct dpci_mng_tbl *dt = (struct dpci_mng_tbl *)\
			sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	memset(&queue_cfg, 0, sizeof(struct dpci_rx_queue_cfg));
	memset(&attr, 0, sizeof(attr));

	dpci_cfg.num_of_priorities = 2;

	err |= dpci_create(&dprc->io, &dpci_cfg, &dpci);

	/* Get attributes just for dpci id fqids are not there yet */
	err |= dpci_get_attributes(&dprc->io, dpci, &attr);

	/* Connect to dpci that belongs to MC */
	dt->mc_dpci_id = g_init_data.sl_info.mc_dpci_id;
	pr_debug("MC dpci ID[%d] \n", dt->mc_dpci_id);
	
	memset(&endpoint1, 0, sizeof(struct dprc_endpoint));
	memset(&endpoint2, 0, sizeof(struct dprc_endpoint));
	endpoint1.id = (int)g_init_data.sl_info.mc_dpci_id;
	endpoint1.interface_id = 0;
	strcpy(endpoint1.type, "dpci");

	endpoint2.id = attr.id;
	endpoint2.interface_id = 0;
	strcpy(endpoint2.type, "dpci");

	err = dprc_connect(&dprc->io, dprc->token, &endpoint1, &endpoint2);
	if (err) {
		pr_err("dprc_connect failed\n");
	}
	err = dpci_close(&dprc->io, dpci);
	ASSERT_COND(!err);
	
	err = dpci_event_assign((uint32_t)attr.id);
	ASSERT_COND(!err);

	err = dpci_drv_enable((uint32_t)attr.id);
	return err;
}

__COLD_CODE int dpci_drv_init()
{
	struct dprc_obj_desc dev_desc;
	struct mc_dprc *dprc = sys_get_unique_handle(FSL_OS_MOD_AIOP_RC);
	struct dpci_mng_tbl *dpci_tbl = NULL;
	int dev_count  = 0;
	int dpci_count = 0;
	int err        = 0;
	int i          = 0;

	if (dprc == NULL) {
		pr_err("No AIOP root container \n");
		return -ENODEV;
	}

	if ((err = dprc_get_obj_count(&dprc->io, dprc->token, &dev_count)) != 0) {
		pr_err("Failed to get device count for RC auth_d = %d\n",
		       dprc->token);
		return err;
	}

	/* First count how many DPCI objects we have */
	for (i = 0; i < dev_count; i++) {
		dprc_get_obj(&dprc->io, dprc->token, i, &dev_desc);
		if (strcmp(dev_desc.type, "dpci") == 0) {
			dpci_count++;
		}
	}

	err = dpci_tbl_create(&dpci_tbl,
	                      dpci_count + MC_DPCI_NUM + DPCI_DYNAMIC_MAX);
	if (err != 0) {
		pr_err("Failed dpci_tbl_create() \n");
		return err;
	}

	err = dpci_for_mc_add(dprc);
	if (err) {
		pr_err("Failed to create and link AIOP<->MC DPCI \n");
	}
	
	dpci_tbl_dump();

	return err;
}

__COLD_CODE void dpci_drv_free()
{
	void *dpci_tbl = sys_get_unique_handle(FSL_OS_MOD_DPCI_TBL);

	sys_remove_handle(FSL_OS_MOD_DPCI_TBL, 0);

	if (dpci_tbl != NULL)
		fsl_free(dpci_tbl);
	/*
	 * TODO free all entries inside dpci_tbl
	 */
}
