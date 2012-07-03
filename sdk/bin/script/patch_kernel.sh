#!/bin/bash

###########################################################
### Unpack kernel base source
###########################################################
echo "Unpack sdk/os/linux-2.6.32.9.tar.gz"
tar xzf ../linux-2.6.32.9.tar.gz
mv ./linux-2.6.32.9 ./kernel-2.6.32


###########################################################
### Patch kernel
###########################################################
echo "Patch kernel"
cd ./kernel-2.6.32
patch -p3 < ../kernel.diff
cp -aR ../mach-spmp8050/* ./arch/arm/mach-spmp8050/
cp -aR ../mach-gpl32900/* ./arch/arm/mach-gpl32900/
cd - > /dev/null


###########################################################
### Update kernel to sdk/os/
###########################################################
echo "Update sdk/os/kernel-2.6.32"
rm -rf ../kernel-2.6.32
mv ./kernel-2.6.32 ../kernel-2.6.32

