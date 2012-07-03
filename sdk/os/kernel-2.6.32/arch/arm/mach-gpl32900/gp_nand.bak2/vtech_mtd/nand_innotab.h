#ifndef __NAND_INNOTAB__
#define __NAND_INNOTAB__

#define INNOTAB_OOB_SIZE		8		// Temporary and will be removed later.  Please ignore this marco.
#define GEPLUS_OOB_SIZE			INNOTAB_OOB_SIZE		// Temporary and will be removed later.  Please ignore this marco.
#define INNOTAB_OOB_AVAIL		6		// The size of the OOB available for modules at MTD level or higher

/* 
 * Read data from NAND at the given offset, relative the start of the device.
 * All data are supposed to be ecc corrected.
 *
 * @dev_no:	device number -		0 for internal flash, 1 for cartridge
 * @start:	start address to read from, relative to start of the device (in byte).  
 *			Always aligned to the start of a page.
 * @len:	number of bytes to read, excluding the OOB data  Always read a single page.
 * @retlen:	pointer to variable to store the number of read bytes, excluding OOB data
 * @buf:	the data buffer to put data
 *			Must be 16 byte-aligned DMA buffer, e.g. allocated by kmalloc(size_of_data_buf, GFP_DMA);
 * @oobbuf:	the data buffer for oob data, if non-NULL.  Otherwise throw away the OOB data.
 *			For non-NULL oobbuf, the first INNOTAB_OOB_AVAIL bytes will hold the OOB data.  
 *			The library should NOT write anything beyond that.
 *
 * Remark:  The restriction to a single page is based on the assumption that due to 
 *			implementation of the low-level driver, optimization for multi-page read is not 
 *			feasible.  The driver may need to be amended if such optimization is needed in
 *			future.
 *
 * return:	0			success
 *			-EINVAL		end address (i.e. len + start - 1) beyond the end of device.
 *			-EBADMSG	incorrectable ecc error
*/
int innotab_nand_read_ecc( int dev_no, loff_t start, size_t len, size_t * retlen, 
						   u_char *buf, u_char *oobbuf );

/* 
 * Write data to NAND at the given page, relative to the start of the device.
 *
 * ECC is to be generated internally.  Other data in OOB area is from the oob_buf.
 *
 * @dev_no:		device number -		0 for internal flash, 1 for cartridge
 * @page:		page number of the location to write.  Start of the device has page number = 0.
 * @buf:		the data to write
 *				Must be 16 byte-aligned DMA buffer, e.g. allocated by kmalloc(size_of_data_buf, GFP_DMA);
 * @oob_buf:	out of band data buffer to write
 *
 * return:	0			success
 *			-EIO		write failure
*/
int innotab_nand_write_page( int dev_no, int page, u_char *buf, u_char *oob_buf );

/*
 * Erase a single page
 *
 * @dev_no:	device number -		0 for internal flash, 1 for cartridge
 * @page:	page number of the 1st page of the block to be erased
 *
 * return:	0					success
 *			NAND_STATUS_FAIL	failed
*/
int innotab_nand_erase_block( int dev_no, int page );


int innotab_nand_erase_all(int dev_no);

/*
 * Write the bad block marker to OOB area of a page.
 *
 * @dev_no:	device number -		0 for internal flash, 1 for cartridge
 * @page:	page number of the location to be marked bad.
 *
 * return:	0			success
 *			-EIO		write failure
 */
int innotab_nand_mark_bd_oob( int dev_no, int page );

/*
 * Check the OOB area to see if it is a bad block
 *
 * @dev_no:	device number -		0 for internal flash, 1 for cartridge
 * @page:	Check bad block marker at this page.
 *
 * return:	0			good block
 *			1			bad block
 *
 * remark:	This functions should check all the followings:
 *			1.  Factory-marked bad block marker (chip-specific)
 *			2.  Bad block marker immediately after erase.
 *			3.	Pattern after calling innotab_nand_mark_bd_oob() to a block that may be bad
 */
int	innotab_nand_check_bd_oob( int dev_no, int page );

#define NAND_ROM			0
#define NAND_FLASH			1
struct innotab_nand_chip_param
{
	int				nand_maf_id;/* Manufacturer ID */
	int				nand_dev_id;/* Device ID */
	int				numchips;	/* Number of chip of the current device */
	unsigned long	chipsize;	/* Size of a chip */
	int				memoryType;	/* NAND_ROM - NAND ROM, guaranteed not to have bad block.
											  innotab_nand_check_bd_oob() should alway
											  return 0 (good block) for this case.
								   NAND_FLASH - NAND flash, may have bad block.  MTD layer
								              should handle the bad block management. */
	int				firstMTDBlk;/* Frist block accessible by MTD.  Block 0 to block 
								   firstMTDBlk - 1 is reserved area for low-level driver.
								   For NAND cartridge, this field should always be 0x00. */

	u_int32_t erasesize; /* Size of erase block. */
	u_int32_t oobblock;  /* page size in byte (e.g. 512) */
	u_int32_t oobsize;   /* Amount of OOB data per block (e.g. 16) */
	u_int32_t oobavail;	 /* Amount of OOB available to application (i.e. excluding the ECC)
							Remark:  This field reserved for future expansion.  Currently 
							fill in 0. */
	u_int32_t oobavailstart; /* Starting of the area within OOB for user data.  
							 Remark:  This field reserved for future expansion.  Currently 
							 fill with 0. */
						
	
	/* Side note: OOB data format assumed fixed. */
};

/*
 * Initialization routine.  Carries out chip detection and returns NAND chip parameters.
 * 
 * @dev_no:	device number, currently always 0.
 * @param:	structure containing device parameters
 *
 * return:	0		chip detected successfully, parameters in @param.
 *			-1		chip cannot be detected or unrecognized type of chip
 */
int innotab_nand_init( int dev_no, struct innotab_nand_chip_param *param );


/*
 * Check whether the device is write protected.  Assumed to be unchanged after initialization.
 *
 * return:	0		writable
 *			1		write-protected
 */ 
int innotab_nand_check_wp( int dev_no );


/*
 * After the NandCard unplug, we should call this function to release internal resource
 */
void innotab_nand_close( int dev_no );


#endif /* __NAND_INNOTAB__ */
