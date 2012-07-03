/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file gp_version.c
 * @brief GPL329XXA Version get driver
 * @author DominantYang
 */
 
#include <mach/module.h>
#include <mach/hal/hal_uart.h>
#include <mach/hal/hal_clock.h>
#include <linux/serial_core.h>
#include <linux/console.h>
#include <asm/irq.h>		/*for request_irq*/
#include <linux/delay.h>	/*for udelay*/
#include <mach/hardware.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define VER_TAG_ADDR        0x00000FDC   //0xFFFF7FD0

#define GPL329XXA_VerA_Tag  0x302E3176
#define GPL329XXA_VerB_Tag  0x332E3276

#define GPL329XXA_VerA      1
#define GPL329XXA_VerB      2



#define DRV_Reg32(addr)               (*(volatile UINT32 *)(addr))

#ifndef _GPL329XXA_VER
#define _GPL329XXA_VER
typedef enum {
    GPL329XXA_VER_A=1,
    GPL329XXA_VER_B=2,
    GPL329XXA_VER_C=3
} GPL329XXA_VER;
#endif

GPL329XXA_VER gp_version_get(void);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**
 * @brief gp_version_get request function
 * @return 1: VerA, 2: VerB
 */
GPL329XXA_VER gp_version_get(void)
{
    GPL329XXA_VER ver;

    switch (DRV_Reg32(ROM_LAST4K_ADDRESS(VER_TAG_ADDR)))
    {
        case GPL329XXA_VerA_Tag:
            ver = GPL329XXA_VER_A;
            break;
        case GPL329XXA_VerB_Tag:
        default:    
            ver = GPL329XXA_VER_B;
            break;
    }
    return ver;
}
EXPORT_SYMBOL(gp_version_get);





