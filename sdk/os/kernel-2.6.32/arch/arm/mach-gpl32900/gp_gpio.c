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
 * @file    gp_gpio.c
 * @brief   Implement of GPIO driver.
 * @author  qinjian
 * @since   2010-9-27
 * @date    2010-11-10
 */
#include <mach/kernel.h>
#include <mach/module.h>
#include <mach/cdev.h>
#include <mach/diag.h>
#include <mach/gp_gpio.h>
#include <mach/hal/hal_gpio.h>
#include <mach/hal/hal_clock.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define NUM_GPIO_CHANNEL	4

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#define GPIO_IRQ_TRIGGER(prority)   (prority & 0xFF)
#define GPIO_IRQ_METHOD(prority)    ((prority >> 8) & 0xFF)

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct gpio_info_s {
	struct miscdevice dev;      /*!< @brief gpio device */
	struct semaphore sem;       /*!< @brief mutex semaphore for gpio ops */
} gpio_info_t;

typedef void (*irq_callback)(void *);

typedef struct gpio_isr_s
{
    irq_callback cbk;           /*!< @brief callback function */
	void *priv_data;            /*!< @brief private data */
} gpio_isr_t;

typedef struct gpio_handle_s {
	unsigned int pin_index;     /*!< @brief pin index */
	char *name;                 /*!< @brief owner name */
	gpio_isr_t isr;             /*!< @brief pin isr */
} gpio_handle_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static gpio_info_t *gpio = NULL;
static gpio_handle_t *pin_table[NUM_GPIO_CHANNEL][32]; /* channel:0-3, pin_number:0-31 */
static int g_gpio_regSave = 0;
static const int gpio_irqs[NUM_GPIO_CHANNEL] = {
	IRQ_GPIO0,
	IRQ_GPIO1,
	IRQ_GPIO2,
	IRQ_GPIO3
};

static gpio_isr_t gIsr[NUM_GPIO_CHANNEL][32];

/**
 * @brief   Gpio clock enable/disable
 * @param   enable [in] 0:disable, 1:enable
 * @return  None
 * @see
 */
static void gpio_clock_enable(int enable)
{
	gpHalScuClkEnable(SCU_B_PERI_GPIO, SCU_B, enable);
}

/**
 * @brief   Gpio irq handler
 */
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	int ch;

	for (ch = 0; ch < NUM_GPIO_CHANNEL; ch++) {
		if (irq == gpio_irqs[ch]) {
			unsigned int pending;
			int i;

			gpHalGpioGetIntPending(ch << 24, &pending);
			gpHalGpioSetIntPending(ch << 24, pending);
			for (i = 0; i < 32; i++) {
				if ((pin_table[ch][i] != NULL) && ((pending & (1 << i)) != 0)) {
					gpio_isr_t *isr = &gIsr[ch][i];
					if (isr->cbk != NULL) {
						isr->cbk(isr->priv_data);
					}
				}
			}
			gpHalGpioSetIntPending(ch << 24, pending);
			break;
		}
	}

	return (IRQ_HANDLED);
}

/**
 * @brief   Gpio pin request function.
 * @param   pin_index[in]: gpio channel + function id + gid + pin number
 * @param   name[in]: caller's name
 * @return  gpio handle/ERROR_ID
 * @see
 */
int gp_gpio_request(unsigned int pin_index, char *name)
{
	return pin_index;
}
EXPORT_SYMBOL(gp_gpio_request);

/**
 * @brief   Gpio pin release function.
 * @param   handle[in]: gpio handle to release
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_gpio_release(int handle)
{
	return 0;
}
EXPORT_SYMBOL(gp_gpio_release);

int gp_gpio_real_request(unsigned int pin_index, char* name) {
	int ret;
	unsigned int ch = GPIO_CHANNEL(pin_index);
	unsigned int pin_number = GPIO_PIN_NUMBER(pin_index);
	gpio_handle_t *handle;

	if (gpio == NULL) {
		DIAG_ERROR("gpio not initialized!");
		return -ENODEV;
	}

	if (down_interruptible(&gpio->sem) != 0) {
		return -ERESTARTSYS;
	}

	/* check pin requested */
	if ((ch < NUM_GPIO_CHANNEL) && (pin_number < 32) && (pin_table[ch][pin_number] != NULL)) {
        name = pin_table[ch][pin_number]->name;
		DIAG_ERROR("pin already requested by %s!\n", (name != NULL) ? name : "unknown module");
		ret = -EBUSY;
		goto out;
	}

	ret = gpHalGpioSetPadGrp(pin_index);
	if (ret != 0) {
		ret = -EINVAL;
		goto out;
	}

	handle = (gpio_handle_t *)kzalloc(sizeof(gpio_handle_t), GFP_KERNEL);
	if (handle == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	handle->pin_index = pin_index;
	handle->name = name;
	ret = (int)handle;

	/* set pin requested */
    if ((ch < NUM_GPIO_CHANNEL) && (pin_number < 32)) {
        pin_table[ch][pin_number] = handle;
    }

out:
	up(&gpio->sem);
	return ret;
}

int gp_gpio_real_release(int handle) {
	gpio_handle_t *h = (gpio_handle_t *)handle;
    unsigned int ch = GPIO_CHANNEL(h->pin_index);
	unsigned int pin_number = GPIO_PIN_NUMBER(h->pin_index);

	if (down_interruptible(&gpio->sem) != 0) {
		return -ERESTARTSYS;
	}

	/* clear pin requested */
    if ((ch < NUM_GPIO_CHANNEL) && (pin_number < 32)) {
        pin_table[ch][pin_number] = NULL;
    }

    kfree(h);

	up(&gpio->sem);
	return 0;
}



/**
 * @brief   Gpio direction setting function.
 * @param   handle[in]: gpio handle
 * @param   direction[in]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_gpio_set_direction(int handle, unsigned int direction)
{
	int ret;

	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "set_direction");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioSetDirection(handle, direction);

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_set_direction);

/**
 * @brief   Gpio direction getting function.
 * @param   handle[in]: gpio handle
 * @param   direction[out]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/ERROR_ID
 * @see
 */
int gp_gpio_get_direction(int handle, unsigned int *direction)
{
	int ret;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	ret = gpHalGpioGetDirection(handle, direction);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_get_direction);

/**
 * @brief   Gpio GPIO/Normal setting function
 * @param   handle[in]: gpio handle
 * @param   function[in]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_function(int handle, unsigned int function)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "set_function");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioSetFunction(handle, function);

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_set_function);

/**
 * @brief   Gpio GPIO/Normal getting function
 * @param   handle[in]: gpio handle
 * @param   function[out]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_function(int handle, unsigned int *function)
{
	int ret;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	ret = gpHalGpioGetFunction(handle, function);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_get_function);

/**
 * @brief   Gpio output value setting function
 * @param   handle[in]: gpio handle
 * @param   value[in]: output value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_value(int handle, unsigned int value)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "set_value");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioSetValue(handle, value);

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_set_value);

/**
 * @brief   Gpio input value getting function
 * @param   handle[in]: gpio handle
 * @param   value[out]: input value
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_value(int handle, unsigned int *value)
{
	int ret;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

	
    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}
	ret = gpHalGpioGetValue(handle, value);
	
	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_get_value);

/**
 * @brief   Gpio internal pull high/low setting function
 * @param   handle[in]: gpio pin number + group index
 * @param   pull_level[in]: pull level,
 *  				  GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_pullfunction(int handle, unsigned int pull_level)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "set_pullfunction");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioSetPullFunction(handle, pull_level);

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_set_pullfunction);

/**
 * @brief   Gpio internal pull high/low getting function
 * @param   handle[in]: gpio handle
 * @param   pull_level[out]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_pullfunction(int handle, unsigned int *pull_level)
{
	int ret;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	ret = gpHalGpioGetPullFunction(handle, pull_level);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_get_pullfunction);

/**
 * @brief   Gpio driving current setting function
 * @param   handle[in]: gpio handle
 * @param   current[in]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_driving_current(int handle, unsigned int driving_current)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "set_driving_current");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioSetDrivingCurrent(handle, driving_current);

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_set_driving_current);

/**
 * @brief   Gpio driving current getting function
 * @param   handle[in]: gpio handle
 * @param   current[out]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_driving_current(int handle, unsigned int *driving_current)
{
	int ret;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	ret = gpHalGpioGetDrivingCurrent(handle, driving_current);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_get_driving_current);

/**
 * @brief   Gpio debounce counter setting function
 * @param   handle[in]: gpio handle
 * @param   count[in]: debounce count (0 for disable debounce)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_debounce(int handle, unsigned int count)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "set_debounce");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioSetDebounce(handle, count);

	if (ret == 0) {
		ret = gpHalGpioEnableDebounce(handle, count);
	}

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_set_debounce);

/**
 * @brief   Gpio debounce counter getting function
 * @param   handle[in]: gpio handle
 * @param   count[out]: debounce count
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_get_debounce(int handle, unsigned int *count)
{
	int ret;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	ret = gpHalGpioGetDebounce(handle, count);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_get_debounce);

/**
 * @brief   Gpio irq enable/disable function
 * @param   handle[in]: gpio handle
 * @param   enable[in]: GPIO_IRQ_DISABLE(0)/GPIO_IRQ_ENABLE(1)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_enable_irq(int handle, unsigned int enable)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "enable_irq");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gpHalGpioEnableIrq(handle, enable);

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_enable_irq);

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
int gp_gpio_irq_property(int handle, unsigned int property, unsigned int *args)
{
	int ret;
	unsigned int trigger = GPIO_IRQ_TRIGGER(property);
	unsigned int method = GPIO_IRQ_METHOD(property);
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "irq_property");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	switch (trigger) {
	case GPIO_IRQ_LEVEL_TRIGGER:
		ret = gpHalGpioSetPolarity(handle, method);
		break;
	case GPIO_IRQ_EDGE_TRIGGER:
		ret = gpHalGpioSetEdge(handle, method);
		break;
	default:
		ret = EINVAL;
		break;
	}

	if (ret == 0 && args != NULL) {
		/* args[0]: debounce count */
		ret = gp_gpio_set_debounce(handle, args[0]);
	}

	gp_gpio_real_release((int)gpio_handle);

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_irq_property);

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
int gp_gpio_irq_property_get(int handle, unsigned int property, unsigned int *args)
{
	int ret;
	unsigned int trigger = GPIO_IRQ_TRIGGER(property);
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	switch (trigger) {
	case GPIO_IRQ_LEVEL_TRIGGER:
		ret = gpHalGpioGetPolarity(handle, args);
		break;
	case GPIO_IRQ_EDGE_TRIGGER:
		ret = gpHalGpioGetEdge(handle, args);
		break;
	default:
		ret = EINVAL;
		break;
	}

	if ((ret == 0) && (args != NULL)) {
		/* args[0]: debounce count */
		ret = gp_gpio_get_debounce(handle, &args[0]);
	}

	return (-ret);
}
EXPORT_SYMBOL(gp_gpio_irq_property_get);

/**
 * @brief   Gpio isr register function
 * @param   handle [in] gpio handle
 * @param   cbk [in] isr callback function
 * @param   data [in] private data
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_register_isr(int handle, void (*cbk)(void *), void *data)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "register_isr");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gp_gpio_enable_irq(handle, GPIO_IRQ_ENABLE);
	if (ret == 0) {
		gIsr[ch][pin_number].cbk = cbk;
		gIsr[ch][pin_number].priv_data = data;
	}

	gp_gpio_real_release((int)gpio_handle);

	return ret;
}
EXPORT_SYMBOL(gp_gpio_register_isr);

/**
 * @brief   Gpio isr unregister function
 * @param   handle [in] gpio handle
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_unregister_isr(int handle)
{
	int ret;
	gpio_handle_t *gpio_handle = 0;
    unsigned int ch = GPIO_CHANNEL(handle);
	unsigned int pin_number = GPIO_PIN_NUMBER(handle);

    if ((ch >= NUM_GPIO_CHANNEL) || (pin_number >= 32)) {
		return -1;
	}

	gpio_handle = (gpio_handle_t*)gp_gpio_real_request(handle, "unregister_isr");

	if (IS_ERR_VALUE(gpio_handle)) {
		return -1;
	}

	ret = gp_gpio_enable_irq(handle, GPIO_IRQ_DISABLE);
	if (ret == 0) {
		gIsr[ch][pin_number].cbk = NULL;
	}

	gp_gpio_real_release((int)gpio_handle);

	return ret;
}
EXPORT_SYMBOL(gp_gpio_unregister_isr);

/**
 * @brief   Gpio integrate setting output functin
 * @param   handle[in]: gpio handle
 * @param   value[in]: output value
 * @param   driving_current[in]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_output(int handle, unsigned int value, int driving_current)
{
	int ret;

	ret = gp_gpio_set_function(handle, GPIO_FUNC_GPIO);
	if (ret != 0) {
		goto out;
	}
	ret = gp_gpio_set_direction(handle, GPIO_DIR_OUTPUT);
	if (ret != 0) {
		goto out;
	}
	ret = gp_gpio_set_driving_current(handle, driving_current);
	if (ret != 0) {
		goto out;
	}
	ret = gp_gpio_set_value(handle, value);

out:
	return ret;
}
EXPORT_SYMBOL(gp_gpio_set_output);

/**
 * @brief   Gpio integrate setting input functin
 * @param   handle[in]: gpio handle
 * @param   pull_level[in]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/ERROR_ID
 */
int gp_gpio_set_input(int handle, int pull_level)
{
	int ret;

	ret = gp_gpio_set_function(handle, GPIO_FUNC_GPIO);
	if (ret != 0) {
		goto out;
	}
	ret = gp_gpio_set_direction(handle, GPIO_DIR_INPUT);
	if (ret != 0) {
		goto out;
	}
	if(pull_level!=GPIO_NO_PULL) {
		ret = gp_gpio_set_pullfunction(handle, pull_level);
	}

out:
	return ret;
}
EXPORT_SYMBOL(gp_gpio_set_input);

#ifdef GPIO_IOCTL_TEST
static void gpio_test_isr(void *data)
{
	int p1 = (int)data;
	int p2;

	gp_gpio_enable_irq(p1, 0);
	p2 = gp_gpio_request(MK_GPIO_INDEX(2,2,4,6), "gpio_test"); /* IOC6 */
	gp_gpio_set_output(p2, 1, 1);
	gp_gpio_release(p2);
	printk("!!! gpio_test_isr\n");
}
#endif

/**
 * @brief   Gpio device ioctl function
 */
static long gpio_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	int handle;
	gpio_content_t ctx;
	gpio_handle_t handler;

	switch (cmd) {
#if 0
	case GPIO_IOCTL_GET_VALUE: //Added by Kevin, 20110809
	case GPIO_IOCTL_GET_VALUE_DIRECT:
		{
			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}
			handler.pin_index = ctx.pin_index;

			ret = gpHalGpioSetPadGrp(ctx.pin_index);
			if (ret != 0) {
				ret = -EINVAL;
				break;
			}
			//GPIO_PULL_HIGH
			//ret = gp_gpio_set_input((int)&handler, ctx.pull);
			ret = gp_gpio_set_input((int)&handler, GPIO_PULL_HIGH);//modified by Kevin, 20110809
			if (ret == 0) {
				ret = gp_gpio_get_value((int)&handler, &ctx.value);
				if (ret == 0) {
					if (copy_to_user((void __user*)arg, &ctx, sizeof(ctx))) {
						printk("copy_to_user ERROR\n");
						ret = -EFAULT;
					}
				}
			}			
		}
		break;
	
	case GPIO_IOCTL_SET_VALUE://Added by Kevin, 20110809
	case GPIO_IOCTL_SET_VALUE_DIRECT:
		{
			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}
			handler.pin_index = ctx.pin_index;
			ret = gpHalGpioSetPadGrp(ctx.pin_index);
			if (ret != 0) {
				ret = -EINVAL;
				break;
			}
			ret = gp_gpio_set_output((int)&handler, ctx.value, 0);		
		}
		break;
#endif

#if 1 //Modified by Kevin, 20110809
	case GPIO_IOCTL_SET_VALUE:
	case GPIO_IOCTL_SET_VALUE_DIRECT:
		{
			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}

			handle = gp_gpio_request(ctx.pin_index, NULL);
			if (IS_ERR_VALUE(handle)) {
				ret = (long)handle; /* gpio request error code */
				break;
			}

			ret = gp_gpio_set_output(handle, ctx.value, 0);

			gp_gpio_release(handle);
		}
		break;


	case GPIO_IOCTL_GET_VALUE:
	case GPIO_IOCTL_GET_VALUE_DIRECT:
		{
			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}

			handle = gp_gpio_request(ctx.pin_index, NULL);
			if (IS_ERR_VALUE(handle)) {
				ret = (long)handle; /* gpio request error code */
				break;
			}

			ret = gp_gpio_set_input(handle, GPIO_PULL_HIGH);
			if (ret == 0) {
				ret = gp_gpio_get_value(handle, &ctx.value);
				if (ret == 0) {
					if (copy_to_user((void __user*)arg, &ctx, sizeof(ctx))) {
						ret = -EFAULT;
					}
				}
			}

			gp_gpio_release(handle);
		}
		break;
#endif //#if 0

	case GPIO_IOCTL_SET_INPUT:
		{
			if (copy_from_user(&ctx, (void __user*)arg, sizeof(ctx))) {
				ret = -EFAULT;
				break;
			}

			handle = gp_gpio_request(ctx.pin_index, NULL);
			if (IS_ERR_VALUE(handle)) {
				ret = (long)handle; /* gpio request error code */
				break;
			}
			ret = gp_gpio_set_input(handle, ctx.value);
			
			gp_gpio_release(handle);			
		}
		break;
#ifdef GPIO_IOCTL_TEST
	case GPIO_IOCTL_TEST:
		{
			static int p1 = 0;
			int p2;

			if (p1 == 0) {
				printk("--- gpio test begin!\n");
				p2 = gp_gpio_request(MK_GPIO_INDEX(2,2,4,6), "gpio_test"); /* IOC6 */
				gp_gpio_set_output(p2, 0, 1);
				gp_gpio_release(p2);

				p1 = gp_gpio_request(MK_GPIO_INDEX(2,2,4,5), "gpio_test"); /* IOC5 */
				gp_gpio_set_input(p1, GPIO_PULL_LOW);
				gp_gpio_irq_property(p1, GPIO_IRQ_LEVEL_TRIGGER|GPIO_IRQ_LEVEL_HIGH, NULL);
				gp_gpio_register_isr(p1, gpio_test_isr, (void *)p1);
			}
			else {
				gp_gpio_unregister_isr(p1);
				gp_gpio_release(p1);
				p1 = 0;
				printk("--- gpio test end!\n");
			}
		}
		break;
#endif
	default:
		ret = -ENOTTY; /* Inappropriate ioctl for device */
		break;
	}

	return ret;
}

static int gpio_release(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations gpio_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = gpio_ioctl,
	.release = gpio_release
};

/**                                                                         
 * @brief   gpio device release                                              
 */                                                                         
static void gp_gpio_device_release(struct device *dev)                       
{                                                                           

}                                                                           
                                                                            
static struct platform_device gp_gpio_device = {                             
	.name	= "gp-gpio",                                                         
	.id	= 0,                                                                  
	.dev	= {                                                                 
		.release = gp_gpio_device_release,                                       
	},                                                                        
};                                                                                                                                                                                                                       
                                                                            
static void gp_gpio_suspend_set( void ){
	int* ptr; 
	if( g_gpio_regSave == 0 ) {
		ptr = kmalloc( 260, GFP_KERNEL);
		gpHalGpioRegSave( ptr );
		g_gpio_regSave = (int)ptr;
		if( ptr == NULL ) {
			printk("[%s][%d]Suspend Error, it allocates memory fail\n", __FUNCTION__, __LINE__);
		}
		gpio_clock_enable(0);
	}
}
EXPORT_SYMBOL(gp_gpio_suspend_set);

static void gp_gpio_resume_set( void ){
	int* ptr; 
	if( g_gpio_regSave != 0 ) {
		gpio_clock_enable(1);
		ptr = (int *)g_gpio_regSave;
		gpHalGpioRegRestore( ptr );
		kfree( ptr );
		g_gpio_regSave = 0;	
	}
}
EXPORT_SYMBOL(gp_gpio_resume_set);                                                               
                                                                            
#ifdef CONFIG_PM                                                            
static int gp_gpio_suspend(struct platform_device *pdev, pm_message_t state){
	gp_gpio_suspend_set();                                              
	return 0;                                                                 
}                                                                           
                                                                            
static int gp_gpio_resume(struct platform_device *pdev){  
	gp_gpio_resume_set();                                                      
	return 0;                                                                 
}                                                                           
#else                                                                       
#define gp_gpio_suspend NULL                                                 
#define gp_gpio_resume NULL                                                  
#endif                                                                      
                                                                            
/**                                                                         
 * @brief   wdt driver define                                               
 */                                                                         
static struct platform_driver gp_gpio_driver = {                             
	.suspend = gp_gpio_suspend,                                                
	.resume = gp_gpio_resume,                                                  
	.driver	= {                                                               
		.owner	= THIS_MODULE,                                                  
		.name	= "gp-gpio"                                                        
	}                                                                     
};                                                                          

/**
 * @brief   Gpio driver init function
 */
static int __init gpio_init(void)
{
	int ret = -ENXIO;

	int ch = 0;
	int i = 0;
	for (ch = 0; ch < NUM_GPIO_CHANNEL; ch++) {
		for (i = 0; i < 32; i++) {
			pin_table[ch][i] = 0;
		}
	}


	gpio = (gpio_info_t *)kzalloc(sizeof(gpio_info_t),  GFP_KERNEL);
	if (gpio == NULL) {
		ret = -ENOMEM;
		DIAG_ERROR("gpio kmalloc fail\n");
		goto fail_kmalloc;
	}

	/* request irq */
	ret = request_irq(gpio_irqs[0], gpio_irq_handler, 0, "GPIO_0", gpio);
	if (ret < 0) {
		DIAG_ERROR("request irq GPIO_0 fail\n");
		goto fail_request_irq_gpio_0;
	}
	ret = request_irq(gpio_irqs[1], gpio_irq_handler, 0, "GPIO_1", gpio);
	if (ret < 0) {
		DIAG_ERROR("request irq GPIO_1 fail\n");
		goto fail_request_irq_gpio_1;
	}
	ret = request_irq(gpio_irqs[2], gpio_irq_handler, 0, "GPIO_2", gpio);
	if (ret < 0) {
		DIAG_ERROR("request irq GPIO_2 fail\n");
		goto fail_request_irq_gpio_2;
	}
	ret = request_irq(gpio_irqs[3], gpio_irq_handler, 0, "GPIO_3", gpio);
	if (ret < 0) {
		DIAG_ERROR("request irq GPIO_3 fail\n");
		goto fail_request_irq_gpio_3;
	}

	/* initliaze */
	init_MUTEX(&gpio->sem);
	//gpHalGpioClkEnable(1);
	gpio_clock_enable(1);

	/* register device */
	gpio->dev.name  = "gpio";
	gpio->dev.minor = MISC_DYNAMIC_MINOR;
	gpio->dev.fops  = &gpio_fops;
	ret = misc_register(&gpio->dev);
	if (ret != 0) {
		DIAG_ERROR("gpio device register fail\n");
		goto fail_device_register;
	}

	platform_device_register(&gp_gpio_device);
	return platform_driver_register(&gp_gpio_driver);

	/* error rollback */
fail_device_register:
	gpio_clock_enable(0);
	free_irq(gpio_irqs[3], gpio);
fail_request_irq_gpio_3:
	free_irq(gpio_irqs[2], gpio);
fail_request_irq_gpio_2:
	free_irq(gpio_irqs[1], gpio);
fail_request_irq_gpio_1:
	free_irq(gpio_irqs[0], gpio);
fail_request_irq_gpio_0:
	kfree(gpio);
	gpio = NULL;
fail_kmalloc:
	return ret;
}

/**
 * @brief   Gpio driver exit function
 */
static void __exit gpio_exit(void)
{
	misc_deregister(&gpio->dev);
	//gpHalGpioClkEnable(0);
	gpio_clock_enable(0);
	free_irq(gpio_irqs[3], gpio);
	free_irq(gpio_irqs[2], gpio);
	free_irq(gpio_irqs[1], gpio);
	free_irq(gpio_irqs[0], gpio);
	kfree(gpio);
	gpio = NULL;

	platform_device_unregister(&gp_gpio_device);
	platform_driver_unregister(&gp_gpio_driver);
}

module_init(gpio_init);
module_exit(gpio_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus GPIO Driver");
MODULE_LICENSE_GP;
