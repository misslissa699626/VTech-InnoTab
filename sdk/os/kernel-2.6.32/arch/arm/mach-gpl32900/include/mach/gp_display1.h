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
 * @file gp_display1.h
 * @brief Display interface header file
 * @author Daniel Huang
 */

#ifndef _GP_DISPLAY1_DEVICE_H_
#define _GP_DISPLAY1_DEVICE_H_


#include <mach/typedef.h>
#include <mach/gp_display.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/



/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern int32_t register_paneldev1(uint32_t type, char *name, gp_disp_panel_ops_t *fops);
extern int32_t unregister_paneldev1(uint32_t type, char *name);

extern int32_t disp1_set_pri_bitmap(gp_bitmap_t *pbitmap);
extern void disp1_set_pri_enable(uint32_t enable);
extern void disp1_set_pri_frame_addr(uint32_t addr);
extern void disp1_update(void);
extern int32_t disp1_wait_frame_end(void);
extern void disp1_get_panel_res(gp_size_t *res);
extern void disp1_set_dither_enable(uint32_t enable);
extern void disp1_set_dither_type(uint32_t type);
extern void disp1_set_gamma_enable(uint32_t enable);
extern void disp1_set_gamma_table(uint32_t id, uint8_t *pTable);
extern int32_t disp1_spi(uint32_t val, uint32_t bitsLen, uint32_t lsbFirst);

#endif //endif _GP_DISPLAY1_DEVICE_H_
