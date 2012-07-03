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
 
/**************************************************************************
 *                         H E A D E R   F I L E S						  *
 **************************************************************************/
#include <linux/module.h>
#include <linux/fs.h> /* everything... */
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <mach/module.h>
#include <mach/gp_csi.h>
#include <mach/gp_i2c_bus.h>
#include <mach/diag.h>
#include <mach/gp_csi.h>
#include <mach/sensor_mgr.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define CMATRIX_LEN		6
#define VGA_WIDTH		1600
#define VGA_HEIGHT		1200
#define QVGA_WIDTH		320
#define QVGA_HEIGHT		240

#define COM7_FMT_VGA	0x00

#define SENSOR_I2C_SLAVE_ADDR 0x78


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
static char *param[] = {"0", "PORT0", "0", "NONE", "0", "NONE"};
static int nstrs = 6;
module_param_array(param, charp, &nstrs, S_IRUGO);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/


/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int MT9D112_init(struct v4l2_subdev *sd,	u32 val);
static int MT9D112_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc);
static int MT9D112_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt);
static int MT9D112_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int MT9D112_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int MT9D112_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt);
static int MT9D112_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int MT9D112_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int MT9D112_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int MT9D112_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
static int MT9D112_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int MT9D112_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
static int MT9D112_cropcap(struct v4l2_subdev *sd, struct v4l2_cropcap *cc);
static int MT9D112_g_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int MT9D112_s_crop(struct v4l2_subdev *sd, struct v4l2_crop *crop);
static int MT9D112_s_interface(struct v4l2_subdev *sd, struct v4l2_interface *interface);
static int MT9D112_suspend(struct v4l2_subdev *sd);
static int MT9D112_resume(struct v4l2_subdev *sd);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

typedef struct{
	unsigned char reg_num_h;
	unsigned char reg_num_l;
	unsigned char value_h;
	unsigned char value_l;
}regval_list;

static regval_list MT9D112_default_regs[] = {
	{0x30,0x1A, 0x0A,0xCC},	 // RESET_REGISTER
	{0x32,0x02, 0x00,0x08},	 // STANDBY_CONTROL 	  
	{0x34,0x1E, 0x8F,0x09},	 // PLL_CLK_IN_CONTROL
	{0x34,0x1C, 0x02,0x18},	 // PLL_DIVIDERS1			  
	{0x34,0x1E, 0x8F,0x09},	 // PLL_CLK_IN_CONTROL
	{0x34,0x1E, 0x8F,0x08},	 // PLL_CLK_IN_CONTROL
	{0x30,0x44, 0x05,0x40},	 // DARK_CONTROL
	{0x32,0x16, 0x02,0xCF},	 // INTERNAL_CLOCK_CONTROL
	{0x32,0x1C, 0x04,0x02},	 // OF_CONTROL_STATUS
	{0x32,0x12, 0x00,0x01},	 // FACTORY_BYPASS
	{0x34,0x1E, 0x8F,0x09},	 // PLL_CLK_IN_CONTROL
	{0x34,0x1C, 0x01,0x20},	 // PLL_DIVIDERS1
	{0x34,0x1E, 0x8F,0x09},	 // PLL_CLK_IN_CONTROL
	{0x34,0x1E, 0x8F,0x08},	 // PLL_CLK_IN_CONTROL
	{0x30,0x44, 0x05,0x40},	 // DARK_CONTROL
	{0x32,0x16, 0x02,0xCF},	 // INTERNAL_CLOCK_CONTROL
	{0x32,0x1C, 0x04,0x02},	 // OF_CONTROL_STATUS
	{0x32,0x12, 0x00,0x01},	 // FACTORY_BYPASS//1??бд?ижии?3имDи░ид?ио????буж╠?07005
 
	 //[MT9D112 (SOC2020) Register Wizard Defaults
	 //PLL START
	{0x32,0x14, 0x06,0xe6},        //ZT want to 	PLL
	{0x34,0x1E, 0x8F,0x09},		//PLL/ Clk_in control: BYPASS PLL = 36617
	{0x34,0x1C, 0x02,0x4f},		   //PLL Control 1 = 288	  // Allow PLL to lock 250
	 
	{0x34,0x1E, 0x8F,0x09},		//PLL/ Clk_in control: PLL ON, bypassed = 36617
	{0x34,0x1E, 0x8F,0x08},		//PLL/ Clk_in control: USE PLL = 36616
	{0x30,0x44, 0x05,0x40},		//Reserved = 1344
	{0x32,0x16, 0x02,0xCF},		//Internal Clock Control = 719
	{0x32,0x1C, 0x04,0x02},		//OF Control Status = 1026
	{0x32,0x12, 0x00,0x01},		//Factory Bypass = 1
	//{0x30,0x40, 0x00,0x24},		//read_mode
	 
 // QVGA IS OK  ANYSIZE IS ok
	{0x33,0x8C, 0x27,0x03},		//Output Width (A)
	{0x33,0x90, 0x02,0x80},		//		= 320
	{0x33,0x8C, 0x27,0x05},		//Output Height (A)
	{0x33,0x90, 0x01,0xe0},		//		= 240
	{0x33,0x8C, 0x27,0x07},		//Output Width (B)
	{0x33,0x90, 0x06,0x40},		//		= 1600
	{0x33,0x8C, 0x27,0x09},		//Output Height (B)
	{0x33,0x90, 0x04,0xB0},		//		= 1200
	{0x33,0x8C, 0x27,0x0D},		//Row Start (A)
	{0x33,0x90, 0x00,0x00},		//		= 0
	{0x33,0x8C, 0x27,0x0F},		//Column Start (A)
	{0x33,0x90, 0x00,0x00},		//		= 0
	{0x33,0x8C, 0x27,0x11},		//Row End (A)
	{0x33,0x90, 0x04,0xBD},		//		= 1213
	{0x33,0x8C, 0x27,0x13},		//Column End (A)
	{0x33,0x90, 0x06,0x4D},		//		= 1613
	{0x33,0x8C, 0x27,0x15},		//Extra Delay (A) //0x
	{0x33,0x90, 0x01,0xA8},		//		= 0//0x0000
	{0x33,0x8C, 0x27,0x17},		//Row Speed (A)
	{0x33,0x90, 0x21,0x11},		//		= 8465
	{0x33,0x8C, 0x27,0x19},		//Read Mode (A)
	{0x33,0x90, 0x04,0x6C},		  //	  = 1132
	//0x33,0x90, 0x04,0x6d},		//		= 1132
	{0x33,0x8C, 0x27,0x1B},		//sensor_sample_time_pck (A)
	{0x33,0x90, 0x02,0x4F},		//		= 591
	{0x33,0x8C, 0x27,0x1D},		//sensor_fine_correction (A)
	{0x33,0x90, 0x01,0x02},		//		= 258
	{0x33,0x8C, 0x27,0x1F},		//sensor_fine_IT_min (A)
	{0x33,0x90, 0x02,0x79},		//		= 633
	{0x33,0x8C, 0x27,0x21},		//sensor_fine_IT_max_margin (A)
	{0x33,0x90, 0x01,0x55},		//		= 341
	{0x33,0x8C, 0x27,0x23},		//Frame Lines (A)
	{0x33,0x90, 0x03,0x41},		//		= 659 0x0293
	{0x33,0x8C, 0x27,0x25},		//Line Length (A)
	{0x33,0x90, 0x06,0x0F},		//		= 1551

	{0x33,0x8C, 0x27,0x27},		//sensor_dac_id_4_5 (A)
	{0x33,0x90, 0x20,0x20},		//		= 8224
	{0x33,0x8C, 0x27,0x29},		//sensor_dac_id_6_7 (A)
	{0x33,0x90, 0x20,0x20},		//		= 8224
	{0x33,0x8C, 0x27,0x2B},		//sensor_dac_id_8_9 (A)
	{0x33,0x90, 0x10,0x20},		//		= 4128
	{0x33,0x8C, 0x27,0x2D},		//sensor_dac_id_10_11 (A)
	{0x33,0x90, 0x20,0x07},		//		= 8199
	{0x33,0x8C, 0x27,0x2F},		//Row Start (B)
	{0x33,0x90, 0x00,0x04},		//		= 4
	{0x33,0x8C, 0x27,0x31},		//Column Start (B)
	{0x33,0x90, 0x00,0x04},		//		= 4
	{0x33,0x8C, 0x27,0x33},		//Row End (B)
	{0x33,0x90, 0x04,0xBB},		//		= 1211
	{0x33,0x8C, 0x27,0x35},		//Column End (B)
	{0x33,0x90, 0x06,0x4B},		//		= 1611
	{0x33,0x8C, 0x27,0x37},		//Extra Delay (B)
	{0x33,0x90, 0x02,0x2C},		//		= 556
	{0x33,0x8C, 0x27,0x39},		//Row Speed (B)
	{0x33,0x90, 0x21,0x11},		//		= 8465
	{0x33,0x8C, 0x27,0x3B},		//Read Mode (B)
	{0x33,0x90, 0x00,0x24},		  //	  = 36
	//0x33,0x90, 0x00,0x27},		//		= 36
	{0x33,0x8C, 0x27,0x3D},		//sensor_sample_time_pck (B)
	{0x33,0x90, 0x01,0x20},		//		= 288
	{0x33,0x8C, 0x27,0x3F},		//sensor_fine_correction (B)
	{0x33,0x90, 0x00,0xA4},		//		= 164
	{0x33,0x8C, 0x27,0x41},		//sensor_fine_IT_min (B)
	{0x33,0x90, 0x01,0x69},		//		= 361
	{0x33,0x8C, 0x27,0x43},		//sensor_fine_IT_max_margin (B)
	{0x33,0x90, 0x00,0xA4},		//		= 164
	{0x33,0x8C, 0x27,0x45},		//Frame Lines (B)
	{0x33,0x90, 0x06,0x25},		//		= 1573
	{0x33,0x8C, 0x27,0x47},		//Line Length (B)
	{0x33,0x90, 0x08,0x24},		//		= 2084

	//CH 2
	{0x33,0x8C, 0x27,0x51},		//Crop_X0 (A)
	{0x33,0x90, 0x00,0x00},		//		= 0
	{0x33,0x8C, 0x27,0x53},		//Crop_X1 (A)
	{0x33,0x90, 0x03,0x20},		//		= 800
	{0x33,0x8C, 0x27,0x55},		//Crop_Y0 (A)
	{0x33,0x90, 0x00,0x00},		//		= 0
	{0x33,0x8C, 0x27,0x57},		//Crop_Y1 (A)
	{0x33,0x90, 0x02,0x58},		//		= 600
	{0x33,0x8C, 0x27,0x5F},		//Crop_X0 (B)
	{0x33,0x90, 0x00,0x00},		//		= 0
	{0x33,0x8C, 0x27,0x61},		//Crop_X1 (B)
	{0x33,0x90, 0x06,0x40},		//		= 1600
	{0x33,0x8C, 0x27,0x63},		//Crop_Y0 (B)
	{0x33,0x90, 0x00,0x00},		//		= 0
	{0x33,0x8C, 0x27,0x65},		//Crop_Y1 (B)
	{0x33,0x90, 0x04,0xB0},		//		= 1200 




		//[CCM]													  
	{0x33,0x8C, 0x23,0x06},	 // MCU_ADDRESS [AWB_CCM_L_0]	   
	{0x33,0x90, 0x00,0x55},	 // MCU_DATA_0	   
	{0x33,0x7C, 0x00,0x04},	 //cgz oppo 20081021
	{0x33,0x8C, 0x23,0x08},	 // MCU_ADDRESS [AWB_CCM_L_1]	   
	{0x33,0x90, 0xFE,0xC3},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x0A},	 // MCU_ADDRESS [AWB_CCM_L_2]	   
	{0x33,0x90, 0x02,0x36},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x0C},	 // MCU_ADDRESS [AWB_CCM_L_3]	   
	{0x33,0x90, 0xFE,0x2A},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x0E},	 // MCU_ADDRESS [AWB_CCM_L_4]	   
	{0x33,0x90, 0x02,0x29},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x10},	 // MCU_ADDRESS [AWB_CCM_L_5]	   
	{0x33,0x90, 0x01,0x4A},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x12},	 // MCU_ADDRESS [AWB_CCM_L_6]	   
	{0x33,0x90, 0xFE,0xB0},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x14},	 // MCU_ADDRESS [AWB_CCM_L_7]	   
	{0x33,0x90, 0xF8,0x67},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x16},	 // MCU_ADDRESS [AWB_CCM_L_8]	   
	{0x33,0x90, 0x0A,0xCD},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x18},	 // MCU_ADDRESS [AWB_CCM_L_9]	   
	{0x33,0x90, 0x00,0x24},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x1A},	 // MCU_ADDRESS [AWB_CCM_L_10]	   
	{0x33,0x90, 0x00,0x3D},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x1C},	 // MCU_ADDRESS [AWB_CCM_RL_0]	   
	{0x33,0x90, 0x03,0x53},	 // MCU_DATA_0
	{0x33,0x8C, 0x23,0x1E},	 // MCU_ADDRESS [AWB_CCM_RL_1]	   
	{0x33,0x90, 0xFE,0xCA},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x20},	 // MCU_ADDRESS [AWB_CCM_RL_2]	   
	{0x33,0x90, 0xFD,0xE6},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x22},	 // MCU_ADDRESS [AWB_CCM_RL_3]	   
	{0x33,0x90, 0x01,0x1A},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x24},	 // MCU_ADDRESS [AWB_CCM_RL_4]	   
	{0x33,0x90, 0x01,0xB2},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x26},	 // MCU_ADDRESS [AWB_CCM_RL_5]	   
	{0x33,0x90, 0xFC,0xE2},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x28},	 // MCU_ADDRESS [AWB_CCM_RL_6]	   
	{0x33,0x90, 0x01,0x4A},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x2A},	 // MCU_ADDRESS [AWB_CCM_RL_7]	   
	{0x33,0x90, 0x06,0x0E},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x2C},	 // MCU_ADDRESS [AWB_CCM_RL_8]	   
	{0x33,0x90, 0xF8,0x1D},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x2E},	 // MCU_ADDRESS [AWB_CCM_RL_9]	   
	{0x33,0x90, 0x00,0x10},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x30},	 // MCU_ADDRESS [AWB_CCM_RL_10]    
	{0x33,0x90, 0xFF,0xEC},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x48},	 // MCU_ADDRESS [AWB_GAIN_BUFFER_SP
	{0x33,0x90, 0x00,0x08},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x49},	 // MCU_ADDRESS [AWB_JUMP_DIVISOR] 
	{0x33,0x90, 0x00,0x02},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x4A},	 // MCU_ADDRESS [AWB_GAIN_MIN]	   
	{0x33,0x90, 0x00,0x59},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x4B},	 // MCU_ADDRESS [AWB_GAIN_MAX]	   
	{0x33,0x90, 0x00,0xA6},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x4F},	 // MCU_ADDRESS [AWB_CCM_POSITION_M
	{0x33,0x90, 0x00,0x00},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x50},	 // MCU_ADDRESS [AWB_CCM_POSITION_M
	{0x33,0x90, 0x00,0x7F},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x52},	 // MCU_ADDRESS [AWB_SATURATION]   
	{0x33,0x90, 0x00,0x80},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x53},	 // MCU_ADDRESS [AWB_MODE]		   
	{0x33,0x90, 0x00,0x01},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x5B},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_O
	{0x33,0x90, 0x00,0x78},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x5C},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_O
	{0x33,0x90, 0x00,0x86},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x5D},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_I
	{0x33,0x90, 0x00,0x7E},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x5E},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_I
	{0x33,0x90, 0x00,0x82},	 // MCU_DATA_0					   
	{0x33,0x8C, 0x23,0x5F},	 // MCU_ADDRESS [AWB_CNT_PXL_TH]   
	{0x33,0x90, 0x00,0x40},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x61},	 // MCU_ADDRESS [AWB_TG_MIN0]	   
	{0x33,0x90, 0x00,0xD7},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x62},	 // MCU_ADDRESS [AWB_TG_MAX0]	   
	{0x33,0x90, 0x00,0xF6},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x02},	 // MCU_ADDRESS [AWB_WINDOW_POS]   
	{0x33,0x90, 0x00,0x00},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x03},	 // MCU_ADDRESS [AWB_WINDOW_SIZE]  
	{0x33,0x90, 0x00,0xEF},	 // MCU_DATA_0					   		
	{0x33,0x8C, 0xA3,0x64},  // MCU_ADDRESS [AWB_KR_L]		   
	{0x33,0x90, 0x00,0x98},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x65},	 // MCU_ADDRESS [AWB_KG_L]		   
	{0x33,0x90, 0x00,0x96},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x66},	 // MCU_ADDRESS [AWB_KB_L]		   
	{0x33,0x90, 0x00,0x84},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x67},	 // MCU_ADDRESS [AWB_KR_R]		   
	{0x33,0x90, 0x00,0x87},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x68},	 // MCU_ADDRESS [AWB_KG_R]		   
	{0x33,0x90, 0x00,0x80},	 // MCU_DATA_0					   
	{0x33,0x8C, 0xA3,0x69},	 // MCU_ADDRESS [AWB_KB_R]		   
	{0x33,0x90, 0x00,0x89},	 // MCU_DATA_0		 
	{0x33,0x8C, 0xA1,0x03},	 // MCU_ADDRESS [SEQ_CMD]		   
	{0x33,0x90, 0x00,0x06},	 // MCU_DATA_0	   
	{0x33,0x8C, 0xA1,0x03},	 // MCU_ADDRESS [SEQ_CMD]		   
	{0x33,0x90, 0x00,0x05},	 // MCU_DATA_0	 

	
	
	//[Fix Frame rate]
	{0x33,0x8C, 0xA1,0x23},	 // MCU_ADDRESS [SEQ_PREVIEW_0_FD]
	{0x33,0x90, 0x00,0x02},	 // MCU_DATA_0
	{0x33,0x8C, 0xA4,0x04},	 // MCU_ADDRESS [FD_MODE]
	{0x33,0x90, 0x00,0x42},	 // MCU_DATA_0
	{0x33,0x8C, 0xA4,0x13},	 // MCU_ADDRESS [FD_MODE]
	{0x33,0x90, 0x0a,0xba},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x30},	 // MCU_ADDRESS [SEQ_PREVIEW_2_AE]	  // add by sheree 1008
	{0x33,0x90, 0x00,0x04}, // MCU_DATA_0 
	{0x33,0x8C, 0xA1,0x03},  // MCU_ADDRESS
	{0x33,0x90, 0x00,0x05},  // MCU_DATA_0
                    
                     
 //[noise reduce setting]}
	{0x33,0x8C, 0xA1,0x15},	 // MCU_ADDRESS [SEQ_LLMODE]
	{0x33,0x90, 0x00,0xED},	 // MCU_DATA_0                                //EF
	{0x33,0x8C, 0xA1,0x18},	 // MCU_ADDRESS [SEQ_LLSAT1]
	{0x33,0x90, 0x00,0x36},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x19},	 // MCU_ADDRESS [SEQ_LLSAT2]
	{0x33,0x90, 0x00,0x03},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x1A},	 // MCU_ADDRESS [SEQ_LLINTERPTHRESH1]
	{0x33,0x90, 0x00,0x0A},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x1B},	 // MCU_ADDRESS [SEQ_LLINTERPTHRESH2]
	{0x33,0x90, 0x00,0x20},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x1C},	 // MCU_ADDRESS [SEQ_LLAPCORR1]
	{0x33,0x90, 0x00,0x02},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x1D},	 // MCU_ADDRESS [SEQ_LLAPCORR2]
	{0x33,0x90, 0x00,0x00},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x1E},	 // MCU_ADDRESS [SEQ_LLAPTHRESH1]
	{0x33,0x90, 0x00,0x00},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x1F},	 // MCU_ADDRESS [SEQ_LLAPTHRESH2]
	{0x33,0x90, 0x00,0x04},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x3E},	 // MCU_ADDRESS [SEQ_NR_TH1_R]
	{0x33,0x90, 0x00,0x04},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x3F},	 // MCU_ADDRESS [SEQ_NR_TH1_G]
	{0x33,0x90, 0x00,0x0E},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x40},	 // MCU_ADDRESS [SEQ_NR_TH1_B]
	{0x33,0x90, 0x00,0x04},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x41},	 // MCU_ADDRESS [SEQ_NR_TH1_OL]
	{0x33,0x90, 0x00,0x04},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x42},	 // MCU_ADDRESS [SEQ_NR_TH2_R]
	{0x33,0x90, 0x00,0x32},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x43},	 // MCU_ADDRESS [SEQ_NR_TH2_G]
	{0x33,0x90, 0x00,0x0F},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x44},	 // MCU_ADDRESS [SEQ_NR_TH2_B]
	{0x33,0x90, 0x00,0x32},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x45},	 // MCU_ADDRESS [SEQ_NR_TH2_OL]
	{0x33,0x90, 0x00,0x32},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x46},	 // MCU_ADDRESS [SEQ_NR_GAINTH1]
	{0x33,0x90, 0x00,0x05},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x47},	 // MCU_ADDRESS [SEQ_NR_GAINTH2]
	{0x33,0x90, 0x00,0x3A},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x4F},	 // MCU_ADDRESS [SEQ_CLUSTERDC_TH]
	{0x33,0x90, 0x00,0x0D},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x03},		//Refresh Sequencer Mode
	{0x33,0x90, 0x00,0x06},	  //	  = 6
	{0x33,0x8C, 0xA1,0x03},
	{0x33,0x90, 0x00,0x05},
 // test-5_max index change_target change]
	{0x33,0x8C, 0xA1,0x18},	 // MCU_ADDRESS [SEQ_LLSAT1]
	{0x33,0x90, 0x00,0x20},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x19},	 // MCU_ADDRESS [SEQ_LLSAT2]
	{0x33,0x90, 0x00,0x03},	 // MCU_DATA_0
	{0x33,0x8C, 0xA2,0x06},	 // MCU_ADDRESS [AE_TARGET]
	{0x33,0x90, 0x00,0x28},//0x0031,	 // MCU_DATA_0         AE TATGET
	{0x33,0x8C, 0xA2,0x07},	 // MCU_ADDRESS [AE_GATE]
	{0x33,0x90, 0x00,0x0B},	 // MCU_DATA_0
	{0x33,0x8C, 0xA2,0x0C},	 // MCU_ADDRESS [AE_MAX_INDEX]
	{0x33,0x90, 0x00,0x03},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x09},	 // MCU_ADDRESS [SEQ_AE_FASTBUFF]
	{0x33,0x90, 0x00,0x20},	 //0x0024 cgz oppo 2008-1020// MCU_DATA_0
                          
                          
/*                        
	{0x33,0x8C, 0xA7,0x6D},	 // MCU_ADDRESS [MODE_GAM_CONT_A]
	{0x33,0x90, 0x00,0x03},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x6F},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_0]
	{0x33,0x90, 0x00,0x00},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x70},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_1]
	{0x33,0x90, 0x00,0x07},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x71},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_2]
	{0x33,0x90, 0x00,0x17},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x72},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_3]
	{0x33,0x90, 0x00,0x3B},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x73},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_4]
	{0x33,0x90, 0x00,0x60},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x74},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_5]
	{0x33,0x90, 0x00,0x7A},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x75},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_6]
	{0x33,0x90, 0x00,0x8F},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x76},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_7]
	{0x33,0x90, 0x00,0xA0},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x77},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_8]
	{0x33,0x90, 0x00,0xAE},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x78},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_9]
	{0x33,0x90, 0x00,0xBA},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x79},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_10]
	{0x33,0x90, 0x00,0xC5},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x7A},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_11]
	{0x33,0x90, 0x00,0xCE},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x7B},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_12]
	{0x33,0x90, 0x00,0xD7},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x7C},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_13]
	{0x33,0x90, 0x00,0xDF},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x7D},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_14]
	{0x33,0x90, 0x00,0xE6},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x7E},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_15]
	{0x33,0x90, 0x00,0xED},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x7F},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_16]
	{0x33,0x90, 0x00,0xF3},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x80},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_17]
	{0x33,0x90, 0x00,0xF9},	 // MCU_DATA_0
	{0x33,0x8C, 0xA7,0x81},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_18]
	{0x33,0x90, 0x00,0xFF},	 // MCU_DATA_0
	{0x33,0x8C, 0xA1,0x03},	 // MCU_ADDRESS [SEQ_CMD]
	{0x33,0x90, 0x00,0x06},	 // MCU_DATA_0
	*/                    
                          
	{0x33,0x8C, 0xA1,0x02},	 // MCU_ADDRESS [SEQ_CMD]
	{0x33,0x90, 0x00,0x0F},	 // MCU_DATA_0	  
	{0x33,0x8C, 0xA1,0x03},	 // MCU_ADDRESS [SEQ_CMD]
	{0x33,0x90, 0x00,0x05},	 // MCU_DATA_0

//lens [Register Log 09/03/09 15:10:52]
	{0x32,0x10, 0x01,0xF8}, 	// COLOR_PIPELINE_CONTROL [1]
	{0x34,0xCE, 0x01,0xE8}, 	// LENS_CORRECTION_CONTROL [1]
	{0x34,0xD0, 0x61,0x31}, 	// ZONE_BOUNDS_X1_X2 [1]
	{0x34,0xD2, 0x34,0x92}, 	// ZONE_BOUNDS_X0_X3 [1]
	{0x34,0xD4, 0x9B,0x68}, 	// ZONE_BOUNDS_X4_X5 [1]
	{0x34,0xD6, 0x4B,0x25}, 	// ZONE_BOUNDS_Y1_Y2 [1]
	{0x34,0xD8, 0x26,0x70}, 	// ZONE_BOUNDS_Y0_Y3 [1]
	{0x34,0xDA, 0x72,0x4C}, 	// ZONE_BOUNDS_Y4_Y5 [1]
	{0x34,0xDC, 0xFF,0xFA}, 	// CENTER_OFFSET [1]
	{0x34,0xDE, 0x00,0xB4}, 	// FX_RED [1]
	{0x34,0xE0, 0x00,0x7E}, 	// FX_GREEN [1]
	{0x34,0xE2, 0x00,0xA0}, 	// FX_GREEN2 [1]
	{0x34,0xE4, 0x00,0x7C}, 	// FX_BLUE [1]
	{0x34,0xE6, 0x00,0xB4}, 	// FY_RED [1]
	{0x34,0xE8, 0x00,0x7E}, 	// FY_GREEN [1]
	{0x34,0xEA, 0x00,0xA0}, 	// FY_GREEN2 [1]
	{0x34,0xEC, 0x00,0x7C}, 	// FY_BLUE [1]
	{0x34,0xEE, 0x0A,0x97}, 	// DF_DX_RED [1]
	{0x34,0xF0, 0x0C,0xB0}, 	// DF_DX_GREEN [1]
	{0x34,0xF2, 0x0A,0x7D}, 	// DF_DX_GREEN2 [1]
	{0x34,0xF4, 0x0C,0xA7}, 	// DF_DX_BLUE [1]
	{0x34,0xF6, 0x0B,0xE9}, 	// DF_DY_RED [1]
	{0x34,0xF8, 0x0B,0x43}, 	// DF_DY_GREEN [1]
	{0x34,0xFA, 0x09,0xF0}, 	// DF_DY_GREEN2 [1]
	{0x34,0xFC, 0x0A,0x1C}, 	// DF_DY_BLUE [1]
	{0x35,0x00, 0xDF,0x4C}, 	// SECOND_DERIV_ZONE_0_RED [1]
	{0x35,0x02, 0x2A,0x34}, 	// SECOND_DERIV_ZONE_0_GREEN [1]
	{0x35,0x04, 0x45,0x51}, 	// SECOND_DERIV_ZONE_0_GREEN2 [1]
	{0x35,0x06, 0x3D,0x37}, 	// SECOND_DERIV_ZONE_0_BLUE [1]
	{0x35,0x08, 0x2D,0xF9}, 	// SECOND_DERIV_ZONE_1_RED [1]
	{0x35,0x0A, 0x1C,0xE9}, 	// SECOND_DERIV_ZONE_1_GREEN [1]
	{0x35,0x0C, 0x31,0x0B}, 	// SECOND_DERIV_ZONE_1_GREEN2 [1]
	{0x35,0x0E, 0x30,0xEA}, 	// SECOND_DERIV_ZONE_1_BLUE [1]
	{0x35,0x10, 0x20,0x2C}, 	// SECOND_DERIV_ZONE_2_RED [1]
	{0x35,0x12, 0x0C,0x14}, 	// SECOND_DERIV_ZONE_2_GREEN [1]
	{0x35,0x14, 0x18,0x24}, 	// SECOND_DERIV_ZONE_2_GREEN2 [1]
	{0x35,0x16, 0x1D,0x1A}, 	// SECOND_DERIV_ZONE_2_BLUE [1]
	{0x35,0x18, 0x24,0x38}, 	// SECOND_DERIV_ZONE_3_RED [1]
	{0x35,0x1A, 0x18,0x1F}, 	// SECOND_DERIV_ZONE_3_GREEN [1]
	{0x35,0x1C, 0x26,0x2B}, 	// SECOND_DERIV_ZONE_3_GREEN2 [1]
	{0x35,0x1E, 0x1E,0x16}, 	// SECOND_DERIV_ZONE_3_BLUE [1]
	{0x35,0x20, 0x30,0x31}, 	// SECOND_DERIV_ZONE_4_RED [1]
	{0x35,0x22, 0x27,0x21}, 	// SECOND_DERIV_ZONE_4_GREEN [1]
	{0x35,0x24, 0x1C,0x17}, 	// SECOND_DERIV_ZONE_4_GREEN2 [1]
	{0x35,0x26, 0x1D,0x20}, 	// SECOND_DERIV_ZONE_4_BLUE [1]
	{0x35,0x28, 0x20,0x2B}, 	// SECOND_DERIV_ZONE_5_RED [1]
	{0x35,0x2A, 0x0A,0x11}, 	// SECOND_DERIV_ZONE_5_GREEN [1]
	{0x35,0x2C, 0xEF,0x0E}, 	// SECOND_DERIV_ZONE_5_GREEN2 [1]
	{0x35,0x2E, 0xF0,0x0E}, 	// SECOND_DERIV_ZONE_5_BLUE [1]
	{0x35,0x30, 0x1B,0xD2}, 	// SECOND_DERIV_ZONE_6_RED [1]
	{0x35,0x32, 0x10,0x0B}, 	// SECOND_DERIV_ZONE_6_GREEN [1]
	{0x35,0x34, 0xF8,0xD4}, 	// SECOND_DERIV_ZONE_6_GREEN2 [1]
	{0x35,0x36, 0x06,0x0B}, 	// SECOND_DERIV_ZONE_6_BLUE [1]
	{0x35,0x38, 0x13,0xD1}, 	// SECOND_DERIV_ZONE_7_RED [1]
	{0x35,0x3A, 0x13,0x4A}, 	// SECOND_DERIV_ZONE_7_GREEN [1]
	{0x35,0x3C, 0xE3,0x0A}, 	// SECOND_DERIV_ZONE_7_GREEN2 [1]
	{0x35,0x3E, 0xDD,0x34}, 	// SECOND_DERIV_ZONE_7_BLUE [1]
	{0x35,0x40, 0x00,0x01}, 	// X2_FACTORS [1]
	{0x35,0x42, 0x00,0x07}, 	// GLOBAL_OFFSET_FXY_FUNCTION [24]
	{0x35,0x44, 0x07,0xFF}, 	// K_FACTOR_IN_K_FX_FY_R_TR [1]
	{0x35,0x46, 0x02,0x97}, 	// K_FACTOR_IN_K_FX_FY_G1_TR [1]
	{0x35,0x48, 0x00,0x00}, 	// K_FACTOR_IN_K_FX_FY_G2_TR [1]
	{0x35,0x4A, 0x03,0xFF}, 	// K_FACTOR_IN_K_FX_FY_B_TR [1]
	{0x35,0x4C, 0x06,0x00}, 	// K_FACTOR_IN_K_FX_FY_R_TL [1]
	{0x35,0x4E, 0x06,0x25}, 	// K_FACTOR_IN_K_FX_FY_G1_TL [1]
	{0x35,0x50, 0x03,0xFF}, 	// K_FACTOR_IN_K_FX_FY_G2_TL [1]
	{0x35,0x52, 0x04,0x39}, 	// K_FACTOR_IN_K_FX_FY_B_TL [1]
	{0x35,0x54, 0x06,0xd0}, 	// K_FACTOR_IN_K_FX_FY_R_BR [22]
	{0x35,0x56, 0x03,0xFF}, 	// K_FACTOR_IN_K_FX_FY_G1_BR [1]
	{0x35,0x58, 0x07,0xD9}, 	// K_FACTOR_IN_K_FX_FY_G2_BR [1]
	{0x35,0x5A, 0x00,0x00}, 	// K_FACTOR_IN_K_FX_FY_B_BR [1]
	{0x35,0x5C, 0x00,0x00}, 	// K_FACTOR_IN_K_FX_FY_R_BL [59]
	{0x35,0x5E, 0x00,0x00}, 	// K_FACTOR_IN_K_FX_FY_G1_BL [1]
	{0x35,0x60, 0x00,0x00}, 	// K_FACTOR_IN_K_FX_FY_G2_BL [1]
	{0x35,0x62, 0x00,0x00}, 	// K_FACTOR_IN_K_FX_FY_B_BL [1];
                        
	{0x35,0xa4, 0x04,0xc5},   //saturation
#if 1          //MT9D112_full_regs
	{0x30,0x1a, 0x0a,0xcc},
	{0x32,0x02, 0x00,0x08},		                
	{0x33,0x8c, 0x27,0x07},
	{0x33,0x90, 0x06,0x40},
	{0x33,0x8c, 0x27,0x09},
	{0x33,0x90, 0x04,0xB0},
	{0x33,0x8c, 0x27,0x5f},
	{0x33,0x90, 0x00,0x00},
	{0x33,0x8c, 0x27,0x61},
	{0x33,0x90, 0x06,0x40},
	{0x33,0x8c, 0x27,0x63},
	{0x33,0x90, 0x00,0x00},
	{0x33,0x8c, 0x27,0x65},
	{0x33,0x90, 0x04,0xb0},
		                
	{0x33,0x8c, 0xa1,0x20},
	{0x33,0x90, 0x00,0x72},

	{0x33,0x8c, 0xa1,0x03},
	{0x33,0x90, 0x00,0x02},

	{0x33,0x8C, 0xA1,0x02},	
	{0x33,0x90, 0x00,0x0f},//DISABLE AE
#endif		
	{0xff,0xff, 0xff,0xff},   //end
};                          
                            
static regval_list MT9D112_fmt_yuv422[] = {
	{0xff, 0xff, 0xff, 0xff}	//end
};

static regval_list MT9D112_fmt_rgb565[] = {
	{0xff, 0xff, 0xff, 0xff}	//end
};

static regval_list MT9D112_resume_regs[] = {
	{0xff, 0xff, 0xff, 0xff}	//end
};

static regval_list MT9D112_suspend_regs[] = {
	{0xff, 0xff, 0xff, 0xff}	//end
};

static const struct v4l2_subdev_core_ops MT9D112_core_ops = {
//	.g_chip_ident = MT9D112_g_chip_ident,
	.g_ctrl = MT9D112_g_ctrl,
	.s_ctrl = MT9D112_s_ctrl,
	.queryctrl = MT9D112_queryctrl,
//	.reset = MT9D112_reset,
	.init = MT9D112_init,
};

static const struct v4l2_subdev_video_ops MT9D112_video_ops = {
	.enum_fmt = MT9D112_enum_fmt,
	.try_fmt = MT9D112_try_fmt,
	.s_fmt = MT9D112_s_fmt,
	.g_fmt = MT9D112_g_fmt,
	.s_parm = MT9D112_s_parm,
	.g_parm = MT9D112_g_parm,
	.cropcap = MT9D112_cropcap,
	.g_crop = MT9D112_g_crop,
	.s_crop = MT9D112_s_crop,	
};

static const struct v4l2_subdev_ext_ops MT9D112_ext_ops = {
	.s_interface = MT9D112_s_interface,
	.suspend = MT9D112_suspend,
	.resume = MT9D112_resume,
};

static const struct v4l2_subdev_ops MT9D112_ops = {
	.core = &MT9D112_core_ops,
	.video = &MT9D112_video_ops,
	.ext = &MT9D112_ext_ops
};

static struct MT9D112_format_struct {
	__u8 *desc;
	__u32 pixelformat;
	regval_list *regs;
	int cmatrix[CMATRIX_LEN];
	int bpp;   /* Bytes per pixel */
} MT9D112_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat= V4L2_PIX_FMT_YUYV,
		.regs		= MT9D112_fmt_yuv422,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
	{
		.desc		= "RGB 5-6-5",
		.pixelformat= V4L2_PIX_FMT_RGB565,
		.regs		= MT9D112_fmt_rgb565,
		.cmatrix	= { 128, -128, 0, -34, -94, 128 },
		.bpp		= 2,
	},
};
#define N_MT9D112_FMTS ARRAY_SIZE(MT9D112_formats)

static struct MT9D112_win_size {
	int	width;
	int	height;
	unsigned char com7_bit;
	int	hstart;		/* Start/stop values for the camera.  Note */
	int	hstop;		/* that they do not always make complete */
	int	vstart;		/* sense to humans, but evidently the sensor */
	int	vstop;		/* will do the right thing... */
	regval_list *regs; /* Regs to tweak */
/* h/vref stuff */
} MT9D112_win_sizes[] = {
	/* VGA */
	{
		.width		= VGA_WIDTH,
		.height		= VGA_HEIGHT,
		.com7_bit	= COM7_FMT_VGA,
		.hstart		= 158,		/* These values from */
		.hstop		=  14,		/* Omnivision */
		.vstart		=  10,
		.vstop		= 490,
		.regs 		= NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(MT9D112_win_sizes))

struct MT9D112_format_struct;  /* coming later */
typedef struct MT9D112_info_t {
	struct v4l2_subdev sd;
	struct MT9D112_format_struct *fmt;  /* Current format */
	unsigned char sat;		/* Saturation value */
	int hue;			/* Hue value */
}MT9D112_info_s;

static MT9D112_info_s MT9D112_info;
struct i2c_bus_attr_t *i2c_attr;


/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/

static int MT9D112_read(
	unsigned char reg,
	unsigned char *value
)
{
	char data[2];
	int ret;
	
	data[0] = reg;	
	ret = gp_i2c_bus_read((int)i2c_attr, data, 2);
	*value = data[1];
	
	return ret;
}

static int MT9D112_write(unsigned char *pvals)
{	
	return gp_i2c_bus_write((int)i2c_attr, pvals, sizeof(regval_list));
}

static int MT9D112_write_array(regval_list *vals)
{
	int ret;
	int i,k;
	unsigned char *pvals;

	pvals = (unsigned char*) vals;

	k = sizeof(regval_list);

	while (1) {
		for(i=0;i<k;i++)
		{
			if(*(pvals+i) != 0xff)
			{
				goto snedi2cdata;
			}
		}
		break;//all vals is 0xff,reach end, then break
snedi2cdata:
		//printk("reg=0x%02x, value=0x%02x\n", *pvals,*(pvals+1));
		ret = MT9D112_write(pvals);
		if (ret < 0)
			return ret;
		vals++;
		pvals = (unsigned char*) vals;//get next vals
	}
	return 0;
 }

static int MT9D112_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int MT9D112_enum_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_fmtdesc *fmt
)
{
	struct MT9D112_format_struct *ofmt;

	if (fmt->index >= N_MT9D112_FMTS)
		return -EINVAL;

	ofmt = MT9D112_formats + fmt->index;
	fmt->flags = 0;
	strcpy(fmt->description, ofmt->desc);
	fmt->pixelformat = ofmt->pixelformat;
	
	return 0;
}

static int MT9D112_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int index;
	struct MT9D112_win_size *wsize;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;
	
	for( index=0; index<N_MT9D112_FMTS; index++ )
		if (MT9D112_formats[index].pixelformat == pix->pixelformat)
			break;
	if (index >= N_MT9D112_FMTS) {
		/* default to first format */
		index = 0;
		pix->pixelformat = MT9D112_formats[0].pixelformat;
		printk(KERN_NOTICE "No match format\n");
	}
	
	pix->field = V4L2_FIELD_NONE;
	
	for (wsize = MT9D112_win_sizes; wsize < MT9D112_win_sizes + N_WIN_SIZES; wsize++)
		if (pix->width >= wsize->width && pix->height >= wsize->height)
			break;
	if (wsize >= MT9D112_win_sizes + N_WIN_SIZES)
		wsize--;   /* Take the smallest one */

	/*
	 * Note the size we'll actually handle.
	 */
	pix->width = wsize->width;
	pix->height = wsize->height;
	pix->bytesperline = pix->width*MT9D112_formats[index].bpp;
	pix->sizeimage = pix->height*pix->bytesperline;
	return 0;
}

static int MT9D112_s_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int MT9D112_g_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	return 0;
}

static int MT9D112_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int MT9D112_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int MT9D112_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int MT9D112_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int MT9D112_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int MT9D112_queryctrl(
	struct v4l2_subdev *sd,
	struct v4l2_queryctrl *qc
)
{
	/* Fill in min, max, step and default value for these controls. */
	switch (qc->id) {
		case V4L2_CID_BRIGHTNESS:
			qc->minimum = 0;
			qc->maximum = 255;
			qc->step = 1;
			qc->default_value = 128;
		break;
				
		case V4L2_CID_CONTRAST:
			qc->minimum = 0;
			qc->maximum = 127;
			qc->step = 1;
			qc->default_value = 64;
		break;

		case V4L2_CID_VFLIP:
			qc->minimum = 0;
			qc->maximum = 1;
			qc->step = 1;
			qc->default_value = 0;
		break;
		
		case V4L2_CID_HFLIP:
			qc->minimum = 0;
			qc->maximum = 1;
			qc->step = 1;
			qc->default_value = 0;
		break;
		
		case V4L2_CID_SATURATION:
			qc->minimum = 0;
			qc->maximum = 256;
			qc->step = 1;
			qc->default_value = 128;
		break;
		
		case V4L2_CID_HUE:
			qc->minimum = -180;
			qc->maximum = 180;
			qc->step = 5;
			qc->default_value = 0;
		break;
	}
	return -EINVAL;
}

static int MT9D112_g_ctrl(
	struct v4l2_subdev *sd,
	struct v4l2_control *ctrl
)
{
	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		break;
		case V4L2_CID_CONTRAST:
		break;
		case V4L2_CID_VFLIP:
		break;
		case V4L2_CID_HFLIP:

		break;
		case V4L2_CID_SATURATION:
		break;
		case V4L2_CID_HUE:
		break;
	}
	return 0;	
}

static int MT9D112_s_ctrl(
	struct v4l2_subdev *sd,
	struct v4l2_control *ctrl
)
{
	switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		break;
		case V4L2_CID_CONTRAST:
		break;
		case V4L2_CID_VFLIP:
		break;
		case V4L2_CID_HFLIP:
		break;
		case V4L2_CID_SATURATION:
		break;
		case V4L2_CID_HUE:
		break;
	}
	return 0;	
}

int MT9D112_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	return MT9D112_write_array(MT9D112_default_regs);
}

int MT9D112_suspend(
	struct v4l2_subdev *sd
)
{
	return MT9D112_write_array(MT9D112_suspend_regs);
}

int MT9D112_resume(
	struct v4l2_subdev *sd
)
{
	return MT9D112_write_array(MT9D112_resume_regs);
}

static int gp_MT9D112_init(void)
{
	int ret;

	v4l2_subdev_init(&(MT9D112_info.sd), &MT9D112_ops);

	strcpy(MT9D112_info.sd.name, "mt9d112");
	
	ret = register_sensor(&(MT9D112_info.sd), (int *)&param[0]);
	if( ret<0 )
		return ret;

	i2c_attr = (struct i2c_bus_attr_t *)gp_i2c_bus_request(SENSOR_I2C_SLAVE_ADDR, 20);
	return 0;
}

static void gp_MT9D112_exit(void) {
	unregister_sensor(&(MT9D112_info.sd));
	return;
}

module_init(gp_MT9D112_init);
module_exit(gp_MT9D112_exit);

MODULE_LICENSE_GP;
