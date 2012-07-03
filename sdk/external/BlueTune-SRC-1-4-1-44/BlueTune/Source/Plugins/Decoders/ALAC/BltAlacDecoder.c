/*****************************************************************
|
|   BlueTune - ALAC Decoder Module
|
|   This code is based on the original code from
|   David Hammerton.
|
|   The rest of the code is
|   Copyright (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltAlacDecoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"
#include "BltBitStream.h"
#include "BltCommonMediaTypes.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.alac")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 iso_base_es_type_id;
} AlacDecoderModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    BLT_BitStream bits;
    BLT_Boolean   eos;
} AlacDecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    ATX_List*        packets;
    BLT_PcmMediaType media_type;
} AlacDecoderOutput;

typedef struct {
    ATX_UInt32 samples_per_frame;
    ATX_UInt8  bits_per_sample;
    ATX_UInt8  rice_history_mult;
    ATX_UInt8  rice_initial_history;
    ATX_UInt8  rice_k_scale;
    ATX_UInt8  channel_count; 
    ATX_UInt32 max_frame_size;
    ATX_UInt32 bitrate;
    ATX_UInt32 sample_rate;
} AlacDecoderConfig;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    AlacDecoderInput  input;
    AlacDecoderOutput output;
    AlacDecoderConfig config;
    struct {
        int* prediction_errors[2];
        int* samples[2];
    } buffers;
} AlacDecoder;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_ALAC_FORMAT_ID 0x616c6163  /* 'alac' */
#define BLT_ALAC_MAX_SAMPLES_PER_FRAME  65536
#define BLT_ALAC_MAX_FRAME_SIZE         65536

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AlacDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(AlacDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AlacDecoder, ATX_Referenceable)

/*----------------------------------------------------------------------
|   AlacDecoder_Configure
+---------------------------------------------------------------------*/
static BLT_Result
AlacDecoder_Configure(AlacDecoder*         self,
                      const unsigned char* decoder_info,
                      unsigned int         decoder_info_length)
{
    if (decoder_info_length < 28) return BLT_ERROR_INVALID_PARAMETERS;
    
    decoder_info += 4; /* skip first 4 bytes (could be version/flags) */
    
    self->config.samples_per_frame = ATX_BytesToInt32Be(decoder_info);
    if (self->config.samples_per_frame > BLT_ALAC_MAX_SAMPLES_PER_FRAME) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    decoder_info += 4;
    
    decoder_info += 1; /* skip unknown byte */
    
    self->config.bits_per_sample = *decoder_info;
    decoder_info += 1;
    
    self->config.rice_history_mult = *decoder_info;
    decoder_info += 1;
    
    self->config.rice_initial_history = *decoder_info;
    decoder_info += 1;
    
    self->config.rice_k_scale = *decoder_info;
    decoder_info += 1;
    
    self->config.channel_count = *decoder_info;
    decoder_info += 1;
    
    decoder_info += 2; /* skip 2 unknown bytes */
    
    self->config.max_frame_size = ATX_BytesToInt32Be(decoder_info);
    decoder_info += 4;
    
    self->config.bitrate = ATX_BytesToInt32Be(decoder_info);
    decoder_info += 4;
    
    self->config.sample_rate = ATX_BytesToInt32Be(decoder_info);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AlacDecoder_AllocateBuffers
+---------------------------------------------------------------------*/
static void
AlacDecoder_AllocateBuffers(AlacDecoder* self)
{
    /* allocate buffers (max 2 channels for now) */
    unsigned int i;
    for (i=0; i<2; i++) {
        self->buffers.prediction_errors[i] = 
            (ATX_Int32*)ATX_AllocateMemory(sizeof(ATX_Int32)*self->config.samples_per_frame);
        self->buffers.samples[i] = 
            (ATX_Int32*)ATX_AllocateMemory(sizeof(ATX_Int32)*self->config.samples_per_frame);
    }
}

/*----------------------------------------------------------------------
|   AlacDecoder_FreeBuffers
+---------------------------------------------------------------------*/
static void
AlacDecoder_FreeBuffers(AlacDecoder* self)
{
    /* free buffers (max 2 channels for now) */
    unsigned int i;
    for (i=0; i<2; i++) {
        ATX_FreeMemory(self->buffers.prediction_errors[i]);
        ATX_FreeMemory(self->buffers.samples[i]);
    }
}

/*----------------------------------------------------------------------
|   AlacDecoder_CLZ
+---------------------------------------------------------------------*/
#if defined(__GNUC__) && defined(__i386__)
static inline int 
AlacDecoder_CLZ(int x)
{
    int lz = 0;
    if (x == 0) return 32;
    asm("bsr %1, %0\n"
        : "=&r" (lz)
        : "r" (x));
    return (31 - lz);
}
#elif defined(_MSC_VER) && defined(_M_IX86)
static __inline int 
AlacDecoder_CLZ(int x)
{
    int lz;
    if (x == 0) return 32;
    __asm
    {
        mov eax, x;
        mov edx, 31;
        bsr ecx, eax;
        sub edx, ecx;
        mov lz, edx;
    }
    return lz;
}
#else
static const unsigned char
AlacDecoder_LZTable[256] = 
{
8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static inline int 
AlacDecoder_CLZ(unsigned int x)
{
    int lz = 0;
    unsigned int b;
    
    b = x>>24;
    if (b) goto nonzero;
    lz += 8;

    b = x>>16;
    if (b) goto nonzero;
    lz += 8;

    b = x>>8;
    if (b) goto nonzero;
    lz += 8;

    b = x;
    if (b) goto nonzero;

    return 32;

nonzero:
    return lz+AlacDecoder_LZTable[b&0xFF];
}
#endif

/*----------------------------------------------------------------------
|   AlacDecoder_DecompressRiceCode
+---------------------------------------------------------------------*/
static void 
AlacDecoder_DecompressRiceCode(AlacDecoder* self,
                               ATX_Int32*   out,
                               unsigned int sample_count,
                               unsigned int sample_size, 
                               ATX_Int32    rice_initial_history, 
                               ATX_Int32    rice_k_scale, 
                               ATX_Int32    rice_history_mult)
{
    BLT_BitStream* bits = &self->input.bits;
    ATX_Int32      history = rice_initial_history;
    ATX_Int32      sign_modifier = 0;
    unsigned int   i;
    
    for (i=0; i<sample_count; i++) {
        ATX_Int32 x = 0;

        /* count leading 1s up to 8 bits */
        while (x <= 8 && BLT_BitStream_ReadBit(bits)) {
            ++x;
        }

        if (x == 9) {
            /* escaped value: the value is encoded as-is */
            x = BLT_BitStream_ReadBits(bits, sample_size);
        } else {
            int k;

            k = 31 - AlacDecoder_CLZ((history >> 9) + 3) - rice_k_scale;
            if (k < 0) {
                k += rice_k_scale;
            } else {
                k = rice_k_scale;
            }
            
            /* the rice multiplier is (2^k-1) */
            if (k != 1) {
                /* read the remainder */
                unsigned int remainder = BLT_BitStream_ReadBits(bits, k-1);

                /* multiply x by 2^k - 1 */
                x = (x << k) - x;

                if (remainder) {
                    /* read one more remainder bit */
                    remainder = (remainder<<1) + BLT_BitStream_ReadBit(bits);
                    
                    /* add the remainder to the multiple */
                    x += remainder - 1;
                }
            }
        }

        /* negative and positive values are interlaced: non-negative values */
        /* are mapped to even numbers, and negative values to odd numbers.  */
        {
            ATX_Int32 absolute_value;
            x += sign_modifier;
            absolute_value = (x+1)/2;
            out[i] = (x&1) ? -absolute_value : absolute_value;
        }
        sign_modifier = 0;

        /* update the history. the history keeps a sort of running average  */
        /* of the values. this is used to update the rice multiplier and to */
        /* detect when 0 values are frequent enough to encode a run of 0s   */
        if (x > 0xFFFF) {
            /* past this threshold, just clamp the value */
            history = 0xFFFF;
        } else {
            history += (x * rice_history_mult) - ((history * rice_history_mult) >> 9);
        }

        /* the history value has dropped below the threshold. this indicates */
        /* that the probability of a run of 0s is high, so the next bits     */
        /* encode a (possibly empty) run of 0s.                              */
        if ((history < 128) && (i+1 < sample_count)) {
            int run_length;

            /* count leading 1s up to 8 bits */
            x = 0;
            while (x <= 8 && BLT_BitStream_ReadBit(bits)) {
                ++x;
            }

            if (x == 9) {
                /* escaped value: the run length is encoded as-is */
                run_length = BLT_BitStream_ReadBits(bits, 16);
            } else {
                unsigned int k;
                unsigned int remainder;
                
                k = AlacDecoder_CLZ(history)-24 + ((history + 16) >> 6);
                run_length = (((1 << k) - 1) & ((1<<rice_k_scale)-1)) * x;

                /* read the remainder (k is always >= 2) */
                remainder = BLT_BitStream_ReadBits(bits, k-1);
                if (remainder) {
                    /* read one more remainder bit */
                    remainder = (remainder<<1) + BLT_BitStream_ReadBit(bits);
                    run_length += remainder-1;
                }
            }

            if (run_length+i > sample_count) {
                /* something is wrong: too many 0s */
                run_length = sample_count-i;
            }
            if (run_length > 0) {
                ATX_SetMemory(&out[i+1], 0, run_length * sizeof(out[0]));
                i += run_length;
            }

            if (run_length > 0xFFFF) {
                sign_modifier = 0;
            } else {
                sign_modifier = 1;
            }

            history = 0;
        }
    }
}

/*----------------------------------------------------------------------
|   local macros
+---------------------------------------------------------------------*/
#define BLT_ALAC_EXTEND_SIGN_32(x, bits) \
((((ATX_Int32)(x)) << (32 - bits)) >> (32 - bits))

/*----------------------------------------------------------------------
|   AlacDecoder_ApplyPredictor
+---------------------------------------------------------------------*/
static void 
AlacDecoder_ApplyPredictor(const ATX_Int32* in,
                           ATX_Int32*       out,
                           unsigned int     sample_count,
                           ATX_Int16*       predictor_coef_table,
                           unsigned int     predictor_coef_count,
                           unsigned int     predictor_quantization)
{
    unsigned int i;

    /* special case when there are not coefficients */
    if (predictor_coef_count == 0) {
        ATX_CopyMemory(out, in, sample_count * sizeof(*in));
        return;
    }

    if (predictor_coef_count == 31) { 
        /* delta encoding (the coefs seem to be ignored in this case) */
        ATX_Int32 previous_value = 0;
        for (i = 0; i < sample_count; i++) {
            out[i] = previous_value+in[i];
            previous_value = in[i];
        }
        return;
    }

    /* compute the initial values */
    out[0] = in[0];
    for (i = 1; i <= predictor_coef_count; i++) {
        out[i] = out[i-1]+in[i];
    }

    /* apply the filter to the rest of the samples */
    for (; i < sample_count; i++) {
        ATX_Int32    sample = 0;
        ATX_Int32    error  = in[i];
        unsigned int j;

        /* compute the convolution */
        for (j = 0; j < predictor_coef_count; j++) {
            sample += (out[predictor_coef_count-j] - out[0]) * predictor_coef_table[j];
        }

        /* round to the nearest quantized value */
        sample = ((1 << (predictor_quantization-1)) + sample) >> predictor_quantization;
        
        /* reconstruct the original sample */
        sample += out[0] + error;

        /* emmit the sample */
        out[predictor_coef_count+1] = sample;

        /* adapt the filter coefficients */
        {
            unsigned int p;
            if (error > 0) {
                for (p = 1; p <= predictor_coef_count && error > 0; p++) {
                    int diff = out[0] - out[p];
                    if (diff > 0) {
                        predictor_coef_table[predictor_coef_count-p]--;
                        error -= (diff >> predictor_quantization)*(int)p;
                    } else if (diff < 0) {
                        predictor_coef_table[predictor_coef_count-p]++;
                        error -= ((-diff) >> predictor_quantization)*(int)p;
                    }
                }
            } else if (error < 0) {
                for (p = 1; p <= predictor_coef_count && error < 0; p++) {
                    int diff = out[0] - out[p];
                    if (diff > 0) {
                        predictor_coef_table[predictor_coef_count-p]++;
                        error -= ((-diff) >> predictor_quantization)*(int)p;
                    } else if (diff < 0) {
                        predictor_coef_table[predictor_coef_count-p]--;
                        error -= (diff >> predictor_quantization)*(int)p;
                    }
                }
            }
        }
        
        out++;
    }
}


/*----------------------------------------------------------------------
|   AlacDecoder_ProcessMono
+---------------------------------------------------------------------*/
static void 
AlacDecoder_ProcessMono(ATX_Int32*   samples, 
                        unsigned int sample_count,
                        ATX_Int16*   out)
{
    /* convert all samples to the output format */
    while (sample_count--) {
        *out++ = *samples++;
    }
}

/*----------------------------------------------------------------------
|   AlacDecoder_ProcessStero
+---------------------------------------------------------------------*/
static void 
AlacDecoder_ProcessStereo(ATX_Int32*   left, 
                          ATX_Int32*   right,
                          unsigned int sample_count,
                          unsigned int mid_side_scale,
                          int          mid_side_weight,
                          ATX_Int16*   out)
{
    if (mid_side_weight) {
        /* here, the left and right channels contain: */
        /* mid  = L*w+R*(1-w)                         */
        /* side = L-R                                 */
        /* where w = mid_side_weight>>mid_side_scale  */
        while (sample_count--) {
            int mid, side, L, R;

            mid  = *left++;
            side = *right++;

            R = mid - ((side * mid_side_weight) >> mid_side_scale);
            L  = R + side;

            *out++ = L;
            *out++ = R;
        }
    } else {
        while (sample_count--) {
            *out++ = *left++;
            *out++ = *right++;
        }
    }
}

/*----------------------------------------------------------------------
|   AlacDecoder_DecodeFrame
+---------------------------------------------------------------------*/
static BLT_Result
AlacDecoder_DecodeFrame(AlacDecoder* self, BLT_MediaPacket* out)
{
    BLT_BitStream* bits = &self->input.bits;
    unsigned int   channel_count;
    unsigned int   sample_count = self->config.samples_per_frame;
    BLT_Boolean    short_frame;
    unsigned int   sample_padding;
    BLT_Boolean    uncompressed;
    unsigned int   sample_size;

    channel_count  = BLT_BitStream_ReadBits(bits, 3)+1;
                     BLT_BitStream_SkipBits(bits, 16);
    short_frame    = BLT_BitStream_ReadBit(bits);
    sample_padding = BLT_BitStream_ReadBits(bits, 2);
    uncompressed   = BLT_BitStream_ReadBit(bits);
    if (short_frame) {
        /* this is a short frame, the sample count is encoded on 32 bits */
        sample_count = BLT_BitStream_ReadBits(bits, 32);
    }
    
    /* we only support 1 or 2 channels */
    if (channel_count > 2) return BLT_ERROR_NOT_SUPPORTED;
    
    /* check that the padding is not larger than the sample size */
    if (sample_padding*8 >= self->config.bits_per_sample) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    sample_size = self->config.bits_per_sample - (sample_padding * 8) + (channel_count-1);

    if (uncompressed) {
        unsigned int i;
        for (i=0; i<sample_count; i++) {
            unsigned int c;
            for (c=0; c<channel_count; c++) {
                ATX_Int32 sample = BLT_BitStream_ReadBits(bits, sample_size);
                self->buffers.samples[c][i] = BLT_ALAC_EXTEND_SIGN_32(sample, sample_size);
            }
        }
    } else {
        unsigned int mid_side_scale;
        unsigned int mid_side_weight;
        ATX_Int16    predictor_coef_table[2][32];
        unsigned int predictor_coef_count[2];
        unsigned int prediction_type[2];
        unsigned int prediction_quantitization[2];
        unsigned int rice_modifier[2];
        unsigned int i;
        unsigned int c;
        
        /* read the mid-side scale and weight */
        mid_side_scale  = BLT_BitStream_ReadBits(bits, 8);
        mid_side_weight = BLT_BitStream_ReadBits(bits, 8); 

        for (c=0; c<channel_count; c++) {
            prediction_type[c]           = BLT_BitStream_ReadBits(bits, 4);
            prediction_quantitization[c] = BLT_BitStream_ReadBits(bits, 4);
            rice_modifier[c]             = BLT_BitStream_ReadBits(bits, 3);
            predictor_coef_count[c]      = BLT_BitStream_ReadBits(bits, 5);

            /* read the predictor table */
            for (i = 0; i < predictor_coef_count[c]; i++) {
                predictor_coef_table[c][i] = (ATX_Int16)BLT_BitStream_ReadBits(bits, 16);
            }
        }
        
        for (c=0; c<channel_count; c++) {
            /* decompress the rice codes */
            AlacDecoder_DecompressRiceCode(self,
                                           self->buffers.prediction_errors[c],
                                           sample_count,
                                           sample_size,
                                           self->config.rice_initial_history,
                                           self->config.rice_k_scale,
                                           rice_modifier[c] * self->config.rice_history_mult / 4);

            if (prediction_type[c] == 0) { 
                /* adaptive filter */
                AlacDecoder_ApplyPredictor(self->buffers.prediction_errors[c],
                                           self->buffers.samples[c],
                                           sample_count,
                                           predictor_coef_table[c],
                                           predictor_coef_count[c],
                                           prediction_quantitization[c]);
            } else {
                /* unsupported predictor type */
                return BLT_ERROR_NOT_SUPPORTED;
            }
        }
        
        {
            /*unsigned int in_bytes_per_sample  = (self->config.bits_per_sample+7)/8;*/
            /*unsigned int out_bytes_per_sample = in_bytes_per_sample<=16 ? 16 : 32;*/
        
            BLT_MediaPacket_SetPayloadSize(out, 
                                           sample_count*channel_count * 
                                           2 /*out_bytes_per_sample*/);
            if (channel_count == 1) {
                AlacDecoder_ProcessMono(self->buffers.samples[0],
                                        sample_count,
                                        /*in_bytes_per_sample,
                                        out_bytes_per_sample,*/
                                        BLT_MediaPacket_GetPayloadBuffer(out));
            } else {
                AlacDecoder_ProcessStereo(self->buffers.samples[0],
                                          self->buffers.samples[1],
                                          sample_count,
                                          mid_side_scale,
                                          mid_side_weight,
                                          /*in_bytes_per_sample,
                                          out_bytes_per_sample,*/
                                          BLT_MediaPacket_GetPayloadBuffer(out));
            }
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AlacDecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                           BLT_MediaPacket*    in_packet)
{
    AlacDecoder*     self = ATX_SELF_M(input, AlacDecoder, BLT_PacketConsumer);
    BLT_MediaPacket* out_packet;
    ATX_Result       result;

    /* check to see if this is the end of a stream */
    if (BLT_MediaPacket_GetFlags(in_packet) & 
        BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
        self->input.eos = BLT_TRUE;
    }

    /* copy the packet into the input bitstream */
    if (BLT_MediaPacket_GetPayloadSize(in_packet) > self->input.bits.buffer_size) {
        /* reallocate the bit stream buffer */
        BLT_BitStream_Destruct(&self->input.bits);
        BLT_BitStream_Construct(&self->input.bits, BLT_MediaPacket_GetPayloadSize(in_packet));
    }
    BLT_BitStream_SetData(&self->input.bits, 
                          BLT_MediaPacket_GetPayloadBuffer(in_packet),
                          BLT_MediaPacket_GetPayloadSize(in_packet));

    /* create a packet for the output */
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        0,
                                        (BLT_MediaType*)&self->output.media_type,
                                        &out_packet);
    if (BLT_FAILED(result)) return result;

    /* decode the packet as a frame */
    result = AlacDecoder_DecodeFrame(self, out_packet); 
    if (BLT_FAILED(result)) {
        BLT_MediaPacket_Release(out_packet);
        return BLT_ERROR_PORT_HAS_NO_DATA;
    }

    /* copy the timestamp and set flags */
    BLT_MediaPacket_SetTimeStamp(out_packet, BLT_MediaPacket_GetTimeStamp(in_packet));
    if (self->input.eos) {
        BLT_MediaPacket_SetFlags(out_packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
    }
    
    /* add to the output packet list */
    ATX_List_AddData(self->output.packets, out_packet);

    return BLT_SUCCESS;
}

/*---------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlacDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(AlacDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AlacDecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AlacDecoderInput, BLT_PacketConsumer)
    AlacDecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AlacDecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(AlacDecoderInput, BLT_MediaPort)
    AlacDecoderInput_GetName,
    AlacDecoderInput_GetProtocol,
    AlacDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AlacDecoderOutput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
AlacDecoderOutput_Flush(AlacDecoder* self)
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
|   AlacDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                            BLT_MediaPacket**   packet)
{
    AlacDecoder*   self = ATX_SELF_M(output, AlacDecoder, BLT_PacketProducer);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlacDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(AlacDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AlacDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AlacDecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AlacDecoderOutput, BLT_MediaPort)
    AlacDecoderOutput_GetName,
    AlacDecoderOutput_GetProtocol,
    AlacDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AlacDecoderOutput, BLT_PacketProducer)
    AlacDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AlacDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
AlacDecoder_SetupPorts(AlacDecoder* self)
{
    ATX_Result result;

    /* init the input port */
    BLT_BitStream_Construct(&self->input.bits, 
                            self->config.max_frame_size < 32768 ?
                            self->config.max_frame_size : 0);
    self->input.eos = BLT_FALSE;

    /* create a list of input packets */
    result = ATX_List_Create(&self->output.packets);
    if (ATX_FAILED(result)) return result;
    
    /* setup the output port */
    BLT_PcmMediaType_Init(&self->output.media_type);
    self->output.media_type.channel_count   = self->config.channel_count;
    self->output.media_type.sample_rate     = self->config.sample_rate;
    self->output.media_type.bits_per_sample = self->config.bits_per_sample;
    self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
    self->output.media_type.channel_mask    = 0;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlacDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
AlacDecoder_Create(BLT_Module*              module,
                   BLT_Core*                core, 
                   BLT_ModuleParametersType parameters_type,
                   BLT_CString              parameters, 
                   BLT_MediaNode**          object)
{
    AlacDecoder*              self;
    BLT_Mp4AudioMediaType*    media_type;
    BLT_MediaNodeConstructor* constructor;
    BLT_Result                result;

    ATX_LOG_FINE("AlacDecoder::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }
    constructor = (BLT_MediaNodeConstructor*)parameters;
                
    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(AlacDecoder));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* configure the decoder base on the media type details */
    media_type = (BLT_Mp4AudioMediaType*)constructor->spec.input.media_type;
    if (BLT_FAILED(AlacDecoder_Configure(self,
                                         media_type->decoder_info, 
                                         media_type->decoder_info_length))) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    
    /* setup the input and output ports */
    result = AlacDecoder_SetupPorts(self);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }
    
    /* check that we support all the parameters */
    if (self->config.channel_count > 2 ||
        self->config.bits_per_sample != 16) {
        return BLT_ERROR_UNSUPPORTED_CODEC;
    }

    /* allocate decoding buffers */
    AlacDecoder_AllocateBuffers(self);
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, AlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, AlacDecoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  AlacDecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  AlacDecoderInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, AlacDecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, AlacDecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlacDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AlacDecoder_Destroy(AlacDecoder* self)
{ 
    ATX_LOG_FINE("AlacDecoder::Destroy");

    /* release any packet we may hold */
    AlacDecoderOutput_Flush(self);
    ATX_List_Destroy(self->output.packets);
    
    /* free the buffers */
    AlacDecoder_FreeBuffers(self);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free resources */
    BLT_BitStream_Destruct(&self->input.bits);
    
    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   AlacDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoder_GetPortByName(BLT_MediaNode*  _self,
                          BLT_CString     name,
                          BLT_MediaPort** port)
{
    AlacDecoder* self = ATX_SELF_EX(AlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);

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
|    AlacDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoder_Seek(BLT_MediaNode* _self,
                 BLT_SeekMode*  mode,
                 BLT_SeekPoint* point)
{
    AlacDecoder* self = ATX_SELF_EX(AlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);

    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);
    
    /* clear the eos flag */
    self->input.eos  = BLT_FALSE;

    /* remove any packets in the output list */
    AlacDecoderOutput_Flush(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlacDecoder_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoder_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    AlacDecoder*   self = ATX_SELF_EX(AlacDecoder, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_StreamInfo stream_info;

    ATX_BASE(self, BLT_BaseMediaNode).context = stream;
    
    stream_info.data_type       = "Apple Lossless Audio";
    stream_info.sample_rate     = self->config.sample_rate;
    stream_info.channel_count   = self->config.channel_count;
    stream_info.nominal_bitrate = self->config.bitrate;
    stream_info.average_bitrate = self->config.bitrate;
    stream_info.mask = BLT_STREAM_INFO_MASK_DATA_TYPE       |
                       BLT_STREAM_INFO_MASK_SAMPLE_RATE     |
                       BLT_STREAM_INFO_MASK_CHANNEL_COUNT   |
                       BLT_STREAM_INFO_MASK_NOMINAL_BITRATE |
                       BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
    BLT_Stream_SetInfo(stream, &stream_info);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlacDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(AlacDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AlacDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AlacDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AlacDecoder_GetPortByName,
    AlacDecoder_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    AlacDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AlacDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   AlacDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    AlacDecoderModule* self = ATX_SELF_EX(AlacDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*           registry;
    BLT_Result              result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the type id */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_ISO_BASE_AUDIO_ES_MIME_TYPE,
        &self->iso_base_es_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("AlacDecoderModule::Attach (" BLT_ISO_BASE_AUDIO_ES_MIME_TYPE " = %d)", 
                   self->iso_base_es_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AlacDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AlacDecoderModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    AlacDecoderModule* self = ATX_SELF_EX(AlacDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
    
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input and output protocols should be PACKET */
            if ((constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be BLT_ISO_BASE_ES_MIME_TYPE */
            if (constructor->spec.input.media_type->id != self->iso_base_es_type_id) {
                return BLT_FAILURE;
            } else {
                /* check the format */
                BLT_Mp4MediaType* media_type = (BLT_Mp4MediaType*)constructor->spec.input.media_type;
                if (media_type->stream_type != BLT_MP4_STREAM_TYPE_AUDIO ||
                    media_type->format_or_object_type_id != BLT_ALAC_FORMAT_ID) {
                    return BLT_FAILURE;
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
                if (ATX_StringsEqual(constructor->name, "AlacDecoder")) {
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

            ATX_LOG_FINE_1("AlacDecoderModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlacDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AlacDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AlacDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AlacDecoderModule, AlacDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AlacDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    AlacDecoderModule_Attach,
    AlacDecoderModule_CreateInstance,
    AlacDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AlacDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AlacDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(AlacDecoderModule,
                                         "Apple Lossless Audio Decoder",
                                         "com.axiosys.decoder.alac",
                                         "1.2.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
