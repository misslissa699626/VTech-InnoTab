/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

#ifndef _REG_TV_H_
#define _REG_TV_H_
	
#include <mach/hardware.h>
#include <mach/typedef.h>

#define SYSTEM_BASE_REG  (IO3_BASE + 0x7000)
#define	TV_BASE_REG		   (IO3_BASE + 0x20000)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpTvReg_s {
	volatile  UINT8 offset00[0x00f0];	         /* offset00, 0x93020000 ~ 0x930200F0*/
	volatile UINT32 gpTvCotrl0;		             /* P_TV1_CTRL, 0x930200F0 */
	volatile UINT32 gpTvCotrl1;		             /* P_TV1_CTRL2, 0x930200F4 */	
  volatile  UINT8 offset01[0x0090];          /* offset01, 0x930200F8 ~ 0x93020188*/ 
  volatile UINT32 gpTVIrqen;	               /* P_TV1_IRQEN, 0x93020188 */
  volatile UINT32 gpTVIrqsts;	             /* P_TV1_IRQSTATUS, 0x9302018C */
  volatile  UINT8 offset02[0x0050];          /* offset02, 0x93020190 ~ 0x930201E0*/	
  volatile UINT32 gpTVFbiaddr;	             /* P_TV1_FBADDR, 0x930201E0 */
  volatile  UINT8 offset03[0x0014];          /* offset03, 0x930201E4 ~ 0x930201F8*/  
  volatile UINT32 gpTVCotrl21;	             /* P_TV1_CTRL31, 0x930201F8 */
  volatile UINT32 gpTVCotrl2;	               /* P_TV1_CTRL3, 0x930201FC */	
	volatile UINT32 gpTVSaturation;		         /* P_TV1_SATURATION, 0x93020200 */
	volatile UINT32 gpTVHue;	                 /* P_TV1_HUE, 0x93020204 */
	volatile UINT32 gpTVBrightness;		         /* P_TV1_BRIGHTNESS, 0x93020208 */
	volatile UINT32 gpTVSharpness;	           /* P_TV1_SHARPNESS, 0x93020020C */
	volatile UINT32 gpTVYGain;		             /* P_TV1_Y_GAIN, 0x93020210 */
	volatile UINT32 gpTVYDelay;	               /* P_TV1_Y_DELAY, 0x93020214 */
	volatile UINT32 gpTVVPosition;		         /* P_TV1_V_POSITION, 0x93020218 */
  volatile UINT32 gpTVHPosition;		         /* P_TV1_H_POSITION, 0x9302021C */
  volatile UINT32 gpTVVideodac;	             /* P_TV1_VIDEODAC, 0x93020220 */
  volatile  UINT8 offset04[0x0148];          /* offset04, 0x93020224 ~ 0x9302036C*/
  volatile UINT32 gpTVCotrl3;		             /* P_TV1_CTRL4, 0x9302036C */
} gpTvReg_t;

typedef struct gpTvSysReg_s {
	volatile  UINT8 offset00[0x0004];	         /* offset00, 0x93007000 ~ 0x93007004*/
	volatile UINT32 gpTvSysCotrl0;		         /* P_TV1_SYS_CTRL0, 0x93007004 */
	volatile  UINT8 offset01[0x0008];	         /* offset01, 0x93007008 ~ 0x93007010*/
	volatile UINT32 gpTvEnable2;		           /* P_TV1_ENABLE2, 0x93007010 */	
  volatile  UINT8 offset02[0x0004];          /* offset02, 0x93007014 ~ 0x93007018*/ 
  volatile UINT32 gpTvSysCotrl1;	           /* P_TV1_SYS_CTRL1, 0x93007018 */
  volatile  UINT8 offset03[0x0024];          /* offset03, 0x9300701C ~ 0x93007040*/ 
  volatile UINT32 gpTvEnable1;		           /* P_TV1_ENABLE1, 0x93007040 */
  volatile  UINT8 offset04[0x009C];          /* offset04, 0x93007044 ~ 0x930070E0*/ 
  volatile UINT32 gpTvEnable0;		           /* P_TV1_ENABLE0, 0x930070E0 */	  	
} gpTvSysReg_t;

#endif /* _REG_TV_H_ */


