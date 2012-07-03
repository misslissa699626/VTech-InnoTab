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
#ifndef _GP_SCALE2_H_
#define _GP_SCALE2_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#ifndef ENABLE
#define ENABLE 		1
#endif
#ifndef DISABLE
#define DISABLE		0
#endif

/* input format */
#define	C_SCALE2_CTRL_IN_RGB1555			0x00000000
#define	C_SCALE2_CTRL_IN_RGB565				0x00000001
#define	C_SCALE2_CTRL_IN_RGBG				0x00000002
#define	C_SCALE2_CTRL_IN_GRGB				0x00000003
#define	C_SCALE2_CTRL_IN_YUYV				0x00000004
#define	C_SCALE2_CTRL_IN_UYVY				0x00000005
#define	C_SCALE2_CTRL_IN_VYUY				0x10000005
#define	C_SCALE2_CTRL_IN_YUYV8X32			0x00100006
#define	C_SCALE2_CTRL_IN_YUYV8X64			0x00100007
#define	C_SCALE2_CTRL_IN_YUYV16X32			0x00200006
#define	C_SCALE2_CTRL_IN_YUYV16X64			0x00200007
#define	C_SCALE2_CTRL_IN_YUYV32X32			0x00000006
#define	C_SCALE2_CTRL_IN_YUYV64X64			0x00000007
#define	C_SCALE2_CTRL_IN_VYUY8X32			0x10100006
#define	C_SCALE2_CTRL_IN_VYUY8X64			0x10100007
#define	C_SCALE2_CTRL_IN_VYUY16X32			0x10200006
#define	C_SCALE2_CTRL_IN_VYUY16X64			0x10200007
#define	C_SCALE2_CTRL_IN_VYUY32X32			0x10000006
#define	C_SCALE2_CTRL_IN_VYUY64X64			0x10000007
#define	C_SCALE2_CTRL_IN_YUV422				0x00000008
#define	C_SCALE2_CTRL_IN_YUV420				0x00000009
#define	C_SCALE2_CTRL_IN_YUV411				0x0000000A
#define	C_SCALE2_CTRL_IN_YUV444				0x0000000B
#define	C_SCALE2_CTRL_IN_Y_ONLY				0x0000000C
#define	C_SCALE2_CTRL_IN_YUV422V			0x0000000D
#define	C_SCALE2_CTRL_IN_YUV411V			0x0000000E
#define	C_SCALE2_CTRL_IN_ARGB4444			0x0000000F
#define	C_SCALE2_CTRL_IN_ARGB8888			0x2000000F
#define	C_SCALE2_CTRL_IN_MASK				0x3030000F

/* output format */
#define	C_SCALE2_CTRL_OUT_RGB1555			0x00000000
#define	C_SCALE2_CTRL_OUT_RGB565			0x00000010
#define	C_SCALE2_CTRL_OUT_RGBG				0x00000020
#define	C_SCALE2_CTRL_OUT_GRGB				0x00000030
#define	C_SCALE2_CTRL_OUT_YUYV				0x00000040
#define	C_SCALE2_CTRL_OUT_UYVY				0x00000050
#define	C_SCALE2_CTRL_OUT_VYUY				0x10000050
#define	C_SCALE2_CTRL_OUT_YUYV8X32			0x00040060
#define	C_SCALE2_CTRL_OUT_YUYV8X64			0x00040070
#define	C_SCALE2_CTRL_OUT_YUYV16X32			0x00080060
#define	C_SCALE2_CTRL_OUT_YUYV16X64			0x00080070
#define	C_SCALE2_CTRL_OUT_YUYV32X32			0x00000060
#define	C_SCALE2_CTRL_OUT_YUYV64X64			0x00000070
#define	C_SCALE2_CTRL_OUT_VYUY8X32			0x10040060
#define	C_SCALE2_CTRL_OUT_VYUY8X64			0x10040070
#define	C_SCALE2_CTRL_OUT_VYUY16X32			0x10080060
#define	C_SCALE2_CTRL_OUT_VYUY16X64			0x10080070
#define	C_SCALE2_CTRL_OUT_VYUY32X32			0x10000060
#define	C_SCALE2_CTRL_OUT_VYUY64X64			0x10000070
#define	C_SCALE2_CTRL_OUT_YUV422			0x00000080
#define	C_SCALE2_CTRL_OUT_YUV420			0x00000090
#define	C_SCALE2_CTRL_OUT_YUV411			0x000000A0
#define	C_SCALE2_CTRL_OUT_YUV444			0x000000B0
#define	C_SCALE2_CTRL_OUT_Y_ONLY			0x000000C0
#define	C_SCALE2_CTRL_OUT_ARGB4444			0x000000F0
#define	C_SCALE2_CTRL_OUT_ARGB8888			0x400000F0
#define	C_SCALE2_CTRL_OUT_MASK				0x400C00F0

/* scale para */
#define C_SCALE2_FULL_SCREEN				0x00000000
#define C_SCALE2_BY_RATIO					0x00000001
#define C_SCALE2_FULL_SCREEN_BY_RATIO		0x00000002
#define C_SCALE2_FULL_SCREEN_BY_DIGI_ZOOM	0x00000003

/* fifo mode */
#define C_SCALE2_CTRL_FIFO_DISABLE			0x00000000

#define C_SCALE2_CTRL_IN_FIFO_16LINE		0x00001000
#define C_SCALE2_CTRL_IN_FIFO_32LINE		0x00002000
#define C_SCALE2_CTRL_IN_FIFO_64LINE		0x00003000
#define C_SCALE2_CTRL_IN_FIFO_128LINE		0x00004000
#define C_SCALE2_CTRL_IN_FIFO_256LINE		0x00005000

#define C_SCALE2_CTRL_OUT_FIFO_16LINE		0x00400000
#define C_SCALE2_CTRL_OUT_FIFO_32LINE		0x00800000
#define C_SCALE2_CTRL_OUT_FIFO_64LINE		0x00C00000

/* YUV type */
#define	C_SCALE2_CTRL_TYPE_YUV				0x00000400
#define	C_SCALE2_CTRL_TYPE_YCBCR			0x00000000

/* scale2 status */
#define C_SCALE2_STATUS_INPUT_EMPTY			0x00000001
#define C_SCALE2_STATUS_BUSY				0x00000002
#define C_SCALE2_STATUS_DONE				0x00000004
#define C_SCALE2_STATUS_STOP				0x00000008
#define C_SCALE2_STATUS_TIMEOUT				0x00000010
#define C_SCALE2_STATUS_INIT_ERR			0x00000020
#define C_SCALE2_STATUS_OUTPUT_FULL			0x00000040

/* scale2 error */
#define C_SCALE2_START_ERR					(-1)
#define C_SCALE2_INPUT_SIZE_ERR				(-2)
#define C_SCALE2_INPUT_OFFSET_ERR			(-3)
#define C_SCALE2_INPUT_BUF_ERR				(-4)
#define C_SCALE2_INPUT_FMT_ERR				(-5)
#define C_SCALE2_INPUT_FIFO_ERR				(-6)
#define C_SCALE2_OUTPUT_SIZE_ERR			(-7)
#define C_SCALE2_OUTPUT_OFFSET_ERR			(-8)
#define C_SCALE2_OUTPUT_BUF_ERR				(-9)
#define C_SCALE2_OUTPUT_FMT_ERR				(-10)
#define C_SCALE2_OUTPUT_FIFO_ERR			(-11)
#define C_SCALE2_EXT_BUF_ERR				(-12)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gpScale2Format_s
{
	/* img input para */
	unsigned int input_format;			/* input format*/
	unsigned short input_width;			/* 1~0x1FFF, input image x size */
	unsigned short input_height;		/* 1~0x1FFF, input image x size */
	unsigned short input_visible_width;	/* 0~0x1FFF, 0 is disable, clip x size*/
	unsigned short input_visible_height;/* 0~0x1FFF, 0 is disable, clip y size */
	unsigned short input_x_offset;		/* 0~0x1FFF, x start offset in effective area */
	unsigned short input_y_offset;		/* 0~0x1FFF, y start offset in effective area */
	
	unsigned int input_y_addr;			/* input y addr, must be 4-align */
	unsigned int input_u_addr;			/* input u addr, must be 4-align*/
	unsigned int input_v_addr;			/* input v addr, must be 4-align */
	
	/* img output para */
	unsigned int output_format;			/* output format*/
	unsigned short output_width;		/* 1~0x1FFF, must be 8-align, but YUV444/YUV422/YUV420 is 16-align, YUV411 is 32-align */
	unsigned short output_height;		/* 1~0x1FFF */
	unsigned short output_buf_width;	/* 1~0x1FFF, must be 8-align, but YUV444/YUV422/YUV420 is 16-align, YUV411 is 32-align */
	unsigned short output_buf_height;	/* 1~0x1FFF */
	unsigned short output_x_offset;		/* 0~0x1FFF, must be 8-align, skip x size after every line output */
	unsigned short reserved0;
	
	unsigned int output_y_addr;			/* output y addr, must be 4-align */
	unsigned int output_u_addr;			/* output u addr, must be 4-align */
	unsigned int output_v_addr;			/* output v addr, must be 4-align */
	
	/* scale para */
	unsigned int fifo_mode;				/* FIFO in or FIFO out mode */
	unsigned char scale_mode;			/* C_SCALE2_FULL_SCREEN / C_SCALE2_BY_RATIO.... */
	unsigned char digizoom_m;			/* digital zoom, ratio =  m/n */
	unsigned char digizoom_n;
	unsigned char reserved1;
}gpScale2Format_t;

typedef struct gpScale2Para_s
{
	unsigned char boundary_mode;		/* 0:Use boundary data, 1:Use register define data */ 
	unsigned char gamma_en;				/* 0:Disable, 1:Enable */
	unsigned char color_matrix_en;		/* 0:Disable, 1:Enable */
	unsigned char reserved0;

	unsigned int yuv_type;				/* C_SCALE2_CTRL_TYPE_YUV / C_SCALE2_CTRL_TYPE_YCBCR */ 
	unsigned int boundary_color;		/* format is YcbCr, 0x008080:Block */
	unsigned char gamma_table[256];
	unsigned short A11;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A12;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A13;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A21;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A22;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A23;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A31;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A32;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short A33;					/* 0x000:0, 0x080:0.5, 0x380:-0.5, 0x100:1,0, 0x300:-1.0 */
	unsigned short reserved1;
}gpScale2Para_t;
	
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/* Ioctl for device node definition */
#define SCALE2_IOCTL_ID         'S'
#define SCALE2_IOCTL_G_PARA		_IOR(SCALE2_IOCTL_ID, 0, gpScale2Para_t)
#define SCALE2_IOCTL_S_PARA		_IOW(SCALE2_IOCTL_ID, 1, gpScale2Para_t)
#define SCALE2_IOCTL_S_START	_IOW(SCALE2_IOCTL_ID, 2, gpScale2Format_t)
#define SCALE2_IOCTL_S_RESTART	_IOW(SCALE2_IOCTL_ID, 3, int)
#define SCALE2_IOCTL_S_PAUSE	_IOW(SCALE2_IOCTL_ID, 4, int)
#define SCALE2_IOCTL_S_RESUME	_IOW(SCALE2_IOCTL_ID, 5, int)
#define SCALE2_IOCTL_S_STOP		_IOW(SCALE2_IOCTL_ID, 6, int)

#endif /* _GP_SCALE2_H_ */
