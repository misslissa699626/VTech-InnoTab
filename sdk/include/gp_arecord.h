#ifndef __AUDIO_RECORD_H__
#define __AUDIO_RECORD_H__

#define INT
//#include "application.h"
//=============================================================
// audio encode configure set 
//=============================================================
//#if(MCU_VERSION == GPL326XX)||(MCU_VERSION == GPL326XX_B)||(MCU_VERSION == GPL326XX_C)
#define ADC_LINE_IN						0
#define	GPY0050_IN						1
#define	BUILD_IN_MIC					2  						//only GPL32600 support
#define AUDIO_INPUT_SRC					BUILD_IN_MIC
//#else
//#define	BUILD_IN_MIC					0xFF	//no built-in MIC for GPL32_A/B/C  	
//#define ADC_LINE_IN						0
//#define	GPY0050_IN						1
//#define AUDIO_INPUT_SRC					BUILD_IN_MIC
//#endif

///#if AUDIO_INPUT_SRC == ADC_LINE_IN || AUDIO_INPUT_SRC == BUILD_IN_MIC
//	#define C_ADC_USE_TIMER				ADC_AS_TIMER_C			//adc:ADC_AS_TIMER_C ~ F, mic: ADC_AS_TIMER_C ~ F
//	#define C_ADC_USE_CHANNEL			ADC_LINE_1				//adc channel: 0 ~ 3 
//#elif AUDIO_INPUT_SRC == GPY0050_IN
//	#define C_AUDIO_RECORD_TIMER		TIMER_B		 			//timer, A,B,C
//	#define C_GPY0050_SPI_CS_PIN		IO_F5					//gpy0500 spi interface cs pin
//#endif

#define APP_MP3_ENCODE_EN   1

#define C_AUD_PCM_BUFFER_NO				2						//pcm buffer number

#define C_A1800_RECORD_BUFFER_SIZE		A18_ENC_FRAMESIZE		//PCM buffer size, fix 320
#define C_WAVE_RECORD_BUFFER_SIZE		1024*16					//PCM buffer size, depend on SR=16KHz 	
#define C_BS_BUFFER_SIZE				1024*64					//file buffer size, fix 64Kbyte 	

#define A1800_TIMES						30						//a1800 encode times, 320*30, <80*30, depend on SR=16KHz
#define ADPCM_TIMES						16						//adpcm encode times, 500*16, 256*16, depend on SR=16KHz
#define MP3_TIME						30						//MP3 encode times, 1152*15

#define LPF_ENABLE						0						//low pass filter enable
//=============================================================
// audio encode status 
//=============================================================
#define C_GP_FS				1
#define C_USER_DEFINE		0

#define C_MONO_RECORD		1
#define C_STEREO_RECORD 	2

#if DBG_MESSAGE == 1
	#define DEBUG_MSG(x)	{x;}
#else
	#define DEBUG_MSG(x)	{}
#endif

#define C_STOP_RECORD		0x00000000
#define C_STOP_RECORDING	0x00000001
#define C_START_RECORD		0x00000002
#define C_PAUSE_RECORD		0x00000003
#define C_PAUSE_RECORDING		0x00000004
#define C_START_FAIL		0x80000001
#define C_START_OPEN_DSP_FAIL		0x80000002
#define C_START_OPEN_MIXER_FAIL		0x80000003
//=======wave format tag====================
#define	WAVE_FORMAT_PCM			(0x0001)
#define	WAVE_FORMAT_ADPCM			(0x0002)
#define	WAVE_FORMAT_ALAW			(0x0006)
#define	WAVE_FORMAT_MULAW			(0x0007)
#define 	WAVE_FORMAT_IMA_ADPCM		(0x0011)

#define 	WAVE_FORMAT_A1800			(0x1000)
#define 	WAVE_FORMAT_MP3			(0x2000)



#define AUD_RECORD_STATUS_OK			0x00000000
#define AUD_RECORD_STATUS_ERR			0x80000001
#define AUD_RECORD_INIT_ERR				0x80000002
#define AUD_RECORD_DMA_ERR				0x80000003
#define AUD_RECORD_RUN_ERR				0x80000004
#define AUD_RECORD_FILE_WRITE_ERR		0x80000005
#define AUD_RECORD_MEMORY_ALLOC_ERR		0x80000006

//gpy0500 command
#define C_CMD_RESET_IN1				0x83
#define C_CMD_RESET_IN4				0x89		
#define C_CMD_ENABLE_ADC			0x98
#define C_CMD_ENABLE_MIC_AGC_ADC	0x9B
#define C_CMD_ENABLE_MIC_ADC		0x9B
#define C_CMD_ENABLE_MIC_AGC		0x93
#define C_CMD_DUMMY_COM				0xC0
#define C_CMD_ADC_IN1				0x82
#define C_CMD_ADC_IN4				0x88
#define C_CMD_ZERO_COM				0x00
#define C_CMD_POWER_DOWN			0x90
#define C_CMD_TEST_MODE				0xF0

typedef enum
{
		AUD_AUTOFORMAT=0,
		MIDI,
		WMA,
		MP3,
		WAV,
		A1800,
		S880,
		A6400,
		A1600, 
		IMA_ADPCM,
		MICROSOFT_ADPCM
} AUDIO_FORMAT;

typedef enum
{
		START_OK=0,
		RESOURCE_NO_FOUND_ERROR,
		RESOURCE_READ_ERROR,
		RESOURCE_WRITE_ERROR,
		CHANNEL_ASSIGN_ERROR,
		REENTRY_ERROR,
		AUDIO_ALGORITHM_NO_FOUND_ERROR,
		CANNOT_OPEN_DSP,
		CANNOT_OPEN_MIXER,
		CODEC_START_STATUS_ERROR_MAX
} CODEC_START_STATUS;

typedef enum
{
		AUDIO_CODEC_PROCESSING=0,					// Decoding or Encoding
		AUDIO_CODEC_PROCESS_END,					// Decoded or Encoded End
		AUDIO_CODEC_BREAK_OFF,						// Due to unexpended card-plug-in-out
		AUDIO_CODEC_PROCESS_PAUSED,
		AUDIO_CODEC_STATUS_MAX
} AUDIO_CODEC_STATUS;

typedef enum
{
		SOURCE_TYPE_FS=0,
		SOURCE_TYPE_SDRAM,
		SOURCE_TYPE_NVRAM,
		SOURCE_TYPE_USER_DEFINE,
		SOURCE_TYPE_FS_RESOURCE_IN_FILE,	// added by Bruce, 2010/01/22
		SOURCE_TYPE_MAX
} SOURCE_TYPE;

typedef enum
{
		IMG_AUTOFORMAT=0,
		JPEG,
		JPEG_P,		// JPEG Progressive
		MJPEG_S,	// Single JPEG from M-JPEG video
		GIF,
		BMP
} IMAGE_FORMAT;

typedef enum
{
		VID_AUTOFORMAT=0,
		MJPEG,
		MPEG4
} VIDEO_FORMAT;

typedef struct {
		SOURCE_TYPE type;
			//0: GP FAT16/32 File System by File SYSTEM 
			//1: Directly from Memory Mapping (SDRAM)
			//2: Directly from Memory Mapping (NVRAM)
			//3: User Defined defined by call out function:audio_encoded_data_read								
		
		struct User							//Source File handle and memory address
		{
				//INT16S		FileHandle;		//File Number by File System or user Define	
				FILE       *FileHandle;//101208
				int      temp;			//Reserve for special use 
				char       *memptr;		//Memory start address					
		}type_ID;
		
		union SourceFormat					//Source File Format
		{
				AUDIO_FORMAT	AudioFormat;		//if NULL,auto detect
				IMAGE_FORMAT	ImageFormat;		//if NULL,auto detect
				VIDEO_FORMAT	VideoFormat;		//if NULL,auto detect
		}Format;
} MEDIA_SOURCE;



typedef enum
{
	MSG_ADC_DMA_DONE = 0x00000001,
	MSG_AUDIO_ENCODE_START = 0x10000000,
	MSG_AUDIO_ENCODE_STOPING,
	MSG_AUDIO_ENCODE_STOP,
	MSG_AUDIO_ENCODE_ERR,
	MSG_AUDIO_ENCODE_PAUSE,
	MSG_AUDIO_ENCODE_PAUSING,
	MSG_AUDIO_ENCODE_RESUME,
	MSG_AUDIO_ENCODE_STOPERR
}AUDIO_RECORD_ENUM;

typedef struct 
{
	char	 		RIFF_ID[4];	//= {'R','I','F','F'};
	unsigned int  	RIFF_len;	//file size -8
	char	 		type_ID[4];	// = {'W','A','V','E'};
	char			fmt_ID[4];	// = {'f', 'm','t',' '};
	unsigned int		fmt_len;	//16 + extern format byte
	unsigned short	format;		// = 1; 	//pcm
	unsigned short	channel;	// = 1;	// mono
	unsigned int		sample_rate;// = 8000;
	unsigned int		avg_byte_per_sec;// = 8000*2;	//AvgBytesPerSec = SampleRate * BlockAlign 
	unsigned short	Block_align;// = (16 / 8*1) ;				//BlockAlign = SignificantBitsPerSample / 8 * NumChannels 
	unsigned short	Sign_bit_per_sample;// = 16;		//8, 16, 24 or 32
	char			data_ID[4];// = {'d','a','t','a'};
	unsigned int		data_len; //extern format byte
}AUD_ENC_WAVE_HEADER;

typedef struct
{
	unsigned int		Status;				
	unsigned int		SourceType;
	//INT16S  FileHandle;
	FILE				*FileHandle;//101208
	unsigned short		Channel;			//1,mono or 2,stereo
	unsigned int		AudioFormat;
	unsigned int		SampleRate;			//sample rate
	unsigned int		BitRate;            //bite rate
	unsigned int		FileLenth;			//byte 
	unsigned long long	NumSamples;			//total samples
	unsigned int		TimeStamp;			//time stamp 
	
#if	APP_DOWN_SAMPLE_EN	
	char			bEnableDownSample;
	char			*DownSampleWorkMem;
	char			DownsampleFactor;
#endif
	
	char			*EncodeWorkMem;		//wrok memory
	char			*Bit_Stream_Buffer;	//encode bit stream buffer
	unsigned int		read_index;			//bit stream buffer index
	unsigned int		PackSize;			//for file write size, byte
	char			*pack_buffer;		//a1800 and wav lib use
	unsigned int		PCMInFrameSize;		//pcm input buffer, short
	unsigned int		OnePCMFrameSize; 	//short
	
	//allow record size
	unsigned long int	disk_free_size;
}Audio_Encode_Para;

//task api
//INT32S adc_record_task_create(INT8U priority);
//INT32S adc_record_task_del(INT8U priority);
//void adc_record_entry(void *param);

//api
void audio_encode_entrance(void);
void audio_encode_exit(void);
CODEC_START_STATUS audio_encode_start(MEDIA_SOURCE src, unsigned short SampleRate, unsigned int BitRate);
int audio_encode_stop(void);
int audio_encode_pause(void);
int audio_encode_resume(void);
AUDIO_CODEC_STATUS audio_encode_status(void);

void audio_record_set_status(unsigned int status);
unsigned int audio_record_get_status(void);
void audio_record_set_source_type(unsigned int type);
unsigned int audio_record_get_source_type(void);
long int audio_record_set_file_handle_free_size(FILE* file_handle);
void audio_record_set_info(unsigned int audio_format, unsigned int sample_rate, unsigned int bit_rate, unsigned short channel);
//INT32S audio_record_set_down_sample(BOOLEAN b_down_sample, INT8U ratio);
unsigned int audio_record_get_timestamp(void);	/*only MP3 is ok now*/
int audio_encode_start_hardware(void);

#if LPF_ENABLE == 1
#define Max_LoopCnt 10  
extern void LPF_init(long coef_frq,short coef_loop);
extern unsigned short LPF_process(unsigned short FilterBuf);
#endif

#endif
