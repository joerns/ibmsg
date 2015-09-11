#include <string.h>
#include <stdlib.h>
#include "ibmsg.h"


static struct ibv_device*
find_device_by_name(const char* name)
{
	struct ibv_device** devices;
	int i, num_devices;

	devices = ibv_get_device_list(&num_devices);

	for(i=0; i<num_devices; i++)
	{
		if(strcmp(devices[i]->name, name) == 0)
		{
			return devices[i];
		}
	}

	return NULL;
}


static int
open_device(ibmsg_connection* conn, const char* name, uint8_t portid)
{
	struct ibv_device* dev = find_device_by_name(name);
	if(dev == NULL)
		return IBMSG_DEVICE_NOT_FOUND;

	conn->context = ibv_open_device(dev);
	if(conn->context == NULL)
		return IBMSG_COULD_NOT_OPEN;

	return IBMSG_OK;
}


static int
close_device(ibmsg_connection* conn)
{
	if(ibv_close_device(conn->context))
		return IBMSG_COULD_NOT_CLOSE;

	return IBMSG_OK;
}


static int
init_pd(ibmsg_connection* conn)
{
	conn->pd = ibv_alloc_pd(conn->context);
	if(conn->pd == NULL)
		return IBMSG_PD_FAILED;
	return IBMSG_OK;
}


static int
destroy_pd(ibmsg_connection* conn)
{
	if(ibv_dealloc_pd(conn->pd))
		return IBMSG_PD_FAILED;
	return IBMSG_OK;
}


static int
init_cq(ibmsg_connection* conn)
{
	conn->rx_cq = ibv_create_cq(conn->context, IBMSG_MIN_CQE, NULL, NULL, 0);
	if(conn->rx_cq == NULL)
		return IBMSG_CQ_FAILED;

	conn->tx_cq = ibv_create_cq(conn->context, IBMSG_MIN_CQE, NULL, NULL, 0);
	if(conn->tx_cq == NULL)
		return IBMSG_CQ_FAILED;

	return IBMSG_OK;
}


static int
destroy_cq(ibmsg_connection* conn)
{
	int result[2];
	result[0] = ibv_destroy_cq(conn->rx_cq);
	result[1] = ibv_destroy_cq(conn->tx_cq);
	if(result[0] | result[1])
		return IBMSG_CQ_FAILED;
	return IBMSG_OK;
}


static int
init_qp(ibmsg_connection* conn)
{
	struct ibv_qp_init_attr attr;
	attr.send_cq = conn->tx_cq;
	attr.recv_cq = conn->rx_cq;
	attr.srq = NULL;
	attr.cap.max_send_wr = IBMSG_MAX_WR;
	attr.cap.max_recv_wr = IBMSG_MAX_WR;
	attr.cap.max_send_sge = IBMSG_MAX_SGE;
	attr.cap.max_recv_sge = IBMSG_MAX_SGE;
	attr.cap.max_inline_data = IBMSG_MAX_INLINE;
	attr.qp_type = IBV_QPT_RC;
	attr.sq_sig_all = 1;

	conn->qp = ibv_create_qp(conn->pd, &attr);
	if(conn->qp == NULL)
		return IBMSG_QP_FAILED;
	return IBMSG_OK;
}


static int
destroy_qp(ibmsg_connection* conn)
{
	if(ibv_destroy_qp(conn->qp))
		return IBMSG_QP_FAILED;
	return IBMSG_OK;
}


int
ibmsg_open(ibmsg_connection* conn, const char* name, uint8_t portid)
{
	int result;
	conn->portid = portid;

	result = open_device(conn, name, portid);
	if(result != 0)
		return result;

	result = init_cq(conn);
	if(result != 0)
		return result;

	result = init_pd(conn);
	if(result != 0)
		return result;

	result = init_qp(conn);
	if(result != 0)
		return result;

	return IBMSG_OK;	
}


int
ibmsg_close(ibmsg_connection* conn)
{
	int result;

	result = destroy_qp(conn);
	if(result != 0)
		return result;

	result = destroy_pd(conn);
	if(result != 0)
		return result;

	result = destroy_cq(conn);
	if(result != 0)
		return result;

	result = close_device(conn);
	if(result != 0)
		return result;

	return IBMSG_OK;	
}
