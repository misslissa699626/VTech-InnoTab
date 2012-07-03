/*
 * arch/arm/mach-spmp8000/include/mach/reg_interrupt.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * System Interrupt regsters.
 *
 */



#define VIC0_BASE			IO0_ADDRESS(0x10000)
#define VIC1_BASE			IO0_ADDRESS(0x20000)

/* VIC 0 register */
#define VIC0_IRQSTATUS		(*(volatile unsigned int*)(VIC0_BASE+0x000)) 
#define VIC0_FIQSTATUS		(*(volatile unsigned int*)(VIC0_BASE+0x004)) 
#define VIC0_RAWINTR		(*(volatile unsigned int*)(VIC0_BASE+0x008)) 
#define VIC0_INTSELECT		(*(volatile unsigned int*)(VIC0_BASE+0x00C)) 
#define VIC0_INTENABLE		(*(volatile unsigned int*)(VIC0_BASE+0x010)) 
#define VIC0_INTENCLEAR		(*(volatile unsigned int*)(VIC0_BASE+0x014)) 
#define VIC0_SOFTINT		(*(volatile unsigned int*)(VIC0_BASE+0x018)) 
#define VIC0_SOFTINTCLEAR	(*(volatile unsigned int*)(VIC0_BASE+0x01C)) 
#define VIC0_PROTECTION		(*(volatile unsigned int*)(VIC0_BASE+0x020)) 
#define VIC0_PRIORITYMASK	(*(volatile unsigned int*)(VIC0_BASE+0x024)) 

/* VIC 1 register */
#define VIC1_IRQSTATUS		(*(volatile unsigned int*)(VIC1_BASE+0x000)) 
#define VIC1_FIQSTATUS		(*(volatile unsigned int*)(VIC1_BASE+0x004)) 
#define VIC1_RAWINTR		(*(volatile unsigned int*)(VIC1_BASE+0x008)) 
#define VIC1_INTSELECT		(*(volatile unsigned int*)(VIC1_BASE+0x00C)) 
#define VIC1_INTENABLE		(*(volatile unsigned int*)(VIC1_BASE+0x010)) 
#define VIC1_INTENCLEAR		(*(volatile unsigned int*)(VIC1_BASE+0x014)) 
#define VIC1_SOFTINT		(*(volatile unsigned int*)(VIC1_BASE+0x018)) 
#define VIC1_SOFTINTCLEAR	(*(volatile unsigned int*)(VIC1_BASE+0x01C)) 
#define VIC1_PROTECTION		(*(volatile unsigned int*)(VIC1_BASE+0x020)) 
#define VIC1_PRIORITYMASK	(*(volatile unsigned int*)(VIC1_BASE+0x024)) 


