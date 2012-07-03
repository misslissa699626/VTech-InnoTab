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
 * @file    hal_clock.c
 * @brief   Implement of clock hardware function
 * @author  Roger Hsu
 */
#include <mach/kernel.h>
#include <mach/hal/hal_common.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_i2c_bus.h>

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <mach/hal/hal_clock.h>
// for test 
#include <mach/clock_mgr/gp_clock_private.h>

#include <mach/hal/sysregs.h>
#include <mach/typedef.h>
#include <mach/hal/hal_common.h>

// for test 
extern int clk_ref_arm_set_rate(struct clk *clk, unsigned long rate);

static UINT32
calPanelDivFreqInput(
	UINT32 aInFreq
)
{
	struct clk *clk;
	UINT32 ceva_ref_rate;
	UINT32 ans = 0, currFreq, ansNext = 0;
	clk = clk_get(NULL, "clk_ref_ceva");
	ceva_ref_rate = clk_get_rate(clk);
	currFreq = ceva_ref_rate;
    currFreq += (aInFreq - 1);
    currFreq /= aInFreq;
    ans = ceva_ref_rate  / currFreq;
    ansNext = ceva_ref_rate / (currFreq - 1);
	if(XTAL_RATE == aInFreq){
		return 0x00010001;
	}
	if(XTAL_RATE > aInFreq){
		return 0x00010000 | (XTAL_RATE / aInFreq);
	}

    if(ans > aInFreq){
        ans -= aInFreq;
    }
    else{
        ans = aInFreq - ans;
    }

    if(ansNext > aInFreq){
        ansNext -= aInFreq;
    }
    else{
        ansNext = aInFreq - ansNext;
    }


   printk("[calPanelDivFreqInput] ans %d ans2 is %d.\n", ans, ansNext);
   printk("[calPanelDivFreqInput] in freq is %d dst freq is %d.\n", ceva_ref_rate, aInFreq);
    if(ans > ansNext){
		 printk("[calPanelDivFreqInput] div ratio is %d.\n", currFreq - 1);
        return (currFreq - 1);
    }
    else{
		 printk("[calPanelDivFreqInput] div ratio is %d.\n", currFreq);
        return currFreq;
    }
}

#define		DISP_TYPE_MSK						0x00030000
#define		DISP_TYPE_RGB565					0x00000000
#define		DISP_TYPE_RGB666					0x00010000
#define		DISP_TYPE_LCM						0x00020000
#define		DISP_TYPE_YPbPr						0x00030000

SINT32
gpHalLcdGetFreq(
	void
)
{
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	UINT32 val, freq = 0;
	UINT32 ceva_ref_rate;
	UINT8 clockSrc;
	struct clk *clk;


	val = pscuaReg->scuaLcdClkCfg;
	clockSrc = (val >> 16)&0x3;
	if (clockSrc == 0) {
		clk = clk_get(NULL, "clk_ref_ceva");
		ceva_ref_rate = clk_get_rate(clk);
		freq = ceva_ref_rate / ((val &0xff) + 1);
	}
	else if (clockSrc == 1)
		freq = XTAL_RATE/((val &0xff) + 1);

	return freq;

}

/**
* @brief 	Hal lcd clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkLcdSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	UINT32 ratio, clockSrc, val;
	UINT32 realParentRate = parentRate;

	/* @todo : remove, using normal enable function */
	pscuaReg->scuaLcdClkCfg &= ~(LCD_CLK_EN);

	val = pscuaReg->scuaLcdClkCfg;
	clockSrc = (val & (0x7 << HAL_LCD_CLK_SEL_OFFSET));
	if ( clockSrc == HAL_LCD_CLK_XTAL) {
		//using external 27MHZ
		realParentRate = XTAL_RATE;
	}
	else if (clockSrc == HAL_LCD_CLK_USBPHY) {
		//using USBPHY 96MHz, USB must enable first.
		realParentRate = USBPHY_RATE;
	}
	
	ratio = ((realParentRate / rate) -1 ) & 0xFF;    
	val &= ~0xFF;
	val |= ratio;
	pscuaReg->scuaLcdClkCfg = val;

	// move to lcd clock enable functino
	//pscuaReg->scuaLcdClkCfg |= (LCD_CLK_EN);

	//printk("[%s][%d] realParentRate=%d,pscuaReg->scuaLcdClkCfg=0x%x \n",__FUNCTION__, __LINE__,realParentRate, pscuaReg->scuaLcdClkCfg);

	*realRate = realParentRate / ((pscuaReg->scuaLcdClkCfg&0Xff) + 1);

	return SP_SUCCESS;
}
EXPORT_SYMBOL(gpHalClkLcdSetRate);

/**
* @brief 	Hal lcd clock set function
* @param 	source[in]: 0<<16: SPLL, 1<<16: XTAL(27MHz), 6<<16: USBPHY(96MHz)
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkLcdSetSrc(UINT32 source)
{
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

	pscuaReg->scuaLcdClkCfg &= ~(7<<HAL_LCD_CLK_SEL_OFFSET);
	pscuaReg->scuaLcdClkCfg |= source;
	return SP_SUCCESS;
}
EXPORT_SYMBOL(gpHalClkLcdSetSrc);

int
gpHalLcdScuEnable(
	UINT32 workFreq
)
{

	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	UINT32 val;
	UINT32 lcd_div;

//	SCUB_PGS0 &= 0xFFFFFF0F;
	lcd_div = calPanelDivFreqInput(workFreq);

	val = pscuaReg->scuaLcdClkCfg;

	val &= ~(0x100);  //LCD_CLK_CNT_EN = 0x100
	pscuaReg->scuaLcdClkCfg = val;
	val = pscuaReg->scuaLcdClkCfg;
	val &= 0xFFFFFF00;
	val |= lcd_div-1;
	pscuaReg->scuaLcdClkCfg = val;
	pscuaReg->scuaLcdClkCfg |= 0x100;
	pscuaReg->scuaDispType &= ~(DISP_TYPE_MSK);
	return SP_SUCCESS;
}

EXPORT_SYMBOL(gpHalLcdScuEnable);

SINT32 gpHalLcdClkEnable(UINT32 enable)
{
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
#if 1

	gpHalScuClkEnable(SCU_A_PERI_LCD_CTRL, SCU_A, enable);
	// Enable/Disable lcd clock macro
	if (enable)
		pscuaReg->scuaLcdClkCfg |= (1<<8);
	else
		pscuaReg->scuaLcdClkCfg &= ~(1<<8);

	return SP_TRUE;
#else
   struct clk	*lcd_clk;
   //struct device *dev = (struct device *) devinfo;
	lcd_clk = clk_get(NULL, "LCD_CTRL");
	if(lcd_clk)
	{
	  if(enable)
	  {
        clk_enable(lcd_clk);
	  }
	  else
	  {
		 clk_disable(lcd_clk);
	  }
       return SP_TRUE;
	}
	else
	{
	   printk("error open lcd_clk\n");
		return SP_FALSE;
	}
#endif
}

EXPORT_SYMBOL(gpHalLcdClkEnable);

SP_BOOL gpSpiClkEnable(void* devinfo, SP_BOOL enable)
{
	struct clk *spi_clk;
	struct device *dev = (struct device *) devinfo;
	spi_clk = clk_get(dev, "SSP");

	if(spi_clk){
		if(enable){
			clk_enable(spi_clk);
		}
		else{
			clk_disable(spi_clk);
		}

		return SP_TRUE;
	}
	else
	{
		printk("error open spi_clk\n");
		return SP_FALSE;
	}

}
EXPORT_SYMBOL(gpSpiClkEnable);

SP_BOOL tvout_clk_enable(void* devinfo, SP_BOOL enable)
{
   struct clk	*tvout_clk;
   struct device *dev = (struct device *) devinfo;
	tvout_clk = clk_get(dev, "TVOUT");
	if(tvout_clk)
	{
	  if(enable)
        clk_enable(tvout_clk);
	  else
		 clk_disable(tvout_clk);
       return SP_TRUE;
	}
	else
	{
	   printk("error open tvout_clk\n");
       return SP_FALSE;
	}

}

SP_BOOL lcm_clk_enable(void* devinfo, SP_BOOL enable)
{
   struct clk	*nor_clk,*iram_clk;
   struct device *dev = (struct device *) devinfo;
	nor_clk = clk_get(dev, "ROM");
	iram_clk = clk_get(dev, "INT_MEM");
	if( nor_clk &&iram_clk)
	{
	  if(enable)
	  {
        clk_enable(nor_clk);
        clk_enable(iram_clk);
	  }
	  else
	  {
        clk_disable(nor_clk);
        clk_disable(iram_clk);
	  }
       return SP_TRUE;
	}
	else
	{
	   printk("error open lcm_clk\n");
       return SP_FALSE;
	}

}


SP_BOOL audioplay_clk_enable(void* devinfo, SP_BOOL enable)
{
   struct clk	*sar_clk,*i2s_clk;
   struct device *dev = (struct device *) devinfo;
	sar_clk = clk_get(dev, "SAACC");
	i2s_clk = clk_get(dev, "I2S");
	if( sar_clk && i2s_clk)
	{
	  if(enable)
	  {
        clk_enable(sar_clk);
        clk_enable(i2s_clk);
	  }
	  else
	  {
        clk_disable(sar_clk);
        clk_disable(i2s_clk);
	  }
       return SP_TRUE;
	}
	else
	{
	   printk("error open audioplay_clk\n");
       return SP_FALSE;
	}

}

SP_BOOL audiorec_clk_enable(void* devinfo, SP_BOOL enable)
{
   struct clk	*sar_clk,*i2srx_clk;
   struct device *dev = (struct device *) devinfo;
	sar_clk = clk_get(dev, "SAACC");
	i2srx_clk = clk_get(dev, "I2SRX");
	if( sar_clk &&i2srx_clk)
	{
	  if(enable)
	  {
        clk_enable(sar_clk);
        clk_enable(i2srx_clk);
	  }
	  else
	  {
        clk_disable(sar_clk);
        clk_disable(i2srx_clk);
	  }
       return SP_TRUE;
	}
	else
	{
	   printk("error open audiorec_clk\n");
       return SP_FALSE;
	}

}

/**
* @brief 	SCU A clock enable function
* @param 	bitMask[in]: enable bits. Ex: 1<<19 (SCUA - SAACC),
* @param 	scu[in]: 0 : SCU_A, 1 : SCU_B, 2: SCU_C
* @param 	enable[in]: 1 : enable, 0 : disable
* @return 	SUCCESS/SP_FAIL.
*/
SINT32 gpHalScuClkEnable(UINT32 bitMask, UINT8 scu, UINT8 enable)
{
	//printk("[%s][%d] bitMask = 0x%x, enable=%d \n",__FUNCTION__, __LINE__, bitMask, enable);

	switch (scu) {
	case SCU_A :
	{
		scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
		if (enable){
			pScuaReg->scuaPeriClkEn |= bitMask;
		}
		else{
			pScuaReg->scuaPeriClkEn &= ~bitMask;
		}
		break;
	}
	case SCU_B :
	{
		scubReg_t *pScubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
		if (enable){
			pScubReg->scubPeriClkEn |= bitMask;
		}
		else{
			pScubReg->scubPeriClkEn &= ~bitMask;
		}
		break;
	}
	case SCU_C :
	{
		scucReg_t *pScucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
		if (enable){
			pScucReg->scucPeriClkEn |= bitMask;
		}
		else{
			pScucReg->scucPeriClkEn &= ~bitMask;
		}
		break;
	}
	case SCU_A2:
	{
		scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
		if (enable){
			pScuaReg->scuaPeriClkEn2 |= bitMask;
		}
		else{
			pScuaReg->scuaPeriClkEn2 &= ~bitMask;
		}
		break;
	}
	case SCU_BASE_SYS:
	{
		scubReg_t *pScubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
		if (bitMask != 0xFF) {
			/* @todo:must check it works with RD */
			if (enable){
				pScubReg->scubSysCntEn |= bitMask;
			}
			else{
				pScubReg->scubSysCntEn &= ~bitMask;
			}
		}
		break;
	}
	case SCU_BASE_CEVA:
	{
		scucReg_t *pScucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
		if (bitMask != 0xFF) {
			/* @todo:must check it works with RD */
			if (enable){
				pScucReg->scucCevaCntEn |= bitMask;
			}
			else{
				pScucReg->scucCevaCntEn &= ~bitMask;
			}
		}
		break;
	}
	
	default :
		break;
	}
	return SP_SUCCESS;
}
EXPORT_SYMBOL(gpHalScuClkEnable);

/**
* @brief 	SCU A USB PHY clock enable function
* @param 	enable[in]: 1 : enable, 0 : disable
* @return 	none
*/
void gpHalScuUsbPhyClkEnable ( int enable )
{
    scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	//printk("[%s][%d] enable=%d reg=%x\n",__FUNCTION__, __LINE__, enable, pScuaReg->scuaUsbPhyCfg);
    if( enable ) {
        pScuaReg->scuaUsbPhyCfg = pScuaReg->scuaUsbPhyCfg | 0x04;
    }
    else{
        pScuaReg->scuaUsbPhyCfg = pScuaReg->scuaUsbPhyCfg & 0xfffffffb;
    }
}
EXPORT_SYMBOL(gpHalScuUsbPhyClkEnable);

/**
* @brief 	clock change function
* @param 	clkBuf[in]: clock tree buffer
* @return 	SUCCESS/SP_FAIL.
*/
SINT32 gpHalClockChange(UINT32 *clkBuf)
{
	int clk;
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);
	//gp_reg_dump(0x93007000, 0xF4, 4);
	//gp_reg_dump(0x90005000, 0x15C, 4);
	//gp_reg_dump(0x92005000, 0x50, 4);
	//gp_reg_dump(0x92005100, 0x20, 4);

	clk_ref_arm_set_rate((struct clk *)&clk, 1);
	//gp_reg_dump(0x93007000, 0xF4, 4);
	//gp_reg_dump(0x90005000, 0x15C, 4);
	//gp_reg_dump(0x92005000, 0x50, 4);
	//gp_reg_dump(0x92005100, 0x20, 4);
	printk("[%s][%d] run\n", __FUNCTION__, __LINE__);
	return SP_SUCCESS;
}

/**
* @brief 	clock update active function
* @param 	clkSrc[in]: clock source, 0 : arm clock tree, 1 : sys/ceva clock tree
* @param 	value[in]: update value
* @return 	SUCCESS/SP_FAIL.
*/
SINT32 gpHalClockUpdate(UINT32 clkSrc, UINT32 value)
{
	scubReg_t *pscubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	scucReg_t *pscucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	SINT32 ret = SP_SUCCESS;

	//printk("[%s][%d] clkSrc=%d,value=%d\n", __FUNCTION__, __LINE__,clkSrc, value);

	if (clkSrc == 0) {
		pscubReg->scubUpdateRatio = value;
	}
	else {
		/* @todo , why write 2 times ? */
		pscucReg->scucSysRatioUpdate = value;
		pscucReg->scucSysRatioUpdate = value;
	}
	 __asm__ __volatile__(
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 "	nop\n"
	 );
	 if (clkSrc == 1) {
		ret = HAL_BUSY_WAITING( ((pscucReg->scucSysRatioUpdate&0x80000000) == 0), 1);
		if (ret >= 0) {
			ret = SP_SUCCESS;
		}else{
			ret = SP_FAIL;	
		}
	}

	//printk("[%s][%d] finish, ret=%d\n", __FUNCTION__, __LINE__, ret);
	//gp_reg_dump(0x92005028, 0x10, 4);
	//gp_reg_dump(0x900050d0, 0x20, 4);
	//gp_reg_dump(0x92005100, 0x20, 4);

	return ret;
}
EXPORT_SYMBOL(gpHalClockUpdate);

/**
* @brief 	Hal apll clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalApllSetRate(UINT32 rate)
{
	UINT32 val;
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	val = pscuaReg->scuaApllCfg;
	// clear apll setting bit
	val &= ~(1<<1);
	pscuaReg->scuaApllCfg = (val | rate );

	return SP_SUCCESS;
	
}
EXPORT_SYMBOL(gpHalApllSetRate);

/**
* @brief 	Hal apll clock enable function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalApllClkEnable(UINT32 enable)
{
	UINT32 val;
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	val = pscuaReg->scuaApllCfg;
	// clear enable bit
	val &= ~(1<<0);
	pscuaReg->scuaApllCfg = (val | (enable&0x1) );

	return SP_SUCCESS;
	
}
EXPORT_SYMBOL(gpHalApllClkEnable);

/**
* @brief 	Hal arm clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkArmSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scubReg_t *pscubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	UINT32 ratio, ret = SP_SUCCESS;

	ratio = ((parentRate / rate) -1 ) & 0x3f;
	pscubReg->scubArmRatio = ratio;

	ret = gpHalClockUpdate(UPDATE_ARM_CLK, ARM_RATIO_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscubReg->scubArmRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkArmSetRate);

/**
* @brief 	Hal spll2 clock set function
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkSpll2SetRate(UINT32 rate, UINT32 *realRate)
{
	scubReg_t *pscubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	UINT32 ratio, ret = SP_SUCCESS;

  if(rate > 800000000)
     rate = 800000000;
     
	ratio = ((rate/1000000) / 4) & 0xff;

	pscubReg->scubSpllCfg1 &= ~0xff;
	pscubReg->scubSpllCfg1 |= ratio;

	*realRate = ratio * 4 *1000000;   	   	
 
 	return ret;
}
EXPORT_SYMBOL(gpHalClkSpll2SetRate);

/**
* @brief 	Hal arm ahb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkArmAhbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scubReg_t *pscubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;    
	pscubReg->scubArmAhbRatio = ratio;

	ret = gpHalClockUpdate(UPDATE_ARM_CLK, AHB_RATIO_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscubReg->scubArmAhbRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkArmAhbSetRate);

/**
* @brief 	Hal arm apb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkArmApbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scubReg_t *pscubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;    
	pscubReg->scubArmApbRatio = ratio;
	
	ret = gpHalClockUpdate(UPDATE_ARM_CLK, APB_RATIO_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscubReg->scubArmApbRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkArmApbSetRate);

/**
* @brief 	Hal ceva clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCevaSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scucReg_t *pscucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;
	pscucReg->scucCevaRatio = ratio;
	
	ret = gpHalClockUpdate(UPDATE_SYS_CEVA_CLK, CEVA_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscucReg->scucCevaRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkCevaSetRate);

/**
* @brief 	Hal ceva ahb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCevaAhbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scucReg_t *pscucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;    
	pscucReg->scucCevaAhbRatio = ratio;
	
	ret = gpHalClockUpdate(UPDATE_SYS_CEVA_CLK, CEVA_AHB_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscucReg->scucCevaAhbRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkCevaAhbSetRate);

/**
* @brief 	Hal ceva apb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCevaApbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scucReg_t *pscucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;    
	pscucReg->scucCevaApbRatio = ratio;
	
	ret = gpHalClockUpdate(UPDATE_SYS_CEVA_CLK, CEVA_APB_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscucReg->scucCevaApbRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkCevaApbSetRate);

/**
* @brief 	Hal sys ahb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkSysAhbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scucReg_t *pscucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;    
	pscucReg->scucSysAhbRatio = ratio;
	
	ret = gpHalClockUpdate(UPDATE_SYS_CEVA_CLK, SYS_AHB_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscucReg->scucSysAhbRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkSysAhbSetRate);

/**
* @brief 	Hal sys apb clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkSysApbSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scucReg_t *pscucReg = (scucReg_t *)LOGI_ADDR_SCU_C_REG;
	UINT32 ratio, ret;

	ratio = ((parentRate / rate) -1 ) & 0x3f;    
	pscucReg->scucSysApbRatio = ratio;
	
	ret = gpHalClockUpdate(UPDATE_SYS_CEVA_CLK, SYS_APB_U);
	if (ret == SP_SUCCESS) 
		*realRate = parentRate / (pscucReg->scucSysApbRatio + 1);
	else
		*realRate = rate;

	return ret;
}
EXPORT_SYMBOL(gpHalClkSysApbSetRate);

/**
* @brief 	Hal cmos clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkCsiSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	UINT32 ratio, clockSrc, val;
	UINT32 realParentRate = parentRate;

	pscuaReg->scuaCsiClkCfg &= ~(CSI_CLK_EN);

	val = pscuaReg->scuaCsiClkCfg;
	clockSrc = (val >> 16)&0x3;
	if ( (clockSrc&0x1) == 0x1) {
		//using USBPHY 96MHz, USB must enable first.
		realParentRate = USBPHY_RATE;
	}
	
	ratio = ((realParentRate / rate) -1 ) & 0xFF;    
	val &= ~0xFF;
	val |= ratio;
	pscuaReg->scuaCsiClkCfg = val;

	pscuaReg->scuaCsiClkCfg |= (CSI_CLK_EN);

	*realRate = realParentRate / ((pscuaReg->scuaCsiClkCfg&0Xff) + 1);

	return SP_SUCCESS;

}
EXPORT_SYMBOL(gpHalClkCsiSetRate);

/**
* @brief 	Hal I2S clock set function
* @param 	parentRate[in]: clock parent rate
* @param 	rate[in]: clock change rate
* @param 	realRate[in]: real clock rate after change
* @return 	SUCCESS/FAIL.
*/
SINT32 gpHalClkI2sSetRate(UINT32 parentRate, UINT32 rate, UINT32 *realRate)
{
	scuaReg_t *pscuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
	UINT32 ratio, clockSrc, val;
	UINT32 realParentRate = parentRate;

	pscuaReg->scuaCsiClkCfg &= ~(CSI_CLK_EN);

	val = pscuaReg->scuaCsiClkCfg;
	clockSrc = (val >> 16)&0x3;
	if ( (clockSrc&0x1) == 0x1) {
		//using USBPHY 96MHz, USB must enable first.
		realParentRate = USBPHY_RATE;
	}
	
	ratio = ((realParentRate / rate) -1 ) & 0xFF;    
	val &= ~0xFF;
	val |= ratio;
	pscuaReg->scuaCsiClkCfg = val;

	pscuaReg->scuaCsiClkCfg |= (CSI_CLK_EN);

	*realRate = realParentRate / ((pscuaReg->scuaCsiClkCfg&0Xff) + 1);

	return SP_SUCCESS;

}
EXPORT_SYMBOL(gpHalClkI2sSetRate);

/**
* @brief 	Hal clock enable/disable function
* @param 	clk[in]: clock structure
* @param 	enable[in]: 1 : enable , 0 : disable
* @return 	SUCCESS/FAIL.
*/
int gpHalClockEnable(struct clk *clk, UINT32 enable)
{
	if (IS_ERR(clk) || clk == NULL)
		return SP_FAIL;

	//printk("[%s][%d] run, [%s] usage=%d,enable=%d\n", __FUNCTION__, __LINE__, clk->name, clk->usage, enable);

#if 0
	// move to clk_enable/clk_disable
	if(clk->parent)
	{
		gp_clk_t *parentClk = clk->parent;
		printk("[%s][%d] run, [%s] usage=%d\n", __FUNCTION__, __LINE__, parentClk->name, parentClk->usage);
		if (enable == 0) {
			if (parentClk->usage > 0) 
				parentClk->usage --;
		}
		else {
			parentClk->usage ++;
		}
	}
#endif

	//printk("[%s][%d]clk = [%s],usage=%d, parent=[%s],usage=%d\n", __FUNCTION__, __LINE__,	(clk == NULL) ? "NULL" : clk->name ,clk->usage
	//,(clk->parent == NULL) ? "NULL" : clk->parent->name ,(clk->parent == NULL) ? 0 : clk->parent->usage );

	// recursive enable the parent clock source
	if (clk->parent) {
		gpHalClockEnable(clk->parent, enable);
	}

	if(clk->enable_func)
	{
		if ( !((enable == 0) && (clk->usage > 0)) ) {
			(clk->enable_func)(clk->ctrlbit, clk->clock_class, enable);
		}
	}

	/* @todo : remove in the furture */
	// Using original clock setting
	if (enable)
		clk_enable(clk);
	else
		clk_disable(clk);
	
	//printk("[%s][%d]clk = [%s],usage=%d, parent=[%s],usage=%d\n", __FUNCTION__, __LINE__,	(clk == NULL) ? "NULL" : clk->name ,clk->usage
	//,(clk->parent == NULL) ? "NULL" : clk->parent->name ,(clk->parent == NULL) ? 0 : clk->parent->usage );

	return SP_FAIL;
}
EXPORT_SYMBOL(gpHalClockEnable);

/**
* @brief 	Hal keep enable clock check
* @param 	scua_peri_clock[out]: bits keep SCUA clock enable
* @param 	scub_peri_clock[out]: bits keep SCUB clock enable
* @param 	scuc_peri_clock[out]: bits keep SCUC clock enable
* @param 	scua2_peri_clock[out]: bits keep SCUA2 clock enable
* @return 	SUCCESS/FAIL.
*/
int gpHalPeriClokcCheck(unsigned int *scua_peri_clock, unsigned int *scub_peri_clock, unsigned int *scuc_peri_clock, unsigned int *scua2_peri_clock)
{
	volatile unsigned int *dummy_scua_peri_clock = (unsigned int *)(0xFC80E2B8);
	volatile unsigned int *dummy_scub_peri_clock = (unsigned int *)(0xFC80E2BC);
	volatile unsigned int *dummy_scuc_peri_clock = (unsigned int *)(0xFC80E3EC);
	volatile unsigned int *dummy_scua2_peri_clock= (unsigned int *)(0xFD0050A0);

	//enable OVG clock 0x93007004 bit[27] = 1 to read OVG dummy
	gpHalScuClkEnable(SCU_A_PERI_OVG, SCU_A, 1);

	if ((*dummy_scua_peri_clock)||(*dummy_scub_peri_clock)||(*dummy_scuc_peri_clock)||(*dummy_scua2_peri_clock)) {
		*scua_peri_clock = *dummy_scua_peri_clock;
		*scub_peri_clock = *dummy_scub_peri_clock;
		*scuc_peri_clock = *dummy_scuc_peri_clock;
		if (*dummy_scua2_peri_clock) {
			*scua2_peri_clock = *dummy_scua2_peri_clock&0x3;
			*scua2_peri_clock |= (*dummy_scua2_peri_clock&0xFC) << 22;
		}
		printk("[%s]Keep SCUA enable clock:[0x%x]\n", __FUNCTION__, *scua_peri_clock);
		printk("[%s]Keep SCUB enable clock:[0x%x]\n", __FUNCTION__, *scub_peri_clock);
		printk("[%s]Keep SCUC enable clock:[0x%x]\n", __FUNCTION__, *scuc_peri_clock);
		printk("[%s]Keep SCUA2 enable clock:[0x%x]\n",__FUNCTION__, *scua2_peri_clock);
	}

	gpHalScuClkEnable(SCU_A_PERI_OVG, SCU_A, 0);
	return SP_SUCCESS;
}
EXPORT_SYMBOL(gpHalPeriClokcCheck);

