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
 * @file    gp_partition.c
 * @brief   Link kernel fs partition function for sd card GP special partition usage.
 * @author  Dunker Chen
 */
 
#include <linux/genhd.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
*                         G L O B A L    D A T A                         *
**************************************************************************/

/**************************************************************************
*             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/

/**
* @brief 	Add partition function.
* @param 	disk[in]: General disk structure.
* @param 	partno[in]: Partition number.
* @param 	start[in]: Start sector.
* @param 	len[in]: Partition size.
* @param	flags[in]: Control flag.
* @return 	ERROR_ID(<0).
*/ 
struct hd_struct *gp_add_partition(struct gendisk *disk, int partno, sector_t start, sector_t len, int flags)
{
	return add_partition(disk, partno, start, len, flags);
}
EXPORT_SYMBOL(gp_add_partition);

/**
* @brief 	Delete partition function.
* @param 	disk[in]: General disk structure.
* @param 	partno[in]: Partition number.
* @return 	None.
*/ 
void gp_delete_partition(struct gendisk *disk, int partno)
{
	return 	delete_partition(disk, partno);
}
EXPORT_SYMBOL(gp_delete_partition);