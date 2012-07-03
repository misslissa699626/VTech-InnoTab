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
# *  Author: clhuang                                                       *
# *                                                                        *
# **************************************************************************/
TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

GOAL     = libcsi.a

CFLAGS += -I$(SDK_DIR)/include
CFLAGS += -I.
CFLAGS += -g

#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += csi_driver.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL)
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(SDK_DIR)/lib
	@$(CP) -u $(GOAL) $(SDK_DIR)/lib/$(GOAL)

$(GOAL): $(OBJS)
	$(MSG_SPLIT_LINE)
	$(MSG_ARCHIVE)
	@$(AR) ru $(GOAL) $(OBJS)
	$(MSG_GOAL_OK)

.PHONY: clean
clean:
	$(MSG_SPLIT_LINE)
	$(RM) -f $(SDK_DIR)/lib/$(GOAL)
	$(RM) -f $(OBJS:.o=.d) $(OBJS) $(GOAL)

.PHONY: release
release:

-include $(OBJS:.o=.d)

