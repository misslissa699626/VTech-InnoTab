/*
 * arch/arm/mach-spmp8000/include/mach/regs-uart.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * UART - System peripherals regsters.
 *
 */



#define UART_BASE			IO2_ADDRESS(0xB04000)
#define UART0_BASE			(UART_BASE + 0x0)
#define UART1_BASE			(UART_BASE + 0x1000)
#define UART2_BASE			(UART_BASE + 0x2000)

/* Timer 0 register */
#define UART0_DLL	(*(volatile unsigned *) (UART0_BASE + 0x0))
#define UART0_DLM	(*(volatile unsigned *) (UART0_BASE + 0x4))
#define UART0_RBR	(*(volatile unsigned *) (UART0_BASE + 0x0))
#define UART0_THR	(*(volatile unsigned *) (UART0_BASE + 0x0))
#define UART0_IER	(*(volatile unsigned *) (UART0_BASE + 0x4))
#define UART0_IIR	(*(volatile unsigned *) (UART0_BASE + 0x8))
#define UART0_FCR	(*(volatile unsigned *) (UART0_BASE + 0x8))
#define UART0_LCR	(*(volatile unsigned *) (UART0_BASE + 0xC))
#define UART0_MCR	(*(volatile unsigned *) (UART0_BASE + 0x10))
#define UART0_LSR	(*(volatile unsigned *) (UART0_BASE + 0x14))
#define UART0_MSR	(*(volatile unsigned *) (UART0_BASE + 0x18))
#define UART0_FSR	(*(volatile unsigned *) (UART0_BASE + 0x20))
#define UART0_SPR	(*(volatile unsigned *) (UART0_BASE + 0x24))
#define UART0_SIR	(*(volatile unsigned *) (UART0_BASE + 0x30))


/* Timer 1 register */
#define UART1_DLL	(*(volatile unsigned *) (UART1_BASE + 0x0))
#define UART1_DLM	(*(volatile unsigned *) (UART1_BASE + 0x4))
#define UART1_RBR	(*(volatile unsigned *) (UART0_BASE + 0x0))
#define UART1_THR	(*(volatile unsigned *) (UART1_BASE + 0x0))
#define UART1_IER	(*(volatile unsigned *) (UART1_BASE + 0x4))
#define UART1_IER	(*(volatile unsigned *) (UART1_BASE + 0x4))
#define UART1_IIR	(*(volatile unsigned *) (UART1_BASE + 0x8))
#define UART1_FCR	(*(volatile unsigned *) (UART1_BASE + 0x8))
#define UART1_LCR	(*(volatile unsigned *) (UART1_BASE + 0xC))
#define UART1_MCR	(*(volatile unsigned *) (UART1_BASE + 0x10))
#define UART1_LSR	(*(volatile unsigned *) (UART1_BASE + 0x14))
#define UART1_MSR	(*(volatile unsigned *) (UART1_BASE + 0x18))
#define UART1_FSR	(*(volatile unsigned *) (UART1_BASE + 0x20))
#define UART1_SPR	(*(volatile unsigned *) (UART1_BASE + 0x24))
#define UART1_SIR	(*(volatile unsigned *) (UART1_BASE + 0x30))


/* Timer 2 register */
#define UART2_DLL	(*(volatile unsigned *) (UART2_BASE + 0x0))
#define UART2_DLM	(*(volatile unsigned *) (UART2_BASE + 0x4))
#define UART2_RBR	(*(volatile unsigned *) (UART0_BASE + 0x0))
#define UART2_THR	(*(volatile unsigned *) (UART2_BASE + 0x0))
#define UART2_IER	(*(volatile unsigned *) (UART2_BASE + 0x4))
#define UART2_IER	(*(volatile unsigned *) (UART2_BASE + 0x4))
#define UART2_IIR	(*(volatile unsigned *) (UART2_BASE + 0x8))
#define UART2_FCR	(*(volatile unsigned *) (UART2_BASE + 0x8))
#define UART2_LCR	(*(volatile unsigned *) (UART2_BASE + 0xC))
#define UART2_MCR	(*(volatile unsigned *) (UART2_BASE + 0x10))
#define UART2_LSR	(*(volatile unsigned *) (UART2_BASE + 0x14))
#define UART2_MSR	(*(volatile unsigned *) (UART2_BASE + 0x18))
#define UART2_FSR	(*(volatile unsigned *) (UART2_BASE + 0x20))
#define UART2_SPR	(*(volatile unsigned *) (UART2_BASE + 0x24))
#define UART2_SIR	(*(volatile unsigned *) (UART2_BASE + 0x30))


/*-----IER----------------*/
#define 	UART_ERBFI		0x01 // enable received data available interrupt
#define 	UART_ETBEI 		0x02 // enable transmitter holding register empty interrupt
#define 	UART_ELSI 		0x04 // enable receiver line status interrupt
#define 	UART_EDSSI 		0x08 // enable MODEM status interrupt

/*-----IIR----------------*/
#define  	UART_FFES		0x80
#define		UART_ID_NONE	0x02
#define		UART_ID_RLS		0x0C
#define		UART_ID_RDA		0x08
#define		UART_ID_CTI		0x08
#define		UART_ID_THRE	0x04
#define		UART_ID_MS		0x00
#define 	UART_IRQ		0x01

/*-----FCR----------------*/
#define     UART_RFTRG_14Byte	0xC0
#define     UART_RFTRG_8Byte	0x80
#define     UART_RFTRG_4Byte	0x40
#define     UART_RFTRG_1Byte	0x00
#define		UART_DMA			0x08
#define		UART_TFRST			0x04
#define		UART_RFRST			0x02
#define		UART_FFE			0x01

/*-----LCR----------------*/
#define		UART_DLAB		0x80
#define		UART_BRK		0x40
#define		UART_SPS		0x20
#define		UART_EPS		0x10 
#define		UART_PEN		0x08
#define		UART_STB		0x04
#define		UART_WLS_8		0x03
#define		UART_WLS_7		0x02
#define		UART_WLS_6		0x01
#define		UART_WLS_5		0x00

/*-----MCR--UART_--------------*/
#define 	UART_AFC		0x20
#define		UART_LOOP		0x10
#define		UART_RTS		0x02
#define		UART_DTR		0x01

/*-----LSR----------------*/
#define		UART_RFERR		0x80 // FIFO mode Only
#define		UART_TEMT		0x40
#define		UART_THRE		0x20
#define		UART_BI			0x10
#define		UART_FE			0x08
#define		UART_PE			0x04
#define		UART_OE			0x02
#define		UART_DR			0x01

/*-----MSR----------------*/
#define		UART_DCD		0x80
#define		UART_RI			0x40
#define		UART_DSR		0x20
#define		UART_CTS		0x10
#define		UART_DDCD		0x08
#define		UART_TERI		0x04
#define		UART_DDSR		0x02
#define		UART_DCTS		0x01

/*-----FSR----------------*/
#define		UART_RFFUL		0x40
#define		UART_RFHLF		0x20
#define		UART_RFEMT		0x10
#define		UART_TFFUL		0x04
#define		UART_TFHLF		0x02
#define		UART_TFEMT		0x01

