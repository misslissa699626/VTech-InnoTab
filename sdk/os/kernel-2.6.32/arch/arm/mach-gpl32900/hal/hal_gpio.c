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
#include <mach/hal/regmap/reg_mipi.h>
#include <mach/hal/regmap/reg_adc.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define GPIO_PULL_LOW           0
#define GPIO_PULL_HIGH          1
#define GPIO_PULL_FLOATING      2

/* P_GPIO_PINMUX */
#define PINMUX_00_ARMJTAG       (1 << 0)
#define PINMUX_01_CEVAJTAG      (1 << 1)
#define PINMUX_02_CSIDAT        (1 << 2)
#define PINMUX_03_NAND0CS       (1 << 3)
#define PINMUX_04_NAND1CS       (1 << 4)
#define PINMUX_05_CSIFIELD      (1 << 5)
#define PINMUX_06_LCDTCON       (1 << 6)
#define PINMUX_07_I2C           (1 << 7)
#define PINMUX_08_PPUI80        (1 << 8)
#define PINMUX_09_NANDRDY       (1 << 9)
#define PINMUX_10_LCMTE1        (1 << 10)
#define PINMUX_11_SPI1          (1 << 11)
#define PINMUX_12_CSICH         (1 << 12)
#define PINMUX_13_NANDCTRL      (1 << 13)
#define PINMUX_14_NANDCS        (1 << 14)
#define PINMUX_15_I2S           (1 << 15)
#define PINMUX_16_PWM1          (1 << 16)
#define PINMUX_17_PWM2          (1 << 17)
#define PINMUX_18_CDSPRAW1      (1 << 18)
#define PINMUX_19_SHARENAND     (1 << 19)
#define PINMUX_20_SD0CMD        (1 << 20)
#define PINMUX_21_SD1CMD        (1 << 21)
#define PINMUX_22_RESERVED      (1 << 22)
#define PINMUX_23_RESERVED      (1 << 23)
#define PINMUX_24_CSICTRL       (1 << 24)
#define PINMUX_25_MIPICLK1      (1 << 25)
#define PINMUX_26_MIPICLK2      (1 << 26)
#define PINMUX_27_CDSPRAW2      (1 << 27)
#define PINMUX_28_LCMTE2        (1 << 28)
#define PINMUX_29_SPICS1        (1 << 29)
#define PINMUX_30_SPICS0        (1 << 30)
#define PINMUX_31_SPI2          (1 << 31)

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

#define MK_MASK(m0, m1, m2, m3)         \
    {                                   \
        .clearMask = m0 | m1 | m2 | m3, \
        .setMask = {m0, m1, m2, m3 }    \
    }

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct pinMuxMask_s {
    UINT32 clearMask;   /*!< @brief all bits for clear special functions,
                                    clearMask = setMask[0] | setMask[1] | setMask[2] | setMask[3] */
    UINT32 setMask[4];  /*!< @brief bit for special function (fid: 4~7) */
} pinMuxMask_t;

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
 * @brief   Gpio hardware Pad group setting function
 * @param   pinIndex[in]: gpio pin index
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32
gpHalGpioSetPadGrp(
    UINT32 pinIndex
)
{
    static pinMuxMask_t pinMuxTable[GPIO_GID_MAX + 2] = {
        MK_MASK(PINMUX_26_MIPICLK2, 0, 0, 0), /* GID_0*/
        {}, /* GID_1 */
        {}, /* GID_2 */
        {}, /* GID_3 */
        {}, /* GID_4 */
        {}, /* GID_5 */
        {}, /* GID_6 */
        {}, /* GID_7 */
        MK_MASK(PINMUX_02_CSIDAT, PINMUX_25_MIPICLK1, PINMUX_10_LCMTE1, 0), /* GID_8 */
        MK_MASK(PINMUX_06_LCDTCON, 0, 0, 0), /* GID_9 */
        MK_MASK(PINMUX_06_LCDTCON, 0, 0, 0), /* GID_10 */
        {}, /* GID_11 */
        {}, /* GID_12 */
        MK_MASK(PINMUX_00_ARMJTAG, PINMUX_15_I2S, PINMUX_17_PWM2, PINMUX_11_SPI1), /* GID_13 */
        {}, /* GID_14 */
        {}, /* GID_15 */
        {}, /* GID_16 */
        {}, /* GID_17 */
        MK_MASK(0, 0, PINMUX_12_CSICH, 0), /* GID_18 */
		MK_MASK(0, 0, PINMUX_12_CSICH, 0), /* GID_19 */
		MK_MASK(0, 0, PINMUX_12_CSICH, 0), /* GID_20 */
        MK_MASK(PINMUX_01_CEVAJTAG, PINMUX_16_PWM1, PINMUX_12_CSICH, PINMUX_14_NANDCS), /* GID_21 */
        MK_MASK(PINMUX_01_CEVAJTAG, 0, PINMUX_12_CSICH, PINMUX_13_NANDCTRL), /* GID_22 */
        MK_MASK(PINMUX_01_CEVAJTAG, 0, PINMUX_12_CSICH, PINMUX_13_NANDCTRL), /* GID_23 */
        MK_MASK(PINMUX_01_CEVAJTAG, 0, PINMUX_12_CSICH, PINMUX_13_NANDCTRL), /* GID_24 */
        {}, /* GID_25 */
        {}, /* GID_26 */
        {}, /* GID_27 */
        {}, /* GID_28 */
        {}, /* GID_29 */
        {}, /* GID_30 */
        MK_MASK(PINMUX_00_ARMJTAG, PINMUX_15_I2S, PINMUX_12_CSICH, PINMUX_11_SPI1), /* GID_31 */
        MK_MASK(PINMUX_00_ARMJTAG, PINMUX_15_I2S, PINMUX_12_CSICH, PINMUX_11_SPI1), /* GID_32 */
        MK_MASK(0/*DSUB?*/, PINMUX_07_I2C, PINMUX_12_CSICH, 0), /* GID_33 */
        {}, /* GID_34 */
        MK_MASK(PINMUX_24_CSICTRL, PINMUX_29_SPICS1|PINMUX_30_SPICS0|PINMUX_31_SPI2, PINMUX_28_LCMTE2, PINMUX_27_CDSPRAW2)  /* GID_UNKOWN */
    };
    UINT32 gid = GPIO_GROUP_ID(pinIndex);
    UINT32 fid = GPIO_FUNCTION_ID(pinIndex);
    UINT32 pin = GPIO_PIN_NUMBER(pinIndex);
    UINT32 ch = GPIO_CHANNEL(pinIndex);
    UINT32 regVal;
	pinMuxMask_t *pinMux;
	mipiReg_t *mipiReg = (mipiReg_t *)LOGI_ADDR_MIPI_REG;
	regADC_t  *adcReg  = (regADC_t *)LOGI_ADDR_ADC_REG;

    if ((fid > 7) || (gid > GPIO_GID_UNKOWN)) {
        return SP_FAIL;
    }

    if (gid == GPIO_GID_UNKOWN) {
        /* Analog Module configure */
        if ((ch == 0) && (pin >= 16) && (pin <= 19)) {      /* IOA16~19 */
            if (fid == 0) {
                /* Enable Touch Panel */
            }
            else {
                /* Disable Touch Panel */
                adcReg->SARCTRL &= ~0x3; 
            }
        }
        else if ((ch == 2) && (pin >= 24) && (pin <= 29)) { /* IOC24~29 */
            if (fid == 0) {
                /* Enable MIPI */
                mipiReg->mipiGlbCsr |= 0x1;
            }
            else {
                /* Disable MIPI */
                mipiReg->mipiGlbCsr &= ~0x1;
            }
        }
        else if (ch == 3) {
            if (pin == 25) {                                /* IOD25 */
                if (fid == 0) {
                    scubReg->scubPwmCtrl |= 0x02;   /* Enable PWM0 */
                }
                else {
                    scubReg->scubPwmCtrl &= ~0x02;  /* Disable PWM0 */
                }
            }
            else if ((pin == 26) || (pin == 27)) {          /* IOD26~27 */
                if (fid == 0) {
                    scubReg->scubPwmCtrl |= 0x04;   /* Enable PWM1 */
                }
                else {
                    scubReg->scubPwmCtrl &= ~0x04;  /* Disable PWM1 */
                }
            }
        }
    }

    /* Pin Mux configure */
	pinMux = &pinMuxTable[gid];
    regVal = scubReg->scubPinMux;
    regVal &= ~pinMux->clearMask;
	if (pinMux->clearMask & PINMUX_00_ARMJTAG) {
		regVal |= PINMUX_00_ARMJTAG; /* this bit is reversed: 1 disable, 0 enable */
	}
    if (fid > 4) {
		if (pinMux->setMask[fid - 4] != PINMUX_00_ARMJTAG) {
			regVal |= pinMux->setMask[fid - 4];
		}
		else {
			regVal &= ~PINMUX_00_ARMJTAG;
		}
    }
    scubReg->scubPinMux = regVal;

    /* PGS configure */
    if ((gid <= GPIO_GID_MAX) && (fid < 4)) {
        volatile UINT32 *reg;
        UINT32 regIdx = gid / 16;
        UINT32 shift = (gid % 16) << 1;

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
    }

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
 * @brief   Gpio Value Save function
 * @param   ptr[in]: buffer point
 */
void
gpHalGpioRegSave( int* ptr ) {
	int j = 0;
	/*Save pin mux*/
	ptr[j++] = scubReg->scubPinMux;
	
	/*Save pin configuration*/
	ptr[j++] = scubReg->scubPadGrpSel0;
	ptr[j++] = scubReg->scubPadGrpSel1;
	ptr[j++] = scubReg->scubPadGrpSel2;
	ptr[j++] = scubReg->scubPadGrpSel3;
	
	ptr[j++] = scubReg->scubGpio0PinEn;
	ptr[j++] = scubReg->scubGpio0PinDs;
	ptr[j++] = scubReg->scubGpio0PinPe;
	ptr[j++] = scubReg->scubGpio0PinPs;
	               
	ptr[j++] = scubReg->scubGpio1PinEn;
	ptr[j++] = scubReg->scubGpio1PinDs;
	ptr[j++] = scubReg->scubGpio1PinPe;
	ptr[j++] = scubReg->scubGpio1PinPs;
	               
	ptr[j++] = scubReg->scubGpio2PinEn;
	ptr[j++] = scubReg->scubGpio2PinDs;
	ptr[j++] = scubReg->scubGpio2PinPe;
	ptr[j++] = scubReg->scubGpio2PinPs;
	               
	ptr[j++] = scubReg->scubGpio3PinEn;
	ptr[j++] = scubReg->scubGpio3PinDs;
	ptr[j++] = scubReg->scubGpio3PinPe;
	ptr[j++] = scubReg->scubGpio3PinPs;
	
	ptr[j++] = gpioReg->gpioEnable0;     
	ptr[j++] = gpioReg->gpioEnable1;     
	ptr[j++] = gpioReg->gpioEnable2;     
	ptr[j++] = gpioReg->gpioEnable3;     
	                                     
	ptr[j++] = gpioReg->gpioOutData0;    
	ptr[j++] = gpioReg->gpioOutData1;    
	ptr[j++] = gpioReg->gpioOutData2;    
	ptr[j++] = gpioReg->gpioOutData3;    
	                                     
	ptr[j++] = gpioReg->gpioDirection0;  
	ptr[j++] = gpioReg->gpioDirection1;  
	ptr[j++] = gpioReg->gpioDirection2;  
	ptr[j++] = gpioReg->gpioDirection3;  
	                                     
	ptr[j++] = gpioReg->gpioPolarity0;   
	ptr[j++] = gpioReg->gpioPolarity1;   
	ptr[j++] = gpioReg->gpioPolarity2;   
	ptr[j++] = gpioReg->gpioPolarity3;   
	                                     
	ptr[j++] = gpioReg->gpioSticky0;     
	ptr[j++] = gpioReg->gpioSticky1;     
	ptr[j++] = gpioReg->gpioSticky2;     
	ptr[j++] = gpioReg->gpioSticky3;     
	                                     
	ptr[j++] = gpioReg->gpioIntEn0;      
	ptr[j++] = gpioReg->gpioIntEn1;      
	ptr[j++] = gpioReg->gpioIntEn2;      
	ptr[j++] = gpioReg->gpioIntEn3;      
	                                     
	ptr[j++] = gpioReg->gpioIntPending0; 
	ptr[j++] = gpioReg->gpioIntPending1; 
	ptr[j++] = gpioReg->gpioIntPending2; 
	ptr[j++] = gpioReg->gpioIntPending3; 
	                                     
	ptr[j++] = gpioReg->gpioDebounceReg0;
	ptr[j++] = gpioReg->gpioDebounceReg1;
	ptr[j++] = gpioReg->gpioDebounceReg2;
	ptr[j++] = gpioReg->gpioDebounceReg3;
	                                     
	ptr[j++] = gpioReg->gpioDebounceEn0; 
	ptr[j++] = gpioReg->gpioDebounceEn1; 
	ptr[j++] = gpioReg->gpioDebounceEn2; 
	ptr[j++] = gpioReg->gpioDebounceEn3; 
	                                     
	ptr[j++] = gpioReg->gpioWakeupEn0;   
	ptr[j++] = gpioReg->gpioWakeupEn1;   
	ptr[j++] = gpioReg->gpioWakeupEn2;   
	ptr[j++] = gpioReg->gpioWakeupEn3;   
	                                     
	ptr[j++] = gpioReg->gpioStatus0;     
	ptr[j++] = gpioReg->gpioStatus1;     
	ptr[j++] = gpioReg->gpioStatus2;     
	ptr[j++] = gpioReg->gpioStatus3;     
}
EXPORT_SYMBOL(gpHalGpioRegSave);

/**
 * @brief   Gpio Value Save function
 * @param   ptr[in]: buffer point
 */
void
gpHalGpioRegRestore( int* ptr ) {
	int j = 0;

	scubReg->scubPinMux = ptr[j++];
	                                                    
	scubReg->scubPadGrpSel0 = ptr[j++];  
	scubReg->scubPadGrpSel1 = ptr[j++];  
	scubReg->scubPadGrpSel2 = ptr[j++];  
	scubReg->scubPadGrpSel3 = ptr[j++];  
	                          
	scubReg->scubGpio0PinEn = ptr[j++];;  
	scubReg->scubGpio0PinDs = ptr[j++];;  
	scubReg->scubGpio0PinPe = ptr[j++];;  
	scubReg->scubGpio0PinPs = ptr[j++];;  
	                          
	scubReg->scubGpio1PinEn = ptr[j++];;  
	scubReg->scubGpio1PinDs = ptr[j++];;  
	scubReg->scubGpio1PinPe = ptr[j++];;  
	scubReg->scubGpio1PinPs = ptr[j++];;  
	                          
	scubReg->scubGpio2PinEn = ptr[j++];;  
	scubReg->scubGpio2PinDs = ptr[j++];;  
	scubReg->scubGpio2PinPe = ptr[j++];;  
	scubReg->scubGpio2PinPs = ptr[j++];;  
	                          
	scubReg->scubGpio3PinEn = ptr[j++];;  
	scubReg->scubGpio3PinDs = ptr[j++];;  
	scubReg->scubGpio3PinPe = ptr[j++];;  
	scubReg->scubGpio3PinPs = ptr[j++];;  
	                          
	gpioReg->gpioEnable0 = ptr[j++];;     
	gpioReg->gpioEnable1 = ptr[j++];;     
	gpioReg->gpioEnable2 = ptr[j++];;     
	gpioReg->gpioEnable3 = ptr[j++];;     
	                          
	gpioReg->gpioOutData0 = ptr[j++];;    
	gpioReg->gpioOutData1 = ptr[j++];;    
	gpioReg->gpioOutData2 = ptr[j++];;    
	gpioReg->gpioOutData3 = ptr[j++];;    
	                          
	gpioReg->gpioDirection0 = ptr[j++];;  
	gpioReg->gpioDirection1 = ptr[j++];;  
	gpioReg->gpioDirection2 = ptr[j++];;  
	gpioReg->gpioDirection3 = ptr[j++];;  
	                          
	gpioReg->gpioPolarity0 = ptr[j++];;   
	gpioReg->gpioPolarity1 = ptr[j++];;   
	gpioReg->gpioPolarity2 = ptr[j++];;   
	gpioReg->gpioPolarity3 = ptr[j++];;   
	                          
	gpioReg->gpioSticky0 = ptr[j++];;     
	gpioReg->gpioSticky1 = ptr[j++];;     
	gpioReg->gpioSticky2 = ptr[j++];;     
	gpioReg->gpioSticky3 = ptr[j++];;     
	                          
	gpioReg->gpioIntEn0 = ptr[j++];;      
	gpioReg->gpioIntEn1 = ptr[j++];;      
	gpioReg->gpioIntEn2 = ptr[j++];;      
	gpioReg->gpioIntEn3 = ptr[j++];;      
	                          
	gpioReg->gpioIntPending0 = ptr[j++];; 
	gpioReg->gpioIntPending1 = ptr[j++];; 
	gpioReg->gpioIntPending2 = ptr[j++];; 
	gpioReg->gpioIntPending3 = ptr[j++];; 
	                          
	gpioReg->gpioDebounceReg0 = ptr[j++];;
	gpioReg->gpioDebounceReg1 = ptr[j++];;
	gpioReg->gpioDebounceReg2 = ptr[j++];;
	gpioReg->gpioDebounceReg3 = ptr[j++];;
	                          
	gpioReg->gpioDebounceEn0 = ptr[j++];; 
	gpioReg->gpioDebounceEn1 = ptr[j++];; 
	gpioReg->gpioDebounceEn2 = ptr[j++];; 
	gpioReg->gpioDebounceEn3 = ptr[j++];; 
	                          
	gpioReg->gpioWakeupEn0 = ptr[j++];;   
	gpioReg->gpioWakeupEn1 = ptr[j++];;   
	gpioReg->gpioWakeupEn2 = ptr[j++];;   
	gpioReg->gpioWakeupEn3 = ptr[j++];;   
	                          
	gpioReg->gpioStatus0 = ptr[j++];;     
	gpioReg->gpioStatus1 = ptr[j++];;     
	gpioReg->gpioStatus2 = ptr[j++];;     
	gpioReg->gpioStatus3 = ptr[j++];;     
}
EXPORT_SYMBOL(gpHalGpioRegRestore);
