#!/bin/sh 
# based on http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=65516 

#debug 
#echo "action: $ACTION / mdev: $MDEV / devpath: $DEVPATH / subsystem: $SUBSYSTEM / mntdir: $MNT_DIR" >> /hotplug.log

#flock
#(
#flock -x 3
#if [ $? -ne 0 ]; then
#	echo "ERROR: Unable to acquire the sdmount.sh lock. Is another is running?" >> /dev/console
#	exit 1
#fi

# MMC slot 0 
mmc_name=$2

ACTION=$1

#LOGFILE=/tmp/vic.sdmount.log.$SEQNUM
#LOGFILE=/dev/null
LOGFILE=/dev/console

VPMNT_SD=/vp_mnt/sd
mkdir -p /vp_mnt

MOUNT_CNT=0
DELAY_TIME=100000

echo "sdmount.sh: Enter $0 ACTION=$ACTION, mmc_name=$mmc_name, SEQNUM=$SEQNUM" >> $LOGFILE

USB_CONNECTED_FILE=/var/run/USB_CONNECTED


export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/lib:/usr/lib:/system/lib/vtech:/system/lib
export PATH=$PATH:/system/bin/vtech


if [ ! -f $USB_CONNECTED_FILE ]; then
	if [ "$ACTION" = "remove" ]; then
		if [ -e /media/$mmc_name ]; then
			if [ $VPMNT_SD -ef /media/$mmc_name ]; then
				ln -sfn /dev/null $VPMNT_SD
			fi
			umount -lf "/media/$mmc_name"
			rmdir "/media/$mmc_name"
			echo "$mmc_name: umount" >> $LOGFILE
			systemset -w "sd_plug_change" "YES"
			systemset -add "sd_plug_count" 1
			if [ -f /var/run/desktopManager.pid ]; then
				kill -60 `cat /var/run/desktopManager.pid`
			fi
		fi
	
	elif [ "$ACTION" = "add" ]; then
		#add
		if [ ! -e /media/$mmc_name ]; then
			mkdir -p /media/$mmc_name
			dosfsck -a -w /media/$mmc_name
			MOUNT_CNT=0
			while [ $MOUNT_CNT -lt 5 ];do
				if mount -t vfat -o utf8,noatime /dev/$mmc_name "/media/$mmc_name" ; then
					echo "$mmc_name: mount" >> $LOGFILE
					systemset -w "sd_plug_change" "YES"
					systemset -add "sd_plug_count" 1
					if [ "$mmc_name" != "sdcarda2" ]; then
						if [ ! -d $VPMNT_SD ]; then
							#no SD partition is mounted FAT
							ln -sfn /media/$mmc_name $VPMNT_SD
						fi
					fi
					break				
				else
					result=$?
					if [ $MOUNT_CNT -eq 4 ]; then
						rmdir /media/$mmc_name
					fi
					DELAY_TIME=$(((($RANDOM%10)+1)*10000))
					usleep $DELAY_TIME
					echo "sdmount.sh: mount /dev/$mmc_name Fail  result: $result SEQNUM=$SEQNUM ###  MOUNT_CNT = $MOUNT_CNT DELAY_TIME = $DELAY_TIME" >> $LOGFILE
				fi
				MOUNT_CNT=$(($MOUNT_CNT+1))
			done
	
			
		fi
		
	elif [ "$ACTION" = "usb_connected" ]; then
		#desktopManager notify USB is connected
		media_mmc_name=`readlink -f $VPMNT_SD`
		if [ -e "$media_mmc_name" ]; then
			if umount -f $media_mmc_name ; then
				if [ $VPMNT_SD -ef $media_mmc_name ]; then
					ln -sfn /dev/null $VPMNT_SD
				fi
				rmdir $media_mmc_name
			fi
		fi
		touch $USB_CONNECTED_FILE
	fi 
else
	if [ "$ACTION" = "usb_disconnected" ]; then
		#desktopManager notify USB is connected
		rm $USB_CONNECTED_FILE
		systemset -w "sd_plug_change" ""
	fi
fi

echo "sdmount.sh: Leave $0 ACTION=$ACTION, mmc_name=$mmc_name, SEQNUM=$SEQNUM MOUNT_TIME=$MOUNT_CNT" >> $LOGFILE
#) 3<>/var/locksdscriptfile
