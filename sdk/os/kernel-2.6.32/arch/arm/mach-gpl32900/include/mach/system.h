/* include/asm-arm/arch-goldfish/system.h
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

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <asm/proc-fns.h>

void gp_cpu_do_idle(void);
extern volatile int idle_counter;

extern void disable_do_idle(void);
extern void enable_do_idle(void);

static inline void arch_idle(void)
{
	if (!idle_counter) {
		gp_cpu_do_idle(); //cpu_do_idle();
    }
}

static inline void arch_reset(char mode, const char *cmd)
{
	extern void gpl32900_power_reset(void);
	gpl32900_power_reset();
}

#endif
