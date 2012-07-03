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
 * @file    gp_sd_core.c
 * @brief   Implement of SD card module driver.
 * @author  Dunker Chen
 */
 
#include <mach/io.h>
#include <mach/module.h> 
#include <mach/kernel.h> 
#include <mach/diag.h>
#include <mach/bdev.h>
#include <mach/gp_sd.h>
#include <mach/hal/hal_sd.h>
#include <mach/hal/hal_clock.h>
#include <mach/gp_cache.h>
#include <mach/gp_chunkmem.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/
#define USE_DMA		
//#define POLL		
#define INITIAL_TIMEOUT		5*HZ
#define IDENT_CLOCK		(300 / 100)
#define SECTOR_SIZE			512
/* ----- Card Version defintition ----- */
#define SDCARD10 				0
#define SDCARD11 				1
#define SDCARD20 				2
/* ----- Card Capacity Type defintition ----- */
#define SD_StandardCapacity		0
#define SD_HighCapacity			1
/* ----- SD command ----- */
static const  gpHalSDCmd_t SD_CMD0		= {0, 	SDC_RESPTYPE_NONE, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD2		= {2, 	SDC_RESPTYPE_R2, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD3		= {3, 	SDC_RESPTYPE_R6, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD5		= {5, 	SDC_RESPTYPE_R1, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD6		= {6, 	SDC_RESPTYPE_R1, 	SDC_SINGLE_DATA, 	SDC_READ};
static const  gpHalSDCmd_t SD_CMD7		= {7, 	SDC_RESPTYPE_R1b, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD7_DIS	= {7, 	SDC_RESPTYPE_NONE, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD8		= {8, 	SDC_RESPTYPE_R7, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD9		= {9, 	SDC_RESPTYPE_R2, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD12		= {12, 	SDC_RESPTYPE_R1b, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD13		= {13, 	SDC_RESPTYPE_R1, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_CMD18		= {18, 	SDC_RESPTYPE_R1, 	SDC_MULTI_DATA, 	SDC_READ};
static const  gpHalSDCmd_t SD_CMD25		= {25, 	SDC_RESPTYPE_R1, 	SDC_MULTI_DATA, 	SDC_WRITE};
static const  gpHalSDCmd_t SD_CMD55		= {55, 	SDC_RESPTYPE_R1, 	SDC_NO_DATA, 		0};
/* ----- SD application command ----- */
static const  gpHalSDCmd_t SD_ACMD6		= {6, 	SDC_RESPTYPE_R1, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_ACMD41	= {41, 	SDC_RESPTYPE_R3, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t SD_ACMD51	= {51, 	SDC_RESPTYPE_R1, 	SDC_SINGLE_DATA, 	SDC_READ};
/* ----- MMC command ---- */
static const  gpHalSDCmd_t MMC_CMD1		= {1, 	SDC_RESPTYPE_R3, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t MMC_CMD6		= {6, 	SDC_RESPTYPE_R1b, 	SDC_NO_DATA, 		0};
static const  gpHalSDCmd_t MMC_CMD8		= {8, 	SDC_RESPTYPE_R1, 	SDC_SINGLE_DATA, 	SDC_READ};

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

#if 0
#define DEBUG(fmt, arg...)	DIAG_DEBUG("[%s][%d][DBG]: "fmt, __FUNCTION__, __LINE__, ##arg)
#else
#define DEBUG(...)
#endif

#define DERROR(fmt, arg...)  DIAG_DEBUG("[%s][%d][ERR]: "fmt, __FUNCTION__, __LINE__, ##arg)

#define READ_TIMEOUT	15
#define WRITE_TIMEOUT	55
//#define SD_CLK_LIMIT	500		/* 50MHz */

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

extern int sd_clock;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
 
/**************************************************************************
 *                         G L O B A L    D A T A                         *
**************************************************************************/
static int SD_CLK_LIMIT;		/* 20MHz */

extern gpSDInfo_t *sd_info;

/**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
**************************************************************************/
 
/**
* @brief 	Interpret speed function.
* @param 	factor[in]: Speed factor.
* @return 	Card speed (unit: 100KHz).
*/ 
static unsigned int gp_sdcard_despeed(unsigned char factor)
{
	if(sd_clock > 0)
		SD_CLK_LIMIT = sd_clock * 10;
	else
		SD_CLK_LIMIT = 500;

	unsigned char SpeedTable[16]={0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
	unsigned short TranTable[4]={1, 10, 100, 1000};
	unsigned int speed = SpeedTable[(factor& 0x78)>>3] * TranTable[factor & 0x07] / 10;
	return (speed > SD_CLK_LIMIT)? SD_CLK_LIMIT:speed;
}

/**
* @brief 	SD card read command function.
* @param 	device_id[in]: Index of SD controller.
* @param 	sector[in]: Start sector.
* @return 	SUCCESS/ERROR_ID.
*/
static int gp_sdcard_readcmd(unsigned int device_id, unsigned int sector)
{
	unsigned int response = 0;
	if(device_id>=SD_NUM)
		return SP_FALSE;
	/* ----- Check card initial or not ----- */
	if(sd_info[device_id].present == 0)
	{
		DERROR("[%d]: Card not present\n",device_id);
		return SP_FALSE;
	}
	/* ----- Check card is in transfer state or not ----- */
	if(gpHalSDSendCmd(device_id, SD_CMD13, sd_info[device_id].RCA, &response, 4)==SP_FALSE)
	{
		DERROR("[%d]: CMD13 fail, resp = %d \n", device_id, response );
		return SP_FALSE;
	}
	if((response&0x1F00)!=0x0900)
	{
		if((response&0x1F00)==0x0700)			/* In standby state */
			gpHalSDSendCmd(device_id, SD_CMD7, sd_info[device_id].RCA, &response, 4);		/* Set to transfer state */
		else
		{
			DERROR("[%d]: Card status error, resp = %d\n", device_id, response);
			return SP_FALSE;
		}
	}
	if(sd_info[device_id].cap_type == SD_StandardCapacity)
		sector = sector <<9;
	/* ----- Send multi read command ----- */
	if(gpHalSDSendCmd(device_id, SD_CMD18, sector, &response, 4) == SP_FALSE)
	{
		DERROR("[%d]: CMD18 fail, resp = %d\n",device_id, response);
		return SP_FALSE;
	}
	return SP_TRUE;	
}

/**
* @brief 	SD card write command function.
* @param 	device_id[in]: Index of SD controller.
* @param 	sector[in]: Start sector.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_sdcard_writecmd(unsigned int device_id, unsigned int sector)
{
	unsigned int response = 0;
	if(device_id>=SD_NUM)
		return SP_FALSE;
	/* ----- Check card initial or not ----- */
	if(sd_info[device_id].present == 0)
	{
		DERROR("[%d]: Card not present\n",device_id);
		return SP_FALSE;
	}
	/* ----- Check card is in transfer state or not ----- */
	if(gpHalSDSendCmd(device_id, SD_CMD13, sd_info[device_id].RCA, &response, 4)==SP_FALSE)
	{
		DERROR("[%d]: CMD13 fail, resp = %d \n", device_id, response );
		return SP_FALSE;
	}
	if((response&0x1F00)!=0x0900)
	{
		if((response&0x1F00)==0x0700)			/* In standby state */
			gpHalSDSendCmd(device_id, SD_CMD7, sd_info[device_id].RCA, &response, 4);		/* Set to transfer state */
		else
		{
			DERROR("[%d]: Card status error, resp = %d\n", device_id, response);
			return SP_FALSE;
		}
	}
	if(sd_info[device_id].cap_type == SD_StandardCapacity)
		sector = sector <<9;
	/* ----- Send multi read command ----- */
	if(gpHalSDSendCmd(device_id, SD_CMD25, sector, &response, 4) == SP_FALSE)
	{
		DERROR("[%d]: CMD25 fail, resp = %d\n",device_id, response);
		return SP_FALSE;
	}
	return SP_TRUE;	
}

/**
* @brief 	SD card dma enable function.
* @param 	device_id[in]: Index of SD controller.
* @param 	buf[in]: Buffer start address.
* @param 	ln[in]: Buffer length (unit: byte).
* @param 	dir[in]: Direction, 1 for write and 0 for read.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_sdcard_dma_en(unsigned int device_id, unsigned char* buf, unsigned int ln, unsigned int dir)
{
#ifdef USE_DMA
	sd_info[device_id].dma_param.buf0 = buf;
	sd_info[device_id].dma_param.ln0 = ln;
	sd_info[device_id].dma_param.dir = dir;
	gp_apbdma0_en(sd_info[device_id].handle_dma,sd_info[device_id].dma_param);
	DEBUG("SD dma: buf = 0x%x, ln = %d\n", (unsigned int)buf, ln);	
	return SP_TRUE;
#else
	if(gpHalSDDataTxRx(device_id, buf, ln, dir) == SP_FALSE)
		return SP_FALSE;	
	return SP_TRUE;
#endif
}

/**
* @brief 	SD card wait dma finish function.
* @param 	device_id[in]: Index of SD controller.
* @param 	timeout[in]: Timeout (unit: system tick).
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_sdcard_dma_finish(unsigned int device_id, unsigned short timeout)
{
#ifdef USE_DMA
#ifndef POLL
	return gp_apbdma0_wait(sd_info[device_id].handle_dma, timeout);
#else
	int init = jiffies;
	while(gp_apbdma0_trywait(sd_info[device_id].handle_dma)!=0)
	{
		if((jiffies-init)>= (timeout*HZ/10))
		{
			int i;
			for(i=0;i<0x20;i++)
			{
				unsigned int reg = *(unsigned int*)(0xfdb00000+i*4);
				DERROR("0x%x\n", reg);	
			}
			return -ETIMEDOUT;
		}
	}
#endif
#endif
	return 0;
}

/**
* @brief 	SD card stop transaction function.
* @param 	device_id[in]: Index of SD controller.
* @return 	SUCCESS/ERROR_ID.
 */
static int gp_sdcard_stop(unsigned int device_id)
{
	unsigned int response = 0;
	/* ----- Stop (reset) SD controller ----- */
	if(gpHalSDStop(device_id)==SP_FALSE)
	{
		DERROR("[%d]: Stop SD fail\n",device_id);
		goto out_error;
	}
	/* ----- Stop the card ----- */
	if(gpHalSDSendCmd(device_id, SD_CMD12, 0x00000000, &response, 4)==SP_FALSE)
	{
		DERROR("[%d]: Stop CMD(12) fail\n",device_id);
		goto out_error;	
	}
	return SP_TRUE;
out_error:
	return SP_FALSE;		
}

/**
* @brief 	SD card read function with scatter list.
* @param 	sd[in]: Card information.
* @param 	sector[in]: Start sector.
* @param 	sg[in]: Scatter list pointer.
* @param 	ln[in]: List number.
* @return 	Actual sectors by reading/ERROR_ID(<0).
*/
int gp_sdcard_read_scatter(gpSDInfo_t * sd, unsigned int sector, struct scatterlist *sg, unsigned int ln)
{
	int i;
	struct scatterlist *sg_ev;
	int ret = 0;
	int sector_num = 0;
#ifdef USE_DMA	
	int sgln;
	sgln = dma_map_sg(NULL, sg, ln, DMA_FROM_DEVICE);
	if(sgln!=ln)
	{
		dma_unmap_sg(NULL, sg, sgln, DMA_FROM_DEVICE);
		DERROR("[%d]: SG map fail, sgln = %d, ln = %d\n", sd->device_id, sgln, ln);
		return -ENOMEM;
	}
#endif
	if(gp_sdcard_readcmd(sd->device_id,sector)==SP_FALSE)
	{
		ret = -EIO;
		goto out_error;
	}
	DEBUG("SD: read sector %d\n", sector);	
	for_each_sg(sg, sg_ev, ln, i) 
	{
		unsigned int number = sg_dma_len(sg_ev)>>9;
		/* ----- Start dma ----- */ 
	#ifdef USE_DMA
	#ifndef GP_SYNC_OPTION
		gp_invalidate_dcache_range((unsigned int)sg_virt(sg_ev),number*512);	
	#endif
		if(gp_sdcard_dma_en(sd->device_id, (unsigned char*)sg_phys(sg_ev), sg_dma_len(sg_ev),0)==SP_FALSE)
	#else
		if(gp_sdcard_dma_en(sd->device_id, (unsigned char*)sg_virt(sg_ev), sg_dma_len(sg_ev),0)==SP_FALSE)
	#endif
		{
			ret = -ENOMEM;
			DERROR("[%d]:DMA Enable error\n", sd->device_id);
			goto out_error;
		}
		/* ----- Wait dma finish ----- */
		ret = gp_sdcard_dma_finish(sd->device_id, (sg_dma_len(sg_ev)>>9)*READ_TIMEOUT);
		if(ret!=0)
		{
			gp_apbdma0_stop(sd->handle_dma);
			DERROR("[%d]:DMA error: %d, SD status 0x%x\n", sd->device_id, ret, gpHalSDGetStatus(sd->device_id));
			goto out_error;
		}
		sector_num += number;
	}
	
#ifdef GP_SYNC_OPTION
	GP_SYNC_CACHE();
#endif

out_error:
#ifdef USE_DMA	
	dma_unmap_sg(NULL, sg, sgln, DMA_FROM_DEVICE);
#endif
	if(gp_sdcard_stop(sd->device_id)==SP_FALSE)
		return -EIO;
	if(ret)
		return ret;
	else
		return sector_num;
}

/**
* @brief 	SD card write function with scatter list.
* @param 	sd[in]: Card information.
* @param 	sector[in]: Start sector.
* @param 	sg[in]: Scatter list pointer.
* @param 	ln[in]: List number.
* @return 	Actual sectors by reading/ERROR_ID(<0).
*/
int gp_sdcard_write_scatter(gpSDInfo_t * sd, unsigned int sector, struct scatterlist *sg, unsigned int ln)
{
	int i;
	struct scatterlist *sg_ev;
	int ret = 0;
	int sector_num = 0;
#ifdef USE_DMA	
	unsigned sgln;
	sgln = dma_map_sg(NULL, sg, ln, DMA_TO_DEVICE);
	if(sgln!=ln)
	{
		dma_unmap_sg(NULL, sg, sgln, DMA_TO_DEVICE);
		DERROR("[%d]: SG map fail, sgln = %d, ln = %d\n", sd->device_id, sgln, ln);
		return -ENOMEM;
	}
#endif	
	if(gp_sdcard_writecmd(sd->device_id,sector)==SP_FALSE)
	{
		ret = -EIO;
		goto out_error;
	}
	DEBUG("SD: write sector %d\n", sector);
#ifdef GP_SYNC_OPTION
	GP_SYNC_CACHE();
#endif
	for_each_sg(sg, sg_ev, ln, i) 
	{
		unsigned int number = sg_dma_len(sg_ev)>>9;
		/* ----- Start dma ----- */ 
	#ifdef USE_DMA
	#ifndef GP_SYNC_OPTION
		gp_clean_dcache_range((unsigned int)sg_virt(sg_ev),number*512);	
	#endif
		if(gp_sdcard_dma_en(sd->device_id, (unsigned char*)sg_phys(sg_ev), sg_dma_len(sg_ev),1)==SP_FALSE)
	#else
		if(gp_sdcard_dma_en(sd->device_id, (unsigned char*)sg_virt(sg_ev), sg_dma_len(sg_ev),1)==SP_FALSE)
	#endif
		{
			ret = -ENOMEM;
			DERROR("[%d]:DMA Enable error\n", sd->device_id);
			goto out_error;
		}
		/* ----- Wait dma finish ----- */
		ret = gp_sdcard_dma_finish(sd->device_id, (sg_dma_len(sg_ev)>>9)*WRITE_TIMEOUT);
		if(ret!=0)
		{
			gp_apbdma0_stop(sd->handle_dma);
			DERROR("[%d]:DMA error: %d, SD status 0x%x\n", sd->device_id, ret,gpHalSDGetStatus(sd->device_id));
			goto out_error;
		}
	#ifdef USE_DMA	
		if(gpHalSDWaitDataComplete(sd->device_id) == SP_FALSE)
		{
			DERROR("[%d]: wait complete error: SD status 0x%x\n", sd->device_id, gpHalSDGetStatus(sd->device_id));
			ret = -ETIMEDOUT;
			goto out_error;
		}
	#endif
		sector_num += number;
	}
	
out_error:
#ifdef USE_DMA
	dma_unmap_sg(NULL, sg, sgln, DMA_TO_DEVICE);
#endif
	if(gp_sdcard_stop(sd->device_id)==SP_FALSE)
		return -EIO;
	if(ret)
		return ret;
	else
		return sector_num;
}

/**
* @brief 	Card identification function.
* @param 	sd[in]: Card information.
* @return 	Card type: SD, MMC, SDIO or unknow.
*/
static gpSDType_t gp_sdcard_idmode(gpSDInfo_t* sd)
{
	unsigned int init;	
	unsigned int CardVer;
	gpSDType_t cardtype = SDCARD;
	unsigned int response;
       
	/* ----- Start 74 cycles on SD Clk Bus ----- */
	gpHalSD74Clk(sd->device_id);
	/* ----- Reset Command, Should be no response ----- */
	gpHalSDSendCmd(sd->device_id, SD_CMD0, 0x00000000, &response, 0);
	/* ----- Run CMD8 for detection version 2.0 or later ----- */
	CardVer = SDCARD10;
	if(gpHalSDSendCmd(sd->device_id, SD_CMD8, 0x000001AA, &response, 4)==SP_TRUE)
	{
		 /* ----- mismatch voltage or check pattern error ----- */
		 if(response == 0x000001AA)
	     	CardVer = SDCARD20;
	}
    
    /* ----- Send CMD5 to detect SDIO card ----- */    
	if(gpHalSDSendCmd(sd->device_id, SD_CMD5, 0x0, &response, 4)==SP_TRUE)
	{
            DEBUG("\nCMD5 SDIO response=0x%x!", response);
            sd->RCA = response;
            cardtype = SDIO;
            return cardtype;
	}
	/* ----- Run ACMD 41 until card finish power up sequence ----- */
	init = jiffies;
	do
	{
		if (cardtype== SDCARD) 
		{
			if (gpHalSDSendCmd(sd->device_id, SD_CMD55, 0x00000000, &response, 4) == SP_FALSE) 
				cardtype = MMCCARD;
		}
		/* ----- ACMD 41 ----- */
		if (cardtype == SDCARD)
		{
			unsigned int Argu = 0x00200000;
			if (CardVer == SDCARD20)
				Argu = 0x40200000;		/* For high capacity setting */
			gpHalSDSendCmd(sd->device_id, SD_ACMD41, Argu, &response, 4);		
		}
		/* ----- MMC card ----- */
		else
		{
			if(gpHalSDSendCmd(sd->device_id, MMC_CMD1, 0x40FF8000, &response, 4)==SP_FALSE)
				return UNKNOWCARD;
		}
		
		if ((jiffies-init)>=INITIAL_TIMEOUT) 
		{
			return UNKNOWCARD;
		}
	} while ((response & 0x80000000) == 0);
	if(response & 0x40000000)
		sd->cap_type = SD_HighCapacity;
	/* ----- Step 5, CMD 2 Read CID Register ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD2, 0x00000000, sd->CID, 16) == SP_FALSE)
		return UNKNOWCARD;
	
	if (cardtype == SDCARD)
	{
		/* ----- Step 6, CMD 3 Read New RCA, SD will generate RCA itself ----- */
		if(gpHalSDSendCmd(sd->device_id, SD_CMD3, 0x00000000, &response, 4) == SP_FALSE)
			return UNKNOWCARD;
		sd->RCA = response & 0xFFFF0000;
	} 
	else
	{
		/* ----- Step 6, CMD 3 Set New RCA, MMC need to be assigned a new RCA ----- */
		if(gpHalSDSendCmd(sd->device_id, SD_CMD3, 0xFFFF0000, &response, 4) == SP_FALSE)
			return UNKNOWCARD;
		sd->RCA = 0xFFFF0000;
	}
	/* ----- Read Status ----- */
	gpHalSDSendCmd(sd->device_id, SD_CMD13, sd->RCA, &response, 4);
	if (response != 0x0700) 
		return UNKNOWCARD;
	return cardtype;	
}

/**
* @brief 	Card set bus function.
* @param 	sd[in]: Card information.
* @param	mode[in]: 1 for 4 bit, 0 for 1 bit.
* @return 	SUCCESS/ERROR_ID.
*/
int gp_sdcard_setbus(gpSDInfo_t* sd, unsigned int mode)
{
	unsigned int response;
	unsigned int arg;
	/* ----- Check card is in transfer state or not ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD13, sd->RCA, &response, 4)==SP_FALSE)
		return -EIO;
	if((response&0x1F00)!=0x0900)
	{
		if((response&0x1F00)==0x0700)			/* In standby state */
			gpHalSDSendCmd(sd->device_id, SD_CMD7, sd->RCA, &response, 4);		/* Set to transfer state */
		else
			return -EIO;
	}	
	/* ----- Set Bus width ----- */	
	if(sd->card_type == SDCARD)
	{
		arg = (mode)? 0x02:0x00;
		
		if(gpHalSDSendCmd(sd->device_id, SD_CMD55, sd->RCA, &response, 4)==SP_FALSE)
			return -EIO;
		if(gpHalSDSendCmd(sd->device_id, SD_ACMD6, arg, &response, 4)==SP_FALSE)
			return -EIO;
		gpHalSDSetBus(sd->device_id, mode);	
	}
	else if(sd->card_type == MMCCARD)
	{
		arg = (mode)? 0x03b70100:0x03b70000;
		if(gpHalSDSendCmd(sd->device_id, MMC_CMD6, arg, &response, 4) == SP_FALSE)
			return -EIO;
		if((response&0x08)==0)
			gpHalSDSetBus(sd->device_id, mode);		
	}
	else
		return -EIO;
	return 0;
}

/**
* @brief 	SD switch high speed function.
* @param 	sd[in]: Card information.
* @param	mode[in]: 1 for high speed (50MHz), 0 for default speed (25MHz).
* @return 	SUCCESS/ERROR_ID.
*/
static bool gp_sdcard_switch(gpSDInfo_t* sd, unsigned int mode)
{
	unsigned int buf[16]={0};
	unsigned int resp={0};
	unsigned int argu1 = 0x00FFFFF0|mode;
	unsigned int argu2 = 0x80FFFFF0|mode;	
	unsigned char* cbuf = (unsigned char *)buf;
	gpHalSDSetBlk(sd->device_id, 64);
	/* ----- Mode 0 operation - Check function ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD6, argu1, &resp, 4)==SP_FALSE)
		return SP_FALSE;
	if(gpHalSDDataTxRx(sd->device_id, cbuf, 64, 0)== SP_FALSE)
		return SP_FALSE;
	/* ----- does not support high speed ----- */
	if(mode)
	{
		if(((cbuf[13]&0x02)==0)||((cbuf[16]&0x01)==0))
			return SP_FALSE;
	}
	/* ----- Mode 1 operation - Set function ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD6, argu2, &resp, 4)==SP_FALSE)
		return SP_FALSE;
	if(gpHalSDDataTxRx(sd->device_id, cbuf, 64, 0)== SP_FALSE)
		return SP_FALSE;
	if((cbuf[0]==0x00)&&(cbuf[1]==0x00))
		return SP_FALSE;
	if(((cbuf[14]>>8)==0x0F)||((cbuf[14]&0x0F)==0x0F))
		return SP_FALSE;
	if(((cbuf[15]>>8)==0x0F)||((cbuf[15]&0x0F)==0x0F))
		return SP_FALSE;
	if(((cbuf[16]>>8)==0x0F)||((cbuf[16]&0x0F)==0x0F))
		return SP_FALSE;
	gpHalSDSetBlk(sd->device_id, SECTOR_SIZE);
	return SP_TRUE;
}

/**
* @brief 	SD card setup function.
* @param 	sd[in]: Card information.
* @param	sys_apb[in]: System apb clock (unit:100k).
* @return 	SUCCESS/ERROR_ID.
*/
static bool gp_sdcard_sd_setup(gpSDInfo_t* sd, unsigned int sys_apb)
{
	unsigned int response[4] = {0};
	unsigned int buf[2]={0};
	unsigned char TRANS_SPEED;
	unsigned int c_size, mult, blocklen;
	/* ----- Set to Transfer State ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD7, sd->RCA, response, 4)==SP_FALSE)
		return SP_FALSE;
	/* ----- Short block size to 8 bytes ----- */
	gpHalSDSetBlk(sd->device_id, 8);
	/* ----- ACMD 51 ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD55, sd->RCA, response, 4)==SP_FALSE)
		return SP_FALSE;
	if(gpHalSDSendCmd(sd->device_id, SD_ACMD51, sd->RCA, response,4) == SP_FALSE)
		return SP_FALSE;
	if(gpHalSDDataTxRx(sd->device_id, (UINT8*)buf, 8, 0)== SP_FALSE)
			return SP_FALSE;
	/* ----- Switch function to high speed mode ----- */
	if((buf[0]&0x0F)>1)	
	{		
		gp_sdcard_switch(sd, 1);
	}
	/* ----- Set bus width ----- */
	if (buf[0] & 0x0400)
	{
		/* ----- Set Bus width to 4 bits ----- */
		if(gpHalSDSendCmd(sd->device_id, SD_CMD55, sd->RCA, response, 4)==SP_FALSE)
			return SP_FALSE;
		if(gpHalSDSendCmd(sd->device_id, SD_ACMD6, 0x00000002, response,4)==SP_FALSE)
			return SP_FALSE;
		gpHalSDSetBus(sd->device_id,1);
	}
	/* ----- Set block size to 512 bytes ----- */
	gpHalSDSetBlk(sd->device_id, SECTOR_SIZE);	
	gpHalSDSendCmd(sd->device_id, SD_CMD7_DIS, 0x00, response, 4);
	/* ----- CMD 9 Read CSD Register -----  */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD9, sd->RCA, response, 16) == SP_FALSE)
		return SP_FALSE;
	TRANS_SPEED = response[0];
	/* ----- Calculate Totoal Memory Size ----- */
	if(response[0]&0x40000000)					
	{
		c_size = ((response[2]&0xFFFF0000)>>16)+((response[1]&0x0000003F)<<16);
		sd->capacity = (c_size+1)<<10;
	}
	else
	{
		blocklen = ((response[1] & 0x000F0000)>>16)-8;
		c_size = ((response[1] & 0x000003FF)<<2) + (response[2]>>30);
		mult = ((response[2] & 0x00030000)>>15) + ((response[2] & 0x0000FFFF)>>15);
		sd->capacity = (blocklen*(c_size+1))<<(mult + 2);
	}
	/* ----- Calculate Data Transfer Rate -----	*/
	sd->speed = gp_sdcard_despeed(TRANS_SPEED);
	gpHalSDSetClk(sd->device_id, sys_apb, sd->speed);
	/* ----- Set to Transfer State ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD7, sd->RCA, response, 4)==SP_FALSE)
		return SP_FALSE;
	/* ----- Check SD status ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD13, sd->RCA, response, 4)==SP_FALSE)
		return SP_FALSE;
	if (response[0] != 0x0900) 
		return SP_FALSE;
	return SP_TRUE;
}

/**
* @brief 	MMC card setup function.
* @param 	sd[in]: Card information.
* @param	sys_apb[in]: System apb clock (unit:100k).
* @return 	SUCCESS/ERROR_ID.
*/
static bool gp_sdcard_mmc_setup(gpSDInfo_t* sd, unsigned short sys_apb)
{
	unsigned int response = 0;
	unsigned int CSD[4] = {0};
	unsigned int buf[128]={0};
	unsigned char SPEC_VAR;
	unsigned char TRANS_SPEED;
	unsigned int c_size, mult, blocklen;	
	/* ----- CMD 9 Read CSD Register ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD9, sd->RCA, CSD, 16) == SP_FALSE)
		return SP_FALSE;
	if(gpHalSDSendCmd(sd->device_id, SD_CMD7, sd->RCA, &response, 4)==SP_FALSE)
		return SP_FALSE;
	SPEC_VAR = (CSD[0]&0x3C000000)>>26;
	TRANS_SPEED = CSD[0];
	gpHalSDSetClk(sd->device_id, sys_apb, 200);
	/* ----- Only spec. 4.0 or latter can support SWITCH function ----- */
	if(SPEC_VAR>=4)
	{
		/* ----- Setup MMC bus width 4 bit----- */
		if(gpHalSDSendCmd(sd->device_id, MMC_CMD6, 0x03b70100, &response, 4) == SP_FALSE)
			return SP_FALSE;
		if((response&0x08)==0)
			gpHalSDSetBus(sd->device_id, 1);
		/* ----- Setup MMC to high speed mode ----- */
		if(gpHalSDSendCmd(sd->device_id, MMC_CMD6, 0x03b90100, &response, 4) == SP_FALSE)
			return SP_FALSE;
		if((response&0x08)==0)
		{
			TRANS_SPEED = 0x5A;	
		}
	}
	/* ----- Calculate Totoal Memory Size ----- */
	if(sd->cap_type == SD_HighCapacity)					
	{
		if(gpHalSDSendCmd(sd->device_id, MMC_CMD8, 0x00, &response, 4) == SP_FALSE)
			return SP_FALSE;
		if(gpHalSDDataTxRx(sd->device_id, (UINT8*)buf, SECTOR_SIZE, 0)== SP_FALSE)
			return SP_FALSE;
		sd->capacity = buf[53];
	}
	else
	{
		blocklen = ((CSD[1] & 0x000F0000)>>16)-8;
		c_size = ((CSD[1] & 0x000003FF)<<2) + (CSD[2]>>30);
		mult = ((CSD[2] & 0x00030000)>>15) + ((CSD[2] & 0x0000FFFF)>>15);
		sd->capacity = (blocklen*(c_size+1))<<(mult + 2);
	}
	/* ----- Set MMC clock ----- */ 
	//if(TRANS_SPEED==0x5A)
	//	sd->speed = 520;	/* 52MHz */
	//else
	sd->speed = gp_sdcard_despeed(TRANS_SPEED);
	gpHalSDSetClk(sd->device_id, sys_apb, sd->speed);
	/* ----- Check MMC status ----- */
	if(gpHalSDSendCmd(sd->device_id, SD_CMD13, sd->RCA, &response, 4)==SP_FALSE)
		return SP_FALSE;
	if (response!= 0x0900) 
		return SP_FALSE;
	return SP_TRUE;
}

/**
* @brief 	SD card initial function.
* @param 	sd[in]: Card information.
* @return 	SUCCESS/ERROR_ID.
*/
int gp_sdcard_carduninit (gpSDInfo_t* sd)
{
	gpHalSDUninit(sd->device_id);
	gpHalScuClkEnable(0x040000<<sd->device_id, SCU_C, 0);
	return 0;
}

/**
* @brief 	SD card initial function.
* @param 	sd[in]: Card information.
* @return 	SUCCESS/ERROR_ID.
*/
int gp_sdcard_cardinit (gpSDInfo_t* sd)
{
	unsigned long sys_apb_clk = 0;
	struct clk *pclk;
	
	/* ----- Get sys_apb clock ----- */	
	pclk = clk_get(NULL,"clk_sys_apb");

	if(pclk)
	{
		sys_apb_clk = clk_get_rate(pclk)/100000;	
		clk_put(pclk);
	}
	else
	{
		sys_apb_clk = 270;
	}
	sd->RCA = 0;
	sd->cap_type = SD_StandardCapacity;	
	sd->capacity = 0;
	//sd->present = 0;	

	/* ----- Stop (reset) SD controller ----- */
	if(gpHalSDStop(sd->device_id)==SP_FALSE)
		goto init_error;
	/* ----- Set bus width ----- */	
	gpHalSDSetBus(sd->device_id,0);
	/* ----- Set Initial clock 300KHz ----- */ 
	gpHalSDSetClk(sd->device_id, sys_apb_clk, IDENT_CLOCK);
	/*----- Set block size -----*/
	gpHalSDSetBlk(sd->device_id, SECTOR_SIZE);
	sd->card_type = gp_sdcard_idmode(sd);

	if (sd->card_type == SDIO)
	{
		DEBUG("\nSDIO card detect!");
	}
	/* ----- Setup SD card ----- */
	else if(sd->card_type == SDCARD)
	{
		if(gp_sdcard_sd_setup(sd, sys_apb_clk)==SP_FALSE)
			goto init_error;	
		DEBUG("SD card Information:\n");
		DEBUG("Clock: %3d MHz\n",sd->speed/10);
		DEBUG("Capacity: %d MB\n",sd->capacity/2048);
	}
	/* ----- Setup MMC card ----- */
	else if(sd->card_type == MMCCARD)
	{
		if(gp_sdcard_mmc_setup(sd, sys_apb_clk)==SP_FALSE)
			goto init_error;
		DEBUG("MMC card Information:\n");
		DEBUG("Clock: %3d MHz\n", sd->speed/10);
		DEBUG("Capacity: %d MB\n", sd->capacity/2048);
	}
	else
		goto init_error;
	sd->present = 1; 

	return 0;
init_error:
	return -EIO;
}

