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
/**
 * @file    gp_ppu.c
 * @brief   Implement of PPU module driver.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */
 
//#include <linux/io.h>
#include <linux/module.h> 
#include <linux/cdev.h>
//#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
//#include <linux/hdreg.h> 		/* HDIO_GETGEO */
#include <linux/blkdev.h>
#include <media/v4l2-common.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <mach/general.h>
#include <mach/kernel.h>
#include <mach/diag.h>
#include <mach/gp_ppu.h>
#include <mach/gp_gpio.h>
#include <mach/gp_chunkmem.h>
#include <mach/hal/hal_clock.h>
#if PPU_HARDWARE_MODULE == MODULE_ENABLE
#include <mach/hal/hal_ppu.h>
#endif
//#include <mach/gp_tv.h>
//#include <mach/gp_ppu_irq.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/
//#define VIC_PPU                 20

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DEBUG	printk
#else
	#define DEBUG(...)
#endif

#define	PPU_MINOR		             0
#define PPU_NR_DEVS	             1
#define PPU_SPRITE_NUM_REG_MAX   0xFF
#define VIC_PPU                  20
#define PPU_GPIO_SET             0 

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
static signed int g_scl_handle,frame_counter;
#define I2C_SCL_IO  0x4
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
int gp_ppu_open(struct inode *inode, struct file *filp);
int gp_ppu_release(struct inode *inode, struct file *filp);
int gp_ppu_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
static unsigned int gp_ppu_poll(struct file *filp, struct poll_table_struct *poll);
static irqreturn_t gp_ppu_irq_handler(int irq, void *dev_id);

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
struct file_operations ppu_fops = {
	.owner = THIS_MODULE,
	.poll = gp_ppu_poll,
	.open = gp_ppu_open,
	.ioctl = gp_ppu_ioctl,
	.release = gp_ppu_release,
};

typedef struct gp_ppu_dev_s {
	struct cdev c_dev;
	wait_queue_head_t ppu_wait_queue;
	bool done;
} gp_ppu_dev_t;

int ppu_major;
static gp_ppu_dev_t *ppu_devices=NULL;
struct class *ppu_class;
unsigned int PPU_GO_REENTRY=TRUE,PPU_FIFO_GO_REENTRY=TRUE;

 /**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
//PPU Module Register

/**
 * @brief   PPU clock enable/disable
 * @param   enable [in] 0:disable, 1:enable
 * @return  None
 * @see
 */
static void ppu_clock_enable(int enable)
{
	gpHalScuClkEnable( SCU_A_PERI_LINEBUFFER | SCU_A_PERI_SAACC | 
					   SCU_A_PERI_NAND_ABT | SCU_A_PERI_REALTIME_ABT | 
					   SCU_A_PERI_SCA, SCU_A, enable);
	gpHalScuClkEnable( SCU_A_PERI_CEVA_L2RAM | SCU_A_PERI_PPU | 
					   SCU_A_PERI_PPU_REG | SCU_A_PERI_PPU_FB, SCU_A2, enable);
}

/**
 * @brief 	PPU module isr function.
* @return 	isr number.
 */
signed int 
gp_ppu_module_isr(
void
)
{
	 signed int temp=0;
	
	 temp = gpHalPPUIsr();
	
	 return temp;
}

/**
 * @brief 		PPU driver initial function.
* @param 	p_register_set [in]: PPU struct value initiation.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_set_isr(
PPU_REGISTER_SETS *p_register_set
)
{
	
	return 0;
}

/**
 * @brief 		PPU driver initial function.
* @param 	p_register_set [in]: PPU struct value initiation.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_init(
PPU_REGISTER_SETS *p_register_set
)
{
	int nRet=0;
	#if PPU_GPIO_SET == 2
	int pin;
	#endif
	//struct device *dev = &pdev->dev;
	
	gp_ppu_set_mem((unsigned int *)p_register_set, 0,sizeof(PPU_REGISTER_SETS),32);
	// Initiate PPU register sets
	p_register_set->update_register_flag = C_UPDATE_REG_SET_PPU | C_UPDATE_REG_SET_TEXT1 | C_UPDATE_REG_SET_TEXT2 | C_UPDATE_REG_SET_TEXT3 | C_UPDATE_REG_SET_TEXT4 | C_UPDATE_REG_SET_SPRITE;
	p_register_set->ppu_window1_x = 0x7FF;
	p_register_set->ppu_window1_y = 0x7FF;
	p_register_set->ppu_window2_x = 0x7FF;
	p_register_set->ppu_window2_y = 0x7FF;
	p_register_set->ppu_window3_x = 0x7FF;
	p_register_set->ppu_window3_y = 0x7FF;
	p_register_set->ppu_window4_x = 0x7FF;
	p_register_set->ppu_window4_y = 0x7FF;

	ppu_clock_enable(1);

  #if 1
  gpHalPPUEn(1);
  g_scl_handle=-1;
  frame_counter=0;
  #endif

  #if PPU_GPIO_SET == 1
  g_scl_handle = gp_gpio_request(I2C_SCL_IO, "SCL");
  printk("gpio_handle = %d\n", g_scl_handle);
  gp_gpio_set_output(g_scl_handle, 1, 0);
  #elif PPU_GPIO_SET == 2
	pin = (0<<24)|(2<<16)|(31<<8)|29;
	g_scl_handle = gp_gpio_request(pin,"ppu_pin");
	printk("handle=%d\n", g_scl_handle);
	gp_gpio_set_direction(g_scl_handle,GPIO_DIR_OUTPUT);
	gp_gpio_set_driving_current(g_scl_handle, 1);
	gp_gpio_set_value(g_scl_handle, 1);        
	#endif
	
	#if 1
		#if 0
		 #if PPU_HARDWARE_MODULE == MODULE_ENABLE
		  nRet = request_irq(VIC_PPU,gp_ppu_irq_handler,IRQF_DISABLED,"PPU_IRQ",NULL);
		  gpHalPPUIrqCtrl(1);
		 #endif
		#elif 0
		 #if PPU_HARDWARE_MODULE == MODULE_ENABLE
		  nRet = request_irq(VIC_PPU,gp_ppu_irq_handler,SA_SHIRQ,"PPU_IRQ",NULL);
		  gpHalPPUIrqCtrl(1);
		 #endif
		#else
		 #if PPU_HARDWARE_MODULE == MODULE_ENABLE
		  nRet = request_irq(VIC_PPU,gp_ppu_irq_handler,IRQF_SHARED,"PPU_IRQ",&ppu_devices);		 
		  gpHalPPUIrqCtrl(1);
		 #endif
		#endif
  #endif
  
	return nRet;
}
EXPORT_SYMBOL(gp_ppu_init);

/**
 * @brief 		PPU module enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_set_enable(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{

	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_ENABLE;
	} else {
		p_register_set->ppu_enable &= ~PPU_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_set_enable);

/**
 * @brief 		PPU text character0 transparent enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_char0_set_transparent(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= CHAR0_TRANSPARENT_ENABLE;
	} else {
		p_register_set->ppu_enable &= ~CHAR0_TRANSPARENT_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_char0_set_transparent);

/**
 * @brief 		PPU line or frame buffer calculate enable function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=from top to bottom 1=from bottom to top.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_bottom_up_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= TX_BOT2UP;
	} else {
		p_register_set->ppu_enable &= ~TX_BOT2UP;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_bottom_up_set_mode);

/**
 * @brief 		PPU tv display resolution set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=QVGA 1=VGA.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_vga_set_mode(
PPU_REGISTER_SETS *p_register_set,
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_VGA;
	} else {
		p_register_set->ppu_enable &= ~PPU_VGA;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_vga_set_mode);

/**
 * @brief 		PPU tv display interlace set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=interlace 1=non-interlace.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_non_interlace_set(
PPU_REGISTER_SETS *p_register_set,
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_VGA_NONINTL;
	} else {
		p_register_set->ppu_enable &= ~PPU_VGA_NONINTL;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_non_interlace_set);

/**
 * @brief 		PPU line or frame buffer mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	enable [in]: enable:0=frame buffer mode disable 1= frame buffer mode enable.
* @param 	select [in]: select: enable:0(select:0=TV is line mode and TFT is frame mode) 1=both TV and TFT are frame buffer mode.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_frame_buffer_set_mode(
PPU_REGISTER_SETS *p_register_set, 
unsigned int enable, 
unsigned int select
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (enable) {
		p_register_set->ppu_enable |= PPU_FRAME_BASE;
	} else {
		p_register_set->ppu_enable &= ~PPU_FRAME_BASE;
	}

	if (select) {
		p_register_set->ppu_enable |= FB_SEL1;
	} else {
		p_register_set->ppu_enable &= ~FB_SEL1;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_frame_buffer_set_mode);

/**
 * @brief 		PPU line or frame buffer color mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	format [in]: format:0=yuyv or rgbg mode disable 1= yuyv or rgbg mode enable.
* @param 	mono [in]: mono: format:0(mono:0=RGB565 1=Mono 2=4-color 3=16-color) 1(mono:0=RGBG 1=YUYV 2=RGBG 3=YUYV).
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_fb_format_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int format, 
unsigned int mono
)
{
	if (!p_register_set || mono>3) {
		return -ENOIOCTLCMD;
	}

	if (format) {
		p_register_set->ppu_enable |= PPU_RGBG;
	} else {
		p_register_set->ppu_enable &= ~PPU_RGBG;
	}
	p_register_set->ppu_enable &= ~MASK_FB_MONO;
	p_register_set->ppu_enable |= (mono<<B_FB_MONO) & MASK_FB_MONO;

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_fb_format_set);

/**
 * @brief 		PPU image date rom save mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_save_rom_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= SAVE_ROM_ENABLE;
	} else {
		p_register_set->ppu_enable &= ~SAVE_ROM_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_save_rom_set);

/**
 * @brief 		PPU TFT resolution set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:see the constants defined.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_resolution_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>7) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_enable &= ~MASK_TFT_SIZE;
	p_register_set->ppu_enable |= (value<<B_TFT_SIZE) & MASK_TFT_SIZE;

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_resolution_set);

/**
 * @brief 		PPU test and sprite color type mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value: 0=BGRG/VYUY 1=GBGR/YVYU 2=RGBG/UYVY 3=GRGB/YUYV, value[2]:0=UV is unsigned(YCbCr) 1=UV is signed(YUV).
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_yuv_type_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>7) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_enable &= ~MASK_YUV_TYPE;
	p_register_set->ppu_enable |= (value<<B_YUV_TYPE) & MASK_YUV_TYPE;

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_yuv_type_set);

/**
 * @brief 		PPU color mapping mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	enable [in]: enable:0=disable color mapping 1=enable color mapping, value: 32-bit address of color mapping table.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_color_mapping_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int enable, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (enable) {
		p_register_set->ppu_enable |= PPU_CM_ENABLE;

		p_register_set->color_mapping_ptr = value;
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_COLOR;
	} else {
		p_register_set->ppu_enable &= ~PPU_CM_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_color_mapping_set);

/**
 * @brief 		PPU tft long burst mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable TFT long burst 1=enable TFT long burst.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_tft_long_burst_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)	
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_TFT_LONG_BURST;
	} else {
		p_register_set->ppu_enable &= ~PPU_TFT_LONG_BURST;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_tft_long_burst_set);

/**
 * @brief 		PPU long burst mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable PPU long burst 1=enable PPU long burst.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_long_burst_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_LONG_BURST;
	} else {
		p_register_set->ppu_enable &= ~PPU_LONG_BURST;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_long_burst_set);

/**
 * @brief 		PPU blend4 mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0~3 level.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_blend4_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>3) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_blending_level = (unsigned short) value;

	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_blend4_set);

/**
 * @brief 		PPU rgb565 or yuyv and rgbg mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	enable [in]: enable:0=disable 1=enable.
* @param 	value [in]: value:0~0xFFFF.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_rgb565_transparent_color_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int enable, 
unsigned int value
)
{
	if (!p_register_set || value>0xFFFFFF) {
		return -ENOIOCTLCMD;
	}
	if (enable) {
		p_register_set->ppu_rgb565_transparent_color = (1<<24) | value;
	} else {
		p_register_set->ppu_rgb565_transparent_color = value;
	}

	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_rgb565_transparent_color_set);

/**
 * @brief 		PPU fade effect set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0~255 level.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_fade_effect_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set || value>0xFF) {
		return -ENOIOCTLCMD;
	}

	p_register_set->ppu_fade_control = (unsigned short) value;

	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_fade_effect_set);

/**
 * @brief 		PPU window mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	window_index [in]: windows_index:0(window 1)~3(window 4).
* @param 	window_x[in]: window_x:mask + start_x + end_x.
* @param 	window_x[in]: window_y:start_y + end_y.
* @return 	SUCCESS/ERROR_ID.
 */
signed int 
gp_ppu_window_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int window_index, 
unsigned int window_x, 
unsigned int window_y
)
{
	if (!p_register_set || window_index>3) {
		return -ENOIOCTLCMD;
	}

	switch (window_index) {
	case 0:			// Window 1
		p_register_set->ppu_window1_x = window_x;
		p_register_set->ppu_window1_y = window_y;
		break;
	case 1:			// Window 2
		p_register_set->ppu_window2_x = window_x;
		p_register_set->ppu_window2_y = window_y;
		break;
	case 2:			// Window 3
		p_register_set->ppu_window3_x = window_x;
		p_register_set->ppu_window3_y = window_y;
		break;
	case 3:			// Window 4
		p_register_set->ppu_window4_x = window_x;
		p_register_set->ppu_window4_y = window_y;
		break;
	}

	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_window_set);

/**
 * @brief 		PPU palette mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	p1024 [in]: p1024:0=disable 1=enable.
* @param 	type [in]: type: 0:text and sprite share for type 16,1:text and sprite palette independent for type 16 ,2: text and sprite share for type 25,3: text and sprite palette independent for type 25.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_palette_type_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int p1024, 
unsigned int type
)
{
	if (!p_register_set || type>0x3) {
		return -ENOIOCTLCMD;
	}

	if (p1024) {
		p_register_set->ppu_palette_control = (1<<4) | type;
	} else {
		p_register_set->ppu_palette_control = type;
	}

	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_palette_type_set);

/**
 * @brief 		PPU palette ram address set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	bank [in]: bank:0(palette0)~3(palette3).
* @param 	value [in]: value: 32-bit address of palette ram buffer.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_palette_ram_ptr_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int bank, 
unsigned int value
)
{
	unsigned int temp;
	
	if (!p_register_set || bank>3) {
		return -ENOIOCTLCMD;
	}
	
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)value);
	if (bank == 0) {
		p_register_set->ppu_palette0_ptr = temp;
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_PALETTE0;
	} else if (bank == 1) {
		p_register_set->ppu_palette1_ptr = temp;
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_PALETTE1;
	} else if (bank == 2) {
		p_register_set->ppu_palette2_ptr = temp;
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_PALETTE2;
	} else if (bank == 3) {
		p_register_set->ppu_palette3_ptr = temp;
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_PALETTE3;
	}

	return 0;
}
EXPORT_SYMBOL(gp_ppu_palette_ram_ptr_set);

/**
 * @brief 		PPU frame buffer address set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	buffer [in]: buffer: 32-bit address of frame buffer.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_frame_buffer_add(
PPU_REGISTER_SETS *p_register_set, 
unsigned int buffer
)
{
	unsigned int temp;
	
	if (!p_register_set || !buffer) {
		return -ENOIOCTLCMD;
	}	
  temp = (unsigned int)gp_user_va_to_pa((unsigned short *)buffer);
  #if PPU_HARDWARE_MODULE == MODULE_ENABLE
	if(gpHalPPUFramebufferAdd((unsigned int *)temp,(unsigned int *)buffer)) 
	{		// Add frame buffer to PPU driver layer
		return 0;
	}
  #endif

	if ((buffer & 0x3F) && (p_register_set->ppu_enable & PPU_TFT_LONG_BURST)) {
		// Disable long burst mode when frame buffer is not 64-byte alignment
		p_register_set->ppu_enable &= ~PPU_TFT_LONG_BURST;
		p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;
	}

	return 0;
}
EXPORT_SYMBOL(gp_ppu_frame_buffer_add);

/**
 * @brief 		PPU deflicker set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable deflicker function 1=enable deflicker function.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_deflicker_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_enable |= PPU_DEFEN_ENABLE;
	} else {
		p_register_set->ppu_enable &= ~PPU_DEFEN_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_deflicker_set);

/**
 * @brief 		PPU sprite addrx2 mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sp_addrx2_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= SP_ADDRX2_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~SP_ADDRX2_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_sp_addrx2_set);

/**
 * @brief 		This function returns when PPU registers are updated, it will not wait for PPU frame buffer output to complete.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_go_without_wait(
PPU_REGISTER_SETS *p_register_set
)
{
     signed int result=0;

    if(PPU_GO_REENTRY)
    {
       PPU_GO_REENTRY=FALSE;
     #if PPU_HARDWARE_MODULE == MODULE_ENABLE
	result = gpHalPPUGo(p_register_set, 0, 0);
	if (result == 0) {
		gp_ppu_text_number_array_update_flag_clear();
	}
    #else
       result=-ENOIOCTLCMD;
    #endif
	PPU_GO_REENTRY=TRUE;
    }
	
    return result;
}
EXPORT_SYMBOL(gp_ppu_go_without_wait);

/**
 * @brief 		This function returns when PPU registers are updated, it will not wait for PPU frame buffer output to complete.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/

signed int 
gp_ppu_go(
PPU_REGISTER_SETS *p_register_set
)
{
     signed int result=0;

    if(PPU_GO_REENTRY)
    {
       PPU_GO_REENTRY=FALSE;
     #if PPU_HARDWARE_MODULE == MODULE_ENABLE
	    #if 0
	     result = gpHalPPUGo(p_register_set, 1, 0);
	    #else
	     result = gpHalPPUGo(p_register_set, 0, 0);
	    #endif
	    if (result == 0) {
		    gp_ppu_text_number_array_update_flag_clear();
	    }
     #else
       result=-ENOIOCTLCMD;
     #endif
	    PPU_GO_REENTRY=TRUE; 
    }

    return result;
}
EXPORT_SYMBOL(gp_ppu_go);

/**
 * @brief 		This function returns when PPU registers are updated and operation is done.
* @param 	p_register_set [in]: PPU struct value set.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_go_and_wait_done(
PPU_REGISTER_SETS *p_register_set
)
{
     signed int result=0;

    if(PPU_GO_REENTRY)
    {
       PPU_GO_REENTRY=FALSE;
     #if PPU_HARDWARE_MODULE == MODULE_ENABLE
	    #if 0
	     result = gpHalPPUGo(p_register_set, 1, 1);
	    #else
	     result = gpHalPPUGo(p_register_set, 0, 0);
	     
	    #endif
	    if (result == 0) {
		     gp_ppu_text_number_array_update_flag_clear();
	     }
     #else
       result=-ENOIOCTLCMD;
     #endif	 
	PPU_GO_REENTRY=TRUE; 
    }
	
    return result;
}
EXPORT_SYMBOL(gp_ppu_go_and_wait_done);

/**
 * @brief 	PPU dual blending set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_dual_blend_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= DUAL_BLEND_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~DUAL_BLEND_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_dual_blend_set);

/**
 * @brief 	PPU free size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	INTL [in]: INTL:0=TFT Display 1=TV Display.
* @param 	H_size [in]: H_size:16~1920.
* @param 	V_size [in]: V_size:1~1024.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_free_size_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned short INTL, 
unsigned short H_size, 
unsigned short V_size
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

    p_register_set->ppu_free_mode = (((INTL << B_FREE_INIT) & MASK_FREE_INIT_SIZE)|((H_size << B_FREE_H_SIZE) & MASK_FREE_H_SIZE )|(V_size & MASK_FREE_V_SIZE));

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_free_size_set);

/**
 * @brief 	PPU rgba color mode for text set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_rgba_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TEXT_RGBA_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TEXT_RGBA_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_text_rgba_set);

/**
 * @brief 	PPU alpha color mode for text set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_alpha_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TXT_ALPHA_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TXT_ALPHA_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_text_alpha_set);

/**
 * @brief 	PPU rgba color mode for sprite set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_sprite_rgba_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= SPRITE_RGBA_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~SPRITE_RGBA_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_sprite_rgba_set);

/**
 * @brief 	PPU new specialbmp mode for sprite set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_new_specialbmp_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TXT_NEWCMP_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TXT_NEWCMP_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_text_new_specialbmp_set);

/**
 * @brief 	PPU new compression mode for sprite set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_text_new_compression_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TXT_NEWSPECBMP_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TXT_NEWSPECBMP_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_text_new_compression_set);

/**
 * @brief 	PPU delaygo for frame mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable, The PPU_GO will not be blocked by any condition. 1=enable, The PPU_GO will be delayed until both TVFBI_UPD and TFTFBI_UPD is 1.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_delgo_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TXT_DELGO_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TXT_DELGO_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_delgo_set);

/**
 * @brief 	PPU tftvtq mode set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0= TFT's frame buffer and display size is the same. 1= TFT's frame buffer must be set to 1(PPU_SIZE is 1) and the display size is QVGA.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_tftvtq_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TXT_TFTVTQ_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TXT_TFTVTQ_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_tftvtq_set);

/**
 * @brief 	PPU tv long burst set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_tv_long_burst_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

	if (value) {
		p_register_set->ppu_misc |= TXT_TVLB_ENABLE;
	} else {
		p_register_set->ppu_misc &= ~TXT_TVLB_ENABLE;
	}

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_tv_long_burst_set);

/**
 * @brief 	PPU interpolation set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	value [in]: value:0=disable 1=enable.
* @param 	set_value [in]: value VGA_EN is 0:0= No interpolation function.1= Do QVGA to VGA up-scaling.2= Do QVGA to D1 up-scaling., VGA_EN is 1:0= No interpolation function.1= No interpolation function.2= Do QVGA to D1 up-scaling.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_interpolation_set(
PPU_REGISTER_SETS *p_register_set, 
unsigned int value,
unsigned int set_value
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}
	
	if (value)
	{
		  if (set_value == 0) {
			p_register_set->ppu_misc |= TXT_INTPMODE0;
		  } else if(set_value == 1){
			p_register_set->ppu_misc |= TXT_INTPMODE1;
		  } else if(set_value == 2){
		    p_register_set->ppu_misc |= TXT_INTPMODE2;
		  } else if(set_value == 3){
		    p_register_set->ppu_misc |= TXT_INTPMODE3;
		  } 
       }
       else
       {
      		  p_register_set->ppu_misc &= ~TXT_INTPMODE3;    
       }
    
	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;
}
EXPORT_SYMBOL(gp_ppu_interpolation_set);

/**
 * @brief 	PPU fifo size set function.
* @param 	p_register_set [in]: PPU struct value set.
* @param 	line_start [in]: line_start[9:0]: Line start value when restart PPU for multi-time PPU process.
* @param 	fifo_type [in]: fifo_type: 0 = No frame buffer output mode. 1 =8 lines FIFO mode. 2 =16 lines FIFO mode. 3 =32 lines FIFO mode.
* @param 	addr_offset [in]: addr_offset[13:0]: Address offset of each line, this register Is in byte unit.  This value will be add to the output.
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_ppu_frame_buffer_output_fifo_set 
(PPU_REGISTER_SETS *p_register_set, 
unsigned short line_start, 
unsigned short fifo_type, 
unsigned short addr_offset
)
{
	if (!p_register_set) {
		return -ENOIOCTLCMD;
	}

       p_register_set->ppu_frame_buffer_fifo = (((line_start << B_LINE_START) & MASK_LINE_START_SIZE)|((fifo_type << B_FIFO_TYPE) & MASK_FIFO_TYPE_SIZE)|(addr_offset & MASK_ADDR_OFFSET_SIZE));

	// Notify PPU driver to update PPU registers
	p_register_set->update_register_flag |= C_UPDATE_REG_SET_PPU;

	return 0;	
}
EXPORT_SYMBOL(gp_ppu_frame_buffer_output_fifo_set);

/**
 * @brief 	PPU fifo go for frame mode function.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_fifo_go_and_wait_done(
void
)
{
    signed int fifo_go_status=0;

    if(PPU_FIFO_GO_REENTRY)
    {
       PPU_GO_REENTRY=FALSE;	   
    #if PPU_HARDWARE_MODULE == MODULE_ENABLE
       fifo_go_status=gpHalPPUFifogowithdone();
    #else
       fifo_go_status=-ENOIOCTLCMD
    #endif
       PPU_GO_REENTRY=TRUE;
    }	
	return fifo_go_status;
}
EXPORT_SYMBOL(gp_ppu_fifo_go_and_wait_done);

/**
 * @brief 	PPU irq function.
* @return 	SUCCESS/ERROR_ID.
*/
irqreturn_t 
gp_ppu_irq_handler(
	int irq, 
	void *dev_id
)
{
	      signed int ret=-1;
	      
	      #if PPU_HARDWARE_MODULE == MODULE_ENABLE
	       ret=gpHalPPUIsr();
        #endif
	      //printk("irq = %x\n", ret);      
	      if(ret!=PPU_MODULE_PPU_VBLANK)
	         return IRQ_NONE;  
    	  
    	  #if 0
    	  printk("frame_counter = %d\n", frame_counter);
    	  #endif
    	  
    	  #if PPU_GPIO_SET == 1
      	  if(frame_counter)
      	  {
      	     gp_gpio_set_value(g_scl_handle,1);
      	     frame_counter=0;
      	  }   
      	  else
      	  {
      	     gp_gpio_set_value(g_scl_handle,0);
      	     frame_counter=1;
      	  }
    	  #elif PPU_GPIO_SET == 2
					gp_gpio_set_value(g_scl_handle,(frame_counter&0x1));
					frame_counter++;
    	  #endif 
        ppu_devices->done = 1;
        wake_up_interruptible(&ppu_devices->ppu_wait_queue);
	      
	      return IRQ_HANDLED;	
}

/**
* @brief	ppu poll function
* @param	filp [in]:
* @param	poll [in]:
* @return	SUCCESS/ERROR_ID
*/
static unsigned int 
gp_ppu_poll(
	struct file *filp, 
	struct poll_table_struct *poll
)
{
	unsigned int mask = 0;

	/* wait_event_interruptible(p_cdsp_dev->cdsp_wait_queue, (p_cdsp_dev->done != 0)); */
	poll_wait(filp, &ppu_devices->ppu_wait_queue, poll);
	if(ppu_devices->done != 0)
	{
		ppu_devices->done = 0;
		mask = POLLIN | POLLRDNORM;
	}
	
	return mask;
}

/**
 * @brief 		PPU memory copy.
* @param 	pDest [in]: Target address.
* @param 	pSrc:[in]: Source address.
* @param 	nBytes [in]: data length.
* @param 	mem_copy_8_16_32:[in]: move data length of once.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_cpu_cpy_mem
(unsigned int *pDest, 
unsigned int *pSrc, 
unsigned int nBytes, 
unsigned short mem_copy_8_16_32
)
{
    signed int  i;
    unsigned int  *pPtrDest, *pPtrSrc;
    char    *pPtr8Dest, *pPtr8Src;
    unsigned short  *pPtr16Dest, *pPtr16Src;

    if (!pDest) {
        return STATUS_FAIL;
    }
    
    if(mem_copy_8_16_32==32){
       //  32-bit memory copy
       pPtrDest = pDest;
       pPtrSrc = pSrc;
       for (i=0;i<(nBytes/sizeof (unsigned int));i++) {
           *(pPtrDest++) = *(pPtrSrc++);
       }
    }else if(mem_copy_8_16_32==16){    
       //  32-bit memory copy
       pPtr16Dest = (unsigned short *)pDest;
       pPtr16Src = (unsigned short *)pSrc;
       for (i=0;i<(nBytes/sizeof (unsigned short));i++) {
           *(pPtr16Dest++) = *(pPtr16Src++);
       }   
    }else{   
       //  8-bit memory copy
       pPtr8Dest = (char *)pDest;
       pPtr8Src = (char *)pSrc;
       for (i=0;i<nBytes;i++) {
           *(pPtr8Dest++) = *(pPtr8Src++);
       }
    }  
	
    return STATUS_OK;
}
EXPORT_SYMBOL(gp_cpu_cpy_mem);


/**
 * @brief 	PPU frame buffer get for display.
* @return 	SUCCESS/ERROR_ID/FRAME BUFFER VIRTUAL ADDRESS.
*/
signed int 
gp_ppu_get_frame_buffer(
void
)
{
		signed int temp;
	
		#if PPU_HARDWARE_MODULE == MODULE_ENABLE
		  temp=gpHalPPUframebufferGet();
		#else
		  temp=-1;
		#endif 
		
	  return temp;
}
EXPORT_SYMBOL(gp_ppu_get_frame_buffer);

/**
 * @brief 	PPU frame buffer release.
 * @param 	buffer [in]: release Target address.
* @return 	SUCCESS/ERROR_ID.
*/
signed int 
gp_ppu_release_frame_buffer(
unsigned int buffer
)
{
		signed int temp,ret;
	
	    temp = (unsigned int)gp_user_va_to_pa((unsigned short *)buffer);
		#if PPU_HARDWARE_MODULE == MODULE_ENABLE
		  ret=gpHalPPUframebufferRelease((unsigned int)temp);
		#else
		  ret=-1;
		#endif 
		
	  return ret;
}
EXPORT_SYMBOL(gp_ppu_release_frame_buffer);

/**
 * \brief Open ppu device
 */
int
gp_ppu_open(
	struct inode *inode,
	struct file *filp
)
{
	/* Success */
	filp->private_data = ppu_devices;
	printk(KERN_WARNING "PPU open \n");

	return 0;
}
int
gp_ppu_release(
	struct inode *inode,
	struct file *filp
)
{
	/* Success */
	printk(KERN_WARNING "PPU release \n");
	
	return 0;
}

#if 1
static PPU_REGISTER_SETS *ppu_register_set;
#else
static PPU_REGISTER_SETS ppu_register_structure;
static PPU_REGISTER_SETS *ppu_register_set;	
#endif

int
gp_ppu_ioctl(
	struct inode *inode,
	struct file *filp,
	unsigned int cmd,
	unsigned long arg
)
{
  int ret = 0;
  unsigned int temp;

  /* initial ppu register parameter set structure */
  #if 1
  ppu_register_set = (PPU_REGISTER_SETS *)arg;
  #else
  copy_from_user(&ppu_register_structure, (void __user*)arg, sizeof(PPU_REGISTER_SETS));
  /* initial ppu register parameter set structure */
	ppu_register_set = (PPU_REGISTER_SETS *) &ppu_register_structure;	  
  #endif
  
  switch(cmd)
  {
      case PPUIO_SET_INIT:
           ret=gp_ppu_init(ppu_register_set);
           break;
           
      case PPUIO_SET_ENABLE:     
           ret=gp_ppu_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SET_CHAR0_TRANSPARENT:     
           ret=gp_ppu_char0_set_transparent(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SET_BOTTOM_UP:     
           ret=gp_ppu_bottom_up_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SET_VGA:     
           ret=gp_ppu_vga_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SET_NON_INTERLACE:     
           ret=gp_ppu_non_interlace_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SET_FRAME_BUFFER:
           ret=gp_ppu_frame_buffer_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
           
      case PPUIO_SET_FB_FORMAT:     
           ret=gp_ppu_fb_format_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
      
      case PPUIO_SET_SAVE_ROM:     
           ret=gp_ppu_save_rom_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SET_RESOLUTION:     
           ret=gp_ppu_resolution_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SET_YUV_TYPE:     
           ret=gp_ppu_yuv_type_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
           
      case PPUIO_SET_COLOR_MAPPING:     
           ret=gp_ppu_color_mapping_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;             

      case PPUIO_SET_TFT_LONG_BURST:
           ret=gp_ppu_tft_long_burst_set(ppu_register_set,(unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SET_LONG_BURST:     
           ret=gp_ppu_long_burst_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SET_BLEND4:     
           ret=gp_ppu_blend4_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;           
 
      case PPUIO_SET_RGB565_TRANSPARENT_COLOR:     
           ret=gp_ppu_rgb565_transparent_color_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;  
 
      case PPUIO_SET_FADE_EFFECT:     
           ret=gp_ppu_fade_effect_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
           
      case PPUIO_SET_WINDOW:     
           ret=gp_ppu_window_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode,
           (unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode,(unsigned int)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;            
           
      case PPUIO_SET_PALETTE_TYPE:
           ret=gp_ppu_palette_type_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
           
      case PPUIO_SET_PALETTE_RAM:     
           ret=gp_ppu_palette_ram_ptr_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode,
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;
      
      case PPUIO_ADD_FRAME_BUFFER:  
           ret=gp_ppu_frame_buffer_add(ppu_register_set,(unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);   
           break;           
 
      case PPUIO_SET_DEFLICKER:     
           ret=gp_ppu_deflicker_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SET_SP_ADDRX2:     
           ret=gp_ppu_sp_addrx2_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_PPUGO_WITHOUT_WAIT:     
           ret=gp_ppu_go_without_wait(ppu_register_set);
           break; 
                   
      case PPUIO_PPUGO:     
           ret=gp_ppu_go(ppu_register_set);
           break; 
           
      case PPUIO_PPUGO_AND_WAIT_DONE:     
           ret=gp_ppu_go_and_wait_done(ppu_register_set);
           break;            
           
      case PPUIO_SET_DUAL_BLEND:
           ret=gp_ppu_dual_blend_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SET_FREE_SIZE:     
           ret=gp_ppu_free_size_set(ppu_register_set, (unsigned short)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned short)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned short)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;
      
      case PPUIO_SET_TEXT_RGBA:     
           ret=gp_ppu_text_rgba_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SET_TEXT_ALPHA:     
           ret=gp_ppu_text_alpha_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SET_SPRITE_RGBA:     
           ret=gp_ppu_sprite_rgba_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SET_TEXT_NEW_SPECIALBMP:     
           ret=gp_ppu_text_new_specialbmp_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SET_TEXT_NEW_COMPRESSION:     
           ret=gp_ppu_text_new_compression_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SET_PPU_DELGO:     
           ret=gp_ppu_delgo_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SET_TFTVTQ:
           ret=gp_ppu_tftvtq_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SET_TV_LONG_BURST:     
           ret=gp_ppu_tv_long_burst_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SET_INTERPOLATION:     
           ret=gp_ppu_interpolation_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;           
 
      case PPUIO_SET_FRAME_BUFFER_OUTPUT_FIFO:     
           ret=gp_ppu_frame_buffer_output_fifo_set(ppu_register_set, (unsigned short)ppu_register_set->ppu_set.PPU_hsize_mode, 
           (unsigned short)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned short)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;  
 
      case PPUIO_SET_PPUFIFO_GO_AND_WAIT_DONE:     
           ret=gp_ppu_fifo_go_and_wait_done();
           break;
           
      case PPUIO_SET_PPU_FRAME_BUFFER_GET:     
           ret=gp_ppu_get_frame_buffer();          
           break; 
           
      case PPUIO_SET_PPU_FRAME_BUFFER_RELEASE:              
           ret=gp_ppu_release_frame_buffer((unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;                        
    	         	                        
      // Text Moudle Set
      case PPUIO_TEXT_SET_INIT:
           ret=gp_ppu_text_init(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_TEXT_SET_ENABLE:     
           ret=gp_ppu_text_enable_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
      
      case PPUIO_TEXT_SET_COMPRESS_DISABLE:     
           ret=gp_ppu_text_compress_disable_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_TEXT_SET_MODE:     
           ret=gp_ppu_text_mode_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;  
 
      case PPUIO_TEXT_SET_DIRECT_MODE:     
           ret=gp_ppu_text_direct_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_TEXT_SET_WALLPAPER_MODE:     
           ret=gp_ppu_text_wallpaper_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;            
           
      case PPUIO_TEXT_SET_ATTRIBUTE_SOURCE:
           ret=gp_ppu_text_attribute_source_select(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
           
      case PPUIO_TEXT_SET_HORIZONTAL_MOVE_ENABLE:     
           ret=gp_ppu_text_horizontal_move_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
      
      case PPUIO_TEXT_SET_TEXT_SIZE:     
           ret=gp_ppu_text_size_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;           
 
      case PPUIO_TEXT_SET_CHARACTER_SIZE:     
           ret=gp_ppu_text_character_size_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;  
 
      case PPUIO_TEXT_SET_BITMAP_MODE:     
           ret=gp_ppu_text_bitmap_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
           
      case PPUIO_TEXT_SET_COLOR:     
           ret=gp_ppu_text_color_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_value_mode);
           break;             

      case PPUIO_TEXT_SET_PALETTE:
           ret=gp_ppu_text_palette_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_value_mode);
           break;
           
      case PPUIO_TEXT_SET_SEGMENT:     
           ret=gp_ppu_text_segment_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;
      
      case PPUIO_TEXT_SET_ATTRIBUTE_ARRAY_PTR:     
           ret=gp_ppu_text_attribute_array_set_ptr(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;           
 
      case PPUIO_TEXT_SET_NUMBER_ARRAY_PTR:     
           ret=gp_ppu_text_number_array_set_ptr(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;  
 
      case PPUIO_TEXT_CALCULATE_NUMBER_ARRAY:     
           ret=gp_ppu_text_calculate_number_array(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_vsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break; 
           
      case PPUIO_TEXT_SET_POSITION:     
           ret=gp_ppu_text_position_set(ppu_register_set, (unsigned int)(ppu_register_set->ppu_set.PPU_enable_mode), 
           (unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;            
           
      case PPUIO_TEXT_SET_OFFSET:
           ret=gp_ppu_text_offset_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;
           
      case PPUIO_TEXT_SET_DEPTH:     
           ret=gp_ppu_text_depth_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
      
      case PPUIO_TEXT_SET_BLEND:  
           ret=gp_ppu_text_blend_set(ppu_register_set,(unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned int)((ppu_register_set->ppu_set.PPU_select_type_value_mode & PPU_VALUE_RANGE_SET)>>PPU_DATA_OFFSET), 
           (unsigned int)(ppu_register_set->ppu_set.PPU_select_type_value_mode & 0xFF));   
           break;           
 
      case PPUIO_TEXT_SET_FLIP:     
           ret=gp_ppu_text_flip_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;  
 
      case PPUIO_TEXT_SET_SINE_COSINE:     
           ret=gp_ppu_text_sine_cosine_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned short)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned short)ppu_register_set->ppu_set.PPU_vsize_mode);
           break; 
           
      case PPUIO_TEXT_SELECT_WINDOW:     
           ret=gp_ppu_text_window_select(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
                   
      case PPUIO_TEXT_SET_SPECIAL_EFFECT:     
           ret=gp_ppu_text_special_effect_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
           
      case PPUIO_TEXT_SET_VERTICAL_COMPRESS:     
           ret=gp_ppu_text_vertical_compress_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_value_mode);
           break;            
           
      case PPUIO_TEXT_SET_HORIZONTAL_MOVE_PTR:
           ret=gp_ppu_text_horizontal_move_ptr_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;
           
      case PPUIO_TEXT_SET_HORIZONTAL_COMPRESS_PTR:     
           ret=gp_ppu_text1_horizontal_compress_ptr_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;
      
      case PPUIO_TEXT_SET_ROTATE_ZOOM:     
           ret=gp_ppu_text_rotate_zoom_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (signed short)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (signed short)ppu_register_set->ppu_set.PPU_select_type_value_mode);
           break;           
 
      case PPUIO_TEXT3_SET_25D:     
           ret=gp_ppu_text3_25d_set(ppu_register_set, (signed short)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (signed short *)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;  
 
      case PPUIO_TEXT3_SET_25D_Y_COMPRESS:     
           ret=gp_ppu_text3_25d_y_compress_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
           
      case PPUIO_TEXT3_SET_25D_COSINEBUF:     
           ret=gp_ppu_text25D_cossinebuf_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;  
 
      case PPUIO_TEXT_SET_INTERPOLATION:     
           ret=gp_ppu_text_interpolation_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break; 
           
      case PPUIO_TEXT_SET_COLOR_MASK:     
           ret=gp_ppu_text_color_mask_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;            
           
      case PPUIO_TEXT_UPDATE_NUMBER_ARRAY:
           gp_ppu_text_number_array_update((PnTX_Num_Arr)&ppu_register_set->text_info);
           break;
     
      // Sprite Moudle Set             
      case PPUIO_SPRITE_SET_INIT:
           ret=gp_ppu_sprite_init(ppu_register_set);
           break;
           
      case PPUIO_SPRITE_SET_ENABLE:     
           ret=gp_ppu_sprite_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SPRITE_SET_COORDINATE:     
           ret=gp_ppu_sprite_set_coordinate(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SPRITE_SET_BLEND:     
           ret=gp_ppu_sprite_blend_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_DIRECT:     
           ret=gp_ppu_sprite_direct_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_SET_ZOOM_ENABLE:     
           ret=gp_ppu_sprite_zoom_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SPRITE_SET_ROTATE_ENABLE:
           ret=gp_ppu_sprite_rotate_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_MOSAIC_ENABLE:     
           ret=gp_ppu_sprite_mosaic_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SPRITE_SET_NUMBER:     
           ret=gp_ppu_sprite_number_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SPRITE_SET_SPECIAL_EFFECT:     
           ret=gp_ppu_sprite_special_effect_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_COLOR_DITHER:     
           ret=gp_ppu_sprite_color_dither_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_SET_25D:     
           ret=gp_ppu_sprite_25d_set_mode(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;             

      case PPUIO_SPRITE_SET_WINDOW:
           ret=gp_ppu_sprite_window_enable_set(ppu_register_set,(unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
                 
      case PPUIO_SPRITE_SET_SEGMENT:     
           ret=gp_ppu_sprite_segment_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;           
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_RAM_PTR:     
           ret=gp_ppu_sprite_attribute_ram_set_ptr(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;  
 
      case PPUIO_SPRITE_SET_SFR:     
           ret=gp_ppu_sprite_sfr_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_SET_LARGE_ENABLE:     
           ret=gp_ppu_large_sprite_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SPRITE_SET_INTERPOLATION_ENABLE:
           ret=gp_ppu_sprite_interpolation_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_GROUP_ENABLE:     
           ret=gp_ppu_sprite_group_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SPRITE_SET_ATTRIBUTE_CDM_ENABLE:  
           ret=gp_ppu_sprite_cdm_attribute_set_enable((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr,
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);   
           break;           
 
      case PPUIO_SPRITE_SET_FRACTION_ENABLE:     
           ret=gp_ppu_sprite_fraction_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_EXTEND_ATTRIBUTE_RAM_PTR:     
           ret=gp_ppu_sprite_extend_attribute_ram_set_ptr(ppu_register_set, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break; 
           
      case PPUIO_SPRITE_SET_EXSP_ENABLE:     
           ret=gp_ppu_exsprite_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
                   
      case PPUIO_SPRITE_SET_EXSP_CDM_ENABLE:     
           ret=gp_ppu_exsprite_cdm_set_enable(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_SET_EXSP_INTERPOLATION_ENABLE:     
           ret=gp_ppu_exsprite_interpolation_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SPRITE_SET_EXSP_LARGE_SIZE_ENABLE:
           ret=gp_ppu_exsprite_large_size_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_EXSP_GROUP_ENABLE:     
           ret=gp_ppu_exsprite_group_set(ppu_register_set, (unsigned short)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SPRITE_SET_EXSP_FRACTION_ENABLE:     
           ret=gp_ppu_exsprite_fraction_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SPRITE_SET_EXSP_NUMBER:     
           ret=gp_ppu_exsprite_number_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_EXSP_START_ADDRESS:     
           ret=gp_ppu_exsprite_start_address_set(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break; 
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_2D_POSITION:     
           ret=gp_ppu_sprite_attribute_2d_position_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (signed short)ppu_register_set->ppu_set.PPU_hsize_mode, (signed short)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;  
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_25D_POSITION:     
           ret=gp_ppu_sprite_attribute_25d_position_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, (POS_STRUCT_PTR)ppu_register_set->ppu_set.PPU_hsize_mode);
           break; 
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_ROTATE:     
           ret=gp_ppu_sprite_attribute_rotate_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_ZOOM:
           ret=gp_ppu_sprite_attribute_zoom_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_COLOR:     
           ret=gp_ppu_sprite_attribute_color_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SPRITE_SET_ATTRIBUTE_FLIP:     
           ret=gp_ppu_sprite_attribute_flip_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_CHARACTER_SIZE:     
           ret=gp_ppu_sprite_attribute_character_size_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_vsize_mode);
           break;  
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_PALETTE:     
           ret=gp_ppu_sprite_attribute_palette_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned short)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned short)ppu_register_set->ppu_set.PPU_select_type_value_mode);
           break;  
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_DEPTH:
           ret=gp_ppu_sprite_attribute_depth_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_BLEND64:     
           ret=gp_ppu_sprite_attribute_blend64_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;
      
      case PPUIO_SPRITE_SET_ATTRIBUTE_BLEND16:     
           ret=gp_ppu_sprite_attribute_blend16_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode);
           break;           
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_WINDOW:     
           ret=gp_ppu_sprite_attribute_window_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_MOSAIC:     
           ret=gp_ppu_sprite_attribute_mosaic_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_CHARNUM:     
           ret=gp_ppu_sprite_attribute_charnum_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_EXSP_ATTRIBUTE_GROUP:     
           ret=gp_ppu_exsprite_group_attribute_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_GROUP:     
           ret=gp_ppu_sprite_group_attribute_set((SpN_EX_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;            
           
      case PPUIO_SPRITE_SET_EXSP_ATTRIBUTE_LARGE_SIZE:
           ret=gp_ppu_exsprite_large_size_attribute_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_ATTRIBUTE_LARGE_SIZE:     
           ret=gp_ppu_sprite_large_size_attribute_set((SpN_EX_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
      
      case PPUIO_SPRITE_SET_EXSP_ATTRIBUTE_INTERPOLATION:     
           ret=gp_ppu_exsprite_interpolation_attribute_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;           
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_INTERPOLATION:     
           ret=gp_ppu_sprite_interpolation_attribute_set((SpN_EX_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;  
 
      case PPUIO_SPRITE_SET_SP_ATTRIBUTE_CDM:     
           ret=gp_ppu_sprite_cdm_attribute_set((SpN_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (CDM_STRUCT *)(unsigned int)ppu_register_set->ppu_set.PPU_hsize_mode);
           break;  
 
      case PPUIO_SPRITE_SET_ATTRIBUTE_COLOR_MASK:
           ret=gp_ppu_sprite_color_mask_attribute_set((SpN_EX_RAM *)ppu_register_set->ppu_set.PPU_buffer_ptr, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
           
      case PPUIO_SPRITE_SET_IMAGE_DATA_INIT:     
           gp_ppu_sprite_image_data_init((unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;
      
      case PPUIO_SPRITE_SET_DISPLAY:     
           gp_ppu_sprite_display_set_init((unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (signed short)ppu_register_set->ppu_set.PPU_hsize_mode, 
           (signed short)ppu_register_set->ppu_set.PPU_vsize_mode, (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;           
 
      case PPUIO_SPRITE_SET_IMAGE_NUMBER:     
           gp_ppu_sprite_image_number_set((unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;  
 
      case PPUIO_SPRITE_SET_DISABLE:     
           gp_ppu_sprite_disable_set((unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
           
      case PPUIO_SPRITE_GET_IMAGE_INFO:     
           Get_ppu_sprite_image_info((unsigned int)ppu_register_set->ppu_set.PPU_enable_mode, 
           (SpN_ptr *)ppu_register_set->ppu_set.PPU_buffer_ptr);
           break;  
 
      case PPUIO_SPRITE_PAINT_SPRITERAM:     
           gp_ppu_paint_spriteram(ppu_register_set, (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_value_mode, 
           (unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break; 
     
      case PPUIO_SPRITE_SET_SPRAM:     
           ret=gp_ppu_spriteram_init(ppu_register_set,(unsigned int)ppu_register_set->ppu_set.PPU_enable_mode);
           break;
                  
      case DISPLAY_NONE_PPU_SET:
           //gp_tv1_start(TVSTD_NTSC_J_NONINTL,TV_QVGA,TV_INTERLACE);
           temp = (unsigned int)gp_user_va_to_pa((unsigned short *)ppu_register_set->ppu_set.PPU_buffer_ptr);
           ppu_reg_set((unsigned char)ppu_register_set->ppu_set.PPU_enable_mode,(unsigned int)temp,
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_mode,
           (unsigned int)ppu_register_set->ppu_set.PPU_select_type_value_mode);
           break;                      		  
 		  
 		  default:
			     ret = -ENOIOCTLCMD;
			     break;                      
  }
	
	return ret;
}


void __exit
gp_ppu_module_exit(
	void
)
{
	dev_t devno = MKDEV(ppu_major, PPU_MINOR);
	cdev_del(&(ppu_devices->c_dev));
	kfree(&ppu_devices);
	unregister_chrdev_region(devno, PPU_NR_DEVS);
  printk(KERN_WARNING "PPU module exit \n");
}


/**
 * \brief Initialize display device
 */
int __init
gp_ppu_module_init(
	void
)
{
	int result;
	dev_t dev;
	int devno;

	result = alloc_chrdev_region(&dev, PPU_MINOR, 1, "PPU");
	if( result<0 )	{
		printk(KERN_WARNING "PPU: can't get major \n");
		return result;
	}
	ppu_major = MAJOR(dev);
	ppu_class = class_create(THIS_MODULE, "PPU");
	
	ppu_devices = kmalloc(sizeof(gp_ppu_dev_t), GFP_KERNEL);
	if(!ppu_devices) {
		printk(KERN_WARNING "PPU: can't kmalloc \n");
		result = -ENOMEM;
		goto fail;
	}
	memset(ppu_devices, 0, sizeof(gp_ppu_dev_t));
	
	devno = MKDEV(ppu_major, PPU_MINOR);
	cdev_init(&(ppu_devices->c_dev), &ppu_fops);
	ppu_devices->c_dev.owner = THIS_MODULE;
	ppu_devices->c_dev.ops = &ppu_fops;
	result = cdev_add(&(ppu_devices->c_dev), devno, 1);
	device_create(ppu_class, NULL, devno, NULL, "ppu%d", 0);
	
	if(result)
		printk(KERN_WARNING "Error adding ppu");
  
  init_waitqueue_head(&ppu_devices->ppu_wait_queue);
  
  #if PPU_HARDWARE_MODULE == MODULE_ENABLE
	ppu_clock_enable(1);
	// Initiate PPU hardware and driver
	gpHalPPUEn(1);
  #endif
	printk(KERN_WARNING "PPU module init\n");
		
	return 0;

fail:
	
	printk(KERN_WARNING "PPU module init failed \n");
	kfree(ppu_devices);
	unregister_chrdev_region(dev, PPU_NR_DEVS);

	return result;

}

module_init(gp_ppu_module_init);
module_exit(gp_ppu_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus PPU Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
	
