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
 * @brief PHY1 Get Software Connect function Status
 * @param None
 * @return  1:connect, 0:disconnect
 * @see
 */
int gpHalUsbSlaveSwConnectGet(void);

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
 * @brief USB PHY Configuration Get
 * @param config [in] : 
 * 2'b00: PHY0 Host PHY1 Device 2'b01: PHY0 Disable PHY1 Device 
 * 2'b10: PHY0 Device PHY1 Host 2'b11: PHY0 Disable PHY1 Host 
 * @return  none 
 */
void gpHalUsbPhyConfigSet(int config);

/**
 * @brief USB PHY Configuration Get
 * @param void
 * @return 
 * 2'b00: PHY0 Host PHY1 Device 2'b01: PHY0 Disable PHY1 Device 
 * 2'b10: PHY0 Device PHY1 Host 2'b11: PHY0 Disable PHY1 Host 
 */
int gpHalUsbPhyConfigGet(void);


/**
 * @brief USB PHY Power Control Set
 * @param phyNum [in] : 0 PHY0, 1 PHY1 
 * @param control [in] : 0 disable, 1 force suspend, 3 force 
 *  			  wakeup.
 * @return   
 */
void gpHalUsbPhyPowerControlSet(int phyNum,	int control);

/**
 * @brief Host Configuration Get
 * @param [IN] en : enable
 * @return  None
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

/**
 * @brief Host Configuration Get
 * @return  1 Detect host safty remove, 0 Host does't issue safty remove
 * @see
 *	UDC_LLCS contorller status value for different usb state shows as below:
 *	[initial]			0x83
 *	[plug in]			0x61
 *	[safy remove] 		0x45
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
int gpHalUsbHostSafyRemoved(void);

/**
 * @brief   Host set delay time to wait first first setup
 * @param   d_t : ms unit
 * @return  none
 * @see
 */
void gpHalUsbSetDelayTime(int d_t);


/**
 * @brief   Detect PC's first packet 
 * @param   none
 * @return 	0 [not detect in specific duration] 
 * 			1 [detect pc's setup packet in specific duration]
 *  		
 * @see
 */
int gpHalUsbDetectFirstPacket(void);

void set_first_setup(int);
int get_first_setup(void);

#endif	/* _HAL_USB_H_ */
