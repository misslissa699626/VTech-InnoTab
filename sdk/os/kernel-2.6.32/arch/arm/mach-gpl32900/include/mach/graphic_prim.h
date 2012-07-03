//----------------------------------------------------------------------------
#ifndef _GRAPHICS_PRIM_H_
#define _GRAPHICS_PRIM_H_
typedef enum 
{
    COLOR_RGB565,           // 0
    COLOR_RGB1555,          // 1
    COLOR_RGB4444,          // 2
    COLOR_RGB888,           // 3
    COLOR_YUYV,             // 4
    COLOR_4Y4U4Y4V,         // 5
    COLOR_YUV420,           // 6
    COLOR_YCbYCr,           // 7
    COLOR_4Y4Cb4Y4Cr,       // 8
    COLOR_YCbCr420,         // 9
    COLOR_4BIT_IDX,         // 10
    COLOR_8BIT_IDX,         // 11
    COLOR_RGB444,           // 12
    COLOR_YUV422,           // 13
    COLOR_YCbCr422,         // 14
    COLOR_YUV400,           // 15
    COLOR_YCbCr400,         // 16
    COLOR_MODE_NUMBER       // 17
}color_format;
//----------------------------------------------------------------------------
#define RGB565_RED(x)   ((( x >> 3 )& 0x1F) << 11)
#define RGB565_GREEN(x) ((( x >> 2 )& 0x3F) << 5)
#define RGB565_BLUE(x)  ((( x >> 3 )& 0x1F))

#define	RED_565(v)		((v >> 11) & 0x1F)
#define	GREEN_565(v)	((v >> 5) & 0x3F)
#define	BLUE_565(v)		((v) & 0x1F)

#define	RGB565(r,g,b)	(((r & 0x1F) << 11) |	((g & 0x3F) << 5) | (b & 0x1F))
//----------------------------------------------------------------------------
/////// 	define for the color translation of RGB 555 mode

#define RGB555_RED(x)   ((( x >> 3 )& 0x1F) << 10)
#define RGB555_GREEN(x) ((( x >> 3 )& 0x1F) << 5)
#define RGB555_BLUE(x)  ((( x >> 3 )& 0x1F))

#define	RED_555(v)		((v >> 10) & 0x1F)
#define	GREEN_555(v)	((v >> 5) & 0x1F)
#define	BLUE_555(v)		((v) & 0x1F)

#define	RGB555(r,g,b)	(((r & 0x1F) << 10) | ((g & 0x1F) << 5) | (b & 0x1F))
//----------------------------------------------------------------------------
/////// 	defines for the color translation of RGB 888 mode

#define RGB888_RED(x)   (((x) & 0xFF) << 16)
#define RGB888_GREEN(x) (((x) & 0xFF) << 8)
#define RGB888_BLUE(x)  (((x) & 0xFF))

#define	RED_888(v)		((v >> 16) & 0xFF)
#define	GREEN_888(v)	((v >> 8) & 0xFF)
#define	BLUE_888(v)		((v) & 0xFF)

#define	RGB888(r,g,b)	(((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))
//----------------------------------------------------------------------------
// RGB_888 2 YUV444
//Y = ( (  66 * R + 129 * G +  25 * B + 128) >> 8) +  16
//U = ( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128
//V = ( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128
#define ConvertRGB888_Y444(r,g,b) (((66*(int)r+129*(int)g+25*(int)b+128)>>8)+16)
#define ConvertRGB888_U444(r,g,b) (((-38*(int)r-74*(int)g+112*(int)b+128)>>8)+128)
#define ConvertRGB888_V444(r,g,b) (((112*(int)r-94*(int)g-18*(int)b+128)>>8)+128)
//----------------------------------------------------------------------------
//C = Y - 16
//D = U - 128
//E = V - 128
//R = clip(( 298 * C           + 409 * E + 128) >> 8)
//G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)
//B = clip(( 298 * C + 516 * D           + 128) >> 8)

//Range need to be clipped between [0..255]
#define ConvertYUV444_R_need_clip(y,u,v) ((298*(y-16)+409*(v-128)+128)>>8)
#define ConvertYUV444_G_need_clip(y,u,v) ((298*(y-16)-100*(u-128)-208*(v-128)+128)>>8)
#define ConvertYUV444_B_need_clip(y,u,v) ((298*(y-16)+516*(u-128)+128)>> 8)
//----------------------------------------------------------------------------
#endif


