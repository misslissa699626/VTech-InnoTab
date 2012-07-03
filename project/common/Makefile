
FDK_DIR := ./fdk

ifdef COMSPEC  # Win32
LUA     = $(FDK_DIR)/bin/lua/lua5.1.exe
else  # linux
LUA     = $(FDK_DIR)/bin/lua/lua -e "package.path='$(FDK_DIR)/bin/lua/?.lua';package.cpath='$(FDK_DIR)/bin/lua/?.so'"
endif


KERNEL_PATH = ..
KERNEL_IMAGE = $(KERNEL_PATH)/arch/arm/boot/Image


all:



.PHONY: config
config:
	@$(LUA) -e "package.path='$(FDK_DIR)/bin/lua/?.lua';package.cpath='$(FDK_DIR)/bin/lua/?.so'" \
		$(FDK_DIR)/bin/mkconfig/mkconfig.lua --non-interactive

.PHONY: kernel
kernel:
	make -C ../ Image
	ls -l $(KERNEL_IMAGE)
	rm -rf ./out_kernel
	mkdir ./out_kernel
	make -C ../ modules_install INSTALL_MOD_PATH=./opensys/out_kernel

.PHONY: oImage
oImage: kernel
	cp $(FDK_DIR)/config/base.cpio ./initramfs.cpio
	cd $(FDK_DIR)/rootfs; \
	find . | cpio -H newc -o --owner root:root --append -F ../../initramfs.cpio	
	cd ./out_kernel; \
	find . | cpio -H newc -o --owner root:root --append -F ../initramfs.cpio
#	rm -rf ./out_kernel
	
	cat initramfs.cpio | gzip > initramfs.igz
	ls -l initramfs.*
	rm initramfs.cpio

	@$(LUA)	$(FDK_DIR)/bin/script/kernel_packer.lua kernel=$(KERNEL_IMAGE) initrd=initramfs.igz cmdline=$(FDK_DIR)/config/cmdline.txt out=packed.bin
	rm initramfs.igz
	@$(LUA) $(FDK_DIR)/bin/script/padding.lua $(FDK_DIR)/config/u-boot.bin $(KERNEL_PATH)/oImage 262144
	@cat packed.bin >> $(KERNEL_PATH)/oImage
	@rm packed.bin
	ls -l $(KERNEL_PATH)/oImage


.PHONY: openfs
openfs:
	sudo rm -rf ../rootfs
	mkdir ../rootfs
	cd ../rootfs; \
	sudo cpio -i -d -H newc -F ../opensys/fdk/config/base.cpio --no-absolute-filenames
	sudo cp -R ./fdk/rootfs/* ../rootfs/
	sudo cp -R ./fdk/system ../rootfs/


.PHONY: initramfs
initramfs:
	cp $(FDK_DIR)/config/base.cpio ./initramfs.cpio
	cd $(FDK_DIR)/rootfs; \
	find . | cpio -H newc -o --owner root:root --append -F ../../initramfs.cpio
	cat initramfs.cpio | gzip > initramfs.igz
	ls -l initramfs.*
	rm initramfs.cpio

	
unpack-initramfs:
	mkdir unpack
	cd unpack; \
	sudo cpio -i -d -H newc -F ../initramfs.cpio --no-absolute-filenames

	
dev.cpio:
	sudo mkdir dev
	sudo mknod dev/console c 5 1
	sudo mknod dev/null c 1 3
	find ./dev | cpio -H newc -o --owner root:root -F dev.cpio
	sudo rm -rf dev/

	
base.cpio:
	tar xjf dl/busybox-1.17.3.tar.bz2
	cp config/busybox/config-1.17.3 busybox-1.17.3/.config

	cd busybox-1.17.3; \
	make CROSS_COMPILE=arm-none-linux-gnueabi-; \
	make install CROSS_COMPILE=arm-none-linux-gnueabi-

	sudo mkdir busybox-1.17.3/_install/dev
	sudo mknod busybox-1.17.3/_install/dev/console c 5 1
	sudo mknod busybox-1.17.3/_install/dev/null c 1 3

	cd busybox-1.17.3/_install; \
	ln -s bin/busybox init; \
	find . | cpio -H newc -o --owner root:root -F ../../base.cpio

	sudo rm -rf busybox-1.17.3/_install/dev

