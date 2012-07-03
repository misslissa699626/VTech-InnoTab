/*****************************************************************
|
|   BlueTune - Helix AAC Decoder Module
|
|   (c) 2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltAacDecoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"

//#include "aacdec.h"

#include "gp_avcodec.h"
#include "auddec.h"
#include "gp_acodec.h"
#include <dlfcn.h>

#define DEBUG	0
#if DEBUG
	#define DEBUG0 printf
#else
	#define DEBUG0
#endif


#if 0
#define DEBUG	1
#if DEBUG
	#define DEBUG0(args...) fprintf(stderr, args...)
#else
	#define DEBUG0(...)
#endif
#endif
/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.aac")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 mp4es_type_id;
} AacDecoderModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    BLT_Boolean eos;
} AacDecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    ATX_List*        packets;
    BLT_PcmMediaType media_type;
} AacDecoderOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    AacDecoderInput   input;
    AacDecoderOutput  output;
    BLT_UInt32        mp4es_type_id;
   // HAACDecoder       helix_decoder;
    unsigned int      sample_buffer_size;
} AacDecoder;

typedef enum {
    BLT_AAC_OBJECT_TYPE_AAC_MAIN        = 1,  /**< AAC Main Profile              */
    BLT_AAC_OBJECT_TYPE_AAC_LC          = 2,  /**< AAC Low Complexity            */
    BLT_AAC_OBJECT_TYPE_AAC_SSR         = 3,  /**< AAC Scalable Sample Rate      */
    BLT_AAC_OBJECT_TYPE_AAC_LTP         = 4,  /**< AAC Long Term Prediction           */
    BLT_AAC_OBJECT_TYPE_SBR             = 5,  /**< Spectral Band Replication          */
    BLT_AAC_OBJECT_TYPE_AAC_SCALABLE    = 6,  /**< AAC Scalable                       */
    BLT_AAC_OBJECT_TYPE_ER_AAC_LC       = 17, /**< Error Resilient AAC Low Complexity */
    BLT_AAC_OBJECT_TYPE_ER_AAC_LTP      = 19, /**< Error Resilient AAC Long Term Prediction */
    BLT_AAC_OBJECT_TYPE_ER_AAC_SCALABLE = 20, /**< Error Resilient AAC Scalable */
    BLT_AAC_OBJECT_TYPE_ER_AAC_LD       = 23  /**< Error Resilient AAC Low Delay */
} AacObjectTypeIdentifier;

/**
 * Channel configuration for multichannel audio buffers.
 */
typedef enum {
    BLT_AAC_CHANNEL_CONFIG_NONE   = 0, /**< No channel (not used)       */
    BLT_AAC_CHANNEL_CONFIG_MONO   = 1, /**< Mono (single audio channel) */
    BLT_AAC_CHANNEL_CONFIG_STEREO = 2, /**< Stereo (Two audio channels) */
    BLT_AAC_CHANNEL_CONFIG_STEREO_PLUS_CENTER = 3, /**< Stereo plus one center channel */
    BLT_AAC_CHANNEL_CONFIG_STEREO_PLUS_CENTER_PLUS_REAR_MONO = 4, /**< Stereo plus one center and one read channel */
    BLT_AAC_CHANNEL_CONFIG_FIVE = 5,           /**< Five channels */
    BLT_AAC_CHANNEL_CONFIG_FIVE_PLUS_ONE = 6,  /**< Five channels plus one low frequency channel */
    BLT_AAC_CHANNEL_CONFIG_SEVEN_PLUS_ONE = 7, /**< Seven channels plus one low frequency channel */
    BLT_AAC_CHANNEL_CONFIG_UNSUPPORTED
} AacChannelConfiguration;

/**
 * Detailed decoder configuration information.
 * This information is necessary in order to create a decoder object.
 * It is normally obtained from the DecoderSpecificInfo field of the
 * DecoderConfigDescriptor descriptor carried in the sample description
 * for the audio samples. See 14496-3, subpart 1, p 1.6.2.1 for details.
 */
typedef struct {
    AacObjectTypeIdentifier object_type;              /**< Type identifier for the audio data */
    unsigned int            sampling_frequency_index; /**< Index of the sampling frequency in the sampling frequency table */
    unsigned int            sampling_frequency;       /**< Sampling frequency */
    AacChannelConfiguration channel_configuration;    /**< Channel configuration */
    BLT_Boolean             frame_length_flag;        /**< Frame Length Flag     */
    BLT_Boolean             depends_on_core_coder;    /**< Depends on Core Coder */
    BLT_Boolean             core_coder_delay;         /**< Core Code delay       */
    /** Extension details */
    struct {
        BLT_Boolean             sbr_present;              /**< SBR is present        */
        AacObjectTypeIdentifier object_type;              /**< Extension object type */
        unsigned int            sampling_frequency_index; /**< Sampling frequency index of the extension */
        unsigned int            sampling_frequency;       /**< Sampling frequency of the extension */
    } extension;
} AacDecoderConfig;

typedef struct
{
    const unsigned char* data;
    BLT_Size             data_size;
    unsigned int         pos;
} AacBitStream;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AacDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(AacDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AacDecoder, ATX_Referenceable)

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_AAC_FRAME_SIZE                  1024
#define BLT_AAC_OBJECT_TYPE_ID_MPEG2_AAC_LC 0x67
#define BLT_AAC_OBJECT_TYPE_ID_MPEG4_AUDIO  0x40
#define BLT_AAC_MAX_SAMPLING_FREQUENCY_INDEX 12
#define RING_ALLOCATE_SIZE 					100 * 1024
static const unsigned int AacSamplingFreqTable[13] =
{
	96000, 88200, 64000, 48000, 
    44100, 32000, 24000, 22050, 
    16000, 12000, 11025, 8000, 
    7350
};

static const  audec_interface_t*	audec = NULL;
static audec_param_t 				adp;
static BLT_UInt32 					seekable = 0;
static ATX_UInt8 					es_buf [RING_ALLOCATE_SIZE];
static ATX_Int32					RefillSize = 0;
static ATX_UInt32					ringCnt = 0;
static ATX_Int32					first_init = 1;
static const ATX_Any 				*hlib = NULL;
static BLT_UInt64 					pcm_size = 0;
static BLT_UInt32					out_samRate = 0;
static BLT_UInt32					out_chcnt = 0;
static BLT_UInt32					out_bitsRate = 0;


/*----------------------------------------------------------------------
|       AacBitStream_Construct
+---------------------------------------------------------------------*/
static void
AacBitStream_Construct(AacBitStream*        bits, 
                       const unsigned char* data, 
                       BLT_Size             data_size)
{
    bits->data      = data;
    bits->data_size = data_size;
    bits->pos       = 0;
}

/*----------------------------------------------------------------------
|       AacBitStream_GetBitsLeft
+---------------------------------------------------------------------*/
static BLT_Size   
AacBitStream_GetBitsLeft(AacBitStream* bits)
{
    return 8*bits->data_size-bits->pos;
}

/*----------------------------------------------------------------------
|       AacBitStream_ReadBits
+---------------------------------------------------------------------*/
static unsigned int
AacBitStream_ReadBits(AacBitStream* bits, unsigned int n)
{
    unsigned int result = 0;
    while (n) {
        unsigned int bits_avail = 8-(bits->pos%8);
        unsigned int chunk_size = bits_avail >= n ? n : bits_avail;
        unsigned int chunk_bits = (((unsigned int)(bits->data[bits->pos/8]))>>(bits_avail-chunk_size))&((1<<chunk_size)-1);
        result = (result << chunk_size) | chunk_bits;
        n -= chunk_size;
        bits->pos += chunk_size;
    }
    
    return result;
}

/*----------------------------------------------------------------------
|   AacGetAudioObjectType
+---------------------------------------------------------------------*/
static BLT_Result
AacGetAudioObjectType(AacBitStream* bits, AacObjectTypeIdentifier* audio_object_type)
{
    if (AacBitStream_GetBitsLeft(bits) < 5) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    *audio_object_type = AacBitStream_ReadBits(bits, 5);
	if (*audio_object_type == 31) {
        if (AacBitStream_GetBitsLeft(bits) < 6) return BLT_ERROR_INVALID_MEDIA_FORMAT;
		*audio_object_type = 32 + AacBitStream_ReadBits(bits, 6);
	}
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AacGetGASpecificInfo
+---------------------------------------------------------------------*/
static BLT_Result
AacGetGASpecificInfo(AacBitStream* bits, AacDecoderConfig* config)
{
    if (AacBitStream_GetBitsLeft(bits) < 2) return BLT_ERROR_INVALID_MEDIA_FORMAT;
	config->frame_length_flag = AacBitStream_ReadBits(bits,1);
	config->depends_on_core_coder = AacBitStream_ReadBits(bits, 1);
	if (config->depends_on_core_coder) {		
        if (AacBitStream_GetBitsLeft(bits) < 14) return BLT_ERROR_INVALID_MEDIA_FORMAT;
		config->core_coder_delay = AacBitStream_ReadBits(bits, 14);
    } else {
        config->core_coder_delay = 0;
    }
    if (AacBitStream_GetBitsLeft(bits) < 1) return BLT_ERROR_INVALID_MEDIA_FORMAT;
	AacBitStream_ReadBits(bits, 1); /* extensionFlag */ 
	if (config->channel_configuration == BLT_AAC_CHANNEL_CONFIG_NONE) {		
		/*program_config_element (); */
	}		

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AacGetSamplingFrequency
+---------------------------------------------------------------------*/
static BLT_Result
AacGetSamplingFrequency(AacBitStream* bits, 
                        unsigned int* sampling_frequency_index,
                        unsigned int* sampling_frequency)
{
    if (AacBitStream_GetBitsLeft(bits) < 4) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    *sampling_frequency_index = AacBitStream_ReadBits(bits, 4);;
    if (*sampling_frequency_index == 0xF) {
        if (AacBitStream_GetBitsLeft(bits) < 24) {
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        *sampling_frequency = AacBitStream_ReadBits(bits, 24);
    } else if (*sampling_frequency_index <= BLT_AAC_MAX_SAMPLING_FREQUENCY_INDEX) {
        *sampling_frequency = AacSamplingFreqTable[*sampling_frequency_index];
    } else {
        *sampling_frequency = 0;
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AacDecoderConfig_Parse
+---------------------------------------------------------------------*/
static BLT_Result
AacDecoderConfig_Parse(const unsigned char* encoded, 
                       BLT_Size             encoded_size, 
                       AacDecoderConfig*    config)
{
    BLT_Result   result;
    AacBitStream bits;
    AacBitStream_Construct(&bits, encoded, encoded_size);

    /* default config */
    ATX_SetMemory(config, 0, sizeof(*config));

	result = AacGetAudioObjectType(&bits, &config->object_type);
    if (BLT_FAILED(result)) return result;

    result = AacGetSamplingFrequency(&bits, 
                                     &config->sampling_frequency_index, 
                                     &config->sampling_frequency);
    if (BLT_FAILED(result)) return result;

    if (AacBitStream_GetBitsLeft(&bits) < 4) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
	config->channel_configuration = AacBitStream_ReadBits(&bits, 4);

	if (config->object_type == BLT_AAC_OBJECT_TYPE_SBR) {
		config->extension.object_type = config->object_type;
		config->extension.sbr_present = BLT_TRUE;
        result = AacGetSamplingFrequency(&bits, 
                                         &config->extension.sampling_frequency_index, 
                                         &config->extension.sampling_frequency);
        if (BLT_FAILED(result)) return result;
		result = AacGetAudioObjectType(&bits, &config->object_type);
        if (BLT_FAILED(result)) return result;
	}
    
	switch (config->object_type) {
        case BLT_AAC_OBJECT_TYPE_AAC_MAIN:
        case BLT_AAC_OBJECT_TYPE_AAC_LC:
        case BLT_AAC_OBJECT_TYPE_AAC_SSR:
        case BLT_AAC_OBJECT_TYPE_AAC_LTP:
        case BLT_AAC_OBJECT_TYPE_AAC_SCALABLE:
        case BLT_AAC_OBJECT_TYPE_ER_AAC_LC:
        case BLT_AAC_OBJECT_TYPE_ER_AAC_LTP:
        case BLT_AAC_OBJECT_TYPE_ER_AAC_SCALABLE:
        case BLT_AAC_OBJECT_TYPE_ER_AAC_LD:
            result = AacGetGASpecificInfo(&bits, config);
            break;

        default:
            break;
    }

    /* extension (only supported for non-ER AAC types here) */
	if ((config->object_type == BLT_AAC_OBJECT_TYPE_AAC_MAIN ||
         config->object_type == BLT_AAC_OBJECT_TYPE_AAC_LC   ||
         config->object_type == BLT_AAC_OBJECT_TYPE_AAC_SSR  ||
         config->object_type == BLT_AAC_OBJECT_TYPE_AAC_LTP  ||
         config->object_type == BLT_AAC_OBJECT_TYPE_AAC_SCALABLE) &&
        AacBitStream_GetBitsLeft(&bits) >= 16) {
        unsigned int sync_extension_type = AacBitStream_ReadBits(&bits, 11);
        if (sync_extension_type == 0x2b7) {
            result = AacGetAudioObjectType(&bits, &config->extension.object_type);
            if (BLT_FAILED(result)) return result;
            if (config->extension.object_type == BLT_AAC_OBJECT_TYPE_SBR) {
                config->extension.sbr_present = AacBitStream_ReadBits(&bits, 1);
                if (config->extension.sbr_present) {
                    result = AacGetSamplingFrequency(&bits, 
                                      &config->extension.sampling_frequency_index, 
                                      &config->extension.sampling_frequency);
                    if (BLT_FAILED(result)) return result;
                }
            }
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AacDecoderConfig_GetChannelCount
+---------------------------------------------------------------------*/
static unsigned int
AacDecoderConfig_GetChannelCount(const AacDecoderConfig* config)
{
    switch (config->channel_configuration) {
        case BLT_AAC_CHANNEL_CONFIG_MONO: return 1;
        case BLT_AAC_CHANNEL_CONFIG_STEREO: return 2;
        case BLT_AAC_CHANNEL_CONFIG_STEREO_PLUS_CENTER: return 3;
        case BLT_AAC_CHANNEL_CONFIG_STEREO_PLUS_CENTER_PLUS_REAR_MONO: return 4;
        case BLT_AAC_CHANNEL_CONFIG_FIVE: return 5;
        case BLT_AAC_CHANNEL_CONFIG_FIVE_PLUS_ONE: return 6;
        case BLT_AAC_CHANNEL_CONFIG_SEVEN_PLUS_ONE: return 8;
        default: return 0;
    }
}

/*----------------------------------------------------------------------
|   AacDecoderConfig_GetSampleRate
+---------------------------------------------------------------------*/
static unsigned int
AacDecoderConfig_GetSampleRate(const AacDecoderConfig* config)
{
    return config->sampling_frequency_index <= BLT_AAC_MAX_SAMPLING_FREQUENCY_INDEX ?
        AacSamplingFreqTable[config->sampling_frequency_index]:config->sampling_frequency;
}

/*----------------------------------------------------------------------
|   AacDecoderInput_PutPacket add by junp.zhang 2011.4.22
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                          BLT_MediaPacket*    packet)
{
    AacDecoder* self = ATX_SELF_M(input, AacDecoder, BLT_PacketConsumer);
    ATX_Result  result;
	ATX_UInt8 *packet_ptr = NULL;
	ATX_UInt32 packet_size = 0;
	ATX_UInt32 out_size = RING_ALLOCATE_SIZE;
	ATX_UInt8 pcm_buf[RING_ALLOCATE_SIZE];
	BLT_UInt32 eat_size = 0;
	static BLT_UInt32 frame_count = 0;
	

    /* check to see if this is the end of a stream */
    if (BLT_MediaPacket_GetFlags(packet) & 
        BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
        self->input.eos = BLT_TRUE;
    }

    packet_ptr = BLT_MediaPacket_GetPayloadBuffer(packet);
	packet_size = BLT_MediaPacket_GetPayloadSize(packet);

    /* check to see if we need to create a decoder for this */
    if (first_init || seekable) {

        AacDecoderConfig             decoder_config;
        const BLT_MediaType*         media_type;
        const BLT_Mp4AudioMediaType* mp4_media_type;

        BLT_MediaPacket_GetMediaType(packet, &media_type);
        if (media_type == NULL || media_type->id != self->mp4es_type_id) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        mp4_media_type = (const BLT_Mp4AudioMediaType*)media_type;
        if (mp4_media_type->base.stream_type != BLT_MP4_STREAM_TYPE_AUDIO) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

		if(NULL != mp4_media_type){
			frame_count = mp4_media_type->frame_count;
	
		}
				
		if (BLT_FAILED(AacDecoderConfig_Parse(mp4_media_type->decoder_info, mp4_media_type->decoder_info_length, &decoder_config))) {
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        if (decoder_config.object_type != BLT_AAC_OBJECT_TYPE_AAC_LC &&
            decoder_config.object_type != BLT_AAC_OBJECT_TYPE_SBR) {
            return BLT_ERROR_UNSUPPORTED_CODEC;
        }
		
		
		if((hlib = dlopen("/system/lib/libaacdec.so", RTLD_NOW))== NULL){
			perror("dlopen");
			exit(1);
		}
		audec = (audec_interface_t *)dlsym(hlib, "gp_aud_dec_api");
		if(audec == NULL){
			perror("dlsym");
			exit(1);
		}

		ATX_CopyMemory(es_buf, (char *)packet_ptr, packet_size);
		adp.codec_id = CODEC_ID_AAC;
		adp.RingWI = packet_size;
		adp.sample_rate = AacDecoderConfig_GetSampleRate(&decoder_config);
		adp.channels = AacDecoderConfig_GetChannelCount(&decoder_config);	
		adp.Ring = es_buf; 
		adp.RingSize = RING_ALLOCATE_SIZE;
		adp.RingRI = 0;
		adp.hDecoder = ATX_AllocateZeroMemory(audec->instance_size());
		if(adp.hDecoder == NULL){
			//DEBUG0("adp.hDecoder is NULL\n");
			return BLT_FAILURE;
		}
		ringCnt += packet_size; 
				
		/*init decoder*/
		result = audec->init(&adp);
		if (BLT_FAILED(result)) {
			//DEBUG0("init decoder fail\n");
			return BLT_FAILURE;
		}       
	}/*end of if(!flag)*/
   if(!first_init){ 		
		RefillSize = adp.RingSize - ringCnt - 1;
		if(RefillSize <= 0) {
			//DEBUG0("Ring Buffer is full\n");
			return BLT_SUCCESS;
		}
		if(RefillSize < packet_size){
			return BLT_SUCCESS;
		}else{
			RefillSize = packet_size;
		}
		if((adp.RingWI + RefillSize) >= (adp.RingSize)){
			ATX_Int32 cnt = adp.RingSize - adp.RingWI;
			ATX_CopyMemory(adp.Ring + adp.RingWI, packet_ptr, cnt);
			packet_ptr += cnt;
			adp.RingWI = 0;
			RefillSize -= cnt;
			ringCnt += cnt;
		}
		if(RefillSize){
			ATX_CopyMemory(adp.Ring + adp.RingWI, packet_ptr, RefillSize);
			adp.RingWI += RefillSize;
			ringCnt += RefillSize;
			RefillSize = 0;
			packet_ptr = NULL;
		}
	}
		
    {	
		BLT_MediaPacket* out_packet;
		BLT_UInt64		ms = 0;

	
	
		result = audec->dec(&adp, pcm_buf, &out_size);
		if (result < 0) {
			//DEBUG0("\n audec.dec fail\n");
			return BLT_FAILURE;
		}
	
		ringCnt -= result;
		
		/* create a PCM packet for the output */
		result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
											out_size,
                                            (BLT_MediaType*)&self->output.media_type,
                                            &out_packet);
		if (BLT_FAILED(result)){
			return result;
		} 
		/* copy the timestamp */
		pcm_size += out_size;
		DEBUG0("&&&&&&&&&&&&&\n");
		DEBUG0("out_size = %u \n", out_size);
		DEBUG0("pcm_size = %llu \n", pcm_size);
		ms = (double)pcm_size / (double)(out_samRate * (out_chcnt * out_bitsRate / 8)) * 1000;
		DEBUG0("ms = %llu \n", ms);
		BLT_MediaPacket_SetTimeStamp(out_packet, BLT_TimeStamp_FromMillis(ms));
			
		/*set payload size*/
		BLT_MediaPacket_SetPayloadSize(out_packet, out_size);
		BLT_MediaPacket_SetAllocatedSize(out_packet, out_size);
		
		/*copy pcm data to out_packet*/
		ATX_CopyMemory((char *)BLT_MediaPacket_GetPayloadBuffer(out_packet), pcm_buf, out_size);

		if(self->output.media_type.channel_count == 0){
			/* first time, setup our media type */
			self->output.media_type.channel_count   = adp.out_channels;
			self->output.media_type.sample_rate     = adp.out_sample_rate;
			self->output.media_type.bits_per_sample = 16;	
			self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
			self->output.media_type.channel_mask    = 0;
			
			/* update the stream info */
            if (ATX_BASE(self, BLT_BaseMediaNode).context) {
                BLT_StreamInfo stream_info;
				BLT_UInt64	bytes_per_second;
				BLT_TimeStamp timestamp;
				BLT_UInt64		duration;
                stream_info.data_type     = "MPEG-4 AAC";
                stream_info.sample_rate   = adp.out_sample_rate;
                stream_info.channel_count = adp.out_channels;

				DEBUG0("******** update the stream info********\n");
				duration = frame_count * out_size;
				DEBUG0("frame_count = %u\n", frame_count);
				timestamp = BLT_TimeStamp_FromSamples(duration, stream_info.sample_rate);
				
				duration = timestamp.seconds * 1000 + timestamp.nanoseconds / 1000000;
				stream_info.duration = duration / 4;
				DEBUG0("duration = %llu\n", duration);
				DEBUG0("stream_info.duration =%llu\n",stream_info.duration);
				//while(1);
				stream_info.mask = BLT_STREAM_INFO_MASK_DATA_TYPE    |
                                   BLT_STREAM_INFO_MASK_SAMPLE_RATE  |
                                   BLT_STREAM_INFO_MASK_CHANNEL_COUNT|
                                   BLT_STREAM_INFO_MASK_DURATION;
                BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_info);
            }
			
			/* update the packet media type */
    		BLT_MediaPacket_SetMediaType(out_packet, (BLT_MediaType*)&self->output.media_type);
		}
		
		out_samRate = adp.out_sample_rate;
		out_chcnt = adp.out_channels;
		out_bitsRate = 16;
		first_init = 0;
		seekable = 0;
		
		/* add to the output packet list */
		ATX_List_AddData(self->output.packets, out_packet);
	}

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AacDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(AacDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AacDecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AacDecoderInput, BLT_PacketConsumer)
    AacDecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AacDecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(AacDecoderInput, BLT_MediaPort)
    AacDecoderInput_GetName,
    AacDecoderInput_GetProtocol,
    AacDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AacDecoderOutput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
AacDecoderOutput_Flush(AacDecoder* self)
{
    ATX_ListItem* item;
    while ((item = ATX_List_GetFirstItem(self->output.packets))) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) BLT_MediaPacket_Release(packet);
        ATX_List_RemoveItem(self->output.packets, item);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AacDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    AacDecoder*   self = ATX_SELF_M(output, AacDecoder, BLT_PacketProducer);
    ATX_ListItem* packet_item;
    
    /* default return */
    *packet = NULL;

    /* check if we have a packet available */
    packet_item = ATX_List_GetFirstItem(self->output.packets);

    if (packet_item) {
        *packet = (BLT_MediaPacket*)ATX_ListItem_GetData(packet_item);
        ATX_List_RemoveItem(self->output.packets, packet_item);
				return BLT_SUCCESS; 
    }

    return BLT_ERROR_PORT_HAS_NO_DATA;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AacDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(AacDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AacDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AacDecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AacDecoderOutput, BLT_MediaPort)
    AacDecoderOutput_GetName,
    AacDecoderOutput_GetProtocol,
    AacDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AacDecoderOutput, BLT_PacketProducer)
    AacDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AacDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
AacDecoder_SetupPorts(AacDecoder* self, BLT_MediaTypeId mp4es_type_id)
{
    ATX_Result result;

    /* init the input port */
    self->mp4es_type_id = mp4es_type_id;
    self->input.eos = BLT_FALSE;

    /* create a list of input packets */
    result = ATX_List_Create(&self->output.packets);
    if (ATX_FAILED(result)) return result;
    
    /* setup the output port */
    BLT_PcmMediaType_Init(&self->output.media_type);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AacDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
AacDecoder_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    AacDecoder*       self;
    AacDecoderModule* aac_decoder_module = (AacDecoderModule*)module;
    BLT_Result        result;

    ATX_LOG_FINE("AacDecoder::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(AacDecoder));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* setup the input and output ports */
    result = AacDecoder_SetupPorts(self, aac_decoder_module->mp4es_type_id);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, AacDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, AacDecoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  AacDecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  AacDecoderInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, AacDecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, AacDecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AacDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AacDecoder_Destroy(AacDecoder* self)
{ 
    ATX_LOG_FINE("AacDecoder::Destroy");
	BLT_Result result;
    /* release any packet we may hold */
    AacDecoderOutput_Flush(self);
    ATX_List_Destroy(self->output.packets);
  
    /* destroy the MCPlayer decoder */
	result = audec->uninit(&adp);
	   if(BLT_FAILED(result)){
			return BLT_FAILURE;
	   }
	   if(hlib != NULL){
			dlclose(hlib);
	   }
	   
	   if(adp.hDecoder != NULL){
		ATX_FreeMemory(adp.hDecoder);
		adp.hDecoder = NULL;
		}

	   if(audec != NULL){
		 audec = NULL;
	   }
	  DEBUG0("***  aaaaa *****\n");
	   seekable = 0;
	   ATX_SetMemory(es_buf, 0, RING_ALLOCATE_SIZE);
		RefillSize = 0;
		ringCnt = 0;
		first_init = 1;
		pcm_size = 0;
	   	out_samRate = 0;
		out_chcnt = 0;
		out_bitsRate = 0;
		
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   AacDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoder_GetPortByName(BLT_MediaNode*  _self,
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
    AacDecoder* self = ATX_SELF_EX(AacDecoder, BLT_BaseMediaNode, BLT_MediaNode);

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
|    AacDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoder_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    AacDecoder* self = ATX_SELF_EX(AacDecoder, BLT_BaseMediaNode, BLT_MediaNode);
	BLT_UInt32 ms = 0;

    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);


    /* clear the eos flag */
    self->input.eos  = BLT_FALSE;

    /* flush pending input packets */
    AacDecoderOutput_Flush(self);

	/* estimate the seek point in time_stamp mode */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
        return BLT_FAILURE;
    }

	/* update info */
	self->input.eos  = BLT_FALSE;
	DEBUG0("in aacdecoder_seek pcm_size = %llu\n", pcm_size);
	pcm_size = 0;
	ms = point->time_stamp.seconds * 1000 + point->time_stamp.nanoseconds /1000000;
	pcm_size = ms / 1000  * out_samRate * (out_chcnt * out_bitsRate / 8);
	DEBUG0("in aacdecoder_seek ms = %u\n", ms);
	DEBUG0("in aacdecoder_seek out_samRate = %u\n", out_samRate);
	DEBUG0("in aacdecoder_seek out_bitsRate = %u\n", out_bitsRate);
	DEBUG0("in aacdecoder_seek out_chcnt = %u\n", out_chcnt);
	DEBUG0("in aacdecoder_seek pcm_size = %llu\n", pcm_size);
	seekable = 1;
	first_init = 1;
	

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AacDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(AacDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AacDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AacDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AacDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    AacDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AacDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   AacDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    AacDecoderModule* self = ATX_SELF_EX(AacDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*           registry;
    BLT_Result              result;

	DEBUG0("***** into AacDecoderModule_Attach*******\n");
    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the type id */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_MP4_AUDIO_ES_MIME_TYPE,
        &self->mp4es_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("AacDecoderModule::Attach (" BLT_MP4_AUDIO_ES_MIME_TYPE " = %d)", self->mp4es_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AacDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoderModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    AacDecoderModule* self = ATX_SELF_EX(AacDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
    DEBUG0("***** into AacDecoderModule_Probe*******\n");
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input and output protocols should be PACKET */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be BLT_MP4_ES_MIME_TYPE */
            if (constructor->spec.input.media_type->id != 
                self->mp4es_type_id) {
                return BLT_FAILURE;
            } else {
                /* check the object type id */
                BLT_Mp4AudioMediaType* media_type = (BLT_Mp4AudioMediaType*)constructor->spec.input.media_type;
                if (media_type->base.stream_type != BLT_MP4_STREAM_TYPE_AUDIO) return BLT_FAILURE;
                if (media_type->base.format_or_object_type_id != BLT_AAC_OBJECT_TYPE_ID_MPEG2_AAC_LC &&
                    media_type->base.format_or_object_type_id != BLT_AAC_OBJECT_TYPE_ID_MPEG4_AUDIO) {
                    return BLT_FAILURE;
                }
                if (media_type->base.format_or_object_type_id == BLT_AAC_OBJECT_TYPE_ID_MPEG4_AUDIO) {
                    /* check that this is AAC LC */
                    AacDecoderConfig decoder_config;
                    if (BLT_FAILED(AacDecoderConfig_Parse(media_type->decoder_info, media_type->decoder_info_length, &decoder_config))) {
                        return BLT_FAILURE;
                    }
                    if (decoder_config.object_type != BLT_AAC_OBJECT_TYPE_AAC_LC &&
                        decoder_config.object_type != BLT_AAC_OBJECT_TYPE_SBR) {
                        return BLT_FAILURE;
                    }
                }
            }

            /* the output type should be unspecified, or audio/pcm */
            if (!(constructor->spec.output.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.output.media_type->id ==
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "AacDecoder")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("AacDecoderModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AacDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AacDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AacDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AacDecoderModule, AacDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AacDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    AacDecoderModule_Attach,
    AacDecoderModule_CreateInstance,
    AacDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AacDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AacDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(AacDecoderModule,
                                         "AAC Audio Decoder (Helix)",
                                         "com.axiosys.decoder.aac",
                                         "1.2.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
