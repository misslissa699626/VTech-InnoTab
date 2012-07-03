#ifndef _NF_TYPE_H_
#define _NF_TYPE_H_

#include "NF_cfgs.h"
//////////////////////////////////////////
#define MAX_USBBLK 5//20
#define MAX_USEPAGE 1//10
#define TABLE_START 0

#define LAYOUT_HEARDS 0x4C415953 //"LAYS"
#define LAYOUT_VERSION 0
#define LAYOUT_PATTERN 0
#define LAYOUT_HEARDE 0x4C415945 //"LAYE"
#define PF_HEARDS 0x50464853 //"PFHS"
#define PF_VERSION 0
#define PF_PATTERN 0
#define PF_HEARDE 0x50464845 //"PFHE"
enum Layout_table {
	TABLE_FLG =0,
	CFG_FLG,
	REDBOOT_FLG,
	TOTAL_FLG
};
typedef struct RomBlock_Info
{
	unsigned short start;
	unsigned short count;
}RomBlock_Info_t;

typedef struct LayoutInfo
{
	unsigned int heards;
	unsigned int version;
	unsigned int pattern;
	unsigned int hearde;
	RomBlock_Info_t Section[TOTAL_FLG];
}LayoutInfo_t;
typedef struct PF_Info
{
	unsigned int heards;
	unsigned int version;
	unsigned int pattern;
	unsigned int hearde;
}PFInfo_t;

typedef struct PFInfo
{
	unsigned int PageSize;
	unsigned int PageNoPerBlk;
	unsigned int TotalBlkNo;
	unsigned int ecc_mode;
}PF_INFO_t;

typedef struct Manage_Info
{
	LayoutInfo_t lay;
	USB_Store_Info_t usb;
	PFInfo_t pf;
	psysinfo_t nand_info;

	PFInfo_t pf1;
	psysinfo_t nand_info1;	
}Manage_Info_t;
#endif
