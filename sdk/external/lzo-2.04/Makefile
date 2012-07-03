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

GOAL     = liblzo

CFLAGS  += -I.
CFLAGS  += -I./include
CFLAGS  += -I$(PRODUCT_DIR)
CFLAGS  += -g -O2 
CFLAGS  += -DHAVE_CONFIG_H -DLZO_HAVE_CONFIG_H=1

HDRS = lzoconf.h lzodefs.h lzoutil.h lzo_asm.h \
	lzo1.h lzo1a.h lzo1b.h lzo1c.h lzo1f.h lzo1x.h lzo1y.h lzo1z.h \
	lzo2a.h

ROOTHDRS = config.h

#---------------------------------------------------------------------------
# Intermediate objects
#---------------------------------------------------------------------------
OBJS += \
	./src/lzo_crc.o ./src/lzo_init.o ./src/lzo_ptr.o ./src/lzo_str.o ./src/lzo_util.o \
	./src/lzo1.o ./src/lzo1_99.o \
	./src/lzo1a.o ./src/lzo1a_99.o \
	./src/lzo1b_1.o ./src/lzo1b_2.o ./src/lzo1b_3.o ./src/lzo1b_4.o ./src/lzo1b_5.o ./src/lzo1b_6.o \
	./src/lzo1b_7.o ./src/lzo1b_8.o ./src/lzo1b_9.o ./src/lzo1b_99.o ./src/lzo1b_9x.o \
	./src/lzo1b_cc.o ./src/lzo1b_d1.o ./src/lzo1b_d2.o ./src/lzo1b_rr.o ./src/lzo1b_xx.o \
	./src/lzo1c_1.o ./src/lzo1c_2.o ./src/lzo1c_3.o ./src/lzo1c_4.o ./src/lzo1c_5.o ./src/lzo1c_6.o \
	./src/lzo1c_7.o ./src/lzo1c_8.o ./src/lzo1c_9.o ./src/lzo1c_99.o ./src/lzo1c_9x.o \
	./src/lzo1c_cc.o ./src/lzo1c_d1.o ./src/lzo1c_d2.o ./src/lzo1c_rr.o ./src/lzo1c_xx.o \
	./src/lzo1f_1.o ./src/lzo1f_9x.o ./src/lzo1f_d1.o ./src/lzo1f_d2.o \
	./src/lzo1x_1.o ./src/lzo1x_9x.o ./src/lzo1x_d1.o ./src/lzo1x_d2.o ./src/lzo1x_d3.o \
	./src/lzo1x_o.o ./src/lzo1x_1k.o ./src/lzo1x_1l.o ./src/lzo1x_1o.o \
	./src/lzo1y_1.o ./src/lzo1y_9x.o ./src/lzo1y_d1.o ./src/lzo1y_d2.o ./src/lzo1y_d3.o ./src/lzo1y_o.o \
	./src/lzo1z_9x.o ./src/lzo1z_d1.o ./src/lzo1z_d2.o ./src/lzo1z_d3.o \
	./src/lzo2a_9x.o ./src/lzo2a_d1.o ./src/lzo2a_d2.o

#---------------------------------------------------------------------------
# Explicit ruls
#---------------------------------------------------------------------------
all: $(GOAL).a $(GOAL).so
	$(MSG_SPLIT_LINE)
	@$(MKDIR) -p $(OUT_SDK_DIR)/lib
	@$(CP) -u $(GOAL).a $(OUT_SDK_DIR)/lib/$(GOAL).a
	@$(CP) -u $(GOAL).so $(OUT_SDK_DIR)/lib/$(GOAL).so

	@$(MKDIR) -p $(OUT_SDK_DIR)/include/lzo
	@for file in $(HDRS); do \
		cp -u ./include/lzo/$$file $(OUT_SDK_DIR)/include/lzo/$$file; \
	done

	@for file in $(ROOTHDRS); do \
		cp -u ./$$file $(OUT_SDK_DIR)/include/lzo/$$file; \
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
