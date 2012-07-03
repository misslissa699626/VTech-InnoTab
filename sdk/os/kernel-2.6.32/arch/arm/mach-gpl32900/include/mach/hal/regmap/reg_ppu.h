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

#ifndef _REG_PPU_H_
#define _REG_PPU_H_
	
#include <mach/hardware.h>
#include <mach/typedef.h>

#define	PPU_BASE_REG		  (IO3_BASE + 0x20000)
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct ppuText3Reg_s {
	volatile UINT32 ppuText3XPos;		/* P_PPU_TEXT3_X_POSITION, 0x93020000 */
	volatile UINT32 ppuText3YPos;		/* P_PPU_TEXT3_Y_POSITION, 0x93020004 */
	volatile UINT32 ppuText3XOffset;		/* P_PPU_TEXT3_X_OFFSET, 0x93020008 */
	volatile UINT32 ppuText3YOffset;	       /* P_PPU_TEXT3_X_OFFSET, 0x9302000C */
	volatile UINT32 ppuText3Attr;		/* P_PPU_TEXT3_ATTRIBUTE, 0x93020010 */
	volatile UINT32 ppuText3Ctrl;	/* P_PPU_TEXT3_CONTROL, 0x93020014 */
	volatile UINT32 ppuText3NumPtr;		/* P_PPU_TEXT3_NUMBER_PTR, 0x93020018 */
	volatile UINT32 ppuText3AttrPtr;	/* P_PPU_TEXT3_ATTRIBUTE_PTR, 0x9302001C */
       volatile  UINT8 offset00[0x006C];	/* offset00, 0x93020020 ~ 0x9302008C*/
	volatile UINT32 ppuText3Segment;		/* P_PPU_TEXT3_SEGMENT, 0x9302008C */
       volatile  UINT8 offset01[0x0074];	/* offset00, 0x93020090 ~ 0x93020104*/
       volatile UINT32 ppuText3YComp;		/* P_PPU_TEXT3_Y25D_COMPRESS, 0x93020104 */
	volatile  UINT8 offset02[0x06F8];	/* offset00, 0x93020108 ~ 0x93020800*/
       volatile UINT32 ppuText3Cosine;	/* P_PPU_TEXT3_COSINE, 0x93020800 */
	volatile UINT32 ppuText3Sine;	/* P_PPU_TEXT3_SINE, 0x93020804 */	
	
} ppuText3Reg_t;

typedef struct ppuText4Reg_s {
	volatile  UINT8 offset00[0x0020];	/* offset00, 0x93020000 ~ 0x9302001C*/
	volatile UINT32 ppuText4XPos;		/* P_PPU_TEXT4_X_POSITION, 0x93020020 */
	volatile UINT32 ppuText4YPos;		/* P_PPU_TEXT4_Y_POSITION, 0x93020024 */
	volatile UINT32 ppuText4XOffset;		/* P_PPU_TEXT4_X_OFFSET, 0x93020028 */
	volatile UINT32 ppuText4YOffset;	       /* P_PPU_TEXT4_X_OFFSET, 0x9302002C */
	volatile UINT32 ppuText4Attr;		/* P_PPU_TEXT4_ATTRIBUTE, 0x93020030 */
	volatile UINT32 ppuText4Ctrl;	/* P_PPU_TEXT4_CONTROL, 0x93020034 */
	volatile UINT32 ppuText4NumPtr;		/* P_PPU_TEXT4_NUMBER_PTR, 0x93020038 */
	volatile UINT32 ppuText4AttrPtr;	/* P_PPU_TEXT4_ATTRIBUTE_PTR, 0x9302003C */
       volatile  UINT8 offset01[0x0050];	/* offset00, 0x93020040 ~ 0x93020090*/
	volatile UINT32 ppuText4Segment;	/* P_PPU_TEXT4_SEGMENT, 0x93020090 */
	volatile  UINT8 offset02[0x000C];	/* offset00, 0x93020094 ~ 0x930200A0*/
       volatile UINT32 ppuText4Cosine;	/* P_PPU_TEXT4_COSINE, 0x930200A0 */
	volatile UINT32 ppuText4Sine;	/* P_PPU_TEXT4_SINE, 0x930200A4 */
	
} ppuText4Reg_t;

typedef struct ppuText1Reg_s {
       volatile  UINT8 offset00[0x0040];	/* offset00, 0x93020000 ~ 0x9302003C*/
	volatile UINT32 ppuText1XPos;		/* P_PPU_TEXT1_X_POSITION, 0x93020040 */
	volatile UINT32 ppuText1YPos;		/* P_PPU_TEXT1_Y_POSITION, 0x93020044 */
	volatile UINT32 ppuText1Attr;		/* P_PPU_TEXT1_ATTRIBUTE, 0x93020048 */
	volatile UINT32 ppuText1Ctrl;	/* P_PPU_TEXT1_CONTROL, 0x9302004C */
	volatile UINT32 ppuText1NumPtr;		/* P_PPU_TEXT1_NUMBER_PTR, 0x93020050 */
	volatile UINT32 ppuText1AttrPtr;	/* P_PPU_TEXT1_ATTRIBUTE_PTR, 0x93020054 */
       volatile UINT8 offset01[0x0028];	/* offset00, 0x93020058 ~ 0x930200080*/
	volatile UINT32 ppuText1Segment;	       /* P_PPU_TEXT1_SEGMENT, 0x93020080 */
	volatile UINT8 offset02[0x02BC];	/* offset00, 0x93020084 ~ 0x930200340*/
	volatile UINT32 ppuText1XOffset;		/* P_PPU_TEXT1_X_OFFSET, 0x93020340 */
	volatile UINT32 ppuText1YOffset;	       /* P_PPU_TEXT1_X_OFFSET, 0x93020344 */
	volatile UINT32 ppuText1Cosine;		/* P_PPU_TEXT1_COSINE, 0x93020348 */
	volatile UINT32 ppuText1Sine;	       /* P_PPU_TEXT1_SINE, 0x9302034C */
	volatile UINT8 offset03[0x00B0];	/* offset00, 0x93020350 ~ 0x930200400*/
	volatile UINT32 ppuText1HOffset;		/* P_PPU_TEXT1_HOFFSET, 0x93020400 */
	volatile UINT8 offset04[0x00B0];	/* offset00, 0x93020400 ~ 0x930200800*/
	volatile UINT32 ppuText1HCompValue;	       /* P_PPU_TEXT1_HCOMP_VALUE, 0x93020800 */	
} ppuText1Reg_t;

typedef struct ppuText2Reg_s {
       volatile  UINT8 offset00[0x0058];	/* offset00, 0x93020000 ~ 0x93020058*/
	volatile UINT32 ppuText2XPos;		/* P_PPU_TEXT2_X_POSITION, 0x93020058 */
	volatile UINT32 ppuText2YPos;		/* P_PPU_TEXT2_Y_POSITION, 0x9302005C */
	volatile UINT32 ppuText2Attr;		/* P_PPU_TEXT2_ATTRIBUTE, 0x93020060 */
	volatile UINT32 ppuText2Ctrl;	/* P_PPU_TEXT2_CONTROL, 0x93020064 */
	volatile UINT32 ppuText2NumPtr;		/* P_PPU_TEXT2_NUMBER_PTR, 0x93020068 */
	volatile UINT32 ppuText2AttrPtr;	/* P_PPU_TEXT2_ATTRIBUTE_PTR, 0x9302006C */
       volatile UINT8 offset01[0x0014];	/* offset00, 0x93020070 ~ 0x930200084*/
	volatile UINT32 ppuText2Segment;	       /* P_PPU_TEXT1_SEGMENT, 0x93020084 */	
       volatile UINT8 offset02[0x02C8];	/* offset00, 0x93020088 ~ 0x930200350*/
	volatile UINT32 ppuText2XOffset;		/* P_PPU_TEXT2_X_OFFSET, 0x93020350 */
	volatile UINT32 ppuText2YOffset;	       /* P_PPU_TEXT2_X_OFFSET, 0x93020354 */
	volatile UINT32 ppuText2Cosine;		/* P_PPU_TEXT2_COSINE, 0x93020358 */
	volatile UINT32 ppuText2Sine;	       /* P_PPU_TEXT2_SINE, 0x9302035C */	
} ppuText2Reg_t;


typedef struct ppuSpriteReg_s {
       volatile  UINT8 offset00[0x0088];	/* offset00, 0x93020000 ~ 0x93020088*/	   
	volatile UINT32 ppuSpriteSegment;		/* P_PPU_SPRITE_SEGMENT, 0x93020088 */
       volatile  UINT8 offset01[0x007C];	/* offset00, 0x93020090 ~ 0x93020108*/
       volatile UINT32 ppuSpriteCtrl;		/* P_PPU_SPRITE_CTRL, 0x930200108 */
	volatile  UINT8 offset02[0x00B4];	/* offset00, 0x9302010C ~ 0x930201C0*/
	volatile UINT32 ppuSpriteDmaSrc;		/* P_PPU_SPRITE_DMA_SOURCE, 0x9302001C0 */
	volatile UINT32 ppuSpriteDmaTarg;		/* P_PPU_SPRITE_DMA_TARGENT, 0x9302001C4 */
	volatile UINT32 ppuSpriteDmaNum;		/* P_PPU_SPRITE_DMA_NUMBER, 0x9302001C8 */
	volatile  UINT8 offset03[0x0134];	/* offset00, 0x930201CC ~ 0x93020300*/
	volatile UINT32 ppuSpriteX0;		/* P_PPU_SPRITE_X0, 0x93020300 */
       volatile UINT32 ppuSpriteY0;		/* P_PPU_SPRITE_Y0, 0x93020304 */
	volatile UINT32 ppuSpriteX1;		/* P_PPU_SPRITE_X1, 0x93020308 */
       volatile UINT32 ppuSpriteY1;		/* P_PPU_SPRITE_Y1, 0x9302030C */	
	volatile UINT32 ppuSpriteX2;		/* P_PPU_SPRITE_X0, 0x93020310 */
       volatile UINT32 ppuSpriteY2;		/* P_PPU_SPRITE_Y0, 0x93020314 */
	volatile UINT32 ppuSpriteX3;		/* P_PPU_SPRITE_X1, 0x93020318 */
       volatile UINT32 ppuSpriteY3;		/* P_PPU_SPRITE_Y1, 0x9302031C */	
       volatile UINT32 ppuSpriteW0;		/* P_PPU_SPRITE_W0, 0x93020320 */	
	volatile UINT32 ppuSpriteW1;		/* P_PPU_SPRITE_W1, 0x93020324 */
       volatile UINT32 ppuSpriteW2;		/* P_PPU_SPRITE_W2, 0x93020328 */
	volatile UINT32 ppuSpriteW3;		/* P_PPU_SPRITE_W3, 0x9302032C */
       volatile UINT32 ppuSpriteW4;		/* P_PPU_SPRITE_W4, 0x93020330 */
	volatile UINT32 ppuExspriteCtrl;		/* P_PPU_EXSPRITE_CTRL, 0x93020334 */
       volatile UINT32 ppuExspriteAddr;		/* P_PPU_EXSPRITE_ADDR, 0x93020338 */
	volatile  UINT8 offset04[0x0040];	   /* offset00, 0x9302033C ~ 0x9302037C*/
	volatile UINT32 ppuSpriteRgbOffset;		/* P_PPU_SPRITE_RGB_OFFSET, 0x9302037C */
	volatile  UINT8 offset05[0x1C80];	   /* offset00, 0x93020380 ~ 0x93022000*/
	volatile UINT32 ppuSpriteramAttribute;		/* P_PPU_SPRITE_ATTRIBUTE_BASE, 0x93022000 */
	volatile  UINT8 offset06[0x3FFC];	   /* offset00, 0x93022004 ~ 0x93026000*/
	volatile UINT32 ppuSpriteramExattribute;		/* P_PPU_SPRITE_EXTEND_ATTRIBUTE, 0x93026000 */
} ppuSpriteReg_t;

typedef struct ppuFunReg_s {
	//volatile  UINT32 offset;	/* offset, 0x93020000 ~ 0x93020070*/
       volatile  UINT8 offset00[0x0070];	/* offset00, 0x93020000 ~ 0x93020070*/
	volatile UINT32 ppuVcompValue;		/* P_PPU_VCOMP_VALUE, 0x93020070 */
	volatile UINT32 ppuVcompOffset;		/* P_PPU_VCOMP_OFFSET, 0x93020074 */
	volatile UINT32 ppuVcompStep;		/* P_PPU_VCOMP_SETP, 0x93020078 */
       volatile  UINT8 offset01[0x002C];	/* offset00, 0x9302007C ~ 0x930200A8*/
	volatile UINT32 ppuBlending;		/* P_PPU_BLENDING, 0x930200A8 */   
       volatile  UINT8 offset02[0x0014];	/* offset00, 0x930200AC ~ 0x930200C0*/
	volatile UINT32 ppuFadeCtrl;		/* P_PPU_FADE_CTRL, 0x930200C0 */    
	volatile  UINT8 offset03[0x001C];	/* offset00, 0x930200C4 ~ 0x930200E0*/
       volatile UINT32 ppuLineCounter;		/* P_PPU_LINE_COUNTER, 0x930200E0 */
       volatile UINT32 ppuLightpenCtrl;		/* P_PPU_LIGHTPEN_CTRL, 0x930200E4 */
	volatile UINT32 ppuPaletteCtrl;		/* P_PPU_PALETTE_CTRL, 0x930200E8 */
	volatile UINT32 reserved0;			/* reserved0, 0x930200EC */   
	volatile UINT32 reserved00;			/* reserved00, 0x930200F0 */
	volatile UINT32 reserved01;			/* reserved01, 0x930200F4 */
	volatile UINT32 ppuLightpenHPos;		/* P_PPU_LIGHTPEN_H_POSITION, 0x930200F8 */ 
	volatile UINT32 ppuLightpenVPos;		/* P_PPU_LIGHTPEN_V_POSITION, 0x930200FC */ 
       volatile  UINT8 offset04[0x0020];	/* offset00, 0x93020100 ~ 0x93020120*/
       volatile UINT32 ppuWindow0X;		/* P_PPU_WINDOW0_X, 0x93020120 */ 
	volatile UINT32 ppuWindow0Y;		/* P_PPU_WINDOW0_Y, 0x93020124 */
       volatile UINT32 ppuWindow1X;		/* P_PPU_WINDOW1_X, 0x93020128 */ 
	volatile UINT32 ppuWindow1Y;		/* P_PPU_WINDOW1_Y, 0x9302012C */ 
       volatile UINT32 ppuWindow2X;		/* P_PPU_WINDOW2_X, 0x93020130 */ 
	volatile UINT32 ppuWindow2Y;		/* P_PPU_WINDOW3_Y, 0x93020134 */
       volatile UINT32 ppuWindow3X;		/* P_PPU_WINDOW3_X, 0x93020138 */ 
	volatile UINT32 ppuWindow3Y;		/* P_PPU_WINDOW3_Y, 0x9302013C */
       volatile  UINT8 offset05[0x0084];	/* offset00, 0x93020140 ~ 0x930201D4*/
       volatile UINT32 ppuFbfifoSetup;		/* P_PPU_FBFIFO_SETUP, 0x930201D4 */ 
	volatile UINT32 ppuFbfifoGo;		/* P_PPU_FBFIFO_GO, 0x930201D8 */	   
       volatile UINT32 ppuHblankCtrl;		/* P_PPU_HB_CTRL, 0x930201CC */ 
	volatile UINT32 ppuHblankGo;		/* P_PPU_HB_GO, 0x930201D0 */	
	volatile UINT32 reserved02;			/* reserved02, 0x930201D4 */
	volatile UINT32 reserved03;			/* reserved03, 0x930201D8 */
	volatile UINT32 reserved04;			/* reserved04, 0x930201DC */
	volatile UINT32 ppuTvFbiAddr;		/* P_PPU_TV_FBI_ADDR, 0x930201E0 */
	volatile UINT32 reserved05;			/* reserved05, 0x930201E4 */
	volatile UINT32 ppuFboAddr;		/* P_PPU_FBO_ADDR, 0x930201E8 */
       volatile UINT32 reserved06;			/* reserved06, 0x930201EC */
       volatile UINT32 ppuFbGo;		/* P_PPU_FB_GO, 0x930201F0 */
       volatile UINT32 ppuBldColor;		/* P_PPU_BLD_COLOR, 0x930201F4 */
       volatile UINT32 ppuMisc;		/* P_PPU_MISC, 0x930201F8 */	
       volatile UINT32 ppuEnable;		/* P_PPU_ENABLE, 0x930201FC */
	volatile  UINT8 offset06[0x013C];	/* offset00, 0x93020200 ~ 0x9302033C*/   
	volatile UINT32 ppuTftFbiAddr;		/* P_PPU_TFT_FBI_ADDR, 0x9302033C */
	volatile  UINT8 offset07[0x002C];	/* offset00, 0x93020340 ~ 0x9302036C*/
	volatile UINT32 ppuFreesize;		/* P_PPU_FREE_SIZE, 0x9302036C */
	volatile UINT32 ppuDefPapa;		/* P_PPU_DEF_PAPA, 0x93020370 */
       volatile  UINT8 offset08[0x003C];	/* offset00, 0x93020374 ~ 0x930203A8*/
	volatile UINT32 ppuTft3dOffset;		/* P_PPU_TFT_3D_OFFSET, 0x930203A8 */
	volatile UINT32 ppuTftFbiAddrR;	/* P_PPU_TFT_FBI_ADDR_R, 0x930203AC */
	volatile  UINT8 offset09[0x0C50];	/* offset00, 0x930203B0 ~ 0x93021000*/
	volatile UINT32 ppuPaletteRam0;	/* P_PPU_PALETTE_RAM0, 0x93021000 */
	volatile  UINT8 offset10[0x0400];	/* offset00, 0x93021000 ~ 0x93021400*/
	volatile UINT32 ppuPaletteRam1;	/* P_PPU_PALETTE_RAM1, 0x93021400 */
	volatile  UINT8 offset11[0x0400];	/* offset00, 0x93021400 ~ 0x93021800*/	
	volatile UINT32 ppuPaletteRam2;	/* P_PPU_PALETTE_RAM2, 0x93021800 */
	volatile  UINT8 offset12[0x0400];	/* offset00, 0x93021800 ~ 0x93021C00*/
	volatile UINT32 ppuPaletteRam3;	/* P_PPU_PALETTE_RAM2, 0x93021C00 */
	volatile  UINT8 offset13[0x5000];	/* offset00, 0x93022000 ~ 0x93027000*/
	volatile UINT32 ppuColormappingRam;	/* P_PPU_COLOR_MAPPING_RAM, 0x93027000 */
	volatile  UINT8 offset14[0x0400];	/* offset14, 0x93027000 ~ 0x93027400*/
	volatile UINT32 ppuColormappingRamG;	/* P_PPU_COLOR_MAPPING_RAMG, 0x93027400 */
	volatile  UINT8 offset15[0x0400];	/* offset15, 0x93027400 ~ 0x93027800*/
	volatile UINT32 ppuColormappingRamR;	/* P_PPU_COLOR_MAPPING_RAMR, 0x93027800 */
} ppuFunReg_t;

typedef struct ppuFunIrq_s {
       volatile  UINT8 offset00[0x0188];	/* offset00, 0x93020000 ~ 0x93020188*/
	volatile UINT32 ppuIrqEn;		/* P_PPU_IRQ_EN, 0x93020188 */
	volatile UINT32 ppuIrqState;		/* P_PPU_IRQ_STATE, 0x9302018C */
} ppuFunIrq_t;

typedef struct ppustnFun_s {
  volatile  UINT8 offset00[0x017C];	/* offset00, 0x93020000 ~ 0x9302017C*/
	volatile UINT32 ppuStnEn;		      /* P_PPUSTN_EN, 0x9302017C */
} ppustnFun_t;

#endif /* _REG_PPU_H_ */


