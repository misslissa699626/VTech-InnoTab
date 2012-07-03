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
 * @file    hal_spu.c
 * @brief   Implement of SPU HAL API.
 * @author  
 * @since   2010-10-20
 * @date    2010-10-20
 */
#include <linux/io.h>
#include <mach/hal/hal_spu.h>
#include <mach/hal/regmap/reg_spu.h>
#include <mach/hal/regmap/reg_scu.h>
#include <mach/hal/hal_clock.h>
//#include <mach/hal/hal_dac.h>
//#include <mach/hal/hal_spu_lib.h>
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/


/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
static spuReg_t *pSpuReg = (spuReg_t *)(SPU_BASE_REG);
static spuAtt_t *pSpuAtt = (spuAtt_t *)(SPU_BASE_Att);
static spuPhase_t *pSpuPhase = (spuPhase_t *)(SPU_BASE_Phase);

static scuaReg_t *pScuaReg = (scuaReg_t *)(LOGI_ADDR_SCU_A_REG);
static iisReg_t *piisReg = (iisReg_t *)(IIS_BASE_Addr);
static dacReg_t *pdacReg = (dacReg_t *)(DAC_BASE_Addr);
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
/////////////spu_midi_driver.h
extern UINT32 T_InstrumentSectionAddr[];
extern UINT32 T_InstrumentStartSection[];
extern UINT32 T_InstrumentPitchTable[];
extern UINT32 T_DrumAddr[];
extern const UINT32 T_VarPhaseTableBottom[];
extern const UINT32 T_PicthWheelTable[];
extern const UINT32 T_PicthWheelTable_TWO[];
extern const UINT32 T_BitEnable[];
extern const UINT32 T_VarPhaseTableBottom[];
#define T_VarPhaseTable T_VarPhaseTableBottom + 107


#define D_PITCH_BEND_TABLE_TOTAL	 0x000D

/////////////mcu_spu.h
#define C_BEAT_IRQ_STATUS			0x4000
// bit[15] : beat IRQ enable
#define C_BEAT_IRQ_EN				0x8000

// P_SPU_CONTROL_FLAG (0xD0400E34)
// bit[2:0] : reserved
// bit[3] : Initial channel's accumulator
#define C_INIT_ACC					0x0008
// bit[4] : reserved
// bit[5] : sample rate of tone color too high flag
#define C_FOF_FLAG					0x0020
// bit[7:6] : volume of single channel
#define C_CH_VOL_SEL				0x00C0
#define C_CH_VOL_1_DIV_32			0x0000		// 1/32
#define C_CH_VOL_1_DIV_8			0x0040		// 1/8
#define C_CH_VOL_1_DIV_2			0x0080		// 1/4
#define C_CH_VOL_1_DIV_1			0x00C0		// 1
// bit[8] : reserved
// bit[9] : 1: interpolation off, 0: interpolation on
#define C_NO_INTER					0x0200		
// bit[10] : 1: high quality interpolation off, 0: high quality interpolation on
#define C_NO_HIGH_INTER				0x0400
// bit[11] : 1: compressor on, 0: compressor off
#define C_COMP_EN					0x0800
// bit[14:12] : reserved
// bit[15] : signal saturate flag
#define C_SATURATE					0x8000

// P_SPU_COMPRESSOR_CONTROL (0xD0400E38)
// bit[2:0] : compress ratio
#define C_COMPRESS_RATIO			0x0007
#define C_COMP_RATIO_1_DIV_2		0x0000
#define C_COMP_RATIO_1_DIV_3		0x0001
#define C_COMP_RATIO_1_DIV_4		0x0002
#define C_COMP_RATIO_1_DIV_5		0x0003
#define C_COMP_RATIO_1_DIV_6		0x0004
#define C_COMP_RATIO_1_DIV_7		0x0005
#define C_COMP_RATIO_1_DIV_8		0x0006
#define C_COMP_RATIO_1_DIV_INIF		0x0007
// bit[3] : 1: disable zero cross, 0: enable zero cross
#define C_DISABLE_ZC				0x0008
// bit[5:4] : release time scale
#define C_RELEASE_SCALE				0x0030
#define C_RELEASE_TIME_MUL_1		0x0000
#define C_RELEASE_TIME_MUL_4		0x0010
#define C_RELEASE_TIME_MUL_16		0x0020
#define C_RELEASE_TIME_MUL_64		0x0030
// bit[7:6] : attack time scale
#define C_ATTACK_SCALE				0x00C0
#define C_ATTACK_TIME_MUL_1			0x0000
#define C_ATTACK_TIME_MUL_4			0x0040
#define C_ATTACK_TIME_MUL_16		0x0080
#define C_ATTACK_TIME_MUL_64		0x00C0
// bit[14:8] : compress threshold, 0x01~0x7F
#define C_COMPRESS_THRESHOLD		0x7F00
// bit[15] : peak mode
#define C_COMPRESS_PEAK_MODE		0x8000
#define C_PW_OVERFLOW				0x0001
// bit[4:1] : reserved
// bit[5] : post wave output right channel enable  0: LLLLLL... 1: LRLRLR....
#define C_PW_LR						0x0020
// bit[6] : 1: post wave output to DAC, 0: post wave do not output to DAC
#define C_PW_TO_DAC					0x0040
// bit[7] : 1: post wave is signed, 0: post wave is unsigned
#define C_PW_SIGNED					0x0080
// bit[9:8] : reserved
// bit[10] : 1: post wave IRQ active, 0: post wave IRQ inactive
#define C_PW_IRQ_ACTIVE				0x0400
// bit[11] : 1: post wave IRQ enable, 0: post wave IRQ disable
#define C_PW_IRQ_ENABLE				0x0800
// bit[12] : 1: post wave processing in 288KHz, 0: post wave processing is not valid
#define C_PW_CLOCK_SET				0x1000
// bit[13] : 1: enable post wave low pass filter, 0: disable post wave low pass filter
#define C_PW_LPF_ENABLE				0x2000
// bit[14] : 1: post wave down sample by 6, 0: post wave not down sample
#define C_PW_DOWN_SAMPLE			0x4000
// bit[15] : 1: post wave output enable, 0: post wave output disable
#define C_PW_DMA					0x8000


#define C_TONE_COLOR_MODE			0x3000
#define C_SW_MODE					0x0000
#define C_HW_AUTO_END_MODE			0x1000
#define C_HW_AUTO_REPEAT_MODE		0x2000
#define C_HW_AUTO_REPEAT_MODE1		0x3000
// bit[14] : 1: 16-bit data mode, 0: 8-bit data mode
#define C_16BIT_DATA_MODE			0x4000
// bit[15] : 1: ADPCM mode, 0: PCM mode
#define C_ADPCM_MODE				0x8000
#define C_ENVELOPE_SIGN				0x0080
#define C_ENVELOPE_REPEAT			0x0100
#define C_ENVELOPE_IRQ_ENABLE		0x0040
#define C_ADPCM36_MODE				0x8000

#define C_RAMP_DOWN_CLK_0738us		0x0000
#define C_RAMP_DOWN_CLK_2955us		0x0001
#define C_RAMP_DOWN_CLK_11821us		0x0002
#define C_RAMP_DOWN_CLK_47284us		0x0003
#define C_RAMP_DOWN_CLK_189137us	0x0004
#define C_RAMP_DOWN_CLK_756548us	0x0005
#define C_RAMP_DOWN_CLK_1513ms		0x0006
#define C_RAMP_DOWN_CLK_1513ms_2	0x0007

#define C_PHASE_SIGN				0x1000
// bit[15:13] : pitch bend phase time step
#define C_PHASE_TIME_STEP_0114us	0x0000
#define C_PHASE_TIME_STEP_0227us	0x2000
#define C_PHASE_TIME_STEP_0455us	0x4000
#define C_PHASE_TIME_STEP_0909us	0x6000
#define C_PHASE_TIME_STEP_1819us	0x8000
#define C_PHASE_TIME_STEP_3637us	0xA000
#define C_PHASE_TIME_STEP_7274us	0xC000
#define C_PHASE_TIME_STEP_14549us	0xE000


/////////////////drv_spu_midi.c




//#define C_SPU_FIQ_Number		4
const UINT32 T_BitEnable_Table[]={
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

const UINT32 T_BitDisable_Table[]={
	~0x00000001, ~0x00000002, ~0x00000004, ~0x00000008,
	~0x00000010, ~0x00000020, ~0x00000040, ~0x00000080,
	~0x00000100, ~0x00000200, ~0x00000400, ~0x00000800,
	~0x00001000, ~0x00002000, ~0x00004000, ~0x00008000,
	~0x00010000, ~0x00020000, ~0x00040000, ~0x00080000,
	~0x00100000, ~0x00200000, ~0x00400000, ~0x00800000,
	~0x01000000, ~0x02000000, ~0x04000000, ~0x08000000,
	~0x10000000, ~0x20000000, ~0x40000000, ~0x80000000
};

//------------------------------------------------------------------------------------------------------------20110505
unsigned int gpHalSPU_RegRead(unsigned int addr)
{
	//unsigned int temp;
	//temp = *((unsigned int*)addr);
	//return temp;
	return (*((volatile unsigned int*)addr));
}

int gpHalSPU_RegReadMany(unsigned int addr, unsigned int *buf, int n)
{
	volatile unsigned int *p_s;
	unsigned int *p_t,i;//temp,i;
	
	p_s = (unsigned int*)addr;
	p_t = (unsigned int*)buf;
	
	for(i=0;i<n;i++)
	{
		//temp = *p_s++;
		//*p_t++ = temp;
		*p_t++ = *p_s++;
	}
	return 0;
}

int gpHalSPU_RegWrite(unsigned int addr, unsigned int value)
{
	*((volatile unsigned int*)addr) = value;
	return 0;
}

int gpHalSPU_RegWriteMany(unsigned int addr, unsigned int *buf, int n)
{
	unsigned int *p_s,i;//temp,i;
	volatile unsigned int *p_t;
	
	p_s = (unsigned int*)buf;
	p_t = (unsigned int*)addr;
	
	for(i=0;i<n;i++)
	{
		//temp = *p_s++;
		//*p_t++ = temp;
		*p_t++ = *p_s++;
	}
	return 0;
}

int gpHalSPU_RegModifyWrite(unsigned int addr, unsigned int and_this, unsigned int or_this)
{
	//unsigned int temp;
	//temp = (*((unsigned int*)addr) & and_this) | or_this;
	//*((unsigned int*)addr) = temp;
	*((volatile unsigned int*)addr) = (*((volatile unsigned int*)addr) & and_this) | or_this;
	return 0;
}


/**
* @brief	spu set envelope 0
* @param	envelope0 value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetEnvelope_0(UINT16 Envelope_0, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuEnv0) + 0x10 * ChannelIndex) = Envelope_0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnv0) + 0x10 * ChannelIndex), Envelope_0);
}

/**
* @brief	spu set envelope 1
* @param	envelope1 value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetEnvelope_1(UINT16 Envelope_1, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuEnv1) + 0x10 * ChannelIndex) = Envelope_1;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnv1) + 0x10 * ChannelIndex), Envelope_1);
}

/**
* @brief	spu set velocity
* @param 	velocity value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetVelocity(UINT8 VelocityValue, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;

	//Temp = *(&(pSpuAtt->spuPanVol) + 0x10*ChannelIndex);
	//Temp &= 0xFF00;
	//Temp |= (VelocityValue & 0x007F);
	//*(&(pSpuAtt->spuPanVol) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuPanVol) + 0x10 * ChannelIndex), 0xFF00, (VelocityValue & 0x007F));
}

/**
* @brief	spu set pan value
* @param 	pan value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPan(UINT8 PanValue, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	
	//Temp = *(&(pSpuAtt->spuPanVol) + 0x10*ChannelIndex);
	//Temp &= 0x00FF;
	//Temp |= ((PanValue & 0x007F) << 8);
	//*(&(pSpuAtt->spuPanVol) + 0x10*ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuPanVol) + 0x10*ChannelIndex), 0x00FF, ((PanValue & 0x007F) << 8));
}

/**
* @brief	spu set wave data 0
* @param 	wave data 0
* @param	channel
* @return 	none
*/
void gpHalSPU_SetWaveData_0(UINT16 WDD_0, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;

	//*(&(pSpuAtt->spuWavDat0) + 0x10 * ChannelIndex) = WDD_0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWavDat0) + 0x10 * ChannelIndex), WDD_0);
}

/**
* @brief	spu set wave data
* @param 	wave data
* @param	channel
* @return 	none
*/
void gpHalSPU_SetWaveData(UINT16 WDD, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	
	//*(&(pSpuAtt->spuWavData) + 0x10 * ChannelIndex) = WDD;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWavData) + 0x10 * ChannelIndex), WDD);
}

/**
* @brief	spu set loop address
* @param 	loop address
* @param	channel
* @return 	none
*/
void gpHalSPU_SetLoopAddress(UINT32 LoopAddr, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuLoopAddr) + 0x10 * ChannelIndex) = LoopAddr;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuLoopAddr) + 0x10 * ChannelIndex), LoopAddr);
	//Temp = *(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex);
	//Temp &= ~0x0FC0;
	//Temp |= ((LoopAddr >> 10) & 0x0FC0);
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), ~0x0FC0, ((LoopAddr >> 10) & 0x0FC0));

	// ycliao, for more address bits
	//Temp = *(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex);
	//Temp &= ~0x0FC0;
	//Temp |= ((LoopAddr >> 16) & 0x0FC0);
	//*(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex), ~0x0FC0, ((LoopAddr >> 16) & 0x0FC0));
}


void gpHalSPU_Set_two_ch_LoopAddress(UINT32 LoopAddr0, UINT32 LoopAddr1, UINT8 spu_ch0, UINT8 spu_ch1)
{
	//UINT16 temp0,temp1;
	//UINT16 *p0, *p1;
	
	//p0 = (UINT16*)(&(pSpuAtt->spuLoopAddr) + 0x10 * spu_ch0);
	//p1 = (UINT16*)(&(pSpuAtt->spuLoopAddr) + 0x10 * spu_ch1);
	//temp0 = LoopAddr0;
	//temp1 = LoopAddr1;
	//*p0 = temp0;
	//*p1 = temp1;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuLoopAddr) + 0x10 * spu_ch0), LoopAddr0);
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuLoopAddr) + 0x10 * spu_ch1), LoopAddr1);
	
	//p0 = (UINT16*)(&(pSpuAtt->spuMode) + 0x10 * spu_ch0);
	//p1 = (UINT16*)(&(pSpuAtt->spuMode) + 0x10 * spu_ch1);
	//temp0 = *p0;
	//temp1 = *p1;
	//temp0 &= ~0x0FC0;
	//temp1 &= ~0x0FC0;
	//temp0 |= ((LoopAddr0 >> 10) & 0x0FC0);
	//temp1 |= ((LoopAddr1 >> 10) & 0x0FC0);
	//*p0 = temp0;
	//*p1 = temp1;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * spu_ch0), ~0x0FC0, ((LoopAddr0 >> 10) & 0x0FC0));
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * spu_ch1), ~0x0FC0, ((LoopAddr1 >> 10) & 0x0FC0));
	
	//p0 = (UINT16*)(&(pSpuAtt->spuWlAddrH) + 0x10 * spu_ch0);
	//p1 = (UINT16*)(&(pSpuAtt->spuWlAddrH) + 0x10 * spu_ch1);
	//temp0 = *p0;
	//temp1 = *p1;
	//temp0 &= ~0x0FC0;
	//temp1 &= ~0x0FC0;
	//temp0 |= ((LoopAddr0 >> 16) & 0x0FC0);
	//temp1 |= ((LoopAddr1 >> 16) & 0x0FC0);
	//*p0 = temp0;
	//*p1 = temp1;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuWlAddrH) + 0x10 * spu_ch0), ~0x0FC0, ((LoopAddr0 >> 16) & 0x0FC0));
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuWlAddrH) + 0x10 * spu_ch1), ~0x0FC0, ((LoopAddr1 >> 16) & 0x0FC0));
}



/**
* @brief	spu set tone color mode
* @param 	tone color mode
* @param	channel
* @return 	none
*/
void gpHalSPU_SetToneColorMode(UINT8 ToneColorMode, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex);
	//Temp &= ~0x3000;
	//Temp |= (ToneColorMode << 12);
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), ~0x3000, (ToneColorMode << 12));
}

/**
* @brief	spu set 8-bit mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_Set_8bit_Mode(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;

	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) &= ~C_16BIT_DATA_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), ~C_16BIT_DATA_MODE, 0);
}

/**
* @brief	spu set 16-bit mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_Set_16bit_Mode(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) |= C_16BIT_DATA_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), 0xFFFFFFFF, C_16BIT_DATA_MODE);
}

/**
* @brief	spu set adpcm mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_SetADPCM_Mode(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) |= C_ADPCM_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), 0xFFFFFFFF, C_ADPCM_MODE);
}

/**
* @brief	spu set pcm mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_SetPCM_Mode(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) &= ~C_ADPCM_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), ~C_ADPCM_MODE, 0);
}

/**
* @brief	spu set adpcm mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_SelectADPCM_Mode(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuAdpcmSel)+ 0x10 * ChannelIndex) &= ~C_ADPCM36_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuAdpcmSel)+ 0x10 * ChannelIndex), ~C_ADPCM36_MODE, 0);
}

//20110409
/**
* @brief	spu clear adpcm36 mode
* @param 	channel
* @return 	none
*/
void gpHal_SPU_ClearADCPM36_Mode(UINT8 ChannelIndex)	
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuAdpcmSel)+ 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuAdpcmSel)+ 0x10 * ChannelIndex), 0);
}

/**
* @brief	spu set adpcm36 mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_SelectADPCM36_Mode(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex) |= C_ADPCM36_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex), 0xFFFFFFFF, C_ADPCM36_MODE);
}

/**
* @brief	spu set envelope address
* @param 	envelope address
* @param	channel
* @return 	none
*/
void gpHalSPU_SetEnvelopeAddress(UINT32 EnvelopeAddr, UINT8 ChannelIndex)
{
//	UINT16 Temp;
		
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuEnvAddrL) + 0x10 * ChannelIndex) = EnvelopeAddr;
	//Temp = *(&(pSpuAtt->spuEnvAddrH) + 0x10 * ChannelIndex);
	//Temp &= ~0x003F;
	//Temp |= ((EnvelopeAddr >> 16) & 0x003F);
	//*(&(pSpuAtt->spuEnvAddrH) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnvAddrL) + 0x10 * ChannelIndex), EnvelopeAddr);
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuEnvAddrH) + 0x10 * ChannelIndex), ~0x003F, ((EnvelopeAddr >> 16) & 0x003F));

	// ycliao, for more address bits
	//Temp = *(&(pSpuAtt->spuEnvAddrH1) + 0x10 * ChannelIndex);
	//Temp &= ~0x003F;
	//Temp |= ((EnvelopeAddr >> 22) & 0x003F);
	//*(&(pSpuAtt->spuEnvAddrH1) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuEnvAddrH1) + 0x10 * ChannelIndex), ~0x003F, ((EnvelopeAddr >> 22) & 0x003F));
}

/**
* @brief	spu set adpcm point number
* @param 	adpcm point number
* @param 	channel
* @return 	none
*/
void gpHalSPU_SetADPCM_PointNumber(UINT8 PointNumber, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex);
	//Temp &= ~0x7E00;
	//Temp |= ((PointNumber & 0x001F) << 9);
	//*(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex), ~0x7E00, ((PointNumber & 0x001F) << 9));
}

/**
* @brief	spu set envelope auto mode
* @param 	channel
* @return 	none
*/
void gpHalSPU_EnvelopeAutoMode(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitDisable_Table;
	pAddr += ChannelIndex;	
	//pSpuReg->spuChEnvMod &= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnvMod), *pAddr, 0);
	//pSpuReg->spuChEnvModH &= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnvModH), (*pAddr >> 16), 0);
}

void gpHalSPU_EnvelopeManualMode(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;	
	//pSpuReg->spuChEnvMod |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnvMod), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuChEnvModH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnvModH), 0xFFFFFFFF, (*pAddr >> 16));
}

/**
* @brief	spu set start address
* @param 	start address
* @param 	channel
* @return 	none
*/
void gpHalSPU_SetStartAddress(UINT32 StartAddr, UINT8 ChannelIndex)
{
	//UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//*(&(pSpuAtt->spuWavAddr) + 0x10 * ChannelIndex) = StartAddr;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWavAddr) + 0x10 * ChannelIndex), StartAddr);
	//Temp = *(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex);
	//Temp &= ~0x003F;
	//Temp |= ((StartAddr >> 16) & 0x003F);
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) =Temp;	
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), ~0x003F, ((StartAddr >> 16) & 0x003F));
	
	// ycliao, for more address bits
	//Temp = *(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex);
	//Temp &= ~0x003F;
	//Temp |= ((StartAddr >> 22) & 0x003F);
	//*(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex), ~0x003F, ((StartAddr >> 22) & 0x003F));

	// ycliao, for more address bits
	//Temp = ((StartAddr >> 29) & 0x000F) << 6;
	//Temp = Temp << 6;
	//*(&(pSpuReg->spuBankAddr)) = Temp;	
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuBankAddr), (((StartAddr >> 29) & 0x000F) << 6));
}

/**
* @brief	spu set envelope data
* @param	envelope data
* @param	channel
* @return 	none
*/
void gpHalSPU_SetEnvelopeData(UINT8 EnvData, UINT8 ChannelIndex)
{
	//UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex);
	//Temp &= ~0x007F;
	//Temp |= (EnvData & 0x007F);
	//*(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex), ~0x007F, (EnvData & 0x007F));
}

/**
* @brief	spu set envelope ramp down offset
* @param	ramp down offset value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetEnvelopeRampDownOffset(UINT8 RampDownOffset, UINT8 ChannelIndex)
{
//	UINT16 Temp;

	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex);
	//Temp &= 0x01FF;
	//Temp |= ((RampDownOffset & 0x007F) << 9);
	//*(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex), 0x01FF, ((RampDownOffset & 0x007F) << 9));
}

/**
* @brief	spu set envelope repeat address offset
* @param	envelope repeat address offset value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetEnvelopeRepeatAddrOffset(UINT16 EAOffset, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex);
	//Temp &= ~0x01FF;
	//Temp |= (EAOffset & 0x01FF);
	//*(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex), ~0x01FF, (EAOffset & 0x01FF));
}

/**
* @brief	spu set phase
* @param	phase value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPhase(UINT32 Phase, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex) = Phase;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex), Phase);
	//*(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex) = Phase >> 16;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex), (Phase >> 16));
}

void gpHalSPU_Set_two_channel_Phase(UINT32 Phase0, UINT8 ChannelIndex0, UINT32 Phase1, UINT8 ChannelIndex1)
{
	ChannelIndex0 &= 0x1F;
	ChannelIndex1 &= 0x1F;
	//*(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex) = Phase;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex0), Phase0);
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex1), Phase1);
	//*(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex) = Phase >> 16;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex0), (Phase0 >> 16));
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex1), (Phase1 >> 16));
}

//0409
/**
* @brief	spu read phase value
* @param	channel
* @return	phase value
*/
UINT32 gpHalSPU_ReadPhase(UINT8 ChannelIndex)
{
	UINT32 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex) | ((*(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex) & 0x0007) << 16);
	Temp = gpHalSPU_RegRead((unsigned int)(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex)) 
		| (gpHalSPU_RegRead((unsigned int)(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex)) << 16);
	return Temp;
}

/**
* @brief	spu set phase accumulator
* @param	phase accumulator value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPhaseAccumulator(UINT32 PhaseAcc, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuPhase->spuPhaseAcc) + 0x10 * ChannelIndex) = PhaseAcc;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseAcc) + 0x10 * ChannelIndex), PhaseAcc);
	//*(&(pSpuPhase->spuPhaseAccH) + 0x10 * ChannelIndex) = PhaseAcc >> 16;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseAccH) + 0x10 * ChannelIndex), (PhaseAcc >> 16));
}

/**
* @brief	spu set targe phase value
* @param	target phase value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetTargetPhase(UINT32 TargetPhase, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuPhase->spuTarPhase) + 0x10 * ChannelIndex) = TargetPhase;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuTarPhase) + 0x10 * ChannelIndex), TargetPhase);
	//*(&(pSpuPhase->spuTarPhaseH) + 0x10 * ChannelIndex) = TargetPhase >> 16;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuTarPhaseH) + 0x10 * ChannelIndex), TargetPhase >> 16);
}

/**
* @brief	spu set phase offset
* @param	phase offset value
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPhaseOffset(UINT16 PhaseOffset, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex);
	//Temp &= 0xF000;
	//Temp |= (PhaseOffset & 0x0FFF);
	//*(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex), 0xF000, (PhaseOffset & 0x0FFF));
}

/**
* @brief	spu set phase increase
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPhaseIncrease(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex) &= ~C_PHASE_SIGN;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex), ~C_PHASE_SIGN, 0);
}

/**
* @brief	spu set phase decrease
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPhaseDecrease(UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex) |= C_PHASE_SIGN;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex), 0xFFFFFFFF, C_PHASE_SIGN);
}

/**
* @brief	spu set phase time step
* @param	phase time step
* @param	channel
* @return 	none
*/
void gpHalSPU_SetPhaseTimeStep(UINT8 PhaseTimeStep, UINT8 ChannelIndex)
{
//	UINT16 Temp;
		
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex);
	//Temp &= 0x1FFF;
	//Temp |= ((PhaseTimeStep & 0x0007) << 13);
	//*(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex), 0x1FFF, ((PhaseTimeStep & 0x0007) << 13));
}

/**
* @brief	spu set envelope ramp down
* @param	channel
* @return 	none
*/
void gpHalSPU_Set_EnvRampDown(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuEnvRampD |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvRampD), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuEnvRampDH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvRampDH), 0xFFFFFFFF, (*pAddr >> 16));
}

/**
* @brief	spu set channel status
* @param	none
* @return	channel status
*/
UINT32 gpHalSPU_GetChannelStatus(void)
{
	UINT32 Status;
	
	//Status = (pSpuReg->spuChStsH << 16) | pSpuReg->spuChSts;
	Status = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChSts)) 
			| (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChStsH)) << 16);
	return Status;
}

/**
* @brief	spu set single channel satus
* @param	channel
* @return	channel status
*/
UINT8 gpHalSPU_Get_SingleChannel_Status(UINT8 SPU_Channel)
{
	UINT32 temp;
	temp = gpHalSPU_GetChannelStatus();
	if(temp & (0x00000001 << SPU_Channel))
		return(0x01);
	else
		return(0x00);
}

/**
* @brief	spu set envelope counter
* @param	envelope counter
* @param	channnel
* @return	none
*/
void gpHalSPU_SetEnvelopeCounter(UINT8 EnvCounter, UINT8 ChannelIndex)
{
//	UINT16 Temp;
	
	ChannelIndex &= 0x1F;
	//Temp = *(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex);
	//Temp &= ~0xFF00;
	//Temp |= ((EnvCounter & 0x00FF) << 8);
	//*(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex) = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex), ~0xFF00, ((EnvCounter & 0x00FF) << 8));
}

/**
* @brief	spu set ramp down clock
* @param	ramp down clock
* @param	channnel
* @return	none
*/
void gpHalSPU_SetRampDownClock(UINT8 RampDownClock, UINT8 ChannelIndex)
{
	ChannelIndex &= 0x1F;
	//*(&(pSpuPhase->spuRampDownClk) + 0x10 * ChannelIndex) = (RampDownClock & 0x07);
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuRampDownClk) + 0x10 * ChannelIndex), (RampDownClock & 0x07));
}

/**
* @brief	spu clear multi-channel stop flag
* @param	channels
* @return	none
*/
void gpHalSPU_Clear_MultiCh_StopFlag(UINT32 ChannelBit)
{
	//spuReg_t *pSpuReg = (spuReg_t *)(SPU_BASE_REG);
	
	//pSpuReg->spuChStopSts = ChannelBit;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuChStopSts), ChannelBit);
	//pSpuReg->spuChStopStsH = (ChannelBit >> 16);
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuChStopStsH), (ChannelBit >> 16));
}

/**
* @brief	spu enable beat irq
* @return	none
*/
void gpHalSPU_Enable_BeatIRQ(void)
{
	//pSpuReg->spuBeatCnt |= C_BEAT_IRQ_EN;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuBeatCnt), 0xFFFFFFFF, C_BEAT_IRQ_EN);
}

/**
* @brief	spu disable beat irq
* @param	none
* @return	none
*/
void gpHalSPU_Disable_BeatIRQ(void)
{
	//pSpuReg->spuBeatCnt &= ~C_BEAT_IRQ_EN;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuBeatCnt), ~C_BEAT_IRQ_EN, 0);
}

/**
* @brief	spu get beat irq flag
* @param	none
* @return	beat irq status
*/
UINT8 gpHalSPU_Get_BeatIRQ_Flag(void)
{
	//if(pSpuReg->spuBeatCnt & C_BEAT_IRQ_STATUS)
	if(gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuBeatCnt)) & C_BEAT_IRQ_STATUS )
		return 1;
	else
		return 0;
}
/**
* @brief	spu clear beat irq flag
* @return	none
*/
void gpHalSPU_Clear_BeatIRQ_Flag(void)
{
	//pSpuReg->spuBeatCnt |= C_BEAT_IRQ_STATUS;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuBeatCnt), 0xFFFFFFFF, C_BEAT_IRQ_STATUS);
}


UINT8 gpHalSPU_Get_BeatIRQ_Enable_Flag(void)//20110411
{
	//if(*P_SPU_BEAT_COUNTER & C_BEAT_IRQ_EN)
	//if(pSpuReg->spuBeatCnt & C_BEAT_IRQ_EN)
	if(gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuBeatCnt)) & C_BEAT_IRQ_EN)
		return 1;
	else
		return 0;
}

/**
* @brief	spu set beat counter
* @param	beat counter
* @return	none
*/
void gpHalSPU_Set_BeatCounter(UINT16 BeatCounter)
{
//	UINT16 BeatCountReg;
	
	//BeatCountReg = pSpuReg->spuBeatCnt;
	//BeatCountReg &= ~0x7FFF;	// bit14 is beat count interrupt flag, 
	//                            // do not write '1' to this bit otherwise the interrupt flag will be cleared
	//BeatCountReg |= (BeatCounter & 0x3FFF);
	//pSpuReg->spuBeatCnt = BeatCountReg;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuBeatCnt), ~0x7FFF, (BeatCounter & 0x3FFF));
}

/**
* @brief	spu set beat base counter
* @param	beat base counter
* @return	none
*/
void gpHalSPU_Set_BeatBaseCounter(UINT16 BeatBaseCounter)
{
	BeatBaseCounter &= 0x07FF;
	//pSpuReg->spuBeatBaseCnt = BeatBaseCounter;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuBeatBaseCnt), BeatBaseCounter);
}

//////////////////////////////////////////////////////////////////////////
void gpHalSpuClkEnable(
	void
)
{
//	int i;
//	int temp;
/*
	//pScuaReg->scuaPeriClkEn = 0xFFFFFFFF;
	gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaPeriClkEn), 0xFFFFFFFF);
	//pScuaReg-> scuaSysSel = 0x03000080;
	gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaSysSel), 0x03000080);
	//pScuaReg-> scuaApllCfg = 0x05050129;
	gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaApllCfg), 0x05050129);
*/

	gpHalScuClkEnable(SCU_A_PERI_SPU, SCU_A, 1);

	gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaSysSel), 0x02000080);
//	gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaApllCfg), 0x05050129);
	
//0506
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuPostWavCtrl), 0x100);
	
	//piisReg-> iisCtrl = 0x104421;
	gpHalSPU_RegWrite((unsigned int)&(piisReg->iisCtrl), 0x104421);
/*	
	for (i=0;i<32;i++)
	{
		//piisReg-> iisIsdr = 0xe0002000;
		gpHalSPU_RegWrite((unsigned int)&(piisReg->iisIsdr), 0xe0002000);
	}
*/	
	//pScuaReg-> scuaVdacCfg = 0x08;
	//gpHalSPU_RegWrite((unsigned int)&(pScuaReg->scuaVdacCfg), 0x08);
		
	//pdacReg-> dacHdpHn |= 0x18f;
	gpHalSPU_RegModifyWrite((unsigned int)&(pdacReg->dacHdpHn), 0xFFFFFFFF, 0x18f);
	//pdacReg-> dacCtrl |= 0xd03;
	gpHalSPU_RegModifyWrite((unsigned int)&(pdacReg->dacCtrl), 0xFFFFFFFF, 0xd03);
	//pdacReg-> dacPwrCtrl |= 0x03;
	gpHalSPU_RegModifyWrite((unsigned int)&(pdacReg->dacPwrCtrl), 0xFFFFFFFF, 0x03);
}	

////////////////////////////
// SPU registers and internal SRAM initial
/*
void gpHalSPU_Clear_SRAM(void)
{
	int i;
	
	for( i = 0; i < 0x400; i++)
	{
		*(&(pSpuAtt->spuWavAddr) + i) = 0;
	}
}

static void gpHalSPU_Clear_Register(void)
{
	int i;
	
	
	for( i = 0; i < 0x40; i++)
	{
		*(&(pSpuReg->spuChEn) + i) = 0;
	}
	pSpuReg->spuWavInL = 0x8000;
	pSpuReg->spuWavInR = 0x8000;
}
*/

/**
* @brief	spu enable channel
* @param	channel
* @return	none
*/
void gpHalSPU_Enable_Channel(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChEn |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEn), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuChEnH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnH), 0xFFFFFFFF, (*pAddr >> 16));
}

/**
* @brief	spu disable channel
* @param	channel
* @return	none
*/
void gpHalSPU_Disable_Channel(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	//spuReg_t *pSpuReg = (spuReg_t *)(SPU_BASE_REG);
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitDisable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChEn &= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEn), *pAddr, 0);
	//pSpuReg->spuChEnH &= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnH), (*pAddr >> 16), 0);
}

/**
* @brief	spu enable multi-channel
* @param	channels
* @return	none
*/
void gpHalSPU_Enable_MultiChannel(UINT32 ChannelBit)
{
	//pSpuReg->spuChEn |= ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEn), 0xFFFFFFFF, ChannelBit);
	//pSpuReg->spuChEnH |= (ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnH), 0xFFFFFFFF, (ChannelBit >> 16));
}

/**
* @brief	spu disable multi-channel
* @param	channel
* @return	none
*/
void gpHalSPU_Disable_MultiChannel(UINT32 ChannelBit)
{
	//pSpuReg->spuChEn &= ~ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEn), ~ChannelBit, 0);
	//pSpuReg->spuChEnH &= ~(ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChEnH), ~(ChannelBit >> 16), 0);
}

/**
* @brief	spu get channel enable status
* @return	status
*/
UINT32 gpHalSPU_GetChannelEnableStatus(void)
{
	UINT32 Temp;
		
	//Temp = (pSpuReg->spuChEnH << 16) | pSpuReg->spuChEn;
	Temp = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChEn)) 
		| (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChEnH)) << 16);
	return Temp;
}

/**
* @brief	spu set main volume
* @param	volume
* @return	none
*/
void gpHalSPU_Set_MainVolume(UINT8 VolumeData)
{
	//pSpuReg->spuMainVol = VolumeData & 0x007F;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuMainVol), (VolumeData & 0x007F));
}

/**
* @brief	spu get main volume
* @param	none
* @return	main volume
*/
UINT8 gpHalSPU_Get_MainVolume(void)
{
	//return pSpuReg->spuMainVol;
	return (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuMainVol)));
}

/**
* @brief	spu enable fiq channel
* @param	channel
* @return	none
*/
/*
void gpHalSPU_Enable_FIQ_Channel(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChFiqEn |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEn), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuChFiqEnH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEnH), 0xFFFFFFFF, (*pAddr >> 16));
}
*/

void gpHalSPU_Channel_FIQ_Enable(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChFiqEn |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEn), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuChFiqEnH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEnH), 0xFFFFFFFF, (*pAddr >> 16));
}

/**
* @brief	spu disable fiq channel
* @param	channel
* @return	none
*/
void gpHalSPU_Disable_FIQ_Channel(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitDisable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChFiqEn &= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEn), *pAddr, 0);
	//pSpuReg->spuChFiqEnH &= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEnH), (*pAddr >> 16), 0);
}

/**
* @brief	spu enable fiq multi-channel
* @param	channels
* @return	none
*/
void gpHalSPU_Enable_FIQ_MultiChannel(UINT32 ChannelBit)
{
	//pSpuReg->spuChFiqEn |= ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEn), 0xFFFFFFFF, ChannelBit);
	//pSpuReg->spuChFiqEnH |= (ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEnH), 0xFFFFFFFF, (ChannelBit >> 16));
}

/**
* @brief	spu disable fiq multi-channel
* @param	channels
* @return	none
*/
void gpHalSPU_Disable_FIQ_MultiChannel(UINT32 ChannelBit)
{
	//pSpuReg->spuChFiqEn &= ~ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEn), ~ChannelBit, 0);
	//pSpuReg->spuChFiqEnH &= ~(ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChFiqEnH), ~(ChannelBit >> 16), 0);
}

/**
* @brief	spu clear fiq status
* @param	channels
* @return	none
*/
void gpHalSPU_Clear_FIQ_Status(UINT32 ChannelBit)
{
	//pSpuReg->spuChFiqSts = ChannelBit;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuChFiqSts), ChannelBit);
	//pSpuReg->spuChFiqStsH = ChannelBit >>16;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuChFiqStsH), (ChannelBit >>16));
}

/**
* @brief	spu get fiq status
* @param	none
* @return	fiq status
*/
UINT32 gpHalSPU_Get_FIQ_Status(void)
{
	UINT32 FIQ_Status;
	
	//FIQ_Status = (pSpuReg->spuChFiqStsH << 16) | pSpuReg->spuChFiqSts;
	FIQ_Status = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChFiqSts)) 
				| (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChFiqStsH)) << 16);
	return FIQ_Status;
}

/**
* @brief	spu get beat base counter
* @param	none
* @return	beat base counter
*/
UINT16 gpHalSPU_Get_BeatBaseCounter(void)
{
	UINT16 BeatBaseCounter;
	
	//BeatBaseCounter = pSpuReg->spuBeatBaseCnt;
	BeatBaseCounter = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuBeatBaseCnt));
	return BeatBaseCounter;
}

/**
* @brief	spu get beat counter
* @param	none
* @return	beat counter
*/
UINT16 gpHalSPU_Get_BeatCounter(void)
{
	UINT16 BeatCounter;
	
	//BeatCounter = pSpuReg->spuBeatCnt & 0x3FFF;
	BeatCounter = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuBeatCnt)) & 0x3FFF;
	return BeatCounter;
}

/*
#define P_SPU_BASE_ADDR					0xFC800B000		//GP12

#define P_SPU_ENV_CLK_CH0_3			((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0E18))	// channel 0~3 envelope interval selection
#define P_SPU_ENV_CLK_CH4_7			((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0E1C))	// channel 4~7 envelope interval selection
#define P_SPU_ENV_CLK_CH8_11		((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0E20))	// channel 8~11 envelope interval selection
#define P_SPU_ENV_CLK_CH12_15		((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0E24))	// channel 12~15 envelope interval selection

#define P_SPU_ENV_CLK_CH16_19		((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0E98))	// channel 16~19 envelope interval selection
#define P_SPU_ENV_CLK_CH20_23		((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0E9C))	// channel 20~23 envelope interval selection
#define P_SPU_ENV_CLK_CH24_27		((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0EA0))	// channel 24~27 envelope interval selection
#define P_SPU_ENV_CLK_CH28_31		((volatile unsigned short *) (P_SPU_BASE_ADDR + 0x0EA4))	// channel 28~31 envelope interval selection
*/

/**
* @brief	spu get envelope clock
* @param	envelope clock
*	@param	channel
* @return	none
*/
void gpHalSPU_Set_EnvelopeClock(UINT8 EnvClock, UINT8 ChannelIndex)
{
//	static UINT16 TempData;
	static UINT16 MaskBit, EnvClockData;
	
	ChannelIndex &= 0x1F;
	MaskBit = 0x000F;
	MaskBit = MaskBit << (ChannelIndex & 0x0003) * 4;
	EnvClockData = EnvClock << (ChannelIndex & 0x0003) * 4;

	if (ChannelIndex<4)
	{
					//TempData = pSpuReg->spuEnvClkCh03;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh03 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh03), ~MaskBit, EnvClockData);
	}else if(ChannelIndex<8)
	{
					//TempData = pSpuReg->spuEnvClkCh47;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh47 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh47), ~MaskBit, EnvClockData);
	}else if(ChannelIndex<12)
	{
					//TempData = pSpuReg->spuEnvClkCh811;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh811 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh811), ~MaskBit, EnvClockData);
	}else if(ChannelIndex<16)
	{
					//TempData = pSpuReg->spuEnvClkCh1215;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh1215 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh1215), ~MaskBit, EnvClockData);
	}else if(ChannelIndex<20)
	{
					//TempData = pSpuReg->spuEnvClkCh1619;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh1619 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh1619), ~MaskBit, EnvClockData);
	}else if(ChannelIndex<24)
	{
					//TempData = pSpuReg->spuEnvClkCh2023;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh2023 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh2023), ~MaskBit, EnvClockData);
	}else if(ChannelIndex<28)
	{
					//TempData = pSpuReg->spuEnvClkCh2427;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh2427 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh2427), ~MaskBit, EnvClockData);
	}else 
	{
					//TempData = pSpuReg->spuEnvClkCh2831;
					//TempData &= ~MaskBit;
					//TempData |= EnvClockData;
					//pSpuReg->spuEnvClkCh2831 = TempData;
					gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvClkCh2831), ~MaskBit, EnvClockData);
	}
}

/**
* @brief	spu set multi-channel envelope ramp down
*	@param	channels
* @return	none
*/
void gpHalSPU_Set_EnvRampDownMultiChannel(UINT32 ChannelBit)
{
	//pSpuReg->spuEnvRampD |= ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvRampD), 0xFFFFFFFF, ChannelBit);
	//pSpuReg->spuEnvRampDH |= (ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuEnvRampDH), 0xFFFFFFFF, (ChannelBit >> 16));
}

/**
* @brief	spu get envelope ramp down value
* @return	envelope ramp down value
*/
UINT32 gpHalSPU_Get_EnvRampDown(void)
{
	UINT32 Temp;
	
	//Temp = (pSpuReg->spuEnvRampDH << 16) | pSpuReg->spuEnvRampD;
	Temp = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuEnvRampD)) 
		| (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuEnvRampDH)) << 16);
	return Temp;
}

/**
* @brief	spu clear channel stop flag
*	@param	channel
* @return	none
*/
void gpHalSPU_Clear_Ch_StopFlag(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChStopSts = *pAddr;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuChStopSts), *pAddr);
	//pSpuReg->spuChStopStsH = (*pAddr >> 16);
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuChStopStsH), (*pAddr >> 16));
}


/**
* @brief	spu get channel stop status
*	@param	noe
* @return	status
*/
UINT32 gpHalSPU_Get_Ch_StopStatus(void)
{
	UINT32 Temp;
	
	//Temp = (pSpuReg->spuChStopStsH << 16) | pSpuReg->spuChStopSts;
	Temp = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChStopSts)) 
		| (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuChStopStsH)) << 16);
	return Temp;
}

/**
* @brief	spu enable channel zero-crossing
*	@param	channel
* @return	none
*/
void gpHalSPU_EnableChannelZC(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChZcEn |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChZcEn), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuChZcEnH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChZcEnH), 0xFFFFFFFF, (*pAddr >> 16));
}

/**
* @brief	spu disable channel zero-crossing
*	@param	channel
* @return	none
*/
void gpHalSPU_DisableChannelZC(UINT8 ChannelIndex)
{
	UINT32 *pAddr;

	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitDisable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuChZcEn &= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChZcEn), *pAddr, 0);
	//pSpuReg->spuChZcEnH &= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChZcEnH), (*pAddr >> 16), 0);
}

/**
* @brief	spu init accumulator
*	@param	none
* @return	none
*/
void gpHalSPU_AccumulatorInit(void)
{
	//pSpuReg->spuCtrlFlag |= C_INIT_ACC;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), 0xFFFFFFFF, C_INIT_ACC);
}

/**
* @brief	spu get FOF flag
*	@param	channel
* @return	status
*/
UINT8 gpHalSPU_Read_FOF_Status(void)
{
	UINT16 Temp;
	
	Temp = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCtrlFlag));//pSpuReg->spuCtrlFlag;
	Temp &= C_FOF_FLAG;
	Temp = Temp >> 5;
	return Temp;
}

/**
* @brief	spu clear FOF flag
*	@param	none
* @return	none
*/
void gpHalSPU_Clear_FOF(void)
{
	//pSpuReg->spuCtrlFlag |= C_FOF_FLAG;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), 0xFFFFFFFF, C_FOF_FLAG);
}


void gpHalSPU_SingleChVolumeSelect(UINT8 VolumeSelect)
{
//	UINT16 Temp;
		
	//Temp = pSpuReg->spuCtrlFlag;
	//Temp &= ~C_CH_VOL_SEL;
	//Temp |= ((VolumeSelect & 0x0003) << 6);
	//pSpuReg->spuCtrlFlag = Temp;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), ~C_CH_VOL_SEL, ((VolumeSelect & 0x0003) << 6));
}

UINT8 gpHalSPU_GetSingleChVolumeSetting(void)
{
	UINT8 Temp;
	
	Temp = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCtrlFlag));//pSpuReg->spuCtrlFlag;
	Temp &= C_CH_VOL_SEL;
	Temp = Temp >> 6;
	return Temp;
}

void gpHalSPU_InterpolationON(void)
{
	//pSpuReg->spuCtrlFlag &= ~C_NO_INTER;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), ~C_NO_INTER, 0);
}

void gpHalSPU_InterpolationOFF(void)
{
	//pSpuReg->spuCtrlFlag |= C_NO_INTER;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), 0xFFFFFFFF, C_NO_INTER);
}

void gpHalSPU_HQ_InterpolationON(void)
{
	//pSpuReg->spuCtrlFlag &= ~C_NO_HIGH_INTER;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), ~C_NO_HIGH_INTER, 0);
}

void gpHalSPU_HQ_InterpolationOFF(void)
{
	//pSpuReg->spuCtrlFlag |= C_NO_HIGH_INTER;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), 0xFFFFFFFF, C_NO_HIGH_INTER);
}

void gpHalSPU_CompressorON(void)
{
	//pSpuReg->spuCtrlFlag |= C_COMP_EN;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), 0xFFFFFFFF, C_COMP_EN);
}

void gpHalSPU_CompressorOFF(void)
{
	//pSpuReg->spuCtrlFlag &= ~C_COMP_EN;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), ~C_COMP_EN, 0);
}

void gpHalSPU_ClearSaturateFlag(void)
{
	//pSpuReg->spuCtrlFlag |= C_SATURATE;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCtrlFlag), 0xFFFFFFFF, C_SATURATE);
}

UINT8 gpHalSPU_ReadSaturateFlag(void)
{
	UINT8 Temp;
	
	//Temp = (pSpuReg->spuCtrlFlag & C_SATURATE) >> 15;
	Temp = (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCtrlFlag)) & C_SATURATE) >> 15;
	return Temp;
}

void gpHalSPU_SetCompressorRatio(UINT8 ComRatio)
{
//	UINT16 Ratio;
	
	//Ratio = pSpuReg->spuCompressCtrl;
	//Ratio &= ~C_COMPRESS_RATIO;
	//Ratio |= (ComRatio & C_COMPRESS_RATIO);
	//pSpuReg->spuCompressCtrl = Ratio;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), ~C_COMPRESS_RATIO, (ComRatio & C_COMPRESS_RATIO));
}

UINT8 gpHalSPU_GetCompressorRatio(void)
{
	UINT8 Ratio;
	
	//Ratio = (pSpuReg->spuCompressCtrl & C_COMPRESS_RATIO);
	Ratio = (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCompressCtrl)) & C_COMPRESS_RATIO);
	return Ratio;
}

void gpHalSPU_EnableCompZeroCrossing(void)
{
	//pSpuReg->spuCompressCtrl &= ~C_DISABLE_ZC;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), ~C_DISABLE_ZC, 0);
}

void gpHalSPU_DisableCompZeroCrossing(void)
{
	//pSpuReg->spuCompressCtrl |= C_DISABLE_ZC;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), 0xFFFFFFFF, C_DISABLE_ZC);
}

void gpHalSPU_SetReleaseTimeScale(UINT8 ReleaseTimeScale)
{
//	UINT16 Scale;
	
	//Scale = pSpuReg->spuCompressCtrl;
	//Scale &= ~C_RELEASE_SCALE;
	//Scale |= ((ReleaseTimeScale & 0x0003) << 4);
	//pSpuReg->spuCompressCtrl = Scale;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), ~C_RELEASE_SCALE, ((ReleaseTimeScale & 0x0003) << 4));
}

UINT8 gpHalSPU_ReadReleaseTimeScale(void)
{
	UINT8 Temp;
	
	//Temp = (pSpuReg->spuCompressCtrl & C_RELEASE_SCALE) >> 4;
	Temp = (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCompressCtrl)) & C_RELEASE_SCALE) >> 4;
	return Temp;
}

void gpHalSPU_SetAttackTimeScale(UINT8 AttackTimeScale)
{
//	UINT16 Scale;
	
	//Scale = pSpuReg->spuCompressCtrl;
	//Scale &= ~C_ATTACK_SCALE;
	//Scale |= ((AttackTimeScale & 0x0003) << 6);
	//pSpuReg->spuCompressCtrl = Scale;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), ~C_ATTACK_SCALE, ((AttackTimeScale & 0x0003) << 6));
}

UINT8 gpHalSPU_ReadAttackTimeScale(void)
{
	UINT8 Temp;
	
	//Temp = (pSpuReg->spuCompressCtrl & C_ATTACK_SCALE) >> 6;
	Temp = (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCompressCtrl)) & C_ATTACK_SCALE) >> 6;
	return Temp;
}

void gpHalSPU_SetCompressThreshold(UINT8 CompThreshold)
{
//	UINT16 Threshold;
	
	//Threshold = pSpuReg->spuCompressCtrl;
	//Threshold &= ~C_COMPRESS_THRESHOLD;
	//Threshold |= ((CompThreshold & 0x007F) << 8);
	//pSpuReg->spuCompressCtrl = Threshold;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), ~C_COMPRESS_THRESHOLD, ((CompThreshold & 0x007F) << 8));
}

UINT8 gpHalSPU_ReadCompressThreshold(void)
{
	UINT8 Temp;
	
	//Temp = (pSpuReg->spuCompressCtrl & C_COMPRESS_THRESHOLD) >> 8;
	Temp = (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuCompressCtrl)) & C_COMPRESS_THRESHOLD) >> 8;
	return Temp;
}

void gpHalSPU_SelectRMS_Mode(void)
{
	//pSpuReg->spuCompressCtrl &= ~C_COMPRESS_PEAK_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), ~C_COMPRESS_PEAK_MODE, 0);
}

void gpHalSPU_SelectPeakMode(void)
{
	//pSpuReg->spuCompressCtrl |= C_COMPRESS_PEAK_MODE;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuCompressCtrl), 0xFFFFFFFF, C_COMPRESS_PEAK_MODE);
}



void gpHalSPU_SendToSoftChannel_Left(UINT16 PCM_Data)
{
	//pSpuReg->spuWavInL = PCM_Data;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuWavInL), PCM_Data);
}

void gpHalSPU_SendToSoftChannel_Right(UINT16 PCM_Data)
{
	//pSpuReg->spuWavInR = PCM_Data;
	gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuWavInR), PCM_Data);
}

UINT16 gpHalSPU_GetSPU_PlusSoftOutLeft(void)
{
	return gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuWavOutL));//pSpuReg->spuWavOutL;
}

UINT16 gpHalSPU_GetSPU_PlusSoftOutRight(void)
{
	return gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuWavOutR));//pSpuReg->spuWavOutR;
}

UINT16 gpHalSPU_GetSPU_OutLeft(void)
{
	return gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuPostWavOutL));//pSpuReg->spuPostWavOutL;
}

UINT16 gpHalSPU_GetSPU_OutRight(void)
{
	return gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuPostWavOutR));//pSpuReg->spuPostWavOutR;
}

void gpHalSPU_EnableChannelRepeat(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitEnable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuRepeatEn |= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEn), 0xFFFFFFFF, *pAddr);
	//pSpuReg->spuRepeatEnH |= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEnH), 0xFFFFFFFF, (*pAddr >> 16));
}

void gpHalSPU_DisableChannelRepeat(UINT8 ChannelIndex)
{
	UINT32 *pAddr;
	
	ChannelIndex &= 0x1F;
	pAddr = (UINT32 *)T_BitDisable_Table;
	pAddr += ChannelIndex;
	//pSpuReg->spuRepeatEn &= *pAddr;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEn), *pAddr, 0);
	//pSpuReg->spuRepeatEnH &= (*pAddr >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEnH), (*pAddr >> 16), 0);
}

void gpHalSPU_EnableMultiChannelRepeat(UINT32 ChannelBit)
{
	//pSpuReg->spuRepeatEn |= ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEn), 0xFFFFFFFF, ChannelBit);
	//pSpuReg->spuRepeatEnH |= (ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEnH), 0xFFFFFFFF, (ChannelBit >> 16));
}

void gpHalSPU_DisableMultiChannelRepeat(UINT32 ChannelBit)
{
	//pSpuReg->spuRepeatEn &= ~ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEn), ~ChannelBit, 0);
	//pSpuReg->spuRepeatEnH &= ~(ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuRepeatEnH), ~(ChannelBit >> 16), 0);
}

void gpHalSPU_EnablePitchBend(UINT32 ChannelBit)
{
	//pSpuReg->spuChPitchBendEn |= ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChPitchBendEn), 0xFFFFFFFF, ChannelBit);
	//pSpuReg->spuChPitchBendEnH |= (ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChPitchBendEnH), 0xFFFFFFFF, (ChannelBit >> 16));
}

void gpHalSPU_DisablePitchBend(UINT32 ChannelBit)
{
	//pSpuReg->spuChPitchBendEn &= ChannelBit;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChPitchBendEn), ChannelBit, 0);
	//pSpuReg->spuChPitchBendEnH  &= (ChannelBit >> 16);
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuChPitchBendEnH), (ChannelBit >> 16), 0);
}

void gpHalSPU_SetReleaseTime(UINT8 ReleaseTime)
{
//	unsigned short RelTime;
	
	//RelTime = pSpuReg->spuAttackReleaseT;
	//RelTime &= 0xFF00;
	//RelTime |= ReleaseTime;
	//pSpuReg->spuAttackReleaseT = RelTime;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuAttackReleaseT), 0xFF00, ReleaseTime);
}

unsigned char gpHalSPU_ReadReleaseTime(void)
{
	unsigned char Temp;
	
	//Temp = (pSpuReg->spuAttackReleaseT) & 0x00FF;
	Temp = gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuAttackReleaseT)) & 0x00FF;
	return Temp;
}

void gpHalSPU_SetAttackTime(UINT8 AttackTime)
{
//	unsigned short AttTime;
		
	//AttTime = pSpuReg->spuAttackReleaseT;
	//AttTime &= 0x00FF;
	//AttTime |= (AttackTime << 8);
	//pSpuReg->spuAttackReleaseT = AttTime;
	gpHalSPU_RegModifyWrite((unsigned int)&(pSpuReg->spuAttackReleaseT), 0x00FF, (AttackTime << 8));
}

unsigned char gpHalSPU_ReadAttackTime(void)
{
	unsigned char Temp;
		
	//Temp = ((pSpuReg->spuAttackReleaseT) & 0xFF00) >> 8;
	Temp = (gpHalSPU_RegRead((unsigned int)&(pSpuReg->spuAttackReleaseT)) & 0xFF00) >> 8;
	return Temp;
}

void gpHalSPU_SetBankAddr(UINT8 BankAddr)
{
		//pSpuReg->spuBankAddr = BankAddr & 0x01FF;
		gpHalSPU_RegWrite((unsigned int)&(pSpuReg->spuBankAddr), (BankAddr & 0x01FF));
}

/**
* @brief		SPU Initialization
* @param 	none
* @return 	none
*/
/*
void 
gpHalSpuInit (
	void
)
{
	UINT8 ChIndex;
	
//	SPU_Clear_Register();
	gpHalSPU_Set_MainVolume(0x7F);			// main volume (0x00~0x7F)
//g	SPU_Clear_FIQ_Status(0xFFFFFFFF);	// chear channel 0 ~ 31 FIQ status
	gpHalSPU_Set_BeatBaseCounter(0x0000);
	gpHalSPU_Set_BeatCounter(0x0000);
//g	SPU_Disable_BeatIRQ();
//	gpHalSPU_Clear_BeatIRQ_Flag();
	for(ChIndex = 0; ChIndex < 32; ChIndex++)
	{
		gpHalSPU_Disable_Channel(ChIndex);
		gpHalSPU_Clear_Ch_StopFlag(ChIndex);
//g		SPU_Disable_FIQ_Channel(ChIndex);
		
		gpHalSPU_Set_EnvelopeClock(0x03, ChIndex);
		gpHalSPU_Set_EnvRampDown(ChIndex);
		gpHalSPU_DisableChannelRepeat(ChIndex);
		gpHalSPU_EnvelopeAutoMode(ChIndex);
		gpHalSPU_DisablePitchBend(ChIndex);
		gpHalSPU_DisableChannelZC(ChIndex);
	}
	gpHalSPU_AccumulatorInit();
	gpHalSPU_Clear_FOF();
	gpHalSPU_SingleChVolumeSelect(0x02);
	gpHalSPU_InterpolationON();
	gpHalSPU_HQ_InterpolationON();
	gpHalSPU_CompressorON();


	gpHalSPU_SetCompressorRatio(0x05);
	gpHalSPU_DisableCompZeroCrossing();
	gpHalSPU_SetReleaseTimeScale(0x03);
	gpHalSPU_SetAttackTimeScale(0x00);
	gpHalSPU_SetCompressThreshold(0x60);
	gpHalSPU_SelectPeakMode();

	gpHalSPU_SetReleaseTime(0xFF);
	gpHalSPU_SetAttackTime(0x02);
	
	gpHalSPU_SetBankAddr(0x0000);

	//gpHalSPU_Clear_SRAM();	
	//SPU_Get_DrvVersion();	
}	
*/

void gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(UINT8 ChannelIndex) //0409
{ 
	//*(&(pSpuAtt->spuWavAddr) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWavAddr) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuMode) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuLoopAddr) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuLoopAddr) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuPanVol) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuPanVol) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuEnv0) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnv0) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnvData) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuEnv1) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnv1) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuEnvAddrH) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnvAddrH) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuEnvAddrL) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnvAddrL) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuWavDat0) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWavDat0) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuLoopCtrl) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuWavData) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWavData) + 0x10 * ChannelIndex), 0);	
//	*(&(pSpuAtt->spuOffset06) + 0x10 * ChannelIndex) = 0;
	//*(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuAdpcmSel) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuWlAddrH) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuAtt->spuEnvAddrH1) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuAtt->spuEnvAddrH1) + 0x10 * ChannelIndex), 0);

	//*(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex) &= 0x07;//bit[2:0] can't be cleared
	gpHalSPU_RegModifyWrite((unsigned int)(&(pSpuPhase->spuPhaseH) + 0x10 * ChannelIndex), 0x07, 0);
	//*(&(pSpuPhase->spuPhaseAccH) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseAccH) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuPhase->spuTarPhaseH) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuTarPhaseH) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuPhase->spuRampDownClk) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuRampDownClk) + 0x10 * ChannelIndex), 0);
//	*(&(pSpuPhase->spuPhase) + 0x10 * ChannelIndex) = 0;	//do not clear this reg	
	//*(&(pSpuPhase->spuPhaseAcc) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseAcc) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuPhase->spuTarPhase) + 0x10 * ChannelIndex) = 0;
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuTarPhase) + 0x10 * ChannelIndex), 0);
	//*(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex) = 0;	
	gpHalSPU_RegWrite((unsigned int)(&(pSpuPhase->spuPhaseCtrl) + 0x10 * ChannelIndex), 0);
}


