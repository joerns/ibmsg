#include <string.h>
#include "ibmsg.h"

int
ibmsg_send(ibmsg_connection* conn, ibmsg_message* msg, uint64_t* handle)
{
	static uint64_t wrid = 0;

	struct ibv_send_wr wr;
	struct ibv_send_wr* bad_wr;
	struct ibv_sge sge;
	
	memset(&sge, 0, sizeof(sge));
	memset(&wr, 0, sizeof(wr));

	wr.wr_id = wrid++;
	wr.next = NULL;
	sge.addr = (uint64_t)(msg->addr);
	sge.length = msg->length;
	sge.lkey = msg->mr->lkey;
	wr.sg_list = &sge;
	wr.num_sge = 1;
	wr.opcode = IBV_WR_SEND;
	wr.send_flags = IBV_SEND_SIGNALED;

	*handle = wr.wr_id;

	if(ibv_post_send(conn->qp, &wr, &bad_wr))
		return IBMSG_POST_SEND_FAILED;

	return IBMSG_OK;
}
