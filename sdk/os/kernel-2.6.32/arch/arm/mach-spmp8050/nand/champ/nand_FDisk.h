#ifndef __NAND_FDISK_H__
#define __NAND_FDISK_H__

#include "../spmp8k_nand.h"
#include "../hal/nf_s330.h"
/*
 0 Empty                  1b Hidden Win95 FAT32     9f BSD/OS                
 1 FAT12                  1c Hidden W95 FAT32 (LBA) a0 Thinkpad hibernation  
 4 FAT16 <32M             1e Hidden W95 FAT16 (LBA) a5 FreeBSD               
 5 Extended               3c Part.Magic recovery    a6 OpenBSD               
 6 FAT16                  41 PPC PReP Boot          a8 Darwin UFS            
 7 HPFS/NTFS              42 SFS                    a9 NetBSD                
 a OS/2 Boot Manager      63 GNU HURD or SysV       ab Darwin boot           
 b Win95 FAT32            80 Old Minix              b7 BSDI fs               
 c Win95 FAT32 (LBA)      81 Minix / old Linux      b8 BSDI swap             
 e Win95 FAT16 (LBA)      82 Linux swap             be Solaris boot          
 f Win95 Ext'd (LBA)      83 Linux                  eb BeOS fs               
11 Hidden FAT12           84 OS/2 hidden C: drive   ee EFI GPT               
12 Compaq diagnostics     85 Linux extended         ef EFI (FAT-12/16/32)    
14 Hidden FAT16 <32M      86 NTFS volume set        f0 Linux/PA-RISC boot    
16 Hidden FAT16           87 NTFS volume set        f2 DOS secondary         
17 Hidden HPFS/NTFS       8e Linux LVM              fd Linux raid autodetect 
*/
#define PARTITION_TYPE_EMPTY			0x00  /*¿Õ·ÖÇø*/
#define PARTITION_TYPE_FAT12			0x01  /*FAT12 ( < 32680 sectors )*/
#define PARTITION_TYPE_FAT16			0x04  /*FAT16 ( 32680 - 65535 sectors )*/
#define PARTITION_TYPE_EXTENDED			0x05  /*DOS EXTENDED*/
#define PARTITION_TYPE_BIGDOS_FAT16		0x06  /*FAT16 ( 33MB - 4GB )*/
#define PARTITION_TYPE_NTFS				0x07  /*NTFS*/
#define PARTITION_TYPE_FAT32			0x0B  /*FAT32*/
#define PARTITION_TYPE_FAT32_LBA		0x0C  /*FAT32 LBA*/
#define PARTITION_TYPE_FAT16_LBA		0x0E  /*FAT16 LBA*/
#define PARTITION_TYPE_EXTENDED_LBA		0x0F  /*LBA EXTENDED*/
#define PARTITION_TYPE_DYNAMIC_DISK		0x42  /*Dynamic Disk Volume*/
#define PARTITION_TYPE_MINIX_OLD		0x80
#define PARTITION_TYPE_MINIX			0x81
#define PARTITION_TYPE_LINUX_SWAP		0x82
#define PARTITION_TYPE_LINUX			0x83
#define PARTITION_TYPE_LINUX_EXT		0x85
#define PARTITION_TYPE_LINUX_LVM		0x8E

typedef struct 
{
	unsigned int  uiStartSector; /* this member is invalid for fdisk function*/
	unsigned int  uiLastSector;  /* this member is invalid for fdisk function*/
	unsigned int  uiPartitionType;
	unsigned int  uiSizeByMB;
}PartitionInfo_S;

#define FS_FDISK_RESERVE_MB 1
#define PARTITION_ITEM      4
#define PARTITION_OFFSET    0x1BE

#pragma pack(1)
struct tagPartitionTable
{
	unsigned char  ucBootId;
	unsigned char  ucStartHead;
	unsigned char  ucStartSector;
	unsigned char  ucStartCylinder;
	unsigned char  ucSysId;
	unsigned char  ucEndHead;
	unsigned char  ucEndSector;
	unsigned char  ucEndCylinder;
	int  uiStartLogSector;
	int  uiTotalSectors;
};

typedef struct tagPartitionTable PartitionTable_S;

struct tagMBR
{
	unsigned char    aucBootCmd[PARTITION_OFFSET];
	PartitionTable_S partition[PARTITION_ITEM];
	unsigned char    aucsignature[2];
};

typedef struct tagMBR MBR_S;
#pragma pack()

extern int Nand_FDisk(PartitionInfo_S astPartitionBaseInfo[4]);
extern void nf_DiskInfo_Set(psysinfo_t *psysInfo);
#endif

