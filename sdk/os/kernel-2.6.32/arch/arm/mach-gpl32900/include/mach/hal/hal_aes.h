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
#ifndef _HAL_AES_H_
#define _HAL_AES_H_

#include <mach/hal/hal_common.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
// DMA Control register
#define	C_DMA_CTRL_ENABLE			0x00000001
#define	C_DMA_CTRL_BUSY				0x00000002
#define	C_DMA_CTRL_SOFTWARE			0x00000000
#define	C_DMA_CTRL_EXTERNAL			0x00000004
#define	C_DMA_CTRL_NORMAL_INT		0x00000008
#define	C_DMA_CTRL_DEST_INCREASE	0x00000000
#define	C_DMA_CTRL_DEST_DECREASE	0x00000010
#define	C_DMA_CTRL_SRC_INCREASE		0x00000000
#define	C_DMA_CTRL_SRC_DECREASE		0x00000020
#define	C_DMA_CTRL_DEST_FIX			0x00000040
#define	C_DMA_CTRL_SRC_FIX			0x00000080
#define	C_DMA_CTRL_INT				0x00000100
#define	C_DMA_CTRL_RESET			0x00000200
#define	C_DMA_CTRL_M2M				0x00000000
#define	C_DMA_CTRL_M2IO				0x00000400
#define	C_DMA_CTRL_IO2M				0x00000800
#define	C_DMA_CTRL_8BIT				0x00000000
#define	C_DMA_CTRL_16BIT			0x00001000
#define	C_DMA_CTRL_32BIT			0x00002000
#define	C_DMA_CTRL_SINGLE_TRANS		0x00000000
#define	C_DMA_CTRL_DEMAND_TRANS		0x00004000
#define	C_DMA_CTRL_DBF				0x00008000
#define	C_DMA_CTRL_SINGLE_ACCESS	0x00000000
#define	C_DMA_CTRL_BURST4_ACCESS	0x00010000

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
/**
* @brief	aes module reset
* @param 	enable[in]: 1 is enable; 0 is diable.
* @return 	none
*/
void gpHalAesModuleReset(UINT32 enable);

/**
* @brief	aes clcok enable
* @param 	enable[in]: 1 is enable; 0 is diable.
* @return 	none
*/
void gpHalAesClkEnable(UINT32 enable);

/**
* @brief	aes key set 
* @param 	key0[in]: key0
* @param 	key0[in]: key1
* @param 	key0[in]: key2
* @param 	key0[in]: key3
* @return 	none
*/
SINT32 gpHalAesSetKey(UINT32 key0, UINT32 key1, UINT32 key2, UINT32 key3);

/**
* @brief	aes set decrypt
* @param 	
* @return 	none
*/
void gpHalAesSetDecrypt(void);

/**
* @brief	aes set encrypt
* @param 	
* @return 	none
*/
void gpHalAesSetEncrypt(void);

/**
* @brief	aes set stop
* @param 	
* @return 	none
*/
void gpHalAesSetStop(void);

/**
* @brief	aes dma init
* @param 	
* @return 	none
*/
void gpHalAesSetDmaInit(void);

/**
* @brief	aes dma set
* @param 	dma_ctrl[in]: dma control
* @return 	none
*/
void gpHalAesSetDmaCtrl(UINT32 dma_ctrl);

/**
* @brief	aes dma set addr
* @param 	src_addr[in]: dma source address
* @param 	dst_addr[in]: dma target address
* @param 	count[in]: dma length
* @return 	none
*/
void gpHalAesSetDmaAddr(UINT32 src_addr,UINT32 dst_addr,UINT32 count);

/**
* @brief	aes dma get status 
* @param 	
* @return 	dma status, 1: busy, 0:idle
*/
SINT32 gpHalAeGetDmaStatus(void);
#endif /* _HAL_AES_H_ */

