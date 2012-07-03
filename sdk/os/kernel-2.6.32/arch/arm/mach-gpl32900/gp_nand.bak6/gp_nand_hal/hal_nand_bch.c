#include "hal_nand_bch.h"
#include <asm/delay.h>               
#include <mach/diag.h>
#include <mach/gp_cache.h>
#include "hal_nand.h"

#include <linux/interrupt.h>
#include <mach/irqs.h>

extern void dump_buffer(unsigned char *addr, unsigned long size);

#define BCH_USE_ISR // remark to use polling

//#define BCH_DECODE_DEBUG_1 // debug option

#define BCH_DEBUG_INFO	DIAG_INFO
#define BCH_S332_BASE		BCH_S336_BASE_ADDRESS

static UINT8 iboot_bch_reg = 0;
static UINT16 max_bit_err_cnt=0;
//static BCH_MODE_ENUM   bch_mode;
static BCH_MODE_ENUM   bch_mode[NAND_CHIP_NUM];

static UINT8 all_ff_temp[128];

// 60 bits user Data=23 byte, Parity=105 byte, Pseudo=0 byte
static UINT8 all_ff_parity_of_bch_60[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3d, 0x80, 0xed, 0xda, 0xef, 0xe8, 0xb5, 0xe3, 0x44, 
	0xa1, 0x08, 0x62, 0x30, 0x25, 0x46, 0xe9, 0xd0, 0xcf, 0x7b, 0xdf, 0x65, 0x6f, 0x6c, 0x89, 0x39, 
	0xb6, 0xb0, 0xec, 0x89, 0x58, 0xde, 0x0e, 0xa8, 0x81, 0xef, 0x7a, 0x51, 0x7f, 0x36, 0xb4, 0x37, 
	0x78, 0x83, 0x23, 0xe3, 0xa6, 0xe8, 0x0e, 0x40, 0x6e, 0x12, 0x94, 0x85, 0xce, 0xbf, 0x1c, 0x6c, 
	0x06, 0x20, 0xea, 0x7c, 0x3c, 0x01, 0xca, 0x7e, 0x66, 0x84, 0x88, 0xf3, 0x94, 0x88, 0xbe, 0xb3, 
	0xfb, 0x12, 0x29, 0x21, 0x0b, 0x01, 0x45, 0xf2, 0x78, 0x2c, 0xc0, 0x1d, 0x0a, 0x01, 0xb6, 0x6a, 
	0x94, 0xb3, 0xdd, 0x2d, 0x83, 0x73, 0xfd, 0xb1, 0x0b, 0xcd, 0x94, 0x28, 0x08, 0x85, 0xbd, 0xd5, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3d, 0x80, 0xed, 0xda, 0xef, 0xe8, 0xb5, 0xe3, 0x44, 
	0xa1, 0x08, 0x62, 0x30, 0x25, 0x46, 0xe9, 0xd0, 0xcf, 0x7b, 0xdf, 0x65, 0x6f, 0x6c, 0x89, 0x39, 
	0xb6, 0xb0, 0xec, 0x89, 0x58, 0xde, 0x0e, 0xa8, 0x81, 0xef, 0x7a, 0x51, 0x7f, 0x36, 0xb4, 0x37, 
	0x78, 0x83, 0x23, 0xe3, 0xa6, 0xe8, 0x0e, 0x40, 0x6e, 0x12, 0x94, 0x85, 0xce, 0xbf, 0x1c, 0x6c, 
	0x06, 0x20, 0xea, 0x7c, 0x3c, 0x01, 0xca, 0x7e, 0x66, 0x84, 0x88, 0xf3, 0x94, 0x88, 0xbe, 0xb3, 
	0xfb, 0x12, 0x29, 0x21, 0x0b, 0x01, 0x45, 0xf2, 0x78, 0x2c, 0xc0, 0x1d, 0x0a, 0x01, 0xb6, 0x6a, 
	0x94, 0xb3, 0xdd, 0x2d, 0x83, 0x73, 0xfd, 0xb1, 0x0b, 0xcd, 0x94, 0x28, 0x08, 0x85, 0xbd, 0xd5 
};

// 40 bits User Data=22 byte, Parity=70 byte, Pseudo=4 byte
static UINT8 all_ff_parity_of_bch_40[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xb2, 0x49, 0xdf, 0x39, 0xa0, 0xd6, 0xc6, 0x7e, 0xa4, 0x3e, 
	0x55, 0x81, 0x8d, 0xbf, 0xad, 0xa6, 0xef, 0x37, 0x26, 0x64, 0xaa, 0x11, 0xd2, 0x3d, 0xfe, 0x66, 
	0x84, 0x10, 0x77, 0x79, 0x2d, 0x2c, 0x9b, 0x8c, 0x5b, 0xe5, 0x31, 0xcb, 0xdd, 0x58, 0xa4, 0x6c, 
	0x0b, 0x21, 0xce, 0xea, 0x0b, 0x53, 0x94, 0x03, 0x2b, 0xa0, 0x81, 0x1b, 0x92, 0xd5, 0x7f, 0xf6, 
	0xb1, 0x3b, 0x1c, 0x51, 0x2f, 0xb6, 0x51, 0xb7, 0xb6, 0x2b, 0x88, 0xc1, 0xff, 0xff, 0xff, 0xff
};

// 24 bits User Data=6 byte, Parity=42 byte, Pseudo=16 byte
static UINT8 all_ff_parity_of_bch_24[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xa0, 0x2c, 0x73, 0x4a, 0x1f, 0x6b, 0x46, 0xf8, 0x70, 0x6a, 
	0x79, 0x00, 0x45, 0x36, 0x38, 0xc4, 0x71, 0xfb, 0x38, 0x4b, 0xbe, 0x28, 0xd4, 0x6f, 0x77, 0x97, 
	0x4e, 0x8d, 0xe9, 0x7e, 0x08, 0x33, 0xdf, 0x83, 0xea, 0xc2, 0x73, 0x93, 0x2e, 0xde, 0x80, 0x9f,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// 16 bits User Data=4 byte, Parity=28 byte, Pseudo=32 byte
static UINT8 all_ff_parity_of_bch_16[] = {
	0xff, 0xff, 0xff, 0xff, 0xf8, 0x24, 0xfa, 0xe3, 0x2a, 0xcc, 0x54, 0x98, 0x78, 0x3a, 0x86, 0x7c, 
	0xb9, 0xed, 0x51, 0x20, 0xcd, 0x1c, 0xba, 0x86, 0xe9, 0x15, 0xe0, 0xf2, 0xc4, 0x33, 0x45, 0x90,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// 8 bits/512 User Data=2 byte, Parity=14 byte, Pseudo=16 byte
static UINT8 all_ff_parity_of_bch_8[] = {
	0xff, 0xff, 0x30, 0x9a, 0x0c, 0x9d, 0x90, 0x48, 0x5f, 0x6e, 0xb6, 0x9a, 0xa7, 0x26, 0xf0, 0x10,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// 4 bits/512 User Data=1 byte, Parity=7 byte, Pseudo=16 byte, bch software byte=0
#if 1
static UINT8 all_ff_parity_of_bch_4[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x66, 0x52, 0x9d, 0xc8, 0xc3, 0x09, 0x2c,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
#else

// 4 bits/512 User Data=9 byte, Parity=7 byte, Pseudo=16 byte, bch software byte=2
static UINT8 all_ff_parity_of_bch_4[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x5a, 0x3a, 0x95, 0xb0, 0xfc, 0x92, 0x03,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

#endif


UINT16 bch_get_max_bit_err_cnt(void)
{
	return max_bit_err_cnt;
}

static UINT8 *bch_get_all_ff_parity(BCH_MODE_ENUM BchMode, UINT16 *pPartitySize)
{
	UINT8 *all_ff_parity = NULL;
	
	switch (BchMode) {
    case BCH512B4_BITS_MODE:
    	*pPartitySize = sizeof(all_ff_parity_of_bch_4);
  		all_ff_parity = all_ff_parity_of_bch_4;
			break;
			
  	case BCH512B8_BITS_MODE:
  		*pPartitySize = sizeof(all_ff_parity_of_bch_8);
  		all_ff_parity = all_ff_parity_of_bch_8;
  		break;

		case BCH1K60_BITS_MODE:
			*pPartitySize = sizeof(all_ff_parity_of_bch_60);
  		all_ff_parity = all_ff_parity_of_bch_60;
			break;
			
		case BCH1K40_BITS_MODE:   
			*pPartitySize = sizeof(all_ff_parity_of_bch_40);
  		all_ff_parity = all_ff_parity_of_bch_40;
			break;
			
		case BCH1K24_BITS_MODE:
			*pPartitySize = sizeof(all_ff_parity_of_bch_24);
  		all_ff_parity = all_ff_parity_of_bch_24;  
			break;
			
		case BCH1K16_BITS_MODE: 
			*pPartitySize = sizeof(all_ff_parity_of_bch_16);
  		all_ff_parity = all_ff_parity_of_bch_16;  
      break;  
        
    default:
   	case BCH_OFF:
   		*pPartitySize = 0;
      break;
	}	
	return all_ff_parity;
}

void bch_strong_reset(void)
{
	volatile UINT32 k;
	
	rSCUA_PERI_RST = 0x2000 ; // bit 13 strong BCH reset
	rSCUA_PERI_RST &= (~0x2000);
	
	for (k=0; k<0x2000; k++) {
		if ((rSCUA_PERI_RST & 0x2000) == 0) // strong reset complete
			break;
	}
	if (k == 0x2000)
		BCH_DEBUG_INFO("bch_strong_reset timeout after[%d]\n", k);
}

void bch_sw_reset(void)
{
	volatile UINT32 k;
		
	(*(volatile unsigned *)(BCH_S332_BASE+0x10)) = 0x1;
	for (k=0; k<0x2000; k++) {
		if (( (*(volatile unsigned *)(BCH_S332_BASE+0x10)) & 0x01 ) == 0) // reset complete
			return;
	}
	BCH_DEBUG_INFO("bch_sw_reset timeout after[%d]\n", k);
}

void bch_reset(void)
{
	bch_strong_reset();
	bch_sw_reset();
}

#define R_BCH_S332_INT_STATUS			(*(volatile UINT32 *) (BCH_S332_BASE + 0x0C))

#if defined(BCH_USE_ISR)
#define R_BCH_S332_REPORT_STATUS	(*(volatile UINT32 *) (BCH_S332_BASE + 0x18))
#define BCH_INTERRUPTTED (1<<0)

static wait_queue_head_t bch_int_wait_queue;
static UINT32 bch_int_status;
static bool bch_int_inited = 0;

static irqreturn_t bch_interrupt_isr(int irq, void *dev_id)
{
	unsigned int int_status = R_BCH_S332_INT_STATUS;
	
	R_BCH_S332_INT_STATUS = int_status; // clear bch interrupt
	
	if ((int_status & 0x10) != 0) {
		BCH_DEBUG_INFO("bch int while still busy[%x]int_status[%x]bch_int_status[%x]\n", R_BCH_S332_REPORT_STATUS, int_status, bch_int_status);
	}
	
	bch_int_status |= int_status;
	if (bch_int_status & BCH_INTERRUPTTED)
		wake_up(&bch_int_wait_queue);
	
	return IRQ_HANDLED;
}

SINT32 bch_init_intr(void) 
{
	SINT32 ret = 0;
	
	BCH_DEBUG_INFO("bch_init_intr\n");
	if (!bch_int_inited) {
		init_waitqueue_head(&bch_int_wait_queue);

		/* clear interrupts before register */ 
		(*(volatile unsigned *)(BCH_S332_BASE+0x14)) = 0; 
		R_BCH_S332_INT_STATUS = 0xffffffff; 

		ret =request_irq(IRQ_BCH, bch_interrupt_isr, IRQF_DISABLED, BCH_DEVICE_NAME, NULL);
		bch_int_inited = true;
		BCH_DEBUG_INFO("BCH_USE_ISR on. reg ret[%d]\n", ret);
	}
	return ret;
}

void bch_uninit_intr(void) 
{
	BCH_DEBUG_INFO("bch_uninit_intr\n");
	free_irq(IRQ_BCH, NULL);
	bch_int_inited = false;
}

#else

SINT32 bch_init_intr(void) 
{
	return 0;
}

void bch_uninit_intr(void) 
{
	return;
}

#endif


UINT16 nand_bch_err_bits_get(void)
{
	if((max_bit_err_cnt==0xFE00) ||(max_bit_err_cnt==0xFF00))
	{
		return 0;	
	}
	else
	{
    return max_bit_err_cnt;
  } 
}


BCH_MODE_ENUM nand_bch_get(void)
{
    return bch_mode[Nand_Select];
}

void nand_bch_set(BCH_MODE_ENUM type)   /*更新 bch mode , 一定要更新 spare size*/
{
  UINT16 spare_size = 0;
  UINT16 page_size = 0;
  UINT16 parity_size = 0;
  UINT16 sector_nums_per_page = 0;
  UINT8	 spare_align_size = 0;

  bch_mode[Nand_Select]=type;
  page_size = gPhysical_NandInfo->wPageSize;
  sector_nums_per_page = (page_size/512);
  //spare_size = (page_size/32);  // default spare size                                                                                                                              
  switch (bch_mode[Nand_Select])
  {         
        case BCH1K60_BITS_MODE:  // base on 1-bit/14-bitss      
            parity_size = 128;   // 60*14/8=105 Byte  Align(105)
            spare_size  = parity_size*page_size/1024;  //each 2KB need 96Bytes Parity in Spare Sapce
            spare_align_size = 128;
            break;
        case BCH1K40_BITS_MODE:
            parity_size = 96;
            spare_size  = parity_size*page_size/1024;  //each 2KB need 96Bytes Parity in Spare Sapce
            spare_align_size = 96;
            break;
        
        case BCH1K24_BITS_MODE:
            parity_size = 48;
            spare_size  = parity_size*page_size/1024;  //each 2KB need 96Bytes Parity in Spare Sapce
            spare_align_size = 64;
            break;
        
        case BCH1K16_BITS_MODE:
            parity_size = 32;
            spare_size  = parity_size*page_size/1024;  //each 2KB need 96Bytes Parity in Spare Sapce
            spare_align_size = 64;
            break;
            
        case BCH512B8_BITS_MODE:

            parity_size = 16;
            spare_size  = parity_size*page_size/512;  //each 2KB need 96Bytes Parity in Spare Sapce
            spare_align_size = 32;
            break;
        case BCH512B4_BITS_MODE:
            parity_size = 16;
            //spare_size = 16*page_size/512;  //each 512Byte need 16Bytes Parity in Spare Sapce
            //spare_align_size = 16;
            spare_size = parity_size*page_size/512;  //each 512Byte need 16Bytes Parity in Spare Sapce
            spare_align_size = 16;
            break;
        case BCH_OFF:
            break;           
        default:
            parity_size = 64;
            spare_size = parity_size*page_size/1024;
            spare_align_size = 64;
            break;
    }
    gPhysical_NandInfo->wBchParityAlignSize=spare_align_size;
    gPhysical_NandInfo->wParitySize=parity_size;
    gPhysical_NandInfo->wPageSectorSize = sector_nums_per_page;
    gPhysical_NandInfo->wSpareSize = spare_size;
    gPhysical_NandInfo->wPage_with_spare_size = page_size + spare_size;

    spare_buf_init();// 改變 BCH 會改變 Spare buffer 使用分佈, 因此需要 initail
   
   return;
}

static SINT32 BCH_Decode_Basic(BCH_MODE_ENUM BchMode, UINT32 DataSrcAddr,UINT16 data_length ,UINT32 ParitySrcAddr, UINT32 pageAddr)
{
	unsigned long t, ms_timeout;
	//unsigned long k, j;
	#if defined(BCH_DECODE_DEBUG_1) && defined(BCH_USE_ISR)
	unsigned long retry = 0;
	#endif
	UINT32	pDataSrcAddr;
	UINT32	pParitySrcAddr;
	volatile SINT8 SectorNums;
	volatile UINT16 mode=(UINT16) (BchMode&0xFF);
  UINT32 user_data_mode = 0;
	UINT32 support_corrected_bit_cnt=0;
	
	#if defined(BCH_DECODE_DEBUG_1) && defined(BCH_USE_ISR)
	L_RETRY:
	#endif
	
		//DIAG_INFO ("BCH Decode Enter\n");
		#if defined(BCH_USE_ISR)
		bch_int_status = 0;
		if (!bch_int_inited) {
			max_bit_err_cnt = -1;
			BCH_DEBUG_INFO("Bch decode int init fail\n");
			return -1;
		}
		#endif

		#if defined(BCH_DECODE_DEBUG_1)
		k = 0;
		while ( (R_BCH_S332_INT_STATUS & 0x10) != 0 ) {
			BCH_DEBUG_INFO("BCH ERROR: Bch decode is busy:pageAddr[%u]max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]nandsts[%x]k[%d]\n", \
            										pageAddr, \
            										max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)), \
  	    												R_NAND_INTR_STS, \
  	    												k );
			bch_reset();
			k++;
			if (k > 10)
				break;
		}
		#endif
		
		//DIAG_INFO ("DataSrcAddr virt addr:0x%x\n",DataSrcAddr);
		pDataSrcAddr = (UINT32)virt_to_phys((UINT32*)DataSrcAddr);
		//DIAG_INFO ("DataSrcAddr phy addr:0x%x\n",DataSrcAddr);
		
		//DIAG_INFO ("ParitySrcAddr virt addr:0x%x\n",ParitySrcAddr);
		pParitySrcAddr = (UINT32)virt_to_phys((UINT32*)ParitySrcAddr);
		//DIAG_INFO ("ParitySrcAddr phy addr:0x%x\n",ParitySrcAddr);
	
    switch (BchMode) 
    {
        case BCH512B8_BITS_MODE:
        		support_corrected_bit_cnt = 8;
        case BCH512B4_BITS_MODE:
        		support_corrected_bit_cnt = 4;
            user_data_mode = 0x00000000;
            SectorNums = (data_length/512)-1;
            break;
        case BCH1K60_BITS_MODE:
        		support_corrected_bit_cnt = 60;
        case BCH1K40_BITS_MODE:    
        		support_corrected_bit_cnt = 40;
            user_data_mode = 0x50000000;
            SectorNums = (data_length/1024)-1; 
            break;
        case BCH1K24_BITS_MODE: 
        		support_corrected_bit_cnt = 24;
            user_data_mode = 0x10000000;
            SectorNums = (data_length/1024)-1; 
            break;
        case BCH1K16_BITS_MODE: 
        		support_corrected_bit_cnt = 16;
            user_data_mode = 0x00000000;
            SectorNums = (data_length/1024)-1; 
            break;    
        case BCH_OFF:
        		support_corrected_bit_cnt = 0;
            return -1;
            break;
    }

    if (SectorNums<0) {return -1;}

    if(iboot_bch_reg==1)
    {
        user_data_mode = 0x50000000; // modify to BCH1K60_BITS_MODE, since header is using 60 bits ECC
    }
    
    gp_clean_dcache_range(DataSrcAddr,data_length);
		gp_clean_dcache_range(ParitySrcAddr,1024);
		gp_invalidate_dcache_range(DataSrcAddr,data_length);
		gp_invalidate_dcache_range(ParitySrcAddr,1024);
	  
		(*(volatile unsigned *)(BCH_S332_BASE+0xC)) = 1; //clear int flag
  	(*(volatile unsigned *)(BCH_S332_BASE+0x4))		=	pDataSrcAddr ;		//Data pointer
    (*(volatile unsigned *)(BCH_S332_BASE+0x8))		=	pParitySrcAddr ;	//Parity pointer
    (*(volatile unsigned *)(BCH_S332_BASE+0x14))  =	0x1 ;							//INT_MASK, must 1
		
		(*(volatile unsigned *)(BCH_S332_BASE+0x0))		=	SectorNums*0x10000 + mode*0x100 + 0x1*0x10 +0x1 + user_data_mode;
  	
  	#if defined(BCH_USE_ISR)
  	ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(bch_int_wait_queue, bch_int_status & BCH_INTERRUPTTED, ms_timeout);
  	if (ms_timeout == 0) {
  		BCH_DEBUG_INFO("Bch decode int fail:%lu.page[%u]\n", jiffies - t, pageAddr);
  		max_bit_err_cnt = -1;
  		BCH_DEBUG_INFO("Bch decode int fail:pageAddr[%u]max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]\n", \
            										pageAddr, \
            										max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)) );
  	  
  	  #if defined(BCH_DECODE_DEBUG_1)
  	  for (k=1; k < 10; k++) {
  	  	ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  			t=jiffies;
  			ms_timeout = wait_event_timeout(bch_int_wait_queue, bch_int_status & BCH_INTERRUPTTED, ms_timeout);
  			BCH_DEBUG_INFO("Bch decode int fail:pageAddr[%u]max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]k[%d]\n", \
            										pageAddr, \
            										max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)), \
  	    												k );
  	  }
  	  #endif
  	  											
  		bch_reset();
  		
  		#if defined(BCH_DECODE_DEBUG_1)
  		if (retry < 1) {
  			retry++;
  			BCH_DEBUG_INFO("bch_reset, pageAddr[%u] try again[%lu]\n", pageAddr, retry);
  			goto L_RETRY;
  		}
  		#endif
  		
  		return -1;
  	}
  	#else
  	ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	while(((*(volatile unsigned *)(BCH_S332_BASE+0xC)) & 0x1 ) == 0x0) { //check int flag
  		if (time_after(jiffies, t + ms_timeout)) {
  			BCH_DEBUG_INFO("Bch decode int fail:%lu.page[%u]\n", jiffies - t, pageAddr);
  			max_bit_err_cnt = -1;
  			BCH_DEBUG_INFO("Bch decode int fail:pageAddr[%u]max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]\n", \
            										pageAddr, \
            										max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)) );
  			bch_reset();
  			return -1;
  		}
  	}
  	#endif
 
 		ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
    t=jiffies;
  	while(((*(volatile unsigned *)(BCH_S332_BASE+0x18)) & 0x1 ) == 0x0) { //check bch finish
  		if (time_after(jiffies, t + ms_timeout)) {
  			BCH_DEBUG_INFO("Bch decode finish fail:%lu.page[%u]\n", jiffies - t, pageAddr);
  			max_bit_err_cnt = -1;
  			bch_reset();
  			return -1;
  		}
  	}
  	// (*(volatile unsigned *)(BCH_S332_BASE+0xC)) = 1; //clear int flag
  	  	
    if(((*(volatile unsigned *)(BCH_S332_BASE+0x18)) & 0x10 ) == 0x10) 
    {
        if((*(volatile unsigned *)(BCH_S332_BASE+0x18))&0x01000000) // 全 0xFF
        {
  	    		max_bit_err_cnt = 0xFE00;  // Clear Page Pattern
  	    		//BCH_DEBUG_INFO("decode all 0xff.page[%u]\n", pageAddr);
        } else {
            max_bit_err_cnt = 0xFD00;
            
            #if 0
            // check how many ff, check only data area
            for (k=0, j=0; k < data_length; k++) {
            	if ( ( *(unsigned char *)(DataSrcAddr+k) ) == 0xff )
            		j++;
          	}
          	
          	if ((j >= (data_length - 2)) && (j <= (data_length - 1))) {
         	 	}
          	else {
            	BCH_DEBUG_INFO("Warning, page[%u] decode fail detected:not all ff.max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]\n", \
            										pageAddr, \
            										max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)) );
  	    			dump_buffer((unsigned char *)DataSrcAddr, data_length);
  	    			dump_buffer((unsigned char *)ParitySrcAddr, 1024);
  	    		}
  	    		#endif
        }
        
        bch_sw_reset(); // according to IC designer, when happen decode fail, should do software reset to clear decode fail flag else will impact encode
  	} else {   
        if((*(volatile unsigned *)(BCH_S332_BASE+0x18))&0x10000000) // 全 0x00
        {
            max_bit_err_cnt = 0xFF00;  // Bad block token
        } else {
  	    		max_bit_err_cnt = (*(volatile unsigned *)(BCH_S332_BASE+0x24));
  	    		if (max_bit_err_cnt > 60) {
  	    			BCH_DEBUG_INFO("Error, page[%u] no decode fail but max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]\n",
  	    												pageAddr, \
  	    												max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)) );
  	    			//dump_buffer((unsigned char *)DataSrcAddr, data_length);
  	    			//dump_buffer((unsigned char *)ParitySrcAddr, 1024);
  	    		}
  	    		#if 0 // debug message
  	    		else {
  	    			if ((support_corrected_bit_cnt >=8) && (max_bit_err_cnt > support_corrected_bit_cnt/2)) {
  	    				BCH_DEBUG_INFO("Info, page[%u] decode success but max_bit_err_cnt[%x]0x18[%x]0x1C[%x]0x20[%x]0xC[%x]0x24[%x]0x0[%x]\n",
  	    												pageAddr, \
  	    												max_bit_err_cnt, \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x18)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x1C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x20)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0C)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x24)), \
  	    												(*(volatile unsigned *)(BCH_S332_BASE+0x0)) );
  	   				}
  	    		}
  	    		#endif
        }
    }
    
  	(*(volatile unsigned *)(BCH_S332_BASE+0xC)) = 1; //clear int flag  	

    return (SINT16)max_bit_err_cnt;
}

SINT32 BCH_Decode(BCH_MODE_ENUM BchMode, UINT32 DataSrcAddr,UINT16 data_length ,UINT32 ParitySrcAddr, UINT32 pageAddr)
{
	SINT32 ret, ret1, ret2;
	UINT16 sector_length, k, j;
	UINT16 num_of_sector;
	UINT16 num_of_allff;
	UINT16 save_max_bit_err_cnt;
	UINT16 partitySize;
	UINT8 *all_ff_parity;
	UINT8 *parity_addr;
	
	ret = BCH_Decode_Basic(BchMode, DataSrcAddr, data_length, ParitySrcAddr, pageAddr);
	save_max_bit_err_cnt = max_bit_err_cnt;
	if ((BchMode != BCH_OFF) && (ret == 0xFFFFFD00)) {
		//BCH_DEBUG_INFO("detect decode fail.page[%u]\n", pageAddr);
		/* decode fail case, try assume all 0xff to avoid regarded few zero bits cause regarded as non ff */
		all_ff_parity = bch_get_all_ff_parity(BchMode, &partitySize);
		if (all_ff_parity != NULL && partitySize != 0) {
			if (BchMode == BCH512B8_BITS_MODE || BchMode == BCH512B4_BITS_MODE)
				sector_length = 512;
			else
				sector_length = 1024;
			num_of_sector = data_length / sector_length;
			num_of_allff = 0;
			for (k = 0; k < num_of_sector; k++ ) {
				parity_addr = (UINT8 *)(ParitySrcAddr+(k*partitySize));
				if (num_of_sector > 1) {
					ret2 = BCH_Decode_Basic(BchMode, DataSrcAddr+(k*sector_length), sector_length, (UINT32)parity_addr, pageAddr);
					if (ret2 == 0xFFFFFE00) {
						num_of_allff++;
						continue;
					}
					else if (ret2 != 0xFFFFFD00)
						break;
				}
				memcpy(all_ff_temp, parity_addr, partitySize); // back up original parity 
				/* and all ff parity with current parity */
				for (j=0; j < partitySize; j++)
					parity_addr[j] &= all_ff_parity[j];
				ret1 = BCH_Decode_Basic(BchMode, DataSrcAddr+(k*sector_length), sector_length, (UINT32)parity_addr, pageAddr);
				if ((ret1 <= -1) || (ret > 60)) {
					memcpy(parity_addr, all_ff_temp, partitySize); // restore up original parity 
					gp_clean_dcache_range((UINT32)parity_addr, partitySize);
					gp_invalidate_dcache_range((UINT32)parity_addr, partitySize);
				}
				else {
					num_of_allff++;
					//BCH_DEBUG_INFO("become all 0xff ret1[0x%x]k[%u]num_of_sector[%u]page[%u]\n", ret1, k, num_of_sector, pageAddr);
					memset(parity_addr, 0xff, partitySize); // restore up original parity 
					gp_clean_dcache_range((UINT32)parity_addr, partitySize);
					gp_invalidate_dcache_range((UINT32)parity_addr, partitySize);
				}
			} // end for
			if (num_of_allff == num_of_sector) {
				//BCH_DEBUG_INFO("decode fail become all 0xff.page[%u]", pageAddr);
				max_bit_err_cnt = 0xFE00; 
				ret = (SINT16)max_bit_err_cnt;
				// dump_buffer((unsigned char *)DataSrcAddr, data_length);
				// dump_buffer((unsigned char *)ParitySrcAddr, 1024);
			}
		}
	}
	else {
		max_bit_err_cnt = save_max_bit_err_cnt;
	}

	if (ret == 0xFFFFFD00) {
		/* decode fail case return as -1 */
		max_bit_err_cnt = -1; 
		ret = (SINT16)max_bit_err_cnt;
		//BCH_DEBUG_INFO("decode fail case return as -1.page[%u]\n", pageAddr);
	}
	#if 0
	else if (ret == 0xFFFFFE00)
		BCH_DEBUG_INFO("detect all ff.page[%u]\n", pageAddr);
	#endif
	return ret;
}

void bch_boot_area_init(void)
{
    iboot_bch_reg=1;
}


void bch_boot_area_uninit(void)
{
    iboot_bch_reg=0;
}


SINT32 BCH_Encode(BCH_MODE_ENUM BchMode, UINT32 DataSrcAddr,UINT16 data_length ,UINT32 ParityGenAddr, UINT32 pageAddr)
{
		unsigned long t, ms_timeout;
		UINT32	pDataSrcAddr;
		UINT32	pParityGenAddr;
		volatile SINT8 SectorNums;
		volatile UINT16 mode=(UINT16) (BchMode&0xFF);
		UINT32   user_data_mode = 0;

		//DIAG_INFO ("BCH Encode Enter\n");
		#if defined(BCH_USE_ISR)
		bch_int_status = 0;
		if (!bch_int_inited) {
			max_bit_err_cnt = -1;
			BCH_DEBUG_INFO("Bch encode int init fail\n");
			return -1;
		}
		#endif
		
		//DIAG_INFO ("DataSrcAddr virt addr:0x%x\n",DataSrcAddr);
		pDataSrcAddr = (UINT32)virt_to_phys((UINT32*)DataSrcAddr);
		//DIAG_INFO ("DataSrcAddr phy addr:0x%x\n",DataSrcAddr);
		
		//DIAG_INFO ("ParitySrcAddr virt addr:0x%x\n",ParityGenAddr);
		pParityGenAddr = (UINT32)virt_to_phys((UINT32*)ParityGenAddr);
		//DIAG_INFO ("ParitySrcAddr phy addr:0x%x\n",ParityGenAddr);

    switch (BchMode) 
    {
        case BCH512B8_BITS_MODE:
        case BCH512B4_BITS_MODE:
            user_data_mode = 0x00000000;
            SectorNums = (data_length/512)-1;
            break;
        case BCH1K60_BITS_MODE:
        case BCH1K40_BITS_MODE:    
            user_data_mode = 0x50000000;
            SectorNums = (data_length/1024)-1; 
            break;
        case BCH1K24_BITS_MODE: 
            user_data_mode = 0x10000000;
            SectorNums = (data_length/1024)-1; 
            break;
        case BCH1K16_BITS_MODE: 
            user_data_mode = 0x00000000;
            SectorNums = (data_length/1024)-1; 
            break;    
        case BCH_OFF:
            return STATUS_OK;
            break;
    }

    if(iboot_bch_reg==1)
    {
        user_data_mode = 0x00000000;
    }

    if((BchMode==BCH512B8_BITS_MODE)||(BchMode==BCH512B4_BITS_MODE)){
        SectorNums = (data_length/512)-1;
    } else {
        SectorNums = (data_length/1024)-1;
    }

  if (SectorNums<0) {return -1;}
		
	gp_clean_dcache_range(DataSrcAddr,data_length);
	gp_clean_dcache_range(ParityGenAddr,1024);
	gp_invalidate_dcache_range(ParityGenAddr,1024);
	
	(*(volatile unsigned *)(BCH_S332_BASE+0xC)) = 1; //clear int flag
	(*(volatile unsigned *)(BCH_S332_BASE+0x4))		=	pDataSrcAddr ;		//Data pointer
	(*(volatile unsigned *)(BCH_S332_BASE+0x8))		=	pParityGenAddr ;	//Parity pointer
	(*(volatile unsigned *)(BCH_S332_BASE+0x14))  =	0x1 ;				//INT_MASK      
	(*(volatile unsigned *)(BCH_S332_BASE+0x0))	=	SectorNums*0x10000 + mode*0x100 + 0x0*0x10 +0x1 + user_data_mode ;

	#if defined(BCH_USE_ISR)
	ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  t=jiffies;
  ms_timeout = wait_event_timeout(bch_int_wait_queue, bch_int_status & BCH_INTERRUPTTED, ms_timeout);
  if (ms_timeout == 0) {
  	BCH_DEBUG_INFO("Bch encode int fail:%lu.page[%u]\n", jiffies - t, pageAddr);
  	bch_reset();
		return -1;
	}
  #else
  ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  t=jiffies;
	while( ( (*(volatile unsigned *)(BCH_S332_BASE+0xC)) & 0x1 ) == 0x0  ) { //check int flag
		if (time_after(jiffies, t + ms_timeout)) {
			BCH_DEBUG_INFO("Bch encode int fail:%lu.page[%u]\n", jiffies - t, pageAddr);
			bch_reset();
			return -1;
		}
	}
	#endif

	ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
	t=jiffies;
	while(((*(volatile unsigned *)(BCH_S332_BASE+0x18)) & 0x1 ) == 0x0) { //check bch finish
		if (time_after(jiffies, t + ms_timeout)) {
			BCH_DEBUG_INFO("Bch encode finish fail:%lu\n", jiffies - t);
			bch_reset();
			return -1;
		}
	}

	(*(volatile unsigned *)(BCH_S332_BASE+0xC)) = 1; //clear int flag
	
	return STATUS_OK;
}

void bch_mode_set(BCH_MODE_ENUM bch_set)
{
    nand_bch_set(bch_set); 
}

BCH_MODE_ENUM bch_mode_get(void)
{
	return nand_bch_get();
}

UINT16 get_bch_mode(void)
{
	return nand_bch_get();
}
