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
 * @file    gp_gpio.h
 * @brief   Declaration of GPIO driver.
 * @author  clhuang
 * @since   2010-10-13
 * @date    2010-10-13
 */
#ifndef _G2D_H_
#define _G2D_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define NONE_TRASPARENT 0XFF000000

/* Function trigger. */
#define G2D_FUNC_BLEND      (1<<1)
#define G2D_FUNC_SCALE      (1<<3)
#define G2D_FUNC_ROTATE     (1<<5)
    
/* Color mode trigger. */
#define G2D_ARGB1555		0
#define G2D_RGB565			(1<<0)
#define G2D_RGBA8888		(1<<1)
#define G2D_YUV422          3

#define G2D_FRAME_NUM		3
#define PPU_TV_OUTPUT       0

enum {
	FULL_SCREEN_MODE = 0,
	RATIO_MODE,
	RATIO_EQU1_MODE,
	FULL_SCREEN_BY_RATIO_MODE,
    FULL_SCREEN_BY_DIGI_ZOOM
};
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct g2d_selrect_s {
	unsigned int x;
	unsigned int y;
	unsigned int x1;
	unsigned int y1;
}g2d_selrect_t;

typedef struct g2d_rect_s {
	unsigned short x;
	unsigned short y;
	unsigned short width;
	unsigned short height;
	unsigned short rotate_x;
	unsigned short rotate_y;
}g2d_rect_t;

 /*argument for 2D engine*/
typedef struct g2d_draw_s {
    unsigned int func_flag;
	unsigned int *sourcedata;
	g2d_rect_t rect;
	unsigned char color_mode;
	unsigned short colorkey;
	unsigned char blend_level;
	unsigned char scale_level;
	unsigned int angle;
} g2d_draw_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern int text_num;
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   g2d draw bitmap.
 * @param   nOutFormat[in]: 1 RGBA8888, 0 RGB565 
 * @param   nWidth[in]: 
 * @param   nHeight[in]: 
 * @param   *selrect[in]: rectangle window set (for partial copy) 
 * @param   *draw_ctx[in]: bitmaps structure) 
 * @return  SP_OK/ERROR_ID
 * @see
 */
int g2d_draw_bitmap(unsigned char nOutFormat, unsigned int  nWidth, unsigned int  nHeight, g2d_selrect_t *selrect, unsigned char num, g2d_draw_t *draw_ctx);

/**
 * @brief   g2d function initial.
 * @param   none
 * @return  SP_OK/ERROR_ID
 * @see
 */
int g2d_init( void );


/**
* @brief	add destination buffer 
* @param	draw_texture - all texture information
* @return	success or failure
*/
int g2d_dst_buffer_add(unsigned int buf);

/**
* @brief	remove destination buffer 
* @param	draw_texture - all texture information
* @return	success or failure
*/
int g2d_dst_buffer_remove(unsigned int buf);

/**
* @brief	get destination buffer point
* @param	none
* @return	buffer point
*/
char* g2d_dst_buffer_get (void);


/**
* @brief	bitmap copy (Support RGB565 and RGBA) 
* @param	source_num[in]: source bitmap number  
* @param	*pdstBitmap[in]: desination bitmap 
* @param	dstRect[in]: desination window set 
* @param	*psrcBitmap[in]: source bitmap 
* @param	srcRect[in]: source window set  
* @param	keyColor[in]: transparent color set 
* @param	*rotate[in]: rotate level set 
* @param	*blend[in]: rotate level set 
* @param	flip[in]: flip set 
* @return	SUCCESS or FAIL
*/
int g2dPpuBitBlt (
    UINT8 source_num,
	gp_bitmap_t *pdstBitmap, 
    gp_rect_t dstRect, 
    gp_bitmap_t *psrcBitmap, 
    gp_rect_t srcRect,
    SINT32 keyColor,
    UINT32 *rotate,
    UINT32 *blendLevel,
    SINT32 flip);

/**
* @brief	2d graphic scale init
* @param	void
* @return	success or failure
*/
int
g2dScaleInit(
	void
);

/**
* @brief	2d graphic scale init
* @param	void
* @return	success or failure
*/
int
g2dScaleDeInit(
	void
);

#if 0
/**
* @brief	bitmap scale (Support ARGB) 
* @param	*pdstBitmap[in]: desination bitmap 
* @param	*psrcBitmap[in]: source bitmap 
* @return	SUCCESS or FAIL
*/
int g2dScale(
	gp_bitmap_t *pdstBitmap, 
    gp_rect_t *pdstRect,
	gp_bitmap_t *psrcBitmap,
    gp_rect_t *psrcRect
);
#else
/**
* @brief	bitmap scale (Support ARGB) 
* @param	*pdstBitmap[in]: desination bitmap 
* @param	*psrcBitmap[in]: source bitmap 
* @return	SUCCESS or FAIL
*/
int g2dScale(
	gp_bitmap_t *pdstBitmap, 
	gp_bitmap_t *psrcBitmap
);
#endif

int 
g2dScaleByRatio(
	unsigned int scaleMode,
	gp_bitmap_t *pdstBitmap,
	gp_rect_t *pdstRect, 
	gp_bitmap_t *psrcBitmap,
	gp_rect_t *psrcRect 
);

#endif /* _GP_GRAPHIC_H_ */
