/*
 * arch/arm/mach-spmp8000/include/mach/regs-i2c_bus.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * i2c_bus - System peripherals regsters.
 *
 */



#define I2C_C_BASE		IO2_ADDRESS(0xB03000)

#define I2C_C_ICCR	    	(*(volatile unsigned *) (I2C_C_BASE + 0x00))
#define I2C_C_ICSR	    	(*(volatile unsigned *) (I2C_C_BASE + 0x04))
#define I2C_C_IAR	    		(*(volatile unsigned *) (I2C_C_BASE + 0x08))
#define I2C_C_IDSR	    	(*(volatile unsigned *) (I2C_C_BASE + 0x0C))
#define I2C_C_IDEBCLK			(*(volatile unsigned *) (I2C_C_BASE + 0x10))
#define I2C_C_TXLCLK			(*(volatile unsigned *) (I2C_C_BASE + 0x14))

#define I2C_C_ICCR_OFST    0x00
#define I2C_C_ICSR_OFST    0x04
#define I2C_C_IAR_OFST     0x08
#define I2C_C_IDSR_OFST    0x0C
#define I2C_C_IDEBCLK_OFST 0x10
#define I2C_C_TXLCLK_OFST  0x14

#define ICCR_INIT				0x00
#define IDEBCLK_INIT		0x04
#define TXLCLK_INIT			0x40

#define ICCR_TXCLKMSB_MASK  0x0F
#define ICCR_INTPREND       0x10
#define ICCR_INTREN         0x20
#define ICCR_ACKEN          0x80

#define ICSR_NONACK          	0x01
#define ICSR_ZEROSTS_SSCON    0x00
#define ICSR_ZEROSTS_SLV_ARRS 0x02
#define ICSR_SLVSTS_SSCON     0x00
#define ICSR_SLVSTS_SLV_MATCH 0x04
#define ICSR_ARB_OK           0x00
#define ICSR_ARB_FAIL         0x08
#define ICSR_TXRX_ENABLE     	0x10
#define ICSR_NONBUSY_STS      0x00
#define ICSR_BUSY_STS         0x20
#define ICSR_STOP             0x00
#define ICSR_START            0x20
#define ICSR_SLAVE_RX       	0x00
#define ICSR_SLAVE_TX       	0x40
#define ICSR_MASTER_RX      	0x80
#define ICSR_MASTER_TX     		0xC0

#define GPIO_50_BASE IO0_ADDRESS(0xA000)
#define REG_GPIO_ENABLE_0 (*(volatile unsigned int*)(GPIO_50_BASE+0x00))
#define REG_GPIO_ENABLE_1 (*(volatile unsigned int*)(GPIO_50_BASE+0x04))
#define REG_GPIO_ENABLE_2 (*(volatile unsigned int*)(GPIO_50_BASE+0x08))
#define REG_GPIO_ENABLE_3 (*(volatile unsigned int*)(GPIO_50_BASE+0x0C))
#define REG_GPIO_OUT_DATA_0 (*(volatile unsigned int*)(GPIO_50_BASE+0x10))
#define REG_GPIO_OUT_DATA_1 (*(volatile unsigned int*)(GPIO_50_BASE+0x14))
#define REG_GPIO_OUT_DATA_2 (*(volatile unsigned int*)(GPIO_50_BASE+0x18))
#define REG_GPIO_OUT_DATA_3 (*(volatile unsigned int*)(GPIO_50_BASE+0x1C))
#define REG_GPIO_DIRECTION_0 (*(volatile unsigned int*)(GPIO_50_BASE+0x20))
#define REG_GPIO_DIRECTION_1 (*(volatile unsigned int*)(GPIO_50_BASE+0x24))
#define REG_GPIO_DIRECTION_2 (*(volatile unsigned int*)(GPIO_50_BASE+0x28))
#define REG_GPIO_DIRECTION_3 (*(volatile unsigned int*)(GPIO_50_BASE+0x2C))

