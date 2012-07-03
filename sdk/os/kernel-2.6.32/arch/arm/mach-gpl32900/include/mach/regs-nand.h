/*
 * arch/arm/mach-spmp8000/include/mach/regs-nand.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * RTC - System peripherals regsters.
 *
 */



#define NAND0_BASE			IO3_ADDRESS(0x8000)
#define NAND1_BASE			IO3_ADDRESS(0x9000)

#define NAND0_CSR	    (*(volatile unsigned *) (NAND0_BASE + 0x00))
#define NAND0_DESC_BA	(*(volatile unsigned *) (NAND0_BASE + 0x04))
#define NAND0_AC_TIMING	(*(volatile unsigned *) (NAND0_BASE + 0x08))
#define NAND0_RDYBSY_EN	(*(volatile unsigned *) (NAND0_BASE + 0x0c))
#define NAND0_PIO_CTRL1	(*(volatile unsigned *) (NAND0_BASE + 0x10))
#define NAND0_PIO_CTRL2	(*(volatile unsigned *) (NAND0_BASE + 0x14))
#define NAND0_PIO_CTRL3	(*(volatile unsigned *) (NAND0_BASE + 0x18))
#define NAND0_PIO_CTRL4	(*(volatile unsigned *) (NAND0_BASE + 0x1c))
#define NAND0_PIO_CTRL5	(*(volatile unsigned *) (NAND0_BASE + 0x20))
#define NAND0_PIO_CTRL6	(*(volatile unsigned *) (NAND0_BASE + 0x24))
#define NAND0_PIO_CTRL7	(*(volatile unsigned *) (NAND0_BASE + 0x28))
#define NAND0_PIO_CTRL8	(*(volatile unsigned *) (NAND0_BASE + 0x2c))
#define NAND0_PIO_INIRMSK  (*(volatile unsigned *) (NAND0_BASE + 0x40))
#define NAND0_PIO_INTR_STS (*(volatile unsigned *) (NAND0_BASE + 0x44))

#define NAND1_CSR	    (*(volatile unsigned *) (NAND1_BASE + 0x00))
#define NAND1_DESC_BA	(*(volatile unsigned *) (NAND1_BASE + 0x04))
#define NAND1_AC_TIMING	(*(volatile unsigned *) (NAND1_BASE + 0x08))
#define NAND1_RDYBSY_EN	(*(volatile unsigned *) (NAND1_BASE + 0x0c))
#define NAND1_PIO_CTRL1	(*(volatile unsigned *) (NAND1_BASE + 0x10))
#define NAND1_PIO_CTRL2	(*(volatile unsigned *) (NAND1_BASE + 0x14))
#define NAND1_PIO_CTRL3	(*(volatile unsigned *) (NAND1_BASE + 0x18))
#define NAND1_PIO_CTRL4	(*(volatile unsigned *) (NAND1_BASE + 0x1c))
#define NAND1_PIO_CTRL5	(*(volatile unsigned *) (NAND1_BASE + 0x20))
#define NAND1_PIO_CTRL6	(*(volatile unsigned *) (NAND1_BASE + 0x24))
#define NAND1_PIO_CTRL7	(*(volatile unsigned *) (NAND1_BASE + 0x28))
#define NAND1_PIO_CTRL8	(*(volatile unsigned *) (NAND1_BASE + 0x2c))
#define NAND1_PIO_INIRMSK  (*(volatile unsigned *) (NAND1_BASE + 0x40))
#define NAND1_PIO_INTR_STS (*(volatile unsigned *) (NAND1_BASE + 0x44))

#define NANDX_CSR	         0x00
#define NANDX_DESC_BA	     0x04
#define NANDX_AC_TIMING	     0x08
#define NANDX_RDYBSY_EN	     0x0c
#define NANDX_PIO_CTRL1	     0x10   // For Pio 1/2
#define NANDX_PIO_CTRL2	     0x14   // For Pio 1. for write
#define NANDX_PIO_CTRL3	     0x18   // For Pio 1. for read
#define NANDX_PIO_CTRL4	     0x1c   // For Pio 2. for cmd
#define NANDX_PIO_CTRL5	     0x20   // For Pio 2. ADDR1[39:08]
#define NANDX_PIO_CTRL6	     0x24   // For Pio 2. ALE cyc and ADDR2[07:00]
#define NANDX_PIO_CTRL7	     0x28   // For Pio 2. DMA Start address , will trigger auto when write
#define NANDX_PIO_CTRL8	     0x2c   // For Pio 2 DMA read/write
#define NANDX_INIRMSK    0x40   // INTMASK
#define NANDX_INTR_STS   0x44   // INTSTATUS   

// NAND CSR
#define NAND_EN        0x01  
#define NAND_EN        0x02
#define NAND_EN        0x04
#define AHB_ACC        0x08
#define FETCH_DESC_EN  0x10
#define EDO_TYPE       0x20

//AC TIMING
#define AC_NO_MASK    0xF 
#define CLE_NO_SHIFT   0
#define ALE_NO_SHIFT   4
#define ACT_NO_SHIFT   8
#define REC_NO_SHIFT   12
#define WAIT_NO_SHIFT   16
#define RDSTS_NO_SHIFT   20

//RDYBSY EN
#define RDYBSY_MASK      0xF 
#define RDYBSY0_SHIFT    0
#define RDYBSY1_SHIFT    4
#define RDYBSY2_SHIFT    8
#define RDYBSY3_SHIFT    12

//PIO CTRL 1  (only for PIO mode)
#define PIO_csnn0    0x001
#define PIO_csnn1    0x002
#define PIO_csnn2    0x004
#define PIO_csnn3    0x008
#define PIO_cle      0x010
#define PIO_ale      0x020
#define PIO_wrnn     0x040
#define PIO_rdnn     0x080
#define PIO_oenn     0x100
#define PIO_wpnn     0x200

//PIO CTRL 8 
#define PIO_DMA_READ   0x0000
#define PIO_DMA_WRITE  0x8000
#define DMA_LEN_MASK   0x1FFF

//INTRMASK and INTRSTS
#define DESC_COMP        0x0001
#define DESC_END         0x0002
#define DESC_ERROR       0x0004
#define DESC_INVALID     0x0008
#define INVALID_CMDTYPE  0x0010
#define AHB_BUS_BUSY     0x0020
#define RD_STS_ERROR     0x0040
#define DESC_NFC_BUSY    0x0080
#define PIO2_STS_MSK     0x0F00
#define CMD_INTR         0x0100 //for PIO2
#define ADDR_INTR        0x0200 //for PIO2
#define WDATA_INTR       0x0400 //for PIO2
#define RDATA_INTR       0x0800 //for PIO2

#define RB1_INTR         0x1000 
#define RB2_INTR         0x2000 
#define ERR_DESC_ADD_MSK 0xFFFF0000

