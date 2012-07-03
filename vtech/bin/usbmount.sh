#!/bin/sh 
# based on http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=65516 

#debug 
#echo "action: $ACTION / mdev: $MDEV / devpath: $DEVPATH / subsystem: $SUBSYSTEM / mntdir: $MNT_DIR" >> /hotplug.log

prefix='usb'
input_name=$1
usb_name="$prefix${input_name:2:2}"

# check mount / unmount
if [ "$ACTION" = "remove" ]; then
	# unmount
	/bin/umount -lf "/media/$usb_name"
	/bin/rmdir "/media/$usb_name"
else
	# mount
	if [ ! -d "/media/$usb_name" ]; then
		/bin/mkdir "/media/$usb_name"
	fi
	/bin/mount -t vfat -o noatime,nodiratime /dev/$1 "/media/$usb_name"

	result=$?

	if [ ! "$result" = "0" ]; then
		/bin/rmdir "/media/$usb_name"
	fi
fi
