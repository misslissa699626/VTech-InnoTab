TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

SUBDIRS += ../../common/sysinit
SUBDIRS += ../../common/showlogo
SUBDIRS += app/esound
SUBDIRS += app/id3tag
SUBDIRS += app/driver
SUBDIRS += app/pulseaudio
#SUBDIRS += app/audiomixer
SUBDIRS += app/middleware
SUBDIRS += app/ts
SUBDIRS += app/pwm_init

ifeq ($(SYSCONFIG_PPU),y)
SUBDIRS += app/ppu
endif

.PHONY: all
all:
	+$(COMMAND)

.PHONY: clean
clean:
	+$(COMMAND)

.PHONY: release
release:
	+$(COMMAND)

.PHONY: project_post_all
project_post_all:


# ###############################################################
# initramfs
# ###############################################################
.PHONY: initramfs
initramfs: initramfs_rootfs initramfs_system


#.PHONY: initramfs_rootfs
initramfs_rootfs:
#	// cpio common/rootfs/*
	@chmod a+x $(TOPDIR)project/common/$(SYSCONFIG_ARCH)/rootfs/usr/bin/*
	@cd $(TOPDIR)project/common/$(SYSCONFIG_ARCH)/rootfs; \
	$(FIND) . ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)

#	// cpio <PROJECT_DIR>/rootfs/*
	@cd ./rootfs; \
	$(FIND) . ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)
	
.PHONY: initramfs_system
initramfs_system:
#	// cpio common/system
	@cd $(TOPDIR)project/common; \
	$(FIND) ./system ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)

#	// cpio <PROJECT_DIR>/system
	@$(CP) -R -u $(KERNEL_OUT_MODULES)/* ./system
	@$(FIND) ./system ! -regex ".*\.svn.*" | $(CPIO) -H newc -o --owner root:root --append -F $(INITRAMFS_CPIO)

# ###############################################################
# System Image
# ###############################################################
.PHONY: system_image
system_image:
	@cp -R -u $(TOPDIR)project/common/system/* $(PRODUCT_DIR)/system_image
	@cp -R -u ./system/* $(PRODUCT_DIR)/system_image
