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
 
 /***************************************************************************
  * Name: gp_avcodec.h
  *
  * Purpose:
  *
  * Developer:
  * 	zhoulu, 2010-8-18
  *
  * Copyright (c) 2010-2011 by Sunplus mMobile Inc.
  ***************************************************************************/

#ifndef _GP_AVCODEC_H
#define _GP_AVCODEC_H

/***************************************************************************
 * Header Files
 ***************************************************************************/
#include <typedef.h>
 
/***************************************************************************
 * Constants
 ***************************************************************************/
	
/***************************************************************************
 * Macros
 ***************************************************************************/
#define FOUR_CC( a, b, c, d ) \
        ( ((UINT32)a) | ( ((UINT32)b) << 8 ) \
         | ( ((UINT32)c) << 16 ) | ( ((UINT32)d) << 24 ) )

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
#define MP4_CHAN         FOUR_CC('C','H','A','N')


/***************************************************************************
 * Data Types
 ***************************************************************************/
#if 0 //ndef S32
#define S64 SINT64
#define S32 SINT32
#define S16 SINT16
#define S08 SINT8
#define U64 UINT64
#define U32 UINT32
#define U16 UINT16
#define U08 UINT8
#endif

/**
 * Identifies the syntax and semantics of the bitstream.
 * The principle is roughly:
 * Two decoders with the same ID can decode the same streams.
 * Two encoders with the same ID can encode compatible streams.
 * There may be slight deviations from the principle due to implementation
 * details.
 *
 * If you add a codec ID to this list, add it so that
 * 1. no value of a existing codec ID changes (that would break ABI),
 * 2. it is as close as possible to similar codecs.
 */
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
	VIDEO_TYPE_H263_PLUS,
	
	SUBTITAL_TYPE_SPU_DVB
} vidVideoType_t;
	
#ifdef USE_LIBAVCODEC
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
#else

typedef enum audAudioType
{
	AUDIO_TYPE_NONE = 0,
	AUDIO_TYPE_MP2,
	AUDIO_TYPE_MP3,
	AUDIO_TYPE_MP2A,
	AUDIO_TYPE_MP4A,
	AUDIO_TYPE_AAC,
	AUDIO_TYPE_AAC_PLUS,
	AUDIO_TYPE_PCM,
	AUDIO_TYPE_ADPCM,
	AUDIO_TYPE_ADPCM_IMA_WAV,
	AUDIO_TYPE_OGG, 
	AUDIO_TYPE_APE, // 10
	AUDIO_TYPE_FLAC,
	AUDIO_TYPE_COOK,
	AUDIO_TYPE_WMA,
	AUDIO_TYPE_WMA2,
	AUDIO_TYPE_WMAPRO,
	AUDIO_TYPE_AMR_NB,
	AUDIO_TYPE_AMR_WB,
	AUDIO_TYPE_A52,
	AUDIO_TYPE_WAV,
	AUDIO_TYPE_AC3,
	AUDIO_TYPE_VORBIS
	
} audAudioType_t;
#endif



enum CodecType {
    CODEC_TYPE_UNKNOWN = -1,
    CODEC_TYPE_VIDEO,
    CODEC_TYPE_AUDIO,
    CODEC_TYPE_DATA,
    CODEC_TYPE_SUBTITLE,
    CODEC_TYPE_ATTACHMENT,
    CODEC_TYPE_NB
};

// define for element stream type
typedef enum gpEsType_e {
    GP_ES_TYPE_UNKNOWN = 0,
    GP_ES_TYPE_VIDEO,
    GP_ES_TYPE_AUDIO,
    GP_ES_TYPE_DATA,
    GP_ES_TYPE_SUBTITLE,
    GP_ES_TYPE_ATTACHMENT,
    GP_ES_TYPE_NB
}gpEsType_t;

// define for contain stream type
typedef enum gpCsType_e {
    GP_CS_TYPE_UNKNOWN = -1,
	GP_CS_TYPE_RM = 0,
	GP_CS_TYPE_AVI,
	GP_CS_TYPE_MP4,
	GP_CS_TYPE_MKV,
	GP_CS_TYPE_FLV,
	GP_CS_TYPE_ASF,
	GP_CS_TYPE_OGG,
	GP_CS_TYPE_VOB, // dvd vob
	GP_CS_TYPE_PS, // mpg / dat / vob
}gpCsType_t;

#endif /* _GP_AVCODEC_H */
