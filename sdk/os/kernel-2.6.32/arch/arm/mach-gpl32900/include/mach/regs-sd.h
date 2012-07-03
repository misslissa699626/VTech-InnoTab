/*
 * arch/arm/mach-spmp8000/include/mach/regs-timer.h
 *
 * Copyright (C) 2007 Sunplus MultiMedia
 *
 * Timer - System peripherals regsters.
 *
 */



#define SD0_BASE			IO2_ADDRESS(0xB0B000)
#define SD1_BASE			IO2_ADDRESS(0xB0C000)

/* SD 0 register */
#define SD0_DATATX		(*(volatile unsigned int*)(SD0_BASE+0x00))  
#define SD0_DATARX		(*(volatile unsigned int*)(SD0_BASE+0x04))  
#define SD0_COMMAND		(*(volatile unsigned int*)(SD0_BASE+0x08)) 
#define SD0_ARGUMENT	(*(volatile unsigned int*)(SD0_BASE+0x0c))  
#define SD0_RESPONSE	(*(volatile unsigned int*)(SD0_BASE+0x10))  
#define SD0_STATUS		(*(volatile unsigned int*)(SD0_BASE+0x00))  
#define SD0_CONTROL		(*(volatile unsigned int*)(SD0_BASE+0x04)) 
#define SD0_INTEN		(*(volatile unsigned int*)(SD0_BASE+0x08)) 

/* SD 1 register */
#define SD1_DATATX		(*(volatile unsigned int*)(SD1_BASE+0x00))  
#define SD1_DATARX		(*(volatile unsigned int*)(SD1_BASE+0x04))  
#define SD1_COMMAND		(*(volatile unsigned int*)(SD1_BASE+0x08)) 
#define SD1_ARGUMENT	(*(volatile unsigned int*)(SD1_BASE+0x0c))  
#define SD1_RESPONSE	(*(volatile unsigned int*)(SD1_BASE+0x10))  
#define SD1_STATUS		(*(volatile unsigned int*)(SD1_BASE+0x14))  
#define SD1_CONTROL		(*(volatile unsigned int*)(SD1_BASE+0x18)) 
#define SD1_INTEN		(*(volatile unsigned int*)(SD1_BASE+0x1c)) 


/* SD reg index */
#define SDX_DATATX		0x00
#define SDX_DATARX		0x04
#define SDX_COMMAND		0x08 
#define SDX_ARGUMENT	0x0c 
#define SDX_RESPONSE	0x10
#define SDX_STATUS		0x14
#define SDX_CONTROL		0x18
#define SDX_INTEN		0x1c

#define INIT_SPEED0			0x0000
#define INIT_SPEED1			0x0001

#define MASK_C_BUSWIDTH_1			0x0000	
#define MASK_C_BUSWIDTH_4			0x0100 
#define MASK_C_DMAMODE				0x0200
#define MASK_C_IOMODE					   0x0400
#define MASK_C_ENSDBUS				   0x0800

#define MASK_C_TXTRI_1         	 0x0000
#define MASK_C_RXTRI_8         	 0xC000
#define MASK_C_RXTRI            	MASK_C_RXTRI_8
#define MASK_C_TXTRI            	MASK_C_TXTRI_1

#define MASK_C_BL_256		0x01000000
#define MASK_C_BL_512		0x02000000 				
#define MASK_C_BL_8		0x00080000
#define MASK_C_BL_2		0x00020000

#define MASK_BL_8	(0x0008)
#define MASK_BL_2	(0x0002)

//===================================================================================
#define MASK_S_BUSY			0x0001
#define MASK_S_CARDBUSY		0x0002
#define MASK_S_CMDCOM		0x0004
#define MASK_S_DATCOM		0x0008
#define MASK_S_RSPIDXERR	0x0010
#define MASK_S_RSPCRCERR	0x0020
#define MASK_S_CMDBUFFULL	0x0040
#define MASK_S_DATABUFFULL	0x0080
#define MASK_S_DATABUFEMPTY	0x0100
#define MASK_S_TIMEOUT		0x0200
#define MASK_S_DATCRCERR	0x0400
#define MASK_S_CARDWP		0x0800
#define MASK_S_CARDPRE		0x1000
#define MASK_S_CARDINT		0x2000
#define MASK_S_CEATACMDCOM	0x4000
#define MASK_S_ClrAllBits	0xFFFF

#define MASK_CMD_STOP			0x0040		// Stop Command
#define MASK_CMD_RUN			0x0080		// Initiate the SD command, will be cleared to '0' after the transaction start.
#define MASK_CMD_WITHDATA		0x0100		// 0: Command without data, 1: Command with data
#define MASK_TRANSDATA			0x0200		// !MASK_TransferData = MASK_ReceiveData
#define MASK_MULTIBLOCK			0x0400		// !MASK_TransMultiBlock = MASK_TransSingleBlock
#define MASK_CC74				0x0800		// 74 Clock cycles on the clock line



#define MASK_RESPTYPE  		0x7000		// Response type R1b
#define MASK_RESPTYPE0  		0x0000		// No response
#define MASK_RESPTYPE1  		0x1000 		// Response type R1
#define MASK_RESPTYPE2  		0x2000 		// Response type R2	
#define MASK_RESPTYPE3  		0x3000		// contains OCR register R3
#define MASK_RESPTYPE6  		0x6000		// Response type R6
#define MASK_RESPTYPE7  		MASK_RESPTYPE1	// Response type R7, almost the same as type 1
#define MASK_RESPTYPE1b 		0x7000		// Response type R1b


#define INTEN0    0x01   // CMDCOM     
#define INTEN1    0x02   // DATCOM
#define INTEN2    0x04   // CMDBUFFULL
#define INTEN3    0x08   // DATABUFFULL
#define INTEN4    0x10   // DATABUFEMPTY
#define INTEN5    0x20   // Card Insert remove 
#define INTEN6    0x40   // SDIO card interrupt
#define INTEN7    0x80   // CE-ATA command complete interrupt

