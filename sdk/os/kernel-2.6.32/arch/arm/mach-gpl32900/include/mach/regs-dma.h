/*
 * arch/arm/mach-spmp8000/include/mach/regs-rtc.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * RTC - System peripherals regsters.
 *
 */


#define APBDMAA_BASE			IO3_ADDRESS(0x010000)
#define APBDMAC_BASE			IO2_ADDRESS(0xB00000)

#define PA_APBDMAA_BASE			(0x93010000)
#define PA_APBDMAC_BASE			(0x92B00000)
#define APBDMA_SIZE			0x80
#if 0
#define DMAA_BUSY_STATUS (*(volatile unsigned *) (APBDMAA_BASE + 0x00))
#define DMAA_IRQ_STATUS  (*(volatile unsigned *) (APBDMAA_BASE + 0x04))

#define DMAA_AHB_SA1A	(*(volatile unsigned *) (APBDMAA_BASE + 0x08))
#define DMAA_AHB_SA2A	(*(volatile unsigned *) (APBDMAA_BASE + 0x0C))
#define DMAA_AHB_SA3A	(*(volatile unsigned *) (APBDMAA_BASE + 0x10))
#define DMAA_AHB_SA4A	(*(volatile unsigned *) (APBDMAA_BASE + 0x14))

#define DMAA_AHB_EA1A	(*(volatile unsigned *) (APBDMAA_BASE + 0x18))
#define DMAA_AHB_EA2A	(*(volatile unsigned *) (APBDMAA_BASE + 0x1C))
#define DMAA_AHB_EA3A	(*(volatile unsigned *) (APBDMAA_BASE + 0x20))
#define DMAA_AHB_EA4A	(*(volatile unsigned *) (APBDMAA_BASE + 0x24))

#define DMAA_APB_SA1	(*(volatile unsigned *) (APBDMAA_BASE + 0x28))
#define DMAA_APB_SA2	(*(volatile unsigned *) (APBDMAA_BASE + 0x2C))
#define DMAA_APB_SA3	(*(volatile unsigned *) (APBDMAA_BASE + 0x30))
#define DMAA_APB_SA4	(*(volatile unsigned *) (APBDMAA_BASE + 0x34))

#define DMAA_AHB_SA1B	(*(volatile unsigned *) (APBDMAA_BASE + 0x4C))
#define DMAA_AHB_SA2B	(*(volatile unsigned *) (APBDMAA_BASE + 0x50))
#define DMAA_AHB_SA3B	(*(volatile unsigned *) (APBDMAA_BASE + 0x54))
#define DMAA_AHB_SA4B	(*(volatile unsigned *) (APBDMAA_BASE + 0x58))

#define DMAA_AHB_EA1B	(*(volatile unsigned *) (APBDMAA_BASE + 0x5C))
#define DMAA_AHB_EA2B	(*(volatile unsigned *) (APBDMAA_BASE + 0x60))
#define DMAA_AHB_EA3B	(*(volatile unsigned *) (APBDMAA_BASE + 0x64))
#define DMAA_AHB_EA4B	(*(volatile unsigned *) (APBDMAA_BASE + 0x68))

#define DMAA_CR1		(*(volatile unsigned *) (APBDMAA_BASE + 0x6C))
#define DMAA_CR2		(*(volatile unsigned *) (APBDMAA_BASE + 0x70))
#define DMAA_CR3		(*(volatile unsigned *) (APBDMAA_BASE + 0x74))
#define DMAA_CR4		(*(volatile unsigned *) (APBDMAA_BASE + 0x78))
#define DMAA_RST		(*(volatile unsigned *) (APBDMAA_BASE + 0x7C))


/* APBDMAC register */
#define DMAC_BUSY_STATUS (*(volatile unsigned *) (APBDMAC_BASE + 0x00))
#define DMAC_IRQ_STATUS  (*(volatile unsigned *) (APBDMAC_BASE + 0x04))

#define DMAC_AHB_SA1A	(*(volatile unsigned *) (APBDMAC_BASE + 0x08))
#define DMAC_AHB_SA2A	(*(volatile unsigned *) (APBDMAC_BASE + 0x0C))
#define DMAC_AHB_SA3A	(*(volatile unsigned *) (APBDMAC_BASE + 0x10))
#define DMAC_AHB_SA4A	(*(volatile unsigned *) (APBDMAC_BASE + 0x14))

#define DMAC_AHB_EA1A	(*(volatile unsigned *) (APBDMAC_BASE + 0x18))
#define DMAC_AHB_EA2A	(*(volatile unsigned *) (APBDMAC_BASE + 0x1C))
#define DMAC_AHB_EA3A	(*(volatile unsigned *) (APBDMAC_BASE + 0x20))
#define DMAC_AHB_EA4A	(*(volatile unsigned *) (APBDMAC_BASE + 0x24))

#define DMAC_APB_SA1	(*(volatile unsigned *) (APBDMAC_BASE + 0x28))
#define DMAC_APB_SA2	(*(volatile unsigned *) (APBDMAC_BASE + 0x2C))
#define DMAC_APB_SA3	(*(volatile unsigned *) (APBDMAC_BASE + 0x30))
#define DMAC_APB_SA4	(*(volatile unsigned *) (APBDMAC_BASE + 0x34))

#define DMAC_AHB_SA1B	(*(volatile unsigned *) (APBDMAC_BASE + 0x4C))
#define DMAC_AHB_SA2B	(*(volatile unsigned *) (APBDMAC_BASE + 0x50))
#define DMAC_AHB_SA3B	(*(volatile unsigned *) (APBDMAC_BASE + 0x54))
#define DMAC_AHB_SA4B	(*(volatile unsigned *) (APBDMAC_BASE + 0x58))

#define DMAC_AHB_EA1B	(*(volatile unsigned *) (APBDMAC_BASE + 0x5C))
#define DMAC_AHB_EA2B	(*(volatile unsigned *) (APBDMAC_BASE + 0x60))
#define DMAC_AHB_EA3B	(*(volatile unsigned *) (APBDMAC_BASE + 0x64))
#define DMAC_AHB_EA4B	(*(volatile unsigned *) (APBDMAC_BASE + 0x68))

#define DMAC_CR1		(*(volatile unsigned *) (APBDMAC_BASE + 0x6C))
#define DMAC_CR2		(*(volatile unsigned *) (APBDMAC_BASE + 0x70))
#define DMAC_CR3		(*(volatile unsigned *) (APBDMAC_BASE + 0x74))
#define DMAC_CR4		(*(volatile unsigned *) (APBDMAC_BASE + 0x78))
#define DMAC_RST		(*(volatile unsigned *) (APBDMAC_BASE + 0x7C))
#endif


#define DMAX_BUSY_STATUS 0x00
#define DMAX_IRQ_STATUS  0x04
#define DMAX_AHB_SAXA    0x08  //0x08~0x14
#define DMAX_AHB_EAXA    0x18  //0x18~0x24
#define DMAX_APB_SAX     0x28  //0x28~0x34
#define DMAX_AHB_SAXB    0x4C  //0x4C~0x58
#define DMAX_AHB_EAXB    0x5C  //0x5C~0x68
#define DMAX_CRX         0x6C  //0x6C~0x78
#define DMAX_RST         0x7C


// APB DMA
#define DMA_M2P			0x00000000   //MIU to APB
#define DMA_P2M			0x00000001   //APB to MIU

#define DMA_AUTO		0x00000000   //AUTO ENABLE
#define DMA_REQ			0x00000002   //REQ ENABLE 

#define DMA_CON			0x00000000   //Address mode = CONTINUE
#define DMA_FIX			0x00000004   //Address mode = FIX

#define DMA_SINGLE_BUF	0x00000000   //Single Buffer
#define DMA_DOUBLE_BUF	0x00000008   //Nulti Buffer

#define DMA_8BIT		0x00000000   //8-bits single transfer
#define DMA_16BIT		0x00000010   //16-bits single transfer
#define DMA_32BIT		0x00000020   //32-bits single transfer
#define DMA_32BIT_BURST	0x00000030   //32-bits burst transfer

#define DMA_IRQOFF		0x00000000
#define DMA_IRQON		0x00000040

#define DMA_OFF			0x00000000
#define DMA_ON			0x00000080


