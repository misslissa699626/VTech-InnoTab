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
 * @file typedef.h
 * @brief Basic datatype definition
 * @author kt.huang@sunplusmm.com
 */
#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/** @brief A structure of representing a point, the position */
typedef struct gp_point_s {
	int16_t x; /*!< @brief The x value of the point structure */
	int16_t y; /*!< @brief The y value of the point structure */
} gp_point_t;

/** @brief A structure of a rectange size, only width and height */
typedef struct gp_size_s {
	int16_t width;  /*!< @brief The width of the gp_size_s structure */
	int16_t height; /*!< @brief The height of the gp_size_s structure */
} gp_size_t;

/** @brief The structure of a rectangle */
typedef struct gp_rect_s {
	int16_t x;			/*!< @brief Top-left X position */
	int16_t y;			/*!< @brief Top-left Y position */
	int16_t width;		/*!< @brief Width */
	int16_t height;		/*!< @brief Height */
} gp_rect_t;

/** @brief The structure of a color */
typedef struct gp_color_s {
	uint8_t blue;     /*!< @brief Color blue */
	uint8_t green;    /*!< @brief Color green */
	uint8_t red;      /*!< @brief Color red */
	uint8_t alpha;    /*!< @brief The alpha value of a color. */
} gp_color_t;

/** @brief The structure of the bitmap */
typedef struct gp_bitmap_s {
	uint16_t width;             /*!< @brief The width of bitmap */
	uint16_t height;            /*!< @brief The height of bitmap */
	uint16_t bpl;               /*!< @brief Bytes per line */
	uint8_t type;               /*!< @brief One of SP_BITMAP_YUV422, SP_BITMAP_RGB565, SP_BITMAP_8BPP, SP_BITMAP_1BPP */
	uint8_t *pData;             /*!< @brief The data pointer of bitmap */
	uint8_t *pDataU;            /*!< @brief The data pointer of bitmap (U channel) */
	uint8_t *pDataV;            /*!< @brief The data pointer of bitmap (V channel) */
	uint16_t strideUV;          /*!< @brief stride of UV data */
	gp_color_t keyColor;        /*!< @brief The keycolor of bitmap */
	uint32_t keyColorIdx;       /*!< @brief The color key index of this bitmap (if any).
								If keycolor exceeds the max color value,
								the keycolor will be disabled. */
	gp_color_t *pPalette;       /*!< @brief The palette of bitmap (if any) */
	gp_rect_t validRect;		/*!< @brief The real data region */
} gp_bitmap_t;



/** @defgroup typedef Basic Types
 *  @{
 */

#ifndef __SP_INT_TYPES__
#define __SP_INT_TYPES__
/** @brief UINT64 is a 64 bits unsigned integer */
typedef unsigned long long UINT64;
/** @brief SINT64 is a 64 bits signed integer */
typedef signed long long   SINT64;
/** @brief UINT32 is a 32 bits unsigned integer */
typedef unsigned int       UINT32;
/** @brief SINT32 is a 32 bits signed integer */
typedef signed int         SINT32;
/** @brief UINT16 is a 16 bits unsigned integer */
typedef unsigned short     UINT16;
/** @brief SINT16 is a 16 bits signed integer */
typedef signed short       SINT16;
/** @brief UINT8 is a 8 bits unsigned integer */
typedef unsigned char      UINT8;
/** @brief SINT8 is a 8 bits signed integer */
typedef signed char        SINT8;
#endif

/** @brief SP_BOOL is a boolean value */
typedef UINT32             SP_BOOL;
/** @brief SP_CHAR is a 8 bits character */
typedef char               SP_CHAR;
/** @brief SP_WCHAR is a 16 bits wide character (for unicode) */
typedef UINT16             SP_WCHAR;
/** @brief HANDLE is a 32 bits integer (it is a uniform HANDLE) */
typedef void*              HANDLE;

#ifndef NULL
/** @brief A value that represents NULL. */
#define NULL ((void *)0)
#endif

/** @brief A value that represents a "OK" result. */
#define SP_OK              0
/** @brief A value that represents a "FAIL" result. */
#define SP_FAIL            1
/** @brief A value that represents a "SUCCESS" result */
#define SP_SUCCESS         0
/** @brief TRUE value */
#define SP_TRUE            1
/** @brief FALSE value */
#define SP_FALSE           0

#define DO_MEMLOG	0	/* a flag to turn on the memory log ! */

/** @} */


/** @defgroup spErrCode Error Codes
 *  @{
 */

enum {
	SP_ERR = 1,                  /*!< @brief Generic error */
	SP_ERR_MEM,                  /*!< @brief Insufficient memory */
	SP_ERR_LIST_IS_EMPTY,        /*!< @brief The list is empty */
	SP_ERR_NO_NODE,              /*!< @brief Cannot insert message to global message queue */
	SP_ERR_FNT_OPEN_FAIL,        /*!< @brief Open font error */
	SP_ERR_FNT_READ_FAIL,        /*!< @brief Read font error */
	SP_ERR_FNT_SIGNATURE,        /*!< @brief Font signature error */
	SP_ERR_FNT_READ_IDX_FAIL,    /*!< @brief Font read index error */
	SP_ERR_FNT_SEEK_FAIL,        /*!< @brief Font seek error */
	SP_ERR_FNT_CODEPAGE,         /*!< @brief Font codepage error */
	SP_ERR_OPEN,                 /*!< @brief Open error */
	SP_ERR_READ,                 /*!< @brief Read error */
	SP_ERR_WRITE,                /*!< @brief Write error */
	SP_ERR_BMPFMT_MISMATCH,      /*!< @brief The bitmap format mismtach */
	SP_ERR_EXCEED_MAX_DRIVER,    /*!< @brief Exceed maxinum of drivers */
	SP_ERR_TMR_POSTPONE,         /*!< @brief Postpone the timer event. */
	SP_ERR_PARAM,                /*!< @brief Parameter to the function is incorrect. */
	SP_ERR_NOT_FOUND,            /*!< @brief Cannot find data */
	SP_ERR_ITEM_EXIST,           /*!< @brief Item is exist */
	SP_ERR_BUSY,                 /*!< @brief Resource is busy */
	SP_ERR_FORMAT_UNSUPPORTED,   /*!< @brief Format unsupported */
	SP_ERR_NOT_IMPLEMENT,        /*!< @brief The service is not implemented */
	SP_ERR_EOF,                  /*!< @brief Read EOF error */
	SP_ERR_STOP,                 /*!< @brief The service is stop error */
	SP_ERR_UNHANDLED,            /*!< @brief Message unhandled */
	SP_ERR_SETTINGS_SIGNATURE,   /*!< @brief Settings signature error */
	SP_ERR_SETTINGS_VERSION,     /*!< @brief Settings version error */
	SP_ERR_HW_LIMIT,             /*!< @brief Hardware limitation error */
	SP_ERR_RESOURCE_NOT_FOUND,   /*!< @brief Resource not found */
	SP_ERR_JPEG_DECODE,          /*!< @brief Jpeg decode error */
	SP_ERR_JPEG_FORMAT,          /*!< @brief Jpeg format error */
	SP_ERR_SPACE_NOT_ENOUGH,     /*!< @brief Storage is full */	
	SP_ERR_NOT_SUPPORTED,		 /*!< @brief Not supported function */
	SP_ERR_MAX                   /*!< @brief The last error message */
};

/** @} */


/** @defgroup spPoint_t Point
 *  @{
 */

/** @brief A structure of representing a point, the position */
typedef struct spPoint_s {
	SINT16 x; /*!< @brief The x value of the point structure */
	SINT16 y; /*!< @brief The y value of the point structure */
} spPoint_t;


/** @breif A structure of representing a fixed point, the position */
typedef struct spFixPoint_s {
	SINT32 x; /*!< @breif The x value of the point structure */
	SINT32 y; /*!< @breif The y value of the point structure */
} spFixPoint_t;

/** @brief A macro for initialing the point structure */
#define spPointReset( p, a, b ) (p).x = (a); (p).y = (b);

/** @} */


/** @defgroup spRectSize_t RectSize
 *  @{
 */

/** @brief A structure of a rectange size, only width and height */
typedef struct spRectSize_s {
	SINT16 width;  /*!< @brief The width of the spRectSize_s structure */
	SINT16 height; /*!< @brief The height of the spRectSize_s structure */
} spRectSize_t;

/** @brief A macro for initialing the rectangle size structure */
#define spRectSizeReset( p, w, h ) (p).width = (w); (p).height = (h);

/** @} */


/** @defgroup color Color
 *  @{
 */

/** @brief The structure of a color */
typedef struct spRgbQuad_s {
	UINT8 blue;     /*!< @brief Color blue */
	UINT8 green;    /*!< @brief Color green */
	UINT8 red;      /*!< @brief Color red */
	UINT8 alpha;    /*!< @brief The alpha value of a color. This is reserved for future  */
} spRgbQuad_t;

/** @brief The structure of a color (union with a unsigned long integer) */
typedef struct spRgbQuadVal_s
{
	union {
		spRgbQuad_t rgb;    /*!< @brief The RGB structure */
		UINT32	value;	    /*!< @brief The unsigned long value */
	} color;
} spRegQuadVal_t;

#if 0
/**
 * @brief Helper function to make a color.
 * @param r The red channel.
 * @param g The green channel.
 * @param b The blue channel.
 * @return a spRgbQuad_t instance.
 *
 * \par Sample:
 * \code
 * spRgbQuad_t white = MAKE_RGB(255, 255, 255);
 * spRgbQuad_t red = MAKE_RGB(255, 0, 0);
 * spRgbQuad_t green = MAKE_RGB(0, 255, 0);
 * spRgbQuad_t blue = MAKE_RGB(0, 0, 255);
 * spRgbQuad_t yellow = MAKE_RGB(255, 255, 0);
 * \endcode
 */
spRgbQuad_t MAKE_RGB(UINT8 r, UINT8 g, UINT8 b);

/**
 * @brief Helper function to make a color.
 * @param a The alpha channel.
 * @param r The red channel.
 * @param g The green channel.
 * @param b The blue channel.
 * @return a spRgbQuad_t instance.
 *
 * Same as MAKE_RGB(), but can also specify the alpha channel of the color.
 * @note The alpha channel is currently unused.
 */
spRgbQuad_t MAKE_ARGB(UINT8 a, UINT8 r, UINT8 g, UINT8 b);


/**
 * @brief Predefined color to represent no color key.
 *
 * This color guarantees that the pixel with this color is non-transparent.
 */
extern const spRgbQuad_t SP_COLOR_NO_KEY_COLOR;

/** @brief Predefined color to represent transparent color. */
extern const spRgbQuad_t SP_COLOR_TRANSPARENT;

/** @brief Predefined color to represent black. */
extern const spRgbQuad_t SP_COLOR_BLACK;

/** @brief Predefined color to represent white. */
extern const spRgbQuad_t SP_COLOR_WHITE;
#endif

/** @} */


/** @defgroup rect Rectangle
 *  
 *  @{
 */
/** @brief The structure of a rectangle */
typedef struct spRect_s {
	SINT16 x;			/*!< @brief Top-left X position */
	SINT16 y;			/*!< @brief Top-left Y position */
	SINT16 width;		/*!< @brief Width */
	SINT16 height;		/*!< @brief Height */
} spRect_t;

/** @} */


/** @defgroup bitmap Module Bitmap
 *  @{
 */

/** @brief Bitmap format: YUV422 (YYYYYYYYUUUUVVVV) */

enum {
	SP_BITMAP_RGB565 = 0,
	SP_BITMAP_RGAB5515,
	SP_BITMAP_ARGB1555,
	SP_BITMAP_ARGB4444,
	SP_BITMAP_RGB888,
	SP_BITMAP_ARGB8888,
	SP_BITMAP_BGR565,
	SP_BITMAP_RGB555,
	SP_BITMAP_BGAR5515,
	SP_BITMAP_ABGR1555,
	SP_BITMAP_ABGR4444,
	SP_BITMAP_BGR888,
	SP_BITMAP_ABGR8888,
	SP_BITMAP_1BPP,
	SP_BITMAP_2BPP,
	SP_BITMAP_4BPP,
	SP_BITMAP_8BPP,
	SP_BITMAP_YCbCr,
	SP_BITMAP_YUV,
	SP_BITMAP_YCbYCr,
	SP_BITMAP_YUYV,
	SP_BITMAP_4Y4U4Y4V,
	SP_BITMAP_4Y4Cb4Y4Cr,
	SP_BITMAP_YCbCr400,
	SP_BITMAP_YUV400,
	SP_BITMAP_YCbCr422,
	SP_BITMAP_YUV422,
	SP_BITMAP_YCbCr444,
	SP_BITMAP_YUV444,
	SP_BITMAP_YCbCr420,       /*!< @brief Ceva decoder output format */
	SP_BITMAP_YUV420,         /*!< @brief Ceva decoder output format */
	NUM_BITMAP_TYPE
};

/** @brief The structure of the bitmap */
typedef struct spBitmap_s {
	UINT16 width;             /*!< @brief The width of bitmap */
	UINT16 height;            /*!< @brief The height of bitmap */
	UINT16 bpl;               /*!< @brief Bytes per line */
	UINT8 type;               /*!< @brief One of SP_BITMAP_YUV422, SP_BITMAP_RGB565, SP_BITMAP_8BPP, SP_BITMAP_1BPP */
	UINT8 *pData;             /*!< @brief The data pointer of bitmap */
	UINT8 *pDataU;            /*!< @brief The data pointer of bitmap (U channel) */
	UINT8 *pDataV;            /*!< @brief The data pointer of bitmap (V channel) */
	UINT16 strideUV;          /*!< @brief stride of UV data */
	spRgbQuad_t keyColor;     /*!< @brief The keycolor of bitmap */
	UINT32 keyColorIdx;       /*!< @brief The color key index of this bitmap (if any).
								If keycolor exceeds the max color value,
								the keycolor will be disabled. */
	spRgbQuad_t *pPalette;    /*!< @brief The palette of bitmap (if any) */
	spRect_t validRect;		/*!< @brief The real data region */
} spBitmap_t;

/** @} */


#ifdef __cplusplus
};
#endif


#endif

