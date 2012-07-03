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
#include <mach/kernel.h>
#include <mach/hal/hal_aes.h>
#include <mach/hal/regmap/reg_aes.h>
#include <mach/hal/regmap/reg_scu.h>

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
static aesReg_t *aesReg = (aesReg_t *)LOGI_ADDR_AES_REG;

/**
* @brief	aes module reset
* @param 	enable[in]: 1 is enable; 0 is diable.
* @return 	none
*/
void 
gpHalAesModuleReset(
	UINT32 enable
)
{
	scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

	if(enable)
	{
		pScuaReg->scuaPeriRst |= (1<<29);	/* aes module reset */
		pScuaReg->scuaPeriRst &= ~(1<<29);
	}
}

/**
* @brief	aes key set 
* @param 	key0[in]: key0
* @param 	key0[in]: key1
* @param 	key0[in]: key2
* @param 	key0[in]: key3
* @return 	none
*/
SINT32
gpHalAesSetKey(
	UINT32 key0,
	UINT32 key1,
	UINT32 key2,
	UINT32 key3
)
{
	SINT32 nRet;

	aesReg->aesKey0 = key0;
	aesReg->aesKey1 = key1;
	aesReg->aesKey2 = key2;
	aesReg->aesKey3 = key3;

	/* load key */
	aesReg->aesCtrl = 0x01;
	nRet = HAL_BUSY_WAITING(aesReg->aesCtrl & 0x10, 1000);
	return nRet;
}

/**
* @brief	aes set decrypt
* @param 	
* @return 	none
*/
void
gpHalAesSetDecrypt(
	void
)
{
	aesReg->aesCtrl = (1<<12)| (1<<8);
}

/**
* @brief	aes set encrypt
* @param 	
* @return 	none
*/
void
gpHalAesSetEncrypt(
	void
)
{
	aesReg->aesCtrl = (1<<12);
}

/**
* @brief	aes set stop
* @param 	
* @return 	none
*/
void
gpHalAesSetStop(
	void
)
{
	aesReg->aesCtrl = 0;
}

/**
* @brief	aes dma init
* @param 	
* @return 	none
*/
void
gpHalAesSetDmaInit(
	void
)
{
	/* Software reset, this bit will auto clear after reset complete */
	aesReg->aesDmaCtrl = C_DMA_CTRL_RESET;
	aesReg->aesDmaTxCount = 0x0;
}

/**
* @brief	aes dma set
* @param 	dma_ctrl[in]: dma control
* @return 	none
*/
void
gpHalAesSetDmaCtrl(
	UINT32 dma_ctrl
)
{
	aesReg->aesDmaCtrl = dma_ctrl;
}

/**
* @brief	aes dma set addr
* @param 	src_addr[in]: dma source address
* @param 	dst_addr[in]: dma target address
* @param 	count[in]: dma length
* @return 	none
*/
void
gpHalAesSetDmaAddr(
	UINT32 src_addr,
	UINT32 dst_addr,
	UINT32 count
)
{
	aesReg->aesDmaSrcAddr = src_addr;
	aesReg->aesDmaDstAddr = dst_addr;
	aesReg->aesDmaTxCount = count;
}

/**
* @brief	aes dma get status 
* @param 	
* @return 	dma status, 1: busy, 0:idle
*/
SINT32
gpHalAeGetDmaStatus(
	void
)
{
	if(aesReg->aesDmaInt & (1<<11))
		return 1;
	
	/* dma idle and disable dma */
	aesReg->aesDmaCtrl = 0;
	return 0;
}
