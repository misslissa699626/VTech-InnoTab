/*****************************************************************
|
|   M4A Parser Module
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "stdio.h"
#include "Atomix.h"
#include "BltConfig.h"
#include "BltM4aParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"
#include "unistd.h"



#if 0
#define DEBUG0 printf
#else
	#define DEBUG0(...)
#endif
#if 0
#if 0
#define DEBUG0(args...) fprintf(stderr, args...)
#else
#define DEBUG0(...)
#endif
#endif

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.m4a")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define 	M4A_ID_STSD				0x73747364
#define 	SAMPLEING_TABLE_NUM     9
#define 	AUDIO_AAC_PLUS_OPT      1
#define 	ERR_FILE_NOT_SUPPORT    0x0006
#define VIDEO_TRAK              1
#define AUDIO_TRAK              2
#define INFO_TRAK               3

#define PKT_BUF_SIZE            (16 * 1024)
#define ADDR_ALIGN_SIZE         (1024)
#define VID_DATA_BUF_SIZE       (256 * 1024)
#define AUD_DATA_BUF_SIZE       (32 * 1024)
#define VID_CHECK_BUF_SIZE      (VID_DATA_BUF_SIZE >> 1)
#define AUD_CHECK_BUF_SIZE      (AUD_DATA_BUF_SIZE >> 1) 
#define SAMPLEING_TABLE_NUM     9
#define MP4_BOUNDER_SIZE        (512)
#define MAX_TOTALFRAME_NUM      (60000)


#define USE_LIBAVCODEC		1



#define 	VID_PLAY_ERR01D         (0x42D0 | ERR_FILE_NOT_SUPPORT)     /* 3GP file contains two video or audio tracks */
#define 	VID_PLAY_ERR01E         (0x42E0 | ERR_FILE_NOT_SUPPORT)     /* Doesn't contain video/audio */
#define 	VID_PLAY_ERR002         (0x4120 | ERR_FILE_NOT_SUPPORT)     /* 3GP: entry count of STSZ, STCO... is 0 */

#define FOUR_CC( a, b, c, d ) \
        ( ((BLT_UInt32)a) | ( ((BLT_UInt32)b) << 8 ) \
         | ( ((BLT_UInt32)c) << 16 ) | ( ((BLT_UInt32)d) << 24 ) )

#define VID_UNKNOWN      0x0000

/*WMV Video*/
#define VID_WMV7         FOUR_CC('W','M','V','1')
#define VID_WMV8         FOUR_CC('W','M','V','2')
#define VID_WMV9         FOUR_CC('W','M','V','3')
#define VID_WMVA         FOUR_CC('W','M','V','A')
#define VID_WVC1         FOUR_CC('W','V','C','1')

/*Audio*/
#define AUD_SAMR         FOUR_CC('S','A','M','R')
#define AUD_MP4A         FOUR_CC('M','P','4','A')
#define AUD_SAWB         FOUR_CC('S','A','W','B')
#define AUD_MPL3         FOUR_CC('.','M','P','3')
#define AUD_MSU_         FOUR_CC('M','S',  0,'U')
#define AUD_PCM_         FOUR_CC('R','A','W',' ')
#define AUD_TWOS         FOUR_CC('T','W','O','S')
/* Montion JPEG*/
#define VID_MJPG         FOUR_CC('M','J','P','G')
#define VID_JPEG         FOUR_CC('J','P','E','G')

/* MPEG4 video */
/* 3IVX */
#define VID_3IV1         FOUR_CC('3','I','V','1')
#define VID_3iv1         FOUR_CC('2','i','v','1')
#define VID_3IV2         FOUR_CC('3','I','V','2')
#define VID_3iv2         FOUR_CC('3','i','v','2')
#define VID_3IVD         FOUR_CC('3','I','V','D')
#define VID_3ivd         FOUR_CC('3','i','v','d')
#define VID_3VID         FOUR_CC('3','V','I','D')
#define VID_3vid         FOUR_CC('3','v','i','d')

/* XVID */
#define VID_XVID         FOUR_CC('X','V','I','D')
#define VID_XVIX         FOUR_CC('X','V','I','X')
 
/* DIVX 4/5/6 */	
#define VID_DIVX         FOUR_CC('D','I','V','X')
#define VID_DX50         FOUR_CC('D','X','5','0')
#define VID_BLZ0         FOUR_CC('B','L','Z','0')
#define VID_DXGM         FOUR_CC('D','X','G','M')

/* other MPEG4 */
#define VID_MP4V         FOUR_CC('M','P','4','V')
#define VID_3IVX         FOUR_CC('3','I','V','X')
#define VID_3IV1         FOUR_CC('3','I','V','1')
#define VID_3IV2         FOUR_CC('3','I','V','2')
#define VID_MP4S         FOUR_CC('M','P','4','S')
#define VID_M4S2         FOUR_CC('M','4','S','2')
#define VID_RMP4         FOUR_CC('R','M','P','4')
#define VID_DM4V         FOUR_CC('D','M','4','V')
#define VID_WV1F         FOUR_CC('W','V','1','F')
#define VID_FMP4         FOUR_CC('F','M','P','4')
#define VID_HDX4         FOUR_CC('H','D','X','4')
#define VID_SMP4         FOUR_CC('S','M','P','4')
#define VID_LMP4         FOUR_CC('L','M','P','4')
#define VID_NDIG         FOUR_CC('N','D','I','G')
#define VID_SEDG         FOUR_CC('S','E','D','G')
#define VID_4            FOUR_CC( 4,  0,  0,  0 )

//---------------------------------DIV3--------------------------------
#define VID_DIVX3        (0x1111)
/* MSMPEG4 v1 */
#define VID_DIV1         FOUR_CC('D','I','V','1')
#define VID_MPG4         FOUR_CC('M','P','G','4')
#define VID_mpg4         FOUR_CC('m','p','g','4')
#define VID_MP41         FOUR_CC('M','P','4','1')
#define VID_DIV1         FOUR_CC('D','I','V','1')

/* MSMPEG4 v2 */
#define VID_DIV2         FOUR_CC('D','I','V','2')
#define VID_div2         FOUR_CC('d','i','v','2')
#define VID_MP42         FOUR_CC('M','P','4','2')
#define VID_mp42         FOUR_CC('m','p','4','2')

/* MSMPEG4 v3 / M$ mpeg4 v3 */
#define VID_MPG3         FOUR_CC('M','P','G','3')
#define VID_mpg3         FOUR_CC('m','p','g','3')
#define VID_div3         FOUR_CC('d','i','v','3')
#define VID_MP43         FOUR_CC('M','P','4','3')
#define VID_mp43         FOUR_CC('m','p','4','3')
#define VID_AP41         FOUR_CC('A','P','4','1')
#define VID_DVX3         FOUR_CC('D','V','X','3')
#define VID_COL1         FOUR_CC('C','O','L','1')

/* DivX 3.20 */
#define VID_DIV3         FOUR_CC('D','I','V','3')
#define VID_DIV4         FOUR_CC('D','I','V','4')
#define VID_div4         FOUR_CC('d','i','v','4')
#define VID_DIV5         FOUR_CC('D','I','V','5')
#define VID_div5         FOUR_CC('d','i','v','5')
#define VID_DIV6         FOUR_CC('D','I','V','6')
#define VID_div6         FOUR_CC('d','i','v','6')


//---------------------------------H263----------------------------
/* H263 and H263i */
#define VID_H263         FOUR_CC('H','2','6','3')
#define VID_U263         FOUR_CC('U','2','6','3')
#define VID_I263         FOUR_CC('I','2','6','3')
#define VID_M263         FOUR_CC('M','2','6','3')
#define VID_L263         FOUR_CC('L','2','6','3')
#define VID_X263         FOUR_CC('X','2','6','3')


/* Sorenson H.263*/
#define VID_S263         FOUR_CC('S','2','6','3')

//----------------------------------H264---------------------------
/* H264 */
#define VID_H264         FOUR_CC('H','2','6','4')
#define VID_X264         FOUR_CC('X','2','6','4')
#define VID_VSSH         FOUR_CC('V','S','S','H')
#define VID_DAVC         FOUR_CC('D','A','V','C')
#define VID_PAVC         FOUR_CC('P','A','V','C')
#define VID_AVC1         FOUR_CC('A','V','C','1')

/* MPEG 1/2*/
#define VID_MPEG         FOUR_CC('M','P','E','G')
#define VID_MPG1         FOUR_CC('M','P','G','1')
#define VID_MPG2         FOUR_CC('M','P','G','2')

/*other*/
#define MP4_ESDS         FOUR_CC('E','S','D','S')

#define VID_PLAY_ERR015          0x4250    /* End of read data */

#define VIDEO_TRAK              1
#define AUDIO_TRAK              2
#define INFO_TRAK               3

#define IDX_NUM                 9
#define IDX_VID_STTS            0
#define IDX_VID_STSZ            1
#define IDX_VID_STCO            2
#define IDX_VID_STSS            3
#define IDX_VID_STSC            4
#define IDX_AUD_STSZ            5
#define IDX_AUD_STCO            6
#define IDX_AUD_STSC            7
#define IDX_AUD_STTS            8

#define FIRST_CHUNK_OFFSET      0x20
#define FTYP_BOX_SIZE           0x18
#define BOX_HEADER_SIZE         8

#define _3GP_ID_FTYP            0x66747970
#define _3GP_ID_MDAT            0x6D646174
#define _3GP_ID_MOOV            0x6D6F6F76
#define _3GP_ID_MVHD            0x6D766864
#define _3GP_ID_TKHD            0x746B6864
#define _3GP_ID_TRAK            0x7472616B
#define _3GP_ID_MDIA            0x6D646961
#define _3GP_ID_MDHD            0x6D646864
#define _3GP_ID_MINF            0x6D696E66
#define _3GP_ID_VMHD            0x766D6864
#define _3GP_ID_SMHD            0x736D6864
#define _3GP_ID_STBL            0x7374626C
#define _3GP_ID_STSD            0x73747364
#define _3GP_ID_STSC            0x73747363
#define _3GP_ID_STTS            0x73747473
#define _3GP_ID_STSZ            0x7374737A
#define _3GP_ID_STCO            0x7374636F
#define _3GP_ID_STSS            0x73747373
#define _3GP_ID_HMHD            0x6E6D6864
#define _3GP_ID_NMHD            0x686D6864


#define FRAMERATE_MULTIPLE   (1000)

/* AUDIO */
#define AUD_WAV_ENABLE                           1
#define AUD_MIDI_EXTRA_FMT_ENABLE                1
#define AUD_MA_LED_MTR_ENABLE                    1
#define AUDIO_MP3_OPT                            1
#define AUDIO_AMR_OPT                            1
#define AUDIO_AMRWB_OPT                          1
#define AUDIO_AAC_OPT                            1
#define AUDIO_AAC_PLUS_OPT                       1
#define AUDIO_WAV_OPT                            AUD_WAV_ENABLE
#define AUDIO_WMA_OPT                            1
#define AUDIO_AUTO_GAIN_CTRL                     1
#define SUPPORT_AUDIO_HOST_OPT                   0 /*teddy add@20061106 for play audio from host*/
#define SUPPORT_MIDI_OPT                      1 /* keep 1:enable*/ /*ruilenlu@20070124_[Mantis:7016] for close ma drv in 557 */
#define AUDIO_STREAM_OPT                    1   /*snail@20070327_[Mantis:7940] for audio streaming*/


#if AUDIO_AAC_PLUS_OPT
	BLT_UInt32 isaacPlusPresent;
#endif


/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 m4a_type_id;
} M4aParserModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType    media_type;
    ATX_InputStream* stream;
    BLT_Boolean      eos;
} M4aParserInput;

typedef struct m4a_audio_s{	
	/* Stream Properties Object */	
	BLT_Int16		wAudioStreamId;	
	BLT_Int16		wFormatTag;	
	BLT_Int16		nChannels;	
	BLT_Int32		nSamplesPerSec;	
	BLT_Int32		nAvgBytesPerSec;	
	BLT_Int16		nBlockAlign;	
	BLT_Int16		wBitsPerSample;	
	BLT_Int32		nSamplesPerBlock;	
	BLT_Int16		nEncodeOpt;		
	/* Packet / payload header information */
	BLT_Int8		bStreamId;	
	BLT_Int8		cbRepData;	
	BLT_UInt16 		cbPayloadSize;
}m4a_audio_t;

typedef struct {	
	BLT_MediaType   base;  	
	m4a_audio_t		m4a_audio_info;	
	BLT_UInt32		extra_data_size;
  	BLT_Int8*    	extra_data; 
}BLT_M4aAudioMediaType;


typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_M4aAudioMediaType* media_type;
} M4aParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    M4aParserInput  input;
    M4aParserOutput output;
} M4aParser;

typedef enum SubTitleSupport
{
	SUB_TITLE_NONE = 0,
	SUB_TITLE_TXT,
	SUB_TITLE_PICTURE
}SubTitleSupport_t;

typedef struct _3gpIdxInfo_s {
	BLT_UInt32 offset;
	BLT_UInt32 size;
} _3gpIdxInfo_t;


typedef struct _3gpSTTSInfo_s {
	BLT_UInt32 filePos_start;
	BLT_UInt32 filePos;
	BLT_UInt32 entry_count;
	BLT_UInt32 entry_idx;
	BLT_UInt32 entry_fill;
	BLT_UInt32 sample_count;
	BLT_Int32 sample_idx;
	BLT_UInt32 sample_duration;
} _3gpSTTSInfo_t;


typedef struct _3gpSTSZInfo_s {
	BLT_UInt32 filePos_start;
	BLT_UInt32 filePos;
	BLT_UInt32 sample_size;
	BLT_UInt32 sample_count;
	BLT_UInt32 sample_fill;
	BLT_UInt32 sample_idx;
} _3gpSTSZInfo_t;


typedef struct _3gpSTCOInfo_s {
	BLT_UInt32 filePos_start;
	BLT_UInt32 filePos;
	BLT_UInt32 entry_count;
	BLT_UInt32 entry_fill;
	BLT_UInt32 entry_idx;
} _3gpSTCOInfo_t;


typedef struct _3gpSTSCInfo_s {
	BLT_UInt32 filePos_start;
	BLT_UInt32 filePos;
	BLT_UInt32 entry_count;
	BLT_UInt32 entry_fill;
	BLT_UInt32 entry_idx;
	BLT_UInt32 sample_count;
	BLT_UInt32 sample_idx;
	BLT_UInt32 curr_chunk_id;
	BLT_UInt32 next_chunk_id;
	BLT_UInt32 isLastChunk;
	BLT_UInt8  isFirstFilled;
} _3gpSTSCInfo_t;


typedef struct _3gpSTSSInfo_s {
	BLT_UInt32 filePos_start;
	BLT_UInt32 filePos;
	BLT_UInt32 entry_count;
	BLT_UInt32 entry_fill;
	BLT_UInt32 entry_idx;
} _3gpSTSSInfo_t;


typedef struct bufInfo_s {
	BLT_UInt32 offsetInFileStart;
	BLT_UInt32 offsetInFileEnd;
	BLT_UInt32 bufSize;
	BLT_UInt8* pbufStart;
	BLT_UInt8* pbufEnd;
} bufInfo_t;

typedef struct mp4_wav_format_s {
	BLT_UInt16 wFormatTag;  
	BLT_UInt16 nChannels;        
	BLT_UInt32 nSamplesPerSec;     
	BLT_UInt32 nAvgBytesPerSec;
	BLT_UInt16 nBlockAlign;      
	BLT_UInt16 wBitsPerSample;    
	BLT_UInt16 cbSize;          
	BLT_UInt16 wSamplesPerBlock;
} mp4_wav_format_t;


// define for audio element stream
typedef struct audio_info_s 
{
	BLT_UInt32 es_id;// the id of audio element stream in all element streams
	BLT_UInt32 code_format;// audio codec standard
	BLT_UInt32 channel_cnt;// audio codec channel count
	BLT_UInt32 sample_rate;// audio codec sample rate
	BLT_UInt32 bit_rate;// audio codec bit rate, unit in bps
	BLT_UInt32 block_align;// audio duration, unit in ms
	BLT_UInt32 duration;// audio duration, unit in ms
	BLT_UInt32 v_es_id; // the  element stream id of video that attend by this audio
	BLT_UInt32 start_time;// audio begin time, unit in ms
	BLT_UInt32 audBits;
	BLT_UInt32 vbr;
	BLT_UInt32 seek_mode;	   // 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	BLT_UInt32 extra_data_size;// audio extra data size, unit in byte
	BLT_UInt8 *extra_data;// audio extra data
	//struct media_contain_s *contain;
	
}audio_info_t;


enum CodecID {
    CODEC_ID_NONE,

    /* video codecs */
    CODEC_ID_MPEG1VIDEO,
    CODEC_ID_MPEG2VIDEO, ///< preferred ID for MPEG-1/2 video decoding
    CODEC_ID_MPEG2VIDEO_XVMC,
    CODEC_ID_H261,
    CODEC_ID_H263,
    CODEC_ID_RV10,
    CODEC_ID_RV20,
    CODEC_ID_MJPEG,
    CODEC_ID_MJPEGB,
    CODEC_ID_LJPEG,
    CODEC_ID_SP5X,
    CODEC_ID_JPEGLS,
    CODEC_ID_MPEG4,
    CODEC_ID_RAWVIDEO,
    CODEC_ID_MSMPEG4V1,
    CODEC_ID_MSMPEG4V2,
    CODEC_ID_MSMPEG4V3,
    CODEC_ID_WMV1,
    CODEC_ID_WMV2,
    CODEC_ID_H263P,
    CODEC_ID_H263I,
    CODEC_ID_FLV1,
    CODEC_ID_SVQ1,
    CODEC_ID_SVQ3,
    CODEC_ID_DVVIDEO,
    CODEC_ID_HUFFYUV,
    CODEC_ID_CYUV,
    CODEC_ID_H264,
    CODEC_ID_INDEO3,
    CODEC_ID_VP3,
    CODEC_ID_THEORA,
    CODEC_ID_ASV1,
    CODEC_ID_ASV2,
    CODEC_ID_FFV1,
    CODEC_ID_4XM,
    CODEC_ID_VCR1,
    CODEC_ID_CLJR,
    CODEC_ID_MDEC,
    CODEC_ID_ROQ,
    CODEC_ID_INTERPLAY_VIDEO,
    CODEC_ID_XAN_WC3,
    CODEC_ID_XAN_WC4,
    CODEC_ID_RPZA,
    CODEC_ID_CINEPAK,
    CODEC_ID_WS_VQA,
    CODEC_ID_MSRLE,
    CODEC_ID_MSVIDEO1,
    CODEC_ID_IDCIN,
    CODEC_ID_8BPS,
    CODEC_ID_SMC,
    CODEC_ID_FLIC,
    CODEC_ID_TRUEMOTION1,
    CODEC_ID_VMDVIDEO,
    CODEC_ID_MSZH,
    CODEC_ID_ZLIB,
    CODEC_ID_QTRLE,
    CODEC_ID_SNOW,
    CODEC_ID_TSCC,
    CODEC_ID_ULTI,
    CODEC_ID_QDRAW,
    CODEC_ID_VIXL,
    CODEC_ID_QPEG,
    CODEC_ID_XVID,
    CODEC_ID_PNG,
    CODEC_ID_PPM,
    CODEC_ID_PBM,
    CODEC_ID_PGM,
    CODEC_ID_PGMYUV,
    CODEC_ID_PAM,
    CODEC_ID_FFVHUFF,
    CODEC_ID_RV30,
    CODEC_ID_RV40,
    CODEC_ID_VC1,
    CODEC_ID_WMV3,
    CODEC_ID_LOCO,
    CODEC_ID_WNV1,
    CODEC_ID_AASC,
    CODEC_ID_INDEO2,
    CODEC_ID_FRAPS,
    CODEC_ID_TRUEMOTION2,
    CODEC_ID_BMP,
    CODEC_ID_CSCD,
    CODEC_ID_MMVIDEO,
    CODEC_ID_ZMBV,
    CODEC_ID_AVS,
    CODEC_ID_SMACKVIDEO,
    CODEC_ID_NUV,
    CODEC_ID_KMVC,
    CODEC_ID_FLASHSV,
    CODEC_ID_CAVS,
    CODEC_ID_JPEG2000,
    CODEC_ID_VMNC,
    CODEC_ID_VP5,
    CODEC_ID_VP6,
    CODEC_ID_VP6F,
    CODEC_ID_TARGA,
    CODEC_ID_DSICINVIDEO,
    CODEC_ID_TIERTEXSEQVIDEO,
    CODEC_ID_TIFF,
    CODEC_ID_GIF,
    CODEC_ID_FFH264,
    CODEC_ID_DXA,
    CODEC_ID_DNXHD,
    CODEC_ID_THP,
    CODEC_ID_SGI,
    CODEC_ID_C93,
    CODEC_ID_BETHSOFTVID,
    CODEC_ID_PTX,
    CODEC_ID_TXD,
    CODEC_ID_VP6A,
    CODEC_ID_AMV,
    CODEC_ID_VB,
    CODEC_ID_PCX,
    CODEC_ID_SUNRAST,
    CODEC_ID_INDEO4,
    CODEC_ID_INDEO5,
    CODEC_ID_MIMIC,
    CODEC_ID_RL2,
    CODEC_ID_8SVX_EXP,
    CODEC_ID_8SVX_FIB,
    CODEC_ID_ESCAPE124,
    CODEC_ID_DIRAC,
    CODEC_ID_BFI,
    CODEC_ID_CMV,
    CODEC_ID_MOTIONPIXELS,
    CODEC_ID_TGV,
    CODEC_ID_TGQ,
    CODEC_ID_TQI,

    /* various PCM "codecs" */
    CODEC_ID_PCM_S16LE= 0x10000,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_PCM_U16LE,
    CODEC_ID_PCM_U16BE,
    CODEC_ID_PCM_S8,
    CODEC_ID_PCM_U8,
    CODEC_ID_PCM_MULAW,
    CODEC_ID_PCM_ALAW,
    CODEC_ID_PCM_S32LE,
    CODEC_ID_PCM_S32BE,
    CODEC_ID_PCM_U32LE,
    CODEC_ID_PCM_U32BE,
    CODEC_ID_PCM_S24LE,
    CODEC_ID_PCM_S24BE,
    CODEC_ID_PCM_U24LE,
    CODEC_ID_PCM_U24BE,
    CODEC_ID_PCM_S24DAUD,
    CODEC_ID_PCM_ZORK,
    CODEC_ID_PCM_S16LE_PLANAR,
    CODEC_ID_PCM_DVD,
    CODEC_ID_PCM_F32BE,
    CODEC_ID_PCM_F32LE,
    CODEC_ID_PCM_F64BE,
    CODEC_ID_PCM_F64LE,

    /* various ADPCM codecs */
    CODEC_ID_ADPCM_IMA_QT= 0x11000,
    CODEC_ID_ADPCM_IMA_WAV,
    CODEC_ID_ADPCM_IMA_DK3,
    CODEC_ID_ADPCM_IMA_DK4,
    CODEC_ID_ADPCM_IMA_WS,
    CODEC_ID_ADPCM_IMA_SMJPEG,
    CODEC_ID_ADPCM_MS,
    CODEC_ID_ADPCM_4XM,
    CODEC_ID_ADPCM_XA,
    CODEC_ID_ADPCM_ADX,
    CODEC_ID_ADPCM_EA,
    CODEC_ID_ADPCM_G726,
    CODEC_ID_ADPCM_CT,
    CODEC_ID_ADPCM_SWF,
    CODEC_ID_ADPCM_YAMAHA,
    CODEC_ID_ADPCM_SBPRO_4,
    CODEC_ID_ADPCM_SBPRO_3,
    CODEC_ID_ADPCM_SBPRO_2,
    CODEC_ID_ADPCM_THP,
    CODEC_ID_ADPCM_IMA_AMV,
    CODEC_ID_ADPCM_EA_R1,
    CODEC_ID_ADPCM_EA_R3,
    CODEC_ID_ADPCM_EA_R2,
    CODEC_ID_ADPCM_IMA_EA_SEAD,
    CODEC_ID_ADPCM_IMA_EA_EACS,
    CODEC_ID_ADPCM_EA_XAS,
    CODEC_ID_ADPCM_EA_MAXIS_XA,
    CODEC_ID_ADPCM_IMA_ISS,

    /* AMR */
    CODEC_ID_AMR_NB= 0x12000,
    CODEC_ID_AMR_WB,

    /* RealAudio codecs*/
    CODEC_ID_RA_144= 0x13000,
    CODEC_ID_RA_288,

    /* various DPCM codecs */
    CODEC_ID_ROQ_DPCM= 0x14000,
    CODEC_ID_INTERPLAY_DPCM,
    CODEC_ID_XAN_DPCM,
    CODEC_ID_SOL_DPCM,

    /* audio codecs */
    CODEC_ID_MP2= 0x15000,
    CODEC_ID_MP3, ///< preferred ID for decoding MPEG audio layer 1, 2 or 3
    CODEC_ID_AAC,
    CODEC_ID_AC3,
    CODEC_ID_DTS,
    CODEC_ID_VORBIS,
    CODEC_ID_DVAUDIO,
    CODEC_ID_WMAV1,
    CODEC_ID_WMAV2,
    CODEC_ID_MACE3,
    
    CODEC_ID_MACE6,
    CODEC_ID_VMDAUDIO,
    CODEC_ID_SONIC,
    CODEC_ID_SONIC_LS,
    CODEC_ID_FLAC,
    CODEC_ID_MP3ADU,
    CODEC_ID_MP3ON4,
    CODEC_ID_SHORTEN,
    CODEC_ID_ALAC,
    CODEC_ID_WESTWOOD_SND1,
    
    CODEC_ID_GSM, ///< as in Berlin toast format
    CODEC_ID_QDM2,
    CODEC_ID_COOK,
    CODEC_ID_TRUESPEECH,
    CODEC_ID_TTA,
    CODEC_ID_SMACKAUDIO,
    CODEC_ID_QCELP,
    CODEC_ID_WAVPACK,
    CODEC_ID_DSICINAUDIO,
    CODEC_ID_IMC,
    CODEC_ID_MUSEPACK7,
    CODEC_ID_MLP,
    CODEC_ID_GSM_MS, /* as found in WAV */
    CODEC_ID_ATRAC3,
    CODEC_ID_VOXWARE,
    CODEC_ID_APE,
    CODEC_ID_NELLYMOSER,
    CODEC_ID_MUSEPACK8,
    CODEC_ID_SPEEX,
    CODEC_ID_WMAVOICE,
    CODEC_ID_WMAPRO,
    CODEC_ID_WMALOSSLESS,
    CODEC_ID_ATRAC3P,
    CODEC_ID_EAC3,
    CODEC_ID_SIPR,
    CODEC_ID_MP1,
	CODEC_ID_AAC_PLUS, /* for gp */
	CODEC_ID_MP2A,/* for gp */
	CODEC_ID_MP4A,/* for gp */
	CODEC_ID_A52,/* for gp */

    /* subtitle codecs */
    CODEC_ID_DVD_SUBTITLE= 0x17000,
    CODEC_ID_DVB_SUBTITLE,
    CODEC_ID_TEXT,  ///< raw UTF-8 text
    CODEC_ID_XSUB,
    CODEC_ID_SSA,
    CODEC_ID_MOV_TEXT,

    /* other specific kind of codecs (generally used for attachments) */
    CODEC_ID_TTF= 0x18000,

    CODEC_ID_PROBE= 0x19000, ///< codec_id is not known (like CODEC_ID_NONE) but lavf should attempt to identify it

    CODEC_ID_MPEG2TS= 0x20000, /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                * stream (only used by libavformat) */
};




typedef enum vidVideoType 
{ 
	VIDEO_TYPE_NONE = 0, 
	VIDEO_TYPE_MPEG4_SP,		// mpeg4	sp
	VIDEO_TYPE_MPEG4_ASP,	// mpeg4 asp
	VIDEO_TYPE_MJPEG, 
	VIDEO_TYPE_H263, 
	VIDEO_TYPE_S263, /*for Sorenson H.263*/ 
	VIDEO_TYPE_H264_BP, //6:	
	VIDEO_TYPE_H264_MP, 		// not supported
	VIDEO_TYPE_H264_HP, 		// not supported
	VIDEO_TYPE_WMV7,			// not supported
	VIDEO_TYPE_WMV8,	//10:		// not supported
	VIDEO_TYPE_VC1_SP, /*for VC1 SP */ 
	VIDEO_TYPE_VC1_AP, /*for VC1 MP and AP */	// not supported 
	VIDEO_TYPE_MPEG2, //13: 		 
	VIDEO_TYPE_MPEG1,			
	VIDEO_TYPE_RV10, //15:			// not supported 
	VIDEO_TYPE_RV20, /* RVG2 */ // not supported 
	VIDEO_TYPE_RV30, /* RV8 */ 
	VIDEO_TYPE_RV40, /* RV9 and RV10 */ 
	VIDEO_TYPE_DIV3, //19:		// not supported
	VIDEO_TYPE_JPG,	
	VIDEO_TYPE_MPEG4_HD,
	VIDEO_TYPE_THEORA,
	VIDEO_TYPE_VP6,
	
	SUBTITAL_TYPE_SPU_DVB
} vidVideoType_t;


	typedef enum Video3GPType
	{
		VIDEO_3GP_TYPE_NONE = 0,
		VIDEO_3GP_TYPE_MPEG4,
		VIDEO_3GP_TYPE_H264
	}Video3GPType_E;



	//typedef enum CodecID audAudioType_t
#define audAudioType_t      enum CodecID
	
#define AUDIO_TYPE_NONE     CODEC_ID_NONE
#define AUDIO_TYPE_AC3      CODEC_ID_AC3
#define AUDIO_TYPE_A52      CODEC_ID_A52
#define AUDIO_TYPE_MP2      CODEC_ID_MP2
#define AUDIO_TYPE_MP3      CODEC_ID_MP3
#define AUDIO_TYPE_MP2A     CODEC_ID_MP2A
#define AUDIO_TYPE_MP4A     CODEC_ID_MP4A
#define AUDIO_TYPE_AAC      CODEC_ID_AAC
#define AUDIO_TYPE_AAC_PLUS CODEC_ID_AAC_PLUS
#define AUDIO_TYPE_AMR_NB   CODEC_ID_AMR_NB
#define AUDIO_TYPE_AMR_WB   CODEC_ID_AMR_WB
#define AUDIO_TYPE_FLAC     CODEC_ID_FLAC
#define AUDIO_TYPE_APE      CODEC_ID_APE
#define AUDIO_TYPE_COOK     CODEC_ID_COOK
#define AUDIO_TYPE_WMA      CODEC_ID_WMAV1
#define AUDIO_TYPE_WMA2     CODEC_ID_WMAV2
#define AUDIO_TYPE_WMAPRO   CODEC_ID_WMAPRO
	
#define AUDIO_TYPE_PCM      CODEC_ID_PCM_S16LE // need fix
#define AUDIO_TYPE_ADPCM    CODEC_ID_ADPCM_MS  // need fix
#define AUDIO_TYPE_ADPCM_IMA_WAV    CODEC_ID_ADPCM_IMA_WAV  // need fix
#define AUDIO_TYPE_VORBIS   CODEC_ID_VORBIS  // need fix	




typedef struct ContainBitsInfo_s 
{
	vidVideoType_t vidType;
	audAudioType_t audType;       /* e.g. AMR, AAC, etc */
	unsigned int width;         /* pixels in x-axis */
	unsigned int height;        /* pixels in y-axis */
	unsigned int frmRate;       /* frame rate (fps) */
	unsigned int vopTimeIncLen; /* time increment length */
	unsigned int dpEn;          /* Data Partitioned enable */
	unsigned int rvlcEn;        /* RVLC enable */
	unsigned int gobEn;         /* GOB enable */
	unsigned int gopNo;         /* GOP number (length between two I frames) */
	unsigned int vidLen;        /* video total million seconds */
	unsigned int audLen;        /* audio total million seconds */
	unsigned int TotalFrameCount;  /*total video frame number*/
	unsigned int IFrameCount;      /*video I frame number*/
	unsigned int vidBitrate;    /*video bitrate*/
	unsigned int vInterFlag;    /*the video is interleaved*/
	SubTitleSupport_t enSubTitleSup;

	unsigned int vidExtraLen;   /* The count in bytes of the size of  video extra information */ 
	unsigned char* pVidExtraData; /* video extra information */
	unsigned int audExtraLen;   /* The count in bytes of the size of  audio extra information */ 
	unsigned char* pAudExtraData; /* audio extra information */	
	unsigned int vidStreamNum;  /* video stream number */
	unsigned int audStreamNum;  /* audio stream number */

	unsigned int audBitrate;    /* audio bitrate*/
	unsigned short audSR;         /* audio sample rate */
	unsigned char  audChannel;    /* audio channel */
	unsigned char  audBits;       /* Bits of per audio sample */
	unsigned char  bSeekable;     /* Can be seek or not */ 
    /* francis@20061121 : Add PMP Video Record Rotate flow */
	unsigned char  rotate;        /* record rotate */
	unsigned char  reserved[2];
	unsigned char  isVBR; /*Audio:0-noVBR 1-VBR 2-Unknown*/
	unsigned short uiBlockAlign;  /*++lyh@20070202_[mantis:6219] for asf*/	
	unsigned int clusterSize;     /* jerry: add for cluster alighment */
	unsigned int targetDisk;	/* jerry: save target storage, e.g. sd, nand  */
}ContainBitsInfo_t;


#define _3GP_IDX_VID_STTS_BUF_SIZE            (340*1024)
#define _3GP_IDX_VID_STSZ_BUF_SIZE            (320*1024)
#define _3GP_IDX_VID_STCO_BUF_SIZE            (320*1024)
#define _3GP_IDX_VID_STSS_BUF_SIZE            (128*1024)
#define _3GP_IDX_VID_STSC_BUF_SIZE            (32*1024)
#define _3GP_IDX_AUD_STSZ_BUF_SIZE            (200*1024)
#define _3GP_IDX_AUD_STCO_BUF_SIZE            (200*1024)
#define _3GP_IDX_AUD_STSC_BUF_SIZE            (32*1024)
#define _3GP_IDX_AUD_STTS_BUF_SIZE            (16*1024)

static BLT_UInt32 IDX_VID_STTS_BUF_SIZE = _3GP_IDX_VID_STTS_BUF_SIZE;
static BLT_UInt32 IDX_VID_STSZ_BUF_SIZE = _3GP_IDX_VID_STSZ_BUF_SIZE;
static BLT_UInt32 IDX_VID_STCO_BUF_SIZE = _3GP_IDX_VID_STCO_BUF_SIZE;
static BLT_UInt32 IDX_VID_STSS_BUF_SIZE = _3GP_IDX_VID_STSS_BUF_SIZE;
static BLT_UInt32 IDX_VID_STSC_BUF_SIZE = _3GP_IDX_VID_STSC_BUF_SIZE;
static BLT_UInt32 IDX_AUD_STSZ_BUF_SIZE = _3GP_IDX_AUD_STSZ_BUF_SIZE;
static BLT_UInt32 IDX_AUD_STCO_BUF_SIZE = _3GP_IDX_AUD_STCO_BUF_SIZE;
static BLT_UInt32 IDX_AUD_STSC_BUF_SIZE = _3GP_IDX_AUD_STSC_BUF_SIZE;
static BLT_UInt32 IDX_AUD_STTS_BUF_SIZE = _3GP_IDX_AUD_STTS_BUF_SIZE;

#define IDX_NUM                 9

static BLT_UInt8* p3GPIdxBufStart[IDX_NUM];
static BLT_UInt8* p3GPIdxBufCurr[IDX_NUM];
static BLT_UInt8* p3GPIdxBufEnd[IDX_NUM];
static BLT_UInt32 g3gpVidLastTime; /* The continue time of the video sample */
static BLT_UInt64 gVidSampleTotalTime;
static BLT_UInt32 gFirstFillVidData;
static BLT_UInt32 gVidFrameNumber;
static BLT_UInt32 gVidKeyFrame;
static BLT_UInt64 gAudSampleTotalTime;
static BLT_UInt32 guTotalFrame;
static BLT_UInt32 guiVidTotalLen;
static BLT_UInt32 gAudTotalFrameNum;/*++snail@20061212_[mantis:6177] for speed playback too long time audio time out*/
static BLT_UInt32 guiVidFramerate;
static audAudioType_t  gAudType;
static BLT_UInt32 gAudSR;
static BLT_UInt32 guiFileSize;
static BLT_UInt8* p3GPPktBufStart;
static BLT_UInt32 mdatOffset;
static BLT_UInt32 vidTimeScale, audTimeScale;
static BLT_UInt32 vidFirstSampleOffset, vidFirstSampleSize;
static BLT_UInt8 gbVidTrakExist, gbAudTrakExist;
static BLT_UInt32 gbvidHeaderSize;
static BLT_UInt8*	pvidHeaderData;
static BLT_UInt8*	pvidH264Data;
static BLT_UInt8*	pAudExtraData;
static BLT_UInt8  gbVidSTSSExist;
static bufInfo_t _3gpBufInfo[2];
static BLT_UInt8  gblengthSizeMinusOne;
static BLT_UInt32 vidNextOffset, vidNextSize;
static BLT_UInt32 audNextOffset, audNextSize;

static BLT_UInt32 g3gpVidLastTime; /* The continue time of the video sample */

static BLT_UInt64 gVidSampleTotalTime;
static BLT_UInt32 gFirstFillVidData;
static BLT_UInt32 gVidFrameNumber;
static BLT_UInt32 gVidKeyFrame;
static BLT_UInt64 gAudSampleTotalTime;
static BLT_UInt32 guTotalFrame;
static BLT_UInt32 gAudTotalFrameNum;/*++snail@20061212_[mantis:6177] for speed playback too long time audio time out*/
static BLT_UInt32 guiVidFramerate;
static BLT_UInt32 gAudSR;


static  _3gpSTTSInfo_t _3gpVidSTTS, _3gpAudSTTS;
static  _3gpSTSSInfo_t _3gpVidSTSS, _3gpAudSTSS;
static  _3gpSTSZInfo_t _3gpVidSTSZ, _3gpAudSTSZ;
static  _3gpSTCOInfo_t _3gpVidSTCO, _3gpAudSTCO;
static  _3gpSTSCInfo_t _3gpVidSTSC, _3gpAudSTSC;


static const BLT_UInt32 samplingTable[SAMPLEING_TABLE_NUM] = {
	48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000
};

static BLT_UInt32 gCustomKey = 0x504E5553;



static Video3GPType_E en3GPVType;
static BLT_UInt8 ucMustSeekKeyFrame;



typedef struct mp4_frame_s
{
	BLT_UInt32 pts;
	BLT_UInt32 size;
	BLT_UInt32 type;
}mp4_frame_t;


// define for video element stream
typedef struct video_info_s 
{
	BLT_UInt32 es_id;// the id of video element stream in all element streams
	BLT_UInt32 code_format;// video codec standard
	BLT_UInt32 width, height;// unit in pixel
	BLT_UInt32 frame_rate;// unit in fps
	BLT_UInt32 bit_rate;// unit in bps
	BLT_UInt32 interlace;// if 1, video codec used interlace , or not
	BLT_UInt32 duration;// video duration, unit in ms
	BLT_UInt32 cfps; // if 1, video is constant fps, or not
	BLT_UInt32 apect;// display apect
	BLT_UInt32 start_time;// video begin time, unit in ms
	BLT_UInt32 frame_interval; // unit in ms
	BLT_UInt32 decode_size;   // min 5s data, max 500 * 1024 for 720p 
	BLT_UInt32 seek_mode;	   // 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	struct media_contain_s *contain;// the contain that include the video es 
	// find frame start position
	BLT_Int32 (*sync_frame)(BLT_UInt8 *buf, BLT_UInt32 size);// find the begin position of frame in es buffer
	// parse frame type
	BLT_Int32 (*parse_frame)(BLT_UInt8 *buf, BLT_UInt32 size);// parse frame type
}video_info_t;

// define for media contain
typedef struct media_contain_s 
{
	BLT_UInt32 contain_format;
	// function
	// probe the file if adapter this contain
	BLT_Int32 (*probe)(BLT_Int32 fd); 
	// probe the file if adapter this contain by ext_name that must be lowercase
	BLT_Int32 (*probe_ext)(BLT_UInt8 *ext_name); 
	// contain needed memory size
	BLT_Int32 (*mem_size)(void); 
	// init contain
	BLT_Int32 (*init)(struct media_demux_s *demux, BLT_UInt8 *buf); 
	// uninit contain
	BLT_Int32 (*uninit)(void);
	// probe all element streams in contain
	BLT_Int32 (*probe_es)(BLT_UInt32 flag); 
	// choose appointed element stream to play by flag(video, audio, sub)
	// if id = -1, don't parse video or audio or sub by flag
	BLT_Int32 (*play_es)(BLT_Int32 idx, BLT_UInt32 flag); 
	// parse element stream by flag(video, audio, sub)
	BLT_Int32 (*parse)(BLT_UInt32 flag); 
	// load unfilled es packets by flag(video, audio, sub)
	BLT_Int32 (*load)(BLT_UInt32 flag); 
	// seek es to dest by flag(video, audio, sub)
	BLT_Int32 (*seek)(BLT_UInt32 *dest, BLT_UInt32 mode, BLT_UInt32 flag); 
	// get play time length
	BLT_Int32 (*duration)(BLT_Int32 fd, BLT_UInt32 fast_flag);
}media_contain_t;

typedef struct es_packet_s
{
	BLT_UInt8 *ptr_data; // packet prt
	BLT_UInt32 size; // unit in byte
	BLT_UInt32 frame_type; // 0: I, 1: P, 2: B
	BLT_UInt32 pts; // presentation time, unit in ms
	BLT_UInt32 duration; // for audio es data 
	BLT_UInt32 base_time; // base_time + pts = show time, unit in ms
	BLT_UInt32 sync_request; //0: no, 1: only video, 2: only audio, 3: audio / video at the sync time
	BLT_UInt64 url_pos; //unit in bytes, 
	BLT_UInt32 flag; // frame_start/end, seq_end, no_fill;
	BLT_UInt32 addr; // this es packet struct malloc address
	struct es_packet_s *next;
}es_packet_t;


typedef struct mesp_s
{
	BLT_UInt8 *buf;     // buffer address
	BLT_UInt32 size;     // buffer size
	BLT_UInt8 *w, *r, *e; // buffer write, read ,end address
	es_packet_t *first_esp; // ready packet link
	es_packet_t *last_esp;
	es_packet_t *first_unfill_esp; 
	BLT_UInt32 delay_time; // last parsed valid pts
	BLT_UInt32 delay_size; // parsed size from last parsed pts
	BLT_UInt32 self_malloc;     // buffer address
	BLT_UInt32 bottom_max_size;// the bottom max size for probe frame end
	BLT_UInt32 count;
	void (*mutex_lock)(void);
	void (*mutex_unlock)(void);
	// set the bottom max size
	void (*set_bottom_max_size)(struct mesp_s *mesp, BLT_UInt32 size);
	BLT_Int32 (*get_bottom_max_size)(struct mesp_s *mesp);
	// get total size of filled data
	BLT_Int32 (*fill_size)(struct mesp_s *mesp);
	// get bottom size of filled data
	BLT_Int32 (*fill_bot_size)(struct mesp_s *mesp);
	// get idle buffer, space size, bottom size
	BLT_UInt32 *(*buf_space)(struct mesp_s *mesp, BLT_UInt32 *space_size, BLT_UInt32 *bottom_size);
	// add a packet to ready packet link
	BLT_Int32 (*add)(struct mesp_s *mesp, es_packet_t *esp, BLT_UInt32 flag);
	// turn write ptr to begin
	BLT_Int32 (*turn)(struct mesp_s *mesp);
	// update info for not fill esp
	BLT_Int32 (*update)(struct mesp_s *mesp, es_packet_t *esp, BLT_UInt32 flag);
	// get a packet from ready packet link
	es_packet_t *(*get)(struct mesp_s *mesp, BLT_UInt32 flag);
	// check video frame
	BLT_Int32 (*check_frame)(struct mesp_s *mesp);
	// clear ready packet link
	BLT_Int32 (*clear)(struct mesp_s *mesp);
	// skip some packet in ready packet link, 
	// flag: 0: skip "size" bytes, 1: skip "size" packet
	BLT_Int32 (*skip)(struct mesp_s *mesp, BLT_UInt32 size, BLT_UInt32 flag);
	// creat a new packet
	es_packet_t *(*newp)(void);
	// free a packet
	BLT_Int32 (*freep)(es_packet_t *esp);
	// find frame start position
	BLT_Int32 (*sync_frame)(BLT_UInt8 *buf, BLT_UInt32 size);
	// parse frame type
	BLT_Int32 (*parse_frame)(BLT_UInt8 *buf, BLT_UInt32 size);
}mesp_t;





typedef struct sub_info_s 
{
	BLT_UInt32 es_id;// the id of subtitle element stream in all element streams
	BLT_UInt32 code_format; // text, picture in contain, or not in contain
	BLT_UInt32 type; // text, picture
	BLT_UInt32 start_time;// subtitle begin time, unit in ms
	BLT_UInt8 name[8];// the language name of the subtitle
	BLT_UInt32 seek_mode;	   // 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	struct media_contain_s *contain;// the contain that include the subtitle es 
}sub_info_t;


// define demux es max count
#define MEDIA_VES_MAX_CNT     8
#define MEDIA_AES_MAX_CNT     16
#define MEDIA_SES_MAX_CNT     16


// define for demux media stream
typedef struct media_demux_s 
{
	// the count of element stream in all contains
	BLT_UInt32 v_cnt, a_cnt, s_cnt;
	// the index of playing element stream
	BLT_UInt32 play_ves, play_aes, play_ses;
	// the array of element stream information
	video_info_t v_es[MEDIA_VES_MAX_CNT];
	audio_info_t a_es[MEDIA_AES_MAX_CNT];
	sub_info_t s_es[MEDIA_SES_MAX_CNT];
	// 0: disable seek, 1: seek time, 2: seek bytes, 4: seek 1/100 file size
	BLT_UInt32 seek_mode;
	BLT_UInt32 bookmark_mode;
	BLT_UInt32 reloadable;
	BLT_UInt32 play_ess; // 0: no play es, 1: only video, 2: olny audio, 3: video and audio, &=4 > 0: have sub

	// input from media control
	BLT_Int8 *file_name; // url name
	BLT_UInt32 d_addr;// all memory address
	BLT_UInt32 c_size;// contain memory size
	BLT_Int32 c_fd; // contain/video file handle
	BLT_Int32 a_fd; // audio file handle
	BLT_Int32 st_fd; // sub text file handle
	BLT_Int32 sp_fd; // sub picture file handle
	BLT_UInt64 length; // ms
	// the mesp of playing element
	mesp_t v_mesp, a_mesp, s_mesp;
	// the contain of playing element
	media_contain_t *v_contain, *a_contain, *s_contain;
	// the min limit time length of es that store in buffer unit in ms
	// if less than it, need to fill element stream data to buffer
	BLT_Int32 es_delay_time;
	BLT_Int32 ves_delay_time, aes_delay_time;
	BLT_Int32 trans_ves; // transform packet to frame
	BLT_Int32 fast_step;
	BLT_Int32 fast_flag;
	BLT_Int32 rgb_buf[256]; // decode vob subpic 
	BLT_Int8 *subtital_buf;
	media_contain_t *init_contain;

	BLT_Int32 parsed_pts; // the pts of parsed es
}media_demux_t;




typedef struct mp4_parse_s
{
	BLT_UInt32 vid;
	BLT_UInt32 aid;
	BLT_Int32 fd, seek_fd;
	BLT_UInt64 file_size;
	BLT_UInt32 start_pos;
	BLT_UInt32 end_time;
	BLT_UInt32 base_time;
	BLT_UInt32 base_time_per;
	BLT_UInt32 time_len;
	BLT_UInt32 seek_step;
	
	BLT_Int32 file_end;
	BLT_UInt64 file_pos;
	BLT_UInt32 cur_time;
	BLT_UInt32 start_time;
	BLT_UInt32 sync_req;
	BLT_UInt32 first_pkt;
	
	media_demux_t *demux;
	video_info_t *vi;
	audio_info_t *ai;
	es_packet_t *v_esp;
	es_packet_t *a_esp;
	
	mp4_frame_t v_frame;
	mp4_frame_t a_frame;
}mp4_parse_t;

mp4_parse_t* g_mp4_parse = NULL;

#define ALLOC_SIZE	1024 * 1024
static BLT_UInt8 init_buf[ALLOC_SIZE];


/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(M4aParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(M4aParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(M4aParser, ATX_Referenceable)

/*----------------------------------------------------------------------
|   BREAD32
+---------------------------------------------------------------------*/
static BLT_UInt32
BREAD32(void *psrc)
{
	
	BLT_UInt32 	i;
	BLT_UInt32 	tmp;
	BLT_UInt8*	pdata;
	BLT_UInt32 	ret;

	pdata = (BLT_UInt8 *)psrc;
	ret = 0;
	for ( i = 0; i < 4; i++ ) 
	{
		tmp = *(pdata + i);
		ret += tmp << (8 * (4 - i - 1)) ; 
	}

	return ret;
}

/*----------------------------------------------------------------------
|   LREAD32
+---------------------------------------------------------------------*/

static BLT_UInt32
LREAD32(void *psrc)
{
	BLT_UInt8*	pdata;
	BLT_UInt32 	ret;

	pdata = (BLT_UInt8 *)psrc;
	ret = (*(pdata + 3) << 24) + 
		  (*(pdata + 2) << 16) + 
		  (*(pdata + 1) << 8 ) + 
		  (*(pdata    )      );

	return ret;
}

BLT_UInt32 VFS_UpperCase(BLT_UInt32 DCase)
{
	BLT_UInt32 uiCase;
	BLT_UInt8  *pucCase;
	BLT_UInt32 i;

	uiCase = DCase;
	pucCase = (BLT_UInt8*)&uiCase;
	for (i = 0; i < 4; i++)
	{
		if ((*pucCase > 0x60) && (*pucCase < 0x7B)) 
		{
			*pucCase = (BLT_UInt8)(*pucCase-0x20);
		}
		pucCase++;
	}

	return uiCase;
}


/*----------------------------------------------------------------------
|   vid3GPVoVolParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPVoVolParse(BLT_UInt8 *pbuf, BLT_UInt32 uiTrakSize)
{
	BLT_UInt8 tmp1, tmp2, tmp3;
	BLT_UInt8 urlLength;
	BLT_UInt32 videoDSISize;
	BLT_UInt32 i;
	BLT_Result result;
	BLT_UInt8  ucaHeader[5];

	pbuf += 86;
	if(uiTrakSize <= 90)
	{
		return BLT_FAILURE;
	}
	uiTrakSize -= 86;
	do 
	{
		ATX_SetMemory(ucaHeader, 0, sizeof(ucaHeader));
		ATX_CopyMemory(ucaHeader, pbuf, 4);
		if (ucaHeader[0] == 0x65 && ucaHeader[1] == 0x73 && ucaHeader[2] == 0x64 && ucaHeader[3] == 0x73)//'esds'
		{
			pbuf += 8;
			break;
		} 
		pbuf++;
		uiTrakSize--;
	} while (uiTrakSize > 1);
	if (uiTrakSize <= 1)
	{
		return BLT_FAILURE;
	}
	/* Detect the size field of ES_Descriptor */
	tmp1 = *pbuf;
	if (tmp1 == 0x03)//MP4ESDescrTag
	{
		pbuf++;
	}
	else
	{
		return BLT_FAILURE;
	}
	do 
	{
		tmp1 = *pbuf;
		tmp1 = (BLT_UInt8)(tmp1 & 0x80);
		pbuf++;
	}
	while( tmp1 == 0x80 );
	pbuf += 2;  /* after this line is done, the address is the location of
				   streamDependenceFlag, URL_Flag, OCRstreamFlag and streamPriority */
	tmp1 = *pbuf;  /* tmp1 is the value of streamDependenceFlag, URL_Flag, OCRstreamFlag and streamPriority */
	pbuf++;
	if ( (tmp1 & 0x80) == 0x80 ) 
	{
		pbuf += 2;
	}
	if ( (tmp1 & 0x40) == 0x40 )
	{
		urlLength = *pbuf;
		pbuf++;
		pbuf += urlLength;
	}
	if ( (tmp1 & 0x20) == 0x20 ) 
	{
		pbuf += 2;
	}
	/* Detect the size field of DecoderConfigDescriptor */
	pbuf++;
	do
	{
		tmp1 = *pbuf;
		tmp1 = (BLT_UInt8)(tmp1 & 0x80);
		pbuf++;
	}
	while( tmp1 == 0x80 );
	/* end of detection */

	pbuf += 14;   /* after this line is done, the address is the location of
					 the size field of DecoderSpecificInfo */
	/* Get the size of DecoderSpecificInfo */
	i = 0;
	videoDSISize = 0;
	do {
		tmp1 = *pbuf;
		tmp2 = (BLT_UInt8)(tmp1 & 0x80);
		if (tmp2 == 0x80) 
		{
			videoDSISize = videoDSISize << (i * 7);
		}
		else 
		{
			videoDSISize = videoDSISize << 8;
		}
		tmp3 = (BLT_UInt8)(tmp1 & 0x7F);
		videoDSISize = videoDSISize | tmp3;
		pbuf++;
		i++;
	}
	while( tmp2 == 0x80 );
	/* end of getting */

	if (videoDSISize > 512)
	{
		result = BLT_FAILURE;
	} 
	else
	{
		if (videoDSISize > 12)
		{
			gbvidHeaderSize = videoDSISize;
			ATX_CopyMemory(pvidHeaderData, pbuf, videoDSISize);
		}
		else
		{
			gbvidHeaderSize = 0;
		}
		
		result = BLT_SUCCESS;
	}	
	//ret = vidMP4BitstreamCheck(pbuf, videoDSISize, pVidInfo);

	return result;
}

/*----------------------------------------------------------------------
|   ParserMPEG4Profile
+---------------------------------------------------------------------*/

BLT_UInt8	ParserMPEG4Profile(BLT_UInt8 *pVData, BLT_UInt32 ui32VSize)
{
	BLT_UInt8 nProfile;
	BLT_UInt32 nCode = 0;
	BLT_UInt32 ii;
	BLT_UInt8 nBytes;

	nProfile = 0xff;
	for(ii = 0; ii < ui32VSize; ii++)
	{
		nCode = (nCode << 8) | (*pVData++);
		if(0x000001b0 == nCode)//vs start code
		{
			nBytes = *pVData;
			if(0x01 == nBytes ||
			   0x02 == nBytes ||
			   0x03 == nBytes ||
			   0x08 == nBytes)
			{
				nProfile = 1;
				break;
			}
			else if(0xf0 == nBytes ||
				    0xf1 == nBytes ||
				    0xf2 == nBytes ||
				    0xf3 == nBytes ||
				    0xf4 == nBytes ||
				    0xf5 == nBytes ||
				    0xf7 == nBytes)
			{
				nProfile = 2;
				break;
			}
			else
			{
				nProfile = 0;
			}
			
		}
		else if(0x00000120 == (nCode & 0xffffff0))//vol start code
		{
			nBytes = *pVData++;
			nBytes = (BLT_UInt8)((nBytes << 1) | (*pVData >> 7));
			if(0x01 == nBytes)
			{
				nProfile = 1;
				break;
			}
			else if(0x11 == nBytes)
			{
				nProfile = 2;
				break;
			}
			else
			{
				nProfile = 0;
			}
		}
	}

	return nProfile;
}

/*----------------------------------------------------------------------
|   ParserH264Profile
+---------------------------------------------------------------------*/
BLT_UInt8 ParserH264Profile(BLT_UInt8 *pVData, BLT_UInt32 ui32VSize)
{
	BLT_UInt32 i;
	BLT_UInt8 ucH264;

	ucH264 = 0;
	for (i = 0; i < (ui32VSize - 3); i++)
	{
		if(pVData[i] == 0x00 && pVData[i+1] == 0x00 && pVData[i+2] == 0x00 && pVData[i+3] == 0x01)
		{
			switch (pVData[i+5])	//0x42, 0x4D, 88, 100, 110, 122, 144
			{
			case 66:
				return 1;//Baseline profile
			case 77:
				return 2;//Main profile
			case 88:
			case 100:
			case 110:
			case 122:
			case 144:
				return 3;//High profile
			default:
				//return 0;//H264 ES, but can not analyse profile				
				break;
			}
			ucH264 = 1;
		}
	}
	
	if (ucH264 == 1)
	{
		return 0;
	} 
	else
	{
		return 0xFF;
	}
}



static BLT_UInt32
vid3GPMP4AaacPlusParse(
	BLT_UInt8 *pbuf,
	BLT_UInt8 *pbufStart,
	BLT_UInt8 audioObjType
)
{
	BLT_UInt8 extAudioObjType;
	BLT_UInt8 *ptr;
	BLT_Int32 size;
	BLT_UInt16 syncExtType;
	DEBUG0("######## into vid3GPMP4AaacPlusParse ########");
	isaacPlusPresent = 0;
	if (audioObjType == 5) 
	{
		BLT_UInt32 extSFIdx;
		//UINT32 samplingRate;
		BLT_UInt8 found;
		BLT_UInt8 i;
		
		//DEBUG0("pbuf: %x, pbuf+1: %x\n", *pbuf, *(pbuf + 1));
		extSFIdx = ((*pbuf & 0x07) << 1) | ((*(pbuf + 1) & 0x80) >> 7);
		//DEBUG0("extensionSamplingFrequencyIndex: %x\n", extSFIdx);
		pbuf++;
		if (extSFIdx == 0x0F) 
		{
			extSFIdx = BREAD32(pbuf);
			pbuf += 4;
			extSFIdx = (extSFIdx & 0x7fffff80) >> 7;
			//DEBUG0("extSFIdx: 0x%x\n", extSFIdx);
			found = 0;
			for ( i = 0; i < SAMPLEING_TABLE_NUM; i++ )
			{
				if ( extSFIdx == samplingTable[i] ) 
				{
					found = 1;
					break;
				}
			}
			if ( found == 1 ) 
			{
				//samplingRate = extSFIdx;
			}
			else
			{
				//DEBUG0("Unsupported sampling rate\n");
				return BLT_FAILURE;
			}
		} 
		else if (extSFIdx >= 3 && extSFIdx <= 11)
		{
			//samplingRate = samplingTable[extSFIdx - 3];
		}
		else
		{
			//DEBUG0("Unsupported sampling rate\n");
			return BLT_FAILURE;
		}

		extAudioObjType = audioObjType;
		isaacPlusPresent = 1;
		return BLT_SUCCESS;
	}
	else 
	{
		extAudioObjType = 0;
	}

	/* pbufStart points to size field of pDecoderSpecificInfoSize */
	/* point ptr to size field of esds */
	ptr = pbufStart;
	size = *pbufStart;
	//DEBUG0("DecoderSpecificInfoSize: %x\n", size);
	ptr++;
	size = size - (pbuf - ptr);
	//DEBUG0("rest size in bytes: %x\n", size);
	size = size * 8 - 5;
	//DEBUG0("rest size in bits: %x\n", size);
	if ((audioObjType == 2) && (size >= 16)) 
	{
		pbuf++;
		syncExtType = (BLT_UInt16)((*pbuf << 3) | ((*(pbuf + 1) & 0xE0) >> 5));
		//DEBUG0("syncExtType: %x\n", syncExtType);
		pbuf++;
		if (syncExtType == 0x2B7) 
		{
			extAudioObjType = (BLT_UInt16)(*pbuf & 0x07);
			//DEBUG0("extAudioObjType: %x\n", extAudioObjType);
			pbuf++;
			if (extAudioObjType == 5) 
			{
				isaacPlusPresent = (*pbuf & 0x80) >> 7;
				//DEBUG0("aacPlusPresent: %x\n", isaacPlusPresent);
			}
		}
	}

	return BLT_SUCCESS;
}



/*----------------------------------------------------------------------
|   vid3GPMP4AParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPMP4AParse(
	BLT_UInt8 *pbuf1,
	ContainBitsInfo_t* pVidInfo
)
{
	BLT_UInt8 *pbuf;
	BLT_UInt8 flag;
	BLT_UInt8 audioObjType;
	BLT_UInt32 samplingFrequencyIndex;
	BLT_UInt16 channel;
	BLT_UInt16 samplingRate;
	BLT_UInt32 found;
	BLT_UInt32 uiVersion;
	BLT_UInt32 uiSize;
	BLT_UInt32 tag, uiMaxBit, uiAvgBit;
	BLT_UInt8 i;
#if AUDIO_AAC_PLUS_OPT
	BLT_UInt8 *pDecoderSpecificInfoSizeAddr;
#endif

	DEBUG0("####### into vid3GPMP4AParse #######\n");

	isaacPlusPresent = 0;
	i = 0;

	pbuf = pbuf1 + 33;
	uiVersion = *pbuf;
	DEBUG0("%%%%%%%%%uiVersion = %0x\n", uiVersion);

	/* Jump to ES_Descriptor's size field */
	if (uiVersion)
	{//for QT
		switch(uiVersion)
		{
		case 1:	
			pbuf = pbuf1 + 68;
			break;
		case 2:
			pbuf = pbuf1 + 88;		
			break;
		default:
			return BLT_SUCCESS;//Unsupported audio but for upported video
			break;
		}
		found = BREAD32(pbuf);
		DEBUG0("found = %0x\n", found);
		pbuf += 4;
		tag = LREAD32(pbuf);
		DEBUG0("tag = %0x\n", tag);
		tag = VFS_UpperCase(tag);
		pbuf += 4;
		if (tag == MP4_ESDS)
		{
			DEBUG0("tag == MP4_ESDS\n");
			pbuf += 4;
		} 
		else
		{	DEBUG0("else\n");
			for (i = 8; i < found;)
			{
				uiSize = BREAD32(pbuf);
				pbuf += 4;
				tag = LREAD32(pbuf);
				tag = VFS_UpperCase(tag);
				pbuf += 4;
				if (tag == MP4_ESDS)
				{
					pbuf += 4;
					break;
				}
				else
				{					
					pbuf += (uiSize - 8);
				}
				i = (BLT_UInt8)(i + uiSize);
			}
		}		
		if (i == found)
		{
			DEBUG0("i == found\n");
			return BLT_SUCCESS;//Unsupported audio but for upported video
		}
		//DEBUG0("ES_Descriptor: 0x%x\n", *pbuf);
		pbuf++;
	}
	else
	{
		pbuf = pbuf1 + 65;
		DEBUG0("pbuf = pbuf1 + 65\n");
		//DEBUG0("ES_Descriptor: 0x%x\n", *(pbuf1 + 64));
	}	
	
	/* skip size field */
	while ((*pbuf & 0x80) == 0x80) 
	{
		pbuf++;
	}
	pbuf++;
	/* skip ES_ID */
	pbuf += 2;

	flag = *pbuf;
	pbuf++;
	DEBUG0("flag = %0x\n", flag);
	/* streamDependenceFlag */
	if ((flag & 0x80) == 0x80) 
	{
		pbuf += 2;
	}
	/* URL_Flag */
	if ((flag & 0x40) == 0x40) 
	{
		BLT_UInt8 urlLength;
		urlLength = *pbuf;
		pbuf += urlLength;
	}
	/* OCRstreamFlag */
	if ((flag & 0x20) == 0x20)
	{
		pbuf += 2;
	}
	//DEBUG0("DecoderConfigDescrTag: 0x%x\n", *pbuf);
	/* skip DecoderConfigDescrTag */
	pbuf++;
	/* skip size field */
	while ((*pbuf & 0x80) == 0x80)
	{
		pbuf++;
	}
	pbuf++;

	//pbuf += 13;
	pbuf += 5;
	uiMaxBit = BREAD32(pbuf);
	pbuf += 4;
	uiAvgBit = BREAD32(pbuf);
	pbuf += 4;
	pVidInfo->isVBR = (BLT_UInt8)(uiMaxBit == uiAvgBit ? 0 : 1);
	DEBUG0("pVidInfo->isVBR = %0x\n", pVidInfo->isVBR);
	//DEBUG0("DecoderSpecificInfoTag: 0x%x\n", *pbuf);
	pbuf++;
	/* skip size field */
	while ((*pbuf & 0x80) == 0x80) 
	{
		pbuf++;
	}
#if AUDIO_AAC_PLUS_OPT
	pDecoderSpecificInfoSizeAddr = pbuf;
#endif
	pVidInfo->audExtraLen = *pbuf;
	DEBUG0("pVidInfo->audExtraLen = %0x\n", pVidInfo->audExtraLen);
	

	pVidInfo->pAudExtraData = (BLT_UInt8 *)ATX_AllocateZeroMemory(pVidInfo->audExtraLen);
	pbuf++;
	if (pVidInfo->pAudExtraData)
	{
		pAudExtraData = pVidInfo->pAudExtraData;
		ATX_CopyMemory(pVidInfo->pAudExtraData, pbuf, pVidInfo->audExtraLen);
	} 
	else
	{
		return BLT_FAILURE;
	}


	for(i=0;i<pVidInfo->audExtraLen;i++){
		DEBUG0("%02x ",  (pVidInfo->pAudExtraData)[i]);
	}
	DEBUG0("\n");
	
	//DEBUG0("*pbuf: 0x%x, *pbuf+1: 0x%x\n", *pbuf, *(pbuf + 1));
	audioObjType = (BLT_UInt8)(*pbuf >> 3);
	//DEBUG0("audioObjType: 0x%x\n", audioObjType);
#if AUDIO_AAC_PLUS_OPT
	if ((audioObjType != 1) && (audioObjType != 2) && (audioObjType != 5)) 
	{		
		//DEBUG0("Unsupported audio object type: %d\n", audioObjType);
		//ret = VID_PLAY_ERR017;
		//return ret;
		return BLT_SUCCESS;//Unsupported audio but for upported video
	}
#else
	if ( (audioObjType != 1) && audioObjType != 2 ) 
	{
		//DEBUG0("Unsupported audio object type: %d\n", audioObjType);
		return BLT_SUCCESS;//Unsupported audio but for upported video
	}
#endif
	samplingFrequencyIndex = ( (*pbuf & 0x07) << 1 ) | ( (*(pbuf + 1) & 0x80) >> 7 );
	pbuf++;
	//DEBUG0("samplingFrequencyIndex: 0x%x\n", samplingFrequencyIndex);
	if ( samplingFrequencyIndex == 0x0f) 
	{
		samplingFrequencyIndex = BREAD32(pbuf);
		pbuf += 4;
		samplingRate = (BLT_UInt16)((samplingFrequencyIndex & 0x7fffff80) >> 7);
		//DEBUG0("samplingFrequencyIndex: 0x%x\n", samplingFrequencyIndex);		
	}
	else if ((samplingFrequencyIndex >= 3) && (samplingFrequencyIndex <= 11)) 
	{
		samplingRate = (BLT_UInt16)samplingTable[samplingFrequencyIndex - 3];
	}
	else 
	{
		//DEBUG0("Unsupported sampling rate\n");
		//ret = VID_PLAY_ERR018;
		//DBG_CHECK(VID_PLAY_ERR018);
		return BLT_SUCCESS;//Unsupported audio but for upported video
	}

	channel = (BLT_UInt16)((*pbuf & 0x78) >> 3);
	//DEBUG0("channel: %d\n", channel);
	//support channel > 2
	/*if ( channel > 2 )
	{
		//DEBUG0("Unsupported channel: %d\n", channel);
		ret = VID_PLAY_ERR019;
		//DBG_CHECK(VID_PLAY_ERR019);
		return ret;
	}*/
	//DEBUG0("Channel: %d, samplingRate: %d\n", channel, samplingRate);

#if AUDIO_AAC_PLUS_OPT
	if (samplingRate <= 24000) 
	{DEBUG0("samplingRate <= 24000\n");
		vid3GPMP4AaacPlusParse(pbuf, pDecoderSpecificInfoSizeAddr, audioObjType);
		if (isaacPlusPresent == 1) 
		{
			samplingRate *= 2;
		}
	} 
	else 
	{
		isaacPlusPresent = 0;
	}
#endif

	pVidInfo->audChannel = (BLT_UInt8)channel;
	pVidInfo->audSR = samplingRate;
	DEBUG0("pVidInfo->audChannel = %d\n", pVidInfo->audChannel);
	DEBUG0("pVidInfo->audSR= %d\n", pVidInfo->audSR);
	if (isaacPlusPresent == 1)
	{
		DEBUG0(" 1 \n");
		pVidInfo->audType = AUDIO_TYPE_AAC_PLUS;
	} 
	else
	{
		DEBUG0(" else1 \n");
		pVidInfo->audType = AUDIO_TYPE_AAC;
	}
	
	pVidInfo->audBits = 16;
	pVidInfo->audBitrate = uiAvgBit;
	DEBUG0("pVidInfo->audBitrate = %d\n", pVidInfo->audBitrate);
	return BLT_SUCCESS;
}

static BLT_UInt32
vid3GPMP3Parse(
				BLT_UInt8 *pbuf1,
				ContainBitsInfo_t* pVidInfo
				)
{
	BLT_UInt8 *pbuf;
	BLT_UInt32 U32Flag;
	

	pbuf = pbuf1 + 40;
	U32Flag = BREAD32(pbuf);
	pVidInfo->audChannel = (BLT_UInt8)(U32Flag >> 16);
	pVidInfo->audBits    = (BLT_UInt8)U32Flag;
	pbuf += 8;
	U32Flag = BREAD32(pbuf);
	pVidInfo->audSR      = (BLT_UInt8)(U32Flag >> 16);
 
	return BLT_SUCCESS;
}


static BLT_UInt32 vidH264VoVolParse (BLT_UInt8 *pbuf, BLT_UInt32 uiTrakSize)
{//N570_clean.doc 5.2.4.1.1
	BLT_UInt8 tmp1;
	BLT_UInt8  ucNum;
	BLT_UInt8  ucaHeader[5];
	BLT_UInt32 videoDSISize;
	BLT_UInt32 i;

	gbvidHeaderSize = 0;
	videoDSISize = 0;

	pbuf += 86;
	if(uiTrakSize <= 90)
	{
		return BLT_FAILURE;
	}
	uiTrakSize -= 86;
	do 
	{
		ATX_SetMemory(ucaHeader, 0, sizeof(ucaHeader));
		ATX_CopyMemory(ucaHeader, pbuf, 4);
		if (ucaHeader[0] == 0x61 && ucaHeader[1] == 0x76 &&ucaHeader[2] == 0x63 &&ucaHeader[3] == 0x43)//'avcC'
		{
			pbuf += 8;
			break;
		} 
		pbuf++;
		uiTrakSize--;
	} while (uiTrakSize > 1);
	if (uiTrakSize <= 1)
	{
		return BLT_FAILURE;
	}
	
	tmp1 = *pbuf;  /* lengthSizeMinusOne */
	pbuf++;
	gblengthSizeMinusOne = (BLT_UInt8)(tmp1 & 0x03);

	if (gblengthSizeMinusOne == 0 ||
		gblengthSizeMinusOne == 1 ||
		gblengthSizeMinusOne == 2)
	{
		if (pvidH264Data == NULL)
		{
			pvidH264Data = (BLT_UInt8 *)ATX_AllocateZeroMemory(VID_DATA_BUF_SIZE);
			if (pvidH264Data == NULL)
			{
				return BLT_FAILURE;
			}
		} 
		else
		{
			return BLT_FAILURE;
		}		
	}

	tmp1 = *pbuf;  /* numOfSequenceParameterSets */
	pbuf++;
	ucNum = (BLT_UInt8)(tmp1 & 0x1F);

	ucaHeader[0] = 0x00;
	ucaHeader[1] = 0x00;
	ucaHeader[2] = 0x00;
	ucaHeader[3] = 0x01;
	ucaHeader[4] = 0x00;

	for (i = 0; i < ucNum; i++)
	{
		tmp1 = *pbuf;  
		pbuf++;
		videoDSISize = tmp1 << 8;
		tmp1 = *pbuf;  
		pbuf++;
		videoDSISize |= tmp1;

		ATX_CopyMemory(pvidHeaderData + gbvidHeaderSize, ucaHeader, 4);
		gbvidHeaderSize += 4;
		ATX_CopyMemory(pvidHeaderData + gbvidHeaderSize, pbuf, videoDSISize);
		gbvidHeaderSize += videoDSISize;

		pbuf += videoDSISize;
	}

	tmp1 = *pbuf;  /* numOfPictureParameterSets */
	pbuf++;
	ucNum = tmp1;

	for (i = 0; i < ucNum; i++)
	{
		tmp1 = *pbuf;  
		pbuf++;
		videoDSISize = tmp1 << 8;
		tmp1 = *pbuf;  
		pbuf++;
		videoDSISize |= tmp1; /*pictureParameterSetLength*/

		ATX_CopyMemory(pvidHeaderData + gbvidHeaderSize, ucaHeader, 4);
		gbvidHeaderSize += 4;
		ATX_CopyMemory(pvidHeaderData + gbvidHeaderSize, pbuf, videoDSISize);
		gbvidHeaderSize += videoDSISize;

		pbuf += videoDSISize;
	}

	if (gbvidHeaderSize > 512)
	{
		return BLT_FAILURE;
	} 
	else
	{
		return BLT_SUCCESS;
	}	
}

static void mp4Info_to_wmainfo(mp4_wav_format_t *raw_info, ContainBitsInfo_t *pVidInfo)
{	
	raw_info->wFormatTag = 0x0001;
	raw_info->nChannels = pVidInfo->audChannel;
	raw_info->nSamplesPerSec = pVidInfo->audSR;
	raw_info->nAvgBytesPerSec = 8000;
	if ( pVidInfo->audBits == 8 ) {
	    raw_info->nBlockAlign = 1;	
	}
	else {
	    raw_info->nBlockAlign = 2;	
	}	
	raw_info->wBitsPerSample = pVidInfo->audBits;	
	raw_info->cbSize = 0;
	raw_info->wSamplesPerBlock = 0;
}


/*----------------------------------------------------------------------
|   vid3GPSTSDParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPSTSDParse(ContainBitsInfo_t* pVidInfo,	BLT_UInt32 trakID,BLT_UInt8 *pbuf1)
{
	BLT_UInt8 *pbuf;
	BLT_UInt8 *pbufdata;
	vidVideoType_t typeVideo;
	BLT_Result	result = BLT_SUCCESS;
	BLT_UInt32 count;
	BLT_UInt32 tag;
	BLT_UInt32 uiProfile = 0;
	BLT_UInt32 uiTrakSize;

	DEBUG0("##########  into vid3GPSTSDParse ##############\n");
	/* check mpv4 os s263*/
	if ( trakID == VIDEO_TRAK ) 
	{	DEBUG0("##########  trakID == VIDEO_TRAK ##############\n");	
		uiTrakSize = BREAD32(pbuf1);
		pbuf = pbuf1 + 12;
		count = BREAD32(pbuf);
		DEBUG0("count = %d\n", count);
		if (count == 0) 
		{
			//DBG_CHECK(VID_PLAY_ERR002);
			return VID_PLAY_ERR002;
		}
		pVidInfo->vidStreamNum++;
		pbuf = pbuf1 + 20;
		tag = LREAD32(pbuf);
		tag = VFS_UpperCase(tag);
		
		pbufdata = pbuf;
		pbufdata += 28;	
		pVidInfo->width = (*pbufdata) << 8;
		pbufdata++;
		pVidInfo->width |= *pbufdata;
		pbufdata++;
		pVidInfo->height = (*pbufdata) << 8;
		pbufdata++;
		pVidInfo->height |= *pbufdata;
		pbufdata++;
		DEBUG0("tag = %d\n", tag);
		//DEBUG0("width:%d  height:%d \n",pVidInfo->width,pVidInfo->height);
		switch(tag) 
		{
			case VID_MP4V: /* mp4v */
				en3GPVType = VIDEO_3GP_TYPE_MPEG4;
				/* MP4 info data are hidden in VOVOL header */
				result = vid3GPVoVolParse(pbuf, uiTrakSize - (pbuf1 - pbuf));
				if (result != BLT_SUCCESS)
				{
					return result;
				}
				if (pvidH264Data == NULL)
				{
					pvidH264Data = (BLT_UInt8*)ATX_AllocateZeroMemory(VID_DATA_BUF_SIZE);
					if (pvidH264Data == NULL)
					{
						return BLT_FAILURE;
					}
				} 
				else
				{
					return BLT_FAILURE;
				}
				uiProfile = ParserMPEG4Profile(pvidHeaderData, gbvidHeaderSize);
				switch (uiProfile)
				{
				case 0:
					typeVideo = VIDEO_TYPE_MPEG4_ASP;	//if profile reserved				
					break;
				case 1:
					typeVideo = VIDEO_TYPE_MPEG4_SP;
					break; 
				case 2:
					typeVideo = VIDEO_TYPE_MPEG4_ASP;
					break;
				default:
					typeVideo = VIDEO_TYPE_NONE;					
					break;
				}				
				break;
			case VID_S263: /* s263 */
				/* H263 info data are hidden in bitstream */
				typeVideo = VIDEO_TYPE_H263;
				break;
			case VID_AVC1: /* avc1 */
				en3GPVType = VIDEO_3GP_TYPE_H264;
				uiTrakSize = uiTrakSize - (pbuf - pbuf1);
				result = vidH264VoVolParse(pbuf, uiTrakSize);
				if (result == BLT_SUCCESS)
				{
				uiProfile = ParserH264Profile(pvidHeaderData, gbvidHeaderSize);
				} 
				else
				{
					uiProfile = 0;
				}
				
				switch (uiProfile)
				{
				case 0:
					typeVideo = VIDEO_TYPE_NONE;				
					break;
				case 1:
					typeVideo = VIDEO_TYPE_H264_BP;
					break;
				case 2:
					typeVideo = VIDEO_TYPE_H264_MP;
					break;
				case 3:
					typeVideo = VIDEO_TYPE_H264_HP;
					break;
				default:
					typeVideo = VIDEO_TYPE_NONE;							
					break;
				}
				break;
			case VID_MJPG:
			case VID_JPEG:
				typeVideo = VIDEO_TYPE_MJPEG;
				break;
			default:
				typeVideo = VIDEO_TYPE_NONE;					
		}
		pVidInfo->vidType = typeVideo;
	}
	else if (trakID == AUDIO_TRAK) 
	{
		DEBUG0("####### trakID == AUDIO_TRAK #######\n");

		/* check mp4a os samr */		
		pbuf = pbuf1 + 12;
		count = BREAD32(pbuf);
		DEBUG0("count = %d\n", count);
		if (count == 0) 
		{
			//DBG_CHECK(VID_PLAY_ERR002);
			return VID_PLAY_ERR002;
		}
		pVidInfo->audStreamNum++;
		pbuf = pbuf1 + 20;
		tag = LREAD32(pbuf);
		tag = VFS_UpperCase(tag);
		DEBUG0("tag = %d\n", tag);
		switch(tag)
		{
		#if AUDIO_AMR_OPT
			case AUD_SAMR: /* samr */
				pVidInfo->audType  = AUDIO_TYPE_AMR_NB;
 				pbuf += 20;				
				pVidInfo->audChannel = (*pbuf) << 8;
				pbuf++;
				pVidInfo->audChannel |= (*pbuf);
				pbuf++;
				pVidInfo->audBits = (*pbuf) << 8;
				pbuf++;
				pVidInfo->audBits |= (*pbuf);
				pbuf += 5;
				pVidInfo->audSR = (BREAD32(pbuf) >> 16);
				pVidInfo->isVBR = 0;
				break;
		#endif
		#if AUDIO_AAC_OPT
			case AUD_MP4A: /* mp4a */
				// DEBUG0("MP4A audio\n");	
				
				DEBUG0("####### case AUD_MP4A #######\n");
				result = vid3GPMP4AParse(pbuf1, pVidInfo);
				if (result != BLT_SUCCESS) 
				{
					return result;
				}
				break;
		#endif
			case AUD_SAWB:
				pVidInfo->audType  = AUDIO_TYPE_AMR_WB;
 				pVidInfo->audSR = 16000;
				pVidInfo->audChannel = 2;
				pVidInfo->audBits = 16;
				break;

            case AUD_MPL3:
			case AUD_MSU_:
				result = vid3GPMP3Parse(pbuf1, pVidInfo);
				if (result != BLT_SUCCESS) 
				{
					return result;
				}
				pVidInfo->audType  = AUDIO_TYPE_MP3;
				break;


			case AUD_PCM_:
			case AUD_TWOS:
				pVidInfo->audType  = AUDIO_TYPE_PCM;
				pbuf += 20;				
				pVidInfo->audChannel = (*pbuf) << 8;
				pbuf++;
				pVidInfo->audChannel |= (*pbuf);
				pbuf++;
				pVidInfo->audBits = (*pbuf) << 8;
				pbuf++;
				pVidInfo->audBits |= (*pbuf);
				pbuf += 5;
				pVidInfo->audSR = (BREAD32(pbuf) >> 16);
#ifdef USE_LIBAVCODEC
				//DEBUG0(">>> vid3GPSTSDParse: audBits = %d\n", pVidInfo->audBits);
				if (pVidInfo->audBits == 8){
					pVidInfo->audType = CODEC_ID_PCM_U8;
				}
				else if (pVidInfo->audBits == 24){
					pVidInfo->audType = CODEC_ID_PCM_S24LE;
				}
				else if (pVidInfo->audBits == 32){
					pVidInfo->audType = CODEC_ID_PCM_S32LE;
				}
				else{
					pVidInfo->audType  = AUDIO_TYPE_PCM;
				}
#endif
				if ( pVidInfo->pAudExtraData == NULL ) {
				    pVidInfo->pAudExtraData = ATX_AllocateZeroMemory(sizeof(mp4_wav_format_t));
					if ( pVidInfo->pAudExtraData == NULL ) {
					    return BLT_FAILURE;
					}
					else {
					    pAudExtraData = pVidInfo->pAudExtraData;
						mp4Info_to_wmainfo((mp4_wav_format_t *)pAudExtraData, pVidInfo);
					}
				}
				 
				break;

		default:
			pVidInfo->audType  = AUDIO_TYPE_NONE;
		}
		gAudType = pVidInfo->audType;
		gAudSR = pVidInfo->audSR;
		DEBUG0("####### pVidInfo->audType = %d #######\n", pVidInfo->audType);
		DEBUG0("####### pVidInfo->audSR = %d #######\n", pVidInfo->audSR);
	}

	return result;
}

/*----------------------------------------------------------------------
|   vid3GPSTSCParse
+---------------------------------------------------------------------*/


static BLT_UInt32
vid3GPSTSCParse(
	BLT_UInt32 startPos,
	BLT_UInt32 trakID,
	BLT_UInt8 *pbuf1
)
{
	BLT_UInt8 *pbuf;
	_3gpSTSCInfo_t *stsc;

	pbuf = pbuf1 + 12;
	if ( trakID == VIDEO_TRAK ) 
	{
		stsc = &_3gpVidSTSC;
	}
	else if (trakID == AUDIO_TRAK) 
	{
		stsc = &_3gpAudSTSC;
	}
	else 
	{
		return BLT_SUCCESS;
	}
	stsc->entry_count = BREAD32(pbuf);
	if (stsc->entry_count == 0) 
	{
		//DBG_CHECK(VID_PLAY_ERR002);
		return VID_PLAY_ERR002;
	}
	stsc->entry_idx = 0;
	stsc->entry_fill = 0;
	stsc->filePos = startPos + 16;
	stsc->filePos_start = stsc->filePos;
	stsc->sample_count = 0;
	stsc->sample_idx = 0;
	stsc->isLastChunk = 0;
	stsc->next_chunk_id = 0;
	stsc->curr_chunk_id = 0;
	//DEBUG0("filePos: 0x%x\n", stsc->filePos);
	//DEBUG0("entry_count: 0x%x\n", stsc->entry_count);

	return BLT_SUCCESS;
	}

/*----------------------------------------------------------------------
|   vid3GPSTSZParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPSTSZParse(
	BLT_UInt32 startPos,
	BLT_UInt32 trakID,
	BLT_UInt8 *pbuf1
)
{
	BLT_UInt8 *pbuf;
	_3gpSTSZInfo_t *stsz;

	pbuf = pbuf1 + 12;
	if ( trakID == VIDEO_TRAK ) 
	{
		stsz = &_3gpVidSTSZ;
	}
	else if (trakID == AUDIO_TRAK) 
	{
		stsz = &_3gpAudSTSZ;
	}
	else
	{
		return BLT_SUCCESS;
	}
	stsz->sample_size = BREAD32(pbuf);
	stsz->sample_count = BREAD32(pbuf + 4);
	if (stsz->sample_count == 0) 
	{
		//DBG_CHECK(VID_PLAY_ERR002);
		return VID_PLAY_ERR002;
	}
	stsz->sample_idx = 0;
	stsz->sample_fill = 0;
	stsz->filePos = startPos + 20; /* point to the first sample_size */
	stsz->filePos_start = stsz->filePos;
	if ( trakID == VIDEO_TRAK )
	{
		vidFirstSampleSize = BREAD32(pbuf + 8);
	}
	//DEBUG0("filePos: 0x%x\n", stsz->filePos);
	//DEBUG0("sample_count: 0x%x\n", stsz->sample_count);
	//DEBUG0("sample_size: 0x%x\n", stsz->sample_size);

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   vid3GPSTCOParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPSTCOParse(
	BLT_UInt32 startPos,
	BLT_UInt32 trakID,
	BLT_UInt8 *pbuf1
)
{
	BLT_UInt8 *pbuf;
	_3gpSTCOInfo_t *stco;

	pbuf = pbuf1 + 12;
	if ( trakID == VIDEO_TRAK ) 
	{
		stco = &_3gpVidSTCO;
	}
	else if (trakID == AUDIO_TRAK) 
	{
		stco = &_3gpAudSTCO;
	}
	else
	{
		return BLT_SUCCESS;
	}
	stco->entry_count = BREAD32(pbuf);
	if (stco->entry_count == 0) 
	{
		//DBG_CHECK(VID_PLAY_ERR002);
		return VID_PLAY_ERR002;
	}
	stco->entry_idx = 0;
	stco->entry_fill = 0;
	stco->filePos = startPos + 16;
	stco->filePos_start = stco->filePos;
	if ( trakID == VIDEO_TRAK ) 
	{
		vidFirstSampleOffset = BREAD32(pbuf + 4);
	}
	//DEBUG0("filePos: 0x%x\n", stco->filePos);
	//DEBUG0("entry_count: 0x%x\n", stco->entry_count);

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   vid3GPSTTSParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPSTTSParse(
	BLT_UInt32 startPos,
	BLT_UInt32 trakID,
	BLT_UInt8 *pbuf1
)
{
	BLT_UInt8 *pbuf;
	_3gpSTTSInfo_t *stts;

	pbuf = pbuf1 + 12;
	if ( trakID == VIDEO_TRAK ) 
	{
		stts = &_3gpVidSTTS;
	}
	else if (trakID == AUDIO_TRAK)
	{
		stts = &_3gpAudSTTS;
	}
	else 
	{
		return BLT_SUCCESS;
	}
	stts->entry_count = BREAD32(pbuf);
	if (stts->entry_count == 0)
	{
		//DBG_CHECK(VID_PLAY_ERR002);
		return VID_PLAY_ERR002;
	}
	stts->entry_fill = 0;
	stts->entry_idx = 0;
	stts->filePos = startPos + 16; /* point to the first sample_count */
	stts->filePos_start = stts->filePos;
	stts->sample_count = 0;
	stts->sample_idx = 0; 
	//DEBUG0("filePos: 0x%x\n", stts->filePos); 
	//DEBUG0("entry_count: 0x%x\n", stts->entry_count);

	return BLT_SUCCESS; 
}

/*----------------------------------------------------------------------
|   vid3GPSTSSParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPSTSSParse(
	BLT_UInt32 startPos,
	BLT_UInt32 trakID,
	BLT_UInt8 *pbuf1
)
{
	BLT_UInt8 *pbuf;
	_3gpSTSSInfo_t *stss;

	pbuf = pbuf1 + 12;
	if ( trakID == VIDEO_TRAK )
	{
		stss = &_3gpVidSTSS;
		gbVidSTSSExist = 1;
	}
	else if (trakID == AUDIO_TRAK) 
	{
		stss = &_3gpAudSTSS;
	}
	else 
	{
		return BLT_SUCCESS;
	}
	stss->entry_count = BREAD32(pbuf);
	stss->entry_idx = 0;
	stss->entry_fill = 0;
	stss->filePos = startPos + 16;
	stss->filePos_start = stss->filePos;
	//DEBUG0("[vid3GPSTSSParse] filePos: 0x%x\n", stss->filePos);
	//DEBUG0("[vid3GPSTSSParse] entry_count: 0x%x\n", stss->entry_count);
	return BLT_SUCCESS;
}


/*----------------------------------------------------------------------
|   vid3GPPosBounderGet
+---------------------------------------------------------------------*/

static void
vid3GPPosBounderGet(
	BLT_UInt32 offset,
	BLT_UInt32 originSize,
	BLT_UInt32 *pOffsetStart,
	BLT_UInt32 *pOffsetLeft,
	BLT_UInt32 *pSize
)
{
	BLT_UInt32 bounder;
	BLT_UInt32 offsetStart;
	BLT_UInt32 offsetLeft;
	BLT_UInt32 offsetEnd;
	BLT_UInt32 size;

	bounder = MP4_BOUNDER_SIZE;
	offsetStart = (offset / bounder) * bounder;
	offsetLeft = offset % bounder;
	offsetEnd = offset + originSize;
	/* wilsonlu@20041129, fixed offset and size are wrong */
	if ( offsetEnd % bounder != 0 ) 
	{
		offsetEnd = (offsetEnd / bounder + 1) * bounder;
	}
	size = offsetEnd - offsetStart;

	*pOffsetStart = offsetStart;
	*pOffsetLeft = offsetLeft;
	*pSize = size;
	DEBUG0("[vid3GPPosBounderGet]:\n");
	DEBUG0("0x%-6x, 0x%-6x, 0x%-6x\n", offset, offset + originSize, originSize);
	DEBUG0("0x%-6x, 0x%-6x, 0x%-6x, 0x%-6x\n", offsetStart, offsetEnd, offsetEnd - offsetStart, offsetLeft );
}


static BLT_UInt16
vid3GPSTSCIdxRefill(
	M4aParser *pParser,
	_3gpSTSCInfo_t *pstsc,
	BLT_UInt32 idx
)
{
	BLT_UInt32 size;
	BLT_UInt32 count;
	BLT_UInt8 *pbufStart;
	BLT_UInt32 filePos;
	BLT_UInt32 bufSize;
	BLT_UInt32 num;
	BLT_UInt32 offset, offsetLeft, sizeRead;
	BLT_Int32  ifilled;
	BLT_UInt32 uiBufSize;
	ATX_Size	bytes_read;

	DEBUG0("***** into  vid3GPSTSCIdxRefill ***\n");
	
	count = pstsc->entry_count - pstsc->entry_fill;
	DEBUG0("***** count = %u ***\n", count);
	if ( count == 0 ) 
	{DEBUG0("***** count == 0 ***\n");
		/* No more to refill */
		return VID_PLAY_ERR015;
	}
	num = 12;
	size = count * num;
	DEBUG0("***** size = %u ***\n", size);
	if (idx == IDX_VID_STSC)
	{
	DEBUG0("***** idx == IDX_VID_STSC ***\n");
		uiBufSize = IDX_VID_STSC_BUF_SIZE;
		bufSize = ( (BLT_UInt32) (uiBufSize / num) ) * num ;
	} 
	else if (idx == IDX_AUD_STSC)
	{
		DEBUG0("***** idx == IDX_AUD_STSC ***\n");
		uiBufSize = IDX_AUD_STSC_BUF_SIZE;
		bufSize = ( (BLT_UInt32) (uiBufSize / num) ) * num ;
	}
	else
	{
		return BLT_FAILURE;
	}
	DEBUG0("***** size = %u ***\n", size);
	DEBUG0("***** count = %u ***\n", count);
	if ( size > bufSize ) 
	{
		size = bufSize;
		count = bufSize / num;
	}
	pbufStart = p3GPIdxBufStart[idx];
	filePos = pstsc->filePos;
	DEBUG0("***** filePos = %u ***\n", filePos);
	vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
	DEBUG0("***** after  vid3GPPosBounderGet***\n");
	DEBUG0("***** offset = %u ***\n", offset );
	DEBUG0("***** offsetLeft = %u ***\n", offsetLeft);
	DEBUG0("***** sizeRead  = %u ***\n", sizeRead );

	ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
	//if (ifilled < 0)
	if ((BLT_UInt32)ifilled < 0 )
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	DEBUG0("***** after seek ***\n");
	DEBUG0("is pbufStart == NULL = %d\n",pbufStart == NULL);

	//char aaa[1024];
	ifilled = ATX_InputStream_Read(pParser->input.stream, pbufStart, sizeRead, &bytes_read);
	if (ifilled < 0)	
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	DEBUG0("***** after read ***\n");
	pbufStart += offsetLeft;
	
	pstsc->filePos = filePos + size;
	pstsc->entry_idx = pstsc->entry_fill;
	pstsc->entry_fill += count;
	pstsc->sample_idx = 0;
	p3GPIdxBufCurr[idx] = pbufStart;
	DEBUG0("***** pstsc->filePos  = %u ***\n", pstsc->filePos );
	DEBUG0("***** pstsc->entry_idx = %u ***\n", pstsc->entry_idx);
	DEBUG0("***** pstsc->entry_fill  = %u ***\n", pstsc->entry_fill );
	DEBUG0("***** pstsc->sample_idx  = %u ***\n", pstsc->sample_idx );
	return BLT_SUCCESS;
}

static BLT_UInt32 vid3GPMemSizeGet(BLT_UInt32 uiMemSel)
{
	BLT_UInt32 size;

    size = 0;
	/* index buffer size */
	size  = IDX_VID_STTS_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_VID_STSZ_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_VID_STCO_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_VID_STSS_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_VID_STSC_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_AUD_STSZ_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_AUD_STCO_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_AUD_STSC_BUF_SIZE + ADDR_ALIGN_SIZE;
	size += IDX_AUD_STTS_BUF_SIZE + ADDR_ALIGN_SIZE;

	/* video buffer size */
	size += VID_DATA_BUF_SIZE;

	/* audio buffer size */
	size += AUD_DATA_BUF_SIZE;

	/*for STSC index*/
	size += 1024;
	/* video codec header size */
	size += 512;

	return size;
}


static BLT_UInt16
vid3GPSTSCIdxPeek(M4aParser * pParser, _3gpSTSCInfo_t *pstsc)
{	
	BLT_UInt8 *pbuf;
	BLT_UInt32 filePos, fileLeft;
	BLT_Int32  ifilled;
	ATX_Size   bytes_read;

	//pbuf = (UINT8 *)((UINT32)buf | NONE_CACHE_LOGI_ADDR); /*for SPCA556 EVB*/
	if (pvidHeaderData)
	{
		pbuf = pvidHeaderData - 1024;
	}
	else
	{
		pbuf = p3GPPktBufStart + (vid3GPMemSizeGet(0) - 1536);
	}

	filePos = pstsc->filePos;
	fileLeft = filePos % 512;
	filePos = (filePos / 512) * 512;
	ifilled = ATX_InputStream_Seek(pParser->input.stream, filePos);
	//if (ifilled < 0)
	if ((BLT_Int32)ifilled < 0)
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	ifilled = ATX_InputStream_Read(pParser->input.stream, pbuf, 1024, &bytes_read);
	if (ifilled < 0)	
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	pstsc->next_chunk_id = BREAD32(pbuf + fileLeft);

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPSTSCGet(
	M4aParser *pParser,
	_3gpSTSCInfo_t *pstsc,
	BLT_UInt32 idx,
	BLT_UInt32 *pNewChunk
)
{
	BLT_UInt32 ret;

	DEBUG0("1\n");
	DEBUG0("entry_idx: %d, pstsc->entry_count: %d, pstsc->entry_fill: %d\n", pstsc->entry_idx, pstsc->entry_count, pstsc->entry_fill);
	DEBUG0("sample_idx: %d, pstsc->sample_count: %d\n", pstsc->sample_idx, pstsc->sample_count);
	DEBUG0("isLastChunk: %d\n", pstsc->isLastChunk);
	DEBUG0("curr_chunk_id: %d, next_chunk_id: %d\n", pstsc->curr_chunk_id, pstsc->next_chunk_id);
	DEBUG0("\n");

	if ( pstsc->isLastChunk == 0 ) 
	{
		DEBUG0("***** pstsc->isLastChunk == 0 ***\n");
		DEBUG0("pstsc->entry_idx =  %d\n", pstsc->sample_idx);
		DEBUG0("pstsc->entry_fill =  %d\n", pstsc->entry_fill);
		DEBUG0("pstsc->sample_count =  %d\n", pstsc->sample_count);
		
		
		if ((pstsc->entry_idx >= pstsc->entry_fill) && (pstsc->sample_idx >= pstsc->sample_count)) 
		{
			DEBUG0("***** 11111111 ***\n");
			/* Refill data */
			if ( (ret = vid3GPSTSCIdxRefill(pParser,pstsc, idx)) != BLT_SUCCESS ) 
			{
				return ret;
			}
			if (pstsc->isFirstFilled == 0) 
			{
				pstsc->isFirstFilled = 1;
			}
			else 
			{
				pstsc->curr_chunk_id += 1;
			}
			if ( pstsc->curr_chunk_id == pstsc->next_chunk_id ) 
			{
				pstsc->curr_chunk_id = BREAD32(p3GPIdxBufCurr[idx]); /* VFS_Read curr chunk id */
				(p3GPIdxBufCurr[idx]) += 4;
				pstsc->sample_count = BREAD32(p3GPIdxBufCurr[idx]);
				(p3GPIdxBufCurr[idx]) += 8;     /* skip description id */
				pstsc->entry_idx += 1;

				if ( pstsc->entry_idx == pstsc->entry_count )
				{
					/* There is no next chunk ID */
					pstsc->isLastChunk = 1;
					pstsc->next_chunk_id = 0xFFFFFFFF;
				}
				else {
					/* Read next chunk ID */
					pstsc->next_chunk_id = BREAD32(p3GPIdxBufCurr[idx]); /* VFS_Read next chunk id */
				}
			}
			pstsc->sample_idx = 0;
			DEBUG0("2\n");
			DEBUG0("sample_idx: %d, pstsc->sample_count: %d\n", pstsc->sample_idx, pstsc->sample_count);
			DEBUG0("curr_chunk_id: %d, next_chunk_id: %d\n", pstsc->curr_chunk_id, pstsc->next_chunk_id);
			DEBUG0("\n");
		}
	}

	DEBUG0("3\n");
	DEBUG0("entry_idx: %d, pstsc->entry_count: %d, pstsc->entry_fill: %d\n", pstsc->entry_idx, pstsc->entry_count, pstsc->entry_fill);
	DEBUG0("sample_idx: %d, pstsc->sample_count: %d\n", pstsc->sample_idx, pstsc->sample_count);
	DEBUG0("isLastChunk: %d\n", pstsc->isLastChunk);
	DEBUG0("curr_chunk_id: %d, next_chunk_id: %d\n", pstsc->curr_chunk_id, pstsc->next_chunk_id);
	DEBUG0("\n");

	if ( pstsc->sample_idx >= pstsc->sample_count ) 
	{
		pstsc->sample_idx = 0;
		if ( pstsc->isLastChunk == 0 )
		{
			pstsc->curr_chunk_id += 1;
			if ( pstsc->curr_chunk_id == pstsc->next_chunk_id ) 
			{
				pstsc->curr_chunk_id = BREAD32(p3GPIdxBufCurr[idx]); /* VFS_Read curr chunk id */
				(p3GPIdxBufCurr[idx]) += 4;
				pstsc->sample_count = BREAD32(p3GPIdxBufCurr[idx]);
				(p3GPIdxBufCurr[idx]) += 8;     /* skip description id */
				pstsc->entry_idx += 1;
				if ( pstsc->entry_idx == pstsc->entry_count )
				{
					/* There is no next chunk ID */
					pstsc->isLastChunk = 1;
					pstsc->next_chunk_id = 0xFFFFFFFF;
				}
				else if (pstsc->entry_idx == pstsc->entry_fill )
				{
					/* we should peek the next packet's first chunk id */
					/* The next chunk ID will be filled in pstsc->next_chunk_id automatically */
					if ((ret = vid3GPSTSCIdxPeek(pParser, pstsc)) != BLT_SUCCESS)
					{
						return ret;
					}
					DEBUG0("end of peek!\n");
				}
				else 
				{
					pstsc->next_chunk_id = BREAD32(p3GPIdxBufCurr[idx]); /* VFS_Read next chunk id */
				}
			}
		}
		*pNewChunk = 1;
	}
	else {
		if ( pstsc->sample_idx == 0 ) {
			*pNewChunk = 1;
		}
		else {
			*pNewChunk = 0;
		}
	}
	pstsc->sample_idx++;

	DEBUG0("e_idx: 0x%x, fill: 0x%x, count: 0x%x\n", pstsc->entry_idx, pstsc->entry_fill, pstsc->entry_count);
	DEBUG0("s_idx: 0x%x, s_cout: 0x%x\n", pstsc->sample_idx, pstsc->sample_count);
	DEBUG0("curr chunk: %d, next chunk: %d\n", pstsc->curr_chunk_id, pstsc->next_chunk_id);
	DEBUG0("new: %d\n", *pNewChunk);

	return BLT_SUCCESS;
}

static BLT_UInt16
vid3GPSTCOIdxRefill(
	M4aParser *pParser,
	_3gpSTCOInfo_t *pstco,
	BLT_UInt32 idx
)
{
	BLT_UInt32 size;
	BLT_UInt32 count;
	BLT_UInt8 *pbufStart;
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;
	BLT_Int32  ifilled;
	BLT_UInt32 uiBufSize;
	ATX_Size   bytes_read;

	count = pstco->entry_count - pstco->entry_fill;
	if ( count == 0 ) 
	{
		/* No more to refill */
		return VID_PLAY_ERR015;
	}
	size = count * 4;
	if (idx == IDX_VID_STCO)
	{
		uiBufSize = IDX_VID_STCO_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 4;
		}
	} 
	else if (idx == IDX_AUD_STCO)
	{
		uiBufSize = IDX_AUD_STCO_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 4;
		}
	}
	else
	{
		return BLT_FAILURE;
	}

	pbufStart = p3GPIdxBufStart[idx];
	filePos = pstco->filePos;

	vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
	ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
	//if (ifilled < 0)
	if ((BLT_UInt32)ifilled < 0)
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	ifilled = ATX_InputStream_Read(pParser->input.stream, pbufStart, sizeRead, &bytes_read);
	if (ifilled < 0) 	
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	pbufStart += offsetLeft;
	
	pstco->filePos = filePos + size;
	pstco->entry_fill += count;
	p3GPIdxBufCurr[idx] = pbufStart;

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPSTCOGet(
	M4aParser *pParser,
	_3gpSTCOInfo_t *pstco,
	BLT_UInt32 idx,
	BLT_UInt32 *poffset
)
{
	BLT_UInt32 uiOffset;
	BLT_UInt32 ret;
	BLT_UInt32 size;	
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;

	DEBUG0("###### into  vid3GPSTCOGet ######\n");
	if ( pstco->entry_idx >= pstco->entry_count )
	{
		//DBG_CHECK(VID_PLAY_ERR015);
		return VID_PLAY_ERR015;
	}
	if ( pstco->entry_idx >= pstco->entry_fill ) 
	{DEBUG0("###### pstco->entry_idx >= pstco->entry_fill ######\n");
		/* Refill data */
		if ( (ret = vid3GPSTCOIdxRefill(pParser,pstco, idx)) != BLT_SUCCESS ) 
		{
			return ret;
		}
	}
	else if (pstco->entry_idx == 0)
	{	DEBUG0("###### pstco->entry_idx == 0 ######\n");
		size = pstco->entry_count * 4;
		filePos = pstco->filePos_start;
		vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
		p3GPIdxBufCurr[idx] = p3GPIdxBufStart[idx] + offsetLeft;		
	}
	uiOffset = BREAD32(p3GPIdxBufCurr[idx]);
	DEBUG0("idx: 0x%x, offset: 0x%x, fill: 0x%x\n", pstco->entry_idx, offset, pstco->entry_fill);
	(p3GPIdxBufCurr[idx]) += 4;
	pstco->entry_idx += 1;

	*poffset = uiOffset;
	DEBUG0("###### uiOffset = %u ######\n", uiOffset);

	return BLT_SUCCESS;
}



static BLT_UInt16
vid3GPSTSZIdxRefill(
	M4aParser *pParser,
	_3gpSTSZInfo_t *pstsz,
	BLT_UInt32 idx
)
{
	BLT_UInt32 size;
	BLT_UInt32 count;
	BLT_UInt8 *pbufStart;
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;
	BLT_Int32 ifilled;
	BLT_UInt32 uiBufSize;
	ATX_Size   bytes_read;

	DEBUG0("** intyo vid3GPSTSZIdxRefill **\n");
	count = pstsz->sample_count - pstsz->sample_fill;
	if ( count == 0 )
	{
		/* No more to refill */
		return VID_PLAY_ERR015;
	}
	size = count * 4;
	if (idx == IDX_VID_STSZ)
	{
		uiBufSize = IDX_VID_STSZ_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 4;
		}
	} 
	else if (idx == IDX_AUD_STSZ)
	{
		uiBufSize = IDX_AUD_STSZ_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 4;
		}
	}
	else
	{
		return BLT_FAILURE;
	}

	pbufStart = p3GPIdxBufStart[idx];
	filePos = pstsz->filePos;
	vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
	ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
	//if (ifilled < 0) 
	if ((BLT_UInt32)ifilled < 0)
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	ifilled = ATX_InputStream_Read(pParser->input.stream, pbufStart, sizeRead, &bytes_read);
	if (ifilled < 0) 	
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	pbufStart += offsetLeft;
	
	pstsz->filePos = filePos + size;
	pstsz->sample_fill += count;
	p3GPIdxBufCurr[idx] = pbufStart;

	DEBUG0("** exit vid3GPSTSZIdxRefill **\n");

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPSTSZGet(
	M4aParser* pParser,
	_3gpSTSZInfo_t *pstsz,
	BLT_UInt32 idx,
	BLT_UInt32 *psize
)
{
	BLT_UInt32 uiSize;
	BLT_UInt32 ret;
	BLT_UInt32 size;	
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;

	DEBUG0("######### into vid3GPSTSZGet ###########\n");
	if ( pstsz->sample_size == 0 ) 
	{
		if ( pstsz->sample_idx >= pstsz->sample_count ) 
		{
			//DBG_CHECK(VID_PLAY_ERR015);
			return VID_PLAY_ERR015;
		}
		if ( pstsz->sample_idx >= pstsz->sample_fill )
		{
			/* Refill data */
			if ( (ret = vid3GPSTSZIdxRefill(pParser, pstsz, idx)) !=BLT_SUCCESS ) 
			{
				return ret;
			}
		}
		else if (pstsz->sample_idx == 0)
		{	
			size = pstsz->sample_count * 4;
			filePos = pstsz->filePos_start;
			vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
			p3GPIdxBufCurr[idx] = p3GPIdxBufStart[idx] + offsetLeft;
		}
		uiSize = BREAD32(p3GPIdxBufCurr[idx]);
		(p3GPIdxBufCurr[idx]) += 4;
		pstsz->sample_idx += 1;
	}
	else 
	{
		uiSize = pstsz->sample_size;
	}

	*psize = uiSize;
	DEBUG0("######### return  vid3GPSTSZGet  success###########\n");
	return BLT_SUCCESS;
}



static BLT_UInt16
vid3GPSTTSIdxRefill(
	M4aParser* pParser,
	_3gpSTTSInfo_t *pstts,
	BLT_UInt32 idx
)
{
	BLT_UInt32 size;
	BLT_UInt32 count;
	BLT_UInt8 *pbufStart;
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;
	BLT_Int32  ifilled;
	BLT_UInt32 uiBufSize;
	ATX_Size   bytes_read;

	count = pstts->entry_count - pstts->entry_fill;
	if ( count == 0 ) 
	{
		/* No more to refill */
		return VID_PLAY_ERR015;
	}
	size = count * 8;
	if (idx == IDX_VID_STTS)
	{
		uiBufSize = IDX_VID_STTS_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 8;
		}
	} 
	else if (idx == IDX_AUD_STTS)
	{
		uiBufSize = IDX_AUD_STTS_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 8;
		}
	}
	else
	{
		return BLT_FAILURE;
	}
	pbufStart = p3GPIdxBufStart[idx];
	filePos = pstts->filePos;
	vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
	ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
	//if (ifilled < 0)
	if ((BLT_UInt32)ifilled < 0)
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}

	ifilled = ATX_InputStream_Read(pParser->input.stream, pbufStart, sizeRead, &bytes_read);
	if (ifilled < 0)	
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	pbufStart += offsetLeft;	

	pstts->filePos = filePos + size;
	pstts->entry_fill += count;
	p3GPIdxBufCurr[idx] = pbufStart;

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPSTTSGet(
	M4aParser* pParser,
	_3gpSTTSInfo_t *pstts,
	BLT_UInt32 idx,
	BLT_UInt32 *ptime
)
{
	BLT_UInt32 time;
	BLT_UInt32 ret;
	BLT_UInt32 size;	
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;	
	DEBUG0("&&&&& INTO vid3GPSTTSGet &&&&&&&\n");
	if ( pstts->entry_idx > pstts->entry_count )
	{
		//DBG_CHECK(VID_PLAY_ERR015);
		return VID_PLAY_ERR015;
	}
	if (pstts->sample_idx >= (BLT_Int32)pstts->sample_count)
	{
		if ( pstts->entry_idx >= pstts->entry_fill ) 
		{
			/* Refill data */
			if ( (ret = vid3GPSTTSIdxRefill(pParser, pstts, idx)) != BLT_SUCCESS )
			{
				return ret;
			}
		}
		else if (pstts->entry_idx == 0)
		{
			size = pstts->entry_count * 8;
			filePos = pstts->filePos_start;
			vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
			p3GPIdxBufCurr[idx] = p3GPIdxBufStart[idx] + offsetLeft;
		}
		pstts->sample_count = BREAD32(p3GPIdxBufCurr[idx]);
		(p3GPIdxBufCurr[idx]) += 4;
		pstts->sample_duration = BREAD32(p3GPIdxBufCurr[idx]);
		(p3GPIdxBufCurr[idx]) += 4;
		pstts->sample_idx = 0;
		pstts->entry_idx += 1;
	}
	time = pstts->sample_duration;
	pstts->sample_idx += 1;
	*ptime = time;
	DEBUG0("&&&&& pstts->sample_idx = %d&&&&&&&\n", pstts->sample_idx);
	DEBUG0("&&&&& time = %d&&&&&&&\n", time);
	DEBUG0("&&&&& exit vid3GPSTTSGet &&&&&&&\n");

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPIdxInfoGet(
	M4aParser* pParser,
	BLT_UInt32 trak,
	BLT_UInt32 *poffset,
	BLT_UInt32 *psize,
	BLT_UInt32 *ptime
)
{
	static BLT_UInt32 lastVidOffset;
	static BLT_UInt32 lastVidSize;
	static BLT_UInt32 lastAudOffset;
	static BLT_UInt32 lastAudSize;
	BLT_UInt32 newChunk;
	BLT_UInt32 ret;

	DEBUG0("######### into vid3GPIdxInfoGet ##########\n");
	
	if ( trak == VIDEO_TRAK )
	{
		if ( (ret = vid3GPSTSCGet(pParser, &_3gpVidSTSC, IDX_VID_STSC, &newChunk)) != BLT_SUCCESS ) 
		{
			//DEBUG0("1 Get stsc fail\n");
			return ret;
		}
		if ( newChunk == 1 ) 
		{
			if ( (ret = vid3GPSTCOGet(pParser, &_3gpVidSTCO, IDX_VID_STCO, poffset)) != BLT_SUCCESS ) 
			{
				//DEBUG0("Get stco fail\n");
				return ret;
			}		
		}
		else 
		{
			*poffset = lastVidOffset + lastVidSize;
		}
		if (guiFileSize <= *poffset)
		{
			return BLT_FAILURE;
		}
		if ( (ret = vid3GPSTSZGet(pParser, &_3gpVidSTSZ, IDX_VID_STSZ, psize)) != BLT_SUCCESS ) 
		{
			//DEBUG0("Get stsz fail\n");
			return ret;
		}
		if ( (ret = vid3GPSTTSGet(pParser,&_3gpVidSTTS, IDX_VID_STTS, ptime)) != BLT_SUCCESS ) 
		{
			//DEBUG0("Get stts fail\n");
			return ret;
		}
		lastVidOffset = *poffset;
		lastVidSize = *psize;
	}
	else 
	{
		DEBUG0("vid3GPIdxInfoGet::AUDIO_TRACK\n");
		if ( (ret = vid3GPSTSCGet(pParser,&_3gpAudSTSC, IDX_AUD_STSC, &newChunk)) != BLT_SUCCESS )
		{
			return ret;

		}
		DEBUG0("### 1111 ###\n");
		if ( newChunk == 1 ) 
		{DEBUG0("### 2222 ###\n");
			if ( (ret = vid3GPSTCOGet(pParser,&_3gpAudSTCO, IDX_AUD_STCO, poffset)) != BLT_SUCCESS )
			{DEBUG0("### 3333 ###\n");
				return ret;
			}
		}
		else 
		{
			*poffset = lastAudOffset + lastAudSize;
		}
		if (guiFileSize <= *poffset)
		{DEBUG0("### 4444 ###\n");
			return BLT_FAILURE;
		}
		if ( (ret = vid3GPSTSZGet(pParser,&_3gpAudSTSZ, IDX_AUD_STSZ, psize)) != BLT_SUCCESS )
		{DEBUG0("### 5555 ###\n");
			return ret;
		}
		
		if ( (ret = vid3GPSTTSGet(pParser,&_3gpAudSTTS, IDX_AUD_STTS, ptime)) != BLT_SUCCESS) 
		{DEBUG0("### 6666 ###\n");
			DEBUG0("Get stts fail\n");
			return ret;
		}
		lastAudOffset = *poffset;
		lastAudSize = *psize;
		DEBUG0("offset: 0x%x, size: 0x%x\n", *poffset, *psize);
	}
	DEBUG0("######### return success from vid3GPIdxInfoGet ###########\n");
	return BLT_SUCCESS;
}



static BLT_UInt16
vid3GPSTSSIdxRefill(
	M4aParser* pParser,
	_3gpSTSSInfo_t *pstss,
	BLT_UInt32 idx
)
{
	BLT_UInt32 size;
	BLT_UInt32 count;
	BLT_UInt8 *pbufStart;
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;
	BLT_Int32  ifilled;
	BLT_UInt32 uiBufSize;
	ATX_Size bytes_read = 0;

	count = pstss->entry_count - pstss->entry_fill;
	if ( count == 0 ) {
		/* No more to refill */
		return VID_PLAY_ERR015;
	}
	size = count * 4;
	if (idx == IDX_VID_STSS)
	{
		uiBufSize = IDX_VID_STSS_BUF_SIZE;
		if (size > uiBufSize)
		{
			size = uiBufSize;
			count = uiBufSize / 4;
		}
	} 	
	else
	{
		return BLT_FAILURE;
	}

	pbufStart = p3GPIdxBufStart[idx];
	filePos = pstss->filePos;
	vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
	ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
	//if (ifilled < 0)
	if ((BLT_UInt32)ifilled < 0)
	{
		//DBG_CHECK(ret);
		return BLT_FAILURE;
	}
	ifilled = ATX_InputStream_Read(pParser->input.stream, pbufStart, sizeRead, &bytes_read);
	if (ifilled < 0)
	{
		return BLT_FAILURE;
	}
	pbufStart += offsetLeft;
	
	pstss->filePos = filePos + size;
	pstss->entry_fill += count;
	p3GPIdxBufCurr[idx] = pbufStart;

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPSTSSGet(
	M4aParser* pParser,
	_3gpSTSSInfo_t *pstss,
	BLT_UInt32 idx,
	BLT_UInt32 *pkeyFrameNum
)
{
	BLT_UInt32 ret = BLT_SUCCESS;
	BLT_UInt32 keyFrameNum = 0;
	BLT_UInt32 size;	
	BLT_UInt32 filePos;
	BLT_UInt32 offset, offsetLeft, sizeRead;

	if ( pstss->entry_idx >= pstss->entry_count )
	{
		return VID_PLAY_ERR015;
	}
	if ( pstss->entry_idx >= pstss->entry_fill )
	{
		/* Refill data */
		if ( (ret = vid3GPSTSSIdxRefill(pParser, pstss, idx)) != BLT_SUCCESS ) 
		{
			return ret;
		}
	}
	else if (pstss->entry_idx == 0)
	{	
		size = pstss->entry_count * 4;
		filePos = pstss->filePos_start;
		vid3GPPosBounderGet(filePos, size, &offset, &offsetLeft, &sizeRead);
		p3GPIdxBufCurr[idx] = p3GPIdxBufStart[idx] + offsetLeft;		
	}
	keyFrameNum = BREAD32(p3GPIdxBufCurr[idx]);
	(p3GPIdxBufCurr[idx]) += 4;
	pstss->entry_idx += 1;

	*pkeyFrameNum = keyFrameNum;

	return BLT_SUCCESS;
}


static BLT_UInt32
vid3GPDataFill(
	M4aParser* pParser,
	BLT_UInt8 *pBuf,
	BLT_UInt32 offset,
	BLT_UInt32 size,
	bufInfo_t *pInfo
)
{
	BLT_UInt8 *pDest;
	BLT_UInt8 *pSrc;
	BLT_UInt32 readSize;
	BLT_UInt32 seekOffset;
	BLT_UInt32 diffOffset;
	BLT_UInt32 bNeedSeek;
	BLT_Int32  ifilled;
	ATX_Size   bytes_read;

	pDest = pBuf;
	pSrc = 0;
	
	DEBUG0("########### into vid3GPDataFill #############\n");
	DEBUG0("##### size = %u ######\n", size);
	DEBUG0("##### pInfo->bufSize = %u ######\n", pInfo->bufSize);


	if (size >= pInfo->bufSize)
	{DEBUG0("##### size >= pInfo->bufSize ######\n");
		ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
		if ((BLT_UInt32)ifilled < 0)
		{
			return BLT_FAILURE;
		}
		DEBUG0("##### seek success ######\n");
		ifilled = ATX_InputStream_Read(pParser->input.stream, pDest, size, &bytes_read);
		if (ifilled < 0)
			{
			return BLT_FAILURE;
		}
		DEBUG0("##### read success ######\n");
		int i=0;
		for(i=0;i<size;i++){
			if(i%16==0 &&i!=0){
				DEBUG0("\n");
			}
			DEBUG0("%02x ", pDest[i]);
		}
		DEBUG0("\n");
		return BLT_SUCCESS;
	}

	bNeedSeek = 1;
	if ((offset >= pInfo->offsetInFileEnd) || (offset < pInfo->offsetInFileStart)) 
	{
		DEBUG0("##### || ######\n");
		seekOffset = (offset >> 9) << 9;
		diffOffset = offset - seekOffset;
		ifilled = ATX_InputStream_Seek(pParser->input.stream, seekOffset);
		if ((BLT_UInt32)ifilled < 0)
		{
			return BLT_FAILURE;
		}
		ifilled = ATX_InputStream_Read(pParser->input.stream,  pInfo->pbufStart, pInfo->bufSize, &bytes_read);
		if (ifilled < 0) 
		{
			return BLT_FAILURE;
		}
		pInfo->offsetInFileStart = seekOffset;
		pInfo->offsetInFileEnd = seekOffset + pInfo->bufSize;
		if (size > (pInfo->bufSize - diffOffset)) 
		{
			readSize = pInfo->bufSize - diffOffset;
		}
		else 
		{
			readSize = size;
		}
		pSrc = pInfo->pbufStart + diffOffset;
		ATX_CopyMemory(pDest, pSrc, readSize);
		size -= readSize;	
		pDest += readSize;
		bNeedSeek = 0;
	}
	else 
	{DEBUG0("##### else ######\n");
		pSrc = pInfo->pbufStart + (offset - pInfo->offsetInFileStart);
		if ((offset + size) > pInfo->offsetInFileEnd)
		{
			readSize = pInfo->offsetInFileEnd - offset;
		}
		else
		{
			readSize = size;
		}
		ATX_CopyMemory(pDest, pSrc, readSize);
		size -= readSize;
		pDest += readSize;
	}

	if (size > 0) 
	{DEBUG0("##### size > 0 ######\n");
		if (bNeedSeek == 1) 
		{			
			ifilled = ATX_InputStream_Seek(pParser->input.stream, pInfo->offsetInFileEnd);
			if ((BLT_UInt32)ifilled < 0 )
			{
				return BLT_FAILURE;
			}
		}
		ifilled = ATX_InputStream_Read(pParser->input.stream, pInfo->pbufStart, pInfo->bufSize, &bytes_read);
		if (ifilled < 0) 
		{
			return BLT_FAILURE;
		}
		pInfo->offsetInFileStart = pInfo->offsetInFileEnd;
		pInfo->offsetInFileEnd += pInfo->bufSize;
		pSrc = pInfo->pbufStart;

		DEBUG0("##### pInfo->offsetInFileStart = %u ######\n", pInfo->offsetInFileStart);
		DEBUG0("##### pInfo->offsetInFileEndt = %u ######\n", pInfo->offsetInFileEnd);
		
		ATX_CopyMemory(pDest, pSrc, size);
	}
	DEBUG0("##### return from vid3GPDataFill ######\n");

	return BLT_SUCCESS;
}



static BLT_UInt8 CheckMPEG4KeyFrm(BLT_UInt8 *pVData, BLT_UInt32 ui32VSize)
{
	BLT_UInt8 bKeyFrm;
	BLT_UInt32 nCode = 0;
	BLT_UInt32 ii;
	BLT_UInt8 nBytes;
	bKeyFrm = 0xff;
	for(ii = 0; ii < ui32VSize;ii++)
	{
		nCode = (nCode << 8) | (*pVData++);
		if(0x000001b6 == nCode)//vop start code
		{
			nBytes = (BLT_UInt8)(*pVData >> 6);
			if(0 == nBytes)
			{
				bKeyFrm = 0;
				break;
			}
			else
			{
				bKeyFrm = 1;
				break;
			}
		}
	}
	return bKeyFrm;
}


static BLT_UInt32
vid3GPVidNextGet(
	M4aParser*	pParser,
	BLT_UInt32* pSize,
	BLT_UInt32* pTime,
	BLT_UInt32* pType
)
{
	BLT_UInt32 ret;
	BLT_UInt32 offset;
	BLT_UInt32 time;
	BLT_UInt32 i;
	BLT_UInt32 tmp1, uikeySize;
	BLT_Int32  ifilled, iTell;
	BLT_UInt8 *pTemp;
	BLT_UInt8 ucMustKey;
	ATX_Position	where;
	ATX_Size bytes_read = 0;

	ucMustKey = 0;
ReSeekKeyFrame:
	do 
	{
		ret = vid3GPIdxInfoGet(pParser, VIDEO_TRAK, &offset, pSize, &time);
		if (ret != BLT_SUCCESS)
		{
			return ret;
		}
		if ((BLT_Int32)time < 0)
		{
			time = 0;
		}		
		gVidFrameNumber++;	
		
		if (gVidFrameNumber == gVidKeyFrame)
		{
			*pType = 0;
		}
		else
		{
			*pType = 1;
			while (gVidFrameNumber > gVidKeyFrame)
			{		
				ret = vid3GPSTSSGet(pParser, &_3gpVidSTSS, IDX_VID_STSS, &gVidKeyFrame);	
				if (ret == VID_PLAY_ERR015)
				{
					ret = BLT_SUCCESS;
					gVidKeyFrame = 0xFFFFFFFF;
					break;
				}
				if (ret != BLT_SUCCESS) 
				{
					return ret;
				}
				if (gVidFrameNumber == gVidKeyFrame)
				{
					*pType = 0;
				}
			}
		}
		
		gVidSampleTotalTime += (BLT_UInt64)g3gpVidLastTime;
		g3gpVidLastTime = time;
		vidNextOffset = offset;
		vidNextSize = *pSize;

		if (vidNextSize > 0x1FFFF)//mask frame size error
		{
			ucMustKey = 1;
		} 
		else if (ucMustKey == 1)
		{
			if (*pType == 0)
			{
				break;
			} 
		}
		else
		{
			break;
		}
	} while (1);

	if (gFirstFillVidData == 0)
	{
		*pSize += gbvidHeaderSize;
	} 

	if (en3GPVType == VIDEO_3GP_TYPE_MPEG4 && *pType == 0 && 
		gVidSampleTotalTime != 0 && ucMustSeekKeyFrame != 0xFF)
	{	
		iTell = ATX_InputStream_Tell(pParser->input.stream, &where);
		//if (iTell < 0)
		if ((BLT_UInt32)where == 0xFFFFFFFF)
		{
			return BLT_FAILURE;
		}
		ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
		//if (ifilled < 0)
		if ((BLT_UInt32)ifilled < 0)
		{			
			return BLT_FAILURE;
		}
		//uikeySize = vidNextSize;
		uikeySize = 64;
		ifilled = ATX_InputStream_Read(pParser->input.stream, pvidH264Data, uikeySize, &bytes_read);
		if ((BLT_UInt32)ifilled != uikeySize) 
		{			
			return BLT_FAILURE;
		}
		else
		{
			*pType = CheckMPEG4KeyFrm(pvidH264Data, uikeySize);
			if (*pType == 0xFF)
			{
				*pType = 1;
			}
			ifilled = ATX_InputStream_Seek(pParser->input.stream, iTell);
			//if (ifilled < 0)
			if (ifilled < 0)
			{			
				return BLT_FAILURE;
			}
		}
		
		if (ucMustSeekKeyFrame == 1)
		{
			if (*pType == 1)
			{				
				goto ReSeekKeyFrame;
			}		
		}		
	}
	else if (en3GPVType ==  VIDEO_3GP_TYPE_H264 && gblengthSizeMinusOne != 3)
	{
		if (vidNextSize > VID_DATA_BUF_SIZE)
		{
			return BLT_FAILURE;
		}
		else if (pvidH264Data == NULL)
		{
			return BLT_FAILURE;
		}
		ret = vid3GPDataFill(pParser,pvidH264Data, vidNextOffset, vidNextSize, &_3gpBufInfo[0]);
		if (ret != BLT_SUCCESS)
		{
			return ret;
		}
		pTemp = pvidH264Data;
		switch(gblengthSizeMinusOne)
		{
		case 0:
			for (i = 0; i< vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pTemp);			

				pTemp += tmp1 + 1;
				i += tmp1 + 1;
				*pSize += 3;

			}
			break;
		case 1:
			for (i = 0; i< vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pTemp) << 8;
				tmp1 |= *(pTemp + 1);

				pTemp += tmp1 + 2;
				i += tmp1 + 2;
				*pSize += 2;

			}
			break;
		case 2:
			for (i = 0; i< vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pTemp) << 16;
				tmp1 |= (*(pTemp + 1)) << 8;
				tmp1 |= *(pTemp + 2);

				pTemp += tmp1 + 3;
				i += tmp1 + 3;
				*pSize += 1;

			}
			break;		
		default:
			ret = BLT_FAILURE;			
			break;
		}		
	}

	if (ucMustSeekKeyFrame != 0xFF)
	{
		ucMustSeekKeyFrame = 0;
	}
	time = (BLT_UInt32)(gVidSampleTotalTime * (BLT_UInt64)1000 / (BLT_UInt64)vidTimeScale);
	*pTime = time;
    //DEBUG0("video time stamp: %d \n",time);
	return ret;
}




static BLT_UInt32
vid3GPVidDataFill(
	M4aParser* pParser,
	BLT_UInt8* pBuf
)
{
	BLT_UInt32 i;
	BLT_UInt32 tmp1;
	BLT_UInt32 ret;
	BLT_UInt8 *pSrc;
	BLT_UInt8 *pBufData;

	pBufData = pBuf;
	if (gFirstFillVidData == 0) // if ((gVidSampleTotalTime == 0)
	{
		ATX_CopyMemory(pBufData, pvidHeaderData, gbvidHeaderSize);
		pBufData += gbvidHeaderSize;
        gFirstFillVidData = 1;
	}	

	if (en3GPVType ==  VIDEO_3GP_TYPE_H264 && gblengthSizeMinusOne != 3)
	{	
		pSrc = pvidH264Data;
		switch(gblengthSizeMinusOne)
		{
		case 0:
			for (i = 0; i < vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pSrc);			

				*pBufData = 0x00;
				*(pBufData + 1) = 0x00;
				*(pBufData + 2) = 0x00;
				*(pBufData + 3) = 0x01;

				ATX_CopyMemory(pBufData + 4, pSrc + 1, tmp1);

				pBufData += tmp1 + 4;
				pSrc += tmp1 + 1;
				i += tmp1 + 1;
			}
			break;
		case 1:			
			for (i = 0; i < vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pSrc) << 8;
				tmp1 |= *(pSrc + 1);				

				*pBufData = 0x00;
				*(pBufData + 1) = 0x00;
				*(pBufData + 2) = 0x00;
				*(pBufData + 3) = 0x01;

				ATX_CopyMemory(pBufData + 4, pSrc + 2, tmp1);

				pBufData += tmp1 + 4;
				pSrc += tmp1 + 2;
				i += tmp1 + 2;
			}
			break;
		case 2:
			for (i = 0; i < vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pSrc) << 16;
				tmp1 |= (*(pSrc + 1)) << 8;	
				tmp1 |= *(pSrc + 2);			

				*pBufData = 0x00;
				*(pBufData + 1) = 0x00;
				*(pBufData + 2) = 0x00;
				*(pBufData + 3) = 0x01;

				ATX_CopyMemory(pBufData + 4, pSrc + 3, tmp1);

				pBufData += tmp1 + 4;
				pSrc += tmp1 + 3;
				i += tmp1 + 3;
			}
			break;				
		default:
			ret = BLT_FAILURE;			
			break;
		}

		ret = BLT_SUCCESS;
	}
	else
	{
		ret = vid3GPDataFill(pParser, pBufData, vidNextOffset, vidNextSize, &_3gpBufInfo[0]);
		if (en3GPVType ==  VIDEO_3GP_TYPE_H264 && gblengthSizeMinusOne == 3)
		{
			for (i = 0; i < vidNextSize;)
			{
				tmp1 = 0;
				tmp1 |= (*pBufData) << 24;
				tmp1 |= (*(pBufData + 1)) << 16;
				tmp1 |= (*(pBufData + 2)) << 8;
				tmp1 |= (*(pBufData + 3));

				*pBufData = 0x00;
				*(pBufData + 1) = 0x00;
				*(pBufData + 2) = 0x00;
				*(pBufData + 3) = 0x01;

				pBufData += tmp1 + 4;
				i += tmp1 + 4;
			}
		}
	}

	if(ret == BLT_FAILURE)
	{
		DEBUG0("vid3GPVidDataFill - fail\n");
	}

	return ret;
}

static void
vidVideoInfoReset(
	void
)
{
	bufInfo_t *pbufInfo;
	BLT_UInt32 uiBufSize;

	/* STSS */
	uiBufSize = IDX_VID_STSS_BUF_SIZE;
	if (_3gpVidSTSS.entry_idx >= _3gpVidSTSS.entry_fill || _3gpVidSTSS.entry_fill == 0)
	{
		_3gpVidSTSS.filePos = _3gpVidSTSS.filePos_start;
		_3gpVidSTSS.entry_fill = 0;	
	}
	else if ((_3gpVidSTSS.filePos - _3gpVidSTSS.filePos_start) > uiBufSize)
	{
	_3gpVidSTSS.filePos = _3gpVidSTSS.filePos_start;
	_3gpVidSTSS.entry_fill = 0;
	}
	_3gpVidSTSS.entry_idx = 0;

	/* STSZ */
	uiBufSize = IDX_VID_STSZ_BUF_SIZE;
	if (_3gpVidSTSZ.sample_idx >= _3gpVidSTSZ.sample_fill || _3gpVidSTSZ.sample_fill == 0)
	{
		_3gpVidSTSZ.filePos = _3gpVidSTSZ.filePos_start;
		_3gpVidSTSZ.sample_fill = 0;
	}
	else if ((_3gpVidSTSZ.filePos - _3gpVidSTSZ.filePos_start) > uiBufSize)
	{
	_3gpVidSTSZ.filePos = _3gpVidSTSZ.filePos_start;
	_3gpVidSTSZ.sample_fill = 0;
	}
	_3gpVidSTSZ.sample_idx = 0;

	/* STCO */
	uiBufSize = IDX_VID_STCO_BUF_SIZE;
	if (_3gpVidSTCO.entry_idx >= _3gpVidSTCO.entry_fill || _3gpVidSTCO.entry_fill == 0)
	{
		_3gpVidSTCO.filePos = _3gpVidSTCO.filePos_start;
		_3gpVidSTCO.entry_fill = 0;
	}
	else if ((_3gpVidSTCO.filePos - _3gpVidSTCO.filePos_start) > uiBufSize)
	{
	_3gpVidSTCO.filePos = _3gpVidSTCO.filePos_start;
	_3gpVidSTCO.entry_fill = 0;
	}	
	_3gpVidSTCO.entry_idx = 0;

	/* STSC */
	_3gpVidSTSC.filePos = _3gpVidSTSC.filePos_start;
	_3gpVidSTSC.entry_idx = 0;
	_3gpVidSTSC.entry_fill = 0;
	_3gpVidSTSC.sample_count = 0;
	_3gpVidSTSC.sample_idx = 0;
	_3gpVidSTSC.isLastChunk = 0;
	_3gpVidSTSC.next_chunk_id = 0;
	_3gpVidSTSC.curr_chunk_id = 0;
	_3gpVidSTSC.isFirstFilled = 0;

	/* STTS */
	uiBufSize = IDX_VID_STTS_BUF_SIZE;
	if(_3gpVidSTTS.entry_idx >= _3gpVidSTTS.entry_fill || _3gpVidSTTS.entry_fill == 0)
	{
		_3gpVidSTTS.filePos = _3gpVidSTTS.filePos_start;
		_3gpVidSTTS.entry_fill = 0;
	}	
	else if ((_3gpVidSTTS.filePos - _3gpVidSTTS.filePos_start) > uiBufSize)
	{
	_3gpVidSTTS.filePos = _3gpVidSTTS.filePos_start;
	_3gpVidSTTS.entry_fill = 0;
	}
	_3gpVidSTTS.entry_idx = 0;
	_3gpVidSTTS.sample_count = 0;
	_3gpVidSTTS.sample_idx = 0;

	pbufInfo = &_3gpBufInfo[0];
	pbufInfo->offsetInFileStart = 0;
	pbufInfo->offsetInFileEnd = 0;
}



/*----------------------------------------------------------------------
|   vid3GPHeaderParse
+---------------------------------------------------------------------*/

static BLT_UInt32
vid3GPHeaderParse(M4aParser *pParser, ContainBitsInfo_t* pVidInfo)
{
	BLT_UInt32	bufSize;
	BLT_UInt32 bufSizeUsed;
	BLT_UInt8 *pbuf, *pbuf1;
	BLT_UInt32 size;
	BLT_UInt32 filePos;
	BLT_UInt32 foundVideo, foundAudio;
	BLT_UInt32 stringID;
	BLT_UInt32 timescale;
	BLT_UInt32 trakID;
	BLT_UInt32 isRefill;
	BLT_UInt64 duration;
	BLT_UInt8 isSTBLBefore;
	BLT_UInt32 tmpPos;
	BLT_UInt32 offset, offsetLeft, sizeRead;
	BLT_Result result = BLT_SUCCESS;
	BLT_Int32  ifilled;
	BLT_UInt32 audDuration, vidDuration;
	BLT_UInt32 uiSampleCount, uiICount;
	BLT_UInt32 bytes_read;
	/*store the ftyp box length*/

	DEBUG0("######### into vid3GPHeaderParse ##########\n");

	result = ATX_InputStream_GetSize(pParser->input.stream, &guiFileSize);
	if (result != BLT_SUCCESS)  
	{
		//DBG_CHECK(ret);
		return result;
	}
	else
	{
		DEBUG0("*****file size:%d ******\n",guiFileSize);
	}

	g_mp4_parse->file_size = guiFileSize;
	
	bufSize = 1024;
	pbuf1 = p3GPPktBufStart;

	DEBUG0("***** 11111 ******\n");
	ATX_SetMemory(pbuf1, 0, bufSize);
	ifilled = ATX_InputStream_Seek(pParser->input.stream, 0);
	//if (ifilled < 0)
	if ((BLT_UInt32)ifilled != 0)
	{
		return BLT_FAILURE;
	}
	ifilled = ATX_InputStream_Read(pParser->input.stream, pbuf1, bufSize, bytes_read );
	if (ifilled < 0) 
	{
		return BLT_FAILURE;
	}
	DEBUG0("***** 2222 ******\n");
	#if 1
	int i;
	for(i=0;i<bufSize;i++){
		if(i%16==0 && i!=0){
			DEBUG0("\n");
		}
		DEBUG0("%02x ", pbuf1[i]);
	}
	DEBUG0("\n");

	#endif
	pbuf = pbuf1;

	size = 0;
	bufSizeUsed = 0;
	timescale = 0;
	duration = 0;
	foundVideo = 0;
	foundAudio = 0;
	trakID = 0;
	filePos = 0;
	isRefill = 0;
	isSTBLBefore = 0;
	tmpPos = 0;
	mdatOffset = 0;
	vidTimeScale = 0;
	audTimeScale = 0;
	vidDuration = 0;
	audDuration = 0;
	vidFirstSampleOffset = 0;
	vidFirstSampleSize = 0;
	uiSampleCount = 0;
	uiICount = 0;

	guiVidTotalLen = 0;
	guiVidFramerate = 0;
	guTotalFrame = 0;
	gbVidTrakExist = 0;
	gbAudTrakExist = 0;
	gAudType = AUDIO_TYPE_NONE;

	gbvidHeaderSize = 0;
	pvidHeaderData = p3GPPktBufStart + vid3GPMemSizeGet(0) - 512;
	gblengthSizeMinusOne = 0xFF;
	pvidH264Data = NULL;
	pAudExtraData = NULL;
	
	ATX_SetMemory(pVidInfo, 0, sizeof(ContainBitsInfo_t));
	pVidInfo->isVBR = 2;
	pVidInfo->bSeekable = 0;
	/* TODO: Use memAlloc instead static variable */
	ATX_SetMemory(&_3gpVidSTTS, 0, sizeof(_3gpSTTSInfo_t));
	ATX_SetMemory(&_3gpVidSTSS, 0, sizeof(_3gpSTSSInfo_t));
	ATX_SetMemory(&_3gpVidSTSZ, 0, sizeof(_3gpSTSZInfo_t));
	ATX_SetMemory(&_3gpVidSTCO, 0, sizeof(_3gpSTCOInfo_t));
	ATX_SetMemory(&_3gpVidSTSC, 0, sizeof(_3gpSTSCInfo_t));
	ATX_SetMemory(&_3gpAudSTSS, 0, sizeof(_3gpSTSSInfo_t));
	ATX_SetMemory(&_3gpAudSTSZ, 0, sizeof(_3gpSTSZInfo_t));
	ATX_SetMemory(&_3gpAudSTCO, 0, sizeof(_3gpSTCOInfo_t));
	ATX_SetMemory(&_3gpAudSTSC, 0, sizeof(_3gpSTSCInfo_t));
	ATX_SetMemory(&_3gpAudSTTS, 0, sizeof(_3gpSTTSInfo_t));

	do {
		size = BREAD32(pbuf);
		stringID = BREAD32(pbuf + 4);

		DEBUG0("size = %d\n", size);
		DEBUG0("stringID = %d\n", stringID);
		DEBUG0("===== token: 0x%x, size: 0x%x ===== \n", stringID, size);
		if ( size == 0 ) 
		{
			DEBUG0("Header error, size is 0, token: 0x%x\n", stringID);
			break;
		}
		switch (stringID)
		{
		case _3GP_ID_MDAT:
#if 1
			DEBUG0("_3GP_ID_MDAT \n");
			mdatOffset = filePos + bufSizeUsed;
#endif
			break;
		case _3GP_ID_MOOV:
#if 1
			DEBUG0("_3GP_ID_MOOV \n");
			DEBUG0("Found moov at: 0x%x\n", filePos + bufSizeUsed );
			size = 8;
#endif
			break;
		case _3GP_ID_TRAK:
#if 1

			DEBUG0("_3GP_ID_TRAK \n");
			trakID = 0;
			size = 8;
#endif

			break;
		case _3GP_ID_MDIA:
#if 1
			DEBUG0("_3GP_ID_MDIA \n");
			DEBUG0("Found mdia at: 0x%x\n", filePos + bufSizeUsed );
			size = 8;
#endif
			break;
		case _3GP_ID_MDHD:
#if 1
			DEBUG0("_3GP_ID_MDHD \n");
			timescale = BREAD32(pbuf + 20); /* time scale is offset 20 */
			duration = BREAD32(pbuf + 24); /* duration is offset 24 */
			DEBUG0("timescale = %u \n",timescale );
			DEBUG0("duration = %llu \n", duration);
			if (vidDuration == 1)
			{
				if (trakID == VIDEO_TRAK)
				{
					vidTimeScale = timescale;
					vidDuration = (BLT_UInt32)((duration * 1000) / (BLT_UInt64)timescale);
					pVidInfo->vidLen = vidDuration;
				} 
				else if (trakID == AUDIO_TRAK)
				{
					audTimeScale = timescale;
					audDuration = (BLT_UInt32)((duration * 1000) / (BLT_UInt64)timescale);
					pVidInfo->audLen = audDuration;
				}
			}
			DEBUG0("mdhdTimeScale: %d\n", timescale);
			DEBUG0("duration = %llu \n", duration);
#endif
			break;
		case _3GP_ID_MINF:
#if 1
			DEBUG0("_3GP_ID_MINF \n");
			trakID = 0;
			size = 8;
#endif
			break;
		case _3GP_ID_VMHD:
			DEBUG0("_3GP_ID_VMHD \n");
#if 1
			if (gbVidTrakExist == 0) 
			{
				gbVidTrakExist = 1;
			}
			else 
			{
				return VID_PLAY_ERR01D;
			}

			DEBUG0("Found video trak at: 0x%x\n", filePos + bufSizeUsed );
			trakID = VIDEO_TRAK;
			vidTimeScale = timescale;
			vidDuration = (BLT_UInt32)((duration * 1000) / (BLT_UInt64)timescale);
			pVidInfo->vidLen = vidDuration;

			DEBUG0("V: Timescale: 0x%x, Duration: 0x%x 0x%x\n", vidTimeScale, duration, vidDuration);
			foundVideo = 1;

			if ( isSTBLBefore == 1 )
			{
				isRefill = 1;
				filePos = tmpPos;
				bufSizeUsed = 0;
				size = 0;
				isSTBLBefore = 0;
				gbVidTrakExist = 0; /* Avoid to happen VID_PLAY_ERR01D*/
			}
#endif
			break;
		case _3GP_ID_SMHD:
			DEBUG0("_3GP_ID_SMHD \n");
#if 1
			if (gbAudTrakExist == 0) 
			{
				gbAudTrakExist = 1;
			}
			else
			{
				return VID_PLAY_ERR01D;
			}

			DEBUG0("Found audio trak at: 0x%x\n", filePos + bufSizeUsed );
			trakID = AUDIO_TRAK;
			audTimeScale = timescale;
			audDuration = (BLT_UInt32)((duration * 1000) / (BLT_UInt64)timescale);
			pVidInfo->audLen = audDuration;

			
			DEBUG0("duration = %llu\n", duration);
			DEBUG0("timescale = 0x%x\n", timescale);
			DEBUG0("audDuration = 0x%x\n", audDuration);
			DEBUG0("pVidInfo->audLen = 0x%x\n", pVidInfo->audLen);
		
			DEBUG0("A: Timescale: 0x%x, Duration: %llu 0x%x\n", audTimeScale, duration, audDuration);
			foundAudio = 1;

			if ( isSTBLBefore == 1 )
			{
				isRefill = 1;
				filePos = tmpPos;
				bufSizeUsed = 0;
				size = 0;
				isSTBLBefore = 0;
				gbAudTrakExist = 0; /* Avoid to happen VID_PLAY_ERR01D*/
			}
#endif
			break;
		case _3GP_ID_HMHD:
		case _3GP_ID_NMHD:
			DEBUG0("_3GP_ID_NMHD \n");
#if 1
			trakID = INFO_TRAK;
			if ( isSTBLBefore == 1 ) 
			{
				isRefill = 1;
				filePos = tmpPos;
				bufSizeUsed = 0;
				size = 0;
				isSTBLBefore = 0;
			}
#endif
			break;
		case _3GP_ID_STBL:
			DEBUG0("_3GP_ID_STBL \n");
#if 1
			if ( trakID != 0 )
			{
			DEBUG0("trakID != 0 \n");
				size = 8;
				isRefill = 0;
			}
			else 
			{
			DEBUG0("trakID == 0 \n");
				isSTBLBefore = 1;
				tmpPos = filePos + bufSizeUsed;
				/* vmhd or smhd may be after STBL box */
				//DEBUG0("tmpPos: 0x%x\n", tmpPos);
			}
#endif
			break;
		case _3GP_ID_STSD:
			DEBUG0("_3GP_ID_STSD \n");
			DEBUG0("######### into case _3GP_ID_STSD ##########\n");
			DEBUG0("STSD filePos: 0x%x\n", filePos + bufSizeUsed);
#if 1
			result = (BLT_UInt16)vid3GPSTSDParse(pVidInfo, trakID, pbuf);
			if (result != BLT_SUCCESS) 
			{
				DEBUG0("STSD error!\n");
				return result;
			}
			isRefill = 0;
#endif
			break;
		case _3GP_ID_STSC:
			DEBUG0("_3GP_ID_STSC \n");
			DEBUG0("STSC filePos: 0x%x\n", filePos + bufSizeUsed);
#if 1

			result = (BLT_UInt16)vid3GPSTSCParse(filePos + bufSizeUsed, trakID, pbuf);
			if (result != BLT_SUCCESS)
			{
				DEBUG0("STSC error!\n");
				return result;
			}
#endif

			isRefill = 0;
			break;
		case _3GP_ID_STCO:
			DEBUG0("_3GP_ID_STCO \n");
			DEBUG0("STCO filePos: 0x%x\n", filePos + bufSizeUsed);
#if 1

			result = (BLT_UInt16)vid3GPSTCOParse(filePos + bufSizeUsed, trakID, pbuf);
			if (result != BLT_SUCCESS) 
			{
				DEBUG0("STCO error!\n");
				return result;
			}
			isRefill = 0;
#endif

			break;
		case _3GP_ID_STSZ:
#if 1
			DEBUG0("_3GP_ID_STSZ \n");
			DEBUG0("STSZ filePos: 0x%x\n", filePos + bufSizeUsed);
			result = (BLT_UInt16)vid3GPSTSZParse(filePos + bufSizeUsed, trakID, pbuf);
			if (result != BLT_SUCCESS) 
			{
				DEBUG0("STSZ error!\n");
				return result;
			}
			else if (trakID == VIDEO_TRAK)
			{
				_3gpSTSZInfo_t *stsz = NULL;
				stsz = &_3gpVidSTSZ;
				uiSampleCount = stsz->sample_count;
			}
			isRefill = 0;
#endif
			break;
		case _3GP_ID_STTS:
#if 1
			DEBUG0("_3GP_ID_STTS \n");
			DEBUG0("STTS filePos: 0x%x\n", filePos + bufSizeUsed);
			result = (BLT_UInt16)vid3GPSTTSParse(filePos + bufSizeUsed, trakID, pbuf);
			if (result != BLT_SUCCESS)
			{
				DEBUG0("STTS error!\n");
				return result;
			}
			isRefill = 0;
#endif
			break;
		case _3GP_ID_STSS:
#if 1
			DEBUG0("_3GP_ID_STSS \n");
			DEBUG0("STSS filePos: 0x%x\n", filePos + bufSizeUsed);
			vid3GPSTSSParse(filePos + bufSizeUsed, trakID, pbuf);
			isRefill = 0;
			if (trakID == VIDEO_TRAK)
			{
				_3gpSTSSInfo_t *stss = NULL;
				stss = &_3gpVidSTSS;
				uiICount = stss->entry_count;
			}
#endif
			break;
		default:
			break;
		}
		DEBUG0("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \n");
		#if 1
		if ( (isRefill == 1) || (bufSizeUsed + size + 128) > bufSize ) 
		{
			DEBUG0("######### (isRefill == 1) || (bufSizeUsed + size + 128) > bufSize ##########\n");
			DEBUG0("Refill=> filePos: 0x%x, bufSizeUsed: 0x%x, size: 0x%x\n", filePos, bufSizeUsed, size);
			filePos += bufSizeUsed + size;
			DEBUG0("###### filePos=%u\n ####\n", filePos);
			DEBUG0("###### guiFileSize=%u\n ####\n", guiFileSize);
			if ( filePos >= guiFileSize ) 
			{
				DEBUG0("###### break while loop ####\n");
				break;
			}
			
			vid3GPPosBounderGet(filePos, bufSize, &offset, &offsetLeft, &sizeRead);
			DEBUG0("###### offset=%u\n ####\n", offset);
			DEBUG0("###### sizeRead=%u\n ####\n", sizeRead);
			DEBUG0("###### offsetLeft=%u\n ####\n", offsetLeft);
			
			ifilled = ATX_InputStream_Seek(pParser->input.stream, offset);
			DEBUG0("###### ifilled=%u\n ####\n", ifilled);
			//if (ifilled < 0)
			if ((BLT_UInt32)ifilled < 0)
			{
				return BLT_FAILURE;
			}
			#if 1
			ifilled = ATX_InputStream_Read(pParser->input.stream, pbuf1, sizeRead, bytes_read);
			if (ifilled < 0)
			{
				return BLT_FAILURE;
			}
			DEBUG0("###### offsetLeft=%u\n ####\n", offsetLeft);
			pbuf = pbuf1 + offsetLeft;
			bufSizeUsed = 0;
			isRefill = 0;
			#endif
		}
		else 
		{
			DEBUG0("###### else ####\n");
			pbuf += size;
			bufSizeUsed += size;
		}	
		#endif
	} while ( (filePos + bufSizeUsed) < guiFileSize );
#if 1
	
	if ( (foundAudio == 0) && (foundVideo == 0) )
	{
	DEBUG0("######### (foundAudio == 0) && (foundVideo == 0) ##########\n");
		//DBG_CHECK(VID_PLAY_ERR01E);
		return VID_PLAY_ERR01E;
	}

	if (gbVidSTSSExist == 1)
	{
		DEBUG0("######### gbVidSTSSExist == 1 ##########\n");
		if (_3gpVidSTSS.entry_count <= 1) 
		{
			pVidInfo->bSeekable = 0;
		}
		else 
		{
			pVidInfo->bSeekable = 1;
		}
	}
	else 
	{
		DEBUG0("###### pVidInfo->bSeekable = 1 ####\n");
		pVidInfo->bSeekable = 1;
	}

	if (pVidInfo->vidType == VIDEO_TYPE_NONE && en3GPVType != VIDEO_3GP_TYPE_NONE)
	{
		BLT_UInt32	ui32PSize, ui32PTime, ui32PFType;
		BLT_UInt8  *pVData;
		BLT_UInt32 uiProfile;
		DEBUG0("######### pVidInfo->vidType == VIDEO_TYPE_NONE && en3GPV ##########\n");
		uiProfile = 0;
		pVData = NULL;
		DEBUG0("######### 1 ##########\n");
		result = vid3GPVidNextGet(pParser, &ui32PSize, &ui32PTime, &ui32PFType);
		DEBUG0("######### result = %d ##########\n", result);
		if (result != BLT_SUCCESS)
		{
			return result;
		}
		DEBUG0("######### 2 ##########\n");
		pVData = ATX_AllocateZeroMemory(ui32PSize);
		DEBUG0("######### (pVData == NULL) = %d ##########\n",pVData == NULL);
		if (pVData == NULL)
		{
			return BLT_FAILURE;
		}
		DEBUG0("######### 3 ##########\n");
		result = vid3GPVidDataFill(pParser, pVData);
		DEBUG0("######### result = %d ##########\n",result);
		if (result != BLT_SUCCESS)
		{
			return result;
		}
		
		if (en3GPVType == VIDEO_3GP_TYPE_MPEG4)
		{
			DEBUG0("######### en3GPVType == VIDEO_3GP_TYPE_MPEG4 ##########\n");
			uiProfile = ParserMPEG4Profile(pVData, ui32PSize);
			DEBUG0("uiProfile = %d\n", uiProfile);
			switch (uiProfile)
			{
			case 0:
				pVidInfo->vidType = VIDEO_TYPE_MPEG4_ASP;					
				break;
			case 1:
				pVidInfo->vidType = VIDEO_TYPE_MPEG4_SP;
				break; 
			case 2:
				pVidInfo->vidType = VIDEO_TYPE_MPEG4_ASP;
				break;
			default:
				pVidInfo->vidType = VIDEO_TYPE_NONE;					
				break;
			}			
		} 
		else if (en3GPVType == VIDEO_3GP_TYPE_H264)
		{
			DEBUG0("######### en3GPVType == VIDEO_3GP_TYPE_H264 ##########\n");
			uiProfile = ParserH264Profile(pVData, ui32PSize);
			DEBUG0("uiProfile = %d\n", uiProfile);
			switch (uiProfile)
			{
			case 0:
				pVidInfo->vidType = VIDEO_TYPE_NONE;					
				break;
			case 1:
				pVidInfo->vidType = VIDEO_TYPE_H264_BP;
				break; 
			case 2:
				pVidInfo->vidType = VIDEO_TYPE_H264_MP;
				break;
			case 3:
				pVidInfo->vidType = VIDEO_TYPE_H264_HP;
				break;
			default:
				pVidInfo->vidType = VIDEO_TYPE_NONE;					
				break;
			}
		}

		if (pVData != NULL)
		{
			DEBUG0("#### free pVData ####\n");
			ATX_FreeMemory(pVData);
		}

		vidVideoInfoReset();
	} 

	DEBUG0("###### here ####\n");
	if ( foundVideo == 1 ) 
	{
		DEBUG0("#### foundVideo == 1 ####\n");
		result = BLT_SUCCESS;
		if (vidDuration == 0 || uiSampleCount == 0)
		{
			pVidInfo->frmRate = 0;
		}
		/* ++ruilen@20070409_[mantis:8202] */		
		else  
		{
			pVidInfo->IFrameCount = uiICount;
			pVidInfo->TotalFrameCount = uiSampleCount;
			if ((uiICount != uiSampleCount) || (uiSampleCount > 20000))
			{
				ucMustSeekKeyFrame = 0xFF;
			}
			pVidInfo->frmRate = (uiSampleCount * 1000) / vidDuration;
			tmpPos = (uiSampleCount * 1000) % vidDuration;
			tmpPos = (tmpPos * FRAMERATE_MULTIPLE) / vidDuration;
			pVidInfo->frmRate = pVidInfo->frmRate * FRAMERATE_MULTIPLE + tmpPos;
		}
		guTotalFrame = pVidInfo->TotalFrameCount;
		guiVidTotalLen = pVidInfo->vidLen;
		guiVidFramerate = pVidInfo->frmRate;
		DEBUG0("###### pVidInfo->TotalFrameCount = %d ####\n", pVidInfo->TotalFrameCount);
		DEBUG0("###### pVidInfo->vidLen = %d ####\n", pVidInfo->vidLen);
		DEBUG0("###### pVidInfo->frmRate = %d ####\n", pVidInfo->frmRate);
		return result;
	}
#endif
	DEBUG0("###### return function ####\n");
	return BLT_SUCCESS;
}



/*----------------------------------------------------------------------
|   M4aParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParserInput_SetStream(BLT_InputStreamUser* _self,
                          ATX_InputStream*     stream,
                          const BLT_MediaType* media_type)
{
    M4aParser* self = ATX_SELF_M(input, M4aParser, BLT_InputStreamUser);
	BLT_Result	result;
	ContainBitsInfo_t *av_info;
	ContainBitsInfo_t st_av_info;
	BLT_StreamInfo 		stream_info;

	DEBUG0("######### into M4aParserInput_SetStream ##########\n");
	
	av_info = &st_av_info;
	
    /* check media type */
    if (media_type == NULL || media_type->id != self->input.media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
	
    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);
    
    /* keep a reference to the stream */
    self->input.stream = stream;
    ATX_REFERENCE_OBJECT(stream);

	ATX_SetMemory(av_info, 0, sizeof(ContainBitsInfo_t));
	/*prob es*/
	result = vid3GPHeaderParse(self, av_info);
	DEBUG0("######### result = %d #########\n", result);
	if(BLT_FAILED(result)){
		return result;
	}

	if(g_mp4_parse != NULL){
								DEBUG0("g_mp4_parse->vid = %u\n", g_mp4_parse->vid);
								DEBUG0("g_mp4_parse->aid = %u\n", g_mp4_parse->aid);
								DEBUG0("g_mp4_parse->file_size = %llu\n", g_mp4_parse->file_size);
								DEBUG0("g_mp4_parse->start_pos = %u\n", g_mp4_parse->start_pos);
								DEBUG0("g_mp4_parse->end_time = %u\n", g_mp4_parse->end_time);
								DEBUG0("g_mp4_parse->base_time = %u\n", g_mp4_parse->base_time);
								DEBUG0("g_mp4_parse->base_time_per = %u\n", g_mp4_parse->base_time_per);
								DEBUG0("g_mp4_parse->time_len= %u\n", g_mp4_parse->time_len);
								DEBUG0("g_mp4_parse->seek_step = %u\n", g_mp4_parse->seek_step);
					
					
								DEBUG0("g_mp4_parse->file_end = %u\n", g_mp4_parse->file_end);
								DEBUG0("g_mp4_parse->file_pos = %llu\n", g_mp4_parse->file_pos);
								DEBUG0("g_mp4_parse->cur_time = %u\n", g_mp4_parse->cur_time);
								DEBUG0("g_mp4_parse->start_time= %u\n", g_mp4_parse->start_time);
								DEBUG0("g_mp4_parse->sync_req = %u\n", g_mp4_parse->sync_req);
					
					
								DEBUG0(" = %u\n", g_mp4_parse->demux == NULL);
								DEBUG0(" = %u\n", g_mp4_parse->vi == NULL);
								DEBUG0(" = %u\n", g_mp4_parse->ai == NULL);
								DEBUG0("= %u\n", g_mp4_parse->v_esp == NULL);
								DEBUG0(" = %u\n", g_mp4_parse->a_esp == NULL);
					
					
								DEBUG0("g_mp4_parse->v_frame.pts = %u\n", g_mp4_parse->v_frame.pts);
								DEBUG0("g_mp4_parse->v_frame.size = %u\n", g_mp4_parse->v_frame.size);
								DEBUG0("g_mp4_parse->v_frame.type = %u\n", g_mp4_parse->v_frame.type);
								DEBUG0("g_mp4_parse->aframe.pts = %u\n", g_mp4_parse->a_frame.pts);
								DEBUG0("g_mp4_parse->aframe.size = %u\n", g_mp4_parse->a_frame.size);
								DEBUG0("g_mp4_parse->aframe.type = %u\n", g_mp4_parse->a_frame.type);
								 
							
							}
					
	/*print*/
	DEBUG0("######################## start printf av_info #########################");
	#if 1
	DEBUG0("sav_info->audBitrate = %x\n", av_info->audBitrate);			
	DEBUG0("av_info->audBits = %x\n", av_info->audBits);			
	DEBUG0("av_info->audChannel = %x\n",av_info->audChannel);			
	DEBUG0("av_info->audExtraLen = %x\n", av_info->audExtraLen);	
	DEBUG0("av_info->audLen = %x\n",av_info->audLen);			
	DEBUG0("av_info->audSR = %x\n", av_info->audSR);
	DEBUG0("av_info->audStreamNum = %x\n",av_info->audStreamNum);			
	DEBUG0("av_info->audType = %x\n", av_info->audType);
	DEBUG0("av_info->bSeekable = %x\n",av_info->bSeekable);			
	DEBUG0("av_info->clusterSize = %x\n", av_info->clusterSize);
	DEBUG0("av_info->dpEn = %x\n",av_info->dpEn);			
	DEBUG0("av_info->enSubTitleSup = %x\n", av_info->enSubTitleSup);
	DEBUG0("av_info->frmRate = %x\n",av_info->frmRate);			
	DEBUG0("av_info->gobEn = %x\n", av_info->gobEn);
	DEBUG0("av_info->gopNo = %x\n",av_info->gopNo);			
	DEBUG0("av_info->height = %x\n", av_info->height);
	DEBUG0("av_info->IFrameCount = %x\n",av_info->IFrameCount);			
	DEBUG0("av_info->isVBR = %x\n", av_info->isVBR);
	DEBUG0("av_info->reserved = %x\n",av_info->reserved);			
	DEBUG0("av_info->rotate = %x\n", av_info->rotate);
	
	DEBUG0("av_info->rvlcEn = %x\n", av_info->rvlcEn);
	DEBUG0("av_info->targetDisk = %x\n",av_info->targetDisk); 		
	DEBUG0("av_info-->TotalFrameCount = %x\n", av_info->TotalFrameCount);
	DEBUG0("av_info->uiBlockAlign = %x\n",av_info->uiBlockAlign); 		
	DEBUG0("av_info->vidBitrate = %x\n", av_info->vidBitrate);
	DEBUG0("av_info->vidExtraLen = %x\n",av_info->vidExtraLen); 		
	DEBUG0("av_info->vidLen = %x\n", av_info->vidLen);
	DEBUG0("av_info->vidStreamNum = %x\n",av_info->vidStreamNum);			
	DEBUG0("av_info->vidType = %x\n", av_info->vidType);
	DEBUG0("av_info->vInterFlag = %x\n", av_info->vInterFlag);
	DEBUG0("av_info->vopTimeIncLen = %x\n",av_info->vopTimeIncLen);			
	DEBUG0("av_info->width = %x\n", av_info->width);

	DEBUG0("######################## end  printf av_info #########################");

	#endif
	#if 1
	
	
	if (av_info->audType != AUDIO_TYPE_NONE && av_info->audSR){
	self->output.media_type->m4a_audio_info.wFormatTag = (short)av_info->audType;
	self->output.media_type->m4a_audio_info.nChannels = av_info->audChannel;
	self->output.media_type->m4a_audio_info.nSamplesPerSec = av_info->audSR;
	self->output.media_type->m4a_audio_info.wBitsPerSample = av_info->audBits;
	self->output.media_type->m4a_audio_info.nBlockAlign = av_info->uiBlockAlign;
	self->output.media_type->extra_data_size = av_info->audExtraLen;
	self->output.media_type->extra_data = (BLT_UInt8*)ATX_AllocateZeroMemory(av_info->audExtraLen);
	if(NULL == self->output.media_type->extra_data){
		return BLT_ERROR_OUT_OF_MEMORY; 
	}
	ATX_CopyMemory(self->output.media_type->extra_data, av_info->pAudExtraData, av_info->audExtraLen);
	//self->output.media_type->extra_data = (BLT_UInt8 *)av_info->pAudExtraData;


#if 0

	

		
		if(CODEC_ID_WMAV1 == av_info->audType ){				
			stream_info.data_type = "WMAV1";							
		}if(CODEC_ID_WMAV2 == av_info->audType){				
			stream_info.data_type = "WMAV2";			
		}else{				
			stream_info.data_type = "UNKOWN";			
		}
#endif
		stream_info.mask = 0;			
		stream_info.sample_rate = av_info->audSR;			
		stream_info.channel_count	= av_info->audChannel;			
		stream_info.duration		= av_info->audLen;

		stream_info.mask |=	BLT_STREAM_INFO_MASK_SAMPLE_RATE   |  
							BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
							BLT_STREAM_INFO_MASK_DURATION;			 
	if (stream_info.mask && ATX_BASE(self, BLT_BaseMediaNode).context) {  
			BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context,&stream_info);   			 
		}	
	
		M4aParser_UpdateMediaType(self);	

	}
	#endif
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   M4aParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParserInput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    M4aParserInput* self = ATX_SELF(M4aParserInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(M4aParserInput)
    ATX_GET_INTERFACE_ACCEPT(M4aParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(M4aParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(M4aParserInput, BLT_InputStreamUser)
    M4aParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(M4aParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(M4aParserInput, BLT_MediaPort)
    M4aParserInput_GetName,
    M4aParserInput_GetProtocol,
    M4aParserInput_GetDirection,
    M4aParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   M4aParser_MakeEosPacket
+---------------------------------------------------------------------*/
static BLT_Result
M4aParser_MakeEosPacket(M4aParser* self, BLT_MediaPacket** packet)
{
    BLT_Result result;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        0,
                                        (BLT_MediaType*)self->output.media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;

    BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   M4aParser_UpdateMediaType
+---------------------------------------------------------------------*/
void
M4aParser_UpdateMediaType(M4aParser* self)
{
   
}

/*----------------------------------------------------------------------
|   M4aParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    M4aParserOutput* self = ATX_SELF(M4aParserOutput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (BLT_MediaType*)self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}


/*----------------------------------------------------------------------
|   WmaParser_MakeEosPacket+---------------------------------------------
------------------------*/
static BLT_Result WmaParser_MakeEosPacket(M4aParser* self, 
		BLT_MediaPacket** packet)
{	
	BLT_Result result;    
	result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
										0,
										(BLT_MediaType*)self->output.media_type,
										packet);    
	if (BLT_FAILED(result)) return result;   
	BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);    

	return BLT_SUCCESS;
}



static BLT_UInt32
vid3GPAudNextGet(
	M4aParser*	pParser,
	BLT_UInt32* pSize,
	BLT_UInt32* pTime
)
{
	BLT_UInt32 ret;
	BLT_UInt32 offset;
	BLT_UInt32 time;
	DEBUG0("######### into vid3GPAudNextGet ##########\n");

	if (gAudType == AUDIO_TYPE_NONE)
	{
		*pTime = 0;
		*pSize = 0;
		return BLT_FAILURE;
	}

	ret = vid3GPIdxInfoGet(pParser,AUDIO_TRAK, &offset, pSize, &time);
	
	DEBUG0("######### ret = %d ##########\n", ret);
	if (ret == BLT_SUCCESS)
	{
		audNextOffset = offset;
		audNextSize = *pSize;
		*pTime = (BLT_UInt32)(gAudSampleTotalTime * (BLT_UInt64)1000 / (BLT_UInt64)audTimeScale);
		gAudSampleTotalTime += (BLT_UInt64)time;		
		gAudTotalFrameNum ++;

#if 1  //ndef USE_LIBAVCODEC
		if ((gAudType == AUDIO_TYPE_AAC) || (gAudType == AUDIO_TYPE_AAC_PLUS))
		{
			*pSize += 7;
			DEBUG0("##### *pSize = %u #####\n", *pSize);
		}
#endif
		DEBUG0("##### audNextOffset = %u #####\n", audNextOffset);
		DEBUG0("##### audNextSize = %u #####\n", audNextSize);
		DEBUG0("##### *pTime = %d #####\n", *pTime);
		DEBUG0("##### gAudSampleTotalTime = %llu #####\n", gAudSampleTotalTime);
		DEBUG0("##### gAudTotalFrameNum = %u #####\n", gAudTotalFrameNum);

		DEBUG0("[vid3GPAudNextGet]size:%d time:%d\n", *pSize, *pTime);
	}
	DEBUG0("audio time stamp: %d \n",*pTime);
	DEBUG0("##### ret = %d #####\n", ret);
	return ret;
}


static BLT_UInt32 MakeADTSHeader(
		BLT_UInt8 *ptr, 
		BLT_UInt32 len,
		BLT_UInt32 channel, 
		BLT_UInt32 samprate) 
{ 
    BLT_Int32 fs_ind, sr, ch; 
	audio_info_t *ai;
	media_demux_t *demux;

	DEBUG0("######    into  MakeADTSHeader #######\n");
	
	ai = g_mp4_parse->ai;

	sr = samprate;
	ch = channel;

	DEBUG0("###### sr = %u #######\n",  sr);
	DEBUG0("###### ch = %u #######\n",  ch);

	
    if(sr>=96000) fs_ind = 0; 
    else if (sr>=88200) fs_ind = 1; 
    else if (sr>=64000) fs_ind = 2; 
    else if (sr>=48000) fs_ind = 3; 
    else if (sr>=44100) fs_ind = 4; 
    else if (sr>=32000) fs_ind = 5; 
    else if (sr>=24000) fs_ind = 6; 
    else if (sr>=22000) fs_ind = 7; 
    else if (sr>=16000) fs_ind = 8; 
    else if (sr>=12000) fs_ind = 9; 
    else if (sr>=11025) fs_ind = 10; 
    else if (sr>=8000)  fs_ind = 11; 
    else fs_ind = 15; // UNKNOWN 
    
    ptr[0] = 0xFF; 
    ptr[1] = 0xF1; 
    ptr[2] = 0x40 | (fs_ind << 2) | (ch >> 2); 
    ptr[3] = (char)((ch << 6) | ((len >> 11) & 0x3));
    ptr[4] = (char)((len >> 3) & 0xFF);
    ptr[5] = (char)((len << 5) & 0xE0 |0x1F);
    ptr[6] = (char)(0xFC);

	DEBUG0("ptr[0] = %02x\n", ptr[0]);
	DEBUG0("ptr[1] = %02x\n", ptr[1]);
	DEBUG0("ptr[2] = %02x\n", ptr[2]);
	DEBUG0("ptr[3] = %02x\n", ptr[3]);
	DEBUG0("ptr[4] = %02x\n", ptr[4]);
	DEBUG0("ptr[5] = %02x\n", ptr[5]);
	DEBUG0("ptr[6] = %02x\n", ptr[6]);
	DEBUG0("######    exit MakeADTSHeader #######\n");
	
	return 7;
} 


static BLT_UInt32
vid3GPAudDataFill(
	M4aParser*	pParser,
	BLT_UInt8* pBuf,
	BLT_UInt32	channel,
	BLT_UInt32	samprate
	
)
{
	BLT_UInt32 ret;
	BLT_UInt8* pABuf;
	
	DEBUG0("###### into  vid3GPAudDataFill #######\n");
	DEBUG0("###### gAudType = %d #######\n", gAudType);
	DEBUG0("###### audNextSize = %d #######\n", audNextSize);

	pABuf = pBuf;	
#if 1  //ndef USE_LIBAVCODEC
	if ((gAudType == AUDIO_TYPE_AAC) || (gAudType == AUDIO_TYPE_AAC_PLUS))
	{
		DEBUG0("#### audNextSize = %d #####", audNextSize);
		DEBUG0("###### here#######\n");	
		typedef struct mp4_parse_s
		{
			BLT_UInt32 vid;
			BLT_UInt32 aid;
			BLT_Int32 fd, seek_fd;
			BLT_UInt64 file_size;
			BLT_UInt32 start_pos;
			BLT_UInt32 end_time;
			BLT_UInt32 base_time;
			BLT_UInt32 base_time_per;
			BLT_UInt32 time_len;
			BLT_UInt32 seek_step;
			
			BLT_Int32 file_end;
			BLT_UInt64 file_pos;
			BLT_UInt32 cur_time;
			BLT_UInt32 start_time;
			BLT_UInt32 sync_req;
			BLT_UInt32 first_pkt;
			
			media_demux_t *demux;
			video_info_t *vi;
			audio_info_t *ai;
			es_packet_t *v_esp;
			es_packet_t *a_esp;
			
			mp4_frame_t v_frame;
			mp4_frame_t a_frame;
		}mp4_parse_t;


#if 0
		
		if(g_mp4_parse != NULL){
							DEBUG0("g_mp4_parse->vid = %u\n", g_mp4_parse->vid);
							DEBUG0("g_mp4_parse->aid = %u\n", g_mp4_parse->aid);
							DEBUG0("g_mp4_parse->file_size = %u\n", g_mp4_parse->file_size);
							DEBUG0("g_mp4_parse->start_pos = %u\n", g_mp4_parse->start_pos);
							DEBUG0("g_mp4_parse->end_time = %u\n", g_mp4_parse->end_time);
							DEBUG0("g_mp4_parse->base_time = %u\n", g_mp4_parse->base_time);
							DEBUG0("g_mp4_parse->base_time_per = %u\n", g_mp4_parse->base_time_per);
							DEBUG0("g_mp4_parse->time_len= %u\n", g_mp4_parse->time_len);
							DEBUG0("g_mp4_parse->seek_step = %u\n", g_mp4_parse->seek_step);
				
				
							DEBUG0("g_mp4_parse->file_end = %u\n", g_mp4_parse->file_end);
							DEBUG0("g_mp4_parse->file_pos = %u\n", g_mp4_parse->file_pos);
							DEBUG0("g_mp4_parse->cur_time = %u\n", g_mp4_parse->cur_time);
							DEBUG0("g_mp4_parse->start_time= %u\n", g_mp4_parse->start_time);
							DEBUG0("g_mp4_parse->sync_req = %u\n", g_mp4_parse->sync_req);
				
				
							DEBUG0(" = %u\n", g_mp4_parse->demux == NULL);
							DEBUG0(" = %u\n", g_mp4_parse->vi == NULL);
							DEBUG0(" = %u\n", g_mp4_parse->ai == NULL);
							DEBUG0("= %u\n", g_mp4_parse->v_esp == NULL);
							DEBUG0(" = %u\n", g_mp4_parse->a_esp == NULL);
				
				
							DEBUG0("g_mp4_parse->v_frame.pts = %u\n", g_mp4_parse->v_frame.pts);
							DEBUG0("g_mp4_parse->v_frame.size = %u\n", g_mp4_parse->v_frame.size);
							DEBUG0("g_mp4_parse->v_frame.type = %u\n", g_mp4_parse->v_frame.type);
							DEBUG0("g_mp4_parse->aframe.pts = %u\n", g_mp4_parse->a_frame.pts);
							DEBUG0("g_mp4_parse->aframe.size = %u\n", g_mp4_parse->a_frame.size);
							DEBUG0("g_mp4_parse->aframe.type = %u\n", g_mp4_parse->a_frame.type);
							 
						
						}
	#endif			
		
		pABuf += MakeADTSHeader(pABuf, audNextSize + 7, channel, samprate);
		
	}
#endif
	ret = vid3GPDataFill(pParser, pABuf, audNextOffset, audNextSize, &_3gpBufInfo[1]);
	if(ret == BLT_FAILURE)
	{
		DEBUG0("vid3GPAudDataFill - fail\n");
	}
	return ret;
}



/*----------------------------------------------------------------------
|   M4aParserOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParserOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    M4aParser* 	self   = ATX_SELF_M(output, M4aParser, BLT_PacketProducer);
    BLT_Result  result = BLT_SUCCESS;
	BLT_UInt8*  tempbuf = NULL;
	BLT_UInt8	buf[20 * 1024];
	BLT_UInt32 	size = 0;  
	BLT_UInt32 	pts;
	BLT_UInt32 	channel;  
	BLT_UInt32 	samprate;
	BLT_UInt32 	total_size = 0;
	
	DEBUG0("######### into M4aParserOutput_GetPacket ##########\n");

	channel = self->output.media_type->m4a_audio_info.nChannels;
	samprate = self->output.media_type->m4a_audio_info.nSamplesPerSec;

	*packet = NULL;

	do{
		result = vid3GPAudNextGet(self, &size, &pts);	

		if (BLT_FAILED(result)) {		
				DEBUG0("**** result = %d****\n", result);		
				DEBUG0("**** M4aAudNextGet fail ****\n");		
				if(result != VID_PLAY_ERR015 ){			
						return BLT_ERROR_PORT_HAS_NO_DATA;		
				}else{
						if(total_size > 0){
							DEBUG0("&&&& zui hou yige packet &&&&&\n");	
							goto pack_packet;
						} else {
							DEBUG0("&&&& at end of the file &&&&&\n");	
							self->input.eos = BLT_TRUE;			
							return M4aParser_MakeEosPacket(self, packet);	
						}		
				}
		}
		DEBUG0("############ total_size = %d\n##########", total_size);
		tempbuf = (BLT_UInt8*)ATX_AllocateZeroMemory(size);    
		if (tempbuf == NULL) {		
			DEBUG0("**** alloc tmpbuf fail ****\n");        
			return BLT_ERROR_OUT_OF_MEMORY;    
		}

		ATX_SetMemory(tempbuf, 0, size);	

		result = vid3GPAudDataFill(self, tempbuf, channel, samprate);	
		if (BLT_FAILED(result)) {		
			DEBUG0("**** M4aAudDataFill fail ****\n");		
			return BLT_FAILURE;	
		}
		
		ATX_CopyMemory(buf + total_size, tempbuf, size);
		total_size += size;
		ATX_FreeMemory(tempbuf);
		tempbuf = NULL;
		
	}while(total_size < 4 * 1024);
	


pack_packet:
	result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
											total_size,
											(BLT_MediaType*)self->output.media_type,
											packet);	
		if (BLT_FAILED(result)) {		
			DEBUG0("**** create packet fail ****\n");		
			return result;	
		}	   
		ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(*packet),
						buf,
						total_size);	   
		BLT_MediaPacket_SetPayloadSize(*packet, total_size);

		if(tempbuf != NULL){
			ATX_FreeMemory(tempbuf);
			tempbuf = NULL;
		}
		
		return BLT_SUCCESS;

}
/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(M4aParserOutput)
    ATX_GET_INTERFACE_ACCEPT(M4aParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(M4aParserOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(M4aParserOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(M4aParserOutput, BLT_MediaPort)
    M4aParserOutput_GetName,
    M4aParserOutput_GetProtocol,
    M4aParserOutput_GetDirection,
    M4aParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(M4aParserOutput, BLT_PacketProducer)
    M4aParserOutput_GetPacket
ATX_END_INTERFACE_MAP



BLT_Int32  mp4_mem_size(void)
{
	return vid3GPMemSizeGet(0) + sizeof (mp4_parse_t) + 1024;
}



static void
vid3GPInit(
	BLT_UInt32	addr
)
{
	BLT_UInt8* pFreeAddr;
	bufInfo_t* pBufInfo;
	BLT_UInt32 uiBufSize;

	DEBUG0("############# into vid3GPInit ####################\n");
	pFreeAddr = (BLT_UInt8*)addr;
	/*pFreeAddr = (UINT8*)(addr | 0x3C000000);*/

	/* We temporary use the data buffer for header parse */
	p3GPPktBufStart = pFreeAddr;
	//DEBUG0("************ vid3GPInit ***********\n");	

	/* Initial index buffer */
	uiBufSize = IDX_VID_STTS_BUF_SIZE;
	p3GPIdxBufStart[IDX_VID_STTS] = pFreeAddr;
	p3GPIdxBufEnd[IDX_VID_STTS] = p3GPIdxBufStart[IDX_VID_STTS] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_VID_STTS] = p3GPIdxBufStart[IDX_VID_STTS];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_VID_STSZ_BUF_SIZE;
	p3GPIdxBufStart[IDX_VID_STSZ] = pFreeAddr;
	p3GPIdxBufEnd[IDX_VID_STSZ] = p3GPIdxBufStart[IDX_VID_STSZ] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_VID_STSZ] = p3GPIdxBufStart[IDX_VID_STSZ];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_VID_STCO_BUF_SIZE;
	p3GPIdxBufStart[IDX_VID_STCO] = pFreeAddr;
	p3GPIdxBufEnd[IDX_VID_STCO] = p3GPIdxBufStart[IDX_VID_STCO] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_VID_STCO] = p3GPIdxBufStart[IDX_VID_STCO];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_VID_STSS_BUF_SIZE;
	p3GPIdxBufStart[IDX_VID_STSS] = pFreeAddr;
	p3GPIdxBufEnd[IDX_VID_STSS] = p3GPIdxBufStart[IDX_VID_STSS] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_VID_STSS] = p3GPIdxBufStart[IDX_VID_STSS];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_VID_STSC_BUF_SIZE;
	p3GPIdxBufStart[IDX_VID_STSC] = pFreeAddr;
	p3GPIdxBufEnd[IDX_VID_STSC] = p3GPIdxBufStart[IDX_VID_STSC] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_VID_STSC] = p3GPIdxBufStart[IDX_VID_STSC];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_AUD_STSZ_BUF_SIZE;
	p3GPIdxBufStart[IDX_AUD_STSZ] = pFreeAddr;
	p3GPIdxBufEnd[IDX_AUD_STSZ] = p3GPIdxBufStart[IDX_AUD_STSZ] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_AUD_STSZ] = p3GPIdxBufStart[IDX_AUD_STSZ];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_AUD_STCO_BUF_SIZE;
	p3GPIdxBufStart[IDX_AUD_STCO] = pFreeAddr;
	p3GPIdxBufEnd[IDX_AUD_STCO] = p3GPIdxBufStart[IDX_AUD_STCO] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_AUD_STCO] = p3GPIdxBufStart[IDX_AUD_STCO];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_AUD_STSC_BUF_SIZE;
	p3GPIdxBufStart[IDX_AUD_STSC] = pFreeAddr;
	p3GPIdxBufEnd[IDX_AUD_STSC] = p3GPIdxBufStart[IDX_AUD_STSC] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_AUD_STSC] = p3GPIdxBufStart[IDX_AUD_STSC];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	uiBufSize = IDX_AUD_STTS_BUF_SIZE;
	p3GPIdxBufStart[IDX_AUD_STTS] = pFreeAddr;
	p3GPIdxBufEnd[IDX_AUD_STTS] = p3GPIdxBufStart[IDX_AUD_STTS] + uiBufSize + ADDR_ALIGN_SIZE;
	p3GPIdxBufCurr[IDX_AUD_STTS] = p3GPIdxBufStart[IDX_AUD_STTS];
	pFreeAddr += (uiBufSize + ADDR_ALIGN_SIZE);

	/* Initial video data buffer */
	pBufInfo = &_3gpBufInfo[0];
	pBufInfo->pbufStart = pFreeAddr;
	pBufInfo->pbufEnd = pBufInfo->pbufStart + VID_DATA_BUF_SIZE;
	pBufInfo->bufSize = VID_DATA_BUF_SIZE;
	pBufInfo->offsetInFileStart = 0;
	pBufInfo->offsetInFileEnd = 0;
	pFreeAddr += VID_DATA_BUF_SIZE;

	/* Initial audio data buffer */
	pBufInfo = &_3gpBufInfo[1];
	pBufInfo->pbufStart = pFreeAddr;
	pBufInfo->pbufEnd = pBufInfo->pbufStart + AUD_DATA_BUF_SIZE;
	pBufInfo->bufSize = AUD_DATA_BUF_SIZE;
	pBufInfo->offsetInFileStart = 0;
	pBufInfo->offsetInFileEnd = 0;
	pFreeAddr += AUD_DATA_BUF_SIZE;

	en3GPVType = VIDEO_3GP_TYPE_NONE;
	guiFileSize = 0;
	pvidHeaderData = 0;
	gVidSampleTotalTime = 0;
    gFirstFillVidData = 0;
	gVidFrameNumber = 0;
	gVidKeyFrame = 0;
	gbVidSTSSExist = 0;
	g3gpVidLastTime = 0;
	/*++snail@20061212_[mantis:6177] for speed playback too long time audio time out*/
	gAudSampleTotalTime = (BLT_UInt64)0;
	gAudTotalFrameNum = 0;
	/*--snail@20061212_[mantis:6177] for speed playback too long time audio time out*/
	ucMustSeekKeyFrame = 0;

	DEBUG0("#############return vid3GPInit ####################\n");
}


BLT_Int32 mp4_init(BLT_UInt8 *buf)
{
	BLT_UInt32 size;
	BLT_UInt8 *pDatabuf;
	BLT_UInt32 i, addr;
	BLT_UInt32	check_value = 0;
	if ( !buf || g_mp4_parse)
	{
		//DEBUG0("-%s:%d\n", __FUNCTION__, __LINE__);
		return 0;
	}
	DEBUG0(" ###########into mp4_init ##########\n", addr);
	g_mp4_parse = (mp4_parse_t *)((((BLT_UInt32)buf) + 3) & ~3);
	DEBUG0(" ###########1 ##########\n");
	ATX_SetMemory(g_mp4_parse, 0, sizeof(mp4_parse_t));
	DEBUG0(" ###########2 ##########\n");
	g_mp4_parse->aid = ~0;
	g_mp4_parse->vid = ~0;
	g_mp4_parse->fd = ~0;
	g_mp4_parse->seek_fd = ~0;
	
	addr = ((BLT_UInt32)g_mp4_parse) + sizeof(mp4_parse_t);
	DEBUG0(" ###########addr = %u ##########\n", addr);
	vid3GPInit(addr);
	
	//DEBUG0("-%s:%d\n", __FUNCTION__, __LINE__);
	return 1;
}



/*----------------------------------------------------------------------
|   M4aParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
M4aParser_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    M4aParser* 	self;
	BLT_Int32 	size;
	BLT_UInt8*	addr;
	DEBUG0(" ###########M4aParser_Create##########\n");
	DEBUG0("vid3GPAudDataFill = %08x \n", (int) vid3GPAudDataFill);

    
	/* check parameters */
	   if (parameters == NULL || 
		   parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
		   return BLT_ERROR_INVALID_PARAMETERS;
	   }
	
	   /* allocate memory for the object */
	   self = ATX_AllocateZeroMemory(sizeof(M4aParser));
	   if (self == NULL) {
		   *object = NULL;
		   return BLT_ERROR_OUT_OF_MEMORY;
	   }
	
	   /* construct the object */
	   /* construct the object */
	   BLT_MediaType_Init(&self->input.media_type,
						  ((M4aParserModule*)module)->m4a_type_id);
	   self->input.stream = NULL;
	   BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);
	   self->output.media_type = (BLT_M4aAudioMediaType*)ATX_AllocateZeroMemory(sizeof(BLT_M4aAudioMediaType)+1);
	   BLT_MediaType_InitEx(&self->output.media_type->base, ((M4aParserModule*)module)->m4a_type_id, sizeof(BLT_M4aAudioMediaType)+1);
	   
	   /* setup interfaces */
	   ATX_SET_INTERFACE_EX(self, M4aParser, BLT_BaseMediaNode, BLT_MediaNode);
	   ATX_SET_INTERFACE_EX(self, M4aParser, BLT_BaseMediaNode, ATX_Referenceable);
	   ATX_SET_INTERFACE(&self->input,	M4aParserInput,  BLT_MediaPort);
	   ATX_SET_INTERFACE(&self->input,	M4aParserInput,  BLT_InputStreamUser);
	   ATX_SET_INTERFACE(&self->output, M4aParserOutput, BLT_MediaPort);
	   ATX_SET_INTERFACE(&self->output, M4aParserOutput, BLT_PacketProducer);
	   *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

		size = mp4_mem_size();

		addr = (BLT_UInt8*)ATX_AllocateMemory(size);
		if(addr == NULL){
			return BLT_ERROR_OUT_OF_MEMORY;
		}
		DEBUG0(" ########### add = %d ##########\n", addr);
		mp4_init(addr);

		DEBUG0(" ########### return M4aParser_Create##########\n");

    return BLT_SUCCESS;
}

static BLT_UInt32
vid3GPParserFinalize(void)
{
	if (pAudExtraData != NULL)
	{
		ATX_FreeMemory(pAudExtraData);
		pAudExtraData = NULL;
	}
	if (pvidH264Data != NULL)
	{
		ATX_FreeMemory(pvidH264Data);
		pvidH264Data = NULL;
	}
	return BLT_SUCCESS;
}


static BLT_Int32 mp4_uninit(void)
{
	if (g_mp4_parse)
	{
		g_mp4_parse = NULL;
	}

	if (vid3GPParserFinalize() == BLT_SUCCESS)
	{
		return BLT_SUCCESS;
	}
	else
	{
		return BLT_FAILURE;
	}
}


/*----------------------------------------------------------------------
|   M4aParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
M4aParser_Destroy(M4aParser* self)
{
    ATX_LOG_FINE("M4aParser::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* free the media type extensions */
    BLT_MediaType_Free((BLT_MediaType*)self->output.media_type);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);
	
	if(NULL != self->output.media_type->extra_data){
		ATX_FreeMemory(self->output.media_type->extra_data);
	}

	mp4_uninit();

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    M4aParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParser_Deactivate(BLT_MediaNode* _self)
{
    M4aParser* self = ATX_SELF_EX(M4aParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("M4aParser::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   M4aParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    M4aParser* self = ATX_SELF_EX(M4aParser, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(&self->input, BLT_MediaPort);
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "output")) {
        *port = &ATX_BASE(&self->output, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}
static void
vidAudioInfoReset(
	void
)
{
	bufInfo_t *pbufInfo;

	/* STSS */
	_3gpAudSTSS.filePos = _3gpAudSTSS.filePos_start;
	_3gpAudSTSS.entry_idx = 0;
	_3gpAudSTSS.entry_fill = 0;

	/* STSZ */
	_3gpAudSTSZ.filePos = _3gpAudSTSZ.filePos_start;
	_3gpAudSTSZ.sample_idx = 0;
	_3gpAudSTSZ.sample_fill = 0;

	/* STCO */
	_3gpAudSTCO.filePos = _3gpAudSTCO.filePos_start;
	_3gpAudSTCO.entry_idx = 0;
	_3gpAudSTCO.entry_fill = 0;

	/* STSC */
	_3gpAudSTSC.filePos = _3gpAudSTSC.filePos_start;
	_3gpAudSTSC.entry_idx = 0;
	_3gpAudSTSC.entry_fill = 0;
	_3gpAudSTSC.sample_count = 0;
	_3gpAudSTSC.sample_idx = 0;
	_3gpAudSTSC.isLastChunk = 0;
	_3gpAudSTSC.next_chunk_id = 0;
	_3gpAudSTSC.curr_chunk_id = 0;
	_3gpAudSTSC.isFirstFilled = 0;

	/* STTS */
	_3gpAudSTTS.filePos = _3gpAudSTTS.filePos_start;
	_3gpAudSTTS.entry_fill = 0;
	_3gpAudSTTS.entry_idx = 0;
	_3gpAudSTTS.sample_count = 0;
	_3gpAudSTTS.sample_idx = 0;

	pbufInfo = &_3gpBufInfo[1];
	pbufInfo->offsetInFileStart = 0;
	pbufInfo->offsetInFileEnd = 0;
}


static BLT_UInt32
vid3GPAudFrmTimeGet(void)
{
	BLT_UInt32 ret;

	switch(gAudType)
	{
		case AUDIO_TYPE_AMR_NB:
			/* AMR 1 frame 160 samples, Sample Rate 8K, so 20ms */
			ret = 20;
			break;
		case AUDIO_TYPE_AAC:
			/* AAC 1 frame 1024 samples */
			ret = (1024 * 1000) / gAudSR;
			break;

		default:
			ret = 0;
	}
	return ret;
}


static BLT_UInt32
vid3GPAudSeek(
	M4aParser* pParser,
	BLT_UInt32 ms
)
{
	BLT_UInt32 frmTime;
	BLT_UInt32 audSampleOffset, audSampleSize, audSampleTime;
	BLT_UInt32 audTmpTime;
	BLT_UInt64 audTotalTime;
	BLT_UInt32 audCurrFrameNum;
	BLT_UInt32 ret = BLT_SUCCESS;

	vidAudioInfoReset();
	frmTime = vid3GPAudFrmTimeGet();

	/* TODO: Refine the frame limitation */
	/* The duration of the target frame is included in tmpTime */
	audTmpTime = 0;
	audCurrFrameNum = 0;
	audTotalTime = (BLT_UInt64) 0;
	while (audTmpTime < ms) 
	{
		ret = vid3GPIdxInfoGet(pParser, AUDIO_TRAK, &audSampleOffset, &audSampleSize, &audSampleTime);
		if ( ret != BLT_SUCCESS )
		{
			return ret;
		}
		audTotalTime += (BLT_UInt64) audSampleTime;
		audTmpTime = (BLT_UInt32)(audTotalTime * (BLT_UInt64)1000 / (BLT_UInt64)audTimeScale);
		audCurrFrameNum++;
		if ((audTmpTime + frmTime) > ms) 
		{
			if ((ms - audTmpTime) < (frmTime >> 1)) 
			{
				break;
			}
		}

		if ( audCurrFrameNum >= (_3gpAudSTSZ.sample_count - 2) ) 
		{
			break;
		}
	}
	gAudSampleTotalTime = audTotalTime;
	gAudTotalFrameNum = audCurrFrameNum;
	/*++snail@20061212_[mantis:6177] for speed playback too long time audio time out*/
	return ret;
}




/*----------------------------------------------------------------------
|   M4aParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParser_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    M4aParser* self = ATX_SELF_EX(M4aParser, BLT_BaseMediaNode, BLT_MediaNode);
 	BLT_Result  result;
	BLT_UInt32  ms;
 
	
	 /* seek to the estimated offset */
	 /* seek into the input stream (ignore return value) */
	 DEBUG0("point->time_stamp.seconds = %d\n", point->time_stamp.seconds);
	 DEBUG0("point->time_stamp.nanoseconds = %d\n", point->time_stamp.nanoseconds);
	 ms = point->time_stamp.seconds * 1000 + point->time_stamp.nanoseconds /1000000;
	 DEBUG0("ms = %d\n", ms);
	 
	 result = vid3GPAudSeek(self, ms);;
	 if(BLT_FAILED(result)){
		 return BLT_FAILURE;
	 }
	 
	 /* set the mode so that the nodes down the chain know the seek has */
	 /* already been done on the stream 								 */
	 *mode = BLT_SEEK_MODE_IGNORE;

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(M4aParser)
    ATX_GET_INTERFACE_ACCEPT_EX(M4aParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(M4aParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(M4aParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    M4aParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    M4aParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    M4aParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(M4aParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   M4aParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    M4aParserModule* self = ATX_SELF_EX(M4aParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;
	DEBUG0(" ###########M4aParserModule_Attach##########\n");

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".aac" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".m4a",
                                            "audio/m4a");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/m4a" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/m4a",
        &self->m4a_type_id);

	DEBUG0("self->m4a_type_id =%d\n",self->m4a_type_id);
    if (BLT_FAILED(result)) return result;

	/* register an alias for the same mime type */	
	BLT_Registry_RegisterNameForId(registry, 									  
			BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,		
			"audio/m4a", self->m4a_type_id);
 
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   M4aParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
M4aParserModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    M4aParserModule* self = ATX_SELF_EX(M4aParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
	DEBUG0(" ###########M4aParserModule_Probe##########\n");
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need the input protocol to be STREAM_PULL and the output */
            /* protocol to be STREAM_PULL                                  */
             if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }
			DEBUG0("constructor->spec.input.media_type->id  =%d\n",constructor->spec.input.media_type->id );
            /* we need the input media type to be 'audio/m4a' */
            if (constructor->spec.input.media_type->id != self->m4a_type_id) {
                return BLT_FAILURE;
            }
			DEBUG0("constructor->spec.output.media_type->id	=%d\n",constructor->spec.output.media_type->id);

            /* compute the match level */
            if (constructor->name != NULL) {
			DEBUG0("constructor->name =%s\n",constructor->name);
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "M4aParser")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not out name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("M4aParserModule::Probe - Ok [%d]", *match);
			DEBUG0("*** prob ok **match = %d**\n", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(M4aParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(M4aParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(M4aParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(M4aParserModule, M4aParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(M4aParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    M4aParserModule_Attach,
    M4aParserModule_CreateInstance,
    M4aParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define M4aParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(M4aParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(M4aParserModule,
                                         "M4A Parser",
                                         "com.axiosys.parser.m4a",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
