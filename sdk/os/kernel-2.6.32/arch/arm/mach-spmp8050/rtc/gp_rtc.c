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
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_rtc.c
 * @brief RTC driver interface 
 * @author chao.chen
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>
#include <mach/hal/regmap/reg_rtc.h>
#include <mach/hal/hal_rtc.h>


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
struct gp_rtc {
	int			irq_rtc;
	struct rtc_device	*rtc;
	spinlock_t		lock;		/* Protects this structure */
	struct rtc_time		rtc_alarm;
};

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static struct gp_rtc *gp_rtc_info = NULL;

/**
 * @brief   RTC irq handler
 */
static irqreturn_t gp_rtcdrv_irq(int irq, void *dev_id)
{
	/*struct platform_device *pdev = to_platform_device(dev_id);*/
	struct gp_rtc *rtc = gp_rtc_info;
	unsigned int rtsr;
	unsigned long events = 0;

	spin_lock(&rtc->lock);

	/* get and clear interrupt sources */
	rtsr = gpHalRtcIntSrcGet();
   	
	/* update irq data & counter */
	if (rtsr & SEC_INTR_STATUS)
		events |= RTC_UF | RTC_IRQF;
	if (rtsr & ALARM_INTR_STATUS)
		events |= RTC_AF | RTC_IRQF;

	rtc_update_irq(rtc->rtc, 1, events);
	
	spin_unlock(&rtc->lock);
	return IRQ_HANDLED;
}

/**
 * @brief   RTC interrupt control
 */
static int gp_rtcdrv_int_ctl(struct device *dev, unsigned int cmd,
		unsigned long arg)
{
	struct gp_rtc *rtc = gp_rtc_info;
	unsigned char sec_int_en = 2, alarm_int_en = 2;	
	int ret = 0;

	DIAG_VERB("[%s:%s] [0x%x]\n", __FILE__, __FUNCTION__, cmd);
	spin_lock_irq(&rtc->lock);

	/* alarm/sec interrupt enable/disable */
	switch (cmd) {
	case RTC_AIE_OFF:
		alarm_int_en = 0;
		break;
	case RTC_AIE_ON:
		alarm_int_en = 1;
		break;
	case RTC_UIE_OFF:
		sec_int_en = 0;
		break;
	case RTC_UIE_ON:
		sec_int_en = 1;
		break;			
	default:		
		ret = -ENOIOCTLCMD;
	}
	
	if (ret == 0){
		gpHalRtcIntEnable(sec_int_en, alarm_int_en);	
	}
	
	spin_unlock_irq(&rtc->lock);
	return ret;
}

/**
 * @brief   RTC time read
 */
static int gp_rtcdrv_read_time(struct device *dev, struct rtc_time *tm)
{
	struct gp_rtc *rtc = gp_rtc_info;
	unsigned long rtc_sec_cnt = 0;

	spin_lock_irq(&rtc->lock);					
	rtc_sec_cnt = gpHalRtcGetTime();
	spin_unlock_irq(&rtc->lock); 
	
	rtc_time_to_tm(rtc_sec_cnt, tm);
	return 0;
}

/**
 * @brief   RTC time set
 */
static int gp_rtcdrv_set_time(struct device *dev, struct rtc_time *tm)
{
	struct gp_rtc *rtc = gp_rtc_info;
	unsigned long time;
	unsigned long real_time;
	int ret;

	DIAG_INFO("[%s:%s] :\n", __FILE__, __FUNCTION__);
	DIAG_INFO("%d %d %d %d %d %d\n",tm->tm_year
		,tm->tm_mon
		,tm->tm_mday
		,tm->tm_hour
		,tm->tm_min
		,tm->tm_sec);
	
	ret = rtc_tm_to_time(tm, &time);

	spin_lock_irq(&rtc->lock);

	/*Set time, and feedback the new time*/	
	if (ret == 0){
		real_time = gpHalRtcSetTime(time);        
	}
	else{
		real_time = gpHalRtcGetTime();
	}

	spin_unlock_irq(&rtc->lock); 
	
	rtc_time_to_tm(real_time, tm);
	return ret;
}

/**
 * @brief   RTC alarm read
 */
static int gp_rtcdrv_read_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct gp_rtc *rtc = gp_rtc_info;
	unsigned int rtc_sec_cnt = 0;

	DIAG_VERB("[%s:%s]\n", __FILE__, __FUNCTION__);
	
	spin_lock_irq(&rtc->lock);		
	gpHalRtcGetAlarmStatus(&alrm->enabled, &alrm->pending, &rtc_sec_cnt);
	spin_unlock_irq(&rtc->lock);  
	
	rtc_time_to_tm((unsigned long)rtc_sec_cnt, &alrm->time);

	return 0;
}

/**
 * @brief   RTC alarm set
 */
static int gp_rtcdrv_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	struct gp_rtc *rtc = gp_rtc_info;
	unsigned long time;

	int ret;

	DIAG_VERB("[%s:%s]\n", __FILE__, __FUNCTION__);
	ret = rtc_tm_to_time(&alrm->time, &time);

	if (ret == 0)
	{
		spin_lock_irq(&rtc->lock);		
		gpHalRtcSetAlarmStatus(alrm->enabled, alrm->pending, time);
		spin_unlock_irq(&rtc->lock);        
	}
	
	return ret;
}

static const struct rtc_class_ops gp_rtc_ops = {
	.ioctl = gp_rtcdrv_int_ctl,
	.read_time = gp_rtcdrv_read_time,
	.set_time = gp_rtcdrv_set_time,
	.read_alarm = gp_rtcdrv_read_alarm,
	.set_alarm = gp_rtcdrv_set_alarm,
};

/**
 * @brief   RTC driver enable
 */
static void gp_rtcdrv_enable(struct platform_device *pdev, int en)
{
	struct gp_rtc *rtc = gp_rtc_info;

	spin_lock_irq(&rtc->lock);	
	gpHalRtcEnable(en);
	spin_unlock_irq(&rtc->lock); 
}

/**
 * @brief   RTC driver probe
 */
static int __init gp_rtcdrv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gp_rtc *rtc;
	int ret = -ENXIO;
	
	DIAG_INFO("RTC PROBE.\n");
	
	rtc = kzalloc(sizeof(struct gp_rtc), GFP_KERNEL);
	if (!rtc)
		return -ENOMEM;

	gp_rtc_info = rtc;

	/*platform_set_drvdata(pdev, rtc);*/
	spin_lock_init(&rtc->lock);
	gpHalRtcClkEnable(1);
	
	/* Enable RTC*/	
	gp_rtcdrv_enable(pdev, 1);

	rtc->rtc = rtc_device_register("gp-rtc", &pdev->dev, &gp_rtc_ops, THIS_MODULE);
	ret = PTR_ERR(rtc->rtc);
	if (IS_ERR(rtc->rtc)) {
		DIAG_ERROR("Failed to register RTC device -> %d\n", ret);
		goto err_rtc;
	}

	rtc->irq_rtc = IRQ_REALTIME_CLOCK;
	ret = request_irq(rtc->irq_rtc, gp_rtcdrv_irq, IRQF_DISABLED, "rtc_irq", dev);
	if (ret < 0) {
		DIAG_ERROR("RTC can't get irq %i, err %d\n", rtc->irq_rtc, ret);
		goto err_rtc;
	}
	
	device_init_wakeup(dev, 1);
	return 0;

err_rtc:
	gpHalRtcClkEnable(0);
	kfree(rtc);
	gp_rtc_info = NULL;
	return ret;
}

/**
 * @brief   RTC driver remove
 */
static int __exit gp_rtcdrv_remove(struct platform_device *pdev)
{
	struct gp_rtc *rtc = gp_rtc_info;

	rtc_device_unregister(rtc->rtc);

	spin_lock_irq(&rtc->lock);
	
	gpHalRtcClkEnable(0);
	free_irq(rtc->irq_rtc, &pdev->dev);	
	gp_rtcdrv_enable(pdev, 0);	

	spin_unlock_irq(&rtc->lock);

	kfree(rtc);

	gp_rtc_info = NULL;

	return 0;
}

#ifdef CONFIG_PM
/**
 * @brief   RTC driver suspend
 */
static int gp_rtcdrv_suspend(struct platform_device *pdev, pm_message_t state)
{
	/*
	struct gp_rtc *rtc = gp_rtc_info;

	
	if (device_may_wakeup(&pdev->dev))
		enable_irq_wake(rtc->irq_rtc);
	*/	
	return 0;
}

/**
 * @brief   RTC driver resume
 */
static int gp_rtcdrv_resume(struct platform_device *pdev)
{
	/*
	struct gp_rtc *rtc = gp_rtc_info;


	if (device_may_wakeup(&pdev->dev))
		disable_irq_wake(rtc->irq_rtc);
	*/
	return 0;
}
#else
#define gp_rtc_suspend	NULL
#define gp_rtc_resume	NULL
#endif

static struct platform_driver gp_rtc_driver = {
	.probe		= gp_rtcdrv_probe,
	.remove		= __exit_p(gp_rtcdrv_remove),
	.suspend	= gp_rtcdrv_suspend,
	.resume		= gp_rtcdrv_resume,
	.driver		= {
		.name		= "gp-rtc",
	},
};

/**
 * @brief   RTC driver init
 */
static int __init gp_rtcdrv_init(void)
{
	return platform_driver_register(&gp_rtc_driver);
}

/**
 * @brief   RTC driver exit
 */
static void __exit gp_rtcdrv_exit(void)
{
	platform_driver_unregister(&gp_rtc_driver);
}

module_init(gp_rtcdrv_init);
module_exit(gp_rtcdrv_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/
MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP Realtime Clock Driver (RTC)");
MODULE_LICENSE_GP;

