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
 *  Hsinchu City 300    {  0x, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
 
/**************************************************************************
 *                         H E A D E R   F I L E S												*
 **************************************************************************/
#include <linux/module.h>
#include <linux/fs.h> /* everything... */
#include <linux/videodev2.h>
#include <media/v4l2-device.h>

#include <linux/delay.h>
#include <mach/gp_cdsp.h>
#include <mach/gp_gpio.h>
#include <mach/gp_i2c_bus.h>
#include <mach/sensor_mgr.h>


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
/* max resolution: 2048*1536 */
#define	mt_9d112_ID					0x78
#define mt_9d112_WIDTH				1600
#define mt_9d112_HEIGHT				1200

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define I2C_USE_GPIO				0
#define I2C_SCL_IO					0x0
#define I2C_SDA_IO					0x1

static char *param[] = {"0", "PORT0", "0", "NONE", "0", "NONE"};
static int nstrs = 6;
module_param_array(param, charp, &nstrs, S_IRUGO);

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct regval_list_s 
{
	unsigned short reg_num;
	unsigned short value;
};


struct mt_9d112_fmt_s
{
	__u8 *desc;
	__u32 pixelformat;
	struct regval_list_s *pInitRegs;
	struct regval_list_s *pScaleRegs;
	int bpp;					/* Bytes per pixel */
}; 

typedef struct mt_9d112_dev_s 
{
	struct v4l2_subdev sd;
	struct mt_9d112_fmt_s *fmt;	/* Current format */
	short width;
	short height;
	int hue;					/* Hue value */
	unsigned char sat;			/* Saturation value */
	unsigned char reserved0;
	unsigned char reserved1;
	unsigned char reserved2;
}mt_9d112_dev_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

 static struct regval_list_s MT9D112_default_regs[] = {
	{0x301A, 0x0ACC},	 // RESET_REGISTER
	{0x3202, 0x0008},	 // STANDBY_CONTROL 	  
	{0x341E, 0x8F09},	 // PLL_CLK_IN_CONTROL
	{0x341C, 0x0218},	 // PLL_DIVIDERS1			  
	{0x341E, 0x8F09},	 // PLL_CLK_IN_CONTROL
	{0x341E, 0x8F08},	 // PLL_CLK_IN_CONTROL
	{0x3044, 0x0540},	 // DARK_CONTROL
	{0x3216, 0x02CF},	 // INTERNAL_CLOCK_CONTROL
	{0x321C, 0x0402},	 // OF_CONTROL_STATUS
	{0x3212, 0x0001},	 // FACTORY_BYPASS
	{0x341E, 0x8F09},	 // PLL_CLK_IN_CONTROL
	{0x341C, 0x0120},	 // PLL_DIVIDERS1
	{0x341E, 0x8F09},	 // PLL_CLK_IN_CONTROL
	{0x341E, 0x8F08},	 // PLL_CLK_IN_CONTROL
	{0x3044, 0x0540},	 // DARK_CONTROL
	{0x3216, 0x02CF},	 // INTERNAL_CLOCK_CONTROL
	{0x321C, 0x0402},	 // OF_CONTROL_STATUS
	{0x3212, 0x0001},	 // FACTORY_BYPASS//1??бд?ижии?3имDи░ид?ио????буж╠?07005
 
	 //[MT9D112 (SOC2020) Register Wizard Defaults
	 //PLL START
	{0x3214, 0x06e6},        //ZT want to 	PLL
	{0x341E, 0x8F09},		//PLL/ Clk_in control: BYPASS PLL = 36617
	{0x341C, 0x024f},		   //PLL Control 1 = 288	  // Allow PLL to lock 250
	 
	{0x341E, 0x8F09},		//PLL/ Clk_in control: PLL ON, bypassed = 36617
	{0x341E, 0x8F08},		//PLL/ Clk_in control: USE PLL = 36616
	{0x3044, 0x0540},		//Reserved = 1344
	{0x3216, 0x02CF},		//Internal Clock Control = 719
	{0x321C, 0x0402},		//OF Control Status = 1026
	{0x3212, 0x0001},		//Factory Bypass = 1
	//{0x3040, 0x0024},		//read_mode
	 
 // QVGA IS OK  ANYSIZE IS ok
	{0x338C, 0x2703},		//Output Width (A)
	{0x3390, 0x0280},		//		= 320
	{0x338C, 0x2705},		//Output Height (A)
	{0x3390, 0x01e0},		//		= 240
	{0x338C, 0x2707},		//Output Width (B)
	{0x3390, 0x0640},		//		= 1600
	{0x338C, 0x2709},		//Output Height (B)
	{0x3390, 0x04B0},		//		= 1200
	{0x338C, 0x270D},		//Row Start (A)
	{0x3390, 0x0000},		//		= 0
	{0x338C, 0x270F},		//Column Start (A)
	{0x3390, 0x0000},		//		= 0
	{0x338C, 0x2711},		//Row End (A)
	{0x3390, 0x04BD},		//		= 1213
	{0x338C, 0x2713},		//Column End (A)
	{0x3390, 0x064D},		//		= 1613
	{0x338C, 0x2715},		//Extra Delay (A) //0x
	{0x3390, 0x01A8},		//		= 0//0x0000
	{0x338C, 0x2717},		//Row Speed (A)
	{0x3390, 0x2111},		//		= 8465
	{0x338C, 0x2719},		//Read Mode (A)
	{0x3390, 0x046C},		  //	  = 1132
	//0x3390, 0x046d},		//		= 1132
	{0x338C, 0x271B},		//sensor_sample_time_pck (A)
	{0x3390, 0x024F},		//		= 591
	{0x338C, 0x271D},		//sensor_fine_correction (A)
	{0x3390, 0x0102},		//		= 258
	{0x338C, 0x271F},		//sensor_fine_IT_min (A)
	{0x3390, 0x0279},		//		= 633
	{0x338C, 0x2721},		//sensor_fine_IT_max_margin (A)
	{0x3390, 0x0155},		//		= 341
	{0x338C, 0x2723},		//Frame Lines (A)
	{0x3390, 0x0341},		//		= 659 0x0293
	{0x338C, 0x2725},		//Line Length (A)
	{0x3390, 0x060F},		//		= 1551

	{0x338C, 0x2727},		//sensor_dac_id_4_5 (A)
	{0x3390, 0x2020},		//		= 8224
	{0x338C, 0x2729},		//sensor_dac_id_6_7 (A)
	{0x3390, 0x2020},		//		= 8224
	{0x338C, 0x272B},		//sensor_dac_id_8_9 (A)
	{0x3390, 0x1020},		//		= 4128
	{0x338C, 0x272D},		//sensor_dac_id_10_11 (A)
	{0x3390, 0x2007},		//		= 8199
	{0x338C, 0x272F},		//Row Start (B)
	{0x3390, 0x0004},		//		= 4
	{0x338C, 0x2731},		//Column Start (B)
	{0x3390, 0x0004},		//		= 4
	{0x338C, 0x2733},		//Row End (B)
	{0x3390, 0x04BB},		//		= 1211
	{0x338C, 0x2735},		//Column End (B)
	{0x3390, 0x064B},		//		= 1611
	{0x338C, 0x2737},		//Extra Delay (B)
	{0x3390, 0x022C},		//		= 556
	{0x338C, 0x2739},		//Row Speed (B)
	{0x3390, 0x2111},		//		= 8465
	{0x338C, 0x273B},		//Read Mode (B)
	{0x3390, 0x0024},		  //	  = 36
	//0x3390, 0x0027},		//		= 36
	{0x338C, 0x273D},		//sensor_sample_time_pck (B)
	{0x3390, 0x0120},		//		= 288
	{0x338C, 0x273F},		//sensor_fine_correction (B)
	{0x3390, 0x00A4},		//		= 164
	{0x338C, 0x2741},		//sensor_fine_IT_min (B)
	{0x3390, 0x0169},		//		= 361
	{0x338C, 0x2743},		//sensor_fine_IT_max_margin (B)
	{0x3390, 0x00A4},		//		= 164
	{0x338C, 0x2745},		//Frame Lines (B)
	{0x3390, 0x0625},		//		= 1573
	{0x338C, 0x2747},		//Line Length (B)
	{0x3390, 0x0824},		//		= 2084

	//CH 2
	{0x338C, 0x2751},		//Crop_X0 (A)
	{0x3390, 0x0000},		//		= 0
	{0x338C, 0x2753},		//Crop_X1 (A)
	{0x3390, 0x0320},		//		= 800
	{0x338C, 0x2755},		//Crop_Y0 (A)
	{0x3390, 0x0000},		//		= 0
	{0x338C, 0x2757},		//Crop_Y1 (A)
	{0x3390, 0x0258},		//		= 600
	{0x338C, 0x275F},		//Crop_X0 (B)
	{0x3390, 0x0000},		//		= 0
	{0x338C, 0x2761},		//Crop_X1 (B)
	{0x3390, 0x0640},		//		= 1600
	{0x338C, 0x2763},		//Crop_Y0 (B)
	{0x3390, 0x0000},		//		= 0
	{0x338C, 0x2765},		//Crop_Y1 (B)
	{0x3390, 0x04B0},		//		= 1200 




		//[CCM]													  
	{0x338C, 0x2306},	 // MCU_ADDRESS [AWB_CCM_L_0]	   
	{0x3390, 0x0055},	 // MCU_DATA_0	   
	{0x337C, 0x0004},	 //cgz oppo 20081021
	{0x338C, 0x2308},	 // MCU_ADDRESS [AWB_CCM_L_1]	   
	{0x3390, 0xFEC3},	 // MCU_DATA_0					   
	{0x338C, 0x230A},	 // MCU_ADDRESS [AWB_CCM_L_2]	   
	{0x3390, 0x0236},	 // MCU_DATA_0					   
	{0x338C, 0x230C},	 // MCU_ADDRESS [AWB_CCM_L_3]	   
	{0x3390, 0xFE2A},	 // MCU_DATA_0					   
	{0x338C, 0x230E},	 // MCU_ADDRESS [AWB_CCM_L_4]	   
	{0x3390, 0x0229},	 // MCU_DATA_0					   
	{0x338C, 0x2310},	 // MCU_ADDRESS [AWB_CCM_L_5]	   
	{0x3390, 0x014A},	 // MCU_DATA_0					   
	{0x338C, 0x2312},	 // MCU_ADDRESS [AWB_CCM_L_6]	   
	{0x3390, 0xFEB0},	 // MCU_DATA_0					   
	{0x338C, 0x2314},	 // MCU_ADDRESS [AWB_CCM_L_7]	   
	{0x3390, 0xF867},	 // MCU_DATA_0					   
	{0x338C, 0x2316},	 // MCU_ADDRESS [AWB_CCM_L_8]	   
	{0x3390, 0x0ACD},	 // MCU_DATA_0					   
	{0x338C, 0x2318},	 // MCU_ADDRESS [AWB_CCM_L_9]	   
	{0x3390, 0x0024},	 // MCU_DATA_0					   
	{0x338C, 0x231A},	 // MCU_ADDRESS [AWB_CCM_L_10]	   
	{0x3390, 0x003D},	 // MCU_DATA_0					   
	{0x338C, 0x231C},	 // MCU_ADDRESS [AWB_CCM_RL_0]	   
	{0x3390, 0x0353},	 // MCU_DATA_0
	{0x338C, 0x231E},	 // MCU_ADDRESS [AWB_CCM_RL_1]	   
	{0x3390, 0xFECA},	 // MCU_DATA_0					   
	{0x338C, 0x2320},	 // MCU_ADDRESS [AWB_CCM_RL_2]	   
	{0x3390, 0xFDE6},	 // MCU_DATA_0					   
	{0x338C, 0x2322},	 // MCU_ADDRESS [AWB_CCM_RL_3]	   
	{0x3390, 0x011A},	 // MCU_DATA_0					   
	{0x338C, 0x2324},	 // MCU_ADDRESS [AWB_CCM_RL_4]	   
	{0x3390, 0x01B2},	 // MCU_DATA_0					   
	{0x338C, 0x2326},	 // MCU_ADDRESS [AWB_CCM_RL_5]	   
	{0x3390, 0xFCE2},	 // MCU_DATA_0					   
	{0x338C, 0x2328},	 // MCU_ADDRESS [AWB_CCM_RL_6]	   
	{0x3390, 0x014A},	 // MCU_DATA_0					   
	{0x338C, 0x232A},	 // MCU_ADDRESS [AWB_CCM_RL_7]	   
	{0x3390, 0x060E},	 // MCU_DATA_0					   
	{0x338C, 0x232C},	 // MCU_ADDRESS [AWB_CCM_RL_8]	   
	{0x3390, 0xF81D},	 // MCU_DATA_0					   
	{0x338C, 0x232E},	 // MCU_ADDRESS [AWB_CCM_RL_9]	   
	{0x3390, 0x0010},	 // MCU_DATA_0					   
	{0x338C, 0x2330},	 // MCU_ADDRESS [AWB_CCM_RL_10]    
	{0x3390, 0xFFEC},	 // MCU_DATA_0					   
	{0x338C, 0xA348},	 // MCU_ADDRESS [AWB_GAIN_BUFFER_SP
	{0x3390, 0x0008},	 // MCU_DATA_0					   
	{0x338C, 0xA349},	 // MCU_ADDRESS [AWB_JUMP_DIVISOR] 
	{0x3390, 0x0002},	 // MCU_DATA_0					   
	{0x338C, 0xA34A},	 // MCU_ADDRESS [AWB_GAIN_MIN]	   
	{0x3390, 0x0059},	 // MCU_DATA_0					   
	{0x338C, 0xA34B},	 // MCU_ADDRESS [AWB_GAIN_MAX]	   
	{0x3390, 0x00A6},	 // MCU_DATA_0					   
	{0x338C, 0xA34F},	 // MCU_ADDRESS [AWB_CCM_POSITION_M
	{0x3390, 0x0000},	 // MCU_DATA_0					   
	{0x338C, 0xA350},	 // MCU_ADDRESS [AWB_CCM_POSITION_M
	{0x3390, 0x007F},	 // MCU_DATA_0					   
	{0x338C, 0xA352},	 // MCU_ADDRESS [AWB_SATURATION]   
	{0x3390, 0x0080},	 // MCU_DATA_0					   
	{0x338C, 0xA353},	 // MCU_ADDRESS [AWB_MODE]		   
	{0x3390, 0x0001},	 // MCU_DATA_0					   
	{0x338C, 0xA35B},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_O
	{0x3390, 0x0078},	 // MCU_DATA_0					   
	{0x338C, 0xA35C},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_O
	{0x3390, 0x0086},	 // MCU_DATA_0					   
	{0x338C, 0xA35D},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_I
	{0x3390, 0x007E},	 // MCU_DATA_0					   
	{0x338C, 0xA35E},	 // MCU_ADDRESS [AWB_STEADY_BGAIN_I
	{0x3390, 0x0082},	 // MCU_DATA_0					   
	{0x338C, 0x235F},	 // MCU_ADDRESS [AWB_CNT_PXL_TH]   
	{0x3390, 0x0040},	 // MCU_DATA_0					   
	{0x338C, 0xA361},	 // MCU_ADDRESS [AWB_TG_MIN0]	   
	{0x3390, 0x00D7},	 // MCU_DATA_0					   
	{0x338C, 0xA362},	 // MCU_ADDRESS [AWB_TG_MAX0]	   
	{0x3390, 0x00F6},	 // MCU_DATA_0					   
	{0x338C, 0xA302},	 // MCU_ADDRESS [AWB_WINDOW_POS]   
	{0x3390, 0x0000},	 // MCU_DATA_0					   
	{0x338C, 0xA303},	 // MCU_ADDRESS [AWB_WINDOW_SIZE]  
	{0x3390, 0x00EF},	 // MCU_DATA_0					   		
	{0x338C, 0xA364},  // MCU_ADDRESS [AWB_KR_L]		   
	{0x3390, 0x0098},	 // MCU_DATA_0					   
	{0x338C, 0xA365},	 // MCU_ADDRESS [AWB_KG_L]		   
	{0x3390, 0x0096},	 // MCU_DATA_0					   
	{0x338C, 0xA366},	 // MCU_ADDRESS [AWB_KB_L]		   
	{0x3390, 0x0084},	 // MCU_DATA_0					   
	{0x338C, 0xA367},	 // MCU_ADDRESS [AWB_KR_R]		   
	{0x3390, 0x0087},	 // MCU_DATA_0					   
	{0x338C, 0xA368},	 // MCU_ADDRESS [AWB_KG_R]		   
	{0x3390, 0x0080},	 // MCU_DATA_0					   
	{0x338C, 0xA369},	 // MCU_ADDRESS [AWB_KB_R]		   
	{0x3390, 0x0089},	 // MCU_DATA_0		 
	{0x338C, 0xA103},	 // MCU_ADDRESS [SEQ_CMD]		   
	{0x3390, 0x0006},	 // MCU_DATA_0	   
	{0x338C, 0xA103},	 // MCU_ADDRESS [SEQ_CMD]		   
	{0x3390, 0x0005},	 // MCU_DATA_0	 

	
	
	//[Fix Frame rate]
	{0x338C, 0xA123},	 // MCU_ADDRESS [SEQ_PREVIEW_0_FD]
	{0x3390, 0x0002},	 // MCU_DATA_0
	{0x338C, 0xA404},	 // MCU_ADDRESS [FD_MODE]
	{0x3390, 0x0042},	 // MCU_DATA_0
	{0x338C, 0xA413},	 // MCU_ADDRESS [FD_MODE]
	{0x3390, 0x0aba},	 // MCU_DATA_0
	{0x338C, 0xA130},	 // MCU_ADDRESS [SEQ_PREVIEW_2_AE]	  // add by sheree 1008
	{0x3390, 0x0004}, // MCU_DATA_0 
	{0x338C, 0xA103},  // MCU_ADDRESS
	{0x3390, 0x0005},  // MCU_DATA_0
                    
                     
 //[noise reduce setting]}
	{0x338C, 0xA115},	 // MCU_ADDRESS [SEQ_LLMODE]
	{0x3390, 0x00ED},	 // MCU_DATA_0                                //EF
	{0x338C, 0xA118},	 // MCU_ADDRESS [SEQ_LLSAT1]
	{0x3390, 0x0036},	 // MCU_DATA_0
	{0x338C, 0xA119},	 // MCU_ADDRESS [SEQ_LLSAT2]
	{0x3390, 0x0003},	 // MCU_DATA_0
	{0x338C, 0xA11A},	 // MCU_ADDRESS [SEQ_LLINTERPTHRESH1]
	{0x3390, 0x000A},	 // MCU_DATA_0
	{0x338C, 0xA11B},	 // MCU_ADDRESS [SEQ_LLINTERPTHRESH2]
	{0x3390, 0x0020},	 // MCU_DATA_0
	{0x338C, 0xA11C},	 // MCU_ADDRESS [SEQ_LLAPCORR1]
	{0x3390, 0x0002},	 // MCU_DATA_0
	{0x338C, 0xA11D},	 // MCU_ADDRESS [SEQ_LLAPCORR2]
	{0x3390, 0x0000},	 // MCU_DATA_0
	{0x338C, 0xA11E},	 // MCU_ADDRESS [SEQ_LLAPTHRESH1]
	{0x3390, 0x0000},	 // MCU_DATA_0
	{0x338C, 0xA11F},	 // MCU_ADDRESS [SEQ_LLAPTHRESH2]
	{0x3390, 0x0004},	 // MCU_DATA_0
	{0x338C, 0xA13E},	 // MCU_ADDRESS [SEQ_NR_TH1_R]
	{0x3390, 0x0004},	 // MCU_DATA_0
	{0x338C, 0xA13F},	 // MCU_ADDRESS [SEQ_NR_TH1_G]
	{0x3390, 0x000E},	 // MCU_DATA_0
	{0x338C, 0xA140},	 // MCU_ADDRESS [SEQ_NR_TH1_B]
	{0x3390, 0x0004},	 // MCU_DATA_0
	{0x338C, 0xA141},	 // MCU_ADDRESS [SEQ_NR_TH1_OL]
	{0x3390, 0x0004},	 // MCU_DATA_0
	{0x338C, 0xA142},	 // MCU_ADDRESS [SEQ_NR_TH2_R]
	{0x3390, 0x0032},	 // MCU_DATA_0
	{0x338C, 0xA143},	 // MCU_ADDRESS [SEQ_NR_TH2_G]
	{0x3390, 0x000F},	 // MCU_DATA_0
	{0x338C, 0xA144},	 // MCU_ADDRESS [SEQ_NR_TH2_B]
	{0x3390, 0x0032},	 // MCU_DATA_0
	{0x338C, 0xA145},	 // MCU_ADDRESS [SEQ_NR_TH2_OL]
	{0x3390, 0x0032},	 // MCU_DATA_0
	{0x338C, 0xA146},	 // MCU_ADDRESS [SEQ_NR_GAINTH1]
	{0x3390, 0x0005},	 // MCU_DATA_0
	{0x338C, 0xA147},	 // MCU_ADDRESS [SEQ_NR_GAINTH2]
	{0x3390, 0x003A},	 // MCU_DATA_0
	{0x338C, 0xA14F},	 // MCU_ADDRESS [SEQ_CLUSTERDC_TH]
	{0x3390, 0x000D},	 // MCU_DATA_0
	{0x338C, 0xA103},		//Refresh Sequencer Mode
	{0x3390, 0x0006},	  //	  = 6
	{0x338C, 0xA103},
	{0x3390, 0x0005},
 // test-5_max index change_target change]
	{0x338C, 0xA118},	 // MCU_ADDRESS [SEQ_LLSAT1]
	{0x3390, 0x0020},	 // MCU_DATA_0
	{0x338C, 0xA119},	 // MCU_ADDRESS [SEQ_LLSAT2]
	{0x3390, 0x0003},	 // MCU_DATA_0
	{0x338C, 0xA206},	 // MCU_ADDRESS [AE_TARGET]
	{0x3390, 0x0028},//0x0031,	 // MCU_DATA_0         AE TATGET
	{0x338C, 0xA207},	 // MCU_ADDRESS [AE_GATE]
	{0x3390, 0x000B},	 // MCU_DATA_0
	{0x338C, 0xA20C},	 // MCU_ADDRESS [AE_MAX_INDEX]
	{0x3390, 0x0003},	 // MCU_DATA_0
	{0x338C, 0xA109},	 // MCU_ADDRESS [SEQ_AE_FASTBUFF]
	{0x3390, 0x0020},	 //0x0024 cgz oppo 2008-1020// MCU_DATA_0
                          
                          
/*                        
	{0x338C, 0xA76D},	 // MCU_ADDRESS [MODE_GAM_CONT_A]
	{0x3390, 0x0003},	 // MCU_DATA_0
	{0x338C, 0xA76F},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_0]
	{0x3390, 0x0000},	 // MCU_DATA_0
	{0x338C, 0xA770},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_1]
	{0x3390, 0x0007},	 // MCU_DATA_0
	{0x338C, 0xA771},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_2]
	{0x3390, 0x0017},	 // MCU_DATA_0
	{0x338C, 0xA772},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_3]
	{0x3390, 0x003B},	 // MCU_DATA_0
	{0x338C, 0xA773},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_4]
	{0x3390, 0x0060},	 // MCU_DATA_0
	{0x338C, 0xA774},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_5]
	{0x3390, 0x007A},	 // MCU_DATA_0
	{0x338C, 0xA775},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_6]
	{0x3390, 0x008F},	 // MCU_DATA_0
	{0x338C, 0xA776},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_7]
	{0x3390, 0x00A0},	 // MCU_DATA_0
	{0x338C, 0xA777},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_8]
	{0x3390, 0x00AE},	 // MCU_DATA_0
	{0x338C, 0xA778},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_9]
	{0x3390, 0x00BA},	 // MCU_DATA_0
	{0x338C, 0xA779},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_10]
	{0x3390, 0x00C5},	 // MCU_DATA_0
	{0x338C, 0xA77A},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_11]
	{0x3390, 0x00CE},	 // MCU_DATA_0
	{0x338C, 0xA77B},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_12]
	{0x3390, 0x00D7},	 // MCU_DATA_0
	{0x338C, 0xA77C},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_13]
	{0x3390, 0x00DF},	 // MCU_DATA_0
	{0x338C, 0xA77D},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_14]
	{0x3390, 0x00E6},	 // MCU_DATA_0
	{0x338C, 0xA77E},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_15]
	{0x3390, 0x00ED},	 // MCU_DATA_0
	{0x338C, 0xA77F},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_16]
	{0x3390, 0x00F3},	 // MCU_DATA_0
	{0x338C, 0xA780},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_17]
	{0x3390, 0x00F9},	 // MCU_DATA_0
	{0x338C, 0xA781},	 // MCU_ADDRESS [MODE_GAM_TABLE_A_18]
	{0x3390, 0x00FF},	 // MCU_DATA_0
	{0x338C, 0xA103},	 // MCU_ADDRESS [SEQ_CMD]
	{0x3390, 0x0006},	 // MCU_DATA_0
	*/                    
                          
	{0x338C, 0xA102},	 // MCU_ADDRESS [SEQ_CMD]
	{0x3390, 0x000F},	 // MCU_DATA_0	  
	{0x338C, 0xA103},	 // MCU_ADDRESS [SEQ_CMD]
	{0x3390, 0x0005},	 // MCU_DATA_0

//lens [Register Log 09/03/09 15:10:52]
	{0x3210, 0x01F8}, 	// COLOR_PIPELINE_CONTROL [1]
	{0x34CE, 0x01E8}, 	// LENS_CORRECTION_CONTROL [1]
	{0x34D0, 0x6131}, 	// ZONE_BOUNDS_X1_X2 [1]
	{0x34D2, 0x3492}, 	// ZONE_BOUNDS_X0_X3 [1]
	{0x34D4, 0x9B68}, 	// ZONE_BOUNDS_X4_X5 [1]
	{0x34D6, 0x4B25}, 	// ZONE_BOUNDS_Y1_Y2 [1]
	{0x34D8, 0x2670}, 	// ZONE_BOUNDS_Y0_Y3 [1]
	{0x34DA, 0x724C}, 	// ZONE_BOUNDS_Y4_Y5 [1]
	{0x34DC, 0xFFFA}, 	// CENTER_OFFSET [1]
	{0x34DE, 0x00B4}, 	// FX_RED [1]
	{0x34E0, 0x007E}, 	// FX_GREEN [1]
	{0x34E2, 0x00A0}, 	// FX_GREEN2 [1]
	{0x34E4, 0x007C}, 	// FX_BLUE [1]
	{0x34E6, 0x00B4}, 	// FY_RED [1]
	{0x34E8, 0x007E}, 	// FY_GREEN [1]
	{0x34EA, 0x00A0}, 	// FY_GREEN2 [1]
	{0x34EC, 0x007C}, 	// FY_BLUE [1]
	{0x34EE, 0x0A97}, 	// DF_DX_RED [1]
	{0x34F0, 0x0CB0}, 	// DF_DX_GREEN [1]
	{0x34F2, 0x0A7D}, 	// DF_DX_GREEN2 [1]
	{0x34F4, 0x0CA7}, 	// DF_DX_BLUE [1]
	{0x34F6, 0x0BE9}, 	// DF_DY_RED [1]
	{0x34F8, 0x0B43}, 	// DF_DY_GREEN [1]
	{0x34FA, 0x09F0}, 	// DF_DY_GREEN2 [1]
	{0x34FC, 0x0A1C}, 	// DF_DY_BLUE [1]
	{0x3500, 0xDF4C}, 	// SECOND_DERIV_ZONE_0_RED [1]
	{0x3502, 0x2A34}, 	// SECOND_DERIV_ZONE_0_GREEN [1]
	{0x3504, 0x4551}, 	// SECOND_DERIV_ZONE_0_GREEN2 [1]
	{0x3506, 0x3D37}, 	// SECOND_DERIV_ZONE_0_BLUE [1]
	{0x3508, 0x2DF9}, 	// SECOND_DERIV_ZONE_1_RED [1]
	{0x350A, 0x1CE9}, 	// SECOND_DERIV_ZONE_1_GREEN [1]
	{0x350C, 0x310B}, 	// SECOND_DERIV_ZONE_1_GREEN2 [1]
	{0x350E, 0x30EA}, 	// SECOND_DERIV_ZONE_1_BLUE [1]
	{0x3510, 0x202C}, 	// SECOND_DERIV_ZONE_2_RED [1]
	{0x3512, 0x0C14}, 	// SECOND_DERIV_ZONE_2_GREEN [1]
	{0x3514, 0x1824}, 	// SECOND_DERIV_ZONE_2_GREEN2 [1]
	{0x3516, 0x1D1A}, 	// SECOND_DERIV_ZONE_2_BLUE [1]
	{0x3518, 0x2438}, 	// SECOND_DERIV_ZONE_3_RED [1]
	{0x351A, 0x181F}, 	// SECOND_DERIV_ZONE_3_GREEN [1]
	{0x351C, 0x262B}, 	// SECOND_DERIV_ZONE_3_GREEN2 [1]
	{0x351E, 0x1E16}, 	// SECOND_DERIV_ZONE_3_BLUE [1]
	{0x3520, 0x3031}, 	// SECOND_DERIV_ZONE_4_RED [1]
	{0x3522, 0x2721}, 	// SECOND_DERIV_ZONE_4_GREEN [1]
	{0x3524, 0x1C17}, 	// SECOND_DERIV_ZONE_4_GREEN2 [1]
	{0x3526, 0x1D20}, 	// SECOND_DERIV_ZONE_4_BLUE [1]
	{0x3528, 0x202B}, 	// SECOND_DERIV_ZONE_5_RED [1]
	{0x352A, 0x0A11}, 	// SECOND_DERIV_ZONE_5_GREEN [1]
	{0x352C, 0xEF0E}, 	// SECOND_DERIV_ZONE_5_GREEN2 [1]
	{0x352E, 0xF00E}, 	// SECOND_DERIV_ZONE_5_BLUE [1]
	{0x3530, 0x1BD2}, 	// SECOND_DERIV_ZONE_6_RED [1]
	{0x3532, 0x100B}, 	// SECOND_DERIV_ZONE_6_GREEN [1]
	{0x3534, 0xF8D4}, 	// SECOND_DERIV_ZONE_6_GREEN2 [1]
	{0x3536, 0x060B}, 	// SECOND_DERIV_ZONE_6_BLUE [1]
	{0x3538, 0x13D1}, 	// SECOND_DERIV_ZONE_7_RED [1]
	{0x353A, 0x134A}, 	// SECOND_DERIV_ZONE_7_GREEN [1]
	{0x353C, 0xE30A}, 	// SECOND_DERIV_ZONE_7_GREEN2 [1]
	{0x353E, 0xDD34}, 	// SECOND_DERIV_ZONE_7_BLUE [1]
	{0x3540, 0x0001}, 	// X2_FACTORS [1]
	{0x3542, 0x0007}, 	// GLOBAL_OFFSET_FXY_FUNCTION [24]
	{0x3544, 0x07FF}, 	// K_FACTOR_IN_K_FX_FY_R_TR [1]
	{0x3546, 0x0297}, 	// K_FACTOR_IN_K_FX_FY_G1_TR [1]
	{0x3548, 0x0000}, 	// K_FACTOR_IN_K_FX_FY_G2_TR [1]
	{0x354A, 0x03FF}, 	// K_FACTOR_IN_K_FX_FY_B_TR [1]
	{0x354C, 0x0600}, 	// K_FACTOR_IN_K_FX_FY_R_TL [1]
	{0x354E, 0x0625}, 	// K_FACTOR_IN_K_FX_FY_G1_TL [1]
	{0x3550, 0x03FF}, 	// K_FACTOR_IN_K_FX_FY_G2_TL [1]
	{0x3552, 0x0439}, 	// K_FACTOR_IN_K_FX_FY_B_TL [1]
	{0x3554, 0x06d0}, 	// K_FACTOR_IN_K_FX_FY_R_BR [22]
	{0x3556, 0x03FF}, 	// K_FACTOR_IN_K_FX_FY_G1_BR [1]
	{0x3558, 0x07D9}, 	// K_FACTOR_IN_K_FX_FY_G2_BR [1]
	{0x355A, 0x0000}, 	// K_FACTOR_IN_K_FX_FY_B_BR [1]
	{0x355C, 0x0000}, 	// K_FACTOR_IN_K_FX_FY_R_BL [59]
	{0x355E, 0x0000}, 	// K_FACTOR_IN_K_FX_FY_G1_BL [1]
	{0x3560, 0x0000}, 	// K_FACTOR_IN_K_FX_FY_G2_BL [1]
	{0x3562, 0x0000}, 	// K_FACTOR_IN_K_FX_FY_B_BL [1];
                        
	{0x35a4, 0x04c5},   //saturation
#if 1          //MT9D112_full_regs
	{0x301a,0x0acc},
	{0x3202,0x0008},	                
	{0x338c,0x2707},
	{0x3390,0x0640},
	{0x338c,0x2709},
	{0x3390,0x04B0},
	{0x338c,0x275f},
	{0x3390,0x0000},
	{0x338c,0x2761},
	{0x3390,0x0640},
	{0x338c,0x2763},
	{0x3390,0x0000},
	{0x338c,0x2765},
	{0x3390,0x04b0},
		                
	{0x338c,0xa120},
	{0x3390,0x0072},

	{0x338c,0xa103},
	{0x3390,0x0002},

	{0x338C, 0xA102},	
	{0x3390, 0x000f},//DISABLE AE
#endif
	{0xffff, 0xffff},   //end
};

#define C_mt_9d112_FMT_MAX	1
static struct mt_9d112_fmt_s g_mt_9d112_fmt[] = 
{
	{
		.desc		= "YUYV 4:2:2",
		.pixelformat = V4L2_PIX_FMT_UYVY,
		.pInitRegs	= MT9D112_default_regs,
		.pScaleRegs = NULL,
		.bpp		= 2,
	},
};

static mt_9d112_dev_t	g_mt_9d112_dev;
#if I2C_USE_GPIO == 1
static int g_scl_handle, g_sda_handle;
#else
static int g_i2c_handle;
#endif

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
static void 
sccb_delay (
	int i
) 
{
	udelay(i*10);
}

#if I2C_USE_GPIO == 1
static void 
sccb_start(
	void
)
{
	gp_gpio_set_value(g_scl_handle, 1);
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 1);
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 0);	
	sccb_delay (2);
}

static void 
sccb_stop(
	void
)
{
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 0);					
	sccb_delay (2);
	gp_gpio_set_value(g_scl_handle, 1);					
	sccb_delay (2);
	gp_gpio_set_value(g_sda_handle, 1);					
	sccb_delay (2);
}

static int 
sccb_w_phase(
	unsigned short value
)
{
	int i, nRet = 0;

	for(i=0;i<8;i++)
	{
		gp_gpio_set_value(g_scl_handle,0);		/* SCL0 */
		sccb_delay (2);
		if (value & 0x80)
			gp_gpio_set_value(g_sda_handle, 1);	/* SDA1 */
		else
			gp_gpio_set_value(g_sda_handle, 0);	/* SDA0 */
		gp_gpio_set_value(g_scl_handle, 1);		/* SCL1 */
		sccb_delay(2);
		value <<= 1;
	}
	/* The 9th bit transmission */
	gp_gpio_set_value(g_scl_handle, 0);				/* SCL0 */
	gp_gpio_set_input(g_sda_handle, GPIO_PULL_HIGH);/* SDA is Hi-Z mode */
	
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,1);				/* SCL1 */
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,0);				/* SCL0 */

	gp_gpio_get_value(g_sda_handle, &i);			/* check ack */
	if(i != 0) nRet = -1;
	gp_gpio_set_output(g_sda_handle, 1, 0);			/* SDA is output */
	return nRet;
}

static int 
sccb_r_phase(
	void
)
{
	int i;
	int data, temp;

	gp_gpio_set_input(g_sda_handle, GPIO_PULL_HIGH);/* SDA is Hi-Z mode */
	data = 0x00;
	for (i=0;i<8;i++)
	{
		gp_gpio_set_value(g_scl_handle,0);			/* SCL0 */
		sccb_delay(2);
		gp_gpio_set_value(g_scl_handle,1);			/* SCL1 */
		gp_gpio_get_value(g_sda_handle, &temp);
		data <<= 1;
		data |= temp;
		sccb_delay(2);
	}
	/* The 9th bit transmission */
	gp_gpio_set_value(g_scl_handle, 0);				/* SCL0 */
	gp_gpio_set_output(g_sda_handle, 1, 0);			/* SDA is output mode */
	gp_gpio_set_value(g_sda_handle, 1);				/* SDA0, the nighth bit is NA must be 1 */

	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,1);				/* SCL1 */
	sccb_delay(2);
	gp_gpio_set_value(g_scl_handle,0);				/* SCL0 */
	return data;		
}

static int
sccb_read (
	unsigned short id,			
	unsigned short addr,		
	unsigned char *value
) 
{
	int nRet = 0;
	
	/* Data re-verification */
	id &= 0xFF;
	addr &= 0xFFFF;

	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);	

	/* 2-Phase write transmission cycle is starting now ...*/
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1	*/	
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */
	
	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (id);				/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr >> 8);			/* Phase 2 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr & 0xFF);						
	if(nRet < 0) goto Return;
	sccb_stop ();							/* Transmission stop */

	/* 2-Phase read transmission cycle is starting now ... */
	sccb_start ();							/* Transmission start */
	nRet = sccb_w_phase (id | 0x01);		/* Phase 1 (read) */
	if(nRet < 0) goto Return;
	*value = sccb_r_phase();				/* Phase 2 */

Return:
	sccb_stop ();							/* Transmission stop */
	return nRet;
}

static int 
sccb_write (
	unsigned short id,
	unsigned short addr,
	unsigned char data
) 
{
	int nRet = 0;
	
	/* Data re-verification */
	id &= 0xFF;
	addr &= 0xFFFF;
	data &= 0xFF;
	
	/* Serial bus output mode initialization */
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);

	/* 3-Phase write transmission cycle is starting now ... */
	gp_gpio_set_value(g_scl_handle, 1);		/* SCL1 */		
	gp_gpio_set_value(g_sda_handle, 0);		/* SDA0 */
	sccb_start();							/* Transmission start */

	nRet = sccb_w_phase(id);				/* Phase 1 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase((addr >> 8)& 0xFF);	/* Phase 2 */
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(addr & 0xFF);
	if(nRet < 0) goto Return;
	nRet = sccb_w_phase(data);				/* Phase 3 */
	if(nRet < 0) goto Return;
	
Return:
	sccb_stop();							/* Transmission stop */
	return nRet;
}
#endif 

#if 0 
static int 
sensor_read(
	unsigned int reg,
	unsigned int *value
)
{
#if I2C_USE_GPIO == 1
	return sccb_read(mt_9d112_ID, reg, value);
#else
	char addr[2], data[0];
	int nRet;
	
	addr[0] = (reg >> 8) & 0xFF;
	addr[1] = reg & 0xFF;
	nRet = gp_i2c_bus_write(g_i2c_handle, addr, 2);
	nRet = gp_i2c_bus_read(g_i2c_handle, data, 1);
	*value = data[0];
	return nRet;
#endif
}
#endif

static int 
sensor_write(
	unsigned int reg,
	unsigned int value
)
{	
#if I2C_USE_GPIO == 1
	return sccb_write(mt_9d112_ID, reg, value);
#else
	char data[4];
	data[0] = (reg >> 8) & 0xFF;
	data[1] = reg & 0xFF;
	data[2] = (value >> 8) & 0xFF;	
	data[3] = value & 0xFF;	
	return gp_i2c_bus_write(g_i2c_handle, data, 4);
#endif
}

static int
mt_9d112_write_table(
	struct regval_list_s *vals
)
{
	int i, nRet;
	
	while (vals->reg_num != 0xffff || vals->value != 0xffff) 
	{
		for(i = 0; i< 10; i++)
		{
			nRet = sensor_write(vals->reg_num, vals->value);
			if(nRet >= 0)
			{
			#if 0
				unsigned char value;
				sensor_read(vals->reg_num, &value);
				printk("0x%x, 0x%x\n", vals->reg_num, value);
			#endif
				break;
			}
			else
				printk("I2C Fail\n");
		}
		if(i == 10) return -1;
		vals++;
	}
	return 0;
}

static int 
gp_mt_9d112_s_interface(
	struct v4l2_subdev *sd,
	struct v4l2_interface *interface
)
{
	return 0;
}

static int 
gp_mt_9d112_enum_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_fmtdesc *fmtdesc
)
{
	if(fmtdesc->index >= C_mt_9d112_FMT_MAX)
		return -EINVAL;

	fmtdesc->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	memcpy((void *)fmtdesc->description, (void *)g_mt_9d112_fmt[fmtdesc->index].desc, 10);
	fmtdesc->pixelformat = g_mt_9d112_fmt[fmtdesc->index].pixelformat;
	return 0;
}


static int gp_mt_9d112_try_fmt(
	struct v4l2_subdev *sd,
	struct v4l2_format *fmt
)
{
	int i;
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	for(i=0; i<C_mt_9d112_FMT_MAX; i++)
	{
		if (g_mt_9d112_fmt[i].pixelformat == pix->pixelformat)
			break;
	}
	
	if(i == C_mt_9d112_FMT_MAX)
		return -1;
	
	pix->width = mt_9d112_WIDTH;
	pix->height = mt_9d112_HEIGHT;
	pix->bytesperline = mt_9d112_WIDTH * g_mt_9d112_fmt[i].bpp;
	pix->sizeimage = pix->bytesperline * mt_9d112_HEIGHT;
	return 0;
}

static int 
gp_mt_9d112_s_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	g_mt_9d112_dev.width = fmt->fmt.pix.width;
	g_mt_9d112_dev.height = fmt->fmt.pix.height;
	if(fmt->fmt.pix.priv == C_CDSP_FRONT)
	{
		switch(fmt->fmt.pix.pixelformat)
		{
		case V4L2_PIX_FMT_UYVY:
			g_mt_9d112_dev.fmt = &g_mt_9d112_fmt[0];
			break;
		default:
			return -EINVAL;
		}
	}
	else
	{
		return -EINVAL;
	}
	
	return 0;
}

static int 
gp_mt_9d112_g_fmt(
	struct v4l2_subdev *sd, 
	struct v4l2_format *fmt
)
{
	fmt->fmt.pix.width = g_mt_9d112_dev.width;
	fmt->fmt.pix.height = g_mt_9d112_dev.height;
	fmt->fmt.pix.pixelformat = g_mt_9d112_dev.fmt->pixelformat;
	fmt->fmt.pix.field = V4L2_FIELD_NONE;
	fmt->fmt.pix.bytesperline = g_mt_9d112_dev.width * g_mt_9d112_dev.fmt->bpp;
	fmt->fmt.pix.sizeimage = fmt->fmt.pix.bytesperline * g_mt_9d112_dev.height;
	return 0;
}

static int 
gp_mt_9d112_cropcap(
	struct v4l2_subdev *sd,
	struct v4l2_cropcap *cc
)
{
	return 0;
}


static int 
gp_mt_9d112_g_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_mt_9d112_s_crop(
	struct v4l2_subdev *sd,
	struct v4l2_crop *crop
)
{
	return 0;
}

static int 
gp_mt_9d112_g_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *parms
)
{
	return 0;
}

static int 
gp_mt_9d112_s_parm(
	struct v4l2_subdev *sd,
	struct v4l2_streamparm *param
)
{
	return 0;
}

static int 
gp_mt_9d112_queryctrl(
	struct v4l2_subdev *sd,
	struct v4l2_queryctrl *qc
)
{
	/* Fill in min, max, step and default value for these controls. */
	switch(qc->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		qc->minimum = 0;
		qc->maximum = 1;
		qc->step = 1;
		qc->default_value = 1;
		break;

	case V4L2_CID_POWER_LINE_FREQUENCY:
		qc->minimum = 50;
		qc->maximum = 60;
		qc->step = 10;
		qc->default_value = 50;
		break;
	
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		qc->minimum = 0;
		qc->maximum = 3;
		qc->step = 1;
		qc->default_value = 0;
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:
		qc->minimum = 0;
		qc->maximum = 1;
		qc->step = 1;
		qc->default_value = 0;
		break;

	default:
		return -EINVAL;
	}
	return 0;
}
	
static int 
gp_mt_9d112_g_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{	
	int nRet = 0;
	
	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		break;
	
	case V4L2_CID_POWER_LINE_FREQUENCY:
		break;
			
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:	
		break;
		
	default:
		return -EINVAL;
	}
	return nRet; 
}

static int 
gp_mt_9d112_s_ctrl(
	struct v4l2_subdev *sd, 
	struct v4l2_control *ctrl
)
{
	int nRet = 0;

	switch(ctrl->id)
	{
	case V4L2_CID_AUTO_WHITE_BALANCE:
		break;
	
	case V4L2_CID_POWER_LINE_FREQUENCY:
		break;
		
	case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		break;

	case V4L2_CID_BACKLIGHT_COMPENSATION:
		break;
		
	default:
		return -EINVAL;
	}
	
	return nRet; 
}

static int 
gp_mt_9d112_reset(
	struct v4l2_subdev *sd, 
	u32 val
)
{
	sensor_write(0x301A, 0x0ACC);
	sccb_delay(val);
	return 0;
}

static int 
gp_mt_9d112_init(
	struct v4l2_subdev *sd,
	u32 val
)
{
	int nRet;

	if(g_mt_9d112_dev.fmt->pInitRegs)
	{
		printk("mt9d112InitReg\n");
		nRet = mt_9d112_write_table(g_mt_9d112_dev.fmt->pInitRegs);
		if(nRet < 0) return -1;
	}
	
	if(g_mt_9d112_dev.fmt->pScaleRegs)
	{
		printk("mt9d112ScaleReg\n");
		nRet = mt_9d112_write_table(g_mt_9d112_dev.fmt->pScaleRegs);
		if(nRet < 0) return -1;
	}
	return 0;
}

static int 
gp_mt_9d112_suspend(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static int 
gp_mt_9d112_resume(
	struct v4l2_subdev *sd
)
{
	return 0;
}

static const struct v4l2_subdev_core_ops cdsp_mt_9d112_core_ops = 
{
	.g_ctrl = gp_mt_9d112_g_ctrl,
	.s_ctrl = gp_mt_9d112_s_ctrl,
	.queryctrl = gp_mt_9d112_queryctrl,
	.reset = gp_mt_9d112_reset,
	.init = gp_mt_9d112_init,
};

static const struct v4l2_subdev_video_ops cdsp_mt_9d112_video_ops = 
{
	.enum_fmt = gp_mt_9d112_enum_fmt,
	.try_fmt = gp_mt_9d112_try_fmt,
	.g_fmt = gp_mt_9d112_g_fmt,
	.s_fmt = gp_mt_9d112_s_fmt,
	.s_parm = gp_mt_9d112_s_parm,
	.g_parm = gp_mt_9d112_g_parm,
	.cropcap = gp_mt_9d112_cropcap,
	.g_crop = gp_mt_9d112_g_crop,
	.s_crop = gp_mt_9d112_s_crop,	
};

static const struct v4l2_subdev_ext_ops cdsp_mt_9d112_ext_ops = 
{
	.s_interface = gp_mt_9d112_s_interface,
	.suspend = gp_mt_9d112_suspend,
	.resume = gp_mt_9d112_resume,
};

static const struct v4l2_subdev_ops cdsp_mt_9d112_ops = 
{
	.core = &cdsp_mt_9d112_core_ops,
	.video = &cdsp_mt_9d112_video_ops,
	.ext = &cdsp_mt_9d112_ext_ops
};

static int __init 
mt_9d112_init_module(
		void
)
{
#if I2C_USE_GPIO == 1
	g_scl_handle = gp_gpio_request(I2C_SCL_IO, "SCL"); 
	g_sda_handle = gp_gpio_request(I2C_SDA_IO, "SDA");
	if((g_scl_handle == 0) || (g_scl_handle == -EINVAL) || (g_scl_handle == -ENOMEM)||
		(g_sda_handle == 0) || (g_sda_handle == -EINVAL) || (g_sda_handle == -ENOMEM))
	{
		printk(KERN_WARNING "GpioReqFail %d, %d\n", g_scl_handle, g_sda_handle);
		gp_gpio_release(g_scl_handle);
		gp_gpio_release(g_sda_handle);	
		return -1;
	}
	gp_gpio_set_output(g_scl_handle, 1, 0);
	gp_gpio_set_output(g_sda_handle, 1, 0);
#else
	g_i2c_handle = gp_i2c_bus_request(mt_9d112_ID, 100);	/*100KHZ*/
	if((g_i2c_handle == 0) ||(g_i2c_handle == -ENOMEM))
	{
		printk(KERN_WARNING "i2cReqFail %d\n", g_i2c_handle);
		return -1;
	}
#endif

	printk(KERN_WARNING "ModuleInit: cdsp_mt_9d112 \n");
	g_mt_9d112_dev.fmt = &g_mt_9d112_fmt[0];
	g_mt_9d112_dev.width = mt_9d112_WIDTH;
	g_mt_9d112_dev.height = mt_9d112_HEIGHT;
	v4l2_subdev_init(&(g_mt_9d112_dev.sd), &cdsp_mt_9d112_ops);
	strcpy(g_mt_9d112_dev.sd.name, "cdsp_mt_9d112");
	register_sensor(&g_mt_9d112_dev.sd, (int *)&param[0]);
	return 0;
}

static void __exit
mt_9d112_module_exit(
		void
) 
{
#if I2C_USE_GPIO == 1	
	gp_gpio_release(g_scl_handle);
	gp_gpio_release(g_sda_handle);	
#else
	gp_i2c_bus_release(g_i2c_handle);
#endif	
	unregister_sensor(&(g_mt_9d112_dev.sd));
}

module_init(mt_9d112_init_module);
module_exit(mt_9d112_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus cdsp mt_9d112 Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");



