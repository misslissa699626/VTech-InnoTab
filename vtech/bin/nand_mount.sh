#!/bin/sh
echo $0 $*
if [ "$1" = "add" ]; then
	ext_nand_mount.sh
else
	ext_nand_mount.sh -d
fi

