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
#include <mach/hardware.h>
#include <mach/regs-scu.h>
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
static int usbHostUsage = 0;

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
 * @brief Host Configuration Get
 * @param [IN] en : enable
 * @return  None
 * @see
 */
int 
gpHalUsbHostConfigGet(
	void
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return SCUA_USBPHY_CFG & 0x01;
}

EXPORT_SYMBOL(gpHalUsbHostConfigGet);

/**
 * @brief Host Configuration Get
 * @return  1 Host Connect, 0 Host Disconnect
 * @see
 */
int 
gpHalUsbVbusDetect(
	void
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return UDC_LLCS & 0x01;
}

EXPORT_SYMBOL(gpHalUsbVbusDetect);

/**
 * @brief Host Configuration Get
 * @return  1 Host Configed, 0 Host does't enumerate device
 * @see
 */
int 
gpHalUsbHostConfiged(
	void
)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return (UDC_LLCS & 0x40) >> 6;
}

EXPORT_SYMBOL(gpHalUsbHostConfiged);




