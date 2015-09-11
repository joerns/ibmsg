#include <unistd.h>
#include <stdio.h>
#include "ibmsg.h"

/* Wait for a single event */
static int
wait_single(struct ibv_cq* cq)
{
	struct ibv_wc wc;
	int n = 0;
	n = ibv_poll_cq(cq, 1, &wc);
	while(n == 0)
	{
		n = ibv_poll_cq(cq, 1, &wc);
		if(n == -1)
			return IBMSG_POLL_CQ_FAILED;

		usleep(100000);
	}

	fprintf(stderr, "wc.status: %d\n", wc.status);
	if(wc.status == IBV_WC_RETRY_EXC_ERR)
		fprintf(stderr, "wc.status: IBV_WC_RETRY_EXC_ERR\n");
	if(wc.status != IBV_WC_SUCCESS)
		return IBMSG_COMPLETION_FAILED;

	return IBMSG_OK;
}


int
ibmsg_wait_recv(ibmsg_connection* conn)
{
	return wait_single(conn->rx_cq);
}

int
ibmsg_wait_send(ibmsg_connection* conn)
{
	return wait_single(conn->tx_cq);
}
