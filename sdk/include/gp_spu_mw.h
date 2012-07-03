#ifndef __GP_SPU_MW_H__
#define __GP_SPU_MW_H__
//-------------------------------------------------------------------------------from gplib_spu.h
#define C_SPU_PW_FIQ			1
#define C_SPU_BEAT_FIQ			2
#define C_SPU_ENV_FIQ			3
#define C_SPU_FIQ				4
#define C_MAX_FIQ				5				// Don't forget to modify this when new fast interrupt source is added

#define B_SPU_PW_FIQ			0x01
#define B_SPU_BEAT_FIQ			0x02
#define B_SPU_ENV_FIQ			0x04
#define B_SPU_FIQ				0x08
//---------------------------------------------------------------------------------------

#define SPU_ChannelNumber		32
#define MIDI_ChannelNumber		16
#define C_DefaultBeatBase		352
#define C_DefaultTempo		120


#define STS_MIDI_PLAY		0x0001		// SPU Status
#define STS_MIDI_CALLBACK		0x0100		// MIDI stop call back routine has been initiated
#define STS_MIDI_REPEAT_ON		0x0200		// MIDI play back continuously
#define STS_MIDI_PAUSE_ON		0x0400		// MIDI Pause flag
#define STS_ADPCM_MODE_ON		0x0800		// SPU is at ADPCM mode
#define STS_NORMAL_MODE_ON		0x0000  	// SPU is at PCM mode
#define C_MIDI_Delay			0x00010000	// wait beat count equal to 0x0000

#define C_NoteEvent			0x0000
#define C_BeatCountEvent		0x0001
#define C_PitchBendEvent		0x0002
#define C_ControlEvent		0x0003
#define C_ProgramChangeEvent	0x0004
#define C_TempoEvent			0x0005
#define C_MIDI_EndEvent		0x0006
#define C_LyricEvent			0x0007
#define C_TextEvent			0x0008

// the following definition is for control event
#define C_DataEntryEvent		0x0006
#define C_VolumeEvent		0x0007
#define C_PanEvent			0x000A
#define C_ExpressionEvent		0x000B
#define C_RPN_LSB_Event		0x0064
#define C_RPN_MSB_Event		0x0065

#define MIDI_RING_BUFFER_SIZE	512*8//4096 byte

/* gp gpu mixer*/
#define FORMAT_MONO			0x0001
#define FORMAT_MONO_R_L_S_S16 	0x0002
#define FORMAT_STEREO 		0x0003


#define GP_SPU_LOOP_SIZE						(2*8*1024)//bytes
#define GP_SPU_QUEU_SIZE						6
//#define MIXER_SPU_PCM_BUF_NUM	5
//#define MIXER_SPU_PCM_ONE_FRAME_LENGTH	1024*16


typedef struct{
	unsigned int ALP_ID[4];
	unsigned int SampleRate;
	unsigned int SampleLength;
	unsigned int LoopStartAddr;
	unsigned int EnvelopStartAddr;
	unsigned int WaveType;
	unsigned int BasePitch;
	unsigned int MaxPitch;
	unsigned int MinPitch;
	unsigned int RampDownClock;
	unsigned int RampDownStep;
	unsigned int Pan;
	unsigned int Velocity;
}ALP_Header_Struct;

typedef struct {
	unsigned char	SPU_pitch;                                
	unsigned int*	SPU_pAddr;    
	unsigned int* SPU_vAddr;                      
	unsigned char	SPU_pan;
	unsigned char	SPU_velocity;                                
	unsigned char SPU_channel;
	unsigned char SPU_drumIdx;
	
	unsigned int SampleRate;
	unsigned int SampleLength;
	unsigned int LoopStartAddr;
	unsigned int EnvelopStartAddr;
	unsigned int WaveType;
	unsigned int BasePitch;
	unsigned int MaxPitch;
	unsigned int MinPitch;
	unsigned int RampDownClock;
	unsigned int RampDownStep;
	unsigned int Pan;
	unsigned int Velocity;
} SPU_MOUDLE_STRUCT, *SPU_MOUDLE_STRUCT_PTR;

typedef struct{
	UINT32 R_MIDI_CH_PAN;
	UINT32 R_MIDI_CH_VOLUME;
	UINT32 R_MIDI_CH_EXPRESSION;
	UINT32 R_MIDI_CH_PitchBend;
	UINT32 R_CH_SMFTrack;			// Record 16 MIDI channel's instrument
	UINT32 R_ChannelInst;			// Instrument Index mapping of Logical Channel in MIDI file
	UINT32 *R_PB_TABLE_Addr;			//PitchBend Table Address
	UINT32 R_RPN_ReceiveFlag;		//RPN Receive Flag
	UINT32 R_RPN_DATA;				//MIDI CH RPN Value
	
}MIDI_ChannelInfoStruct;

typedef struct{
	UINT32 R_NOTE_PITCH;
	UINT32 R_NOTE_VELOCITY;
	UINT32 R_MIDI_ToneLib_PAN;
	UINT32 R_MIDI_ToneLib_Volume;
//	UINT32 R_DUR_Tone;
	UINT32 R_MIDI_CH_MAP;
	UINT32 R_NoteOnHist;			// Log the NoteOn mapping channel to a Circular Queue
	UINT32 R_PB_PhaseRecord;		//Original Channel Phase Value
}SPU_ChannelInfoStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 Pitch;
	UINT32 Velocity;
	UINT32 Duration;
}NoteEventStruct;

typedef struct{
	UINT32 BeatCountValue;
}BeatCountEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 PitchBendValue;
}PitchBendEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 ControlNumber;
	UINT32 ControlValue;
}ControlEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 InstrumentIndex;
}ProgramChangeEventStruct;

typedef struct{
	UINT32 TempoValue;
}TempoEventStruct;

typedef struct{
	UINT32 ChannelNumber;
	UINT32 LyricWordCount;
	UINT32 Duration;
}LyricEventStruct;

typedef struct{
	UINT32 TextType;
	UINT32 LyricWordCount;
}TextEventStruct;

typedef union{
	NoteEventStruct NoteEvent;
	BeatCountEventStruct BeatCountEvent;
	PitchBendEventStruct PitchBendEvent;
	ControlEventStruct ControlEvent;
	ProgramChangeEventStruct ProgramChangeEvent;
	TempoEventStruct TempoEvent;
	LyricEventStruct LyricEvent;
	TextEventStruct TextEvent;
}MIDI_EventStruct;


typedef struct{//20110412
	UINT32 MIDI_PlayFlag;
	UINT32 MIDI_Tempo;
	UINT8 R_MIDI_Volume;	//0409
	UINT8 R_MIDI_Pan;		//0409
	ALP_Header_Struct ALP_Header;
	MIDI_ChannelInfoStruct MIDI_ChannelInfo[MIDI_ChannelNumber];
	SPU_ChannelInfoStruct SPU_ChannelInfo[SPU_ChannelNumber];
	UINT32 R_DUR_Tone[SPU_ChannelNumber + 1];
	
	UINT32 uiAddr;
	UINT32 uiData;
	UINT32 uiPhaseLow;
	UINT32 uiPhaseMiddle1;
	UINT32 uiPhaseMiddle2;
	UINT32 uiPhaseHigh;
	UINT32 *pTableAddr;
	UINT32 R_channel_original_phase[SPU_ChannelNumber];//0409
	//UINT32 R_Total_Voice;
	//UINT8 *pMIDI_StartAddr;
	UINT8 *pMIDI_DataPointer;
	MIDI_ChannelInfoStruct *pMIDI_ChInfo;
	SPU_ChannelInfoStruct *pSPU_ChInfo;
	//union MIDI_EventStruct MIDI_Event;
	MIDI_EventStruct MIDI_Event;
	UINT32 R_MIDI_EndFlag;
	UINT32 R_CH_NoteOff;
	UINT32 EventIndex;
	//UINT32 R_PlayChannel;
	UINT32 R_CH_OneBeat;
	UINT32 R_MIDI_CH_MASK;
	UINT32 R_SourceMIDIMask;
	//UINT32 R_NoteOnHistPtr;
	//UINT32 R_Avail_Voice;
	//void* (*user_memory_malloc)(INT32U size);
	//void (*user_memory_free)(void* buffer_addr);
	void (*MIDI_StopCallBack)(void);
	void (*MIDI_DataEntryEventCallBack)(void);		//20081028 Roy
	void (*MIDI_PlayDtStopCallBack)(void);			//20081028 Roy
	UINT8	MIDI_Control_Event[4];					
	UINT32	MIDI_Current_Dt;						
	UINT32  MIDI_Stop_Dt;							
	UINT8	MIDI_Skip_Flag;							
	
	UINT8 User_FIQ_Flag;
	void (*SPU_User_FIQ_ISR[C_MAX_FIQ])(void);

	UINT8  SPU_load_data_mode;
	UINT32 total_inst;
	UINT32 total_drum;
	UINT32 idi_offset_addr;
	UINT32 currt_midi_data_offset;
	UINT32 static_midi_offset;
	UINT32 static_midi_length;
	UINT32 remain_midi_length;
	UINT32 midi_ring_buffer_addr;
	UINT32 midi_ring_buffer_ri;
	UINT32 midi_ring_buffer_wi;
	UINT32 static_midi_ring_buffer_ri;
	UINT32 static_midi_ring_buffer_wi;
	UINT32 adpcm_comb_offset_addr;
	UINT32 adpcm_ram_buffer_addr;
	UINT32 flag_malloc_adpcm_ram_buffer;
	UINT32 adpcm_data_temp_buffer_addr;
	UINT32 inst_start_addr;
	UINT32 lib_start_addr;
	UINT32 midi_start_addr;

	UINT32 T_InstrumentStartSection[129];
	UINT32 T_InstrumentPitchTable[500];
	UINT32 *T_InstrumentSectionAddr[500];
	UINT32 T_InstrumentSectionPhyAddr[500];
	UINT32 *T_DrumAddr[128];
	UINT32 *T_DrumPhyAddr[128];

	UINT32 T_InstrumentStartSection_1[129];//
	UINT32 T_InstrumentPitchTable_1[500];//
	UINT32 *T_InstrumentSectionAddr_1[500];//
	
	UINT8 T_channel[SPU_ChannelNumber];
	FILE* static_fd_idi;
}STRUCT_MIDI_SPU;
/*
typedef struct{
	UINT8  mixer_channel;//0/1/2
	UINT8  vol_L;
	UINT8	vol_R;
	UINT8  pan_L;
	UINT8	pan_R;
	UINT32 pcm_buf_vir_addr_L[MIXER_SPU_PCM_BUF_NUM];
	UINT32 pcm_buf_vir_addr_R[MIXER_SPU_PCM_BUF_NUM];
	UINT32 pcm_buf_phy_addr_L[MIXER_SPU_PCM_BUF_NUM];
	UINT32 pcm_buf_phy_addr_R[MIXER_SPU_PCM_BUF_NUM];
	UINT32 start_buf_index;
	UINT32 loop_buf_index;
	UINT32 phase;
}STRUCT_MIXER_SPU;
*/
typedef struct{//20110503
	UINT32 *pAddr;
	UINT32 phyAddr;
	UINT8 uiPan;
	UINT8 uiVelocity;
	UINT8 uiSPUChannel;
}STRUCT_DRM_SPU;

#define SPU_INIT	               				_IOW('D', 0x00, STRUCT_MIDI_SPU)			  /* Set PPU Hardware Enable*/
#define SPU_MIDI_PLAY							_IOW('D', 0x01, STRUCT_MIDI_SPU)			  /* play Midi */
#define SPU_MIDI_STOP							_IOW('D', 0x02, STRUCT_MIDI_SPU)			  /* stop Midi */
#define SPU_MIDI_PAUSE							_IOW('D', 0x03, STRUCT_MIDI_SPU)			  /* pasue Midi play*/
#define SPU_MIDI_RESUME							_IOW('D', 0x04, STRUCT_MIDI_SPU)			  /* resume Midi play */
#define SPU_MIDI_SET_VOL						_IOW('D', 0x05, STRUCT_MIDI_SPU)			  /* set Midi volume */
#define SPU_PLAY_TONE							_IOW('D', 0x06, STRUCT_MIDI_SPU)			  /* Play tone from a fixed channel*/
#define SPU_PLAY_DRUM							_IOW('D', 0x07, STRUCT_MIDI_SPU)			  /* Play drum from a fixed channel*/
#define SPU_PLAY_SFX							_IOW('D', 0x08, STRUCT_MIDI_SPU)			  /* Play sound effect from a fixed channel*/

#define SPU_SET_CH_VOL							_IOW('D', 0x09, STRUCT_MIDI_SPU)			  
#define SPU_SET_CH_PAN							_IOW('D', 0x0A, STRUCT_MIDI_SPU)			  
#define SPU_SET_BEAT_BASE_CNT					_IOW('D', 0x0B, STRUCT_MIDI_SPU)			  

#define SPU_MIDI_SET_PAN						_IOW('D', 0x0C, STRUCT_MIDI_SPU)				//pan of midi
#define SPU_MIDI_BUF_WR							_IOW('D', 0x0D, STRUCT_MIDI_SPU)				//
#define SPU_MIDI_BUF_RD							_IOW('D', 0x0E, STRUCT_MIDI_SPU)				//

#define SPU_MIDI_SET_SPU_Channel_Mask				_IOW('D', 0x10, STRUCT_MIDI_SPU)
#define SPU_PLAY_DRM						_IOW('D', 0x18, STRUCT_MIDI_SPU)
//----------------------------------------------------------------------------------------------------------------

extern void SPU_MIDI_INIT(SINT32 handle);
extern SINT32 SPU_PLAY_MW(SINT32 handle, FILE* file_handle, UINT32 midi_index, UINT8 repeat_en);
extern void SPU_STOP_MW(SINT32 handle);
extern void SPU_PAUSE_MW(SINT32 handle);
extern void SPU_RESUME_MW(SINT32 handle);
extern UINT32 SPU_GET_MIDI_STATUS_MW(SINT32 handle);
extern void SPU_SET_VOL_MW(SINT32 handle, UINT32 MIDI_Volume);
extern void SPU_SET_MIDI_PAN_MW(SINT32 handle, UINT32 MIDI_PAN);
extern SINT32 SPU_check_fill_midi_ring_buffer(SINT32 handle);
extern void SPU_PLAY_DRM_MW(SINT32 handle,  STRUCT_DRM_SPU *p_drm_struct);

extern void SPU_MIDI_Set_SPU_Channel_Mask(SINT32 handle, UINT32 SPU_CH_MASK);
extern UINT32 SPU_MIDI_Get_Status(SINT32 handle);
extern void SPU_MIDI_SetMidiMask(SINT32 handle, UINT32 MIDI_Ch_Mask);

#endif
