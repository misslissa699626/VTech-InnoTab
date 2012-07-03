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
/**
 * @file    hal_2d.h
 * @brief   Declaration of 2D HAL API.
 * @author  clhuang
 * @since   2010-10-07
 * @date    2010-10-07
 */
#ifndef _HAL_2D_H_
#define _HAL_2D_H_

//#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/    
#define G2DENG_MSK_DISABLE		0x00
#define G2DENG_MSK_ENABLE_STIPPLE	0x04
#define G2DENG_MSK_ENABLE_MSKBLT	0x06

#define G2DENG_GRDT_HORIZONTAL  0x00
#define G2DENG_GRDT_VERTICAL    0x01
#define G2DENG_GRDT_LEFT_TO_RIGHT   0x00
#define G2DENG_GRDT_RIGHT_TO_LEFT   0x01
#define G2DENG_GRDT_TOP_TO_BOTTOM   0x00
#define G2DENG_GRDT_BOTTOM_TO_TOP   0x01

#define G2DENG_ROTATE_0     0x00
#define G2DENG_ROTATE_90    0x01
#define G2DENG_ROTATE_180   0x02
#define G2DENG_ROTATE_270   0x03

#define G2DENG_CLIP_RECTANGLE   0x00
#define G2DENG_CLIP_SCREEN      0x01 

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/    
typedef struct gp2dGrdtCtx_s
{
    UINT16 deltaR;
    UINT16 deltaG;
    UINT16 deltaB;
    UINT8 flipDir;
} gp2dGrdtCtx_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
* @brief initial 2D graphic engine.
* @param resetEngine [in] : 1, reset 2D engine; 0, not reset 2D engine
* @return : none
*/
void gpHal2dInit (UINT8 resetEngine); 

/**
* @brief start 2D graphic engine.
* @return : none
*/
void gpHal2dExec (void); 

/**
* @brief enable 2D engine interrupt.
* @param enable [in] : 1, enable interrupt; 0, disable interrupt
* @return : none
*/
void gpHal2dEnableInterrupt (UINT8 enable); 

/**
* @brief clear 2D engine interrupt.
* @return : none
*/
void gpHal2dClearInterrupt (void);

/**
* @brief Set 2D source bitmap, operation rectangle may equal to destination, if not, scale enable.
* @param imgAddr [in] : Pointer of source bitmap data
* @param imgSize [in] : Size of source bitmap
* @param colorFmt [in] : Image color format of source bitmap
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dSetSrcBitmap (UINT32 imgAddr, spRectSize_t imgSize, UINT8 colorFmt, spRect_t opRect);

/**
* @brief Set 2D source bitmap palette when color format is 16-color or 256-color.
* @param palAddr [in] : Address of palette data
* @param palOffset [in] : Offset by bytes of palette data
* @param colorFmt [in] : Palette data format
* @param loadEnable [in] : Enable load palette into 2D engine
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dSetPalette (UINT32 palAddr, UINT32 palOffset, UINT8 colorFmt, UINT8 loadEnable);

/**
* @brief Set 2D destination bitmap.
* @param imgAddr [in] : Pointer of destination bitmap data
* @param imgSize [in] : Size of destination bitmap
* @param colorFmt [in] : Image color format of destination bitmap
* @param opRect [in] : operation rectangle of destination bitmap
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dSetDstBitmap (UINT32 imgAddr, spRectSize_t *pimgSize, UINT8 colorFmt, spRect_t *popRect);

/**
* @brief set 2D internal 8x8 stipple mask register
* @param stippleMskLo [in] : Low part of stipple register
* @param stippleMskHi [in] : High part of stipple register
*  @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dSetStippleMsk (UINT32 stippleMskLo, UINT32 stippleMskHi);

/**
* @brief enable stipple mask or mask blt function, if fifo mode disable and size 8*8, set internal mask register
* @param enableType [in] : enable mask type 
*                        (G2D_DISABLE_MSK/ G2D_ENABLE_STIPPLE_MSK/ G2D_ENABLE_MSKBLT)
* @param fifoMode [in] : 1, fifo mode, use external mask memory data; 0, user internal mask register
* @param bgOp [in] : Background raster operator
* @param bgPattern [in] : Background color pattern
* @param imgAddr [in] : Address of mask bitmap data
* @param imgSize [in] : Size of mask bitmap
*  @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableMsk (UINT8 enableType, UINT8 fifoMode, 
        UINT8 bgRop, UINT32 bgPattern, UINT32 imgAddr, spRectSize_t *pimgSize);

/**
* @brief enable 2D ROP function. 
* @param fgOp [in] : Foreground raster operator
* @param fgPattern [in] : Foreground color pattern
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableRop(UINT8 fgRop, UINT32 fgPattern);

/**
* @brief enable bit plane. 
* @param bpRdEnable [in] : Enable read bit plane
* @param bpRdMsk [in] : Read bit plane mask
* @param bpWrEnable [in] : Enable write bit plane
* @param bpWrMsk [in] : Write bit plane mask
* @return : SP_OK(0)/SP_FAIL
*/
UINT32
gpHal2dEnableBitPlane(
    UINT8 bpRdEnable,
    UINT32 bpRdMsk,
    UINT8 bpWrEnable,
    UINT32 bpWrMsk
);

/**
* @brief enable 2D alpha blend function. 
* @param alphaFmt [in] : Alpha format, per-pixel-alpha/srcconstalpha/dstconstalpha
* @param blendOp [in] : Blend operation
* @param srcAlpha [in] : Srcconstalpha
* @param dstAlpha [in] : Dstconstalpha
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableBlend(UINT8 alphaFmt, UINT8 blendOp, UINT16 srcAlpha, UINT16 dstAlpha);

/**
* @brief enable 2D Trop function. 
* @param srcHiColorKey [in] : Source color key range high
* @param srcLoColorKey [in] : Source color key range low
* @param dstHiColorKey [in] : Destination color key range high
* @param dstLoColorKey [in] : Destination color key range low
* @param trop [in] : Trop operation code
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableTrop(UINT32 srcHiColorKey, UINT32 srcLoColorKey, UINT32 dstHiColorKey, UINT32 dstLoColorKey, UINT8 trop);

/**
* @brief Set scale parameter and enable scale, the function will compute horizontal/vertical scale factor and 
*       correspond horizontal/vertical scale enable by the src/dst width/height and set relative register. 
* @param srcWidth [in] : Source scale width
* @param srcHeight [in] : Source scale height
* @param dstWidth [in] : Destination width
* @param dstHeight [in] : Destination height
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableScale(UINT32 srcWidth, UINT32 srcHeight, UINT32 dstWidth, UINT32 dstHeight);


/**
* @brief enable 2D gradient fill function. 
* @param grdtDir [in] : horizontal or vertical gradient direction
* @param grdtCtx [in] : gradient context include delta color and flip direction
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableGradientFill(UINT8 grdtDir, gp2dGrdtCtx_t *pgrdtCtx);


/**
* @brief enable 2D rotate and mirror function. 
* @param rotateType [in] : rotate type
* @param mirrorEnable [in] : enable mirror function or not
* @param srcRefPos [in] : Source reference point
* @param dstRefPos [in] : Destination reference point
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableRotate(UINT8 rotateType, UINT8 mirrorEnable, 
                        spPoint_t *psrcRefPos, spPoint_t *pdstRefPos, spRect_t *popRect);

/**
* @brief enable 2D rotate and mirror function. 
* @param enableType [in] : enable screen or rectangle bound clipping
* @param clipRect [in] : clipping rectangle
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableClip(UINT8 enableType, spRect_t *pclipRect);


/**
* @brief get 2D engine status. 
* @return : status reg value
*/
UINT32 gpHal2dGetStatus(void);

/**
 * @brief   2D clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHal2dClkEnable(UINT32 enable);

/**
* @brief dump 2D graphic engine register value
* @return : none
*/
void gpHal2dDump (void);

/**
* @brief Enable/disable dither
* @param enable [in] : Zero for disable, other enable
* @return : SP_OK(0)/SP_FAIL
*/
UINT32 gpHal2dEnableDither(UINT32 enable);

#endif /* _HAL_2D_H_ */
