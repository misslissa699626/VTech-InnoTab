/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordapMIDI_DataPointernce with the                 *
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
 * @file    gp_spu.c
 * @brief   Implement of spu module driver.
 * @author  Cater Chen
 * @since   2010-11-18
 * @date    2010-11-18
 */

#include <linux/module.h> 
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/slab.h>
#include <mach/gp_chunkmem.h>
#include <mach/hal/hal_spu.h>
#include <mach/gp_spu.h>
#include <mach/hal/hal_dac.h>
#include <mach/hal/hal_i2s.h>

//#include <mach/hal/hal_spu_instdrum.h>
//#include <mach/hal/hal_spu_lib.h>//0409 comment
/**************************************************************************
 *                           C O N S T A N T S                            *
**************************************************************************/
#define	SPU_MINOR		0
#define SPU_NR_DEVS	1
/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 1
	#define DEBUG	printk
#else
	#define DEBUG(...)
#endif

#if 1
	#define SPU_DBG	printk
#else
	#define SPU_DBG(...)
#endif
 
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
STRUCT_MIDI_SPU *spu_struct_p;
static STRUCT_MIDI_SPU spu_struct;
static STRUCT_DRM_SPU drm_struct;

//st_gp_mixer		*st_gp_mixer_p;
st_gp_mixer		struct_gp_mixer[3];
st_gp_mixer		struct_gp_mixer_tmp;

st_gp_mixer_cmd	*st_gp_mixer_cmd_p;
st_gp_mixer_cmd	struct_gp_mixer_cmd_tmp;

//static UINT8 mixer_channel_0,mixer_channel_1;
static UINT8 mixer_playing_flag[3];
//static SINT32 mixer_start_block[3],
static SINT32 mixer_loop_block[3];
static SINT32 mixer_loop_block_bak[3];
static UINT8 mixer_pause_status[3];
static UINT32 mixer_loop_offset[3];

const UINT8 T_mixer_spu_channel[3][2] ={
	{MIXER_SPU_CH0, MIXER_SPU_CH1},
	{MIXER_SPU_CH2, MIXER_SPU_CH3},
	{MIXER_SPU_CH4, MIXER_SPU_CH5}
};

static UINT16 R_BeatCnt_temp;

//static UINT32 test_flag = 0;

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

 /**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/ 



/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
struct file_operations spu_fops = {
	.owner = THIS_MODULE,
	.open = gp_spu_open,
	.ioctl = gp_spu_ioctl,
	.release = gp_spu_release,
};

typedef struct gp_spu_dev_s {
	struct cdev c_dev;
} gp_spu_dev_t;

int spu_major;
static gp_spu_dev_t *spu_devices=NULL;
struct class *spu_class;  


//static SPU_ChannelInfoStruct *spu_ch_info_ptr;
//static SPU_ChannelInfoStruct *midi_ch_info_ptr;

//static int spu_init_flag = 0;
void SPU_err(void)
{
	SPU_DBG("gp_SPU_err\n");	
}
 /**************************************************************************
 *             F U N C T I O N    I M P L E M E N T A T I O N S           *
 **************************************************************************/
/**
 * \brief load ALP header
 */
void Load_ALP_Header(unsigned int *pAddr)
{
	volatile unsigned int uiALP_HeaderData;
	
	(spu_struct_p->ALP_Header).ALP_ID[0] = *pAddr;
	if((spu_struct_p->ALP_Header).ALP_ID[0] != 0x32465053)
	{
		SPU_err();
	}
	(spu_struct_p->ALP_Header).ALP_ID[1] = *(pAddr + 1);
	(spu_struct_p->ALP_Header).ALP_ID[2] = *(pAddr + 2);
	(spu_struct_p->ALP_Header).ALP_ID[3] = *(pAddr + 3);
	(spu_struct_p->ALP_Header).SampleRate = *(pAddr + 4);
	(spu_struct_p->ALP_Header).SampleLength = *(pAddr + 5);
	(spu_struct_p->ALP_Header).LoopStartAddr = *(pAddr + 6);
	(spu_struct_p->ALP_Header).EnvelopStartAddr = *(pAddr + 7);
	uiALP_HeaderData = *(pAddr + 8);
	(spu_struct_p->ALP_Header).WaveType = uiALP_HeaderData & 0x000000FF;
	(spu_struct_p->ALP_Header).BasePitch = (uiALP_HeaderData & 0x0000FF00) >> 8;
	(spu_struct_p->ALP_Header).MaxPitch = (uiALP_HeaderData & 0x00FF0000) >> 16;
	(spu_struct_p->ALP_Header).MinPitch = (uiALP_HeaderData & 0xFF000000) >> 24;
	uiALP_HeaderData = *(pAddr + 9);
	(spu_struct_p->ALP_Header).RampDownClock = uiALP_HeaderData & 0x000000FF;
	(spu_struct_p->ALP_Header).RampDownStep = (uiALP_HeaderData & 0x0000FF00) >> 8;
	(spu_struct_p->ALP_Header).Pan = (uiALP_HeaderData & 0x00FF0000) >> 16;
	(spu_struct_p->ALP_Header).Velocity = (uiALP_HeaderData & 0xFF000000) >> 24;
}

void SPU_StopChannel(UINT32 StopChannel) 
{
	UINT32 Temp, BitMask;
	UINT32 *pAddr;
	
	SPU_resume_channel((UINT8)StopChannel);//081225
	//-----------------------------------------------------------------
	Temp = gpHalSPU_ReadPhase((UINT8)StopChannel);//100817 避免通道无法停止的问题
	if(Temp == 0x00000000)
	{
		gpHalSPU_SetPhase(0x00010000,(UINT8)StopChannel);
	}
	//-----------------------------------------------------------------
	spu_struct_p->R_DUR_Tone[StopChannel] = 0x0000;
	pAddr = (UINT32 *)T_BitEnable;
	BitMask = *(pAddr + StopChannel);
	Temp = gpHalSPU_GetChannelStatus();
	if(Temp & BitMask)	// channel not stop
	{
		gpHalSPU_Set_EnvRampDown(StopChannel);
		gpHalSPU_SetEnvelope_0(0x0000, StopChannel);
		gpHalSPU_SetEnvelope_1(0x0000, StopChannel);
		gpHalSPU_SetEnvelopeData(0x0000, StopChannel);
		gpHalSPU_SetEnvelopeCounter(0x0000, StopChannel);
		gpHalSPU_SetWaveData(0x8000, StopChannel);
		gpHalSPU_Disable_Channel(StopChannel);
	}
}
 
void MidiSRAM_Initial(void)
{
	int i;
	for(i=0;i<SPU_ChannelNumber;i++)//100817
	{
		spu_struct_p->T_channel[i] = 0xff;
	}	
	
	spu_struct_p->User_FIQ_Flag &= ~B_SPU_BEAT_FIQ;//ggg
//	spu_struct_p->User_FIQ_Flag = 0x00;
	spu_struct_p->R_MIDI_EndFlag = 0x0000;
	spu_struct_p->R_CH_NoteOff = 0x0000;
	spu_struct_p->EventIndex = 0x0000;
//	R_PlayChannel = 0x0000;
	spu_struct_p->R_CH_OneBeat = 0x0000;
	spu_struct_p->R_MIDI_CH_MASK = 0x03FFFFFF;
	spu_struct_p->R_SourceMIDIMask = 0x0000FFFF;		// mask MIDI track
//	R_NoteOnHistPtr = 0x0000;
//	R_Avail_Voice = SPU_ChannelNumber;
}

void MidiPlay_Initialize(void)
{
	UINT32 ChannelIndex;
	
	MidiSRAM_Initial();
//	spu_struct_p->MIDI_Tempo = C_DefaultTempo;
//	(spu_struct_p->MIDI_ChannelInfo[1]).R_MIDI_CH_PAN = 0x0000;
	spu_struct_p->pMIDI_ChInfo = (MIDI_ChannelInfoStruct *)(spu_struct_p->MIDI_ChannelInfo);
	spu_struct_p->pSPU_ChInfo = (SPU_ChannelInfoStruct *)(spu_struct_p->SPU_ChannelInfo);
	for(ChannelIndex = 0; ChannelIndex < MIDI_ChannelNumber; ChannelIndex++)
	{
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_MIDI_CH_PAN = 64;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_MIDI_CH_VOLUME = 127;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_MIDI_CH_EXPRESSION = 127;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_MIDI_CH_PitchBend = 64;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_CH_SMFTrack = 0x0000;
//		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_ChannelInst = 0x0000;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_ChannelInst &= 0xffff0000;//081029
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_PB_TABLE_Addr = (UINT32 *)T_PicthWheelTable_TWO;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_RPN_ReceiveFlag = 0x0000;
		(spu_struct_p->pMIDI_ChInfo + ChannelIndex)->R_RPN_DATA = 0x0000;
	}

	for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
	{
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_NOTE_PITCH = 0;//081108
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_NOTE_VELOCITY = 127;
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_ToneLib_PAN = 64;
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_ToneLib_Volume = 127;
//		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->spu_struct_p->R_DUR_Tone = 
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_CH_MAP = 0xFFFF;
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_NoteOnHist = 0x0000;
		(spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_PB_PhaseRecord = 0x0000;
	}
	for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber + 1; ChannelIndex++)
	{
		spu_struct_p->R_DUR_Tone[ChannelIndex] = 0x0000;
	}
//0409
	for(ChannelIndex=0; ChannelIndex<SPU_ChannelNumber; ChannelIndex++)
	{
		if(spu_struct_p->R_MIDI_CH_MASK & (1<<ChannelIndex))
		{
			gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(ChannelIndex);//090527
		}
	}	
	
	
	spu_struct_p->R_MIDI_EndFlag = 0x0000;
}

void SPU_MIDI_Stop(void)
{
	UINT32 ChannelIndex;
	UINT32 Temp;
	
	spu_struct_p->MIDI_Current_Dt = 0;  	//20081028 Roy 
	spu_struct_p->MIDI_Stop_Dt = 0;     	//20081028 Roy    	
	
	gpHalSPU_Clear_BeatIRQ_Flag();
	gpHalSPU_Disable_BeatIRQ();
	gpHalSPU_Set_BeatCounter(0x00);
	spu_struct_p->MIDI_PlayFlag &= ~(STS_MIDI_PLAY | STS_MIDI_REPEAT_ON | STS_MIDI_PAUSE_ON);//080709 tangqt
	spu_struct_p->R_CH_NoteOff = 0x00000000;//0x0000;
	Temp = spu_struct_p->R_MIDI_CH_MASK;
	for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
	{
		if(Temp & 0x0001)
		{
			SPU_StopChannel(ChannelIndex);
		}
		Temp = Temp >> 1;
	}
}

void F_StopExpiredCH(void)
{
	unsigned int Temp;
	unsigned int Ch;
	Temp = gpHalSPU_Get_EnvRampDown();
	Temp |= spu_struct_p->R_CH_NoteOff;
//	Temp = spu_struct_p->R_CH_NoteOff;
//	SPU_Set_EnvRampDown(Temp);
	gpHalSPU_Set_EnvRampDownMultiChannel(Temp);
	Temp = spu_struct_p->R_CH_NoteOff;
	for(Ch = 0; Ch < SPU_ChannelNumber; Ch++)
	{
		if(Temp & 0x0001)
		{
			(spu_struct_p->pSPU_ChInfo + Ch)->R_MIDI_CH_MAP = 0xFFFF;
			(spu_struct_p->pSPU_ChInfo + Ch)->R_NOTE_PITCH = 0;//ggg				
		}
		Temp = Temp >> 1;
	}
}
//0409
UINT16 get_a_word(void)
{
	UINT16 data_temp;
	UINT8 temp,temp1;

	temp = *spu_struct_p->pMIDI_DataPointer++;//low 8 bit
	if( (UINT32)spu_struct_p->pMIDI_DataPointer >= ((UINT32)spu_struct_p->midi_ring_buffer_addr + MIDI_RING_BUFFER_SIZE) )
		spu_struct_p->pMIDI_DataPointer = (UINT8 *)spu_struct_p->midi_ring_buffer_addr;
	temp1 = *spu_struct_p->pMIDI_DataPointer++;//high 8 bit
	if( (UINT32)spu_struct_p->pMIDI_DataPointer >= ((UINT32)spu_struct_p->midi_ring_buffer_addr + MIDI_RING_BUFFER_SIZE) )
	{
//		SPU_DBG("(UINT32)spu_struct_p->pMIDI_DataPointer = %d\n",(UINT32)spu_struct_p->pMIDI_DataPointer);		
		spu_struct_p->pMIDI_DataPointer = (UINT8 *)spu_struct_p->midi_ring_buffer_addr;
//		SPU_DBG("(UINT32)spu_struct_p->pMIDI_DataPointer = %d\n",(UINT32)spu_struct_p->pMIDI_DataPointer);	
	}
	data_temp = (UINT16)temp | (((UINT16)temp1)<<8);
//	SPU_DBG("get_a_word() = 0x%x \n",data_temp);
	return data_temp;
	
}

void F_GetSeqCmd(void)
{
	unsigned short MIDI_Data,i;
	
	spu_struct_p->MIDI_PlayFlag &= ~C_MIDI_Delay;
	do
	{
		MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
		spu_struct_p->EventIndex = (MIDI_Data & 0xF000) >> 12;
		//SPU_DBG("Event Index = 0x%x \n",spu_struct_p->EventIndex);
		switch(spu_struct_p->EventIndex)
		{
			case C_NoteEvent:
				(spu_struct_p->MIDI_Event).NoteEvent.ChannelNumber = MIDI_Data & 0x000F;
				MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
				(spu_struct_p->MIDI_Event).NoteEvent.Pitch = (MIDI_Data & 0x7F00) >> 8;
				(spu_struct_p->MIDI_Event).NoteEvent.Velocity = MIDI_Data & 0x007F;
				MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
				(spu_struct_p->MIDI_Event).NoteEvent.Duration = MIDI_Data;
				ProcessNoteEvent();
				//printk("NoteEvent\n");
				break;
			
			case C_BeatCountEvent:
				//printk("BeatCountEvent\r\n");
				(spu_struct_p->MIDI_Event).BeatCountEvent.BeatCountValue = MIDI_Data & 0x7FF;
				if(MIDI_Data & 0x0800)
				{
					MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
					(spu_struct_p->MIDI_Event).BeatCountEvent.BeatCountValue |= ((MIDI_Data &= 0x0007) << 11);
				}
				ProcessBeatCountEvent();
//				SPU_Clear_BeatIRQ_Flag();
//				SPU_Enable_BeatIRQ();
				spu_struct_p->MIDI_PlayFlag |= C_MIDI_Delay;
				
				//IOA_gpio_write_io(0,0);
				
				break;
			
			case C_PitchBendEvent:
				//printk("PitchBendEvent\r\n");
				(spu_struct_p->MIDI_Event).PitchBendEvent.ChannelNumber = (MIDI_Data & 0x0F00) >> 8;
				(spu_struct_p->MIDI_Event).PitchBendEvent.PitchBendValue = MIDI_Data & 0x007F;
				MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
				ProcessPitchBendEvent();
				break;
			
			case C_ControlEvent:
				//printk("ControlEvent\r\n");
				(spu_struct_p->MIDI_Event).ControlEvent.ChannelNumber = (MIDI_Data & 0x0F00) >> 8;
				(spu_struct_p->MIDI_Event).ControlEvent.ControlNumber = MIDI_Data & 0x007F;
				MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
				(spu_struct_p->MIDI_Event).ControlEvent.ControlValue = MIDI_Data & 0x007F;
				ProcessControlEvent();
				break;
			
			case C_ProgramChangeEvent:
				(spu_struct_p->MIDI_Event).ProgramChangeEvent.ChannelNumber = (MIDI_Data & 0x0F00) >> 8;
				(spu_struct_p->MIDI_Event).ProgramChangeEvent.InstrumentIndex = MIDI_Data & 0x007F;
				ProcessProgramChangeEvent();
				//printk("ProgramChangeEvent\r\n");
				break;
			
			case C_TempoEvent:
				(spu_struct_p->MIDI_Event).TempoEvent.TempoValue = MIDI_Data & 0x00FF;
				//ProcessTempoEvent();
				//printk("TempoEvent\r\n");
				break;
			
			case C_MIDI_EndEvent:
				ProcessEndEvent();
				printk("EndEvent \r\n");
				break;
			
			case C_LyricEvent:
				//printk("LyricEvent\r\n");
				(spu_struct_p->MIDI_Event).LyricEvent.ChannelNumber = (MIDI_Data & 0x0F00) >> 8;
				(spu_struct_p->MIDI_Event).LyricEvent.LyricWordCount = MIDI_Data & 0x007F;
				MIDI_Data = get_a_word();//*spu_struct_p->pMIDI_DataPointer++;
				(spu_struct_p->MIDI_Event).LyricEvent.Duration = MIDI_Data;
				for(i = (spu_struct_p->MIDI_Event).LyricEvent.LyricWordCount; i > 0; i--)
				{
					get_a_word();
				}
				break;
			
			case C_TextEvent:
				//printk("TextEvent\r\n");
				(spu_struct_p->MIDI_Event).TextEvent.TextType = (MIDI_Data & 0x0F00) >> 8;
				(spu_struct_p->MIDI_Event).TextEvent.LyricWordCount = MIDI_Data & 0x007F;
				break;
			
			default:
				break;
		};
	}while((!(spu_struct_p->MIDI_PlayFlag & C_MIDI_Delay)) && (spu_struct_p->MIDI_PlayFlag & STS_MIDI_PLAY));
}

void ProcessNoteEvent(void)
{
	unsigned int MIDI_Ch, SPU_Ch;
	unsigned int R_NoteVolume;
	unsigned int InstrumentIndex;
	unsigned int MaxPitch, R_BasePitch;
	unsigned int SectionLow, SectionHigh, SectionIndex;
	unsigned int *pAddr, phyAddr;
	unsigned int PanWord;
	unsigned int ChannelMask;
	
	//unsigned int value;//ggg
	
	MIDI_Ch = (spu_struct_p->MIDI_Event).NoteEvent.ChannelNumber;
	R_NoteVolume = ((spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_VOLUME * (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_EXPRESSION) / 128;
	pAddr = (unsigned int *)T_BitEnable;
	ChannelMask = *(pAddr + MIDI_Ch);
	if(ChannelMask & spu_struct_p->R_SourceMIDIMask)
	{
		//Volume=CH_Volume * CH_Expression / 128
		if(MIDI_Ch == 9)
		{
			// play drum
			SPU_Ch = FindEmptyChannel();
		
			(spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_CH_SMFTrack = SPU_Ch; // ??
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_CH_MAP = MIDI_Ch; // ??
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_NOTE_VELOCITY = (spu_struct_p->MIDI_Event).NoteEvent.Velocity;
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_NOTE_PITCH = (spu_struct_p->MIDI_Event).NoteEvent.Pitch;//081024//ggg
	//		spu_struct_p->R_DUR_Tone[SPU_Ch] = (spu_struct_p->MIDI_Event).NoteEvent.Duration;
	//		InstrumentIndex = (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_ChannelInst;
	//		pAddr = (unsigned int *)spu_struct_p->T_InstrumentStartSection;
//ggg			pAddr = (unsigned int *)spu_struct_p->T_DrumAddr;
	//		SectionLow = *(pAddr + InstrumentIndex);
	//		SectionHigh = *(pAddr + InstrumentIndex + 1);
	//		for(SectionIndex = SectionLow; SectionIndex < SectionHigh; SectionIndex++)
	//		{
	//			pAddr = (unsigned int *)spu_struct_p->T_InstrumentPitchTable;
	//			MaxPitch = *(pAddr + SectionIndex) & 0x00FF;
	//			if((spu_struct_p->MIDI_Event).NoteEvent.Pitch <= MaxPitch)
	//				break;
	//		}
	//		R_BasePitch = *(pAddr + SectionIndex) >> 8;
	//		pAddr = (unsigned int *)(spu_struct_p->T_InstrumentSectionAddr + SectionIndex);
			pAddr = (unsigned int *)(spu_struct_p->T_DrumAddr + (spu_struct_p->MIDI_Event).NoteEvent.Pitch);
//			phyAddr = (UINT32*)(spu_struct_p->T_DrumPhyAddr + (spu_struct_p->MIDI_Event).NoteEvent.Pitch);//20110415
			phyAddr = (unsigned int)(spu_struct_p->T_DrumPhyAddr [(spu_struct_p->MIDI_Event).NoteEvent.Pitch]);//20110415
			Load_ALP_Header((unsigned int *)(*pAddr));
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_ToneLib_PAN = (spu_struct_p->ALP_Header).Pan;
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_ToneLib_Volume = (spu_struct_p->ALP_Header).Velocity;
			PanWord = Calculate_Pan(SPU_Ch);
			
	//		SPU_PlayNote((spu_struct_p->MIDI_Event).NoteEvent.Pitch, (unsigned int *)(*pAddr), 
	//					(PanWord >> 8), (PanWord & 0x00FF), SPU_Ch);
//			value = *pAddr;
//			printk(KERN_WARNING "drum addr = %x \r\n", pAddr);
//			printk(KERN_WARNING "drum value = %x \r\n", value);
			SPU_PlayDrum((spu_struct_p->MIDI_Event).NoteEvent.Pitch, (unsigned int *)(*pAddr), phyAddr,
						(PanWord >> 8), (PanWord & 0x00FF), SPU_Ch);
			pAddr = (unsigned int *)(T_BitEnable + SPU_Ch);
			spu_struct_p->R_CH_OneBeat |= *pAddr;
		}
		else	// not drum
		{
			if((spu_struct_p->MIDI_Event).NoteEvent.Duration == 0)//20110411
				return;
			SPU_Ch = FindEmptyChannel();
			(spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_CH_SMFTrack = SPU_Ch; // ??
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_CH_MAP = MIDI_Ch; // ??
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_NOTE_VELOCITY = (spu_struct_p->MIDI_Event).NoteEvent.Velocity;
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_NOTE_PITCH = (spu_struct_p->MIDI_Event).NoteEvent.Pitch;//081024//ggg
			spu_struct_p->R_DUR_Tone[SPU_Ch] = (spu_struct_p->MIDI_Event).NoteEvent.Duration;
			
			//InstrumentIndex = (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_ChannelInst;
			InstrumentIndex = ((spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_ChannelInst) & 0x0000ffff;	//ggg
			
			pAddr = (unsigned int *)spu_struct_p->T_InstrumentStartSection;
			SectionLow = *(pAddr + InstrumentIndex);
			SectionHigh = *(pAddr + InstrumentIndex + 1);
			for(SectionIndex = SectionLow; SectionIndex < SectionHigh; SectionIndex++)
			{
				pAddr = (unsigned int *)spu_struct_p->T_InstrumentPitchTable;
				MaxPitch = *(pAddr + SectionIndex) & 0x00FF;
				if((spu_struct_p->MIDI_Event).NoteEvent.Pitch <= MaxPitch)
					break;
			}
			if(SectionIndex >= SectionHigh)
			{
				SectionIndex = SectionHigh - 1;
			}
			R_BasePitch = *(pAddr + SectionIndex) >> 8;
			pAddr = (unsigned int *)(spu_struct_p->T_InstrumentSectionAddr + SectionIndex);
			phyAddr = spu_struct_p->T_InstrumentSectionPhyAddr[SectionIndex];

//			phyAddr = (UINT32*)(spu_struct_p->T_InstrumentSectionPhyAddr + SectionIndex);//20110415
			//SPU_DBG("phyAddr@NoteEvent = 0x%x\n", phyAddr);
			Load_ALP_Header((unsigned int *)(*pAddr));
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_ToneLib_PAN = (spu_struct_p->ALP_Header).Pan;
			(spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_ToneLib_Volume = (spu_struct_p->ALP_Header).Velocity;
			PanWord = Calculate_Pan(SPU_Ch);
			
			SPU_PlayNote((spu_struct_p->MIDI_Event).NoteEvent.Pitch, (unsigned int *)(*pAddr), phyAddr, 
						(PanWord >> 8), (PanWord & 0x00FF), SPU_Ch);
			pAddr = (unsigned int *)(T_BitEnable + SPU_Ch);
			spu_struct_p->R_CH_OneBeat |= *pAddr;
		}
	}
}

void SPU_MIDI_Service(void)
{
	F_StopExpiredCH();
	
	if(spu_struct_p->R_DUR_Tone[SPU_ChannelNumber] != 0)
	{
		F_CheckDuration();
	}
	else
	{
		F_GetSeqCmd();
	}
	//gpHalSPU_Clear_BeatIRQ_Flag();
	//gpHalSPU_Enable_BeatIRQ();
}

/**
 * @brief 	PPU irq function.
* @return 	SUCCESS/ERROR_ID.
*/
static irqreturn_t
gp_spu_irq_handler(int irq, void *dev_id)
{
	UINT32 temp;

	if(gpHalSPU_Get_BeatIRQ_Flag())//beat irq
	{
		SPU_MIDI_IRQ();
	}
//	else
//	{
	temp = gpHalSPU_Get_FIQ_Status();//channel irq
	if(temp)
	{
		SPU_MIXER_IRQ(temp);
	}
//	}
	return IRQ_HANDLED;
}



//static irqreturn_t 
//gp_spu_irq_handler(
//	int irq, 
//	void *dev_id
//)
void SPU_MIDI_IRQ(void)
{
	unsigned int Temp,ChannelIndex;
	
	if(spu_struct_p->MIDI_PlayFlag & STS_MIDI_PLAY)
	{
		//gpHalSPU_Disable_BeatIRQ();
		SPU_MIDI_Service();
		gpHalSPU_Clear_BeatIRQ_Flag();
		//gpHalSPU_Enable_BeatIRQ();
	}
	else
	{
		if(spu_struct_p->R_MIDI_EndFlag == 0xFFFF)
		{
			spu_struct_p->R_MIDI_EndFlag = 0x0000;
//			F_MIDI_CH_StopAll();
			gpHalSPU_Disable_BeatIRQ();
			gpHalSPU_Clear_BeatIRQ_Flag();
			
			spu_struct_p->MIDI_PlayFlag &= ~(STS_MIDI_PLAY | STS_MIDI_REPEAT_ON | STS_MIDI_PAUSE_ON);
			spu_struct_p->R_CH_NoteOff = 0x00000000;
			Temp = spu_struct_p->R_MIDI_CH_MASK;
			for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
			{
				if(Temp & 0x0001)
				{
					SPU_StopChannel(ChannelIndex);
				}
				Temp = Temp >> 1;
			}
		}
	}
	if(spu_struct_p->User_FIQ_Flag & B_SPU_BEAT_FIQ)
	{
		printk("FIQ \r\n");	
		(*(spu_struct_p->SPU_User_FIQ_ISR)[C_SPU_BEAT_FIQ])();
	}
//	return IRQ_HANDLED;	
}

void SPU_MIXER_IRQ(UINT32 fiq_ch_bitmask)
{
	SINT32 i, j;
	UINT32 temp,temp1, bit_mask;
	st_gp_mixer *st_gp_mixer_tmp_p;

//	SPU_DBG("channel irq happens!!! \n");	
//	if(test_flag)
//	{
//		SPU_DBG("test_flag == 1,  warning!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
//	}
//--------------------------------------------------------------------------------------------------------------------------------------------------
	for(i=0;i<3;i++)
	{
		if(fiq_ch_bitmask & T_BitEnable[T_mixer_spu_channel[i][0]])//hardware_ch == 0
		{
			st_gp_mixer_tmp_p = (st_gp_mixer*)&struct_gp_mixer[i];
			//st_gp_mixer_tmp_p->wave_play_block = mixer_loop_block[i];//这时已切换到block来播放，并告诉上层当前使用的block
//			SPU_DBG("mixer_loop_block[i] = %d\n", mixer_loop_block[i]);
//			SPU_DBG("st_gp_mixer_tmp_p->wave_play_block = %d\n", st_gp_mixer_tmp_p->wave_play_block);
//			SPU_DBG("st_gp_mixer_tmp_p->wave_write_block = %d\n", st_gp_mixer_tmp_p->wave_write_block);
//			SPU_DBG("st_gp_mixer_tmp_p->wave_block_write_pos = %d\n", st_gp_mixer_tmp_p->wave_block_write_pos);
			
			j = st_gp_mixer_tmp_p->wave_write_block - mixer_loop_block[i];
			if(j < 0)
			{
				j += st_gp_mixer_tmp_p->iLoopQueuSize;
				//SPU_DBG("i < 0 @ irq \n");
			}

			if(   (j>1)
				|| ((j==1) && (st_gp_mixer_tmp_p->wave_block_write_pos)) 
			)
			{
				st_gp_mixer_tmp_p->wave_play_block = mixer_loop_block[i];
				mixer_loop_block_bak[i] = mixer_loop_block[i];
				if(++mixer_loop_block[i] >= st_gp_mixer_tmp_p->iLoopQueuSize)
					mixer_loop_block[i] = 0;
				temp  = (st_gp_mixer_tmp_p->l_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*mixer_loop_block[i]))>>1;
				temp1 = (st_gp_mixer_tmp_p->r_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*mixer_loop_block[i]))>>1;
				//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
				//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
				gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
			}
			else if(j==1)
			{
				if(st_gp_mixer_tmp_p->wave_block_write_pos==0)
				{
					st_gp_mixer_tmp_p->wave_play_block = mixer_loop_block[i];
					if(++mixer_loop_block[i] >= st_gp_mixer_tmp_p->iLoopQueuSize)
						mixer_loop_block[i] = 0;
					mixer_loop_block_bak[i] = mixer_loop_block[i];
					temp  = (st_gp_mixer_tmp_p->l_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*st_gp_mixer_tmp_p->iLoopQueuSize))>>1;
					temp1 = (st_gp_mixer_tmp_p->r_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*st_gp_mixer_tmp_p->iLoopQueuSize))>>1;
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}
			}
			else if(j==0)
			{
				if(mixer_loop_block_bak[i] != mixer_loop_block[i])
				{
					st_gp_mixer_tmp_p->wave_play_block = mixer_loop_block[i];
					mixer_loop_block_bak[i] = mixer_loop_block[i];
					temp  = (st_gp_mixer_tmp_p->l_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*st_gp_mixer_tmp_p->iLoopQueuSize))>>1;
					temp1 = (st_gp_mixer_tmp_p->r_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*st_gp_mixer_tmp_p->iLoopQueuSize))>>1;
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}
				else//nodata, pause
				{
					//gpHalSPU_Disable_Channel(T_mixer_spu_channel[i][0]);//20110429
					//gpHalSPU_Disable_Channel(T_mixer_spu_channel[i][1]);	//20110429
					bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]] | T_BitEnable[T_mixer_spu_channel[i][1]];
					gpHalSPU_EnvelopeAutoMode(T_mixer_spu_channel[i][0]);
					gpHalSPU_EnvelopeAutoMode(T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_EnvRampDownMultiChannel(bit_mask);					
					gpHalSPU_Disable_FIQ_Channel(T_mixer_spu_channel[i][0]);
					//SPU_pause_two_channel(T_mixer_spu_channel[i][0],T_mixer_spu_channel[i][1]);
					//SPU_pause_channel(T_mixer_spu_channel[i][0]);
					//SPU_pause_channel(T_mixer_spu_channel[i][1]);	
					mixer_loop_block[i] = st_gp_mixer_tmp_p->wave_write_block;
					mixer_loop_offset[i] = st_gp_mixer_tmp_p->wave_block_write_pos;
					st_gp_mixer_tmp_p->wave_play_block = mixer_loop_block[i];
					mixer_loop_block_bak[i] = mixer_loop_block[i];
					//mixer_pause_status[i] = 1;//20110506 mask
					st_gp_mixer_tmp_p->underrun = 1;
					SPU_DBG("mixer channel[%d] is underrun!\n",i);
					//SPU_DBG("mixer_loop_block[i] = %d\n", mixer_loop_block[i]);
					//SPU_DBG("st_gp_mixer_tmp_p->wave_play_block = %d\n", st_gp_mixer_tmp_p->wave_play_block);
					//SPU_DBG("st_gp_mixer_tmp_p->wave_write_block = %d\n", st_gp_mixer_tmp_p->wave_write_block);
					//SPU_DBG("st_gp_mixer_tmp_p->wave_block_write_pos = %d\n", st_gp_mixer_tmp_p->wave_block_write_pos);
				}
			}
			else
			{
				SPU_DBG("Error  mixer @ irq !!!!!!!!!!!!!!!!!!!\n");
			}


/*
			if(   (j > 1) 
				|| (j==1)// && (st_gp_mixer_tmp_p->wave_block_write_pos))
//				|| ((j==0) && (st_gp_mixer_tmp_p->wave_block_write_pos==0)) 
			)
			{
//				if((j==0) && (st_gp_mixer_tmp_p->wave_block_write_pos))//	//当前切换到的loop block已经写了部分数据
//				{					
//				}
//				else//将loop地址设置为下一个block的开始
//				{	
				if((j==1) && (st_gp_mixer_tmp_p->wave_block_write_pos==0))//刚好将目前播放的这个block写满，这时write block指向下一个block
				{
					if(mixer_loop_block_bak[i] != mixer_loop_block[i])
					{
						mixer_loop_block_bak[i] = mixer_loop_block[i];
					}
					else
					{
						//SPU_pause_channel(T_mixer_spu_channel[i][0]);
						//SPU_pause_channel(T_mixer_spu_channel[i][1]);			
						gpHalSPU_Disable_Channel(T_mixer_spu_channel[i][0]);//20110429
						gpHalSPU_Disable_Channel(T_mixer_spu_channel[i][1]);	//20110429
						SPU_DBG("hareware_ch %d repeat loop happen\n", i);
						SPU_DBG("st_gp_mixer_tmp_p->wave_play_block= %d \n",st_gp_mixer_tmp_p->wave_play_block);
						SPU_DBG("st_gp_mixer_tmp_p->wave_write_block = %d \n",st_gp_mixer_tmp_p->wave_write_block);	
						SPU_DBG("mixer_loop_block[i] = %d \n",mixer_loop_block[i]);					
						mixer_pause_status[i] = 1;
						st_gp_mixer_tmp_p->loop_flag = 1;//20110428
						SPU_DBG("st_gp_mixer_tmp_p->loop_flag = 1\n");		
					}
				}
				else
				{
					mixer_loop_block_bak[i] = mixer_loop_block[i];//20110428					
					mixer_loop_block[i] += 1;
					if(mixer_loop_block[i] >= st_gp_mixer_tmp_p->iLoopQueuSize)
						mixer_loop_block[i] = 0;
					//hal_set_loop(mixer_loop_block);//设置更新后的loop地址
					temp = (st_gp_mixer_tmp_p->l_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*mixer_loop_block[i]))>>1;
					temp1 = (st_gp_mixer_tmp_p->r_wavedata_phy_addr + ((4 + st_gp_mixer_tmp_p->iLoopBufLen)*mixer_loop_block[i]))>>1;
					gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					//SPU_DBG("physical address1@irq = %x\n",temp);		
					//SPU_DBG("physical address2@irq = %x\n",temp1);
//					SPU_DBG("update loop = %d\n",mixer_loop_block[i]);
				}
//				}
			}
			else//还未准备好下一个block的数据，重复播放当前的loop block
			{
				SPU_pause_channel(T_mixer_spu_channel[i][0]);
				SPU_pause_channel(T_mixer_spu_channel[i][1]);				
				SPU_DBG("hareware_ch %d repeat loop happen\n", i);
				SPU_DBG("st_gp_mixer_tmp_p->wave_play_block= %d \n",st_gp_mixer_tmp_p->wave_play_block);
				SPU_DBG("st_gp_mixer_tmp_p->wave_write_block = %d \n",st_gp_mixer_tmp_p->wave_write_block);	
				SPU_DBG("mixer_loop_block[i] = %d \n",mixer_loop_block[i]);					
				mixer_pause_status[i] = 1;
				//if((j==1) && (st_gp_mixer_tmp_p->wave_block_write_pos==0))
				//{
				//	st_gp_mixer_tmp_p->loop_flag = 1;//20110428
				//}
//				temp = (st_gp_mixer_p->l_wavedata_phy_addr + ((4 + st_gp_mixer_p->iLoopBufLen)*(mixer_loop_block[i]+1)))>>1;
//				temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4 + st_gp_mixer_p->iLoopBufLen)*(mixer_loop_block[i]+1)))>>1;
//				gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
//				gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
			}
*/
			gpHalSPU_Clear_FIQ_Status(T_BitEnable[T_mixer_spu_channel[i][0]]|T_BitEnable[T_mixer_spu_channel[i][1]]);
		}
	}
}

/**
* @brief		SPU Initialization
* @param 	none
* @return 	none
*/
void 
SPU_Init (
	void
)
{
	UINT8 ChIndex;

//	MidiPlay_Initialize();//20110426 mask
	gpHalSPU_Set_MainVolume(0x7F);			// main volume (0x00~0x7F)
	gpHalSPU_Clear_FIQ_Status(0xFFFFFFFF);	// chear channel 0 ~ 31 FIQ status
	gpHalSPU_Set_BeatBaseCounter(0x0000);
	gpHalSPU_Set_BeatCounter(0x0000);
	gpHalSPU_Disable_BeatIRQ();
	gpHalSPU_Clear_BeatIRQ_Flag();
	for(ChIndex = 0; ChIndex < 32; ChIndex++)
	{
		gpHalSPU_Disable_Channel(ChIndex);
		gpHalSPU_Clear_Ch_StopFlag(ChIndex);
		gpHalSPU_Disable_FIQ_Channel(ChIndex);
		
		gpHalSPU_Set_EnvelopeClock(0x03, ChIndex);
		gpHalSPU_Set_EnvRampDown(ChIndex);
		gpHalSPU_DisableChannelRepeat(ChIndex);
		gpHalSPU_EnvelopeAutoMode(ChIndex);
		gpHalSPU_DisablePitchBend(ChIndex);//0410
		gpHalSPU_DisableChannelZC(ChIndex);//0410
	}

	gpHalSPU_AccumulatorInit();
	gpHalSPU_Clear_FOF();//0410
	gpHalSPU_SingleChVolumeSelect(0x02);
	gpHalSPU_InterpolationON();//0410
	gpHalSPU_HQ_InterpolationON();//0410
	gpHalSPU_CompressorOFF();

//	gpHalSPU_SetCompressorRatio(0x05);//0410
//	gpHalSPU_DisableCompZeroCrossing();//0410
//	gpHalSPU_SetReleaseTimeScale(0x03);//0410
//	gpHalSPU_SetAttackTimeScale(0x00);//0410
//	gpHalSPU_SetCompressThreshold(0x60);//0410
//	gpHalSPU_SelectPeakMode();//0410

//	gpHalSPU_SetReleaseTime(0xFF);//0410
//	gpHalSPU_SetAttackTime(0x02);//0410
	
	gpHalSPU_SetBankAddr(0x0000);
//	gpHalSPU_SetPost_48khz();//20110509

}	

//void SPU_MIDI_Play(unsigned int *MIDI_Addr, unsigned int PlayType)
//void SPU_MIDI_Play(char repeat_en)
void SPU_MIDI_Play(void)
{
//	int nRet;
//	SPU_MIDI_Stop();
//	spu_struct_p->MIDI_PlayFlag |= STS_MIDI_PLAY;
//	if(PlayType)
//		spu_struct_p->MIDI_PlayFlag |= STS_MIDI_REPEAT_ON;
//	else
//		spu_struct_p->MIDI_PlayFlag &= ~STS_MIDI_REPEAT_ON;
//	spu_struct_p->pMIDI_StartAddr = MIDI_Addr;
//	spu_struct_p->pMIDI_DataPointer = (unsigned short *)MIDI_Addr;
	gpHalSPU_Set_BeatBaseCounter(C_DefaultBeatBase);
	MidiPlay_Initialize();
	gpHalSPU_Set_BeatCounter(0x30);
	gpHalSPU_Clear_BeatIRQ_Flag();
	
	//nRet = request_irq(51,gp_spu_irq_handler,IRQF_DISABLED,"SPU_IRQ",NULL);
//	nRet = request_irq(51,gp_spu_irq_handler,IRQF_DISABLED,"SPU_IRQ",NULL);
//	if(nRet < 0) 
//	   nRet=-1;
	gpHalSPU_Enable_BeatIRQ();
	//VIC_P110_REGISTER (51, 0, SPU_BEAT_ISR);
	//SPU_EnableBeatCountFIQ();//how to implement this????
}

/**
* @brief		SPU PlayTone
* @param 	none
* @return 	none
*/
//void gpHalSpuPlayTone( unsigned char uiPitch, unsigned int *pAddr, unsigned char uiPan, unsigned char uiVelocity, unsigned char uiSPUChannel)

void SPU_PlayTone(SPU_MOUDLE_STRUCT *spu_register_set)
{
	unsigned short Temp;
	unsigned short *pStartAddr;
	unsigned char uiPitch;
	unsigned char uiPan;
	unsigned char uiVelocity;
	unsigned char uiSPUChannel;
	
	uiPitch = spu_register_set->SPU_pitch;
	uiPan = spu_register_set->SPU_pan;
	uiVelocity = spu_register_set->SPU_velocity;
	uiSPUChannel = spu_register_set->SPU_channel;
	
//	uiSPUChannel = 0;
//	Load_ALP_Header(pAddr);
	if(uiPitch == 0xFF)
		uiPitch = spu_register_set->BasePitch;
	gpHalSPU_EnvelopeAutoMode(uiSPUChannel);
	spu_struct_p->uiAddr = ((unsigned int)(spu_register_set->SPU_pAddr + 10) / 2);
	gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
	
	if(spu_register_set->WaveType & 0x0040)	// ADPCM mode
	{
		// ADPCM mode, check!
		gpHalSPU_SetADPCM_Mode(uiSPUChannel);	// set ADPCM mode
		if(spu_register_set->WaveType & 0x0080)	// ADPCM36 mode
		{
			gpHalSPU_SelectADPCM36_Mode(uiSPUChannel);	// set ADPCM36 mode
			gpHalSPU_SetADPCM_PointNumber(31, uiSPUChannel); // set point number = 31
			gpHalSPU_SetToneColorMode(3, uiSPUChannel);// HW auto-repeat mode 1
		}
		else
		{
			gpHalSPU_SelectADPCM_Mode(uiSPUChannel);		// set ADPCM mode
			gpHalSPU_SetADPCM_PointNumber(0, uiSPUChannel);
			gpHalSPU_SetToneColorMode(2, uiSPUChannel);
//			SPU_SetToneColorMode(3, uiSPUChannel);
			spu_struct_p->uiAddr += 1;
			gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
		}
		if(spu_register_set->WaveType & 0x0010)	// check loop mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);	// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);	// set 8-bit PCM mode
	}
	else	// PCM mode
	{
		gpHalSPU_SetPCM_Mode(uiSPUChannel);		// set PCM mode
		if(spu_register_set->WaveType & 0x0010)	// 16-bit PCM mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);	// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);	// set 8-bit PCM mode
		gpHalSPU_SetToneColorMode(2, uiSPUChannel);// HW auto-repeat mode
	}
	gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);
	gpHalSPU_SetWaveData(*(spu_register_set->SPU_vAddr + 10), uiSPUChannel);	// tone color first sample
	
	spu_struct_p->uiAddr = ((unsigned int)spu_register_set->SPU_pAddr)/2 + spu_register_set->LoopStartAddr;
	gpHalSPU_SetLoopAddress(spu_struct_p->uiAddr, uiSPUChannel);
	
	gpHalSPU_SetVelocity(uiVelocity, uiSPUChannel);
	gpHalSPU_SetPan(uiPan, uiSPUChannel);


	spu_struct_p->uiAddr = (unsigned int)spu_register_set->SPU_vAddr + spu_register_set->EnvelopStartAddr * 2;
	spu_struct_p->pTableAddr = (unsigned int *)spu_struct_p->uiAddr;
	pStartAddr = (unsigned short *)spu_struct_p->uiAddr;
	Temp = *pStartAddr;
//	SPU_SetEnvelope_0(*spu_struct_p->pTableAddr & 0xFFFF, uiSPUChannel);
	gpHalSPU_SetEnvelope_0(Temp, uiSPUChannel);
	pStartAddr += 1;
	Temp = *pStartAddr;
//	SPU_SetEnvelope_1(*spu_struct_p->pTableAddr >> 16, uiSPUChannel);
	gpHalSPU_SetEnvelope_1(Temp, uiSPUChannel);
	gpHalSPU_SetEnvelopeData(0x0000, uiSPUChannel);
//	SPU_SetEnvelopeCounter(0x0000, uiSPUChannel);
	gpHalSPU_SetEnvelopeRampDownOffset(spu_register_set->RampDownStep, uiSPUChannel);
	gpHalSPU_SetEnvelopeRepeatAddrOffset(0x0000, uiSPUChannel);

	spu_struct_p->uiAddr = spu_struct_p->uiAddr / 2;
	gpHalSPU_SetEnvelopeAddress(spu_struct_p->uiAddr, uiSPUChannel);
	gpHalSPU_SetWaveData(0x8000, uiSPUChannel);

	spu_struct_p->pTableAddr = (unsigned int *)T_VarPhaseTable;
	spu_struct_p->uiData = *(spu_struct_p->pTableAddr + uiPitch - spu_register_set->BasePitch);
//	spu_struct_p->uiPhaseLow = (spu_struct_p->uiData & 0xFFFF) * ALP_Header.SampleRate;
//	ulData = (spu_struct_p->uiData & 0xFFFF) * ALP_Header.SampleRate;
//	spu_struct_p->uiPhaseHigh = (spu_struct_p->uiData >> 16) * ALP_Header.SampleRate;
//	spu_struct_p->uiPhaseLow = (spu_struct_p->uiPhaseLow + 0x00008000) >> 16;
//	spu_struct_p->uiPhaseHigh += spu_struct_p->uiPhaseLow;
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiData & 0xFFFF) * (spu_register_set->SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseMiddle1 = (spu_struct_p->uiData & 0xFFFF) * (spu_register_set->SampleRate >> 16);
	spu_struct_p->uiPhaseMiddle2 = (spu_struct_p->uiData >> 16) * (spu_register_set->SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseHigh = (spu_struct_p->uiData >> 16) * (spu_register_set->SampleRate >> 16);
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiPhaseLow + 0x00008000) >> 16;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseMiddle2;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseLow;
	spu_struct_p->uiPhaseHigh = spu_struct_p->uiPhaseHigh << 16;
	spu_struct_p->uiPhaseHigh += spu_struct_p->uiPhaseMiddle1;

	gpHalSPU_SetPhase(spu_struct_p->uiPhaseHigh, uiSPUChannel);
	gpHalSPU_SetPhaseAccumulator(0x0000, uiSPUChannel);
	gpHalSPU_SetTargetPhase(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseOffset(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseIncrease(uiSPUChannel);
	gpHalSPU_SetPhaseTimeStep(0x0000, uiSPUChannel);
	gpHalSPU_SetRampDownClock(spu_register_set->RampDownClock, uiSPUChannel);

//	SPU_Enable_Channel(uiSPUChannel);
//	SPU_Clear_Ch_StopFlag(uiSPUChannel);
	
//	SPU_Set_BeatBaseCounter(0x80);
//	SPU_Set_BeatCounter(0x100);
//	gpHalSPU_Enable_BeatIRQ();	
	
}



void SPU_PlayDrum(UINT8 uiDrumIndex, UINT32* pAddr, UINT32 phyAddr, UINT8 uiPan, UINT8 uiVelocity, UINT8 uiSPUChannel)
{
	UINT16 Temp;
	UINT16 *pStartAddr;
	//ggg 0316
	UINT32 tmp_addr;
//	UINT32 *p_temp_addr;
	UINT32 value;
	
	gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(uiSPUChannel);
	Load_ALP_Header(pAddr);
	gpHalSPU_EnvelopeAutoMode(uiSPUChannel);
	
	value = *pAddr;
//	SPU_DBG("drum value = %x \r\n", value);
	
//	tmp_addr = virt_to_phys(pAddr);
//	p_temp_addr = (UINT32*)tmp_addr;
//	printk(KERN_WARNING "phys addr = %x \r\n", tmp_addr);
	
//	spu_struct_p->uiAddr = ((UINT32)(tmp_addr + 10) / 2);
	//tmp_addr = gp_chunk_pa(pAddr);
	tmp_addr = (UINT32)phyAddr;
	spu_struct_p->uiAddr = (tmp_addr + 40)/2;
//	printk(KERN_WARNING "spu_addr = %x \r\n", spu_struct_p->uiAddr);
	//ggg 0316
//	tmp_addr = virt_to_phys((unsigned int*)spu_struct_p->uiAddr);
	//tmp_addr = gp_chunk_pa((unsigned int*)spu_struct_p->uiAddr);
	//spu_struct_p->uiAddr = (UINT32)(tmp_addr / 2);

	//ggg 0316
//	tmp_addr = virt_to_phys((unsigned int*)spu_struct_p->uiAddr);
//	gpHalSPU_SetStartAddress(tmp_addr, uiSPUChannel);
	gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);

	//gpHalSPU_SetADPCM_PointNumber(0x0000, uiSPUChannel);
	gpHal_SPU_ClearADCPM36_Mode(uiSPUChannel);
	
	if((spu_struct_p->ALP_Header).WaveType & 0x0040)	// ADPCM mode
	{
		// ADPCM mode, check!
		gpHalSPU_SetADPCM_Mode(uiSPUChannel);	// set ADPCM mode
		if((spu_struct_p->ALP_Header).WaveType & 0x0080)	// ADPCM36 mode
		{
			gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);
			gpHalSPU_SelectADPCM36_Mode(uiSPUChannel);	// set ADPCM36 mode
			gpHalSPU_SetADPCM_PointNumber(31, uiSPUChannel); // set point number = 31
			gpHalSPU_SetToneColorMode(1, uiSPUChannel);// HW auto-repeat mode 1
		}
		else
		{
			gpHalSPU_SelectADPCM_Mode(uiSPUChannel);		// set ADPCM mode
			gpHalSPU_SetADPCM_PointNumber(0, uiSPUChannel);
			gpHalSPU_SetToneColorMode(1, uiSPUChannel);
			spu_struct_p->uiAddr += 1;
			gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
		}
		if((spu_struct_p->ALP_Header).WaveType & 0x0010)	// check loop mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);	// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);	// set 8-bit PCM mode
	}
	else	// PCM mode
	{
		gpHalSPU_SetPCM_Mode(uiSPUChannel);		// set PCM mode
		if((spu_struct_p->ALP_Header).WaveType & 0x0010)	// 16-bit PCM mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);	// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);	// set 8-bit PCM mode
		gpHalSPU_SetToneColorMode(1, uiSPUChannel);// HW auto-repeat mode
	}
	gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);
	gpHalSPU_SetWaveData(*(pAddr + 10), uiSPUChannel);	// tone color first sample

	gpHalSPU_SetLoopAddress(0x0000, uiSPUChannel);	
	gpHalSPU_SetVelocity(uiVelocity, uiSPUChannel);
	gpHalSPU_SetPan(uiPan, uiSPUChannel);

	spu_struct_p->uiAddr = (UINT32)pAddr + (spu_struct_p->ALP_Header).EnvelopStartAddr * 2;
	tmp_addr = (UINT32)phyAddr + (spu_struct_p->ALP_Header).EnvelopStartAddr * 2;//20110415
	spu_struct_p->pTableAddr = (UINT32 *)spu_struct_p->uiAddr;
	pStartAddr = (UINT16 *)spu_struct_p->uiAddr;
	Temp = *pStartAddr;
	gpHalSPU_SetEnvelope_0(Temp, uiSPUChannel);
	pStartAddr += 1;
	Temp = *pStartAddr;
	gpHalSPU_SetEnvelope_1(Temp, uiSPUChannel);
	gpHalSPU_SetEnvelopeData(0x0000, uiSPUChannel);
	gpHalSPU_SetEnvelopeRampDownOffset((spu_struct_p->ALP_Header).RampDownStep, uiSPUChannel);
	gpHalSPU_SetEnvelopeRepeatAddrOffset(0x0000, uiSPUChannel);

//	spu_struct_p->uiAddr = spu_struct_p->uiAddr / 2;//??
//	//ggg 0316
//	tmp_addr = virt_to_phys((unsigned int*)spu_struct_p->uiAddr);//??
//	gpHalSPU_SetEnvelopeAddress(tmp_addr, uiSPUChannel);//??
	//tmp_addr = gp_chunk_pa((unsigned int*)spu_struct_p->uiAddr);//??
	tmp_addr /= 2;//??
	gpHalSPU_SetEnvelopeAddress(tmp_addr, uiSPUChannel);//??
	//gpHalSPU_SetEnvelopeAddress(spu_struct_p->uiAddr, uiSPUChannel);
	gpHalSPU_SetWaveData(0x8000, uiSPUChannel);

	spu_struct_p->pTableAddr = (UINT32 *)T_VarPhaseTable;
	spu_struct_p->uiData = *spu_struct_p->pTableAddr;
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiData & 0xFFFF) * ((spu_struct_p->ALP_Header).SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseMiddle1 = (spu_struct_p->uiData & 0xFFFF) * ((spu_struct_p->ALP_Header).SampleRate >> 16);
	spu_struct_p->uiPhaseMiddle2 = (spu_struct_p->uiData >> 16) * ((spu_struct_p->ALP_Header).SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseHigh = (spu_struct_p->uiData >> 16) * ((spu_struct_p->ALP_Header).SampleRate >> 16);
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiPhaseLow + 0x00008000) >> 16;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseMiddle2;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseLow;
	spu_struct_p->uiPhaseHigh = spu_struct_p->uiPhaseHigh << 16;
	spu_struct_p->uiPhaseHigh += spu_struct_p->uiPhaseMiddle1;

	gpHalSPU_SetPhase(spu_struct_p->uiPhaseHigh, uiSPUChannel);
	gpHalSPU_SetPhaseAccumulator(0x0000, uiSPUChannel);
	gpHalSPU_SetTargetPhase(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseOffset(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseIncrease(uiSPUChannel);
	gpHalSPU_SetPhaseTimeStep(0x0000, uiSPUChannel);
	gpHalSPU_SetRampDownClock((spu_struct_p->ALP_Header).RampDownClock, uiSPUChannel);

	(spu_struct_p->pSPU_ChInfo + uiSPUChannel)->R_PB_PhaseRecord = spu_struct_p->uiPhaseHigh;

}

void SPU_PlayNote(UINT8 uiPlayPitch, UINT32* pAddr, UINT32 phyAddr, UINT8 uiPan, UINT8 uiVelocity, UINT8 uiSPUChannel)
{
	UINT32 MIDI_Channel;
	UINT32 PitchBendIndex, PitchBendValue;
	UINT32 NewPhase;
	UINT32 *pAddrPB;
	UINT16 Temp;
	UINT16 *pStartAddr;
	
	//ggg 0316
	UINT32 tmp_addr;

	gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(uiSPUChannel);
	Load_ALP_Header(pAddr);
//	SPU_DBG("instrument value = %x \r\n", *pAddr);
	gpHalSPU_EnvelopeAutoMode(uiSPUChannel);
//	printk(KERN_WARNING "addr = %x \r\n", pAddr);
//	spu_struct_p->uiAddr = ((UINT32)(pAddr + 10) / 2);
//	printk(KERN_WARNING "spu_addr = %x \r\n", spu_struct_p->uiAddr);
	//ggg 0316
//	tmp_addr = virt_to_phys((unsigned int*)spu_struct_p->uiAddr);
//	printk(KERN_WARNING "phys_addr = %x \r\n", tmp_addr);
	//tmp_addr = virt_to_phys(pAddr);
	//SPU_DBG("phys addr = 0x%x",(UINT32)tmp_addr);
	//tmp_addr = gp_user_va_to_pa((void*)pAddr);	
	tmp_addr = (UINT32)phyAddr;
//	SPU_DBG("vir_addr = 0x%x\n", (UINT32)pAddr);
//	SPU_DBG("tmp_addr = 0x%x\n", tmp_addr);
//	SPU_DBG("phy_addr = 0x%x\n", (UINT32)phyAddr);
	spu_struct_p->uiAddr = (tmp_addr + 40)/2;
//	printk(KERN_WARNING "spu_addr = %x \r\n", spu_struct_p->uiAddr);
	//ggg 0316
	//tmp_addr = gp_chunk_pa((unsigned int*)spu_struct_p->uiAddr);
	//spu_struct_p->uiAddr = (UINT32)(tmp_addr / 2);

//	gpHalSPU_SetStartAddress(tmp_addr, uiSPUChannel);
	gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
	gpHal_SPU_ClearADCPM36_Mode(uiSPUChannel);
	
	if((spu_struct_p->ALP_Header).WaveType & 0x0040)	// ADPCM mode
	{
		// ADPCM mode, check!
		gpHalSPU_SetADPCM_Mode(uiSPUChannel);	// set ADPCM mode
		if((spu_struct_p->ALP_Header).WaveType & 0x0080)	// ADPCM36 mode
		{
			gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);
			gpHalSPU_SelectADPCM36_Mode(uiSPUChannel);	// set ADPCM36 mode
			gpHalSPU_SetADPCM_PointNumber(31, uiSPUChannel); // set point number = 31
			gpHalSPU_SetToneColorMode(3, uiSPUChannel);// HW auto-repeat mode 1
		}
		else
		{
			gpHalSPU_SelectADPCM_Mode(uiSPUChannel);		// set ADPCM mode
			gpHalSPU_SetADPCM_PointNumber(0, uiSPUChannel);
			gpHalSPU_SetToneColorMode(2, uiSPUChannel);
			spu_struct_p->uiAddr += 1;
			gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
		}
		if((spu_struct_p->ALP_Header).WaveType & 0x0010)	// check loop mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);	// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);	// set 8-bit PCM mode
	}
	else	// PCM mode
	{
		gpHalSPU_SetPCM_Mode(uiSPUChannel);		// set PCM mode
		if((spu_struct_p->ALP_Header).WaveType & 0x0010)	// 16-bit PCM mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);	// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);	// set 8-bit PCM mode
		gpHalSPU_SetToneColorMode(2, uiSPUChannel);// HW auto-repeat mode
	}
	gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);
	gpHalSPU_SetWaveData(*(pAddr + 10), uiSPUChannel);	// tone color first sample
	
//	spu_struct_p->uiAddr = ((UINT32)pAddr)/2 + (spu_struct_p->ALP_Header).LoopStartAddr;

	//ggg 0316
//	tmp_addr = virt_to_phys((unsigned int*)spu_struct_p->uiAddr);
	spu_struct_p->uiAddr = (UINT32)pAddr + (spu_struct_p->ALP_Header).LoopStartAddr * 2;
	tmp_addr = (UINT32)phyAddr + (spu_struct_p->ALP_Header).LoopStartAddr * 2;//20110415
//	SPU_DBG("phy_addr1 = 0x%x\n", tmp_addr);
	tmp_addr /= 2;
	//tmp_addr = gp_chunk_pa((unsigned int*)spu_struct_p->uiAddr);
	//spu_struct_p->uiAddr = tmp_addr/2;
	gpHalSPU_SetLoopAddress(tmp_addr, uiSPUChannel);
	
	gpHalSPU_SetVelocity(uiVelocity, uiSPUChannel);
	gpHalSPU_SetPan(uiPan, uiSPUChannel);


	spu_struct_p->uiAddr = (UINT32)pAddr + (spu_struct_p->ALP_Header).EnvelopStartAddr * 2;
	tmp_addr = (UINT32)phyAddr + (spu_struct_p->ALP_Header).EnvelopStartAddr * 2;//20110415
//	SPU_DBG("phy_addr2 = 0x%x\n", tmp_addr);
	spu_struct_p->pTableAddr = (UINT32 *)spu_struct_p->uiAddr;
	pStartAddr = (UINT16 *)spu_struct_p->uiAddr;
	Temp = *pStartAddr;
//	SPU_SetEnvelope_0(*spu_struct_p->pTableAddr & 0xFFFF, uiSPUChannel);
	gpHalSPU_SetEnvelope_0(Temp, uiSPUChannel);
	pStartAddr += 1;
	Temp = *pStartAddr;
//	SPU_SetEnvelope_1(*spu_struct_p->pTableAddr >> 16, uiSPUChannel);
	gpHalSPU_SetEnvelope_1(Temp, uiSPUChannel);
	gpHalSPU_SetEnvelopeData(0x0000, uiSPUChannel);
	gpHalSPU_SetEnvelopeRampDownOffset((spu_struct_p->ALP_Header).RampDownStep, uiSPUChannel);
	gpHalSPU_SetEnvelopeRepeatAddrOffset(0x0000, uiSPUChannel);

	//spu_struct_p->uiAddr = spu_struct_p->uiAddr / 2;
	//ggg 0316
	//tmp_addr = gp_chunk_pa((unsigned int*)spu_struct_p->uiAddr);
	tmp_addr /= 2;
	//spu_struct_p->uiAddr = tmp_addr/2;
	//gpHalSPU_SetEnvelopeAddress(tmp_addr, uiSPUChannel);	
	gpHalSPU_SetEnvelopeAddress(tmp_addr, uiSPUChannel);
	gpHalSPU_SetWaveData(0x8000, uiSPUChannel);

	spu_struct_p->pTableAddr = (UINT32 *)T_VarPhaseTable;
	spu_struct_p->uiData = *(spu_struct_p->pTableAddr + uiPlayPitch - (spu_struct_p->ALP_Header).BasePitch);

	spu_struct_p->uiPhaseLow = (spu_struct_p->uiData & 0xFFFF) * ((spu_struct_p->ALP_Header).SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseMiddle1 = (spu_struct_p->uiData & 0xFFFF) * ((spu_struct_p->ALP_Header).SampleRate >> 16);
	spu_struct_p->uiPhaseMiddle2 = (spu_struct_p->uiData >> 16) * ((spu_struct_p->ALP_Header).SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseHigh = (spu_struct_p->uiData >> 16) * ((spu_struct_p->ALP_Header).SampleRate >> 16);
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiPhaseLow + 0x00008000) >> 16;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseMiddle2;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseLow;
	spu_struct_p->uiPhaseHigh = spu_struct_p->uiPhaseHigh << 16;
	spu_struct_p->uiPhaseHigh += spu_struct_p->uiPhaseMiddle1;

	(spu_struct_p->pSPU_ChInfo + uiSPUChannel)->R_PB_PhaseRecord = spu_struct_p->uiPhaseHigh;
	MIDI_Channel = (spu_struct_p->MIDI_Event).NoteEvent.ChannelNumber;
	PitchBendIndex = (spu_struct_p->pMIDI_ChInfo + MIDI_Channel)->R_MIDI_CH_PitchBend;
	pAddrPB = (UINT32 *)((spu_struct_p->pMIDI_ChInfo + MIDI_Channel)->R_PB_TABLE_Addr + PitchBendIndex);
	PitchBendValue = *pAddrPB;

	NewPhase = (PitchBendValue * spu_struct_p->uiPhaseHigh) >> 14;

	gpHalSPU_SetPhase(NewPhase, uiSPUChannel);
	gpHalSPU_SetPhaseAccumulator(0x0000, uiSPUChannel);
	gpHalSPU_SetTargetPhase(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseOffset(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseIncrease(uiSPUChannel);
	gpHalSPU_SetPhaseTimeStep(0x0000, uiSPUChannel);
	gpHalSPU_SetRampDownClock((spu_struct_p->ALP_Header).RampDownClock, uiSPUChannel);
}
/*
UINT32 Calculate_Pan(UINT32 SPU_Ch)
{
	UINT32 uiPan, uiVelo;
	UINT32 MIDI_Ch;
	UINT32 Temp1, Temp2;
	
	MIDI_Ch = (spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_CH_MAP;
	Temp1 = (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_PAN;
	Temp2 = (spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_ToneLib_PAN;

	if(Temp1 < 65)
	{
		// Left Pan
		uiPan = (Temp1 * Temp2) >> 6;
	}
	else
	{
		// Right Pan
		uiPan = 127 - (((127 - Temp1) * (127 - Temp2)) >> 6);
		if(uiPan > 127)
			uiPan = 127;
	}

//	uiPan = (Temp1 * Temp2) >> 7;
	Temp1 = (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_EXPRESSION;
	Temp2 = (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_VOLUME;
	Temp1 = (Temp1 * Temp2) >> 7;
	Temp2 = (spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_NOTE_VELOCITY;
	Temp1 = (Temp1 * Temp2) >> 7;
	Temp2 = (spu_struct_p->pSPU_ChInfo + SPU_Ch)->R_MIDI_ToneLib_Volume;
	uiVelo = (Temp1 * Temp2) >> 7;
	return ((uiPan << 8) | uiVelo);
}*/
UINT32 Calculate_TempPan(UINT32 Pan1, UINT32 Pan2)
{
	UINT32 ret;
	
	if(Pan1 == 0)
	{
		if(Pan2 == 127)
			ret = 64;
		else
			ret = 0;
		return ret;
	}
	if(Pan2 == 0)
	{
		if(Pan1 == 127)
			ret = 64;
		else
			ret = 0;
		return ret;
	}
	if(Pan1 == 127)
	{
		ret = 127;
		return ret;
	}
	if(Pan2 == 127)
	{
		ret = 127;
		return ret;
	}
	if(Pan1 < 64)
	{
		if(Pan2 < 64)
		{
			ret = (Pan1 * Pan2) >> 6; 
		}
		else
		{
			ret = 127 - (((127 - Pan1) * (127 - Pan2)) >> 6);
		}
	}
	else
	{
		if(Pan2 < 64)
		{
			ret = 127 - (((127 - Pan1) * (127 - Pan2)) >> 6);
		}
		else
		{
			ret = (Pan1 * Pan2) >> 6; 
		}
	}
	return ret;
}

UINT32 Calculate_Pan(UINT32 SPU_Ch)
{
	UINT32 uiPan, uiVelo;
	UINT32 MIDI_Ch;
	UINT32 Temp1, Temp2;
	
//	DEBUG("Calculate Pan Entry \n");
	//spu_ch_info_ptr = &(spu_struct_p->pSPU_ChInfo);
	MIDI_Ch = ((spu_struct_p->pSPU_ChInfo) + SPU_Ch)->R_MIDI_CH_MAP;
	//MIDI_Ch = (spu_ch_info_ptr + SPU_Ch)->R_MIDI_CH_MAP;
//	DEBUG("MIDI_Ch = %x \n" ,MIDI_Ch);
	if(MIDI_Ch > 16)//081010
		MIDI_Ch = 0;
	//--------------------------------------------------------------------------080808
	if(spu_struct_p->R_MIDI_Pan == 0)
	{
		uiPan = 0;
	}
	else
		if(spu_struct_p->R_MIDI_Pan == 0x7f)
		{
			uiPan = 0x7f;
		}
		else
		{
			Temp1 = ((spu_struct_p->pMIDI_ChInfo) + MIDI_Ch)->R_MIDI_CH_PAN;
			Temp2 = ((spu_struct_p->pSPU_ChInfo) + SPU_Ch)->R_MIDI_ToneLib_PAN;
			Temp1 = Calculate_TempPan(Temp1,Temp2);
			uiPan = Calculate_TempPan(Temp1,spu_struct_p->R_MIDI_Pan);
//			DEBUG("uiPan = %x \n" ,uiPan);	
		}
	//--------------------------------------------------------------------------
	Temp1 = ((spu_struct_p->pMIDI_ChInfo) + MIDI_Ch)->R_MIDI_CH_EXPRESSION;
	Temp2 = ((spu_struct_p->pMIDI_ChInfo) + MIDI_Ch)->R_MIDI_CH_VOLUME;
	Temp1 = (Temp1 * Temp2) >> 7;
	Temp2 = ((spu_struct_p->pSPU_ChInfo) + SPU_Ch)->R_NOTE_VELOCITY;
	Temp1 = (Temp1 * Temp2) >> 7;
	Temp2 = ((spu_struct_p->pSPU_ChInfo) + SPU_Ch)->R_MIDI_ToneLib_Volume;
	//--------------------------------------------------------------------------
	Temp1 = (Temp1 * spu_struct_p->R_MIDI_Volume) >> 7;
	//--------------------------------------------------------------------------
	uiVelo = ((Temp1 * Temp2) >> 7) & 0x007f;
	return ((uiPan << 8) | uiVelo);
}
/*
sw_spu_init()
{
	R_MIDI_MAIN_VOL= (spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_MAIN_VOL;
	
	(spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_EXPRESSION= st_spu_midi->(spu_struct_p->pMIDI_ChInfo + MIDI_Ch)->R_MIDI_CH_EXPRESSION;
	
	
}
*/
	
/**
* @brief		spu find empty channel
* @param 	none
* @return 	empty Channel
*/
/*
UINT32 
FindEmptyChannel(
	void
)
{
	UINT32 Temp;
	UINT32 ChannelIndex;
	UINT32 EmptyChannel;
	UINT32 SPU_IdleChannel;
	
	SPU_IdleChannel = gpHalSPU_GetChannelStatus();
	SPU_IdleChannel |= spu_struct_p->R_CH_OneBeat;
	SPU_IdleChannel ^= 0xFFFFFFFF;
	SPU_IdleChannel &= spu_struct_p->R_MIDI_CH_MASK;
	Temp = 0x00000001;
	for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
	{
		if(SPU_IdleChannel & Temp)
			break;
		else
		{
			Temp = Temp << 1;
		}
	}
	EmptyChannel = ChannelIndex;
	if(EmptyChannel < SPU_ChannelNumber)	// channel found
	{
		if(R_NoteOnHistPtr >= R_Avail_Voice)
		{
			for(Temp = 0; Temp < R_Avail_Voice - 1; Temp++)
			{
				(spu_struct_p->pSPU_ChInfo + Temp)->R_NoteOnHist = (spu_struct_p->pSPU_ChInfo + Temp + 1)->R_NoteOnHist;
			}
			R_NoteOnHistPtr = R_Avail_Voice - 1;
		}
		(spu_struct_p->pSPU_ChInfo + R_NoteOnHistPtr)->R_NoteOnHist = EmptyChannel;
		R_NoteOnHistPtr += 1;
	}
	else	// all channels are busy
	{		// search note off channel
		Temp = 0x00000001;
		for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
		{
			if(Temp & spu_struct_p->R_MIDI_CH_MASK)
			{
				if((spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_CH_MAP == 0xFFFF)
					break;
			}
			Temp = Temp << 1;
		}
		if(ChannelIndex == SPU_ChannelNumber)	// no note off channel
		{
			// search the same instrument channel
			for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
			{
				Temp = (spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_NoteOnHist;
				if((spu_struct_p->pSPU_ChInfo + Temp)->R_MIDI_CH_MAP == (spu_struct_p->MIDI_Event).NoteEvent.ChannelNumber)
				{
					break;
				}
			}
		}

		if(ChannelIndex == SPU_ChannelNumber)
		{
			// no the same instrument channel
			ChannelIndex = SPU_ChannelNumber - 1;
		}
		for(Temp = ChannelIndex; Temp < R_Avail_Voice - 1; Temp++)
		{
			(spu_struct_p->pSPU_ChInfo + Temp)->R_NoteOnHist = (spu_struct_p->pSPU_ChInfo + Temp + 1)->R_NoteOnHist;
		}
		R_NoteOnHistPtr = Temp;
		
		(spu_struct_p->pSPU_ChInfo + R_NoteOnHistPtr)->R_NoteOnHist = ChannelIndex;
		R_NoteOnHistPtr += 1;
		// stop channel
		EmptyChannel = ChannelIndex;
		SPU_StopChannel(EmptyChannel);
	}
	
	(spu_struct_p->pSPU_ChInfo + EmptyChannel)->R_MIDI_CH_MAP = 0xFFFF;
	return EmptyChannel;
}
*/


UINT8 find_channel(UINT8 ch)//查找数组中是否有ch这个值
{
	UINT8 i;
	for(i=0;i<SPU_ChannelNumber;i++)
	{
		if(spu_struct_p->T_channel[i] == ch)
			break;
	}
	if(i<SPU_ChannelNumber)
		return i;
	else
		return 0xff;
}

void delete_channel(UINT8 index)//将数组中第index个单元删除，且将后面的单元往前移，最后一个单元补0xff值
{
	UINT8 i;
	for(i=index;i<SPU_ChannelNumber-1;i++)
	{
		spu_struct_p->T_channel[i] = spu_struct_p->T_channel[i+1];
	}
	spu_struct_p->T_channel[SPU_ChannelNumber-1] = 0xff;
}

void insert_channel(UINT8 ch)//在数组的最开始处插入一个通道
{
	UINT8 temp,i;
	temp = find_channel(ch);
	if(temp != 0xff)//在数组中有该channel
		delete_channel(temp);//删除数组中的第temp个单元
	for(i=SPU_ChannelNumber-1;i>0;i--)
	{
		spu_struct_p->T_channel[i] = spu_struct_p->T_channel[i-1];//将数组往后移一个单元，将第一个单元空出来
	}
	spu_struct_p->T_channel[0] = ch;
}

UINT8 get_oldest_channel(void)
{
	SINT8 ch;
	for(ch=SPU_ChannelNumber-1;ch>=0;ch--)
	{
		if(spu_struct_p->T_channel[ch] != 0xff)
			break;
	}
	if(ch >= 0)
		return spu_struct_p->T_channel[ch];	
	else
		return 0;//数组全为空，强制返回通道0
}

UINT32 FindEmptyChannel(void)
{
	UINT32 Temp;
	UINT32 ChannelIndex;
	UINT32 SPU_IdleChannel;
	
	SPU_IdleChannel = gpHalSPU_GetChannelStatus();
	SPU_IdleChannel |= spu_struct_p->R_CH_OneBeat;
	SPU_IdleChannel ^= 0xFFFFFFFF;
	SPU_IdleChannel &= spu_struct_p->R_MIDI_CH_MASK;
	
	if(SPU_IdleChannel)
	{
		Temp = 0x00000001;
		for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
		{
			if(SPU_IdleChannel & Temp)
				break;
			else
			{
				Temp = Temp << 1;
			}
		}
		insert_channel((UINT8)ChannelIndex);
	}
	else	// all channels are busy
	{
		ChannelIndex = get_oldest_channel();
		insert_channel((UINT8)ChannelIndex);
	}
	SPU_StopChannel(ChannelIndex);
	return ChannelIndex;
}



/**
* @brief		SPU Check Duration
* @param 	none
* @return 	none
	  */
void F_CheckDuration(void)
{
	UINT16 MinDuration;
	
	UINT32 Ch;
	SINT32 Temp;
	UINT32 *pAddr;
	MinDuration = 0x0000;

	for(Ch = 0; Ch < SPU_ChannelNumber + 1; Ch++)
	{
		if(spu_struct_p->R_DUR_Tone[Ch] != 0x0000)
		{
			if(MinDuration == 0x0000)
				MinDuration = spu_struct_p->R_DUR_Tone[Ch];
			else
			{
				if(spu_struct_p->R_DUR_Tone[Ch] < MinDuration)
					MinDuration = spu_struct_p->R_DUR_Tone[Ch];
			}
		}
	}
	spu_struct_p->R_CH_NoteOff = 0x0000;
	for(Ch = 0; Ch < SPU_ChannelNumber + 1; Ch++)
	{
		if(spu_struct_p->R_DUR_Tone[Ch] != 0x0000)
		{
			Temp = (SINT32)(spu_struct_p->R_DUR_Tone[Ch] - MinDuration);
			if(Temp <= 0)
			{
				if(Ch < SPU_ChannelNumber)
				{
					Temp = 0;
					pAddr = (UINT32 *)T_BitEnable;
					spu_struct_p->R_CH_NoteOff |= *(pAddr + Ch);
				}
			}
			spu_struct_p->R_DUR_Tone[Ch] = Temp;
		}
	}
	spu_struct_p->R_CH_NoteOff &= spu_struct_p->R_MIDI_CH_MASK;
	gpHalSPU_Set_BeatCounter(MinDuration);
	//SPU_DBG("BeatCounter = %d\n" ,MinDuration);
}

/**
* @brief		SPU ProcessPitchBendEvent
* @param 	none
* @return 	none
	  */
void ProcessPitchBendEvent(void)
{
	UINT32 MIDIChannel, PitchBendIndex;
	UINT32 PitchBendValue;
	UINT32 ChannelIndex;
	UINT32 NewPhase;
	UINT32 *pAddr;
	
	MIDIChannel = (spu_struct_p->MIDI_Event).PitchBendEvent.ChannelNumber;
	PitchBendIndex = (spu_struct_p->MIDI_Event).PitchBendEvent.PitchBendValue;
	(spu_struct_p->pMIDI_ChInfo + MIDIChannel)->R_MIDI_CH_PitchBend = PitchBendIndex;
	pAddr = (UINT32 *)(spu_struct_p->pMIDI_ChInfo + MIDIChannel)->R_PB_TABLE_Addr;
	PitchBendValue = *(pAddr + PitchBendIndex);
	for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
	{
		if((spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_CH_MAP == MIDIChannel)
		{
			NewPhase = (PitchBendValue * (spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_PB_PhaseRecord) >> 14;
			gpHalSPU_SetPhase(NewPhase, ChannelIndex);
		}
	}
}

/**
* @brief		SPU ProcessControlEvent
* @param 	none
* @return 	none
	  */
void ProcessControlEvent(void)
{
	UINT32 ChannelNumber, ControlNumber, ControlValue;
	UINT32 ChannelIndex;
	UINT32 PanWord;
	UINT32 *pAddr;
	
	ChannelNumber = (spu_struct_p->MIDI_Event).ControlEvent.ChannelNumber;
	ControlNumber = (spu_struct_p->MIDI_Event).ControlEvent.ControlNumber;
	ControlValue = (spu_struct_p->MIDI_Event).ControlEvent.ControlValue;
	switch(ControlNumber)
	{
		case C_VolumeEvent:
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_MIDI_CH_VOLUME = ControlValue;
			for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
			{
				if((spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_CH_MAP == ChannelNumber)
				{
					PanWord = Calculate_Pan(ChannelIndex);
					gpHalSPU_SetVelocity(PanWord & 0x00FF, ChannelIndex);
					gpHalSPU_SetPan(PanWord >> 8, ChannelIndex);
				}
			}
			break;
		
		case C_PanEvent:
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_MIDI_CH_PAN = ControlValue;
			for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
			{
				if((spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_CH_MAP == ChannelNumber)
				{
					PanWord = Calculate_Pan(ChannelIndex);
					gpHalSPU_SetVelocity(PanWord & 0x00FF, ChannelIndex);
					gpHalSPU_SetPan(PanWord >> 8, ChannelIndex);
				}
			}
			break;
		
		case C_ExpressionEvent:
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_MIDI_CH_EXPRESSION = ControlValue;
			for(ChannelIndex = 0; ChannelIndex < SPU_ChannelNumber; ChannelIndex++)
			{
				if((spu_struct_p->pSPU_ChInfo + ChannelIndex)->R_MIDI_CH_MAP == ChannelNumber)
				{
					PanWord = Calculate_Pan(ChannelIndex);
					gpHalSPU_SetVelocity(PanWord & 0x00FF, ChannelIndex);
					gpHalSPU_SetPan(PanWord >> 8, ChannelIndex);
				}
			}
			break;
		
		case C_RPN_LSB_Event:
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_ReceiveFlag |= 0x0F00;
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_DATA &= 0xFF00;
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_DATA |= ControlValue;
			break;
			
		case C_RPN_MSB_Event:
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_ReceiveFlag |= 0xF000;
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_DATA &= 0x00FF;
			(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_DATA |= ControlValue;
			break;
		
		case C_DataEntryEvent:
			if((spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_ReceiveFlag == 0xFF00)
			{
				(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_ReceiveFlag = 0x0000;
				if((spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_RPN_DATA == 0x0000)
				{
					if(ControlValue <= D_PITCH_BEND_TABLE_TOTAL)
					{
						switch(ControlValue)
						{
							case 1:
								pAddr = (UINT32 *)T_PicthWheelTable;
								break;
							case 2:
								pAddr = (UINT32 *)T_PicthWheelTable_TWO;
								break;
							case 3:
								pAddr = (UINT32 *)T_PicthWheelTable_THREE;
								break;
							case 4:
								pAddr = (UINT32 *)T_PicthWheelTable_FOUR;
								break;
							case 5:
								pAddr = (UINT32 *)T_PicthWheelTable_FIVE;
								break;
							case 6:
								pAddr = (UINT32 *)T_PicthWheelTable_SIX;
								break;
							case 7:
								pAddr = (UINT32 *)T_PicthWheelTable_SEVEN;
								break;
							case 8:
								pAddr = (UINT32 *)T_PicthWheelTable_EIGHT;
								break;
							case 9:
								pAddr = (UINT32 *)T_PicthWheelTable_NINE;
								break;
							case 10:
								pAddr = (UINT32 *)T_PicthWheelTable_TEN;
								break;
							case 11:
								pAddr = (UINT32 *)T_PicthWheelTable_ELEVEN;
								break;
							case 12:
								pAddr = (UINT32 *)T_PicthWheelTable_TWELVE;
								break;
							case 13:
								pAddr = (UINT32 *)T_PicthWheelTable_THIRTEEN;
								break;
							default:
								pAddr = (UINT32 *)T_PicthWheelTable_TWO;
								break;
						}
						(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_PB_TABLE_Addr = pAddr;
					}
				}
			}
			break;
	}
}

/**
* @brief		SPU ProcessProgramChangeEvent
* @param 	none
* @return 	none
	  */
void ProcessProgramChangeEvent(void)
{
	UINT32 ChannelNumber,temp;

	ChannelNumber = (spu_struct_p->MIDI_Event).ProgramChangeEvent.ChannelNumber;
	temp = (spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_ChannelInst;
	temp &= 0xffff0000;
	temp |= (spu_struct_p->MIDI_Event).ProgramChangeEvent.InstrumentIndex & 0x0000ffff;
	(spu_struct_p->pMIDI_ChInfo + ChannelNumber)->R_ChannelInst = temp;
}


/**
* @brief		SPU ProcessTempoEvent
* @param 	none
* @return 	none
	  */
void ProcessTempoEvent(void)
{
	UINT16 Temp;
	
	Temp = C_DefaultBeatBase;
	gpHalSPU_Set_BeatBaseCounter(Temp);
}

/**
* @brief		SPU ProcessEndEvent
* @param 	none
* @return 	none
	  */
void ProcessEndEvent(void)
{
	UINT32 ch;
	for(ch=0; ch<SPU_ChannelNumber; ch++)
	{
		SPU_StopChannel(ch);
	}
	MidiPlay_Initialize();
	if(spu_struct_p->MIDI_PlayFlag & STS_MIDI_CALLBACK)
		(*spu_struct_p->MIDI_StopCallBack)();
	if(spu_struct_p->MIDI_PlayFlag & STS_MIDI_REPEAT_ON)
	{
		//spu_struct_p->pMIDI_DataPointer = (UINT16 *)spu_struct_p->pMIDI_StartAddr;
		spu_struct_p->pMIDI_DataPointer = (UINT8*)(spu_struct_p->midi_ring_buffer_addr + spu_struct_p->static_midi_ring_buffer_ri);
		gpHalSPU_Set_BeatCounter(0x0000);
		gpHalSPU_Clear_BeatIRQ_Flag();
		gpHalSPU_Enable_BeatIRQ();	
	}
	else
	{
		spu_struct_p->MIDI_PlayFlag &= ~STS_MIDI_PLAY;
		spu_struct_p->R_MIDI_EndFlag = 0xFFFF;
		gpHalSPU_Set_BeatCounter(0x0100);
		gpHalSPU_Clear_BeatIRQ_Flag();
		gpHalSPU_Enable_BeatIRQ();	
		gpHalSPU_Set_BeatCounter(0x0000);
	}
} 

/**
* @brief		SPU ProcessBeatCountEvent
* @param 	none
* @return 	none
	  */
void ProcessBeatCountEvent(void)
{
	UINT32 MinDuration;

	MinDuration = 0x0000;
	spu_struct_p->R_DUR_Tone[SPU_ChannelNumber] = (spu_struct_p->MIDI_Event).BeatCountEvent.BeatCountValue;

	F_CheckDuration();
	gpHalSPU_Enable_MultiChannel(spu_struct_p->R_CH_OneBeat);
	gpHalSPU_Clear_MultiCh_StopFlag(spu_struct_p->R_CH_OneBeat);
	spu_struct_p->R_CH_OneBeat = 0x0000;
} 

void SPU_MIDI_Pause(void)
{
	UINT32 SPU_ChannelIndex;
	UINT32 Temp;
	
	if(spu_struct_p->MIDI_PlayFlag & STS_MIDI_PLAY)
	{
		R_BeatCnt_temp = gpHalSPU_Get_BeatCounter();		
		gpHalSPU_Disable_BeatIRQ();		
		gpHalSPU_Clear_BeatIRQ_Flag();
		//gpHalSPU_Set_BeatCounter(0x00);
		spu_struct_p->R_CH_NoteOff = 0x00000000;//0x0000;
		Temp = spu_struct_p->R_MIDI_CH_MASK;
		for(SPU_ChannelIndex = 0; SPU_ChannelIndex < SPU_ChannelNumber; SPU_ChannelIndex++)
		{
			if(Temp & 0x0001)
			{
				SPU_StopChannel(SPU_ChannelIndex);
			}
			Temp = Temp >> 1;
		}
		spu_struct_p->MIDI_PlayFlag |= STS_MIDI_PAUSE_ON;		
	}
}

void SPU_MIDI_Resume(void)
{
	if(spu_struct_p->MIDI_PlayFlag & STS_MIDI_PAUSE_ON)
	{
		spu_struct_p->MIDI_PlayFlag &= ~STS_MIDI_PAUSE_ON;
		if(spu_struct_p->MIDI_PlayFlag & STS_MIDI_PLAY)
		{
			gpHalSPU_Set_BeatCounter(R_BeatCnt_temp);
			gpHalSPU_Clear_BeatIRQ_Flag();
			gpHalSPU_Enable_BeatIRQ();
		}
	}
}

void SPU_MIDI_Set_MIDI_Volume(void)
{
	UINT32 temp,spu_channel;
	UINT32 PanWord;
//	spu_struct_p->R_MIDI_Volume = (UINT8)MIDI_Volume;
//	if(spu_struct_p->R_MIDI_Volume > 127)
//		spu_struct_p->R_MIDI_Volume = 127;
	for(temp = 0x0001,spu_channel = 0; spu_channel < SPU_ChannelNumber; spu_channel++,temp <<= 1)
	{	
		if(temp & spu_struct_p->R_MIDI_CH_MASK)
		{
			PanWord = Calculate_Pan(spu_channel);
			gpHalSPU_SetVelocity( (UINT8)(PanWord&0x007f), (UINT8)spu_channel );
			gpHalSPU_SetPan((UINT8)((PanWord>>8)&0x007f), (UINT8)spu_channel );
		}
	}
}

void SPU_MIDI_Set_MIDI_Pan(void)//080808 tangqt
{
	UINT32 temp,spu_channel;
	UINT32 PanWord;
//	spu_struct_p->R_MIDI_Pan = (UINT8)MIDI_Pan;
//	if(spu_struct_p->R_MIDI_Pan > 127)
//		spu_struct_p->R_MIDI_Pan = 127;
	for(temp = 0x0001,spu_channel = 0; spu_channel < SPU_ChannelNumber; spu_channel++,temp <<= 1)
	{
		if(temp & spu_struct_p->R_MIDI_CH_MASK)
		{
			PanWord = Calculate_Pan(spu_channel);
			gpHalSPU_SetVelocity( (UINT8)(PanWord&0x007f), (UINT8)spu_channel );
			gpHalSPU_SetPan((UINT8)((PanWord>>8)&0x007f), (UINT8)spu_channel );
		}
	}
}

void SPU_pause_channel(UINT8 spu_channel)
{
	UINT32 temp;
	temp = gpHalSPU_ReadPhase(spu_channel);
	if(temp != 0)
	{
		//spu_struct_p->R_channel_original_phase[spu_channel] = temp;
		spu_struct.R_channel_original_phase[spu_channel] = temp;
		gpHalSPU_SetPhase(0, spu_channel);
	}
}
void SPU_resume_channel(UINT8 spu_channel)
{
	UINT32 temp;
	temp = gpHalSPU_ReadPhase(spu_channel);
	//if((temp == 0) && (spu_struct_p->R_channel_original_phase[spu_channel]!=0))
	if((temp == 0) && (spu_struct.R_channel_original_phase[spu_channel]!=0))
	{
		//gpHalSPU_SetPhase(spu_struct_p->R_channel_original_phase[spu_channel], spu_channel);
		//spu_struct_p->R_channel_original_phase[spu_channel] = 0;
		gpHalSPU_SetPhase(spu_struct.R_channel_original_phase[spu_channel], spu_channel);
		spu_struct.R_channel_original_phase[spu_channel] = 0;
	}
}

void SPU_pause_two_channel(UINT8 spu_channel0, UINT8 spu_channel1)
{
	UINT32 temp0,temp1;
	temp0 = gpHalSPU_ReadPhase(spu_channel0);
	temp1 = gpHalSPU_ReadPhase(spu_channel1);
	if((temp0 != 0) || (temp1 != 0))
	{
		spu_struct.R_channel_original_phase[spu_channel0] = temp0;
		spu_struct.R_channel_original_phase[spu_channel1] = temp1;
		gpHalSPU_Set_two_channel_Phase(0, spu_channel0, 0, spu_channel1);
	}
}
void SPU_resume_two_channel(UINT8 spu_channel0, UINT8 spu_channel1)
{
	UINT32 temp0,temp1;
	temp0 = gpHalSPU_ReadPhase(spu_channel0);
	temp1 = gpHalSPU_ReadPhase(spu_channel1);
	if( ((temp0 == 0) && (spu_struct.R_channel_original_phase[spu_channel0]!=0))
	  || ((temp1 == 0) && (spu_struct.R_channel_original_phase[spu_channel1]!=0))
	)
	{
		gpHalSPU_Set_two_channel_Phase(spu_struct.R_channel_original_phase[spu_channel0], spu_channel0, spu_struct.R_channel_original_phase[spu_channel1], spu_channel1);
		spu_struct.R_channel_original_phase[spu_channel0] = 0;
		spu_struct.R_channel_original_phase[spu_channel1] = 0;
	}
}

void SPU_PlayPCM_NoEnv_FixCH(UINT32 *pAddr, UINT32 phyAddr, UINT8 uiPan, UINT8 uiVelocity, UINT8 uiSPUChannel)//20110503
{
	UINT8  temp_BeatIRQ_Enable_Flag;
	UINT16 temp_Beatcnt;
	
	if(gpHalSPU_Get_SingleChannel_Status(uiSPUChannel))
		SPU_StopChannel(uiSPUChannel);
		
	temp_BeatIRQ_Enable_Flag = gpHalSPU_Get_BeatIRQ_Enable_Flag();
	gpHalSPU_Disable_BeatIRQ();
	temp_Beatcnt = gpHalSPU_Get_BeatCounter();//20110503
	
	gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(uiSPUChannel);//090527
	Load_ALP_Header(pAddr);
	gpHalSPU_EnvelopeManualMode((UINT8)uiSPUChannel);	//set Manual Mode
	spu_struct_p->uiAddr = ((UINT32)(phyAddr + 40) / 2);
	gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
	gpHal_SPU_ClearADCPM36_Mode(uiSPUChannel);			// 20080521 Roy
	
	if((spu_struct_p->ALP_Header).WaveType & 0x0040)				// ADPCM mode
	{
		// ADPCM mode, check!
		gpHalSPU_SetADPCM_Mode(uiSPUChannel);			// set ADPCM mode
		if((spu_struct_p->ALP_Header).WaveType & 0x0080)			// ADPCM36 mode
		{
			gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);	// 20080521 Roy
			gpHalSPU_SelectADPCM36_Mode(uiSPUChannel);		// set ADPCM36 mode
			gpHalSPU_SetADPCM_PointNumber(31, uiSPUChannel); // set point number = 31
			gpHalSPU_SetToneColorMode(1, uiSPUChannel);		// HW auto-end mode
		}
		else
		{
			gpHalSPU_SelectADPCM_Mode(uiSPUChannel);			// set ADPCM mode
			gpHalSPU_SetADPCM_PointNumber(0, uiSPUChannel);
			gpHalSPU_SetToneColorMode(1, uiSPUChannel);
			spu_struct_p->uiAddr += 1;
			gpHalSPU_SetStartAddress(spu_struct_p->uiAddr, uiSPUChannel);
		}
		if((spu_struct_p->ALP_Header).WaveType & 0x0010)				// check loop mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);			// set 16-bit PCM mode
		}
		else
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);			// set 8-bit PCM mode
	}
	else	// PCM mode
	{
		gpHalSPU_SetPCM_Mode(uiSPUChannel);					// set PCM mode
		if((spu_struct_p->ALP_Header).WaveType & 0x0010)				// 16-bit PCM mode
		{
			gpHalSPU_Set_16bit_Mode(uiSPUChannel);			// set 16-bit PCM mode
		}
		else
		{
			gpHalSPU_Set_8bit_Mode(uiSPUChannel);			// set 8-bit PCM mode
		}
		gpHalSPU_SetToneColorMode(1, uiSPUChannel);			// HW auto-end mode
	}
	gpHalSPU_SetWaveData_0(0x8000, uiSPUChannel);
	gpHalSPU_SetWaveData(*(pAddr + 10), uiSPUChannel);		// tone color first sample
	
	gpHalSPU_SetLoopAddress(0x0000, uiSPUChannel);
	gpHalSPU_SetVelocity(uiVelocity, uiSPUChannel);
	gpHalSPU_SetPan(uiPan, uiSPUChannel);
	
	// Play With No Envelop 
	gpHalSPU_SetEnvelope_0(0, uiSPUChannel);
	gpHalSPU_SetEnvelope_1(0, uiSPUChannel);
	gpHalSPU_SetEnvelopeData(0x007F, uiSPUChannel);
	gpHalSPU_SetEnvelopeRampDownOffset((spu_struct_p->ALP_Header).RampDownStep, uiSPUChannel);
	gpHalSPU_SetEnvelopeRepeatAddrOffset(0x0000, uiSPUChannel);
	
	gpHalSPU_SetEnvelopeAddress(0, uiSPUChannel);
	//gpHalSPU_SetWaveData(0x8000, uiSPUChannel);

	spu_struct_p->pTableAddr = (UINT32 *)T_VarPhaseTable;
	spu_struct_p->uiData = *spu_struct_p->pTableAddr;
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiData & 0xFFFF) * ((spu_struct_p->ALP_Header).SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseMiddle1 = (spu_struct_p->uiData & 0xFFFF) * ((spu_struct_p->ALP_Header).SampleRate >> 16);
	spu_struct_p->uiPhaseMiddle2 = (spu_struct_p->uiData >> 16) * ((spu_struct_p->ALP_Header).SampleRate & 0xFFFF);
	spu_struct_p->uiPhaseHigh = (spu_struct_p->uiData >> 16) * ((spu_struct_p->ALP_Header).SampleRate >> 16);
	spu_struct_p->uiPhaseLow = (spu_struct_p->uiPhaseLow + 0x00008000) >> 16;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseMiddle2;
	spu_struct_p->uiPhaseMiddle1 += spu_struct_p->uiPhaseLow;
	spu_struct_p->uiPhaseHigh = spu_struct_p->uiPhaseHigh << 16;
	spu_struct_p->uiPhaseHigh += spu_struct_p->uiPhaseMiddle1;
	
	gpHalSPU_SetPhase(spu_struct_p->uiPhaseHigh, uiSPUChannel);
	gpHalSPU_SetPhaseAccumulator(0x0000, uiSPUChannel);
	gpHalSPU_SetTargetPhase(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseOffset(0x0000, uiSPUChannel);
	gpHalSPU_SetPhaseIncrease(uiSPUChannel);
	gpHalSPU_SetPhaseTimeStep(0x0000, uiSPUChannel);
	gpHalSPU_SetRampDownClock((spu_struct_p->ALP_Header).RampDownClock, uiSPUChannel);

	gpHalSPU_Enable_Channel(uiSPUChannel);
	gpHalSPU_Clear_Ch_StopFlag(uiSPUChannel);
	
	gpHalSPU_Set_BeatCounter(temp_Beatcnt);
	
	if(temp_BeatIRQ_Enable_Flag)	//080805 tangqt
	{
		gpHalSPU_Enable_BeatIRQ();
	}
}
//---------------------------------------------------------------------------------------------------------mixer
void MIXER_SPU_CH_Init(UINT8 spu_channel)
{
	gpHalSPU_Disable_Channel(spu_channel);//20110417
	gpHalSPU_Clear_FIQ_Status(T_BitEnable[spu_channel]);//20110427
//	gpHalSPU_CLEAR_CHANNEL_ATT_SRAM(spu_channel);//20110421 can't call this func
	gpHalSPU_EnvelopeManualMode(spu_channel);
	gpHal_SPU_ClearADCPM36_Mode(spu_channel);
	gpHalSPU_SetPCM_Mode(spu_channel);
	gpHalSPU_Set_16bit_Mode(spu_channel);
	gpHalSPU_SetToneColorMode(2, spu_channel);
	gpHalSPU_SetWaveData_0(0x8000, spu_channel);
	gpHalSPU_SetWaveData(0x8000, spu_channel);
	//gpHalSPU_SetVelocity(0x7f, spu_channel);
	//gpHalSPU_SetPan(0x3f, spu_channel);
	gpHalSPU_SetEnvelope_0(0x0000, spu_channel);
	gpHalSPU_SetEnvelope_1(0x0000, spu_channel);
	gpHalSPU_SetEnvelopeData(0x7f, spu_channel);
	//gpHalSPU_SetEnvelopeRampDownOffset(0x15, spu_channel);
	gpHalSPU_SetEnvelopeRampDownOffset(0x08, spu_channel);
	gpHalSPU_SetEnvelopeRepeatAddrOffset(0x0000, spu_channel);
	gpHalSPU_SetEnvelopeAddress(0x00000000, spu_channel);
	gpHalSPU_SetPhaseAccumulator(0x00000000, spu_channel);
	//gpHalSPU_SetRampDownClock(0x01, spu_channel);
	gpHalSPU_SetRampDownClock(0x00, spu_channel);
}

void MIXER_SPU_PLAY_PCM_LOOP_START(void)
{
	
}

void load_tone_color_phys_addr(void)
{
	UINT16 i;
	UINT32 *virAddr;
	UINT32 out_ptr;
	
	SPU_DBG("total instrument = %d \n", spu_struct_p->total_inst);
	SPU_DBG("total drum = %d \n", spu_struct_p->total_drum);
	
	for(i=0;i<spu_struct_p->total_inst;i++)
	{
		virAddr = (unsigned int *)(spu_struct_p->T_InstrumentSectionAddr + i);
//		SPU_DBG("VirtualInst[%d] = 0x%x \n", i, (UINT32*)*virAddr);
		out_ptr = (UINT32) gp_user_va_to_pa((UINT32*)*virAddr);
		spu_struct_p->T_InstrumentSectionPhyAddr[i] = out_ptr;
//		SPU_DBG("T_InstrumentSectionPhyAddr[%d] = 0x%x \n", i, out_ptr);
	}	
	for(i=0;i<spu_struct_p->total_drum;i++)
	{
			virAddr = (unsigned int *)(spu_struct_p->T_DrumAddr + i);
//			SPU_DBG("VirtualDrum[%d] = 0x%x \n", i, (UINT32*)(*virAddr));
			out_ptr = (UINT32) gp_user_va_to_pa((UINT32*)(*virAddr));
			spu_struct_p->T_DrumPhyAddr[i] = (UINT32*)out_ptr;
//			SPU_DBG("T_DrumPhyAddr[%d] = 0x%x \n",i, out_ptr);
	}
}
//-------------------------------------------------------------------------------------------------------------
/**
* @brief	       gp spu initial
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_spu_init(void)
{
      #if GP_SPU_HARDWARE_MODULE == GP_SPU_ENABLE
//         gpHalSpuInit();//0409 comment
         return 0; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_spu_init);

/**
* @brief	       gp spu playtone
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_spu_play_tone(SPU_MOUDLE_STRUCT *spu_register_set )
{
      #if GP_SPU_HARDWARE_MODULE == GP_SPU_ENABLE
//         gpHalSpuPlayTone(spu_register_set->SPU_pitch, spu_register_set->SPU_pAddr, spu_register_set->SPU_pan,
//  	       spu_register_set->SPU_velocity, spu_register_set->SPU_channel);
					SPU_PlayTone(spu_register_set);	
         return 0; 
      #else
         return -1; 
      #endif   
}

/**
* @brief	       gp spu playdrum
* @param 	none
* @return 	SUCCESS/ERROR_ID.
*/
signed int gp_spu_play_drum(SPU_MOUDLE_STRUCT *spu_register_set )
{
      #if GP_SPU_HARDWARE_MODULE == GP_SPU_ENABLE
//         SPU_PlayDrum(spu_register_set->SPU_drumIdx, spu_register_set->SPU_pAddr, spu_register_set->SPU_pan,
 // 	       spu_register_set->SPU_velocity, spu_register_set->SPU_channel);
         return 0; 
      #else
         return -1; 
      #endif   
}
EXPORT_SYMBOL(gp_spu_play_tone);

/**
 * \brief Open spu device
 */
int
gp_spu_open(
	struct inode *inode,
	struct file *filp
)
{
  DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	/* Success */
	filp->private_data = spu_devices;
	printk(KERN_WARNING "SPU open \n");

//	audio2_Power_ctrl(1);
//	gpHalSpuClkEnable();
	
	return 0;
}
int
gp_spu_release(
	struct inode *inode,
	struct file *filp
)
{
	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	/* Success */
	printk(KERN_WARNING "SPU release \n");

	return 0;
}

//static SPU_MOUDLE_STRUCT *spu_register_set;
int
gp_spu_ioctl(
	struct inode *inode,
	struct file *filp,
	unsigned int cmd,
	unsigned long arg
)
{
	int ret = 0;
//	unsigned int pAddr;
//	unsigned int kAddr;
//	SPU_MOUDLE_STRUCT *spu_register_set;
	UINT32 temp,temp1;
//	SINT32 nRet;

//ggg 0316
	int i=0,j;
	float ftemp;
	//unsigned int *tmp_ptr;
	UINT32 bit_mask;
	st_gp_mixer	*st_gp_mixer_p;
//ggg 0316 end

  /* initial tv register parameter set structure */
//  spu_register_set = (SPU_MOUDLE_STRUCT *)arg;	
//	DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);
	/* Success */
//  printk(KERN_WARNING "_SPU_ioctl_Entry_\n");
//	test_flag = 1;
  
  switch(cmd)
  {
/*
	case SPU_INIT:

		//spu_struct_p = (STRUCT_MIDI_SPU*)arg;//20110412
		copy_from_user((void*) &spu_struct, (const void __user *) arg, sizeof(STRUCT_MIDI_SPU));
		spu_struct_p = (STRUCT_MIDI_SPU*)&spu_struct;
			
		SPU_DBG("spu_init_flag = %d\n",spu_init_flag);	
		if(!spu_init_flag)
		{
			for(i=0;i<3;i++)
			{		
				struct_gp_mixer[i].hardware_ch = 0xffff;
				SPU_DBG("struct_gp_mixer[%d].hardware_ch = %d\n",i ,struct_gp_mixer[i].hardware_ch); 	
			}								
			SPU_Init();
			spu_init_flag = -1;	
		}
		SPU_DBG("SPU_INIT check! \n");
		break;  	  
*/
	case SPU_PLAY_TONE:

		break;

	case SPU_MIDI_PLAY:

			
		copy_from_user((void *)&spu_struct, (const void __user *) arg, sizeof(STRUCT_MIDI_SPU));
		//spu_struct_p = (STRUCT_MIDI_SPU*)&spu_struct;
			
		load_tone_color_phys_addr();
			
		SPU_MIDI_Play();
		break;
	
	case SPU_MIDI_STOP:
			SPU_MIDI_Stop();//20110412
			break;
	
	case SPU_MIDI_PAUSE:
			SPU_MIDI_Pause();
			break;
	
	case SPU_MIDI_RESUME:
			SPU_MIDI_Resume();
			break;
	
	case SPU_MIDI_SET_VOL:
			SPU_MIDI_Set_MIDI_Volume();
			break;
	
	case SPU_PLAY_DRUM:
			break;
	
	case SPU_PLAY_SFX:
			break;
	
	case SPU_SET_CH_VOL:
			//gpHalSPU_SetVelocity();
			break;
	
	case SPU_SET_CH_PAN:
			//gpHalSPU_SetPan();
			break;
			
	case SPU_SET_BEAT_BASE_CNT:
			//gpHalSPU_Set_BeatBaseCounter();
			break;
			
	case SPU_MIDI_SET_PAN://20110412
			SPU_MIDI_Set_MIDI_Pan();
			break;
	case SPU_MIDI_BUF_WR:
//			copy_to_user ((void __user *) arg, (const void *) &spu_struct, sizeof(STRUCT_SPU));
			copy_from_user((void *)&spu_struct, (const void __user *) arg, sizeof(STRUCT_MIDI_SPU));
//			spu_struct_p = (STRUCT_SPU*)&spu_struct;	
			break;

	case SPU_MIDI_BUF_RD:
			copy_to_user ((void __user *) arg, (const void *) &spu_struct, sizeof(STRUCT_MIDI_SPU));
//			copy_from_user((void *)&spu_struct, (const void __user *) arg, sizeof(STRUCT_SPU));
//			spu_struct_p = (STRUCT_SPU*)&spu_struct;	
			break;
			
	case SPU_PLAY_DRM://play *.drm  20110503
			copy_from_user((void *)&drm_struct, (const void __user *)arg, sizeof(STRUCT_DRM_SPU));
			
			SPU_DBG("drm_struct.pAddr @ gp_spu = 0x%x\n", (UINT32)drm_struct.pAddr);
			SPU_DBG("first word is @ gp_spu = 0x%x\n", *(drm_struct.pAddr));
			SPU_DBG("drm_struct.phyAddr @ gp_spu = 0x%x\n", drm_struct.phyAddr);
			SPU_DBG("drm_struct.uiPan @ gp_spu = 0x%x\n", drm_struct.uiPan);
			
			SPU_PlayPCM_NoEnv_FixCH((UINT32 *)drm_struct.pAddr, drm_struct.phyAddr, drm_struct.uiPan, drm_struct.uiVelocity, drm_struct.uiSPUChannel);
			
			break;	

	//---------------------------------------------------------------------------mixer
	case MIXER_NEW_CHANNEL:
//		SPU_DBG("***** MIXER_NEW_CHANNE *****\n");
		copy_from_user((void*) &struct_gp_mixer_tmp, (const void __user *) arg, sizeof(st_gp_mixer));
		
//		SPU_DBG("struct_gp_mixer_tmp.sample_spec_freq = 0x%x \n", struct_gp_mixer_tmp.sample_spec_freq);		
		
		for(i=0;i<3;i++)
		{
			if(struct_gp_mixer[i].hardware_ch == 0xffff)
			{ 
				//SPU_DBG("copy_from_user() for real structure \n");
				//copy_from_user((void*) &struct_gp_mixer[i], (const void __user *) arg, sizeof(st_gp_mixer));
				struct_gp_mixer[i] = struct_gp_mixer_tmp;//20110430
				st_gp_mixer_p = (st_gp_mixer*)&struct_gp_mixer[i];					

				st_gp_mixer_p->hardware_ch = i;

				ftemp = st_gp_mixer_p->sample_spec_freq;
				ftemp = ftemp* 524288;
				ftemp = ftemp / 281250;
				temp = (UINT32)ftemp;
				if((st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_S16) || (st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_U16))//mono
				{
					MIXER_SPU_CH_Init(T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPhase(temp, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPan(64, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetVelocity(127, T_mixer_spu_channel[i][0]);
				}
				else
				{
					MIXER_SPU_CH_Init(T_mixer_spu_channel[i][0]);
					MIXER_SPU_CH_Init(T_mixer_spu_channel[i][1]);
					gpHalSPU_SetPhase(temp, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPhase(temp, T_mixer_spu_channel[i][1]);
					gpHalSPU_SetPan(127, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPan(0, T_mixer_spu_channel[i][1]);
					gpHalSPU_SetVelocity(127, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetVelocity(127, T_mixer_spu_channel[i][1]);	
				}
				mixer_playing_flag[i] = 0;
				mixer_pause_status[i] = 0;//20110428
				mixer_loop_offset[i] = 0;//20110428
				mixer_loop_block[i] = 0;//20110502
				mixer_loop_block_bak[i] = -1;//20110428
				break;
			}
		}
		if(i>=3)//no hardware to request
		{
			SPU_DBG("request spu hardware channel fail\n");
		}
		else
		{
			SPU_DBG("MIXER_NEW_CHANNEL = %d\n",i);
		}
		copy_to_user ((void __user *) arg, (const void *) &struct_gp_mixer[i], sizeof(st_gp_mixer));
//#if 0	
//		st_gp_mixer_p = (st_gp_mixer*)arg;
//#else
//		copy_from_user((void*) &struct_gp_mixer, (const void __user *) arg, sizeof(st_gp_mixer));
//		st_gp_mixer_p = (st_gp_mixer*)&struct_gp_mixer;	
//#endif				
/*
		SPU_DBG("st_gp_mixer_p->iFirst = %x \n",st_gp_mixer_p->iFirst);
		SPU_DBG("st_gp_mixer_p->wave_play_block = %x \n",st_gp_mixer_p->wave_play_block);
		SPU_DBG("st_gp_mixer_p->wave_write_block = %x \n",st_gp_mixer_p->wave_write_block);
		SPU_DBG("st_gp_mixer_p->r_wavedata_addr = %x \n",st_gp_mixer_p->r_wavedata_addr);
		st_gp_mixer_p->iFirst = 1;
		st_gp_mixer_p->wave_play_block = 0x55;
		st_gp_mixer_p->wave_write_block = 0xAA;
		copy_to_user ((void __user *) arg, (const void *) &struct_gp_mixer, sizeof(st_gp_mixer));
			
		break;//ggg
*/

//		SPU_DBG("st_gp_mixer_p->iLoopQueuSize  = %x \n",st_gp_mixer_p->iLoopQueuSize);
/*
		temp = gpHalSPU_GetChannelStatus();
		if((temp & (T_BitEnable[26] | T_BitEnable[27])) == 0)
		{
			st_gp_mixer_p->hardware_ch = 1;
			mixer_channel_0 = 26;
			mixer_channel_1 = 27;
		}
		else
		if((temp & (T_BitEnable[28] | T_BitEnable[29])) == 0)
		{
			st_gp_mixer_p->hardware_ch = 2;
			mixer_channel_0 = 28;
			mixer_channel_1 = 29;
		}
		else
		if((temp & (T_BitEnable[30] | T_BitEnable[31])) == 0)
		{
			st_gp_mixer_p->hardware_ch = 3;
			mixer_channel_0 = 30;
			mixer_channel_1 = 31;
		}
		else
		{
			st_gp_mixer_p->hardware_ch = -1;
			mixer_channel_0 = 0xff;
			mixer_channel_1 = 0xff;
			break;
			//return mix_ch;
		}
*/
		//st_gp_mixer		*st_gp_mixer_p;
		//st_gp_mixer_cmd	*st_gp_mixer_cmd_p;
		//nRet = request_irq(51,gp_spu_channel_irq_handler,IRQF_DISABLED,"SPU_IRQ",NULL);
		//if(nRet < 0) 
		//{
		//	nRet=-1;
		//	printk("request gp_spu_channel_irq_handler fail\n");
		//	break;
		//}
//		MIXER_SPU_CH_Init(mixer_channel_0);//L
//		MIXER_SPU_CH_Init(mixer_channel_1);//R
		//samplerate to phase;
		//i = phase;//??
		//i = (UINT32)((unsigned long int)st_gp_mixer_p->sample_spec_freq * 524288 / 281250);
//		SPU_DBG("st_gp_mixer_p->sample_spec_freq = 0x%x \n", st_gp_mixer_p->sample_spec_freq);
//		ftemp = st_gp_mixer_p->sample_spec_freq;
//		//SPU_DBG("ftemp 1 = %d \n", (UINT32)ftemp);
//		ftemp = ftemp* 524288;
//		//SPU_DBG("ftemp 2 = %d \n", ftemp);
//		ftemp = ftemp / 281250;
//		//SPU_DBG("ftemp 3 = %d \n", ftemp);
//		//SPU_DBG("ftemp 4 = %d \n", (UINT32)ftemp);
//		i = (UINT32)ftemp;
//		SPU_DBG("phase = 0x%x \n", i);
//		gpHalSPU_SetPhase((UINT32)i, mixer_channel_0);
//		gpHalSPU_SetPhase((UINT32)i, mixer_channel_1);
//		gpHalSPU_SetPan(127, mixer_channel_0);
//		gpHalSPU_SetPan(0, mixer_channel_1);
//		gpHalSPU_SetVelocity(127, mixer_channel_0);
//		gpHalSPU_SetVelocity(127, mixer_channel_1);	
//		mixer_playing_flag[i] = 0;
//		copy_to_user ((void __user *) arg, (const void *) &struct_gp_mixer, sizeof(st_gp_mixer));
		break;		
	
	case	MIXER_SET_WRITE_POS_CHANNEL:
		//SPU_DBG("MIXER_SET_WRITE_POS_CHANNEL\n");
		//test_flag = 1;
		copy_from_user((void*) &struct_gp_mixer_tmp, (const void __user *) arg, sizeof(st_gp_mixer));
//		st_gp_mixer_p = (st_gp_mixer*)&struct_gp_mixer;	
		i = (SINT32)struct_gp_mixer_tmp.hardware_ch;
//		SPU_DBG("(SINT32)struct_gp_mixer_tmp.hardware_ch = %d\n",i);
		if(i == 0xffff)
		{
			SPU_err();
			ret = -1;
			break;
			//while(1);
		}
		//copy_from_user((void*) &struct_gp_mixer[i], (const void __user *) arg, sizeof(st_gp_mixer));
		struct_gp_mixer[i].wave_write_block = struct_gp_mixer_tmp.wave_write_block;//20110430
		struct_gp_mixer[i].wave_block_write_pos = struct_gp_mixer_tmp.wave_block_write_pos;//20110430
		
		st_gp_mixer_p = (st_gp_mixer*)&struct_gp_mixer[i];

		if(mixer_playing_flag[i] == 0)//还未启动播放
		{
			//first entry
//			if(st_gp_mixer_p->wave_write_block > st_gp_mixer_p->wave_play_block)/* if wr_ptr > rd_ptr => resume play */
//			{
//				SPU_DBG("first entry \n");
				ftemp = st_gp_mixer_p->sample_spec_freq;
				ftemp = ftemp* 524288;
				ftemp = ftemp / 281250;
				temp = (UINT32)ftemp;
				if((st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_S16) || (st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_U16))//mono
				{
					MIXER_SPU_CH_Init(T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPhase(temp, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPan(64, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetVelocity(127, T_mixer_spu_channel[i][0]);
				}
				else
				{
					MIXER_SPU_CH_Init(T_mixer_spu_channel[i][0]);
					MIXER_SPU_CH_Init(T_mixer_spu_channel[i][1]);
					gpHalSPU_SetPhase(temp, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPhase(temp, T_mixer_spu_channel[i][1]);
					gpHalSPU_SetPan(127, T_mixer_spu_channel[i][0]);
					gpHalSPU_SetPan(0, T_mixer_spu_channel[i][1]);
					//gpHalSPU_SetVelocity(127, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetVelocity(127, T_mixer_spu_channel[i][1]);	
				}
				
				mixer_playing_flag[i] = 1;
				st_gp_mixer_p->wave_play_block = 0;
//				mixer_start_block[i] = st_gp_mixer_p->wave_play_block;
//				mixer_loop_block[i] = mixer_start_block[i] + 1;	
				//mixer_loop_block[i] = st_gp_mixer_p->wave_play_block;	
				st_gp_mixer_p->underrun = 0;//20110428
				mixer_loop_block[i] = 0;//20110429
				mixer_loop_block_bak[i] = 0;//20110429

				if(   (st_gp_mixer_p->wave_write_block > 1)
					|| ((st_gp_mixer_p->wave_write_block == 1) && (st_gp_mixer_p->wave_block_write_pos != 0))
					|| ((st_gp_mixer_p->wave_write_block == 0) && (st_gp_mixer_p->wave_block_write_pos == 0))
				)//下一个block写有数据
				//if(((st_gp_mixer_p->wave_write_block) && (st_gp_mixer_p->wave_block_write_pos)) || ((st_gp_mixer_p->wave_write_block == 0) && (st_gp_mixer_p->wave_block_write_pos == 0)))//20110422
				{
					mixer_loop_block[i] = 1;//st_gp_mixer_p->wave_play_block +1;
					temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;
					temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}
				else//下一个block没有写数据
				{
					mixer_loop_block[i] = st_gp_mixer_p->wave_write_block;
					mixer_loop_offset[i] = st_gp_mixer_p->wave_block_write_pos;
					//if((st_gp_mixer_p->wave_write_block==0)&&(st_gp_mixer_p->wave_block_write_pos!=0))//第0block没有写满
					//{
					//	mixer_loop_block[i] = 0;
					//	mixer_loop_offset[i] = 0;//20110429 
					//}
					//else//第0block写满了
					//{
					//	mixer_loop_block[i] = 1;	
					//	mixer_loop_offset[i] = 0;//20110429 
					//}
					//st_gp_mixer_p->wave_play_block = mixer_loop_block[i];//20110429
					mixer_loop_block_bak[i] = mixer_loop_block[i];//20110429
					temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*st_gp_mixer_p->iLoopQueuSize)) >> 1;//指向buffer最后4个字节
					temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*st_gp_mixer_p->iLoopQueuSize)) >> 1;//指向buffer最后4个字节
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}

				temp = st_gp_mixer_p->l_wavedata_phy_addr;// + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i]);//都以byte为单位
				gpHalSPU_SetStartAddress(temp/2, T_mixer_spu_channel[i][0]);
//				temp = st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i]);
//				gpHalSPU_SetLoopAddress(temp/2, T_mixer_spu_channel[i][0]);

				temp = st_gp_mixer_p->r_wavedata_phy_addr;// + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i]);//都以byte为单位				
				gpHalSPU_SetStartAddress(temp/2, T_mixer_spu_channel[i][1]);
//				temp = st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i]);
//				gpHalSPU_SetLoopAddress(temp/2, T_mixer_spu_channel[i][1]);
				if((st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_S16) || (st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_U16))//mono
				{
					bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]];
				}
				else
				{
					bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]] | T_BitEnable[T_mixer_spu_channel[i][1]];
				}
				gpHalSPU_SetPhaseAccumulator(0x00000000, T_mixer_spu_channel[i][1]);
				gpHalSPU_SetPhaseAccumulator(0x00000000, T_mixer_spu_channel[i][0]);
				gpHalSPU_Enable_MultiChannel(bit_mask);
				gpHalSPU_Clear_MultiCh_StopFlag(bit_mask);
				gpHalSPU_Channel_FIQ_Enable(T_mixer_spu_channel[i][0]);
//			}
		}
		else
		{
			//resume channel
			//if(mixer_pause_status[i])
			if(st_gp_mixer_p->underrun)//underrun
			{
//				SPU_DBG("channel is pause status!\n");
				//SPU_resume_channel(T_mixer_spu_channel[i][0]);
				//SPU_resume_channel(T_mixer_spu_channel[i][1]);
				//SPU_resume_two_channel(T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				gpHalSPU_Disable_Channel(T_mixer_spu_channel[i][0]);
				gpHalSPU_Disable_Channel(T_mixer_spu_channel[i][1]);
				//while(1)
				//{
				//	if((gpHalSPU_Get_SingleChannel_Status(T_mixer_spu_channel[i][0]) == 0) 
				//	   && (gpHalSPU_Get_SingleChannel_Status(T_mixer_spu_channel[i][1]) == 0) 
				//	)
				//	{
				//		break;
				//	}
				//}
				mixer_pause_status[i] = 0;//20110506 mask
				st_gp_mixer_p->underrun = 0;
				temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i]) + mixer_loop_offset[i]) >> 1;
				temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i]) + mixer_loop_offset[i]) >> 1;
				gpHalSPU_SetStartAddress(temp, T_mixer_spu_channel[i][0]);
				gpHalSPU_SetStartAddress(temp1, T_mixer_spu_channel[i][1]);
//				SPU_DBG("mixer_loop_block[i] @ resume = %d \n",mixer_loop_block[i]);
				j=st_gp_mixer_p->wave_write_block - mixer_loop_block[i];
				if(j<0)
				{	
					j += st_gp_mixer_p->iLoopQueuSize;			
				}
				if(  (j>1)
				   ||((j==1) && (st_gp_mixer_p->wave_block_write_pos!=0))
				)//(1)
				{
					if(++mixer_loop_block[i] >= st_gp_mixer_p->iLoopQueuSize)
						mixer_loop_block[i] = 0;
					temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;//都以byte为单位	
					temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;			
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);	
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}
				else
				if( (j==1) && (st_gp_mixer_p->wave_block_write_pos==0))//(2)
				{
					if(++mixer_loop_block[i] >= st_gp_mixer_p->iLoopQueuSize)
						mixer_loop_block[i] = 0;
					mixer_loop_block_bak[i] = mixer_loop_block[i];
					temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*st_gp_mixer_p->iLoopQueuSize)) >> 1;//指向buffer最后4个字节
					temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*st_gp_mixer_p->iLoopQueuSize)) >> 1;//指向buffer最后4个字节
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}
				else
				if(j==0)
				{
					temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*st_gp_mixer_p->iLoopQueuSize)) >> 1;//指向buffer最后4个字节
					temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*st_gp_mixer_p->iLoopQueuSize)) >> 1;//指向buffer最后4个字节
					//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
					//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);
					gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
				}
				//启动channel
				if((st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_S16) || (st_gp_mixer_p->sample_spec_type == FORMAT_2CH_PM_U16))//mono
				{
					bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]];
				}
				else
				{
					bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]] | T_BitEnable[T_mixer_spu_channel[i][1]];
				}
				gpHalSPU_EnvelopeManualMode(T_mixer_spu_channel[i][0]);
				gpHalSPU_EnvelopeManualMode(T_mixer_spu_channel[i][1]);
				gpHalSPU_SetEnvelopeData(0x7f, T_mixer_spu_channel[i][0]);
				gpHalSPU_SetEnvelopeData(0x7f, T_mixer_spu_channel[i][1]);
				gpHalSPU_SetPhaseAccumulator(0x00000000, T_mixer_spu_channel[i][1]);//20110430
				gpHalSPU_SetPhaseAccumulator(0x00000000, T_mixer_spu_channel[i][0]);//20110430
				gpHalSPU_Enable_MultiChannel(bit_mask);
				gpHalSPU_Clear_MultiCh_StopFlag(bit_mask);
				gpHalSPU_Channel_FIQ_Enable(T_mixer_spu_channel[i][0]);
			}
			else//not underrun
			{
				if(mixer_loop_block_bak[i] == mixer_loop_block[i])
				{
	//				SPU_DBG("no_pause\n");
					j=st_gp_mixer_p->wave_write_block - mixer_loop_block[i];
					if(j<0)
					{	
						j += st_gp_mixer_p->iLoopQueuSize;			
					}
			
					if(st_gp_mixer_p->wave_play_block != mixer_loop_block[i])//(1)
					{
						mixer_loop_block_bak[i] = st_gp_mixer_p->wave_play_block;
						temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;//都以byte为单位	
						temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;			
						//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
						//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);	
						gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
					}
					else 
					if(  ((j>1) && (st_gp_mixer_p->wave_play_block == mixer_loop_block[i]))
					   || ((j==1) && (st_gp_mixer_p->wave_play_block == mixer_loop_block[i]) && (st_gp_mixer_p->wave_block_write_pos!=0))
					)//(2)
					{
						if(++mixer_loop_block[i] >= st_gp_mixer_p->iLoopQueuSize)
							mixer_loop_block[i] = 0;
						temp  = (st_gp_mixer_p->l_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;//都以byte为单位	
						temp1 = (st_gp_mixer_p->r_wavedata_phy_addr + ((4+st_gp_mixer_p->iLoopBufLen)*mixer_loop_block[i])) >> 1;			
						//gpHalSPU_SetLoopAddress(temp, T_mixer_spu_channel[i][0]);
						//gpHalSPU_SetLoopAddress(temp1, T_mixer_spu_channel[i][1]);	
						gpHalSPU_Set_two_ch_LoopAddress(temp, temp1, T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
					}
					else
					if((j==1) && (st_gp_mixer_p->wave_play_block == mixer_loop_block[i]) && (st_gp_mixer_p->wave_block_write_pos==0))//(3)
					{
						if(++mixer_loop_block[i] >= st_gp_mixer_p->iLoopQueuSize)
							mixer_loop_block[i] = 0;
						mixer_loop_block_bak[i] = mixer_loop_block[i];
					}
				}
				if(mixer_pause_status[i])//user pause
				{
					mixer_pause_status[i] = 0;//20110506 mask
					st_gp_mixer_p->underrun = 0;
					SPU_resume_two_channel(T_mixer_spu_channel[i][0],T_mixer_spu_channel[i][1]);
				}
			}			
		}
		//test_flag = 0;
		break;
	
	case 	MIXER_GET_READ_POS_CHANNEL:
//		SPU_DBG("MIXER_GET_READ_POS_CHANNEL\n");				
		copy_from_user((void*) &struct_gp_mixer_tmp, (const void __user *) arg, sizeof(st_gp_mixer));
		i = struct_gp_mixer_tmp.hardware_ch;
//		SPU_DBG("(SINT32)struct_gp_mixer_tmp.hardware_ch = %d\n",i);
		if(i == 0xffff)
		{
			SPU_err();
			ret = -1;
			break;
			//while(1);
		}
		copy_to_user ((void __user *) arg, (const void *) &struct_gp_mixer[i], sizeof(st_gp_mixer));
		break;

	case MIXER_RELEASE_CHANNEL:
		copy_from_user((void*)&struct_gp_mixer_tmp, (const void __user *)arg, sizeof(st_gp_mixer));
		i = struct_gp_mixer_tmp.hardware_ch;
		st_gp_mixer_p = (st_gp_mixer*)&struct_gp_mixer[i];
		st_gp_mixer_p->hardware_ch = 0xffff;
		bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]] | T_BitEnable[T_mixer_spu_channel[i][1]];
		gpHalSPU_EnvelopeAutoMode(T_mixer_spu_channel[i][0]);
		gpHalSPU_EnvelopeAutoMode(T_mixer_spu_channel[i][1]);
		gpHalSPU_Set_EnvRampDownMultiChannel(bit_mask);
		gpHalSPU_Disable_FIQ_Channel(T_mixer_spu_channel[i][0]);
		SPU_DBG("MIXER_RELEASE_CHANNEL [i] = %d \n", i);	
		break;

	case MIXER_CMD_SET_VOL:
		copy_from_user((void*)&struct_gp_mixer_cmd_tmp, (const void __user *)arg, sizeof(st_gp_mixer_cmd));
		i = struct_gp_mixer_cmd_tmp.hardware_ch;
		//SPU_DBG("vol=  %d \n", struct_gp_mixer_cmd_tmp.parameter);
		gpHalSPU_SetVelocity((UINT8)struct_gp_mixer_cmd_tmp.parameter, T_mixer_spu_channel[i][0]);
		gpHalSPU_SetVelocity((UINT8)struct_gp_mixer_cmd_tmp.parameter, T_mixer_spu_channel[i][1]);	
		break;
		
	case MIXER_CMD_PAUSE:
		copy_from_user((void*)&struct_gp_mixer_cmd_tmp, (const void __user *)arg, sizeof(st_gp_mixer_cmd));
		i = struct_gp_mixer_cmd_tmp.hardware_ch;
		if(mixer_pause_status[i] == 0)
		{
			SPU_pause_two_channel(T_mixer_spu_channel[i][0],T_mixer_spu_channel[i][1]);
			mixer_pause_status[i] = 1;
		}
		break;
	case MIXER_CMD_RESUME:
		copy_from_user((void*)&struct_gp_mixer_cmd_tmp, (const void __user *)arg, sizeof(st_gp_mixer_cmd));
		i = struct_gp_mixer_cmd_tmp.hardware_ch;
		if(mixer_pause_status[i])
		{
			mixer_pause_status[i] = 0;
			SPU_resume_two_channel(T_mixer_spu_channel[i][0],T_mixer_spu_channel[i][1]);
		}
		break;
		
	case MIXER_CMD_STOP:
		copy_from_user((void*)&struct_gp_mixer_cmd_tmp, (const void __user *)arg, sizeof(st_gp_mixer_cmd));
		i = struct_gp_mixer_cmd_tmp.hardware_ch;
		//st_gp_mixer_p = (st_gp_mixer*)&struct_gp_mixer[i];
		bit_mask = T_BitEnable[T_mixer_spu_channel[i][0]] | T_BitEnable[T_mixer_spu_channel[i][1]];
		SPU_resume_two_channel(T_mixer_spu_channel[i][0], T_mixer_spu_channel[i][1]);
		gpHalSPU_EnvelopeAutoMode(T_mixer_spu_channel[i][0]);
		gpHalSPU_EnvelopeAutoMode(T_mixer_spu_channel[i][1]);
		gpHalSPU_Set_EnvRampDownMultiChannel(bit_mask);
		gpHalSPU_Disable_FIQ_Channel(T_mixer_spu_channel[i][0]);
		
		struct_gp_mixer[i].wave_play_block = 0;//20110509
		struct_gp_mixer[i].underrun = 0;//20110509
		
		mixer_playing_flag[i] = 0;
		mixer_pause_status[i] = 0;//20110428
		mixer_loop_offset[i] = 0;//20110428
		mixer_loop_block[i] = 0;//20110502
		mixer_loop_block_bak[i] = -1;//20110428
		break;
		
	
	case MIXER_GET_LOOP_BLOCK_CHANNEL:
		copy_from_user((void*)&i, (const void __user *)arg, sizeof(int));
//		SPU_DBG("(SINT32)struct_gp_mixer_tmp.hardware_ch = %d\n",i);
		if(i == 0xffff)
		{
			SPU_err();
			//while(1);
		}
		j = mixer_loop_block[i];
		copy_to_user ((void __user *)arg, (const void *)&j, sizeof(int));
		break;
	//--------------------------------------------------------------------------------------
	default:
			ret = -ENOIOCTLCMD;
			break;                      
  }
  
//  	test_flag = 0;
//	SPU_DBG("ret = 0x%x \n",ret);
	return ret;
}

void __exit
gp_spu_module_exit(
void
)
{
dev_t devno = MKDEV(spu_major, SPU_MINOR);
device_destroy(spu_class, devno);
cdev_del(&(spu_devices->c_dev));
free_irq(51, spu_devices);
kfree(spu_devices);
unregister_chrdev_region(devno, SPU_NR_DEVS);
printk(KERN_WARNING "SPU module exit \n");
}

/**
* \brief Initialize display device
*/
int __init
gp_spu_module_init(
void
)
{
int result;
dev_t dev;
int devno;
int nRet;
SINT32 i;

DEBUG("[%s:%d]\n", __FUNCTION__, __LINE__);

result = alloc_chrdev_region(&dev, SPU_MINOR, 1, "SPU");
if( result<0 ) {
printk(KERN_WARNING "SPU: can't get major \n");
return result;
}
spu_major = MAJOR(dev);
spu_class = class_create(THIS_MODULE, "SPU");

spu_devices = kmalloc(sizeof(gp_spu_dev_t), GFP_KERNEL);
if(!spu_devices) {
printk(KERN_WARNING "SPU: can't kmalloc \n");
result = -ENOMEM;
goto fail;
}
memset(spu_devices, 0, sizeof(gp_spu_dev_t));

devno = MKDEV(spu_major, SPU_MINOR);
cdev_init(&(spu_devices->c_dev), &spu_fops);
spu_devices->c_dev.owner = THIS_MODULE;
spu_devices->c_dev.ops = &spu_fops;
result = cdev_add(&(spu_devices->c_dev), devno, 1);
device_create(spu_class, NULL, devno, NULL, "spu");

nRet = request_irq(51,gp_spu_irq_handler,IRQF_DISABLED,"SPU_IRQ",spu_devices);//spu request irq
if(nRet < 0) 
{
nRet=-1; 
printk("SPU request irq fail\n"); 
}

audio2_Power_ctrl(1);
gpHalSpuClkEnable();


SPU_Init();//init spu 20110426
spu_struct_p = (STRUCT_MIDI_SPU*)&spu_struct;//20110503
for(i=0;i<3;i++){ 
struct_gp_mixer[i].hardware_ch = 0xffff;
//SPU_DBG("struct_gp_mixer[%d].hardware_ch = %d\n",i ,struct_gp_mixer[i].hardware_ch); 
} 

if(result)
printk(KERN_WARNING "Error adding spu \n");

printk(KERN_WARNING "SPU module init \n");

return 0;

fail:

printk(KERN_WARNING "SPU module init failed \n");
kfree(spu_devices);
unregister_chrdev_region(dev, SPU_NR_DEVS);

return result;

}

module_init(gp_spu_module_init);
module_exit(gp_spu_module_exit);

/**************************************************************************
 *                  M O D U L E    D E C L A R A T I O N                  *
 **************************************************************************/

MODULE_AUTHOR("Generalplus");
MODULE_DESCRIPTION("Generalplus SPU Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
