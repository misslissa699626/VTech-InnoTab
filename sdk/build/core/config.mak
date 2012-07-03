# This is included by the top-level Makefile.
# It sets up standard variables based on the
# current configuration and platform, which
# are not specific to what is being built.

# Only use ANDROID_BUILD_SHELL to wrap around bash.
# DO NOT use other shells such as zsh.
ifdef ANDROID_BUILD_SHELL
SHELL := $(ANDROID_BUILD_SHELL)
else
# Use bash, not whatever shell somebody has installed as /bin/sh
# This is repeated from main.mk, since envsetup.sh runs this file
# directly.
SHELL := /bin/bash
endif

TOP_DIR_FULL := $(shell pwd)/$(TOPDIR)

BUILD_SYSTEM := $(TOPDIR)sdk/build/core

ifeq ($(PRODUCT),)
-include $(TOPDIR)product_config.mak
endif
-include $(TOPDIR)out/$(PRODUCT)/config/sysconfig.mak

PRODUCT_DIR := $(TOPDIR)out/$(PRODUCT)
PRODUCT_INSTALL_DIR := $(TOPDIR)out/$(PRODUCT)/_install
PRODUCT_DIR_FULL := $(TOP_DIR_FULL)out/$(PRODUCT)
PROJECT_DIR := $(TOPDIR)$(SYSCONFIG_PROJECT_DIR)
PLATFORM_DIR := $(TOPDIR)$(SYSCONFIG_PLATFORM_DIR)


# ###############################################################
# Include sub-configuration files
# ###############################################################

# ---------------------------------------------------------------
# Try to include buildspec.mk, which will try to set stuff up.
# If this file doesn't exist, the environemnt variables will
# be used, and if that doesn't work, then the default is an
# arm build
#-include $(TOPDIR)buildspec.mak

# ---------------------------------------------------------------
# Define most of the global variables.  These are the ones that
# are specific to the user's build configuration.
#include $(BUILD_SYSTEM)/envsetup.mak

# Bring in standard build system definitions.
#include $(BUILD_SYSTEM)/definitions.mk


# ###############################################################
# Dir
# ###############################################################
OUT_DIR := $(TOPDIR)out
OUT_SDK_DIR := $(PRODUCT_DIR)/sdk
OUT_SDK_DIR_FULL := $(PRODUCT_DIR_FULL)/sdk

OUT_TARGET_DIR := $(TOPDIR)out/target
SDK_DIR := $(TOPDIR)sdk
BIN_DIR := $(TOPDIR)sdk/bin
OUT_DIR_FULL := $(TOP_DIR_FULL)/out


# ###############################################################
# Build system internal files
# ###############################################################
CLEAR_VARS:= $(BUILD_SYSTEM)/clear_vars.mak
BUILD_SYSTEM_APP:= $(BUILD_SYSTEM)/system_app.mak
BUILD_EXECUTABLE:= $(BUILD_SYSTEM)/executable.mak
BUILD_HOST_STATIC_LIBRARY:= $(BUILD_SYSTEM)/host_static_library.mak
BUILD_HOST_SHARED_LIBRARY:= $(BUILD_SYSTEM)/host_shared_library.mak
BUILD_STATIC_LIBRARY:= $(BUILD_SYSTEM)/static_library.mak
BUILD_RAW_STATIC_LIBRARY := $(BUILD_SYSTEM)/raw_static_library.mak
BUILD_SHARED_LIBRARY:= $(BUILD_SYSTEM)/shared_library.mak
BUILD_RAW_EXECUTABLE:= $(BUILD_SYSTEM)/raw_executable.mak
BUILD_HOST_EXECUTABLE:= $(BUILD_SYSTEM)/host_executable.mak
BUILD_PACKAGE:= $(BUILD_SYSTEM)/package.mak
BUILD_HOST_PREBUILT:= $(BUILD_SYSTEM)/host_prebuilt.mak
BUILD_PREBUILT:= $(BUILD_SYSTEM)/prebuilt.mak
BUILD_MULTI_PREBUILT:= $(BUILD_SYSTEM)/multi_prebuilt.mak
BUILD_JAVA_LIBRARY:= $(BUILD_SYSTEM)/java_library.mak
BUILD_STATIC_JAVA_LIBRARY:= $(BUILD_SYSTEM)/static_java_library.mak
BUILD_HOST_JAVA_LIBRARY:= $(BUILD_SYSTEM)/host_java_library.mak
BUILD_DROIDDOC:= $(BUILD_SYSTEM)/droiddoc.mak
BUILD_COPY_HEADERS := $(BUILD_SYSTEM)/copy_headers.mak
BUILD_KEY_CHAR_MAP := $(BUILD_SYSTEM)/key_char_map.mak

# ###############################################################
# Host Toolchain
# ###############################################################
ifneq ($(SYSCONFIG_HOST),)
include $(BUILD_SYSTEM)/toolchain/HOST_$(SYSCONFIG_HOST).mak
else
include $(BUILD_SYSTEM)/toolchain/HOST_linux-x86.mak
endif

# ###############################################################
# Target Toolchain
# ###############################################################
ifneq ($(SYSCONFIG_HOST),)
include $(BUILD_SYSTEM)/toolchain/TARGET_$(SYSCONFIG_TARGET).mak
else
include $(BUILD_SYSTEM)/toolchain/TARGET_linux-arm.mak
endif

# ###############################################################
# Kernel
# ###############################################################
ifneq ($(KERNEL_SRC),)
KERNEL_BUILD_IMAGE := $(KERNEL_SRC)/arch/arm/boot/Image
KERNEL_OUT_DIR := $(OUT_DIR)/kernel
KERNEL_OUT_IMAGE := $(KERNEL_OUT_DIR)/Image
KERNEL_OUT_MODULES := $(KERNEL_OUT_DIR)/modules_install
else
KERNEL_OUT_IMAGE := $(KERNEL_PREBUILD_DIR)/Image
KERNEL_OUT_MODULES := $(KERNEL_PREBUILD_DIR)/modules_install
endif

INITRAMFS_CPIO := $(PRODUCT_DIR_FULL)/initramfs.cpio
INITRAMFS_IMAGE := $(PRODUCT_DIR_FULL)/initramfs.igz
INITRAMFS_OIMAGE := $(PRODUCT_DIR_FULL)/initramfs_oImage.igz

# ###############################################################
# Rules
# ###############################################################

# -- Common Message, please don't remove. --
MSG_SPLIT_LINE =
MSG_COMPILING  = @$(ECHO) "  CC <$<>"
MSG_COMPILING_CXX = @$(ECHO) "  CXX <$<>"
MSG_ARCHIVE    = @$(ECHO) "  AR <$@>"
MSG_LINKING    = @$(ECHO) "  LD <$@>"
MSG_GOAL_OK    = @$(ECHO) "***" $(GOAL) "is built successfully! ***"


COMMAND =	@set fnord $$MAKEFLAGS; amf=$$2; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  if test -f $$subdir/Makefile; \
	  then \
		  echo "Making $$target in $$subdir"; \
		  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$target) \
		   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	  fi; \
	done;


#---------------------------------------------------------------------------
# Implicit rules
#---------------------------------------------------------------------------

ifeq ('$(SHOW_COMMAND)', 'yes')
	NO_SHOW = 
else
	NO_SHOW = @
endif

.c.o:
	$(MSG_SPLIT_LINE)
	$(MSG_COMPILING)
	$(NO_SHOW)$(TARGET_CC) $(TARGET_GLOBAL_CFLAGS) $(CFLAGS) -MM -MT $@ -o $*.d $<
	@cp $*.d $*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
			-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
		rm -f $*.d
	$(NO_SHOW)$(TARGET_CC) -c $(TARGET_GLOBAL_CFLAGS) $(CFLAGS) -o $*.o $<

.S.o:
	$(MSG_SPLIT_LINE)
	$(MSG_COMPILING)
	$(NO_SHOW)$(TARGET_CC) $(TARGET_GLOBAL_CFLAGS) $(CFLAGS) -MM -MT $@ -o $*.d $<
	@cp $*.d $*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
			-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
		rm -f $*.d
	$(NO_SHOW)$(TARGET_CC) -c $(ASFLAGS) $(TARGET_GLOBAL_CFLAGS) $(CFLAGS) -o $*.o $<

.cpp.o:
	$(MSG_SPLIT_LINE)
	$(MSG_COMPILING_CXX)
	$(NO_SHOW)$(TARGET_CXX) $(TARGET_GLOBAL_CXXFLAGS) $(CFLAGS) -MM -MT $@ -o $*.d $<
	@cp $*.d $*.P; \
		sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
			-e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
		rm -f $*.d
	$(NO_SHOW)$(TARGET_CXX) -c $(TARGET_GLOBAL_CXXFLAGS) $(CFLAGS) -o $*.o $<
