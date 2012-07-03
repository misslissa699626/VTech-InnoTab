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
 * @file    gp_usb.h
 * @brief   Declaration of usb driver.
 * @author  allen.chang
 * @since   2010/11/22
 * @date    2010/11/22
 */
#ifndef _GP_USB_H_
#define _GP_USB_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define USB_PHY1_SLAVE 0
#define USB_PHY1_HOST 1

#define USB_SLAVE_VBUS_POWERON1 -1
#define USB_SLAVE_VBUS_GPIO 0x1
#define USB_SLAVE_VBUS_NONE 0xff

enum {
	USB_TESTMODE_EYE11 = 0,
	USB_TESTMODE_EYE20,
    USB_TESTMODE_DISABLE
};

enum {
    PHY0 = 0,
    PHY1
};

enum {
    PHY0_DISABLE = 0,
    PHY0_HOST
};

enum {
    PHY1_HOST = 0,
    PHY1_SLAVE,
    PHY1_HOST_SLAVE,
    PHY1_DISABLE
};

enum {
	USB_HOST_PHY0_DEV_PHY1 = 0,
	USB_HOST_NULL_DEV_PHY1,
    USB_HOST_PHY0_DEV_PHY1_2,
    USB_HOST_PHY1_DEV_NULL
};

enum usb_host_port_status{
	USB_PORT_STATUS_PLUGOUT = 0,
	USB_PORT_STATUS_PLUGIN,
	USB_PORT_STATUS_TOMANY_HUBS,
	USB_PORT_STATUS_UNSUPPORTED_DEVICE,
};

enum usb_slave_infor_index{
    USB_INDEX_VID = 0,
    USB_INDEX_PID,
	USB_INDEX_SERIAL,
	USB_INDEX_VENDOR_NAME,
	USB_INDEX_PRODUCT_NAME,
    USB_INDEX_MSDC_MANUFACTURER_NAME,
    USB_INDEX_MSDC_PRODUCT_NAME,
	USB_INDEX_MSDC_REVISION_ID,
};


/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
/* Ioctl for device node definition */
#define USB_IOCTL_ID           'U'
#define USBDEVFS_PHY_SELECT_SET    _IOW(USB_IOCTL_ID, 0, int)
#define USBDEVFS_SW_CONNECT_SET    _IOW(USB_IOCTL_ID, 1, int)
#define USBDEVFS_TVID_SET			  _IOW(USB_IOCTL_ID, 2, uint32_t*)
#define USBDEVFS_TVID_GET    		  _IOR(USB_IOCTL_ID, 3, uint32_t*)
#define USBDEVFS_VBUS_HIGH			  _IOR(USB_IOCTL_ID, 4, uint32_t)
#define USBDEVFS_HOST_CONFIGED	  _IOR(USB_IOCTL_ID, 5, uint32_t)
#define USBDEVFS_HOST_EYE_TEST      _IOW(USB_IOCTL_ID, 6, uint32_t)
#define USBDEVFS_HOST_SAFTY_REMOVED      _IOR(USB_IOCTL_ID, 7, uint32_t)
#define USBDEVFS_SET_DELAY_TIME       _IOW(USB_IOCTL_ID, 8, uint32_t)
#define USBDEVFS_DETECT_FIRST_PACKET      _IOR(USB_IOCTL_ID, 9, uint32_t)
#define USBDEVFS_DISK_PLUG_STATUS_GET      _IOR(USB_IOCTL_ID, 10, int*)
#define USBDEVFS_WAIT_DISK_SYNC       _IOW(USB_IOCTL_ID, 11, uint32_t)
/* ---- Following 3 ioctrl is used for vtech ----- */
#define USBDEVFS_GETCBW				_IOR(USB_IOCTL_ID, 21, char *) 
#define USBDEVFS_WRITE_USB_BUFFER	_IOW(USB_IOCTL_ID, 22, uint32_t*)
#define USBDEVFS_READ_USB_BUFFER	_IOR(USB_IOCTL_ID, 23, uint32_t*)
#define USBDEVFS_OPEN_DEBUG			_IOW(USB_IOCTL_ID, 20, uint32_t*)


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct usb_content_s {
	unsigned int enable;     /*!< @brief enable */
	//unsigned int value;      /*!< @brief gpio value */
} usb_content_t;
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * @brief   USB Clock enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_clock_en(int en);

/**
 * @brief   USB software connect setting
 * @param   connect [IN]: [1]connect [0]disconnect
 * @return  none
 * @see
 */
void gp_usb_slave_sw_connect(int connect);

/**
 * @brief   USB PHY0 enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_phy0_en(int en);

/**
 * @brief   USB PHY1 enable
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_phy1_en(int en);

/**
 * @brief   USB PHY1 mode config
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_phy1_config(int mode);

/**
 * @brief   USB HOST config get
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
int gp_usb_host_config_get(void);

/**
 * @brief   USB Slave VBUS Detect Get
 * @param   none
 * @return  1 Host Connect, 0 Host Disconnect
 * @see
 */
int gp_usb_vbus_detect_get(void);

/**
 * @brief   USB Slave Host Configed
 * @param   none
 * @return   1 Host Configed, 0 Host does't enumerate device
 * @see
 */
int gp_usb_host_configed(void);

/**
 * @brief   USB enable 
 * @param   phyConfig [IN]: config USB PHY 
 * 2'b00: PHY0 Host PHY1 Device 2'b01: PHY0 Disable PHY1 Device 
 * 2'b10: PHY0 Device PHY1 Host 2'b11: PHY0 Disable PHY1 Host 
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_en(int phyConfig, int en);

/**
 * @brief   USB HOST TVID Buf Get
 * @param   pBuf [OUT]: the buffer used to stored the TVID.
 * @return  none
 * @see
 */
void gp_usb_tvid_get( uint8_t *pBuf );

/**
 * @brief   USB HOST TVID Buf Set
 * @param   pBuf [IN]: the buffer used to stored the TVID.
 * @return  none
 * @see
 */
void gp_usb_tvid_set( uint8_t *pBuf );

/**
 * @brief   USB CBW Buf Get
 * @param   pBuf [OUT]: the buffer used to stored the CBW.
 * @return  none
 * @see
 */
void gp_usb_cbw_get(uint8_t *pBuf);

/**
 * @brief   USB HOST Data Buf Set
 * @param   pBuf [IN]: the buffer used to stored the pUsbDataBuf.
 * @return  none
 * @see
 */
void gp_usb_data_set( uint8_t *pBuf );

/**
 * @brief   USB HOST Data Buf get
 * @param   pBuf [IN]: the buffer used to stored the pUSBDataBuf.
 * @return  none
 * @see
 */
void gp_usb_data_get( uint8_t *pBuf );


/**
 * @brief   Host set delay time to wait first first setup
 * @param   d_t : ms unit
 * @return  none
 * @see
 */
void gp_usb_set_delay_time(int d_t);
/**
 * @brief   USB HOST safty remove
 * @param   none
 * @return  host , 0 [not safty remove] or 1 [issue fsaty remove]
 * @see
 */
int gp_usb_host_safty_removed(void);

/**
 * @brief   Detect PC's first packet 
 * @param   none
 * @return 	0 [not detect in specific duration] 
 * 			1 [detect pc's setup packet in specific duration]
 *  		0xff [ error usage case reply ]
 * @see
 */
int gp_usb_detect_first_packet(void);

/**
 * @brief   Set for waiting disk was sync.
 * @param   setting [IN]: [1] wait disk sync, [0] not wait.
 * @return 	none.
 * @see
 */
int gp_usb_disk_sync_wait_get( void );
#endif /*_GP_USB_H_ */
