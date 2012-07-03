/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Technology Co., Ltd.         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Technology Co., Ltd. All rights are reserved by Generalplus Technology*
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Technology Co., Ltd. reserves the right to modify this    *
 *  software without notice.                                              *
 *                                                                        *
 *  Generalplus Technology Co., Ltd.                                      *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/

 /**
 * @file gp_cache.h
 */

#ifndef _GP_CACHE_H_
#define _GP_CACHE_H_


#include <mach/typedef.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/* Ioctl for device node definition */
#define CACHE_IOCTL_MAGIC	'C'
#define DCACHE_FLUSH	_IOWR(CACHE_IOCTL_MAGIC, 1, unsigned int)
#define TLB_FLUSH	_IOW(CACHE_IOCTL_MAGIC,  2, unsigned int)
#define DCACHE_CLEAN_RANGE	_IOW(CACHE_IOCTL_MAGIC,  3, struct gp_cache_address_s)
#define DCACHE_INVALIDATE_RANGE	_IOW(CACHE_IOCTL_MAGIC,  4, struct gp_cache_address_s)


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct gp_cache_address_s {
	unsigned int start;
	unsigned int size;
}gp_cache_address_t;

/**
 * @brief   Cache clean function
 * @param   start [in]: start address
 * @param   size [in]: size of range
 * @return  None
 */
void gp_clean_dcache_range(unsigned int start,unsigned int size);

/**
 * @brief   Cache invalidate function
 * @param   start [in]: start address
 * @param   size [in]: size of range
 * @return  None
 */
void gp_invalidate_dcache_range(unsigned int start,unsigned int size);


#endif	/*_GP_CACHE_H_*/
