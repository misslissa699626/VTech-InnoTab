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
 * @file external_rtc.c
 * @brief Implement of Seiko Instruments S-35390A RTC Driver
 * @author Daolong Li
 */

#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/diag.h>

#include <mach/gp_i2c_bus.h>
#include <mach/typedef.h>

/*Below file should be moved to mach/kernel.h*/
#include <linux/bitrev.h>	/*For bitrev8*/
#include <linux/bcd.h>		/*bcd2bin, bin2bcd*/


#define ENABLE_INT1_OUTPUT_FREQUENCY 0

#if 0
#define DEBUG0 DIAG_VERB
#else
#define DEBUG0(...)
#endif

#define DEVICE_ADDR 			0x60

/*S-35390A register address*/
#define	EXT_RTC_STS_REG1		0
#define	EXT_RTC_STS_REG2		1
#define	EXT_RTC_TIME_REG1		2
#define	EXT_RTC_TIME_REG2		3
#define	EXT_RTC_INT_REG1		4    /*INT1: 4;  INT2: 5*/
#define	EXT_RTC_INT_REG2		5
#define	EXT_RTC_ADJ_REG		6
#define	EXT_RTC_FREE_REG		7

#define S35390A_INT_FLAG_INT1		0x01
#define S35390A_INT_FLAG_INT2		0x02

typedef struct gp_ext_rtc_s{
	int				irq_rtc;
	struct	 		clk  *clk_rtc;
	struct 			rtc_device	*rtc;
	spinlock_t		lock;			/* Protects this structure */
	struct rtc_time	rtc_alarm;
} gp_ext_rtc_t;


static gp_ext_rtc_t *gp_ext_rtc_info = NULL;

/**
*@brief Get week day of given date
*/
static int day_of_week(unsigned int year, unsigned int mon, unsigned int day)
{
	static int t[]={0,3,2,5,0,3,5,1,4,6,2,4};
	int wday;

	year -= mon<3;
	wday = (year + year/4 - year/100 + year/400 + t[mon-1] + day) % 7;
	DEBUG0("Today is:%d\n",wday);
	return wday;
}

/**
*@brief Write S35390A register
*/
static int s35390a_set_reg(unsigned char addr, unsigned char *dataGroup, unsigned char cnt)
{
	unsigned char slaveAddress = DEVICE_ADDR;
   	int ret = SP_OK;
	unsigned char i = 0;
	int i2c_handle = 0;
	
	addr = (addr<<1)&0x0e;
	slaveAddress = (slaveAddress|addr);
	i2c_handle = gp_i2c_bus_request(slaveAddress, 0x0f);
	if(!i2c_handle){
		DIAG_ERROR("[%s] Fail to request I2c bus!\n", __FUNCTION__);
		return SP_FAIL;
	}

	//msb2Lsb(dataGroup, cnt);
        for(i = 0; i < cnt; i++){
		dataGroup[i] = bitrev8(dataGroup[i]);
	}

	ret = gp_i2c_bus_write(i2c_handle, dataGroup, cnt);

	if(ret < 0){
		DIAG_ERROR("Fail to write external RTC register!\n");
		ret = SP_FAIL;
	}else{
		ret = SP_OK;
	}

	if(i2c_handle)
		gp_i2c_bus_release(i2c_handle);

	return ret;	
}

/**
*@brief Read S35390A register
*/
static int s35390a_get_reg(unsigned char addr, unsigned char *dataGroup, unsigned char cnt)
{
	unsigned char slaveAddress = DEVICE_ADDR;
	unsigned char recData[10];
	unsigned char i;
   	int ret = SP_OK;
	int i2c_handle = 0;

	addr = (addr<<1)&0x0e;
	slaveAddress = (slaveAddress|addr);
	i2c_handle = gp_i2c_bus_request(slaveAddress, 0x0f);
	if(!i2c_handle){
		DIAG_ERROR("[%s] Fail to request I2c bus!\n", __FUNCTION__);
		return SP_FAIL;
	}

	ret = gp_i2c_bus_read(i2c_handle, recData, cnt);

	if(ret < 0){
		DIAG_ERROR("Fail to read external RTC register!\n");
		ret = SP_FAIL;
	}else{
		ret = SP_OK;
	}
	
	//msb2Lsb(recData, cnt);
	for(i=0; i<cnt; i++) {
		dataGroup[i] = bitrev8(recData[i]);
	}

	if(i2c_handle)
		gp_i2c_bus_release(i2c_handle);

	return ret;
}

/**
*@brief Set time format
*/
static int s35390a_set_12or24_format(unsigned char format)
{
	unsigned char reg1_sts;
	int ret;

	ret = s35390a_get_reg(EXT_RTC_STS_REG1,&reg1_sts,1);
	if( ret != SP_OK ) {
		return ret;
	}
	
	if (format == 0) {
		/* 12 hours */
		reg1_sts = reg1_sts&0xfd;
	} else {
		/* 24 hours */
		reg1_sts = reg1_sts|0x02;		
	}
	
	return  s35390a_set_reg(EXT_RTC_STS_REG1, &reg1_sts, 1); /*set sts_reg1*/	
	
}

#if ENABLE_INT1_OUTPUT_FREQUENCY
/**
*@brief Set int1 output frequency
*/
static int s35390a_int1_output_freq(unsigned int frequency)
{	
	unsigned char sts_reg2, int_reg1;
	
	s35390a_get_reg(EXT_RTC_STS_REG2, &sts_reg2, 1);
	sts_reg2 &= 0xf0;
	sts_reg2 |= 0x01;
	s35390a_set_reg(EXT_RTC_STS_REG2, &sts_reg2, 1);
	if(frequency == 16)
		int_reg1 = 0x08;/*Out put 16HZ to INT1*/
	else
		int_reg1 = 0x10;/*Out put 8HZ to INT1*/
	s35390a_set_reg(EXT_RTC_INT_REG1, &int_reg1, 1);

	s35390a_get_reg(EXT_RTC_STS_REG2, &sts_reg2, 1);
	s35390a_set_reg(EXT_RTC_INT_REG1, &int_reg1, 1);
	
	DIAG_VERB("output_frequency: sts_reg2, int_reg1 = 0x%x, 0x%x \n", sts_reg2, int_reg1);
	return SP_OK;
}
#endif

#if 0
static int s35390a_get_adjust(unsigned char *adj_data)
{
	int ret;
	unsigned char reg1;

	/* Get External RTC EXT_RTC_ADJ_REG */
	ret = s35390a_get_reg( EXT_RTC_ADJ_REG, &reg1, 1 );

	*adj_data = reg1;
	
	return ret;
}
#endif

static int s35390a_set_adjust(unsigned char adj_data)
{
	unsigned char reg1, reg2 = 0;
	int retryCount = 10;
	int ret;

	/* Set External RTC EXT_RTC_ADJ_REG */
	reg1 = adj_data;

	while ( retryCount-- > 0 ) {
		/* write */
		ret = s35390a_set_reg( EXT_RTC_ADJ_REG, &reg1, 1 );
		if ( SP_OK == ret ) {
			break;
		}
	}
	
	if ( SP_FAIL == ret ) {
		return SP_FAIL;
	}
	
	retryCount = 10;	 
	while ( retryCount-- > 0 ) {
		/* read back */
		s35390a_get_reg(EXT_RTC_ADJ_REG, &reg2, 1);
		if ( reg1 == reg2 ) 
			break;
	}	

	return retryCount == 0 ? SP_FAIL : SP_OK;
}

/*
Éè¶š×ŽÌ¬ŒÄŽæÆ÷2
Int1Mode value:
0:ÎÞÖÐ¶Ï£»
1:32 kHz Êä³ö;
2:Ñ¡ÔñÆµÂÊÎÈÌ¬ÖÐ¶Ï;
3:·Öµ¥Î»±ßÔµÖÐ¶Ï;
4:·Öµ¥Î»ÎÈÌ¬ÖÐ¶Ï1(ÕŒ¿ÕÏµÊý 50%)
5:±šŸ¯ÖÐ¶Ï;
6:·Öµ¥Î»ÎÈÌ¬ÖÐ¶Ï2

Int2Mode value:
0:ÎÞÖÐ¶Ï;
1:Ñ¡ÔñÆµÂÊÎÈÌ¬ÖÐ¶Ï;
2:·Öµ¥Î»±ßÔµÖÐ¶Ï;
3:·Öµ¥Î»ÎÈÌ¬ÖÐ¶Ï1(ÕŒ¿ÕÏµÊý 50%)
4:±šŸ¯ÖÐ¶Ï;
7:·Öµ¥Î»ÎÈÌ¬ÖÐ¶Ï2
8:32 kHz Êä³ö
*/

/**
*@brief Set alarm interrupt
*/
static int s35390a_set_alarm_int(unsigned char enable)
{
	unsigned char int_reg2 = 0;
	int ret = 0;

	/*int2 for alarm */
	ret = s35390a_get_reg(EXT_RTC_STS_REG2, &int_reg2, 1);
	if(ret != SP_OK)
		return ret;
	
	if(enable == 1){
		int_reg2 &= 0x0f;
		int_reg2 |= 0x40;
	}
	else{
		int_reg2 &= 0x0f;
	}

	return s35390a_set_reg(EXT_RTC_STS_REG2, &int_reg2, 1); /*set sts_reg2*/
}


static int s35390a_clear_alarm_int(void)
{
	unsigned char sts = 0;
	unsigned char reg = 0x00;
	int ret = 0;

	ret = s35390a_get_reg(EXT_RTC_STS_REG2,&reg,1);
	if(ret != SP_OK)
		return ret;
	
	reg &= 0x0f;
	ret = s35390a_set_reg(EXT_RTC_STS_REG2,&reg,1);
	if(ret != SP_OK)
		return ret;

	ret = s35390a_get_reg(EXT_RTC_STS_REG1,&sts,1);
	if(ret != SP_OK)
		return ret;
	
	if ((sts&0x20)==0x20) {	/*INT1:(sts&0x10)==0x10; INT2:(sts&0x20)==0x20 */
		DEBUG0("External RTC alarm INT clear OK\n ");
		return SP_OK;
	}

	return SP_FAIL;
}

/**
*@brief Get S35390A RTC Interrupt source.
*/
static int s35390a_get_int_src(unsigned int *int_src)
{
	unsigned char sts = 0;
	int ret = 0;

	ret = s35390a_get_reg(EXT_RTC_STS_REG1, &sts, 1);
	if(ret != SP_OK){
		*int_src = 0;
		return ret; 
	}

	if(sts & 0x10)
		*int_src= S35390A_INT_FLAG_INT1;	

	if(sts & 0x20)
		*int_src = S35390A_INT_FLAG_INT2;

	return ret;
}


/**
* @brief Set S35390A Alarm Interrupt settings, using INT2
* @param enable[in] : Alarm Interrupt enable (0 disable/1 enable).
* @param pending[in] : Alarm Interrupt pending (0 not pending/1 pending).
* @param time[in] : Alarm time value.
*/
static int  s35390a_set_alarm(unsigned char enable, unsigned char pending, struct rtc_time *alarm)
{
	unsigned char time[4] = {0};
	unsigned char reg = 0x40; /*use INT2: 0x40     use INT1:  0x04;*/
	unsigned char tmp = 0;
	unsigned char weekDayEn = 0x80;
	int ret = 0;

	if(alarm->tm_hour >= 12){
		tmp = 0x40;
	}
	DEBUG0("Set alarm %d-%d-%d-%d-%d-%d\n", alarm->tm_year+1900, alarm->tm_mon, alarm->tm_mday, alarm->tm_hour, alarm->tm_min, alarm->tm_sec);

	alarm->tm_wday = day_of_week(alarm->tm_year+1900, alarm->tm_mon, alarm->tm_mday);
	time[0] = bin2bcd(alarm->tm_wday) | weekDayEn;/*0x80 will make weekDay enable*/
	time[1] = bin2bcd(alarm->tm_hour) | 0x80;
	time[1] = time[1] |tmp;/*must set am/pm whether 12 or 24*/
	time[2] = bin2bcd(alarm->tm_min) | 0x80;
	
	
	//DEBUG0("Set alarm[0x%x][0x%x][0x%x]\n", time[0], time[1], time[2]);
	
	/**/
	ret = s35390a_get_reg(EXT_RTC_STS_REG2, &reg, 1);
	if(ret != SP_OK)
		return ret;
	
	if(enable == 1){
		reg &= 0x0f;
		reg |= 0x40;
		ret = s35390a_set_reg(EXT_RTC_STS_REG2, &reg, 1);
	}else{
		reg &= 0x0f;
		ret = s35390a_set_reg(EXT_RTC_STS_REG2, &reg, 1);
	}
	if(ret != SP_OK)
		return ret;

	ret = s35390a_set_reg(EXT_RTC_INT_REG2, time, 3);
	if(ret != SP_OK)
		return ret;
	
	ret = s35390a_get_reg(EXT_RTC_STS_REG2, &reg, 1);
	if(ret != SP_OK)
		return ret;
	//DEBUG0("enable = %d, REG = 0x%x \n", enable, reg);

	return ret;

}
/**
* @brief Get S35390A Alarm Interrupt settings.
* @param enable[out] : Alarm Interrupt enable (0 disable/1 enable).
* @param pending[out] : Alarm Interrupt pending (0 not pending/1 pending).
* @param time[out] : Alarm time value.
*/
static int s35390a_get_alarm(unsigned char *enable, unsigned char *pending, struct rtc_time *alarm)
{
	unsigned char sts = 0;
	unsigned char time[4] = {0};
	int ret = 0;
	
	/*check enalbe*/
	ret = s35390a_get_reg(EXT_RTC_STS_REG2, &sts, 1);
	if(ret != SP_OK){
		DEBUG0("[%s] get alarm enable fail!\n", __FUNCTION__);
		return ret;
	}
	*enable = (sts & 0x40)?1:0;
	//DEBUG0("sts = 0x%x, Enable = %d \n", sts, *enable);

	/*read alarm time*/
	ret = s35390a_get_reg(EXT_RTC_INT_REG2, time, 3);
	if(ret != SP_OK){
		DEBUG0("[%s] Get alarm time fail!\n", __FUNCTION__);
		return ret;
	}
	//DEBUG0("Get alarm[0x%x][0x%x][0x%x]\n", time[0], time[1], time[2]);

	alarm->tm_year = 0;
	alarm->tm_mon = 0;
	alarm->tm_mday = 0;
	alarm->tm_wday = bcd2bin(time[0] & 0x7f);
	if((time[0]&0x80)==0x00)
	{
		alarm->tm_wday = 7;/*发现闹钟未使能星期设定，则回传7*/
	}
	alarm->tm_hour = bcd2bin(time[1] & 0x3f);
	alarm->tm_min =  bcd2bin(time[2] & 0x7f);
	alarm->tm_sec = 0;
	
	return ret;
}

/**
*@brief Check started or not
*/
static int s35390a_check_started(unsigned char *pReg)
{
	unsigned char reg1_sts;
	unsigned char reg2_sts;
	int ret = 0;
	int retry = 10;

	/*unsigned char tmp = 0;*/
	ret = s35390a_get_reg(EXT_RTC_STS_REG1, &reg1_sts,1);
	if( ret != SP_OK ) {
		*pReg = 0;
		return ret;
	}
	
	if(((reg1_sts&0x80)==0x80)||((reg1_sts&0x40)==0x40)){	/*poc or boc is 1*/	
		DIAG_INFO("\nExternal RTC first start up!\n\n");
		while(1)	{
			/*Ïò×ŽÌ¬ŒÄŽæÆ÷1µÄBIT0ÐŽÈë1œøÐÐreset*/
			reg1_sts = reg1_sts|0x01;
			s35390a_set_reg(EXT_RTC_STS_REG1, &reg1_sts, 1);

			#if 0
			/*×ÔÓÉŒÄŽæÆ÷£¬Öµ¿É×ÔÓÉÉè¶š£¬¿ÉÓÃÎªRTC reliableµÄÅÐ±ð*/
			tmp = 0xa5;
			halExtRtcRegWrite(EXT_RTC_FREE_REG,&tmp,1);
			#endif
			
			s35390a_get_reg(EXT_RTC_STS_REG2,&reg2_sts, 1);
			if ((reg2_sts&0x80)==0){/*TEST bit is 0*/	
				ret = SP_OK;
				break;
			}
			if( retry < 0 ) {
				ret = SP_FAIL;
				break;
			}
			else {
				retry--;
			}
		}		
	}
	*pReg = reg1_sts;

	return ret;
}


static  int s35390a_init(void)
{
	unsigned char reg1 = 0;
	unsigned int ret = 0;
	
	
	/*Check Frist Start Up*/
	ret = s35390a_check_started(&reg1);
	if( ret != SP_OK ) {
		DIAG_ERROR("[%s]External RTC init fail!\n", __FUNCTION__);
		return ret;
	}

	/*Set 12/24 format*/
	ret = s35390a_set_12or24_format(1);
	if( ret != SP_OK ) {
		DIAG_ERROR("[%s]Set External RTC 12/24 format fail!\n", __FUNCTION__);
		return ret;
	}

	/*Reset External RTC EXT_RTC_ADJ_REG*/
	ret = s35390a_set_adjust(0x82 ); /* +1 ppm */

	ret = s35390a_clear_alarm_int();
	
	DIAG_INFO("External RTC initialize OK!\n");
	return SP_OK;
}

/*******************************************************************************/
/**
*@brief Alarm/Update irq callback
*/
static irqreturn_t gp_ext_rtc_int_irq(int irq, void *dev_id)
{
	unsigned int rtsr;
	unsigned long events = 0;
	int ret = 0;
	
	if(!gp_ext_rtc_info){
		return IRQ_HANDLED;
	}
	
	spin_lock(&gp_ext_rtc_info->lock);

	/* get and clear interrupt sources */
	ret = s35390a_get_int_src(&rtsr);
	if(ret != SP_OK){
		spin_unlock(&gp_ext_rtc_info->lock);
		return IRQ_HANDLED;
	}
	
	/* update irq data & counter */
	if (rtsr & S35390A_INT_FLAG_INT1)
		events |= RTC_UF | RTC_IRQF;
	if (rtsr & S35390A_INT_FLAG_INT2)
		events |= RTC_AF | RTC_IRQF;

	rtc_update_irq(gp_ext_rtc_info->rtc, 1, events);

	spin_unlock(&gp_ext_rtc_info->lock);
	
	return IRQ_HANDLED;
}

static int gp_ext_rtc_set_alarm_int(unsigned int enable)
{
	int ret = 0;
	if(!gp_ext_rtc_info){
		return (-EPERM);
	}
	if(enable == 1){
		ret = s35390a_set_alarm(1, 0, &gp_ext_rtc_info->rtc_alarm);
	}else{
		ret = s35390a_set_alarm_int(0);
	}
	if(ret != SP_OK)
		ret = -(EIO);
	
	return ret;
}
/*
static int gp_ext_rtc_set_update_int(unsigned int enable)
{
	return 0;
}
*/
static int gp_ext_rtc_ioctrl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	DEBUG0("[%s]: cmd = [0x%x]\n",__FUNCTION__, cmd);
	if(!gp_ext_rtc_info){
		return (-ENODEV);
	}	
	spin_lock_irq(&gp_ext_rtc_info->lock);
	
	switch (cmd) {
	case RTC_AIE_OFF:
		ret = gp_ext_rtc_set_alarm_int(0);
		break;
	case RTC_AIE_ON:
		ret = gp_ext_rtc_set_alarm_int(1);
		break;
	case RTC_UIE_OFF:		
		//ret = gp_ext_rtc_set_update_int(0);
		break;
	case RTC_UIE_ON:
		//ret = gp_ext_rtc_set_update_int(1);
		break;			
	default:		
		ret = -ENOIOCTLCMD;
		break;
	}
	
	spin_unlock_irq(&gp_ext_rtc_info->lock);
	
	return ret;
}

static int gp_ext_rtc_get_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char i = 0;
	unsigned char time[7];
	int ret = 0;
	DEBUG0("[%s]:Enter \n", __FUNCTION__);

	if(!gp_ext_rtc_info){
		return (-EPERM);
	}
	spin_lock_irq(&gp_ext_rtc_info->lock);
	
	ret = s35390a_get_reg(EXT_RTC_TIME_REG1, time, 7);	/*read real time*/
	if( ret != SP_OK ) {
		return (-EIO);
	}
	
	for(i=0;i<7;i++){
		if (i==4){
			if (time[4]&0x40){
				time[4] = time[4]&0xbf;
			}
		}
		time[i] = bcd2bin(time[i]);
	}

	tm->tm_year = time[0]+100;/*UI base on 1900,but external base on 2000*/
	tm->tm_mon = time[1] - 1; /*upper layer:0~11; s35390a: 1~12*/
	tm->tm_mday = time[2];
	tm->tm_wday = time[3];
	tm->tm_hour = time[4];
	tm->tm_min = time[5];
	tm->tm_sec = time[6];
	
	spin_unlock_irq(&gp_ext_rtc_info->lock);
          
	//DEBUG0("From Reg:%d %d %d %d %d %d\n",time[0],time[1],time[2],time[4],time[5],time[6]);	

	return 0;
}

static int gp_ext_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned char i = 0;
	unsigned char time[7];
	int ret = 0;
	
	DEBUG0("[%s]: Enter!\n", __FUNCTION__);	
	DEBUG0("Y,M,D = %d, %d, %d \n", tm->tm_year, tm->tm_mon, tm->tm_mday);

	if(!gp_ext_rtc_info){
		return (-EPERM);
	}
	
	/*Check year*/	
	if(tm->tm_year >= 1900)
		tm->tm_year -= 1900;

	if(tm->tm_year < 100)	/*Base on 2000*/
		return (-EPERM);
		
	tm->tm_mon += 1; /*upper layer:0~11; s35390a: 1~12*/
	
	time[0] = tm->tm_year-100;	/*UI base on 1900,but external base on 2000*/
	time[1] = tm->tm_mon;	
	time[2] = tm->tm_mday;
	tm->tm_wday = day_of_week(tm->tm_year+1900, tm->tm_mon, tm->tm_mday);	
	time[3] = tm->tm_wday;
	time[4] = tm->tm_hour;
	time[5] = tm->tm_min;
	time[6] = tm->tm_sec;

	//DEBUG0("To Reg:y[%d] m[%d] d[%d] h[%d] m[%d] s[%d]\n",time[0],time[1],time[2],time[4],time[5],time[6]);

	for (i=0;i<7;i++) {
		time[i] = bin2bcd(time[i]);
	}
	
	spin_lock_irq(&gp_ext_rtc_info->lock);
	ret = s35390a_set_reg(EXT_RTC_TIME_REG1, time, 7);/*read real time*/
	if(ret != SP_OK)
		ret = (-EIO);
	spin_unlock_irq(&gp_ext_rtc_info->lock); 	
	
	return ret;
}


static int gp_ext_rtc_get_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int ret = 0;
	DEBUG0("[%s] Enter!\n", __FUNCTION__);
	if(!gp_ext_rtc_info){
		return (-EPERM);
	}
		
	spin_lock_irq(&gp_ext_rtc_info->lock);		
	ret = s35390a_get_alarm(&alrm->enabled, &alrm->pending, &alrm->time);
	if(ret != SP_OK){
		spin_unlock_irq(&gp_ext_rtc_info->lock);  
		return (-EIO);
	}
	
	if(alrm->enabled){
		alrm->time.tm_year = gp_ext_rtc_info->rtc_alarm.tm_year;
		alrm->time.tm_mon = gp_ext_rtc_info->rtc_alarm.tm_mon-1;
		alrm->time.tm_mday = gp_ext_rtc_info->rtc_alarm.tm_mday;
	}
	spin_unlock_irq(&gp_ext_rtc_info->lock);  

	return 0;
}

static int gp_ext_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alrm)
{
	int ret = 0;
	DEBUG0("[%s]: Enter !\n", __FUNCTION__);	
	if(!gp_ext_rtc_info){
		return (-EPERM);
	}
		
	if(alrm->time.tm_year >= 1900)	
		alrm->time.tm_year -= 1900;

	if(alrm->time.tm_year < 100)	/*Base on 2000*/
		return (-EPERM);

	alrm->time.tm_mon += 1;	/*upper layer:0~11; s35390a: 1~12*/
	
	spin_lock_irq(&gp_ext_rtc_info->lock);
	ret = s35390a_set_alarm(alrm->enabled, alrm->pending, &alrm->time);	
	if(ret != SP_OK){
		spin_unlock_irq(&gp_ext_rtc_info->lock);  
		return (-EIO);
	}

	/*Backup to gp_ext_rtc_info*/
	gp_ext_rtc_info->rtc_alarm.tm_year = alrm->time.tm_year;
	gp_ext_rtc_info->rtc_alarm.tm_mon = alrm->time.tm_mon;
	gp_ext_rtc_info->rtc_alarm.tm_mday = alrm->time.tm_mday;
	gp_ext_rtc_info->rtc_alarm.tm_wday = alrm->time.tm_wday;
	gp_ext_rtc_info->rtc_alarm.tm_hour = alrm->time.tm_hour;
	gp_ext_rtc_info->rtc_alarm.tm_min = alrm->time.tm_min;
	gp_ext_rtc_info->rtc_alarm.tm_sec = alrm->time.tm_sec;
	
	spin_unlock_irq(&gp_ext_rtc_info->lock);  
	
	return ret;
}


static const struct rtc_class_ops gp_ext_rtc_ops = {
	.ioctl = gp_ext_rtc_ioctrl,
	.read_time = gp_ext_rtc_get_time,
	.set_time = gp_ext_rtc_set_time,
	.read_alarm = gp_ext_rtc_get_alarm,
	.set_alarm = gp_ext_rtc_set_alarm,
};


static int __init gp_ext_rtc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret = 0;

	gp_ext_rtc_info = kzalloc(sizeof(gp_ext_rtc_t), GFP_KERNEL);
	if (!gp_ext_rtc_info)
		return -ENOMEM;

	if(s35390a_init()){
		DIAG_ERROR("S35390A Initialize Fail!!!\n");
		goto err_init;
	}
#if ENABLE_INT1_OUTPUT_FREQUENCY
	s35390a_int1_output_freq(16);
#endif

	spin_lock_init(&gp_ext_rtc_info->lock);

	gp_ext_rtc_info->rtc = rtc_device_register("gp-ext-rtc", &pdev->dev, &gp_ext_rtc_ops, THIS_MODULE);
	ret = PTR_ERR(gp_ext_rtc_info->rtc);
	if (IS_ERR(gp_ext_rtc_info->rtc)) {
		DIAG_ERROR("Failed to register RTC device -> %d\n", ret);
		goto err_init;
	}
	
	device_init_wakeup(dev, 1);
	
	return 0;

err_init:
	kfree(gp_ext_rtc_info);	
	gp_ext_rtc_info = NULL;
	return ret;
}

static int __exit gp_ext_rtc_remove(struct platform_device *pdev)
{
	if(!gp_ext_rtc_info){
		return (-EPERM);
	}
	rtc_device_unregister(gp_ext_rtc_info->rtc);	
	kfree(gp_ext_rtc_info);	
	gp_ext_rtc_info = NULL;

	return 0;
}

#ifdef CONFIG_PM
static int gp_ext_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int gp_ext_rtc_resume(struct platform_device *pdev)
{
	return 0;
}
#else
#define gp_ext_rtc_suspend		NULL
#define gp_ext_rtc_resume		NULL
#endif

static struct platform_device gp_ext_rtc_device = {
	.name		= "gp-ext-rtc",
	.id		= -1,
	.num_resources  = 0,
	.resource       = 0,
};

static struct platform_driver gp_ext_rtc_driver = {
	.probe		= gp_ext_rtc_probe,
	.remove		= gp_ext_rtc_remove,
	.suspend		= gp_ext_rtc_suspend,
	.resume		= gp_ext_rtc_resume,
	.driver		= {
		.name		= "gp-ext-rtc",
	},
};


static int __init gp_ext_rtc_init(void)
{
	int ret = 0;
	ret = platform_device_register(&gp_ext_rtc_device);
	if (ret != 0){
		DIAG_ERROR("Unable to register gp ext rtc device: %d\n", ret);
		return ret;
	}
	return platform_driver_register(&gp_ext_rtc_driver);
}

static void __exit gp_ext_rtc_exit(void)
{
	platform_driver_unregister(&gp_ext_rtc_driver);
	platform_device_unregister(&gp_ext_rtc_device);
}

module_init(gp_ext_rtc_init);
module_exit(gp_ext_rtc_exit);

MODULE_AUTHOR("GeneralPlus");
MODULE_DESCRIPTION("S35390A RTC driver on G+ open platform");
MODULE_LICENSE_GP;



