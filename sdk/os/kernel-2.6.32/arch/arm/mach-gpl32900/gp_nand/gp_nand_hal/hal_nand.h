#ifndef _GP_HAL_NAND_H_
#define _GP_HAL_NAND_H_

#include <mach/gp_chunkmem.h>
#include <mach/hardware.h>
#include <asm/cacheflush.h>
#include <mach/typedef.h>

//#define NF_DATA_DEBUG_WRITE_TO_NON_FF_PAGE // check page is all ff before write
#define NF_DATA_DEBUG_PRINT_READ_RETRY	// print out debug information when there are retry become good

#define NF_TIME_OUT_VALUE           1000 // 1s, ms unit

#define BCH_DEVICE_NAME "bch"
#define NF0_DEVICE_NAME "nf0"

#define STATUS_OK               0
#define STATUS_FAIL            -1

#define NAND0_S330_BASE								IO3_ADDRESS(0x8000)
#define SCUA_XXX_BASE									IO3_ADDRESS(0x7000)
#define SCUC_XXX_BASE									IO2_ADDRESS(0x5000)
#define SCUB_XXX_BASE									IO0_ADDRESS(0x5000)
#define NAND_S330_BASE								IO3_ADDRESS(0x8000)
#define BCH_S336_BASE_ADDRESS					IO3_ADDRESS(0xA000)

#define FM_CSR          							(NAND_S330_BASE + 0x00)
#define FM_AC_TIMING    							(NAND_S330_BASE + 0x08)
#define FM_INTRMSK      							(NAND_S330_BASE + 0x40)


#define rFM_CSR         							(*(volatile unsigned *)FM_CSR)
#define rFM_AC_TIMING   							(*(volatile unsigned *)FM_AC_TIMING)
#define rFM_INTRMSK     							(*(volatile unsigned *)FM_INTRMSK)

                                  		
#define R_NAND_CSR              			(*(volatile UINT32 *) (NAND_S330_BASE + 0x00 )) 
#define R_NAND_DESC_BA          			(*(volatile UINT32 *) (NAND_S330_BASE + 0x04 ))
#define R_NAND_INTR_STS         			(*(volatile UINT32 *) (NAND_S330_BASE + 0x44 ))//0x93008044
#define R_NAND_FAST_STATUS_CTRL 			(*(volatile UINT32 *) (NAND_S330_BASE + 0xc0 ))//0x930080C0
#define R_NAND_FAST_STATUS      			(*(volatile UINT32 *) (NAND_S330_BASE + 0xc4 ))//0x930080C4
#define R_NAND_FAST_STATUS_FLAG 			(*(volatile UINT32 *) (NAND_S330_BASE + 0xc8 ))//0x930080C8

// PINMAX 
#define R_SCUB_PGS0     							(*(volatile UINT32 *) (SCUB_XXX_BASE + 0x80))//0x90005080
#define R_SCUB_PGS1     							(*(volatile UINT32 *) (SCUB_XXX_BASE + 0x84))//0x90005084
#define R_SCUB_PGS2     							(*(volatile UINT32 *) (SCUB_XXX_BASE + 0x88))//0x90005088
#define R_SCUB_PGS3     							(*(volatile UINT32 *) (SCUB_XXX_BASE + 0x8C))//0x9000508C
#define R_PINMUX_NEW						  		(*(volatile UINT32 *) (SCUB_XXX_BASE + 0x144))//0x90005144


#define FM0_SHARE_CTRL      					(NAND0_S330_BASE + 0x30)
#define FM0_SHARE_CTRL1     					(NAND0_S330_BASE + 0x34)


#define rSCUC_PERI_CLKEN							(*(volatile unsigned *)(SCUC_XXX_BASE+0x04))
#define rSCUB_GPIO3_IE      					(*(volatile unsigned *)(SCUB_XXX_BASE+0x130))
#define rSCUA_PERI_CLKEN							(*(volatile unsigned *)(SCUA_XXX_BASE+0x04))
#define rSCUA_PERI_RST								(*(volatile unsigned *)(SCUA_XXX_BASE+0x00))
#define rFM0_SHARE_CTRL     					(*(volatile unsigned *)FM0_SHARE_CTRL)
#define rFM0_SHARE_CTRL1  						(*(volatile unsigned *)FM0_SHARE_CTRL1)

#define NAND_CHIP_NUM		2
/* Nand register */

#define iROM_DBG_MODE7  0   	// Nand Burning Test Enable
//#define vu(x) 								(*(volatile unsigned *)(x))
#define MAX_SPARE_SIZE 1024

typedef enum 
{
	NAND_TRUE=0,
	NAND_OK=0,
	NAND_ROM_FIND=1,
  NAND_FAIL,
	NAND_PAD_FAIL,
  NAND_DMA_ALIGN_ADDRESS_NEED,
  NAND_DMA_LENGTH_ERROR,
	NAND_BCH_LENGTH_ERROR,
  NAND_TIMEOUT,
	NAND_BCH_ERROR,
	NAND_UNKNOWN,
	NAND_STATUS_MAX
} NAND_STATUS_ENUM;

typedef enum {
    NAND_PAD_AUTO 			= 0x2,
    NAND_SHARE 					= 0x3,   
    NAND_PARTIAL_SHARE 	= 0x6,
    NAND_NON_SHARE 			= 0x7,
    NAND_NON_SHARE2 		= 0x8,
    NAND_NONE 					= 0xFF
} NAND_PAD_ENUM;

typedef enum {
    NF_PROGRAM_DENY 		=	0x00,  /*Write Protect*/
    NF_PROGRAM_ERR7 		=	0x01,  /*Write Protect*/
    NF_PROGRAM_ERR8 		=	0x02,  /*Write Protect*/
    NF_PROGRAM_ERR9 		=	0x03,  /*Write Protect*/  
    NF_PROGRAM_ERR10		=	0x04,  /*Write Protect*/ 
    NF_PROGRAM_ERR11		=	0x05,  /*Write Protect*/
    NF_PROGRAM_ERR12		=	0x06,  /*Write Protect*/  
    NF_PROGRAM_ERR13		=	0x07,  /*Write Protect*/
    NF_PROGRAM_WELL 		=	0x80,  /*Program Well*/
    NF_PROGRAM_ERR0 		=	0x81,  /*Program Error*/
    NF_PROGRAM_ERR1 		=	0x82,  /*Program Error1*/
    NF_PROGRAM_ERR2 		=	0x83,  /*Program Error2*/
    NF_PROGRAM_ERR3 		=	0x84,  /*Program Error3*/  
    NF_PROGRAM_ERR4 		=	0x85,  /*Program Error4*/
    NF_PROGRAM_ERR5 		=	0x86,  /*Program Error5*/
    NF_PROGRAM_ERR6 		=	0x87,   /*Program Error6*/
    NF_PROGRAM_TIME_OUT =	0xFF
} NFCMD_STATUS;


typedef enum
{
    BCH1K60_BITS_MODE=0x00,  // phy register define, fixed
    BCH1K40_BITS_MODE=0x01,  // phy register define, fixed
    BCH1K24_BITS_MODE=0x02,  // phy register define, fixed
    BCH1K16_BITS_MODE=0x03,  // phy register define, fixed
    BCH512B8_BITS_MODE=0x04,  // phy register define, fixed
    BCH512B4_BITS_MODE=0x05,  // phy register define, fixed
    BCH_OFF=0xFF
} BCH_MODE_ENUM;


typedef struct NandInfo_Str
{
	UINT32    wNandID;
	UINT16    wNandType;
	UINT16    wAddrCycle;
	UINT32    wNandBlockNum;
	UINT16    wPageSize;
	UINT16    wSpareSize;      // Total BCH Parity Size
	UINT16    wParitySize;     // the BCH ParitySize per set
	UINT16    wPageSectorSize;
	UINT16    wPage_with_spare_size;
	UINT16    wBlkPageNum;
	UINT8     wBchParityAlignSize;
	UINT16	  wBadFlgByteOfs;
	UINT16	  wBadFlgPage1;
	UINT16	  wBadFlgPage2;	
}Physical_NandInfo;


#define DRV_LOOP_DELAY(loops)  \
{\
   UINT32 loopcnt;\
   loopcnt = (UINT32)loops;\
   while (loopcnt--) {}\
}


#define DRV_Reg32(addr)               (*(volatile UINT32 *)(addr))
#define MEM_BASE_DRAM        		0x00000000   
#define MEM_BASE_iROM        		0xffff0000   /*32KB*/
#define MEM_BASE_iROM_DMA    		0x98000000   /*32KB*/
#define MEM_BASE_iRAM_CPU    		0xB0000000   /*16KB*/
#define MEM_BASE_iRAM_DMA    		0x9D800000   /*16KB*/
#define MEM_BASE_CEVA_L2_CPU 		0xB1000000   /*32KB*/
#define MEM_BASE_CEVA_L2_DMA 		0x90600000   /*32KB*/
#define MEM_BASE_CEVA_L1_CPU 		0xB2000000   /*64KB*/
#define MEM_BASE_CEVA_L1_DMA 		0x90400000   /*32KB*/
#define MEM_BASE_iRAM_CPU_test	0xfc900000


typedef	struct	parameter
{
	UINT8 	u8CMDType;
	UINT8 	u8MultiFunc;
	UINT8 	u8ReadStatusData;
	UINT8 	u8CMD0;
	UINT8 	u8CMD1;
	UINT16 	u16PyLdLen;
	UINT16 	u16ReduntLen;
	UINT16 	u16InterruptMask;
	UINT16 	u16InterruptStatus;
	UINT8 	u8Owner;
	UINT8 	u8End;
	UINT8 	u8LstSec;
	UINT8 	u8RedEn;
	UINT8 	u8AddrNo;
	UINT8 	u8Addr0;
	UINT8 	u8Addr1;
	UINT8 	u8Addr2;
	UINT8 	u8Addr3;
	UINT8 	u8Addr4;
	UINT16 	u16NextDesBP;
	UINT32  u32PyLdBP;
	UINT32  u32ReduntBP;
	UINT16 	u16Redunt_Sector_Addr_Offset;
	UINT16 	u16Redunt_Sector_Len;	

}para_t, *ppara_t, DescPara;

typedef struct 
{
    UINT16 Main;
    UINT32 Vendor;    
} NAND_ID;

extern Physical_NandInfo Physical_NandGroup[NAND_CHIP_NUM];
extern Physical_NandInfo* gPhysical_NandInfo;
extern UINT32 Nand_Select;
//extern HEADER_NF_INFO  nf_info;
extern UINT8 nand_spare_space[MAX_SPARE_SIZE];
extern NAND_PAD_ENUM Nand_Pad_Scan(NAND_PAD_ENUM pad_mode);
extern UINT32 NFTransMemGet(UINT32 org_addr, UINT8 dma_en_flag);
extern SINT32 Nand_phy_read(UINT32 page_addr, UINT32 buf_addr);
extern void nand_page_size_set(UINT16 page_size);
extern UINT32 Nand_ReadPhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer);
extern NAND_STATUS_ENUM nand_intelligent_id_init(UINT16 main_id, UINT32 vendor_id);
extern NAND_PAD_ENUM Nand_Pad_Scan(NAND_PAD_ENUM pad_mode);
extern UINT16 Nand_MainId_Get(void);
extern UINT32 Nand_VendorId_Get(void);
extern SINT32 Nand_ErasePhyBlock(UINT32 wPhysicBlkNum);
extern UINT16 nand_page_size_get(void); 
extern UINT16 nand_block_size_get(void);
extern SINT32 DrvL1_Nand_Init(NAND_PAD_ENUM nand_pad_set);
extern UINT32 Nand_RandomReadPhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer); 
extern UINT32 Nand_WritePhyPage(UINT32 wPhysicPageNum, UINT32 WorkBuffer);
extern void spare_flag_set_L(UINT32 spare_flag_L);
extern void spare_flag_set_H(UINT32 spare_flag_H);
extern UINT32 spare_flag_get_L(void);
extern UINT32 spare_flag_get_H(void);
extern void DrvNand_WP_Disable(void);
extern void DrvNand_WP_Enable(void);
extern UINT16 nand_bch_err_bits_get(void);
extern SINT32 Nand_Init(void);
extern SINT32 Nand_UnInit(void);
extern UINT32 nand_page_nums_per_block_get(void);
extern UINT32 nand_total_MByte_size_get(void);
extern void nand_total_MByte_size_set(UINT32 nand_MB_size);
extern void nand_page_nums_per_block_set(UINT16 blk_page_size);
extern void nand_addr_cycle_set(UINT16 addr_cycle);
extern NFCMD_STATUS Nand_phy_Erase(UINT32 block_id);
extern SINT16 good_block_check(UINT16 blk_id, UINT32 WorkBuffer);
extern void spare_buf_init(void);
extern NAND_STATUS_ENUM nand_smart_id_init(UINT16 main_id, UINT32 vendor_id);
extern void nand_bch_set(BCH_MODE_ENUM type);
extern BCH_MODE_ENUM nand_bch_get(void);
extern void bch_mode_set(BCH_MODE_ENUM bch_set);
extern BCH_MODE_ENUM bch_mode_get(void);
extern SINT32 BCH_Encode(BCH_MODE_ENUM BchMode, UINT32 DataSrcAddr,UINT16 data_length ,UINT32 ParityGenAddr, UINT32 pageAddr);  
extern SINT32 BCH_Decode(BCH_MODE_ENUM BchMode, UINT32 DataSrcAddr,UINT16 data_length ,UINT32 ParitySrcAddr, UINT32 pageAddr);
extern void DrvNand_WP_Initial(void);
extern NFCMD_STATUS nand_write_status_get(void);
extern void bch_boot_area_uninit(void);
extern void Nand_Getinfo(UINT16* PageSize,UINT16* BlkPageNum,UINT32* NandBlockNum);

extern SINT32 Nand_sw_bad_block_set(UINT32 blk_id,UINT32 page_buf_addr,UINT8 sw_bad_flag);


extern UINT32 Nand_DumpPhyPage(UINT32 wPhysicPageNum);
extern void dump_buffer(unsigned char *addr, unsigned long size);
extern UINT32 Nand_DumpPhyPage(UINT32 wPhysicPageNum);
extern UINT8 *nand_get_dbg_buf(void);
extern UINT8 *nand_get_spare_buf(void);
extern void Nand_Chip_Switch(UINT32 Nand_Switch_Num);
extern SINT32 NandParseBootHeader(UINT8 *Buffer);
extern UINT16 GetAppStartBlkFromBth(void);
extern UINT16 GetAppSizeOfBlkFromBth(void);
extern UINT16 GetAppPercentFromBth(void);
extern UINT16 GetDataBankLogicSizeFromBth(void);
extern UINT16 GetDataBankRecycleSizeFromBth(void);
extern UINT32 GetNoFSAreaSectorSizeFromBth(void);
extern UINT8 GetPartNumFromBth(void);
extern void GetPartInfoFromBth(UINT8 part,UINT16 *MB_size,UINT8 *attr);
extern UINT16 get_bch_mode(void);
extern void nandAdjustDmaTiming(void);
extern SINT32 nf_page_bit_bite(UINT32 page_id, UINT32 PageBuf, UINT8 bite_count);

#endif

