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
 * @file    hal_ppu.h
 * @brief   Implement of PPU HAL API header file.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */

#ifndef HAL_PPU_H
#define HAL_PPU_H

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>
 #include <mach/gp_ppu.h>
 
/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/
#define   C_PPU_TEXT3_V25D_EN          0x40
#define PPU_FRAME_REGISTER_WAIT             0

// Interrupt control register
#define C_PPU_INT_EN_PPU_VBLANK				0x00000001
#define C_PPU_INT_EN_VIDEO_POSITION			0x00000002
#define C_PPU_INT_EN_DMA_COMPLETE			0x00000004
#define C_PPU_INT_EN_PALETTE_ERROR			0x00000008
#define C_PPU_INT_EN_TEXT_UNDERRUN			0x00000010
#define C_PPU_INT_EN_SPRITE_UNDERRUN		0x00000020
#define C_PPU_INT_EN_SENSOR_FRAME_END		0x00000040
#define C_PPU_INT_EN_MOTION_DETECT			0x00000080
#define C_PPU_INT_EN_SENSOR_POSITION_HIT	0x00000100
#define C_PPU_INT_EN_MOTION_UNDERRUN		0x00000200
#define C_PPU_INT_EN_TV_UNDERRUN			0x00000400
#define C_PPU_INT_EN_TV_VBLANK				0x00000800
#define C_PPU_INT_EN_TFT_UNDERRUN			0x00001000
#define C_PPU_INT_EN_TFT_VBLANK				0x00002000
#define C_PPU_INT_EN_PPU_HBLANK				0x00004000
#define C_PPU_INT_EN_SENSOR_UNDERRUN				0x00008000
#define C_PPU_INT_EN_IIIEGAL_WRITE				0x00020000

#define C_PPU_INT_EN_PPU_MASK				0x00007C3D

// Interrupt pending register
#define C_PPU_INT_PEND_PPU_VBLANK			0x00000001
#define C_PPU_INT_PEND_VIDEO_POSITION		0x00000002
#define C_PPU_INT_PEND_DMA_COMPLETE			0x00000004
#define C_PPU_INT_PEND_PALETTE_ERROR		0x00000008
#define C_PPU_INT_PEND_TEXT_UNDERRUN		0x00000010
#define C_PPU_INT_PEND_SPRITE_UNDERRUN		0x00000020
#define C_PPU_INT_PEND_SENSOR_FRAME_END		0x00000040
#define C_PPU_INT_PEND_MOTION_DETECT		0x00000080
#define C_PPU_INT_PEND_SENSOR_POSITION_HIT	0x00000100
#define C_PPU_INT_PEND_MOTION_UNDERRUN		0x00000200
#define C_PPU_INT_PEND_TV_UNDERRUN			0x00000400
#define C_PPU_INT_PEND_TV_VBLANK			0x00000800
#define C_PPU_INT_PEND_TFT_UNDERRUN			0x00001000
#define C_PPU_INT_PEND_TFT_VBLANK			0x00002000
#define C_PPU_INT_PEND_PPU_HBLANK			0x00004000
#define C_PPU_INT_PEND_SENSOR_UNDERRUN			0x00008000
#define C_PPU_INT_PEND_IIIEGAL_WRITE			0x00020000

#define C_PPU_INT_PEND_PPU_MASK				0x00007C3D

/**************************************************************************
*                          D A T A    T Y P E S
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
 
/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/******************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 *****************************************************************************/
/**
* @brief	       ppu initial
* @param 	none
* @return 	none
*/
extern void gpHalPPUEn( int en );

/**
* @brief	       ppu irq mode
* @param 	enable[in]:0=disable,1=enable.
* @return 	none
*/
extern void gpHalPPUIrqCtrl(UINT32 enable);

/**
* @brief	       added ppu frame buffer
* @param 	frame_buf: 32 bit sdram address of 32 or 64 aligns.
* @return 	success=0,fail=-1.
*/

extern SINT32 gpHalPPUFramebufferAdd(UINT32 *frame_buf,UINT32 *frame_buf_va);

/**
* @brief	  ppu frame buffer release
* @param 	  frame_buf[in]:frame buffer for release
* @return 	ppu release buffer number.
*/
extern SINT32 gpHalPPUframebufferRelease(UINT32 frame_buf);

/**
* @brief	  get ppu frame buffer
* @return 	buffer of ppu frame end.
*/
extern SINT32 gpHalPPUframebufferGet(void);

/**
* @brief	       ppu module register set
* @param 	wait_available[in]:0:without check ppu done. 1:check ppu done
* @param 	wait_done[in]:0:without wait ppu frame end.1:wait ppu frame end.
* @return 	success=0,fail=-1.
*/
extern SINT32 gpHalPPUGo(PPU_REGISTER_SETS *p_register_set, UINT32 wait_available, UINT32 wait_done);

/**
* @brief	       ppu fb mode
* @param 	enable[in]:0=disable,1=enable.
* @return 	none
*/
extern void gpHalPPUFbCtrl(UINT32 enable);

/**
* @brief	       ppu fifo go mode
* @param 	none
* @return 	success=0,fail=-1.
*/
extern SINT32 gpHalPPUFifogowithdone(void);

/**
* @brief	ppu irq isr
* @param 	none
* @return 	none
*/
extern SINT32 gpHalPPUIsr(void);

/**
* @brief	none ppu module register set
* @param 	TV_TFT[in]:display device.
* @param 	display_buffer[in]:display buffer.
* @param 	DISPLAY_MODE[in]:display resolution.
* @param 	SHOW_TYPE[in]:0:display color type.
* @return 	none
*/
extern void ppu_reg_set(UINT8 TV_TFT,UINT32 display_buffer,UINT32 DISPLAY_MODE ,UINT32 SHOW_TYPE);

/**
* @brief	sprite 25d position convert
* @param 	pos_in[in]:sprite position.
* @param 	sp_out[in]:sprite address of sprite ram.
* @return 	none
*/
extern void gpHalPPUsprite25dPosconvert(SpN_RAM *sp_out, POS_STRUCT_PTR pos_in);

extern void gpHalPPUSetTftBurst(UINT32 mode);
extern void gpHalPPUSetYuvType(UINT32 type);
extern void gpHalPPUSetRes(UINT32 mode);
extern void gpHalPPUSetFbMono(UINT32 mode);
extern void gpHalPPUSetFbFormat(UINT32 mode);
extern void gpHalPPUSetFbEnable(UINT32 enable);
extern void gpHalPPUSetVgaEnable(UINT32 enable);
extern void gpHalPPUSetIrqEnable(UINT32 field);
extern void gpHalPPUSetIrqDisable(UINT32 field);
extern void gpHalPPUClearIrqFlag(UINT32 mask);
extern UINT32 gpHalPPUGetIrqStatus(void);
extern void gpHalPPUSetTftBufferAddr(UINT32 addr);
extern void gpHalPPUSetTvBufferAddr(UINT32 addr);
extern void gpHalPPUSetFlip(UINT32 enable);

#endif
