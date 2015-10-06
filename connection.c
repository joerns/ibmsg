#include "ibmsg.h"


static int
reset_to_init(ibmsg_connection* conn)
{
	struct ibv_qp_attr attr;
	enum ibv_qp_attr_mask mask;

	attr.qp_state = IBV_QPS_INIT;
	attr.pkey_index = 0;
	attr.port_num = 1 + conn->portid; /* TODO port numbering from 1 to n? */
	attr.qp_access_flags = IBV_ACCESS_LOCAL_WRITE |
	                       IBV_ACCESS_REMOTE_WRITE |
	                       IBV_ACCESS_REMOTE_ATOMIC;

	mask = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

	if(ibv_modify_qp(conn->qp, &attr, mask))
		return 1;
	return 0;
}


static int
init_to_rtr(ibmsg_connection* conn, ibmsg_address addr)
{
	struct ibv_qp_attr attr;
	enum ibv_qp_attr_mask mask;

	struct ibv_ah_attr ah;
	ah.grh.dgid = addr.gid; 
	ah.grh.flow_label = 0;
	ah.grh.sgid_index = 0;
	ah.grh.hop_limit = IBMSG_HOP_LIMIT;
	ah.grh.traffic_class = 0;
	ah.dlid = addr.lid;
	ah.sl = 0;
	ah.src_path_bits = 0;
	ah.static_rate = 0;
	ah.is_global = addr.is_global;
	ah.port_num = 1 + conn->portid;

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_512;
	attr.ah_attr = ah;
	attr.dest_qp_num = 0;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = IBMSG_MAX_DEST_RD_ATOMIC;
	attr.min_rnr_timer = 12;

	mask = IBV_QP_STATE | IBV_QP_PATH_MTU | IBV_QP_AV | IBV_QP_DEST_QPN
	       | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

	if(ibv_modify_qp(conn->qp, &attr, mask))
		return 1;
	return 0;
}



static int
rtr_to_rts(ibmsg_connection* conn)
{
	struct ibv_qp_attr attr;
	enum ibv_qp_attr_mask mask;

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 14;
	attr.retry_cnt = 7;
	attr.rnr_retry = 7;
	attr.sq_psn = 0;
	attr.max_rd_atomic = IBMSG_MAX_RD_ATOMIC;

	mask = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY
	       | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

	if(ibv_modify_qp(conn->qp, &attr, mask))
		return 1;
	return 0;
}


static int
init_rdma(ibmsg_connection* conn)
{
	if(rdma_create_id(conn->event_channel, &conn->cmid, (void*)conn, RDMA_PS_TCP))
		return 1;
	return 0;
}


static int
destroy_rdma(ibmsg_connection* conn)
{
	if(rdma_destroy_id(conn->cmid))
		return 1;
	conn->cmid = NULL;
	return 0;
}


int
ibmsg_connect(ibmsg_connection* conn, ibmsg_address addr)
{
	if(init_rdma(conn))
		return IBMSG_RDMA_INIT_FAILED;

	if(reset_to_init(conn))
		return IBMSG_RESET_TO_INIT_FAILED;

	if(init_to_rtr(conn, addr))
		return IBMSG_INIT_TO_RTR_FAILED;

	if(rtr_to_rts(conn))
		return IBMSG_RTR_TO_RTS_FAILED;

	return IBMSG_OK;
}


int
ibmsg_disconnect(ibmsg_connection* conn)
{
	if(destroy_rdma(conn))
		return IBMSG_RDMA_DESTROY_FAILED;

	return IBMSG_OK;
}
