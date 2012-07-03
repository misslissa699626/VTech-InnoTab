#!/bin/sh

#Format MTD partition for if_dlg2011
#Copy from _Run_Once and renamed, as temp fixes...
__wait_dev_up_init_project_rc() {
	local COUNTER=0
	while [ $COUNTER -lt 30 ]; do
		if [ ! -e "$1" ]; then
			echo Waiting for device "$1" to up...
			sleep 0.1
			let COUNTER=COUNTER+1
		else
			echo Device "$1" found
			break
		fi 
	done
	if [ -e "$1" ]; then
		true
	else
		false		
	fi
}

#Copy from _Run_Once and modified, as temp fixes...
__fix_internal_partitions () {
	local MTD_PART_NAME="$1"
	local UBI_DIR_NAME="$2"
	local UBI_MOUNT_POINT=/vp_mnt/$UBI_DIR_NAME
	local ALL_MTD_NUM="0 1 2 3 4 5 6 7 8 9"
	for MTD_NUM in $ALL_MTD_NUM ; do
		local MTD_NAME=`cat /sys/class/mtd/mtd${MTD_NUM}/name 2> /dev/null`
		case "$MTD_NAME" in
			"$MTD_PART_NAME")
				local UBI_D=$MTD_NUM
				local UBI_M=$MTD_NUM
				mkdir -p $UBI_MOUNT_POINT
				flash_erase /dev/mtd$UBI_M 0 0
				ubiattach -d $UBI_D -m $UBI_M && \
					__wait_dev_up_init_project_rc /dev/ubi$UBI_D && \
					ubimkvol /dev/ubi$UBI_D -m -N ubi_on_mtd$UBI_M -n 0 -t dynamic && \
					__wait_dev_up_init_project_rc /dev/ubi${UBI_D}_0 && \
					mount -t ubifs -o "compr=$3" ubi${UBI_D}_0 $UBI_MOUNT_POINT && \
					umount $UBI_MOUNT_POINT && \
					ubidetach -d $UBI_D && \
					rmdir $UBI_MOUNT_POINT 
				local ubi_result=$?
				# FIXME: What if this is error?
				if [ $ubi_result -ne 0 ]; then
					echo ========================================================
					echo @ FAILED TO MOUNT ON INTERNAL NAND FLASH! 
					echo ========================================================
				else
					if [ $MTD_PART_NAME = Inno_I_IF_DLG_2011 ]; then
						mtdctl -if 0
						if [ $? -ne 0 ]; then
							echo ========================================================
							echo @ FAILED TO initialize $MTD_PART_NAME 
							echo ========================================================
						fi					
					fi
				fi
				break
				;;
			*)
				;;	
		esac
	done 
}

__mk_if_dlg2011 () {
echo Initialize new partition...
mtdctl -cif 0
if [ $? -ne 0 ]; then
	__fix_internal_partitions Inno_I_IF_DLG_2011 if_dlg2011 none
else
	echo "=========================================================="
	echo NAND already formatted.  Re-initialization not necessary.
	echo "=========================================================="
fi
}
