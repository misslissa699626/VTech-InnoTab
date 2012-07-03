/*****************************************************************
|
|   Amr Parser Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltAmrParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"

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
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.amr")

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 amr_type_id;
} AmrParserModule;


typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    ATX_InputStream* 	stream;
    BLT_MediaType 		media_type;
	BLT_Boolean      	eos;
} AmrParserInput;


typedef struct {
	BLT_MediaType   base;
	BLT_UInt32		CodecID;
  	BLT_Int32		nChannels;
	BLT_Int32		nSamplesRate;
	BLT_Int32		nBitRate;
}BLT_AmrAudioMediaType;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_AmrAudioMediaType* 	media_type;
} AmrParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
	BLT_UInt32			CodecID;
	BLT_UInt32 			BitRate;
	BLT_UInt32 			PackSize;
	BLT_UInt32 			cntFrame;
	BLT_UInt32 			totalFrame;
    AmrParserInput  	input;
    AmrParserOutput 	output;	
} AmrParser;




/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

enum CodecID {
    CODEC_ID_NONE,

    /* video codecs */
    CODEC_ID_MPEG1VIDEO,
    CODEC_ID_MPEG2VIDEO,
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
    CODEC_ID_MP3, 
    CODEC_ID_AAC,
    CODEC_ID_AC3,
    CODEC_ID_DTS,
    CODEC_ID_VORBIS,
    CODEC_ID_DVAUDIO,
    CODEC_ID_AMRV1,
    CODEC_ID_AMRV2,
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
    
    CODEC_ID_GSM,
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
    CODEC_ID_GSM_MS, 
    CODEC_ID_ATRAC3,
    CODEC_ID_VOXWARE,
    CODEC_ID_APE,
    CODEC_ID_NELLYMOSER,
    CODEC_ID_MUSEPACK8,
    CODEC_ID_SPEEX,
    CODEC_ID_AMRVOICE,
    CODEC_ID_AMRPRO,
    CODEC_ID_AMRLOSSLESS,
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

    CODEC_ID_MPEG2TS= 0x20000 /**< _FAKE_ codec to indicate a raw MPEG-2 TS
                                * stream (only used by libavformat) */
};

/*----------------------------------------------------------------------
|   Global 
+---------------------------------------------------------------------*/
static const int BitrateTab[2][9] =
{
	{4750,	5150,	5900,	6700,	7400,	7950,	10200,	12200, 0},
	{6600,	8850,	12650,	14250,	15850,	18250,	19850,	23050, 23850}
};
static const int PackedSize[2][9] =
{
	{13,		14, 	16, 	18, 	20, 	21, 	27, 	32, 	0},
	{17,		23, 	32, 	36, 	40, 	46, 	50, 	58, 	60}
};

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AmrParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(AmrParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AmrParser, ATX_Referenceable)


/*----------------------------------------------------------------------
|   AmrParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParserInput_SetStream(BLT_InputStreamUser* _self,
                          ATX_InputStream*     stream,
                          const BLT_MediaType* media_type)
{
    AmrParser*    		self = ATX_SELF_M(input, AmrParser, BLT_InputStreamUser);
    ATX_LargeSize		fileSize;
	BLT_UInt8			bsbuf[10];
	ATX_Size			bytes_read;
	BLT_Int32			mode;
	ATX_Position		where;
	BLT_StreamInfo 		stream_info;
	BLT_Int8 			hdr;
		
	DEBUG0("&&&&&&&&& into AmrParserInput_SetStream &&&&&&&&&&\n");
    /* check media type */
    if (media_type == NULL || media_type->id != self->input.media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->input.stream);
    

    self->input.stream = stream;

	ATX_InputStream_Seek(self->input.stream, 0);
	ATX_InputStream_GetSize(self->input.stream, &fileSize);
	ATX_InputStream_Read(self->input.stream, bsbuf, 10, &bytes_read);
#if 0
	int i=0;
	for(i=0;i<10;i++){
		DEBUG0("%02x ",  bsbuf[i]);
	}
	DEBUG0("\n");
#endif
	if(ATX_CompareMemory("#!AMR\xa", bsbuf, 6)==0)
	{
		self->CodecID = CODEC_ID_AMR_NB;
		hdr = bsbuf[6];
		DEBUG0(" ****** 111111111111 ******\n");
		ATX_InputStream_Tell(self->input.stream, where);
		ATX_InputStream_Seek(self->input.stream, -4 + (BLT_UInt32)where);
		fileSize -= 6;
	}
	else if(ATX_CompareMemory("#!AMR-WB\xa", bsbuf, 9)==0)
	{DEBUG0(" ****** 2222222222222222 ******\n");
		self->CodecID = CODEC_ID_AMR_WB;
		hdr = bsbuf[9];
		ATX_InputStream_Seek(self->input.stream, -1 + (BLT_UInt32)where);
		fileSize -= 9;
	}
	else
	{DEBUG0(" ****** 3333333333333333 ******\n");		
		DEBUG0("file is not ARM/AMRWB format!!!\n");
		return BLT_FAILURE;
	}

	mode = (hdr >> 3) & 0xF;
	//mode = (bsbuf[sizeof(bsbuf) - 1] >> 3) & 0xF;
	DEBUG0("mode = %d\n", mode);
	if(self->CodecID == CODEC_ID_AMR_NB)
	{
		if(mode>=8)
		{
			DEBUG0("format check error\n");
			return BLT_FAILURE;
		}
		self->BitRate = BitrateTab[0][mode];
		self->PackSize = PackedSize[0][mode] + 1;
	
	} else {
			if(mode>=9)
			{
				DEBUG0("format check error\n");
				return BLT_FAILURE;
			}
			self->BitRate = BitrateTab[1][mode];
			self->PackSize = PackedSize[1][mode] + 1;
	}

	self->cntFrame = 0;
	self->totalFrame = (fileSize - 6) / self->PackSize;
	
	
	
	self->output.media_type->nChannels = 1;
	self->output.media_type->nSamplesRate= (self->CodecID == CODEC_ID_AMR_NB ? 8000 : 16000);
	self->output.media_type->nBitRate = self->BitRate;
	self->output.media_type->CodecID = self->CodecID;
	
	DEBUG0("self->BitRate = %d\n", self->BitRate);
	DEBUG0("self->PackSize = %d\n", self->PackSize);
	DEBUG0("self->output.media_type->nSamplesRate = %d\n", self->output.media_type->nSamplesRate);

	stream_info.mask = 0;
	stream_info.sample_rate   	= (self->CodecID == CODEC_ID_AMR_NB ? 8000 : 16000);
    stream_info.channel_count 	= 1;
	stream_info.duration		= self->totalFrame * 20;
	DEBUG0("stream_info.duration = %llu\n", stream_info.duration);

	if(CODEC_ID_AMR_WB == self->CodecID ){
		stream_info.data_type = "#!AMR\xa";				
	}if(CODEC_ID_AMR_WB == self->CodecID){
		stream_info.data_type = "#!AMR-WB\xa";
	}else{
		stream_info.data_type = "UNKOWN";
	}

	stream_info.mask |=
                      BLT_STREAM_INFO_MASK_SAMPLE_RATE   |
                      BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
                      BLT_STREAM_INFO_MASK_DATA_TYPE	 |
                      BLT_STREAM_INFO_MASK_DURATION;
	 /* update the stream info */
    if (stream_info.mask && ATX_BASE(self, BLT_BaseMediaNode).context) {
        BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context,  &stream_info);
   	}
			
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AmrParser_MakeEosPacket
+---------------------------------------------------------------------*/
static BLT_Result
AmrParser_MakeEosPacket(AmrParser* self, BLT_MediaPacket** packet)
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
|   AmrParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParserInput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    AmrParser* self = ATX_SELF_M(input, AmrParser, BLT_MediaPort);

    if (index == 0) {
        *media_type = &self->input.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrParserInput)
    ATX_GET_INTERFACE_ACCEPT(AmrParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AmrParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AmrParserInput, BLT_InputStreamUser)
    AmrParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AmrParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(AmrParserInput, BLT_MediaPort)
    AmrParserInput_GetName,
    AmrParserInput_GetProtocol,
    AmrParserInput_GetDirection,
    AmrParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AmrParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    AmrParser* self = ATX_SELF_M(output, AmrParser, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = self->output.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}



/*----------------------------------------------------------------------
|   AmrParserOutput_GetStream
+---------------------------------------------------------------------*/                         
BLT_METHOD
AmrParserOutput_GetPacket(BLT_PacketProducer*  _self,
                          BLT_MediaPacket**   packet)
{

   AmrParser* self = ATX_SELF_M(output, AmrParser, BLT_PacketProducer);
   BLT_Result 	result;
   ATX_Size 	bytes_read = 0;
   ATX_Position where;
   BLT_UInt8	buf[8 * 1024];
   BLT_UInt32   n = 0;
   BLT_UInt32   size = 0;

	n = 4 * 1024 / self->PackSize;
	if(n > 0){
		size = (n + 1) * self->PackSize;
	}else{
		size = self->PackSize;
	}

	DEBUG0("**** into AmrParserOutput_GetPacket ****\n");	

	/* default value */
	*packet = NULL;
	ATX_SetMemory(buf, 0, 8 * 1024);	
	
	result = ATX_InputStream_Read(self->input.stream, 
                                              buf,
                                              size,
                                              &bytes_read);	
	if (BLT_FAILED(result)) {
		if (result != BLT_ERROR_EOS) {
			return BLT_ERROR_PORT_HAS_NO_DATA;
		 }else{
		 		self->input.eos = BLT_TRUE;	
				return AmrParser_MakeEosPacket(self, packet);
			}	
	}

	result = BLT_Core_CreateMediaPacket(
                        ATX_BASE(self, BLT_BaseMediaNode).core,
                        bytes_read,
                        (BLT_MediaType*)self->output.media_type,
                        packet);


    if (BLT_FAILED(result)) {
		return result;
	}

	ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(*packet),
                                  	buf,
                                   	bytes_read);
	
	DEBUG0("**** after copy ****\n");

   BLT_MediaPacket_SetPayloadSize(*packet, bytes_read);

   return BLT_SUCCESS;

}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrParserOutput)
    ATX_GET_INTERFACE_ACCEPT(AmrParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AmrParserOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AmrParserOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AmrParserOutput, BLT_MediaPort)
    AmrParserOutput_GetName,
    AmrParserOutput_GetProtocol,
    AmrParserOutput_GetDirection,
    AmrParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AmrParserOutput, BLT_PacketProducer)
    AmrParserOutput_GetPacket
ATX_END_INTERFACE_MAP


/*----------------------------------------------------------------------
|   AmrParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
AmrParser_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    AmrParser* self;

    ATX_LOG_FINE("AmrParser::Create");
	DEBUG0("&&&&&&&&& into AmrParser_Create &&&&&&&&&&\n");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(AmrParser));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
	/* construct the object */
    BLT_MediaType_Init(&self->input.media_type,
                       ((AmrParserModule*)module)->amr_type_id);
	self->input.stream = NULL;
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);
	self->output.media_type = (BLT_AmrAudioMediaType*)ATX_AllocateZeroMemory(sizeof(BLT_AmrAudioMediaType)+1);
	BLT_MediaType_InitEx(&self->output.media_type->base, ((AmrParserModule*)module)->amr_type_id, sizeof(BLT_AmrAudioMediaType)+1);
    
	/* setup interfaces */
    ATX_SET_INTERFACE_EX(self, AmrParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, AmrParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  AmrParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  AmrParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, AmrParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, AmrParserOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);
   
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AmrParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AmrParser_Destroy(AmrParser* self)
{
    ATX_LOG_FINE("AmrParser::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* free the media type extensions */
    BLT_MediaType_Free(self->output.media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AmrParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParser_Deactivate(BLT_MediaNode* _self)
{
    AmrParser* self = ATX_SELF_EX(AmrParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("AmrParser::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

	/* release the stream */
    ATX_RELEASE_OBJECT(self->input.stream);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AmrParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    AmrParser* self = ATX_SELF_EX(AmrParser, BLT_BaseMediaNode, BLT_MediaNode);

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

/*----------------------------------------------------------------------
|   AmrParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParser_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    AmrParser* 	self = ATX_SELF_EX(AmrParser, BLT_BaseMediaNode, BLT_MediaNode);
	BLT_Result 	result;
	BLT_UInt32	ms;
	BLT_Int32 	cnt;

	
    /* seek to the estimated offset */
    /* seek into the input stream (ignore return value) */
	ms = point->time_stamp.seconds * 1000 + point->time_stamp.nanoseconds /1000000;

	cnt = (ms + 10) / 20;

	if(cnt < 0) {
		cnt = 0;
	} else {
		if(cnt > self->totalFrame){
			cnt = self->totalFrame;
		} 	
	}

	ATX_InputStream_Seek(self->input.stream, 6 + cnt * self->PackSize);
	
	self->cntFrame = cnt;
	ms = cnt * 20;
	
    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                  */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrParser)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AmrParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AmrParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    AmrParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    AmrParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AmrParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   AmrParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    AmrParserModule* self = ATX_SELF_EX(AmrParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;

	DEBUG0("&&&&&&&&& into AmrParserModule_Attach &&&&&&&&&&\n");
    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".amr" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".amr",
                                            "audio/amr");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/amr" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/amr",
        &self->amr_type_id);

	DEBUG0("&&&&& self->amr_type_id = %u  &&&&\n", self->amr_type_id);
    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AmrParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AmrParserModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    AmrParserModule* self = ATX_SELF_EX(AmrParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
	DEBUG0("&&&&&&&&& into AmrParserModule_Probe &&&&&&&&&&\n");
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
			
			DEBUG0("&&&&& constructor->spec.input.media_type->id = %u  &&&&\n", constructor->spec.input.media_type->id);
			DEBUG0("&&&&& self->amr_type_id = %u  &&&&\n", self->amr_type_id);

            /* we need the input media type to be 'audio/wav' */
            if (constructor->spec.input.media_type->id != self->amr_type_id) {
                return BLT_FAILURE;
            }
			
			
			DEBUG0("&&&&& constructor->spec.output.media_type->id = %u  &&&&\n", constructor->spec.output.media_type->id);
            /* the output type should be unknown at this point */
            if (constructor->spec.output.media_type->id != 
                BLT_MEDIA_TYPE_ID_UNKNOWN) {
                return BLT_FAILURE;
            }
			DEBUG0("&&&&& constructor->name = %s  &&&&\n", constructor->name);
            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "AmrParser")) {
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

            DEBUG0("AmrParserModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AmrParserModule, AmrParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AmrParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    AmrParserModule_Attach,
    AmrParserModule_CreateInstance,
    AmrParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AmrParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AmrParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(AmrParserModule,
                                         "AMR Parser",
                                         "com.axiosys.parser.amr",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)

