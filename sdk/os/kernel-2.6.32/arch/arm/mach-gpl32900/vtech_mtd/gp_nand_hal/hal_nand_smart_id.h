#ifndef _GP_HAL_NAND_SMART_ID_H_
#define _GP_HAL_NAND_SMART_ID_H_
#include "hal_nand.h"

#define _MB
#define ID_TBL_SIZE 			25
//#define TBL_SIZE    			ID_TBL_SIZE     
#define MAX_ID_LEN 6//5                                                                                                                                                                                                                                                   
                                                                                                                                                                                                                                                                                                                                                                                                                                                        
/*===Nand Smart ID table about define Start===*/
 /*Page Size*/
#define _512_BYTES   0x0 /*000b*/
#define _1024_BYTES  0x1 /*001b*/                                                                                                                                     
#define _2048_BYTES  0x2 /*010b*/                                                                                                                                     
#define _4096_BYTES  0x3 /*11b*/                                                                                                                                     
#define _8192_BYTES  0x4 /*100b*/ 
 /*Total Block Nums*/
#define _1024_BLOCKS  (0x0) /*000b*/                                                                                                                                
#define _2048_BLOCKS  (0x4) /*001b*/                                                                                                                                
#define _4096_BLOCKS  (0x8) /*010b*/                                                                                                                                
#define _8192_BLOCKS  (0xC) /*011b*/ 
#define _16384_BLOCKS (0x10) /*100b*/ 
  /*Block Size*/
#define _32_PAGES    (0x0) /*00b*/                                                                                                                                
#define _64_PAGES    (0x20) /*01b*/                                                                                                                                
#define _128_PAGES   (0x40) /*10b*/                                                                                                                                
#define _256_PAGES   (0x60) /*11b*/  
 /*Erase Cyccle*/                                                                                                                                                             
#define _ERASE_CYC2  (0x0) /*0*/                                                                                                                               
#define _ERASE_CYC3  (0x80) /*1*/  

typedef enum {
    OLD_SPEC=0,           	// Old Spec, Block Size units is KB
    TABLE_SPEC=1,  			// Table Spec, follow Table define
    DEFAULT_SPEC=2,   		// No spec, follow last one NTYPE
    NEW_BLOCK_KB_SPEC=3,  	// New Spec, Block Size units is KB
    NEW_BLOCK_PG_SPEC=4,  	// New Spec, Block size units is page nums
    IDTH_SPEC_MAX
} ID4TH_SPEC;

typedef enum {
    MF_SAMSUNG  = 0xEC,
    MF_MICRON   = 0x2C,
    MF_HYNIX    = 0xAD,
    MF_INTEL    = 0x89,
    MF_SPANSION = 0x01,
    MF_TOSHIBA  = 0x98,
    MF_ST       = 0x20,
    MF_MX       = 0xC2,  // Nand ROM factory
    MF_SANDISK  = 0x45  // Nand ROM factory
} NF_MF_ENUM;

typedef struct flash_list_tag
{
   UINT8      id;
   UINT16     total_MB_size;
   UINT8  		BlockNums_BlockSize_PageSize;
}flash_list;

extern NAND_STATUS_ENUM nand_smart_id_init(UINT16 main_id, UINT32 vendor_id);
extern UINT32 page_and_block_size_id4th_set(ID4TH_SPEC spec_type, UINT32 vendor_idx);
extern UINT32 page_and_block_size_id4th_set(ID4TH_SPEC spec_type, UINT32 vendor_idx);


#endif

