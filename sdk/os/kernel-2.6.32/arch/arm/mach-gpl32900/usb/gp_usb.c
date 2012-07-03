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
#include <mach/gp_i2c_bus.h>
#include <mach/irqs.h>
#include <mach/gp_usb.h>
#include <mach/hal/hal_usb.h>
#include <mach/module.h>
#include <mach/gp_version.h>
#include <mach/gp_board.h>
#include <mach/general.h>
#include <mach/clock_mgr/gp_clock.h>
#include <mach/hal/hal_clock.h>
#include <linux/usb/android_composite.h>

bool hidden_letter = true;

module_param_named(hidden, hidden_letter, bool, S_IRUGO);

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define USB_HIGH_SPEED 1
#define USB_FULL_SPEED 0

#define Hidden_Driver_Letter
//#define DEBUG_PRINT

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
 
 
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
extern int usb_test_typeSetting(struct usb_device *udev);
extern char* usb_device_connect_state_get ( void );
extern unsigned int spmp_vbus_detect( void );
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
static long gp_usb_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
//static int gp_usb_fops_release(struct inode *inode, struct file *file);
/* ---- This structure is used for vtech control ----- */
#ifdef Hidden_Driver_Letter
typedef struct vt_ctrl_s
{
	struct semaphore 		ra2k;		/*!< @brief Read ap wait for kernel semaphore. */
	struct semaphore 		rk2a;		/*!< @brief Read kernal wait for ap semaphore. */
	struct semaphore 		wa2k;		/*!< @brief Write ap wait for kernel semaphore. */
	struct semaphore 		wk2a;		/*!< @brief Write kernal wait for ap semaphore. */
	struct semaphore 		prot_cbw;	/*!< @brief Protect write cbw buffer */
	wait_queue_head_t		cbw2k;		/*!< @brief CBW wait for kernel semaphore. */
	volatile unsigned long 	status;		/*!< @brief Check status, bit0 for CBW status. */
	unsigned char			rdata[512];	/*!< @brief Read data buffer */
	unsigned char			wdata[512];	/*!< @brief Write data buffer */
	unsigned char			cbw[31];	/*!< @brief CBW buffer */
}vt_ctrl_t;
#endif

typedef struct usb_info_s {
	struct miscdevice dev;      /*!< @brief gpio device */
	struct semaphore sem;       /*!< @brief mutex semaphore for gpio ops */
#ifdef Hidden_Driver_Letter
	vt_ctrl_t		vt;
#endif
} usb_info_t;

static struct file_operations usb_device_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = gp_usb_fops_ioctl,
	//.release = gp_usb_fops_release
};
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static uint8_t* pUsbBuf = NULL;
static usb_info_t *usb = NULL;
static int usb_wait_disk_sync = 0;
//static struct semaphore sem; 
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
 * @brief   Host set delay time to wait first first setup
 * @param   d_t : ms unit
 * @return  none
 * @see
 */
void gp_usb_set_delay_time(int d_t)
{
	//printk("[%s][%d][%\n", __FUNCTION__, __LINE__);
	gpHalUsbSetDelayTime(d_t);
}
EXPORT_SYMBOL(gp_usb_set_delay_time);


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
 * @return  0: HOST in PHY0, 1 HOST in PHY1, 2 HOST is disable.
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
 * @brief   USB HOST safty remove
 * @param   none
 * @return  host , 0 [not safty remove] or 1 [issue fsaty remove]
 * @see
 */
int gp_usb_host_safty_removed(void)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return gpHalUsbHostSafyRemoved();
}
EXPORT_SYMBOL(gp_usb_host_safty_removed);

/**
 * @brief   Detect PC's first packet 
 * @param   none
 * @return 	0 [not detect in specific duration] 
 * 			1 [detect pc's setup packet in specific duration]
 *  		0xff [ error usage case reply ]
 * @see
 */
int gp_usb_detect_first_packet(void)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	return gpHalUsbDetectFirstPacket();
}
EXPORT_SYMBOL(gp_usb_detect_first_packet);


/**
 * @brief   Set for waiting disk was sync.
 * @param   setting [IN]: [1] wait disk sync, [0] not wait.
 * @return 	none.
 * @see
 */
static void gp_usb_disk_sync_wait_set( int setting ){
	if( setting ) {
		usb_wait_disk_sync = setting;
	}
	else{
		usb_wait_disk_sync = 0;
	}
}

/**
 * @brief   Get flag for waiting disk was sync.
 * @param   none
 * @return 	return [1] wait disk sync, [0] not wait.
 * @see
 */
int gp_usb_disk_sync_wait_get( void ){
	return usb_wait_disk_sync;
}
EXPORT_SYMBOL(gp_usb_disk_sync_wait_get);

/**
 * @brief   USB HOST TVID Buf Get
 * @param   pBuf [OUT]: the buffer used to stored the TVID.
 * @return  none
 * @see
 */
void gp_usb_tvid_get(uint8_t *pBuf)
{
#ifdef DEBUG_PRINT
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	if( pBuf != NULL ) {
#ifdef DEBUG_PRINT
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
#endif
		memcpy(pBuf, pUsbBuf, 512);
#ifdef DEBUG_PRINT
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
#endif
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
#ifdef DEBUG_PRINT
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	if( pBuf != NULL ) {
#ifdef DEBUG_PRINT
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
#endif
		
		memcpy( pUsbBuf, pBuf, 512);
#ifdef DEBUG_PRINT
		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, pUsbBuf[0], pUsbBuf[1], pUsbBuf[2]);
#endif
		
	}
}
EXPORT_SYMBOL(gp_usb_tvid_set);

/**
 * @brief   USB CBW Buf Get
 * @param   pBuf [OUT]: the buffer used to stored the CBW.
 * @return  none
 * @see
 */
#ifdef Hidden_Driver_Letter
void gp_usb_cbw_get(uint8_t *pBuf) //eddie
{
	if( pBuf != NULL ) 
	{
//		printk("[%s][%d], [%x][%x][%x]\n", __FUNCTION__, __LINE__, pBuf[0], pBuf[1], pBuf[2]);
		//if (usb->vt.status & 1) {
		//	printk("ERROR ERROR ERROR!!!\n");
		//}
		down(&usb->vt.prot_cbw);
		memcpy(usb->vt.cbw,pBuf,31);
		up(&usb->vt.prot_cbw);
		set_bit(0,&usb->vt.status);
		wake_up_interruptible(&usb->vt.cbw2k);
		wait_event_interruptible(usb->vt.cbw2k, test_and_clear_bit(1, &usb->vt.status));
		//up(&usb->vt.cbw2k);
	}
}
EXPORT_SYMBOL(gp_usb_cbw_get);

/**
 * @brief   USB HOST Data Buf Set
 * @param   pBuf [IN]: the buffer used to stored the pUSBDataBuf.
 * @return  none
 * @see
 */
void gp_usb_data_set( uint8_t *pBuf )
{
	if( pBuf != NULL ) 
	{
		/* ----- Wake up usb ioctrl write buffer -----*/
		up(&usb->vt.ra2k);
		/* ----- Wait for buffer ready ----- */
		//down(&usb->vt.rk2a);
		if(down_interruptible(&usb->vt.rk2a) == 0)
		/* ----- Copy to usb buffer ----- */
			memcpy(pBuf, usb->vt.rdata, 512);
	}
	else
	{
#ifdef DEBUG_PRINT
		printk("[%s][%d] ### NULL ###\n", __FUNCTION__, __LINE__);
#endif

	}
}
EXPORT_SYMBOL(gp_usb_data_set);

/**
 * @brief   USB HOST Data Buf get
 * @param   pBuf [IN]: the buffer used to stored the pUSBDataBuf.
 * @return  none
 * @see
 */
void gp_usb_data_get( uint8_t *pBuf )
{
	if( pBuf != NULL ) 
	{
		/* ----- Wait for buffer ready ----- */
		//down(&usb->vt.wk2a);
		if(down_interruptible(&usb->vt.wk2a) == 0)
		{
		/* ----- Copy from usb buffer ----- */
			memcpy(usb->vt.wdata, pBuf, 512);
		/* ----- Wake up usb ioctrl write buffer -----*/
			up(&usb->vt.wa2k);
		}
	}
	else
	{
#ifdef DEBUG_PRINT
		printk("[%s][%d] ### NULL ###\n", __FUNCTION__, __LINE__);
#endif

	}
}
EXPORT_SYMBOL(gp_usb_data_get);
#endif
#if 0
static int gp_usb_fops_release(struct inode *inode, struct file *file)
{
	int phyCurrentConfig = 0, phy1SwConnect = 0;
	phyCurrentConfig = gpHalUsbPhyConfigGet();
	
#ifdef DEBUG_PRINT
	printk("[%s][%d]\n", __FUNCTION__, __LINE__);
#endif
	if( !((phyCurrentConfig & 0x03) == USB_HOST_PHY1_DEV_NULL) ) {
#ifdef DEBUG_PRINT
		printk("Disable SW\n");
#endif
		phy1SwConnect = gpHalUsbSlaveSwConnectGet();
		gp_usb_slave_sw_connect(0);
	}
	gp_usb_en( phyCurrentConfig, 0);
	return 0;
}
#endif
/**
 * @brief   usb device ioctl function
 */
static long gp_usb_fops_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	//int handle;
	//usb_content_t ctx;
	usb_content_t usb_content;
	uint32_t delay_time;
	struct usb_device udev;
	uint32_t vbus_detect = 0;
	uint32_t host_config = 0;
	uint32_t safty_remove = 0;
	uint32_t first_packet = 0;	
	uint32_t wait_disk_sync = 0;
	char* usbDiskPlugStatus = NULL;
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	//unsigned int buf_address;
	//unsigned int* pBuf;

	switch (cmd) {
	case USBDEVFS_PHY_SELECT_SET:
		if (copy_from_user(&usb_content, (void __user*)arg, sizeof(usb_content_t))) {
			ret = -EFAULT;
			break;
		}
#ifdef DEBUG_PRINT
		printk("[%s][%d] USBPHY_SELECT [%x]\n", __FUNCTION__, __LINE__, usb_content.enable);
#endif
		if( usb_content.enable == USB_PHY1_SLAVE) {
#ifdef DEBUG_PRINT
			printk("Disable Host Power\n");
#endif
			pConfig->set_power(1);
		}
		else if ( usb_content.enable == USB_PHY1_HOST ) {
#ifdef DEBUG_PRINT
			printk("Diable SW connect\n");
#endif
			gp_usb_slave_sw_connect(0);
		}

		gp_usb_phy1_config(usb_content.enable);

		if( usb_content.enable == USB_PHY1_SLAVE) {
#ifdef DEBUG_PRINT
			printk("Enable SW connect\n");
#endif
			gp_usb_slave_sw_connect(1);
		}
		else if ( usb_content.enable == USB_PHY1_HOST ) {
#ifdef DEBUG_PRINT
			printk("Enable Host Power\n");
#endif
			pConfig->set_power(0);
		}
		//printk("USBPHY_SELECT END\n");
		break;

	case USBDEVFS_SW_CONNECT_SET:
//		printk("[%s][%d] USBSW_CONNECT\n", __FUNCTION__, __LINE__);
		if (copy_from_user(&usb_content, (void __user*)arg, sizeof(usb_content))) {
			ret = -EFAULT;
			break;
		}
	//	printk("[%s][%d] USBSW_CONNECT [%x]\n", __FUNCTION__, __LINE__, usb_content.enable);
		gp_usb_slave_sw_connect(usb_content.enable);
		//printk("[%s][%d] USBSW_CONNECT\n", __FUNCTION__, __LINE__);
		break;

	case USBDEVFS_TVID_SET:
#ifdef DEBUG_PRINT
		printk("[%s][%d] USBTVID_SET\n", __FUNCTION__, __LINE__);
#endif
		if (copy_from_user(pUsbBuf, (void __user*)arg, 512)) {
			ret = -EFAULT;
			break;
		}
#ifdef DEBUG_PRINT
		printk("[%s][%d] USBTVID_SET Buf[%x], 0x[%x], 0x[%x]\n", __FUNCTION__, __LINE__, (uint32_t) pUsbBuf, pUsbBuf[0], pUsbBuf[1]);
#endif
		break;

	case USBDEVFS_TVID_GET:
#ifdef DEBUG_PRINT
		printk("[%s][%d] USBTVID_GET\n", __FUNCTION__, __LINE__);
#endif
		copy_to_user ((void __user *) arg, (const void *)pUsbBuf, 512);
#ifdef DEBUG_PRINT
		printk("[%s][%d] USBTVID_GET \n", __FUNCTION__, __LINE__);
#endif
		break;

	case USBDEVFS_DISK_PLUG_STATUS_GET:
		usbDiskPlugStatus = usb_device_connect_state_get();
		copy_to_user ((void __user *) arg, (int *)usbDiskPlugStatus, 16);
#ifdef Hidden_Driver_Letter
	case USBDEVFS_GETCBW: //eddie

		if(wait_event_interruptible(usb->vt.cbw2k,test_and_clear_bit(0,&usb->vt.status)) == 0)
		{
			down(&usb->vt.prot_cbw);
			ret = copy_to_user ((void __user *) arg, usb->vt.cbw, 31);
			//printk("[%s][%d] USBDEVFS_GETCBW = 0x%x 0x%x 0x%x\n", __FUNCTION__, __LINE__, *(unsigned int*)(&usb->vt.cbw[12]), *(unsigned int*)(&usb->vt.cbw[16]), *(unsigned int*)(&usb->vt.cbw[20]));
			up(&usb->vt.prot_cbw);
		}
		else
			return -ERESTARTSYS;
		set_bit(1, &usb->vt.status);
		wake_up_interruptible(&usb->vt.cbw2k);
//		printk("[%s][%d] USBDEVFS_GETCBW : %x \n", __FUNCTION__, __LINE__, ret);
		break;

	case USBDEVFS_WRITE_USB_BUFFER: //eddie
		/* -----  Wait pc read in gp_usb_data_set() ----- */
		//down(&usb->vt.ra2k);
		if(down_interruptible(&usb->vt.ra2k) == 0)
		{
			ret = copy_from_user(usb->vt.rdata, (void __user*)arg, 512);
	//		printk("[%s][%d] USBDEVFS_WRITE_USB_BUFFER ARG= 0x%x \n", __FUNCTION__, __LINE__, *(unsigned int*)arg);
	//		printk("[%s][%d] USBDEVFS_WRITE_USB_BUFFER = 0x%x \n", __FUNCTION__, __LINE__, *(unsigned int*)usb->vt.rdata);
		
			/* ----- Wake up in gp_usb_data_set() ----- */
			up(&usb->vt.rk2a);
		}
		else
			return -ERESTARTSYS;
		
		break;
	
	case USBDEVFS_READ_USB_BUFFER: //eddie
		/* ----- Wake up in gp_usb_data_get() ----- */
		up(&usb->vt.wk2a);
		/* -----  Wait pc read in gp_usb_data_get() ----- */
		//down(&usb->vt.wa2k);
		if(down_interruptible(&usb->vt.wa2k) == 0)
		{
			ret = copy_to_user((void __user*)arg, usb->vt.wdata, 512);
		}
		else
			return -ERESTARTSYS;
			
		break;
#endif
	case USBDEVFS_VBUS_HIGH:
		vbus_detect = pConfig->slave_detect();
#ifdef DEBUG_PRINT
		printk("[%s][%d] USBDEVFS_VBUS_HIGH [%x]\n", __FUNCTION__, __LINE__, vbus_detect);
#endif
		copy_to_user ((void __user *) arg, (const void *) &vbus_detect, sizeof(uint32_t));
		break;

	case USBDEVFS_HOST_CONFIGED:
		host_config = gp_usb_host_configed();
		//printk("[%s][%d] USBDEVFS_HOST_CONFIGED [%x]\n", __FUNCTION__, __LINE__, host_config);
		copy_to_user ((void __user *) arg, (const void *) &host_config, sizeof(uint32_t));
		break;

	case USBDEVFS_HOST_EYE_TEST:
#ifdef DEBUG_PRINT
		printk("[%s][%d] USB_EYE_TEST\n", __FUNCTION__, __LINE__);
#endif
		if (copy_from_user(&usb_content, (void __user*)arg, sizeof(usb_content_t))) {
			ret = -EFAULT;
			break;
		}
		if( usb_content.enable == USB_TESTMODE_EYE20) {
#ifdef DEBUG_PRINT
			printk("USB EYE Pattern 2.0\n");
#endif
			udev.descriptor.idProduct = 0x0204;
			//unsigned int pid = 0x104;
			usb_test_typeSetting(&udev);
		}
		else if( usb_content.enable == USB_TESTMODE_EYE11 ) {
#ifdef DEBUG_PRINT
			printk("USB EYE Pattern 1.1\n");
#endif
			udev.descriptor.idProduct = 0x0205;
			//unsigned int pid = 0x104;
			usb_test_typeSetting(&udev);
		}
#ifdef DEBUG_PRINT
		printk("[%s][%d] USB_EYE_TEST FINISH\n", __FUNCTION__, __LINE__);
#endif
		break;

	case USBDEVFS_HOST_SAFTY_REMOVED:
		safty_remove = gp_usb_host_safty_removed();
		//printk("[%s][%d] USBDEVFS_HOST_SAFTY_REMOVED [%x]\n", __FUNCTION__, __LINE__, safty_remove);
		copy_to_user ((void __user *) arg, (const void *) &safty_remove, sizeof(uint32_t));
		break;

	case USBDEVFS_SET_DELAY_TIME:
		if (copy_from_user(&delay_time, (void __user*)arg, sizeof(uint32_t))) {
			ret = -EFAULT;
			break;
		}
		//printk("USBSW_SET_DELAY_TIME [%d]\n",delay_time);
		gp_usb_set_delay_time(delay_time);
		break;

	case USBDEVFS_DETECT_FIRST_PACKET:
		first_packet = gp_usb_detect_first_packet();
#ifdef DEBUG_PRINT
		printk("USBDEVFS_DETECT_FIRST_PACKET [%x]\n",first_packet);
#endif
		copy_to_user ((void __user *) arg, (const void *) &first_packet, sizeof(uint32_t));
		break;

	case USBDEVFS_WAIT_DISK_SYNC:
		if (copy_from_user(&wait_disk_sync, (void __user*)arg, sizeof(uint32_t))) {
			ret = -EFAULT;
			break;
		}
		gp_usb_disk_sync_wait_set(wait_disk_sync);
		printk("USBDEVFS_WAIT_DISK_SYNC.[%x]\n",wait_disk_sync);
		break;

	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	return ret;
}



/**
 * @brief   USB enable 
 * @param   phyConfig [IN]: config USB PHY 
 * 2'b00: PHY0 Host PHY1 Device 2'b01: PHY0 Disable PHY1 Device 
 * 2'b10: PHY0 Device PHY1 Host 2'b11: PHY0 Disable PHY1 Host 
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_en(int phyConfig, int en)
{
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);

	if ( en ) {
		/*Reset default value.*/
		printk("Reset Setting\n");
		gpHalUsbPhyPowerControlSet(0, 0x1);
		gpHalUsbPhyPowerControlSet(1, 0x1);
		gpHalScuUsbPhyClkEnable(0);
		gpHalScuClkEnable(SCU_A_PERI_USB0 | SCU_A_PERI_USB1, SCU_A, 0);

		if( phyConfig == USB_HOST_PHY0_DEV_PHY1) {
			printk("Enable PHY CLK\n");
			gpHalScuUsbPhyClkEnable(1);
			printk("Resume PHY\n");
			gpHalUsbPhyPowerControlSet(0, 0x0);
			gpHalUsbPhyPowerControlSet(1, 0x0);
			printk("Enable SYS_A\n");
			gp_enable_clock((int*)"SYS_A", 1);
			printk("Enable USB0 CLK\n");
			gpHalScuClkEnable(SCU_A_PERI_USB0, SCU_A, 1);
		}
		else if( phyConfig == USB_HOST_NULL_DEV_PHY1) {
			#if 0
			printk("Resume PHY\n");
			gpHalScuUsbPhyClkEnable(1);
			gpHalUsbPhyPowerControlSet(1, 0x0);
			printk("Enable SYS_A\n");
			gp_enable_clock((int*)"SYS_A", 1);
			printk("Enable USB1 CLK\n");
			gpHalScuClkEnable(SCU_A_PERI_USB1, SCU_A, 1);
			#endif
			/*donothing*/
		}
		else if( phyConfig == USB_HOST_PHY1_DEV_NULL ){
			printk("Enable PHY CLK\n");
			gpHalScuUsbPhyClkEnable(1);
			printk("Resume PHY\n");
			gpHalUsbPhyPowerControlSet(1, 0x0);
			printk("Enable SYS_A\n");
			gp_enable_clock((int*)"SYS_A", 1);
			printk("Enable USB1 CLK\n");
			gpHalScuClkEnable(SCU_A_PERI_USB1, SCU_A, 1);
		}
		printk("Reset Config\n");
		gpHalUsbPhyConfigSet(phyConfig);

		if( !((phyConfig & 0x03) == USB_HOST_NULL_DEV_PHY1) ){
			printk("[%s]Enable Host/Power\n", __FUNCTION__);
			gp_usb_host_en(1); /*ok*/
			pConfig->set_power(1); /*ok*/
		}
	}
	else {
		/*Force PHY Suspend*/
		if( gpHalUsbPhyConfigGet() != USB_HOST_PHY0_DEV_PHY1) {
			if(!((phyConfig & 0x03) == USB_HOST_NULL_DEV_PHY1)){
				printk("[%s]Disable Host/Power\n", __FUNCTION__);
				pConfig->set_power(0); /*ok*/
				gp_usb_host_en(0); /*ok*/
			}
			printk("Disable PHY setting\n");
			gpHalUsbPhyConfigSet(0x0);
			gpHalUsbPhyPowerControlSet(0, 0x1);
			gpHalUsbPhyPowerControlSet(1, 0x1);
			printk("Disable PHY CLK\n");
			gpHalScuUsbPhyClkEnable(0);
			printk("Disable USB0/USB1 CLK\n");
			gpHalScuClkEnable(SCU_A_PERI_USB0 | SCU_A_PERI_USB1, SCU_A, 0);
			gp_enable_clock((int*)"SYS_A", 0);
		}
		else{
			printk("Host is working!\n");
			gpHalUsbPhyPowerControlSet(1, 0x1);
			printk("Disable USB1 CLK\n");
			gpHalScuClkEnable(SCU_A_PERI_USB1, SCU_A, 0);
		}
	}
	return;
}
EXPORT_SYMBOL(gp_usb_en);

static void gp_usb_device_release(struct device * dev)
{
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
}

static struct platform_device gp_usb_device = {
	.name	= "gp-usb",
	.id	= -1,
	.dev		= {
		.release = &gp_usb_device_release
	},
};


#ifdef CONFIG_PM
static unsigned int phyCurrentConfig = 0;
static unsigned int phy1SwConnect = 0;
static int gp_usb_suspend(struct platform_device *pdev, pm_message_t state){

#ifdef CONFIG_PM_GPFB
	/* GPFB_TBD */
	if (pm_device_down)
		return 0;
#endif
	//printk("[%s][%d]\n", __FUNCTION__, __LINE__);
	phyCurrentConfig = gpHalUsbPhyConfigGet();

	if( !((phyCurrentConfig & 0x03) == USB_HOST_PHY1_DEV_NULL) ) {
#ifdef DEBUG_PRINT
		printk("Disable SW\n");
#endif
		phy1SwConnect = gpHalUsbSlaveSwConnectGet();
		gp_usb_slave_sw_connect(0);
	}
	gp_usb_en( phyCurrentConfig, 0);

	return 0;
}

static int gp_usb_resume(struct platform_device *pdev){

#ifdef CONFIG_PM_GPFB
	/* GPFB_TBD */
	if (pm_device_down)
		return 0;
#endif

	//printk("[%s][%d][%x]\n", __FUNCTION__, __LINE__, phyCurrentConfig);
	gp_usb_en( phyCurrentConfig, 1);
	if( !((phyCurrentConfig & 0x03) == USB_HOST_PHY1_DEV_NULL) ) {
		/*Delay is needed for resume process.*/
		msleep(1);
#ifdef DEBUG_PRINT
		printk("Set SW to [%x]\n", phy1SwConnect);
#endif
		gp_usb_slave_sw_connect(phy1SwConnect);
	}
	return 0;
}
#else
#define gp_usb_suspend NULL
#define gp_usb_resume NULL
#endif

/**
 * @brief   wdt driver define
 */
static struct platform_driver gp_usb_driver = {
	//.probe	= gp_usb_control_probe,
	//.remove	= gp_usb_control_remove,
	.suspend = gp_usb_suspend,
	.resume = gp_usb_resume,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "gp-usb"
	},
};


/**
 * @brief   Get default config from configuration making.
 * @return  0: PHY0 Host PHY1 Device 
 * 1: PHY0 Disable PHY1 Device 
 * 3: PHY0 Disable PHY1 Host
 * @see
 */
static int gp_usb_default_phy_config_get ( void ){
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	int phy0_config = pConfig->phy0_func_en_get();
	/*0: phy0 host, 1: phy0 disable*/
	int phy1_config = pConfig->phy1_func_sel_get();
	/*0: phy1 host, 1: phy1 slave, 2: phy1 host/slave, 3 phy1 disable*/
	//int host_speed_sel = pConfig->get_host_speed();
	int phyConfig = 0;
	if( phy0_config == PHY0_HOST ) {
#ifdef DEBUG_PRINT
		printk("PHY0 -> HOST\n");
#endif
	}
	else if( phy0_config == PHY0_DISABLE ){
#ifdef DEBUG_PRINT
		printk("PHY0 -> DISABLE\n");
#endif
	}
	if( phy1_config == PHY1_HOST || phy1_config == PHY1_HOST_SLAVE ) {
#ifdef DEBUG_PRINT
		printk("PHY1 -> HOST or HOST/SLAVE\n");
#endif
		phyConfig = USB_HOST_PHY1_DEV_NULL;
	}
	else if( phy1_config == PHY1_SLAVE ) {
#ifdef DEBUG_PRINT
		printk("PHY1 -> SLAVE\n");
#endif
		if( phy0_config == PHY0_HOST ) {
			phyConfig = USB_HOST_PHY0_DEV_PHY1;
		}
		else{
			phyConfig = USB_HOST_NULL_DEV_PHY1;
		}
	}
	else if( phy1_config == PHY1_DISABLE ) {
#ifdef DEBUG_PRINT
		printk("PHY1 -> DISABLE\n");
#endif
		phyConfig = USB_HOST_PHY0_DEV_PHY1;
	}
	if( phy0_config && (phy1_config == 0 || phy1_config == 2) ) {
#ifdef DEBUG_PRINT
		printk("Attention! Host is in PHY1 now. Please change HOST to PHY0 by manual set.\n");
#endif
	}
	return phyConfig;
}

static int gp_usb_intI2c_DataWrite (UINT32 slaveAddr, u8 *buf, int len) {
		UINT32 ret = 0, i = 0;
		int hd = 0;

		hd = gp_i2c_bus_request(slaveAddr, 0x0f);
		if(IS_ERR_VALUE(ret)){
			printk(KERN_ALERT "err gp_i2c_bus_request\n");
		}
	
		for(i=0; i<1; i++){
			gp_i2c_bus_write(hd, (unsigned char *)&buf[i], len);
		}
		gp_i2c_bus_release(hd);
		return SUCCESS;
}

#define PHY0_I2C_ID 0x08
#define PHY1_I2C_ID 0x0a
#define VOLTAGE_LEVEL_CHANGE_ADDRESS 0x0d

static int usb_voltage_level_change( int physelect, int level ) {
	int err;
	int phy_id_select = PHY0_I2C_ID;
	UINT8 buf[8] = {0};

	buf[0] = VOLTAGE_LEVEL_CHANGE_ADDRESS;	
	buf[1] = 0xa0 + level;	
		
	if ( physelect == 0 ) {
		phy_id_select = PHY0_I2C_ID;
	}
	else {
		phy_id_select = PHY1_I2C_ID;	
	}
 	printk("Voltage Up-PHY[%d]level[%x]\n", physelect, level);
	err = gp_usb_intI2c_DataWrite(phy_id_select, buf, 2);
 	return 0;
}

static int32_t  __init gp_usb_init(void)
{
	int ret = -ENXIO;
	int phyDefaultConfig = 0;
	struct gp_board_usb_s *pConfig = gp_board_get_config("usb", gp_board_usb_t);
	int phy0voltageUpValue = (pConfig->get_phy0_voltage_up_config != NULL) ? (pConfig->get_phy0_voltage_up_config()) : 0; 
	int phy1voltageUpValue = (pConfig->get_phy1_voltage_up_config != NULL) ? (pConfig->get_phy1_voltage_up_config()) : 0; 

	pUsbBuf = kmalloc (512, GFP_KERNEL);
	usb = (usb_info_t *)kzalloc(sizeof(usb_info_t), GFP_KERNEL);
	if (usb == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("gpio kmalloc fail\n");
		goto fail_kmalloc;
	}
#ifdef Hidden_Driver_Letter
	init_MUTEX_LOCKED(&usb->vt.ra2k);
	init_MUTEX_LOCKED(&usb->vt.rk2a);
	init_MUTEX_LOCKED(&usb->vt.wa2k);
	init_MUTEX_LOCKED(&usb->vt.wk2a);
	init_MUTEX(&usb->vt.prot_cbw);
	init_waitqueue_head(&usb->vt.cbw2k);
#endif	
	phyDefaultConfig = gp_usb_default_phy_config_get();
	gp_usb_en( phyDefaultConfig, 1);

	if(  phy0voltageUpValue != 0 ) {
		usb_voltage_level_change( 0, phy0voltageUpValue);
	}
	if(  phy1voltageUpValue != 0 ) {
		usb_voltage_level_change( 1, phy1voltageUpValue);
	}
	/* register device */
	usb->dev.name  = "usb_device";
	usb->dev.minor = MISC_DYNAMIC_MINOR;
	usb->dev.fops  = &usb_device_fops;
	ret = misc_register(&usb->dev);
	if (ret != 0) {
#ifdef DEBUG_PRINT
		printk("usb misc device register fail\n");
#endif
		goto fail_device_register;
	}
	//sema_init( &sem, 0);
	platform_device_register(&gp_usb_device);
	platform_driver_register(&gp_usb_driver);
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
	int phyCurrentConfig = 0;
	phyCurrentConfig = gpHalUsbPhyConfigGet();
	gp_usb_en(phyCurrentConfig, 0);
	//gp_usb_host_en(0);
	if( pUsbBuf != NULL ) {
		kfree(pUsbBuf);
	}
#ifdef Hidden_Driver_Letter
	printk("###wake up ra2k ###\n");
	up(&usb->vt.ra2k);
	up(&usb->vt.rk2a);
	up(&usb->vt.wa2k);
	up(&usb->vt.wk2a);
	//up(&usb->vt.cbw2k);
#endif
	ret = misc_deregister(&usb->dev);
	if (ret != 0) {
		printk("usb misc device deregister fail\n");
	}
	kfree(usb);
	usb = NULL;

	platform_device_unregister(&gp_usb_device);
	platform_driver_unregister(&gp_usb_driver);
}

/* Declaration of the init and exit functions */
module_init(gp_usb_init);
module_exit(gp_usb_exit);

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("GP USB Driver");
MODULE_LICENSE_GP;
