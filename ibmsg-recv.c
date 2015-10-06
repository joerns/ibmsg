/*
Receives a single message over the given Infiniband device and port.

Author: Jörn Schumacher <joern.schumacher@cern.ch>
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ibmsg.h"

#define APPLICATION_NAME "ibmsg-recv"

#define TRACE(x)   \
do { int result = x;    \
if(verbose) fprintf(stdout, "%s -> %d\n", #x, result); \
else if(0 != result) fprintf(stderr, APPLICATION_NAME": error: '%s' failed, error=%d\n", #x, result); \
} while(0)


static void*
thread_print_events(void* context)
{
	struct ibv_async_event event;

	while(1)
	{
		if(0 != ibv_get_async_event((struct ibv_context*)context, &event))
			return NULL;
		printf("EVENT TRACE: %s\n", ibv_event_type_str(event.event_type));
		ibv_ack_async_event(&event);
	}
	return NULL;
}

int
main(int argc, char** argv)
{
	int verbose = 0;
	ibmsg_connection conn;
	ibmsg_address addr;
	ibmsg_message msg;
	uint64_t handle;
	pthread_t event_thread;

	char* dev;
	uint8_t port;
	uint16_t dlid;

	if(argc != 4)
	{
		fprintf(stderr, "Usage: "APPLICATION_NAME" DEVICE PORT DESTINATION_LID\n");
		exit(EXIT_FAILURE);
	}
	dev = argv[1];
	port = atoi(argv[2]);
	dlid = atoi(argv[3]);

	addr.lid = dlid;
	addr.is_global = 0;

	TRACE( ibmsg_open(&conn, dev, port) );

	if(0 != pthread_create(&event_thread, NULL,
                          thread_print_events, (void*)conn.context))
	{
		fprintf(stderr, "Could not create event thread\n");
		exit(EXIT_FAILURE);
	}

	TRACE( ibmsg_connect(&conn, addr) );
	TRACE( ibmsg_alloc(&conn, &msg, 512) );

	TRACE( ibmsg_recv(&conn, &msg, &handle) );
	TRACE( ibmsg_wait_recv(&conn) );

	TRACE( ibmsg_free(&msg) );
	TRACE( ibmsg_disconnect(&conn) );
	TRACE( ibmsg_close(&conn) );

	pthread_join(event_thread, NULL);

	return EXIT_SUCCESS;
}
