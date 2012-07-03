/******************************************************************************
 *
 * Copyright (C), 2010-2013, Sunmedia. Co., Ltd.
 *
 *******************************************************************************
 * File Name     : ceva.h
 * Version       : Initial Draft
 * Author        : 
 * Created       : 2010/02/25
 * Last Modified :
 * Description   :
 *              
 * Function List :
 *******************************************************************************/
#ifndef _CEVA_H_
#define _CEVA_H_

#include <mach/typedef.h>

#ifdef cplusplus
extern "C" {
#endif

typedef struct codecLoader_s
{
	SINT8 *pIntCodecFile;  /*!< @brief file path of the internal codec file */
	SINT8 *pExtCodecFile;  /*!< @brief file path of the externel codec file */
	SINT8 *pCodecFile;     /*!< @brief file path of the codec file */
	UINT8 *pIntCodecImg;   /*!< @brief the internal codec image address */
	UINT8 *pExtCodecImg;   /*!< @brief the external codec image address */
	UINT8 *pCodecImg;      /*!< @brief the codec image address */
} codecLoader_t;

SINT32 cevaImageBoot(codecLoader_t *loader, SINT32 fd_ceva);

SINT32 codecBoot(SINT32 fd_ceva, codecLoader_t *loader);
SINT32 codecInit(SINT32 fd_ceva, void *arg);
SINT32 codecDone(SINT32 fd_ceva, void *arg);
SINT32 codecFree(SINT32 fd_ceva, void *arg);

#if 1 // add by zhoulu for ceva codec lib

/***************************************************************************
 * Constants
 ***************************************************************************/
// Decoder Flags
    // jpgdec
#define VFLAG_DOWNx2        0x00000001  // [in]  down sample by 2 directly when decoding
#define VFLAG_DOWNx4        0x00000002  // [in]  down sample by 4 directly when decoding
#define VFLAG_DOWNx8        0x00000004  // [in]  down sample by 8 directly when decoding
#define VFLAG_PARTDEC       0x00000008  // [in]  decode partial image

    // h263dec
#define VFLAG_FLASH         0x00000001  // [in] flash video decoding

	//h264
#define VFLAG_RESET         0x00000001
	
    // rvdec
#define VFLAG_RV9           0x00000001  // [in] RV9 video input
#define VFLAG_RV8           0x00000002  // [in] RV8 video input
#define VFLAG_RVG2          0x00000004  // [in] RVG2 video input
#define VFLAG_RVRAWHDR      0x00000008  // [in] 0: RV format; 1: raw format
#define VFLAG_NOINLOOPF     0x00000010  // [in] disable inloop-filtering of P-frame
    // General
#define VFLAG_DEBLOCK       0x00010000  // [in] enable post-processing
#define VFLAG_ERRRES        0x00020000  // [in] enable error resilience 
#define VFLAG_SEQHEADDEC    0x00040000  // [in] force decoder to parse sequence header
#define VFLAG_FRAMEFETCH    0x00080000  // [in] get the buffered frame without decoding 
#define VFLAG_FIELDYUV      0x00100000  // [in] output field by field for interlace video
#define VFLAG_CUR_FRAME_OUT 0x00200000  // [in] output decoded order frames instead of display order frames
#define VFLAG_OUTBUFRW      0x01000000  // [in] output buffer might be updated by AP (can't be used as reference frame buffer)
#define VFLAG_SKIPB         0x02000000  // [in] does not decode (skip) if B frame encountered
#define VFLAG_ARGBOUT       0x00000040  // [in] output ARGB instead of YUV
#define VFLAG_RGB565OUT     0x00000100  // [in] output RGB565 instead of YUV
#define VFLAG_FRAMEOUT      0x10000000  // [out] output frame available
#define VFLAG_YUV422OUT     0x20000000  // [out] output YUV422 instead of YUV42

#define VFLAG_FIELD_FRAME   0x00001000  // [out] output frame is an interlace frame

#define VFLAG_NOBURST8      0x04000000  // [in]  can't use XDMA burst-8
#define VFLAG_NOBURST4      0x08000000  // [in]  can't use XDMA burst-8 and burst-4

// Encoder Flags
    // jpgenc
#define VFLAG_YUV420IN      0x00000001  // [in] input YUV420 instead of YUV422
    // General
#define VFLAG_SEQHEADENC    0x00010000  // [in] force encoder to generate sequence header

#define VFLAG_ARGBOUT       0x00000040  // [in] output ARGB */
#define VFLAG_RGB565OUT     0x00000100  // [in] output RGB565 */

/// added by chenhn 	2008/05/15
#define	FRAME_I			0
#define	FRAME_P			1
#define	FRAME_B			2

// return error code 
#define ID_OK							0                   // general OK
#define ID_ERR                          -1                  // general Error

#define ERR_MEMORY						-100                // general memory fail
#define ERR_MEMORY_ALLOCATE             (ERR_MEMORY-1)		// memory allocation fail
#define ERR_MEMORY_REALLOC              (ERR_MEMORY-2)		// memory re-allocation fail
#define ERR_MEMORY_FREE                 (ERR_MEMORY-3)		// memory free fail
#define ERR_MEMORY_READ                 (ERR_MEMORY-4)		// memory read fail
#define ERR_MEMORY_WRITE                (ERR_MEMORY-5)		// memory write fail
#define ERR_MEMORY_OUTOF                (ERR_MEMORY-6)		// memory not enough
#define ERR_SRAM_ALLOCATE               (ERR_MEMORY-11)		// fast memory allocation fail
#define ERR_SRAM_REALLOC                (ERR_MEMORY-12)		// fast memory re-allocation fail
#define ERR_SRAM_FREE                   (ERR_MEMORY-13)		// fast memory free fail
#define ERR_SRAM_READ                   (ERR_MEMORY-14)		// fast memory read fail
#define ERR_SRAM_WRITE                  (ERR_MEMORY-15)		// fast memory write fail

#define ERR_ARGUMENT					-200                // general argument fail
#define ERR_ARG_UNKOWN					(ERR_ARGUMENT-1)    // unknown argument
#define ERR_RESOLUTION					(ERR_ARGUMENT-2)    // resolution not support
#define ERR_RESOLUTION_16				(ERR_ARGUMENT-3)	// width or height not the integral multiple of 16
#define ERR_RESOLUTION_LIMIT			(ERR_ARGUMENT-4)	// resolution over the supported limit
#define ERR_PROFILE                     (ERR_ARGUMENT-5)    // profile not support
#define ERR_LEVEL                       (ERR_ARGUMENT-6)    // level not support

#define ERR_INIT_FAIL					-300                // general initialization fail
#define ERR_NOT_INIT					(ERR_INIT_FAIL-1)   // not initialized yet
#define ERR_VERSION						(ERR_INIT_FAIL-2)	// version mismatch
#define ERR_GET_VERSION					(ERR_INIT_FAIL-3)   // fail to get version number
#define ERR_INPUT_BUF                   (ERR_INIT_FAIL-4)   // input buffer null
#define ERR_OUTPUT_BUF                  (ERR_INIT_FAIL-5)   // output buffer null
#define ERR_INIT_DUP                    (ERR_INIT_FAIL-6)   // duplicate initialization

#define ERR_DECODE						-400                // general decode fail
#define ERR_HEADER                      (ERR_DECODE-1)      // error header
#define ERR_FRAME_TYPE                  (ERR_DECODE-2)      // error frame type
#define ERR_MB_TYPE                     (ERR_DECODE-3)      // error MB type
#define ERR_STREAM_END                  (ERR_DECODE-4)      // end of stream reached
#define ERR_DECODE_SEQUENCE             (ERR_DECODE-5)      // decode sequence fail
#define ERR_DECODE_FRAME                (ERR_DECODE-6)      // decode frame fail
#define ERR_DECODE_MB                   (ERR_DECODE-7)      // decode MB fail
#define ERR_DECODE_BLOCK                (ERR_DECODE-8)      // decode block fail
#define ERR_DECODE_MC                   (ERR_DECODE-9)      // motion compensation fail
#define ERR_DECODE_DCT                  (ERR_DECODE-10)     // IDCT fail
#define ERR_DECODE_Q                    (ERR_DECODE-11)     // inverse quantization fail
#define ERR_DECODE_VLD                  (ERR_DECODE-12)     // VLD fail
#define ERR_DECODE_FILTER               (ERR_DECODE-13)     // filtering fail
#define ERR_DECODE_BUFUNDERFLOW         (ERR_DECODE-14)     // input buffer underflow

//ewang fixme
#define ERR_DECODE_NALUNITHEADERERR     (ERR_DECODE-31)     // NAL unit header error
#define ERR_DECODE_SLICETYPETOOLARGE    (ERR_DECODE-33)     // slice type too large
#define ERR_DECODE_NONEXISTPPSREF       (ERR_DECODE-34)     // non existing PPS referenced
#define ERR_DECODE_PPSIDOUTOFRANGE      (ERR_DECODE-35)     // pps_id out of range
#define ERR_DECODE_NONEXISTSPSREF       (ERR_DECODE-36)     // non existing SPS referenced

#define ERR_DECODE_REFBUFUNDERFLOW      (ERR_DECODE-47)     // reference frame buffer is underflow
#define ERR_DECODE_REFCNTOVERFLOW       (ERR_DECODE-49)     // reference count overflow
#define ERR_DECODE_ABSDIFFPICNUMERR     (ERR_DECODE-50)     // abs_diff_pic_num overflow
#define ERR_DECODE_ERRREORDERINGIDC     (ERR_DECODE-51)     // illegal reordering_of_pic_nums_idc
#define ERR_DECODE_QPOUTOFRANGE         (ERR_DECODE-52)     // QP out of range
#define ERR_DECODE_ERRMMCOPERATION      (ERR_DECODE-53)     // illegal memory management control operation
#define ERR_DECODE_UNUSEDPICNOTFOUND    (ERR_DECODE-54)     // unused picture not found
#define ERR_DECODE_NULLFRAMEBUFFER      (ERR_DECODE-60)     // NULL Frame Buffer
#define ERR_DECODE_SPSIDOUTOFRANGE      (ERR_DECODE-61)     // pps_id out of range
#define ERR_DECODE_SHORTREFCNTOVERFLOW  (ERR_DECODE-64)     // short reference count overflow
#define ERR_DECODE_SHORTREFCNTERR       (ERR_DECODE-65)     // short reference count error
#define ERR_DECODE_ERRNALUINTTYPE       (ERR_DECODE-63)     // unknow NAL unit type
#define ERR_DECODE_PPSIDERR             (ERR_DECODE-66)     // pps_id error
#define ERR_DECODE_SPSIDERR             (ERR_DECODE-67)     // pps_id error



#define ERR_ENCODE						-500                // general encode fail
#define ERR_ENCODE_SEQUENCE             (ERR_ENCODE-1)      // encode sequence fail
#define ERR_ENCODE_FRAME                (ERR_ENCODE-2)      // encode frame fail
#define ERR_ENCODE_MB                   (ERR_ENCODE-3)      // encode MB fail
#define ERR_ENCODE_BLOCK                (ERR_ENCODE-4)      // encode block fail
#define ERR_ENCODE_ME                   (ERR_ENCODE-5)      // motion estimation fail
#define ERR_ENCODE_MC                   (ERR_ENCODE-6)      // motion compensation fail
#define ERR_ENCODE_DCT                  (ERR_ENCODE-7)      // DCT/IDCT fail
#define ERR_ENCODE_Q                    (ERR_ENCODE-8)      // (inverse) quantization fail
#define ERR_ENCODE_VLD                  (ERR_ENCODE-9)      // VLC fail
#define ERR_ENCODE_FILTER               (ERR_ENCODE-10)     // filtering fail
#define ERR_ENCODE_RC                   (ERR_ENCODE-11)     // rate control fail
#define ERR_ENCODE_BUFOVERFLOW          (ERR_ENCODE-12)     // output buffer overflow

#define MSG_DECODE                      -600                // general decode message
#define MSG_GET_SEQHEAD                 (MSG_DECODE-1)      // get sequence header

#define MSG_ENCODE                      -700                // general encode message


/***************************************************************************
 * Macros
 ***************************************************************************/

/***************************************************************************
 * Data Types
 ***************************************************************************/
typedef enum cevaCodecType 
{ 
	CEVA_CODEC_TYPE_NONE = 0, 
	CEVA_CODEC_TYPE_MPEG4_SP,		// mpeg4	sp
	CEVA_CODEC_TYPE_MPEG4_ASP,	// mpeg4 asp
	CEVA_CODEC_TYPE_MJPEG, 
	CEVA_CODEC_TYPE_H263, 
	CEVA_CODEC_TYPE_S263, /*for Sorenson H.263*/ 
	CEVA_CODEC_TYPE_H264_BP,	
	CEVA_CODEC_TYPE_H264_MP,
	CEVA_CODEC_TYPE_H264_HP,
	CEVA_CODEC_TYPE_WMV7,			
	CEVA_CODEC_TYPE_WMV8,			
	CEVA_CODEC_TYPE_VC1_SP, /*for VC1 SP */ 
	CEVA_CODEC_TYPE_VC1_AP, /*for VC1 MP and AP */ 
	CEVA_CODEC_TYPE_MPEG2, 			 
	CEVA_CODEC_TYPE_MPEG1,			
	CEVA_CODEC_TYPE_RV10, // not supported 
	CEVA_CODEC_TYPE_RV20, /* RVG2 */ // not supported 
	CEVA_CODEC_TYPE_RV30, /* RV8 */ 
	CEVA_CODEC_TYPE_RV40, /* RV9 and RV10 */ 
	CEVA_CODEC_TYPE_DIV3,  
	CEVA_CODEC_TYPE_JPG,
	CEVA_CODEC_TYPE_JPG_ENCODE,
	CEVA_CODEC_TYPE_MPEG4_ENCODE, 
	CEVA_CODEC_TYPE_H264_ENCODE,   // not supported 
	CEVA_CODEC_TYPE_MJPG_ENCODE,
	CEVA_CODEC_TYPE_VP6,
	CEVA_CODEC_TYPE_MPEG4_HD,
	CEVA_CODEC_TYPE_THEORA,
	CEVA_CODEC_TYPE_KGB
} cevaCodecType_t;
/* -------  */
/* Decoder  */
/* -------  */

/* common decoder struct */
typedef struct 
{
    SINT32  timeStampIn;     // [in]     time stamp of input decoded frame
    UINT8  *pInBuf;         // [in]     buffer to keep input bitstream
    SINT32  nUsefulByte;     // [in]     availabe bytes in pInBuf
    UINT8  *pFrameBuf;      // [in]     large frame buffer (requires 16-byte position alignment)
    SINT32  nFrameBufSize;   // [in/out] availabe/required large frame buffer size

    SINT32  timeStampOut;    // [out]    time stamp of output displayed frame
    UINT8  *pOutBuf[3];     // [out]    buffer to keep decoded YUV
    UINT32 width;           // [out]    frame width
    UINT32 height;          // [out]    frame height
    UINT32 stride;          // [out]    Y frame buffer stride
    UINT32 strideChroma;    // [out]    UV frame buffer stride
    SINT32  frameType;       // [out]    I/P/B etc.		

    UINT32 flags;           // [in/out] ref VFLAG_XXX
    void   *pDecHandle;     // [out]    private working space
    SINT8   *version;        // [out]    library version
} VDecode_t;

typedef VDecode_t cevaDecode_t;
/* -------  */
/* Encoder  */
/* -------  */

/* common encoder struct  */
typedef struct {
    SINT32  timeStampIn;     // [in]     time stamp of input decoded frame
    UINT8  *pInBuf[3];      // [in]     buffer to keep YUV input data
    UINT8  *pFrameBuf;      // [in]     large frame buffer (requires 16-byte position alignment)
    SINT32  nFrameBufSize;   // [in/out] availabe/required large frame buffer size
    UINT32 width;           // [in]     frame width
    UINT32 height;          // [in]     frame height
    UINT32 bitRate;         // [in]     bitrate in kbps
    UINT32 picRate;         // [in]     picture rate
    UINT32 gopLen;          // [in]     key-frame interval

    // if profile==0 then bframes=QType=interlaced=0
    UINT32 profile;         // [in]     0 (baseline or basic profile) / 1 (main profile) / 2 (advance or high profile)
    UINT32 nBFrames;        // [in]     Number of B-frames between I and P
    UINT32 QType;           // [in]     Quantization type: 0 (H.263) / 1 (MPEG)
    UINT32 interlaced;      // [in]     0 (progressive coding) / 1 (interlace coding)
    UINT32 disInloopFilter; // [in]     disable inloop filter

    UINT32 vbr;             // [in]     vbr? 0(CBR) / 1(VBR)
    UINT32 rcMode;          // [in]     bitrate control mode; n: 1(frame level) / 0(macroblock level)

    SINT32  timeStampOut;    // [out]    time stamp of output displayed frame
    UINT8  *pOutBuf;        // [out]    buffer to keep output bitstream
    SINT32  nBytes;          // [out]    availabe bytes in pOutBuf

    UINT32 flags;           // [in/out] ref VFLAG_XXX
    void   *pEncHandle;     // [out]    codec private working space
    SINT8   *version;        // [out]    library version
} VEncode_t;

typedef VEncode_t cevaEncode_t;


typedef struct {
    void *pEncHandle;       // [out]    codec private working space
    UINT8 *pInBuf[3];       // [in]     buffer to keep YUV input data
    UINT8 *pOutBuf;         // [out]    buffer to keep H.264 bitstream
    UINT32 flags;           // [in/out] ref VFLAG_XXX
    UINT32 width;           // [in]     frame width
    UINT32 height;          // [in]     frame height
    UINT32 bitRate;         // [in]     bitrate in kbps
    UINT32 picRate;         // [in]     picture rate
    UINT32 gopLen;          // [in]     key-frame interval

    // if profile==0 then bframes=QType=interlaced=0
    UINT32 profile;         // [in]     0 (baseline or basic profile) / 1 (main profile) / 2 (advance or high profile)
    UINT32 nBFrames;        // [in]     Number of B-frames between I and P
    UINT32 QType;           // [in]     Quantization type: 0 (H.263) / 1 (MPEG)
    UINT32 interlaced;      // [in]     0 (progressive coding) / 1 (interlace coding)
    UINT32 disInloopFilter; // [in]     disable inloop filter

    UINT32 vbr;             // [in]     vbr? 0(CBR) / 1(VBR)
    UINT32 rcMode;          // [in]     bitrate control mode; n: 1(frame level) / 0(macroblock level)
    UINT32 nBytes;          // [in/out] availabe bytes in pOutBuf
    SINT8   *version;        // [in]     library version
} VEncode_Cap_t;

typedef VEncode_Cap_t cevaEncodeCap_t;

SINT32 ceva_codec_load(cevaCodecType_t format);
SINT32 ceva_codec_unload(void);
SINT32 ceva_codec_instance_size(void);
SINT32 ceva_codec_init(void *arg);
SINT32 ceva_codec_done(void *arg);
SINT32 ceva_codec_uninit(void *arg);

/**
 * @brief   Ceva codec path set function
 * @param   codecPath [in] ceva codec path
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecPath(SINT8 *codecPath);

/**
 * @brief   Ceva codec load function
 * @param   format [in] ceva codec type
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecLoad(cevaCodecType_t format);
SINT32 gpCevaCodecLoadEx(cevaCodecType_t format);


/**
 * @brief   Ceva codec unload function
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecUnload(void);

/**
 * @brief   Ceva codec instance size function
 * @param   arg [in] pointer to the argument data
 * @return  success: size,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecInstanceSize(void);

/**
 * @brief   Ceva codec init function
 * @param   arg [in] pointer to the argument data
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecInit(void *arg);

/**
 * @brief   Ceva codec exec function
 * @param   arg [in] pointer to the argument data
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecExec(void *arg);

/**
 * @brief   Ceva codec uninit function
 * @param   arg [in] pointer to the argument data
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaCodecUninit(void *arg);

/**
 * @brief   Ceva codec type get function
 * @return  current Ceva codec type
 */
cevaCodecType_t gpCevaCodecType(void);

/**
 * @brief   Ceva irq priority setting function
 * @param   priority [in] priority value (0~15)
 * @return  success: 0,  fail: error code
 * @see     
 */
SINT32 gpCevaIrqPriority(UINT32 priority);

/**
 * @brief   Ceva debug flag setting function
 * @param   dbgFlag [in] debug flag (see gp_ceva.h)
 * @return  success: 0,  fail: error code
 */
SINT32 gpCevaDebugFlag(UINT32 dbgFlag);

SINT32 gpCevaCheckSumOpen(void);
SINT32 gpCevaCheckSumClose(void);
SINT32 gpCevaDumpCode(char *dumpfile);
SINT32 gpCevaCheckSum(SINT32 count);
#endif

#ifdef cplusplus
}
#endif

#endif /* _CEVA_H_ */

