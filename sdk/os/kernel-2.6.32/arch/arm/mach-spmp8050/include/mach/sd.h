/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2002 by Sunplus Technology Co., Ltd.             *
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
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *  Author:                                                               *
 *                                                                        *
 **************************************************************************/
#ifndef _SD_H_
#define _SD_H_

#include <mach/general.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
typedef void (*dma_callback)(int, void *);
typedef void (*detect_callback)(int, void *);
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
//Process Status Type 
//===================================================================================
#define SD_MMC_S_BUSY			0x0001
#define SD_MMC_S_CARDBUSY		0x0002
#define SD_MMC_S_CMDCOM		    0x0004
#define SD_MMC_S_DATCOM		    0x0008
#define SD_MMC_S_RSP_IDXERR	    0x0010
#define SD_MMC_S_RSP_CRCERR	    0x0020
#define SD_MMC_S_CMDBUFFULL	    0x0040
#define SD_MMC_S_DATABUFFULL	0x0080
#define SD_MMC_S_DATABUFEMPTY	0x0100
#define SD_MMC_S_RSP_TIMEOUT	0x0200
#define SD_MMC_S_DATA_CRCERR	0x0400
#define SD_MMC_S_CARDWP		    0x0800
#define SD_MMC_S_CARDPRE		0x1000
#define SD_MMC_S_CARDINT		0x2000
#define SD_MMC_S_CEATACMDCOM	0x4000
#define SD_MMC_S_CLRALL  	    0xFFFF


//Enable Interrupt Type 
#define SD_MMC_INT_CMDCOM       0x0001
#define SD_MMC_INT_DATACOM      0x0002
#define SD_MMC_INT_CMDBUFFULL   0x0004
#define SD_MMC_INT_DATABUFFULL  0x0008
#define SD_MMC_INT_DATABUFEMPTY 0x0010
#define SD_MMC_INT_CARDDETECT   0x0020
#define SD_MMC_INT_SDIODETECT   0x0040
#define SD_MMC_INT_ALLMASK      0x007F
#define SD_MMC_INT_DISABLE      0
#define SD_MMC_INT_ENABLE       1

//command Response Type
#define SD_MMC_RSP_NONE  0x0000
#define SD_MMC_RSP_R1    0x0001
#define SD_MMC_RSP_R2    0x0002
#define SD_MMC_RSP_R3    0x0003
#define SD_MMC_RSP_R4    0x0004
#define SD_MMC_RSP_R5    0x0005
#define SD_MMC_RSP_R6    0x0006
#define SD_MMC_RSP_R7    0x0007
#define SD_MMC_RSP_R1B   0x0008
#define SD_MMC_RSP_MASK  0xF
#define SD_MMC_RSP_MASK_SHIFT  0

#define SD_MMC_CMD_WITHOUT_DATA    0x0000
#define SD_MMC_CMD_WITH_DATA       0x0010
#define SD_MMC_CMD_DATA_MASK       0x1
#define SD_MMC_CMD_DATA_MASK_SHIFT 4

#define SD_MMC_TRANS_READ        0x0000
#define SD_MMC_TRANS_WRITE       0x0020
#define SD_MMC_TRANS_MASK        0x1
#define SD_MMC_TRANS_MASK_SHIFT  5

#define SD_MMC_MBLK_SINGLE      0x0000
#define SD_MMC_MBLK_MULTI       0x0040
#define SD_MMC_MBLK_MASK        0x1
#define SD_MMC_MBLK_MASK_SHIFT  6

#define SD_DATA_FIFO 0x0
#define SD_DATA_DMA  0x1

#define SD_DATA_READ   0x0
#define SD_DATA_WRITE  0x2

#define SD_MMC_BUS_WIDTH_1		1
#define SD_MMC_BUS_WIDTH_4		4
#define SD_MMC_BUS_WIDTH_8		8

#define SD_MMC_RSP_PRESENT (1 << 0)
#define SD_MMC_RSP_136     (1 << 1)

typedef struct sd_info_s{
	UINT32 device_id;
	UINT32 p_addr;
	UINT32 v_addr;
	SINT32 dma_chan;
	UINT32 max_dma_size;
	BOOL   is_irq;	
	BOOL   is_dmairq;
	BOOL   is_detectirq;
	BOOL   is_readonly;
	BOOL   inserted;	
	UINT32	clk_rate;
	UINT32	clk_div;
	UINT32	max_clkdiv;	
	UINT32  real_rate;
	void    *cli_dmadata;
	void    *cli_detdata;	
	dma_callback    dma_cb;
	UINT32  detect_chan;
	UINT32  detect_delay;
	detect_callback detect_cb;		
}sd_info_t;

typedef struct sd_ops_s {	
	BOOL(*init)    (struct sd_info_s *sd_info,UINT32 p_addr,UINT32 v_addr);
	BOOL(*deInit)  (struct sd_info_s *sd_info);
	BOOL(*is_readonly)(struct sd_info_s *sd_info);
	BOOL(*is_Insert)(struct sd_info_s *sd_info);
	BOOL(*is_Dma)(struct sd_info_s *sd_info);	
	UINT32(*setClkFreq) (struct sd_info_s *sd_info, UINT32 sd_freq);
	UINT32(*setBusWidth)(struct sd_info_s *sd_info,	UINT32 busWidth);
	UINT32(*setBlkLen)  (struct sd_info_s *sd_info,	UINT32 blklen);		
	UINT32(*setDma)  (struct sd_info_s *sd_info,	dma_callback dma_cbk,void *pri_data);	
	UINT32(*setDetect)  (struct sd_info_s *sd_info,	detect_callback detect_cbk,void *pri_data);		
	UINT32(*sendTxCmd)  (struct sd_info_s *sd_info,	UINT32 cmdId, UINT32 arg, UINT32 cmdinfo);
	UINT32(*stopTxCmd)  (struct sd_info_s *sd_info);			
	UINT32(*transData)  (struct sd_info_s *sd_info,UINT32 sendInfo,void *addr,UINT32 len);
	UINT32(*getStatus)  (struct sd_info_s *sd_info);
	UINT32 (*clearStatus) (struct sd_info_s *sd_info,UINT32 status_info);
	UINT32(*intrpt_enable)(struct sd_info_s *sd_info,UINT32 intinfo,BOOL en);
	UINT32(*recvRxRsp)  (struct sd_info_s *sd_info);	
}sd_ops_t;


typedef struct sd_data_s {
	struct sd_info_s info;
	const struct sd_ops_s *ops;
}sd_data_t;

/**************************************************************************
 *                               M A C R O S                              *
 **************************************************************************/
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
extern const struct sd_ops_s spmpmci_ops0;
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
    BOOL  halSd_is_readonly(struct sd_info_s *sd_info);
	BOOL  halSd_is_Insert(struct sd_info_s *sd_info);
	BOOL  halSd_is_Dma(struct sd_info_s *sd_info);
	BOOL  halSd_Init(struct sd_info_s *sd_info,UINT32 p_addr,UINT32 v_addr);
	BOOL  halSd_DeInit(struct sd_info_s *sd_info);
	UINT32 halSd_SetClkFreq(struct sd_info_s *sd_info, UINT32 sd_freq);
	UINT32 halSd_SetBusWidth(struct sd_info_s *sd_info,	UINT32 busWidth);
	UINT32 halSd_SetBlkLen(struct sd_info_s *sd_info,	UINT32 blklen);
	UINT32 halSd_SetDma  (struct sd_info_s *sd_info,	dma_callback dma_cbk,void *pri_data);	
	UINT32 halSd_SetDetect (struct sd_info_s *sd_info,	detect_callback detect_cbk,void *pri_data);		
	UINT32 halSd_SendTxCmd(struct sd_info_s *sd_info,	UINT32 cmdId, UINT32 arg, UINT32 cmdinfo);
	UINT32 halSd_StopTxCmd(struct sd_info_s *sd_info);
	UINT32 halSd_TransData(struct sd_info_s *sd_info,UINT32 sendInfo,void *addr,UINT32 len);
	UINT32 halSd_GetStatus(struct sd_info_s *sd_info);
	UINT32 halSd_ClearStatus(struct sd_info_s *sd_info,UINT32 status_info);
	UINT32 halSd_intrpt_enable(struct sd_info_s *sd_info,UINT32 intinfo,BOOL en);
	UINT32 halSd_RecvRxRsp(struct sd_info_s *sd_info);

#endif /* _SD_H */
