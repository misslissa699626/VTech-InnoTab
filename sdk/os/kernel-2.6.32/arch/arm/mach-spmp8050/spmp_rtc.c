/*
 * Real Time Clock interface for XScale spmp27x and spmp3xx
 *
 * Copyright (C) 2008 Robert Jarzmik
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/seq_file.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <mach/hardware.h>
#include <mach/regs-rtc.h>

struct spmp_rtc {
	struct resource	*ress;
	void __iomem		*base;
	int			irq_rtc;
	struct clk  *clk_rtc;
	struct rtc_device	*rtc;
	spinlock_t		lock;		/* Protects this structure */
	struct rtc_time		rtc_alarm;
};


static unsigned int rtc_macro_read(struct spmp_rtc *spmp_rtc,unsigned int addr)
{
   	writel(addr,spmp_rtc->base+RTC_ADDR_OFST);
   	writel(RTC_CTLREAD,spmp_rtc->base+RTC_RWREQ_OFST);		
	
	//check rtc ctller ready
    while(!(readl(spmp_rtc->base+RTC_RDY_OFST) & RTC_MARCO_READY)) 
 	{;}	
	
	return readl(spmp_rtc->base+RTC_RDATA_OFST);
}
static void rtc_macro_write(struct spmp_rtc *spmp_rtc,unsigned int addr, unsigned int wdata)
{
   	writel(addr,spmp_rtc->base+RTC_ADDR_OFST);
   	writel(wdata,spmp_rtc->base+RTC_WDATA_OFST);	
   	writel(RTC_CTLWRITE,spmp_rtc->base+RTC_RWREQ_OFST);		
	
	//check rtc ctller ready
    while(!(readl(spmp_rtc->base+RTC_RDY_OFST) & RTC_MARCO_READY)) 
 	{;}	
}

static irqreturn_t spmp_rtc_irq(int irq, void *dev_id)
{
	struct platform_device *pdev = to_platform_device(dev_id);
	struct spmp_rtc *spmp_rtc = platform_get_drvdata(pdev);
	unsigned int rtsr;
	unsigned long events = 0;

	spin_lock(&spmp_rtc->lock);

	/* clear interrupt sources */
	rtsr = rtc_macro_read(spmp_rtc, RTC_INTR_STATUS_MARCO);
   	rtc_macro_write(spmp_rtc, RTC_INTR_STATUS_MARCO, ~RTC_INT_ENABLE_MASK); //disable rtc int
   	
	/* update irq data & counter */
	if (rtsr & RTC_SEC_INT_ENABLE)
		events |= RTC_UF | RTC_IRQF;
	if (rtsr & RTC_ALARM_INT_ENABLE)
		events |= RTC_AF | RTC_IRQF;

	rtc_update_irq(spmp_rtc->rtc, 1, events);
	
	spin_unlock(&spmp_rtc->lock);
	return IRQ_HANDLED;
}

static int spmp_rtc_ioctl(struct device *dev, unsigned int cmd,
		unsigned long arg)
{
	struct spmp_rtc *spmp_rtc = dev_get_drvdata(dev);
	unsigned char ucVal;	
	int ret = 0;
    printk("spmp_rtc_ioctl %d\n",cmd);	
	spin_lock_irq(&spmp_rtc->lock);
	ucVal = (unsigned char) rtc_macro_read(spmp_rtc,RTC_INTR_ENABLE_MARCO);
	switch (cmd) {
	case RTC_AIE_OFF:
		ucVal &= ~RTC_ALARM_INT_ENABLE;
		break;
	case RTC_AIE_ON:
		ucVal |= RTC_ALARM_INT_ENABLE;
		break;
	case RTC_UIE_OFF:
		printk("RTC_UIE_OFF");		
		ucVal &= ~RTC_SEC_INT_ENABLE;
		break;
	case RTC_UIE_ON:
		printk("RTC_UIE_ON");		
		ucVal |= RTC_SEC_INT_ENABLE;
		break;			
	default:
		printk("default");			
		ret = -ENOIOCTLCMD;
	}
	if(ret == 0)
	{
   	  rtc_macro_write(spmp_rtc, RTC_INTR_ENABLE_MARCO, ucVal);	
	}
	spin_unlock_irq(&spmp_rtc->lock);
	return ret;
}

static int spmp_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct spmp_rtc *spmp_rtc = dev_get_drvdata(dev);
	unsigned long rtc_cnt_low = 0, rtc_cnt_high = 0;
	unsigned long rtc_sec_cnt = 0;
   	spin_lock_irq(&spmp_rtc->lock);					
    rtc_cnt_low   = rtc_macro_read(spmp_rtc,RTC_TIMERCNT_7_0_MARCO);
    rtc_cnt_low  |= (rtc_macro_read(spmp_rtc,RTC_TIMERCNT_15_8_MARCO) << 8);
    rtc_cnt_low  |= (rtc_macro_read(spmp_rtc,RTC_TIMERCNT_23_16_MARCO) << 16);
    rtc_cnt_low  |= (rtc_macro_read(spmp_rtc,RTC_TIMERCNT_31_24_MARCO) << 24);	
    rtc_cnt_high =  rtc_macro_read(spmp_rtc,RTC_TIMERCNT_39_32_MARCO);
    rtc_cnt_high |= (rtc_macro_read(spmp_rtc,RTC_TIMERCNT_47_40_MARCO) << 8);
	rtc_sec_cnt = (rtc_cnt_high << 17) | (rtc_cnt_low >> 15);   //rtc clk =32k , sec = 32k count
   	spin_unlock_irq(&spmp_rtc->lock);          	
	rtc_time_to_tm(rtc_sec_cnt, tm);
	return 0;
}

static int spmp_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct spmp_rtc *spmp_rtc = dev_get_drvdata(dev);
	unsigned long time;
	
	int ret;
    printk("spmp_rtc_set_time %d %d %d %d %d %d\n",tm->tm_year
		,tm->tm_mon
		,tm->tm_mday
		,tm->tm_hour
		,tm->tm_min
		,tm->tm_sec);
	ret = rtc_tm_to_time(tm, &time);
	
	if (ret == 0)
	{
    	unsigned long rtc_cnt_low = 0, rtc_cnt_high = 0;
		unsigned char ucVal;
    	spin_lock_irq(&spmp_rtc->lock);	
    	ucVal = (unsigned char) rtc_macro_read(spmp_rtc,RTC_CTL_MARCO);
	    ucVal = ucVal|CTL_WRITE_LOAD;
        rtc_macro_write(spmp_rtc,RTC_CTL_MARCO,ucVal);		
		rtc_cnt_low = (time & 0x1FFFF) << 15;
        rtc_macro_write(spmp_rtc,RTC_LOADCNTBIT_7_0_MARCO,rtc_cnt_low & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_LOADCNTBIT_15_8_MARCO,(rtc_cnt_low >> 8) & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_LOADCNTBIT_23_16_MARCO,(rtc_cnt_low >> 16) & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_LOADCNTBIT_31_24_MARCO,(rtc_cnt_low >> 24) & 0xFF); 	
		rtc_cnt_high = time >> 17;
        rtc_macro_write(spmp_rtc,RTC_LOADCNTBIT_39_32_MARCO,rtc_cnt_high & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_LOADCNTBIT_47_40_MARCO,(rtc_cnt_high >> 8) & 0x3F);		
        rtc_macro_write(spmp_rtc,RTC_LOAD_START_VALUE_MARCO, 1); //Load Start Value		 
    	spin_unlock_irq(&spmp_rtc->lock);          
	}
	spmp_rtc_read_time(dev,tm);
	return ret;
}

static int spmp_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct spmp_rtc *spmp_rtc = dev_get_drvdata(dev);
	unsigned long rtc_cnt_low = 0, rtc_cnt_high = 0;
	unsigned long rtc_sec_cnt = 0;
    printk("spmp_rtc_read_alarm\n");	
   	spin_lock_irq(&spmp_rtc->lock);			
    rtc_cnt_low  = (unsigned char) rtc_macro_read(spmp_rtc,RTC_ALARM_7_0_MARCO);
    rtc_cnt_low  |= ((unsigned char) rtc_macro_read(spmp_rtc,RTC_ALARM_15_8_MARCO)) << 8;
    rtc_cnt_low  |= ((unsigned char) rtc_macro_read(spmp_rtc,RTC_ALARM_23_16_MARCO)) << 16;
    rtc_cnt_low  |= ((unsigned char) rtc_macro_read(spmp_rtc,RTC_ALARM_31_24_MARCO)) << 24;
	
    rtc_cnt_high = (unsigned char) rtc_macro_read(spmp_rtc,RTC_ALARM_39_32_MARCO);
    rtc_cnt_high |= ((unsigned char) rtc_macro_read(spmp_rtc,RTC_ALARM_47_40_MARCO) & 0x7F) << 8;
	
	rtc_sec_cnt = (rtc_cnt_high << 17) | (rtc_cnt_low >> 15);   //rtc clk =32k , sec = 32k count
   	spin_unlock_irq(&spmp_rtc->lock);          	
	rtc_time_to_tm(rtc_sec_cnt, &alrm->time);
	alrm->enabled = !!(rtc_macro_read(spmp_rtc,RTC_INTR_ENABLE_MARCO) & RTC_ALARM_INT_ENABLE);
	
	return 0;
}

static int spmp_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct spmp_rtc *spmp_rtc = dev_get_drvdata(dev);
	unsigned long time;
	
	int ret;
    printk("spmp_rtc_set_alarm\n");	
	ret = rtc_tm_to_time(&alrm->time, &time);
	
	if (ret == 0)
	{
    	unsigned long rtc_cnt_low = 0, rtc_cnt_high = 0;	
	    unsigned char reg;
    	spin_lock_irq(&spmp_rtc->lock);		
		rtc_cnt_low = (time & 0x1FFFF) << 15;
        rtc_macro_write(spmp_rtc,RTC_ALARM_7_0_MARCO,rtc_cnt_low & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_ALARM_15_8_MARCO,(rtc_cnt_low >> 8) & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_ALARM_23_16_MARCO,(rtc_cnt_low >> 16) & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_ALARM_31_24_MARCO,(rtc_cnt_low >> 24) & 0xFF); 	
		rtc_cnt_high = time >> 17;
        rtc_macro_write(spmp_rtc,RTC_ALARM_39_32_MARCO,rtc_cnt_high & 0xFF);
        rtc_macro_write(spmp_rtc,RTC_ALARM_47_40_MARCO,(rtc_cnt_high >> 8) & 0x3F);		
    	reg = rtc_macro_read(spmp_rtc,RTC_INTR_ENABLE_MARCO);
	    if (alrm->enabled)
    		reg |= RTC_ALARM_INT_ENABLE;
    	else
    		reg &= ~RTC_ALARM_INT_ENABLE;
    	rtc_macro_write(spmp_rtc,RTC_INTR_ENABLE_MARCO, reg);		
    	spin_unlock_irq(&spmp_rtc->lock);        
	}
	return ret;
}


static const struct rtc_class_ops spmp_rtc_ops = {
	.ioctl = spmp_rtc_ioctl,
	.read_time = spmp_rtc_read_time,
	.set_time = spmp_rtc_set_time,
	.read_alarm = spmp_rtc_read_alarm,
	.set_alarm = spmp_rtc_set_alarm,
};

static void spmp_rtc_enable(struct platform_device *pdev, int en)
{
	struct spmp_rtc *spmp_rtc = platform_get_drvdata(pdev);
	unsigned char ucVal;

	if (spmp_rtc->base == NULL)
		return;

	if (!en) 
	{
		ucVal = (unsigned char) rtc_macro_read(spmp_rtc,RTC_CTL_MARCO);
     	rtc_macro_write(spmp_rtc, RTC_CTL_MARCO, ~CTL_RTC_CLKEN & ucVal);
	} else 
	{
    	writel(RTC_MACRO_CLK_ENABLE,spmp_rtc->base+RTC_SIEN_OFST);
        printk("RTC_SIEN = %08x\n",RTC_SIEN);
	    //check rtc ctller ready
    	while(!(readl(spmp_rtc->base+RTC_RDY_OFST) & RTC_MARCO_READY)) 
 		{;}	

	    //== init rtc , trig hardware pulse
     	rtc_macro_write(spmp_rtc, RTC_CTL_MARCO, CTL_RTC_CLKEN|CTL_COUNT_UP);

        //enable clk
    	ucVal = (unsigned char) rtc_macro_read(spmp_rtc,RTC_RELIABLECODE_MARCO);
    	if (RELIABLE_CODE_CHECK_NUMBER != ucVal)
	    {
       	 rtc_macro_write(spmp_rtc, RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_RTCRST | CTL_COUNT_UP);
         rtc_macro_write(spmp_rtc, RTC_CTL_MARCO, CTL_RTC_CLKEN | CTL_COUNT_UP);
         rtc_macro_write(spmp_rtc, RTC_FDEN_MARCO, FD_ENABLE); //enable FD
         rtc_macro_write(spmp_rtc, RTC_INTR_ENABLE_MARCO, ~RTC_INT_ENABLE_MASK); //disable rtc int
         rtc_macro_write(spmp_rtc, RTC_INTR_STATUS_MARCO, ~RTC_INT_ENABLE_MASK);	//clear intr status	
     	 rtc_macro_write(spmp_rtc, RTC_RELIABLECODE_MARCO, RELIABLE_CODE_CHECK_NUMBER);	//clear intr status	     	
	    }
//   	    rtc_macro_write(spmp_rtc, RTC_INTR_ENABLE_MARCO, RTC_SEC_INT_ENABLE | RTC_ALARM_INT_ENABLE | RTC_WAKEUP_INT_ENABLE); //disable rtc int
    }
}

static int __init spmp_rtc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct spmp_rtc *spmp_rtc;
	int ret;
    printk("rrtc probe\n");
	spmp_rtc = kzalloc(sizeof(struct spmp_rtc), GFP_KERNEL);
	if (!spmp_rtc)
		return -ENOMEM;

	spin_lock_init(&spmp_rtc->lock);
	platform_set_drvdata(pdev, spmp_rtc);

	ret = -ENXIO;
	spmp_rtc->ress = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!spmp_rtc->ress) {
		dev_err(dev, "No I/O memory resource defined\n");
		goto err_ress;
	}

	spmp_rtc->irq_rtc = platform_get_irq(pdev, 0);
	if (spmp_rtc->irq_rtc < 0) {
		dev_err(dev, "No 1Hz IRQ resource defined\n");
		goto err_ress;
	}
	
	ret = -ENOMEM;
	spmp_rtc->base = ioremap(spmp_rtc->ress->start,
				resource_size(spmp_rtc->ress));
	if (!spmp_rtc->base) {
		dev_err(&pdev->dev, "Unable to map spmp RTC I/O memory\n");
		goto err_map;
	}

	spmp_rtc->clk_rtc= clk_get(&pdev->dev, "RTC");
	if (!spmp_rtc->clk_rtc) {
		dev_err(&pdev->dev, "Unable to get rtc clock\n");
		goto err_rtc_reg;
	}
	clk_enable(spmp_rtc->clk_rtc);
	
	/*
	 * Enable RTC
	 */	
	spmp_rtc_enable(pdev, 1);

	spmp_rtc->rtc = rtc_device_register("spmp-rtc", &pdev->dev, &spmp_rtc_ops,
					   THIS_MODULE);
	ret = PTR_ERR(spmp_rtc->rtc);
	if (IS_ERR(spmp_rtc->rtc)) {
		dev_err(dev, "Failed to register RTC device -> %d\n", ret);
		goto err_rtc_clock;
	}

	ret = request_irq(spmp_rtc->irq_rtc, spmp_rtc_irq, IRQF_DISABLED,
			  "rtc_irq", dev);
	if (ret < 0) {
		dev_err(dev, "can't get irq %i, err %d\n", spmp_rtc->irq_rtc,
			ret);
		goto err_rtc_clock;
	}

	
//	ret = request_irq(spmp_rtc->irq_rtc, spmp_rtc_irq, IRQF_DISABLED,
//			  "rtc_irq", dev);
//	if (ret < 0) {
//		dev_err(dev, "can't get irq %i, err %d\n", spmp_rtc->irq_rtc,
//			ret);
//		goto err_rtc_clock;
//	}
	
//	spmp_rtc->rtc = rtc_device_register("spmp-rtc", &pdev->dev, &spmp_rtc_ops,
//					   THIS_MODULE);
//	ret = PTR_ERR(spmp_rtc->rtc);
//	if (IS_ERR(spmp_rtc->rtc)) {
//		dev_err(dev, "Failed to register RTC device -> %d\n", ret);
//		goto err_rtc_irq;
//	}


	
	device_init_wakeup(dev, 1);

	return 0;

err_rtc_irq:
//  free_irq(spmp_rtc->irq_rtc, dev);
	rtc_device_unregister(spmp_rtc->rtc);
err_rtc_clock:
	clk_disable(spmp_rtc->clk_rtc);	
err_rtc_reg:
    iounmap(spmp_rtc->base);
err_ress:
err_map:
	kfree(spmp_rtc);
	return ret;
}

static int __exit spmp_rtc_remove(struct platform_device *pdev)
{
	struct spmp_rtc *spmp_rtc = platform_get_drvdata(pdev);

	rtc_device_unregister(spmp_rtc->rtc);

	spin_lock_irq(&spmp_rtc->lock);
	clk_disable(spmp_rtc->clk_rtc);
	free_irq(spmp_rtc->irq_rtc, &pdev->dev);	
	spmp_rtc_enable(pdev, 0);	
	iounmap(spmp_rtc->base);
	spin_unlock_irq(&spmp_rtc->lock);

	kfree(spmp_rtc);

	return 0;
}

#ifdef CONFIG_PM
static int spmp_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct spmp_rtc *spmp_rtc = platform_get_drvdata(pdev);

//	if (device_may_wakeup(&pdev->dev))
//		enable_irq_wake(spmp_rtc->irq_rtc);
	return 0;
}

static int spmp_rtc_resume(struct platform_device *pdev)
{
	struct spmp_rtc *spmp_rtc = platform_get_drvdata(pdev);

//	if (device_may_wakeup(&pdev->dev))
//		disable_irq_wake(spmp_rtc->irq_rtc);
	return 0;
}
#else
#define spmp_rtc_suspend	NULL
#define spmp_rtc_resume	NULL
#endif

static struct platform_driver spmp_rtc_driver = {
	.probe		= spmp_rtc_probe,
	.remove		= __exit_p(spmp_rtc_remove),
	.suspend	= spmp_rtc_suspend,
	.resume		= spmp_rtc_resume,
	.driver		= {
		.name		= "spmp-rtc",
	},
};

static int __init spmp_rtc_init(void)
{
	return platform_driver_register(&spmp_rtc_driver);
}

static void __exit spmp_rtc_exit(void)
{
	platform_driver_unregister(&spmp_rtc_driver);
}

module_init(spmp_rtc_init);
module_exit(spmp_rtc_exit);

MODULE_AUTHOR("Sunplusmm");
MODULE_DESCRIPTION("spmp Realtime Clock Driver (RTC)");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:spmp-rtc");

