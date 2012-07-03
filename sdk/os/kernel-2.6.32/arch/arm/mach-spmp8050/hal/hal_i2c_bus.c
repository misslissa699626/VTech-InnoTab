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
 * @file    hal_i2c_bus.c
 * @brief   Implement of SPMP8050 I2C Bus HAL API.
 * @author  junp.zhang
 * @since   2010/10/12
 * @date    2010/10/12
 */
#include <mach/kernel.h>
#include <mach/hal/hal_common.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_i2c_bus.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define I2C_BUS_WRITE 			0
#define I2C_BUS_READ  			1
#define DELAY_COUNT 			50000
#define SYS_AHB_CLK 			30375

#define ICCR_INIT				0x00
#define IDEBCLK_INIT			0x04

#define ICCR_TXCLKMSB_MASK  	0x0F
#define ICCR_INTPREND      	 	0x10
#define ICCR_INTREN         	0x20
#define ICCR_ACKEN          	0x80

#define ICSR_NONACK          	0x01
#define ICSR_TXRX_ENABLE		0x10
#define ICSR_NONBUSY_STS      	0x00
#define ICSR_BUSY_STS         	0x20
#define ICSR_START            	0x20
#define ICSR_MASTER_RX      	0x80
#define ICSR_MASTER_TX     		0xC0

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

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

static i2cBusReg_t *i2cBusReg = (i2cBusReg_t *)(LOGI_ADDR_I2C_BUS_REG);
static scubReg_t *scubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;

/**
 * @brief   I2C Bus hardware initial
 * @return  None
 * @see
 */
void 
gpHalI2cBusInit(
	void
)
{
	i2cBusReg->iccr = ICCR_INIT;
	i2cBusReg->ideBClk = IDEBCLK_INIT ;
}

/**
 * @brief   Check I2C Bus is idle
 */
static UINT32 
i2cBusIsIdle(
	void
)
{
  SINT32 ret;
  
  ret = HAL_BUSY_WAITING(((i2cBusReg->icsr & ICSR_BUSY_STS) == 0), 30);
  if (ret >= 0) {
  	ret = SP_OK;
  }else{
  	ret = SP_FAIL;	
  }
  return ret;
}

/**
 * @brief   set I2C Bus clock rate
 */
static void
gpHalI2cBusSetClkRate(
	UINT32 clkRate
)
{
    UINT32 tmp = 0;
    UINT32 iccr = 0;
   
	tmp = SYS_AHB_CLK /(clkRate * 2);
   	i2cBusReg->txClkLSB = tmp & 0xFF;
   	iccr = i2cBusReg->iccr;
   	iccr &= ~0xF;
   	iccr |= (tmp & 0xF00) >> 8;
  	i2cBusReg->iccr = iccr;
}

/**
 * @brief   polling delay
 */
static UINT32 
polling_delay(
	UINT32 aAck
)
{
	SINT32 ret;
	
	ret = HAL_BUSY_WAITING((((!(i2cBusReg->iccr & ICCR_INTPREND)) | \
					(i2cBusReg->icsr & ICSR_NONACK & aAck)) == 0), 30);

#if 0
	if (cmd == I2C_BUS_WRITE) {
		ret = HAL_BUSY_WAITING((((i2cBusReg->iccr & ICCR_INTPREND) > 0) && \
					((i2cBusReg->icsr & ICSR_NONACK)) == 0), 30);
	}else{
		ret = HAL_BUSY_WAITING(((i2cBusReg->iccr & ICCR_INTPREND) > 0), 30);
	}
#endif

	if (ret >= 0) {
  		ret = SP_OK;
  	} else {
  		ret = SP_FAIL;	
  	}
	
	return ret;
}

/**
 * @brief   I2C Bus hardware start to transfer data
 * @param   aSlaveAddr [in] slave device address
 * @param   clkRate [in] i2c bus clock rate 
 * @param   cmd [in] 
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 
gpHalI2cBusStartTran(
	UINT32 aSlaveAddr, 
	UINT32 clkRate,
	UINT32 cmd,
	UINT32 aAck
)
{
	UINT32 ctrl = 0;
	UINT32 iccr = 0;
	UINT32 ret = SP_OK;

	ret = i2cBusIsIdle();
	if (ret == SP_FAIL) {
		return ret;
	}
	
	gpHalI2cBusSetClkRate(clkRate);

	iccr = i2cBusReg->iccr;
	iccr &= ICCR_TXCLKMSB_MASK | ICCR_INTREN;
	
	switch(cmd){
	case I2C_BUS_WRITE:
			iccr |= 0x00;
			ctrl = ICSR_MASTER_TX | ICSR_TXRX_ENABLE;
			break;
	case I2C_BUS_READ:
			iccr |= ICCR_ACKEN;
			ctrl = ICSR_MASTER_RX | ICSR_TXRX_ENABLE;
			break;
	default :
			return SP_FAIL;
	}
	
	i2cBusReg->iccr = iccr;
	i2cBusReg->icsr = ctrl;
	
	if (cmd == I2C_BUS_READ) {
		i2cBusReg->idsr = (aSlaveAddr & 0xff) | 0x01;
	} else {
			i2cBusReg->idsr = aSlaveAddr & 0xff;
	}			
	ctrl |= ICSR_START;
	i2cBusReg->icsr = ctrl;
	
	ret = polling_delay(aAck);

	return ret;	
}

/**
 * @brief   I2C Bus hardware be in transfering data
 * @param   aData [in] data buf transfer
 * @param   cmd [in] 
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
UINT32 
gpHalI2cBusMidTran(
	UINT8 *aData , 
	UINT32 cmd,
	UINT32 aAck
)
{
	UINT32 iccr = 0;
	UINT32 ret = SP_OK;

	iccr = i2cBusReg->iccr;
	iccr &= ICCR_TXCLKMSB_MASK | ICCR_INTREN;
	
	switch(cmd){
	case I2C_BUS_WRITE:
			i2cBusReg->idsr = *aData & 0xff;
			iccr |= ICCR_INTPREND;
			break;		
	case I2C_BUS_READ:
			if(aAck == 1){    
				iccr |= ICCR_ACKEN | ICCR_INTPREND;
			}
			else{      
				iccr |= ICCR_INTPREND;
			}
			
			break;	
	default :
			ret = SP_FAIL;
			return ret;
	}
	
	/*clear irq*/
	i2cBusReg->iccr = iccr;	

	ret = polling_delay(aAck);
	if (ret == SP_FAIL) {
		return ret;
	}

	*aData = i2cBusReg->idsr & 0xff;
	
	return ret;
}

/**
 * @brief   I2C Bus hardware stop transfer data
 * @param   cmd [in]
 * @return  SP_OK(0)/SP_FAIL
 * @see
 */
void 
gpHalI2cBusStopTran(
	UINT32 cmd
)
{
	UINT32 ctrl = 0;

	if (cmd == I2C_BUS_WRITE) {
		ctrl = ICSR_MASTER_TX | ICSR_TXRX_ENABLE;
	} else {
		ctrl = ICSR_MASTER_RX | ICSR_TXRX_ENABLE;
	}
	
	i2cBusReg->icsr = ctrl;  /*stop transfer*/
#if 0
	do {
		ctrl = i2cBusReg->icsr;
	} while ((ctrl & ICSR_BUSY_STS));
#endif	
	HAL_BUSY_WAITING(((i2cBusReg->icsr & ICSR_BUSY_STS) == 0), 60);

}

/**
 * @brief   I2c Bus clock enable/disable function
 * @param   enable[in]: disable(0)/enable(1)
 * @return  None
 * @see
 */
void
gpHalI2cBusClkEnable(
	UINT32 enable
)
{
	if (enable) {
		scubReg->scubPeriClkEn |= SCU_B_PERI_I2C;
	} else {
		scubReg->scubPeriClkEn &= ~SCU_B_PERI_I2C;
	}
}


