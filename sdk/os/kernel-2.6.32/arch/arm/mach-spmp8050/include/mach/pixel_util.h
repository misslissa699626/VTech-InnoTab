#include <mach/common.h>
#include <mach/graphic_prim.h>

//----------------------------------------------------------------------------
void      set_pixel_4Idx(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_4Idx(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_8Idx(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_8Idx(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_888(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_888(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_565(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_565(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_555(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_555(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_yuv_yuyv(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_yuv_yuyv(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_yuv_4y4u4y4v(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_yuv_4y4u4y4v(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_ycbcr_ycbycr(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_ycbcr_ycbycr(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void      set_pixel_ycbcr_4y4cb4y4cr(INT32 row, INT32 col, UINT32 val, UINT32 aWidth, UINT32 aStartAddr);
UINT32 get_pixel_ycbcr_4y4cb4y4cr(INT32 row, INT32 col, UINT32 aWidth, UINT32 aStartAddr);
void clear_framebuffer(UINT32 aAddr, UINT32 aWidth, UINT32 aHeight, color_format aBufferFormat);
	