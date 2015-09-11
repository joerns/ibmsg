LDLIBS:=-libverbs
CFLAGS:=-Wall -Werror -pedantic -g

all: ibmsg-send lsdev

ibmsg-send: ibmsg-send.o init.o connection.o
lsdev: lsdev.o

.PHONY: clean
clean:
	rm -f ibmsg-send lsdev