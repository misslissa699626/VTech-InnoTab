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
#ifndef _REG_SCALE2_H_
#define _REG_SCALE2_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LOGI_ADDR_SCALER2_REG     IO3_ADDRESS(0x2000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct scale2Reg_s {
	volatile UINT32 scale2Ctrl;        	/* 0x0000 ~ 0x0003 controller */
	volatile UINT32 scale2OBColor;     	/* 0x0004 ~ 0x0007 out-of-boundary color */
	volatile UINT32 scale2OutWidth;		/* 0x0008 ~ 0x000B output x width*/
	volatile UINT32 scale2OutHeight;	/* 0x000C ~ 0x000F output y width */

	volatile UINT32 scale2XFactor;		/* 0x0010 ~ 0x0013 x factor */
	volatile UINT32 scale2YFactor;		/* 0x0014 ~ 0x0017 y factor */
	volatile UINT32 scale2XStart;      	/* 0x0018 ~ 0x001B x start position*/
	volatile UINT32 scale2YStart;		/* 0x001C ~ 0x001F y start position*/

	volatile UINT32 scale2InWidth;		/* 0x0020 ~ 0x0023 input x width */
	volatile UINT32 scale2InHeight;		/* 0x0024 ~ 0x0027 input y width */
	volatile UINT32 scale2InYAddr;		/* 0x0028 ~ 0x002B input y address */
	volatile UINT32 scale2InUAddr;		/* 0x002C ~ 0x002F input u address */

	volatile UINT32 scale2InVAddr;		/* 0x0030 ~ 0x0033 input v address */
	volatile UINT32 scale2OutYAddr;		/* 0x0034 ~ 0x0037 output y address */
	volatile UINT32 scale2OutUAddr;		/* 0x0038 ~ 0x003B output u address*/
	volatile UINT32 scale2OutVAddr;		/* 0x003C ~ 0x003F output v address*/

	volatile UINT32 scale2CurrLine; 	/* 0x0040 ~ 0x0043 current line */
	volatile UINT32 scale2A11;        	/* 0x0044 ~ 0x0047 color matrix parameter a11*/
	volatile UINT32 scale2A12;         	/* 0x0048 ~ 0x004B color matrix parameter a12*/
	volatile UINT32 scale2A13;         	/* 0x004C ~ 0x004F color matrix parameter a13*/

	volatile UINT32 scale2A21;         	/* 0x0050 ~ 0x0053 color matrix parameter a21*/
	volatile UINT32 scale2A22;         	/* 0x0054 ~ 0x0057 color matrix parameter a22*/
	volatile UINT32 scale2A23;         	/* 0x0058 ~ 0x005B color matrix parameter a23*/
	volatile UINT32 scale2A31;         	/* 0x005C ~ 0x005F color matrix parameter a31*/

	volatile UINT32 scale2A32;         	/* 0x0060 ~ 0x0063 color matrix parameter a32*/
	volatile UINT32 scale2A33;         	/* 0x0064 ~ 0x0067 color matrix parameter a33*/
	volatile UINT32 scale2InVisWidth;	/* 0x0068 ~ 0x006B input real x width*/
	volatile UINT32 scale2InVisHeight;	/* 0x006C ~ 0x006F input real y width*/

	volatile UINT32 scale2OutOffset;   	/* 0x0070 ~ 0x0073 output offset */
	volatile UINT32 scale2LBAddr;		/* 0x0074 ~ 0x0077 external line buffer pointer */
	volatile UINT32 reserved0;			/* 0x0078 ~ 0x007B reserved */
	volatile UINT32 scale2IntFlag;		/* 0x007C ~ 0x007F interrupt */
	
	volatile UINT32 scale2PostPro;		/* 0x0080 ~ 0x0083 post control */
	volatile UINT32 scale2MaxY;			/* 0x0084 ~ 0x0087 max y */
	volatile UINT32 scale2MinY;			/* 0x0088 ~ 0x008B min y */
	volatile UINT32 scale2SumY;			/* 0x008C ~ 0x008F sum y */

	volatile UINT32 scale2MaxU;			/* 0x0090 ~ 0x0093 max u */
	volatile UINT32 scale2MinU;			/* 0x0094 ~ 0x0097 min u */
	volatile UINT32 scale2SumU;			/* 0x0098 ~ 0x009B sum u */
	volatile UINT32 scale2MaxV;			/* 0x009C ~ 0x009F max v */
	
	volatile UINT32 scale2MinV;			/* 0x00A0 ~ 0x00A3 min v */
	volatile UINT32 scale2SumV;			/* 0x00A4 ~ 0x00A7 sum v */
	volatile UINT32 reserved1;			/* 0x00A8 ~ 0x00AB reserved */
	volatile UINT32 reserved2;			/* 0x00AC ~ 0x007F reserved */

	volatile UINT32 reserved3;			/* 0x00B0 ~ 0x00B3 reserved */
	volatile UINT32 reserved4;			/* 0x00B4 ~ 0x00B7 reserved */
	volatile UINT32 reserved5;			/* 0x00B8 ~ 0x00BB reserved */
	volatile UINT32 reserved6;			/* 0x00BC ~ 0x00BF reserved */

	volatile UINT32 scale2YHis0;		/* 0x00C0 ~ 0x00C3 Y histogram 0~15 */
	volatile UINT32 scale2YHis1;		/* 0x00C4 ~ 0x00C7 Y histogram 16~31*/
	volatile UINT32 scale2YHis2;		/* 0x00C8 ~ 0x00CB Y histogram 32~47*/
	volatile UINT32 scale2YHis3;		/* 0x00CC ~ 0x00CF Y histogram 48~63*/

	volatile UINT32 scale2YHis4;		/* 0x00D0 ~ 0x00D3 Y histogram 64~79*/
	volatile UINT32 scale2YHis5;		/* 0x00D4 ~ 0x00D7 Y histogram 80~95*/
	volatile UINT32 scale2YHis6;		/* 0x00D8 ~ 0x00DB Y histogram 96~111*/
	volatile UINT32 scale2YHis7;		/* 0x00DC ~ 0x00DF Y histogram 112~127*/

	volatile UINT32 scale2YHis8;		/* 0x00E0 ~ 0x00E3 Y histogram 128~143*/
	volatile UINT32 scale2YHis9;		/* 0x00E4 ~ 0x00E7 Y histogram 144~159*/
	volatile UINT32 scale2YHis10;		/* 0x00E8 ~ 0x00EB Y histogram 160~175*/
	volatile UINT32 scale2YHis11;		/* 0x00EC ~ 0x00EF Y histogram 176~191*/

	volatile UINT32 scale2YHis12;		/* 0x00F0 ~ 0x00F3 Y histogram 192~207*/
	volatile UINT32 scale2YHis13;		/* 0x00F4 ~ 0x00F7 Y histogram 208~223*/
	volatile UINT32 scale2YHis14;		/* 0x00F8 ~ 0x00FB Y histogram 224~239*/
	volatile UINT32 scale2YHis15;		/* 0x00FC ~ 0x00FF Y histogram 240~255*/

	volatile UINT32 scale2ContiW1;		/* 0x0100 ~ 0x0103 Continue mode write reg 1*/
	volatile UINT32 scale2ContiR1;		/* 0x0104 ~ 0x0107 Continue mode read reg 1*/
	volatile UINT32 scale2ContiR2;		/* 0x0108 ~ 0x010B Continue mode read reg 2*/
} scale2Reg_t;

typedef struct scale2RegGamma_s 
{
	volatile UINT8 offset[0x0400];		/* 0x0000 ~ 0x0400 offset*/
	volatile UINT32 gamma[256];			/* 0x0400 ~ 0x07FC gamma value */
}scale2RegGamma_t;

typedef struct scale2RegHis_s 
{
	volatile UINT8 offset[0x0800];		/* 0x0000 ~ 0x0800 offset*/
	volatile UINT32 histgm[256];		/* 0x0800 ~ 0x0FFC Y histgm value */     
}scale2RegHis_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_SCALE2_H_ */
