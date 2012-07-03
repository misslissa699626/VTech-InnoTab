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
#include <linux/io.h>
#include <mach/hal/regmap/reg_mipi.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_mipi.h>

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
static mipiReg_t *pMipiReg = (mipiReg_t *)LOGI_ADDR_MIPI_REG;
static scuaReg_t *pScuaReg = (scuaReg_t *)LOGI_ADDR_SCU_A_REG;

static scubReg_t *scubReg = (scubReg_t *)LOGI_ADDR_SCU_B_REG;

/**
 * @brief   mipi separate clock set
 * @param   mipi_pclk_en[in]: mipi separate clk out enable/disable
 * @param   mipi_pclk_div[in]: mipi separate clk out divider set
 * @param   mipi_pclk_src[in]: mipi separate clk source set, 0:SPLL clock, 1:USB 96MHz
 * @return  None
 */
void 
gpHalMipiSetSepPClk(
	UINT8 mipi_pclk_en,
	UINT8 mipi_pclk_div,
	UINT8 mipi_pclk_src /* 0:SPLL clock, 1:USB 96MHz */
)
{
	if(mipi_pclk_en)
	{
		mipi_pclk_en &= 0x1;
		mipi_pclk_src &= 0x1;
		pScuaReg->scuaMipi2ChPclk = 0x0;	
		pScuaReg->scuaMipi2ChPclk = mipi_pclk_div|((UINT32)mipi_pclk_en << 8)|((UINT32)mipi_pclk_src << 16);		
	}
	else
	{
		pScuaReg->scuaMipi2ChPclk = 0;
	}
}

/**
 * @brief   mipi+cdsp module clock set enable
 * @param   enable[in]: enable/disable
 * @return  None
 * @see
 */
void 
gpHalMipiSetModuleClk(
	UINT8 enable
)
{	
	if(enable)
	{
		scubReg->scubPinMux |=(1<<26);
		pScuaReg->scuaPeriClkEn |= (1<<28); 	/* enable mipi clock 0x93007004[28] */
		pScuaReg->scuaPeriRst |= (1<<28);		/* mipi module reset */
		pScuaReg->scuaPeriRst &= ~(1<<28);
	}
	else
		pScuaReg->scuaPeriClkEn &= ~(1<<28); 
}

/**
 * @brief   mipi set global configure
 * @param   low_power_en[in]:  low power enable
 * @param   lane_num[in]: 1: mipi 2 lane, 0: mipi 1lane
 * @param   sel_pix_clk[in]: set ouput clock, 1: use generate clock, 0: use MIPI D_PHY BYTE CLK
 * @return  None
 * @see
 */
void
gpHalMipiSetGloblaCfg(
	UINT8 mipi_en,
	UINT8 low_power_en,
	UINT8 lane_num_sys,
	UINT8 sel_pix_clk
)
{
	mipi_en &= 0x1;
	low_power_en &= 0x1;
	lane_num_sys &= 0x1;
	sel_pix_clk &= 0x1;
	pMipiReg->mipiGlbCsr = mipi_en|(low_power_en << 4)|((UINT32)lane_num_sys << 8)|((UINT32)sel_pix_clk << 12); 
}

/**
 * @brief   mipi set reset
 * @param   enable[in]: enable mipi reset
 */
void
gpHalMipiReset(
	UINT8 enable
)
{
	enable &= 0x1;
	pMipiReg->mipiPhyRst = enable << 4;
}

/**
 * @brief   mipi set ecc order
 * @param   ecc_order[in]: ecc order set
 * @param   ecc_check_en[in]: ecc check enable
 * @param   da_mask_cnt[in]: 
 * @param   check_hs_seq[in]: 
 */
void
gpHalMipiSetEcc(
	UINT8 ecc_order,
	UINT8 ecc_check_en,
	UINT8 da_mask_cnt,
	UINT8 check_hs_seq
)
{
	ecc_order &= 0x3;
	ecc_check_en &= 0x1;
	da_mask_cnt &= 0xFF;
	check_hs_seq &= 0x1;
	pMipiReg->mipiEccOrder= ecc_order|(ecc_check_en << 2) |((UINT32)da_mask_cnt << 8)|((UINT32)check_hs_seq << 16);
}

/**
 * @brief   mipi set ccir601 interface
 * @param   h_back_proch[in]: horizontal back proch
 * @param   h_front_proch[in]: horizontal front proch
 * @param   blanking_line_en[in]: blanking line enable, 0 mask hsync when vsync
 */
void
gpHalMipiSetCCIR601IF(
	UINT8 h_back_porch,
	UINT8 h_front_porch,
	UINT8 blanking_line_en
)
{
	h_back_porch &= 0xF;
	h_front_porch &= 0xF;
	blanking_line_en &= 0x1;
	pMipiReg->mipiCCIR601Timing = h_back_porch|((UINT32)h_front_porch << 8)|((UINT32)blanking_line_en << 16);
}

/**
 * @brief   mipi set image size
 * @param   type[in]: 0: raw, 1: yuv
 * @param   h_size[in]: horizontal size set
 * @param   v_size[in]: vertical size set
 */
void
gpHalMipiSetImageSize(
	UINT8 type,
	UINT16 h_size,
	UINT16 v_size
)
{
	if(h_size < 1) h_size = 1;

	h_size &= 0xFFFF;
	v_size &= 0xFFFF;
	pMipiReg->mipiImgSize = h_size|((UINT32)v_size << 16);
	if(type == 1)
		pScuaReg->scuaSysSel |= (1<<9);
	else
		pScuaReg->scuaSysSel &= ~(1<<9);
}

/**
 * @brief   mipi set data format
 * @param   data_from_mmr[in]: mmr decide data type enable
 * @param   data_type_mmr[in]: data format
 * @param   data_type_cdsp_sys[in]: 1: output to cdsp,
 */
void
gpHalMipiSetDataFmt(
	UINT8 data_from_mmr,
	UINT8 data_type_mmr,
	UINT8 data_type_cdsp_sys
)
{
	data_from_mmr &= 0x1;
	data_type_mmr &= 0x7;
	data_type_cdsp_sys &= 0x1;
	pMipiReg->mipiDataFmt = data_from_mmr|(data_type_mmr << 4)|(data_type_cdsp_sys << 7);

	if(data_type_cdsp_sys) {
		/* CDSP */
		pScuaReg->scuaSysSel |= (1<<12);
		pScuaReg->scuaSysSel &= ~(1<<13); 
	} else {
		/* CSI1 */
		pScuaReg->scuaSysSel &= ~(1<<12);
		pScuaReg->scuaSysSel |= (1<<13); 
	}
}
