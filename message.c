#include <stdlib.h>
#include "ibmsg.h"

int
ibmsg_alloc(ibmsg_connection* conn, ibmsg_message* msg, size_t length)
{
	msg->addr = malloc(length);
	if(msg->addr == NULL)
		return IBMSG_ALLOC_FAILED;
	msg->length = length;
	msg->mr = ibv_reg_mr(conn->pd, msg->addr, msg->length,
	                     IBV_ACCESS_LOCAL_WRITE |
	                     IBV_ACCESS_REMOTE_WRITE |
	                     IBV_ACCESS_REMOTE_ATOMIC);
	if(msg->mr == NULL)
		return IBMSG_REG_MR_FAILED;
	return IBMSG_OK;
}


int
ibmsg_free(ibmsg_message* msg)
{
	if(ibv_dereg_mr(msg->mr))
		return IBMSG_DEREG_MR_FAILED;
	free(msg->addr);
	msg->length = 0;
	return IBMSG_OK;
}
