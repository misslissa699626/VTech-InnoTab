#!/bin/bash
echo "=== Kernel release all ==="
echo "Kernel source: sdk/ok/kernel-2.6.32"
echo "Kernel base source: sdk/ok/linux-2.6.32.9.tar.gz"
echo ""

RELEASE_32900=n
RELEASE_8050=n

if [ -z "$1" ]; then
	RELEASE_32900=y
	RELEASE_8050=y
elif [ $1 == "32900" ]; then
	RELEASE_32900=y
elif $1 == "8050"]; then
	RELEASE_8050=y
fi

echo "RELEASE_32900" = $RELEASE_32900
echo "RELEASE_8050" = $RELEASE_8050

###########################################################
### Copy kernel source to ./release/kernel
###########################################################
echo "Copy Kernel source to release/kernel"
mkdir -p ./release/
rm -rf ./release/kernel
rm -rf ./release/kernel_patch
rm -rf ./release/kernel_patch.tgz

cp -aR sdk/os/kernel-2.6.32/ ./release/kernel/
find ./release/kernel -name ".svn" | xargs rm -rf

rm -rf ./release/kernel-2.6.32-prebuild

rm -rf ./release/kernel-2.6.32-headers

if [ $RELEASE_8050 == "y" ]; then
	mkdir -p ./release/kernel_patch/mach-spmp8050
	mkdir -p ./release/kernel-2.6.32-headers/SPMP8050
fi

if [ $RELEASE_32900 == "y" ]; then
	mkdir -p ./release/kernel_patch/mach-gpl32900
	mkdir -p ./release/kernel-2.6.32-headers/GPL32900
fi

###########################################################
### Build for SPMP8050
###########################################################
if [ $RELEASE_8050 == "y" ]; then
	echo "Build SPMP8050 release lib"
	cd ./release/kernel
	make clean
	rm -rf .config
	rm -rf .config.old
	rm -rf .version
	rm -rf Module.symvers
	rm -f ./include/asm
	make spmp8050_defconfig
	make
	mkdir -p ../kernel-2.6.32-prebuild/SPMP8050
	cp -f ./arch/arm/boot/Image ../kernel-2.6.32-prebuild/SPMP8050/
	make modules_install INSTALL_MOD_PATH=../kernel-2.6.32-prebuild/SPMP8050/modules_install
	rm -rf ../kernel-2.6.32-prebuild/SPMP8050/modules_install/lib/modules/*/build
	rm -rf ../kernel-2.6.32-prebuild/SPMP8050/modules_install/lib/modules/*/source
	cp -f ./Module.symvers ../kernel-2.6.32-headers/SPMP8050/
	cd - > /dev/null

	### Copy prebuild headers
	cd ./release/kernel
	cp -aR ./Makefile ../kernel-2.6.32-headers/
	cp -aR ./scripts ../kernel-2.6.32-headers/
	cp -aR ./include ../kernel-2.6.32-headers/
	mkdir -p ../kernel-2.6.32-headers/arch/arm
	cp -aR ./arch/arm/Makefile ../kernel-2.6.32-headers/arch/arm/
	cp -aR ./arch/arm/include ../kernel-2.6.32-headers/arch/arm/
	mkdir -p ../kernel-2.6.32-headers/arch/arm/mach-spmp8050
	cp -aR ./arch/arm/mach-spmp8050/include ../kernel-2.6.32-headers/arch/arm/mach-spmp8050/
	#mkdir -p ../kernel-2.6.32-headers/arch/arm/mach-gpl32900
	#cp -aR ./arch/arm/mach-gpl32900/include ../kernel-2.6.32-headers/arch/arm/mach-gpl32900/
	cd - > /dev/null
	
	cd ./release/kernel-2.6.32-headers/SPMP8050/
	ln -s ../Makefile ./Makefile
	ln -s ../scripts ./scripts
	ln -s ../include ./include
	ln -s ../arch ./arch
	cd - > /dev/null

	### Create nand lib
	cd ./release/kernel/arch/arm/mach-spmp8050/
	rm -rf ./nand_lib
	mkdir -p ./nand_lib
	find ./nand/ -name "*.o" -exec cp {} ./nand_lib/ \;
	rm -rf ./nand_lib/built-in.o
	rm -rf ./nand_lib/gp_blk_app.*
	rm -rf ./nand_lib/gp_nand_module.*
	rename 's/\.o$/.lib/' ./nand_lib/*.o
	cp ./nand/Makefile.release ./nand_lib/Makefile
	rm -rf ./nand
	mv ./nand_lib ./nand
	cd - > /dev/null
	mv ./release/kernel/arch/arm/mach-spmp8050/nand ./release/kernel_patch/mach-spmp8050/
	
	### Create ms lib (remove ms driver)
	cd ./release/kernel/arch/arm/mach-spmp8050/
	rm -rf ./ms_lib
	mkdir -p ./ms_lib
	#find ./ms/ -name "*.o" -exec cp {} ./ms_lib/ \;
	#rm -rf ./ms_lib/built-in.o
	#rm -rf ./ms_lib/gp_ms_module.*
	#rename 's/\.o$/.lib/' ./ms_lib/*.o
	touch ./ms_lib/ms_dummy.c
	cp ./ms/Makefile.release ./ms_lib/Makefile
	rm -rf ./ms
	mv ./ms_lib ./ms
	cd - > /dev/null
	mv ./release/kernel/arch/arm/mach-spmp8050/ms ./release/kernel_patch/mach-spmp8050/
	
	### Create ceva lib
	cd ./release/kernel/arch/arm/mach-spmp8050/
	rm -rf ./ceva_lib
	mkdir -p ./ceva_lib
	find ./ceva/ -name "*.o" -exec cp {} ./ceva_lib/ \;
	rm -rf ./ceva_lib/built-in.o
	rm -rf ./ceva_lib/gp_ceva_module.*
	rename 's/\.o$/.lib/' ./ceva_lib/*.o
	cp ./ceva/Makefile.release ./ceva_lib/Makefile
	rm -rf ./ceva
	mv ./ceva_lib ./ceva
	cd - > /dev/null
	mv ./release/kernel/arch/arm/mach-spmp8050/ceva ./release/kernel_patch/mach-spmp8050/
fi

###########################################################
### Build for GPL32900
###########################################################
if [ $RELEASE_32900 == "y" ]; then
	echo "Build GPL32900 release lib"
	cd ./release/kernel
	make clean
	rm -rf .config
	rm -rf .config.old
	rm -rf .version
	rm -rf Module.symvers
	rm -f ./include/asm

	make gpl32900_pm_defconfig
	
	make
	mkdir -p ../kernel-2.6.32-prebuild/GPL32900
	cp -f ./arch/arm/boot/Image ../kernel-2.6.32-prebuild/GPL32900/
	make modules_install INSTALL_MOD_PATH=../kernel-2.6.32-prebuild/GPL32900/modules_install
	rm -rf ../kernel-2.6.32-prebuild/GPL32900/modules_install/lib/modules/*/build
	rm -rf ../kernel-2.6.32-prebuild/GPL32900/modules_install/lib/modules/*/source
	cp -f ./Module.symvers ../kernel-2.6.32-headers/GPL32900/
	cd - > /dev/null
	
	### Copy prebuild headers
	cd ./release/kernel
	cp -aR ./Makefile ../kernel-2.6.32-headers/
	cp -aR ./scripts ../kernel-2.6.32-headers/
	cp -aR ./include ../kernel-2.6.32-headers/
	mkdir -p ../kernel-2.6.32-headers/arch/arm
	cp -aR ./arch/arm/Makefile ../kernel-2.6.32-headers/arch/arm/
	cp -aR ./arch/arm/include ../kernel-2.6.32-headers/arch/arm/
	mkdir -p ../kernel-2.6.32-headers/arch/arm/mach-gpl32900
	cp -aR ./arch/arm/mach-gpl32900/include ../kernel-2.6.32-headers/arch/arm/mach-gpl32900/
	cd - > /dev/null
	
	cd ./release/kernel-2.6.32-headers/GPL32900/
	ln -s ../Makefile ./Makefile
	ln -s ../scripts ./scripts
	ln -s ../include ./include
	ln -s ../arch ./arch
	cd - > /dev/null
	
	### Create nand lib
	cd ./release/kernel/arch/arm/mach-gpl32900/
	rm -rf ./nand_lib
	mkdir -p ./nand_lib
	find ./gp_nand/ -name "*.o" -exec cp {} ./nand_lib/ \;
	rm -rf ./nand_lib/built-in.o
	rm -rf ./nand_lib/gp_blk_app.*
	rm -rf ./nand_lib/gp_nand_module.*
	rename 's/\.o$/.lib/' ./nand_lib/*.o
	cp ./gp_nand/Makefile.release ./nand_lib/Makefile
	rm -rf ./gp_nand
	mv ./nand_lib ./gp_nand
	cd - > /dev/null
	mv ./release/kernel/arch/arm/mach-gpl32900/gp_nand ./release/kernel_patch/mach-gpl32900/
	
	### Create ms lib (remove ms driver)
	cd ./release/kernel/arch/arm/mach-gpl32900/
	rm -rf ./ms_lib
	mkdir -p ./ms_lib
	#find ./ms/ -name "*.o" -exec cp {} ./ms_lib/ \;
	#rm -rf ./ms_lib/built-in.o
	#rm -rf ./ms_lib/gp_ms_module.*
	#rename 's/\.o$/.lib/' ./ms_lib/*.o
	touch ./ms_lib/ms_dummy.c
	cp ./ms/Makefile.release ./ms_lib/Makefile
	rm -rf ./ms
	mv ./ms_lib ./ms
	cd - > /dev/null
	mv ./release/kernel/arch/arm/mach-gpl32900/ms ./release/kernel_patch/mach-gpl32900/
	
	### Create ceva lib
	cd ./release/kernel/arch/arm/mach-gpl32900/
	rm -rf ./ceva_lib
	mkdir -p ./ceva_lib
	find ./ceva/ -name "*.o" -exec cp {} ./ceva_lib/ \;
	rm -rf ./ceva_lib/built-in.o
	rm -rf ./ceva_lib/gp_ceva_module.*
	rename 's/\.o$/.lib/' ./ceva_lib/*.o
	cp ./ceva/Makefile.release ./ceva_lib/Makefile
	rm -rf ./ceva
	mv ./ceva_lib ./ceva
	cd - > /dev/null
	mv ./release/kernel/arch/arm/mach-gpl32900/ceva ./release/kernel_patch/mach-gpl32900/
fi

###########################################################
### Clean kernel
###########################################################
echo "Clean kernel"
cd ./release/kernel
make clean
rm -rf .config
rm -rf .config.old
rm -rf .version
rm -rf Module.symvers
rm -f ./include/asm
cd - > /dev/null


###########################################################
### Create the patch file
###########################################################
echo "Create patch file release/kernel_patch.tgz"
cd ./release
rm -rf ./linux-2.6.32.9
tar xzf ../sdk/os/linux-2.6.32.9.tar.gz
cd - > /dev/null
diff -Nur ./release/linux-2.6.32.9 ./release/kernel > ./release/kernel_patch/kernel.diff
rm -rf ./release/linux-2.6.32.9

cp -f sdk/bin/script/patch_kernel.sh ./release/kernel_patch
cd ./release
tar czf ./kernel_patch.tgz ./kernel_patch
cd - > /dev/null

ls -l ./release/kernel_patch.tgz
echo "Done."
