LDLIBS:=-libverbs -lrdmacm
CFLAGS:=-Wall -Werror -pedantic -g -std=gnu99

all: ibmsg-send ibmsg-recv lsdev ibmsg-rdma-send ibmsg-rdma-recv

ibmsg-rdma-send: ibmsg-rdma-send.o ibmsg_rdma.o
ibmsg-rdma-recv: ibmsg-rdma-recv.o ibmsg_rdma.o
ibmsg-send: ibmsg-send.o
ibmsg-recv: ibmsg-recv.o
ibmsg-send ibmsg-recv: init.o connection.o message.o recv.o send.o eventloop.o
lsdev: lsdev.o

.PHONY: clean
clean:
	rm -f ibmsg-send ibmsg-recv lsdev *.o