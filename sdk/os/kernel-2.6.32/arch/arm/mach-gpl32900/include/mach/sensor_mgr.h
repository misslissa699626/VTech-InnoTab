 /**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  3F, No.8, Dusing Rd., Science-Based Industrial Park,                  *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
#ifndef _GP_SENSOR_MGR_H
#define _GP_SENSOR_MGR_H

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
//sensor_timing_mode 
#define MODE_CCIR_601				0
#define MODE_CCIR_656				1
#define MODE_CCIR_HREF				2

//sensor_data_mode
#define MODE_DATA_RGB				0
#define MODE_DATA_YUV				1

//sensor_interlace_mode
#define MODE_NONE_INTERLACE			0
#define MODE_INTERLACE				1

//sensor_pclk_mode
#define MODE_POSITIVE_EDGE			0
#define MODE_NEGATIVE_EDGE			1

//sensor_hsync/vsync_mode
#define MODE_ACTIVE_LOW				0
#define MODE_ACTIVE_HIGH			1

//mclk source, mclk_src
#define MODE_MCLK_SRC_320M			0	/* src is 320M, can not output 24M */
#define MODE_MCLK_SRC_96M			1	/* src is 96M, can output 24M */


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct callbackfunc_s {
//	int (*suspend)(struct v4l2_subdev *sd);
//	int (*resume)(struct v4l2_subdev *sd);
	int (*powerctl)(int ctl);
	int (*standby)(int ctl);
	int (*reset)(int ctl);
	int (*set_port)(char *port);
}callbackfunc_t;

/** @brief A structure of sensor config */
typedef struct sensor_timing_s 
{
	unsigned char *desc;
	unsigned int pixelformat;
	unsigned int bpp;
	unsigned int mclk_src;
	unsigned int mclk;
	unsigned int hpixel;
	unsigned int hoffset;
	unsigned int vline;
	unsigned int voffset;
}sensor_fmt_t;

typedef struct mipi_config_s 
{
	/* mipi clock out set */
	unsigned int mipi_sep_clk_en;	/* 0:use mipi input clk, 1:use separate clk */ 
	unsigned int mipi_sep_clk;		/* separate clock speed */
	unsigned int mipi_sep_clk_src;	/* 0:CEVA pll, 1:USB pll*/
	unsigned int byte_clk_edge;		/* 0:posedge, 1:negedge */
	/* global configure */
	unsigned int low_power_en;		/* 0:disable, 1:enable */
	unsigned int lane_num;			/* 0:1 lane, 1:2 lane */
	/* ecc */
	unsigned int ecc_check_en;		/* 0~3 */
	unsigned int ecc_order;			/* 0:disable, 1:enable */
	unsigned int da_mask_cnt;		/* 0~0xFF, data mask count */
	unsigned int check_hs_seq;		/* 0:disable, 1:enable */
}mipi_config_t;

typedef struct sensor_config_s 
{
	unsigned int sensor_timing_mode;
	unsigned int sensor_data_mode;
	unsigned int sensor_interlace_mode;
	unsigned int sensor_pclk_mode;
	unsigned int sensor_hsync_mode;
	unsigned int sensor_vsync_mode;
	unsigned int sensor_fmt_num;
	sensor_fmt_t *fmt;
	mipi_config_t *mipi_config;
}sensor_config_t;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int32_t gp_get_sensorinfo( int sel, int* sdaddr, int* cbaddr, int* port, int* sensor );
int32_t register_sensor(struct v4l2_subdev *sd, int *port, sensor_config_t *config);
int32_t unregister_sensor(struct v4l2_subdev *sd);

#endif
