# ###############################################################
# Driver modules
# ###############################################################
obj-m += board_config.o
obj-m += key_driver.o
obj-m += touch_driver.o
#obj-m += external_rtc.o
obj-m += gp_joystick_driver.o

# ###############################################################
# Rules for build kernel modules
# ###############################################################
ifeq ($(KERNELRELEASE),)
TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

CFLAGS += -I$(SDK_DIR)/include
CFLAGS += -I$(PRODUCT_DIR)


.PHONY: all
all: sysconfig.h modules platform_common


.PHONY: clean
clean: modules_clean platform_common_clean
	@$(RM) -f sysconfig.h


# ###############################################################
# Copy sysconfig.h to local
# ###############################################################
sysconfig.h: $(PRODUCT_DIR)/config/sysconfig.h
	@$(CP) -f $< $@

# ###############################################################
# Common
# ###############################################################
#include $(BUILD_SYSTEM)/platform_common.mak
.PHONY: platform_common
platform_common:
	@$(MAKE) -C $(TOPDIR)platform/common/driver
	@$(MKDIR) -p $(KERNEL_OUT_MODULES)/lib/modules/common
ifneq ($(SYSCONFIG_DISP0_PANEL), None)
	@$(ECHO) "Display Panel: $(SYSCONFIG_DISP0_PANEL)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_DISP0_PANEL).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_DISP1_PANEL), None)
	@$(ECHO) "Display Panel: $(SYSCONFIG_DISP1_PANEL)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_DISP1_PANEL).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_SENSOR0), None)
	@$(ECHO) "Sensor Device: $(SYSCONFIG_SENSOR0)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_SENSOR0).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_SENSOR1), None)
	@$(ECHO) "Sensor Device: $(SYSCONFIG_SENSOR1)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_SENSOR1).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_SENSOR2), None)
	@$(ECHO) "Sensor Device: $(SYSCONFIG_SENSOR2)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_SENSOR2).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_TOUCHPAD_DEVICE), None)
	@$(ECHO) "TouchPad Device: $(SYSCONFIG_TOUCHPAD_DEVICE)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_TOUCHPAD_DEVICE).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_KEYBOARD_DEVICE), None)
	@$(ECHO) "Keyboard Device: $(SYSCONFIG_KEYBOARD_DEVICE)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_KEYBOARD_DEVICE).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_G_SENSOR_DEVICE), None)
	@$(ECHO) "G Sensor Device: $(SYSCONFIG_G_SENSOR_DEVICE)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_G_SENSOR_DEVICE).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif
ifneq ($(SYSCONFIG_PS2MOUSE_DEVICE), None)
	@$(ECHO) "PS2mouse Device: $(SYSCONFIG_PS2MOUSE_DEVICE)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_PS2MOUSE_DEVICE).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif

ifneq ($(SYSCONFIG_USB_WIFI), None)
	@$(ECHO) "USB-Wifi Device: $(SYSCONFIG_USB_WIFI)"
	@$(CP) -f $(OUT_DIR)/platform/common/driver/$(SYSCONFIG_USB_WIFI).ko $(KERNEL_OUT_MODULES)/lib/modules/common/
endif

.PHONY: platform_common_clean
platform_common_clean:
	@$(MAKE) -C $(TOPDIR)platform/common/driver clean
	@$(RM) -rf $(KERNEL_OUT_MODULES)/lib/modules/common

# ###############################################################
# Build modules for board_config
# ###############################################################
.PHONY: modules
modules:
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(shell pwd) modules
	@$(MKDIR) -p $(KERNEL_OUT_MODULES)/lib/modules
	@$(CP) -f *.ko $(KERNEL_OUT_MODULES)/lib/modules/

.PHONY: modules_clean
modules_clean:
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(shell pwd) clean
	@$(RM) -f $(KERNEL_OUT_MODULES)/lib/modules/*.ko

# ###############################################################
# initramfs
# ###############################################################
.PHONY: initramfs
initramfs:

.PHONY: initramfs_rootfs
initramfs_rootfs:

.PHONY: initramfs_system
initramfs_system:

endif
