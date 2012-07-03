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
 * @file reg_uart.h
 * @brief uart register definition
 * @author zh.l
 */
#ifndef __REG_UART_H__
#define __REG_UART_H__

#include <mach/hardware.h> /*for IO2_ADDRESS*/
#define UART_BASE		IO2_ADDRESS(0xB04000)
#define UART0_BASE		(UART_BASE + 0x0)
#define UART1_BASE		(UART_BASE + 0x1000)
#define UART2_BASE		(UART_BASE + 0x2000)

typedef struct regs_uart_s {
        volatile UINT32 regRBR;	/*+00h DLAB=0 R, receive buffer register*/
#define regTHR        regRBR 	/*+00h DLAB=0 W, transmitter hold register*/
#define regDLL        regRBR	/*+00h DLAB=1 RW, divisor latch LSB*/
        volatile UINT32 regIER;	/*+04h DLAB=0 RW, interrupt enable register*/
#define regDLM        regIER	/*+04h DLAB=1 RW, divisor latch MSB*/
        volatile UINT32 regIIR;	/*+08h R, interrupt identification register*/
#define regFCR        regIIR	/*+08h W, FIFO control register*/
        volatile UINT32 regLCR;	/*+0Ch line control register*/
        volatile UINT32 regMCR;	/*+10h modem control register*/
        volatile UINT32 regLSR;	/*+14h line status register*/
        volatile UINT32 regMSR;	/*+18h modem staus register*/
        volatile UINT32 dummy0;	/*+1Ch */
        volatile UINT32 regFSR;	/*+20h FIFO status register*/
        volatile UINT32 regSPR;	/*+24h sample point register*/
        volatile UINT32 dummy1;	/*+28h*/
        volatile UINT32 dummy2;	/*+2Ch*/
        volatile UINT32 regSIR;	/*+30h slow infrared register*/
}regs_uart_t;

/*-----IER----------------*/
#define 	UART_IER_ERBFI		0x01 /* enable received data available interrupt */
#define 	UART_IER_ETBEI 		0x02 /* enable transmitter holding register empty interrupt */
#define 	UART_IER_ELSI 		0x04 /* enable receiver line status interrupt */
#define 	UART_IER_EDSSI 		0x08 /* enable MODEM status interrupt */

/*-----IIR----------------*/
#define  	UART_IIR_FFES		0x80 /*R, FIFO enable status*/
#define		UART_IIR_ID_NONE	0x02 /*R, no ID*/
#define		UART_IIR_ID_RLS		0x0C /*R, receiver line status*/
#define		UART_IIR_ID_RDA		0x08 /*R, receiver data avalible*/
#define		UART_IIR_ID_CTI		0x08 /*R, character timeout*/
#define		UART_IIR_ID_THRE	0x04 /*R, transmit holding register empty*/
#define		UART_IIR_ID_MS		0x00 /*R, modemo status*/
#define 	UART_IIR_NO_IRQ		0x01 /*R, no interrupt pending*/

/*-----FCR----------------*/
#define		UART_FCR_RFTRG_14	0xC0 /*W, receive FIFO trigger level,14 bytes*/
#define		UART_FCR_RFTRG_8	0x80 /*W, receive FIFO trigger level,8 bytes*/
#define		UART_FCR_RFTRG_4	0x40 /*W, receive FIFO trigger level,4 bytes*/
#define		UART_FCR_RFTRG_1	0x00 /*W, receive FIFO trigger level,1 byte*/
#define		UART_FCR_DMA		0x08 /*W, DMA mode select*/
#define		UART_FCR_TFRST		0x04 /*W, transmit FIFO reset*/
#define		UART_FCR_RFRST		0x02 /*W, receiver FIFO reset*/
#define		UART_FCR_FFE		0x01 /*W, transmit/receive FIFO enable*/

/*-----LCR----------------*/
#define		UART_LCR_DLAB		0x80 /*RW, divisor latch access enable bit*/
#define		UART_LCR_BRK		0x40 /*RW, set break*/
#define		UART_LCR_SPS		0x20 /*RW, set parity bit*/
#define		UART_LCR_EPS		0x10 /*RW, even parity bit*/ 
#define		UART_LCR_PEN		0x08 /*RW, parity enable*/
#define		UART_LCR_STB		0x04 /*RW, stop bit length*/
#define		UART_LCR_WLS8		0x03 /*RW, character length 8bits*/
#define		UART_LCR_WLS7		0x02 /*RW, character length 7bits*/
#define		UART_LCR_WLS6		0x01 /*RW, character length 6bits*/
#define		UART_LCR_WLS5		0x00 /*RW, character length 5bits*/

/*-----MCR--------------*/
#define 	UART_MCR_AFC		0x20 /*RW, auto hardware flow control*/
#define		UART_MCR_LOOP		0x10 /*RW, loopback diagnostic enable*/
#define		UART_MCR_RTS		0x02 /*RW, request to send*/
#define		UART_MCR_DTR		0x01 /*RW, data terminal ready*/

/*-----LSR----------------*/
#define		UART_LSR_RFERR		0x80 /*R, receive FIFO error*/
#define		UART_LSR_TEMT		0x40 /*R, transmitter FIFO empty*/
#define		UART_LSR_THRE		0x20 /*R, transmitter hold register empty*/
#define		UART_LSR_BI		0x10 /*R, break interupt*/
#define		UART_LSR_FE		0x08 /*R, frame error*/
#define		UART_LSR_PE		0x04 /*R, parity error*/
#define		UART_LSR_OE		0x02 /*R, overrun error*/
#define		UART_LSR_DR		0x01 /*R, received data ready*/

/*-----MSR----------------*/
#define		UART_MSR_DCD		0x80 /*R, data carrier dtetected*/
#define		UART_MSR_RI		0x40 /*R, ring indicator*/
#define		UART_MSR_DSR		0x20 /*R, data set ready*/
#define		UART_MSR_CTS		0x10 /*R, clear to send*/
#define		UART_MSR_DDCD		0x08 /*R, data carrier detect has changed*/
#define		UART_MSR_TERI		0x04 /*R, ring indicator has changed from low to high state*/
#define		UART_MSR_DDSR		0x02 /*R, data set read has changed*/
#define		UART_MSR_DCTS		0x01 /*R, clear to send has changed*/

/*-----FSR----------------*/
#define		UART_FSR_RFFUL		0x40 /*R, receive FIFO full*/
#define		UART_FSR_RFHLF		0x20 /*R, receive FIFO half full*/
#define		UART_FSR_RFEMT		0x10 /*R, receive FIFO empty*/
#define		UART_FSR_TFFUL		0x04 /*R, transmit FIFO full*/
#define		UART_FSR_TFHLF		0x02 /*R, transmit FIFO half full*/
#define		UART_FSR_TFEMT		0x01 /*R, transmit FIFO empty*/

#endif
