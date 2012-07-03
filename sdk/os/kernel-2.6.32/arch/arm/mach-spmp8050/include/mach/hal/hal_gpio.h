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
 * @file    hal_gpio.h
 * @brief   Declaration of GPIO HAL API.
 * @author  qinjian
 * @since   2010-9-28
 * @date    2010-11-10
 */
#ifndef _HAL_GPIO_H_
#define _HAL_GPIO_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define GPIO_CHANNEL_0          0
#define GPIO_CHANNEL_1          1
#define GPIO_CHANNEL_2          2
#define GPIO_CHANNEL_3          3
#ifndef GPIO_CHANNEL_SAR
#define GPIO_CHANNEL_SAR        4
#endif

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define GPIO_PIN_NUMBER(pinIndex)   ((pinIndex) & 0xFF)
#define GPIO_GROUP_ID(pinIndex)     (((pinIndex) >> 8) & 0xFF)
#define GPIO_FUNCTION_ID(pinIndex)  (((pinIndex) >> 16) & 0xFF)
#define GPIO_CHANNEL(pinIndex)      ((pinIndex) >> 24)

#define MK_GPIO_INDEX(ch, func, gid, pin) \
			(((ch) << 24) | ((func) << 16) | ((gid) << 8) | (pin))

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
 * @brief   Gpio hardware Pad group select setting function
 * @param   pinIndex[in]: gpio pin index
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 gpHalGpioSetPadGrp(UINT32 pinIndex);

/**
 * @brief   Gpio hardware direction setting fucntion
 * @param   pinIndex[in]: gpio pin index
 * @param   direction[in]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetDirection(UINT32 pinIndex, UINT32 direction);

/**
 * @brief   Gpio hardware direction getting fucntion
 * @param   pinIndex[in]: gpio pin index
 * @param   direction[out]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetDirection(UINT32 pinIndex, UINT32 *direction);

/**
 * @brief   Gpio hardware GPIO/Normal setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   function[in]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetFunction(UINT32 pinIndex, UINT32 function);

/**
 * @brief   Gpio hardware GPIO/Normal getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   function[out]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetFunction(UINT32 pinIndex, UINT32 *function);

/**
 * @brief   Gpio hardware output value setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   value[in]: output value
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetValue(UINT32 pinIndex, UINT32 value);

/**
 * @brief   Gpio hardware input value getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   value[out]: input value
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetValue(UINT32 pinIndex, UINT32 *value);

/**
 * @brief   Gpio hardware interrupt pending setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pending[in]: interrupt pending
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetIntPending(UINT32 pinIndex, UINT32 pending);

/**
 * @brief   Gpio hardware interrupt pending getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pending[out]: interrupt pending
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetIntPending(UINT32 pinIndex, UINT32 *pending);

/**
 * @brief   Gpio hardware debounce counter setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   count[in]: debounce count
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetDebounce(UINT32 pinIndex, UINT32 count);

/**
 * @brief   Gpio hardware debounce counter getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   count[out]: debounce count
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetDebounce(UINT32 pinIndex,	UINT32 *count);

/**
 * @brief   Gpio hardware irq polarity setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   polarity[in]: irq polarity,
 *  				GPIO_IRQ_LEVEL_LOW/GPIO_IRQ_LEVEL_HIGH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetPolarity(UINT32 pinIndex, UINT32 polarity);

/**
 * @brief   Gpio hardware irq polarity getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   polarity[out]: irq polarity,
 *  				GPIO_IRQ_LEVEL_LOW/GPIO_IRQ_LEVEL_HIGH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetPolarity(UINT32 pinIndex, UINT32 *polarity);

/**
 * @brief   Gpio hardware irq edge setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   edge[in]: irq edge,
 *  			GPIO_IRQ_ACTIVE_FALING/GPIO_IRQ_ACTIVE_RISING/GPIO_IRQ_ACTIVE_BOTH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetEdge(UINT32 pinIndex, UINT32 edge);

/**
 * @brief   Gpio hardware irq edge getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   edge[out]: irq edge,
 *  			GPIO_IRQ_ACTIVE_FALING/GPIO_IRQ_ACTIVE_RISING/GPIO_IRQ_ACTIVE_BOTH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetEdge(UINT32 pinIndex, UINT32 *edge);

/**
 * @brief   Gpio hardware internal pull high/low setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pullLevel[in]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetPullFunction(UINT32 pinIndex, UINT32 pullLevel);

/**
 * @brief   Gpio hardware internal pull high/low getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pullLevel[out]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetPullFunction(UINT32 pinIndex, UINT32 *pullLevel);

/**
 * @brief   Gpio hardware driving current setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   drivingCurrent[in]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioSetDrivingCurrent(UINT32 pinIndex, UINT32 drivingCurrent);

/**
 * @brief   Gpio hardware driving current getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   drivingCurrent[out]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioGetDrivingCurrent(UINT32 pinIndex, UINT32 *drivingCurrent);

/**
 * @brief   Gpio hardware irq enable/disable function
 * @param   pinIndex[in]: gpio pin index
 * @param   enable[in]: irq disable(0)/enable(1)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioEnableIrq(UINT32 pinIndex, UINT32 enable);

/**
 * @brief   Gpio hardware debounce enable/disable function
 * @param   pinIndex[in]: gpio pin index
 * @param   enable[in]: debounce disable(0)/enable(1)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32 gpHalGpioEnableDebounce(UINT32 pinIndex, UINT32 enable);

/**
 * @brief   Gpio clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void gpHalGpioClkEnable(UINT32 enable);

#endif /* _HAL_GPIO_H_ */
