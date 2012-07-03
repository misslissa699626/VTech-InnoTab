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
 * @file    gp_usb.c
 * @brief   Implement of usb driver.
 * @author  allen.chang
 * @since   2010/11/22
 * @date    2010/11/22
 */

 
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/usb.h>
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <asm/uaccess.h>

#include <mach/irqs.h>
#include <mach/gp_usb.h>
#include <mach/hal/hal_usb.h>
#include <mach/module.h>
#include <mach/gp_board.h>
#include <linux/usb/android_composite.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define USB_HIGH_SPEED 1
#define USB_FULL_SPEED 0

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
 
 
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern int usb_test_typeSetting(struct usb_device *udev);
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static long usb_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

typedef struct usb_info_s {
	struct miscdevice dev;      /*!< @brief gpio device */
	struct semaphore sem;       /*!< @brief mutex semaphore for gpio ops */
} usb_info_t;

static struct file_operations usb_device_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = usb_ioctl,
};
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
uint8_t* pUsbBuf = NULL;
static usb_info_t *usb = NULL;

/**
 * @brief   USB Clock enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_clock_en(int en)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	gpHalUsbClockEn (en);
}
EXPORT_SYMBOL(gp_usb_clock_en);

/**
 * @brief   USB software connect setting
 * @param   connect [IN]: [1]connect [0]disconnect
 * @return  none
 * @see
 */
void gp_usb_slave_sw_connect(int connect)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	gpHalUsbSlaveSwConnect(connect);
}
EXPORT_SYMBOL(gp_usb_slave_sw_connect);


/**
 * @brief   USB PHY0 enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_phy0_en(int en)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	gpHalUsbPhy0En (en);
}
EXPORT_SYMBOL(gp_usb_phy0_en);

/**
 * @brief   USB PHY1 enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_phy1_en(int en)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	gpHalUsbPhy1En (en);
}
EXPORT_SYMBOL(gp_usb_phy1_en);

/**
 * @brief   USB PHY1 mode config
 * @param   mode [IN]: USB_PHY1_HOST or USB_PHY1_SLAVE
 * @return  none
 * @see
 */
void gp_usb_phy1_config(int mode)
{
	gpHalUsbPhy1Config (mode);
}
EXPORT_SYMBOL(gp_usb_phy1_config);


/**
 * @brief   USB HOST Enable function
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_host_en(int en)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	gpHalUsbHostEn (en);
}
EXPORT_SYMBOL(gp_usb_host_en);

/**
 * @brief   USB HOST config get
 * @param   none
 * @return  host config, 0 [PHY0] or 1 [PHY1]
 * @see
 */
int gp_usb_host_config_get(void)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return gpHalUsbHostConfigGet();
}
EXPORT_SYMBOL(gp_usb_host_config_get);

/**
 * @brief   USB Slave VBUS Detect Get
 * @param   none
 * @return  1 Host Connect, 0 Host Disconnect
 * @see
 */
int gp_usb_vbus_detect_get(void)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return gpHalUsbVbusDetect();
}
EXPORT_SYMBOL(gp_usb_vbus_detect_get);

/**
 * @brief   USB Slave Host Configed
 * @param   none
 * @return   1 Host Configed, 0 Host does't enumerate device
 * @see
 */
int gp_usb_host_configed(void)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return gpHalUsbHostConfiged();
}
EXPORT_SYMBOL(gp_usb_host_configed);

/**
 * @brief   USB HOST TVID Buf Get
 * @param   pBuf [IN]: the buffer used to stored the TVID.
 * @return  none
 * @see
 */
void gp_usb_tvid_get(uint8_t *pBuf)
{
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if( pBuf != NULL ) {
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, 
		pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
		memcpy(pBuf, pUsbBuf, 512);
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, 
		pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
	}
}
EXPORT_SYMBOL(gp_usb_tvid_get);

/**
 * @brief   USB HOST TVID Buf Set
 * @param   pBuf [IN]: the buffer used to stored the TVID.
 * @return  none
 * @see
 */
void gp_usb_tvid_set( uint8_t *pBuf )
{
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if( pBuf != NULL ) {
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, 
		pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
		memcpy( pUsbBuf, pBuf, 512);
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, 
		pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
	}
}
EXPORT_SYMBOL(gp_usb_tvid_set);

/**
 * @brief   Gpio device ioctl function
 */
static long usb_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	//int handle;
	//usb_content_t ctx;
	usb_content_t usb_content;
	struct usb_device udev;
	uint32_t vbus_detect = 0;
	uint32_t host_config = 0;
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	//unsigned int buf_address;
	//unsigned int* pBuf;

	switch (cmd) {
	case USBDEVFS_PHY_SELECT_SET:
		if (copy_from_user(&usb_content, (void __user*)arg, sizeof(usb_content_t))) {
			ret = -EFAULT;
			break;
		}
		//printk("[%s][%d] USBPHY_SELECT [%x]\n", __FUNCTION__, __LINE__, usb_content.enable);
		if( usb_content.enable == USB_PHY1_SLAVE) {
			printk("Disable Host Power\n");
			pConfig->set_power(1);
		}
		else if ( usb_content.enable == USB_PHY1_HOST ) {
			printk("Diable SW connect\n");
			gp_usb_slave_sw_connect(0);
		}

		gp_usb_phy1_config(usb_content.enable);

		if( usb_content.enable == USB_PHY1_SLAVE) {
			printk("Enable SW connect\n");
			gp_usb_slave_sw_connect(1);
		}
		else if ( usb_content.enable == USB_PHY1_HOST ) {
			printk("Enable Host Power\n");
			pConfig->set_power(0);
		}
		//printk("USBPHY_SELECT END\n");
		break;

	case USBDEVFS_SW_CONNECT_SET:
		printk("[%s][%d] USBSW_CONNECT\n", __FUNCTION__, __LINE__);
		if (copy_from_user(&usb_content, (void __user*)arg, sizeof(usb_content))) {
			ret = -EFAULT;
			break;
		}
		printk("[%s][%d] USBSW_CONNECT [%x]\n", __FUNCTION__, __LINE__, usb_content.enable);
		gp_usb_slave_sw_connect(usb_content.enable);
		printk("[%s][%d] USBSW_CONNECT\n", __FUNCTION__, __LINE__);
		break;

	case USBDEVFS_TVID_SET:
		printk("[%s][%d] USBTVID_SET\n", __FUNCTION__, __LINE__);
		if (copy_from_user(pUsbBuf, (void __user*)arg, 512)) {
			ret = -EFAULT;
			break;
		}
		printk("[%s][%d] USBTVID_SET Buf[%x], 0x[%x], 0x[%x]\n", __FUNCTION__, __LINE__, (uint32_t) pUsbBuf, pUsbBuf[0], pUsbBuf[1]);
		break;

	case USBDEVFS_TVID_GET:
		printk("[%s][%d] USBTVID_GET\n", __FUNCTION__, __LINE__);
		copy_to_user ((void __user *) arg, (const void *)pUsbBuf, 512);
		printk("[%s][%d] USBTVID_GET \n", __FUNCTION__, __LINE__);
		break;

	case USBDEVFS_VBUS_HIGH:
		vbus_detect = gp_usb_vbus_detect_get();
		//printk("[%s][%d] USBDEVFS_VBUS_HIGH [%x]\n", __FUNCTION__, __LINE__, vbus_detect);
		copy_to_user ((void __user *) arg, (const void *) &vbus_detect, sizeof(uint32_t));
		break;

	case USBDEVFS_HOST_CONFIGED:
		host_config = gp_usb_host_configed();
		//printk("[%s][%d] USBDEVFS_HOST_CONFIGED [%x]\n", __FUNCTION__, __LINE__, host_config);
		copy_to_user ((void __user *) arg, (const void *) &host_config, sizeof(uint32_t));
		break;

	case USBDEVFS_HOST_EYE_TEST:
		printk("[%s][%d] USB_EYE_TEST\n", __FUNCTION__, __LINE__);
		if (copy_from_user(&usb_content, (void __user*)arg, sizeof(usb_content_t))) {
			ret = -EFAULT;
			break;
		}
		if( usb_content.enable == USB_TESTMODE_EYE20) {
			printk("USB EYE Pattern 2.0\n");
			udev.descriptor.idProduct = 0x0204;
			//unsigned int pid = 0x104;
			usb_test_typeSetting(&udev);
		}
		else if( usb_content.enable == USB_TESTMODE_EYE11 ) {
			printk("USB EYE Pattern 1.1\n");
			udev.descriptor.idProduct = 0x0205;
			//unsigned int pid = 0x104;
			usb_test_typeSetting(&udev);
		}
		printk("[%s][%d] USB_EYE_TEST FINISH\n", __FUNCTION__, __LINE__);
		break;
	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	return ret;
}



/**
 * @brief   USB enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_en(int en)
{
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	int phy0_config = pConfig->phy0_func_en_get();
	int phy1_config = pConfig->phy1_func_sel_get();
	//int host_speed_sel = pConfig->get_host_speed();
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	if ( en && (phy0_config != 0 || phy1_config != 3) ) {
		gp_usb_clock_en(1);

		if( phy0_config ) {
			printk("PHY0 -> HOST\n");
		}
		else {
			printk("PHY0 -> DISABLE\n");
		}
		if( phy1_config == 0 || phy1_config == 2 ) {
			printk("PHY1 -> HOST or HOST/SLAVE\n");
			gp_usb_phy1_config(USB_PHY1_HOST);
		}
		else if( phy1_config == 1 ) {
			printk("PHY1 -> SLAVE\n");
			gp_usb_phy1_config(USB_PHY1_SLAVE);
		}
		else{
			printk("PHY1 -> DISABLE\n");
		}
		if( phy0_config && (phy1_config == 0 || phy1_config == 2) ) {
			printk("Attention! Host is in PHY1 now. Please change HOST to PHY0 by manual set.\n");
		}
		
		if( !(phy0_config == 0 && phy1_config == 1) ) {
			printk("Enable USB Host\n");
			gp_usb_host_en(1);
		
			printk("Enable Host Power\n");
			pConfig->set_power(1);
		}
	}
	else {
		printk("Disable USB Crystal\n");
		gp_usb_clock_en(0);
	}
	return ;
}
EXPORT_SYMBOL(gp_usb_en);

static int32_t  __init gp_usb_init(void)
{
	int ret = -ENXIO;
	pUsbBuf = kmalloc (512, GFP_KERNEL);
	usb = (usb_info_t *)kzalloc(sizeof(usb_info_t), GFP_KERNEL);
	if (usb == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("gpio kmalloc fail\n");
		goto fail_kmalloc;
	}
	
	gp_usb_en(1);
	/* register device */
	usb->dev.name  = "usb_device";
	usb->dev.minor = MISC_DYNAMIC_MINOR;
	usb->dev.fops  = &usb_device_fops;
	ret = misc_register(&usb->dev);
	if (ret != 0) {
		printk("usb misc device register fail\n");
		goto fail_device_register;
	}

	return 0;

fail_device_register:
	kfree(usb);
	usb = NULL;
fail_kmalloc:
	return ret;
}

static void __exit gp_usb_exit(void)
{
	int ret = -ENXIO;
	//printk("Diable USB Host\n");
	gp_usb_en(0);
	gp_usb_host_en(0);
	if( pUsbBuf != NULL ) {
		kfree(pUsbBuf);
	}
	ret = misc_deregister(&usb->dev);
	if (ret != 0) {
		printk("usb misc device deregister fail\n");
	}
	kfree(usb);
	usb = NULL;
}

/* Declaration of the init and exit functions */
module_init(gp_usb_init);
module_exit(gp_usb_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP USB Driver");
MODULE_LICENSE_GP;
