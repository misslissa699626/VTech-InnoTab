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
#ifndef _REG_MIPI_H_
#define _REG_MIPI_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define LOGI_ADDR_MIPI_REG      IO3_ADDRESS(0xD000)

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
typedef struct mipiReg_s 
{
	volatile UINT32 mipiGlbCsr;			/* R_MIPI_GLB_CSR, 0x9300D000 */
	volatile UINT32 mipiPhyRst;			/* R_MIPI_PHY_RST, 0x9300D004 */
	volatile UINT32 mipiRcCtrl;			/* R_MIPI_RC_CTRL, 0x9300D008 */
	volatile UINT32 mipiEccOrder;		/* R_MIPI_ECC_ORDER, 0x9300D00C */
	volatile UINT32 mipiCCIR601Timing;	/* R_MIPI_CCIR601_TIMING, 0x9300D010 */
	volatile UINT32 mipiImgSize;		/* R_MIPI_IMG_SIZE, 0x9300D014 */
	volatile UINT32 reserved0;			/* reserved, 0x9300D018 */
	volatile UINT32 reserved1;			/* reserved, 0x9300D01C */
	volatile UINT32 mipiDataFmt;		/* R_MIPI_DATA_FMT, 0x9300D020 */
	volatile UINT32 mipiPayloadHeader;	/* R_MIPI_PAYLOAD_HEADER, 0x9300D024 */
	volatile UINT32 mipiCtrlState;		/* R_MIPI_CTRL_STATE, 0x9300D028 */
	volatile UINT32 mipiClkChk;			/* R_MIPI_VLK_CHECK, 0x9300D02C */
	volatile UINT32 mipiHeaderData;		/* R_MIPI_HEADER_DATA, 0x9300D030 */
	volatile UINT32 mipiHeaderDtVld;	/* R_MIPI_HEADER_DT_VLD, 0x9300D034 */
	volatile UINT32 reserved2;			/* reserved, 0x9300D038 */
	volatile UINT32 reserved3;			/* reserved, 0x9300D03C */
	volatile UINT32 mipiIntEnable;		/* R_MIPI_INTERRUPT_ENABLE, 0x9300D040 */
	volatile UINT32 reserved4[0x10];	/* reserved, 0x9300D044 */
	volatile UINT32 mipiIntSource;		/* R_MIPI_INTERRUPT_SOURCE, 0x9300D080 */
	volatile UINT32 reserved5[0x10];	/* reserved, 0x9300D084 */
	volatile UINT32 mipiIntFlag;		/* R_MIPI_INTERRUPT_FLAG, 0x9300D0C0 */
}mipiReg_t;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_MIPI_H_ */



