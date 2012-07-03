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
 * @param   mode [IN]: USB_PHY1_HOST or USB_PHY1_SLAVE
 * @return  none
 * @see
 */
void gp_usb_phy1_config(int mode);

/**
 * @brief   USB HOST config get
 * @param   none
 * @return  host config, 0 [PHY0] or 1 [PHY1]
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
 * @param   en [IN]: enable
 * @return  none
 * @see
 */
void gp_usb_en(int en);

/**
 * @brief   USB HOST TVID Buf Get
 * @param   pBuf [IN]: the buffer used to stored the TVID.
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

#endif /*_GP_USB_H_ */
