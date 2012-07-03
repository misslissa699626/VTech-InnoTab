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
 
/**
 * @file reg_spi.h
 * @brief spi register define 
 * @author zaimingmeng
 */

#ifndef _REG_SPI_H_
#define _REG_SPI_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define LOGO_ADDR_SPI_REG		(IO2_BASE + 0xb08000)

/* interrupt enable register define */
#define 	OURNE 		0x08	/*fifo overrun or underrun interrupt enable*/
#define 	RFTGE 		0x04	/*RCVR fifo trigger level reach interrupt enable*/
#define 	RDRRE 		0x02	/*reveive data register ready interrupt enable*/
#define 	TDREE 		0x01	/*transmit data register empty interrupt enable*/


/* interrupt identification register define */
#define		OURN 		0x08	/*fifo overrun or underrun interrupt */
#define		RFTG 		0x04	/*RCVR fifo trigger level reach interrupt */
#define  	RDRR 		0x02	/*reveive data register ready interrupt */
#define  	TDRE		0x01	/*transmit data register empty interrupt */

/*fifo control regsiter setting*/
#define		RFTRG_7		0xC0
#define		RFTRG_5		0x80
#define		RFTRG_3		0x40
#define		RFTRG_1		0x00	/*RCVR fifo trigger level*/
#define 	DMA		0x08	/*DMA mode select*/
#define		TFRST		0x04	/*XMIT fifo reset*/
#define 	RFRST		0x02	/*RCVR fifo reset*/
#define		FFE		0x01	/*XMIT/RCVR fifo enable*/


/*mode control register setting*/
#define 	LOOP		0x10	/*provide a local loopback feature for diagnostic testing*/
#define 	CPOL		0x08	/*clock polarity(SPI mode only) :0:high clock(SCLK idels at low state) */
#define 	CPHA		0x04	/*clock phase(SPI mode only) :0:sampling of data occur at odd edge of SCLK clock*/
#define 	MnSS		0x02	/*manual slave select assertion*/
#define 	LSBF		0x01	/**/

/*slave select register setting*/
#define 	nSS3E		0x80
#define 	nSS2E		0x40
#define 	nSS1E		0x20
#define 	nSS0E		0x10
#define 	MnSS3		0x08
#define 	MnSS2		0x04
#define 	MnSS1		0x02
#define 	MnSS0		0x01

/*fifo status register*/
#define    	RFORN		0x80	/*RCVR fifo overrun*/
#define    	RFURN		0x40	/*RCVR fifo underrun*/
#define    	RFFUL		0x20	/*RCVR fifo full*/
#define    	RFEMT		0x10	/*RCVR fifo empty*/
#define    	TFORN		0x08	/*XMIT fifo overrun*/
#define    	TFURN		0x04	/*XMIT fifo underrun*/
#define    	TFFUL		0x02	/*XMIT fifo full*/
#define    	TFEMT		0x01	/*XMIT fifo empty*/


/*line control register setting*/
#define		SSIEN				0x10	/*SSI normal/network mode enable*/
#define 	SBLEN				0x08	/*fram sysc type(SSI mode enable)*/
#define 	CONT				0x04	/*clock tyoe(SSI mode only)*/
#define		MODE_SSI_Network		0x03	/**/
#define		MODE_SSI_OnDemand		0x02
#define		MODE_SSI_Normal			0x01
#define		MODE_SPI			0x00

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct spiReg_s{
	volatile UINT32 spiDATA;	/* 0x0000 ~ 0x0003 receive/transmit data register*/
	volatile UINT32 spiIER;		/* 0x0004 ~ 0x0007 interrupt enable register*/
	volatile UINT32 spiIIR;		/* 0x0008 ~ 0x000b interrupt identification register*/
	volatile UINT32 NULL_I;
	volatile UINT32 spiFCR;		/* 0x0010 ~ 0x0013 fifo control register*/
	volatile UINT32 spiMCR;		/* 0x0014 ~ 0x0017 mode control register*/
	volatile UINT32 spiSSR;		/* 0x0018 ~ 0x001b slave select register*/
	volatile UINT32 spiFSR;		/* 0x001c ~ 0x001f fifo status register*/
	volatile UINT32 spiDLL;		/* 0x0020 ~ 0x0023 divisor latch LSB*/
	volatile UINT32 spiDLM;		/* 0x0024 ~ 0x0027 divisor latch MSB*/
	
	volatile UINT32 NULL_II;
	volatile UINT32 NULL_III;
	
	volatile UINT32 ssiLCR;		/* 0x0030 ~ 0x0033 line control register*/
	volatile UINT32 ssiTSR;		/* 0x0034 ~ 0x0037 time slot register*/
	volatile UINT32 ssiSDR;		/* 0x0038 ~ 0x003b frame sysc divisor register*/

}spiReg_t;


#endif	/*_REG_SPI_H_*/
