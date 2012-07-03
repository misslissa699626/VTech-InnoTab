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
 
#include <linux/module.h>
#include <linux/io.h>
#include <mach/regs-ms.h>
#include <mach/hal/regmap/reg_ms.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_ms.h>
#include <mach/ms/msproal_common.h>
#include <mach/hal/hal_clock.h>

/**
* @brief  MS init
* @param[in] none
* @param[out] none
* @return none
*/
void gpHalMsInit(
	void
)
{
	scubReg_t *pscubReg = (scubReg_t *)(LOGI_ADDR_SCU_B_REG);
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	
	gpHalScuClkEnable(SCU_C_PERI_MS, SCU_C, 1);
#if 0
	pscubReg->scubPadGrpSel1 &= ~0x3C000000;
	pscubReg->scubPadGrpSel1 |= 0x14000000;
	
	pscubReg->scubPadGrpSel2 &= ~0x30;
	pscubReg->scubPadGrpSel2 |= 0x10;
#endif	
	pscubReg->scubGpio0PinEn |= 0x1e000000; /* data 0 - data 4 */


	pmsReg->msControl = 0;
	//pmsReg->msControl |= 1 << 21; /* 3 RDY signals */

	pmsReg->msStatus = 0xFFFFFFFF;	
}
EXPORT_SYMBOL(gpHalMsInit);

/**
* @brief MS change interface mode
* @param mode[in]: MS_SERIAL or MS_4BIT_PARALLEL
* @param[out] none
* @return none
*/
void gpHalMsChangeIfMode(
	UINT8 mode
)
{
	UINT32 	hppreg;
	
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
    
    hppreg = pmsReg->msControl;
    
    if (MSPROAL_SERIAL_MODE == mode) {
        hppreg &= ~PARALLEL_4BIT_MODE;
    }
    else {
        hppreg |= PARALLEL_4BIT_MODE;
    }
    
    pmsReg->msControl = hppreg;
}
EXPORT_SYMBOL(gpHalMsChangeIfMode);

/**
* @brief Set MS card clock
* @param div[in]: clock divider
* @param [out] none
* @return none
*/
void gpHalMsChangeClock(
	UINT8 div
)
{
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	
	pmsReg->msControl &= ~0xFF;
	pmsReg->msControl |= div;
}
EXPORT_SYMBOL(gpHalMsChangeClock);

/**
* @brief Send MS TPC cmd
* @param MsCmd[in]: TPC command
* @param Data[in]: Argument
* @param Size[in]: Argument length
* @return none
*/
void gpHalMsSendCmd(
	UINT32 MsCmd,
	UINT32 Data,
	UINT8 Size
)
{
	UINT32    ctrl;
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	
	pmsReg->msStatus = 0xFFFF;
	
	if ((MsCmd & 0xF) != TPC_EX_SET_CMD) {
		ctrl = pmsReg->msControl;
        ctrl &= ~0xFF00;
        ctrl |= (Size << 8);   
        pmsReg->msControl = ctrl;
    }
    
	pmsReg->msWriteReg = Data;
    pmsReg->msComand = MsCmd;
}
EXPORT_SYMBOL(gpHalMsSendCmd);

/**
* @brief Get MS status
* @param[in] none
* @param[out] none
* @return Status
*/
UINT32 gpHalMsGetStatus(
	void
)
{
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	return pmsReg->msStatus;
}
EXPORT_SYMBOL(gpHalMsGetStatus);

void gpHalMsClearStatus(
	void
)
{
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	pmsReg->msStatus = 0xFFFF;
}
EXPORT_SYMBOL(gpHalMsClearStatus);

/**
* @brief Read MS register
* @param[in] Size 
* @param[out] Data
* @return Status
*/
UINT32 gpHalMsReadReg(
	void
)
{
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	return pmsReg->msReadReg;
}
EXPORT_SYMBOL(gpHalMsReadReg);

/**
* @brief MS Data read
* @param Size[in]: Read data length in word
* @param Data[out]: Read data
* @return none
*/
void gpHalMsReadData(
	UINT32 *Data, 
	UINT32 Size
)
{
	UINT32 i;
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	
	for (i=0;i<Size;i++) {
		*Data++ = pmsReg->msDataRx;
	}	
}
EXPORT_SYMBOL(gpHalMsReadData);

/**
* @brief MS write data
* @param Size[in]: write data length in word
* @param Data[in]: write data
* @return none
*/
void gpHalMsWriteData(
	UINT32 *Data, 
	UINT32 Size
)
{
	UINT32 i;
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	
	for (i=0;i<Size;i++) {
		pmsReg->msDataTx = *Data++;
	}	
}
EXPORT_SYMBOL(gpHalMsWriteData);

/**
* @brief Write MS register
* @param[in] Data 
* @param[out] none
* @return nooe
*/
void gpHalMsWriteReg(
	UINT32 Data
)
{
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	pmsReg->msWriteReg =  Data;
}
EXPORT_SYMBOL(gpHalMsWriteReg);

/**
* @brief MS set fifo level
* @param TxLevel[in]: TX_TRI_L1, TX_TRI_L2, TX_TRI_L4, TX_TRI_L8
* @param RxLevel[in]: RX_TRI_L1, RX_TRI_L2, RX_TRI_L4, RX_TRI_L8
* @param[out] none
* @return none
*/
void gpHalMsSetFifoLevel(
	UINT32 TxLevel,
	UINT32 RxLevel
)
{
	msReg_t *pmsReg = (msReg_t *)(LOGI_ADDR_MS_REG);
	
	pmsReg->msControl &= ~RX_TRI;
	pmsReg->msControl |= RxLevel;
	
	pmsReg->msControl &= ~TX_TRI;
	pmsReg->msControl |= TxLevel;
}
EXPORT_SYMBOL(gpHalMsSetFifoLevel);
