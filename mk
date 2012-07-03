#!/bin/sh

OUTPATH=./out/gplus.sampleCode__gplus.evm32900
TPATH=./project/gplus/sampleCode/rootfs/etc

rm -rf $OUTPATH/system $OUTPATH/system_image

rm -rf $TPATH/DEVELOPMENT_AND_PRODUCTION

BUILD_DATETIME=`date "+%F %T"; echo rel; echo $USER`
echo "$BUILD_DATETIME" > ./project/gplus/sampleCode/rootfs/etc/build_signature.kernel
echo "$BUILD_DATETIME" > ./vtech/lib/build_signature.system
echo "Build signature:"
echo "$BUILD_DATETIME"


if make; then
	cp -f $OUTPATH/kernel.bin ./kernel_rel.bin
	cp -f $OUTPATH/system.bin ./system_rel.bin
	echo "######################"
	echo "# Success (rel)"
	echo "######################"
else
	echo "!!!!!!!!!!!!!!!!!!!!!!"
	echo "! Fail (rel)"
	echo "!!!!!!!!!!!!!!!!!!!!!!"
fi

