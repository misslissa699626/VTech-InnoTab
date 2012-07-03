/*
 * arch/arm/mach-spmp8000/include/mach/regs-gpio.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Timer - System peripherals regsters.
 *
 */
#define GPIO_BASE_ADDR	IO0_ADDRESS(0xA000)	 

#define GPIO_ENABLE_0			((volatile unsigned int *)(GPIO_BASE_ADDR+0x00))
#define GPIO_ENABLE_1			((volatile unsigned int *)(GPIO_BASE_ADDR+0x04))
#define GPIO_ENABLE_2			((volatile unsigned int *)(GPIO_BASE_ADDR+0x08))
#define GPIO_ENABLE_3			((volatile unsigned int *)(GPIO_BASE_ADDR+0x0C))

#define GPIO_OUT_DATA_0 	    ((volatile unsigned int *)(GPIO_BASE_ADDR+0x10))
#define GPIO_OUT_DATA_1  		((volatile unsigned int *)(GPIO_BASE_ADDR+0x14))
#define GPIO_OUT_DATA_2 	    ((volatile unsigned int *)(GPIO_BASE_ADDR+0x18))
#define GPIO_OUT_DATA_3  		((volatile unsigned int *)(GPIO_BASE_ADDR+0x1C))

#define GPIO_DIRECTION_0 		((volatile unsigned int *)(GPIO_BASE_ADDR+0x20))
#define GPIO_DIRECTION_1 		((volatile unsigned int *)(GPIO_BASE_ADDR+0x24))
#define GPIO_DIRECTION_2 		((volatile unsigned int *)(GPIO_BASE_ADDR+0x28))
#define GPIO_DIRECTION_3 		((volatile unsigned int *)(GPIO_BASE_ADDR+0x2C))

#define GPIO_POLARITY_0			((volatile unsigned int *)(GPIO_BASE_ADDR+0x30))
#define GPIO_POLARITY_1			((volatile unsigned int *)(GPIO_BASE_ADDR+0x34))
#define GPIO_POLARITY_2			((volatile unsigned int *)(GPIO_BASE_ADDR+0x38))
#define GPIO_POLARITY_3			((volatile unsigned int *)(GPIO_BASE_ADDR+0x3C))

#define GPIO_STICKY_0	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x40))
#define GPIO_STICKY_1	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x44))
#define GPIO_STICKY_2	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x48))
#define GPIO_STICKY_3	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x4C))

#define GPIO_PULLUP_EN_0	  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x50))
#define GPIO_PULLUP_EN_1	  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x54))
#define GPIO_PULLUP_EN_2	  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x58))
#define GPIO_PULLUP_EN_3	  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x5C))

#define GPIO_PULLDOWN_EN_0		((volatile unsigned int *)(GPIO_BASE_ADDR+0x60))
#define GPIO_PULLDOWN_EN_1		((volatile unsigned int *)(GPIO_BASE_ADDR+0x64))
#define GPIO_PULLDOWN_EN_2		((volatile unsigned int *)(GPIO_BASE_ADDR+0x68))
#define GPIO_PULLDOWN_EN_3		((volatile unsigned int *)(GPIO_BASE_ADDR+0x6C))

#define GPIO_INTEN_0	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x50))
#define GPIO_INTEN_1	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x54))
#define GPIO_INTEN_2	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x58))
#define GPIO_INTEN_3	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x5C))

#define GPIO_INT_PENDING_0	    ((volatile unsigned int *)(GPIO_BASE_ADDR+0x60))
#define GPIO_INT_PENDING_1	((volatile unsigned int *)(GPIO_BASE_ADDR+0x64))
#define GPIO_INT_PENDING_2	    ((volatile unsigned int *)(GPIO_BASE_ADDR+0x68))
#define GPIO_INT_PENDING_3	((volatile unsigned int *)(GPIO_BASE_ADDR+0x6C))


#define GPIO_DEBOUNCE_REG_0  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x70))
#define GPIO_DEBOUNCE_REG_1  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x74))
#define GPIO_DEBOUNCE_REG_2  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x78))
#define GPIO_DEBOUNCE_REG_3  	((volatile unsigned int *)(GPIO_BASE_ADDR+0x7C))

#define GPIO_DEBOUNCE_EN_0	       ((volatile unsigned int *)(GPIO_BASE_ADDR+0x80))
#define GPIO_DEBOUNCE_EN_1	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x84))
#define GPIO_DEBOUNCE_EN_2	       ((volatile unsigned int *)(GPIO_BASE_ADDR+0x88))
#define GPIO_DEBOUNCE_EN_3	        ((volatile unsigned int *)(GPIO_BASE_ADDR+0x8C))

#define GPIO_WAKEUP_EN_0	((volatile unsigned int *)(GPIO_BASE_ADDR+0x90))
#define GPIO_WAKEUP_EN_1	((volatile unsigned int *)(GPIO_BASE_ADDR+0x94))
#define GPIO_WAKEUP_EN_2	((volatile unsigned int *)(GPIO_BASE_ADDR+0x98))
#define GPIO_WAKEUP_EN_3	((volatile unsigned int *)(GPIO_BASE_ADDR+0x9C))

#define GPIO_STATUS_0	          ((volatile unsigned int *)(GPIO_BASE_ADDR+0xA0))
#define GPIO_STATUS_1	          ((volatile unsigned int *)(GPIO_BASE_ADDR+0xA4))
#define GPIO_STATUS_2	          ((volatile unsigned int *)(GPIO_BASE_ADDR+0xA8))
#define GPIO_STATUS_3	          ((volatile unsigned int *)(GPIO_BASE_ADDR+0xAC))


