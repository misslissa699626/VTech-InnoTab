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
 * @file    hal_sd.c
 * @brief   Implement of SD HAL API.
 * @author  Dunker Chen
 * @since   2010-10-20
 * @date    2010-10-20
 */
 
#include <mach/io.h>
#include <mach/module.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/regmap/reg_sd.h>
#include <mach/hal/hal_sd.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

#define SD_CLK_EN			1
#define SD_CLK_DIS			0
#define COMMAND_TIMEOUT 	HZ	/* Typically 64 clock cycle */
#define DATA_TIMEOUT		HZ	/* Typically 250ms for SDHC, 500ms for SDXC */

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
 
#if 1
#define DEBUG	printk
#else
#define DEBUG(...)
#endif

#define DERROR printk 

/*========================================================================
*	        P_SDCX_CMD: SD Card Command register
* ========================================================================*/
#define MASK_CMDSTOP			0x0040		/*!< @briefStop Command. */
#define MASK_CMDRun				0x0080		/*!< @brief Initiate the SD command, will be cleared to '0' after the transaction start. */
#define MASK_CMDWithData		0x0100		/*!< @brief 0: Command without data, 1: Command with data. */
#define MASK_TransData			0x0200		/*!< @brief !MASK_TransferData = MASK_ReceiveData. */
#define MASK_TransMultiBlock	0x0400		/*!< @brief !MASK_TransMultiBlock = MASK_TransSingleBlock. */
#define MASK_ClockCycle74		0x0800		/*!< @brief 74 Clock cycles on the clock line. */
#define MASK_RESPTYPE  			0x7000		/*!< @brief Response type R1b. */
#define MASK_RESPTYPE0  		0x0000		/*!< @brief No response. */
#define MASK_RESPTYPE1  		0x1000 		/*!< @brief Response type R1. */
#define MASK_RESPTYPE2  		0x2000 		/*!< @brief Response type R2. */	
#define MASK_RESPTYPE3  		0x3000		/*!< @brief contains OCR register R3. */
#define MASK_RESPTYPE6  		0x6000		/*!< @brief Response type R6. */
#define MASK_RESPTYPE1b 		0x7000		/*!< @brief Response type R1b. */

#define MASK_S_DataStatusMask (MASK_S_DataComplete|MASK_S_DataBufFull|MASK_S_DataBufEmpty|MASK_S_DataCRCError)

#define SetSDBase(x) ((x==0)?LOGI_ADDR_SD0_REG:LOGI_ADDR_SD1_REG)
#define IS_SDIO_CMD() ((psdReg->sdCmd&0x3f)==52 || (psdReg->sdCmd&0x3f)==53)
#define IS_SDIO_CMDX(cmd) ((cmd&0x3f)==52 || (cmd&0x3f)==53)
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
* @brief 		SD clock setting function.
* @param 		device_id[in]: Index of SD controller.
* @param 		en[in]: Clock enable or disable.
* @return		None.
*/
static void gpHalSDClk(
	UINT32 device_id,
	UINT32 en)
{
	scucReg_t* scuc = (scucReg_t*)LOGI_ADDR_SCU_C_REG;
	
	if(en==SD_CLK_EN)
	{
		scuc->scucPeriClkEn |= (0x040000<<device_id);
	}
	else
	{
		scuc->scucPeriClkEn &= ~(0x040000<<device_id);	
	}
}

/**
* @brief 		SD reset function.
* @param 		device_id[in]: Index of SD controller.
* @return		None.
*/
static void gpHalSDRst(
	UINT32 device_id)
{
	scucReg_t* scuc = (scucReg_t*)LOGI_ADDR_SCU_C_REG;
	/* ----- Enable SD channel reset ----- */
	scuc->scucPeriRst |= (0x040000<<device_id);
	/* ----- Disable SD channel reset ----- */
	scuc->scucPeriRst &= ~(0x040000<<device_id);	
} 

/**
* @brief 		SD polling stataus function when send command.
* @param 		device_id[in]: Index of SD controller.
* @param 		pollbit[in]: Polling status bit.
* @return		SP_TRUE: success, SP_FALSE: timeout or status fail.
*/
static SP_BOOL WaitSDStatus(
	UINT32 device_id,
	UINT32 pollbit)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	UINT32 init = jiffies;
	
	do
	{
		/* ----- SD status error ----- */
		if (psdReg->sdStatus & (MASK_S_RespIdxError|MASK_S_RespCRCError|MASK_S_TimeOut)) 
		{
			DEBUG("cmd SD%d, CMD = 0x%x, STATUS = 0x%x\n", device_id, psdReg->sdCmd, psdReg->sdStatus);
			return SP_FALSE;
		}
		/* ----- Timeout ----- */
		if((jiffies-init)>=COMMAND_TIMEOUT)
		{
			DEBUG("cmd SD%d timeout, CMD = 0x%x, STATUS = 0x%x\n", device_id, psdReg->sdCmd, psdReg->sdStatus);
			return SP_FALSE;
		}
	}while((psdReg->sdStatus & pollbit)!=pollbit);
	
	return SP_TRUE;
}

/**
* @brief 		SD polling stataus function when send data.
* @param 		device_id[in]: Index of SD controller.
* @param 		pollbit[in]: Polling status bit.
* @return		SP_TRUE: success, SP_FALSE: timeout or status fail.
*/
static SP_BOOL WaitSDStatus_Data(
	UINT32 device_id,
	UINT32 pollbit)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	UINT32 init = jiffies;
 
	do
	{
		/* ----- SD status error ----- */
		if (psdReg->sdStatus & MASK_S_DataCRCError) 
		{
			DEBUG("data SD%d CRC error, CMD = 0x%x, STATUS = 0x%x\n", device_id, psdReg->sdCmd, psdReg->sdStatus);
			return SP_FALSE;
		}
		/* ----- Timeout ----- */
		if((jiffies-init)>=DATA_TIMEOUT)
		{
			DEBUG("data SD%d timeout, CMD = 0x%x, STATUS = 0x%x\n", device_id, psdReg->sdCmd, psdReg->sdStatus);
			return SP_FALSE;
		}
	} while((psdReg->sdStatus & pollbit)!=pollbit);
	
	return SP_TRUE;
}
 
/**
* @brief 	SD init function.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
void gpHalSDInit(
	UINT32 device_id)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	/* ----- Enable SDMA channel clock ----- */
	gpHalSDClk(device_id, SD_CLK_EN);	
	/* ----- Reset SDMA channel ----- */
	gpHalSDRst(device_id);
	/* ----- Set FIFO level trigger  4 bytes ----- */
	psdReg->sdCtrl &= ~0xf000;
}
//EXPORT_SYMBOL(gpHalSDInit);

/**
* @brief 		SD un-init function.
* @return		None.
*/
void gpHalSDUninit(
	UINT32 device_id)
{
	/* ----- Reset SDMA channel ----- */
	gpHalSDRst(device_id);
	/* ----- Disable SDMA channel clock ----- */
	gpHalSDClk(device_id, SD_CLK_DIS);	
}
//EXPORT_SYMBOL(gpHalSDUninit);

/**
* @brief 	Set SD transfer 74 clock.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
void gpHalSD74Clk(
	UINT32 device_id)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	psdReg->sdCmd = MASK_ClockCycle74;	
	WaitSDStatus(device_id,MASK_S_CmdComplete);
	psdReg->sdStatus = MASK_S_ClrAllBits;
}
//EXPORT_SYMBOL(gpHalSD74Clk);

/**
* @brief 	Set SD clock.
* @param 	device_id[in]: Index of SD controller.
* @param 	apb_clk [in]:  System apb clock (unit: 100KHz).
* @param 	clk[in]: SD clock(unit:100KHz).
* @return	Actually clock (unit:100KHz).
*/
UINT16 gpHalSDSetClk(
	UINT32 device_id, 
	UINT16 apb_clk, 
	UINT16 clk)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);	
	UINT8 div;
	if(clk>=(apb_clk>>1))
		div = 0;
	else
	{
		div = (((apb_clk/clk)+1)>>1)-1;	
	}
	psdReg->sdCtrl &= ~0xff;
	psdReg->sdCtrl |= div;
	return (apb_clk/(div+1))>>1;
}
//EXPORT_SYMBOL(gpHalSDSetClk);

/**
* @brief 	Set SD block size.
* @param 	device_id[in]: Index of SD controller.
* @param 	len[in]: Block length (0~4095).
* @return	None.
*/
void gpHalSDSetBlk(
	UINT32 device_id, 
	UINT16 len)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	psdReg->sdCtrl &= ~0xfff0000;
	psdReg->sdCtrl |= (len&0xfff)<<16;
}
//EXPORT_SYMBOL(gpHalSDSetBlk);

/**
* @brief 	Set SD bus width.
* @param 	device_id[in]: Index of SD controller.
* @param 	bus_width[in]: 0 for 1 bit mode, 1 for 4 bits mode.
* @return	None.
*/
void gpHalSDSetBus (
	UINT32 device_id, 
	UINT32 bus_width)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	if(bus_width)	
		psdReg->sdCtrl |= 0x100;
	else
		psdReg->sdCtrl &= ~0x100;
}
//EXPORT_SYMBOL(gpHalSDSetBus);

/**
* @brief 	Stop SD controller, also stop transaction.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
SP_BOOL gpHalSDStop (
	UINT32 device_id)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	UINT32 init = jiffies;
	SP_BOOL ret = SP_TRUE;
	/* ----- Stop the controller ----- */
	psdReg->sdCmd = MASK_CMDSTOP;
	/* ----- Wait for controller idle -----	*/
	while(psdReg->sdStatus & MASK_S_ControllerBusy)
	{
		/* ----- Timeout ----- */
		if((jiffies-init)>=COMMAND_TIMEOUT)
		{
			ret = SP_FALSE;
			DEBUG("stop error, status = 0x%x\n",psdReg->sdStatus);
			break;
		}
	}
	psdReg->sdStatus = MASK_S_ClrAllBits;
	return ret;		
}
//EXPORT_SYMBOL(gpHalSDStop);

/**
* @brief 	Wait for data complete (only for write).
* @param 	device_id[in]: Index of SD controller.
* @return	SUCCESS/ERROR_ID..
*/
SP_BOOL gpHalSDWaitDataComplete (
	UINT32 device_id)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	UINT32 init = jiffies;
    
	/* ----- Wait for data empty and complete ----- */
	if(WaitSDStatus_Data(device_id, MASK_S_DataBufEmpty|MASK_S_DataComplete) == SP_FALSE)
		return SP_FALSE;
    
	/* ----- Wait for card in idle state ----- */
	while(psdReg->sdStatus & MASK_S_CardBusy)
	{
		/* ----- Timeout ----- */
		if((jiffies-init)>=DATA_TIMEOUT)
		{
			DEBUG("Wait SD%d data complete timeout, CMD = 0x%x, STATUS = 0x%x\n", device_id, psdReg->sdCmd, psdReg->sdStatus);
			psdReg->sdStatus = MASK_S_ClrAllBits;
			return SP_FALSE;
		}
	}
	psdReg->sdStatus = MASK_S_ClrAllBits;
	return SP_TRUE;		
}
//EXPORT_SYMBOL(gpHalSDWaitDataComplete);

/**
* @brief 	Stop SD controller, also stop transaction.
* @param 	device_id[in]: Index of SD controller.
* @param 	param[in]: Command parameter.
* @param 	arg[in]: Command argument.
* @param 	resp[out]: Response buffer.
* @param 	ln[in]: Response buffer size (unit: byte).
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpHalSDSendCmd (
	UINT32 device_id, 
	gpHalSDCmd_t param, 
	UINT32 arg, 
	UINT32* resp, 
	UINT32 ln)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	UINT32 rln;
	UINT32 cmd = 0;
	/* ----- Set Response type ----- */
	switch(param.resp_type)
	{
		case SDC_RESPTYPE_NONE:
			rln =0;
			break;
		case SDC_RESPTYPE_R2:
			cmd |= MASK_RESPTYPE2;
			rln = 4;
			break;
		case SDC_RESPTYPE_R3:
			cmd |= MASK_RESPTYPE3;
			rln = 1;
			break;
		case SDC_RESPTYPE_R6:
			cmd |= MASK_RESPTYPE6;
			rln = 1;
			break;
		case SDC_RESPTYPE_R1b:
			cmd |= MASK_RESPTYPE1b;
			rln = 1;
			break;
		case SDC_RESPTYPE_R1:
		case SDC_RESPTYPE_R4:
		case SDC_RESPTYPE_R5:
		case SDC_RESPTYPE_R7:
		default:
			cmd |= MASK_RESPTYPE1;
			rln = 1;
			break;
	}
	/* ----- Set data transfer ----- */
	if(param.with_data)
	{
		cmd |= MASK_CMDWithData;
		/* ----- Set multi-block -----*/
		if(param.with_data&0x02)
		{
			cmd |= MASK_TransMultiBlock;
                    if (!IS_SDIO_CMDX(param.cmd))
			    psdReg->sdCtrl |= 0xf000;
		}
		else
		{
                    if (!IS_SDIO_CMDX(param.cmd))
			    psdReg->sdCtrl &= ~0xf000;
		}
		/* ----- Set data direction -----*/
		if(param.dir)
			cmd |= MASK_TransData;
	}
	/* ----- Set command ----- */
	cmd |= MASK_CMDRun|(param.cmd&0x3f);
	
	rln = (rln>(ln>>2))?(ln>>2):rln;
	psdReg->sdArg = arg;
	psdReg->sdCmd = cmd;
	
	while(rln)
	{
        if(WaitSDStatus(device_id, MASK_S_RespRegFull) == SP_FALSE)
		{
          
            DEBUG("\n%s(), %d, status=0x%x.", __FUNCTION__, __LINE__, psdReg->sdStatus);
			psdReg->sdStatus = MASK_S_ClrAllBits;
			return SP_FALSE;
		}
		*resp++ = psdReg->sdResp;
		rln--;
	}
    
    if (!IS_SDIO_CMD() && WaitSDStatus(device_id,MASK_S_CmdComplete) == SP_FALSE) 
	{
        DEBUG("\n%s(), %d, status=0x%x.", __FUNCTION__, __LINE__, psdReg->sdStatus);
		psdReg->sdStatus = MASK_S_ClrAllBits;
		return SP_FALSE;
	}
    
       if(!IS_SDIO_CMD()  || param.with_data == 0)
       {
	psdReg->sdStatus = MASK_S_ClrAllBits;
       }
	return SP_TRUE;
	
}
//EXPORT_SYMBOL(gpHalSDSendCmd);

/**
* @brief 	Tx Rx SD data.
* @param 	device_id[in]: Index of SD controller.
* @param 	buf [in]: Buffer address (4 byte alignment).
* @param 	ln[in]: Buffer length (unit: byte, multiple of 4).
* @param 	dir[in]: Data direction, 0 for read, others for write.
* @return 	SUCCESS/ERROR_ID.
*/
SP_BOOL gpHalSDDataTxRx (
	UINT32 device_id, 
	UINT8* buf, 
	UINT32 ln, 
	UINT32 dir)
{
	sdReg_t *psdReg = (sdReg_t*) SetSDBase(device_id);
	UINT32* tbuf = (UINT32*)buf;
	psdReg->sdCtrl &= ~0xf000;
	/* ----- Write operation ----- */ 
	if(dir)
	{
		/* ----- Fill data to SD controller data port ----- */	
		while(ln)
		{
			if(WaitSDStatus_Data(device_id, MASK_S_DataBufEmpty) == SP_FALSE)
			{
				psdReg->sdStatus = MASK_S_ClrAllBits;
				return SP_FALSE;	
			}
			psdReg->sdDatTx = *tbuf++;
			ln -= 4;
		}	
		/* ----- Wait for data empty ----- */
		if(gpHalSDWaitDataComplete(device_id) == SP_FALSE) 
		{
			psdReg->sdStatus = MASK_S_ClrAllBits;
			return SP_FALSE;
		}
		psdReg->sdStatus = MASK_S_ClrAllBits;
		return SP_TRUE;		
	}
	/* ----- Read operation ----- */ 
	else
	{
		while(ln)
		{
			if(WaitSDStatus_Data(device_id, MASK_S_DataBufFull) == SP_FALSE)
			{
				psdReg->sdStatus = MASK_S_ClrAllBits;
				return SP_FALSE;	
			}
			*tbuf++ = psdReg->sdDatRx;
			ln -= 4;
		}	
		/* ----- Wait for data complete ----- */
		if(!IS_SDIO_CMD() && WaitSDStatus_Data(device_id, MASK_S_DataComplete) == SP_FALSE)
			return SP_FALSE;
		psdReg->sdStatus = MASK_S_ClrAllBits;
		return SP_TRUE;	
	}
}
//EXPORT_SYMBOL(gpHalSDDataTxRx);

/**
* @brief 	Get SD status.
* @param 	device_id[in]: Index of SD controller.
* @return	current status.
*/
UINT32 
gpHalSDGetStatus (
    UINT32 deviceId
)
{
    sdReg_t *psdReg = (sdReg_t*) SetSDBase(deviceId);

    return psdReg->sdStatus;
}
//EXPORT_SYMBOL(gpHalSDGetStatus);

/**
* @brief 	Clear SD status.
* @param 	device_id[in]: Index of SD controller.
* @return	None.
*/
void 
gpHalSDClearStatus (
    UINT32 deviceId
)
{
    sdReg_t *psdReg = (sdReg_t*) SetSDBase(deviceId);

    psdReg->sdStatus = MASK_S_ClrAllBits;
}
//EXPORT_SYMBOL(gpHalSDClearStatus);

/**
* @brief 	Enable/disable SD interrupt.
* @param 	device_id[in]: Index of SD controller
* @param 	intInfo[in]: Interrupt enable/disable bits
* @param 	enable[in]: 1 for enable, 0 for disable
* @return	None.
*/
void 
gpHalSDEnableInterrupt(
    UINT32 deviceId, 
    UINT32 intInfo, 
    UINT32 enable
)
{
    sdReg_t *psdReg = (sdReg_t*) SetSDBase(deviceId);

    intInfo &= SD_MMC_INT_ALLMASK;
    if (enable) {
        psdReg->sdIntEn |= intInfo;
    }
    else {
        psdReg->sdIntEn &= ~intInfo;
    }

    if(intInfo & SD_MMC_INT_SDIODETECT) {
        if (enable) {
            psdReg->sdCtrl |= (MASK_C_IOMODE|MASK_C_ENSDBUS);
        }
        else {
            psdReg->sdCtrl &= ~(MASK_C_IOMODE/*|MASK_C_ENSDBUS*/);
        }
    }
    
    return ;
}
//EXPORT_SYMBOL(gpHalSDEnableInterrupt);

/**
* @brief 	Enable/disable SD DMA mode.
* @param 	device_id[in]: Index of SD controller
* @param 	enable[in]: 1 for enable, 0 for disable
* @return	None.
*/
void 
gpHalSDEnableDma(
    UINT32 deviceId, 
    UINT32 enable
)
{
    sdReg_t *psdReg = (sdReg_t*) SetSDBase(deviceId);
    
    if (enable) {
        psdReg->sdCtrl |= (MASK_C_DMAMODE | 0xF000);
        /*psdReg->sdCtrl |= (MASK_C_DMAMODE);*/
    }
    else {
        psdReg->sdCtrl &= ~(MASK_C_DMAMODE | 0xF000);
        /*psdReg->sdCtrl &= ~(MASK_C_DMAMODE);*/
    }

    return ;
}
//EXPORT_SYMBOL(gpHalSDEnableDma);

/**
* @brief 	Dump SD registers.
* @param 	deviceId[in]: Index of SD controller
* @return	None.
*/
void
gpHalSDDump(
    UINT32 deviceId
)
{
    unsigned char *buf = (UINT8*) SetSDBase(deviceId);
    unsigned int len = 32;    
    int i = 0;
    
    while (len--) {
        if (i % 16 == 0)
            DEBUG("\n");
        if (i % 4 == 0)
            DEBUG(" ");
        DEBUG("%02x ", *buf++);
        i++;
    }
    DEBUG("\n");
}
//EXPORT_SYMBOL(gpHalSDDump);
