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
 * @file    gp_ppu.h
 * @brief   Declaration of PPU base driver.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */
 
#ifndef _GP_PPU_H_
#define _GP_PPU_H_

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
// PPU Hardware Module 
#define MODULE_DISABLE                        0
#define MODULE_ENABLE                         1
#define PPU_HARDWARE_MODULE                   MODULE_ENABLE  
#define CPU_MOVE_MODE                         0
#define PPU_DATA_OFFSET                       8
#define PPU_VALUE_RANGE_SET                   0xFF00                          

// Global definitions
#define FUNCTION_ENABLE         1
#define FUNCTION_DISABLE        0
#define TRUE                    1
#define FALSE                   0
#define STATUS_OK               0
#define STATUS_FAIL             -1
#define	C_PPU_TEXT1				0
#define	C_PPU_TEXT2				1
#define	C_PPU_TEXT3				2
#define	C_PPU_TEXT4				3
#define	B_BIT0							0
#define	B_BIT1							1
#define	B_BIT2							2
#define	B_BIT3							3
#define	B_BIT4							4
#define	B_BIT5							5
#define	B_BIT6							6
#define	B_BIT7							7
#define	B_BIT8							8
#define	B_BIT9							9
#define	B_BIT10							10
#define	B_BIT11							11
#define	B_BIT12							12
#define	B_BIT13							13
#define	B_BIT14							14
#define	B_BIT15							15
#define	B_BIT16							16
#define	B_BIT17							17
#define	B_BIT18							18
#define	B_BIT19							19
#define	B_BIT20							20
#define	B_BIT21							21
#define	B_BIT22							22
#define	B_BIT23							23
#define	B_BIT24							24
#define	B_BIT25							25
#define	B_BIT26							26
#define	B_BIT27							27
#define	B_BIT28							28
#define	B_BIT29							29
#define	B_BIT30							30
#define	B_BIT31							31
#define	BIT0							(((unsigned int)1)<<B_BIT0)
#define	BIT1							(((unsigned int)1)<<B_BIT1)
#define	BIT2							(((unsigned int)1)<<B_BIT2)
#define	BIT3							(((unsigned int)1)<<B_BIT3)
#define	BIT4							(((unsigned int)1)<<B_BIT4)
#define	BIT5							(((unsigned int)1)<<B_BIT5)
#define	BIT6							(((unsigned int)1)<<B_BIT6)
#define	BIT7							(((unsigned int)1)<<B_BIT7)
#define	BIT8							(((unsigned int)1)<<B_BIT8)
#define	BIT9							(((unsigned int)1)<<B_BIT9)
#define	BIT10							(((unsigned int)1)<<B_BIT10)
#define	BIT11							(((unsigned int)1)<<B_BIT11)
#define	BIT12							(((unsigned int)1)<<B_BIT12)
#define	BIT13							(((unsigned int)1)<<B_BIT13)
#define	BIT14							(((unsigned int)1)<<B_BIT14)
#define	BIT15							(((unsigned int)1)<<B_BIT15)
#define	BIT16							(((unsigned int)1)<<B_BIT16)
#define	BIT17							(((unsigned int)1)<<B_BIT17)
#define	BIT18							(((unsigned int)1)<<B_BIT18)
#define	BIT19							(((unsigned int)1)<<B_BIT19)
#define	BIT20							(((unsigned int)1)<<B_BIT20)
#define	BIT21							(((unsigned int)1)<<B_BIT21)
#define	BIT22							(((unsigned int)1)<<B_BIT22)
#define	BIT23							(((unsigned int)1)<<B_BIT23)
#define	BIT24							(((unsigned int)1)<<B_BIT24)
#define	BIT25							(((unsigned int)1)<<B_BIT25)
#define	BIT26							(((unsigned int)1)<<B_BIT26)
#define	BIT27							(((unsigned int)1)<<B_BIT27)
#define	BIT28							(((unsigned int)1)<<B_BIT28)
#define	BIT29							(((unsigned int)1)<<B_BIT29)
#define	BIT30							(((unsigned int)1)<<B_BIT30)
#define	BIT31							(((unsigned int)1)<<B_BIT31)
#define	MASK_1_BITS						(((unsigned int)BIT1)-1)
#define	MASK_2_BITS						(((unsigned int)BIT2)-1)
#define	MASK_3_BITS						(((unsigned int)BIT3)-1)
#define	MASK_4_BITS						(((unsigned int)BIT4)-1)
#define	MASK_5_BITS						(((unsigned int)BIT5)-1)
#define	MASK_6_BITS						(((unsigned int)BIT6)-1)
#define	MASK_7_BITS						(((unsigned int)BIT7)-1)
#define	MASK_8_BITS						(((unsigned int)BIT8)-1)
#define	MASK_9_BITS						(((unsigned int)BIT9)-1)
#define	MASK_10_BITS					(((unsigned int)BIT10)-1)
#define	MASK_11_BITS					(((unsigned int)BIT11)-1)
#define	MASK_12_BITS					(((unsigned int)BIT12)-1)
#define	MASK_13_BITS					(((unsigned int)BIT13)-1)
#define	MASK_14_BITS					(((unsigned int)BIT14)-1)
#define	MASK_15_BITS					(((unsigned int)BIT15)-1)
#define	MASK_16_BITS					(((unsigned int)BIT16)-1)
#define	MASK_17_BITS					(((unsigned int)BIT17)-1)
#define	MASK_18_BITS					(((unsigned int)BIT18)-1)
#define	MASK_19_BITS					(((unsigned int)BIT19)-1)
#define	MASK_20_BITS					(((unsigned int)BIT20)-1)
#define	MASK_21_BITS					(((unsigned int)BIT21)-1)
#define	MASK_22_BITS					(((unsigned int)BIT22)-1)
#define	MASK_23_BITS					(((unsigned int)BIT23)-1)
#define	MASK_24_BITS					(((unsigned int)BIT24)-1)
#define	MASK_25_BITS					(((unsigned int)BIT25)-1)
#define	MASK_26_BITS					(((unsigned int)BIT26)-1)
#define	MASK_27_BITS					(((unsigned int)BIT27)-1)
#define	MASK_28_BITS					(((unsigned int)BIT28)-1)
#define	MASK_29_BITS					(((unsigned int)BIT29)-1)
#define	MASK_30_BITS					(((unsigned int)BIT30)-1)
#define	MASK_31_BITS					(((unsigned int)BIT31)-1)
#define	MASK_32_BITS					((unsigned int)0xFFFFFFFF)

// PPU Module register 
#define	B_DEFEN_EN						25
#define	PPU_DEFEN_ENABLE			    (((unsigned int) 1)<<B_DEFEN_EN)
#define	B_TFT_LB						24
#define	PPU_TFT_LONG_BURST				(((unsigned int) 1)<<B_TFT_LB)
#define	B_CM_EN							23
#define	PPU_CM_ENABLE					(((unsigned int) 1)<<B_CM_EN)
#define	B_YUV_TYPE						20
#define	MASK_YUV_TYPE					(((unsigned int) 0x7)<<B_YUV_TYPE)
#define	B_PPU_LB						19
#define	PPU_LONG_BURST					(((unsigned int) 1)<<B_PPU_LB)
#define	B_TFT_SIZE						16
#define	MASK_TFT_SIZE					(((unsigned int) 0x7)<<B_TFT_SIZE)
#define	B_SAVE_ROM						15
#define	SAVE_ROM_ENABLE					(((unsigned int) 1)<<B_SAVE_ROM)
#define	B_FB_SEL						14
#define	FB_SEL0							(((unsigned int) 0)<<B_FB_SEL)
#define	FB_SEL1							(((unsigned int) 1)<<B_FB_SEL)
#define	B_SPR_WIN						13
#define	SPR_WIN_DISABLE					(((unsigned int) 0)<<B_SPR_WIN)
#define	SPR_WIN_ENABLE					(((unsigned int) 1)<<B_SPR_WIN)
#define	B_HVCMP_DIS						12
#define	HVCMP_ENABLE					(((unsigned int) 0)<<B_HVCMP_DIS)
#define	HVCMP_DISABLE					(((unsigned int) 1)<<B_HVCMP_DIS)
#define	B_FB_MONO						10
#define	MASK_FB_MONO					(((unsigned int) 0x3)<<B_FB_MONO)
#define	B_SPR25D						9
#define	PPU_SPR25D_DISABLE				(((unsigned int) 0)<<B_SPR25D)
#define	PPU_SPR25D_ENABLE				(((unsigned int) 1)<<B_SPR25D)
#define	B_FB_FORMAT						8
#define	PPU_RGB565						(((unsigned int) 0)<<B_FB_FORMAT)
#define	PPU_RGBG						(((unsigned int) 1)<<B_FB_FORMAT)
#define	B_FB_EN							7
#define	PPU_LINE_BASE					(((unsigned int) 0)<<B_FB_EN)
#define	PPU_FRAME_BASE					(((unsigned int) 1)<<B_FB_EN)
#define	B_VGA_NONINTL					5
#define	PPU_VGA_INTL					(((unsigned int) 0)<<B_VGA_NONINTL)
#define	PPU_VGA_NONINTL					(((unsigned int) 1)<<B_VGA_NONINTL)
#define	B_VGA_EN						4
#define	PPU_QVGA						(((unsigned int) 0)<<B_VGA_EN)
#define	PPU_VGA							(((unsigned int) 1)<<B_VGA_EN)
#define	B_TX_BOTUP						3
#define	TX_UP2BOT						(((unsigned int) 0)<<B_TX_BOTUP)
#define	TX_BOT2UP						(((unsigned int) 1)<<B_TX_BOTUP)
#define	B_TX_DIRECT						2
#define	TX_RELATIVE_ADDRESS				(((unsigned int) 0)<<B_TX_DIRECT)
#define	TX_DIRECT_ADDRESS				(((unsigned int) 1)<<B_TX_DIRECT)
#define	B_CHAR0_TRANSPARENT				1
#define	CHAR0_TRANSPARENT_ENABLE		(((unsigned int) 1)<<B_CHAR0_TRANSPARENT)
#define	B_PPU_EN						0
#define	PPU_DISABLE						(((unsigned int) 0)<<B_PPU_EN)
#define	PPU_ENABLE						(((unsigned int) 1)<<B_PPU_EN)

#define	B_TXT_NEWSPECBMP				16
#define	TXT_NEWSPECBMP_DISABLE		    (((unsigned int) 0)<<B_TXT_NEWSPECBMP)
#define	TXT_NEWSPECBMP_ENABLE			(((unsigned int) 1)<<B_TXT_NEWSPECBMP)
#define	B_TXT_TVLB					    15
#define	TXT_TVLB_DISABLE				(((unsigned int) 0)<<B_TXT_TVLB)
#define	TXT_TVLB_ENABLE				    (((unsigned int) 1)<<B_TXT_TVLB)
#define	B_TXT_TFTVTQ					13
#define	TXT_TFTVTQ_DISABLE				(((unsigned int) 0)<<B_TXT_TFTVTQ)
#define	TXT_TFTVTQ_ENABLE				(((unsigned int) 1)<<B_TXT_TFTVTQ)
#define	B_TXT_DELGO					    12
#define	TXT_DELGO_DISABLE				(((unsigned int) 0)<<B_TXT_DELGO)
#define	TXT_DELGO_ENABLE				(((unsigned int) 1)<<B_TXT_DELGO)
#define	B_TXT_INTPMODE					10
#define	TXT_INTPMODE0				    (((unsigned int) 0)<<B_TXT_INTPMODE)
#define	TXT_INTPMODE1				    (((unsigned int) 1)<<B_TXT_INTPMODE)
#define	TXT_INTPMODE2				    (((unsigned int) 2)<<B_TXT_INTPMODE)
#define	TXT_INTPMODE3				    (((unsigned int) 3)<<B_TXT_INTPMODE)
#define	B_TXT_NEWCMP					9
#define	TXT_NEWCMP_DISABLE				(((unsigned int) 0)<<B_TXT_NEWCMP)
#define	TXT_NEWCMP_ENABLE				(((unsigned int) 1)<<B_TXT_NEWCMP)
#define	B_TXT_ALPHA					    8
#define	TXT_ALPHA_DISABLE				(((unsigned int) 0)<<B_TXT_ALPHA)
#define	TXT_ALPHA_ENABLE				(((unsigned int) 1)<<B_TXT_ALPHA)
#define	B_SP_ADDRX2					    7
#define	SP_ADDRX2_DISABLE				(((unsigned int) 0)<<B_SP_ADDRX2)
#define	SP_ADDRX2_ENABLE				(((unsigned int) 1)<<B_SP_ADDRX2)
#define	B_DUAL_BLEND					6
#define	DUAL_BLEND_DISABLE				(((unsigned int) 0)<<B_DUAL_BLEND)
#define	DUAL_BLEND_ENABLE				(((unsigned int) 1)<<B_DUAL_BLEND)
#define	B_SPRITE_RGBA					3
#define	SPRITE_RGBA_DISABLE				(((unsigned int) 0)<<B_SPRITE_RGBA)
#define	SPRITE_RGBA_ENABLE				(((unsigned int) 1)<<B_SPRITE_RGBA)
#define	B_TEXT_RGBA				        2
#define	TEXT_RGBA_DISABLE		        (((unsigned int) 0)<<B_TEXT_RGBA)
#define	TEXT_RGBA_ENABLE		        (((unsigned int) 1)<<B_TEXT_RGBA)
#define	B_FB_LOCK_EN					1
#define	FB_LOCK_DISABLE					(((unsigned int) 0)<<B_FB_LOCK_EN)
#define	FB_LOCK_ENABLE					(((unsigned int) 1)<<B_FB_LOCK_EN)
#define	B_FIFO_TYPE						14
#define	MASK_FIFO_TYPE_SIZE				(((unsigned int) 0x3)<<B_FIFO_TYPE)
#define	B_LINE_START					16
#define	MASK_LINE_START_SIZE			(((unsigned int) 0x1FF)<<B_LINE_START)
#define	B_ADDR_OFFSET					0
#define	MASK_ADDR_OFFSET_SIZE			(((unsigned int) 0x3FFF)<<B_ADDR_OFFSET)
#define	B_FREE_INIT					    31
#define	MASK_FREE_INIT_SIZE			    (((unsigned int) 0x1)<<B_FREE_INIT)
#define	B_FREE_H_SIZE					16
#define	MASK_FREE_H_SIZE			    (((unsigned int) 0x7FF)<<B_FREE_H_SIZE)
#define	B_FREE_V_SIZE					0
#define	MASK_FREE_V_SIZE			    (((unsigned int) 0x7FF)<<B_FREE_V_SIZE)

// PPU Sprite Module register 
#define	B_TFT_LB						24
#define	PPU_TFT_LONG_BURST				(((unsigned int) 1)<<B_TFT_LB)
#define	B_CM_EN							23
#define	PPU_CM_ENABLE					(((unsigned int) 1)<<B_CM_EN)
#define	B_YUV_TYPE						20
#define	MASK_YUV_TYPE					(((unsigned int) 0x7)<<B_YUV_TYPE)
#define	B_PPU_LB						19
#define	PPU_LONG_BURST					(((unsigned int) 1)<<B_PPU_LB)
#define	B_TFT_SIZE						16
#define	MASK_TFT_SIZE					(((unsigned int) 0x7)<<B_TFT_SIZE)
#define	B_SAVE_ROM						15
#define	SAVE_ROM_ENABLE					(((unsigned int) 1)<<B_SAVE_ROM)
#define	B_FB_SEL						14
#define	FB_SEL0							(((unsigned int) 0)<<B_FB_SEL)
#define	FB_SEL1							(((unsigned int) 1)<<B_FB_SEL)
#define	B_SPR_WIN						13
#define	SPR_WIN_DISABLE					(((unsigned int) 0)<<B_SPR_WIN)
#define	SPR_WIN_ENABLE					(((unsigned int) 1)<<B_SPR_WIN)
#define	B_HVCMP_DIS						12
#define	HVCMP_ENABLE					(((unsigned int) 0)<<B_HVCMP_DIS)
#define	HVCMP_DISABLE					(((unsigned int) 1)<<B_HVCMP_DIS)
#define	B_FB_MONO						10
#define	MASK_FB_MONO					(((unsigned int) 0x3)<<B_FB_MONO)
#define	B_SPR25D						9
#define	PPU_SPR25D_DISABLE				(((unsigned int) 0)<<B_SPR25D)
#define	PPU_SPR25D_ENABLE				(((unsigned int) 1)<<B_SPR25D)
#define	B_FB_FORMAT						8
#define	PPU_RGB565						(((unsigned int) 0)<<B_FB_FORMAT)
#define	PPU_RGBG						(((unsigned int) 1)<<B_FB_FORMAT)
#define	B_FB_EN							7
#define	PPU_LINE_BASE					(((unsigned int) 0)<<B_FB_EN)
#define	PPU_FRAME_BASE					(((unsigned int) 1)<<B_FB_EN)
#define	B_VGA_NONINTL					5
#define	PPU_VGA_INTL					(((unsigned int) 0)<<B_VGA_NONINTL)
#define	PPU_VGA_NONINTL					(((unsigned int) 1)<<B_VGA_NONINTL)
#define	B_VGA_EN						4
#define	PPU_QVGA						(((unsigned int) 0)<<B_VGA_EN)
#define	PPU_VGA							(((unsigned int) 1)<<B_VGA_EN)
#define	B_TX_BOTUP						3
#define	TX_UP2BOT						(((unsigned int) 0)<<B_TX_BOTUP)
#define	TX_BOT2UP						(((unsigned int) 1)<<B_TX_BOTUP)
#define	B_TX_DIRECT						2
#define	TX_RELATIVE_ADDRESS				(((unsigned int) 0)<<B_TX_DIRECT)
#define	TX_DIRECT_ADDRESS				(((unsigned int) 1)<<B_TX_DIRECT)
#define	B_CHAR0_TRANSPARENT				1
#define	CHAR0_TRANSPARENT_ENABLE		(((unsigned int) 1)<<B_CHAR0_TRANSPARENT)
#define	B_PPU_EN						0
#define	PPU_DISABLE						(((unsigned int) 0)<<B_PPU_EN)
#define	PPU_ENABLE						(((unsigned int) 1)<<B_PPU_EN)

// Sprite Control Register Constant Definitions
#define	B_SP_FRAC_EN					22
#define	SP_FRACTION_COORDINATE_ENABLE	(((unsigned int) 1)<<B_SP_FRAC_EN)
#define	B_SP_GPR_EN					    21
#define	SP_GROUP_ENABLE			        (((unsigned int) 1)<<B_SP_GPR_EN)
#define	B_SP_LS_EN					    20
#define	SP_LARGE_SIZE_ENABLE			(((unsigned int) 1)<<B_SP_LS_EN)
#define	B_SP_INTP_EN					19
#define	SP_INTERPOLATION_ENABLE			(((unsigned int) 1)<<B_SP_INTP_EN)
#define	B_SP_FAR_EN						18
#define	SP_FAR_ADDRESS_ENABLE			(((unsigned int) 1)<<B_SP_FAR_EN)
#define	B_SP_CDM_EN						17
#define	SP_COLOR_DITHER_ENABLE			(((unsigned int) 1)<<B_SP_CDM_EN)
#define	B_SP_EFFECT_EN					16
#define	SP_SPECIAL_EFFECT_ENABLE		(((unsigned int) 1)<<B_SP_EFFECT_EN)
#define	B_SP_NUMBER						8
#define	MASK_SP_NUMBER					(((unsigned int) 0xFF)<<B_SP_NUMBER)
#define	B_SP_ZOOMEN						7
#define	SP_ZOOM_ENABLE					(((unsigned int) 1)<<B_SP_ZOOMEN)
#define	B_SP_ROTEN						6
#define	SP_ROTATE_ENABLE				(((unsigned int) 1)<<B_SP_ROTEN)
#define	B_SP_MOSEN						5
#define	SP_MOSAIC_ENABLE				(((unsigned int) 1)<<B_SP_MOSEN)
#define	B_SP_DIRECT						4
#define	SP_DIRECT_ADDRESS_MODE			(((unsigned int) 1)<<B_SP_DIRECT)
#define	B_SP_BLDMODE					2
#define	SP_BLDMODE64_ENABLE				(((unsigned int) 1)<<B_SP_BLDMODE)
#define	B_COORD_SEL						1
#define	SP_COORD1						(((unsigned int) 1)<<B_COORD_SEL)
#define	SP_COORD0						(((unsigned int) 0)<<B_COORD_SEL)
#define	B_SP_EN							0
#define	SP_ENABLE						(((unsigned int) 1)<<B_SP_EN)
#define	SP_DISABLE						(((unsigned int) 0)<<B_SP_EN)

// Extended Sprite Control Register Constant Definitions
#define	B_EXSP_FRAC_EN					5
#define	EXSP_FRACTION_COORDINATE_ENABLE	(((unsigned int) 1)<<B_EXSP_FRAC_EN)
#define	B_EXSP_GPR_EN					4
#define	EXSP_GROUP_ENABLE			    (((unsigned int) 1)<<B_EXSP_GPR_EN)
#define	B_EXSP_LS_EN					3
#define	EXSP_LARGE_SIZE_ENABLE			(((unsigned int) 1)<<B_EXSP_LS_EN)
#define	B_EXSP_INTP_EN					2
#define	EXSP_INTERPOLATION_ENABLE	    (((unsigned int) 1)<<B_EXSP_INTP_EN)
#define	B_EXSP_CDM_EN					1
#define	EXSP_CDM_ENABLE	                (((unsigned int) 1)<<B_EXSP_CDM_EN)

// Sprite RAM Constant Definitions
#define	B_ESPN_INTERPOLATION            11	
#define MASK_ESPN_INTERPOLATION         (0x1 << B_ESPN_INTERPOLATION)
#define	B_ESPN_LG_SIZE                  14	
#define MASK_ESPN_LG_SIZE               (0x1 << B_ESPN_LG_SIZE)
#define	B_ESPN_GROUP_ID                 12	
#define MASK_ESPN_GROUP_ID              (0x3 << B_ESPN_GROUP_ID)

#define	B_SPN_COLOR_MASK_RED            6	
#define MASK_COLOR_MASK_RED             (0x1 << B_SPN_COLOR_MASK_RED)
#define	B_SPN_COLOR_MASK_GREEN          5	
#define MASK_COLOR_MASK_GREEN           (0x1 << B_SPN_COLOR_MASK_GREEN)
#define	B_SPN_COLOR_MASK_BLUE           4	
#define MASK_COLOR_MASK_BLUE            (0x1 << B_SPN_COLOR_MASK_BLUE)
#define MASK_COLOR_MASK                 (0x7 << B_SPN_COLOR_MASK_BLUE)
#define	B_SPN_INTERPOLATION             3	
#define MASK_SPN_INTERPOLATION          (0x1 << B_SPN_INTERPOLATION)
#define	B_SPN_LG_SIZE                   2	
#define MASK_SPN_LG_SIZE                (0x1 << B_SPN_LG_SIZE)
#define	B_SPN_GROUP_ID                  0	
#define MASK_SPN_GROUP_ID               (0x3 << B_SPN_GROUP_ID)

#define	B_ESPN_FRAC_1                   8	
#define MASK_ESPN_FRAC_1                (0x3 << B_ESPN_FRAC_1)
#define	B_ESPN_FRAC_2                   10	
#define MASK_ESPN_FRAC_2                (0x3 << B_ESPN_FRAC_2)
#define	B_ESPN_FRAC_3                   12	
#define MASK_ESPN_FRAC_3                (0x3 << B_ESPN_FRAC_3)
#define	B_ESPN_FRAC_4                   14	
#define MASK_ESPN_FRAC_4                (0x3 << B_ESPN_FRAC_4)

#define	B_ESPN_FRAC_X0                  0	
#define MASK_ESPN_FRAC_X0               (0x3 << B_ESPN_FRAC_X0)
#define	B_ESPN_FRAC_Y0                  2	
#define MASK_ESPN_FRAC_Y0               (0x3 << B_ESPN_FRAC_Y0)
#define	B_ESPN_FRAC_X1                  4	
#define MASK_ESPN_FRAC_X1               (0x3 << B_ESPN_FRAC_X1)
#define	B_ESPN_FRAC_Y1                  6	
#define MASK_ESPN_FRAC_Y1               (0x3 << B_ESPN_FRAC_Y1)
#define	B_ESPN_FRAC_X2                  8	
#define MASK_ESPN_FRAC_X2               (0x3 << B_ESPN_FRAC_X2)
#define	B_ESPN_FRAC_Y2                  10	
#define MASK_ESPN_FRAC_Y2               (0x3 << B_ESPN_FRAC_Y2)
#define	B_ESPN_FRAC_X3                  12	
#define MASK_ESPN_FRAC_X3               (0x3 << B_ESPN_FRAC_X3)
#define	B_ESPN_FRAC_Y3                  14	
#define MASK_ESPN_FRAC_Y3               (0x3 << B_ESPN_FRAC_Y3)

#define	B_SPN_CHARNUM_LO				0
#define	MASK_SPN_CHARNUM_LO				0xFFFF

#define	B_SPN_CDM                       16	
#define MASK_CDM_MODE                   0xFFFF
#define	B_SPN_POSX						0
#define	MASK_SPN_POSX					0x3FF
#define	B_SPN_ROTATE					10
#define	MASK_SPN_ROTATE					(0x3F << 10)
#define	B_SPN_Y1_LO						10
#define	MASK_SPN_Y1_LO					(0x3F << 10)
#define	B_SPN_POSY						0
#define	MASK_SPN_POSY					0x3FF
#define	B_SPN_ZOOM						10
#define	MASK_SPN_ZOOM					(0x3F << 10)
#define	B_SPN_Y2_LO						10
#define	MASK_SPN_Y2_LO					(0x3F << 10)

#define	B_SPN_COLOR						0
#define	MASK_SPN_COLOR					(0x3 << B_SPN_COLOR)
#define	B_SPN_FLIP						2
#define	MASK_SPN_FLIP					(0x3 << B_SPN_FLIP)
#define	B_SPN_HS						4
#define	MASK_SPN_HS						(0x3 << B_SPN_HS)
#define	B_SPN_VS						6
#define	MASK_SPN_VS						(0x3 << B_SPN_VS)
#define	B_SPN_PALETTE					8
#define	MASK_SPN_PALETTE				(0xF << B_SPN_PALETTE)
#define	B_SPN_DEPTH						12
#define	MASK_SPN_DEPTH					(0x3 << B_SPN_DEPTH)
#define	B_SPN_BLD						14
#define	SPN_BLD_ENABLE					(0x1 << B_SPN_BLD)
#define	B_SPN_PB_HIGH					15
#define	MASK_SPN_PB_HIGH				(0x1 << B_SPN_PB_HIGH)

#define	B_SPN_CHARNUM_HI				16
#define	MASK_SPN_CHARNUM_HI				0xFF
#define	B_SPN_PB_LOW					7
#define	MASK_SPN_PB_LOW					(0x1 << B_SPN_PB_LOW)
#define	B_SPN_WIN						8
#define	MASK_SPN_WIN					(0x3 << B_SPN_WIN)
#define	B_SPN_BLD_64_LVL				8
#define	MASK_SPN_BLD_64_LVL				(0x3F << B_SPN_BLD_64_LVL)
#define	B_SPN_BLD_16_LVL				10
#define	MASK_SPN_BLD_16_LVL				(0xF << B_SPN_BLD_16_LVL)
#define	B_SPN_MOSAIC					14
#define	MASK_SPN_MOSAIC					(0x3 << B_SPN_MOSAIC)

#define	B_SPN_X1						0
#define	MASK_SPN_X1						MASK_10_BITS
#define	B_SPN_Y3_LO						10
#define	MASK_SPN_Y3_LO					MASK_6_BITS

#define	B_SPN_X2						0
#define	MASK_SPN_X2						MASK_10_BITS
#define	B_SPN_Y1_HI						10
#define	MASK_SPN_Y1_HI					MASK_4_BITS
#define	B_SPN_Y3_HI1					14
#define	MASK_SPN_Y3_HI1					MASK_2_BITS

#define	B_SPN_X3						0
#define	MASK_SPN_X3						MASK_10_BITS
#define	B_SPN_Y2_HI						10
#define	MASK_SPN_Y2_HI					MASK_4_BITS
#define	B_SPN_Y3_HI2					14
#define	MASK_SPN_Y3_HI2					MASK_2_BITS

// PPU Text Module register 
#define	B_TXN_COLOR_MASK					24
#define	MASK_TXN_COLOR_MASK			(((unsigned int) 0x7)<<B_TXN_COLOR_MASK) 
#define	B_TXN_PB						22
#define	MASK_TXN_PALETTE_BANK			(((unsigned int) 0x3)<<B_TXN_PB)
#define	B_TXN_SPECIAL_EFFECT			20
#define	MASK_TXN_SPECIAL_EFFECT			(((unsigned int) 0x3)<<B_TXN_SPECIAL_EFFECT)
#define	B_TXN_WINDOW					17
#define	MASK_TXN_WINDOW					(((unsigned int) 0x3)<<B_TXN_WINDOW)
#define	B_TXN_SIZE						14
#define	MASK_TXN_SIZE					(((unsigned int) 0x7)<<B_TXN_SIZE)
#define	B_TXN_DEPTH						12
#define	MASK_TXN_DEPTH					(((unsigned int) 0x3)<<B_TXN_DEPTH)
#define	B_TXN_PALETTE					8
#define	MASK_TXN_PALETTE				(((unsigned int) 0xF)<<B_TXN_PALETTE)
#define	B_TXN_VS						6
#define	MASK_TXN_VS						(((unsigned int) 0x3)<<B_TXN_VS)
#define	B_TXN_HS						4
#define	MASK_TXN_HS						(((unsigned int) 0x3)<<B_TXN_HS)
#define	B_TXN_FLIP						2
#define	MASK_TXN_FLIP					(((unsigned int) 0x3)<<B_TXN_FLIP)
#define	TXN_NO_FLIP						0
#define	TXN_H_FLIP						1
#define	TXN_V_FLIP						2
#define	TXN_HV_FLIP						3
#define	B_TXN_COLOR						0
#define	MASK_TXN_COLOR					(((unsigned int) 0x3)<<B_TXN_COLOR)

#define	B_TXN_INTP					    16
#define	TXN_INTP_DISABLE				(((unsigned int) 0)<<B_TXN_INTP)
#define	TXN_INTP_ENABLE					(((unsigned int) 1)<<B_TXN_INTP)
#define	B_TXN_BLDLVL					10
#define	MASK_TXN_BLDLVL					(((unsigned int) 0x3F)<<B_TXN_BLDLVL)
#define	B_TXN_BLDMODE					9
#define	TXN_BLDMODE64_DISABLE			(((unsigned int) 0)<<B_TXN_BLDMODE)
#define	TXN_BLDMODE64_ENABLE			(((unsigned int) 1)<<B_TXN_BLDMODE)
#define	B_TXN_BLD						8
#define	TXN_BLD_DISABLE					(((unsigned int) 0)<<B_TXN_BLD)
#define	TXN_BLD_ENABLE					(((unsigned int) 1)<<B_TXN_BLD)
#define	B_TXN_RGBM						7
#define	TXN_RGB15M						(((unsigned int) 0x0)<<B_TXN_RGBM)
#define	TXN_RGB15P						(((unsigned int) 0x1)<<B_TXN_RGBM)
#define	B_TXN_MODE						5
#define	MASK_TXN_MODE					(((unsigned int) 0x3)<<B_TXN_MODE)
#define	B_TXN_MVE						4
#define	TXN_MVE_DISABLE					(((unsigned int) 0)<<B_TXN_MVE)
#define	TXN_MVE_ENABLE					(((unsigned int) 1)<<B_TXN_MVE)
#define	B_TXN_EN						3
#define	TXN_DISABLE						(((unsigned int) 0)<<B_TXN_EN)
#define	TXN_ENABLE						(((unsigned int) 1)<<B_TXN_EN)
#define	B_TXN_WALL						2
#define	TXN_WALL_DISABLE				(((unsigned int) 0)<<B_TXN_WALL)
#define	TXN_WALL_ENABLE					(((unsigned int) 1)<<B_TXN_WALL)
#define	B_TXN_REGM						1
#define	TXN_REGMODE						(((unsigned int) 1)<<B_TXN_REGM)
#define	B_TXN_BMP						0
#define	TXN_BMP							(((unsigned int) 1)<<B_TXN_BMP)
#define	TXN_CHAR						(((unsigned int) 0)<<B_TXN_BMP)

//------------------------------------------------------------------------------------------------------
// Define Sprite Rotate Level 0 - 63
//------------------------------------------------------------------------------------------------------
#define					SP_ROTATE0			(0 << 10)
#define					SP_ROTATE1			(1 << 10)
#define					SP_ROTATE2			(2 << 10)
#define					SP_ROTATE3			(3 << 10)
#define					SP_ROTATE4			(4 << 10)
#define					SP_ROTATE5			(5 << 10)
#define					SP_ROTATE6			(6 << 10)
#define					SP_ROTATE7			(7 << 10)
#define					SP_ROTATE8			(8 << 10)
#define					SP_ROTATE9			(9 << 10)
#define					SP_ROTATE10			(10 << 10)
#define					SP_ROTATE11			(11 << 10)
#define					SP_ROTATE12			(12 << 10)
#define					SP_ROTATE13			(13 << 10)
#define					SP_ROTATE14			(14 << 10)
#define					SP_ROTATE15			(15 << 10)
#define					SP_ROTATE16			(16 << 10)
#define					SP_ROTATE17			(17 << 10)
#define					SP_ROTATE18			(18 << 10)
#define					SP_ROTATE19			(19 << 10)
#define					SP_ROTATE20			(20 << 10)
#define					SP_ROTATE21			(21 << 10)
#define					SP_ROTATE22			(22 << 10)
#define					SP_ROTATE23			(23 << 10)
#define					SP_ROTATE24			(24 << 10)
#define					SP_ROTATE25			(25 << 10)
#define					SP_ROTATE26			(26 << 10)
#define					SP_ROTATE27			(27 << 10)
#define					SP_ROTATE28			(28 << 10)
#define					SP_ROTATE29			(29 << 10)
#define					SP_ROTATE30			(30 << 10)
#define					SP_ROTATE31			(31 << 10)
#define					SP_ROTATE32			(32 << 10)
#define					SP_ROTATE33			(33 << 10)
#define					SP_ROTATE34			(34 << 10)
#define					SP_ROTATE35			(35 << 10)
#define					SP_ROTATE36			(36 << 10)
#define					SP_ROTATE37			(37 << 10)
#define					SP_ROTATE38			(38 << 10)
#define					SP_ROTATE39			(39 << 10)
#define					SP_ROTATE40			(40 << 10)
#define					SP_ROTATE41			(41 << 10)
#define					SP_ROTATE42			(42 << 10)
#define					SP_ROTATE43			(43 << 10)
#define					SP_ROTATE44			(44 << 10)
#define					SP_ROTATE45			(45 << 10)
#define					SP_ROTATE46			(46 << 10)
#define					SP_ROTATE47			(47 << 10)
#define					SP_ROTATE48			(48 << 10)
#define					SP_ROTATE49			(49 << 10)
#define					SP_ROTATE50			(50 << 10)
#define					SP_ROTATE51			(51 << 10)
#define					SP_ROTATE52			(52 << 10)
#define					SP_ROTATE53			(53 << 10)
#define					SP_ROTATE54			(54 << 10)
#define					SP_ROTATE55			(55 << 10)
#define					SP_ROTATE56			(56 << 10)
#define					SP_ROTATE57			(57 << 10)
#define					SP_ROTATE58			(58 << 10)
#define					SP_ROTATE59			(59 << 10)
#define					SP_ROTATE60			(60 << 10)
#define					SP_ROTATE61			(61 << 10)
#define					SP_ROTATE62			(62 << 10)
#define					SP_ROTATE63			(63 << 10)
//------------------------------------------------------------------------------------------------------
// Define Sprite Zoom Level 0 - 63
//------------------------------------------------------------------------------------------------------
#define					SP_ZOOM0			(0 << 10)
#define					SP_ZOOM1			(1 << 10)
#define					SP_ZOOM2			(2 << 10)
#define					SP_ZOOM3			(3 << 10)
#define					SP_ZOOM4			(4 << 10)
#define					SP_ZOOM5			(5 << 10)
#define					SP_ZOOM6			(6 << 10)
#define					SP_ZOOM7			(7 << 10)
#define					SP_ZOOM8			(8 << 10)
#define					SP_ZOOM9			(9 << 10)
#define					SP_ZOOM10			(10 << 10)
#define					SP_ZOOM11			(11 << 10)
#define					SP_ZOOM12			(12 << 10)
#define					SP_ZOOM13			(13 << 10)
#define					SP_ZOOM14			(14 << 10)
#define					SP_ZOOM15			(15 << 10)
#define					SP_ZOOM16			(16 << 10)
#define					SP_ZOOM17			(17 << 10)
#define					SP_ZOOM18			(18 << 10)
#define					SP_ZOOM19			(19 << 10)
#define					SP_ZOOM20			(20 << 10)
#define					SP_ZOOM21			(21 << 10)
#define					SP_ZOOM22			(22 << 10)
#define					SP_ZOOM23			(23 << 10)
#define					SP_ZOOM24			(24 << 10)
#define					SP_ZOOM25			(25 << 10)
#define					SP_ZOOM26			(26 << 10)
#define					SP_ZOOM27			(27 << 10)
#define					SP_ZOOM28			(28 << 10)
#define					SP_ZOOM29			(29 << 10)
#define					SP_ZOOM30			(30 << 10)
#define					SP_ZOOM31			(31 << 10)
#define					SP_ZOOM32			(32 << 10)
#define					SP_ZOOM33			(33 << 10)
#define					SP_ZOOM34			(34 << 10)
#define					SP_ZOOM35			(35 << 10)
#define					SP_ZOOM36			(36 << 10)
#define					SP_ZOOM37			(37 << 10)
#define					SP_ZOOM38			(38 << 10)
#define					SP_ZOOM39			(39 << 10)
#define					SP_ZOOM40			(40 << 10)
#define					SP_ZOOM41			(41 << 10)
#define					SP_ZOOM42			(42 << 10)
#define					SP_ZOOM43			(43 << 10)
#define					SP_ZOOM44			(44 << 10)
#define					SP_ZOOM45			(45 << 10)
#define					SP_ZOOM46			(46 << 10)
#define					SP_ZOOM47			(47 << 10)
#define					SP_ZOOM48			(48 << 10)
#define					SP_ZOOM49			(49 << 10)
#define					SP_ZOOM50			(50 << 10)
#define					SP_ZOOM51			(51 << 10)
#define					SP_ZOOM52			(52 << 10)
#define					SP_ZOOM53			(53 << 10)
#define					SP_ZOOM54			(54 << 10)
#define					SP_ZOOM55			(55 << 10)
#define					SP_ZOOM56			(56 << 10)
#define					SP_ZOOM57			(57 << 10)
#define					SP_ZOOM58			(58 << 10)
#define					SP_ZOOM59			(59 << 10)
#define					SP_ZOOM60			(60 << 10)
#define					SP_ZOOM61			(61 << 10)
#define					SP_ZOOM62			(62 << 10)
#define					SP_ZOOM63			(63 << 10)
//------------------------------------------------------------------------------------------------------
// Define Sprite Blend Level 0 - 63
//------------------------------------------------------------------------------------------------------
#define					SPBLEND_LEVEL0			(0 << 8)
#define					SPBLEND_LEVEL1			(1 << 8)
#define					SPBLEND_LEVEL2			(2 << 8)
#define					SPBLEND_LEVEL3			(3 << 8)
#define					SPBLEND_LEVEL4			(4 << 8)
#define					SPBLEND_LEVEL5			(5 << 8)
#define					SPBLEND_LEVEL6			(6 << 8)
#define					SPBLEND_LEVEL7			(7 << 8)
#define					SPBLEND_LEVEL8			(8 << 8)
#define					SPBLEND_LEVEL9			(9 << 8)
#define					SPBLEND_LEVEL10			(10 << 8)
#define					SPBLEND_LEVEL11			(11 << 8)
#define					SPBLEND_LEVEL12			(12 << 8)
#define					SPBLEND_LEVEL13			(13 << 8)
#define					SPBLEND_LEVEL14			(14 << 8)
#define					SPBLEND_LEVEL15			(15 << 8)
#define					SPBLEND_LEVEL16			(16 << 8)
#define					SPBLEND_LEVEL17			(17 << 8)
#define					SPBLEND_LEVEL18			(18 << 8)
#define					SPBLEND_LEVEL19			(19 << 8)
#define					SPBLEND_LEVEL20			(20 << 8)
#define					SPBLEND_LEVEL21			(21 << 8)
#define					SPBLEND_LEVEL22			(22 << 8)
#define					SPBLEND_LEVEL23			(23 << 8)
#define					SPBLEND_LEVEL24			(24 << 8)
#define					SPBLEND_LEVEL25			(25 << 8)
#define					SPBLEND_LEVEL26			(26 << 8)
#define					SPBLEND_LEVEL27			(27 << 8)
#define					SPBLEND_LEVEL28			(28 << 8)
#define					SPBLEND_LEVEL29			(29 << 8)
#define					SPBLEND_LEVEL30			(30 << 8)
#define					SPBLEND_LEVEL31			(31 << 8)
#define					SPBLEND_LEVEL32			(32 << 8)
#define					SPBLEND_LEVEL33			(33 << 8)
#define					SPBLEND_LEVEL34			(34 << 8)
#define					SPBLEND_LEVEL35			(35 << 8)
#define					SPBLEND_LEVEL36			(36 << 8)
#define					SPBLEND_LEVEL37			(37 << 8)
#define					SPBLEND_LEVEL38			(38 << 8)
#define					SPBLEND_LEVEL39			(39 << 8)
#define					SPBLEND_LEVEL40			(40 << 8)
#define					SPBLEND_LEVEL41			(41 << 8)
#define					SPBLEND_LEVEL42			(42 << 8)
#define					SPBLEND_LEVEL43			(43 << 8)
#define					SPBLEND_LEVEL44			(44 << 8)
#define					SPBLEND_LEVEL45			(45 << 8)
#define					SPBLEND_LEVEL46			(46 << 8)
#define					SPBLEND_LEVEL47			(47 << 8)
#define					SPBLEND_LEVEL48			(48 << 8)
#define					SPBLEND_LEVEL49			(49 << 8)
#define					SPBLEND_LEVEL50			(50 << 8)
#define					SPBLEND_LEVEL51			(51 << 8)
#define					SPBLEND_LEVEL52			(52 << 8)
#define					SPBLEND_LEVEL53			(53 << 8)
#define					SPBLEND_LEVEL54			(54 << 8)
#define					SPBLEND_LEVEL55			(55 << 8)
#define					SPBLEND_LEVEL56			(56 << 8)
#define					SPBLEND_LEVEL57			(57 << 8)
#define					SPBLEND_LEVEL58			(58 << 8)
#define					SPBLEND_LEVEL59			(59 << 8)
#define					SPBLEND_LEVEL60			(60 << 8)
#define					SPBLEND_LEVEL61			(61 << 8)
#define					SPBLEND_LEVEL62			(62 << 8)
#define					SPBLEND_LEVEL63			(63 << 8)
//------------------------------------------------------------------------------------------------------
// Define Sprite Mosaic Level 0 - 3
//------------------------------------------------------------------------------------------------------
#define					SP_MOSAIC0			(0 << 14)
#define					SP_MOSAIC1			(1 << 14)
#define					SP_MOSAIC2			(2 << 14)
#define					SP_MOSAIC3			(3 << 14)
//------------------------------------------------------------------------------------------------------
// Define Sprite 16-bit Palette Select
//------------------------------------------------------------------------------------------------------
#define					SP_PBANK0			 0 	
#define					SP_PBANK1			 1
#define					SP_PBANK2			 2	
#define					SP_PBANK3			 3   
//------------------------------------------------------------------------------------------------------
// Define Sprite Blending Effect Disable/Enable
//------------------------------------------------------------------------------------------------------
#define					SPBLEND_DISABLE		(0 << 14)	// Set Sprite Blending effect disable
#define					SPBLEND_ENABLE		(1 << 14)   // Set Sprite Blending effect enable
//------------------------------------------------------------------------------------------------------
// Define Sprite Depth 0 - 3
//------------------------------------------------------------------------------------------------------
#define					SP_DEPTH1 			(0 << 12)	
#define					SP_DEPTH3 			(1 << 12)	
#define					SP_DEPTH5 			(2 << 12)	
#define					SP_DEPTH7 			(3 << 12)	
//------------------------------------------------------------------------------------------------------
// Define Sprite Palette bank 0 - 15
//------------------------------------------------------------------------------------------------------
#define					SP_PALETTE0 		(0 << 8)	
#define					SP_PALETTE1 		(1 << 8)
#define					SP_PALETTE2 		(2 << 8)
#define					SP_PALETTE3 		(3 << 8)
#define					SP_PALETTE4 		(4 << 8)
#define					SP_PALETTE5 		(5 << 8)
#define					SP_PALETTE6 		(6 << 8)
#define					SP_PALETTE7 		(7 << 8)
#define					SP_PALETTE8 		(8 << 8)
#define					SP_PALETTE9 		(9 << 8)
#define					SP_PALETTE10 		(10 << 8)
#define					SP_PALETTE11 		(11 << 8)
#define					SP_PALETTE12 		(12 << 8)
#define					SP_PALETTE13 		(13 << 8)
#define					SP_PALETTE14 		(14 << 8)
#define					SP_PALETTE15 		(15 << 8)
//------------------------------------------------------------------------------------------------------
// Define Sprite Character size for Y
//------------------------------------------------------------------------------------------------------
#define					SP_VSIZE8 			(0 << 6)	
#define					SP_VSIZE16 			(1 << 6)	
#define					SP_VSIZE32 			(2 << 6)	
#define					SP_VSIZE64 			(3 << 6)	
//------------------------------------------------------------------------------------------------------
// Define Sprite Character size for X
//------------------------------------------------------------------------------------------------------
#define					SP_HSIZE8 			(0 << 4)	
#define					SP_HSIZE16 			(1 << 4)
#define					SP_HSIZE32 			(2 << 4)
#define					SP_HSIZE64 			(3 << 4)
//------------------------------------------------------------------------------------------------------
// Define Sprite Large Character size for Y
//------------------------------------------------------------------------------------------------------
#define					SP_VSIZE_L32 		(0 << 6)	
#define					SP_VSIZE_L64 		(1 << 6)	
#define					SP_VSIZE_L128 		(2 << 6)	
#define					SP_VSIZE_L256 		(3 << 6)	
//------------------------------------------------------------------------------------------------------
// Define Sprite Large Character size for X
//------------------------------------------------------------------------------------------------------
#define					SP_HSIZE_L32 		(0 << 4)	
#define					SP_HSIZE_L64 		(1 << 4)
#define					SP_HSIZE_L128 		(2 << 4)
#define					SP_HSIZE_L256 		(3 << 4)
//------------------------------------------------------------------------------------------------------
// Define Sprite H/V-Flip Disable/Enable 
//------------------------------------------------------------------------------------------------------
#define					SPVFLIP_DISABLE 	0x0000	
#define					SPVFLIP_ENABLE 		0x0008	
#define					SPHFLIP_DISABLE 	0x0000	
#define					SPHFLIP_ENABLE 		0x0004	
//------------------------------------------------------------------------------------------------------
// Define Sprite Color Mode
//------------------------------------------------------------------------------------------------------
#define					SP_COLOR4 			0x0000
#define					SP_COLOR16 			0x0001
#define					SP_COLOR64 			0x0002
#define					SP_COLOR256 		0x0003
//------------------------------------------------------------------------------------------------------
// Sprite Palette page 0 to 15
//------------------------------------------------------------------------------------------------------
#define					SpPalettePage0    	0x0
#define					SpPalettePage1    	0x1
#define 				SpPalettePage2    	0x2
#define 				SpPalettePage3    	0x3
#define 				SpPalettePage4    	0x4
#define 				SpPalettePage5    	0x5
#define 				SpPalettePage6    	0x6
#define 				SpPalettePage7    	0x7
#define 				SpPalettePage8    	0x8
#define 				SpPalettePage9    	0x9
#define 				SpPalettePage10   	0xa
#define 				SpPalettePage11   	0xb
#define 				SpPalettePage12   	0xc
#define 				SpPalettePage13   	0xd
#define 				SpPalettePage14   	0xe
#define 				SpPalettePage15   	0xf
//------------------------------------------------------------------------------------------------------
// Sprite Depth
//------------------------------------------------------------------------------------------------------
#define 				SpriteDepth1      	0x0
#define 				SpriteDepth3        0x1
#define 				SpriteDepth5        0x2
#define 				SpriteDepth7       	0x3
//------------------------------------------------------------------------------------------------------
// Define Sprite Group Function ID
//------------------------------------------------------------------------------------------------------
#define 				SpriteNoGroupID     0x0                   // Set Sprite Group Function Disable
#define 				SpriteGroupID1      0x1                   // Set Sprite GroupID 1
#define 				SpriteGroupID2      0x2                   // Set Sprite GroupID 2
#define 				SpriteGroupID3      0x3                   // Set Sprite GroupID 3
//------------------------------------------------------------------------------------------------------
// Define Sprite Large Size Disable/Enable
//------------------------------------------------------------------------------------------------------
#define					SPLarge_DISABLE		        (0 << 2)	  // Set Sprite Large size effect disable
#define					SPLarge_ENABLE		        (1 << 2)      // Set Sprite Large size effect enable
//------------------------------------------------------------------------------------------------------
// Define Sprite Bi-liner Interpolation Disable/Enable
//------------------------------------------------------------------------------------------------------
#define					SPInterpolation_DISABLE		(0 << 3)	  // Set Sprite interpolation effect disable
#define					SPInterpolation_ENABLE		(1 << 3)      // Set Sprite interpolation effect enable

// Color Mode Definitions
#define TXN_COLOR_4                     0
#define TXN_COLOR_16                    1
#define TXN_COLOR_64                    2
#define TXN_COLOR_256                   3
#define TXN_COLOR_32768                 4
#define TXN_COLOR_65536                 5
#define TXN_COLOR_RGBG0                 6
#define TXN_COLOR_RGBG1                 7
#define TXN_COLOR_RGBG2                 8
#define TXN_COLOR_RGBG3                 9
#define TXN_COLOR_YUYV0                 10
#define TXN_COLOR_YUYV1                 11
#define TXN_COLOR_YUYV2                 12
#define TXN_COLOR_YUYV3                 13
#define TXN_COLOR_ALPHA_CHANNEL_BLEND   14
#define TXN_COLOR_ARGB4444              15
#define TXN_COLOR_RGBA                  16
//  Palette Index Definitions
#define TXN_PALETTE0                    0
#define TXN_PALETTE1                    1
#define TXN_PALETTE2                    2
#define TXN_PALETTE3                    3
#define TXN_PALETTE4                    4
#define TXN_PALETTE5                    5
#define TXN_PALETTE6                    6
#define TXN_PALETTE7                    7
#define TXN_PALETTE8                    8
#define TXN_PALETTE9                    9
#define TXN_PALETTE10                   10
#define TXN_PALETTE11                   11
#define TXN_PALETTE12                   12
#define TXN_PALETTE13                   13
#define TXN_PALETTE14                   14
#define TXN_PALETTE15                   15
//  PalRAM Definitions
#define PAL_RAM0                        0
#define PAL_RAM1                        1
#define PAL_RAM2                        2
#define PAL_RAM3                        3
//  CellY Definitions
#define TXN_VS0                         0
#define TXN_VS8                         8
#define TXN_VS16                        16
#define TXN_VS32                        32
#define TXN_VS64                        64
#define TXN_VS128                       128
#define TXN_VS256                       256
//  CellX Definitions
#define TXN_HS0                         0
#define TXN_HS8                         8
#define TXN_HS16                        16
#define TXN_HS32                        32
#define TXN_HS64                        64
#define TXN_HS128                       128
#define TXN_HS256                       256
//  TxNFlip Definitions
#define NO_FLIP                         0
#define H_FLIP                          1
#define V_FLIP                          2
#define HV_FLIP                         3
//  TxNSize Definitions
#define TXN_512X256                     0
#define TXN_512X512                     1
#define TXN_1024X512                    2
#define TXN_1024X1024                   3
#define TXN_2048X1024                   4
#define TXN_2048X2048                   5
#define TXN_4096X2048                   6
#define TXN_4096X4096                   7
//  nDepth Definitions
#define TXN_DEPTH0                      0
#define TXN_DEPTH2                      1
#define TXN_DEPTH4                      2
#define TXN_DEPTH6                      3
//  nPalType
#define PAL16_SEPARATE                  0
#define PAL25_SEPARATE                  1
#define PAL1024_DISABLE                 0
#define PAL1024_ENABLE                  1
//  Address mode Definitions
#define TEXT_RELATIVE                   0       
#define TEXT_DIRECT                     1      
//  Text mode Definitions
#define TEXT_CHARACTER                  0
#define TEXT_BITMAP                     1
//  Text Virtual 3D mode Definitions
#define TEXTV3D_DISABLE                 0
#define TEXTV3D_ENABLE                  1
//  Text Bi-Interpolation mode Definitions
#define TEXTINIT_DISABLE                 0
#define TEXTINIT_ENABLE                  1
//  Text Blend Level Definitions
#define	TXBLEND_LEVEL0			            0 
#define	TXBLEND_LEVEL1			            1
#define	TXBLEND_LEVEL2			            2
#define	TXBLEND_LEVEL3			            3
#define	TXBLEND_LEVEL4			            4
#define	TXBLEND_LEVEL5			            5
#define	TXBLEND_LEVEL6			            6
#define	TXBLEND_LEVEL7			            7
#define	TXBLEND_LEVEL8			            8 
#define	TXBLEND_LEVEL9			            9
#define	TXBLEND_LEVEL10			            10
#define	TXBLEND_LEVEL11			            11
#define	TXBLEND_LEVEL12			            12
#define	TXBLEND_LEVEL13			            13
#define	TXBLEND_LEVEL14			            14
#define	TXBLEND_LEVEL15			            15
#define	TXBLEND_LEVEL16			            16
#define	TXBLEND_LEVEL17			            17
#define	TXBLEND_LEVEL18			            18
#define	TXBLEND_LEVEL19			            19
#define	TXBLEND_LEVEL20			            20
#define	TXBLEND_LEVEL21			            21
#define	TXBLEND_LEVEL22			            22
#define	TXBLEND_LEVEL23			            23
#define	TXBLEND_LEVEL24			            24
#define	TXBLEND_LEVEL25			            25
#define	TXBLEND_LEVEL26			            26
#define	TXBLEND_LEVEL27			            27
#define	TXBLEND_LEVEL28			            28
#define	TXBLEND_LEVEL29			            29
#define	TXBLEND_LEVEL30			            30
#define	TXBLEND_LEVEL31			            31
#define	TXBLEND_LEVEL32			            32
#define	TXBLEND_LEVEL33			            33
#define	TXBLEND_LEVEL34			            34
#define	TXBLEND_LEVEL35			            35
#define	TXBLEND_LEVEL36			            36
#define	TXBLEND_LEVEL37			            37
#define	TXBLEND_LEVEL38			            38
#define	TXBLEND_LEVEL39			            39
#define	TXBLEND_LEVEL40			            40
#define	TXBLEND_LEVEL41			            41
#define	TXBLEND_LEVEL42			            42
#define	TXBLEND_LEVEL43			            43
#define	TXBLEND_LEVEL44			            44
#define	TXBLEND_LEVEL45			            45
#define	TXBLEND_LEVEL46			            46
#define	TXBLEND_LEVEL47			            47
#define	TXBLEND_LEVEL48			            48
#define	TXBLEND_LEVEL49			            49
#define	TXBLEND_LEVEL50			            50
#define	TXBLEND_LEVEL51			            51
#define	TXBLEND_LEVEL52			            52
#define	TXBLEND_LEVEL53			            53
#define	TXBLEND_LEVEL54 		            54
#define	TXBLEND_LEVEL55			            55
#define	TXBLEND_LEVEL56			            56
#define	TXBLEND_LEVEL57			            57
#define	TXBLEND_LEVEL58			            58
#define	TXBLEND_LEVEL59			            59
#define	TXBLEND_LEVEL60			            60
#define	TXBLEND_LEVEL61			            61
#define	TXBLEND_LEVEL62			            62
#define	TXBLEND_LEVEL63			            63
/*
// PPU Frame buffer Color Mode Definitions
#define PPU_COLOR_RGB565                0
#define PPU_COLOR_MONO                  1
#define PPU_COLOR_4                     2
#define PPU_COLOR_16                    3
#define PPU_COLOR_RGBG                  4
#define PPU_COLOR_YUYV                  5
#define PPU_COLOR_RGBA                  6
// PPU Resolution Mode Definitions
#define	PPU_TFT_RESOLUTION_320X240		  0
#define	PPU_TFT_RESOLUTION_640X480		  1
#define	PPU_TFT_RESOLUTION_480X234		  2
#define	PPU_TFT_RESOLUTION_480X272		  3
#define	PPU_TFT_RESOLUTION_720X480		  4
#define	PPU_TFT_RESOLUTION_800X480		  5
#define	PPU_TFT_RESOLUTION_800X600		  6
#define	PPU_TFT_RESOLUTION_1024X768		  7
#define	PPU_TV_RESOLUTION_320X240			  8
#define	PPU_TV_RESOLUTION_640X480			  9
#define	PPU_TV_RESOLUTION_720X480			  10
*/
#define	C_TFT_RESOLUTION_320X240		0
#define	C_TFT_RESOLUTION_640X480		1
#define	C_TFT_RESOLUTION_480X234		2
#define	C_TFT_RESOLUTION_480X272		3
#define	C_TFT_RESOLUTION_720X480		4
#define	C_TFT_RESOLUTION_800X480		5
#define	C_TFT_RESOLUTION_800X600		6
#define	C_TFT_RESOLUTION_1024X768		7
#define	C_TV_RESOLUTION_320X240			0
#define	C_TV_RESOLUTION_640X480			0
#define	C_TV_RESOLUTION_720X480			4

// image display
#define IMAGE_OUTPUT_FORMAT_RGB565      0x10
#define IMAGE_OUTPUT_FORMAT_RGBG        0x11
#define IMAGE_OUTPUT_FORMAT_GRGB        0x12
#define IMAGE_OUTPUT_FORMAT_UYVY        0x13
#define IMAGE_OUTPUT_FORMAT_YUYV        0x14
#define IMAGE_OUTPUT_FORMAT_VYUY        0x15

#define QVGA_MODE                       0x20
#define VGA_MODE                        0x21
#define D1_MODE                         0x22
#define DISPLAY_TV              		    0x30
#define DISPLAY_TFT						          0

// PPU Module Interrupt Pending Flag
#define PPU_MODULE_PPU_VBLANK			      0x00000001
#define PPU_MODULE_VIDEO_POSITION		    0x00000002
#define PPU_MODULE_DMA_COMPLETE			    0x00000004
#define PPU_MODULE_PALETTE_ERROR		    0x00000008
#define PPU_MODULE_TEXT_UNDERRUN		    0x00000010
#define PPU_MODULE_SPRITE_UNDERRUN		  0x00000020
#define PPU_MODULE_SENSOR_FRAME_END		  0x00000040
#define PPU_MODULE_MOTION_DETECT		    0x00000080
#define PPU_MODULE_SENSOR_POSITION_HIT	0x00000100
#define PPU_MODULE_MOTION_UNDERRUN		  0x00000200
#define PPU_MODULE_TV_UNDERRUN			    0x00000400
#define PPU_MODULE_TV_VBLANK			      0x00000800
#define PPU_MODULE_TFT_UNDERRUN			    0x00001000
#define PPU_MODULE_TFT_VBLANK			      0x00002000
#define PPU_MODULE_PPU_HBLANK			      0x00004000
#define PPU_MODULE_SENSOR_UNDERRUN			0x00008000
#define PPU_MODULE_IIIEGAL_WRITE			  0x00020000

/*
 * ioctl calls that are permitted to the /dev/ppu interface, if
 * any of the ppu moudle are enabled.
 */
#define PPUIO_TYPE 'D'
/* PPU Module */
#define PPUIO_SET_INIT	                          		_IOW(PPUIO_TYPE, 0x01, unsigned int)	
#define PPUIO_SET_ENABLE	                    				_IOW(PPUIO_TYPE, 0x02, unsigned int)			  /* Set PPU Hardware Enable*/
#define PPUIO_SET_CHAR0_TRANSPARENT           				_IOW(PPUIO_TYPE, 0x03, unsigned int)	
#define PPUIO_SET_BOTTOM_UP                   				_IOW(PPUIO_TYPE, 0x04, unsigned int)																																					
#define PPUIO_SET_VGA                     						_IOW(PPUIO_TYPE, 0x05, unsigned int)																																				
#define PPUIO_SET_NON_INTERLACE           						_IOW(PPUIO_TYPE, 0x06, unsigned int)																																				
#define PPUIO_SET_FRAME_BUFFER            						_IOW(PPUIO_TYPE, 0x07, unsigned int)																																				
#define PPUIO_SET_FB_FORMAT               						_IOW(PPUIO_TYPE, 0x08, unsigned int)																																				
#define PPUIO_SET_SAVE_ROM                						_IOW(PPUIO_TYPE, 0x09, unsigned int)																																				
#define PPUIO_SET_RESOLUTION              						_IOW(PPUIO_TYPE, 0x0A, unsigned int)																																				
#define PPUIO_SET_YUV_TYPE                						_IOW(PPUIO_TYPE, 0x0B, unsigned int)																																				
#define PPUIO_SET_COLOR_MAPPING           						_IOW(PPUIO_TYPE, 0x0C, unsigned int)																																				
#define PPUIO_SET_TFT_LONG_BURST          						_IOW(PPUIO_TYPE, 0x0D, unsigned int)																																				
#define PPUIO_SET_LONG_BURST              						_IOW(PPUIO_TYPE, 0x0E, unsigned int)																																				
#define PPUIO_SET_BLEND4                  						_IOW(PPUIO_TYPE, 0x0F, unsigned int)																																				
#define PPUIO_SET_RGB565_TRANSPARENT_COLOR    				_IOW(PPUIO_TYPE, 0x10, unsigned int)																																				
#define PPUIO_SET_FADE_EFFECT              						_IOW(PPUIO_TYPE, 0x11, unsigned int)																																				
#define PPUIO_SET_WINDOW                 		  				_IOW(PPUIO_TYPE, 0x12, unsigned int)																																				
#define PPUIO_SET_PALETTE_TYPE    										_IOW(PPUIO_TYPE, 0x13, unsigned int)																																				
#define PPUIO_SET_PALETTE_RAM             						_IOW(PPUIO_TYPE, 0x14, unsigned int)																																				
#define PPUIO_ADD_FRAME_BUFFER                				_IOW(PPUIO_TYPE, 0x15, unsigned int)																																				
#define PPUIO_SET_DEFLICKER   												_IOW(PPUIO_TYPE, 0x16, unsigned int)																																				
#define PPUIO_SET_SP_ADDRX2             		  				_IOW(PPUIO_TYPE, 0x17, unsigned int)																																				
#define PPUIO_PPUGO_WITHOUT_WAIT              				_IOW(PPUIO_TYPE, 0x18, unsigned int)																																				
#define PPUIO_PPUGO  								          				_IOW(PPUIO_TYPE, 0x19, unsigned int)																																				
#define PPUIO_PPUGO_AND_WAIT_DONE  										_IOW(PPUIO_TYPE, 0x1A, unsigned int)																																				
#define PPUIO_SET_DUAL_BLEND                  				_IOW(PPUIO_TYPE, 0x1B, unsigned int)																																				
#define PPUIO_SET_FREE_SIZE  								  				_IOW(PPUIO_TYPE, 0x1C, unsigned int)																																				
#define PPUIO_SET_TEXT_RGBA 						      				_IOW(PPUIO_TYPE, 0x1D, unsigned int)																																				
#define PPUIO_SET_TEXT_ALPHA                  				_IOW(PPUIO_TYPE, 0x1E, unsigned int)																																				
#define PPUIO_SET_SPRITE_RGBA 												_IOW(PPUIO_TYPE, 0x1F, unsigned int)																																				
#define PPUIO_SET_TEXT_NEW_SPECIALBMP				 					 _IOW(PPUIO_TYPE, 0x20, unsigned int)																																				
#define PPUIO_SET_TEXT_NEW_COMPRESSION        				_IOW(PPUIO_TYPE, 0x21, unsigned int)																																				
#define PPUIO_SET_PPU_DELGO								    				_IOW(PPUIO_TYPE, 0x22, unsigned int)																																				
#define PPUIO_SET_TFTVTQ            				  				_IOW(PPUIO_TYPE, 0x23, unsigned int)																																				
#define PPUIO_SET_TV_LONG_BURST               				_IOW(PPUIO_TYPE, 0x24, unsigned int)																																				
#define PPUIO_SET_INTERPOLATION												_IOW(PPUIO_TYPE, 0x25, unsigned int)																																				
#define PPUIO_SET_FRAME_BUFFER_OUTPUT_FIFO    				_IOW(PPUIO_TYPE, 0x26, unsigned int)																																				
#define PPUIO_SET_PPUFIFO_GO_AND_WAIT_DONE    				_IOW(PPUIO_TYPE, 0x27, unsigned int)
#define PPUIO_SET_PPU_FRAME_BUFFER_GET    				    _IOW(PPUIO_TYPE, 0x28, unsigned int)																																				
#define PPUIO_SET_PPU_FRAME_BUFFER_RELEASE    				_IOW(PPUIO_TYPE, 0x29, unsigned int)
                                              																																				
/* PPU TEXT Module */                         																																				
#define PPUIO_TEXT_SET_INIT                       	  _IOW(PPUIO_TYPE, 0x30, unsigned int)																																				
#define PPUIO_TEXT_SET_ENABLE                 				_IOW(PPUIO_TYPE, 0x31, unsigned int)																																				
#define PPUIO_TEXT_SET_COMPRESS_DISABLE       				_IOW(PPUIO_TYPE, 0x32, unsigned int)																																				
#define PPUIO_TEXT_SET_MODE                   				_IOW(PPUIO_TYPE, 0x33, unsigned int)																																				
#define PPUIO_TEXT_SET_DIRECT_MODE            				_IOW(PPUIO_TYPE, 0x34, unsigned int)																																				
#define PPUIO_TEXT_SET_WALLPAPER_MODE         				_IOW(PPUIO_TYPE, 0x35, unsigned int)																																																						
#define PPUIO_TEXT_SET_ATTRIBUTE_SOURCE       				_IOW(PPUIO_TYPE, 0x36, unsigned int)																																																						
#define PPUIO_TEXT_SET_HORIZONTAL_MOVE_ENABLE 				_IOW(PPUIO_TYPE, 0x37, unsigned int)																																																						
#define PPUIO_TEXT_SET_TEXT_SIZE              				_IOW(PPUIO_TYPE, 0x38, unsigned int)																																																						
#define PPUIO_TEXT_SET_CHARACTER_SIZE         				_IOW(PPUIO_TYPE, 0x39, unsigned int)																																																						
#define PPUIO_TEXT_SET_BITMAP_MODE            				_IOW(PPUIO_TYPE, 0x3A, unsigned int)																																																						
#define PPUIO_TEXT_SET_COLOR                  				_IOW(PPUIO_TYPE, 0x3B, unsigned int)																																																						
#define PPUIO_TEXT_SET_PALETTE                				_IOW(PPUIO_TYPE, 0x3C, unsigned int)																																																						
#define PPUIO_TEXT_SET_SEGMENT                				_IOW(PPUIO_TYPE, 0x3D, unsigned int)																																																						
#define PPUIO_TEXT_SET_ATTRIBUTE_ARRAY_PTR   				 	_IOW(PPUIO_TYPE, 0x3E, unsigned int)
#define PPUIO_TEXT_SET_NUMBER_ARRAY_PTR       			  _IOW(PPUIO_TYPE, 0x3F, unsigned int)
#define PPUIO_TEXT_CALCULATE_NUMBER_ARRAY             _IOW(PPUIO_TYPE, 0x40, unsigned int)
#define PPUIO_TEXT_SET_POSITION   				 	          _IOW(PPUIO_TYPE, 0x41, unsigned int)
#define PPUIO_TEXT_SET_OFFSET       			            _IOW(PPUIO_TYPE, 0x42, unsigned int)
#define PPUIO_TEXT_SET_DEPTH                          _IOW(PPUIO_TYPE, 0x43, unsigned int)
#define PPUIO_TEXT_SET_BLEND   				 	              _IOW(PPUIO_TYPE, 0x44, unsigned int)
#define PPUIO_TEXT_SET_FLIP       			              _IOW(PPUIO_TYPE, 0x45, unsigned int)
#define PPUIO_TEXT_SET_SINE_COSINE                    _IOW(PPUIO_TYPE, 0x46, unsigned int)
#define PPUIO_TEXT_SELECT_WINDOW   				 	          _IOW(PPUIO_TYPE, 0x47, unsigned int)
#define PPUIO_TEXT_SET_SPECIAL_EFFECT       			    _IOW(PPUIO_TYPE, 0x48, unsigned int)
#define PPUIO_TEXT_SET_VERTICAL_COMPRESS              _IOW(PPUIO_TYPE, 0x49, unsigned int)
#define PPUIO_TEXT_SET_HORIZONTAL_MOVE_PTR  				 	_IOW(PPUIO_TYPE, 0x4A, unsigned int)
#define PPUIO_TEXT_SET_HORIZONTAL_COMPRESS_PTR       	_IOW(PPUIO_TYPE, 0x4B, unsigned int)
#define PPUIO_TEXT_SET_ROTATE_ZOOM                    _IOW(PPUIO_TYPE, 0x4C, unsigned int)
#define PPUIO_TEXT3_SET_25D  				 	                _IOW(PPUIO_TYPE, 0x4D, unsigned int)
#define PPUIO_TEXT3_SET_25D_Y_COMPRESS              	_IOW(PPUIO_TYPE, 0x4E, unsigned int)
#define PPUIO_TEXT3_SET_25D_COSINEBUF                 _IOW(PPUIO_TYPE, 0x4F, unsigned int)
#define PPUIO_TEXT_SET_INTERPOLATION  				 	      _IOW(PPUIO_TYPE, 0x50, unsigned int)
#define PPUIO_TEXT_SET_COLOR_MASK              	      _IOW(PPUIO_TYPE, 0x51, unsigned int)
#define PPUIO_TEXT_UPDATE_NUMBER_ARRAY                _IOW(PPUIO_TYPE, 0x52, unsigned int)

/* PPU SPRITE Module */ 
#define PPUIO_SPRITE_SET_INIT	                        _IOW(PPUIO_TYPE, 0x60, unsigned int)
#define PPUIO_SPRITE_SET_ENABLE	                      _IOW(PPUIO_TYPE, 0x61, unsigned int)	
#define PPUIO_SPRITE_SET_COORDINATE	                  _IOW(PPUIO_TYPE, 0x62, unsigned int)	
#define PPUIO_SPRITE_SET_BLEND	                      _IOW(PPUIO_TYPE, 0x63, unsigned int)
#define PPUIO_SPRITE_SET_DIRECT	                      _IOW(PPUIO_TYPE, 0x64, unsigned int)	
#define PPUIO_SPRITE_SET_ZOOM_ENABLE	                _IOW(PPUIO_TYPE, 0x65, unsigned int)
#define PPUIO_SPRITE_SET_ROTATE_ENABLE	              _IOW(PPUIO_TYPE, 0x66, unsigned int)
#define PPUIO_SPRITE_SET_MOSAIC_ENABLE	              _IOW(PPUIO_TYPE, 0x67, unsigned int)	
#define PPUIO_SPRITE_SET_NUMBER	                      _IOW(PPUIO_TYPE, 0x68, unsigned int)
#define PPUIO_SPRITE_SET_SPECIAL_EFFECT	              _IOW(PPUIO_TYPE, 0x69, unsigned int)
#define PPUIO_SPRITE_SET_COLOR_DITHER	                _IOW(PPUIO_TYPE, 0x6A, unsigned int)	
#define PPUIO_SPRITE_SET_WINDOW                       _IOW(PPUIO_TYPE, 0x6B, unsigned int)
#define PPUIO_SPRITE_SET_SEGMENT                      _IOW(PPUIO_TYPE, 0x6C, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_RAM_PTR            _IOW(PPUIO_TYPE, 0x6D, unsigned int)	
#define PPUIO_SPRITE_SET_SFR	                        _IOW(PPUIO_TYPE, 0x6E, unsigned int)
#define PPUIO_SPRITE_SET_LARGE_ENABLE                 _IOW(PPUIO_TYPE, 0x6F, unsigned int)
#define PPUIO_SPRITE_SET_INTERPOLATION_ENABLE         _IOW(PPUIO_TYPE, 0x70, unsigned int)	
#define PPUIO_SPRITE_SET_GROUP_ENABLE	                _IOW(PPUIO_TYPE, 0x71, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_CDM_ENABLE         _IOW(PPUIO_TYPE, 0x72, unsigned int)
#define PPUIO_SPRITE_SET_FRACTION_ENABLE              _IOW(PPUIO_TYPE, 0x73, unsigned int)	
#define PPUIO_SPRITE_SET_EXTEND_ATTRIBUTE_RAM_PTR	    _IOW(PPUIO_TYPE, 0x74, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_ENABLE                  _IOW(PPUIO_TYPE, 0x75, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_CDM_ENABLE              _IOW(PPUIO_TYPE, 0x76, unsigned int)	
#define PPUIO_SPRITE_SET_EXSP_INTERPOLATION_ENABLE	  _IOW(PPUIO_TYPE, 0x77, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_LARGE_SIZE_ENABLE       _IOW(PPUIO_TYPE, 0x78, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_GROUP_ENABLE            _IOW(PPUIO_TYPE, 0x79, unsigned int)	
#define PPUIO_SPRITE_SET_EXSP_FRACTION_ENABLE	        _IOW(PPUIO_TYPE, 0x7A, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_NUMBER                  _IOW(PPUIO_TYPE, 0x7B, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_START_ADDRESS           _IOW(PPUIO_TYPE, 0x7C, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_2D_POSITION	      _IOW(PPUIO_TYPE, 0x7D, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_25D_POSITION       _IOW(PPUIO_TYPE, 0x7E, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_ROTATE             _IOW(PPUIO_TYPE, 0x7F, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_ZOOM	              _IOW(PPUIO_TYPE, 0x80, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_COLOR              _IOW(PPUIO_TYPE, 0x81, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_FLIP               _IOW(PPUIO_TYPE, 0x82, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_CHARACTER_SIZE     _IOW(PPUIO_TYPE, 0x83, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_PALETTE            _IOW(PPUIO_TYPE, 0x84, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_DEPTH              _IOW(PPUIO_TYPE, 0x85, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_BLEND64            _IOW(PPUIO_TYPE, 0x86, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_BLEND16            _IOW(PPUIO_TYPE, 0x87, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_WINDOW             _IOW(PPUIO_TYPE, 0x88, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_MOSAIC             _IOW(PPUIO_TYPE, 0x89, unsigned int)
#define PPUIO_SPRITE_SET_ATTRIBUTE_CHARNUM            _IOW(PPUIO_TYPE, 0x8A, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_ATTRIBUTE_GROUP         _IOW(PPUIO_TYPE, 0x8B, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_GROUP              _IOW(PPUIO_TYPE, 0x8C, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_ATTRIBUTE_LARGE_SIZE    _IOW(PPUIO_TYPE, 0x8D, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_LARGE_SIZE         _IOW(PPUIO_TYPE, 0x8E, unsigned int)
#define PPUIO_SPRITE_SET_EXSP_ATTRIBUTE_INTERPOLATION _IOW(PPUIO_TYPE, 0x8F, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_INTERPOLATION      _IOW(PPUIO_TYPE, 0x90, unsigned int)
#define PPUIO_SPRITE_SET_SP_ATTRIBUTE_CDM             _IOW(PPUIO_TYPE, 0x91, unsigned int)	
#define PPUIO_SPRITE_SET_ATTRIBUTE_COLOR_MASK         _IOW(PPUIO_TYPE, 0x92, unsigned int)
#define PPUIO_SPRITE_SET_IMAGE_DATA_INIT              _IOW(PPUIO_TYPE, 0x93, unsigned int)	
#define PPUIO_SPRITE_SET_DISPLAY                      _IOW(PPUIO_TYPE, 0x94, unsigned int)
#define PPUIO_SPRITE_SET_IMAGE_NUMBER                 _IOW(PPUIO_TYPE, 0x95, unsigned int)	
#define PPUIO_SPRITE_SET_DISABLE                      _IOW(PPUIO_TYPE, 0x96, unsigned int)
#define PPUIO_SPRITE_GET_IMAGE_INFO                   _IOW(PPUIO_TYPE, 0x97, unsigned int)	
#define PPUIO_SPRITE_PAINT_SPRITERAM                  _IOW(PPUIO_TYPE, 0x98, unsigned int)
#define PPUIO_SPRITE_SET_25D                          _IOW(PPUIO_TYPE, 0x99, unsigned int)
#define PPUIO_SPRITE_SET_SPRAM                        _IOW(PPUIO_TYPE, 0x9A, unsigned int)

#define DISPLAY_NONE_PPU_SET                          _IOW(PPUIO_TYPE, 0xFF, unsigned int)

/*******************************************************************************
*                          D A T A    T Y P E S
*******************************************************************************/
typedef struct {
	unsigned short	DISPLAY_device;                                
	unsigned short	DISPLAY_mode;                            
	unsigned short	DISPLAY_color_mode; 	                                                        	
	unsigned int	  DISPLAY_frame_buffer_ptr;	
} DISPLAY_NONE_PPU_STRUCT, *DISPLAY_NONE_PPU_STRUCT_PTR;

typedef struct {
	unsigned short	 PPU_enable_mode;                                
	unsigned short	 PPU_select_type_mode;
	unsigned short	 PPU_select_type_value_mode;
	unsigned int	   PPU_hsize_mode;                                
	unsigned int	   PPU_vsize_mode;
	unsigned int     PPU_buffer_ptr;	 	
} PPU_MOUDLE_STRUCT, *PPU_MOUDLE_STRUCT_PTR;

typedef struct {
	unsigned short	nCharNumLo_16;                      //  0.  CharNum low half-WORD set
	signed short	uPosX_16;                           //  1.  Sprite 2D X Position or Sprite Virtual 3D X0 Position
	signed short	uPosY_16;                           //  2.  Sprite 2D Y Position or Sprite Virtual 3D Y0 Position
	unsigned short	attr0;                              //  3.  Sprite Attribute 0
	unsigned short	attr1;                              //  4.  Sprite Attribute 1
	unsigned short	uX1_16;                             //  5.  Sprite Virtual 3D Position 1 
	unsigned short	uX2_16;                             //  6.  Sprite Virtual 3D Position 2 
	unsigned short	uX3_16;                             //  7.  Sprite Virtual 3D Position 3
} SpN_RAM, *PSpN_RAM;

typedef struct {
	signed short	ex_attr0;                           //  0.  Sprite Extend Attribute 0
	unsigned short	ex_attr1;                           //  1.  Sprite Extend Attribute 1
} SpN_EX_RAM, *PSpN_EX_RAM;

typedef struct {
	signed short x0;
	signed short y0;
	signed short x1;
	signed short y1;
	signed short x2;
	signed short y2;
	signed short x3;
	signed short y3;
} POS_STRUCT, *POS_STRUCT_PTR;
typedef struct {
	float x0;
	float y0;
	float x1;
	float y1;
	float x2;
	float y2;
	float x3;
	float y3;
} POS_STRUCT_GP32XXX, *POS_STRUCT_PTR_GP32XXX;

typedef struct {
  unsigned char group_id;                               // Sprite Group ID.
  POS_STRUCT_GP32XXX V3D_POS1;                  // Sprite V3D Position Type 1. 
  POS_STRUCT V3D_POS2;                          // Sprite V3D Position Type 2.  
} V3D_POS_STRUCT, *V3D_POS_STRUCT_PTR;

typedef struct {
	unsigned int cdm0;
	unsigned int cdm1;
	unsigned int cdm2;
	unsigned int cdm3;
} CDM_STRUCT, *CDM_STRUCT_PTR;

typedef struct {
	signed short	nCharNum;                           //  0.  Character Number Set
	signed short	nPosX;                              //  1.  Sprite 2D X Position or Sprite Virtual 3D X0 Position
	signed short	nPosY;                              //  2.  Sprite 2D Y Position or Sprite Virtual 3D Y0 Position
	signed short	nSizeX;                             //  3.  Width of Sprite Image
	signed short	nSizeY;                             //  4.  Heigh of Sprite Image
	unsigned short	uAttr0;                             //  5.  Sprite Attribute 0
	unsigned short	uAttr1;                             //  6.  Sprite Attribute 1
	signed short	nPosX1;                             //  7.  Sprite Virtual 3D X1 Position
	signed short	nPosX2;                             //  8.  Sprite Virtual 3D X1 Position
	signed short	nPosX3;                             //  9.  Sprite Virtual 3D X1 Position
	signed short	nPosY1;                             //  10. Sprite Virtual 3D Y1 Position
	signed short	nPosY2;                             //  11. Sprite Virtual 3D Y2 Position
	signed short	nPosY3;                             //  12. Sprite Virtual 3D Y3 Position
	const unsigned int	*SpCell;                    //  13. Sprite Attribute Start Pointer
} SPRITE, *PSPRITE;

typedef struct {
	unsigned short	nSP_CharNum;                        //  0.  Character Number of sprite image
	unsigned short	nSP_Hsize;                          //  1.  Character Number of sprite image
	unsigned short	nSP_Vsize;                          //  2.  Character Number of sprite image
	unsigned int	nSPNum_ptr;                         //  3.  sprite image pointer of sprite ram  		
	unsigned int	nEXSPNum_ptr;                       //  4.  sprite image pointer of sprite exram		
} SpN_ptr, *PSpN_ptr;

typedef enum
{
		Sprite_Coordinate_320X240=0,
		Sprite_Coordinate_640X480,
		Sprite_Coordinate_480X234,
		Sprite_Coordinate_480X272,
		Sprite_Coordinate_720X480,
	       Sprite_Coordinate_800X480,
		Sprite_Coordinate_800X600,
		Sprite_Coordinate_1024X768
} Sprite_Coordinate_Mode;

typedef enum
{
		PPU_hardware_coordinate=0,
		Center2LeftTop_coordinate,
		LeftTop2Center_coordinate
} Coordinate_Change_Mode;

typedef struct {
    unsigned short  nCharNumLo;                         //  0.  CharNum low half-WORD set
    unsigned short  nRotate;                            //  1.  Rotate set
    unsigned short  nPosX;                              //  2.  Position X set
    unsigned short  nZoom;                              //  3.  Zoom set
    unsigned short  nPosY;                              //  4.  Position Y set
    unsigned short  nPaletteBank;                       //  5.  Palette bank set
    unsigned short  nBlend;                             //  6.  Blend64 enable set
    unsigned short  nDepth;                             //  7.  Depth set
    unsigned short  nPalette;                           //  8.  Palette index set
    unsigned short  nVS;                                //  9.  Cell veritcal size set
    unsigned short  nHS;                                //  10. Cell horizontal size set
    unsigned short  nFlipV;                             //  11. Veritical flip set
    unsigned short  nFlipH;                             //  12. Horizontal flip set
    unsigned short  nColor;                             //  13. Color set
    unsigned short  nMosaic;                            //  14. Mosiaic set
    unsigned short  nBldLvl;                            //  15. Blend64 level set
    unsigned short  nCharNumHi;                         //  16. CharNum high half-WORD set
    unsigned short  nSPGroup;                           //  17. Sprite Group Function 
    unsigned short  nSPLargeSize;                       //  18. Sprite Large Size Set
    unsigned short  nSPInterpolation;                   //  19. Sprite bi-liner interpolation  
} SpN16_CellIdx, *PSpN16_CellIdx;

typedef struct {
	unsigned short	nTxAddr_mode;                            //  1.  Character width¡G8, 16, 32, 64, 128, 256
	unsigned short	nChar_width;                             //  2.  Character width¡G8, 16, 32, 64, 128, 256
	unsigned short	nChar_height;                            //  3.  Character height¡G8, 16, 32, 64, 128, 256
	unsigned short	ntext_width;                             //  4.  Width of Text size
	unsigned short	ntext_height;                            //  5.  Height of Text size
	unsigned short	nImage_width;                            //  6.  Image width  N¡GThe number of images
	unsigned short	nImage_height;                           //  7.  Image height N¡GThe number of images
	unsigned int  Num_arr_ptr;                             //  8.  Text Number Array pointer 
	unsigned int	index_ptr;                               //  9.  Image cell index table pointer
	unsigned int	data_ptr;                                //  10. Image cell data table pointer
} nTX_Num_Arr, *PnTX_Num_Arr;

typedef enum
{
		TEXT_DATA_C_CODE=0,
		TEXT_DATA_BIN
} TEXT_DATA_FORMAT;

typedef struct {
	unsigned short  image_number;                            //  1. Display of image number
	unsigned short  position_x;					             //  2. Display of TEXT X position
	unsigned short  position_y;					             //  3. Display of TEXT Y position
	unsigned short  position_x_offset;					     //  4. Display of TEXT X position
	unsigned short  position_y_offset;					     //  5. Display of TEXT Y position	
	unsigned short  pal_bank;                                //  6. Display of TEXT palette bank
	unsigned int  nNum_arr_ptr;                            //  7. Text Number Array pointer 
	unsigned int	ntext_data_ptr;                          //  8. Image data bin pointer or Image header data
	unsigned int	npal_ptr;                                //  9. Image palette bin pointer
	TEXT_DATA_FORMAT  image_type;                    //  10. image date type is the .bin or c code.
} nTX_image_info, *PnTX_image_info;

typedef struct {
	unsigned short	nTx_mode;                                //  0.  Text mode¡G0: Character mode; 1: Bitmap mode
	unsigned short	nAddr_mode;                              //  1.  Addressing mode¡G0: Relative; 1: Direct
	unsigned short	nColor_mode;                             //  2.  Color mode¡G0:4,1:16,2:64,3:256,4:32768,5:65536,6:RGBG0,7: RGBG1,8: RGBG2,9: RGBG3,10:YUYV0, 11:YUYV1, 12:YUYV2, 13:YUYV3.
	unsigned short	nP1024_mode;                             //  3.  P1024 share mode¡G0: disable; 1:enable
	unsigned short	nPal_type;                               //  4.  Palette type¡G0: 16-bit ; 1: 25-bit
	unsigned short	nBank_sel;                               //  5.  Bank selection¡G0 - 15
	unsigned short	nPal_sel;                                //  6.  Palette selection¡G0 - 3
	unsigned short	nDepth_num;                              //  7.  Depth number¡G0 - 3
	unsigned short	nChar_width;                             //  8.  Character width¡G8, 16, 32, 64, 128, 256
	unsigned short	nChar_height;                            //  9.  Character height¡G8, 16, 32, 64, 128, 256
	unsigned short	nFlip_mode;                              //  10. Flip mode¡G0:No Flip, 1:H Flip, 2:V Flip, 3:H/V Flip
	unsigned short	nBlend_level;                            //  11. Blend level¡G0 - 63
	unsigned short	nV3D_Mode;                               //  12. Virtual 3D Mode or not¡G0:No, 1:Yes 	
	unsigned short	nINIT_Mode;                              //  13. Interpolation Mode or not¡G0:No, 1:Yes	
	unsigned short	nTx_image;                               //  14. The number of images in the TEXT
	unsigned short	nImage_size_offset;                      //  15. Image size  offset
	unsigned short	index_offset;                            //  16. Image cell index table offset at____CellIndex.bin N¡GThe number of images
	unsigned short	data_offset;                             //  17. Image cell data table offset at ____CellData.bin  N¡GThe number of images
} Pic_Header, *PPic_Header;
 
typedef struct {
	unsigned short	nTx_mode;                                //  0.  Text mode¡G0: Character mode; 1: Bitmap mode
	unsigned short	nAddr_mode;                              //  1.  Addressing mode¡G0: Relative; 1: Direct
	unsigned short	nColor_mode;                             //  2.  Color mode¡G0:4,1:16,2:64,3:256,4:32768,5:65536,6:RGBG0,7: RGBG1,8: RGBG2,9: RGBG3,10:YUYV0, 11:YUYV1, 12:YUYV2, 13:YUYV3.
	unsigned short	nP1024_mode;                             //  3.  P1024 share mode¡G0: disable; 1:enable
	unsigned short	nPal_type;                               //  4.  Palette type¡G0: 16-bit ; 1: 25-bit
	unsigned short	nBank_sel;                               //  5.  Bank selection¡G0 - 15
	unsigned short	nPal_sel;                                //  6.  Palette selection¡G0 - 3
	unsigned short	nDepth_num;                              //  7.  Depth number¡G0 - 3
	unsigned short	nChar_width;                             //  8.  Character width¡G8, 16, 32, 64, 128, 256
	unsigned short	nChar_height;                            //  9.  Character height¡G8, 16, 32, 64, 128, 256
	unsigned short	nFlip_mode;                              //  10. Flip mode¡G0:No Flip, 1:H Flip, 2:V Flip, 3:H/V Flip
	unsigned short	nBlend_level;                            //  11. Blend level¡G0 - 63
	unsigned short	nV3D_Mode;                               //  12. Virtual 3D Mode or not¡G0:No, 1:Yes 	
	unsigned short	nINIT_Mode;                              //  13. Interpolation Mode or not¡G0:No, 1:Yes
	unsigned short	nTx_image;                               //  14. The number of images in the TEXT
	const unsigned int *nImage_size;                       //  15.  Image size  offset
	const unsigned int *index_data;                        //  16. Image cell index table offset at____CellIndex.bin N¡GThe number of images
	const unsigned int *cell_data;                         //  17. Image cell data table offset at ____CellData.bin  N¡GThe number of images
} Text_Header, *PText_Header;

typedef struct {
	unsigned int update_register_flag;			// This flag indicates which parts of the register sets should be updated
// Updated by DMA engine
#define C_UPDATE_REG_SET_PALETTE0			0x01000000
#define C_UPDATE_REG_SET_PALETTE1			0x02000000
#define C_UPDATE_REG_SET_PALETTE2			0x04000000
#define C_UPDATE_REG_SET_PALETTE3			0x08000000
#define C_UPDATE_REG_SET_HORIZONTAL_MOVE	0x10000000
#define C_UPDATE_REG_SET_TEXT1_HCOMPRESS	0x20000000
#define C_UPDATE_REG_SET_TEXT3_25D			0x40000000
#define C_UPDATE_REG_SET_SPRITE_ATTRIBUTE	0x80000000
#define C_UPDATE_REG_SET_SPRITE_EX_ATTRIBUTE 0x00100000
#define C_UPDATE_REG_SET_DMA_MASK			 0xFFF00000
// Updated by CPU
#define C_UPDATE_REG_SET_PPU				0x00000001
#define C_UPDATE_REG_SET_TEXT1				0x00000002
#define C_UPDATE_REG_SET_TEXT2				0x00000004
#define C_UPDATE_REG_SET_TEXT3				0x00000008
#define C_UPDATE_REG_SET_TEXT4				0x00000010
#define C_UPDATE_REG_SET_SPRITE				0x00000020
#define C_UPDATE_REG_SET_COLOR				0x00000040

	// Registers moved by PPU DMA engine
	unsigned int ppu_palette0_ptr;				// Updated when C_UPDATE_REG_SET_PALETTE0 is set in update_register_flag
	unsigned int ppu_palette1_ptr;				// Updated when C_UPDATE_REG_SET_PALETTE1 is set in update_register_flag
	unsigned int ppu_palette2_ptr;				// Updated when C_UPDATE_REG_SET_PALETTE2 is set in update_register_flag
	unsigned int ppu_palette3_ptr;				// Updated when C_UPDATE_REG_SET_PALETTE3 is set in update_register_flag
	unsigned int ppu_horizontal_move_ptr;			// Updated when C_UPDATE_REG_SET_HORIZONTAL_MOVE is set in update_register_flag
	unsigned int ppu_text1_hcompress_ptr;			// Updated when C_UPDATE_REG_SET_TEXT1_HCOMPRESS is set in update_register_flag
	unsigned int ppu_text3_25d_ptr;				// Updated when C_UPDATE_REG_SET_TEXT3_25D is set in update_register_flag
	unsigned int ppu_text3_25d_ptr_va;				// Updated when C_UPDATE_REG_SET_TEXT3_25D is set in update_register_flag
	unsigned int ppu_sprite_attribute_ptr;		// Updated when C_UPDATE_REG_SET_SPRITE_ATTRIBUTE is set in update_register_flag
       unsigned int ppu_sprite_ex_attribute_ptr;     // Updated when C_UPDATE_REG_SET_SPRITE_EX_ATTRIBUTE is set in update_register_flag     

	// PPU relative registers. Updated when C_UPDATE_REG_SET_PPU is set in update_register_flag
	unsigned short ppu_blending_level;				// R_PPU_BLENDING
	unsigned short ppu_fade_control;				// R_PPU_FADE_CTRL
	unsigned int ppu_palette_control;				// R_PPU_PALETTE_CTRL
	unsigned int ppu_rgb565_transparent_color;	// R_PPU_BLD_COLOR
	unsigned int ppu_window1_x;					// R_PPU_WINDOW0_X
	unsigned int ppu_window1_y;					// R_PPU_WINDOW0_Y
	unsigned int ppu_window2_x;					// R_PPU_WINDOW1_X
	unsigned int ppu_window2_y;					// R_PPU_WINDOW1_Y
	unsigned int ppu_window3_x;					// R_PPU_WINDOW2_X
	unsigned int ppu_window3_y;					// R_PPU_WINDOW2_Y
	unsigned int ppu_window4_x;					// R_PPU_WINDOW3_X
	unsigned int ppu_window4_y;					// R_PPU_WINDOW3_Y
	unsigned int ppu_enable;						// R_PPU_ENABLE
	unsigned int ppu_misc;                        // R_PPU_MISC                        
	unsigned int ppu_free_mode;                   // R_FREE_SIZE                       
	unsigned int ppu_frame_buffer_fifo;           // R_FBO_FIFO_SETUP                  
	unsigned int ppu_special_effect_rgb;			// R_PPU_RGB_OFFSET

	// TEXT relative registers
	struct ppu_text_register_sets {
		unsigned short position_x;					// R_PPU_TEXT1_X_OFFSET
		unsigned short position_y;					// R_PPU_TEXT1_Y_OFFSET
		unsigned short offset_x;					// R_PPU_TEXT1_X_OFFSET
		unsigned short offset_y;					// R_PPU_TEXT1_Y_OFFSET
		unsigned int attribute;					// R_PPU_TEXT1_ATTRIBUTE
		unsigned int control;						// R_PPU_TEXT1_CTRL
		unsigned int n_ptr;						// P_PPU_TEXT1_N_PTR
		unsigned int n_ptr_pa;						// P_PPU_TEXT1_N_PTR
		unsigned int a_ptr;						// P_PPU_TEXT1_A_PTR
		unsigned int a_ptr_pa;
		unsigned short sine;						// R_PPU_TEXT1_SINE
		unsigned short cosine;						// R_PPU_TEXT1_COSINE
		unsigned int segment;						// R_PPU_TEXT1_SEGMENT
	} text[4];
	unsigned short ppu_vcompress_value;				// R_PPU_VCOMP_VALUE
	unsigned short ppu_vcompress_offset;			// R_PPU_VCOMP_OFFSET
	unsigned short ppu_vcompress_step;				// R_PPU_VCOMP_STEP
	unsigned short text3_25d_y_compress;			// R_PPU_Y25D_COMPRESS

	// Sprite relative registers. Updated when C_UPDATE_REG_SET_SPRITE is set in update_register_flag
	unsigned int sprite_control;					// R_PPU_SPRITE_CTRL
	unsigned int sprite_segment;					// R_PPU_SPRITE_SEGMENT
	unsigned int extend_sprite_control;			// R_PPU_EXTENDSPRITE_CONTROL
	unsigned int extend_sprite_addr;				// R_PPU_EXTENDSPRITE_ADDR

	// Color mapping ram. Updated when C_UPDATE_REG_SET_COLOR is set in update_register_flag
	unsigned int color_mapping_ptr;
	
	//DISPLAY_NONE_PPU_STRUCT none_ppu_display;	
	// User input setting
	PPU_MOUDLE_STRUCT ppu_set;
	nTX_Num_Arr text_info;
} PPU_REGISTER_SETS, *PPU_REGISTER_SETS_PTR;

/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/******************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 *****************************************************************************/
 
//PPU API

/**
 * @brief 	PPU module isr function.
* @return 	isr number.
 */
extern signed int gp_ppu_module_isr(void);

/**
 * @brief 		PPU driver initial function.
* @param 	p_register_set [in]: PPU struct value initiation.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_init(PPU_REGISTER_SETS *p_register_set);

/**
 * @brief 		PPU module enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU text character0 transparent enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_char0_set_transparent(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU line or frame buffer calculate enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=from top to bottom 1=from bottom to top.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_bottom_up_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU tv display resolution set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=QVGA 1=VGA.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_vga_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU tv display interlace set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=interlace 1=non-interlace.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_non_interlace_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU line or frame buffer mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	enable [in]: enable:0=frame buffer mode disable 1= frame buffer mode enable.
* @param 	select [in]: select: enable:0(select:0=TV is line mode and TFT is frame mode) 1=both TV and TFT are frame buffer mode.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_frame_buffer_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int enable, unsigned int select);

/**
 * @brief 		PPU line or frame buffer color mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	format [in]: format:0=yuyv or rgbg mode disable 1= yuyv or rgbg mode enable.
* @param 	mono [in]: mono: format:0(mono:0=RGB565 1=Mono 2=4-color 3=16-color) 1(mono:0=RGBG 1=YUYV 2=RGBG 3=YUYV).
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_fb_format_set(PPU_REGISTER_SETS *p_register_set, unsigned int format, unsigned int mono);

/**
 * @brief 		PPU image date rom save mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_save_rom_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU TFT resolution set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:see the constants defined.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_resolution_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU test and sprite color type mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 0=BGRG/VYUY 1=GBGR/YVYU 2=RGBG/UYVY 3=GRGB/YUYV, value[2]:0=UV is unsigned(YCbCr) 1=UV is signed(YUV).
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_yuv_type_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU color mapping mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	enable [in]: enable:0=disable color mapping 1=enable color mapping, value: 32-bit address of color mapping table.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_color_mapping_set(PPU_REGISTER_SETS *p_register_set, unsigned int enable, unsigned int value);

/**
 * @brief 		PPU tft long burst mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable TFT long burst 1=enable TFT long burst.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_tft_long_burst_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU long burst mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable PPU long burst 1=enable PPU long burst.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_long_burst_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU blend4 mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0~3 level.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_blend4_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU rgb565 or yuyv and rgbg mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	enable [in]: enable:0=disable 1=enable.
* @param 	value [in]: value:0~0xFFFF.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_rgb565_transparent_color_set(PPU_REGISTER_SETS *p_register_set, unsigned int enable, unsigned int value);

/**
 * @brief 		PPU fade effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0~255 level.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_fade_effect_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU window mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	window_index [in]: windows_index:0(window 1)~3(window 4).
* @param 	window_x[in]: window_x:mask + start_x + end_x.
* @param 	window_x[in]: window_y:start_y + end_y.
* @return 	SUCCESS/ERROR_ID.
 */
extern signed int gp_ppu_window_set(PPU_REGISTER_SETS *p_register_set, unsigned int window_index, unsigned int window_x, unsigned int window_y);

/**
 * @brief 		PPU palette mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	p1024 [in]: p1024:0=disable 1=enable.
* @param 	type [in]: type: 0:text and sprite share for type 16,1:text and sprite palette independent for type 16 ,2: text and sprite share for type 25,3: text and sprite palette independent for type 25.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_palette_type_set(PPU_REGISTER_SETS *p_register_set, unsigned int p1024, unsigned int type);

/**
 * @brief 		PPU palette ram address set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	bank [in]: bank:0(palette0)~3(palette3).
* @param 	value [in]: value: 32-bit address of palette ram buffer.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_palette_ram_ptr_set(PPU_REGISTER_SETS *p_register_set, unsigned int bank, unsigned int value);

/**
 * @brief 		PPU frame buffer address set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	buffer [in]: buffer: 32-bit address of frame buffer.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_frame_buffer_add(PPU_REGISTER_SETS *p_register_set, unsigned int buffer);

/**
 * @brief 		PPU deflicker set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable deflicker function 1=enable deflicker function.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_deflicker_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);	

/**
 * @brief 		PPU sprite addrx2 mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sp_addrx2_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		This function returns when PPU registers are updated, it will not wait for PPU frame buffer output to complete.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_go_without_wait(PPU_REGISTER_SETS *p_register_set);

/**
 * @brief 		This function returns when PPU registers are updated, it will not wait for PPU frame buffer output to complete.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_go(PPU_REGISTER_SETS *p_register_set);

/**
 * @brief 		This function returns when PPU registers are updated and operation is done.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_go_and_wait_done(PPU_REGISTER_SETS *p_register_set);

/**
 * @brief 	PPU dual blending set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_dual_blend_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU free size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	INTL [in]: INTL:0=TFT Display 1=TV Display.
* @param 	H_size [in]: H_size:16~1920.
* @param 	V_size [in]: V_size:1~1024.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_free_size_set(PPU_REGISTER_SETS *p_register_set, unsigned short INTL, unsigned short H_size, unsigned short V_size);

/**
 * @brief 	PPU rgba color mode for text set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_rgba_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU alpha color mode for text set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_alpha_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU rgba color mode for sprite set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_rgba_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU new specialbmp mode for sprite set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_new_specialbmp_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU new compression mode for sprite set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_new_compression_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU delaygo for frame mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable, The PPU_GO will not be blocked by any condition. 1=enable, The PPU_GO will be delayed until both TVFBI_UPD and TFTFBI_UPD is 1.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_delgo_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU tftvtq mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0= TFT's frame buffer and display size is the same. 1= TFT's frame buffer must be set to 1(PPU_SIZE is 1) and the display size is QVGA.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_tftvtq_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU tv long burst set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_tv_long_burst_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 	PPU interpolation set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @param 	set_value [in]: value VGA_EN is 0:0= No interpolation function.1= Do QVGA to VGA up-scaling.2= Do QVGA to D1 up-scaling., VGA_EN is 1:0= No interpolation function.1= No interpolation function.2= Do QVGA to D1 up-scaling.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_interpolation_set(PPU_REGISTER_SETS *p_register_set, unsigned int value,unsigned int set_value);

/**
 * @brief 	PPU fifo size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	line_start [in]: line_start[9:0]: Line start value when restart PPU for multi-time PPU process.
* @param 	fifo_type [in]: fifo_type: 0 = No frame buffer output mode. 1 =8 lines FIFO mode. 2 =16 lines FIFO mode. 3 =32 lines FIFO mode.
* @param 	addr_offset [in]: addr_offset[13:0]: Address offset of each line, this register Is in byte unit.  This value will be add to the output.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_frame_buffer_output_fifo_set (PPU_REGISTER_SETS *p_register_set, unsigned short line_start, unsigned short fifo_type, unsigned short addr_offset);

/**
 * @brief 	PPU fifo go for frame mode function.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_fifo_go_and_wait_done(void);

/**
 * @brief 	PPU frame buffer get for display.
* @return 	SUCCESS/ERROR_ID/FRAME BUFFER VIRTUAL ADDRESS.
*/
extern signed int gp_ppu_get_frame_buffer(void);

/**
 * @brief 	PPU frame buffer release.
 * @param 	buffer [in]: release Target address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_release_frame_buffer(unsigned int buffer);

//TEXT API
/**
 * @brief 		PPU text initial function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_init(PPU_REGISTER_SETS *p_register_set, unsigned int text_index);

/**
 * @brief 		PPU text enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_enable_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text compress disable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=allow TEXT1 and TEXT2 to use horizontal/vertical compress 1=only allow 2D and rotate mode for TEXT1 and TEXT2.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_compress_disable_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU text mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0(2D) 1(HCMP/Rotate) 2(VCMP/2.5D) 3(HCMP+VCMP).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_mode_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text direct mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=relative mode 1=direct address mode.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_direct_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU text wallpaper mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_wallpaper_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text attribute select mode function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=get attribut from TxN_A_PTR 1=get attribut from register.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_attribute_source_select(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);


/**
 * @brief 		PPU text horizontal move enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_horizontal_move_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0(512x256) 1(512x512) ... 7(4096x4096).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_size_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text character size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	txn_hs [in]:large_text_en=0: txn_hs:0:8,1:16,2:32,3:64, 1:0:32,1:64,2:128,3:256.
* @param 	txn_vs [in]:large_text_en=0: txn_vs:0:8,1:16,2:32,3:64, 1:0:32,1:64,2:128,3:256.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_character_size_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int txn_hs, unsigned int txn_vs);

/**
 * @brief 		PPU text bitmap mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value [in]: value:0=character mode 1=bitmap mode.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_bitmap_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text color mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	rgb_mode:[in]: rgb_mode:0:palette mode,1:high color mode.
* @param 	color:[in]: color=0:1555,1:565,2:RGBG,3:YUYV.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_color_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int rgb_mode, unsigned int color);

/**
 * @brief 		PPU text palette mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	bank:[in]: bank:0~3.
* @param 	palette_idx:[in]: palette_idx:0~15.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_palette_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int bank, unsigned int palette_idx);

/**
 * @brief 		PPU text data segment set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_segment_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text attribute ram ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_attribute_array_set_ptr(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text number ram ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_number_array_set_ptr(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text number ram ptr update flag clear set function.
* @return		None.
*/
extern void gp_ppu_text_number_array_update_flag_clear(void);

/**
 * @brief 		PPU text calculate number ram set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	photo_width:[in]: photo width.
* @param 	photo_height:[in]: photo height.
* @param 	data_ptr:[in]: data_ptr: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_calculate_number_array(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int photo_width, unsigned int photo_height, unsigned int data_ptr);

/**
 * @brief 		PPU text position set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	pos_x:[in]: pos_x:0~0xFFF.
* @param 	pos_y:[in]: pos_y:0~0xFFF.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_position_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int pos_x, unsigned int pos_y);

/**
 * @brief 		PPU text position offset set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	offset_x:[in]: pos_x:0~0xFFF.
* @param 	offset_y:[in]: pos_y:0~0xFFF.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_offset_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int offset_x, unsigned int offset_y);

/**
 * @brief 		PPU text depth set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_depth_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text blend set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	enable:[in]: enable:0~1.
* @param 	mode:[in]: mode:0(4 level) 1(64 level).
* @param 	value:[in]: value:0~3 or 0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_blend_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int enable, unsigned int mode, unsigned int value);

/**
 * @brief 		PPU text flip set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0(no flip) 1(horizontal flip) 2(vertical flip) 3(h+v flip).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_flip_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text sin cosine set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	r_sine:[in]: r_sine:0~0x1FFF.
* @param 	r_cosine:[in]: r_cosine:0~0x1FFF.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_sine_cosine_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned short r_sine, unsigned short r_cosine);

/**
 * @brief 		PPU text window set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0(window1) 1(window2) 2(window3) 3(window4).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_window_select(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text special effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0(no effect) 1(negative color) 2(grayscale) 3(mono color).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_special_effect_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text vertical compress set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0~0x3FF.
* @param 	offset:[in]: offset:0~0x3FF.
* @param 	step:[in]: step:0~0x3FF.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_vertical_compress_set(PPU_REGISTER_SETS *p_register_set, unsigned int value, unsigned int offset, unsigned int step);

/**
 * @brief 		PPU text horizontal move ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to horizontal move control ram.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_horizontal_move_ptr_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU text1 horizontal compress ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to horizontal compress control ram.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text1_horizontal_compress_ptr_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU text rotate zoom set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	angle [in]: angle:0~359.
* @param 	factor_k [in]: factor_k: zoom factor.

* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_rotate_zoom_set(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, signed short angle, signed short factor_k);

/**
 * @brief 		PPU text3 25d set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	angle [in]: angle:0~359.
* @param 	factor_k [in]: factor_k: zoom factor of 240 line.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text3_25d_set(PPU_REGISTER_SETS *p_register_set, signed short angle, signed short *factor_ptr);

/**
 * @brief 		PPU text3 25d y compress set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 0~0x3F.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text3_25d_y_compress_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU text3 25d cossinebuf set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	CosSineBuf [in]: CosSineBuf: 32-bit buffer address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text25D_cossinebuf_set(PPU_REGISTER_SETS *p_register_set, unsigned int CosSineBuf);

/**
 * @brief 		PPU text special effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_interpolation_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text color mask set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	text_index [in]: text_index:0:TEXT0,1: TEXT1,2: TEXT2,3: TEXT3.
* @param 	value:[in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_text_color_mask_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int text_index, unsigned int value);

/**
 * @brief 		PPU text number array update function.
* @param 	text_info [in]: text information struct value set.
* @return 	NONE.
*/
extern signed int gp_ppu_text_number_array_update(PnTX_Num_Arr text_info);

//SPRITE API
/**
 * @brief 		PPU sprite initial set function.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_init(PPU_REGISTER_SETS *p_register_set);

/**
 * @brief 		PPU sprite function enable set.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite coordinate set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=center coordinate 1=top-left coordinate.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_set_coordinate(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite blend set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=4 level blending mode 1=16 or 64 level blending mode.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_blend_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite direct set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=relative address mode 1=direct address mode.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_direct_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite zoom set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_zoom_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite rotate set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_rotate_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite masic set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_mosaic_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite number set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0(1024 sprites) 1(4 sprites) 2(8 sprites) ... 255(1020 sprites).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_number_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite secial effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_special_effect_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite color dither set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_color_dither_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite 25d mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=sprite 2D mode 1= sprite 2.5D mode.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_25d_set_mode(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite window mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable sprite window function 1=enable sprite window function.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_window_enable_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite date segment set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit segment address.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_segment_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite attribute ram set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to sprite attribute ram.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_ram_set_ptr(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite sfr mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_sfr_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);


/**
 * @brief 		PPU large sprite mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_large_sprite_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite interpolation mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_interpolation_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite group mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_group_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite cdm attribute mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_cdm_attribute_set_enable(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite fraction mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_fraction_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU sprite extend attribute mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to sprite exterd attribute ram.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_extend_attribute_ram_set_ptr(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU exsprite enable set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU exsprite cdm set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_cdm_set_enable(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU exsprite interpolation set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_interpolation_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU large exsprite mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_large_size_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU large exsprite group set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_group_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU large exsprite fraction set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_fraction_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU large exsprite number set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_number_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU large exsprite ram ptr set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 32-bit pointer to sprite exterd attribute ram.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_start_address_set(PPU_REGISTER_SETS *p_register_set, unsigned int value);

/**
 * @brief 		PPU 2d sprite position set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	x0 [in]: x0: x0 represent sprite top/left or center position. Only 10-bits are valid.
* @param 	y0 [in]: y0: y0 represent sprite top/left or center position. Only 10-bits are valid.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_2d_position_set(SpN_RAM *sprite_attr, signed short x0, signed short y0);

/**
 * @brief 		PPU 25d sprite position set function.
* @param 	out [in]: sprite ram struct value set.
* @param 	in [in]: position: osition defines four coordinates of the sprite.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_25d_position_set(SpN_RAM *out, POS_STRUCT_PTR in);

/**
 * @brief 		PPU sprite attribute rotate set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_rotate_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute zoom set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_zoom_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute color set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0(2-bit) 1(4-bit) 2(6-bit) 3(8-bit/5-bit/16-bit/RGBG/YUYV/8+6 blending/RGBA888/ARGB444).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_color_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute flip set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0(No flip) 1(H-flip) 2(V-flip) 3(H+V-flip).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_flip_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute character size set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	hs [in]: hs :0(8) 1(16) 2(32) 3(64).
* @param 	vs [in]: vs:0(8) 1(16) 2(32) 3(64).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_character_size_set(SpN_RAM *sprite_attr, unsigned int hs, unsigned int vs);

/**
 * @brief 		PPU sprite attribute palette set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	bank [in]: bank:0~3.
* @param 	palette_idx [in]: palette_idx:0~15.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_palette_set(SpN_RAM *sprite_attr, unsigned int bank, unsigned int palette_idx);

/**
 * @brief 		PPU sprite attribute depth set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	bank [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_depth_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute blend64 set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	enable [in]: enable:0=disable 1=enable.
* @param 	value [in]: value:0~63 level.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_blend64_set(SpN_RAM *sprite_attr, unsigned int enable, unsigned int value);

/**
 * @brief 		PPU sprite attribute blend16 set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	enable [in]: enable:0=disable 1=enable.
* @param 	value [in]: value:0~15 level.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_blend16_set(SpN_RAM *sprite_attr, unsigned int enable, unsigned int value);

/**
 * @brief 		PPU sprite attribute window mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_window_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute mosaic mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: large_sprite_en=0:value:0(no effect) 1(2x2 pixels) 2(4x4 pixels) 3(8x8 pixels), 1:value:0(no effect) 1(8x8 pixels) 2(16x16 pixels) 3(32x32 pixels).
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_mosaic_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute charnum mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: charnum value.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_attribute_charnum_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU exsprite attribute group mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_group_attribute_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU exsprite attribute group mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_group_attribute_set(SpN_EX_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute large size mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_large_size_attribute_set(SpN_EX_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU exsprite attribute large size mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_large_size_attribute_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute interpolation mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_interpolation_attribute_set(SpN_EX_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU exsprite attribute interpolation mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	value [in]: value:0~1.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_exsprite_interpolation_attribute_set(SpN_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite attribute cdm mode set function.
* @param 	sprite_attr [in]: sprite ram struct value set.
* @param 	in [in]: in: Color RGB value defines four point of the CDM sprite.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_cdm_attribute_set(SpN_RAM *sprite_attr, unsigned int value ,CDM_STRUCT_PTR in);

/**
 * @brief 		PPU sprite color mask mode set function.
* @param 	sprite_attr [in]: sprite extend ram struct value set.
* @param 	value [in]: value:0~3.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_sprite_color_mask_attribute_set(SpN_EX_RAM *sprite_attr, unsigned int value);

/**
 * @brief 		PPU sprite G+ director data initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_ptr:[in]: 32 bit address.
* @return 	none.
*/
extern void gp_ppu_sprite_image_data_init(unsigned int sprite_number,unsigned int sprite_ptr);

/**
 * @brief 		PPU sprite G+ director data display initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_pos_x [in]: sprite x position on the screen.
* @param 	sprite_pos_y [in]: sprite y position on the screen.
* @param 	sprite_ptr:[in]: 32 bit address.
* @return 	none.
*/
extern void gp_ppu_sprite_display_set_init(unsigned int sprite_number,signed short sprite_pos_x,signed short sprite_pos_y,unsigned int sprite_ptr);

/**
 * @brief 		PPU sprite G+ director data display number of image initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_ptr:[in]: 32 bit address.
* @return 	none.
*/
extern void gp_ppu_sprite_image_number_set(unsigned int sprite_number,unsigned int sprite_ptr);

/**
 * @brief 		PPU sprite disable number of image initial function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @return 	none.
*/
extern void gp_ppu_sprite_disable_set(unsigned int sprite_number);

/**
 * @brief 		PPU sprite information of sprite ram function.
* @param 	sprite_number [in]: sprite number 0~1024.
* @param 	sprite_ptr:[in]: 32 bit address for SpN_ptr stuct.
* @return 	none.
*/
extern void Get_ppu_sprite_image_info(unsigned int sprite_number,SpN_ptr *sprite_ptr);

/**
 * @brief 		PPU sprite attribute coordination of sprite ram function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	display_mode:[in]: display resolution.
* @param 	coordinate_mode [in]: coordinate mode.
* @param 	sprite_number:[in]: sprite number 0~1024.
* @return 	none.
*/
extern void gp_ppu_paint_spriteram(PPU_REGISTER_SETS *p_register_set,Sprite_Coordinate_Mode display_mode, Coordinate_Change_Mode coordinate_mode, unsigned short sprite_number);

/**
 * @brief 		PPU memory copy.
* @param 	pDest [in]: Target address.
* @param 	pSrc:[in]: Source address.
* @param 	nBytes [in]: data length.
* @param 	mem_copy_8_16_32:[in]: move data length of once.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_cpu_cpy_mem(unsigned int *pDest, unsigned int *pSrc, unsigned int nBytes, unsigned short mem_copy_8_16_32);

/**
 * @brief 		PPU sprite ram initial set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	sprute_number [in]: display sprite number.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_spriteram_init(PPU_REGISTER_SETS *p_register_set,unsigned int sprute_number);

/**
 * @brief 		PPU memory set.
* @param 	pDest [in]: target address.
* @param 	uValue:[in]: memory set value.
* @param 	nBytes [in]: data length.
* @param 	mem_copy_8_16_32:[in]: move data length of once.
* @return 	SUCCESS/ERROR_ID.
*/
extern signed int gp_ppu_set_mem(unsigned int *pDest, unsigned int uValue, signed int nBytes,unsigned short  mem_copy_8_16_32);

#endif /* _GP_PPU_H_ */
