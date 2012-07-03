#/**************************************************************************
# *                                                                        *
# *         Copyright (c) 2002 by Sunplus mMedia Technology Co., Ltd.      *
# *                                                                        *
# *  This software is copyrighted by and is the property of Sunplus        *
# *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
# *  Co., Ltd. This software may only be used in accordance with the       *
# *  corresponding license agreement. Any unauthorized use, duplication,   *
# *  distribution, or disclosure of this software is expressly forbidden.  *
# *                                                                        *
# *  This Copyright notice MUST not be removed or modified without prior   *
# *  written consent of Sunplus Technology Co., Ltd.                       *
# *                                                                        *
# *  Sunplus mMedia Technology Co., Ltd. reserves the right to modify this *
# *  software without notice.                                              *
# *                                                                        *
# *  Sunplus mMedia Technology Co., Ltd.                                   *
# *  19, Innovation First Road, Science-Based Industrial Park,             *
# *  Hsin-Chu, Taiwan, R.O.C.                                              *
# *                                                                        *
# *  Author: Roger Hsu                                                     *
# *                                                                        *
# **************************************************************************/
TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

GOAL     = libjpeg

CFLAGS  += -I./
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2 

SYSDEPMEM= jmemnobs.o

HDRS = jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h jversion.h jmemsys.h


#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
# library object files common to compression and decompression
OBJS += \
	jaricom.o \
	jcomapi.o \
	jutils.o \
	jerror.o \
	jmemmgr.o \
	$(SYSDEPMEM)

# compression library object files
OBJS += \
	jcapimin.o \
	jcapistd.o \
	jcarith.o \
	jctrans.o \
	jcparam.o \
	jdatadst.o \
	jcinit.o \
	jcmaster.o \
	jcmarker.o \
	jcmainct.o \
	jcprepct.o \
	jccoefct.o \
	jccolor.o \
	jcsample.o \
	jchuff.o \
	jcdctmgr.o \
	jfdctfst.o \
	jfdctflt.o \
	jfdctint.o

# decompression library object files
OBJS += \
	jdapimin.o \
	jdapistd.o \
	jdarith.o \
	jdtrans.o \
	jdatasrc.o \
	jdmaster.o \
	jdinput.o \
	jdmarker.o \
	jdhuff.o \
	jdmainct.o \
	jdcoefct.o \
	jdpostct.o \
	jddctmgr.o \
	jidctfst.o \
	jidctflt.o \
	jidctint.o \
	jdsample.o \
	jdcolor.o \
	jquant1.o \
	jquant2.o \
	jdmerge.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/libjpeg
	@for file in $(HDRS); do \
		cp -u ./$$file $(OUT_SDK_DIR)/include/libjpeg/$$file; \
	done
	
$(GOAL).a: $(OBJS)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL).a $(OBJS)
	$(MSG_GOAL_OK)

$(GOAL).so: $(OBJS)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL).so $(OBJS)
	$(MSG_GOAL_OK)

.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL).so
	$(RM) -f $(OBJS:.o=.d) $(OBJS) $(GOAL).a $(GOAL).so

.PHONY: release
release:

-include $(OBJS:.o=.d)
