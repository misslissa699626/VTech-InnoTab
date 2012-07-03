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
 * @file    gp_i2c_bus.h
 * @brief   Declaration of i2c bus driver.
 * @author  junp.zhang
 * @since   2010/10/12
 * @date    2010/10/12
 */
#ifndef _GP_I2C_BUS_H_
#define _GP_I2C_BUS_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Ioctl for device node definition */
#define I2C_BUS_IOCTL_ID         'I'
#define I2C_BUS_ATTR_SET    _IOW(I2C_BUS_IOCTL_ID, 0, int)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct i2c_bus_attr_s{
	unsigned int slaveAddr;			/*!< @brief slave device address*/
	unsigned int clkRate;				/*!< @brief i2c bus clock rate */
}i2c_bus_attr_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   I2C bus request function.
 * @param   slaveAddr[in]: slave device ID
 * @param   clkRate[in]: i2c bus clock rate
 * @return  i2c bus handle/ERROR_ID
 * @see
 */ 
int gp_i2c_bus_request(int slaveAddr, int clkRate);

/**
 * @brief   I2C bus release function.
 * @param   handle[in]: i2c bus handle 
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_i2c_bus_release(int handle);

/**
 * @brief   I2C bus write function
 * @param   handle[in]: i2c bus handle
 * @param   data[in]: data to write
 * @param   len[in]: data length
 * @return  data length/I2C_FAIL(-1)
 */
int gp_i2c_bus_write(int handle, unsigned char* data, unsigned int len);

/**
 * @brief   I2C bus read function
 * @param   handle[in]: i2c bus handle
 * @param   data[out]: data read from slave device
 * @param   len[in]: data length
 * @return  data length/I2C_FAIL(-1)
 */
int gp_i2c_bus_read (int handle, unsigned char* data, unsigned int len);

#endif /*_GP_I2C_BUS_H_ */
