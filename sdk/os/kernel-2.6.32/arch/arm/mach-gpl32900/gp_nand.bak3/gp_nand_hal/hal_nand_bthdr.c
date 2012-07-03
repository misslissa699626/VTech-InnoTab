#include "hal_nand_bthdr.h"
#include <mach/diag.h>

SINT16 check_key_status_get(UINT8 *bt_hdr_buf);
UINT16 iotrap_info_get(UINT8 bit_offset, BIT_NUMS_MASK bit_nums);
UINT32 header_info_get(UINT16 byte_offset, HDR_VAL_LEN ValueSize,UINT8 *buffer);

#define NF_GOOD_BLK_TAG    0xFF

#if 1//#if (defined DRV_L1_NAND) && (DRV_L1_NAND==1)

static UINT8 hdr_sw_fuse;
SINT32 nf_hdr_get_from_bt_area(UINT8* header);
SINT16 hdr_check_key_count(UINT8 * bt_hdr_buf);
SINT32 nf_hdr_to_info(UINT8* header, HEADER_NF_INFO *nv_info);
UINT8  hdr_sw_fuse_get(SW_FUSE_ENUM bit_index);

void bch_boot_area_init(void);
void spare_buf_init(void);

SINT32 nf_hdr_get_from_bt_area(UINT8* header)
{
    UINT32 i;
    UINT32 *HdrWord=(UINT32 *)&header[0];
    UINT16 *HdrShort=(UINT16 *)&header[0];
    UINT16 bthdr_bch_mode;
    UINT16 max_hdr_cpy=1280;
    //UINT16 blk_id;
    UINT32 ret;
    
    bch_boot_area_init();
    spare_buf_init();

#if 0
    if (iotrap_info_get(BOOT_RANDOMIZE_TRAP, BOOT_RANDOMIZE_TRAP_BITS)==1)
    {
        //DBG_PRINT ("SIO Trap: BtLdr Randomize Start\r\n");
        NandRandomInit();
        NandRandomEn(1);
    } else {
        NandRandomEn(0);
    }
#endif       
     
    for (i=0;i<max_hdr_cpy;i++) /*MAX Header Copy Times is 1280*/
    { 
    	
 #if 0   // Dominant mark, no need to detect bad block in BootArea (BootHeader domain)  	 
        if ((i%nand_block_size_get())==0) {
            blk_id = i/nand_block_size_get();
            if(good_block_check(blk_id, (UINT32) &header[0])!=NF_GOOD_BLK_TAG)
            {
                /*發現壞塊*/
                i += nand_block_size_get()-1;
                continue;
            }
        }
 #endif       
        ret = Nand_RandomReadPhyPage(i, (UINT32) &header[0]);
        if(ret==STATUS_OK) 
        {
            if (HdrWord[0] == TAG_NAND)
            {
                break;  // GET Nand Header Well
            }
        }
        else 
        {   // 讀失敗不要灰心, 可能參數設錯而已, 看有沒有後備做法
            if (check_key_status_get((UINT8 *)header)==STATUS_OK)  // 有 Optional Parameter
            {
                bthdr_bch_mode = header[HDR_NF_BTH_BCH_ID];
                if (bthdr_bch_mode != 0)   /*Header有指示喔*/
                { 
                    /*Modify BootHeader Read Parameter*/
                    nand_bch_set(bthdr_bch_mode);
                    /*再讀一次看看, 說不定就對了*/
                    if(Nand_RandomReadPhyPage(i, (UINT32) &header[0])==STATUS_OK) 
                    {
                        if (HdrWord[0] == TAG_NAND)
                        {
                            break;  // Bingo!!! GET Nand Header Well
                        }
                    }
                } 
                if (HdrShort[HDR_BTHDR_COPIES_ID]>1) {
                    max_hdr_cpy = HdrShort[HDR_BTHDR_COPIES_ID];
                }
            } 
            continue;
        }
    }

    if (i==max_hdr_cpy) {
        bch_boot_area_uninit();
        return -1;
    } else {
        bch_boot_area_uninit();
        return 0; // Header Read  from Boot Area well;
    }
    
}

static void dump_bch_name(UINT8 bch_id);

SINT32 nf_hdr_to_info(UINT8* header, HEADER_NF_INFO *nv_info)
{ 
    UINT32 *HdrWord=(UINT32 *)&header[0];
    UINT16 *HdrShort=(UINT16 *)&header[0];
    UINT8 HpId;
    UINT8 smart_flow_flag=0;

    nv_info->block_size = 32;
        
    if (check_key_status_get((UINT8 *)header)==STATUS_OK)  // 有 Optional Parameter
    {
       // DBG_PRINT("====Check Key Get, Config NVRAM by BtHdr=====\n"); 
        nv_info->bthdr_bch = header[HDR_NF_BTH_BCH_ID];      
        nv_info->bthdr_page_size = gPhysical_NandInfo->wPageSize; // 次恰為 Intelligent ID 之 PageSIze
        nv_info->bthdr_page_sectors = nv_info->bthdr_page_size/512;
               
               
		HpId = header[HDR_NF_PAGE_SIZE_ID];
		if(HpId == 0)
		{
			nv_info->btldr_page_size = 512;		
		} else {
			nv_info->btldr_page_size = HpId*1024;
		}	
		nv_info->btldr_page_sectors = nv_info->btldr_page_size/512;
		nv_info->btldr_bch = header[HDR_NF_BCH_ID];
    	nv_info->addr_cycle = header[HDR_NF_ADDR_CYC_ID];
        nv_info->block_size = header[HDR_NF_BLK_SIZE_ID]*32; 
		smart_flow_flag=0;	
    } 
    else   /*沒有 Header information 就要走智能 ID*/ 
    {

       // DBG_PRINT("====Check Key Fail, Entry Smart ID=====\n"); 
    
        nand_intelligent_id_init(Nand_MainId_Get(), Nand_VendorId_Get());
        nv_info->bthdr_page_size = gPhysical_NandInfo->wPageSize;

        if (nv_info->bthdr_page_size==512) 
        {
            nv_info->bthdr_bch = BCH512B4_BITS_MODE;
        } else {
            nv_info->bthdr_bch = BCH1K60_BITS_MODE;
        }
        /*日後智能 ID 要有帶 Exception table 才會完全正確*/
        nand_smart_id_init(Nand_MainId_Get(), Nand_VendorId_Get());
        nv_info->btldr_page_size = gPhysical_NandInfo->wPageSize;
        nv_info->block_size = gPhysical_NandInfo->wBlkPageNum;
        header[HDR_NF_BLK_SIZE_ID] = (nv_info->block_size)/32;
        /*我希望 BCH 是 Polling 出來的*/
        nv_info->btldr_bch = header[HDR_NF_BCH_ID]; 
        nv_info->addr_cycle = gPhysical_NandInfo->wAddrCycle;
        header[HDR_NF_ADDR_CYC_ID] = nv_info->addr_cycle;
        smart_flow_flag=1;

    }

    nv_info->btldr_set_pages = HdrShort[HDR_NV_BTLDR_SIZE_ID/2];
    nv_info->bthdr_page_sectors = nv_info->bthdr_page_size/512;
    nv_info->bthdr_copies = HdrShort[HDR_BTHDR_COPIES_ID/2];
    nv_info->btldr_copies = HdrShort[HDR_BTLDR_COPIES_ID/2];
		nv_info->btldr_start_page_id = HdrShort[HDR_NV_BTLDR_START_ID/2];

		nv_info->app_start = HdrShort[HDR_NV_RTCODE_START_ID/2];

		nv_info->nand_type = header[HDR_NF_TYPE_ID];
		nv_info->StrongEn = header[HDR_NF_STRON_PAGE_EN_ID];
		nv_info->swfuse = header[HDR_SW_FUSE_ID];
    // Total MB Size 是由 GUI 指定的
		nv_info->total_MB_size = HdrShort[HDR_NVRAM_MB_SIZE_ID/2];
    nv_info->app_tag = HdrShort[HDR_APP_TAG_ID/2];
    nv_info->app_size= HdrWord[HDR_APP_SIZE_ID/4]; // APP Size unit is Sectors
    nv_info->app_perscent= HdrShort[HDR_APP_PERCENT_ID/2];
    nv_info->app_rtcode_run_addr = HdrWord[HDR_RTCODE_RUN_ADDR_ID/4];
    nv_info->nf_wp_pin = header[HDR_NF_WP_PIN_ID]; // Dominant new add, 2011/03/09
    
    nv_info->NoFSSize	 				= header[49];
    
    nv_info->partitionNum 		= header[48];
    
    nv_info->part[0].attr			=	header[50];
    nv_info->part[0].MB_size 	=	HdrShort[52/2];
    nv_info->part[1].attr			=	header[51];
    nv_info->part[1].MB_size	=	HdrShort[54/2];
    nv_info->part[2].attr			=	header[290];
    nv_info->part[2].MB_size	=	HdrShort[292/2];
    nv_info->part[3].attr			=	header[291];
    nv_info->part[3].MB_size	=	HdrShort[294/2];
    nv_info->part[4].attr			=	header[298];
    nv_info->part[4].MB_size	=	HdrShort[300/2];
    nv_info->part[5].attr			=	header[299];
    nv_info->part[5].MB_size	=	HdrShort[302/2];
    nv_info->part[6].attr			=	header[306];
    nv_info->part[6].MB_size	=	HdrShort[308/2];
    nv_info->part[7].attr			=	header[307];
    nv_info->part[7].MB_size	=	HdrShort[310/2];
    
#if 0  // Work and run function // 因為 CodePacker 有問題而改出來的

    #define RTCODE_SIZE 30720

    nv_info->app_start = nv_info->block_size * 1;
    HdrShort[HDR_NV_RTCODE_START_ID/2] = nv_info->app_start;
    nv_info->app_size = RTCODE_SIZE*2;  // 15MB = 30720 Sectors 
    HdrWord[HDR_APP_SIZE_ID/4] = nv_info->app_size;    
    nv_info->app_perscent = 1;
    HdrShort[HDR_APP_PERCENT_ID/2] = nv_info->app_perscent;
    nv_info->app_rtcode_run_addr = 0x1000000; 
    HdrWord[HDR_RTCODE_RUN_ADDR_ID/4] = nv_info->app_rtcode_run_addr;
    nv_info->app_rtcode_size = RTCODE_SIZE;
    HdrWord[HDR_RTCODE_SIZE_ID/4]= nv_info->app_rtcode_size;
    
#endif

    if (smart_flow_flag==1)  /*Re-Building Check Key*/
    {
        HdrShort[HDR_NF_CHECK_KEY_ID/2]=hdr_check_key_count((UINT8 *) &header[0]);        
    }

		//DBG_PRINT("\r\n=====Boot Header Information======\n", nv_info->bthdr_copies);
		//DBG_PRINT("boot header copys is: %d\n", nv_info->bthdr_copies);
		//DBG_PRINT("boot header BCH is: %d\n", nv_info->bthdr_bch);
    //DBG_PRINT("boot header BCH mode: ");
    dump_bch_name(nv_info->bthdr_bch);
    //DBG_PRINT ("WP Pin ID: %d (port:%d, PinId:%d)\r\n",nv_info->nf_wp_pin,nv_info->nf_wp_pin/32,nv_info->nf_wp_pin%32);
    //DBG_PRINT("boot header page size : %d Bytes\n", nv_info->bthdr_page_size);

		//DBG_PRINT("=====Boot loader Information======\n", nv_info->bthdr_copies);		
		//DBG_PRINT("boot loader page is: %d\n", nv_info->btldr_page_size);
		//DBG_PRINT("boot loader BCH mode: ");
    dump_bch_name(nv_info->btldr_bch);   
    //DBG_PRINT("boot loader start page id: %d\n", nv_info->btldr_start_page_id);  
		//DBG_PRINT("boot loader Set size is: %d (pages)\n", nv_info->btldr_set_pages);
		//DBG_PRINT("nand block size: %d\n", nv_info->block_size);

    //DBG_PRINT("=====App Area Information======\n");
    //DBG_PRINT("App Start Page ID: %d\n", nv_info->app_start);
    //DBG_PRINT("App Size: %d Sectors\n", nv_info->app_size);
    //DBG_PRINT("App Bank Perscent: %d\n", nv_info->app_perscent);
    //DBG_PRINT("App RtCode Run Addr: 0x%08x\n", nv_info->app_rtcode_run_addr);  
		//DBG_PRINT("=====Information End======\n");	
		
		return 0;

}

static void dump_bch_name(UINT8 bch_id)
{
    switch (bch_id)
    {
        case 0x00:
            //DBG_PRINT("1K60");
            break;
        case 0x01:
            //DBG_PRINT("1K40");
            break;            
        case 0x02:
            //DBG_PRINT("1K24");
            break;
        case 0x03:
            //DBG_PRINT("1K16");
            break;
        case 0x04:
            //DBG_PRINT("512B8");
            break;
        case 0x05:
            //DBG_PRINT("512B4");
            break;
        case 0xFF:
            //DBG_PRINT("BCH_OFF");
        default:
            //DBG_PRINT("Unknown");            
            break;     
    }
    //DBG_PRINT("\r\n");
}

#endif

#if 0
SINT32 sdc_hdr_to_info(UINT8* header, HEADER_SDC_INFO *nv_info)
{ 
    UINT32 *HdrWord=(UINT32 *)&header[0];
    UINT16 *HdrShort=(UINT16 *)&header[0];
    UINT8 HpId;
    UINT8 smart_flow_flag=0;

    nv_info->block_size = 32;
        

    nv_info->bthdr_page_size = 512; // 次恰為 Intelligent ID 之 PageSIze
    nv_info->bthdr_page_sectors = 1;
           
           
	HpId = header[HDR_NF_PAGE_SIZE_ID];
	if(HpId == 0)
	{
		nv_info->btldr_page_size = 512;		
	} else {
		nv_info->btldr_page_size = HpId*1024;
	}	
	nv_info->btldr_page_sectors = nv_info->btldr_page_size/512;
	nv_info->btldr_bch = header[HDR_NF_BCH_ID];
	nv_info->addr_cycle = header[HDR_NF_ADDR_CYC_ID];
    nv_info->block_size = header[HDR_NF_BLK_SIZE_ID]*32; 

    nv_info->btldr_set_pages = HdrShort[HDR_NV_BTLDR_SIZE_ID/2];
    nv_info->bthdr_page_sectors = nv_info->bthdr_page_size/512;
    nv_info->bthdr_copies = HdrShort[HDR_BTHDR_COPIES_ID/2];
    nv_info->btldr_copies = HdrShort[HDR_BTLDR_COPIES_ID/2];
	nv_info->btldr_start_page_id = HdrShort[HDR_NV_BTLDR_START_ID/2];

	nv_info->app_start = HdrShort[HDR_NV_RTCODE_START_ID/2];

	nv_info->nand_type = header[HDR_NF_TYPE_ID];
	nv_info->StrongEn = header[HDR_NF_STRON_PAGE_EN_ID];
	nv_info->swfuse = header[HDR_SW_FUSE_ID];
    // Total MB Size 是由 GUI 指定的
	nv_info->total_MB_size = HdrShort[HDR_NVRAM_MB_SIZE_ID/2];
    nv_info->app_tag = HdrShort[HDR_APP_TAG_ID/2];
    nv_info->app_size= HdrWord[HDR_APP_SIZE_ID/4];
    nv_info->app_perscent= HdrShort[HDR_APP_PERCENT_ID/2];
    nv_info->app_rtcode_run_addr = HdrWord[HDR_RTCODE_RUN_ADDR_ID/4];
    hdr_sw_fuse=nv_info->swfuse;
#if 0  // Work and run function // 因為 CodePacker 有問題而改出來的

    #define RTCODE_SIZE 30720

    nv_info->app_start = nv_info->block_size * 1;
    HdrShort[HDR_NV_RTCODE_START_ID/2] = nv_info->app_start;
    nv_info->app_size = RTCODE_SIZE*2;  // 15MB = 30720 Sectors 
    HdrWord[HDR_APP_SIZE_ID/4] = nv_info->app_size;    
    nv_info->app_perscent = 1;
    HdrShort[HDR_APP_PERCENT_ID/2] = nv_info->app_perscent;
    nv_info->app_rtcode_run_addr = 0x1000000; 
    HdrWord[HDR_RTCODE_RUN_ADDR_ID/4] = nv_info->app_rtcode_run_addr;
    nv_info->app_rtcode_size = RTCODE_SIZE;
    HdrWord[HDR_RTCODE_SIZE_ID/4]= nv_info->app_rtcode_size;
    
#endif

    if (smart_flow_flag==1)  /*Re-Building Check Key*/
    {
        HdrShort[HDR_NF_CHECK_KEY_ID/2]=hdr_check_key_count((UINT8 *) &header[0]);        
    }

		//DBG_PRINT("\r\n=====Boot Header Information======\n", nv_info->bthdr_copies);
		//DBG_PRINT("boot header copys is: %d\n", nv_info->bthdr_copies);
		//DBG_PRINT("boot header BCH is: %d\n", nv_info->bthdr_bch);

    //DBG_PRINT("boot header page size : %d Bytes\n", nv_info->bthdr_page_size);

		//DBG_PRINT("=====Boot loader Information======\n", nv_info->bthdr_copies);		
		//DBG_PRINT("boot loader page is: %d\n", nv_info->btldr_page_size);
   
    //DBG_PRINT("boot loader start page id: %d\n", nv_info->btldr_start_page_id);  
		//DBG_PRINT("boot loader Set size is: %d (pages)\n", nv_info->btldr_set_pages);
		//DBG_PRINT("nand block size: %d\n", nv_info->block_size);

    //DBG_PRINT("=====App Area Information======\n");
    //DBG_PRINT("App Start Page ID: %d\n", nv_info->app_start);
    //DBG_PRINT("App Size: %d Sectors\n", nv_info->app_size);
    //DBG_PRINT("App Bank Perscent: %d\n", nv_info->app_perscent);
    //DBG_PRINT("App RtCode Run Addr: 0x%08x\n", nv_info->app_rtcode_run_addr);  
		//DBG_PRINT("=====Information End======\n");	
}
#endif

SINT16 check_key_status_get(UINT8 *bt_hdr_buf)
{
    SINT16 check_key;
    SINT16 nv_key;
    UINT16 *HdrShort = (UINT16*)bt_hdr_buf;
    
    check_key = 0xA55A+(bt_hdr_buf[HDR_NF_ADDR_CYC_ID] <<8) + 
        (bt_hdr_buf[HDR_NF_PAGE_SIZE_ID] * 1024) + 
        (bt_hdr_buf[HDR_NF_BLK_SIZE_ID] * 32) +
        (bt_hdr_buf[HDR_NF_BCH_ID])+
        (bt_hdr_buf[HDR_NF_BTH_BCH_ID] * 32) +
        (HdrShort[HDR_BTHDR_COPIES_ID/2]);

    nv_key = header_info_get(HDR_NF_CHECK_KEY_ID, HDR_2Bytes,bt_hdr_buf);
    if (nv_key==check_key) {
        return 0;
    }
    else {
        return -1;
  
    }
}


SINT16 hdr_check_key_count(UINT8 * bt_hdr_buf)
{
    SINT16 check_key;
		//UINT32 *HdrWord	= (UINT32*)bt_hdr_buf;
		UINT16 *HdrShort = (UINT16*)bt_hdr_buf;

    check_key = 0xA55A+(bt_hdr_buf[HDR_NF_ADDR_CYC_ID] <<8) + 
        (bt_hdr_buf[HDR_NF_PAGE_SIZE_ID] * 1024) + 
        (bt_hdr_buf[HDR_NF_BLK_SIZE_ID] * 32) +
        (bt_hdr_buf[HDR_NF_BCH_ID])+
        (bt_hdr_buf[HDR_NF_BTH_BCH_ID] * 32) +
        (HdrShort[HDR_BTHDR_COPIES_ID/2]);

    return check_key;
}

UINT32 header_info_get(UINT16 byte_offset, HDR_VAL_LEN ValueSize,UINT8 *buffer)
{
    UINT32 ret=0;
    UINT16 i,j;

    j=0;
    for (i=byte_offset;i<(byte_offset+ValueSize);i++)
    {
        ret |= buffer[i] << j;
        j+=8;
    }
    return ret;
}


UINT16 iotrap_info_get(UINT8 bit_offset, BIT_NUMS_MASK bit_nums)
{
    UINT16 value;
    UINT32 io_trap_val = DRV_Reg32(0x90005050);
    value = (io_trap_val>>bit_offset)&bit_nums;
    return value;
}



UINT8 hdr_sw_fuse_get(SW_FUSE_ENUM bit_index)
{
    return ((hdr_sw_fuse>>bit_index) & 0x1);    
}


HEADER_NF_INFO  nand_info;
SINT32 NandParseBootHeader(UINT8 *Buffer)
{
	SINT32 ret;
	
	if(Nand_Pad_Scan(NAND_PAD_AUTO)!=NAND_NONE)//if(Nand_Pad_Scan(NAND_NON_SHARE)!=NAND_NONE)//if(Nand_Pad_Scan(NAND_PAD_AUTO)!=NAND_NONE)
  {        
     nand_intelligent_id_init(Nand_MainId_Get(),Nand_VendorId_Get()); 
     
     DIAG_INFO ("==Scan Nand Pad OK!!==\n");
     
     DIAG_INFO("Main ID:0x%x \n",Nand_MainId_Get());
     DIAG_INFO("Vendor ID:0x%x \n",Nand_VendorId_Get());       
  } 
  else 
  {
  	DIAG_INFO ("==Scan Nand Pad Fail!!==\n");
  	
  	return -1;
	}
	
	ret = nf_hdr_get_from_bt_area(Buffer);
	if(ret!=0)
	{
		DIAG_INFO ("==Get nand info from boot head failed!!==\n");
		return -1;
	}
	
	nf_hdr_to_info(Buffer, &nand_info);	
 	
	nand_page_size_set(nand_info.btldr_page_size);
	nand_page_nums_per_block_set(nand_info.block_size);  	
	nand_bch_set(nand_info.btldr_bch);
	nand_addr_cycle_set(nand_info.addr_cycle);
	nand_total_MByte_size_set(nand_info.total_MB_size);
	
	DIAG_INFO ("nand_info.btldr_page_size: 0x%x \n",nand_info.btldr_page_size);
	DIAG_INFO ("nand_info.bthdr_page_size: 0x%x \n",nand_info.bthdr_page_size);
	DIAG_INFO ("nand_info.block_size: 0x%x \n",			nand_info.block_size);
	DIAG_INFO ("nand_info.bthdr_bch: 0x%x \n",			nand_info.bthdr_bch);
	DIAG_INFO ("nand_info.btldr_bch: 0x%x \n",			nand_info.btldr_bch);
	DIAG_INFO ("nand_info.addr_cycle: 0x%x \n",			nand_info.addr_cycle);
	DIAG_INFO ("nand_info.NoFSSize: 0x%x \n",				nand_info.NoFSSize);
	
	DIAG_INFO ("nand_info.total_MB_size: 0x%x \n",	nand_info.total_MB_size);
	
	return 0;
}

UINT16 GetAppStartBlkFromBth(void)
{
	return (nand_info.app_start/nand_block_size_get());	// page / block	
}

UINT16 GetAppSizeOfBlkFromBth(void)
{		
	UINT32 app_size_pages;
	UINT16 app_size_blk;	

	app_size_pages = (nand_info.app_size*512/nand_page_size_get());
	app_size_blk = app_size_pages/nand_block_size_get();
	if((app_size_pages%nand_block_size_get())!=0)
	{
			app_size_blk++;

	}
	return (app_size_blk);
}

UINT16 GetAppPercentFromBth(void)
{
	return (UINT16)(nand_info.app_perscent);
}

UINT16 GetDataBankLogicSizeFromBth(void)
{
	return  ((nand_page_size_get()>512)?512:200);
}

UINT16 GetDataBankRecycleSizeFromBth(void)
{
	return  ((nand_page_size_get()>512)?64:16);
}

UINT32 GetNoFSAreaSectorSizeFromBth(void)
{
	UINT32 sectors;
	
	sectors 	= nand_info.NoFSSize*1024*2;

	return (sectors);
}

UINT8 GetPartNumFromBth(void)
{
	return (nand_info.partitionNum);
}

void GetPartInfoFromBth(UINT8 part,UINT16 *MB_size,UINT8 *attr)
{
		*attr 		=	nand_info.part[part].attr;
    *MB_size 	= nand_info.part[part].MB_size;
}