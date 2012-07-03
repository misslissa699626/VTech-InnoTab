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
 *                                                                        *
 **************************************************************************/
#ifndef _REG_ADC_H_
#define _REG_ADC_H_

#include <mach/hardware.h>	/*IO3_ADDRESS*/
#include <mach/typedef.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define	LOGI_ADDR_ADC_REG		IO3_ADDRESS(0x1F000)

//SARADC_SARCTRL
#define ADC_CTL_AUTO_CON_ON	(0x01)	/*auto conversion mode on*/
#define ADC_CTL_MAN_CON_ON	(0x02)	/*manual conversion mode on*/

#define ADC_CTL_BUSY_OFST     21
#define ADC_CTL_MODE_OFST     18
#define ADC_CTL_REF_OFST      17
#define ADC_CTL_TOGEN_OFST    16
#define ADC_CTL_DIVNUM_OFST   8
#define ADC_CTL_BACK2INT_OFST 7
#define ADC_CTL_TPS_OFST      5
#define ADC_CTL_SARS_OFST     2
#define ADC_CTL_MANCON_OFST   1
#define ADC_CTL_AUTOCON_OFST  0

#define ADC_CTL_BUSY_MASK     0x1
#define ADC_CTL_MODE_MASK     0x7
#define ADC_CTL_REF_MASK      0x1
#define ADC_CTL_TOGEN_MASK    0x1
#define ADC_CTL_DIVNUM_MASK   0xFF
#define ADC_CTL_BACK2INT_MASK 0x1
#define ADC_CTL_TPS_MASK      0x3
#define ADC_CTL_SARS_MASK     0x7
#define ADC_CTL_MANCON_MASK   0x1
#define ADC_CTL_AUTOCON_MASK  0x1

/*SARADC_CONDLY*/
#define ADC_CONDLY_OFST   0
#define ADC_CONDLY_MASK   0xFF

/*SARADC_AUTODLY*/
#define ADC_INTDLY_OFST   0
#define ADC_INTDLY_MASK   0x1F
#define ADC_X2YDLY_OFST   5
#define ADC_X2YDLY_MASK   0x3F

/*SARADC_DEBTIME*/
#define ADC_CHKDLY_OFST   0
#define ADC_CHKDLY_MASK   0xFFFF
#define ADC_DEBDLY_OFST   16
#define ADC_DEBDLY_MASK   0xFF

/*SARADC_PNL*/
#define ADC_PNLY_OFST   0
#define ADC_PNLY_MASK   0xFFFF
#define ADC_PNLX_OFST   16
#define ADC_PNLX_MASK   0xFFFF

/*SARADC_INTEN/INTF*/
#define ADC_INTAUX     0x08
#define ADC_INTPNL     0x04
#define ADC_INTPENUP   0x02
#define ADC_INTPENDN   0x01

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct regADC_s {
	volatile UINT32	SARCTRL;	/** +00 SARADC control register*/
	volatile UINT32	CONDLY;		/** conversion delay time*/
	volatile UINT32	AUTODLY;	/** auto conversion mode x-y delay and sample interval*/
	volatile UINT32	DEBTIME;	/** pen down interrupt de-bounce time*/
	volatile UINT32	PNL;		/** touch panel x/y position*/
	volatile UINT32	AUX;		/** auxiliary measurement data*/
	volatile UINT32	INTEN;		/** interrupt enable*/
	volatile UINT32	INTF;		/** interrupt flags*/
}regADC_t;
#endif /*_REG_ADC_H_*/


