LDLIBS:=-libverbs -lrdmacm
CFLAGS:=-Wall -Werror -pedantic -g -std=gnu99

all: lsdev ibmsg-rdma-send ibmsg-rdma-recv

ibmsg-rdma-send: ibmsg-rdma-send.o ibmsg_rdma.o
ibmsg-rdma-recv: ibmsg-rdma-recv.o ibmsg_rdma.o
lsdev: lsdev.o

.PHONY: clean
clean:
	rm -f ibmsg-rdma-send ibmsg-rdma-recv lsdev *.o