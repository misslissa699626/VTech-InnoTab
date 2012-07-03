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

/*!
 * @file gp_tv1.h
 * @brief The tv1 driver
 */
#include <linux/init.h>
#include <linux/configfs.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <asm/system.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */

#include <mach/panel_cfg.h>
#include <mach/hardware.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/hal_disp2.h>
#include <mach/hal/sysregs.h>
#include <mach/hal/hal_ppu.h>
#include <mach/gp_display2.h>
#include <mach/clock_mgr/gp_clock.h>

MODULE_LICENSE("GPL");

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static int32_t tv1_init(void);
static int32_t tv1_suspend(void);
static int32_t tv1_resume(void);
static int32_t tv1_set_mode(void *data);

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define		VDAC_PDALL					(1 << 0)
#define		VDAC_TEST					(1 << 1)
#define		VDAC_UD						(1 << 2)
#define     VDAC_CRYSTAL_EN             (1 << 3)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct
{
	char *name;
	uint32_t resolution;
	uint32_t tvType;
	uint32_t interlace;
	uint32_t saturation;
	uint32_t hue;
	uint32_t brightness;
	uint32_t sharpness;
	uint32_t yGain;
	uint32_t yDelay;
	uint32_t vPosition;
	uint32_t hPosition;
	uint32_t videoDac;
} disp2_TvInfo;

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static const char gPanelName[] = "tv1";

const uint16_t gTv1RegSet[][9] = {
	{0x815E,0x5A00,0x8200,0x0000,0x6D4E,0x1F10,0x7C00,0xF003,0x2100},  //0: NTSC-M
	{0x815E,0x5A00,0x7600,0x0000,0x6D4E,0xB800,0x1E00,0xF003,0x2100},  //1: NTSC-J
	{0x8161,0x5A00,0x8200,0x0000,0x6D4E,0xCB10,0x8A00,0x0903,0x2A00},  //2: NTSC-N
	{0x815E,0x5A00,0x8200,0x0000,0x6D4E,0x27F0,0xA500,0xF007,0x2100},  //3: PAL-M
	{0x8161,0x5A00,0x7600,0x0000,0x6D4E,0xCBE0,0x8A00,0x0905,0x2A00},  //4: PAL-B
	{0x8161,0x5A00,0x8200,0x0000,0x6D4E,0xCBF0,0x8A00,0x0905,0x2A00},  //5: PAL-N
	{0x8161,0x5A00,0x7600,0x0000,0x6D4E,0xCBE0,0x8A00,0x0905,0x2A00},  //6: PAL-Nc
};


static disp2_TvInfo gPanelInfo = {
	.name = (char*) &gPanelName,
	.resolution = HAL_DISP2_RES_640_480,
	.tvType = HAL_DISP2_TV_MODE_RESERVED,
	.interlace = HAL_DISP2_TV_INTERLACE,
	.saturation = 0x815E,
	.hue = 0x5A00,
	.brightness = 0x8200,
	.sharpness = 0x0000,
	.yGain = 0x6D4E,
	.yDelay = 0x1F10,
	.vPosition = 0x7C00,
	.hPosition = 0xF003,
	.videoDac = 0x2100,
};

/* access functions */
gp_disp_panel_ops_t tv1_fops = {
	.init = tv1_init,
	.suspend = tv1_suspend,
	.resume = tv1_resume,
	.get_size = NULL,
	.set_param = tv1_set_mode,
};

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static void
scu_tvout_vdec_enable(
	void
)
{
	int32_t val = 0;
	val = SCUA_VDAC_CFG;
	if(0 == (val & VDAC_CRYSTAL_EN)){
		val |= VDAC_CRYSTAL_EN;
		SCUA_VDAC_CFG = val;
		val &= (~VDAC_PDALL);
		SCUA_VDAC_CFG = val;
	}
}

static void
scu_tvout_vdec_down(
	void
)
{
    uint32_t val = 0;
	
    val = SCUA_VDAC_CFG;
    if(val & VDAC_CRYSTAL_EN){
        val |= VDAC_PDALL;
        SCUA_VDAC_CFG = val;
        val &= (~VDAC_CRYSTAL_EN);
        SCUA_VDAC_CFG = val;
    }
}

static int32_t
tv1_common_init(
	void
)
{
#ifndef CONFIG_PM
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
#endif
  
	printk("[%s:%d]\n", __FUNCTION__, __LINE__);

	gpHalDisp2SetDevType(HAL_DISP2_DEV_TV);
#ifdef CONFIG_PM
	gp_enable_clock((int*)"PPU_TV", 1);
#else
	pscuaReg->scuaPeriClkEn2 |= 0x32000000;
#endif
	scu_tvout_vdec_enable();

	gpHalDisp2SetRes(gPanelInfo.resolution);
	gpHalDisp2SetTvType(gPanelInfo.tvType);
	gpHalDisp2SetInterlace(gPanelInfo.interlace);
	gpHalDisp2SetSaturation(gPanelInfo.saturation);
	gpHalDisp2SetHue(gPanelInfo.hue);
	gpHalDisp2SetBrightness(gPanelInfo.brightness);
	gpHalDisp2SetSharpness(gPanelInfo.sharpness);
	gpHalDisp2SetYGain(gPanelInfo.yGain);
	gpHalDisp2SetYDelay(gPanelInfo.yDelay);
	gpHalDisp2SetVPosition(gPanelInfo.vPosition);
	gpHalDisp2SetHPosition(gPanelInfo.hPosition);
	gpHalDisp2SetVideoDac(gPanelInfo.videoDac);

	gpHalPPUSetFbEnable(1);
	return 0;
}

static int32_t
tv1_init(
	void
)
{
	tv1_common_init();

	return 0;
}

static int32_t
tv1_suspend(
	void
)
{
	scu_tvout_vdec_down();
	gpHalPPUSetFbEnable(0);
#ifdef CONFIG_PM
	gp_enable_clock((int*)"PPU_TV", 0);
#endif
	return 0;
}

static int32_t
tv1_resume(
	void
)
{
	return tv1_common_init();
}

static int32_t
tv1_set_mode(
	void *data
)
{
	int32_t mode;

	mode = *(uint32_t *) data;

#if 1 
		if (mode == SP_DISP_TV_MODE_NTSC) {
				gPanelInfo.resolution = HAL_DISP2_RES_640_480;
				gPanelInfo.tvType = HAL_DISP2_TV_MODE_RESERVED;
				gPanelInfo.interlace = HAL_DISP2_TV_INTERLACE;
				gPanelInfo.saturation = 0x815E;
				gPanelInfo.hue = 0x5A00;
				gPanelInfo.brightness = 0x8200;
				gPanelInfo.sharpness = 0x0000;
				gPanelInfo.yGain = 0x6D4E;
				gPanelInfo.yDelay = 0x1F10;
				gPanelInfo.vPosition = 0x7C00;
				gPanelInfo.hPosition = 0xF003;
				gPanelInfo.videoDac = 0x2100;	
				gpHalDisp2SetRes(gPanelInfo.resolution);
				gpHalDisp2SetTvType(gPanelInfo.tvType);
				gpHalDisp2SetInterlace(gPanelInfo.interlace);
				gpHalDisp2SetSaturation(gPanelInfo.saturation);
				gpHalDisp2SetHue(gPanelInfo.hue);
				gpHalDisp2SetBrightness(gPanelInfo.brightness);
				gpHalDisp2SetSharpness(gPanelInfo.sharpness);
				gpHalDisp2SetYGain(gPanelInfo.yGain);
				gpHalDisp2SetYDelay(gPanelInfo.yDelay);
				gpHalDisp2SetVPosition(gPanelInfo.vPosition);
				gpHalDisp2SetHPosition(gPanelInfo.hPosition);
				gpHalDisp2SetVideoDac(gPanelInfo.videoDac);					  
		}
		else if (mode == SP_DISP_TV_MODE_PAL) {
				gPanelInfo.resolution = HAL_DISP2_RES_640_480,
				gPanelInfo.tvType = HAL_DISP2_TV_MODE_RESERVED,
				gPanelInfo.interlace = HAL_DISP2_TV_INTERLACE,
				gPanelInfo.saturation = 0x8161;
				gPanelInfo.hue = 0x5A00;
				gPanelInfo.brightness = 0x7600;
				gPanelInfo.sharpness = 0x0000;
				gPanelInfo.yGain = 0x6D5F;
				gPanelInfo.yDelay = 0xCBE0;
				gPanelInfo.vPosition = 0x8A00;
				gPanelInfo.hPosition = 0x0905;
				gPanelInfo.videoDac = 0x2A00;	
				gpHalDisp2SetRes(gPanelInfo.resolution);
				gpHalDisp2SetTvType(gPanelInfo.tvType);
				gpHalDisp2SetInterlace(gPanelInfo.interlace);
				gpHalDisp2SetSaturation(gPanelInfo.saturation);
				gpHalDisp2SetHue(gPanelInfo.hue);
				gpHalDisp2SetBrightness(gPanelInfo.brightness);
				gpHalDisp2SetSharpness(gPanelInfo.sharpness);
				gpHalDisp2SetYGain(gPanelInfo.yGain);
				gpHalDisp2SetYDelay(gPanelInfo.yDelay);
				gpHalDisp2SetVPosition(gPanelInfo.vPosition);
				gpHalDisp2SetHPosition(gPanelInfo.hPosition);
				gpHalDisp2SetVideoDac(gPanelInfo.videoDac);					
		}
		else {
			printk("[%s:%d], unknow mode=%d\n", __FUNCTION__, __LINE__, mode);
			return -1;
		}
#endif
	return 0;
}

static int32_t
tv1Panel_init(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  register_paneldev2(SP_DISP_OUTPUT_TV, gPanelInfo.name, &tv1_fops);
  return 0;
}

static void
tv1Panel_exit(
	void
)
{
  printk("[%s:%d]\n", __FUNCTION__, __LINE__);
  unregister_paneldev2(SP_DISP_OUTPUT_TV, gPanelInfo.name);
}

module_init(tv1Panel_init);
module_exit(tv1Panel_exit);
