/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file gp_nand.h
 * @brief Nand flash interface header file
 * @author 
 */

#ifndef _GP_NAND_FLASH_DEVICE_H_
#define _GP_NAND_FLASH_DEVICE_H_


#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define NF_IOCTL_START    0x80000
#define NF_IOCTL_FLUASH   (0 + NF_IOCTL_START)
#define NF_IOCTL_TEST   (1 + NF_IOCTL_START)
#define NF_IOCTL_DBG_MSG   (2 + NF_IOCTL_START)
#define NF_IOCTL_EREASEALL   (3 + NF_IOCTL_START)

#define IOCTL_NAND_GET_BLK_INFO (4 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_OPEN		 (5 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_CLOSE	 (6 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_WRITE  (7 + NF_IOCTL_START)
#define IOCTL_NAND_PHY_READ  (8 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_READ_1K	(9 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_WRITE_1K	(10 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_READ_PAGE	(11 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_WRITE_PAGE	(12 + NF_IOCTL_START)
#define IOCTL_NAND_ISP_OPEN		 (13 + NF_IOCTL_START)
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct gp_nand_blk_info_s {
	int block_size; 	/* in bytes */
	int page_size; 		/* in bytes */
} gp_nand_blk_info_t;

typedef struct gp_nand_phy_partiton_s {
	int offset; 	/* in bytes */
	int size;		/* in bytes */
} gp_nand_phy_partition_t;

typedef struct gp_nand_write_buffer_s {
	void *buf;
	int size; 	/* in bytes */
} gp_nand_write_buffer_t;

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif //endif _GP_NAND_FLASH_DEVICE_H_
