/*
 * arch/arm/mach-gpl32900/include/mach/regs-dc2dc.h
 *
 * Copyright (C) 2010 Generalplus
 *
 * DC2DC - System peripherals regsters.
 *
 */

#define DC2DC_BASE			IO0_ADDRESS(0x5140)     //0x90005140

/* DC2DC register */
#define DC2DC_CTR   (*(volatile unsigned int*)(DC2DC_BASE+0x00))  //control Register


