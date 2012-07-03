#ifndef _DRV_L1_BTHDR_EXT_H_
#define _DRV_L1_BTHDR_EXT_H_
// Boot Header Pardsing driver extern pseudo header


#ifndef _RAM_TYPE
#define _RAM_TYPE
typedef enum {
    TYPE_SDRAM,
    TYPE_DDR,
    TYPE_DDR2,
    TYPE_MDDR,
    TYPE_CEVAL2,
    TYPE_CEVAL1,
    TYPE_iRAM
} RAM_TYPE;
#endif  //_RAM_TYPE

#ifndef _BIT_NUMS_MASK
#define _BIT_NUMS_MASK
typedef enum {
    _0BITS=0x0,
    _1BITS=0x1,
    _2BITS=0x3,
    _3BITS=0x7,
    _4BITS=0xF,
    _5BITS=0x1F,
    _6BITS=0x3F,
    _7BITS=0x7F,
    _8BITS=0xFF
} BIT_NUMS_MASK;
#endif //_BIT_NUMS_MASK


#ifndef _HDR_VAL_LEN
#define _HDR_VAL_LEN
typedef enum 
{
    HDR_1Byte=1,
    HDR_CHAR=HDR_1Byte,
    HDR_2Bytes=2,
    HDR_SHORT=HDR_2Bytes,
    HDR_3Bytes=3,
    HDR_4Bytes=4,
    HDR_WORD=HDR_4Bytes
} HDR_VAL_LEN;
#endif  //_HDR_VAL_LEN

//#ifndef _BCH_MODE_ENUM
//#define _BCH_MODE_ENUM
//typedef enum
//{
//    BCH1K60_BITS_MODE=0x00,  // phy register define, fixed
  //  BCH1K40_BITS_MODE=0x01,  // phy register define, fixed
    //BCH1K24_BITS_MODE=0x02,  // phy register define, fixed
    //BCH1K16_BITS_MODE=0x03,  // phy register define, fixed
    //BCH512B8_BITS_MODE=0x04,  // phy register define, fixed
    //BCH512B4_BITS_MODE=0x05,  // phy register define, fixed
    //BCH_OFF=0xFF
//} BCH_MODE_ENUM;
//#endif  //_BCH_MODE_ENUM

#ifndef __HEADER_NF_PART_INFO__
#define __HEADER_NF_PART_INFO__

typedef struct 
{
	UINT8 attr;
	UINT16 MB_size;
}NF_PART_INFO;	
#endif

#define NF_MAX_PART 8

#ifndef __HEADER_NF_INFO__
#define __HEADER_NF_INFO__
typedef struct {
    BCH_MODE_ENUM bthdr_bch;
    UINT16 bthdr_page_size;
    UINT8  bthdr_copies; // the copy times of Boot Header (1BTH = 1Page)
    BCH_MODE_ENUM btldr_bch;
    UINT8  bthdr_page_sectors;
    UINT16 btldr_page_size;
    UINT8  btldr_page_sectors;
    UINT8  btldr_copies;
    UINT16 btldr_start_page_id;
    UINT16 btldr_set_pages;
    UINT16 block_size;
    UINT16 addr_cycle;
    UINT16 nand_type; 
    UINT16 StrongEn;
    UINT16 app_tag;
    UINT32 app_start; // app start page id
    UINT32 app_size; // app size (unit:sectors)
    UINT32 app_perscent;  // app spare area percent rate
    UINT32 app_rtcode_size;  // how many sectors of rt code size to run in DRAM
    UINT32 app_rtcode_run_addr;
    UINT8  swfuse;
    UINT16 total_MB_size;
    UINT8  nf_wp_pin;
    
    UINT8  NoFSSize;
    UINT8	 partitionNum;
    NF_PART_INFO part[NF_MAX_PART];
}HEADER_NF_INFO;

typedef struct gpAppPart_s{
	UINT32	startSector;	// [24+16N .. 27+16N], (sectors)
	UINT32	partSize;		// [28+16N .. 31+16N], (sectors)
	UINT8	partType;		// [32+16N]         
	UINT8	rfu33[3];		// [33+16N .. 35+16N]
	UINT32	dstAddress;		// [36+16N .. 39+16N]
}gpAppPart_t;

typedef struct gpAppHeader_s{
	UINT32	headerTag;		// [0..3]
	UINT8	imageVersion;	// [4]
	UINT8	driverType;		// [5]
	UINT8	headerSize;		// [6], (Sectors)
	UINT8	rfu7;			// [7]
	UINT32	totalAppSize;	// [8..11], (Sectors)
	UINT16	totalPartNumber;// [12..13]
	//UINT8	rfu14[6];		// [14..19]
	UINT8	upgradeKeyPort;	// [14], port, bit[0..3]:port, bit[4..7] low/high active
	UINT8	upgradeKeyId;	// [15], id
	UINT8	rfu14[4];		// [16..19]
	UINT32	checkSum;		// [20..23]
	gpAppPart_t	*appPartN;	// [24...]
}gpAppHeader_t;

#endif //__HEADER_NF_INFO


#ifndef __HEADER_SDC_INFO__
#define __HEADER_SDC_INFO__
typedef struct {
    BCH_MODE_ENUM bthdr_bch;
    UINT16 bthdr_page_size;
    UINT8  bthdr_copies; // the copy times of Boot Header (1BTH = 1Page)
    BCH_MODE_ENUM btldr_bch;
    UINT8  bthdr_page_sectors;
    UINT16 btldr_page_size;
    UINT8  btldr_page_sectors;
    UINT8  btldr_copies;
    UINT16 btldr_start_page_id;
    UINT16 btldr_set_pages;
    UINT16 block_size;
    UINT16 addr_cycle;
    UINT16 nand_type; 
    UINT16 StrongEn;
    UINT16 app_tag;
    UINT32 app_start; // app start page id
    UINT32 app_size; // app size (unit:sectors)
    UINT32 app_perscent;  // app spare area percent rate
    UINT32 app_rtcode_size;  // how many sectors of rt code size to run in DRAM
    UINT32 app_rtcode_run_addr;
    UINT8  swfuse;
    UINT16 total_MB_size;
    UINT32 mbr_bthdr_offset;
} HEADER_SDC_INFO;
#endif //__HEADER_SDC_INFO__

#ifndef _SW_FUSE_ENUM
#define _SW_FUSE_ENUM
typedef enum {
    SW_FUSE_FORCE_READ   =   0,
    SW_FUSE_CPU_MODE     =   1,
    SW_FUSE_RANDOMIZE_EN =   2,
    SW_FUSE_GPZP_EN      =   3,
    SW_FUSE_AES_EN       =   4
} SW_FUSE_ENUM;
#endif


#define TAG_NAND        			0x444E414E   //'N' 'A' 'N' 'D'
#define TAG_SPINOR      			0x49495053   //SPII
#define TAG_SDeMMC      			0x4D4D4453   //SDMM
#define TAG_eMMC_BTMODE 			0x434D4D65   //eMMC
#define HDR_NV_TAG_ID               0
#define HDR_NV_TAG_LEN              HDR_4Bytes
#define HDR_NF_ADDR_CYC_ID          5
#define HDR_NF_ADDR_CYC_ID_LEN      HDR_1Byte
#define HDR_eMMC_SPI_SDC_MHz_ID     5
#define HDR_eMMC_SPI_SDC_MHz_ID_LEN HDR_1Byte
#define HDR_NF_PAGE_SIZE_ID         6
#define HDR_NF_PAGE_SIZE_ID_LEN     HDR_1Byte
#define HDR_NF_BLK_SIZE_ID          7
#define HDR_NF_BLK_SIZE_ID_LEN      HDR_1Byte
#define HDR_NF_CHECK_KEY_ID         8
#define HDR_NF_CHECK_KEY_ID_LEN     HDR_2Bytes
#define HDR_NOT_NF_CHECK_SUM_ID     8
#define HDR_NOT_NF_CHECK_SUM_ID_LEN HDR_4Bytes
#define HDR_NF_BCH_ID               10
#define HDR_NF_BCH_ID_LEN           HDR_1Byte
#define HDR_NF_BTH_BCH_ID           11
#define HDR_NF_BTH_BCH_ID_LEN       HDR_1Byte
#define HDR_BTHDR_COPIES_ID         12
#define HDR_BTHDR_COPIES_ID_LEN     HDR_2Bytes
#define HDR_NV_BTLDR_START_ID       14
#define HDR_NV_BTLDR_START_ID_LEN   HDR_2Bytes
#define HDR_NV_BTLDR_SIZE_ID        16
#define HDR_NV_BTLDR_SIZE_ID_LEN    HDR_2Bytes
#define HDR_BTLDR_COPIES_ID         18
#define HDR_BTLDR_COPIES_ID_LEN     HDR_2Bytes
#define HDR_NV_RTCODE_START_ID      20
#define HDR_NV_RTCODE_START_ID_LEN  HDR_2Bytes
#define HDR_NF_TYPE_ID              23   // 0:MLC, 1:TLC, 2:SLC, 3:SmallPage 4:NAND ROM
#define HDR_NF_TYPE_ID_LEN          HDR_1Byte
#define HDR_DRAM_TYPE_ID            24
#define HDR_DRAM_TYPE_ID_LEN        HDR_1Byte
#define HDR_DRAM_CNT_ID             25
#define HDR_DRAM_CNT_ID_LEN         HDR_1Byte
#define HDR_NV_DRAM_CLK0_ID         26
#define HDR_NV_DRAM_CLK0_ID_LEN     HDR_1Byte
#define HDR_NV_DRAM_CLK1_ID         27
#define HDR_NV_DRAM_CLK1_ID_LEN     HDR_1Byte
#define HDR_NV_DRAM_CLK2_ID         28
#define HDR_NV_DRAM_CLK2_ID_LEN     HDR_1Byte
#define HDR_NV_DRAM_CLK3_ID         29
#define HDR_NV_DRAM_CLK3_ID_LEN     HDR_1Byte
#define HDR_NV_DRAM_CLK4_ID         30
#define HDR_NV_DRAM_CLK4_ID_LEN     HDR_1Byte
#define HDR_BTLDR_CLK_SET_ID        31
#define HDR_BTLDR_CLK_SET_ID_LEN    HDR_1Byte
#define HDR_BTLDR_WORK_ADDR_ID      32
#define HDR_BTLDR_WORK_ADDR_ID_LEN  HDR_4Bytes
#define HDR_NF_STRON_PAGE_EN_ID     36
#define HDR_NF_STRON_PAGE_EN_ID_LEN HDR_1Byte
#define HDR_PRE_REG_CNT_ID          37
#define HDR_PRE_REG_CNT_ID_LEN      HDR_1Byte
#define HDR_BTLDR_ARM_CLK_DIV4      38
#define HDR_BTLDR_ARM_CLK_DIV4_LEN  HDR_1Byte
#define HDR_NF_ERASE_CYCLE_ID       39
#define HDR_NF_ERASE_CYCLE_ID_LEN   HDR_1Byte
#define HDR_GPZP_SRC_ADDR_ID        40
#define HDR_GPZP_SRC_ADDR_ID_LEN    HDR_4Bytes
#define HDR_NVRAM_MB_SIZE_ID        44
#define HDR_NVRAM_MB_SIZE_ID_LEN    HDR_2Bytes
#define HDR_NF_WP_PIN_ID            46
#define HDR_NF_WP_PIN_LEN           HDR_1Byte
#define HDR_SW_FUSE_ID              47
#define HDR_SW_FUSE_ID_LEN          HDR_1Byte
#define HDR_DRAM_PARA0_START        64
#define HDR_DRAM_PARA1_START        120
#define HDR_DRAM_PARA2_START        176
#define HDR_DRAM_PARA3_START        232
#define HDR_DRAM_PARA4_START        288
#define HDR_PRE_REG_START           328 //344
#define HDR_STRONG_TBL_START        768
#define SDC_GP_TAG_START            0x0
#define SDC_FAT_TAG_START           0x60
#define SDC_MBR_TAG_START           200
#define HDR_PRE_REG_START_ID        328
#define HDR_STRONG_TBL_START_ID     768
#define HDR_RTCODE_SIZE_ID          496   // RtCode Sector Size      
#define HDR_RTCODE_SIZE_ID_LEN  HDR_4Bytes
#define HDR_RTCODE_RUN_ADDR_ID      500     
#define HDR_RTCODE_RUN_ADDR_ID_LEN  HDR_4Bytes
#define HDR_APP_TAG_ID              504     
#define HDR_APP_TAG_ID_LEN          HDR_2Bytes
#define HDR_APP_PERCENT_ID          506     
#define HDR_APP_PERCENT_ID_LEN      HDR_2Bytes
#define HDR_APP_SIZE_ID             508     
#define HDR_APP_SIZE_ID_LEN         HDR_4Bytes


#define HDR_PRE_REG_START_ID        328 //344


#define MBR_GP_TAG              0x4D4D4453  //SDMM Tag for ROM code
#define MBR_GP_TAG_ID           200
#define MBR_GP_TAG_ID_LEN       4
#define MBR_BTHDR_OFFSET_ID     204
#define MBR_BTHDR_OFFSET_ID_LEN 4
#define MBR_FS_START_ID         0x1C6   // Sector count
#define MBR_FS_START_ID_LEN     4
#define MBR_PART0_SIZE_ID       0x1CA
#define MBR_PART0_SIZE_ID_LEN   4
#define MBR_PART1_SIZE_ID       0x1CE
#define MBR_PART1_SIZE_ID_LEN   4
#define MBR_PART2_SIZE_ID       0x1D2
#define MBR_PART2_SIZE_ID_LEN   4

#define DBR_PART_SIZE_ID        0x20
#define DBR_PART_SIZE_ID_LEN    4




/*IO Trap Definitions*/
#define BOOT_DRAM_BT_TRAP           0      /*TRAP[0]*/
#define BOOT_DRAM_BT_TRAP_BITS      _1BITS /*1 bits*/
#define BOOT_ARM_RTCK_EN_TRAP       1      /*TRAP[1]*/
#define BOOT_DRAM_BT_TRAP_BITS      _1BITS /*1 bits*/
#define BOOT_CLK_TRAP               2      /*TRAP[3:2]*/
#define BOOT_CLK_TRAP_BITS          _2BITS /*2 bits*/
#define BOOT_DYN_GATE_BYPASS        4      /*TRAP[4]*/
#define BOOT_DYN_GATE_BYPASS_BITS   _1BITS /*1 bits*/
#define BOOT_DEV_TRAP               5      /*TRAP[8:5]*/
#define BOOT_DEV_TRAP_BITS          _4BITS /*4 bits*/
#define BOOT_DRAM_TYPE_TRAP         9      /*TRAP[10:9]*/
#define BOOT_DRAM_TYPE_TRAP_BITS    _2BITS /*2 bits*/
#define BOOT_NF_FSMALL_TRAP         11     /*TRAP[11]*/ 
#define BOOT_NF_FSMALL_TRAP_BITS    _1BITS /*1 bits*/
#define BOOT_RD_DBG_MODE_TRAP       12     /*TRAP[12]*/ 
#define BOOT_RD_DBG_MODE_TRAP_BITS  _1BITS /*1 bits*/

#define BOOT_RANDOMIZE_TRAP         13     /*TRAP[13]*/ 
#define BOOT_RANDOMIZE_TRAP_BITS    _1BITS /*1 bits*/
#define BOOT_ALL_CLK_EN_TRAP        14     /*TRAP[14]*/ 
#define BOOT_ALL_CLK_EN_TRAP_BITS   _1BITS /*1 bits*/

/*efuse range is 2~6*/
#define BOOT_DDR_EFUSE          2 
#define BOOT_CPU_FORCE_EFUSE    3 
#define BOOT_AES_EN_EFUSE       4 

/*Pre-Register Vendor Command*/
#define PRE_CMD_BASE               0xD0667800
#define PRE_CMD_NULL               0x00000000
#define PRE_CMD_LOOP_DLY           0xD0667801
#define PRE_CMD_MS_DLY             0xD0667802
#define PRE_CMD_PC_JUMP            0xD0667803
#define PRE_CMD_USB_SERVICE        0xD0667804
#define PRE_CMD_POWER_USB_SERVICE  0xD0667805
#define PRE_CMD_BL_FUNCTION        0xD0667806
#define PRE_CMD_PAUSE_TIL_BTL_DONE 0xD0667807
#define PRE_CMD_IO_TRIGLE_MACRO    0xD0667808
#define PRE_CMD_CEVA_INIT          0xD0667809



extern SINT32 nf_hdr_get_from_bt_area(UINT8* header);
extern UINT16 iotrap_info_get(UINT8 bit_offset, BIT_NUMS_MASK bit_nums);
extern SINT32 nf_hdr_to_info(UINT8* header, HEADER_NF_INFO *nv_info);
extern SINT16 check_key_status_get(UINT8 *bt_hdr_buf);
extern SINT16 hdr_check_key_count(UINT8 * bt_hdr_buf);
extern UINT8 hdr_sw_fuse_get(SW_FUSE_ENUM bit_index);


#endif //_DRV_L1_BTHDR_EXT_H_

