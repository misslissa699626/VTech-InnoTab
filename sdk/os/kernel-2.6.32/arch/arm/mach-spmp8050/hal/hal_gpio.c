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
 * @file    hal_gpio.c
 * @brief   Implement of GPIO HAL API.
 * @author  qinjian
 * @since   2010-9-28
 * @date    2010-11-10
 */

#include <mach/kernel.h>
#include <mach/hal/hal_gpio.h>
#include <mach/hal/regmap/reg_gpio.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define GPIO_PULL_LOW           0
#define GPIO_PULL_HIGH          1
#define GPIO_PULL_FLOATING      2

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#if 0
#define WRITE_REG_BITS(reg,mask,shift,value) \
	do { \
		*(reg) = (*(reg) & ~((mask) << (shift))) | (((value) & (mask)) << (shift)); \
	} while (0)

#define READ_REG_BITS(reg,mask,shift,value) \
	do { \
		*(value) = (*(reg) >> (shift)) & (mask); \
	} while (0)
#endif

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static gpioReg_t *gpioReg = (gpioReg_t *)LOGI_ADDR_GPIO_REG;
static scuaReg_t *scuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;
static scubReg_t *scubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;

/**
 * @brief   Write register bits function
 * @param   reg[in]: register address
 * @param   mask[in]: bits mask
 * @param   shift[in]: bits shift
 * @param   value[in]: write value
 * @return  None
 * @see
 */
static void
writeRegBits(
	volatile UINT32 *reg,
	UINT32 mask,
	UINT32 shift,
	UINT32 value
)
{
	UINT32 tmp;

	tmp = *reg;
	tmp &= ~(mask << shift);
	tmp |= (value & mask) << shift;
	*reg = tmp;
}

/**
 * @brief   Read register bits function
 * @param   reg[in]: register address
 * @param   mask[in]: bits mask
 * @param   shift[in]: bits shift
 * @param   value[out]: read value
 * @return  None
 * @see
 */
static void
readRegBits(
	volatile UINT32 *reg,
	UINT32 mask,
	UINT32 shift,
	UINT32 *value
)
{
	*value = (*reg >> shift) & mask;
}

/**
 * @brief   Gpio hardware Pad group select setting function
 * @param   pinIndex[in]: gpio pin index
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32
gpHalGpioSetPadGrp(
	UINT32 pinIndex
)
{
	volatile UINT32 *reg;
	UINT32 regIdx, shift;
	UINT32 gid = GPIO_GROUP_ID(pinIndex);
	UINT32 fid = GPIO_FUNCTION_ID(pinIndex);

	if (gid > 63 || fid > 3) {
		return SP_FAIL;
	}

	regIdx = gid / 16;
	shift = (gid % 16) << 1;

	switch (regIdx) {
	case 0:
		reg = &scubReg->scubPadGrpSel0;
		break;
	case 1:
		reg = &scubReg->scubPadGrpSel1;
		break;
	case 2:
		reg = &scubReg->scubPadGrpSel2;
		break;
	case 3:
		reg = &scubReg->scubPadGrpSel3;
		break;
	}

	writeRegBits(reg, 3, shift, fid);

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetPadGrp);

/**
 * @brief   Gpio hardware direction setting fucntion
 * @param   pinIndex[in]: gpio pin index
 * @param   direction[in]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetDirection(
	UINT32 pinIndex,
	UINT32 direction
)
{
	volatile UINT32 *reg1, *reg2;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg1 = &gpioReg->gpioDirection0;
		reg2 = &scubReg->scubGpio0PinEn;
		break;
	case GPIO_CHANNEL_1:
		reg1 = &gpioReg->gpioDirection1;
		reg2 = &scubReg->scubGpio1PinEn;
		break;
	case GPIO_CHANNEL_2:
		reg1 = &gpioReg->gpioDirection2;
		reg2 = &scubReg->scubGpio2PinEn;
		break;
	case GPIO_CHANNEL_3:
		reg1 = &gpioReg->gpioDirection3;
		reg2 = &scubReg->scubGpio3PinEn;
		break;
	case GPIO_CHANNEL_SAR:
		writeRegBits(&scuaReg->scuaSarGpioCtrl, 1, pin + 4, direction); /* IE */
		writeRegBits(&scuaReg->scuaSarGpioOen, 1, pin, 1 - direction);  /* OE */
		return SP_OK;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg1, 1, pin, direction);
		writeRegBits(reg2, 1, pin, direction);
	}
	else {          /* all bits */
		*reg1 = direction;
		*reg2 = direction;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetDirection);

/**
 * @brief   Gpio hardware direction getting fucntion
 * @param   pinIndex[in]: gpio pin index
 * @param   direction[out]: pin direction, GPIO_DIR_INPUT/GPIO_DIR_OUTPUT
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetDirection(
	UINT32 pinIndex,
	UINT32 *direction
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioDirection0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioDirection1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioDirection2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioDirection3;
		break;
	case GPIO_CHANNEL_SAR:
		readRegBits(&scuaReg->scuaSarGpioCtrl, 1, pin + 4, direction);
		return SP_OK;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		readRegBits(reg, 1, pin, direction);
	}
	else {          /* all bits */
		*direction = *reg;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetDirection);

/**
 * @brief   Gpio hardware GPIO/Normal setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   function[in]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetFunction(
	UINT32 pinIndex,
	UINT32 function
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioEnable0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioEnable1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioEnable2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioEnable3;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg, 1, pin, function);
	}
	else {          /* all bits */
		*reg = function;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetFunction);

/**
 * @brief   Gpio hardware GPIO/Normal getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   function[out]: pin function, GPIO_FUNC_ORG/GPIO_FUNC_GPIO
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetFunction(
	UINT32 pinIndex,
	UINT32 *function
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioEnable0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioEnable1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioEnable2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioEnable3;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		readRegBits(reg, 1, pin, function);
	}
	else {          /* all bits */
		*function = *reg;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetFunction);

/**
 * @brief   Gpio hardware output value setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   value[in]: output value
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetValue(
	UINT32 pinIndex,
	UINT32 value
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioOutData0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioOutData1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioOutData2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioOutData3;
		break;
	case GPIO_CHANNEL_SAR:
		reg = &scuaReg->scuaSarGpioO;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg, 1, pin, value);
	}
	else {          /* all bits */
		*reg = value;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetValue);

/**
 * @brief   Gpio hardware input value getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   value[out]: input value
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetValue(
	UINT32 pinIndex,
	UINT32 *value
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioStatus0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioStatus1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioStatus2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioStatus3;
		break;
	case GPIO_CHANNEL_SAR:
		reg = &scuaReg->scuaSarGpioI;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		readRegBits(reg, 1, pin, value);
	}
	else {          /* all bits */
		*value = *reg;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetValue);

/**
 * @brief   Gpio hardware interrupt pending setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pending[in]: interrupt pending
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetIntPending(
	UINT32 pinIndex,
	UINT32 pending
)
{
	volatile UINT32 *reg;

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioIntPending0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioIntPending1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioIntPending2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioIntPending3;
		break;
	default:
		return SP_FAIL;
	}

	*reg = pending;

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetIntPending);

/**
 * @brief   Gpio hardware interrupt pending getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pending[out]: interrupt pending
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetIntPending(
	UINT32 pinIndex,
	UINT32 *pending
)
{
	volatile UINT32 *reg;

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioIntPending0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioIntPending1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioIntPending2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioIntPending3;
		break;
	default:
		return SP_FAIL;
	}

	*pending = *reg;

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetIntPending);

/**
 * @brief   Gpio hardware debounce counter setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   count[in]: debounce count
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetDebounce(
	UINT32 pinIndex,
	UINT32 count
)
{
	volatile UINT32 *reg;

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioDebounceReg0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioDebounceReg1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioDebounceReg2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioDebounceReg3;
		break;
	default:
		return SP_FAIL;
	}

	*reg = count & 0x003FFFFF; /* 21:0 */

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetDebounce);

/**
 * @brief   Gpio hardware debounce counter getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   count[out]: debounce count
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetDebounce(
	UINT32 pinIndex,
	UINT32 *count
)
{
	volatile UINT32 *reg;

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioDebounceReg0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioDebounceReg1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioDebounceReg2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioDebounceReg3;
		break;
	default:
		return SP_FAIL;
	}

	*count = *reg;

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetDebounce);

/**
 * @brief   Gpio hardware irq polarity setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   polarity[in]: irq polarity,
 *  				GPIO_IRQ_LEVEL_LOW/GPIO_IRQ_LEVEL_HIGH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetPolarity(
	UINT32 pinIndex,
	UINT32 polarity
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioPolarity0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioPolarity1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioPolarity2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioPolarity3;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg, 1, pin, polarity);
	}
	else {          /* all bits */
		*reg = polarity;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetPolarity);

/**
 * @brief   Gpio hardware irq polarity getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   polarity[out]: irq polarity,
 *  				GPIO_IRQ_LEVEL_LOW/GPIO_IRQ_LEVEL_HIGH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetPolarity(
	UINT32 pinIndex,
	UINT32 *polarity
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioPolarity0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioPolarity1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioPolarity2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioPolarity3;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		readRegBits(reg, 1, pin, polarity);
	}
	else {          /* all bits */
		*polarity = *reg;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetPolarity);

/**
 * @brief   Gpio hardware irq edge setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   edge[in]: irq edge,
 *  			GPIO_IRQ_ACTIVE_FALING/GPIO_IRQ_ACTIVE_RISING/GPIO_IRQ_ACTIVE_BOTH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetEdge(
	UINT32 pinIndex,
	UINT32 edge
)
{
	return SP_FAIL;
}
EXPORT_SYMBOL(gpHalGpioSetEdge);

/**
 * @brief   Gpio hardware irq edge getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   edge[out]: irq edge,
 *  			GPIO_IRQ_ACTIVE_FALING/GPIO_IRQ_ACTIVE_RISING/GPIO_IRQ_ACTIVE_BOTH
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetEdge(
	UINT32 pinIndex,
	UINT32 *edge
)
{
	return SP_FAIL;
}
EXPORT_SYMBOL(gpHalGpioGetEdge);

/**
 * @brief   Gpio hardware internal pull high/low setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pullLevel[in]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetPullFunction(
	UINT32 pinIndex,
	UINT32 pullLevel
)
{
	volatile UINT32 *regPE, *regPS;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 31) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		regPE = &scubReg->scubGpio0PinPe;
		regPS = &scubReg->scubGpio0PinPs;
		break;
	case GPIO_CHANNEL_1:
		regPE = &scubReg->scubGpio1PinPe;
		regPS = &scubReg->scubGpio1PinPs;
		break;
	case GPIO_CHANNEL_2:
		regPE = &scubReg->scubGpio2PinPe;
		regPS = &scubReg->scubGpio2PinPs;
		break;
	case GPIO_CHANNEL_3:
		regPE = &scubReg->scubGpio3PinPe;
		regPS = &scubReg->scubGpio3PinPs;
		break;
	case GPIO_CHANNEL_SAR:
		writeRegBits(&scuaReg->scuaSarGpioCtrl, 1, pin + 8, pullLevel);
		return SP_OK;
	default:
		return SP_FAIL;
	}

	switch (pullLevel) {
	case GPIO_PULL_LOW:
		writeRegBits(regPE, 1, pin, 1);
		writeRegBits(regPS, 1, pin, 0);
		break;
	case GPIO_PULL_HIGH:
		writeRegBits(regPE, 1, pin, 1);
		writeRegBits(regPS, 1, pin, 1);
		break;
	case GPIO_PULL_FLOATING:
		writeRegBits(regPE, 1, pin, 0);
		break;
	default:
		return SP_FAIL;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetPullFunction);

/**
 * @brief   Gpio hardware internal pull high/low getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   pullLevel[out]: pull level,
 *  				 GPIO_PULL_LOW/GPIO_PULL_HIGH/GPIO_PULL_FLOATING
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetPullFunction(
	UINT32 pinIndex,
	UINT32 *pullLevel
)
{
	volatile UINT32 *regPE, *regPS;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);
	UINT32 val;

	if (pin > 31) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		regPE = &scubReg->scubGpio0PinPe;
		regPS = &scubReg->scubGpio0PinPs;
		break;
	case GPIO_CHANNEL_1:
		regPE = &scubReg->scubGpio1PinPe;
		regPS = &scubReg->scubGpio1PinPs;
		break;
	case GPIO_CHANNEL_2:
		regPE = &scubReg->scubGpio2PinPe;
		regPS = &scubReg->scubGpio2PinPs;
		break;
	case GPIO_CHANNEL_3:
		regPE = &scubReg->scubGpio3PinPe;
		regPS = &scubReg->scubGpio3PinPs;
		break;
	case GPIO_CHANNEL_SAR:
		readRegBits(&scuaReg->scuaSarGpioCtrl, 1, pin + 8, pullLevel);
		return SP_OK;
	default:
		return SP_FAIL;
	}

	readRegBits(regPE, 1, pin, &val);
	if (val == 0) {
		*pullLevel = GPIO_PULL_FLOATING;
	}
	else {
		readRegBits(regPS, 1, pin, &val);
		*pullLevel = (val == 0) ? GPIO_PULL_LOW : GPIO_PULL_HIGH;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetPullFunction);

/**
 * @brief   Gpio hardware driving current setting function
 * @param   pinIndex[in]: gpio pin index
 * @param   drivingCurrent[in]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioSetDrivingCurrent(
	UINT32 pinIndex,
	UINT32 drivingCurrent
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &scubReg->scubGpio0PinDs;
		break;
	case GPIO_CHANNEL_1:
		reg = &scubReg->scubGpio1PinDs;
		break;
	case GPIO_CHANNEL_2:
		reg = &scubReg->scubGpio2PinDs;
		break;
	case GPIO_CHANNEL_3:
		reg = &scubReg->scubGpio3PinDs;
		break;
	case GPIO_CHANNEL_SAR:
		reg = &scuaReg->scuaSarGpioCtrl;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg, 1, pin, drivingCurrent);
	}
	else {          /* all bits */
		*reg = drivingCurrent;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioSetDrivingCurrent);

/**
 * @brief   Gpio hardware driving current getting function
 * @param   pinIndex[in]: gpio pin index
 * @param   drivingCurrent[out]: driving current value(0:4mA, 1:8mA)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioGetDrivingCurrent(
	UINT32 pinIndex,
	UINT32 *drivingCurrent
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &scubReg->scubGpio0PinDs;
		break;
	case GPIO_CHANNEL_1:
		reg = &scubReg->scubGpio1PinDs;
		break;
	case GPIO_CHANNEL_2:
		reg = &scubReg->scubGpio2PinDs;
		break;
	case GPIO_CHANNEL_3:
		reg = &scubReg->scubGpio3PinDs;
		break;
	case GPIO_CHANNEL_SAR:
		reg = &scuaReg->scuaSarGpioCtrl;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		readRegBits(reg, 1, pin, drivingCurrent);
	}
	else {          /* all bits */
		*drivingCurrent = *reg;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioGetDrivingCurrent);

/**
 * @brief   Gpio hardware irq enable/disable function
 * @param   pinIndex[in]: gpio pin index
 * @param   enable[in]: irq disable(0)/enable(1)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioEnableIrq(
	UINT32 pinIndex,
	UINT32 enable
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioIntEn0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioIntEn1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioIntEn2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioIntEn3;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg, 1, pin, enable);
	}
	else {          /* all bits */
		*reg = enable;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioEnableIrq);

/**
 * @brief   Gpio hardware debounce enable/disable function
 * @param   pinIndex[in]: gpio pin index
 * @param   enable[in]: debounce disable(0)/enable(1)
 * @return  SP_OK(0)/SP_FAIL
 */
UINT32
gpHalGpioEnableDebounce(
	UINT32 pinIndex,
	UINT32 enable
)
{
	volatile UINT32 *reg;
	UINT32 pin = GPIO_PIN_NUMBER(pinIndex);

	if (pin > 32) {
		return SP_FAIL;
	}

	switch (GPIO_CHANNEL(pinIndex)) {
	case GPIO_CHANNEL_0:
		reg = &gpioReg->gpioDebounceEn0;
		break;
	case GPIO_CHANNEL_1:
		reg = &gpioReg->gpioDebounceEn1;
		break;
	case GPIO_CHANNEL_2:
		reg = &gpioReg->gpioDebounceEn2;
		break;
	case GPIO_CHANNEL_3:
		reg = &gpioReg->gpioDebounceEn3;
		break;
	default:
		return SP_FAIL;
	}

	if (pin < 32) { /* single bits */
		writeRegBits(reg, 1, pin, enable?1:0);
	}
	else {          /* all bits */
		*reg = enable;
	}

	return SP_OK;
}
EXPORT_SYMBOL(gpHalGpioEnableDebounce);

/**
 * @brief   Gpio clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalGpioClkEnable(
	UINT32 enable
)
{
	if (enable){
		scubReg->scubPeriClkEn |= SCU_B_PERI_GPIO;
	}
	else{
		scubReg->scubPeriClkEn &= ~SCU_B_PERI_GPIO;
	}
}
EXPORT_SYMBOL(gpHalGpioClkEnable);
