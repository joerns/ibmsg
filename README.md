# ibmsg
Asynchronous Infiniband Message Library

Ibmsg is an open source library providing a reliable messaging service in Infiniband networks.
It is a thin layer on top of librdma and libibverbs for connection handling and communication.

## Documentation

### Examples

Please see [ibmsg-send.c](ibmsg-send.c) and [ibmsg-recv.c](ibmsg-recv.c) for an example of how to use ibmsg.

### Event Loop

Events in ibmsg are processed by an event loop. 
An event loop data structure is used to keep track of events:

    int ibmsg_init_event_loop(ibmsg_event_loop* event_loop)
    int ibmsg_destroy_event_loop(ibmsg_event_loop* event_loop)

The user is responsible to execute the event loop.
The event loop can either be run in its own thread in an endless loop, or mixed with 
other ibmsg operations. This functions processes standing events of the event loop
and then returns to the user when all events have been processed:

    int ibmsg_dispatch_event_loop(ibmsg_event_loop* event_loop)

The user may define several callbacks in an `ibmsg_event_loop` object that are
executed when certain events occur. Currently these callbacks function are:

    void (*connection_request)(ibmsg_connection_request*)
    void (*connection_established)(ibmsg_connection*)
    void (*message_received)(ibmsg_connection*, ibmsg_buffer*)



### Connection Management

Connections are tracked in a `ibmsg_connection` data structure.
Connections are used on the server as well as one the client side.
A server can listen on a specific IP address and port:

    int ibmsg_listen(ibmsg_event_loop* event_loop, ibmsg_connection* socket, char* ip, short port, int max_connections)

Once a client requests to open a connection with the server, the `connection_request` 
callback is executed on the server side. The user has to supply a callback function for
`connection_request` that either accepts or rejects the request. 
The new `ibmsg_connection` object is passed as parameter to the callback function. 
By default all requests are rejected if no callback function is supplied.
If a user wants to accept a connection,
the following function must be called from the callback:

    int ibmsg_accept(ibmsg_connection_request* request, ibmsg_connection* connection)

The function `ibmsg_connect` has to be evoked on the client side to connect to a remote host:

    int ibmsg_connect(ibmsg_event_loop* event_loop, ibmsg_connection* connection, char* ip, unsigned short port)
    
Either client or server can close a connection at all times using `ibmsg_disconnect`:
    
    int ibmsg_disconnect(ibmsg_connection* connection);


### Messages

Messages are enclosed in `ibmsg_buffer` objects and have to allocated and freed with the following
functions:

    int ibmsg_alloc_msg(ibmsg_buffer* msg, ibmsg_connection* connection, size_t size)
    int ibmsg_free_msg(ibmsg_buffer* msg)
    
To send a message to the remote side of a connection, call `ibmsg_post_send`. Remember that ibmsg is
asynchronous: the call will return immediately even if the message is still being sent. Do not
attempt to free the message before the status of the message changed to IBMSG_SENT.

    int ibmsg_post_send(ibmsg_connection* connection, ibmsg_buffer* msg)

### Error Handling

All functions of the ibmsg API return an error code as integer. A return code of 0 (or IBMSG_OK) indicates
success. Any other return code indicates an error. See ibmsg.h for the different error codes.
