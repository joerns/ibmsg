#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

if [[ $# -ne 1 ]]; then
	echo "Usage: pib.sh (start|stop)"
	exit 1
fi

if [[ "$1" == "start" ]]; then
	service rdma start
	modprobe pib
	service opensm start
elif [[ "$1" == "stop" ]]; then
	service opensm stop
	rmmod pib
	service rdma stop
else
	echo "Usage: pib.sh (start|stop)"
	exit 1
fi
