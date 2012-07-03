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

GOAL     = libSDL_image
TEST_GOAL = showimage

CFLAGS  += -I.
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -I$(OUT_SDK_DIR)/include/SDL
CFLAGS  += -I$(OUT_SDK_DIR)/include/libz
CFLAGS  += -I$(OUT_SDK_DIR)/include/libpng
CFLAGS  += -I$(OUT_SDK_DIR)/include/libjpeg
CFLAGS  += -I$(OUT_SDK_DIR)/include/libtiff
CFLAGS  += -g -O2 
CFLAGS  += -D_GNU_SOURCE=1 -D_REENTRANT

CFLAGS  += -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DLT_OBJDIR=\".libs/\" -DPACKAGE=\"SDL_image\" -DVERSION=\"1.2.10\"
CFLAGS  += -DLOAD_BMP=1 -DLOAD_GIF=1 -DLOAD_LBM=1 -DLOAD_PCX=1 -DLOAD_PNM=1 -DLOAD_TGA=1 -DLOAD_XCF=1 -DLOAD_XPM=1 -DLOAD_XV=1
CFLAGS  += -DLOAD_PNG=1 -DLOAD_PNG_DYNAMIC=\"libpng.so\"
CFLAGS  += -DLOAD_JPG=1 -DLOAD_JPG_DYNAMIC=\"libjpeg.so\"
CFLAGS  += -DLOAD_TIF=1 -DLOAD_TIF_DYNAMIC=\"libtiff.so\"

LDFLAGS += -lm -ldl -lpthread -L$(OUT_SDK_DIR)/lib

HDRS = SDL_image.h


#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += \
	IMG.o \
	IMG_bmp.o \
	IMG_gif.o \
	IMG_jpg.o \
	IMG_lbm.o \
	IMG_pcx.o \
	IMG_png.o \
	IMG_pnm.o \
	IMG_tga.o \
	IMG_tif.o \
	IMG_xcf.o \
	IMG_xpm.o \
	IMG_xv.o \
	IMG_ImageIO.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/SDL
	@for file in $(HDRS); do \
		cp -u ./$$file $(OUT_SDK_DIR)/include/SDL/$$file; \
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
	
test: $(TEST_GOAL)
	@echo make $(TEST_GOAL) finish!

$(TEST_GOAL): $(TEST_GOAL).o
	@$(TARGET_CC) $(CFLAGS) -L$(OUT_SDK_DIR)/lib -lSDL -lSDL_image -lts -o $(TEST_GOAL) $(TEST_GOAL).o
	
.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL).so
	$(RM) -f $(OBJS:.o=.d) $(OBJS) $(GOAL).a $(GOAL).so
	$(RM) -f $(TEST_GOAL) $(TEST_GOAL).o

.PHONY: release
release:

-include $(OBJS:.o=.d)
