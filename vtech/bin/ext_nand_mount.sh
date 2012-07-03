#!/bin/sh

. /sbin/nand_ev.sh

MOUNT_POINT=/vp_mnt/cart

if [ "$1" = "-d" ]; then
	rm /vp_mnt/CART_TITLE_BIN_FILE 2> /dev/null

	umount -d $MOUNT_POINT
	while is_mount $MOUNT_POINT ; do 
		echo "try umount..." 
		sleep 0.5
		umount -d $MOUNT_POINT
	done

	rmdir $MOUNT_POINT
	
	mtdctl -dm 1
	mtdctl -dc 1
	
else
	if [ -e $MOUNT_POINT ]; then
		#already mounted
		exit 0
	fi

	rm /vp_mnt/CART_TITLE_BIN_FILE 2> /dev/null
	mkdir -p $MOUNT_POINT

	#add device
	mtdctl -ac 1 
	if [ ! $? -eq 0 ]; then
		echo Add device failure ...
		umount $MOUNT_POINT
		rmdir $MOUNT_POINT
		exit 1
	fi
	
	mtdctl -am 1 1
	if [ ! $? -eq 0 ]; then
		echo Add device failure ...
		umount $MOUNT_POINT
		rmdir $MOUNT_POINT
		mtdctl -dc 1
		exit 1
	fi

	COUNTER=0
	while [ $COUNTER -lt 10 ]; do
	
		NUMBERS="0 1 2 3 4 5 6 7 8 9"
		for MTD_NUM in $NUMBERS ; do
			#echo MTD_NUM=$MTD_NUM
			MTD_NAME=`cat /sys/class/mtd/mtd${MTD_NUM}/name`
			case "$MTD_NAME" in
				"$Cart_Part_Name_ROM" | "$Cart_Part_Name_Flash")
					if [ ! -e /dev/mtdblock${MTD_NUM} ]; then
						break
					fi
					mount -r -t vfat /dev/mtdblock${MTD_NUM} $MOUNT_POINT
					if [ ! $? -eq 0 ]; then 
						echo Cartridge mount failure.  Removing device ...
						umount $MOUNT_POINT
						rmdir $MOUNT_POINT
						mtdctl -dm 1
						mtdctl -dc 1
						exit 1
					fi 
					TITLE_BIN_FILENAME=`ls /vp_mnt/cart/Books/*`
					if [ -z "$TITLE_BIN_FILENAME" ]; then
						echo No Title Binary file!  Remove device...
						umount $MOUNT_POINT
						rmdir $MOUNT_POINT
						mtdctl -dm 1
						mtdctl -dc 1
						exit 1
					fi
					ln -s "$TITLE_BIN_FILENAME" /vp_mnt/CART_TITLE_BIN_FILE
					exit 0
					;;
				*)
					;;	
			esac
		done
		
		echo Waiting for cartridge ready...
		sleep 0.1
		let COUNTER=COUNTER+1 
	done
	
	echo Time out for mounting the cartridge...
	umount $MOUNT_POINT
	rmdir $MOUNT_POINT
	mtdctl -dm 1
	mtdctl -dc 1
	exit 1  
fi
