#ifndef __ASM_ARCH_SPMP_NAND_H
#define __ASM_ARCH_SPMP_NAND_H

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

struct spmp_nand_timing {
	unsigned int	tCLE;   /* CLE pulse width */
	unsigned int	tALE;   /* ALE pulse width */
	unsigned int	tACT;   /* WRNN/RDNN active width */
	unsigned int	tREC;   /* WRNN/RDNN recover width */
	unsigned int	tWAIT;  
	unsigned int	tRDSTS; /* Read Status pulse */
};

struct spmp_nand_cmdset {
	uint16_t	read1;
	uint16_t	read2;
	uint16_t	program;
	uint16_t	read_status;
	uint16_t	read_id;
	uint16_t	erase;
	uint16_t	reset;
	uint16_t	lock;
	uint16_t	unlock;
	uint16_t	lock_status;
};

struct spmp_nand_flash {
	const struct spmp_nand_timing *timing; /* NAND Flash timing */
	const struct spmp_nand_cmdset *cmdset;

	uint32_t page_per_block;/* Pages per block (PG_PER_BLK) */
	uint32_t page_size;	/* Page size in bytes (PAGE_SZ) */
	uint32_t flash_width;	/* Width of Flash memory (DWIDTH_M) */
	uint32_t dfc_width;	/* Width of flash controller(DWIDTH_C) */
	uint32_t num_blocks;	/* Number of physical blocks in Flash */
	uint32_t chip_id;
};

struct spmp_nand_platform_data {

	/* the data flash bus is shared between the Static Memory
	 * Controller and the Data Flash Controller,  the arbiter
	 * controls the ownership of the bus
	 */
	int	enable_arbiter;

	const struct mtd_partition		*parts;
	unsigned int				nr_parts;

	const struct spmp_nand_flash * 	flash;
	size_t					num_flash;
};

extern void spmp_set_nand_info(struct spmp_nand_platform_data *info);
#endif /* __ASM_ARCH_PXA3XX_NAND_H */

