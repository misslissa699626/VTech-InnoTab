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
 * @file    hal_usb.c
 * @brief   Implement of SPMP8050 Host/Slave HAL API.
 * @author  allen.chang
 * @since   2010/11/22
 * @date    2010/11/22
 */
#include <linux/kernel.h> 
#include <linux/module.h> 
#include <asm/delay.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hardware.h>
#include <mach/regs-usbhost.h>
#include <mach/regs-usbdev.h>
#include <mach/hal/hal_usb.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define USBBUS_DEBUG_MSGS 1
#if USBBUS_DEBUG_MSGS
#define DLOG(fmt, args...) \
	do { \
		printk(KERN_INFO "[%s:%s:%d] "fmt, __FILE__, __func__, __LINE__, \
			##args); \
	} while (0)
#else
#define DLOG(fmt, args...) do {} while (0)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
//static usbDevReg_t *usbDevReg = (usbDevReg_t *)(LOGI_ADDR_USB_DEV_BUS_REG);

static int usbHostUsage = 0;
int g_first_setup = 0;
int delay_time = 0;
/**
 * @brief PHY1 Enable function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void 
gpHalUsbPhy1En(
	int en
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
}
EXPORT_SYMBOL(gpHalUsbPhy1En);


/**
 * @brief Clock Enable function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void gpHalUsbClockEn ( int en ) {
	if ( en == 1 ) {
		SCUA_USBPHY_CFG |= USBPHY_XTAL_ENABLE;
   }
   else {
	   SCUA_USBPHY_CFG &= ~USBPHY_XTAL_ENABLE;
   }
}
EXPORT_SYMBOL(gpHalUsbClockEn);

/**
 * @brief PHY1 Switch function
 * @param [IN] mode : 
 * @return  None
 * @see
 */
void 
gpHalUsbPhy1Config(
	int mode
)
{
//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if (  mode == HAL_USB_PHY1_HOST ) {
		SCUA_USBPHY_CFG |= (USBPHY_XTAL_ENABLE & (~(USBPHY1_POWER_CTRL)));
		SCUA_USBPHY_CFG |= (USBPHY_HOST_SEL | USBPHY1_CTRL_SEL);
   }
   else if ( mode == HAL_USB_PHY1_SLAVE ) {
	   SCUA_USBPHY_CFG |= (USBPHY_XTAL_ENABLE & (~(USBPHY1_POWER_CTRL)));
	   SCUA_USBPHY_CFG &= ~(USBPHY_HOST_SEL | USBPHY1_CTRL_SEL);	
   }
   else {
	   printk("[%s][%d]WARNING!!! THE MODE DOESN'T SUPPORT!!!\n\n\n", __FUNCTION__, __LINE__);
   }
}
EXPORT_SYMBOL(gpHalUsbPhy1Config);


/**
 * @brief PHY1 Software Connect function
 * @param [IN] connect : [1]Connect [0]Disconnect
 * @return  None
 * @see
 */
void 
gpHalUsbSlaveSwConnect(
	int connect
)
{
	if ( connect == 1 ) {
		/*Force to Connect*/
		UDC_CS = 0x04;
		UDC_LLCSET0 = UDC_LLCSET0 & 0xFE;
	}
	else {
		UDC_LLCSET0 = UDC_LLCSET0 | 0x01;
		/*Force to Disconnect*/
		UDC_CS = 0x08;
	}
}
EXPORT_SYMBOL(gpHalUsbSlaveSwConnect);

/**
 * @brief   Host set delay time to wait first first setup
 * @param   d_t : ms unit
 * @return  none
 * @see
 */
void 
gpHalUsbSetDelayTime(
	int d_t
)
{
	delay_time = d_t;
}
EXPORT_SYMBOL(gpHalUsbSetDelayTime);


/**
 * @brief PHY1 Get Software Connect function Status
 * @param None
 * @return  1:connect, 0:disconnect
 * @see
 */
int 
gpHalUsbSlaveSwConnectGet(
	void
)
{
	if( UDC_LLCSET0 & 0x1 ) {
		return 0;
	}
	else {
		return 1;
	}
}
EXPORT_SYMBOL(gpHalUsbSlaveSwConnectGet);

/**
 * @brief PHY0 Enable function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void 
gpHalUsbPhy0En(
	int en
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
}
EXPORT_SYMBOL(gpHalUsbPhy0En);

/**
 * @brief Host Enable Function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void
gpHalUsbHostEn(
	int en
)
{
	if(en == 0){
		if(usbHostUsage <= 1){
			UH_CTRL = ~MASTER_EN;
			udelay(1000);
			usbHostUsage = 0;
		}
		else{
			usbHostUsage--;
		}
	}
	else{
		if(usbHostUsage == 0){
			UH_CTRL = MASTER_EN;
			udelay(1000);
		}
		usbHostUsage++;
	}
}
EXPORT_SYMBOL(gpHalUsbHostEn);

/**
 * @brief USB PHY Configuration Get
 * @param config [in] : 
 * 2'b00: PHY0 Host PHY1 Device 2'b01: PHY0 Disable PHY1 Device 
 * 2'b10: PHY0 Device PHY1 Host 2'b11: PHY0 Disable PHY1 Host 
 * @return   
 */
void
gpHalUsbPhyConfigSet(
	int config
)
{
	//printk("[%s][%d] [%x]\n", __FUNCTION__, __LINE__, config);
	SCUA_USBPHY_CFG |=  0x03 & config;
	//printk("[%s][%d] [%x]\n", __FUNCTION__, __LINE__, SCUA_USBPHY_CFG);
}
EXPORT_SYMBOL(gpHalUsbPhyConfigSet);

/**
 * @brief USB PHY Configuration Get
 * @param [IN] en : enable
 * @return  None 
 */
int 
gpHalUsbPhyConfigGet(
	void
)
{
	//printk("[%s][%d] [%x]\n", __FUNCTION__, __LINE__, SCUA_USBPHY_CFG);
	return SCUA_USBPHY_CFG & 0x03;
}
EXPORT_SYMBOL(gpHalUsbPhyConfigGet);

/**
 * @brief USB PHY Power Control Set
 * @param phyNum [in] : 0 PHY0, 1 PHY1 
 * @param control [in] : 0 disable, 1 force suspend, 3 force 
 *  			  wakeup.
 * @return   
 */
void
gpHalUsbPhyPowerControlSet(
	int phyNum,
	int control
)
{
	unsigned int temp = SCUA_USBPHY_CFG;
	//printk("[%s][%d] phyNum[%x]en[%x]\n", __FUNCTION__, __LINE__, phyNum, control);
	if( phyNum == 0 ) {
		temp &= 0xfffffcff;
		SCUA_USBPHY_CFG = (temp | (control <<8));
	}
	else{
		temp &= 0xffffcfff;
		SCUA_USBPHY_CFG = (temp | (control <<12));
	}
	//printk("[%s][%d] [%x]\n", __FUNCTION__, __LINE__, SCUA_USBPHY_CFG);
}
EXPORT_SYMBOL(gpHalUsbPhyPowerControlSet);

/**
 * @brief Host Configuration Get
 * @param none
 * @return  0: HOST in PHY0, 1 HOST in PHY1, 2 HOST is disable.
 * @see
 */
int 
gpHalUsbHostConfigGet(
	void
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if( gpHalUsbPhyConfigGet() == 0x0 || gpHalUsbPhyConfigGet() == 0x02 ) {
		return 0;
	}
	else if (  gpHalUsbPhyConfigGet() == 0x3 ){
		return 1;
	}
	else{
		return 2;
	}
}

EXPORT_SYMBOL(gpHalUsbHostConfigGet);

/**
 * @brief Vbus Detect Get
 * @return  1 Host Connect, 0 Host Disconnect
 * @see
 */
int 
gpHalUsbVbusDetect(
	void
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	//return UDC_LLCS & 0x01;
	scubReg_t *pScubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;
	return ((pScubReg->scubPwrcCfg & 0x20000000) == 0x20000000)? 1:0;
}

EXPORT_SYMBOL(gpHalUsbVbusDetect);

/**
 * @brief Host Enumerate State Get
 * @return  1 Host Configed, 0 Host does't enumerate device
 * @see
 */
int 
gpHalUsbHostConfiged(
	void
)
{
	if( (SCUA_USBPHY_CFG & USBPHY_XTAL_ENABLE) == USBPHY_XTAL_ENABLE ) {
		return (UDC_LLCS & 0x40) >> 6;
	}
	else{
		return 0;
	}
}

EXPORT_SYMBOL(gpHalUsbHostConfiged);

/**
 * @brief Host Configuration Get
 * @return  1 Detect host safty remove, 0 Host does't issue safty remove
 * @see
 *	UDC_LLCS contorller status value for different usb state shows as below(2.0/1.1):
 *	[initial]			0x83
 *	[plug in]			0x61/0x67
 *	[safy remove] 		0x45/0x47
 *	[plug out]			0x8E
 *	Bit define
 *	bit7: 	0 for device is connected, 1 for device is disconneted
 *	bit6:	0 for host has not configured, 1 for host has configured device
 *	bit5:	0 for LNK suspend to PHY,	1 for normal
 *	bit4:	0 for host not allow remote wakeup, 1 for allowed
 *	bit3:2	00 for SE0 state, 01 for J state
 *	bit 1	0 for high speed, 1 for full speed
 *	bit 0	0 for host absent, 1 for host present
 */
int 
gpHalUsbHostSafyRemoved(
	void
)
{
	//printk("[%s][%d][%x]\n", __FUNCTION__, __LINE__,UDC_LLCS);

	if(((UDC_LLCS & 0xEF) ==0x45)||//J state,device connected ,LNK suspend to phy and configured,hsot present
		((UDC_LLCS & 0xEF) ==0x47))// full speed 1.1 case
		return 1;
	else
		return 0;
		
}

EXPORT_SYMBOL(gpHalUsbHostSafyRemoved);

/**
 * @brief   Detect PC's first packet 
 * @param   duration => ms unit
 * @return 	0 [not detect in specific duration] 
 * 			1 [detect pc's setup packet in specific duration]
 *  		0xff [ error usage case reply ]
 * @see
 */
int
gpHalUsbDetectFirstPacket(
	void
)
{
	unsigned long end_time;
	int rt=0;

	//printk("vbus=%x host_cfg=%x dur=%d\n",	gpHalUsbVbusDetect(),gpHalUsbHostConfiged(),delay_time);
	
	end_time = jiffies + msecs_to_jiffies(delay_time);
	set_first_setup(0);
	gpHalUsbSlaveSwConnect(1);
	while(!time_after(jiffies, end_time)) {
		//printk("... %x \n",get_first_setup());
		udelay(10);
		if(get_first_setup()){
			printk("Got first setup \n");
			gpHalUsbSlaveSwConnect(0);
			rt = 1;
			set_first_setup(0);
			break;
		}
	}

	gpHalUsbSlaveSwConnect(0);
	//printk("first packet = %x\n",rt);
	return rt;
	
}
EXPORT_SYMBOL(gpHalUsbDetectFirstPacket);

void
set_first_setup(int val){
	g_first_setup = val;
//	printk("got first setup\n");
}
EXPORT_SYMBOL(set_first_setup);

int 
get_first_setup(void){
	return (g_first_setup);
}
EXPORT_SYMBOL(get_first_setup);

