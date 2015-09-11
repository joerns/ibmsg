#ifndef IBMSG_H
#define IBMSG_H

#include "infiniband/verbs.h"

#define IBMSG_MIN_CQE    (1)
#define IBMSG_MAX_WR     (128)
#define IBMSG_MAX_SGE    (2)
#define IBMSG_MAX_INLINE (2048)
#define IBMSG_HOP_LIMIT  (255)
#define IBMSG_MAX_DEST_RD_ATOMIC (1)
#define IBMSG_MAX_RD_ATOMIC (1)

enum {
	IBMSG_OK = 0,
	IBMSG_DEVICE_NOT_FOUND,
	IBMSG_COULD_NOT_OPEN,
	IBMSG_COULD_NOT_CLOSE,
	IBMSG_CQ_FAILED,
	IBMSG_PD_FAILED,
	IBMSG_QP_FAILED,
	IBMSG_AH_FAILED,
	IBMSG_RESET_TO_INIT_FAILED,
	IBMSG_INIT_TO_RTR_FAILED,
	IBMSG_RTR_TO_RTS_FAILED,
	IBMSG_DISCONNECT_FAILED,
	IBMSG_ALLOC_FAILED,
	IBMSG_REG_MR_FAILED,
	IBMSG_DEREG_MR_FAILED,
	IBMSG_POST_RECV_FAILED,
	IBMSG_POST_SEND_FAILED,
	IBMSG_POLL_CQ_FAILED,
	IBMSG_COMPLETION_FAILED
};

typedef struct {
	uint8_t portid;
	struct ibv_context* context;
	struct ibv_cq* rx_cq;
	struct ibv_cq* tx_cq;
	struct ibv_pd* pd;
	struct ibv_qp* qp;
} ibmsg_connection;

typedef struct {
	union ibv_gid gid;
	uint16_t lid;
	uint8_t is_global;
} ibmsg_address;

typedef struct {
	void* addr;
	size_t length;
	struct ibv_mr* mr;
} ibmsg_message;

int ibmsg_open(ibmsg_connection* conn, const char* name, uint8_t portid);
int ibmsg_close(ibmsg_connection* conn);
int ibmsg_connect(ibmsg_connection* conn, ibmsg_address addr);
int ibmsg_disconnect(ibmsg_connection* conn);
int ibmsg_alloc(ibmsg_connection* conn, ibmsg_message* msg, size_t length);
int ibmsg_free(ibmsg_message* msg);
int ibmsg_recv(ibmsg_connection* conn, ibmsg_message* msg, uint64_t* handle);
int ibmsg_send(ibmsg_connection* conn, ibmsg_message* msg, uint64_t* handle);
int ibmsg_wait_recv(ibmsg_connection* conn);
int ibmsg_wait_send(ibmsg_connection* conn);


#endif
