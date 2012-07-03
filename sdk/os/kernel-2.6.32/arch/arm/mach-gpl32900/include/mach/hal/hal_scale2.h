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
#ifndef _HAL_SCALE2_H_
#define _HAL_SCALE2_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
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

/* scale2 control */
#define	C_SCALE2_CTRL_START					0x00000100
#define	C_SCALE2_CTRL_BUSY					0x00000100
#define	C_SCALE2_CTRL_RESET					0x00000200
#define	C_SCALE2_CTRL_TYPE_YUV				0x00000400
#define	C_SCALE2_CTRL_TYPE_YCBCR			0x00000000
#define	C_SCALE2_CTRL_INT_ENABLE			0x00000800
#define C_SCALE2_CTRL_IN_FIFO_DISABLE		0x00000000
#define C_SCALE2_CTRL_IN_FIFO_16LINE		0x00001000
#define C_SCALE2_CTRL_IN_FIFO_32LINE		0x00002000
#define C_SCALE2_CTRL_IN_FIFO_64LINE		0x00003000
#define C_SCALE2_CTRL_IN_FIFO_128LINE		0x00004000
#define C_SCALE2_CTRL_IN_FIFO_256LINE		0x00005000
#define C_SCALE2_CTRL_IN_FIFO_LINE_MASK		0x00007000
#define C_SCALE2_CTRL_OUT_OF_BOUNDRY    	0x00008000
#define C_SCALE2_CTRL_VISIBLE_RANGE	    	0x00010000
#define C_SCALE2_CTRL_CONTINUOUS_MODE   	0x00020000
#define C_SCALE2_CTRL_OUT_FIFO_DISABLE		0x00000000
#define C_SCALE2_CTRL_OUT_FIFO_16LINE		0x00400000
#define C_SCALE2_CTRL_OUT_FIFO_32LINE		0x00800000
#define C_SCALE2_CTRL_OUT_FIFO_64LINE		0x00C00000
#define C_SCALE2_CTRL_OUT_FIFO_LINE_MASK	0x00C00000
#define	C_SCALE2_CTRL_OUT_FIFO_INT			0x01000000
#define	C_SCALE2_CTRL_RGB1555_TRANSPARENT	0x02000000

/* Out-of-boundry color register */
#define C_SCALE2_OUT_BOUNDRY_COLOR_MAX		0x00FFFFFF

/* Output width and height registers */
#define C_SCALE2_OUT_WIDTH_MAX				0x00001FFF		/* Maximum 8191 pixels */
#define C_SCALE2_OUT_HEIGHT_MAX				0x00001FFF		/* Maximum 8191 pixels */

/* Scaler factor registers */
#define C_SCALE2_X_FACTOR_MAX				0x00FFFFFF
#define C_SCALE2_Y_FACTOR_MAX				0x00FFFFFF

/* Scaler input x/y start offset registers */
#define C_SCALE2_X_START_MAX				0x3FFFFFFF
#define C_SCALE2_Y_START_MAX				0x3FFFFFFF

/* Scaler out x offset registers */
#define C_SCALE2_OUT_X_OFFSET_MAX			0x00001FFF

/* Input width and height registers */
#define C_SCALE2_IN_WIDTH_MAX				0x00001FFF		/* Maximum 8191 pixels */
#define C_SCALE2_IN_HEIGHT_MAX				0x00001FFF		/* Maximum 8191 pixels */

/* Input width and height registers */
#define C_SCALE2_IN_VISIBLE_WIDTH_MAX		0x00001FFF		/* Maximum 8191 pixels */
#define C_SCALE2_IN_VISIBLE_HEIGHT_MAX		0x00001FFF		/* Maximum 8191 pixels */

/* Interrupt flag register */
#define	C_SCALE2_INT_PEND					0x00000001
#define	C_SCALE2_INT_DONE					0x00000002
#define	C_SCALE2_INT_OUT_FULL				0x00000004

/* Post effect control register */
#define	C_SCALE2_HISTOGRAM_EN				0x00000001
#define	C_SCALE2_Y_GAMMA_EN					0x00000002
#define	C_SCALE2_COLOR_MATRIX_EN			0x00000004
#define	C_SCALE2_INTERNAL_LINE_BUFFER		0x00000000
#define	C_SCALE2_HYBRID_LINE_BUFFER			0x00000010
#define	C_SCALE2_EXTERNAL_LINE_BUFFER		0x00000020
#define	C_SCALE2_LINE_BUFFER_MODE_MASK		0x00000030

#define C_SCALE2_STATUS_INPUT_EMPTY			0x00000001
#define C_SCALE2_STATUS_BUSY				0x00000002
#define C_SCALE2_STATUS_DONE				0x00000004
#define C_SCALE2_STATUS_STOP				0x00000008
#define C_SCALE2_STATUS_TIMEOUT				0x00000010
#define C_SCALE2_STATUS_INIT_ERR			0x00000020
#define C_SCALE2_STATUS_OUTPUT_FULL			0x00000040

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define POLLING_TEST    0

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * @brief	scale2 init
 * @param 
 * @return 	none
 */
void gpHalScale2SetInit(void);

/**
* @brief	scale2 image size set 
* @param 	input_x[in]: input x size
* @param 	input_y[in]: input y size
* @param 	output_x[in]: output x size
* @param 	output_y[in]: output y size
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetImgPixel(SINT16 input_x,SINT16 input_y,SINT16 output_x,SINT16 output_y);

/**
* @brief	scale2 input size set 
* @param 	input_x[in]: input x size
* @param 	input_y[in]: input y size
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetInputPixel(UINT16 input_x, UINT16 input_y);

/**
* @brief	scale2 input visible size set 
* @param 	input_x[in]: input x size
* @param 	input_y[in]: input y size
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetInputVisiblePixel(UINT16 input_x, UINT16 input_y);

/**
* @brief	scale2 input address set 
* @param 	y_addr[in]: input y address
* @param 	u_addr[in]: input u address
* @param 	v_addr[in]: input v address
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetInputAddr(UINT32 y_addr, UINT32 u_addr, UINT32 v_addr);

/**
* @brief	scale2 input format set 
* @param 	y_addr[in]: input y address
* @param 	u_addr[in]: input u address
* @param 	v_addr[in]: input v address
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetInputFormat(UINT32 format);

/**
* @brief	scale2 input offset set 
* @param 	offset_x[in]: input x offset
* @param 	offset_y[in]: input y offset
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetInputOffset(UINT32 offset_x, UINT32 offset_y);

/**
* @brief	scale2 output size set 
* @param 	factor_x[in]: x factor
* @param 	factor_y[in]: y factor
* @param 	output_x[in]: output x offset
* @param 	output_y[in]: output y offset
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalscale2SetOutputPixel(UINT32 factor_x, UINT32 factor_y, UINT16 output_x, UINT16 output_y);

/**
* @brief	scale2 output offset set 
* @param 	x_offset[in]: output x offset
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalscale2SetOutputOffset(UINT16 x_out_offset);

/**
* @brief	scale2 output address set 
* @param 	y_addr[in]: x factor
* @param 	u_addr[in]: y factor
* @param 	v_addr[in]: output x offset
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetOutputAddr(UINT32 y_addr, UINT32 u_addr, UINT32 v_addr);

/**
* @brief	scale2 output format set 
* @param 	format[in]: format set
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetOutputFormat(UINT32 format);

/**
* @brief	scale2 input fifo size set 
* @param 	mode[in]: input fifo size
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetInputFifoSize(UINT32 mode);

/**
* @brief	scaler2 output fifo size set 
* @param 	mode[in]: output fifo size
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetOutputFifoSize(UINT32 mode);

/**
* @brief	scale2 set yuv type  
* @param 	type[in]: 0:YCBCR, 1:yuv 
* @return 	None
*/
void gpHalScale2SetYuvType(UINT32 type);

/**
* @brief	scale2 set out of boundary mode  
* @param 	mode[in]: 0:use boundary data, 1:use register data 
* @return 	SUCCESS/ERROR
*/
void gpHalScale2SetBoundaryMode(UINT32 mode);

/**
* @brief	scale2 set out of boundary color  
* @param 	ob_color[in]: color 
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetBoundaryColor(UINT32 ob_color);

/**
* @brief	scale2 set line buffer mode  
* @param 	mode[in]: line buffer mode 
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetLineBufferMode(UINT32 mode);

/**
* @brief	scale2 set external line buffer  
* @param 	addr[in]: line buffer address 
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetExternalLineBuffer(UINT32 addr);

/**
* @brief	scale2 set start  
* @param 	
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2Start(void);

/**
* @brief	scale2 set restart  
* @param 	
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2Restart(void);

/**
* @brief	scale2 set pause  
* @param 	
* @return 	None
*/
void gpHalScale2Pause(void);

/**
* @brief	scale2 set resume  
* @param 	
* @return 	None
*/
void gpHalScale2Resume(void);

/**
* @brief	scale2 set stop  
* @param 	
* @return 	none
*/
void gpHalScale2Stop(void);

/**
* @brief	scale2 get interrupt status 
* @param 	
* @return 	STATUS
*/
SINT32 gpHalScale2GetIntFlag(void);

/**
* @brief	scale2 polling ststus 
* @param 	
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2PollStatus(void);

/**
* @brief	scale2 enable/disable gamma function 
* @param 	gamma_switch[in]: 0:disable, 1: enable
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetGammaSwitch(UINT8 gamma_switch);

/**
* @brief	scale2 set gamma
* @param 	gamma_table_id[in]: id, 0 ~ 255
* @param 	gain_value[in]: gamma value
* @return 	SUCCESS/ERROR
*/
void gpHalScale2SetGamma(UINT8 gamma_table_id, UINT8 gain_value);

/**
* @brief	scale2 enable/disable color matrix function 
* @param 	color_matrix_switch[in]: 0:disable, 1: enable
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetColorMatrixSwitch(UINT8 color_matrix_switch);

/**
* @brief	scale2 enable/disable color matrix function 
* @param 	index[in]: matrix row, 1 ~ 3
* @param 	A1[in]: parameter 1
* @param 	A2[in]: parameter 2
* @param 	A3[in]: parameter 3
* @return 	SUCCESS/ERROR
*/
SINT32 gpHalScale2SetColorMatrix(UINT8 index, UINT32 A1, UINT32 A2, UINT32 A3);

/**
 * @brief   Scale2 clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalScale2ClkEnable(UINT32 enable);

/**
 * @brief   Scale2 module reset function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalScale2ModuleRest(UINT32 enable);

#endif /* _HAL_SCALE2_H_ */
