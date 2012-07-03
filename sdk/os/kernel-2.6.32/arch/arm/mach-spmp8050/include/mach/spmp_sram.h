/*
 * arch/arm/plat-omap/include/mach/sram.h
 *
 * Interface for functions that need to be run in internal SRAM
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ARCH_ARM_OMAP_SRAM_H
#define __ARCH_ARM_OMAP_SRAM_H
extern int __init spmp_sram_init(void);
extern void * spmp_sram_push(void * start, unsigned long size);
#endif

