/* include/asm-arm/arch-goldfish/entry-macro.S
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

#include <mach/hardware.h>
#include <mach/irqs.h>
	.macro	disable_fiq
	.endm

	.macro  get_irqnr_preamble, base, tmp
#if 1
	mov	\base  , #0x1
#endif
	.endm

	.macro  arch_ret_to_user, tmp1, tmp2
	.endm

	.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp

#if 1
		tst	\base  , #1
		mov	\base  , #0
#endif

#if 0
		ldr	\base, =IO0_ADDRESS(0x10000)
		mov	\irqnr  , #0x0
		ldr	\irqstat, [\base]		@ get masked status
		ldr	\base   , =IO0_ADDRESS(0x20000)
		teq	\irqstat, #0
		ldreq	\irqstat, [\base]
		moveq	\irqnr  , #0x20

1001:		tst	\irqstat, #15
		bne	1002f
		add	\irqnr, \irqnr, #4
		movs	\irqstat, \irqstat, lsr #4
		bne	1001b
1002:		tst	\irqstat, #1
		bne	1003f
		add	\irqnr, \irqnr, #1
		movs	\irqstat, \irqstat, lsr #1
		bne	1002b
1003:		/* EQ will be set if no irqs pending */
#endif

	

#if 0
		ldr	\base, =IO0_ADDRESS(0x10000)
		ldr	\irqstat, [\base]
		teq	\irqstat, #0
        beq 1001f	

		ldr	\irqnr, [\base , #0XF00]		@ get irq number from the vic vector add
        
        b 1003f

1001:
		ldr	\base, =IO0_ADDRESS(0x20000)
		ldr	\irqstat, [\base]
		teq	\irqstat, #0
        beq 1003f	
		ldr	\irqnr, [\base , #0XF00]		@ get irq number from the vic vector add

1003:   /* EQ will be set if no irqs pending */

#endif

		.endm	



