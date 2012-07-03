/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *  Aii, SZ, add SPI1 setting. @2011.2.24                                 *
 *                                                                        *
 **************************************************************************/
 
/**
 * @file hal_spi1.c
 * @brief spi HAL interface 
 * @author zaimingmeng
 */

#include <mach/kernel.h>
#include <linux/clk.h>
#include <mach/hal/regmap/reg_spi.h>
#include <mach/hal/hal_common.h>
#include <mach/hal/hal_clock.h>
#include <mach/hal/regmap/reg_scu.h>

/**************************************************************************
								SPI1 setting
***************************************************************************/
/**************************************************************************
 *                              CONSTANTS                               *
 **************************************************************************/
#define HAL_SPI_MACRO_RW_TIMEOUT	(10)	/*in ms*/

/**
 * @brief get spi base clock
 * @return spi base clock value
 */
static unsigned long getCLK1(void)
{
	unsigned long clk_rate = 0;
	struct clk *pclk;

	pclk = clk_get(NULL,"clk_sys_apb");

	if(pclk){
		clk_rate = clk_get_rate(pclk);	
		clk_put(pclk);
	}
	else{
		clk_rate = 37000000;
	}

	return clk_rate;
}

/**
 * @brief read rx fifo state, and clean rx fifo
 * @return 1: fifo empty; 0: fifo have data
 */
static int read_fifo1(void)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	int status;

	status = pspiReg->spiFSR;
	status &= RFEMT;
	if(status == 0){
		status = pspiReg->spiDATA;
		return (pspiReg->spiFSR&RFEMT);
	}
	else{
		return 1;
	}
	
}

/*
 * HAL interface
 */


/**
 * @brief the spi init function
 */
static void gpHalSpiInit1(void)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	//scubReg_t *pscubReg = (scubReg_t *)(LOGI_ADDR_SCU_A_REG);
	int *reg_spi1 = (int*)LOGO_ADDR_SPI1_IOREG;
	int temp;

	//pscubReg->scubGpio0PinEn |= 0x40;      //enable GPIO0[6] input  of SPI_CH0_DI
    *reg_spi1 |= 0xe0000000;
	pspiReg->spiFCR = RFTRG_7|FFE|RFRST|TFRST;

	pspiReg->ssiLCR = MODE_SPI1;


	pspiReg->spiMCR = MnSS;

	pspiReg->spiDLL = 0;
	pspiReg->spiDLM = 0;

	temp = pspiReg->spiSSR;

	pspiReg->spiIIR = 0xf;

}

/**
 * @brief the spi init function
 * @param enable [in] 0: disable; 1: enable
 */
void gpHalSpiClkEnable1(void* devinfo, int enable)
{

	if(enable){
		//gpSpiClkEnable(devinfo,1);
		gpHalScuClkEnable(1<<17, 2, 1);
		gpHalSpiInit1();
	}
	else{
		//gpSpiClkEnable(devinfo,0);
		gpHalScuClkEnable(1<<17, 2, 0);
	}
}

/**
 * @brief the spi init function
 * @param freq [in] freqence value, 0 max freqence
 */
void gpHalSpiSetFreq1(UINT32  freq)
{
	UINT32 count = 0;
	UINT32 temp;
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;

	if(freq){
	
		temp = getCLK1();
		if(temp > 2*freq){
			count = temp/(2*freq) - 1;
		}

		pspiReg->spiDLL = count&0xff;
		pspiReg->spiDLM = count>>8;
	}
	else{
		pspiReg->spiDLL = 0;
		pspiReg->spiDLM = 0;
	}	
}


/**
 * @brief the cs enable function
 * @param id [in] channnel index
 */
void gpHalSpiCsEnable1(int id)
{
	unsigned int temp;
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;

	if(id > 3)
		return;

	temp = pspiReg->spiSSR;
	temp &= ~(1<<id);

	pspiReg->spiSSR = temp;
}

/**
 * @brief the cs disable function
 * @param id [in] channnel index
 */
void gpHalSpiCsDisable1(int id)
{
	unsigned int temp;
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;

	if(id > 3)
		return;

	temp = pspiReg->spiSSR;
	temp |= (1<<id);

	pspiReg->spiSSR = temp;

}

/**
 * @brief the spi DAM control function
 * @param enable [in] 0:disable; 1:enable
 */
void gpHalSpiDmaCtrl1(int enable)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	int temp;

	temp = pspiReg->spiFCR;

	if(enable){
		pspiReg->spiFCR = temp|DMA;
	}
	else{
		pspiReg->spiFCR = temp&(~DMA);
	}
}

/**
 * @brief the spi clock polarity state in slck idles set function
 * @param status [in] 0:SCLK idles at low state; 1:SCLK idles at high state
 */
void gpHalSpiClkPol1(int status)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	int temp;

	temp = pspiReg->spiMCR;

	if(status){
		pspiReg->spiMCR = temp|CPOL;
	}
	else{
		pspiReg->spiMCR = temp&(~CPOL);
	}
}

/**
 * @brief the spi clock phase state set function
 * @param status [in] 0:data occur at odd edge of SCLK clock ; 1:data occur at even edge of SCLK clock 
 */
void gpHalSpiClkPha1(int status)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	int temp;

	temp = pspiReg->spiMCR;

	if(status){
		pspiReg->spiMCR = temp|CPHA;
	}
	else{
		pspiReg->spiMCR = temp&(~CPHA);
	}
}

/**
 * @brief the spi LSB/MSB first set function
 * @param status [in] 0:MSB first ; 1:LSB first
 */
void gpHalSpiSetLsb1(int status)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	int temp;

	temp = pspiReg->spiMCR;

	if(status){
		pspiReg->spiMCR = temp|LSBF;
	}
	else{
		pspiReg->spiMCR = temp&(~LSBF);
	}
}

/**
 * @brief set interrup enable register function
 * @param mode [in] TDREE|RDRRE|RFTGE|OURNE
 */
void gpHalSpiSetIER1(int mode)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;

	pspiReg->spiIER = mode;
}

/**
 * @brief get interrup state register function
 * @return TDREE|RDRRE|RFTGE|OURNE
 */
int gpHalSpiGetIIR1(void)
{
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;

	return pspiReg->spiIIR;
}

/**
 * @brief the spi init function
 * @param buffer [in] write buffer data
 * @param len [in] write data len
 */
void gpHalSpiWrite1(char *buffer,int len)
{
	int tx_num = 0;
	int rx_num = 0;
	UINT8 status;
	int temp;
	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;

	while(rx_num<len)
	{
		/*read fifo status*/
		status = pspiReg->spiFSR;

		/*Tx not full*/
		if(!(status&TFFUL)){

			if(tx_num < len){
				pspiReg->spiDATA = buffer[tx_num];
				tx_num++;
			}
		}

		/*Rx have data*/
		if(!(status&RFEMT)){
			if(rx_num < len){
				temp = pspiReg->spiDATA;		
				rx_num++;

			}
		}
	}

	HAL_BUSY_WAITING(read_fifo1(), HAL_SPI_MACRO_RW_TIMEOUT);
	
	HAL_BUSY_WAITING((pspiReg->spiFSR&TFEMT), HAL_SPI_MACRO_RW_TIMEOUT);

}

/**
 * @brief the spi init function
 * @param buffer [out] read buffer data
 * @param len [in] read data len
 */
void gpHalSpiRead1(char *buffer,int len)
{
	int tx_num = 0;
	int rx_num = 0;
	UINT8 status = 0;

	spiReg_t *pspiReg = (spiReg_t *)LOGO_ADDR_SPI1_REG;
	//printk("gpHalSpiRead &pspiReg=0x%x\n",pspiReg);

	while(rx_num<len)
	{
		/*read fifo status*/
		status = pspiReg->spiFSR;
	
		/*Tx not full*/
		if(!(status&TFFUL)){

			if(tx_num < len){
				pspiReg->spiDATA = 0xff;
				tx_num++;

			}
		}

		/*Rx have data*/
		if(!(status&RFEMT)){
			if(rx_num < len){
				buffer[rx_num] = pspiReg->spiDATA;		
				rx_num++;

			}
		}
	}

	HAL_BUSY_WAITING(read_fifo1(), HAL_SPI_MACRO_RW_TIMEOUT);
	
	HAL_BUSY_WAITING((pspiReg->spiFSR&TFEMT), HAL_SPI_MACRO_RW_TIMEOUT);

}



