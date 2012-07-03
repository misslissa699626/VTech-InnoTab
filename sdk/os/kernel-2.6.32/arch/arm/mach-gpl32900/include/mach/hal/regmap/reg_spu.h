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
 * @file    reg_spu.h
 * @brief   Regmap of GP12 spu
 * @author  
 * @since   2010/10/19
 * @date    2010/10/19
 */
#ifndef _REG_SPU_H_
#define _REG_SPU_H_

#include <mach/hardware.h>
#include <mach/typedef.h>

#define	SPU_BASE_Addr		0xfc80b000//0x9300b000
#define	SPU_BASE_REG		(SPU_BASE_Addr + 0x0E00)
#define	SPU_BASE_Att		(SPU_BASE_Addr + 0x1000)
#define	SPU_BASE_Phase		(SPU_BASE_Addr + 0x1800)

#define IIS_BASE_Addr		0xfc812000
#define DAC_BASE_Addr		0xfc81f020


/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
/* volatile UINT8 regOffset[0xb000]; */ /* 0x9300B000 */
typedef struct spuReg_s {
	
	volatile UINT32 spuChEn;        		/* P_SPU_CH_EN, 0x9300BE00 */
	volatile UINT32 spuMainVol;       	/* P_SPU_MAIN_VOLUME, 0x9300BE04 */
	volatile UINT32 spuChFiqEn;        	/* P_SPU_CH_FIQ_EN, 0x9300BE08 */
	volatile UINT32 spuChFiqSts;    		/* P_SPU_CH_FIQ_STATUS, 0x9300BE0C */
	volatile UINT32 spuBeatBaseCnt;    	/* P_SPU_BEAT_BASE_COUNTER, 0x9300BE10 */
	volatile UINT32 spuBeatCnt;        	/* P_SPU_BEAT_COUNTER, 0x9300BE14 */
	volatile UINT32 spuEnvClkCh03;      /* P_SPU_ENV_CLK_CH0_3, 0x9300BE18 */
	volatile UINT32 spuEnvClkCh47;     	/* P_SPU_ENV_CLK_CH4_7, 0x9300BE1C */
	volatile UINT32 spuEnvClkCh811;     /* P_SPU_ENV_CLK_CH8_11, 0x9300BE20 */
	volatile UINT32 spuEnvClkCh1215;    /* P_SPU_ENV_CLK_CH12_15, 0x9300BE24 */
	volatile UINT32 spuEnvRampD;        /* P_SPU_ENV_RAMP_DOWN, 0x9300BE28 */
	volatile UINT32 spuChStopSts;       /* P_SPU_CH_STOP_STATUS, 0x9300BE2C */
	volatile UINT32 spuChZcEn;    			/* P_SPU_CH_ZC_ENABLE, 0x9300BE30 */
	volatile UINT32 spuCtrlFlag;		    /* P_SPU_CONTROL_FLAG, 0x9300BE34 */
	volatile UINT32 spuCompressCtrl;    /* P_SPU_COMPRESSOR_CONTROL, 0x9300BE38 */
	volatile UINT32 spuChSts;          	/* P_SPU_CH_STATUS, 0x9300BE3C */
	volatile UINT32 spuWavInL;         	/* P_SPU_WAVE_IN_LEFT, 0x9300BE40 */
	volatile UINT32 spuWavInR;         	/* P_SPU_WAVE_IN_RIGHT, 0x9300BE44 */
	volatile UINT32 spuWavOutL;        	/* P_SPU_WAVE_OUT_LEFT, 0x9300BE48 */
	volatile UINT32 spuWavOutR;			    /* P_SPU_WAVE_OUT_RIGHT, 0x9300BE4C */
	volatile UINT32 spuRepeatEn;    		/* P_SPU_CH_REPEAT_EN, 0x9300BE50 */
	volatile UINT32 spuChEnvMod;        /* P_SPU_CH_ENV_MODE, 0x9300BE54 */
	volatile UINT32 spuChToneRelease;   /* P_SPU_CH_TONE_RELEASE, 0x9300BE58 */
	volatile UINT32 spuChIrqSts;        /* P_SPU_CH_IRQ_STATUS, 0x9300BE5C */
	volatile UINT32 spuChPitchBendEn;   /* P_SPU_CH_PITCH_BEND_EN, 0x9300BE60 */
	volatile UINT32 spuOffset00;   			/* spuOffset00, 0x9300BE64 */
	volatile UINT32 spuAttackReleaseT;  /* P_SPU_ATTACK_RELEASE_TIME, 0x9300BE68 */
	volatile UINT32 spuOffset01[4];   			/* spuOffset01, 0x9300BE6C~0x9300BE78 */
	volatile UINT32 spuBankAddr;        /* P_SPU_BENK_ADDR, 0x9300BE7C */
	
	volatile UINT32 spuChEnH;        		/* P_SPU_CH_EN_HI, 0x9300BE80 */
	volatile UINT32 spuOffset02;   			/* spuOffset02, 0x9300BE84 */
	volatile UINT32 spuChFiqEnH;        /* P_SPU_CH_FIQ_EN_HI, 0x9300BE88 */
	volatile UINT32 spuChFiqStsH;    		/* P_SPU_CH_FIQ_STATUS_HI, 0x9300BE8C */
	volatile UINT32 spuOffset03;   			/* spuOffset03, 0x9300BE90 */
	volatile UINT32 spuPostWavCtrl;    	/* P_SPU_POST_WAVE_CONTROL, 0x9300BE94 */
	volatile UINT32 spuEnvClkCh1619;    /* P_SPU_ENV_CLK_CH16_19, 0x9300BE98 */
	volatile UINT32 spuEnvClkCh2023;    /* P_SPU_ENV_CLK_CH20_23, 0x9300BE9C */
	volatile UINT32 spuEnvClkCh2427;    /* P_SPU_ENV_CLK_CH24_27, 0x9300BEA0 */
	volatile UINT32 spuEnvClkCh2831;    /* P_SPU_ENV_CLK_CH28_31, 0x9300BEA4 */
	volatile UINT32 spuEnvRampDH;   		/* P_SPU_ENV_RAMP_DOWN_HI, 0x9300BEA8 */
	volatile UINT32 spuChStopStsH;      /* P_SPU_CH_STOP_STATUS_HI, 0x9300BEAC */
	volatile UINT32 spuChZcEnH;        	/* P_SPU_CH_ZC_ENABLE_HI, 0x9300BEB0 */
	volatile UINT32 spuOffset04[2];   	/* spuOffset04, 0x9300BEB4~ 0x9300BEB8~*/
	volatile UINT32 spuChStsH;    			/* P_SPU_CH_STATUS_HI, 0x9300BEBC */
	volatile UINT32 spuOffset05[2];   	/* spuOffset04, 0x9300BEC0~ 0x9300BEC4~*/
	volatile UINT32 spuPostWavOutL;		  /* P_SPU_POST_WAVE_OUT_LEFT, 0x9300BEC8 */
	volatile UINT32 spuPostWavOutR;    	/* P_SPU_POST_WAVE_OUT_RIGHT, 0x9300BECC */
	volatile UINT32 spuRepeatEnH;      	/* P_SPU_CH_REPEAT_EN_HI, 0x9300BED0 */
	volatile UINT32 spuChEnvModH;       /* P_SPU_CH_ENV_MODE_HI, 0x9300BED4 */
	volatile UINT32 spuChToneReleaseH; 	/* P_SPU_CH_TONE_RELEASE_HI, 0x9300BED8 */
	volatile UINT32 spuChIrqStsH;       /* P_SPU_CH_IRQ_STATUS_HI, 0x9300BEDC */
	volatile UINT32 spuChPitchBendEnH;	/* P_SPU_CH_PITCH_BEND_EN_HI, 0x9300BEE0 */	
} spuReg_t;


typedef struct spuAtt_s {
	
	volatile UINT32 spuWavAddr;        	/* P_SPU_WAVE_ADDR, 0x9300C000 */
	volatile UINT32 spuMode;       			/* P_SPU_MODE, 0x9300C004 */
	volatile UINT32 spuLoopAddr;        /* P_SPU_LOOP_ADDR, 0x9300C008 */
	volatile UINT32 spuPanVol;    			/* P_SPU_PAN_Velo, 0x9300C00C */
	volatile UINT32 spuEnv0;    				/* P_SPU_ENVELOPE_0, 0x9300C010 */
	volatile UINT32 spuEnvData;        	/* P_SPU_ENVELOPE_DATA, 0x9300C014 */
	volatile UINT32 spuEnv1;      			/* P_SPU_ENVELOPE_1, 0x9300C018 */
	volatile UINT32 spuEnvAddrH;     		/* P_SPU_ENV_ADDR_HI, 0x9300C01C */
	volatile UINT32 spuEnvAddrL;     		/* P_SPU_ENV_ADDR_LOW, 0x9300C020 */
	volatile UINT32 spuWavDat0;    			/* P_SPU_WAVE_DATA_0, 0x9300C024 */
	volatile UINT32 spuLoopCtrl;        /* P_SPU_LOOP_CONTROL, 0x9300C028 */
	volatile UINT32 spuWavData;       	/* P_SPU_WAVE_DATA, 0x9300C02C */
	volatile UINT32 spuOffset06;    		/* spuOffset06, 0x9300C030 */
	volatile UINT32 spuAdpcmSel;    		/* P_SPU_ADPCM_SEL, 0x9300C034 */
	volatile UINT32 spuWlAddrH;		    	/* P_SPU_WL_ADDR_HI1, 0x9300C038 */
	volatile UINT32 spuEnvAddrH1;    		/* P_SPU_ENV_ADDR_HI1, 0x9300C03C */
			
} spuAtt_t;

typedef struct spuPhase_s {
	
	volatile UINT32 spuPhaseH;        	/* P_SPU_PHASE_HIGH, 0x9300C800 */
	volatile UINT32 spuPhaseAccH;       /* P_SPU_PHASE_ACC_HIGH, 0x9300C804 */
	volatile UINT32 spuTarPhaseH;       /* P_SPU_TARGET_PHASE_HIGH, 0x9300C808 */
	volatile UINT32 spuRampDownClk;    	/* P_SPU_RAMP_DOWN_CLK, 0x9300C80C */
	volatile UINT32 spuPhase;    				/* P_SPU_PHASE, 0x9300C810 */
	volatile UINT32 spuPhaseAcc;        /* P_SPU_PHASE_ACC, 0x9300C814 */
	volatile UINT32 spuTarPhase;      	/* P_SPU_TARGET_PHASE, 0x9300C818 */
	volatile UINT32 spuPhaseCtrl;     	/* P_SPU_PHASE_CONTROL, 0x9300C81C */
			
} spuPhase_t;





/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
typedef struct iisReg_s {
	
	volatile UINT32 iisCtrl;        		/* P_SPU_CH_EN, 0x93012000 */
	volatile UINT32 iisIsdr;        		/* P_SPU_CH_EN, 0x93012004 */
} iisReg_t;

typedef struct dacReg_s {
	
	volatile UINT32 dacPwrCtrl;        		/* P_SPU_CH_EN, 0x9301f020 */
	volatile UINT32 dacLineInCtrl;        /* P_SPU_CH_EN, 0x9301f024 */
	volatile UINT32 adcCtrl;        			/* P_SPU_CH_EN, 0x9301f028 */
	volatile UINT32 dacCtrl;	        		/* P_SPU_CH_EN, 0x9301f02c */
	volatile UINT32 dacAutoSlp;	        		/* P_SPU_CH_EN, 0x9301f030 */
	volatile UINT32 dacLinOut;	        		/* P_SPU_CH_EN, 0x9301f034 */
	volatile UINT32 dacHdpHn;	        		/* P_SPU_CH_EN, 0x9301f038 */
		
} dacReg_t;


/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif /* _REG_SPU_H_ */
