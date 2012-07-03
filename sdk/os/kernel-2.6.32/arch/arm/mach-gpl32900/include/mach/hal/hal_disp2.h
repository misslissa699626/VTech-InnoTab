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
#ifndef _HAL_DISP2_H_
#define _HAL_DISP2_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>


/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
enum{
	HAL_DISP2_RES_320_240 = 0x0,
	HAL_DISP2_RES_640_480 = 0x1,
	HAL_DISP2_RES_720_480 = 0x2,
};

enum {
	HAL_DISP2_DEV_LCD = 0,
	HAL_DISP2_DEV_LCM,
	HAL_DISP2_DEV_TV,
	HAL_DISP2_DEV_MAX,
};

enum {
	HAL_DISP2_INT_TV_VBLANK = 1 << 11,
};

enum {
	HAL_DISP2_TV_INTERLACE = 0,
	HAL_DISP2_TV_NONINTERLACE,
};

enum {
	HAL_DISP2_TV_MODE_NTSC_M = 0,
	HAL_DISP2_TV_MODE_NTSC_J,
	HAL_DISP2_TV_MODE_NTSC_N,
	HAL_DISP2_TV_MODE_PAL_M,
	HAL_DISP2_TV_MODE_PAL_B,
	HAL_DISP2_TV_MODE_PAL_N,
	HAL_DISP2_TV_MODE_PAL_NC,
	HAL_DISP2_TV_MODE_RESERVED,
};

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/




/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/
/* hal_disp2.c */
UINT32 gpHalDisp2Init(void);
void gpHalDisp2Deinit(void);
void gpHalDisp2SetDevType(UINT32 devType);
UINT32 gpHalDisp2GetDevType(void);
void gpHalDisp2UpdateParameter(void);
void gpHalDisp2SetIntEnable(UINT32 field);
void gpHalDisp2SetIntDisable(UINT32 field);
void gpHalDisp2ClearIntFlag(UINT32 field);
UINT32 gpHalDisp2GetIntStatus(void);
void gpHalDisp2SetFlip(UINT32 value);

/* Primary layer */
void gpHalDisp2SetPriFrameAddr(UINT32 addr);

/* TV panel */
UINT32 gpHalDisp2SetRes(UINT32 mode);
void gpHalDisp2GetRes(UINT16 *width, UINT16 *height);
void gpHalDisp2SetInterlace(UINT32 mode);
UINT32 gpHalDisp2GetInterlace(void);
void gpHalDisp2SetTvType(UINT32 type);
UINT32 gpHalDisp2GetTvType(void);
void gpHalDisp2SetEnable(UINT32 status);
UINT32 gpHalDisp2GetEnable(void);

void gpHalDisp2SetSaturation(UINT32 value);
UINT32 gpHalDisp2GetSaturation(void);
void gpHalDisp2SetHue(UINT32 value);
UINT32 gpHalDisp2GetHue(void);
void gpHalDisp2SetBrightness(UINT32 value);
UINT32 gpHalDisp2GetBrightness(void);
void gpHalDisp2SetSharpness(UINT32 value);
UINT32 gpHalDisp2GetSharpness(void);
void gpHalDisp2SetYGain(UINT32 value);
UINT32 gpHalDisp2GetYGain(void);
void gpHalDisp2SetYDelay(UINT32 value);
UINT32 gpHalDisp2GetYDelay(void);
void gpHalDisp2SetVPosition(UINT32 value);
UINT32 gpHalDisp2GetVPosition(void);
void gpHalDisp2SetHPosition(UINT32 value);
UINT32 gpHalDisp2GetHPosition(void);
void gpHalDisp2SetVideoDac(UINT32 value);
UINT32 gpHalDisp2GetVideoDac(void);


#endif  /* __HAL_DISP2_H__ */

