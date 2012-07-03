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
#ifndef _GP_GRAPHIC_H_
#define _GP_GRAPHIC_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* Ioctl for device node definition */
#define G2D_IOCTL_ID           'g'
#define G2D_IOCTL_DRAW_BITMAP _IO(G2D_IOCTL_ID, 0)
    
#define G2D_FUNC_ROP        (1<<0)
#define G2D_FUNC_BLEND      (1<<1)
#define G2D_FUNC_TROP       (1<<2)
#define G2D_FUNC_SCALE      (1<<3)
#define G2D_FUNC_GRDT_FILL  (1<<4)
#define G2D_FUNC_ROTATE     (1<<5)
#define G2D_FUNC_CLIP     (1<<6)
#define G2D_FUNC_MASK       (1<<7)
    
/* Alpha format and blend operation. */
#define G2D_ALPHA_FMT_PPA				0x0
#define G2D_ALPHA_FMT_IDX				0x1
#define G2D_ALPHA_FMT_SRC_CONSTA         0x2
#define G2D_ALPHA_FMT_SRCDST_CONSTA	    0x3
    
#define G2D_BLEND_OP_OVER			0x0
#define G2D_BLEND_OP_IN				0x1
#define G2D_BLEND_OP_OUT            0x2
#define G2D_BLEND_OP_ATOP			0x3
#define G2D_BLEND_OP_XOR            0x4

#define G2D_ROTATE_0     0x00
#define G2D_ROTATE_90    0x01
#define G2D_ROTATE_180   0x02
#define G2D_ROTATE_270   0x03

#define G2D_CLIP_TYPE_SCREEN 0x01
#define G2D_CLIP_TYPE_RECT	0x02

/* General ROP code. The naming is the same with rop of WIN GDI. */
#define G2D_ROP_BLACKNESS				0x00
#define G2D_ROP_DSTINVERT				0x55
#define G2D_ROP_MERGECOPY				0xC0
#define G2D_ROP_MERGEPAINT			0xBB
#define G2D_ROP_NOTSRCCOPY			0x33
#define G2D_ROP_NOTSRCERASE			0x11
#define G2D_ROP_PATCOPY				0xF0
#define G2D_ROP_PATINVERT				0x5A
#define G2D_ROP_PATPAINT				0xFB
#define G2D_ROP_SRCAND					0x88
#define G2D_ROP_SRCCOPY				0xCC
#define G2D_ROP_SRCERASE				0x44
#define G2D_ROP_SRCINVERT				0x66
#define G2D_ROP_SRCPAINT				0xEE
#define G2D_ROP_WHITENESS				0xFF

/* TROP code. Only source pixels could be transparent. */
#define G2D_TROP_ALL_TRANS			0x0
#define G2D_TROP_ONEFAIL_TRANS			0x1
#define G2D_TROP_DSTFAIL_TRANS			0x3
#define G2D_TROP_SRCFAIL_TRANS			0x5
#define G2D_TROP_BOTHFAIL_TRANS			0x7
#define G2D_TROP_ONEPASS_TRANS			0x8
#define G2D_TROP_SRCPASS_TRANS			0xA
#define G2D_TROP_ALL_OPAQUE			0xF

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
/*you can use the macro in RGB888 directly,but you should notice that in RGB565,the value is using thevalue
   which in RGB565 to mutiply 8,for example:blue red:001F is match to the color MAKE_RGB(0x00,0x00,0xF8)*/
#define MAKE_RGB(red, green, blue) ((red << 16) | (green << 8) | (blue))

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
 /*argument for ROP*/
typedef struct g2d_rop_s {
    unsigned char fg_rop;
    unsigned int fg_pattern;
}g2d_rop_t;

/*argument for alphablend*/
typedef struct g2d_blend_s {
    unsigned char alpha_fmt;
    unsigned char blend_op;
    unsigned int src_alpha;
    unsigned int dst_alpha;
}g2d_blend_t;

/*argument for TROP*/
typedef struct g2d_trop_s {
    unsigned int src_hi_color_key;
    unsigned int src_lo_color_key;
    unsigned int dst_hi_color_key;
    unsigned int dst_lo_color_key;
    unsigned char trop;
}g2d_trop_t;
  
/*argument for gradient fill*/
typedef struct g2d_grdt_s
{
    short hor_delta_r;
    short hor_delta_g;
    short hor_delta_b;
    short ver_delta_r;
    short ver_delta_g;
    short ver_delta_b;
    unsigned char hor_enable;
    unsigned char hor_left_right;
    unsigned char ver_enable;
    unsigned char ver_top_bottom;
} g2d_grdt_t;

/*argument for roate and mirror*/
typedef struct g2d_rotate_s
{
    unsigned char rotation;
    unsigned char flip;
}g2d_rotate_t;

/*argument for clip*/
typedef struct g2d_clip_s
{
    short clip_type;
    gp_rect_t clip_rect;
}g2d_clip_t;

/*argument for MASK*/
typedef struct g2d_mskblt_s {
    unsigned char msk_type;
    unsigned char fifo_mode;
    unsigned char load_msk;
    unsigned char bg_rop;
    unsigned int bg_pattern;
    unsigned int msk_addr;
    unsigned int msk_width;
    unsigned int msk_height;
}g2d_mskblt_t;

/*argument for 2D engine*/
typedef struct g2d_draw_ctx_s {
    unsigned int func_flag;
    gp_bitmap_t dst;
    gp_bitmap_t src;
    gp_rect_t dst_rect;
    gp_rect_t src_rect;
    g2d_rop_t rop;
    g2d_blend_t blend;
    g2d_trop_t trop;
    g2d_grdt_t grdt_fill;
    g2d_rotate_t rotate;
    g2d_clip_t clip; 
    g2d_mskblt_t mask;
} g2d_draw_ctx_t;
    

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   2d draw bitmap.
 * @param   draw_ctx[in]: argument for 2d graphic engine
 * @return  SP_OK/ERROR_ID
 * @see
 */
int gp_2d_draw_bitmap(g2d_draw_ctx_t* draw_ctx);


#endif /* _GP_GRAPHIC_H_ */
