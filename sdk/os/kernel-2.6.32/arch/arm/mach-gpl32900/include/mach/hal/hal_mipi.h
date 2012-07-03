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
#ifndef _HAL_MIPI_H_
#define _HAL_MIPI_H_

#include <mach/common.h>

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

/**
 * @brief   set pclk
 * @param   pclk_en[in]: pclk enable/disable
 * @param   pclk_div[in]: pclk divider set
 * @param   pclk_dly[in]: pclk delay set
 * @return  None
 */
void gpHalMipiSetPClk(UINT8 pclk_en,UINT8 pclk_div,UINT8 pclk_dly);

/**
 * @brief   mipi separate clock set
 * @param   mipi_pclk_en[in]: mipi separate clk out enable/disable
 * @param   mipi_pclk_div[in]: mipi separate clk out divider set
 * @param   mipi_pclk_src[in]: mipi separate clk source set, 0:SPLL clock, 1:USB 96MHz
 * @return  None
 */
void gpHalMipiSetSepPClk(UINT8 mipi_pclk_en,UINT8 mipi_pclk_div,UINT8 mipi_pclk_src);

/**
 * @brief   mipi+cdsp module clock set enable
 * @param   enable[in]: enable/disable
 * @return  None
 * @see
 */
void gpHalMipiSetModuleClk(UINT8 enable);

/**
 * @brief   mipi set global configure
 * @param   low_power_en[in]:  low power enable
 * @param   lane_num[in]: 1: mipi 2 lane, 0: mipi 1lane
 * @param   sel_pix_clk[in]: set ouput clock, 1: use generate clock, 0: use MIPI D_PHY BYTE CLK
 * @return  None
 * @see
 */
void gpHalMipiSetGloblaCfg(UINT8 mipi_enable, UINT8 low_power_en,UINT8 lane_num_sys,UINT8 sel_pix_clk);

/**
 * @brief   mipi set reset
 * @param   enable[in]: enable mipi reset
 */
void gpHalMipiReset(UINT8 enable);

/**
 * @brief   mipi set ecc order
 * @param   ecc_order[in]: ecc order set
 * @param   ecc_check_en[in]: ecc check enable
 * @param   da_mask_cnt[in]: 
 * @param   check_hs_seq[in]: 
 */
void gpHalMipiSetEcc(UINT8 ecc_order,UINT8 ecc_check_en,UINT8 da_mask_cnt,UINT8 check_hs_seq);

/**
 * @brief   mipi set ccir601 interface
 * @param   h_back_proch[in]: horizontal back proch
 * @param   h_front_proch[in]: horizontal front proch
 * @param   blanking_line_en[in]: blanking line enable, 0 mask hsync when vsync
 */
void gpHalMipiSetCCIR601IF(UINT8 h_back_porch,UINT8 h_front_porch,UINT8 blanking_line_en);

/**
 * @brief   mipi set image size
 * @param   type[in]: 0: raw, 1: yuv
 * @param   h_size[in]: horizontal size set
 * @param   v_size[in]: vertical size set
 */
void gpHalMipiSetImageSize(UINT8 type,UINT16 h_size,UINT16 v_size);

/**
 * @brief   mipi set data format
 * @param   data_from_mmr[in]: mmr decide data type enable
 * @param   data_type_mmr[in]: data format
 * @param   data_type_cdsp_sys[in]: 1: output to cdsp,
 */
void gpHalMipiSetDataFmt(UINT8 data_from_mmr,UINT8 data_type_mmr,UINT8 data_type_cdsp_sys);

#endif /* _HAL_MIPI_H_ */
