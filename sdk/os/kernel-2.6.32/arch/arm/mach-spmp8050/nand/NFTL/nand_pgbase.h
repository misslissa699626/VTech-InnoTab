#ifndef _NAND_PGBASE_H_
#define _NAND_PGBASE_H_

#include "../champ/NF_cfgs.h"

#ifdef	_WIN32
#ifndef SUCCESS
#define SUCCESS 0
#endif

#ifndef FAILURE
#define FAILURE -1
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef NO_BLOCK
#define NO_BLOCK (unsigned short)-1
#endif




#define RETRY_COUNT 10
#endif

#define NPB_DEBUG 1

#define NPB_SECTOR_SIZE 512
#define ALIGNED64 64
#define MAX_FIFO_RESERVATIONS 3
//#define NAND_PGBASE_USED_BLK 32
//#define NAND_PGBASE_USED_LOGSECTOR (NAND_PGBASE_USED_BLK<<(6+2)) //128 page(2k), 16 blk
#define MAX_BLK_PER_PAGE 256
//#define 

typedef struct NPB_Phy2Log
{
	unsigned short logpage;
	unsigned short reserved;
	unsigned int version;
}NPB_Phy2Log_t;

typedef struct NPB_Log2Phy
{
	unsigned short phypage;
	unsigned short reserved;
	unsigned int  version;
}NPB_Log2Phy_t;

typedef struct NPB_Phy_blk_Info
{
	unsigned short count;
	unsigned char  In_FIFO;
	unsigned char  reserved;
	//unsigned char *pInfo;
	unsigned char pInfo[(MAX_BLK_PER_PAGE>>3)];
}NPB_Phy_blk_Info_t;

typedef struct NPB_FIFO_Manage_Data
{
	unsigned short p_index;
	unsigned short c_index;
	unsigned int size;
	unsigned short blk_count;
	unsigned char Recovering;
	unsigned char reserved;
}NPB_FIFO_Manage_Data_t;

typedef struct NPB_FIFO_Manage
{
	unsigned short p_index;
	unsigned short c_index;
	unsigned int size;
	unsigned short blk_count;
	unsigned char Recovering;
	unsigned char reserved;
	unsigned short *plist;
}NPB_FIFO_Manage_t;

typedef struct NPB_Info_Data
{
	unsigned short table_save_start_blk;
	unsigned short table_save_blk_count;
	unsigned short start_blk;
	unsigned short phyblk_count;
	unsigned short logblk_count;	
	
	unsigned short pagesize;
	
	unsigned short current_blk;
	unsigned short current_page;
	unsigned short page_per_block;
	unsigned char  sector_per_page;
	unsigned char  blockshift;
	unsigned char  pageshift;
	unsigned char  sectorhift;
	unsigned short logsector_count;
	//unsigned short reserved;
}NPB_Info_Data_t;


typedef struct NPB_Info
{
	unsigned short table_save_start_blk;
	unsigned short table_save_blk_count;
	unsigned short start_blk;
	unsigned short phyblk_count;
	unsigned short logblk_count;	
	
	unsigned short pagesize;
	
	unsigned short current_blk;
	unsigned short current_page;
	unsigned short page_per_block;
	unsigned char  sector_per_page;
	unsigned char  blockshift;
	unsigned char  pageshift;
	unsigned char  sectorhift;
	unsigned short logsector_count;
	//unsigned short reserved;

	unsigned char* pReadBuf;
	unsigned char* pReadBuf_ali;
	unsigned char* pReadBuf_ali_NC;
	unsigned char* pWriteBuf;
	//unsigned char* pWriteBuf_ali;
	//unsigned char* pWriteBuf_ali_NC;
	
	unsigned char* pWriteBuf1;
	unsigned char* pWriteBuf1_ali;
	//unsigned char* pWriteBuf1_ali_NC;
	//unsigned char* pWriteBuf2;
	//unsigned char* pWriteBuf2_ali;
	//unsigned char* pWriteBuf2_ali_NC;


	unsigned char* pEccBuf;
	//unsigned char* pEccBuf_ali;
	//unsigned char* pEccBuf_ali_NC;
	unsigned char* pEccBuf1;
	unsigned char* pEccBuf1_ali;
	//unsigned char* pEccBuf1_ali_NC;
	//unsigned char* pEccBuf2;
	//unsigned char* pEccBuf2_ali;
	//unsigned char* pEccBuf2_ali_NC;	


	NPB_Log2Phy_t *p_l2ptab;
	
	NPB_Phy2Log_t *p_p2ltab;
	
	NPB_Phy_blk_Info_t phy_blk_info[NAND_PGBASE_USED_PHYBLK];

}NPB_Info_t;

#if 0

#else
typedef struct Sector_Info
{
 	unsigned short reserved;  //0's byte for version
 	unsigned short log_page;
	unsigned char bch_parity[28];
}Sector_Info_t;
typedef struct Version_Info
{
 	unsigned int version;
	unsigned char bch_parity[28];
}Version_Info_t;

typedef struct Page_Info
{
	Sector_Info_t sector;
	Version_Info_t Ver;	
}Page_Info_t;

typedef struct Page_Info_24bit
{
 	//unsigned short reserved;  //0's byte for version
	unsigned char reserved[64];
 	unsigned short log_page;
	unsigned char reserved1[62];
 	unsigned short ver1;
	unsigned char reserved2[62];
 	unsigned short ver2;
	unsigned char reserved3[62];	
}Page_Info_24bit_t;

#if 0
typedef struct Sector_mlc_Info
{
 	unsigned char flg;  //0's byte for version
 	unsigned short log_blk;
 	unsigned char reserved;
	unsigned char bch_parity[28];
}Sector_mlc_Info_t;

typedef struct Page_Info_mlc
{
	Sector_mlc_Info_t sector1;
	Sector_mlc_Info_t sector2;	
}Page_Info_mlc_t;
#endif


#endif

#define MAX_NPB_LIST_SIZE 256

typedef struct NPB_list
{
	unsigned int len;
	//unsigned short list[MAX_NPB_LIST_SIZE];
	unsigned short* list;
}NPB_list_t;


#if NPB_SUPPORT_TABLESAVE
#define NPB_HEARD 0x4E504248
#define NPB_PATTERN 0x4B494E47

typedef struct NPB_Store_Info
{
	unsigned int heard;
	unsigned int version;
	unsigned int pattern;
	unsigned int reserved;
		
}NPB_Store_Info_t;
////////////////////////////////////////////////////////////////////////////////////////////////
void NPB_table_erase(void);
unsigned int NPB_table_save(void);
unsigned int NPB_table_load(void);
#endif


////////////////////////////////////////////////////////////////////////////////////////////////
void prin_npb_info(void);

//unsigned int NPB_Init(unsigned short start_blk, unsigned short blk_count, unsigned short page_per_block, unsigned short pagesize, unsigned char Is_First);
unsigned int NPB_Init(unsigned char Is_First);
unsigned int NPB_free(void);
void NPB_FIFO_Get_Info(void);
unsigned int NPB_ReadPage(unsigned short logpage, unsigned char *pdata);
unsigned int NPB_WritePage(unsigned short logpage, unsigned char *pdata);
//unsigned int NPB_read(unsigned int StartSector, unsigned int SectorNum, unsigned char  *pdata);
//unsigned int NPB_write(unsigned int StartSector, unsigned int SectorNum, unsigned char  *pdata);
unsigned int NPB_Creat_log2phy_Table(void);
unsigned short Get_NPB_log_sector_count(void);
unsigned int NPB_Info_free(void);
unsigned short Get_NPB_logpage_count(void);

#define NAND_PGBASE_USED_LOGSECTOR Get_NPB_log_sector_count()
#define NAND_PGBASE_USED_LOGPAGECOUNT Get_NPB_logpage_count()

#endif