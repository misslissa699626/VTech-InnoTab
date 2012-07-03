#include <linux/irqflags.h>
#include <mach/hardware.h>
#include <mach/regs-i2c_bus.h>

#include "hal_base.h"
//=============================================================================
//
//      hal_base.c
//
//      NF HAL Base API
//
//=============================================================================

void nand_cache_sync(void)
{
    unsigned long  irq_state = 0;
	local_irq_save(irq_state); 
	asm volatile (                                           \
		"mcr  p15, 0, r7, c8, c7, 0;"                        \
		:                                                    \
		:                                                    \
		: "r7" /* Clobber list */                            \
	);
	local_irq_restore(irq_state); 
}
//----------------------------------------------------------------------------
void nand_cache_invalidate(void)
{
 	unsigned long  irq_state = 0;
	local_irq_save(irq_state); 

	asm volatile (                                                      \
		"mcr  p15, 0, r7, c8, c7, 0;" /* flush i+d-TLBs */              \
		"mcr  p15, 0, r7, c7, c6, 0;"                                   \
		:                                                               \
		:                                                               \
		: "r7" /* clobber list */
	);     
	 
	local_irq_restore(irq_state); 
}
//----------------------------------------------------------------------------
void cache_sync(void)
{    
/* modify by mm.li 01-12,2011 clean warning */
	/*
	unsigned int irq_state = 0;
	*/
	unsigned long irq_state = 0;
/* modify end */	
	local_irq_save(irq_state); 
	asm volatile (                                                      \
        "1: mrc    p15, 0, r15, c7, c10, 3;"  /* test and clean dcache */ \
        "bne    1b;"  \
        "mov   r7, #0;" \
        "mcr    p15, 0 , r7 , c7 , c10, 4;" /* and drain the write buffer */    \
        :                                                               \
        :                                                               \
        : "r7" /* Clobber list */                                       \
    );
	local_irq_restore(irq_state); 
}
//----------------------------------------------------------------------------
void cache_invalidate(void)
{
/* modify by mm.li 01-12,2011 clean warning */
	/*
	unsigned int irq_state = 0;
	*/
	unsigned long irq_state = 0;
/* modify end */	
	local_irq_save(irq_state); 
	asm volatile (                                                      \
		"mov    r0,#0;"                                                 \
        "mcr    p15,0,r0,c7,c6,0;" /* flush d-cache */                  \
        "mcr    p15,0,r0,c8,c7,0;" /* flush i+d-TLBs */                 \
        :                                                               \
        :                                                               \
        : "r0","memory" /* clobber list */
	);    
	local_irq_restore(irq_state);  
}
void flag_setbits(unsigned int *flag, unsigned int value)
{
	*flag |= value;
}

#if 1
//这部分需要统一到GPIO模块
//----------------------------------------------------------------------------


unsigned int gpio_get_dir(unsigned int aGrp, unsigned int aPin)
{
	unsigned int val = 0;
	unsigned int pin = (1 << aPin);
	switch(aGrp){
	default:
		break;
	case 0:
		REG_GPIO_DIRECTION_0 =  val;
		break;
	case 1:
		REG_GPIO_DIRECTION_1 = val;
		break;
	case 2:
		REG_GPIO_DIRECTION_2 = val;
		break;
	case 3:
		REG_GPIO_DIRECTION_3 = val;
		break;
	}
	
	if(0 == (val & pin)){
		return 0;
	}
	else{
		return 1;
	}
}

void gpio_set_data(unsigned int aGrp, unsigned int aPin, unsigned int aData)
{
	unsigned int val = 0;
	unsigned int datPin = (1 << aPin);
	if(1 == gpio_get_dir(aGrp, aPin)){
		return;
	}
	switch(aGrp){
	case 0:
		REG_GPIO_OUT_DATA_0 = val;
		val &= ~(datPin);
		if(1 == aData){
			val |= datPin;
		}
		REG_GPIO_OUT_DATA_0 = val;
		break;
	case 1:
		REG_GPIO_OUT_DATA_1 = val;
		val &= ~(datPin);
		if(1 == aData){
			val |= datPin;
		}
		REG_GPIO_OUT_DATA_1 = val;
		break;
	case 2:
		REG_GPIO_OUT_DATA_2 = val;
		val &= ~(datPin);
		if(1 == aData){
			val |= datPin;
		}
		REG_GPIO_OUT_DATA_2 = val;
		break;
	case 3:
		REG_GPIO_OUT_DATA_3 = val;
		val &= ~(datPin);
		if(1 == aData){
			val |= datPin;
		}
		REG_GPIO_OUT_DATA_3 = val;
		break;
	}	
}

#endif

