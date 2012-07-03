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
#ifndef _REG_AES_H_
#define _REG_AES_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define LOGI_ADDR_AES_REG     IO3_ADDRESS(0x300C0)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

typedef struct aesReg_s {
	volatile UINT32 aesDmaCtrl;        	/* 0x0000 ~ 0x0003 AES dma channel ctrl */
	volatile UINT32 aesDmaSrcAddr;      /* 0x0004 ~ 0x0007 aes dma source address */
	volatile UINT32 aesDmaDstAddr;      /* 0x0008 ~ 0x000B aes dma target address */
	volatile UINT32 aesDmaTxCount;        /* 0x000C ~ 0x000F aes dma terminal counter */
	volatile UINT32 reserved0[3];		/* 0x0010 ~ 0x001B */
	volatile UINT32 aesCtrl;      		/* 0x001C ~ 0x001F aes control */
	volatile UINT32 aesKey3;        	/* 0x0020 ~ 0x0023 aes key3 */
	volatile UINT32 aesKey2;      		/* 0x0024 ~ 0x0027 aes key2 */
	volatile UINT32 aesKey1;     		/* 0x0028 ~ 0x002B aes key1 */
	volatile UINT32 aesKey0;      		/* 0x002C ~ 0x002F aes key0 */
	volatile UINT32 reserved1[67];		/* 0x0030 ~ 0x013B */
	volatile UINT32 aesDmaInt;        	/* 0x013C ~ 0x013F Output Resolution */
} aesReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_AES_H_ */
