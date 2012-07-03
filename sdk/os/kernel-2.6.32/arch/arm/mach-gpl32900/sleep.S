/*
 * Low-level PXA250/210 sleep/wakeUp support
 *
 * Initial SA1110 code:
 * Copyright (c) 2001 Cliff Brake <cbrake@accelent.com>
 *
 * Adapted for PXA by Nicolas Pitre:
 * Copyright (c) 2002 Monta Vista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 */

#include <linux/linkage.h>
#include <linux/init.h>
#include <asm/assembler.h>
#include <asm/hwcap.h>
#include <asm/pgtable-hwdef.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/ptrace.h>

#include <mach/hardware.h>
#include <mach/regs-dram.h>

#define SREF_EXIT_PERIOD 0x00003200
#define ENABLE_SELF_REF	0x00000100
#define ENABLE_AUTO_INI	0x00000010

		.text


ENTRY(gpl32900_cpu_suspend)

	@ save registers on stack
	stmfd	sp!, {r0 - r12, lr}
	@ prepare to put DRAM into self-refresh manually
	
	@ load base address of Traffic Controller
	mov	r6, #DRAM_M320_VIA_BASE & 0xff000000
	orr	r6, r6, #DRAM_M320_VIA_BASE & 0x00ff0000
	orr	r6, r6, #DRAM_M320_VIA_BASE & 0x0000ff00
	orr	r6, r6, #DRAM_M320_VIA_BASE & 0x000000ff
	@ prepare to put SDRAM into self-refresh manually
	ldr	r7, [r6, #DRAM_REF_ISSUE_OFST]
	ldr	r7, [r6, #DRAM_INI_SET_OFST]
	mov	r9, #0xFFFF01FF & 0xff000000
	orr	r9, r9, #0xFFFF01FF & 0x00ff0000
	orr	r9, r9, #0xFFFF01FF & 0x0000ff00
	orr	r9, r9, #0xFFFF01FF & 0x000000ff		
	and	r7 ,r7 ,r9		
	mov	r9, #SREF_EXIT_PERIOD & 0xff000000
	orr	r9, r9, #SREF_EXIT_PERIOD & 0x00ff0000
	orr	r9, r9, #SREF_EXIT_PERIOD & 0x0000ff00
	orr	r9, r9, #SREF_EXIT_PERIOD & 0x000000ff		
	orr	r7, r7 ,r9
       str	r7, [r6, #DRAM_INI_SET_OFST]
	mov	r9, #ENABLE_SELF_REF & 0xff000000
	orr	r9, r9, #ENABLE_SELF_REF & 0x00ff0000
	orr	r9, r9, #ENABLE_SELF_REF & 0x0000ff00
	orr	r9, r9, #ENABLE_SELF_REF & 0x000000ff
       orr	r7, r7, r9
       str	r7, [r6, #DRAM_INI_SET_OFST]
	@ load base address of Traffic Controller
	mov	r6, #SCU_B_BASE & 0xff000000
	orr	r6, r6, #SCU_B_BASE & 0x00ff0000
	orr	r6, r6, #SCU_B_BASE & 0x0000ff00
       mov	r7, #0
       str	r7, [r6, #0x58 & 0xff]
	@ disable instruction cache
	@mrc	p15, 0, r9, c1, c0, 0
	@bic	r2, r9, #0x1000
	@mcr	p15, 0, r2, c1, c0, 0
	@nop

/*
 * Let's wait for the next wake up event to wake us up. r0 can't be
 * used here because r0 holds ARM_IDLECT1
 */
	mov	r2, #0
	mcr	p15, 0, r2, c7, c0, 4		@ wait for interrupt
/*
 * gpl32900_cpu_suspend()'s resume point.
 *
 * It will just start executing here, so we'll restore stuff from the
 * stack.
 */
	@ re-enable Icache
	@ mcr	p15, 0, r9, c1, c0, 0  
	@ restore regs and return
	ldmfd	sp!, {r0 - r12, pc}

ENTRY(gpl32900_cpu_suspend_sz)
	.word	. - gpl32900_cpu_suspend

