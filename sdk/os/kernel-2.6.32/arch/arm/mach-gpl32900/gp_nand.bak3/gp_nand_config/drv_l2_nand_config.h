#ifndef _NAND_CONFIG_H_
#define _NAND_CONFIG_H_ 

#include <mach/typedef.h>

/*######################################################
#												 																#
#	   Nand Flash Blocks Assignation Configuration				#
#																												#
 ######################################################*/
#define APP_AREA_START				0	    /* Start Physical /Block Address */
#define APP_AREA_SIZE				0 	/* APP Area Size /Block numbers. */
#define APP_AREA_SPARE_START		(APP_AREA_START + APP_AREA_SIZE)
#define APP_AREA_SPARE_PERCENT  	2
#define	APP_AREA_SPARE_SIZE			(APP_AREA_SIZE/APP_AREA_SPARE_PERCENT)	/* Spare area size */
#define APP_DATA_GAP_SIZE			0
#define DATA_AREA_START				(APP_AREA_SPARE_START + APP_AREA_SPARE_SIZE +APP_DATA_GAP_SIZE)


#define MAX_PARTITION_NUM		10

typedef struct
{
	UINT32 offset;
	UINT32 size;
	UINT8  attr;	
}PART_INFO;


typedef struct 
{
	UINT16	uiAppStart;
	UINT16 	uiAppSize;
	UINT16 	uiAppSpareStart;
	UINT16  uiAppSparePercent;
	UINT16 	uiAppSpareSize;
	UINT16 	uiDataStart;	
	UINT16	uiBankSize;
	UINT16	uiBankRecSize;
	UINT32	uiNoFSAreaSize;
	
	UINT8		uiPartitionNum;	
	PART_INFO Partition[MAX_PARTITION_NUM];	
} NF_CONFIG_INFO;

extern NF_CONFIG_INFO 		gSTNandConfigInfo;

extern void Nand_OS_LOCK(void);
extern void Nand_OS_UNLOCK(void);

extern void DMAmmCopy(UINT32 wSourceAddr, UINT32 wTargeAddr, UINT16 Count);

extern SINT32 NandParseBootHeader(UINT8 *Buffer);
extern UINT16 GetAppStartBlkFromBth(void);
extern UINT16 GetAppSizeOfBlkFromBth(void);
extern UINT16 GetAppPercentFromBth(void);
extern UINT16 GetDataBankLogicSizeFromBth(void);
extern UINT16 GetDataBankRecycleSizeFromBth(void);
extern UINT32 GetNoFSAreaSectorSizeFromBth(void);

extern UINT8 GetPartNumFromBth(void);
extern void GetPartInfoFromBth(UINT8 part,UINT16 *MB_size,UINT8 *attr);

extern SINT32 GetNandConfigInfo(void);
extern UINT16 get_bch_mode(void);
extern void SetBitErrCntAsBadBlk(UINT16 bit_cnt);
extern UINT16 GetBitErrCntAsBadBlk(void);
extern void nand_sema_init(void);
extern void nand_sema_lock(void);
extern void nand_sema_unlock(void);
extern void Nand_Chip_Switch(UINT32 Nand_Switch_Num);
extern void Nand_Chip_Set(UINT32 Chip);
#endif
