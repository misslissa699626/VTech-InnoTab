#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "../NFTL/nftl.h"

#include "../NFTL/nand_cfg.h"
#include "NF_cfgs.h"
#include "nand_FDisk.h"
/********************************************************
function definition
********************************************************/
/*!\brief fdisk 
 * \param pstPartitionBaseInfo [in] 
 * \param pstDevice [in] device parameter
 * \return 1 for success, error code for failure(<0) */
/* note: for fdisk, the parameter can set to below:*/

extern void print_buf(unsigned char* buf, int len);

smcDev_t nf_Disk0;
#if (SUPPORT_NF_DISK1==1)
smcDev_t nf_Disk1;
#endif

void nf_DiskInfo_Set(psysinfo_t *psysInfo)
{
	unsigned int ret, temp;
	unsigned int u16TotalBlkNo;
	unsigned int u16PageNoPerBlk;
	unsigned int u16PyldLen;
	unsigned int u16ReduntLen;
	smcDev_t *pdisk = &nf_Disk0;

	u16TotalBlkNo = psysInfo->u16TotalBlkNo << psysInfo->u8Internal_Chip_Number;

	u16PageNoPerBlk = psysInfo->u16PageNoPerBlk;

	u16PyldLen = psysInfo->u16PyldLen << (psysInfo->u8Support_TwoPlan + psysInfo->u8Support_Internal_Interleave);
	u16ReduntLen = (psysInfo->u16ReduntLen << 1) << (psysInfo->u8Support_TwoPlan + psysInfo->u8Support_Internal_Interleave);

	SPMP_DEBUG_PRINT("Total blk: %d, PagesPerBlk: %d, PyldLen: %d\n", u16TotalBlkNo, u16PageNoPerBlk, u16PyldLen);

	temp = ((u16TotalBlkNo >> 10) * u16PageNoPerBlk * u16PyldLen);
	pdisk->common.megaByte = (temp >> 10);

	temp = u16TotalBlkNo / 1024; // get the zone number

	pdisk->blkOffset = get_DiskstartBlk();

	
	pdisk->nrPhyBlks = u16TotalBlkNo - pdisk->blkOffset;

	SPMP_DEBUG_PRINT("u16TotalBlkNo: %x  u8Internal_Chip_Number:%x  u16TotalBlkNo: %x  Used blk: %d, block offset: %d\n", 
			psysInfo->u16TotalBlkNo, psysInfo->u8Internal_Chip_Number, u16TotalBlkNo, pdisk->nrBlks, pdisk->blkOffset);

	// we reserve 24 blks in every zone
	pdisk->nrBlks = (pdisk->nrPhyBlks * BANK_BLK) / 1024;

	pdisk->blkSize = u16PageNoPerBlk * u16PyldLen;
	pdisk->sectorSize = 512; // for fat porting
	pdisk->sectorBit = 9;
	pdisk->addrCycle = 5;
	pdisk->eccCnt = 6;

	temp = (u16PyldLen >> pdisk->sectorBit);
	pdisk->redundBytes = u16ReduntLen / temp;
	pdisk->flashMode = 2; /* Case: NAND on SMC interface */
	ret = 2;

	pdisk->firstBlk= 2;
	
	temp = (pdisk->blkSize >> pdisk->sectorBit);
	pdisk->nrBitsSectorPerBlk = 0;

	while (1)
	{
		temp = temp >> 1;
		if (temp == 0)
		{
			break;
		}
		pdisk->nrBitsSectorPerBlk++;
	}
}

int Nand_FDisk(PartitionInfo_S astPartitionBaseInfo[4])
{
	int iRet;
	int uiTemp;
	unsigned int uiMaxSector = 0;
	unsigned int uiTotalSize;
	unsigned int uiUsedSize = 0;
	unsigned int uiStartSector;
	unsigned int uiCount;
	MBR_S* pStMbr;
	nf_disk_func *disk_op;
	unsigned char TempBuffer[512+4];
	smcDev_t *pnand  = (smcDev_t *)&nf_Disk0;

	disk_op = get_nf_disk_op(0);
	
	pStMbr = (MBR_S*)TempBuffer;
	//uiMaxSector = (pnand->nrBlks+ NAND_PGBASE_USED_LOGBLK) <<(pnand->nrBitsSectorPerBlk);
	uiMaxSector = pnand->nrBlks <<(pnand->nrBitsSectorPerBlk);
	
	uiStartSector = 0;
	uiTotalSize = uiMaxSector / 2048;
	SPMP_DEBUG_PRINT("uiMaxSector=%08x, uiTotalSize=%08x, nrBlks=%08x, nrBitsSectorPerBlk=%08x\n", 
		uiMaxSector, uiTotalSize, pnand->nrPhyBlks + NAND_PGBASE_USED_LOGBLK, pnand->nrBitsSectorPerBlk);

	if (astPartitionBaseInfo[0].uiSizeByMB == -1
		&& astPartitionBaseInfo[1].uiSizeByMB == -1
		&& astPartitionBaseInfo[2].uiSizeByMB == -1
		&& astPartitionBaseInfo[3].uiSizeByMB == -1)
	{
		iRet = disk_op->read(0, (uiMaxSector - 1), (unsigned char*)(pStMbr));
		if (iRet == FAIL)
		{
			iRet = FAILURE;
			goto lable_end;
		}
		SPMP_DEBUG_PRINT("uiMaxSector=%d uiTotalSize=%d \n", uiMaxSector, uiTotalSize);
		//print_buf((unsigned char*)(pStMbr), 512);
		
		iRet = disk_op->write(0, 1, (unsigned char*)(pStMbr));
		if (iRet == FAIL)
		{
			iRet = FAILURE;
			goto lable_end;
		}
		return SUCCESS;
	}

	uiUsedSize = FS_FDISK_RESERVE_MB;
	for(uiCount = 0; uiCount < 4; uiCount++)
	{
		if (astPartitionBaseInfo[uiCount].uiSizeByMB != -1)
		{
			if(astPartitionBaseInfo[uiCount].uiSizeByMB == 0xffffffff)
			{
				uiUsedSize = uiTotalSize;
				break;
			}
			uiUsedSize += astPartitionBaseInfo[uiCount].uiSizeByMB;
			
		}
	}
	
	if (uiTotalSize < uiUsedSize)
	{
		SPMP_DEBUG_PRINT("uiUsedSize=%d uiTotalSize=%d \n", uiUsedSize, uiTotalSize);
		iRet = FAILURE;
		goto lable_end;
	}

	iRet = disk_op->read(0, 1, (unsigned char*)(pStMbr));
	if (iRet == FAIL)
	{
		iRet = FAILURE;
		goto lable_end;
	}
	//SPMP_DEBUG_PRINT("uiUsedSize=%08x \n", uiUsedSize);print_buf((unsigned char*)(pStMbr), 512);
	iRet = disk_op->write((uiMaxSector - 1), 1, (unsigned char*)(pStMbr));//backup
	if (iRet == FAIL)
	{
		iRet = FAILURE;
		goto lable_end;
	}
	
	memset(pStMbr, 0, sizeof(MBR_S));
	uiStartSector = FS_FDISK_RESERVE_MB * 2048; /*No.0 sector is MBR recode*/
	uiUsedSize = FS_FDISK_RESERVE_MB;
	for(uiCount = 0; uiCount < 4 && uiUsedSize < uiTotalSize && astPartitionBaseInfo[uiCount].uiSizeByMB != 0; uiCount++)
	{	
		pStMbr->partition[uiCount].ucSysId = (unsigned char)astPartitionBaseInfo[uiCount].uiPartitionType;
		pStMbr->partition[uiCount].uiStartLogSector = uiStartSector;
		uiTemp = (astPartitionBaseInfo[uiCount].uiSizeByMB > (uiTotalSize - uiUsedSize)) ? (uiTotalSize - uiUsedSize) : astPartitionBaseInfo[uiCount].uiSizeByMB;
		if (astPartitionBaseInfo[uiCount].uiSizeByMB == -1)
		{
			pStMbr->partition[uiCount].uiTotalSectors = uiMaxSector - uiStartSector;
			break;
		}
		pStMbr->partition[uiCount].uiTotalSectors = uiTemp * 2048;
		uiUsedSize += astPartitionBaseInfo[uiCount].uiSizeByMB;
		uiStartSector += pStMbr->partition[uiCount].uiTotalSectors;
		
		iRet = disk_op->write(pStMbr->partition[uiCount].uiStartLogSector, 1, (unsigned char*)(pStMbr));
		if (iRet == FAIL)
		{
			iRet = FAILURE;
			goto lable_end;
		}
		SPMP_DEBUG_PRINT("ucSysId=%d uiStartLogSector=%d uiTotalSectors=%d\n", 
			pStMbr->partition[uiCount].ucSysId, pStMbr->partition[uiCount].uiStartLogSector, pStMbr->partition[uiCount].uiTotalSectors);
		//print_buf((unsigned char*)(pStMbr), 512);
	}   

	pStMbr->aucsignature[0] = 0x55;
	pStMbr->aucsignature[1] = 0xAA;
	
	SPMP_DEBUG_PRINT("ucSysId=%08x uiStartLogSector=%08x uiTotalSectors=%08x\n", 
			pStMbr->partition[0].ucSysId, pStMbr->partition[0].uiStartLogSector, pStMbr->partition[0].uiTotalSectors);
	SPMP_DEBUG_PRINT("ucSysId=%08x uiStartLogSector=%08x uiTotalSectors=%08x\n", 
			pStMbr->partition[1].ucSysId, pStMbr->partition[1].uiStartLogSector, pStMbr->partition[1].uiTotalSectors);
	SPMP_DEBUG_PRINT("ucSysId=%08x uiStartLogSector=%08x uiTotalSectors=%08x\n", 
			pStMbr->partition[2].ucSysId, pStMbr->partition[2].uiStartLogSector, pStMbr->partition[2].uiTotalSectors);
	SPMP_DEBUG_PRINT("ucSysId=%08x uiStartLogSector=%08x uiTotalSectors=%08x\n", 
			pStMbr->partition[3].ucSysId, pStMbr->partition[3].uiStartLogSector, pStMbr->partition[3].uiTotalSectors);
	print_buf((unsigned char*)(pStMbr), 512);
	
	iRet = disk_op->write(0, 1, (unsigned char*)(pStMbr));
	if (iRet == FAIL)
	{
		iRet = FAILURE;
		goto lable_end;
	}

	iRet = SUCCESS;
lable_end:
	iRet = disk_op->read(0, 1, (unsigned char*)(pStMbr));
	if (iRet == SUCCESS)
	{
		SPMP_DEBUG_PRINT("uiUsedSize=%08x \n", uiUsedSize);//print_buf((unsigned char*)(pStMbr), 512);
	}
	
	return (iRet);
}

