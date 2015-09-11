LDLIBS:=-libverbs
CFLAGS:=-Wall -Werror -pedantic -g

all: ibmsg-send ibmsg-recv lsdev

ibmsg-send: ibmsg-send.o
ibmsg-recv: ibmsg-recv.o
ibmsg-send ibmsg-recv: init.o connection.o message.o recv.o send.o eventloop.o
lsdev: lsdev.o

.PHONY: clean
clean:
	rm -f ibmsg-send ibmsg-recv lsdev