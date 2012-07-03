
#ifndef __ARCH_GP_SRAM_H
#define __ARCH_GP_SRAM_H
extern int __init gp_sram_init(void);
extern void * gp_sram_push(void * start, unsigned long size);
#endif

