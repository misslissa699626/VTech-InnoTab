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
 * @file    hal_ppu.h
 * @brief   Implement of PPU HAL API header file.
 * @author  Cater Chen
 * @since   2010-10-27
 * @date    2010-10-27
 */

#ifndef HAL_SPU_H
#define HAL_SPU_H

/**************************************************************************
 *                         H E A D E R   F I L E S                        *
 **************************************************************************/
#include <mach/typedef.h>
//#include <mach/gp_spu.h>

/**************************************************************************
*                           C O N S T A N T S                             *
 **************************************************************************/

/**************************************************************************
*                          D A T A    T Y P E S
**************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define SPU_BIT_VALUE_SET(__BIT_N__,__VALUE_SET__)  \
(((UINT32)(__VALUE_SET__))<<(__BIT_N__)) 
/*******************************************************************************
*               F U N C T I O N    D E C L A R A T I O N S
*******************************************************************************/

/******************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 *****************************************************************************/

extern void gpHalSpuInit (void);
//extern void gpHalSpuPlayTone(unsigned char uiPitch, unsigned int *pAddr, unsigned char uiPan, unsigned char uiVelocity, unsigned char uiSPUChannel);
//extern void gpHalSpuPlayTone(SPU_MOUDLE_STRUCT *spu_register_set);
extern void gpHalSpuClkEnable(void);

extern void gpHalSPU_EnableChannelZC(UINT8 ChannelIndex);
extern void gpHalSPU_Enable_Channel(UINT8 ChannelIndex);
extern void gpHalSPU_Clear_Ch_StopFlag(UINT8 ChannelIndex);
extern UINT8 gpHalSPU_Get_SingleChannel_Status(UINT8 SPU_Channel);
extern void gpHalSPU_Disable_Channel(UINT8 ChannelIndex);

//to play midi
extern void gpHalSPU_Set_BeatBaseCounter(UINT16 BeatBaseCounter);
extern void gpHalSPU_Set_BeatCounter(UINT16 BeatCounter);
extern UINT16 gpHalSPU_Get_BeatCounter(void);
extern void gpHalSPU_Clear_BeatIRQ_Flag(void);
extern void gpHalSPU_Enable_BeatIRQ(void);	
extern UINT8 gpHalSPU_Get_BeatIRQ_Enable_Flag(void);



extern void gpHalSPU_SetEnvelope_0(UINT16 Envelope_0, UINT8 ChannelIndex);
extern void gpHalSPU_SetEnvelope_1(UINT16 Envelope_1, UINT8 ChannelIndex);

extern void gpHalSPU_SetVelocity(UINT8 VelocityValue, UINT8 ChannelIndex);
extern void gpHalSPU_SetPan(UINT8 PanValue, UINT8 ChannelIndex);
extern void gpHalSPU_SetWaveData_0(UINT16 WDD_0, UINT8 ChannelIndex);
extern void gpHalSPU_SetWaveData(UINT16 WDD, UINT8 ChannelIndex);
extern void gpHalSPU_SetLoopAddress(UINT32 LoopAddr, UINT8 ChannelIndex);
extern void gpHalSPU_Set_two_ch_LoopAddress(UINT32 LoopAddr0, UINT32 LoopAddr1, UINT8 spu_ch0, UINT8 spu_ch1);//20110430
extern void gpHalSPU_SetToneColorMode(UINT8 ToneColorMode, UINT8 ChannelIndex);
extern void gpHalSPU_Set_8bit_Mode(UINT8 ChannelIndex);
extern void gpHalSPU_Set_16bit_Mode(UINT8 ChannelIndex);
extern void gpHalSPU_SetADPCM_Mode(UINT8 ChannelIndex);
extern void gpHalSPU_SetPCM_Mode(UINT8 ChannelIndex);
extern void gpHalSPU_SelectADPCM_Mode(UINT8 ChannelIndex);
extern void gpHal_SPU_ClearADCPM36_Mode(UINT8 ChannelIndex);
extern void gpHalSPU_SelectADPCM36_Mode(UINT8 ChannelIndex);
extern void gpHalSPU_SetEnvelopeAddress(UINT32 EnvelopeAddr, UINT8 ChannelIndex);
extern void gpHalSPU_SetADPCM_PointNumber(UINT8 PointNumber, UINT8 ChannelIndex);
extern void gpHalSPU_SetStartAddress(UINT32 StartAddr, UINT8 ChannelIndex);
extern void gpHalSPU_SetEnvelopeData(UINT8 EnvData, UINT8 ChannelIndex);
extern void gpHalSPU_SetEnvelopeRampDownOffset(UINT8 RampDownOffset, UINT8 ChannelIndex);
extern void gpHalSPU_SetEnvelopeRepeatAddrOffset(UINT16 EAOffset, UINT8 ChannelIndex);
extern void gpHalSPU_SetPhase(UINT32 Phase, UINT8 ChannelIndex);
extern void gpHalSPU_Set_two_channel_Phase(UINT32 Phase0, UINT8 ChannelIndex0, UINT32 Phase1, UINT8 ChannelIndex1);
extern UINT32 gpHalSPU_ReadPhase(UINT8 ChannelIndex);
extern void gpHalSPU_SetPhaseAccumulator(UINT32 PhaseAcc, UINT8 ChannelIndex);
extern void gpHalSPU_SetTargetPhase(UINT32 TargetPhase, UINT8 ChannelIndex);
extern void gpHalSPU_SetPhaseOffset(UINT16 PhaseOffset, UINT8 ChannelIndex);
extern void gpHalSPU_SetPhaseIncrease(UINT8 ChannelIndex);
extern void gpHalSPU_SetPhaseDecrease(UINT8 ChannelIndex);
extern void gpHalSPU_SetPhaseTimeStep(UINT8 PhaseTimeStep, UINT8 ChannelIndex);
extern void gpHalSPU_SetEnvelopeCounter(UINT8 EnvCounter, UINT8 ChannelIndex);
extern void gpHalSPU_SetRampDownClock(UINT8 RampDownClock, UINT8 ChannelIndex);
extern void gpHalSPU_EnvelopeAutoMode(UINT8 ChannelIndex);
extern void gpHalSPU_EnvelopeManualMode(UINT8 ChannelIndex);
extern void gpHalSPU_Enable_MultiChannel(UINT32 ChannelBit);
extern void gpHalSPU_Clear_MultiCh_StopFlag(UINT32 ChannelBit);

extern UINT32 gpHalSPU_GetChannelStatus(void);
extern void gpHalSPU_Set_EnvRampDown(UINT8 ChannelIndex);

extern void gpHalSPU_Disable_BeatIRQ(void);
extern SINT16 gpHalSPUIsr(void);
extern UINT32 gpHalSPU_Get_EnvRampDown(void);
extern void gpHalSPU_Set_EnvRampDownMultiChannel(UINT32 ChannelBit);

void gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(UINT8 ChannelIndex);
//0409
extern void gpHalSPU_Set_MainVolume(UINT8 VolumeData);
extern UINT32 gpHalSPU_Get_FIQ_Status(void);
extern void gpHalSPU_Clear_FIQ_Status(UINT32 ChannelBit);
extern void gpHalSPU_Disable_BeatIRQ(void);
extern void gpHalSPU_Clear_BeatIRQ_Flag(void);
extern UINT8 gpHalSPU_Get_BeatIRQ_Flag(void);
extern void gpHalSPU_Channel_FIQ_Enable(UINT8 ChannelIndex);
extern void gpHalSPU_Disable_FIQ_Channel(UINT8 ChannelIndex);
extern void gpHalSPU_Set_EnvelopeClock(UINT8 EnvClock, UINT8 ChannelIndex);
extern void gpHalSPU_DisableChannelRepeat(UINT8 ChannelIndex);
extern void gpHalSPU_DisablePitchBend(UINT32 ChannelBit);
extern void gpHalSPU_DisableChannelZC(UINT8 ChannelIndex);//0410
extern void gpHalSPU_AccumulatorInit(void);
extern void gpHalSPU_Clear_FOF(void);//0410
extern void gpHalSPU_SingleChVolumeSelect(UINT8 VolumeSelect);
extern void gpHalSPU_InterpolationON(void);//0410
extern void gpHalSPU_InterpolationOFF(void);
extern void gpHalSPU_HQ_InterpolationON(void);//0410
extern void gpHalSPU_HQ_InterpolationOFF(void);
extern void gpHalSPU_CompressorON(void);
extern void gpHalSPU_CompressorOFF(void);
//extern void gpHalSPU_SetCompressorRatio(UINT8 ComRatio);//0410
//extern void gpHalSPU_DisableCompZeroCrossing(void);//0410
//extern void gpHalSPU_SetReleaseTimeScale(UINT8 ReleaseTimeScale);//0410
//extern void gpHalSPU_SetAttackTimeScale(UINT8 AttackTimeScale);//0410
//extern void gpHalSPU_SetCompressThreshold(UINT8 CompThreshold);//0410
//extern void gpHalSPU_SelectPeakMode(void);//0410
//extern void gpHalSPU_SetReleaseTime(UINT8 ReleaseTime);//0410
//extern void gpHalSPU_SetAttackTime(UINT8 AttackTime);//0410
extern void gpHalSPU_SetBankAddr(UINT8 BankAddr);




#endif
