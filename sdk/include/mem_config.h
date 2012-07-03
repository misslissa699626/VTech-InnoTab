/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2008 by Sunplus mMedia Technology Co., Ltd.      *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus mMedia Technology Co., Ltd. reserves the right to modify this *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus mMedia Technology Co., Ltd.                                   *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 **************************************************************************/
/**
 * @file mem_config.h
 * @brief Definition of memory configuration
 * @date 2008/02/05
 */

#ifndef _MEM_CONF_H_
#define _MEM_CONF_H_

#include <typedef.h>
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#ifndef WIN32
#define CACHE_TO_UNCACHE(addr)   	(UINT8*)(((UINT32)(addr))|0x20000000)
#define UNCACHE_TO_CACHE(addr)   	(UINT8*)(((UINT32)(addr))&0xDFFFFFFF)
#define IS_CACHE_ADDRESS(ptr) 		(((unsigned long int)(ptr) & 0x20000000) == 0)
#define IS_UNCACHE_ADDRESS(addr) 	((((U32)(addr))&0x20000000) == 0x20000000)
#else
#define CACHE_TO_UNCACHE(addr)   	(addr)
#define UNCACHE_TO_CACHE(addr)   	(addr)
#define IS_CACHE_ADDRESS(ptr) 		(1)
#define IS_UNCACHE_ADDRESS(addr) 	(1)
#endif
/**************************************************************************/

enum {
	MEMID_TOTAL = 0,
	MEMID_FB,
	MEMID_JPG_WORK,
	MEMID_JPG_BTS,
	MEMID_JPG,
	MEMID_GPE,
	MEMID_THUMB,
	MEMID_SHARED_FM,
	MEMID_BUF_NAND,
	MEMID_BUF_SPI,
	MEMID_BUF_USB,
	MEMID_BUF_LIBAV_MP3,
	MEMID_BUF_LIBAV_MP3_RAW,
	MEMID_BUF_LIBAV_MP3_RESAMPLE,
	MEMID_BUF_LIBAV_MJPG,
	MEMID_BOOTLOADER,
	MEMID_BUF_SPI2,
	MEMID_BUF_JPGHELPER,
	MEMID_BUF_JPGTEMP,
	MEMID_DEDICATE_XD,
	NUM_MEMID
};

void *memLock(UINT32 id, UINT32 *pSize);
UINT32 memConfigGetTotalSize();
UINT32 memConfigGetFrameBufferSize();

#endif
