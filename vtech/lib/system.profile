if [ -d /etc/DEVELOPMENT_AND_PRODUCTION ]; then
	export LD_LIBRARY_PATH="/vp_mnt/sd/.VTECH.DEBUG.LIB:$LD_LIBRARY_PATH"
	export PATH="/vp_mnt/sd/.VTECH.DEBUG.LIB:$PATH"
fi
