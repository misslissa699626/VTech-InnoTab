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
#define IO3_SIZE		0x00040000                 // How much?
#define IO3_START		0x93000000                 // PA of IO

#define SRAM_BASE		0xfc900000                 // VA of IO   //for L2 cache
//#define SRAM_SIZE		0x00008000                 // How much?  //give 1M size mapping
#define SRAM_SIZE		0x00004000                 // How much?  //give 1M size mapping
//#define SRAM_START		0x9D800000                 // PA of IO
#define SRAM_START		0xB0000000                 // PA of IO

#define CEVAL2_BASE		0xfca00000                 // VA of IO 
#define CEVAL2_SIZE		0x00008000                 // How much?
#define CEVAL2_START	0xB1000000                 // PA of IO

#define CEVAL1_BASE		0xfcb00000                 // VA of IO 
#define CEVAL1_SIZE		0x00010000                 // How much?
#define CEVAL1_START	0xB2000000                 // PA of IO

#define ROM_BASE		0xfcc07000                 // VA of IO,only mapping last 4k
#define ROM_SIZE		0x00001000                 // How much?
#define ROM_START	    0xffff7000                 // PA of IO

#define FRAMEBUF_BASE	0x03C00000                 // PA of IO 
#define FRAMEBUF_SIZE	0x00400000                 // How much?

#define FB_USE_CHUNKMEM
#define CHUNKMEM_BASE	0x02800000                 // PA of IO
#ifdef FB_USE_CHUNKMEM
#define CHUNKMEM_SIZE	0x01800000                 // How much?
#else
#define CHUNKMEM_SIZE	0x01400000                 // How much?  // reserved 4M frame buffer
#endif

#define SCU_A_BASE			IO3_ADDRESS(0x7000)
#define SCU_B_BASE			IO0_ADDRESS(0x5000)
#define SCU_C_BASE			IO2_ADDRESS(0x5000)
#define SCU_D_BASE			IO0_ADDRESS(0x532800)

/* macro to get at IO space when running virtually */
#define IO0_ADDRESS(x) ((x) + IO0_BASE)
#define IO2_ADDRESS(x) ((x) + IO2_BASE)
#define IO3_ADDRESS(x) ((x) + IO3_BASE)
#define SRAM_ADDRESS(x) ((x) + SRAM_BASE)
#define SRAM_ADDR_END   (SRAM_BASE + SRAM_SIZE)
#define ROM_LAST4K_ADDRESS(x) ((x) + ROM_BASE)

/* define register base address */
#define LOGI_ADDR_REG			(IO0_START)
#define LOGI_ADDR_REG_RANGE		(0x10000000)

#endif
