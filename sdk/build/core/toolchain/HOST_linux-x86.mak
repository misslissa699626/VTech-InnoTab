HOST_TOOLS_PREFIX := 

HOST_CC      := $(HOST_TOOLS_PREFIX)gcc$(HOST_EXECUTABLE_SUFFIX)
HOST_CXX     := $(HOST_TOOLS_PREFIX)g++$(HOST_EXECUTABLE_SUFFIX)
HOST_AS      := $(HOST_TOOLS_PREFIX)as$(HOST_EXECUTABLE_SUFFIX)
HOST_AR      := $(HOST_TOOLS_PREFIX)ar$(HOST_EXECUTABLE_SUFFIX)
HOST_LD      := $(HOST_TOOLS_PREFIX)ld$(HOST_EXECUTABLE_SUFFIX)
HOST_NM      := $(HOST_TOOLS_PREFIX)nm$(HOST_EXECUTABLE_SUFFIX)
HOST_STRIP   := $(HOST_TOOLS_PREFIX)strip$(HOST_EXECUTABLE_SUFFIX)
HOST_OBJCOPY := $(HOST_TOOLS_PREFIX)objcopy$(HOST_EXECUTABLE_SUFFIX)
HOST_OBJDUMP := $(HOST_TOOLS_PREFIX)objdump$(HOST_EXECUTABLE_SUFFIX)
HOST_IMG2BIN := $(HOST_TOOLS_PREFIX)img2bin$(HOST_EXECUTABLE_SUFFIX)


#HOST_GLOBAL_CFLAGS += 
#HOST_GLOBAL_LDFLAGS +=


CPIO = $(TOP_DIR_FULL)/sdk/bin/util/cpio
MKCRAMFS = $(TOP_DIR_FULL)/sdk/bin/util/mkcramfs
MKSQUASHFS = $(TOP_DIR_FULL)/sdk/bin/util/mksquashfs
LUA = $(BIN_DIR)/lua/lua -e "package.path='$(BIN_DIR)/lua/?.lua';package.cpath='$(BIN_DIR)/lua/?.so'"

ECHO     = echo
CAT      = cat
CP       = cp
LS       = ls
RM       = rm
MV       = mv
MKDIR    = mkdir
RMDIR    = rmdir
GREP     = grep
FIND     = find
SED      = sed
CMP      = cmp
TAR      = tar
ZIP      = zip
XARGS    = xargs
SVN      = svn
SORT     = sort
SH       = $(SHELL)
TOUCH    = touch

KERNEL_PACKER = $(LUA) $(BIN_DIR)/script/kernel_packer.lua
PADDING       = $(LUA) $(BIN_DIR)/script/padding.lua
MKCONFIG      = $(LUA) $(BIN_DIR)/script/mkconfig.lua
MKSWITCH      = $(LUA) $(BIN_DIR)/script/mkswitch.lua
FIRMWARE_PACKER = $(LUA) $(BIN_DIR)/script/firmware_packer.lua
SYSTEM_PACKER = $(LUA) $(BIN_DIR)/script/system_packer.lua
KERNEL_CONFIG_SETUP = $(LUA) $(BIN_DIR)/script/kernel_config_setup.lua

