#!/bin/sh
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:/system/lib/pulse-0.9.22/modules"

# check if /dev/shm exist
if [ ! -d /dev/shm ];then
	mkdir -p /dev/shm
	mount -t tmpfs tmpfs /dev/shm
fi

/system/bin/pulseaudio --high-priority --disallow-exit --exit-idle-time=-1 --resample-method=src-linear -L "module-oss device=/dev/dsp sink_name=output fragments=16 fragment_size=4096" -L "module-native-protocol-unix" -D
