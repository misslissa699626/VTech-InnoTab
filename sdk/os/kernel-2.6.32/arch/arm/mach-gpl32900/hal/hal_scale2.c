/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2011 by Generalplus Inc.                         *
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
#include <mach/kernel.h>
#include <mach/hal/hal_scale2.h>
#include <mach/hal/regmap/reg_scale2.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static scale2Reg_t *scale2Reg = (scale2Reg_t *)LOGI_ADDR_SCALER2_REG;
static scale2RegGamma_t *scale2Gamma = (scale2RegGamma_t *)LOGI_ADDR_SCALER2_REG;
static UINT32 register_backup1, register_backup2;

/**
 * @brief	scale2 init
 * @param 
 * @return 	none
 */
void
gpHalScale2SetInit(
	void
)
{
	scale2Reg->scale2Ctrl = C_SCALE2_CTRL_RESET;
	scale2Reg->scale2Ctrl = 0;
	scale2Reg->scale2IntFlag = C_SCALE2_INT_PEND | C_SCALE2_INT_OUT_FULL;
	scale2Reg->scale2PostPro = 0;

	gpHalScale2SetInputVisiblePixel(0, 0);
	gpHalScale2SetYuvType(C_SCALE2_CTRL_TYPE_YCBCR);
	gpHalScale2SetBoundaryMode(1);
	gpHalScale2SetBoundaryColor(0x008080);
	gpHalScale2SetInputOffset(0, 0);
	gpHalscale2SetOutputOffset(0);
	register_backup1 = 0;
	register_backup2 = 0;
}

/**
* @brief	scale2 image size set 
* @param 	input_x[in]: input x size
* @param 	input_y[in]: input y size
* @param 	output_x[in]: output x size
* @param 	output_y[in]: output y size
* @return 	SUCCESS/ERROR
*/
SINT32
gpHalScale2SetImgPixel(
	SINT16 input_x,
	SINT16 input_y,
	SINT16 output_x,
	SINT16 output_y
)
{
	UINT32 factor;
	SINT32 ret;

	ret = 0;
	/* Make sure output width is at least multiple of 8 pixels */
	if(output_x & 0x7)
	{		
		output_x &= ~0x7;
		ret = -1;
	}
	if(!input_x || !input_y || !output_x || !output_y)
	{
		input_x = 8;
		input_y = 8;
		output_x = 32;
		output_y = 8;
		ret = -1;
	}
	
	if(input_x > C_SCALE2_IN_WIDTH_MAX) 
	{
		input_x = C_SCALE2_IN_WIDTH_MAX;
		ret = -1;
	}

	if(input_y > C_SCALE2_IN_HEIGHT_MAX)
	{
		input_y = C_SCALE2_IN_HEIGHT_MAX;
		ret = -1;
	}
	
	if(output_x > C_SCALE2_OUT_WIDTH_MAX)
	{
		output_x = C_SCALE2_OUT_WIDTH_MAX;
		ret = -1;
	}

	if(output_y > C_SCALE2_OUT_HEIGHT_MAX)
	{
		output_y = C_SCALE2_OUT_HEIGHT_MAX;
		ret = -1;
	}

	/* Set scaler X factor */
	factor = (input_x << 16) / output_x;
	if(factor > C_SCALE2_X_FACTOR_MAX)
	{
		factor = C_SCALE2_X_FACTOR_MAX;
		ret = -1;
	}
	scale2Reg->scale2XFactor = factor;

	/* Set scaler Y factor */
	factor = (input_y << 16) / output_y;
	if(factor > C_SCALE2_Y_FACTOR_MAX)
	{
		factor = C_SCALE2_Y_FACTOR_MAX;
		ret = -1;
	}
	scale2Reg->scale2YFactor = factor;
	scale2Reg->scale2InWidth = input_x - 1;
	scale2Reg->scale2InHeight = input_y - 1;;
	scale2Reg->scale2InVisWidth = 0;
	scale2Reg->scale2InVisHeight = 0;
	scale2Reg->scale2OutWidth = output_x - 1;;
	scale2Reg->scale2OutHeight = output_y - 1;;
	return ret;
}

/**
* @brief	scale2 input size set 
* @param 	input_x[in]: input x size
* @param 	input_y[in]: input y size
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetInputPixel(
	UINT16 input_x, 
	UINT16 input_y
)
{
	SINT32 ret;

	ret = 0;
	if(!input_x || !input_y)
	{
		input_x = 8;
		input_y = 8;
		ret = -1;
	}
	
	if(input_x > C_SCALE2_IN_WIDTH_MAX)
	{
		input_x = C_SCALE2_IN_WIDTH_MAX;
		ret = -1;
	}

	if(input_y > C_SCALE2_IN_HEIGHT_MAX)
	{
		input_y = C_SCALE2_IN_HEIGHT_MAX;
		ret = -1;
	}
	scale2Reg->scale2InWidth = input_x - 1;
	scale2Reg->scale2InHeight = input_y - 1;;
	return ret;
}

/**
* @brief	scale2 input visible size set 
* @param 	input_x[in]: input x size
* @param 	input_y[in]: input y size
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetInputVisiblePixel(
	UINT16 input_x, 
	UINT16 input_y
)
{
	SINT32 ret;

	ret = 0;
	if(input_x > C_SCALE2_IN_VISIBLE_WIDTH_MAX)
	{
		input_x = C_SCALE2_IN_VISIBLE_WIDTH_MAX;
		ret = -1;
	}
	
	if(input_y > C_SCALE2_IN_VISIBLE_HEIGHT_MAX)
	{
		input_y = C_SCALE2_IN_VISIBLE_HEIGHT_MAX;
		ret = -1;
	}

	if(input_x)
	{
		scale2Reg->scale2InVisWidth = input_x -1;
		scale2Reg->scale2Ctrl |= C_SCALE2_CTRL_VISIBLE_RANGE; 
	} 
	else
	{
		scale2Reg->scale2InVisWidth = 0;
		scale2Reg->scale2Ctrl &= ~C_SCALE2_CTRL_VISIBLE_RANGE; 
	}
	
	if(input_y)
	{
		scale2Reg->scale2InVisHeight = input_y - 1;
	}
	else 
	{
		scale2Reg->scale2InVisHeight = 0;
		scale2Reg->scale2Ctrl &= ~C_SCALE2_CTRL_VISIBLE_RANGE; 
	}
	return ret;
}

/**
* @brief	scale2 input address set 
* @param 	y_addr[in]: input y address
* @param 	u_addr[in]: input u address
* @param 	v_addr[in]: input v address
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetInputAddr(
	UINT32 y_addr, 
	UINT32 u_addr, 
	UINT32 v_addr
)
{
	SINT32 ret;

	ret = 0;
	/* Make sure all addresses are 4-byte alignment */
	if((y_addr & 0x3) || (u_addr & 0x3) || (v_addr & 0x3))
	{
		y_addr &= ~0x3;
		u_addr &= ~0x3;
		v_addr &= ~0x3;
		ret = -1;
	}

	scale2Reg->scale2InYAddr = y_addr;
	scale2Reg->scale2InUAddr = u_addr;
	scale2Reg->scale2InVAddr = v_addr;
	return ret;
}

/**
* @brief	scale2 input format set 
* @param 	y_addr[in]: input y address
* @param 	u_addr[in]: input u address
* @param 	v_addr[in]: input v address
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetInputFormat(
	UINT32 format
)
{
	if ((format != C_SCALE2_CTRL_IN_RGB1555) &&
		(format != C_SCALE2_CTRL_IN_RGB565) &&
		(format != C_SCALE2_CTRL_IN_RGBG) &&
		(format != C_SCALE2_CTRL_IN_GRGB) &&
		(format != C_SCALE2_CTRL_IN_YUYV) &&
		(format != C_SCALE2_CTRL_IN_UYVY) &&
		(format != C_SCALE2_CTRL_IN_VYUY) &&
		(format != C_SCALE2_CTRL_IN_YUYV8X32) &&
		(format != C_SCALE2_CTRL_IN_YUYV8X64) &&
		(format != C_SCALE2_CTRL_IN_YUYV16X32) &&
		(format != C_SCALE2_CTRL_IN_YUYV16X64) &&
		(format != C_SCALE2_CTRL_IN_YUYV32X32) &&
		(format != C_SCALE2_CTRL_IN_YUYV64X64) &&
		(format != C_SCALE2_CTRL_IN_VYUY8X32) &&
		(format != C_SCALE2_CTRL_IN_VYUY8X64) &&
		(format != C_SCALE2_CTRL_IN_VYUY16X32) &&
		(format != C_SCALE2_CTRL_IN_VYUY16X64) &&
		(format != C_SCALE2_CTRL_IN_VYUY32X32) &&
		(format != C_SCALE2_CTRL_IN_VYUY64X64) &&
  		(format != C_SCALE2_CTRL_IN_YUV422) &&
		(format != C_SCALE2_CTRL_IN_YUV420) &&
		(format != C_SCALE2_CTRL_IN_YUV411) &&
		(format != C_SCALE2_CTRL_IN_YUV444) &&
		(format != C_SCALE2_CTRL_IN_Y_ONLY) &&
		(format != C_SCALE2_CTRL_IN_YUV422V) &&
		(format != C_SCALE2_CTRL_IN_YUV411V) &&
		(format != C_SCALE2_CTRL_IN_ARGB4444) &&
		(format != C_SCALE2_CTRL_IN_ARGB8888)) 
	{
		return -1;
	}
	
	scale2Reg->scale2Ctrl &= ~(C_SCALE2_CTRL_IN_MASK);
	scale2Reg->scale2Ctrl |= format;
	return 0;
}

/**
* @brief	scale2 input offset set 
* @param 	offset_x[in]: input x offset
* @param 	offset_y[in]: input y offset
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetInputOffset(
	UINT32 offset_x, 
	UINT32 offset_y
)
{
	if((offset_x>C_SCALE2_X_START_MAX) || (offset_y>C_SCALE2_Y_START_MAX))
	{
		return -1;
	}
	/* Set scaler start x and y position offset */
	scale2Reg->scale2XStart = offset_x;
	scale2Reg->scale2YStart = offset_y;
	return 0;
}

/**
* @brief	scale2 output size set 
* @param 	factor_x[in]: x factor
* @param 	factor_y[in]: y factor
* @param 	output_x[in]: output x size
* @param 	output_y[in]: output y size
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalscale2SetOutputPixel(
	UINT32 factor_x, 
	UINT32 factor_y, 
	UINT16 output_x, 
	UINT16 output_y
)
{
	SINT32 ret;

	ret = 0;
	/* Make sure output width is at least multiple of 8 pixels */
	if(output_x & 0x7) 
	{		
		output_x &= ~0x7;
		ret = -1;
	}
	
	if(!factor_x || !factor_y)
	{
		factor_x = 1;
		factor_y = 1;
		ret = -1;
	}
	
	if(!output_x || !output_y)
	{
		output_x = 32;
		output_y = 8;
		ret = -1;
	}
	
	if(factor_x > C_SCALE2_X_FACTOR_MAX)
	{
		factor_x = C_SCALE2_X_FACTOR_MAX;
		ret = -1;
	}
	
	if(factor_y > C_SCALE2_Y_FACTOR_MAX)
	{
		factor_y = C_SCALE2_Y_FACTOR_MAX;
		ret = -1;
	}
	
	if(output_x > C_SCALE2_OUT_WIDTH_MAX)
	{
		output_x = C_SCALE2_OUT_WIDTH_MAX;
		ret = -1;
	}

	if(output_y > C_SCALE2_OUT_HEIGHT_MAX)
	{
		output_y = C_SCALE2_OUT_HEIGHT_MAX;
		ret = -1;
	}

	/* Set scaler factor */
	scale2Reg->scale2XFactor = factor_x;
	scale2Reg->scale2YFactor = factor_y;
	scale2Reg->scale2OutWidth = output_x - 1;
	scale2Reg->scale2OutHeight = output_y - 1;
	return ret;
}

/**
* @brief	scale2 output offset set 
* @param 	x_offset[in]: output x offset
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalscale2SetOutputOffset(
	UINT16 x_out_offset
)
{
	SINT32 ret;

	ret = 0;
	/* Make sure output width is at least multiple of 8 pixels */
	if(x_out_offset & 0x7) 
	{		
		x_out_offset &= ~0x7;
		ret = -1;
	}
		
	if(x_out_offset > C_SCALE2_OUT_X_OFFSET_MAX)
	{
		x_out_offset = C_SCALE2_OUT_X_OFFSET_MAX;
		ret = -1;
	}
	
	/* Set scaler factor */
	scale2Reg->scale2OutOffset = x_out_offset;
	return ret;
}

/**
* @brief	scale2 output address set 
* @param 	y_addr[in]: x factor
* @param 	u_addr[in]: y factor
* @param 	v_addr[in]: output x offset
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetOutputAddr(
	UINT32 y_addr, 
	UINT32 u_addr, 
	UINT32 v_addr 
)
{
	SINT32 ret;

	ret = 0;
	/* Make sure all addresses are 4-byte alignment */
	if((y_addr & 0x3) || (u_addr & 0x3) || (v_addr & 0x3))
	{
		y_addr &= ~0x3;
		u_addr &= ~0x3;
		v_addr &= ~0x3;
		ret = -1;
	}
	scale2Reg->scale2OutYAddr = y_addr;
	scale2Reg->scale2OutUAddr = u_addr;
	scale2Reg->scale2OutVAddr = v_addr;
	return ret;
}

/**
* @brief	scale2 output format set 
* @param 	format[in]: format set
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetOutputFormat(
	UINT32 format
)
{
	if( (format != C_SCALE2_CTRL_OUT_RGB1555) &&
		(format != C_SCALE2_CTRL_OUT_RGB565) &&
		(format != C_SCALE2_CTRL_OUT_RGBG) &&
		(format != C_SCALE2_CTRL_OUT_GRGB) &&
		(format != C_SCALE2_CTRL_OUT_YUYV) &&
		(format != C_SCALE2_CTRL_OUT_UYVY) &&
		(format != C_SCALE2_CTRL_OUT_VYUY) &&
		(format != C_SCALE2_CTRL_OUT_YUYV8X32) &&
		(format != C_SCALE2_CTRL_OUT_YUYV8X64) &&
		(format != C_SCALE2_CTRL_OUT_YUYV16X32) &&
		(format != C_SCALE2_CTRL_OUT_YUYV16X64) &&
		(format != C_SCALE2_CTRL_OUT_YUYV32X32) &&
		(format != C_SCALE2_CTRL_OUT_YUYV64X64) &&
		(format != C_SCALE2_CTRL_OUT_VYUY8X32) &&
		(format != C_SCALE2_CTRL_OUT_VYUY8X64) &&
		(format != C_SCALE2_CTRL_OUT_VYUY16X32) &&
		(format != C_SCALE2_CTRL_OUT_VYUY16X64) &&
		(format != C_SCALE2_CTRL_OUT_VYUY32X32) &&
		(format != C_SCALE2_CTRL_OUT_VYUY64X64) &&
		(format != C_SCALE2_CTRL_OUT_YUV422) &&
		(format != C_SCALE2_CTRL_OUT_YUV420) &&
		(format != C_SCALE2_CTRL_OUT_YUV411) &&
		(format != C_SCALE2_CTRL_OUT_YUV444) &&
		(format != C_SCALE2_CTRL_OUT_Y_ONLY) &&
		(format != C_SCALE2_CTRL_OUT_ARGB4444) &&
		(format != C_SCALE2_CTRL_OUT_ARGB8888)) 
	{
		return -1;
	}

	scale2Reg->scale2Ctrl &= ~(C_SCALE2_CTRL_OUT_MASK);
	scale2Reg->scale2Ctrl |= format;
	return 0;
}

/**
* @brief	scale2 input fifo size set 
* @param 	mode[in]: input fifo size
* @return 	SUCCESS/ERROR
*/
SINT32
gpHalScale2SetInputFifoSize(
	UINT32 mode
)
{
	if( (mode != C_SCALE2_CTRL_IN_FIFO_DISABLE) &&
		(mode != C_SCALE2_CTRL_IN_FIFO_16LINE) &&
		(mode != C_SCALE2_CTRL_IN_FIFO_32LINE) &&
		(mode != C_SCALE2_CTRL_IN_FIFO_64LINE) &&
		(mode != C_SCALE2_CTRL_IN_FIFO_128LINE) &&
		(mode != C_SCALE2_CTRL_IN_FIFO_256LINE)) 
	{
		return -1;
	}

	scale2Reg->scale2Ctrl &= ~(C_SCALE2_CTRL_IN_FIFO_LINE_MASK);
	scale2Reg->scale2Ctrl |= mode;
	return 0;
}

/**
* @brief	scaler2 output fifo size set 
* @param 	mode[in]: output fifo size
* @return 	SUCCESS/ERROR
*/
SINT32
gpHalScale2SetOutputFifoSize(
	UINT32 mode
)
{
	if((mode != C_SCALE2_CTRL_OUT_FIFO_DISABLE) &&
		(mode != C_SCALE2_CTRL_OUT_FIFO_16LINE) &&
		(mode != C_SCALE2_CTRL_OUT_FIFO_32LINE) &&
		(mode != C_SCALE2_CTRL_OUT_FIFO_64LINE)) 
	{
		return -1;
	}

	scale2Reg->scale2Ctrl &= ~(C_SCALE2_CTRL_OUT_FIFO_LINE_MASK);
	scale2Reg->scale2Ctrl |= mode;
	return 0;
}

/**
* @brief	scale2 set yuv type  
* @param 	type[in]: 0:YCBCR, 1:yuv 
* @return 	None
*/
void
gpHalScale2SetYuvType(
	UINT32 type
)
{
	if(type == C_SCALE2_CTRL_TYPE_YUV)
		scale2Reg->scale2Ctrl |= C_SCALE2_CTRL_TYPE_YUV;
	else
		scale2Reg->scale2Ctrl &= ~(C_SCALE2_CTRL_TYPE_YUV);
}

/**
* @brief	scale2 set out of boundary mode  
* @param 	mode[in]: 0:use boundary data, 1:use register data 
* @return 	SUCCESS/ERROR
*/
void
gpHalScale2SetBoundaryMode(
	UINT32 mode
)
{
	if(mode)
		scale2Reg->scale2Ctrl |= C_SCALE2_CTRL_OUT_OF_BOUNDRY;
	else
		scale2Reg->scale2Ctrl &= ~C_SCALE2_CTRL_OUT_OF_BOUNDRY;
}

/**
* @brief	scale2 set out of boundary color  
* @param 	ob_color[in]: color 
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetBoundaryColor(
	UINT32 ob_color
)
{
	if(ob_color > C_SCALE2_OUT_BOUNDRY_COLOR_MAX)
		return -1;
	
	scale2Reg->scale2OBColor = ob_color;
	return 0;
}

/**
* @brief	scale2 set line buffer mode  
* @param 	mode[in]: line buffer mode 
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetLineBufferMode(
	UINT32 mode
)
{
	if(mode!=C_SCALE2_INTERNAL_LINE_BUFFER && 
		mode!=C_SCALE2_HYBRID_LINE_BUFFER && 
		mode!=C_SCALE2_EXTERNAL_LINE_BUFFER) 
	{
		return -1;
	} 
	else 
	{
		scale2Reg->scale2PostPro &= ~C_SCALE2_LINE_BUFFER_MODE_MASK;
		scale2Reg->scale2PostPro |= mode;
	}
	return 0;
}

/**
* @brief	scale2 set external line buffer  
* @param 	addr[in]: line buffer address 
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetExternalLineBuffer(
	UINT32 addr
)
{
	if(addr & 0x3)
		return -1;

	scale2Reg->scale2LBAddr = addr;
	return 0;
}

/**
* @brief	scale2 set start  
* @param 	
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2Start(
	void
)
{
	UINT32 output_x;
	UINT32 format;
	SINT32 ret;

 	ret = 0;
	if((scale2Reg->scale2InVisWidth > scale2Reg->scale2InWidth) || 
		(scale2Reg->scale2InVisHeight > scale2Reg->scale2InHeight)) 
	{
		scale2Reg->scale2InVisWidth = scale2Reg->scale2InWidth;
		scale2Reg->scale2InVisHeight = scale2Reg->scale2InHeight;
		ret = -1;
	}

	output_x = scale2Reg->scale2OutWidth + 1;
	format = scale2Reg->scale2Ctrl & C_SCALE2_CTRL_OUT_MASK;
	if((format == C_SCALE2_CTRL_OUT_YUYV8X32) || 
		(format == C_SCALE2_CTRL_OUT_YUYV8X64)) 
	{
		/* Must be less than 2047 pixels and be multiple of 8 pixels */
		if(output_x > 0x7FF) 
		{
			scale2Reg->scale2OutWidth = 2039;
			ret = -1;
		}
	} 
	else if((format == C_SCALE2_CTRL_OUT_YUYV16X32) || 
			(format == C_SCALE2_CTRL_OUT_YUYV16X64)) 
	{
		/* Must be less than 4095 pixels and be multiple of 8 pixels */
		if(output_x > 0xFFF)
		{
			scale2Reg->scale2OutWidth = 4087;
			ret = -1;
		}
	} 
	else if((format == C_SCALE2_CTRL_OUT_YUV422) || 
			(format == C_SCALE2_CTRL_OUT_YUV420) || 
			(format == C_SCALE2_CTRL_OUT_YUV444)) 
	{
		/* Must be multiple of 16 pixels */
		if(output_x & 0xF)
		{
			scale2Reg->scale2OutWidth = (output_x & ~0xF) - 1;
			ret = -1;
		}
	}
	else if (format == C_SCALE2_CTRL_OUT_YUV411) 
	{
		/* Must be multiple of 32 pixels */
		if(output_x & 0x1F)
		{
			scale2Reg->scale2OutWidth = (output_x & ~0x1F) - 1;
			ret = -1;
		}
	}

	if((scale2Reg->scale2Ctrl & C_SCALE2_CTRL_IN_FIFO_LINE_MASK) != C_SCALE2_CTRL_IN_FIFO_DISABLE) 
	{
		if ((scale2Reg->scale2Ctrl & C_SCALE2_CTRL_OUT_FIFO_LINE_MASK) != C_SCALE2_CTRL_OUT_FIFO_DISABLE) 
	  	{
			/* Input FIFO and output FIFO mode can not be enabled at the same time */
			ret = -1;
		}
 	}
 
	if ((scale2Reg->scale2Ctrl & C_SCALE2_CTRL_OUT_FIFO_LINE_MASK) != C_SCALE2_CTRL_OUT_FIFO_DISABLE) 
	{
		scale2Reg->scale2Ctrl |= C_SCALE2_CTRL_OUT_FIFO_INT;
	}

#if !POLLING_TEST
  	scale2Reg->scale2Ctrl |= (C_SCALE2_CTRL_START|C_SCALE2_CTRL_INT_ENABLE|C_SCALE2_CTRL_RGB1555_TRANSPARENT);
#else
	scale2Reg->scale2Ctrl |= (C_SCALE2_CTRL_START|C_SCALE2_CTRL_RGB1555_TRANSPARENT);
#endif
  	return ret;
}

/**
* @brief	scale2 set restart  
* @param 	
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2Restart(
	void
)
{
 	if (scale2Reg->scale2IntFlag & C_SCALE2_INT_DONE) 
	{		
		/* Already done */
		return -1;
	}

	/* Clear pending bit */
	scale2Reg->scale2IntFlag = C_SCALE2_INT_PEND | C_SCALE2_INT_OUT_FULL;	
	scale2Reg->scale2Ctrl |= C_SCALE2_CTRL_START;
  	return 0;
}

/**
* @brief	scale2 set pause  
* @param 	
* @return 	None
*/
void 
gpHalScale2Pause(
	void
)
{
	register_backup1 = scale2Reg->scale2ContiR1;	/* Backup the value of interal status register */
	register_backup2 = scale2Reg->scale2ContiR2;	/* Backup the value of Y-start register */

	/* Disable interrupt and reset scaler */
	scale2Reg->scale2Ctrl = C_SCALE2_CTRL_RESET;	
	/* Clear pending interrupt flags */
	scale2Reg->scale2IntFlag = C_SCALE2_INT_PEND | C_SCALE2_INT_OUT_FULL;
}

/**
* @brief	scale2 set resume  
* @param 	
* @return 	None
*/
void 
gpHalScale2Resume(
	void
)
{
	scale2Reg->scale2ContiW1 = register_backup1;
	scale2Reg->scale2YStart = register_backup2;

	scale2Reg->scale2Ctrl = C_SCALE2_CTRL_RESET | C_SCALE2_CTRL_CONTINUOUS_MODE;/* Reset scaler */
	scale2Reg->scale2Ctrl = 0;	/* Disable scaler */
	scale2Reg->scale2IntFlag = C_SCALE2_INT_PEND | C_SCALE2_INT_OUT_FULL;	/* Clear interrupt pending flag */
	scale2Reg->scale2PostPro = 0;	/* Disable color matrix and gamma function */

	gpHalScale2SetInputVisiblePixel(0, 0);
	gpHalScale2SetYuvType(C_SCALE2_CTRL_TYPE_YCBCR);
	
	gpHalScale2SetBoundaryMode(1);
	gpHalScale2SetBoundaryColor(0x008080);
	scale2Reg->scale2XStart = 0;
}

/**
* @brief	scale2 set stop  
* @param 	
* @return 	none
*/
void 
gpHalScale2Stop(
	void
)
{
	/* Disable interrupt and reset scaler */
	scale2Reg->scale2Ctrl = C_SCALE2_CTRL_RESET;
	/* Clear pending interrupt flags */
	scale2Reg->scale2IntFlag = C_SCALE2_INT_PEND | C_SCALE2_INT_OUT_FULL;
}

/**
* @brief	scale2 get interrupt status 
* @param 	
* @return 	STATUS
*/
SINT32 
gpHalScale2GetIntFlag(
	void
)
{
	SINT32 status;
	UINT32 ctrl, flag;

	status = 0;
	ctrl = scale2Reg->scale2Ctrl;
	flag = scale2Reg->scale2IntFlag;
	if((flag & C_SCALE2_INT_PEND) && (ctrl & C_SCALE2_CTRL_INT_ENABLE))
	{
		/* Scaler completed or input FIFO is empty */
		if(flag & C_SCALE2_INT_DONE)
			status = C_SCALE2_STATUS_DONE;
		else 
			status = C_SCALE2_STATUS_INPUT_EMPTY;
	}
	else if((flag & C_SCALE2_INT_OUT_FULL) && (ctrl & C_SCALE2_CTRL_OUT_FIFO_INT)) 
	{
		/* Output FIFO is full */
		status = C_SCALE2_STATUS_OUTPUT_FULL;
	}
	
	scale2Reg->scale2IntFlag = C_SCALE2_INT_PEND | C_SCALE2_INT_OUT_FULL;
	return status;
}

/**
* @brief	scale2 polling ststus 
* @param 	
* @return 	status
*/
SINT32 
gpHalScale2PollStatus(
	void
)
{
	SINT32 status;

	status = C_SCALE2_STATUS_STOP;
	
	if(scale2Reg->scale2Ctrl & C_SCALE2_CTRL_BUSY)
	{
		status = C_SCALE2_STATUS_BUSY;
	}
	
	if(scale2Reg->scale2IntFlag & C_SCALE2_INT_DONE)
	{
		status |= C_SCALE2_STATUS_DONE;
	} 
	else if(scale2Reg->scale2IntFlag & C_SCALE2_INT_PEND) 
	{
		status |= C_SCALE2_STATUS_INPUT_EMPTY;
	} 
	else if(scale2Reg->scale2IntFlag & C_SCALE2_INT_OUT_FULL) 
	{
		status |= C_SCALE2_STATUS_OUTPUT_FULL;
 	}
	
	return status;
}

/**
* @brief	scale2 enable/disable gamma function 
* @param 	gamma_switch[in]: 0:disable, 1: enable
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetGammaSwitch(
	UINT8 gamma_switch
)
{
    if(gamma_switch == 0)
    	scale2Reg->scale2PostPro &= ~C_SCALE2_Y_GAMMA_EN;
    else
    	scale2Reg->scale2PostPro |= C_SCALE2_Y_GAMMA_EN;
    
    return 0;
}

/**
* @brief	scale2 set gamma
* @param 	gamma_table_id[in]: id, 0 ~ 255
* @param 	gain_value[in]: gamma value
* @return 	SUCCESS/ERROR
*/
void 
gpHalScale2SetGamma(
	UINT8 gamma_table_id, 
	UINT8 gain_value
)
{
	scale2Gamma->offset[gamma_table_id] = gain_value;
}

/**
* @brief	scale2 enable/disable color matrix function 
* @param 	color_matrix_switch[in]: 0:disable, 1: enable
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetColorMatrixSwitch(
	UINT8 color_matrix_switch
)
{
    if(color_matrix_switch == 0)
    	scale2Reg->scale2PostPro &= ~C_SCALE2_COLOR_MATRIX_EN;
    else
    	scale2Reg->scale2PostPro |= C_SCALE2_COLOR_MATRIX_EN;
    
    return 0;
}

/**
* @brief	scale2 enable/disable color matrix function 
* @param 	index[in]: matrix row, 1 ~ 3
* @param 	A1[in]: parameter 1
* @param 	A2[in]: parameter 2
* @param 	A3[in]: parameter 3
* @return 	SUCCESS/ERROR
*/
SINT32 
gpHalScale2SetColorMatrix(
	UINT8 index, 
	UINT32 A1, 
	UINT32 A2,
	UINT32 A3
)
{
	if(index == 1)
	{
		scale2Reg->scale2A11 = A1;
		scale2Reg->scale2A12 = A2;
		scale2Reg->scale2A13 = A3;
	}
	else if(index == 2)
	{
		scale2Reg->scale2A21 = A1;
		scale2Reg->scale2A22 = A2;
		scale2Reg->scale2A23 = A3;
	}
	else if(index == 3)
	{
		scale2Reg->scale2A31 = A1;
		scale2Reg->scale2A32 = A2;
		scale2Reg->scale2A33 = A3;
	}
	else 
	{
		return -1;
	}
	return 0;
}

/**
 * @brief   Scale2 clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalScale2ClkEnable(
	UINT32 enable
)
{
	scuaReg_t *scuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

	if (enable){
		scuaReg->scuaPeriClkEn |= (1<<26);
	}
	else{
		scuaReg->scuaPeriClkEn &= ~(1<<26);
	}
}

/**
 * @brief   Scale2 module reset function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalScale2ModuleRest(
	UINT32 enable
)
{
	scuaReg_t *scuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

	if(enable)
	{
		scuaReg->scuaPeriRst |= (1<<26);
		scuaReg->scuaPeriRst &= ~(1<<26);
	}
}

