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
 * @file    hal_apbdma0.c
 * @brief   Implement of APBDMA0 HAL API.
 * @author  Dunker Chen
 */
 
#include <mach/io.h>
#include <mach/module.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_apbdma0.h>
#include <mach/hal/regmap/reg_sd.h>
#include <mach/hal/regmap/reg_spi.h>
#include <mach/hal/regmap/reg_i2c_bus.h>
#include <mach/hal/regmap/reg_ms.h>
#include <mach/hal/hal_apbdma0.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

#define APBDMA_CLK_EN				1
#define APBDMA_CLK_DIS				0

#define APBDMA_M2P					0x00000000
#define APBDMA_P2M					0x00000001
#define APBDMA_AUTO					0x00000000
#define APBDMA_REQ					0x00000002
#define APBDMA_CON					0x00000000
#define APBDMA_FIX					0x00000004
#define APBDMA_SINGLE_BUF			0x00000000
#define APBDMA_DOUBLE_BUF			0x00000008
#define APBDMA_8BIT					0x00000000
#define APBDMA_16BIT				0x00000010
#define APBDMA_32BIT				0x00000020
#define APBDMA_32BIT_BURST			0x00000030
#define APBDMA_IRQOFF				0x00000000
#define APBDMA_IRQON				0x00000040
#define APBDMA_OFF					0x00000000
#define APBDMA_ON					0x00000080	

/**************************************************************************
 *                              M A C R O S                               *
**************************************************************************/
 
#if 1
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

#define DERROR printk 

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
 
static apbdma0Reg_t *g_apb = (apbdma0Reg_t *)LOGI_ADDR_APBDMA0_REG;
 
/**************************************************************************
*             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/

/**
* @brief 	Enable apbdma channel.
* @param 	ch[in]: Apbdma channel number.
* @param 	param[in]: Channel parameter.
* @return	None.
*/
void gpHalApbdmaEn(UINT32 ch, gpApbdma0Param_t *param)
{
	apbdma0chReg_t *apbch = (apbdma0chReg_t *)(LOGI_ADDR_APBDMA0_CH0_REG + (ch<<2));
	sdReg_t 	*sd ;
	spiReg_t	*spi = (spiReg_t*)LOGO_ADDR_SPI_REG;
	i2cBusReg_t	*i2c = (i2cBusReg_t*)LOGI_ADDR_I2C_BUS_REG;	
	msReg_t	*ms = (msReg_t*)LOGI_ADDR_MS_REG;	
	UINT32 ctrl = APBDMA_REQ|APBDMA_FIX|APBDMA_SINGLE_BUF|APBDMA_IRQON|APBDMA_ON;
	UINT32 data_width;
	/* ----- Set module parameter ----- */
	switch(param->module)	
	{
		case SD0:
		case SD1:
			if(param->module==SD0)
				sd = (sdReg_t*)LOGI_ADDR_SD0_REG;
			else
				sd = (sdReg_t*)LOGI_ADDR_SD1_REG;
			if(param->ln0&0xf)
				ctrl  |= APBDMA_32BIT;
			else
				ctrl  |= APBDMA_32BIT_BURST;
			data_width = 4;
			if(param->dir)
				apbch->apbchSA = (UINT32)&sd->sdDatTx;
			else
				apbch->apbchSA = (UINT32)&sd->sdDatRx;
			break;
		case SPI0:
			ctrl  |= APBDMA_8BIT;
			data_width = 1;
			apbch->apbchSA = (UINT32)&spi->spiDATA;
			break;
		case I2C:
			ctrl  |= APBDMA_8BIT;
			data_width = 1;
			apbch->apbchSA = (UINT32)&i2c->icsr;
			break;
		case MS:
			if(param->ln0&0xf)
				ctrl  |= APBDMA_32BIT;
			else
				ctrl  |= APBDMA_32BIT_BURST;
			data_width = 4;
			if(param->dir)
				apbch->apbchSA = (UINT32)&ms->msDataTx;
			else
				apbch->apbchSA = (UINT32)&ms->msDataRx;
			break;
		default:
			return ;	
	}
	/* ----- Set apbdma parameter ----- */
	apbch->apbchSAA = (UINT32)param->buf0;
	apbch->apbchEAA = apbch->apbchSAA + param->ln0 - data_width;
	/* ----- Double buffer mode ----- */
	if(param->buf1&&(param->ln1!=0))
	{
		apbch->apbchSAB = (UINT32)param->buf1;
		apbch->apbchEAB = apbch->apbchSAB + param->ln1 - data_width;
		ctrl |= APBDMA_DOUBLE_BUF;
	}
	/* ----- Set dircetion ----- */
	if(param->dir)
		ctrl |= APBDMA_M2P;
	else
		ctrl |= APBDMA_P2M;
	/* ----- Enable apbdma -----*/
	apbch->apbchCtrl = ctrl;
	
}
EXPORT_SYMBOL(gpHalApbdmaEn);

/**
* @brief 	Apbdma channel reset.
* @param 	ch[in]: channel number.
* @return	None.
*/
void gpHalApbdmaRst(UINT32 ch)
{
	UINT32 setbit = 1<<ch;
	apbdma0chReg_t *apbch = (apbdma0chReg_t *)(LOGI_ADDR_APBDMA0_CH0_REG + (ch<<2));
	/* ----- Disable channel ----- */
	apbch->apbchCtrl = 0;
	/* ----- Set channel reset ----- */
	g_apb->apbRST |= setbit;
	/* ----- Wait reset end ----- */ 
	while(g_apb->apbRST&setbit);
}
EXPORT_SYMBOL(gpHalApbdmaRst);

/**
* @brief	Set apbdma0 buffer.
* @param	ch[in]: Channel number.
* @param	buf_num[in]: Buffer number 0 or 1.
* @param	addr[in]: Buffer start address.
* @param	ln[in]: Buffer size.
* @return: 	None.
*/
void gpHalApbdmaSetBuf(UINT32 ch, UINT32 buf_num, UINT8* addr, UINT32 ln)
{
	apbdma0chReg_t *apbch = (apbdma0chReg_t *)(LOGI_ADDR_APBDMA0_CH0_REG + (ch<<2));
	UINT8 data_width = 1<<((apbch->apbchCtrl&0x30)>>2);
	
	if(data_width>4)
		data_width =4;
	if(buf_num)	
	{
		apbch->apbchSAB = (UINT32)addr;
		apbch->apbchEAB = (UINT32)(addr+ln-data_width);
	}
	else
	{
		apbch->apbchSAA = (UINT32)addr;
		apbch->apbchEAA = (UINT32)(addr+ln-data_width);	
	}
}
EXPORT_SYMBOL(gpHalApbdmaSetBuf);

/**
* @brief	Check apbdma0 status.
* @param	ch[in]: Channel number.
* @return: 	0 = idle, 1 = busy.
*/
UINT32 gpHalApbdmaChStatus (UINT32 ch)
{
	UINT32 ckbit = 1<<ch;
	return 	(g_apb->apbStatus&ckbit)?1:0;
}
EXPORT_SYMBOL(gpHalApbdmaChStatus);

/**
* @brief	Check which apbdma0 buffer in use.
* @param	ch[in]: Channel number.
* @return: 	0 = buffer 0, 1 = buffer 1.
*/
UINT32 gpHalApbdmaBufStatus (UINT32 ch)
{
	UINT32 ckbit = 0x100<<ch;
	return 	(g_apb->apbStatus&ckbit)?1:0;		
}
EXPORT_SYMBOL(gpHalApbdmaBufStatus);

/**
* @brief	Clear channel IRQ status.
* @param	ch[in]: Channel number.
* @return: 	None.
*/
void gpHalApbdmaClearIRQFlag (UINT32 ch)
{
	UINT32 clrbit = 1<<ch;
	g_apb->apbINT = clrbit;
}
EXPORT_SYMBOL(gpHalApbdmaClearIRQFlag);

/**
* @brief 	Apbdma0 hardware reset function.
* @return	None.
*/
void gpHalApbdmaHWRst(void)
{
	scucReg_t* scuc = (scucReg_t*)LOGI_ADDR_SCU_C_REG;
	/* ----- Enable SDMA channel reset ----- */
	scuc->scucPeriRst |= 0x80;
	/* ----- Disable SDMA channel reset ----- */
	scuc->scucPeriRst &= ~0x80;	
}
EXPORT_SYMBOL(gpHalApbdmaHWRst);
