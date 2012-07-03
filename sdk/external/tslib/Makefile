#/**************************************************************************
# *                                                                        *
# *         Copyright (c) 2008 by Sunplus mMedia Inc.                      *
# *                                                                        *
# *  This software is copyrighted by and is the property of Sunplus        *
# *  mMedia Ltd. All rights are reserved by Sunplus mMedia Inc.            *
# *  This software may only be used in accordance with the                 *
# *  corresponding license agreement. Any unauthorized use, duplication,   *
# *  distribution, or disclosure of this software is expressly forbidden.  *
# *                                                                        *
# *  This Copyright notice MUST not be removed or modified without prior   *
# *  written consent of Sunplus mMedia Inc.                                *
# *                                                                        *
# *  Sunplus mMedia Inc. reserves the right to modify this software        *
# *  without notice.                                                       *
# *                                                                        *
# *  Sunplus mMedia Inc.                                                   *
# *  19, Innovation First Road, Science-Based Industrial Park,             *
# *  Hsin-Chu, Taiwan, R.O.C.                                              *
# *                                                                        *
# *                                                                        *
# **************************************************************************/
TOPDIR := ../../../
include $(TOPDIR)sdk/build/core/config.mak

SUBDIRS += src
SUBDIRS += plugins

HDRS = config.h

all:
	+$(COMMAND)
	@$(MKDIR) -p $(OUT_SDK_DIR)/include/libts
	@for file in $(HDRS); do \
		cp -u ./$$file $(OUT_SDK_DIR)/include/libts/$$file; \
	done

clean:
	+$(COMMAND)

.PHONY: release
release:
	+$(COMMAND)
