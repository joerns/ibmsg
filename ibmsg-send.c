/*
Send a single message over the given Infiniband device and port.

Author: Jörn Schumacher <joern.schumacher@cern.ch>
*/

#include <stdlib.h>
#include <stdio.h>
#include "ibmsg.h"


#define TRACE(x)   \
do { int result = x;    \
if(verbose) fprintf(stdout, "%s -> %d\n", #x, result); \
else if(0 != result) fprintf(stderr, "ibmsg-send: error: '%s' failed, error=%d\n", #x, result); \
} while(0)

int
main(int argc, char** argv)
{
	int verbose = 0;
	ibmsg_connection conn;
	ibmsg_address addr;

	addr.lid = 3;
	addr.is_global = 0;

	TRACE( ibmsg_open(&conn, "pib_0", 0) );
	TRACE( ibmsg_connect(&conn, addr) );

	TRACE( ibmsg_disconnect(&conn) );
	TRACE( ibmsg_close(&conn) );

	return EXIT_SUCCESS;
}