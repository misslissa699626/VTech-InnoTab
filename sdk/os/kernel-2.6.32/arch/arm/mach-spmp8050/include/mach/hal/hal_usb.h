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
 * @file    hal_usb.h
 * @brief   Implement of SPMP8050 Host/Slave HAL API.
 * @author  allen.chang
 * @since   2010/11/22
 * @date    2010/11/22
 */
#ifndef _HAL_USB_H_
#define _HAL_USB_H_

#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define HAL_USB_PHY1_SLAVE 0
#define HAL_USB_PHY1_HOST 1

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
/**
 * @brief Clock Enable Function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void gpHalUsbClockEn ( int en );

/**
 * @brief PHY1 Software Connect function
 * @param [IN] connect : [1]Connect [0]Disconnect
 * @return  None
 * @see
 */
void gpHalUsbSlaveSwConnect(int connect);

/**
 * @brief PHY0 Switch function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void gpHalUsbPhy0En(int en);

/**
 * @brief PHY1 Switch function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void gpHalUsbPhy1En(int en);

/**
 * @brief PHY1 Switch function
 * @param [IN] mode : 
 * @return  None
 * @see
 */
void gpHalUsbPhy1Config(int mode);


/**
 * @brief Host Enable Function
 * @param [IN] en : enable
 * @return  None
 * @see
 */
void gpHalUsbHostEn(int en);


/**
 * @brief Host Configuration Get
 * @return  host config, 0 [PHY0] or 1 [PHY1]
 * @see
 */
int gpHalUsbHostConfigGet(void);

/**
 * @brief Host Configuration Get
 * @return  1 Host Connect, 0 Host Disconnect
 * @see
 */
int gpHalUsbVbusDetect(void);

/**
 * @brief Host Configuration Get
 * @return  1 Host Configed, 0 Host does't enumerate device
 * @see
 */
int gpHalUsbHostConfiged(void);

#endif	/* _HAL_USB_H_ */
