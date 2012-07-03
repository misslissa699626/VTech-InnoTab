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
TOPDIR := ../../
include $(TOPDIR)sdk/build/core/config.mak


all:
	@$(MAKE) -C lua-5.1 -f Makefile.open
	@$(MAKE) -C luafilesystem-1.4.2 -f Makefile.open
	@$(MAKE) -C struct -f Makefile.open
	@$(MAKE) -C LuaBitOp-1.0.1 -f Makefile.open
	@$(MAKE) -C luasdl -f Makefile.open

clean:
	@$(MAKE) -C lua-5.1 -f Makefile.open clean
	@$(MAKE) -C luafilesystem-1.4.2 -f Makefile.open clean
	@$(MAKE) -C struct -f Makefile.open clean
	@$(MAKE) -C LuaBitOp-1.0.1 -f Makefile.open clean
	@$(MAKE) -C luasdl -f Makefile.open clean

