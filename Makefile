LDLIBS:=-libverbs -lrdmacm
CFLAGS:=-Wall -Werror -pedantic -g -std=gnu99

all: lsdev ibmsg-send ibmsg-recv

ibmsg-send: ibmsg-send.o ibmsg.o
ibmsg-recv: ibmsg-recv.o ibmsg.o
lsdev: lsdev.o

.PHONY: clean
clean:
	rm -f ibmsg-send ibmsg-recv lsdev *.o