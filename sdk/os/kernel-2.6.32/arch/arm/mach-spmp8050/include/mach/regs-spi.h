#ifndef _REGS_SPI_H_
#define _REGS_SPI_H_

#define MMP_SSP0_BASE	(IO2_BASE + 0xb08000)
#define SSP_RDR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x00))
#define SSP_TDR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x00))
#define SSP_IER		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x04))
#define SSP_IIR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x08))
#define SSP_FCR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x10))
#define SSP_MCR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x14))
#define SSP_SSR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x18))
#define SSP_FSR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x1C))
#define SSP_DLL		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x20))
#define SSP_DLM		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x24))
#define SSP_LCR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x30))
#define SSP_TSR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x34))
#define SSP_SDR		(*(volatile unsigned int*)(MMP_SSP0_BASE + 0x38))

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
#define		SSIEN					0x10	/*SSI normal/network mode enable*/
#define 	SBLEN					0x08	/*fram sysc type(SSI mode enable)*/
#define 	CONT					0x04	/*clock tyoe(SSI mode only)*/
#define		MODE_SSI_Network		0x03	/**/
#define		MODE_SSI_OnDemand		0x02
#define		MODE_SSI_Normal			0x01
#define		MODE_SPI				0x00










#endif
