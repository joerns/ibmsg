#include "ibmsg.h"

int
ibmsg_recv(ibmsg_connection* conn, ibmsg_message* msg, uint64_t* handle)
{
	static uint64_t wrid = 0;

	struct ibv_recv_wr wr;
	struct ibv_recv_wr* bad_wr;
	struct ibv_sge sge;

	wr.wr_id = wrid++;
	wr.next = 0;
	sge.addr = (uint64_t)msg->addr;
	sge.length = msg->length;
	sge.lkey = msg->mr->lkey;
	wr.sg_list = &sge;
	wr.num_sge = 1;

	*handle = wr.wr_id;

	if(ibv_post_recv(conn->qp, &wr, &bad_wr))
		return IBMSG_POST_RECV_FAILED;

	return IBMSG_OK;
}
