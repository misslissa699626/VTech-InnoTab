#ifndef __GP_NAND_NAND__
#define __GP_NAND_NAND__

#define NAND_FLUASH_ALL_BLK	1

#define NAND_DIRECT_WRITE 	3
#define NAND_DIRECT_READ   	4

typedef struct
{
	void *buffer;				// 数据buffer
	unsigned int start_sector;	// 起始sector
	unsigned int sector_cnt;	// sector个数	
}Xsfer_arg1;

#endif