#include "hal_nand_smart_id.h"


#define PAGESIZE_MASK   0x3  
#define GET_PAGE_SIZE(x) (x&PAGESIZE_MASK)

#define BLOCKNUM_MASK   0x1C  
#define GET_BLOCK_NUMS(x) (x&BLOCKNUM_MASK)  

#define BLOCKSIZE_MASK  0x60
#define GET_BLOCK_SIZE(x) (x&BLOCKSIZE_MASK)  
                                                                                                                        
#define ERASE_CYC_MASK  0x80                                                                                                       
#define GET_ERASE_CYC(x) (x&ERASE_CYC_MASK)

#define ALL_NEW_SPEC_NTYPE_ID_START  20
#define ALL_TBL_FIX_NTYPE_ID_END     10
#define OLD_SPEC_NTYPE_ID_START      11

#define _CYCLES
#define _BYTES
#define _KB
#define _MB
#define _PAGES
#define _BLKS

//#define MF_NUMS 9
static UINT8  NTYPE; 
typedef struct 
{
	UINT16	MainID;
//	UINT32 	VenDorID;
	UINT32	TotalMB;
	UINT32  BlockNum;
	UINT16	BlockSize;
	UINT32 	PageSize;
	UINT16  PageSpareSize;
	UINT8   AddressCycle;
//	UINT8 	EraseCycle;
//    UINT8 	CS_NUMS;
	BCH_MODE_ENUM	BCHMode; 
} SPECIAL_NF_TBL;

SINT32 special_nf_detect(UINT16 main_id, UINT32 vendor_id);

flash_list NAND_ID_Table[ID_TBL_SIZE] =                                                                                                                     
{                                                                                                                                                             
    /*{  MainID,Defalut Size, BlockNums  | BlockSize |PageSize(Byte)| Erase Cycles }*/                                                                                                      
 /*00*/   {0x73,   16 _MB, (_1024_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC2)},   //16MB                                                                                                
 /*01*/   {0x58,   16 _MB, (_1024_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC2)},   //16MB  // Nand ROM 代表                                                                                               
 /*02*/   {0x74,   16 _MB, (_1024_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC2)},   //16MB                                                                                                
 /*03*/   {0x34,   16 _MB, (_1024_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC2)},   //16MB                                                                                                
 /*04*/   {0x75,   32 _MB, (_2048_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC2)},   //32MB                                                                                                 
 /*05*/   {0x35,   32 _MB, (_2048_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC2)},   //32MB                                                                                                                                                                                                 	
 /*06*/   {0x76,   64 _MB, (_4096_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC3)},   //64MB                                                                                                
 /*07*/   {0x36,   64 _MB, (_4096_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC3)},   //64MB	                                                                                              
 /*08*/   {0x5A,   64 _MB, (_4096_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC3)},   //64MB	                                                                                              
 /*19*/   {0x79,  128 _MB, (_8192_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC3)},   //128MB                                                                                               
 /*%10*/  {0x39,  128 _MB, (_8192_BLOCKS | _32_PAGES |  _512_BYTES  | _ERASE_CYC3)},   //128MB
 /*%11*/  {0xF1,  128 _MB, (_1024_BLOCKS | _64_PAGES | _2048_BYTES  | _ERASE_CYC2)},   //128MB
 /*12*/   {0xD3, 1024 _MB, (_8192_BLOCKS | _64_PAGES | _2048_BYTES  | _ERASE_CYC3)},   //1GB 
 /*13*/   {0xD5, 2048 _MB, (_16384_BLOCKS| _64_PAGES | _2048_BYTES  | _ERASE_CYC3)},   //2GB   // default for toshiba , Old or New Spec                                                                                                   
 /*14*/   {0xCA,  256 _MB, (_2048_BLOCKS | _64_PAGES | _2048_BYTES  | _ERASE_CYC3)},   //256MB
 /*15*/   {0xDA,  256 _MB, (_2048_BLOCKS | _64_PAGES | _2048_BYTES  | _ERASE_CYC3)},   //256MB                                                                                               
 /*16*/   {0x71,  256 _MB, (_2048_BLOCKS | _64_PAGES | _2048_BYTES  | _ERASE_CYC3)},   //256MB                                                                                               
 /*17*/   {0xDC,  512 _MB, (_4096_BLOCKS | _64_PAGES | _2048_BYTES  | _ERASE_CYC3)},   //512MB       
	   			{0xD1,  128 _MB, (_2048_BLOCKS | _64_PAGES | _2048_BYTES  |_ERASE_CYC3)},   //128MB                                
 /*18*/   {0xD7, 4096 _MB, (_8192_BLOCKS | _128_PAGES| _4096_BYTES  | _ERASE_CYC3)},   //4GB //Old or New Spec 
 /*19*/   {0xD9, 4096 _MB, (_16384_BLOCKS| _128_PAGES| _4096_BYTES  | _ERASE_CYC3)},   //4GB //Old or New Spec 
 /*%20*/  {0x48, 2048 _MB, (_2048_BLOCKS | _256_PAGES| _4096_BYTES  | _ERASE_CYC3)},   //2GB   //New Spec                                                                                             	
 /*21*/   {0x68, 4096 _MB, (_4096_BLOCKS | _256_PAGES| _4096_BYTES  | _ERASE_CYC3)},   //4GB   //New Spec 
 /*22*/   {0x88, 8192 _MB, (_8192_BLOCKS | _256_PAGES| _4096_BYTES  | _ERASE_CYC3)},   //8GB   //New Spec 
 /*23*/   {0xFF, 1024 _MB, (_2048_BLOCKS | _128_PAGES| _2048_BYTES  | _ERASE_CYC3)}   //NONE                                                                                              
};

SPECIAL_NF_TBL Special_NF_Tbl[] =
{
/*  {MainID,VenDorID,  MB Size,  BlockNums ,BlockSize , PageSize, PageSpareSize, AddressCycle, BchMode}*/

    /* TC58NVG0S3ETA00 */
//    {0xD198/*,0x14761590*/, 128 _MB, 1024 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 4 _CYCLES, BCH1K16_BITS_MODE},
    {0xD198/*,0x14761590*/, 128 _MB, 1024 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 5 _CYCLES, BCH1K16_BITS_MODE},

	/* H27U518S2CTR */
    {0x76AD/*,0x76AD76AD*/, 64  _MB, 4096 _BLKS, 32 _PAGES, 512  _KB, 16 _BYTES, 4 _CYCLES, BCH512B4_BITS_MODE},

    /* TC58DVM92A5TA00_E091124C */
    {0x7698/*,0x0420C0A5*/, 64  _MB, 4096 _BLKS, 32 _PAGES, 512  _KB, 16 _BYTES, 4 _CYCLES, BCH512B4_BITS_MODE},
    /* Found in cards on hand*/
//	{0x7698/*,0x0421C0A5*/, 64  _MB, 4096 _BLKS, 32 _PAGES, 512  _KB, 16 _BYTES, 4 _CYCLES, BCH512B4_BITS_MODE},


	////////////////////////////////////////
	//////Following items Added by Kevin
	
	/* TC58NVG1S3ETA00 */
    {0xDA98/*,0x14761590*/, 256 _MB, 2024 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 5 _CYCLES, BCH1K16_BITS_MODE},

	/* HY27UF(08)2G2B (not test)*/
	{0xDAAD/*,0xAD449510*/, 256 _MB, 2048 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 5 _CYCLES, BCH1K16_BITS_MODE},

    /* HY27UF081G2A (not test)*/
    {0xF1AD/*,0xF1AD1D80*/, 128 _MB, 1024 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 4 _CYCLES, BCH1K16_BITS_MODE},
    
    /* N231GT, nand OTP */
    {0x79C2/*,0xFFFFFFFF*/, 128 _MB, 8192 _BLKS, 32 _PAGES, 512  _KB, 16 _BYTES, 4 _CYCLES, BCH_OFF},
    
    /* N23512T, nand OTP */
    {0x76C2/*,0xFFFFFFFF*/, 64  _MB, 4096 _BLKS, 32 _PAGES, 512  _KB, 16 _BYTES, 4 _CYCLES, BCH_OFF},

	/* Added by Wallace, to be verified by PST before any use in actual production */
	//{0x5AC2/*,0xFFFFFFFF*/, 64  _MB, 4096 _BLKS, 32 _PAGES, 512  _KB, 16 _BYTES, 4 _CYCLES, BCH_OFF},
        
    /* H27U1G8F2B (not test)*/
//    {0xF1AD/*,0xF1AD9500*/, 128 _MB, 1024 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 4 _CYCLES, BCH1K16_BITS_MODE},
    
    /* HY27UF082G2A (not test)*/
//    {0xDAAD/*,0xAD001D80*/, 256 _MB, 2048 _BLKS, 64 _PAGES, 2048 _KB, 64 _BYTES, 5 _CYCLES, BCH1K16_BITS_MODE}
    
};

NAND_STATUS_ENUM nand_smart_id_init(UINT16 main_id, UINT32 vendor_id)
{
    UINT8 chip_id;
    NAND_STATUS_ENUM nand_status=NAND_OK;
    NF_MF_ENUM mf_id;
  //UINT8 i,j,k;
    UINT8 j;
    ID4TH_SPEC spec_type;
  //  NF_MF_ENUM manufactor[MF_NUMS] = {MF_SAMSUNG,MF_TOSHIBA,MF_HYNIX,MF_MICRON,MF_ST,MF_MX,MF_SANDISK,MF_INTEL,MF_SPANSION}; 
    if(special_nf_detect(main_id, vendor_id)==0)	//wenlongxu add 2011 6,15 for vtech
    {
        return NAND_OK;
    }
    NTYPE = (ID_TBL_SIZE-1); /*initial NTYPE to avoid the null assign*/
    
    chip_id = ((main_id & 0xFF00)>>8);   /*Chip id is linear mapping to size*/
    mf_id = main_id&0xFF;

    if (chip_id==0x00) {
        nand_status = NAND_FAIL;
    }

    //Scanning id table 智能 ID 啟動  
    for (j = 0; j<ID_TBL_SIZE ; j++)      /* Get Nand ID table */                                                                                                                                                                                                                                               
    {                                                                                                                                                         
      if ( NAND_ID_Table[j].id == chip_id)                                                                                                                   
      {                                                                                                                                                     
          NTYPE = j;   /* NAND_ID_Table[NTYPE] is the nand parameter table for driver*/                                                                     
    	  break;                                                                                                                                            
      }                                                                                                                                                     
    }    
    
    /*預設 Nand ID 設計*/
    if (NTYPE==ID_TBL_SIZE-1) {
        gPhysical_NandInfo->wPageSize = 2048;  
        gPhysical_NandInfo->wBlkPageNum = 64;                                                                                                                         
        gPhysical_NandInfo->wNandBlockNum = 1024;
        gPhysical_NandInfo->wAddrCycle = 5;   //取最高的
        //erase_cycle = 3;
        nand_status=NAND_UNKNOWN; // 標記為 Unknown Nand
        if ((vendor_id&0xff70)==0) {
            return NAND_UNKNOWN;  // 應該不會有此狀況,除非是有問題的 Nand 
        }        
     }

    /*智能 ID special case */
    if (NAND_ID_Table[NTYPE].id == 0xD5) 
   	{
    	if (mf_id==MF_SAMSUNG)   /* Modify to samsung's spec*/
        {
            NAND_ID_Table[NTYPE].BlockNums_BlockSize_PageSize=(_8192_BLOCKS| _64_PAGES| _4096_BYTES); //Samsung 2GB 
        }
    }
    /*智能 ID special Nand ROM case */
    if (mf_id==MF_MX) //0xC2 is MX, must be nand ROM
    {
       //ECC_check=0;  /*disable 掉 ECC, Nand ROM 毋需 check BCH*/
       nand_status = NAND_ROM_FIND;
    } 
    else if(mf_id==MF_SANDISK) //0x45 is Sandisk OEM Giano/Athena
    {
      if ((vendor_id & 0xFF)==0x04 || (vendor_id & 0xFF)==0x05) 
      {             
        //ECC_check=0;  /*disable 掉 ECC, Nand ROM 毋需 check BCH*/
        nand_status = NAND_ROM_FIND; 
       }
    }

    /*ID4 沒想像般方便使用, MLC 4K big spare 出來後, spec 又開始亂了*/
    if (NTYPE!=ID_TBL_SIZE-1) 
    {
        if (NTYPE >= ALL_NEW_SPEC_NTYPE_ID_START) {  /*設計第19個以後擺的都是新 spec ID*/
            // 0x48/0x68/0x88 確定必為新制程
            if ((mf_id == MF_MICRON) || (mf_id == MF_INTEL) ) {
                spec_type = NEW_BLOCK_PG_SPEC;  // 目前只有 Micron 新制程採用 PG spec
            } else {
                spec_type = NEW_BLOCK_KB_SPEC;
            }         
        } else if (NTYPE  <= ALL_TBL_FIX_NTYPE_ID_END) { /*設計第10個以內擺的都是 fix table ID*/
            spec_type = TABLE_SPEC;
        } else if (NTYPE  >= OLD_SPEC_NTYPE_ID_START) { /*設計第11~18之間擺的都是 OLD Spec ID*/
            spec_type = OLD_SPEC;
        }
    
        if (NAND_ID_Table[NTYPE].id  == 0xD7) {
            if ((vendor_id==0x3829D5)) {  /*新制程*/ //0x3829D5 K9LBG08U0D
                if ((mf_id == MF_SAMSUNG) || (mf_id == MF_HYNIX)) {
                    spec_type = NEW_BLOCK_KB_SPEC;
                }
            }        
        } else if (NAND_ID_Table[NTYPE].id  == 0xD5) {
            //if ((vendor_id&0xFF000000)!=0) {  /*新制程*/
                if (mf_id == MF_HYNIX) {
                    if (vendor_id==0x442594) {  //H27UAG8T2ATR
                        spec_type = NEW_BLOCK_KB_SPEC;
                    }
                } 
           // } 
            if (mf_id == MF_SAMSUNG)  {  // SAMSUNG special case
                if ((vendor_id&0xFF00) == 0x2900) {
                spec_type = NEW_BLOCK_KB_SPEC;
                } 
            }
        } 
    } else {  /*若 ID 不在 Table 中, 則一律用預設萬用參數, 確保可以讀到 Nand data 即可*/
        spec_type = DEFAULT_SPEC;
    }

    /*由 ID_4th 來決定初步參數值*/
    page_and_block_size_id4th_set(spec_type, vendor_id);

    if (nand_status == NAND_ROM_FIND) 
    {
        gPhysical_NandInfo->wPageSize= 512;
        if (chip_id==0xDA) {
         //   nand_total_size = 256;  // 256MB=2Gb
        }
    }

    nand_total_MByte_size_set(NAND_ID_Table[NTYPE].total_MB_size);

    gPhysical_NandInfo->wAddrCycle = 5;
    if (nand_total_MByte_size_get() !=0) 
    {
        if (nand_total_MByte_size_get()<=32) {        
            gPhysical_NandInfo->wAddrCycle=3;    /*  4/8/16/32 MB => 3 cycles */   
        } else if (nand_total_MByte_size_get() <= 128) {
            gPhysical_NandInfo->wAddrCycle=4;    /* 64/128 MB => 4cycles*/  
        } else {
            gPhysical_NandInfo->wAddrCycle=5;    /* 256MB/512MB/1GB/2GB/4GB/8GB/16GB/32GB/64GB => 5cycles*/  
        }
       #if 0
         if (NAND_ID_Table[NTYPE].total_MB_size >= 64) {
             erase_cycle = 3;  // erase cycle 只有 erase 時會用到
         } else {        
             erase_cycle = 2;
         }
       #endif
    }

    return nand_status;

}

UINT32 page_and_block_size_id4th_set(ID4TH_SPEC spec_type, UINT32 vendor_idx)
{
    volatile UINT16 id_4th;
    UINT8 page_size_square_id;
    UINT8 blk_size_id = 0; //,blknums_id;
    UINT8 tbl_val;
    UINT16 tbl_block_pages;
    UINT16 tbl_page_size;
    UINT16 tbl_nand_size;
    UINT16 nand_size=0;

    id_4th = (vendor_idx&0xff00)>>8;
        
    page_size_square_id = id_4th&0x03;

    if (spec_type == NEW_BLOCK_KB_SPEC) 
    {
        page_size_square_id += 1;  /*NEW KB SPEC 00b 就是代表 2K, Old 跟NEW PG 00 還是1K*/
    }
                  
    if (GET_PAGE_SIZE(NAND_ID_Table[NTYPE].BlockNums_BlockSize_PageSize) == _512_BYTES )
    {
        gPhysical_NandInfo->wPageSize = 512;      
    } else { 
    	switch (page_size_square_id)
	    {
    	  case 1://2048       // OLD 1, NEW 0  
            gPhysical_NandInfo->wPageSize = 2048; 
            //i=1;     // 2^1 =2048 KB
  					break;  			
    	  case 2://4096		 // OLD 2, NEW 1  
						gPhysical_NandInfo->wPageSize = 4096; 	
		            //i=2;    // 2^2 = 4096 KB  (Page size)
		  			break;
    	  case 3://8192		 // OLD 3, NEW 2     
						gPhysical_NandInfo->wPageSize = 8192; 
			            page_size_square_id=2;
			            //i=3;   // 2^3 = 8192 KB  (Page size)
						break;
				default: //2048     	    
            gPhysical_NandInfo->wPageSize = 2048;  
            //ECC_bit =1;
            page_size_square_id=1;  // 2^1 = 2048 KB (Page size)
						break;
        }  
    }
  
    if (spec_type == OLD_SPEC) {
   	    blk_size_id  = (id_4th&0x30)>>4;  // only 3Bits effect
    } else if (spec_type ==  NEW_BLOCK_KB_SPEC) {
        blk_size_id  = (id_4th&0x30)>>4;  // only 3Bits effect
        blk_size_id+=1;
    } else if (spec_type ==  NEW_BLOCK_PG_SPEC) {  // Micron 新的就用這一套
        blk_size_id  = (id_4th&0x70)>>4;  // only 3Bits effect
    }
        
    if ((spec_type == NEW_BLOCK_KB_SPEC) || (spec_type == OLD_SPEC)) 
    {
        if (blk_size_id==0) {             // OLD 0   
    				gPhysical_NandInfo->wBlkPageNum = 64 >> page_size_square_id;  //  64KB
        } else if (blk_size_id==1) {      // OLD 1 , NEW 0
        		gPhysical_NandInfo->wBlkPageNum = 128 >> page_size_square_id; // 128KB				
        } else if (blk_size_id==2) {      // OLD 2 , NEW 1
            gPhysical_NandInfo->wBlkPageNum = 256 >> page_size_square_id; // 256KB				
        } else if (blk_size_id==3) {      // OLD 3 , NEW 2
            gPhysical_NandInfo->wBlkPageNum = 512 >> page_size_square_id; // 512KB
        } else if (blk_size_id==4 && spec_type == NEW_BLOCK_KB_SPEC) {  // NEW 3
            gPhysical_NandInfo->wBlkPageNum = 1024 >> page_size_square_id; // 1024KB
        }  else {
    				gPhysical_NandInfo->wBlkPageNum = 64; // Compatible value
        }
    } 
    else if (spec_type == NEW_BLOCK_PG_SPEC) 
    {
        if (blk_size_id==0) {
            gPhysical_NandInfo->wBlkPageNum = 16;
        } else if (blk_size_id==1) {
            gPhysical_NandInfo->wBlkPageNum = 32;
        } else if (blk_size_id==2) {                    
            gPhysical_NandInfo->wBlkPageNum = 64;
        } else if (blk_size_id==3) {
            gPhysical_NandInfo->wBlkPageNum = 128;
        } else if (blk_size_id==4) {
            gPhysical_NandInfo->wBlkPageNum = 256;
        }  else {
            gPhysical_NandInfo->wBlkPageNum = 64;
        }				
    }
    
    if (spec_type == DEFAULT_SPEC) {
        NTYPE=ID_TBL_SIZE-1; 
    }

    tbl_val = GET_PAGE_SIZE(NAND_ID_Table[NTYPE].BlockNums_BlockSize_PageSize);
    if (tbl_val==_512_BYTES) {
        tbl_page_size = 512;
    } else if (tbl_val==_1024_BYTES) {
        tbl_page_size = 1024;
    } else if (tbl_val==_2048_BYTES) {
        tbl_page_size = 2048;
    } else if (tbl_val==_4096_BYTES) {
        tbl_page_size = 4096;
    } else {
        tbl_page_size = 2048;
    } 
    tbl_val = GET_BLOCK_SIZE(NAND_ID_Table[NTYPE].BlockNums_BlockSize_PageSize);
    if (tbl_val==_32_PAGES) {
        tbl_block_pages = 32;
    } else if (tbl_val==_64_PAGES) {
        tbl_block_pages = 64;
    } else if (tbl_val==_128_PAGES) {
        tbl_block_pages = 128;
    } else if (tbl_val==_256_PAGES) {
        tbl_block_pages = 256;
    } else {
        tbl_block_pages = 64;
    }

    tbl_nand_size = NAND_ID_Table[NTYPE].total_MB_size;


    if ((spec_type == TABLE_SPEC) || (spec_type == DEFAULT_SPEC)) 
    {
        gPhysical_NandInfo->wPageSize = tbl_page_size;
        gPhysical_NandInfo->wBlkPageNum = tbl_block_pages;
    }    	    	

    if (gPhysical_NandInfo->wBlkPageNum<tbl_block_pages) {
        gPhysical_NandInfo->wBlkPageNum = tbl_block_pages;
    }                                                                                                                                                          


    /* get the extend information by nand flash id info. */    

    /*table 表的 nand size 僅供預設, 真正 Size 得請 Auto Config 讀 Header 獲得,
      該數值對於 Nand driver L1 是沒有意義的, 因此由 Nand Manager (DrvL2) 獲得也不晚*/
    if (nand_size<tbl_nand_size) 
    {
       nand_size = tbl_nand_size;
    }

    gPhysical_NandInfo->wPageSectorSize = gPhysical_NandInfo->wPageSize/512;
    //gPhysical_NandInfo->wBlkPageNum  = page_nums_per_block;
    gPhysical_NandInfo->wNandBlockNum =(((nand_size*1024)/gPhysical_NandInfo->wPageSize)/nand_page_nums_per_block_get())*1024;
    //nand_block_Byte_size = gPhysical_NandInfo->wPageSize*gPhysical_NandInfo->wBlkPageNum;
    
	return 0;
}
SINT32 special_nf_detect(UINT16 main_id, UINT32 vendor_id)
{
    UINT16 i;
    SINT32 ret=-1;
    SINT32 table_size = sizeof(Special_NF_Tbl)/sizeof(Special_NF_Tbl[0]);

    printk ("SPECIAL NF TBL Hit (0x%x, 0x%x)....\n",main_id,vendor_id);

    for (i=0;i<table_size;i++)
    {
//        if((Special_NF_Tbl[i].MainID == main_id) && (Special_NF_Tbl[i].VenDorID == vendor_id))
        if(Special_NF_Tbl[i].MainID == main_id)
        {
            nand_page_size_set(Special_NF_Tbl[i].PageSize);
            nand_page_nums_per_block_set(Special_NF_Tbl[i].BlockSize);
            nand_addr_cycle_set(Special_NF_Tbl[i].AddressCycle); 
            nand_total_MByte_size_set(Special_NF_Tbl[i].TotalMB);
            nand_bch_set(Special_NF_Tbl[i].BCHMode);
            ret=0;
            break;
        }
    }

    if (ret==0) {
        printk ("NandFlash 0x%02X is found!!\r\n", main_id);
    } else {
        printk ("NandFlash 0x%02X is NOT found in Special table\r\n", main_id);
    }

    return ret;
}
