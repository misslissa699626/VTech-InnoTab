#ifndef _MTD_IOCTL_DEV_H
#define _MTD_IOCTL_DEV_H

/*
** MTD control file device command code.
*/
#define VMTD_IOC_ADD_MTD		0x1000
#define VMTD_IOC_DEL_MTD		0x1001
#define VMTD_IOC_ADD_DEV		0x1002
#define VMTD_IOC_DEL_DEV		0x1003
#define VMTD_IOC_MARK_BAD		0x1004

int vmtd_file_init(void);
void vmtd_file_exit(void);

struct cmd_add_mtd
{
	int phyChipNo;
	int param;									// For VMTD_IOC_ADD_MTD, allow_erase_bb: 1 - allow erase bad block, 0 - not allow
												// For VMTD_IOC_MARK_BAD, block number to mark bad
	struct innotab_nand_chip_config config;
};
#endif