/* include/asm-arm/arch-spmp8000/hardware.h
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>

/*
 * Where in virtual memory the IO devices (timers, system controllers
 * and so on)
 */
#define IO0_BASE		0xfc000000                 // VA of IO 
#define IO0_SIZE		0x00800000                 // How much?
#define IO0_START		0x90000000                 // PA of IO

#define IO2_BASE		0xfd000000                 // VA of IO 
#define IO2_SIZE		0x01000000                 // How much?
#define IO2_START		0x92000000                 // PA of IO

#define IO3_BASE		0xfc800000                 // VA of IO 
#define IO3_SIZE		0x00020000                 // How much?
#define IO3_START		0x93000000                 // PA of IO

#define SRAM_BASE		0xfc900000                 // VA of IO   //for L2 cache
#define SRAM_SIZE		0x00008000                 // How much?  //give 1M size mapping
#define SRAM_START		0x9D800000                 // PA of IO

#define FRAMEBUF_BASE	0x03C00000                 // PA of IO 
#define FRAMEBUF_SIZE	0x00400000                 // How much?

#define FB_USE_CHUNKMEM
#define CHUNKMEM_BASE	0x02800000                 // PA of IO
#ifdef FB_USE_CHUNKMEM
#define CHUNKMEM_SIZE	0x01800000                 // How much?
#else
#define CHUNKMEM_SIZE	0x01400000                 // How much?  // reserved 4M frame buffer
#endif

/* macro to get at IO space when running virtually */
#define IO0_ADDRESS(x) ((x) + IO0_BASE)
#define IO2_ADDRESS(x) ((x) + IO2_BASE)
#define IO3_ADDRESS(x) ((x) + IO3_BASE)
#define SRAM_ADDRESS(x) ((x) + SRAM_BASE)
#define SRAM_ADDR_END   (SRAM_BASE + SRAM_SIZE)

/* define register base address */
#define LOGI_ADDR_REG			(IO0_BASE)
#define LOGI_ADDR_REG_RANGE		(0x10000000)

#endif
