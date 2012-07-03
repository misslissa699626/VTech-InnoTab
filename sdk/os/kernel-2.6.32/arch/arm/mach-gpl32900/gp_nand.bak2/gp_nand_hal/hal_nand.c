#include "hal_nand.h"
#include <asm/cacheflush.h>
#include <mach/gp_board.h>
#include <mach/diag.h>
#include <mach/gp_cache.h>

#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <mach/clock_mgr/gp_clock.h>
#include <linux/delay.h>

// Added by Kevin
#define WRITE32(addr, value) 	do{*((volatile unsigned int *)addr) = value;}while(0)
#define READ32(addr)			(*((volatile unsigned int *)addr))

#define USE_TIME_FOR_TIMEOUT /* define this when want to use time tick for time out instead of variable counter */
#define NF0_USE_ISR // remark to use polling
#if defined(NF_DATA_DEBUG_WRITE_TO_NON_FF_PAGE)
extern UINT16 bch_get_max_bit_err_cnt(void);
 void *debugWorkBuffer; // william
#endif

extern void bch_reset(void);

//#define NF_HAL_DIAG(...)
#define NF_HAL_DIAG	DIAG_INFO

gp_board_nand_t			*nand_func[NAND_CHIP_NUM] = {NULL,NULL};			/*!< @brief Card detect, write protect, power function. */

#define ALL_NEW_SPEC_NTYPE_ID_START  20
#define ALL_TBL_FIX_NTYPE_ID_END     10
#define LAST_SMALLPAGE_TBLID         10
#define OLD_SPEC_NTYPE_ID_START      11

//#define NAND_WAIT_CNT 				100 //10 us 
//#define MAX_NAND_TIMEOUT 			5

//#define MAX_NAND_WAIT_CNT 		200000 //200ms

#define	ND_INTR_DESC_DONE			(1<<0)
#define	ND_INTR_DESC_END			(1<<1)
#define	ND_INTR_DESC_ERROR		(1<<2)
#define	ND_INTR_DESC_INVALID	(1<<3)
#define ND_INTR_DESC_RB1 			(1<<12)
#define ND_INTR_DESC_RB2 			(1<<13)
#define ND_INTR_DESC_RB3 			(1<<14)
#define ND_INTR_DESC_RB4 			(1<<15)
#define ND_INTR_DESC_RB 			(ND_INTR_DESC_RB1|ND_INTR_DESC_RB2)
#define ND_INTR_DESC_NFC_BUSY (1<<7)

#define MAUNCMD_RESET										0xFF00
#define MAUNCMD_READID									0x9000
#define MAUNCMD_BLOCK_ERASE							0x6000
#define MAUNCMD_CONF_BLOCK_ERASE				0xD000
#define MAUNCMD_WRITE										0x8000
#define MAUNCMD_CONF_WRITE							0x1000
#define MAUNCMD_WRITE_TWO_PLAN					0x8100
#define MAUNCMD_CONF_WRITE_TWO_PLAN			0x1100

#define AUTOCMD_READ 										0x0030
#define AUTOCMD_WRITE 									0x8010
#define AUTOCMD_WRITE_TWOPLAN_BEGIN 		0x8011
#define AUTOCMD_WRITE_TWOPLAN_END 			0x8110

//MCD_TYPE
#define CMD_TYPE_READ 										0x1
#define CMD_TYPE_WRITE 										0x2
#define CMD_TYPE_ERASE 										0x3
#define CMD_TYPE_READ_STATUS 							0x4
#define CMD_TYPE_HALFAUTO_ERASE 					0x5 //HalfAutoErase
#define CMD_TYPE_HALFAUTO_WRITE 					0x6//HalfAutoWrite
#define CMD_TYPE_MANUAL_MODE_CMD 					0x7
#define CMD_TYPE_MANUAL_MODE_ADDR 				0x8
#define CMD_TYPE_MANUAL_MODE_PYLOAD_WRITE 0x9
#define CMD_TYPE_MANUAL_MODE_PYLOAD_READ 	0xa
#define CMD_TYPE_MANUAL_MODE_REDUNT_WRITE 0xb
#define CMD_TYPE_MANUAL_MODE_REDUNT_READ 	0xc
#define CMD_TYPE_MANUAL_MODE_SPARE_WRITE 	0xd
#define CMD_TYPE_MANUAL_MODE_SPARE_READ 	0xe
#define CMD_TYPE_MANUAL_MODE_STS_RD 			0xf

#define NAND_WP_PIN_NONE    0xFF
#define NF_GOOD_BLK_TAG     0xFF
DescPara DescParas;  // Manual Mode Need 3-Descripters

static bool hal_buf_inited = false; // william

UINT8* nand_spare_buf;
UINT8  *nf_id;
UINT32 *DescVector;
static UINT8 NTYPE;
Physical_NandInfo* gPhysical_NandInfo;
Physical_NandInfo Physical_NandGroup[NAND_CHIP_NUM];
UINT32 Nand_Select=0;
static UINT8 EraseCycle;
static UINT32 nand_total_MByte_size;

/*ProtoType Declare*/
static NAND_PAD_ENUM nand_pad;
static UINT8 nand_wp_io;
static UINT8 write_status;
UINT8  nand_small_page_cs;
UINT8  wp_bit = 0x00;
static UINT32 SpareFlagH = 0xFFFFFFFF;
static UINT32 SpareFlagL = 0xFFFFFFFF;
void NandPadReg(NAND_PAD_ENUM pad_assign);
SINT32 nand_reset(void); 
SINT32 nand_read_id_all(UINT16* MainID, UINT32* VendorID);
void fillDescriptor(ppara_t pstPara, UINT8 DescId);
void nand_cs_io_init(void);
void nand_cs_io_high(void);
void nand_cs_io_low(void);
SINT32 Nand_read_Status(void); 
void DrvNand_WP_Initial(void);
void nand_wp_pin_reg(UINT8 gpio_pin_number);
NFCMD_STATUS Nand_phy_write(UINT32 page_addr, UINT32 buf_addr);
void DescParasReset(DescPara *DescAddr);
void spare_buf_init(void);
UINT16 spare_flag_get(void);
void nand_small_page_cs_pin_reg(UINT8 nand_cs);
UINT8 Nand_Fast_Status_Get(void);
static void spare_flag_set_into_page(void);
static void spare_flag_get_from_page(void);
SINT32 Nand_LiveResponse(void);
void gpnand_cache_sync(void);
void nand_cache_invalidate(void);
void nand_cache_sync(void);
void ParseNandInfo(void);
static SINT8 Nand_badblk_flag(void);
//SINT32 nand_memcpy(SINT8 *dest, SINT8 *src, UINT32 Len);

void Nand_Malloc_Buffer(void);

NAND_PAD_ENUM Nand_Pad_Scan(NAND_PAD_ENUM pad_mode)
{
    if (pad_mode==NAND_PAD_AUTO)
    {
        if(DrvL1_Nand_Init(NAND_PARTIAL_SHARE)==STATUS_OK){
            return NAND_PARTIAL_SHARE;
        } else if(DrvL1_Nand_Init(NAND_SHARE)==STATUS_OK) {
            return NAND_SHARE;
        } else if(DrvL1_Nand_Init(NAND_NON_SHARE)==STATUS_OK) {
            return NAND_NON_SHARE;
        } else {
            return NAND_NONE;
        }
    } else {
        if(DrvL1_Nand_Init(pad_mode)==STATUS_OK) {
            return pad_mode;
        } 
    }
    return NAND_NONE;
}


#if defined(NF0_USE_ISR)

#define NFC_STS_IS_BUSY 			(0x80) // (1<<7)
#define NFC_STS_INVALID_DESC 	(0x08) // (1<<3)

static wait_queue_head_t nf0_int_wait_queue;
static UINT32 nf0_int_status;
static bool nf0_int_inited = false;
static UINT32 nf0_int_wake_up_flag;

static irqreturn_t nf0_interrupt_isr(int irq, void *dev_id)
{
	unsigned int int_status = R_NAND_INTR_STS;
	
	R_NAND_INTR_STS = int_status; // clear bch interrupt
	
	#if 0
	if ((int_status & NFC_STS_IS_BUSY) != 0) {
		NF_HAL_DIAG("nf0 int while still busy. int_status[%x]nf0_int_status[%x]\n", int_status, nf0_int_status);
	}
	#endif
	
	nf0_int_status |= int_status;
	if (nf0_int_status & nf0_int_wake_up_flag)
		wake_up(&nf0_int_wait_queue);
	
	return IRQ_HANDLED;
}

SINT32 nf0_init_intr(void) 
{
	SINT32 ret = 0;
	
	printk("nf0_init_intr\n");
	if (!nf0_int_inited) {
	init_waitqueue_head(&nf0_int_wait_queue);
		ret = request_irq(IRQ_NAND0, nf0_interrupt_isr, IRQF_DISABLED, NF0_DEVICE_NAME, NULL);
		nf0_int_inited = true;
		printk("NF0_USE_ISR on. reg ret[%d]\n", ret);
	}
	return ret;
}

void nf0_uninit_intr(void)
{
	printk("nf0_uninit_intr\n");
	free_irq(IRQ_NAND0, NULL);
	nf0_int_inited = false;	
}

static void nf0_set_wait_up_flag(UINT32 wait_up_flag)
{
	nf0_int_status = 0;
	nf0_int_wake_up_flag = wait_up_flag;
}

#else
SINT32 nf0_init_intr(void)
{
	return 0;
}

void nf0_uninit_intr(void)
{
	return;
}

#endif

void NFC_Reset(void)
{
	volatile UINT32 k;
	volatile UINT32 save_FM_AC_TIMING;
	
	save_FM_AC_TIMING = rFM_AC_TIMING; // NFC reset will reset rFM_AC_TIMING to default, so keep original value to restore later
	// -- Release SW reset for BCH (bit 13) -- //
  // -- Release SW reset for NAND1 (bit 12) -- //
  // -- Release SW reset for NAND0 (bit 11) -- //
	rSCUA_PERI_RST      = 0x1800 ; // reset NF0, NF1, bit 11, bit 12
	rSCUA_PERI_RST      &= (~0x1800);
	
	for (k=0; k<0x2000; k++) {
		if ((rSCUA_PERI_RST & 0x1800) == 0) // strong reset complete
			break;
	}
	if (k == 0x2000)
		NF_HAL_DIAG("NFC hw reset timeout after[%d]\n", k);
	
	
	R_NAND_INTR_STS 		= 0xffff;   // Reset all interrupt flag for next using.	
	bch_reset();
	
	rFM_AC_TIMING = save_FM_AC_TIMING;
}

struct nand_dma_timing_rec {
	UINT16 tRP; 	// read active time
	UINT16 tREH;	// read recovery time
	UINT16 tWP;		// write active time
	UINT16 tWH;		// write recovery time
	UINT16 tADL;  // WE rising edge of final address cycle to the WE rising edge of first data cycle
	UINT16 tWB;   // WE high to busy
};	

static struct nand_dma_timing_rec nand_dma_timing_safe_for_all = {
	.tRP = 35*100,
	.tREH = 15*100,
	.tWP = 25*100,
	.tWH = 15*100,
	.tADL = 300*100,
	.tWB = 100*100
};

static struct nand_dma_timing_rec nand_dma_timing_safe_for_4K_8K_page = {
	.tRP = (15+10)*100, // according to experiment result, it need add 10 to work ok
	.tREH = 15*100,
	.tWP = 15*100,
	.tWH = 10*100,
	.tADL = 300*100,
	.tWB = 100*100
};

static struct nand_dma_timing_rec nand_dma_time_current = {
	.tRP = 35*100,
	.tREH = 15*100,
	.tWP = 25*100,
	.tWH = 15*100,
	.tADL = 300*100,
	.tWB = 100*100
};

// input nand timing output suggested timing steps
static UINT32 nandCalTimingStep(UINT32 nand_time_ns100)
{
	int clk_sys_ahb_freq = 0;
	UINT32 clk_sys_ahb_time_ns100;
	UINT32 suggest_step;
	
	gp_clk_get_rate((int*)"clk_sys_ahb", &clk_sys_ahb_freq);
	if (clk_sys_ahb_freq != 0) {
		clk_sys_ahb_time_ns100 = (100*1000)/(clk_sys_ahb_freq/(1000*1000));
		suggest_step = (nand_time_ns100 + clk_sys_ahb_time_ns100 - 1)/clk_sys_ahb_time_ns100;
	}
	else
		suggest_step = 5; /* safe nand dma timing */
	return suggest_step;
}

static UINT32 nandCalWaitNoStep(UINT32 nand_time_ns100)
{
	int clk_sys_ahb_freq = 0;
	UINT32 clk_sys_ahb_time_ns100;
	UINT32 suggest_step;
	
	gp_clk_get_rate((int*)"clk_sys_ahb", &clk_sys_ahb_freq);
	if (clk_sys_ahb_freq != 0) {
		clk_sys_ahb_time_ns100 = (100*1000)/(clk_sys_ahb_freq/(1000*1000));
		suggest_step = (nand_time_ns100 + clk_sys_ahb_time_ns100 - 1)/clk_sys_ahb_time_ns100;
	}
	else
		suggest_step = 0x28; /* safe tADL timing */
	return suggest_step;
}

void nandSetDmaTiming(struct nand_dma_timing_rec *timing, bool doWrite)
{
	UINT32 dma_timing;
	UINT32 dma_recovery;
	UINT32 dma_active;
	UINT32 tWB_step;
	UINT32 tADL_step;
	UINT32 wait_no;
	
	if (doWrite) {
		dma_recovery = nandCalTimingStep(timing->tWH); // write recovery
		if (dma_recovery > 0) dma_recovery = dma_recovery - 1;
			
		dma_active = nandCalTimingStep(timing->tWP); // write active
		if (dma_active > 0) dma_active = dma_active - 1;
			
		dma_timing = (dma_recovery << 4) | dma_active;
		
		//printk("write dma timing[%x]\n", dma_timing);
	}
	else {
		dma_recovery = nandCalTimingStep(timing->tREH); // read recovery
		if (dma_recovery > 0) dma_recovery = dma_recovery - 1;
			
		dma_active = nandCalTimingStep(timing->tRP); // read active
		if (dma_active > 0) dma_active = dma_active - 1;
			
		dma_timing = (dma_recovery << 4) | dma_active;
		//printk("read dma timing[%x]\n", dma_timing);
	}
	rFM_AC_TIMING = ( rFM_AC_TIMING & 0xffff00ff) | ( dma_timing << 8); // set dma timing part
	
	// set wait_no
	tWB_step = nandCalWaitNoStep(timing->tWB);
	tADL_step = nandCalWaitNoStep(timing->tADL);
	if (tADL_step >= 2) tADL_step -= 2;
	if (tADL_step >= (dma_recovery+1)) tADL_step -= (dma_recovery+1);
	wait_no = (tADL_step > tWB_step) ? tADL_step : tWB_step;
	if (wait_no > 0) wait_no -= 1;
		
	rFM_AC_TIMING = ( rFM_AC_TIMING & 0xff00ffff) | ( wait_no << 16); // set wait_no part
		
	//printk("%s, rFM_AC_TIMING[0x%08x]\n", doWrite? "write":"read", rFM_AC_TIMING);
}

void nandAdjustDmaTiming(void)
{
	if (gPhysical_NandInfo->wPageSize >= 4096) {
		nand_dma_time_current = nand_dma_timing_safe_for_4K_8K_page;
	}
	else
		nand_dma_time_current = nand_dma_timing_safe_for_all;
}

void nandModifyDmaTiming(UINT16 tRP, UINT16 tREH, UINT16 tWP, UINT16 tWH)
{
	nand_dma_time_current.tRP = tRP;
	nand_dma_time_current.tREH = tREH;
	nand_dma_time_current.tWP = tWP;
	nand_dma_time_current.tWH = tWH;
}

void nandModifytADLtWB(UINT16 tADL, UINT16 tWB)
{
	nand_dma_time_current.tADL = tADL;
	nand_dma_time_current.tWB = tWB;
}

SINT32 DrvL1_Nand_Init(NAND_PAD_ENUM nand_pad_set)
{                                                                                                                                                                                            
    UINT16 main_id;
	  UINT32 vendor_id;
    SINT32 ret;
    UINT32 scub_pgs0;		
		int clk_sys_ahb_freq = 0;
		
		if (!hal_buf_inited) { // william, to avoid allocate hal buffers more than 1 time
			Nand_Malloc_Buffer();
			hal_buf_inited = true;
		}
		
     // -- Turn on farbic A (bit 24) -- //
    rSCUC_PERI_CLKEN    = rSCUC_PERI_CLKEN | 0x01003800;
    //rSCUC_PERI_RST &= ~(1<<11);  // Reset low
    
    // active rdybsy/data pad input enable
    rSCUB_GPIO3_IE      = rSCUB_GPIO3_IE | 0x0107f800;

    // -- Enable BCH (bit 13) -- //
    // -- Enable NAND1 (bit 12) -- //
    // -- Enable NAND0 (bit 11) -- //
    rSCUA_PERI_CLKEN    = rSCUA_PERI_CLKEN | 0x01113800;
		
    scub_pgs0 = R_SCUB_PGS0;
   
 		#if defined(NF0_USE_ISR)
		nf0_int_status = 0;
		if (!nf0_int_inited) {
				NF_HAL_DIAG("nf0 int init fail\n");
				return -1;
			}
		#endif	
		
		gp_clk_get_rate((int*)"clk_sys_ahb", &clk_sys_ahb_freq);
		printk("clk_sys_ahb_freq[%d]\n", clk_sys_ahb_freq);
		
		NFC_Reset();
    rFM_CSR		  				= 0x09;
	  rFM_INTRMSK   			= 0x0008; 	// 0xf05d // william change
	  
	  rFM_AC_TIMING      	= 0x1f2f5502 ; 	// william: tADL min is 300, tWB max is 100, use max of between. so chnage the wait_no to 0x2f. 
	  																		// Note that when special command such as 2 plane read write, tDBSY is short, this 0x2f not suitable
	  																	 	
	  nand_dma_time_current = nand_dma_timing_safe_for_all;
	  nandSetDmaTiming(&nand_dma_time_current, 0);
	  
    if ((nand_reset()!=STATUS_OK) || (Nand_LiveResponse()==STATUS_FAIL) || (nand_read_id_all(&main_id, &vendor_id)!=STATUS_OK))
    {    		
        if (nand_pad_set==NAND_SHARE) {            // Full share            
            rFM0_SHARE_CTRL  = 0x01020107;
            rFM0_SHARE_CTRL1 = 0x000081ff;            
        } else if (nand_pad_set==NAND_PARTIAL_SHARE) {
            // Partial share            
            R_PINMUX_NEW |= (1<<13);   //B_KEYSCAN[7:4] is nand ctrl         
            rFM0_SHARE_CTRL  = 0x01020105;
            rFM0_SHARE_CTRL1 = 0x000081ff;						
        } else if (nand_pad_set==NAND_NON_SHARE){            // Non share            
            rFM0_SHARE_CTRL  = 0x00000000;
            rFM0_SHARE_CTRL1 = 0x00000000; 						
        } else if (nand_pad_set==NAND_NON_SHARE2) {            
            rFM0_SHARE_CTRL  = 0x00000000;
            rFM0_SHARE_CTRL1 = 0x00000000; 
            R_SCUB_PGS0 &= ~(0xFF); // GP0/1/2/3 = 0 to become Nand CTRL Pad
            R_SCUB_PGS0 |= 3<<0x8;  // GP4 (Bit[9:8]=3)												
        } else {            
            nand_pad=NAND_NONE;
            return -1;
        }
    } else {
        if (rFM0_SHARE_CTRL==0x01020107){
            nand_pad_set=NAND_SHARE;
        } else if (rFM0_SHARE_CTRL==0x01020105) {
            nand_pad_set=NAND_PARTIAL_SHARE;
        } else if (rFM0_SHARE_CTRL==0x00000000 && ((R_SCUB_PGS0 & (3<<8))==3) ) {
            nand_pad_set=NAND_NON_SHARE2;
        } else {
            nand_pad_set=NAND_NON_SHARE;
        }
    }

    nand_pad = nand_pad_set;

 
    NFC_Reset();
    rFM_CSR		  			= 0x09;
    rFM_INTRMSK   			= 0x0008; // 0xf05d // william change
    //// -- Set nand-flash ac timing -- //
    //rFM_AC_TIMING     = 0x1f2f5502 ;
		
    // gpio mode selection. PADGRP SEL2, bit 12 set to 1
    // (*(volatile unsigned *)(SCUB_XXX_BASE+0x88))    = 0x1000 ; // must remark
        
	/* NAND Reset */
	ret = nand_reset();
	if (ret!=STATUS_OK) {
	   rFM0_SHARE_CTRL  = 0x00000000;
	   rFM0_SHARE_CTRL1 = 0x00000000; 
	   R_SCUB_PGS0 = scub_pgs0;
	   return -1;
	}
	
	ret = Nand_LiveResponse();
	if (ret==STATUS_FAIL) {
      rFM0_SHARE_CTRL  = 0x00000000;
      rFM0_SHARE_CTRL1 = 0x00000000; 
      R_SCUB_PGS0 = scub_pgs0; 
      return -1;
  }
	
	ret = nand_read_id_all(&main_id, &vendor_id);   // 要做失敗判斷(最好在裡面做 OK?)
  if (ret!=STATUS_OK) {
      rFM0_SHARE_CTRL  = 0x00000000;
      rFM0_SHARE_CTRL1 = 0x00000000; 
      R_SCUB_PGS0 = scub_pgs0;
      return -1;
  }
  spare_buf_init();  
	return STATUS_OK;                                                                                                                                                                    
}

static UINT8 nf_handle_cnt=0; 

SINT32 Nand_Init(void)   // Nand normal initial, only init very basic issue
{             
		SINT32 ret = 0;
	
	 if (nf_handle_cnt==0)  // first nand driver initial
	  {
		  if(Nand_Pad_Scan(NAND_NON_SHARE)==NAND_NONE)
		  {
				 NF_HAL_DIAG ("==Nand_Init Nand_Pad_Scan Fail!!==\n");
		  }
		  else
		  {
			    NF_HAL_DIAG ("Debug,zurong>==Nand_Init Nand_Pad_Scan OK!!==\n");
		  }
	  }
	  nf_handle_cnt++;
		
		return ret;
} 

SINT32 Nand_UnInit(void)   // Nand normal initial, only init very basic issue
{             
		SINT32 ret = 0;
	
		nf_handle_cnt--;
	
		if (nf_handle_cnt==0)
		{
			  rSCUA_PERI_CLKEN &= ~(0x01113800);  // Disable Nand/BCH clock
		}
		
		return ret;
}

SINT16 good_block_check(UINT16 blk_id, UINT32 WorkBuffer)
{
   // ISP 在遇到壞塊後, 無條件將整個 Block 打成0, 如此有助於壞塊檢測
    UINT32 detect_page_id;
    UINT32 detect_page_id2;
    UINT32 tail_offset,tail_offset2;
    SINT16 ret;
		UINT32 ret1;
		SINT16 blkSts1;
		SINT16 blkSts2;
		SINT16 blkSts3;
		SINT16 blkSts4;
		
    detect_page_id = blk_id * nand_block_size_get();//blk_id*nand_page_size_get();
    detect_page_id2 = detect_page_id+1;
    tail_offset = nand_block_size_get()-1;
    tail_offset2 = nand_block_size_get()-2;
    
    //ret = Nand_ReadPhyPage(detect_page_id, WorkBuffer);
    ret1 = Nand_ReadPhyPage(detect_page_id, WorkBuffer);	
    ret = blkSts1 = (Nand_badblk_flag()&0xFF);
		if(ret1!=0)
		{
			NF_HAL_DIAG("Read1 page failed!! Page:0x%x <function:good_block_check> \n", detect_page_id); 
			goto L_TRY_2;
		}
	
    if (ret!=0xFF)
    {
        printk ("BadBlkF1[0x%04x] %d \r\n",ret, blk_id);
        goto GOOD_BLK_CHECK_DONE;
    }

L_TRY_2:
    ret1 = Nand_ReadPhyPage((detect_page_id+tail_offset), WorkBuffer);
    ret = blkSts2 = (Nand_badblk_flag()&0xFF);
		if(ret1!=0)
		{
			NF_HAL_DIAG("Read2 page failed!! Page:0x%x <function:good_block_check> \n", detect_page_id+tail_offset); 
			goto L_TRY_3;
		}
		
    if (ret!=0xFF)
    {
        printk ("BadBlkT1[0x%04x] %d \r\n",ret, blk_id);
        goto GOOD_BLK_CHECK_DONE;  
    }

L_TRY_3:
    ret1=Nand_ReadPhyPage((detect_page_id+tail_offset2), WorkBuffer);
    ret = blkSts3 = (Nand_badblk_flag()&0xFF);
		if(ret1!=0)
		{
			NF_HAL_DIAG("Read3 page failed!! Page:0x%x <function:good_block_check> \n", detect_page_id+tail_offset2); 
			goto L_TRY_4;
		}
		
    if (ret!=0xFF)
    {
        printk ("BadBlkT2[0x%04x] %d \r\n",ret, blk_id);
        goto GOOD_BLK_CHECK_DONE;
    }
    
    
L_TRY_4:
    ret1=Nand_ReadPhyPage((detect_page_id2), WorkBuffer);
    ret = blkSts4 = (Nand_badblk_flag()&0xFF);
		if(ret1!=0)
		{
			NF_HAL_DIAG("Read4 page failed!! Page:0x%x <function:good_block_check> \n", detect_page_id2); 
			goto L_TRY_END;
		}
    if (ret!=0xFF)
    {
        printk ("BadBlkF2[0x%04x] %d \r\n",ret, blk_id);
    }
		goto GOOD_BLK_CHECK_DONE;
		
L_TRY_END:
		if (blkSts1 == 0xFF && blkSts2 == 0xFF && blkSts3 == 0xFF && blkSts4 == 0xFF)
			ret = 0xFF;
		else {
			if (blkSts1 == 0x00 || blkSts2 == 0x00 || blkSts3 == 0x00 || blkSts4 == 0x00)
				ret = 0x00;
			else if (blkSts1 == 0x44 || blkSts2 == 0x44 || blkSts3 == 0x44 || blkSts4 == 0x44) // user bad
				ret = 0x44;
			else
				ret = blkSts1 & blkSts2 & blkSts3 & blkSts4;
		}
	
GOOD_BLK_CHECK_DONE:
    
    return ret;
    
}


SINT8 Nand_badblk_flag(void)
{
    UINT8 index=0;
    UINT8 bad_tag;
    
    if (nand_page_size_get()!=512) {
        index=0;
    } else {
        index=5;
        }

    bad_tag = nand_spare_buf[index];
#if 0
    if (bad_tag!=NF_GOOD_BLK_TAG) {
            spare_buf_init();
            return 1;   // Bad Block fail
        }
#endif		
    return bad_tag;
}


UINT32 Nand_RandomReadPhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer) 
{   
//    DRV_Reg32(FM0_PAGEADDR) = wPhysicPageNum; 
   return  Nand_ReadPhyPage(wPhysicPageNum, WorkBuffer);
}  


UINT16 spare_flag_get(void)
{
   UINT16* spare_short = (UINT16*) &nand_spare_buf[0];

#if 1
    if (nand_bch_get()!=BCH512B4_BITS_MODE)
    {
        return spare_short[gPhysical_NandInfo->wBchParityAlignSize/2];
    } else {
        return spare_short[0];
    }
#else
   switch (nand_bch_get())
   {
     case BCH2K32_BITS_MODE:  //Alignment 64Bytes
        return spare_short[32];
        break;
     case BCH2K48_BITS_MODE:   //Alignment 128 Bytes
        return spare_short[64];
        break;
     case BCH512B4_BITS_MODE:
        return spare_short[0];
     default:    
        return 0xFF;
        break;           
   }
#endif

}
#define ID_TBL2_SIZE 24

NAND_STATUS_ENUM nand_intelligent_id_init(UINT16 main_id, UINT32 vendor_id)
{
    UINT8 chip_id;
    UINT16 j;
    UINT8 NAND_ID_Table[ID_TBL2_SIZE] =                                                                                                                     
    {                                                                                                                                                             
        /*   ChipID  */                                                                                                      
     /*00*/   0x73,   //16MB                                                                                                
     /*01*/   0x58,   //16MB  // Nand ROM 代表                                                                                               
     /*02*/   0x74,   //16MB                                                                                                
     /*03*/   0x34,   //16MB                                                                                                
     /*04*/   0x75,   //32MB                                                                                                 
     /*05*/   0x35,   //32MB                                                                                                                                                                                                 	
     /*06*/   0x76,   //64MB                                                                                                
     /*07*/   0x36,   //64MB	                                                                                              
     /*08*/   0x5A,   //64MB	                                                                                              
     /*19*/   0x79,   //128MB                                                                                               
     /*%10*/  0x39,   //128MB
     /*%11*/  0xF1,   //128MB
     /*12*/   0xD3,   //1GB 
     /*13*/   0xD5,   //2GB   // default for toshiba , Old or New Spec                                                                                                   
     /*14*/   0xCA,   //256MB
     /*15*/   0xDA,   //256MB                                                                                               
     /*16*/   0x71,   //256MB                                                                                               
     /*17*/   0xDC,   //512MB                                                                                               
     /*18*/   0xD7,   //4GB //Old or New Spec 
     /*19*/   0xD9,   //4GB //Old or New Spec 
     /*%20*/  0x48,   //2GB   //New Spec                                                                                             	
     /*21*/   0x68,   //4GB   //New Spec 
     /*22*/   0x88,   //8GB   //New Spec 
     /*23*/   0xFF    //NONE                                                                                              
    };

    NTYPE=0xff;

    chip_id = ((main_id & 0xFF00)>>8);   /*Chip id is linear mapping to size*/

    if (chip_id==0x00) {
        return NAND_FAIL;
    }

    for (j = 0; j<ID_TBL2_SIZE ; j++)      /* Get Nand ID table */                                                                                                                                                                                                                                               
    {                                                                                                                                                         
      if ( NAND_ID_Table[j] == chip_id)                                                                                                                   
      {                                                                                                                                                     
          NTYPE = j;   /* NAND_ID_Table[NTYPE] is the nand parameter table for driver*/                                                                     
    	  break;                                                                                                                                            
      }                                                                                                                                                     
    }   

    if(NTYPE > LAST_SMALLPAGE_TBLID) {
        //large page
        nand_page_size_set(1024);
        gPhysical_NandInfo->wBlkPageNum=64;
        gPhysical_NandInfo->wAddrCycle=5;
        nand_bch_set(BCH1K60_BITS_MODE);
    } else {
        //small page
        nand_small_page_cs_pin_reg(0);
        nand_page_size_set(512);
        gPhysical_NandInfo->wBlkPageNum=32;
        gPhysical_NandInfo->wAddrCycle=5;
        nand_bch_set(BCH512B4_BITS_MODE);        
    }
	
	return NAND_TRUE;
}

UINT16 nand_page_size_get(void) 
{
    return gPhysical_NandInfo->wPageSize;
}

void nand_page_size_set(UINT16 page_size) 
{
    gPhysical_NandInfo->wPageSize 			 = page_size;
    gPhysical_NandInfo->wPageSectorSize = page_size/512;
}

void nand_addr_cycle_set(UINT16 addr_cycle) 
{
    gPhysical_NandInfo->wAddrCycle=addr_cycle;
}

UINT16 nand_block_size_get(void) 
{
    return gPhysical_NandInfo->wBlkPageNum;
}

UINT32 nand_page_nums_per_block_get(void)
{
    return gPhysical_NandInfo->wBlkPageNum;
}

void nand_page_nums_per_block_set(UINT16 blk_page_size)
{
    gPhysical_NandInfo->wBlkPageNum = blk_page_size;
}

void Nand_Getinfo(UINT16* PageSize,UINT16* BlkPageNum,UINT32* NandBlockNum)
{
	*PageSize     = gPhysical_NandInfo->wPageSize;
	*BlkPageNum   = gPhysical_NandInfo->wBlkPageNum;
	*NandBlockNum = gPhysical_NandInfo->wNandBlockNum;
//	*NandID       = gPhysical_NandInfo->wNandID;
}

void nand_reg_init(void)
{
   
}

void Nand_Malloc_Buffer(void)
{
		printk("william:Nand_Malloc_Buffer\n");
		DescVector = (UINT32*)kmalloc(4096, GFP_DMA);
		if(!DescVector) {
			NF_HAL_DIAG("fillDescriptor: can't kmalloc\n");
		}else{
			//NF_HAL_DIAG ("kmalloc DescVector addr:0x%x\n",DescVector);
		}
		memset(DescVector, 0, 4096);
		
		
		nf_id = (UINT8 *)kmalloc(4096, GFP_DMA);
		if(!nf_id) {
			NF_HAL_DIAG("nf_id: can't kmalloc\n");
		}else{
			//NF_HAL_DIAG ("kmalloc nf_id addr:0x%x\n",nf_id);
		}
		memset(nf_id, 0, 4096);
		
		nand_spare_buf = (UINT8 *)kmalloc(4096, GFP_DMA);
		if(!nand_spare_buf) {	
			printk("nand_spare_buf: can't kmalloc\n");
		}else{
			//printk ("nand_spare_buf kmalloc addr:0x%x\n",(UINT32)nand_spare_buf);
		}
		memset(nand_spare_buf, 0, 4096);	
		
	#if defined(NF_DATA_DEBUG_WRITE_TO_NON_FF_PAGE) // william
	debugWorkBuffer = (void *)__get_free_pages(GFP_DMA, get_order(8192));
	if(debugWorkBuffer==0)
	{
		printk("debugWorkBuffer malloc failed!!! \n");
	}
	#endif
}

void fillDescriptor(ppara_t pstPara, UINT8 DescId)
{

    UINT32 desc_addr;
    UINT32 *Desc_ptr;

    if (DescId>3) {
        return;
    }

    Desc_ptr = (UINT32*) &DescVector[8*DescId];


    pstPara->u8CMDType<<=4;

    Desc_ptr[0] = (pstPara->u8CMDType + pstPara->u8MultiFunc)<<24 | (pstPara->u8ReadStatusData)<<16 | (pstPara->u8CMD0)<<8 | pstPara->u8CMD1;
    Desc_ptr[1] = (pstPara->u16PyLdLen)<<16 | (pstPara->u16ReduntLen) ;
    Desc_ptr[2] = (pstPara->u16InterruptMask)<<16 |  (pstPara->u16InterruptStatus);
		
		#if defined(NF0_USE_ISR)
		nf0_set_wait_up_flag(pstPara->u16InterruptMask);
		#endif
		
    pstPara->u8Owner<<=7;
    pstPara->u8End<<=6;
    pstPara->u8LstSec<<=5;
    pstPara->u8RedEn<<=4;
    Desc_ptr[3] = (pstPara->u8Owner|pstPara->u8End|pstPara->u8LstSec|pstPara->u8RedEn|pstPara->u8AddrNo)<<24\
                                    |(pstPara->u8Addr4)<<16 | (pstPara->u8Addr3)<<8 |pstPara->u8Addr2;
    Desc_ptr[4] = (pstPara->u8Addr1)<<24 | (pstPara->u8Addr0)<<16 |pstPara->u16NextDesBP;

    Desc_ptr[5] = pstPara->u32PyLdBP;

    Desc_ptr[6] = pstPara->u32ReduntBP;

    Desc_ptr[7] = (pstPara->u16Redunt_Sector_Addr_Offset<<16) | pstPara->u16Redunt_Sector_Len;

    desc_addr = (UINT32) &DescVector[0];

		desc_addr = (UINT32)virt_to_phys(&DescVector[0]);
	
		gp_clean_dcache_range((unsigned int)&DescVector[8*DescId],32);
		
		R_NAND_DESC_BA = desc_addr; 
}
				
SINT32 Nand_phy_read(UINT32 page_addr, UINT32 buf_addr) 
{
//    UINT8  u8MultiFunc = 0x01;
    UINT16 u16PyLdLen = gPhysical_NandInfo->wPageSize;
    UINT16 u16ReduntLen = gPhysical_NandInfo->wSpareSize;
    #if defined(NF0_USE_ISR)
    UINT16 u16InterruptMask = ND_INTR_DESC_INVALID;
    #else
    UINT16 u16InterruptMask = 0 ;
    #endif
    UINT8  u8End=1;
    UINT32 spare_addr;
    UINT32 nand_csr;
    #if !defined(USE_TIME_FOR_TIMEOUT)
    UINT32 loop_cnt=0;
    #endif
    unsigned long t, ms_timeout;
    DescPara DescParas;
    UINT16 u16NextDesBP = 0x00;
    UINT32 buf_addr_virt = buf_addr;		
		UINT32 spare_addr_virt = (UINT32)&nand_spare_buf[0];    
  	
  	//NF_HAL_DIAG ("Nand physical read\n");
  	nandSetDmaTiming(&nand_dma_time_current, 0);
  	
    //NF_HAL_DIAG ("nand_spare_buf virt addr:0x%x\n",&nand_spare_buf[0]);
		spare_addr = (UINT32)virt_to_phys(&nand_spare_buf[0]);
		
	 //NF_HAL_DIAG ("nand_spare_buf virt addr:0x%x\n",buf_addr);
		buf_addr = (UINT32)virt_to_phys((UINT32*)buf_addr);
		//NF_HAL_DIAG ("nand_spare_buf phy addr:0x%x\n",buf_addr);
    
    DescParasReset((DescPara *) &DescParas);
    if (gPhysical_NandInfo->wPageSize>512) {  /*Large Page*/
        DescParas.u8CMDType = 0x01;  // AutoMode LargePage Read
        DescParas.u8Addr0 = 0x00;
        DescParas.u8Addr1 = 0x00;
        DescParas.u8Addr2 = (page_addr&0xff);
        DescParas.u8Addr3 = (page_addr>>8)&0xff;
        DescParas.u8Addr4 = (page_addr>>16)&0xff;
        nand_csr=0x19; // DMA Burst8
    } else {   /*small page*/
    	
    	//#define iROM_DBG_MODE7  0   	// Nand Burning Test Enable
      //#if iROM_DBG_MODE7==0
      #if 1
        DescParas.u8CMDType = 0x0d;  // AutoMode SmallPage Read
      #else
        DescParas.u8CMDType = 0x01;  // AutoMode SmallPage Read
      #endif
        DescParas.u8Addr0 = 0x00;
        DescParas.u8Addr1 = (page_addr&0xff);
        DescParas.u8Addr2 = (page_addr>>8)&0xff;
        DescParas.u8Addr3 = (page_addr>>16)&0xff;
        DescParas.u8Addr4 = (page_addr>>24)&0xff;
        nand_csr=0x11; // DMA Burst4
    }


//	DescParas.u8MultiFunc            = u8MultiFunc; //huck
		DescParas.u8ReadStatusData       = 0x00;
		DescParas.u8CMD0                 = 0x00;
		DescParas.u8CMD1                 = 0x30;
		
		DescParas.u16PyLdLen             = u16PyLdLen;
		DescParas.u16ReduntLen           = u16ReduntLen;
		
		DescParas.u16InterruptMask       = u16InterruptMask;
		DescParas.u16InterruptStatus     = 0x00;
		
		DescParas.u8Owner                = 1;
		DescParas.u8End                  = u8End;
		DescParas.u8LstSec               = 1;
		DescParas.u8RedEn                = 1;
		DescParas.u8AddrNo               = gPhysical_NandInfo->wAddrCycle;
		DescParas.u16NextDesBP           = u16NextDesBP;
		DescParas.u32PyLdBP              = buf_addr;
		DescParas.u32ReduntBP            = spare_addr;

			
    DescParas.u16Redunt_Sector_Len  = gPhysical_NandInfo->wParitySize; 
    DescParas.u16Redunt_Sector_Addr_Offset = gPhysical_NandInfo->wBchParityAlignSize;

		nand_cs_io_low();
    fillDescriptor((ppara_t)&DescParas,0);

		gp_invalidate_dcache_range((unsigned long)buf_addr_virt,nand_page_size_get());
    gp_invalidate_dcache_range((unsigned long)spare_addr_virt,1024);  
	
		R_NAND_INTR_STS = 0xffff;
		R_NAND_CSR 			= nand_csr;
    
    #if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Read TimeOut:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
    #else
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
    t=jiffies;
    while((R_NAND_INTR_STS&0x8)!=0x8) {        // invalid desc interrupt     
    		#if !defined(USE_TIME_FOR_TIMEOUT)      
        loop_cnt++;   
        if (loop_cnt > 0x1C000) {
        #else
        if (time_after(jiffies, t + ms_timeout)) {
        #endif
            NF_HAL_DIAG("Read TimeOut:%lu\n", jiffies - t);                
            R_NAND_INTR_STS = 0xffff;
            nand_cs_io_high();
            NFC_Reset();
            return -1;
        }
    }
		#endif
		
    R_NAND_INTR_STS = 0xffff;
    
    nand_cs_io_high();
    return 0;
}

UINT32 nand_total_MByte_size_get(void)
{
    return nand_total_MByte_size;
}

void nand_total_MByte_size_set(UINT32 nand_MB_size)
{
    UINT32 blk_Sector_size;
    UINT32 nand_total_Sectors;
    nand_total_MByte_size=nand_MB_size;
   
    blk_Sector_size = gPhysical_NandInfo->wPageSize*nand_page_nums_per_block_get()/512;
    nand_total_Sectors = nand_total_MByte_size*2048;  // each MB has 2048 Sectors

    if (blk_Sector_size !=0 ) {
        gPhysical_NandInfo->wNandBlockNum = nand_total_Sectors/blk_Sector_size; 
    }
}

void nand_wp_pin_reg(UINT8 gpio_pin_number)
{
   	nand_wp_io = gpio_pin_number;
}


void DescParasReset(DescPara *DescAddr)
{
	 DescAddr->u8CMDType=0x00;
	 DescAddr->u8MultiFunc=Nand_Select+0x01;   //CS0 mapping to 1, CS1 mapping to 2
	 DescAddr->u8ReadStatusData=0x00;
	 DescAddr->u8CMD0=0x00;
	 DescAddr->u8CMD1=0x00;
	 DescAddr->u16PyLdLen=0x0000;
	 DescAddr->u16ReduntLen=0x0000;
	 DescAddr->u16InterruptMask=0x0000;
	 DescAddr->u16InterruptStatus=0x0000;
	 DescAddr->u8Owner=0x00;
	 DescAddr->u8End=0x00;
	 DescAddr->u8LstSec=0x00;
	 DescAddr->u8RedEn=0x00;
	 DescAddr->u8AddrNo=0x00;
	 DescAddr->u8Addr0=0x00;
	 DescAddr->u8Addr1=0x00;
	 DescAddr->u8Addr2=0x00;
	 DescAddr->u8Addr3=0x00;
	 DescAddr->u8Addr4=0x00;
	 DescAddr->u16NextDesBP=0x0000;
	 DescAddr->u32PyLdBP=0x00000000;
	 DescAddr->u32ReduntBP=0x00000000;
	 DescAddr->u16Redunt_Sector_Addr_Offset=0x0000;
	 DescAddr->u16Redunt_Sector_Len=0x0000;
}

SINT32 Nand_read_Status(void) 
{
#if 1
    return Nand_Fast_Status_Get();
#else
    UINT8  u8End=1;
    #if !defined(USE_TIME_FOR_TIMEOUT)
   	UINT32 loop_cnt=0;
   	#endif
    unsigned long t, ms_timeout;
    DescPara    DescParas;
    ppara_t pstPara;
    UINT16 u16NextDesBP = 0x00;
//    UINT8  u8MultiFunc = 0x01 ; 

		nandSetDmaTiming(&nand_dma_time_current, 0);
		
    DescParasReset(&DescParas);

    pstPara = (ppara_t) &DescParas;

    DescParas.u8CMDType              = 0x04;
//    DescParas.u8MultiFunc            = 1; //huck
    DescParas.u8CMD0                 = 0x70;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u8Owner                = 1;   // HW
    DescParas.u8End                  = 1;
    DescParas.u8LstSec               = 1;

		#if defined(NF0_USE_ISR)
		DescParas.u16InterruptMask = ND_INTR_DESC_INVALID;
		#endif
		
		nand_cs_io_low();
    DRV_LOOP_DELAY(1);
    
    fillDescriptor(&DescParas,0);
    
    R_NAND_INTR_STS 		= 0xffff;
    R_NAND_CSR         	= 0x19 ;

 		#if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Read Status Time Out:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
  	#else   
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 50 ms */
    t=jiffies;
    while((R_NAND_INTR_STS&0x8)!=0x8) {        // invalid desc interrupt
    		#if !defined(USE_TIME_FOR_TIMEOUT)
        loop_cnt++;
        if (loop_cnt>=0x2000) {
        #else
       	if (time_after(jiffies, t + ms_timeout)) {
       	#endif
            NF_HAL_DIAG ("Read Status Time Out:%lu\n", jiffies - t);
            R_NAND_INTR_STS = 0xffff;
            nand_cs_io_high();
            NFC_Reset();
            return -1;
        }
    }
		#endif
		
    R_NAND_INTR_STS = 0xffff;
    nand_cs_io_high();

    return  ((DescVector[0]>>16) & 0xFF);
    #endif
}

SINT32 Nand_LiveResponse(void) 
{
		#if !defined(USE_TIME_FOR_TIMEOUT)
    UINT32 loop_cnt=0;
    #endif
    unsigned long t, ms_timeout;
    DescPara    DescParas;
    //ppara_t pstPara;
    //UINT16 u16NextDesBP = 0x00;
   // UINT8  u8MultiFunc = 0x01 ; 

		nandSetDmaTiming(&nand_dma_time_current, 0);
		
    DescParasReset(&DescParas);

    //pstPara = (ppara_t) &DescParas;

    DescParas.u8CMDType              = 0x04;
//    DescParas.u8MultiFunc            = 1; //huck
    DescParas.u8CMD0                 = 0x70;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u8Owner                = 1;   // HW
    DescParas.u8End                  = 1;
    DescParas.u8LstSec               = 1;
		#if defined(NF0_USE_ISR)
		DescParas.u16InterruptMask = ND_INTR_DESC_INVALID;
		#endif
		
		nand_cs_io_low();
    DRV_LOOP_DELAY(1);
    
    fillDescriptor(&DescParas,0);
    
    R_NAND_INTR_STS 		= 0xffff;
    R_NAND_CSR         	= 0x19 ;

		#if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Read Status Time Out:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
  	#else    
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 50 ms */
    t=jiffies;
    while((R_NAND_INTR_STS&ND_INTR_DESC_INVALID)!=ND_INTR_DESC_INVALID) {        // invalid desc interrupt
    		#if !defined(USE_TIME_FOR_TIMEOUT)
    		loop_cnt++;
        if (loop_cnt>=0x2000) {
        #else
       	if (time_after(jiffies, t + ms_timeout)) {
       	#endif
        	NF_HAL_DIAG ("Read Status Time Out:%lu\n", jiffies - t);
        	R_NAND_INTR_STS = 0xffff;
        	nand_cs_io_high();
        	NFC_Reset();
        	return -1;
        }
    }
		#endif
		
    R_NAND_INTR_STS = 0xffff;
    nand_cs_io_high();

    return  ((DescVector[0]>>16) & 0xFF);
}



UINT8 Nand_Fast_Status_Get(void)
{
    UINT32 ret=0;
    #if !defined(USE_TIME_FOR_TIMEOUT)
    UINT32 loop_cnt=0;
    #endif
   	unsigned long t, ms_timeout;
   	
   	nandSetDmaTiming(&nand_dma_time_current, 0);
   	
		R_NAND_FAST_STATUS_CTRL = 0x01;
		
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 50 ms */
    t=jiffies;
    while((R_NAND_FAST_STATUS_FLAG&0x01)!=0x01) {        // invalid desc interrupt
    		#if !defined(USE_TIME_FOR_TIMEOUT)
        loop_cnt++;
        if (loop_cnt>0x4000) {
        #else
        if (time_after(jiffies, t + ms_timeout)) {
        #endif
            NF_HAL_DIAG("Read Status Time Out:%lu\n", jiffies - t);
            R_NAND_INTR_STS = 0xffff;
            nand_cs_io_high();
            NFC_Reset();
            return 0xFF;
        }
    }

    ret = R_NAND_FAST_STATUS;                           // read status buffer
		R_NAND_FAST_STATUS_FLAG = 0x01;      

    return ret;
}

SINT32 nand_reset(void)
{	
		unsigned long t, ms_timeout;
		#if !defined(USE_TIME_FOR_TIMEOUT)
    SINT32 loop_cnt=0;
    SINT32 loop_cnt2=0;
    #endif
    DescPara    DescParas;
    ppara_t pstPara;
		unsigned save_rFM_INTRMSK;
		
    DescParasReset(&DescParas);

    pstPara = (ppara_t) &DescParas;
		
    pstPara->u8CMDType      = 0x07;  // Manual Command Mode
//  DescParas.u8MultiFunc   = 1; //huck
    pstPara->u8CMD0         = 0xFF;
    pstPara->u8CMD1         = 0x00;
    pstPara->u8Owner        = 1;   // HW
    pstPara->u8End          = 1;
    pstPara->u8LstSec       = 1;
		
		#if defined(NF0_USE_ISR)
		pstPara->u16InterruptMask = ND_INTR_DESC_RB1;
		#endif

		
		nand_cs_io_low();
    DRV_LOOP_DELAY(100);
    
    fillDescriptor(pstPara,0);

		save_rFM_INTRMSK = rFM_INTRMSK;
		#if defined(NF0_USE_ISR)
		rFM_INTRMSK  		 	= 0xf000; // turn on RDY, manual mode IC dont wait RDY, invalid will come first then ready, so wait ready
		#else
		rFM_INTRMSK  		 	= 0xf008; // turn on RDY, Invalid interrupt
		#endif
		R_NAND_INTR_STS 	= 0xffff;   // Reset all interrupt flag for next using.
    R_NAND_CSR    		= 0x19 ;
    
    #if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & ND_INTR_DESC_RB1, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG ("Reset Ready Pin Polling TimeOut:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      rFM_INTRMSK = save_rFM_INTRMSK;
      return -1;
  	}
    #else
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 50 ms */
    t=jiffies;
    // Description table process ready polling
    while((R_NAND_INTR_STS&ND_INTR_DESC_INVALID)!=ND_INTR_DESC_INVALID) {        // invalid desc interrupt
    		#if !defined(USE_TIME_FOR_TIMEOUT)
        loop_cnt++;
        if (loop_cnt>0x2000) {
        #else
        if (time_after(jiffies, t + ms_timeout)) {
        #endif
        		NF_HAL_DIAG ("Reset TimeOut:%lu\n",jiffies - t);
        		R_NAND_INTR_STS = 0xffff;
            nand_cs_io_high();
            NFC_Reset();
            rFM_INTRMSK = save_rFM_INTRMSK;
            return -1;
        }
    }

    // Ready Pin Polling
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 200 ms */
    t=jiffies;
		while((R_NAND_INTR_STS& ND_INTR_DESC_RB1)!= ND_INTR_DESC_RB1) {
				#if !defined(USE_TIME_FOR_TIMEOUT)
        loop_cnt2++;
        if (loop_cnt2>0x38000) {
        #else
        if (time_after(jiffies, t + ms_timeout)) {
        #endif
        		NF_HAL_DIAG ("Reset Ready Pin Polling TimeOut:%lu\n", jiffies - t);
        		R_NAND_INTR_STS = 0xffff;
            nand_cs_io_high();
            NFC_Reset();
            rFM_INTRMSK = save_rFM_INTRMSK;
            return -1;
        }
    }
		#endif
    
    R_NAND_INTR_STS = 0xffff;   // Reset all interrupt flag for next using.
		rFM_INTRMSK = save_rFM_INTRMSK;
    nand_cs_io_high();
    
    return STATUS_OK;
}

#if 1
SINT32 nand_read_id_all(UINT16* MainID, UINT32* VendorID)
{
		unsigned long t, ms_timeout;
		#if !defined(USE_TIME_FOR_TIMEOUT)
  	SINT32 loop_cnt=0;	
  	#endif
//  UINT32 desc_addr;
   
   	UINT32 id_buf = (UINT32)&nf_id[0];
   	
   	nandSetDmaTiming(&nand_dma_time_current, 0);
   	
   	memset(nf_id,0,4096);
   	
	//	NF_HAL_DIAG ("id_buf virt:0x%x\n",id_buf);		
		id_buf = (UINT32)virt_to_phys(nf_id);
	//	NF_HAL_DIAG ("id_buf phys:0x%x\n",id_buf);		
		
    DescParasReset(&DescParas);  // Initial firstd pre-descript 
    DescParas.u8CMDType              = CMD_TYPE_MANUAL_MODE_CMD;  // Manual Mode Command
//  DescParas.u8MultiFunc   				 = 1; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x90;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u16PyLdLen             = 0;
    DescParas.u16ReduntLen           = 0;  //256  //16 bits
    DescParas.u16InterruptMask       = 0;
    DescParas.u16InterruptStatus     = 0x00;
    DescParas.u8Owner                = 1;
    DescParas.u8End                  = 0;
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = 0;
    DescParas.u16NextDesBP           = 32; // Manual Mode Next 2'nd descript from offset 32Byte
    DescParas.u32PyLdBP              = 0;
    DescParas.u32ReduntBP            = 0;
    fillDescriptor((ppara_t)&DescParas,0);

    DescParasReset(&DescParas);  // Initial 2'nd pre-descript 
    DescParas.u8CMDType              = CMD_TYPE_MANUAL_MODE_ADDR;  // Manual Mode Command
//  DescParas.u8MultiFunc            = 0x01; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x00;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u16PyLdLen             = 0;
    DescParas.u16ReduntLen           = 0;  //256  //16 bits
    DescParas.u16InterruptMask       = 0;
    DescParas.u16InterruptStatus     = 0x00;
    DescParas.u8Owner                = 1;
    DescParas.u8End                  = 0;  // Manual Mode Resume
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = 0;
    DescParas.u16NextDesBP           = 64;  // Manual Mode Next 3'nd descript from offset 64Byte
    DescParas.u32PyLdBP              = 0;
    DescParas.u32ReduntBP            = 0;
    fillDescriptor((ppara_t)&DescParas,1);

    DescParasReset(&DescParas);  // Initial 3'rd pre-descript 
    DescParas.u8CMDType              = CMD_TYPE_MANUAL_MODE_PYLOAD_READ;  // Manual Mode Command
//  DescParas.u8MultiFunc            = 0x01; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x00;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u16PyLdLen             = 6;
    DescParas.u16ReduntLen           = 0;  //256  //16 bits
    DescParas.u16InterruptMask       = ND_INTR_DESC_INVALID;
    DescParas.u16InterruptStatus     = 0x00;
    DescParas.u8Owner                = 1;
    DescParas.u8End                  = 1;  // Manual Mode End
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = 0;
    DescParas.u16NextDesBP           = 0;
    //DescParas.u32PyLdBP              = id_buf;
    DescParas.u32PyLdBP              = NFTransMemGet(id_buf, 1); // force DMA domain address 
    DescParas.u32ReduntBP            = 0;
    
    nand_cs_io_low();
    DRV_LOOP_DELAY(1);
    
    fillDescriptor((ppara_t)&DescParas,2);
	
		gp_invalidate_dcache_range((unsigned int)nf_id,32);
		
		R_NAND_INTR_STS = 0xffff;
    R_NAND_CSR 			= 0x19 ;

		#if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Nand ID Read  TimeOut:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
  	#else
		ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
		t=jiffies;
    // Description table process ready polling
    while((R_NAND_INTR_STS&ND_INTR_DESC_INVALID)!=ND_INTR_DESC_INVALID) {        // invalid desc interrupt
      #if !defined(USE_TIME_FOR_TIMEOUT)
      loop_cnt++;
      if (loop_cnt>0x1C000) {
      #else
    	if (time_after(jiffies, t + ms_timeout)) {
    	#endif
    		NF_HAL_DIAG ("Nand ID Read TimeOut:%lu\n",jiffies - t);
    		R_NAND_INTR_STS = 0xffff;   // Reset all interrupt flag for next using.
    		nand_cs_io_high();
    		NFC_Reset();
        return -1;
      }
    }
    #endif
    
    R_NAND_INTR_STS = 0xffff;   // Reset all interrupt flag for next using.
    nand_cs_io_high();

	//nf_id->Vendor = 0x0014b674;

    *MainID = nf_id[0] | (nf_id[1] << 8);
    *VendorID = nf_id[2] | (nf_id[3]<<8) | (nf_id[4]<<16) | (nf_id[5] <<24);//nf_id->Vendor;
    
    NF_HAL_DIAG ("Nand MainID:0x%x\n",*MainID);
		NF_HAL_DIAG ("Nand VendorID:0x%x\n",*VendorID);
    
    if (((*MainID+*VendorID)==0x0000) || (*MainID==0xFFFF)) {  
    		NF_HAL_DIAG ("Nand ID Read Fail!!\n");
        return -1;
    }
    
    return 0;
}
#else
SINT32 nand_read_id_all(UINT16* MainID, UINT32* VendorID)
{
		unsigned long t, ms_timeout;
 // DescPara DescParas;  // Manual Mode Need 3-Descripters
 		#if !defined(USE_TIME_FOR_TIMEOUT)
    SINT32 loop_cnt=0;
    SINT32 loop_cnt2=0;
  	#endif
    UINT16 id_temp[3];
    UINT16 *short_buf=(UINT16 *) &id_temp[0];
    UINT32 *word_buf=(UINT32 *) &id_temp[1];    
    UINT32 id_buf = (UINT32) &id_temp[0];

		nandSetDmaTiming(&nand_dma_time_current, 0);
		
    DescParasReset(&DescParas);  // Initial firstd pre-descript 
    DescParas.u8CMDType              = CMD_TYPE_MANUAL_MODE_CMD;  // Manual Mode Command
//  DescParas.u8MultiFunc            = 0x01; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x90;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u16PyLdLen             = 0;
    DescParas.u16ReduntLen           = 0;  //256  //16 bits
    DescParas.u16InterruptMask       = 0;
    DescParas.u16InterruptStatus     = 0x00;
    DescParas.u8Owner                = 1;
    DescParas.u8End                  = 0;
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = 0;
    DescParas.u16NextDesBP           = 32; // Manual Mode Next 2'nd descript from offset 32Byte
    DescParas.u32PyLdBP              = 0;
    DescParas.u32ReduntBP            = 0;
    fillDescriptor((ppara_t)&DescParas,0);

    DescParasReset(&DescParas);  // Initial 2'nd pre-descript 
    DescParas.u8CMDType              = CMD_TYPE_MANUAL_MODE_ADDR;  // Manual Mode Command
//  DescParas.u8MultiFunc            = 0x01; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x00;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u16PyLdLen             = 0;
    DescParas.u16ReduntLen           = 0;  //256  //16 bits
    DescParas.u16InterruptMask       = 0;
    DescParas.u16InterruptStatus     = 0x00;
    DescParas.u8Owner                = 1;
    DescParas.u8End                  = 0;  // Manual Mode Resume
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = 0;
    DescParas.u16NextDesBP           = 64;  // Manual Mode Next 3'nd descript from offset 64Byte
    DescParas.u32PyLdBP              = 0;
    DescParas.u32ReduntBP            = 0;
    fillDescriptor((ppara_t)&DescParas,1);

    DescParasReset(&DescParas);  // Initial 3'rd pre-descript 
    DescParas.u8CMDType              = CMD_TYPE_MANUAL_MODE_PYLOAD_READ;  // Manual Mode Command
//  DescParas.u8MultiFunc            = 0x01; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x00;
    DescParas.u8CMD1                 = 0x00;
    DescParas.u16PyLdLen             = 6;
    DescParas.u16ReduntLen           = 0;  //256  //16 bits
    DescParas.u16InterruptMask       = ND_INTR_DESC_INVALID;
    DescParas.u16InterruptStatus     = 0x00;
    DescParas.u8Owner                = 1;
    DescParas.u8End                  = 1;  // Manual Mode End
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = 0;
    DescParas.u16NextDesBP           = 0;
    DescParas.u32PyLdBP              = id_buf; // force DMA domain address
    DescParas.u32ReduntBP            = 0;
    fillDescriptor((ppara_t)&DescParas,2);
	
		nand_cs_io_low();
    DRV_LOOP_DELAY(1);
		gp_invalidate_dcache_range((unsigned int)nf_id,32);
	
		R_NAND_INTR_STS 		= 0xffff;
    R_NAND_CSR         	= 0x19 ;

		#if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Nand ID Read  TimeOut:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
  	#else
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
		t=jiffies;
    // Description table process ready polling
    while((R_NAND_INTR_STS&ND_INTR_DESC_INVALID)!=ND_INTR_DESC_INVALID) {        // invalid desc interrupt
     	#if !defined(USE_TIME_FOR_TIMEOUT)
     	loop_cnt++;
     	if (loop_cnt>0x1C000) {
     	#else
    	if (time_after(jiffies, t + ms_timeout)) {
    	#endif
    		 NF_HAL_DIAG ("Nand ID Read TimeOut:%lu\n", jiffies - t);
    		R_NAND_INTR_STS = 0xffff;   // Reset all interrupt flag for next using.
    		nand_cs_io_high();
    		NFC_Reset();
        return -1;
      }
    }
    #endif
    
    R_NAND_INTR_STS = 0xffff;   // Reset all interrupt flag for next using.
    nand_cs_io_high();

    *MainID = short_buf[0];
    *VendorID = word_buf[0];
    nf_id->Main=*MainID;
    nf_id->Vendor=*VendorID;
    if (((nf_id->Main+nf_id->Vendor)==0x0000) || (nf_id->Main==0xFFFF)) {  
        return -1;
    }
    
    
	NF_HAL_DIAG ("Nand MainID:0x%x\n",*MainID);
	NF_HAL_DIAG ("Nand VendorID:0x%x\n",*VendorID);    
    return 0;
}
#endif


void nand_cs_io_init(void)
{
	#if 0
    DRV_Reg32(0x90005144) |= 0x8; // Release CS1 to GIPI from nand controller
    DRV_Reg32(0x9000A00C) |= 0x800000;     //GPIO3[23] Enable
    DRV_Reg32(0x9000A02C) &= ~(0x800000);     //GPIO3[23] DirectOut
    DRV_Reg32(0x9000A01C) &= ~(0x800000);  //GPIO3[23] Pull Low
	#endif
}

void nand_cs_io_low(void)
{	
#if 0
    if (gPhysical_NandInfo->wPageSize==512)
    {
        DRV_Reg32(0x9000A01C) &= 0x7FFFFF;  //GPIO3[23] Pull Low
    }
#endif
}

void nand_cs_io_high(void)
{
	#if 0
    if (gPhysical_NandInfo->wPageSize==512)
    {
        DRV_Reg32(0x9000A01C) |= (0x800000);  //GPIO3[23] Pull High
    }
 	#endif
}

void NandPadReg(NAND_PAD_ENUM pad_assign)
{
    nand_pad = pad_assign;
}

NAND_PAD_ENUM NandPadGet(void)
{
    return nand_pad;
}


void nand_small_page_cs_pin_reg(UINT8 nand_cs)
{
    nand_cs_io_init();
    nand_small_page_cs = nand_cs;
    wp_bit=0x80;
}

//================================================================//
//write_status[0] : "1" = Error in PROGRAM/ERASE, "0" = Successful PROGRAM/ERASE
//write_status[1] : "1" = Error in PROGRAM/ERASE, "0" = Successful PROGRAM/ERASE  (Plane0)
//write_status[2] : "1" = Error in PROGRAM/ERASE, "0" = Successful PROGRAM/ERASE  (Plane1)
//write_status[7] : "1" = Writeable, "0" = Write Protect

UINT16 Nand_MainId_Get(void)
{
    return (nf_id[0] | (nf_id[1] << 8));//(nf_id->Main);
}

UINT32 Nand_VendorId_Get(void)
{
    return (nf_id[2] | (nf_id[3]<<8) | (nf_id[4]<<16) | (nf_id[5] <<24));//(nf_id->Vendor);
}


void spare_buf_init(void)
{
   UINT32* spare_word = (UINT32*) &nand_spare_buf[0];
   UINT16 i;

   for (i=0;i<MAX_SPARE_SIZE/4;i++)
   {
        spare_word[i]=0xFFFFFFFF;
   }
}

UINT32 NFTransMemGet(UINT32 org_addr, UINT8 dma_en_flag)
{

    UINT32 mem_domain = (org_addr & 0xFFFF0000);
    UINT32 TailAddr=(org_addr & 0x0000FFFF);
    UINT32 TargetAddr=org_addr;
    switch (mem_domain) 
    {
        case MEM_BASE_CEVA_L2_CPU:
            if (dma_en_flag==1) 
            {
                TargetAddr = (MEM_BASE_CEVA_L2_DMA+TailAddr);
            }
            break;
        case MEM_BASE_CEVA_L1_CPU:
            if (dma_en_flag==1) 
            {
                TargetAddr = (MEM_BASE_CEVA_L1_DMA+TailAddr);
            }
            break;
        case MEM_BASE_iRAM_CPU:
            if (dma_en_flag==1) 
            {
                TargetAddr = (MEM_BASE_iRAM_DMA+TailAddr);
            }
            break; 
        case MEM_BASE_CEVA_L2_DMA:
            if (dma_en_flag==0) 
            {
                TargetAddr = (MEM_BASE_CEVA_L2_CPU+TailAddr);
            }
            break;
        case MEM_BASE_CEVA_L1_DMA:
            if (dma_en_flag==0) 
            {
                TargetAddr = (MEM_BASE_CEVA_L1_CPU+TailAddr);
            }
            break;
        case MEM_BASE_iRAM_DMA:
            if (dma_en_flag==0) 
            {
                TargetAddr = (MEM_BASE_iRAM_CPU+TailAddr);
            }
            break;  
        case MEM_BASE_iRAM_CPU_test:
            if (dma_en_flag==1) 
            {
                TargetAddr = (MEM_BASE_iRAM_DMA+TailAddr);
            }
            break;     
             
        default:
						TargetAddr = org_addr;
            break;

    }

    return TargetAddr;

}

SINT32 Nand_ErasePhyBlock(UINT32 wPhysicBlkNum)     
{    
   if((Nand_phy_Erase(wPhysicBlkNum)&0x81)==NF_PROGRAM_WELL)
   {
        return 0x00;
   } else {	
		NF_HAL_DIAG ("Debug: HAL Erase block failed! Block:0x%x =========\n",wPhysicBlkNum);
				mdelay(100); // dominant
        return 0x01;
   }                                                                                                                         
}

UINT32 Nand_ReadPhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer) 
{   
    SINT32 ret;
    UINT32 bch_parity_addr;
//	UINT32 i;

	
    ret = Nand_phy_read(wPhysicPageNum, WorkBuffer);
	if(ret!=0)	// zurong add
	{
		NF_HAL_DIAG ("========HAL Nand_ReadPhyPage Fail 1, page:0x%x =========\n",wPhysicPageNum);
		return -1; //added by Kevin
	}

    bch_parity_addr = (UINT32) &nand_spare_buf[0]; 		

    ret = BCH_Decode(nand_bch_get(), WorkBuffer, nand_page_size_get(), (UINT32) bch_parity_addr, wPhysicPageNum);
    spare_flag_get_from_page(); 
    		
    if(ret!=-1)
    {
        return 0;
    }
    
    NF_HAL_DIAG ("========= HAL Nand_ReadPhyPage Fail 2, page:0x%x ========= \n",wPhysicPageNum);
    return 0x1;  // Normal  fail, BCH error
}         
       
                                                                                                                                                                                           
UINT32 Nand_WritePhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer)                                                                                                                          
{
   UINT32 bch_parity_addr;
//   UINT32 *ptr = (UINT32*) &nand_spare_buf[0]; 
//   unsigned int i;

#if defined(NF_DATA_DEBUG_WRITE_TO_NON_FF_PAGE) // william
   UINT32 saveSpareFlagH;
	 UINT32 saveSpareFlagL;
	 UINT32 ret;
	 UINT32 i=0;
	 UINT32 debug_failed = 0;
	
	debug_failed = 0;
	saveSpareFlagH = SpareFlagH;
	saveSpareFlagL = SpareFlagL;	
	
	memset((void*)debugWorkBuffer,0xff,gPhysical_NandInfo->wPageSize);	// zurong add
	memset((void*)nand_spare_buf,0xff,(gPhysical_NandInfo->wPageSize/1024)*gPhysical_NandInfo->wBchParityAlignSize);	
	
	ret = Nand_ReadPhyPage(wPhysicPageNum, (UINT32)debugWorkBuffer);
	
	if (ret != 0 || bch_get_max_bit_err_cnt() != 0xFE00) {
		printk("william:decode fail when write page, check dump data. Page[%d]Max_Bit_Err_Cnt[%x]ret[%x]\n", wPhysicPageNum, bch_get_max_bit_err_cnt(), ret);
		dump_buffer((unsigned char *)debugWorkBuffer, gPhysical_NandInfo->wPageSize);
		dump_buffer((unsigned char *)nand_spare_buf, (gPhysical_NandInfo->wPageSize/1024)*gPhysical_NandInfo->wBchParityAlignSize);
		//while (1) {
		for(i=0;i<100;i++)
		{
			printk("zurong, you are writing to non ff page[%d]...\n", wPhysicPageNum);
			mdelay(10);
			debug_failed = 0x44;
		}
	}

	SpareFlagH = saveSpareFlagH;
	SpareFlagL = saveSpareFlagL;
	
	if(debug_failed!=0)
	{
		NF_HAL_DIAG ("Debuging: I will be dead! :(    ===== Page:0x%x ====\n",wPhysicPageNum);
		return 0x44;
	}
#endif

#if 0
   if (gPhysical_NandInfo->wPageSize!=512)
   {
       bch_parity_addr = (UINT32) &nand_spare_buf[0];       
   } else {
       bch_parity_addr = (UINT32) &nand_spare_buf[4];       
   }
#else
    bch_parity_addr = (UINT32) &nand_spare_buf[0];  
#endif

   spare_flag_set_into_page();

   BCH_Encode(nand_bch_get(), WorkBuffer, nand_page_size_get(), (UINT32) bch_parity_addr, wPhysicPageNum);

   if((Nand_phy_write(wPhysicPageNum, WorkBuffer)&0x81)==NF_PROGRAM_WELL)
   {
        return 0x00;
   } else {
   		mdelay(100); // dominant
			NF_HAL_DIAG ("========HAL Nand_WritePhyPage Fail 1, page:0x%x =========\n",wPhysicPageNum);
    	return 0x01;
   }
} 

NFCMD_STATUS nand_write_status_get(void) 
{
    return ((NFCMD_STATUS)((write_status&0x81)|wp_bit)); 
}

NFCMD_STATUS Nand_phy_write(UINT32 page_addr, UINT32 buf_addr) 
{
//    UINT8  u8MultiFunc = 0x01 ; 
    UINT16 u16PyLdLen = gPhysical_NandInfo->wPageSize;
    UINT16 u16ReduntLen = gPhysical_NandInfo->wSpareSize;
    #if defined(NF0_USE_ISR)
    UINT16 u16InterruptMask = ND_INTR_DESC_INVALID;
    #else
    UINT16 u16InterruptMask = 0 ;
    #endif
    UINT8  u8End=1;
    UINT32 spare_addr;
    unsigned long t, ms_timeout;
    #if !defined(USE_TIME_FOR_TIMEOUT)
    UINT32 loop_cnt=0;
    #endif
    DescPara DescParas;
    UINT16 u16NextDesBP = 0x00;
    UINT32 nand_csr;
		UINT32 buf_addr_virt = buf_addr;
		UINT32 spare_addr_virt = (UINT32)&nand_spare_buf[0];   

      
    //NF_HAL_DIAG ("Nand physical write enter\n");
  	
  	nandSetDmaTiming(&nand_dma_time_current, 1);
  	
    //NF_HAL_DIAG ("nand_spare_buf virt addr:0x%x\n",&nand_spare_buf[0]);
		spare_addr = (UINT32)virt_to_phys(&nand_spare_buf[0]);
		//NF_HAL_DIAG ("nand_spare_buf phy addr:0x%x\n",spare_addr);
		
		//NF_HAL_DIAG ("write Buffer virt addr:0x%x\n",buf_addr);
		buf_addr = (UINT32)virt_to_phys((UINT32*)buf_addr);
		//NF_HAL_DIAG ("write Buffer phy addr:0x%x\n",buf_addr);
        
    DescParasReset((DescPara *) &DescParas);
    if (gPhysical_NandInfo->wPageSize>512) {  /*Large Page*/
        DescParas.u8Addr0 = 0x00;
        DescParas.u8Addr1 = 0x00;
        DescParas.u8Addr2 = (page_addr&0xff);
        DescParas.u8Addr3 = (page_addr>>8)&0xff;
        DescParas.u8Addr4 = (page_addr>>16)&0xff;
        nand_csr=0x19;  // DMA Burst8
    } else {   /*small page*/
        DescParas.u8Addr0 = 0x00;
        DescParas.u8Addr1 = (page_addr&0xff);
        DescParas.u8Addr2 = (page_addr>>8)&0xff;
        DescParas.u8Addr3 = (page_addr>>16)&0xff;
        DescParas.u8Addr4 = (page_addr>>24)&0xff;
        nand_csr=0x11;  // DMA Burst4
    }

    DescParas.u8CMDType              = 0x02;
//  DescParas.u8MultiFunc            = u8MultiFunc; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x80;
    DescParas.u8CMD1                 = 0x10;

    DescParas.u16PyLdLen             = u16PyLdLen;
    DescParas.u16ReduntLen           = u16ReduntLen;  //256  //16 bits

    DescParas.u16InterruptMask       = u16InterruptMask;
    DescParas.u16InterruptStatus     = 0x00;

    DescParas.u8Owner                = 1;
    DescParas.u8End                  = u8End;
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 1;
    DescParas.u8AddrNo               = gPhysical_NandInfo->wAddrCycle;
    DescParas.u16NextDesBP           = u16NextDesBP;
    DescParas.u32PyLdBP              = buf_addr;
    DescParas.u32ReduntBP            = spare_addr;

    DescParas.u16Redunt_Sector_Len  = gPhysical_NandInfo->wParitySize;   // 8bits
    DescParas.u16Redunt_Sector_Addr_Offset = gPhysical_NandInfo->wBchParityAlignSize; //DescParas.u16Redunt_Sector_Len;

    DrvNand_WP_Disable();
    nand_cs_io_low();
    fillDescriptor((ppara_t)&DescParas,0);

		gp_clean_dcache_range((unsigned long)buf_addr_virt,nand_page_size_get());
		gp_clean_dcache_range((unsigned long)spare_addr_virt,1024);  

		R_NAND_INTR_STS = 0xffff;
    R_NAND_CSR 			= nand_csr;

		#if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Write TimeOut:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
  	#else    
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
    t=jiffies;
    while((R_NAND_INTR_STS&0x8)!=0x8) {        // invalid desc interrupt  
    		#if !defined(USE_TIME_FOR_TIMEOUT)	          
        loop_cnt++;   // 要紀錄 count 倒底是多少, 這邊建議日後採用 TIMER 做 counter
        if (loop_cnt>0x1C000) {
        #else
        if (time_after(jiffies, t + ms_timeout)) {
        #endif
            NF_HAL_DIAG ("Write TimeOut:%lu\n", jiffies - t);                
            R_NAND_INTR_STS = 0xffff;
            DrvNand_WP_Enable();
            nand_cs_io_high();
            NFC_Reset();
            return NF_PROGRAM_TIME_OUT;
        }
    }
		#endif

    R_NAND_INTR_STS = 0xffff;

    nand_cs_io_high();      
    write_status = (Nand_read_Status()&0x81)|wp_bit;
  	DrvNand_WP_Enable();

    return (NFCMD_STATUS)write_status;
}

void spare_flag_set_L(UINT32 spare_flag_L)
{
    SpareFlagL = spare_flag_L;   
}

void spare_flag_set_H(UINT32 spare_flag_H)
{
    SpareFlagH = spare_flag_H;   
}

UINT32 spare_flag_get_L(void)
{
    return SpareFlagL;   
}

UINT32 spare_flag_get_H(void)
{
    return SpareFlagH;   
}

void spare_flag_set_into_page(void)
{
   UINT32* user_word;
   UINT8*  user_char;
   UINT16* user_short;

   switch (nand_bch_get())
   {
     case BCH1K60_BITS_MODE:
        user_word = (UINT32*) &nand_spare_buf[0];
        user_word[1] = SpareFlagL;
        user_word[2] = SpareFlagH;    
        break;
     case BCH1K40_BITS_MODE:   
        user_word = (UINT32*) &nand_spare_buf[0];
        user_word[1] = SpareFlagL;
        user_word[2] = SpareFlagH; 
        break;
     case BCH1K24_BITS_MODE:   
        user_short = (UINT16*) &nand_spare_buf[0];
        user_short[1] = SpareFlagL&0xFFFF;
        user_short[2] = (SpareFlagL>>16)&0xFFFF;
        user_short = (UINT16*) &nand_spare_buf[64];
        user_short[1] = SpareFlagH&0xFFFF;
        user_short[2] = (SpareFlagH>>16)&0xFFFF;
        break;
     case BCH1K16_BITS_MODE:   
        if (nand_page_size_get()>2048)
        {
            user_word = (UINT32*) &nand_spare_buf[0];
            user_word[16] = SpareFlagL;   // 32/4 = 8//luowl
            user_word[32] = SpareFlagH; // 64/4  = 16
        }
        else 
        {
            user_word = (UINT32*) &nand_spare_buf[0];
            user_word[0] = SpareFlagL;  // 0/4 = 0
            //user_word[8] = SpareFlagH;  // 32/4 = 8
            user_word[16] = SpareFlagH;  // 32/4 = 8	// zurong
        }
        break;
     case BCH512B8_BITS_MODE:   //Alignment 128 Bytes
        user_short = (UINT16*) &nand_spare_buf[0];
        user_short[0] = SpareFlagL&0xFF;
        user_short = (UINT16*) &nand_spare_buf[32];  
        user_short[0] = (SpareFlagL>>8)&0xFF;
        user_short = (UINT16*) &nand_spare_buf[64];
        user_short[0] = SpareFlagH&0xFF;
        user_short = (UINT16*) &nand_spare_buf[96]; 
        user_short[0] = (SpareFlagH)>>8&0xFF;       
        break;        
     case BCH512B4_BITS_MODE:
        user_word = (UINT32*) &nand_spare_buf[0];
        user_word[0] = SpareFlagL;
        user_char = (UINT8*) &nand_spare_buf[0];
        user_char[4] = SpareFlagL & 0xFF;
        user_char[8] = (SpareFlagL>>8) & 0xFF;
        user_char[6] = (SpareFlagL>>16) & 0xFF;
        user_char[7] = (SpareFlagL>>24) & 0xFF;        
        break;
     default:    
        break;           
   }
}

void spare_flag_get_from_page(void)
{
   UINT32* user_word;
   UINT8*  user_char;
   UINT16* user_short;

   switch (nand_bch_get())
   {
     case BCH1K60_BITS_MODE:
        user_word = (UINT32*) &nand_spare_buf[0];
        SpareFlagL = user_word[1];
        SpareFlagH = user_word[2];           
        break;
     case BCH1K40_BITS_MODE:   //Alignment 128 Bytes
        user_word = (UINT32*) &nand_spare_buf[0];
        SpareFlagL = user_word[1];
        SpareFlagH = user_word[2]; 
        break;
     case BCH1K24_BITS_MODE:   //Alignment 128 Bytes
        user_short = (UINT16*) &nand_spare_buf[0];
        SpareFlagL = user_short[1] | (user_short[2]<<16);
        user_short = (UINT16*) &nand_spare_buf[64];
        SpareFlagH = user_short[1] | (user_short[2]<<16);
        break;
     case BCH1K16_BITS_MODE:   //Alignment 128 Bytes

        if (nand_page_size_get()>2048)
        {
            user_word = (UINT32*) &nand_spare_buf[0];
            SpareFlagL=user_word[16];//user_word[8];//luowl
            SpareFlagH=user_word[32];        
        }
        else 
        {
            user_word = (UINT32*) &nand_spare_buf[0];
            SpareFlagL=user_word[0];//user_word[8];//luowl
            //SpareFlagH=user_word[8];
            SpareFlagH=user_word[16];
        }
        break;
     case BCH512B8_BITS_MODE:   //Alignment 128 Bytes
        user_short = (UINT16*) &nand_spare_buf[0];
        SpareFlagL = (user_short[16]<<16) | user_short[0];
        SpareFlagH = (user_short[48]<<16) | user_short[32];      
        break;        
     case BCH512B4_BITS_MODE:
        user_word = (UINT32*) &nand_spare_buf[0];
        SpareFlagL=user_word[0];
        user_char = (UINT8*) &nand_spare_buf[0];
        SpareFlagH = user_char[4] | (user_char[8]<<8) |  (user_char[6]<<16) |  (user_char[7]<<24);      
        break;
     default:    
        break;           
   }
}



NFCMD_STATUS Nand_phy_Erase(UINT32 block_id)
{

//    UINT8  u8MultiFunc = 0x01 ; 
    UINT16 u16PyLdLen = gPhysical_NandInfo->wPageSize;
    UINT16 u16ReduntLen = (gPhysical_NandInfo->wPage_with_spare_size - gPhysical_NandInfo->wPageSize);
    #if defined(NF0_USE_ISR)
    UINT16 u16InterruptMask = ND_INTR_DESC_INVALID;
    #else
    UINT16 u16InterruptMask = 0 ;
    #endif
    
    UINT8  u8End=1;
    #if !defined(USE_TIME_FOR_TIMEOUT)
    UINT32 loop_cnt=0;
    #endif
    unsigned long t, ms_timeout;
    DescPara    DescParas;
    UINT16 u16NextDesBP = 0x00;
    UINT32 block_addr;
    UINT32 nand_csr;
//    NFCMD_STATUS ret;

		nandSetDmaTiming(&nand_dma_time_current, 1);
		
  	if(gPhysical_NandInfo->wPageSize > 512)   
  	{   /*Large Page*/
          EraseCycle = gPhysical_NandInfo->wAddrCycle - 2;
          nand_csr=0x19;  // DMA Burst8
  	}
  	else
  	{   /*Small Page*/
          EraseCycle = gPhysical_NandInfo->wAddrCycle - 1;	
          nand_csr=0x11;  // DMA Burst4
  	}

    DescParasReset((DescPara *) &DescParas);

    block_addr = block_id*nand_page_nums_per_block_get();
    DescParas.u8CMDType              = 0x03;
//  DescParas.u8MultiFunc            = u8MultiFunc; //huck
    DescParas.u8ReadStatusData       = 0x00;
    DescParas.u8CMD0                 = 0x60;
    DescParas.u8CMD1                 = 0xd0;

    DescParas.u16PyLdLen             = u16PyLdLen;
    DescParas.u16ReduntLen           = u16ReduntLen;

    DescParas.u16InterruptMask       = u16InterruptMask;
    DescParas.u16InterruptStatus     = 0x00;

    DescParas.u8Owner                = 1;
    DescParas.u8End                  = u8End;
    DescParas.u8LstSec               = 1;
    DescParas.u8RedEn                = 0;
    DescParas.u8AddrNo               = EraseCycle;
    DescParas.u8Addr0                = block_addr&0xFF;
    DescParas.u8Addr1                = (block_addr>>8)&0xFF;
    DescParas.u8Addr2                = (block_addr>>16)&0xFF;
    DescParas.u8Addr3                = (block_addr>>24)&0xFF;
    DescParas.u8Addr4                = 0;
    DescParas.u16NextDesBP           = u16NextDesBP;

    DrvNand_WP_Disable();
    nand_cs_io_low();
    fillDescriptor((ppara_t)&DescParas,0);

		R_NAND_INTR_STS = 0xffff;
    R_NAND_CSR 			= nand_csr ;

		#if defined(NF0_USE_ISR)
    ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
  	t=jiffies;
  	ms_timeout = wait_event_timeout(nf0_int_wait_queue, nf0_int_status & NFC_STS_INVALID_DESC, ms_timeout);
  	if (ms_timeout == 0) {
  		NF_HAL_DIAG("Erase TimeOut:%lu\n", jiffies - t);                
      R_NAND_INTR_STS = 0xffff;
      nand_cs_io_high();
      NFC_Reset();
      return -1;
  	}
  	#else
		ms_timeout = (HZ * NF_TIME_OUT_VALUE)/1000; /* 100 ms */
		t=jiffies;
    while((R_NAND_INTR_STS&ND_INTR_DESC_INVALID)!=ND_INTR_DESC_INVALID) {        // invalid desc interrupt
    	#if !defined(USE_TIME_FOR_TIMEOUT)
      loop_cnt++;
      if (loop_cnt > 0x1c000) {
      #else
      if (time_after(jiffies, t + ms_timeout)) {
      #endif
          NF_HAL_DIAG ("Erase TimeOut:%lu\n",jiffies - t);
          R_NAND_INTR_STS = 0xffff;
          DrvNand_WP_Enable();
          nand_cs_io_high();
          NFC_Reset();
          return NF_PROGRAM_TIME_OUT;
      }
    }
		#endif
		
    R_NAND_INTR_STS = 0xffff;
    nand_cs_io_high();
    
    write_status = (Nand_read_Status()&0x81)|wp_bit;
    DrvNand_WP_Enable();    
    return write_status;

   // ret = (NFCMD_STATUS)((Nand_read_Status()&0x81)|wp_bit);
   // DrvNand_WP_Enable();        
   // return ret;
}

void DrvNand_WP_Initial(void)
{
#if 0
  if(nand_func[Nand_Select]==NULL)	
   {
    switch(Nand_Select)
     {
	case 0:
 	       nand_func[Nand_Select] = gp_board_get_config("nand",gp_board_nand_t);
		break;
	case 1:
		nand_func[Nand_Select] = gp_board_get_config("nand1",gp_board_nand_t);
		break;
     }
  }
#endif

	switch(Nand_Select)
	{
		case 0: //IOA6
			WRITE32(0xfc005144, READ32(0xfc005144) & 0xfffffffd); /* pinmux set ioa6 as orginal function*/
			WRITE32(0xfc005084, READ32(0xfc005084) & 0xffff3fff); /* gid 23 fun as 0*/
			WRITE32(0xfc00a000, READ32(0xfc00a000) | 0x00000040); /* fun as gpio mode*/
			WRITE32(0xfc00a010, READ32(0xfc00a010) & ~0x00000040); /* output LOW*/
			WRITE32(0xfc00a020, READ32(0xfc00a020) & 0xffffffbf); /* as output*/
			WRITE32(0xfc00a010, READ32(0xfc00a010) & ~0x00000040); /* output LOW*/
			break;

		case 1: //IOA5
			WRITE32(0xfc005144, READ32(0xfc005144) & 0xfffffffd); /* pinmux set ioa5 as orginal function*/
			WRITE32(0xfc005084, READ32(0xfc005084) & 0xffffcfff); /* gid 22 fun as 0*/
			WRITE32(0xfc00a000, READ32(0xfc00a000) | 0x00000020); /* fun as gpio mode*/
			WRITE32(0xfc00a010, READ32(0xfc00a010) & ~0x00000020); /* output LOW*/
			WRITE32(0xfc00a020, READ32(0xfc00a020) & 0xffffffdf); /* as output*/
			WRITE32(0xfc00a010, READ32(0xfc00a010) & ~0x00000020); /* output LOW*/
			break;

		default:break;
	}
}


void DrvNand_WP_Enable(void)
{
#if 0
	if(nand_func[Nand_Select]!=NULL)
	{
		nand_func[Nand_Select]->set_wp(0);
	}
#endif
	switch(Nand_Select)
	{
		case 0://IOA6
			WRITE32(0xfc00a010, READ32(0xfc00a010) & ~0x00000040); /* output LOW*/
			break;

		case 1://IOA5
			WRITE32(0xfc00a010, READ32(0xfc00a010) & ~0x00000020); /* output LOW*/
			break;
			
		default:break;
	}	
}

void DrvNand_WP_Disable(void)
{
#if 0
  if(nand_func[Nand_Select]!=NULL)
	{
		nand_func[Nand_Select]->set_wp(1);
	}
#endif
	switch(Nand_Select)
	{
		case 0://IOA6
			WRITE32(0xfc00a010, READ32(0xfc00a010) | 0x00000040); /* output HIGH*/
			break;

		case 1://IOA5
			WRITE32(0xfc00a010, READ32(0xfc00a010) | 0x00000020); /* output HIGH*/
			break;

		default:break;
	}	
}

/*Nand_Switch_Num 0&1 */
void Nand_Chip_Switch(UINT32 Nand_Switch_Num)
{
	Nand_Select = Nand_Switch_Num;
	gPhysical_NandInfo = &Physical_NandGroup[Nand_Select];
	
}

void dump_buffer(unsigned char *addr, unsigned long size)
{
	unsigned long i;	
	
	for(i=0; i<size; i++) {
		if (i%16 == 0) printk("\n[%08lx] ",i);
		printk("%02x ",addr[i]);
	}
	printk("\n\n");	
}

static UINT8 *nand_dbg_data_page_buf = NULL;

UINT8 *nand_get_dbg_buf(void)
{
	return nand_dbg_data_page_buf;
}

UINT8 *nand_get_spare_buf(void)
{
	return nand_spare_buf;
}

UINT32 Nand_DumpPhyPage(UINT32 wPhysicPageNum)
{
	UINT8 *buf;
	UINT32 ret;
	
	if (nand_dbg_data_page_buf == NULL)
		buf = nand_dbg_data_page_buf = (UINT8 *)kmalloc(8192, GFP_DMA);
	else
		buf = nand_dbg_data_page_buf;
	if (buf != NULL) {
		ret = Nand_ReadPhyPage(wPhysicPageNum, (UINT32)buf);
		dump_buffer(buf, nand_page_size_get());
		dump_buffer(&nand_spare_buf[0], 1024);
	}
	else {
		ret = -1;
		printk("fail alloc buf\n");
	}
	//printk("Nand_DumpPhyPage ret[%u]\n", ret);
	
	return ret;
}

SINT32 Nand_sw_bad_block_set(UINT32 blk_id,UINT32 page_buf_addr,UINT8 sw_bad_flag)
{
    UINT32 detect_page_id;
    UINT16 tail_offset,tail_offset2;    
    UINT32 bch_parity_addr = (UINT32) &nand_spare_buf[0];  
	UINT32 erase_ret = 0;
	UINT8  ret1=0,ret2=0,ret3=0,ret4=0;
	UINT32 bad_flag;
	
    if (nand_page_size_get()!=512) {
        nand_spare_buf[0]=sw_bad_flag;

    } else {
        nand_spare_buf[5]=sw_bad_flag;
    }
	
	bad_flag = sw_bad_flag|(sw_bad_flag<<8)|(sw_bad_flag<<16)|(sw_bad_flag<<24);
	spare_flag_set_L(bad_flag);
	spare_flag_set_H(bad_flag);
	
    detect_page_id = blk_id * nand_block_size_get();//blk_id*nand_page_size_get();
    tail_offset = nand_block_size_get()-1; 
    tail_offset2 = nand_block_size_get()-2; 
	
	//Nand_phy_Erase(blk_id); ===> zurong modify to below
	if((Nand_phy_Erase(blk_id)&0x81)!=NF_PROGRAM_WELL)
   {       
		NF_HAL_DIAG ("Debug: HAL Erase block failed! Block:0x%x <function:Nand_sw_bad_block_set> =========\n",blk_id);
		erase_ret = 1;
   }

    spare_flag_set_into_page();
    BCH_Encode(nand_bch_get(), page_buf_addr, nand_page_size_get(), (UINT32) bch_parity_addr,detect_page_id+tail_offset2);

    if((Nand_phy_write(detect_page_id, page_buf_addr)&0x81)==NF_PROGRAM_WELL)
    {
		ret1 = 0;
	}
	else
	{
		NF_HAL_DIAG ("Debug: HAL Set Bad Block page1:0x%x failed <function:Nand_sw_bad_block_set> =========\n",detect_page_id);
		ret1 = 1;
	}
	
	#if 1 // dominant
   if((Nand_phy_write(detect_page_id+1, page_buf_addr)&0x81)==NF_PROGRAM_WELL)
    {
		ret4 = 0;
	}
	else
	{
		NF_HAL_DIAG ("Debug: HAL Set Bad Block page1:0x%x failed <function:Nand_sw_bad_block_set> =========\n",detect_page_id+1);
		ret4 = 1;
	}
	#endif
	
	if((Nand_phy_write(detect_page_id+tail_offset2, page_buf_addr)&0x81)==NF_PROGRAM_WELL)
	{
		ret2 = 0;
	}
	else
	{
		NF_HAL_DIAG ("Debug: HAL Set Bad Block page2:0x%x failed <function:Nand_sw_bad_block_set> =========\n",detect_page_id+tail_offset2);
		ret2 = 1;
	}
	if((Nand_phy_write(detect_page_id+tail_offset, page_buf_addr)&0x81)==NF_PROGRAM_WELL)
	{
		ret3 = 0;
	}
	else
	{
		NF_HAL_DIAG ("Debug: HAL Set Bad Block page3:0x%x failed <function:Nand_sw_bad_block_set> =========\n",detect_page_id+tail_offset);
		ret3 = 1;
	}
	
	if((ret1==0)||(ret2==0) || (ret3==0) || (ret4==0)) // dominant
	{
		spare_buf_init();		
		return 0;
	}	

    spare_buf_init();
	NF_HAL_DIAG ("Debug: HAL Set Bad Block failed:0x%x <function:Nand_sw_bad_block_set> =========\n",blk_id);
    return -1;
}



SINT32 nf_page_bit_bite(UINT32 page_id, UINT32 PageBuf, UINT8 bite_count)
{
    SINT32 ret;
    SINT8  err_bit_cnts;
    SINT8  biting_cnt=0;
    UINT32  bite_mask;
    UINT32* page_word = (UINT32 *) PageBuf;
    SINT32 i,j;
    UINT16 bch_ret;
    
    UINT8 target_bite_count=bite_count;

    ret = Nand_ReadPhyPage(page_id, PageBuf);
    bch_ret = nand_bch_err_bits_get();
    switch(bch_ret)
    {
        case 0xFE00:
          NF_HAL_DIAG ("BitBite Reject:All 0xFF\r\n");
          return -2;
          break;
        case 0xFF00:
          NF_HAL_DIAG ("BitBite Reject:All 0x00\r\n");
          return -3;
          break;
        case 0xFFFF:
          NF_HAL_DIAG ("BitBite Reject:DOA\r\n");
          return -4;
          break;
        default:
          err_bit_cnts = bch_ret;
          break;
    }

    if(bite_count>=err_bit_cnts)
    {
        bite_count = bite_count-err_bit_cnts;
    } else {
        return -1; // Target Bite Bit less then current request , 已經壞太多了
    }

RE_BITE_BIT:
    
    for (i=0;i<256;i++)
    {
        bite_mask=0x00000000;
        for (j=0;j<32;j++)
        {
            if((page_word[i]>>j)&0x1)
            {
                bite_mask |= (1<<j);
                biting_cnt++;
                if (biting_cnt==bite_count)
                {
                    break;
                }
                
            }
        }
        page_word[i] &= ~(bite_mask);

        if (biting_cnt==bite_count)
        {
            break;
        }
    }


    Nand_phy_write(page_id, PageBuf);
   
    ret = Nand_ReadPhyPage(page_id, PageBuf);
    bch_ret = nand_bch_err_bits_get();
    
    if(bch_ret==target_bite_count)
    {
        NF_HAL_DIAG ("Bite %d Bits done!\r\n");
        return 0;
    } 
    else if(bch_ret<target_bite_count)
    {
        biting_cnt = target_bite_count-nand_bch_err_bits_get();
        goto RE_BITE_BIT;
    } else /*if(bch_ret>target_bite_count)*/ {
        NF_HAL_DIAG ("Bite too much Bits, FAIL!\r\n");
        return -3; // Bite too much bits
    }
   
}


