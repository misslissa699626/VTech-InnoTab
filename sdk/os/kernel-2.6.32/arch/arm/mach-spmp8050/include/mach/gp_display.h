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
 * @file display.h
 * @brief Display interface header file
 * @author Anson Chuang
 */

#ifndef _GP_DISPLAY_DEVICE_H_
#define _GP_DISPLAY_DEVICE_H_


#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define DISPIO_TYPE 'd'

/* Common */
#define DISPIO_SET_INITIAL	_IO(DISPIO_TYPE, 0x01)	/* Initial display device */
#define DISPIO_SET_UPDATE	_IO(DISPIO_TYPE, 0x02)
#define DISPIO_GET_PANEL_RESOLUTION	_IOR(DISPIO_TYPE, 0x03, gp_disp_res_t *)
#define DISPIO_GET_PANEL_SIZE		_IOR(DISPIO_TYPE, 0x04, gp_size_t *)
#define DISPIO_SET_OUTPUT	_IOW(DISPIO_TYPE, 0x05, uint32_t)
#define DISPIO_GET_OUTPUT	_IOR(DISPIO_TYPE, 0x05, uint32_t *)
#define DISPIO_ENUM_OUTPUT	_IOR(DISPIO_TYPE, 0x06, gp_disp_output_t *)
#define DISPIO_WAIT_FRAME_END	_IO(DISPIO_TYPE, 0x07)
#define DISPIO_SET_SUSPEND	_IO(DISPIO_TYPE, 0x08)
#define DISPIO_SET_RESUME	_IO(DISPIO_TYPE, 0x09)
#define DISPIO_SET_BACKLIGHT	_IOW(DISPIO_TYPE, 0x0A, uint32_t)
#define DISPIO_DUMP_REGISTER	_IO(DISPIO_TYPE, 0x0B)
#define DISPIO_SET_TV_MODE	_IOW(DISPIO_TYPE, 0x0C, uint32_t)
#define DISPIO_GET_TV_MODE	_IOR(DISPIO_TYPE, 0x0C, uint32_t *)

/* Primary layer */
#define DISPIO_SET_PRI_ENABLE	_IOW(DISPIO_TYPE, 0x10, uint32_t)
#define DISPIO_GET_PRI_ENABLE	_IOR(DISPIO_TYPE, 0x10, uint32_t)
#define DISPIO_SET_PRI_BITMAP	_IOW(DISPIO_TYPE, 0x11, gp_bitmap_t)
#define DISPIO_GET_PRI_BITMAP	_IOR(DISPIO_TYPE, 0x11, gp_bitmap_t *)
#define DISPIO_SET_PRI_SCALEINFO	_IOW(DISPIO_TYPE, 0x12, gp_disp_scale_t)
#define DISPIO_GET_PRI_SCALEINFO	_IOR(DISPIO_TYPE, 0x12, gp_disp_scale_t *)


/* Dithering */
#define DISPIO_SET_DITHER_ENABLE	_IOW(DISPIO_TYPE, 0x20, uint32_t)
#define DISPIO_GET_DITHER_ENABLE	_IOR(DISPIO_TYPE, 0x20, uint32_t)
#define DISPIO_SET_DITHER_TYPE		_IOW(DISPIO_TYPE, 0x21, uint32_t)
#define DISPIO_GET_DITHER_TYPE		_IOR(DISPIO_TYPE, 0x21, uint32_t *)
#define DISPIO_SET_DITHER_PARAM	_IOW(DISPIO_TYPE, 0x22, gp_disp_ditherparam_t)
#define DISPIO_GET_DITHER_PARAM	_IOR(DISPIO_TYPE, 0x22, gp_disp_ditherparam_t *)

/* Color matrix */
#define DISPIO_SET_CMATRIX_PARAM	_IOW(DISPIO_TYPE, 0x30, gp_disp_colormatrix_t)
#define DISPIO_GET_CMATRIX_PARAM	_IOR(DISPIO_TYPE, 0x30, gp_disp_colormatrix_t *)


/* Gamma table */
#define DISPIO_SET_GAMMA_ENABLE	_IOW(DISPIO_TYPE, 0x40, uint32_t)
#define DISPIO_GET_GAMMA_ENABLE	_IOR(DISPIO_TYPE, 0x40, uint32_t)
#define DISPIO_SET_GAMMA_PARAM	_IOW(DISPIO_TYPE, 0x41, gp_disp_gammatable_t)
#define DISPIO_GET_GAMMA_PARAM	_IOR(DISPIO_TYPE, 0x41, gp_disp_gammatable_t *)

/* Color bar */
#define DISPIO_SET_CBAR_ENABLE	_IOW(DISPIO_TYPE, 0x50, uint32_t)
#define DISPIO_GET_CBAR_ENABLE	_IOR(DISPIO_TYPE, 0x50, uint32_t)
#define DISPIO_SET_CBARINFO		_IOW(DISPIO_TYPE, 0x51, gp_disp_colorbar_t *)

/* Buffer control */
#define DISPIO_BUF_ALLOC	_IOW(DISPIO_TYPE, 0x60, gp_disp_bufinfo_t *)
#define DISPIO_BUF_FREE		_IOW(DISPIO_TYPE, 0x61, uint32_t)
#define DISPIO_BUF_MMAP		_IOW(DISPIO_TYPE, 0x62, gp_disp_bufaddr_t *)
#define DISPIO_BUF_MUNMAP	_IOW(DISPIO_TYPE, 0x63, gp_disp_bufaddr_t *)
#define DISPIO_BUF_GETINFO	_IOR(DISPIO_TYPE, 0x64, gp_disp_bufinfo_t *)

/* OSD layer */
#define DISPIO_OSD_BASE		0x80

#define DISPIO_GET_OSD_TOTALNUM		_IOR(DISPIO_TYPE, 0x70, uint32_t *)

#define DISPIO_SET_OSD_ENABLE(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x0 + (osdIndex) * 0x10, uint32_t)
#define DISPIO_GET_OSD_ENABLE(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x0 + (osdIndex) * 0x10, uint32_t *)
#define DISPIO_SET_OSD_BITMAP(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x1 + (osdIndex) * 0x10, gp_bitmap_t)
#define DISPIO_GET_OSD_BITMAP(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x1 + (osdIndex) * 0x10, gp_bitmap_t *)
#define DISPIO_SET_OSD_SCALEINFO(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x2 + (osdIndex) * 0x10, gp_disp_scale_t)
#define DISPIO_GET_OSD_SCALEINFO(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x2 + (osdIndex) * 0x10, gp_disp_scale_t *)
#define DISPIO_SET_OSD_PALETTE(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x3 + (osdIndex) * 0x10, gp_disp_osdpalette_t)
#define DISPIO_GET_OSD_PALETTE(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x3 + (osdIndex) * 0x10, gp_disp_osdpalette_t *)
#define DISPIO_SET_OSD_PALETTEOFFSET(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x4 + (osdIndex) * 0x10, uint32_t)
#define DISPIO_GET_OSD_PALETTEOFFSET(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x4 + (osdIndex) * 0x10, uint32_t *)
#define DISPIO_SET_OSD_ALPHA(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x5 + (osdIndex) * 0x10, uint32_t)
#define DISPIO_GET_OSD_ALPHA(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x5 + (osdIndex) * 0x10, uint32_t *)
#define DISPIO_SET_OSD_KEY(osdIndex)	_IOW(DISPIO_TYPE, DISPIO_OSD_BASE + 0x6 + (osdIndex) * 0x10, uint32_t)
#define DISPIO_GET_OSD_KEY(osdIndex)	_IOR(DISPIO_TYPE, DISPIO_OSD_BASE + 0x6 + (osdIndex) * 0x10, uint32_t *)



/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define GP_DISP_BUFFER_MAX 8	/* max buffer count */

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
/** @brief A structure of output device operations */
typedef struct gp_disp_panel_ops_s {
	int32_t (*init) (void);
	int32_t (*suspend) (void);
	int32_t (*resume) (void);
	int32_t (*get_size) (gp_size_t *size);
	int32_t (*set_param) (void *data);
} gp_disp_panel_ops_t;

enum {
	SP_DISP_OUTPUT_LCD = 0,
	SP_DISP_OUTPUT_LCM,
	SP_DISP_OUTPUT_TV
} SP_DISP_OUTPUT_TYPE;

enum {
	SP_DISP_TV_MODE_NTSC = 0,
	SP_DISP_TV_MODE_PAL,
} SP_DISP_TV_MODE;

/** @brief A structure of output device */
typedef struct gp_disp_output_s
{
	uint32_t index;		/*!< @brief Output device index */
	uint32_t type;		/*!< @brief Output device type */
	uint8_t name[32];	/*!< @brief Output device name */
	gp_disp_panel_ops_t *ops;	/*!< @brief Output device operations */
} gp_disp_output_t;

/** @brief A structure of lcd info */
typedef struct gp_disp_lcdparam_s {
	uint16_t vPolarity;	/*!< @brief Vsync polarity */
	uint16_t vFront;		/*!< @brief Vsync front porch */
	uint16_t vBack;		/*!< @brief Vsync back porch */
	uint16_t vWidth;		/*!< @brief Vsync width */
	uint16_t hPolarity;	/*!< @brief Hsync polarity */
	uint16_t hFront;		/*!< @brief Hsync front porch */
	uint16_t hBack;		/*!< @brief Hsync back porch */
	uint16_t hWidth;		/*!< @brief Hsync width */
} gp_disp_lcdparam_t;

/** @brief A structure of display resolution */
typedef struct gp_disp_res_s {
	uint16_t width;	/*!< @brief Width */
	uint16_t height;	/*!< @brief Height */
} gp_disp_res_t;

/** @brief A structure of display scale setting */
typedef struct gp_disp_scale_s {
	int16_t x;			/*!< @brief X position */
	int16_t y;			/*!< @brief Y position */
	uint16_t width;		/*!< @brief Width */
	uint16_t height;		/*!< @brief Height */
	uint32_t blankcolor;	/*!< @brief Blank color RGB/CrCbY */
} gp_disp_scale_t;

/** @brief A structure of dithering map */
typedef struct gp_disp_ditherparam_s {
	uint32_t d00;
	uint32_t d01;
	uint32_t d02;
	uint32_t d03;
	uint32_t d10;
	uint32_t d11;
	uint32_t d12;
	uint32_t d13;
	uint32_t d20;
	uint32_t d21;
	uint32_t d22;
	uint32_t d23;
	uint32_t d30;
	uint32_t d31;
	uint32_t d32;
	uint32_t d33;
} gp_disp_ditherparam_t;

/** @brief A structure of color matrix parameters */
typedef struct gp_disp_colormatrix_s {
	uint16_t a00;
	uint16_t a01;
	uint16_t a02;
	uint16_t a10;
	uint16_t a11;
	uint16_t a12;
	uint16_t a20;
	uint16_t a21;
	uint16_t a22;
	uint16_t b0;
	uint16_t b1;
	uint16_t b2;
} gp_disp_colormatrix_t;

enum {
	SP_DISP_GAMMA_R = 0,
	SP_DISP_GAMMA_G = 1,
	SP_DISP_GAMMA_B = 2
};

/** @brief A structure of gamma table */
typedef struct gp_disp_gammatable_s {
	uint32_t id;		/*!< @brief The id of gamma table */
	uint8_t table[256];	/*!< @brief The gamma table */
} gp_disp_gammatable_t;

enum {
	SP_DISP_OSD_TYPE_16BPP = 0,
	SP_DISP_OSD_TYPE_8BPP = 1,
	SP_DISP_OSD_TYPE_4BPP = 2,
	SP_DISP_OSD_TYPE_1BPP = 3,
};

/** @brief A structure of osd palette */
typedef struct gp_disp_osdpalette_s {
	uint32_t type;		/*!< @brief The type of osd layer  */
	uint32_t startIndex;	/*!< @brief The offset index of palette table in sram */
	uint32_t count;		/*!< @brief The number of palette color */
	uint32_t table[256];	/*!< @brief The source palette table */
} gp_disp_osdpalette_t;

enum {
	SP_DISP_ALPHA_PERPIXEL = 0,
	SP_DISP_ALPHA_CONSTANT = 1,
};	/* consta */

enum {
	SP_DISP_ALPHA_PERPIXEL_ONLY = 0,
	SP_DISP_ALPHA_COLORKEY_ONLY = 1,
	SP_DISP_ALPHA_BOTH = 2,
};	/* ppamd */

/** @brief A structure of osd alpha */
typedef struct gp_disp_osdalpha_s {
	uint32_t consta; 	/*!< @brief The type1 of alpha blending. */
	uint32_t ppamd; 	/*!< @brief The type2 of alpha blending. */
	uint16_t alpha;	/*!< @brief The value of alpha blending from 0~100. */
} gp_disp_osdalpha_t;

/** @brief A structure of color bar */
typedef struct gp_disp_colorbar_s {
	uint32_t type;		/*!< @brief The type of color bar. */
	uint32_t size;		/*!< @brief The number of column per bar. */
	uint32_t color;	/*!< @brief The color value of color bar. [23:16] = r, [15:8] = g, [7:0] = b */
} gp_disp_colorbar_t;

/** @brief A structure of buffer control */
typedef struct gp_disp_bufinfo_s {
	uint32_t id;		/*!< @brief Buffer id */
	uint16_t width;		/*!< @brief Width */
	uint16_t height;	/*!< @brief Height */
	uint32_t bpp;		/*!< @brief Bits per pixel */
	uint32_t size;		/*!< @brief Buffer size in bytes */
} gp_disp_bufinfo_t;

/** @brief A structure of buffer address */
typedef struct gp_disp_bufaddr_s {
	uint32_t id;	/*!< @brief Buffer id */
	void *ptr;		/*!< @brief Buffer address */
} gp_disp_bufaddr_t;

enum display_state {
	DISP_STATE_SUSPEND,
	DISP_STATE_RESUME,
};

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
extern int32_t register_paneldev(uint32_t type, char *name, gp_disp_panel_ops_t *fops);
extern int32_t unregister_paneldev(uint32_t type, char *name);

extern void disp_update(void);
extern int32_t disp_wait_frame_end(void);
extern void disp_get_panel_res(gp_size_t *res);
extern void disp_set_osd_enable(uint32_t layerNum, uint32_t enable);
extern void disp_set_osd_bitmap(uint32_t layerNum, gp_bitmap_t *pbitmap);
extern void disp_set_osd_scale(uint32_t layerNum, gp_disp_scale_t *pscale);
extern void disp_set_osd_alpha(uint32_t layerNum, gp_disp_osdalpha_t *palpha);
extern void disp_set_osd_frame_addr(uint32_t layerNum, uint8_t *addr);
extern void disp_set_dither_enable(uint32_t enable);
extern void disp_set_dither_type(uint32_t type);
extern void disp_set_dither_param(gp_disp_ditherparam_t *pDitherParam);
extern void disp_set_color_matrix(gp_disp_colormatrix_t *pColorMatrix);
extern void disp_set_gamma_enable(uint32_t enable);
extern void disp_set_gamma_table(uint32_t id, uint8_t *pTable);
extern void* disp_allocate_buffer(gp_disp_bufinfo_t info);
extern int32_t disp_free_buffer(uint32_t id);
extern int32_t disp_spi(uint32_t val, uint32_t bitsLen, uint32_t lsbFirst);

#endif //endif _GP_DISPLAY_DEVICE_H_
