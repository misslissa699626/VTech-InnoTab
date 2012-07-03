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

GOAL     = libfreetype2

AUTOFIT = src/autofit
BASE = src/base
BDF = src/bdf
CACHE = src/cache
CID = src/cid
CFF = src/cff
GZIP = src/gzip
LZW = src/lzw
PCF = src/pcf
PFR = src/pfr
PSAUX = src/psaux
PSHINTER = src/pshinter
PSNAMES = src/psnames
RASTER = src/raster
SFNT = src/sfnt
SMOOTH = src/smooth
TRUETYPE = src/truetype
TYPE1 = src/type1
TYPE42 = src/type42
WINFONTS = src/winfonts

CFLAGS  += -I./include
CFLAGS  += -I./include/freetype/config
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2
CFLAGS  += -DFT_CONFIG_CONFIG_H="<ftconfig.h>" -DFT2_BUILD_LIBRARY -DFT_CONFIG_MODULES_H="<ftmodule.h>"

HDRS = 


#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += \
	$(BASE)/ftbase.o \
	$(BASE)/ftsystem.o \
	$(BASE)/ftdebug.o \
	$(BASE)/ftinit.o \
	$(BASE)/ftbbox.o \
	$(BASE)/ftglyph.o \
	$(BASE)/ftbdf.o \
	$(BASE)/ftbitmap.o \
	$(BASE)/ftcid.o \
	$(BASE)/ftfstype.o \
	$(BASE)/ftgasp.o \
	$(BASE)/ftgxval.o \
	$(BASE)/ftlcdfil.o \
	$(BASE)/ftmm.o \
	$(BASE)/ftotval.o \
	$(BASE)/ftpatent.o \
	$(BASE)/ftpfr.o \
	$(BASE)/ftstroke.o \
	$(BASE)/ftsynth.o \
	$(BASE)/fttype1.o \
	$(BASE)/ftwinfnt.o \
	$(BASE)/ftxf86.o \
	$(BDF)/bdf.o \
	$(CACHE)/ftcache.o \
	$(CFF)/cff.o \
	$(CID)/type1cid.o \
	$(GZIP)/ftgzip.o \
	$(LZW)/ftlzw.o \
	$(PCF)/pcf.o \
	$(PFR)/pfr.o \
	$(PSAUX)/psaux.o \
	$(PSHINTER)/pshinter.o \
	$(PSNAMES)/psnames.o \
	$(RASTER)/raster.o \
	$(SFNT)/sfnt.o \
	$(SMOOTH)/smooth.o \
	$(TRUETYPE)/truetype.o \
	$(TYPE1)/type1.o \
	$(TYPE42)/type42.o \
	$(WINFONTS)/winfnt.o \
	$(AUTOFIT)/autofit.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/libfreetype2
	@for file in $(HDRS); do \
		cp -u ./$$file $(OUT_SDK_DIR)/include/libfreetype2/$$file; \
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
