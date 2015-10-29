#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <rdma/rdma_verbs.h>

#include "ibmsg.h"

#define IBMSG_DEBUG

#ifdef IBMSG_DEBUG
#  include <stdio.h>
#  define LOG(...) do { fprintf(stderr, "LOG: "__VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#else
#  define LOG(...)
#endif

#define IBMSG_MAX_EVENTS          (64)
#define IBMSG_TIMEOUT_MS         (100)
#define IBMSG_RESPONDER_RESOURCES  (1)
#define IBMSG_HW_FLOW_CONTROL      (0)
#define IBMSG_RETRY_COUNT          (7)
#define IBMSG_RNR_RETRY_COUNT      (7)
#define IBMSG_MIN_CQE              (1)
#define IBMSG_MAX_WR             (128)
#define IBMSG_MAX_SGE              (2)
#define IBMSG_MAX_INLINE         (256)
#define IBMSG_HOP_LIMIT          (255)
#define IBMSG_MAX_DEST_RD_ATOMIC   (1)
#define IBMSG_MAX_RD_ATOMIC        (1)


#define CHECK_CALL(x, r)      \
  do {                        \
    int result = x;           \
    if(0 != result) return r; \
  } while(0)


static int
make_nonblocking(int fd)
{
    /* change the blocking mode of the completion channel */
    int flags = fcntl(fd, F_GETFL);
    int rc = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (rc < 0) {
        LOG("Failed to change file descriptor of Completion Event Channel\n");
        return -1;
    }

    return 0;
}

int
ibmsg_init_event_loop(ibmsg_event_loop* event_loop)
{
    event_loop->event_channel = rdma_create_event_channel();

    int fd = event_loop->event_channel->fd;
    if(make_nonblocking(fd))
        return IBMSG_FCNTL_FAILED;

    event_loop->epollfd = epoll_create(IBMSG_MAX_EVENTS);
    if (event_loop->epollfd == -1) {
        return IBMSG_EPOLL_FAILED;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    event_loop->event_description.type = IBMSG_CMA;
    ev.data.ptr = &(event_loop->event_description);
    if (epoll_ctl(event_loop->epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        return IBMSG_EPOLL_ADD_FD_FAILED;
    }

    event_loop->connection_established = NULL;
    event_loop->connection_request = NULL;
    event_loop->message_received = NULL;

    return IBMSG_OK;
}


int
ibmsg_destroy_event_loop(ibmsg_event_loop* event_loop)
{
    rdma_destroy_event_channel(event_loop->event_channel);
    close(event_loop->epollfd);
    return IBMSG_OK;
}


int
ibmsg_connect(ibmsg_event_loop* event_loop, ibmsg_socket* connection, char* ip, unsigned short port)
{
    struct rdma_addrinfo* address_results;                                                                                                                         
    struct sockaddr_in dst_addr;                                                                                                                                   
    char service[16];
    snprintf(service, 16, "%d", port);

    connection->status = IBMSG_UNCONNECTED;

    memset(&dst_addr, 0, sizeof dst_addr);                                                                                                                         
    dst_addr.sin_family = AF_INET;                                                                                                                                 
    dst_addr.sin_port = htons(atoi(service));
    inet_pton(AF_INET, ip, &dst_addr.sin_addr);

    CHECK_CALL( rdma_getaddrinfo(ip, service, NULL, &address_results), IBMSG_GETADDRINFO_FAILED );
    CHECK_CALL( rdma_create_id (event_loop->event_channel, &connection->cmid, connection, RDMA_PS_TCP), IBMSG_CREATE_ID_FAILED );
    CHECK_CALL( rdma_resolve_addr (connection->cmid, NULL, (struct sockaddr*)&dst_addr, IBMSG_TIMEOUT_MS), IBMSG_ADDRESS_RESOLUTION_FAILED );
     
    return IBMSG_OK;
}


int ibmsg_disconnect(ibmsg_socket* connection)
{
    connection->status = IBMSG_DISCONNECTING;
    CHECK_CALL( rdma_disconnect(connection->cmid), IBMSG_DISCONNECT_FAILED );
    return IBMSG_OK;
}


int
ibmsg_alloc_msg(ibmsg_buffer* msg, ibmsg_socket* connection, size_t size)
{
    void* buffer = malloc(size);
    if(!buffer)
        return IBMSG_ALLOC_FAILED;
    msg->status = IBMSG_WAITING;
    msg->data = buffer;
    msg->size = size;
    msg->mr = rdma_reg_msgs(connection->cmid, buffer, size);
    if(!msg->mr)
    {
        LOG("Could not allocate message: %s", strerror(errno));
        free(buffer);
        return IBMSG_MEMORY_REGISTRATION_FAILED;
    }
    return IBMSG_OK;
}


int
ibmsg_free_msg(ibmsg_buffer* msg)
{
    int result = IBMSG_OK;
    if(rdma_dereg_mr(msg->mr))
        result = IBMSG_MEMORY_REGISTRATION_FAILED;
    free(msg->data);
    return result;
}


int
ibmsg_post_send(ibmsg_socket* connection, ibmsg_buffer* msg)
{
    CHECK_CALL( rdma_post_send(connection->cmid, msg /* wrid */, msg->data, msg->size, msg->mr, 0), IBMSG_POST_SEND_FAILED );
    return IBMSG_OK;
}


int
ibmsg_listen(ibmsg_event_loop* event_loop, ibmsg_socket* socket, char* ip, short port, int max_connections)
{
    struct sockaddr_in src_addr;

    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &src_addr.sin_addr);

    CHECK_CALL( rdma_create_id (event_loop->event_channel, &socket->cmid, NULL, RDMA_PS_TCP), IBMSG_CREATE_ID_FAILED );
    CHECK_CALL( rdma_bind_addr (socket->cmid, (struct sockaddr*)&src_addr), IBMSG_BIND_FAILED );
    CHECK_CALL( rdma_listen (socket->cmid, max_connections), IBMSG_LISTEN_FAILED );

    return IBMSG_OK;
}


static void
init_qp_param(struct ibv_qp_init_attr* qp_init_attr)
{
    qp_init_attr->send_cq = NULL;
    qp_init_attr->recv_cq = NULL;
    qp_init_attr->srq = NULL;
    qp_init_attr->cap.max_send_wr = IBMSG_MAX_WR;
    qp_init_attr->cap.max_recv_wr = IBMSG_MAX_WR;
    qp_init_attr->cap.max_send_sge = IBMSG_MAX_SGE;
    qp_init_attr->cap.max_recv_sge = IBMSG_MAX_SGE;
    qp_init_attr->cap.max_inline_data = IBMSG_MAX_INLINE;
    qp_init_attr->qp_type = IBV_QPT_RC;
    qp_init_attr->sq_sig_all = 1;
}


static void
init_conn_param(struct rdma_conn_param* conn_param)
{
    memset(conn_param, 0, sizeof *conn_param);
    conn_param->responder_resources = IBMSG_RESPONDER_RESOURCES;
    conn_param->flow_control = IBMSG_HW_FLOW_CONTROL;
    conn_param->retry_count = IBMSG_RETRY_COUNT;
    conn_param->rnr_retry_count = IBMSG_RNR_RETRY_COUNT;
}


int
ibmsg_accept(ibmsg_connection_request* request, ibmsg_socket* connection)
{
    struct ibv_qp_init_attr qp_init_attr;
    struct rdma_conn_param conn_param;

    init_qp_param(&qp_init_attr);
    CHECK_CALL( rdma_create_qp (request->cmid, NULL, &qp_init_attr), IBMSG_CREATE_QP_FAILED );

    init_conn_param(&conn_param);
    CHECK_CALL( rdma_accept (request->cmid, &conn_param), IBMSG_ACCEPT_FAILED );

    request->status = IBMSG_ACCEPTED;
    connection->cmid = request->cmid;
    connection->cmid->context = connection;
    return IBMSG_OK;
}


static void
free_connection(ibmsg_socket* connection)
{
    rdma_destroy_qp(connection->cmid);
    rdma_destroy_id(connection->cmid);
    connection->cmid = NULL;
}


static int
add_connection_to_epoll(int epollfd, ibmsg_socket* connection)
{
    struct epoll_event ev;
    int fd;

    if(ibv_req_notify_cq(connection->cmid->send_cq, 0))
        return -1;
    fd = connection->cmid->send_cq_channel->fd;
    make_nonblocking(fd);
    connection->send_event_description.type = IBMSG_SEND_COMPLETION;
    connection->send_event_description.ptr = connection;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = &(connection->send_event_description);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        return -1;
    }

    if(ibv_req_notify_cq(connection->cmid->recv_cq, 0))
        return -1;
    fd = connection->cmid->recv_cq_channel->fd;
    make_nonblocking(fd);
    connection->recv_event_description.type = IBMSG_RECV_COMPLETION;
    connection->recv_event_description.ptr = connection;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = &(connection->recv_event_description);
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        return -1;
    }

    return 0;
}


static int
remove_connection_from_epoll(int epollfd, ibmsg_socket* connection)
{
    int fd;

    fd = connection->cmid->send_cq_channel->fd;
    if( epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        return -1;
    }

    fd = connection->cmid->recv_cq_channel->fd;
    if( epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == -1)
    {
        return -1;
    }

    return 0;
}


static void
post_receive(ibmsg_socket* connection)
{
    ibmsg_buffer* msg = &connection->recv_buffer;
    if(ibmsg_alloc_msg(msg, connection, IBMSG_MAX_MSGSIZE) != IBMSG_OK)
    {
        LOG("could not allocate memory for connection receive buffer");
        connection->status = IBMSG_ERROR;
    }
    LOG( "RECV: WRID 0x%llx", (long long unsigned)msg );
    rdma_post_recv(connection->cmid, msg, msg->data, msg->size, msg->mr);
    LOG( "Message receive posted" );
}


static void
process_rdma_event(ibmsg_event_loop* event_loop, struct rdma_cm_event* event)
{
    ibmsg_socket* connection = (ibmsg_socket*)event->id->context;
    struct ibv_qp_init_attr qp_init_attr;
    struct rdma_conn_param conn_param;
    ibmsg_connection_request request;

    LOG("EVENT: %s", rdma_event_str(event->event));

    switch(event->event)
    {
        case RDMA_CM_EVENT_ADDR_RESOLVED:
            connection->status = IBMSG_ADDRESS_RESOLVED;
            init_qp_param(&qp_init_attr);
            if(rdma_create_qp (connection->cmid, NULL, &qp_init_attr))
            {
                LOG("Create QP failed: %d '%s;", errno, strerror(errno));
                connection->status = IBMSG_ERROR;
            }
            if(rdma_resolve_route (connection->cmid, IBMSG_TIMEOUT_MS))
            {
                LOG("Resolve route failed: %d '%s;", errno, strerror(errno));
                connection->status = IBMSG_ERROR;
            }
            rdma_ack_cm_event (event);
            break;
        case RDMA_CM_EVENT_ADDR_ERROR:
            connection->status = IBMSG_ERROR;
            LOG("Could not resolve address: %d '%s;", errno, strerror(errno));
            rdma_ack_cm_event (event);
            break;
        case RDMA_CM_EVENT_ROUTE_RESOLVED:
            connection->status = IBMSG_ROUTE_RESOLVED;
            memset(&conn_param, 0, sizeof conn_param);
            init_conn_param(&conn_param);
            if(rdma_connect(connection->cmid, &conn_param))
            {
                LOG("RDMA connect failed: %d '%s;", errno, strerror(errno));
                connection->status = IBMSG_ERROR;
            }
            rdma_ack_cm_event (event);
            break;
        case RDMA_CM_EVENT_ROUTE_ERROR:
            connection->status = IBMSG_ERROR;
            LOG("Could not route: %d '%s;", errno, strerror(errno));
            rdma_ack_cm_event (event);
            break;
        case RDMA_CM_EVENT_CONNECT_REQUEST:
            if(event_loop->connection_request)
            {
                request.cmid = event->id;
                request.status = IBMSG_UNDECIDED;
                event_loop->connection_request(&request);
            }
            if(request.status != IBMSG_ACCEPTED)
            {
                LOG("connection request rejected");
                rdma_reject(event->id, NULL, 0);                    
            }
            else
            {
                LOG("connection request accepted");
            }
            rdma_ack_cm_event (event);
            break;
        case RDMA_CM_EVENT_ESTABLISHED:
            if(add_connection_to_epoll(event_loop->epollfd, connection))
            {
                connection->status = IBMSG_ERROR;
                free_connection(connection);
            }
            post_receive(connection);
            rdma_ack_cm_event (event);
            connection->status = IBMSG_CONNECTED;
            if(event_loop->connection_established)
                event_loop->connection_established(connection);
            break;
        case RDMA_CM_EVENT_REJECTED:
            connection->status = IBMSG_ERROR;
            rdma_ack_cm_event (event);
            free_connection(connection);
            break;
        case RDMA_CM_EVENT_DISCONNECTED:
            connection->status = IBMSG_UNCONNECTED;
            rdma_ack_cm_event (event);
            remove_connection_from_epoll(event_loop->epollfd, connection);
            free_connection(connection);
            LOG("Connection closed");
            break;
        default:
            break;
    }
}


int
ibmsg_dispatch_event_loop(ibmsg_event_loop* event_loop)
{
    int nfds;
    struct epoll_event events[IBMSG_MAX_EVENTS];
    do
    {
        nfds = epoll_wait(event_loop->epollfd, events, IBMSG_MAX_EVENTS, 10);
        if (nfds == -1) {
            return IBMSG_EPOLL_WAIT_FAILED;
        }
    } while(nfds == 0);

    LOG("Found %d event(s)", nfds);
    struct rdma_cm_event *cm_event;
    struct _ibmsg_event_description* data;
    struct ibv_wc wc;
    for(int i=0; i<nfds; i++)
    {
        data = events[i].data.ptr;
        if(data->type == IBMSG_CMA)
        {
            CHECK_CALL( rdma_get_cm_event (event_loop->event_channel, &cm_event), IBMSG_FETCH_EVENT_FAILED );
            process_rdma_event(event_loop, cm_event);
        }
        else if(data->type == IBMSG_SEND_COMPLETION)
        {
            ibmsg_socket* connection = data->ptr;
            int n_completions = rdma_get_send_comp(connection->cmid, &wc);
            if(n_completions == -1)
            {
                LOG("send completion error: %d ('%s')", errno, strerror(errno));
                return IBMSG_FETCH_EVENT_FAILED;
            }

            ibmsg_buffer* msg = (ibmsg_buffer*) wc.wr_id;
            msg->status = IBMSG_SENT;
            LOG( "%d send completion(s) found", n_completions);
            LOG( "SEND complete: WRID 0x%llx", (long long unsigned)wc.wr_id );
        }
        else if(data->type == IBMSG_RECV_COMPLETION)
        {
            ibmsg_socket* connection = data->ptr;
            int n_completions = rdma_get_recv_comp(connection->cmid, &wc);
            if(n_completions == -1)
            {
                LOG("recv completion error: %d ('%s')", errno, strerror(errno));
                return IBMSG_FETCH_EVENT_FAILED;
            }

            ibmsg_buffer* msg = (ibmsg_buffer*) (wc.wr_id);
            msg->status = IBMSG_RECEIVED;
            LOG( "%d recv completion(s) found", n_completions);
            LOG( "RECV complete: WRID 0x%llx", (long long unsigned)msg );
            if(event_loop->message_received)
            {
                event_loop->message_received(connection, msg);
            }
            else
            {
                if(ibmsg_free_msg(msg))
                    return IBMSG_FREE_BUFFER_FAILED;
            }
            if(connection->status == IBMSG_CONNECTED)
                post_receive(connection);
        }
    }

    return IBMSG_OK;
}
