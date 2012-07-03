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

GOAL_INPUT     = ts_input
GOAL_PTHRES    = ts_pthres
GOAL_VARIANCE  = ts_variance
GOAL_DEJITTER  = ts_dejitter
GOAL_LINEAR    = ts_linear

TS_CONF = $(OUT_SDK_DIR)/include/libts
PLUGIN_DIR = $(OUT_SDK_DIR)/lib

CFLAGS  += -I../
CFLAGS  += -I../src
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2
CFLAGS  += -DHAVE_CONFIG_H -DGCC_HASCLASSVISIBILITY

LDFLAGS += -lm -ldl -lpthread -lts -L$(OUT_SDK_DIR)/lib

#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS_INPUT += input-raw.o
OBJS_PTHRES += pthres.o
OBJS_VARIANCE += variance.o
OBJS_DEJITTER += dejitter.o
OBJS_LINEAR += linear.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL_INPUT).a $(GOAL_INPUT).so $(GOAL_PTHRES).a $(GOAL_PTHRES).so $(GOAL_VARIANCE).a $(GOAL_VARIANCE).so $(GOAL_DEJITTER).a $(GOAL_DEJITTER).so $(GOAL_LINEAR).a $(GOAL_LINEAR).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL_INPUT).a $(OUT_SDK_DIR)/lib/$(GOAL_INPUT).a
	@$(CP) -u $(GOAL_INPUT).so $(OUT_SDK_DIR)/lib/$(GOAL_INPUT).so
	@$(CP) -u $(GOAL_PTHRES).a $(OUT_SDK_DIR)/lib/$(GOAL_PTHRES).a
	@$(CP) -u $(GOAL_PTHRES).so $(OUT_SDK_DIR)/lib/$(GOAL_PTHRES).so
	@$(CP) -u $(GOAL_VARIANCE).a $(OUT_SDK_DIR)/lib/$(GOAL_VARIANCE).a
	@$(CP) -u $(GOAL_VARIANCE).so $(OUT_SDK_DIR)/lib/$(GOAL_VARIANCE).so
	@$(CP) -u $(GOAL_DEJITTER).a $(OUT_SDK_DIR)/lib/$(GOAL_DEJITTER).a
	@$(CP) -u $(GOAL_DEJITTER).so $(OUT_SDK_DIR)/lib/$(GOAL_DEJITTER).so
	@$(CP) -u $(GOAL_LINEAR).a $(OUT_SDK_DIR)/lib/$(GOAL_LINEAR).a
	@$(CP) -u $(GOAL_LINEAR).so $(OUT_SDK_DIR)/lib/$(GOAL_LINEAR).so

$(GOAL_INPUT).a: $(OBJS_INPUT)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL_INPUT).a $(OBJS_INPUT)
	$(MSG_GOAL_OK)

$(GOAL_INPUT).so: $(OBJS_INPUT)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL_INPUT).so $(OBJS_INPUT)
	$(MSG_GOAL_OK)

$(GOAL_PTHRES).a: $(OBJS_PTHRES)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL_PTHRES).a $(OBJS_PTHRES)
	$(MSG_GOAL_OK)

$(GOAL_PTHRES).so: $(OBJS_PTHRES)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL_PTHRES).so $(OBJS_PTHRES)
	$(MSG_GOAL_OK)
$(GOAL_VARIANCE).a: $(OBJS_VARIANCE)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL_VARIANCE).a $(OBJS_VARIANCE)
	$(MSG_GOAL_OK)

$(GOAL_VARIANCE).so: $(OBJS_VARIANCE)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL_VARIANCE).so $(OBJS_VARIANCE)
	$(MSG_GOAL_OK)
$(GOAL_DEJITTER).a: $(OBJS_DEJITTER)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL_DEJITTER).a $(OBJS_DEJITTER)
	$(MSG_GOAL_OK)

$(GOAL_DEJITTER).so: $(OBJS_DEJITTER)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL_DEJITTER).so $(OBJS_DEJITTER)
	$(MSG_GOAL_OK)
$(GOAL_LINEAR).a: $(OBJS_LINEAR)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_AR) ru $(GOAL_LINEAR).a $(OBJS_LINEAR)
	$(MSG_GOAL_OK)

$(GOAL_LINEAR).so: $(OBJS_LINEAR)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(TARGET_CC) -shared $(LDFLAGS) -o $(GOAL_LINEAR).so $(OBJS_LINEAR)
	$(MSG_GOAL_OK)

.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_INPUT).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_INPUT).so
	$(RM) -f $(OBJS_INPUT:.o=.d) $(OBJS_INPUT) $(GOAL_INPUT).a $(GOAL_INPUT).so
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_PTHRES).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_PTHRES).so
	$(RM) -f $(OBJS_PTHRES:.o=.d) $(OBJS_PTHRES) $(GOAL_PTHRES).a $(GOAL_PTHRES).so
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_VARIANCE).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_VARIANCE).so
	$(RM) -f $(OBJS_VARIANCE:.o=.d) $(OBJS_VARIANCE) $(GOAL_VARIANCE).a $(GOAL_VARIANCE).so
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_DEJITTER).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_DEJITTER).so
	$(RM) -f $(OBJS_DEJITTER:.o=.d) $(OBJS_DEJITTER) $(GOAL_DEJITTER).a $(GOAL_DEJITTER).so
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_LINEAR).a
	$(RM) -f $(OUT_SDK_DIR)/lib/$(GOAL_LINEAR).so
	$(RM) -f $(OBJS_LINEAR:.o=.d) $(OBJS_LINEAR) $(GOAL_LINEAR).a $(GOAL_LINEAR).so

.PHONY: release
release:

-include $(OBJS_INPUT:.o=.d)
-include $(OBJS_PTHRES:.o=.d)
-include $(OBJS_VARIANCE:.o=.d)
-include $(OBJS_DEJITTER:.o=.d)
-include $(OBJS_LINEAR:.o=.d)
