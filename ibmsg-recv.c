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

int
main(int argc, char** argv)
{
	int verbose = 0;
	ibmsg_connection conn;
	ibmsg_address addr;
	ibmsg_message msg;
	uint64_t handle;

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
	TRACE( ibmsg_connect(&conn, addr) );
	TRACE( ibmsg_alloc(&conn, &msg, 4096) );

	TRACE( ibmsg_recv(&conn, &msg, &handle) );
	TRACE( ibmsg_wait_recv(&conn) );

	TRACE( ibmsg_free(&msg) );
	TRACE( ibmsg_disconnect(&conn) );
	TRACE( ibmsg_close(&conn) );

	return EXIT_SUCCESS;
}
