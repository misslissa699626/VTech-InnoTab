# Product general makefile
include $(TOPDIR)sdk/build/core/config.mak


# ###############################################################
# all
# ###############################################################
.PHONY: all
all: product

.PHONY: clean
clean: product_clean

# ###############################################################
# Copy sysconfig.h to local
# ###############################################################
sysconfig.h: $(PRODUCT_DIR)/config/sysconfig.h
	@$(CP) -f $< $@

# ###############################################################
# product
# ###############################################################
.PHONY: product
product: local_dir project platform sdk_install oImage firmware

.PHONY: product_clean
product_clean: project_clean platform_clean local_dir_clean oImage_clean firmware_clean

.PHONY: local_dir
local_dir:
	@$(MKDIR) -p rootfs
	@$(MKDIR) -p system

.PHONY: local_dir_clean
local_dir_clean:
	@-$(RM) -rf rootfs
	@-$(RM) -rf system

.PHONY: sdk_install
sdk_install:

#vicsiu
	@-$(RM) -rf $(PRODUCT_DIR)/system/bin/vtech
	$(MKDIR) -p $(PRODUCT_DIR)/system/bin/vtech/
	-$(CP) -R ../../vtech/bin/* $(PRODUCT_DIR)/system/bin/vtech/
	@-$(RM) -rf $(PRODUCT_DIR)/system/lib/vtech
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib/vtech/
	-$(CP) -R ../../vtech/lib/* $(PRODUCT_DIR)/system/lib/vtech/

	$(CP) -u $(SDK_DIR)/lib/libmd.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/
	$(CP) -u $(SDK_DIR)/lib/libfacedetect.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/

ifeq ($(SYSCONFIG_LUA), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lua
	$(CP) -u -R $(OUT_DIR)/lua/* $(PRODUCT_DIR)/system/lua/
endif

ifeq ($(SYSCONFIG_SDL), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
#	$(CP) -u $(OUT_SDK_DIR)/lib/libSDL.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/libSDL.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/
ifeq ($(SYSCONFIG_SDL_GFX), y)
	$(CP) -u $(OUT_SDK_DIR)/lib/libSDL_gfx.so $(PRODUCT_DIR)/system/lib/
endif
ifeq ($(SYSCONFIG_SDL_IMAGE), y)
	$(CP) -u $(OUT_SDK_DIR)/lib/libSDL_image.so $(PRODUCT_DIR)/system/lib/
endif
ifeq ($(SYSCONFIG_SDL_TTF), y)
	$(CP) -u $(OUT_SDK_DIR)/lib/libSDL_ttf.so $(PRODUCT_DIR)/system/lib/
endif
endif

ifeq ($(SYSCONFIG_LIBZ), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
#	$(CP) -u $(OUT_SDK_DIR)/lib/libz.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/libz.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/
endif

ifeq ($(SYSCONFIG_PNG), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libpng.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_LIBID3TAG), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libid3tag.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_JPEG), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libjpeg.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_TIFF), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libtiff.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_FREETYPE2), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libfreetype2.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_TS), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
#	$(CP) -u $(OUT_SDK_DIR)/lib/libts.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/libts.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/ts_dejitter.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/ts_input.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/ts_linear.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/ts_pthres.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(OUT_SDK_DIR)/lib/ts_variance.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_LZO), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/liblzo.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_EXPAT), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libexpat.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_AUDIOFILE), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libaudiofile.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_ESOUND), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(OUT_SDK_DIR)/lib/libesd.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_OPENVG), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(SDK_DIR)/lib/libOpenVG.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_CEVA), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
	$(CP) -u $(SDK_DIR)/lib/libceva.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_FLASH), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/bin/
	$(CP) -u $(SDK_DIR)/bin/flashplayer/$(SYSCONFIG_ARCH)/bin/flashplayer $(PRODUCT_DIR)/system/bin/flashplayer
	chmod +x $(PRODUCT_DIR)/system/bin/flashplayer
#vicsiu
	$(CP) -u $(SDK_DIR)/bin/flashplayer/$(SYSCONFIG_ARCH)/bin/MainEntry.swf $(PRODUCT_DIR)/system/bin/

	$(MKDIR) -p $(PRODUCT_DIR)/system/lib/
	-$(CP) -u $(SDK_DIR)/bin/flashplayer/$(SYSCONFIG_ARCH)/lib/* $(PRODUCT_DIR)/system/lib/
	$(MKDIR) -p $(PRODUCT_DIR)/system/flash/xse/
	-$(CP) -uR $(SDK_DIR)/lib/libflash/* $(PRODUCT_DIR)/system/flash/xse/
	chmod +x -R $(PRODUCT_DIR)/system/flash/xse/*
	$(MKDIR) -p $(PRODUCT_DIR)/system/flash/xse/cgi/services
#	-$(CP) -uR $(SDK_DIR)/bin/ebook/* $(PRODUCT_DIR)/system/flash/xse/cgi/services
#	-$(CP) -uR $(SDK_DIR)/bin/emu/common/* $(PRODUCT_DIR)/system/flash/xse/cgi/services
#	-$(CP) -f $(SDK_DIR)/bin/emu/$(SYSCONFIG_PLATFORM)/* $(PRODUCT_DIR)/system/flash/xse/cgi/services
	#$(MKDIR) -p $(PRODUCT_DIR)/system/usr/
	#$(CP) -R -u $(SDK_DIR)/bin/flashplayer/$(SYSCONFIG_ARCH)/usr/* $(PRODUCT_DIR)/system/usr/
	@-$(FIND) $(PRODUCT_DIR)/system/flash -iname ".svn" | xargs rm -rf
	$(CP) -u $(SDK_DIR)/lib/libflashjpeg.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(SDK_DIR)/lib/libImageIF.so $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_CODEC_VIDEO), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib/image
	-$(CP) -uR $(SDK_DIR)/lib/codec/$(SYSCONFIG_ARCH)/video/* $(PRODUCT_DIR)/system/lib/image/
endif

ifeq ($(SYSCONFIG_CODEC_IMAGE), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib/image
	-$(CP) -uR $(SDK_DIR)/lib/codec/$(SYSCONFIG_ARCH)/image/* $(PRODUCT_DIR)/system/lib/image/
endif

ifeq ($(SYSCONFIG_BTPLAY), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/btplay/
	$(CP) -u $(SDK_DIR)/bin/btplay/btplay $(PRODUCT_DIR)/system/btplay/btplay
	chmod +x $(PRODUCT_DIR)/system/btplay/btplay	
endif

ifeq ($(SYSCONFIG_PULSEAUDIO), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/bin/
	-$(CP) -uR $(SDK_DIR)/bin/pulseaudio/* $(PRODUCT_DIR)/system/bin/
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib/
	-$(CP) -uR $(SDK_DIR)/lib/pulseaudio/* $(PRODUCT_DIR)/system/lib/
endif

ifeq ($(SYSCONFIG_AUDIOMIXER), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/bin/
	-$(CP) -uR $(SDK_DIR)/bin/audiomixer/* $(PRODUCT_DIR)/system/bin/
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
#	$(CP) -u $(SDK_DIR)/lib/libaudiomixer.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(SDK_DIR)/lib/libaudiomixer.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/
endif

ifeq ($(SYSCONFIG_RESAMPLE), y)
	$(MKDIR) -p $(PRODUCT_DIR)/system/lib
#	$(CP) -u $(SDK_DIR)/lib/libresample.so $(PRODUCT_DIR)/system/lib/
	$(CP) -u $(SDK_DIR)/lib/libresample.so $(PROJECT_DIR)/../../common/GPL32900/rootfs/lib/
endif

# 
# Copy Audio Decoder Dynamic Libraries
# 
#ifeq ($(SYSCONFIG_AUDIO_DECODER), y)
	$(CP) -u $(SDK_DIR)/lib/audio/$(SYSCONFIG_ARCH)/*.so $(PRODUCT_DIR)/system/lib
#endif

	$(CP) -u $(SDK_DIR)/lib/libgpmcpvo.so $(PRODUCT_DIR)/system/lib
	$(CP) -u $(SDK_DIR)/lib/libgpmcpao.so $(PRODUCT_DIR)/system/lib

# ###############################################################
# Project
# ###############################################################
.PHONY: project
project:
	@$(ECHO) "  Building Project"
	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) all

.PHONY: project_clean
project_clean:
	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) clean

# ###############################################################
# Platform
# ###############################################################
.PHONY: platform
platform:
	@$(ECHO) "  Building Platform"
	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PLATFORM_DIR) all

.PHONY: platform_clean
platform_clean:
	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PLATFORM_DIR) clean

# ###############################################################
# Product initramfs
# ###############################################################
.PHONY: initramfs
initramfs: sysconfig.h
ifneq ($(SYSCONFIG_SIMULATOR), y)
	@$(ECHO) "  Create initramfs"
#	// cp initramfs.cpio from base.cpio
	@$(CP) -f $(TOPDIR)project/common/base/base.cpio $(PRODUCT_DIR)/initramfs.cpio

#	// cpio kernel modules
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -type d ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "modules.order" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "cramfs.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "squashfs.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_board.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_timer_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_pwm_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "board_config.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_apbdma0_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_i2c_bus_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_adc_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_dc2dc_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "nand_ecc.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "nand.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
ifneq ($(SYSCONFIG_MAINSTORAGE), None)
ifeq ($(SYSCONFIG_MAINSTORAGE), gp_usb_disk) #usb as main storage
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "usbcore.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "ohci-hcd.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "ehci-hcd.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "usb-storage.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "spmp_udc.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_usb.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
else
ifeq ($(SYSCONFIG_MAINSTORAGE), gp_nand_module) #NAND as main storage
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "nand_hal.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_nand_module.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
else #other storage device
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "$(SYSCONFIG_MAINSTORAGE).ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)	
endif
endif
endif

ifeq ($(SYSCONFIG_DISP0), y)
ifneq ($(SYSCONFIG_DISP0_PANEL), None)
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "$(SYSCONFIG_DISP0_PANEL).ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
endif
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "gp_display.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "tv_ntsc.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_fb.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
endif

ifeq ($(SYSCONFIG_DISP1), y)
ifneq ($(SYSCONFIG_DISP1_PANEL), None)
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "$(SYSCONFIG_DISP1_PANEL).ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
endif
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "gp_display1.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_fb1.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
endif

ifeq ($(SYSCONFIG_DISP2), y)
	@cd $(KERNEL_OUT_MODULES); \
	$(FIND) . -name "gp_display2.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "tv1.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO); \
	$(FIND) . -name "gp_fb2.ko" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
endif

	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) initramfs_rootfs
#	// cpio <PRODUCT_DIR>/rootfs/*
	@cd ./rootfs; \
	$(FIND) . ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)

#	// compress initramfs for kernel.bin
	@$(CAT) $(INITRAMFS_CPIO) | gzip > $(INITRAMFS_IMAGE)

	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) initramfs_system
#	// cpio <PRODUCT_DIR>/system
	@$(FIND) ./system ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)

#	// compress initramfs for oImage
	@$(CAT) $(INITRAMFS_CPIO) | gzip > $(INITRAMFS_OIMAGE)
	@$(LS) -l $(PRODUCT_DIR)/initramfs*
endif

# ###############################################################
# Images and initramfs
# ###############################################################
.PHONY: oImage
oImage: initramfs
ifneq ($(SYSCONFIG_SIMULATOR), y)
	@$(ECHO) "  Packing images"
#	// kernel cmdline
	@$(CP) -f $(PROJECT_DIR)/config/kernel_bootparam.txt $(PRODUCT_DIR)/cmdline.txt
	@$(ECHO) " $(SYSCONFIG_KERNEL_CMDLINE)" >> $(PRODUCT_DIR)/cmdline.txt
#	// pack kernel, initramfs_oImage, and cmdline
	@$(KERNEL_PACKER) kernel=$(KERNEL_OUT_IMAGE) initrd=$(INITRAMFS_OIMAGE) cmdline=$(PRODUCT_DIR)/cmdline.txt out=$(PRODUCT_DIR)/packed_oImage.bin
#	// concat u-boot(padding to 256KB) and packed_oImage.bin
	@$(PADDING) $(SDK_DIR)/prebuild/bootloader/u-boot_ram_$(SYSCONFIG_ARCH).bin $(PRODUCT_DIR)/oImage 262144
	@$(CAT) $(PRODUCT_DIR)/packed_oImage.bin >> $(PRODUCT_DIR)/oImage
	@$(LS) -l $(PRODUCT_DIR)/oImage

#	// pack kernel, initramfs, and cmdline
	@$(KERNEL_PACKER) kernel=$(KERNEL_OUT_IMAGE) initrd=$(INITRAMFS_IMAGE) cmdline=$(PRODUCT_DIR)/cmdline.txt out=$(PRODUCT_DIR)/packed.bin
#	// concat u-boot(padding to 256KB) and packed.bin
	@$(PADDING) $(SDK_DIR)/prebuild/bootloader/u-boot_ram_$(SYSCONFIG_ARCH).bin $(PRODUCT_DIR)/kernel.bin 262144
	@$(CAT) $(PRODUCT_DIR)/packed.bin >> $(PRODUCT_DIR)/kernel.bin
	@$(LS) -l $(PRODUCT_DIR)/kernel.bin
endif

.PHONY: oImage_clean
oImage_clean:
ifneq ($(SYSCONFIG_SIMULATOR), y)
#	// rm generated files
	@-$(RM) -f $(PRODUCT_DIR)/cmdline.txt
	@-$(RM) -f $(PRODUCT_DIR)/initramfs.cpio
	@-$(RM) -f $(PRODUCT_DIR)/initramfs_oImage.igz
	@-$(RM) -f $(PRODUCT_DIR)/packed_oImage.bin
	@-$(RM) -f $(PRODUCT_DIR)/oImage
	@-$(RM) -f $(PRODUCT_DIR)/initramfs.igz
	@-$(RM) -f $(PRODUCT_DIR)/packed.bin
	@-$(RM) -f $(PRODUCT_DIR)/kernel.bin
endif

.PHONY: firmware
firmware: 
ifneq ($(SYSCONFIG_SIMULATOR), y)
	@$(RM) -rf $(PRODUCT_DIR)/system_image
	@$(MKDIR) -p $(PRODUCT_DIR)/system_image

	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) system_image

#	// modules
	$(CP) -R -u $(KERNEL_OUT_MODULES)/* $(PRODUCT_DIR)/system_image

#	Temp fix error message when ./system no files	
	@$(MKDIR) -p ./system/app
	@-$(CP) -R -u ./system/* $(PRODUCT_DIR)/system_image

#	@-$(FIND) ./system_image -type d -regex ".*\.svn" -exec rm -rf {} \;
	@-$(FIND) ./system_image -iname ".svn" | xargs rm -rf

ifeq ($(SYSCONFIG_MAIN_FILESYSTEM), squashfs)
	@$(MKSQUASHFS) ./system_image ./system.bin -noappend -all-root
else
	@$(MKCRAMFS) ./system_image ./system.bin
endif
	@$(LS) -l $(PRODUCT_DIR)/system.bin	

	@$(FIRMWARE_PACKER) kernel=./kernel.bin system=./system.bin out=./firmware.bin	
	@$(LS) -l $(PRODUCT_DIR)/firmware.bin
	
	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) project_post_all
endif

.PHONY: firmware_clean
firmware_clean:
ifneq ($(SYSCONFIG_SIMULATOR), y)
#	// rm generated files
	@-$(RM) -rf $(PRODUCT_DIR)/system_image
	@-$(RM) -f $(PRODUCT_DIR)/system.bin
	@-$(RM) -f $(PRODUCT_DIR)/firmware.bin
endif

.PHONY: firmware_pack
firmware_pack:
ifneq ($(SYSCONFIG_SIMULATOR), y)
ifeq ($(SYSCONFIG_MAIN_FILESYSTEM), squashfs)
	@$(MKSQUASHFS) ./system_image ./system.bin -noappend -all-root
else
	@$(MKCRAMFS) ./system_image ./system.bin
endif
	@$(LS) -l $(PRODUCT_DIR)/system.bin	

	@$(FIRMWARE_PACKER) kernel=./kernel.bin system=./system.bin out=./firmware.bin	
	@$(LS) -l $(PRODUCT_DIR)/firmware.bin
	
	@$(MAKE) PRODUCT=$(PRODUCT) -C $(PROJECT_DIR) project_post_all
endif
