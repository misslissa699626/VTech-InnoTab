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

GOAL     = libaudiofile

CFLAGS  += -I.
CFLAGS  += -I./libaudiofile
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2 
CFLAGS  += -DHAVE_CONFIG_H

HDRS = af_vfs.h audiofile.h aupvlist.h

#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += \
	./libaudiofile/openclose.o \
	./libaudiofile/setup.o \
	./libaudiofile/format.o \
	./libaudiofile/data.o \
	./libaudiofile/pcm.o \
	./libaudiofile/error.o \
	./libaudiofile/byteorder.o \
	./libaudiofile/af_vfs.o \
	./libaudiofile/util.o \
	./libaudiofile/debug.o \
	./libaudiofile/aupv.o \
	./libaudiofile/units.o \
	./libaudiofile/compression.o \
	./libaudiofile/aes.o \
	./libaudiofile/instrument.o \
	./libaudiofile/loop.o \
	./libaudiofile/marker.o \
	./libaudiofile/misc.o \
	./libaudiofile/track.o \
	./libaudiofile/query.o \
	./libaudiofile/raw.o \
	./libaudiofile/aiff.o \
	./libaudiofile/aiffwrite.o \
	./libaudiofile/extended.o \
	./libaudiofile/next.o \
	./libaudiofile/nextwrite.o \
	./libaudiofile/wave.o \
	./libaudiofile/wavewrite.o \
	./libaudiofile/ircam.o \
	./libaudiofile/ircamwrite.o \
	./libaudiofile/avr.o \
	./libaudiofile/avrwrite.o \
	./libaudiofile/iff.o \
	./libaudiofile/iffwrite.o \
	./libaudiofile/nist.o \
	./libaudiofile/nistwrite.o \
	./libaudiofile/g711.o \
	./libaudiofile/modules.o \
	./libaudiofile/modules/g711.o \
	./libaudiofile/modules/pcm.o \
	./libaudiofile/modules/msadpcm.o \
	./libaudiofile/modules/ima.o \
	./libaudiofile/modules/adpcm.o \
	./libaudiofile/modules/rebuffer.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/libaudiofile
	@for file in $(HDRS); do \
		cp -u ./libaudiofile/$$file $(OUT_SDK_DIR)/include/libaudiofile/$$file; \
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
