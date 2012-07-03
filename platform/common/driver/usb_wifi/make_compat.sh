#!/bin/bash
# This script make the compat-wireless driver

echo " =============== make_Compat.sh run ==============="
echo $0
echo $1
echo $2

cd usb_wifi/compat-wireless-2.6.39-1
if [ $1 = "make" ]; then
	echo " === make compat wireless driver ==="
	make
fi

if [ $1 = "clean" ]; then
	echo " === clean compat wireless driver ==="
	make clean
fi
cd ../../

