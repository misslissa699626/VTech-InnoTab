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
TOPDIR := ../../../../
include $(TOPDIR)sdk/build/core/config.mak

ZLIBINC = ../../zlib-1.2.5
LIBJPEGINC = ../../jpeg-7

GOAL     = libtiff

CFLAGS  += -I./
CFLAGS  += -I./$(ZLIBINC)
CFLAGS  += -I./$(LIBJPEGINC)
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2 

#LIBS = -lz -lm

HDRS = tiff.h tiffconf.h tiffio.h tiffvers.h tif_config.h


#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += \
	tif_aux.o \
	tif_close.o \
	tif_codec.o \
	tif_color.o \
	tif_compress.o \
	tif_dir.o \
	tif_dirinfo.o \
	tif_dirread.o \
	tif_dirwrite.o \
	tif_dumpmode.o \
	tif_error.o \
	tif_extension.o \
	tif_fax3.o \
	tif_fax3sm.o \
	tif_flush.o \
	tif_getimage.o \
	tif_jpeg.o \
	tif_luv.o \
	tif_lzw.o \
	tif_next.o \
	tif_ojpeg.o \
	tif_open.o \
	tif_packbits.o \
	tif_pixarlog.o \
	tif_predict.o \
	tif_print.o \
	tif_read.o \
	tif_strip.o \
	tif_swab.o \
	tif_thunder.o \
	tif_tile.o \
	tif_unix.o \
	tif_version.o \
	tif_warning.o \
	tif_write.o \
	tif_zip.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/libtiff
	@for file in $(HDRS); do \
		cp -u ./$$file $(OUT_SDK_DIR)/include/libtiff/$$file; \
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
