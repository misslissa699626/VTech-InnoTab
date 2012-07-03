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

#include "aacdec.h"

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
    HAACDecoder       helix_decoder;
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
static const unsigned int AacSamplingFreqTable[13] =
{
	96000, 88200, 64000, 48000, 
    44100, 32000, 24000, 22050, 
    16000, 12000, 11025, 8000, 
    7350
};

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
|   AacDecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AacDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                          BLT_MediaPacket*    packet)
{
    AacDecoder* self = ATX_SELF_M(input, AacDecoder, BLT_PacketConsumer);
    ATX_Result  result;

    /* check to see if this is the end of a stream */
    if (BLT_MediaPacket_GetFlags(packet) & 
        BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
        self->input.eos = BLT_TRUE;
    }

    /* check to see if we need to create a decoder for this */
    if (self->helix_decoder == NULL) {
        AacDecoderConfig             decoder_config;
        AACFrameInfo                 aac_frame_info;
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
        if (BLT_FAILED(AacDecoderConfig_Parse(mp4_media_type->decoder_info, mp4_media_type->decoder_info_length, &decoder_config))) {
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        if (decoder_config.object_type != BLT_AAC_OBJECT_TYPE_AAC_LC &&
            decoder_config.object_type != BLT_AAC_OBJECT_TYPE_SBR) {
            return BLT_ERROR_UNSUPPORTED_CODEC;
        }
        
        /* create the decoder */
        self->helix_decoder = AACInitDecoder();
        if (self->helix_decoder == NULL) return BLT_ERROR_OUT_OF_MEMORY;

        /* configure the decoder */
        ATX_SetMemory(&aac_frame_info, 0, sizeof(aac_frame_info));
        aac_frame_info.nChans       = AacDecoderConfig_GetChannelCount(&decoder_config);
        aac_frame_info.sampRateCore = AacDecoderConfig_GetSampleRate(&decoder_config);
        if (decoder_config.object_type == BLT_AAC_OBJECT_TYPE_AAC_LC) {
            aac_frame_info.profile = AAC_PROFILE_LC;
        }
        self->sample_buffer_size = BLT_AAC_FRAME_SIZE*2*aac_frame_info.nChans*2; /* the last *2 is for SBR support */
        AACSetRawBlockParams(self->helix_decoder, 0, &aac_frame_info);        
    }

    {
        unsigned char*   in_buffer;
        int              in_size;
        short*           out_buffer;
        BLT_MediaPacket* out_packet;
        AACFrameInfo     aac_frame_info;
        
        /* create a PCM packet for the output */
        result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                            self->sample_buffer_size,
                                            (BLT_MediaType*)&self->output.media_type,
                                            &out_packet);
        if (BLT_FAILED(result)) return result;

        /* copy the timestamp */
        BLT_MediaPacket_SetTimeStamp(out_packet, BLT_MediaPacket_GetTimeStamp(packet));

        /* decode the packet as a frame */
        in_buffer  = BLT_MediaPacket_GetPayloadBuffer(packet);
        in_size    = BLT_MediaPacket_GetPayloadSize(packet);
        out_buffer = (short*)BLT_MediaPacket_GetPayloadBuffer(out_packet);
        result = AACDecode(self->helix_decoder, &in_buffer, &in_size, out_buffer); 
        if (result != 0) {
            BLT_MediaPacket_Release(out_packet);
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }

        /* check that the sample buffer matches our current media type */
        AACGetLastFrameInfo(self->helix_decoder, &aac_frame_info);
        if (self->output.media_type.channel_count == 0) {
            /* first time, setup our media type */
            self->output.media_type.channel_count   = aac_frame_info.nChans;
            self->output.media_type.sample_rate     = aac_frame_info.sampRateOut;
            self->output.media_type.bits_per_sample = 16;
            self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
            self->output.media_type.channel_mask    = 0;

            /* update the stream info */
            if (ATX_BASE(self, BLT_BaseMediaNode).context) {
                BLT_StreamInfo stream_info;
                stream_info.data_type     = "MPEG-4 AAC";
                stream_info.sample_rate   = aac_frame_info.sampRateOut;
                stream_info.channel_count = aac_frame_info.nChans;
                stream_info.mask = BLT_STREAM_INFO_MASK_DATA_TYPE    |
                                   BLT_STREAM_INFO_MASK_SAMPLE_RATE  |
                                   BLT_STREAM_INFO_MASK_CHANNEL_COUNT;
                BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_info);
            }

            /* update the packet media type */
            BLT_MediaPacket_SetMediaType(out_packet, (BLT_MediaType*)&self->output.media_type);
        } else {
            /* we've already setup a media type, check that this is the same */
            if (self->output.media_type.sample_rate   != (unsigned int)aac_frame_info.sampRateOut || 
                self->output.media_type.channel_count != aac_frame_info.nChans) {
                BLT_MediaPacket_Release(out_packet);
                return BLT_ERROR_INVALID_MEDIA_FORMAT;
            }
        }

        /* add to the output packet list */
        BLT_MediaPacket_SetPayloadSize(out_packet, aac_frame_info.outputSamps*2);
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

    /* release any packet we may hold */
    AacDecoderOutput_Flush(self);
    ATX_List_Destroy(self->output.packets);
    
    /* destroy the Melo decoder */
    if (self->helix_decoder) AACFreeDecoder(self->helix_decoder);
    
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

    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);
    
    /* clear the eos flag */
    self->input.eos  = BLT_FALSE;

    /* remove any packets in the output list */
    AacDecoderOutput_Flush(self);

    /* reset the decoder */
    if (self->helix_decoder) AACFlushCodec(self->helix_decoder);

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
