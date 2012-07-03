/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2007 by Sunplus mMedia Inc.                      *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  mMedia Inc. All rights are reserved by Sunplus mMedia Inc.            *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus mMedia Inc. reserves the right to modify this software        *
 *  without notice.                                                       *
 *                                                                        *
 *  Sunplus mMedia Inc.                                                   *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *                                                                        *
 **************************************************************************/

/**
 * @file gp_ovg.h
 * @brief OpenVG header
 * @author ytliao@generalplus.com
 */

#ifndef _GP_OVG_H_
#define _GP_OVG_H_

#define GP_OVG_RGB565		0x0
#define GP_OVG_YUYV			0x1
#define GP_OVG_FLASHLITE	0x2
#define GP_OVG_RGBA8888		0x4
#define GP_OVG_SCROLL		0x8
#define GP_OVG_STATISTIC	0x10
#define GP_OVG_API			0x20
#define GP_OVG_CEVA_L2		0x40
#define GP_OVG_FLASH_IN_SYS	0x80

#include "mach/gp_chunkmem.h"
/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Initialize OpenVG state
 * @param width: width of rendering buffer
 * @param mem_size: size of temp buffer
 * @param ovg_config: ovg config parameter
 *
 * @return	OK(0)/Error(1)
 **/
int		
gpOvgInit(
	int width,
	int height,
	int mem_size,
	unsigned int ovg_config
);

/**
 * @brief Exit OpenVG state
 *
 **/
void 	
gpOvgExit(
	void
);

/**
 * @brief set OpenVG Rendering buffer address
 * @param addr: destination buffer address in physical address
 *
 **/
void	
gpOvgSetFrameBuffer(
	chunk_block_t addr
);

void 
gpOvgCopyFrame(
	chunk_block_t dst_addr,
	chunk_block_t src_addr,
	int stride,
	int x,
	int y,
	int width,
	int height
);

int 
gpOvgDmaClear(
	unsigned char* dst_addr,
	int dst_x,
	int dst_y,
	int width,
	int height,
	int dst_stride,
	int dst_format,
	unsigned int color
);

int
gpOvgDmaCopy(
	unsigned char* dst_addr,
	unsigned char* src_addr,
	int dst_x,
	int dst_y,
	int src_x,
	int src_y,
	int width,
	int height,
	int dst_stride,
	int src_stride,
	int dst_format,
	int src_format
);

void
gpOvgStartLog(
	void
);

int
gpOvgIfDirty(
	void
);

void
gpOvgChangeRenderFormat(
	unsigned int color_format
);

void
gpOvgSetImageAA(
	int enable
);

#ifdef __cplusplus
}
#endif
#endif