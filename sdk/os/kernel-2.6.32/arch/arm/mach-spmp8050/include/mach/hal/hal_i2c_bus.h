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
 * @file    hal_i2c_bus.h
 * @brief   Declaration of SPMP8050 I2C Bus HAL API.
 * @author  junp.zhang
 * @since   2010/10/12
 * @date    2010/10/12
 */
#ifndef _HAL_I2C_BUS_H_
#define _HAL_I2C_BUS_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

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
 * @brief   I2C Bus hardware initial
 * @return  None
 * @see
 */
void gpHalI2cBusInit(void);

/**
 * @brief   I2C Bus hardware clock set
 * @param   clkRate [in] KHZ
 * @see
 */
void gpHalI2cBusSetClkRate(UINT32 clkRate);

/**
 * @brief   I2C Bus hardware start to transfer data
 * @param   aSlaveAddr [in] slave device address
 * @param   cmd [in] 
 * @param   aAck [in] 
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 gpHalI2cBusStartTran(UINT32 aSlaveAddr, UINT32 clkRate, UINT32 cmd, UINT32 aAck);
 
/**
 * @brief   I2C Bus hardware be in transfering data
 * @param   aSlaveAddr [in] slave device address
 * @param   cmd [in] 
 * @param   aAck [in] 
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 gpHalI2cBusMidTran(UINT8 *aData , UINT32 cmd, UINT32 aAck);

/**
 * @brief   I2C Bus hardware stop transfer data
 * @param   cmd [in]
 * @see
 */
void gpHalI2cBusStopTran(UINT32 cmd);

/**
 * @brief   I2C Bus clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalI2cBusClkEnable(UINT32 enable);

#endif	/* _HAL_I2C_BUS_H_ */
