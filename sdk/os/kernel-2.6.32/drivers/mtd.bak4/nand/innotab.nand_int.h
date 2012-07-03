#ifndef __INNOTAB_NAND_INT		// Innotab NAND Interface
#define __INNOTAB_NAND_INT

#define INNOTAB_BUSW			0	// "Fake" bus width.  Expected to be useless place holder to minimize the change to existing
									// MTD code
#define MAX_LEN_CHIP_NAME		50	// Maximum length for chip name
#define MAX_LEN_PART_NAME		50	// Maximum length for partition name
struct innotab_nand_int
{
	int (*check_wp)(struct mtd_info *mtd);
	int (*erase_wait)(struct nand_chip *chip, int page);	// Difference from standard MTD callback: 
														// This functions waits until erase operation 
														// is over and return result.
	int (*mark_bad)( struct nand_chip *chip, int page );
	int (*check_bad)( struct nand_chip *chip, int page );
	int (*allow_erase_bb)(struct mtd_info *mtd);			// Allow erasure of bad block?
	char			name[MAX_LEN_CHIP_NAME];
	unsigned long	chipsize;	/* Size of a chip */
	u_int32_t erasesize; /* Size of erase block. */
	u_int32_t oobblock;  /* page size in byte (e.g. 512) */
	u_int32_t oobsize;   /* Amount of OOB data per block visible to MTD. */
	void *priv;			// Private data
};

#endif
