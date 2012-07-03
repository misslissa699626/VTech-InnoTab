#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <mach/hardware.h>
#include <mach/regs-scu.h>
#include <mach/hal/hal_clock.h>

#include <mach/hal/sysregs.h>
#include <mach/typedef.h>

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
	if(27000000 == aInFreq){
		return 0x00010001;
	}
	if(27000000 > aInFreq){
		return 0x00010000 | (27000000 / aInFreq);
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
		freq = 27000000/((val &0xff) + 1);

	return freq;

}

int
gpHalLcdScuEnable(
	SINT32 workFreq
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

SP_BOOL gpHalLcdClkEnable(void* devinfo, SP_BOOL enable)
{

#if 0
	gpHalScuClkEnable(1, SCU_A, 1);
	gpHalScuClkEnable(14, SCU_B, 1);
	return SP_TRUE;
#else
   struct clk	*lcd_clk,*gpio_clk;
   struct device *dev = (struct device *) devinfo;
	lcd_clk = clk_get(dev, "LCD_CTRL");
	gpio_clk = clk_get(dev, "GPIO");
	if(lcd_clk && gpio_clk)
	{
	  if(enable)
	  {
        clk_enable(lcd_clk);
        clk_enable(gpio_clk);
	  }
	  else
	  {
		 clk_disable(lcd_clk);
        clk_disable(gpio_clk);
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
* @param 	bitMask[in]: enable bits
* @param 	scu[in]: 0 : SCU_A, 1 : SCU_B, 2: SCU_C
* @param 	enable[in]: 1 : enable, 0 : disable
* @return 	SUCCESS/SP_FAIL.
*/
SINT32 gpHalScuClkEnable(UINT32 bitMask, UINT8 scu, UINT8 enable)
{
	printk("[%s][%d] bitMask = 0x%x, enable=%d \n",__FUNCTION__, __LINE__, bitMask, enable);

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
	default :
		break;
	}
	return SP_SUCCESS;
}
EXPORT_SYMBOL(gpHalScuClkEnable);
