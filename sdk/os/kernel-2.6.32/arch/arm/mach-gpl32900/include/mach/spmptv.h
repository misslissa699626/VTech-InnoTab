#include <mach/graphic_prim.h>

typedef enum 
{
	TV_OUT_PAL,
	TV_OUT_PAL_Nc,
	TV_OUT_PAL_M,
	TV_OUT_NTSC,
	TV_OUT_NTSC_J,
	TV_OUT_NTSC443
}tv_out_format;
//----------------------------------------------------------------------------
#define	 TV_OUT_FLAG_INTERLACE				0x00000001

#define	 TV_OUT_FLAG_BIG_ENDIAN			0x00000002
#define	 TV_OUT_FLAG_LITTLE_ENDIAN		0x00000000

#define	 TV_OUT_FLAG_BURST_16				0x00000000
#define	 TV_OUT_FLAG_FORCE_BURST_4		0x00000004

#define	 TV_OUT_FLAG_CROSS_1KB_EN		0x00000000
#define	 TV_OUT_FLAG_CROSS_1KB_DIS		0x00000008

#define	 TV_OUT_FLAG_VER_576_PIX			0x00000080
#define	 TV_OUT_FLAG_VER_480_PIX			0x00000010
#define	 TV_OUT_FLAG_VER_240_PIX			0x00000040

#define	 TV_OUT_FLAG_HOR_720_PIX			0x00000020
#define	 TV_OUT_FLAG_HOR_640_PIX			0x00000000

#define	 TV_OUT_FLAG_FULL_SCREEN			0x80000000
//----------------------------------------------------------------------------
#ifndef POS_CENTER
#define POS_CENTER		0x0000
#define POS_RIGHT			0x0001
#define POS_LEFT			0x0002
#define POS_HOR_MSK	0x0003
#define POS_TOP				0x0004
#define POS_BOTTOM		0x0008
#define POS_VER_MSK		0x000C
#define POS_CENTRE		0x000F
#endif
//----------------------------------------------------------------------------
typedef struct _tv_cfg_context_s{
       char *name;
	unsigned int mWidth; 
	unsigned int mHeight; 
	unsigned int mAddr; 
	color_format mBufferFormat;
	tv_out_format mOutFormat;
	unsigned int mFlag;
	unsigned short bpp;	
} tv_cfg_context_t;
//----------------------------------------------------------------------------
int tv_isr(void);
void tv_out_set_position(unsigned int aWidth, unsigned int aHeight, unsigned int aPosFlag);
int tv_out_init(unsigned int aWidth, unsigned int aHeight, unsigned int aAddr, color_format aBufferFormat, tv_out_format aOutFormat, unsigned int aFlag);
void tv_out_irq_enable(unsigned int aMode); // 0 : disable 1 : each frame 2: each field
void tv_out_adjust_y_signal(unsigned int aY_Sync, unsigned int aY_Blank, unsigned int aY_Setup);
void tv_out_clipping(unsigned int aX, unsigned int aY, unsigned int aRecWidth, unsigned int aRecHeight, unsigned int aBufferWidth);
void tv_out_full_screen(void);
void tv_out_hor_scaling(unsigned int aWidth, unsigned int aDstWidth);
void tv_set_shadow_buffer(unsigned int aAddr);
unsigned int tv_get_shadow_buffer_addr(void);
int tv_flip(void);
void tv_out_enable_interrupt(void);
void tv_out_interrupt_clear(void);
void tv_out_setBuffFormat(color_format aBufferFormat,int PollInt);
color_format tv_out_getBuffFormat(void);
unsigned int tv_out_getBuffHeight(void);
unsigned int tv_out_getBuffWidth(void);
unsigned int tv_get_buffer_addr(void);
void tv_set_buffer_addr(unsigned int aAddr);

void tv_outctrl_register(unsigned int  base_addr);
//----------------------------------------------------------------------------

#define TV_MODE_MASK		0x1F
#define TV_MODE_NTSC		0x10
#define TV_MODE_PAL		0x11
#define TV_MODE_QVGA		0x20

#define TV_COLOR_MASK	0x80
#define TV_COLOR_RGB		0x80
#define TV_COLOR_YUV		0x00

#define TV_PAL_SHRINK_WIDTH		40
#define TV_NTSC_SHRINK_WIDTH	40


/// add by chenhn 	2008/06/13
// for enabling the D1 resolution of TVout, this is the default setting
int tv_out_D1_enable(unsigned char mode, unsigned char* p_frame, unsigned char* p_shadow);
int tv_out_QVGA_enable(unsigned char mode, unsigned char* p_frame, unsigned char* p_shadow);
int tv_out_disable(void);


#define TV_PAL_MAX_WIDTH		(720 - TV_PAL_SHRINK_WIDTH)
#define TV_PAL_MAX_HEIGHT		572

#define TV_NTSC_MAX_WIDTH		(720 - TV_NTSC_SHRINK_WIDTH)
#define TV_NTSC_MAX_HEIGHT	480