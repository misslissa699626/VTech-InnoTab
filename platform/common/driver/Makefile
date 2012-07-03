# ###############################################################
# Driver modules
# ###############################################################
obj-m += lcd/
obj-m += lcm/
obj-m += sensor/
obj-m += lcd1/
obj-m += lcm1/
obj-m += gsensor/
obj-m += touchpad/
obj-m += keyboard/
obj-m += mouse/
#obj-m += usb_wifi/

# ###############################################################
# Rules for build kernel modules
# ###############################################################
ifeq ($(KERNELRELEASE),)
TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

USBWIFI_DIR = usb_wifi
COMPAT_DIR = compat-wireless-2.6.39-1

.PHONY: all
all: modules

.PHONY: clean
clean: modules_clean

.PHONY: modules
modules:
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(shell pwd) modules
	@$(MKDIR) -p $(TOPDIR)out/platform/common/driver
	@$(FIND) . -type f -regex ".*\.ko$$" ! -regex ".*\.svn.*" -exec $(CP) -u -f {} $(TOPDIR)out/platform/common/driver/ \;
ifeq ($(SYSCONFIG_USB_WIFI), AR9271)
	# compat wireless driver build 
	usb_wifi/make_compat.sh make
	# copy all wifi driver to output folder
	-mkdir $(TOPDIR)out/platform/common/driver/compat_wireless
	@$(FIND) $(USBWIFI_DIR)/$(COMPAT_DIR) -name *.ko -exec $(CP) -u -f {} $(TOPDIR)out/platform/common/driver/compat_wireless/ \;
endif

.PHONY: modules_clean
modules_clean:
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(shell pwd) clean
	@$(RM) -rf $(TOPDIR)out/platform/common/driver
ifeq ($(SYSCONFIG_USB_WIFI), AR9271)
	$(USBWIFI_DIR)/make_compat.sh clean 
	@-$(RM) -rf $(TOPDIR)out/platform/common/driver/compat_wireless
endif

endif
