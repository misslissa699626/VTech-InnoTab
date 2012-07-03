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
 * @file    gp_gpio.h
 * @brief   Declaration of GPIO driver.
 * @author  qinjian
 * @since   2010-9-27
 * @date    2010-9-27
 */
#ifndef _GP_GPIO_H_
#define _GP_GPIO_H_

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define GPIO_CHANNEL_SAR        4

#define GPIO_FUNC_ORG           0
#define GPIO_FUNC_GPIO          1
#define GPIO_DIR_OUTPUT         0
#define GPIO_DIR_INPUT          1
#define GPIO_PULL_LOW           0
#define GPIO_PULL_HIGH          1
#define GPIO_PULL_FLOATING      2
#define GPIO_IRQ_DISABLE        0
#define GPIO_IRQ_ENABLE         1

/* Interrupt property definition */
#define GPIO_IRQ_LEVEL_TRIGGER  0
#define GPIO_IRQ_EDGE_TRIGGER   1
#define GPIO_IRQ_LEVEL_LOW      0
#define GPIO_IRQ_LEVEL_HIGH     1
#define GPIO_IRQ_ACTIVE_FALING  0
#define GPIO_IRQ_ACTIVE_RISING  1
#define GPIO_IRQ_ACTIVE_BOTH    2

/* Ioctl for device node definition */
#define GPIO_IOCTL_ID           'G'
#define GPIO_IOCTL_SET_VALUE    _IOW(GPIO_IOCTL_ID, 0, gpio_content_t)
#define GPIO_IOCTL_GET_VALUE    _IOR(GPIO_IOCTL_ID, 1, gpio_content_t)
//#define GPIO_IOCTL_TEST         _IO(GPIO_IOCTL_ID, 11)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct gpio_content_s {
	unsigned int pin_index;     /*!< @brief gpio pin index */
	unsigned int value;         /*!< @brief gpio value */
} gpio_content_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**
 * @brief   Gpio pin request function.
 * @param   pin_index[in]: (gpio_channel << 24) | (function_id << 16) | (gid << 8) | pin_number
 * @param   name[in]: caller's name
 * @return  gpio handle/ERROR_ID
 * @see
 */
int gp_gpio_request(unsigned int pin_index, char *name);

/**
 * @brief   Gpio pin release function.
 * @param   handle[in]: gpio handle to release
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_gpio_release(int handle);

/**
 * @brief   Gpio direction setting function.
 * @param   handle[in]: gpio handle
 * @param   direction[in]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_gpio_set_direction(int handle, unsigned int direction);

/**
 * @brief   Gpio direction getting function.
 * @param   handle[in]: gpio handle
 * @param   direction[out]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_gpio_get_direction(int handle, unsigned int *direction);

/**
 * @brief   Gpio GPIO/Normal setting function
 * @param   handle[in]: gpio handle
 * @param   function[in]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_function(int handle, unsigned int function);

/**
 * @brief   Gpio GPIO/Normal getting function
 * @param   handle[in]: gpio handle
 * @param   function[out]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_function(int handle, unsigned int *function);

/**
 * @brief   Gpio output value setting function
 * @param   handle[in]: gpio handle
 * @param   value[in]: output value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_value(int handle, unsigned int value);

/**
 * @brief   Gpio input value getting function
 * @param   handle[in]: gpio handle
 * @param   value[out]: input value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_value(int handle, unsigned int *value);

/**
 * @brief   Gpio internal pull high/low setting function
 * @param   handle[in]: gpio pin number + group index
 * @param   pull_level[in]: pull level,
 *  				  GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_pullfunction(int handle, unsigned int pull_level);

/**
 * @brief   Gpio internal pull high/low getting function
 * @param   handle[in]: gpio handle
 * @param   pull_level[out]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_pullfunction(int handle, unsigned int *pull_level);

/**
 * @brief   Gpio driving current setting function
 * @param   handle[in]: gpio handle
 * @param   current[in]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_driving_current(int handle, unsigned int driving_current);

/**
 * @brief   Gpio driving current getting function
 * @param   handle[in]: gpio handle
 * @param   current[out]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_driving_current(int handle, unsigned int *driving_current);

/**
 * @brief   Gpio debounce counter setting function
 * @param   handle[in]: gpio handle
 * @param   count[in]: debounce count (0 for disable debounce)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_debounce(int handle, unsigned int count);

/**
 * @brief   Gpio debounce counter getting function
 * @param   handle[in]: gpio handle
 * @param   count[out]: debounce count
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_debounce(int handle, unsigned int *count);

/**
 * @brief   Gpio irq enable/disable function
 * @param   handle[in]: gpio handle
 * @param   enable[in]: GPIO_IRQ_DISABLE(0)/GPIO_IRQ_ENABLE(1)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_enable_irq(int handle, unsigned int enable);

/**
 * @brief   Gpio irq property setting function
 * @param   handle[in]: gpio handle
 * @param   property[in]: interrupt property,
 *  			GPIO_IRQ_LEVEL_TRIGGER/GPIO_IRQ_EDGE_TRIGGER +
 *  			(GPIO_IRQ_LEVEL_LOW/GPIO_IRQ_LEVEL_HIGH or
 *  			GPIO_IRQ_ACTIVE_FALING/GPIO_IRQ_ACTIVE_RISING/GPIO_IRQ_ACTIVE_BOTH)
 * @param   args[in]: Reserved parameter, example: debounce time
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_irq_property(int handle, unsigned int property, unsigned int *args);

/**
 * @brief   Gpio irq property Getting function
 * @param   handle[in]: gpio handle
 * @param   property[in]: interrupt property,
 *  			GPIO_IRQ_LEVEL_TRIGGER/GPIO_IRQ_EDGE_TRIGGER
 * @param   args[out]: Reserved parameter, example: debounce
 *  				 time
 * @return  when property == GPIO_IRQ_LEVEL_TRIGGER, return
 *  		GPIO_IRQ_LEVEL_LOW/GPIO_IRQ_LEVEL_HIGH or ERRCODE(<0);
 *  		when property == GPIO_IRQ_EDGE_TRIGGER, return
 *  		GPIO_IRQ_ACTIVE_FALING/GPIO_IRQ_ACTIVE_RISING/GPIO_IRQ_ACTIVE_BOTH)
 *  		or ERRCODE(<0).
 */
int gp_gpio_irq_property_get(int handle, unsigned int property, unsigned int *args);

/**
 * @brief   Gpio isr register function
 * @param   handle [in] gpio handle
 * @param   cbk [in] isr callback function
 * @param   data [in] private data
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_register_isr(int handle, void (*cbk)(void *), void *data);

/**
 * @brief   Gpio isr unregister function
 * @param   handle [in] gpio handle
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_unregister_isr(int handle);

/**
 * @brief   Gpio integrate setting output functin
 * @param   handle[in]: gpio handle
 * @param   value[in]: output value
 * @param   driving_current[in]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_output(int handle, unsigned int value, int driving_current);

/**
 * @brief   Gpio integrate setting input functin
 * @param   handle[in]: gpio handle
 * @param   pull_level[in]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_input(int handle, int pull_level);

#endif /* _GP_GPIO_H_ */
