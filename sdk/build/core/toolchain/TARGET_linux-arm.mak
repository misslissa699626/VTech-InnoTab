# ###############################################################
# Target Toolchain
# ###############################################################

TARGET_TOOLS_PREFIX := arm-none-linux-gnueabi-

TARGET_CC      := $(TARGET_TOOLS_PREFIX)gcc$(HOST_EXECUTABLE_SUFFIX)
TARGET_CXX     := $(TARGET_TOOLS_PREFIX)g++$(HOST_EXECUTABLE_SUFFIX)
TARGET_AS      := $(TARGET_TOOLS_PREFIX)as$(HOST_EXECUTABLE_SUFFIX)
TARGET_AR      := $(TARGET_TOOLS_PREFIX)ar$(HOST_EXECUTABLE_SUFFIX)
TARGET_LD      := $(TARGET_TOOLS_PREFIX)ld$(HOST_EXECUTABLE_SUFFIX)
TARGET_NM      := $(TARGET_TOOLS_PREFIX)nm$(HOST_EXECUTABLE_SUFFIX)
TARGET_STRIP   := $(TARGET_TOOLS_PREFIX)strip$(HOST_EXECUTABLE_SUFFIX)
TARGET_RANLIB  := $(TARGET_TOOLS_PREFIX)ranlib$(HOST_EXECUTABLE_SUFFIX)
TARGET_OBJCOPY := $(TARGET_TOOLS_PREFIX)objcopy$(HOST_EXECUTABLE_SUFFIX)
TARGET_OBJDUMP := $(TARGET_TOOLS_PREFIX)objdump$(HOST_EXECUTABLE_SUFFIX)
TARGET_IMG2BIN := $(TARGET_TOOLS_PREFIX)img2bin$(HOST_EXECUTABLE_SUFFIX)


# ###############################################################
# Kernel
# ###############################################################
# find out if kernel source exist
kernel_mk := $(strip $(wildcard $(TOPDIR)sdk/os/kernel-2.6.32/Makefile))
ifneq ($(kernel_mk),)
	KERNEL_SRC := $(patsubst %/,%,$(dir $(kernel_mk)))
	KERNEL_HEADERS := $(KERNEL_SRC)
else
	KERNEL_SRC :=
	kernel_mk := $(strip $(wildcard $(TOPDIR)sdk/os/kernel-2.6.32-headers/Makefile))
ifneq ($(kernel_mk),)
	KERNEL_HEADERS := $(patsubst %/,%,$(dir $(kernel_mk)))/$(SYSCONFIG_ARCH)
else
	KERNEL_HEADERS :=
endif
endif
KERNEL_PREBUILD_DIR := $(SDK_DIR)/os/kernel-2.6.32-prebuild/$(SYSCONFIG_ARCH)
KERNEL_PREBUILD_HEADERS := $(SDK_DIR)/os/kernel-2.6.32-headers

KERNEL_INCLUDE += -I$(KERNEL_HEADERS)/include
KERNEL_INCLUDE += -I$(KERNEL_HEADERS)/arch/arm/include

ifeq ($(SYSCONFIG_ARCH_SPMP8050),y)
KERNEL_INCLUDE += -I$(KERNEL_HEADERS)/arch/arm/mach-spmp8050/include
endif
ifeq ($(SYSCONFIG_ARCH_GPL32900),y)
KERNEL_INCLUDE += -I$(KERNEL_HEADERS)/arch/arm/mach-gpl32900/include
endif
ifeq ($(SYSCONFIG_ARCH_GPL64000),y)
KERNEL_INCLUDE += -I$(KERNEL_HEADERS)/arch/arm/mach-gpl64000/include
endif

# ###############################################################
# FLAGS
# ###############################################################

TARGET_GLOBAL_CFLAGS += \
			-std=gnu99 \
			$(KERNEL_INCLUDE) \
			-fpic \
#			-include $(global_config_h) \

ifeq ($(SYSCONFIG_ARCH_GPL32900),y)
TARGET_GLOBAL_CFLAGS += -march=armv6 -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=softfp
endif

TARGET_GLOBAL_CXXFLAGS += \
			$(KERNEL_INCLUDE) \
			-fpic \
#			-include $(global_config_h) \

ifeq ($(SYSCONFIG_ARCH_GPL32900),y)
TARGET_GLOBAL_CXXFLAGS += -march=armv6 -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=softfp
endif

TARGET_GLOBAL_LDFLAGS += \


TARGET_C_INCLUDES := \
	$(KERNEL_INCLUDE)



#TARGET_NO_UNDEFINED_LDFLAGS := -Wl,--no-undefined
#TARGET_arm_CFLAGS :=
#TARGET_thumb_CFLAGS := 

