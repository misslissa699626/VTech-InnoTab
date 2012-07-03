/*****************************************************************
|
|   BlueTune - PCM Utilities
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune PCM Implementation file
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltPcm.h"

/*----------------------------------------------------------------------
|   global constants
+---------------------------------------------------------------------*/
const BLT_MediaType BLT_GenericPcmMediaType = {
    BLT_MEDIA_TYPE_ID_AUDIO_PCM, /* id             */
    0,                           /* flags          */
    0                            /* extension size */
};

/*----------------------------------------------------------------------
|   BLT_PcmMediaType_Init
+---------------------------------------------------------------------*/
void
BLT_PcmMediaType_Init(BLT_PcmMediaType* media_type)
{
    media_type->base.id = BLT_MEDIA_TYPE_ID_AUDIO_PCM;
    media_type->base.flags = 0;
    media_type->base.extension_size = sizeof(BLT_PcmMediaType)-sizeof(BLT_MediaType);
    media_type->bits_per_sample = 0;
    media_type->channel_count   = 0;
    media_type->channel_mask    = 0;
    media_type->sample_format   = 0;
    media_type->sample_rate     = 0;
}


#if BLT_CONFIG_CPU_BYTE_ORDER == BLT_CPU_LITTLE_ENDIAN
/*----------------------------------------------------------------------
|   BLT_Pcm_ReadSignedIntBE
+---------------------------------------------------------------------*/
static BLT_Int32
BLT_Pcm_ReadSignedIntBE(const void* buffer, unsigned int width)
{
    unsigned char* x = (unsigned char*)buffer;
    BLT_Int32 sample = 0;
    unsigned char* y = ((unsigned char*)&sample)+sizeof(sample);
    while (width--) {
        *--y = *x++;
    }
    return sample;
}

/*----------------------------------------------------------------------
|   BLT_Pcm_ReadSignedIntLE
+---------------------------------------------------------------------*/
static BLT_Int32
BLT_Pcm_ReadSignedIntLE(const void* buffer, unsigned int width)
{
    unsigned char* x = ((unsigned char*)buffer)+width;
    BLT_Int32 sample = 0;
    unsigned char* y = ((unsigned char*)&sample)+sizeof(sample);
    while (width--) {
        *--y = *--x;
    }
    return sample;
}

/*----------------------------------------------------------------------
|   BLT_Pcm_WriteSignedIntBE
+---------------------------------------------------------------------*/
static void
BLT_Pcm_WriteSignedIntBE(void* buffer, BLT_Int32 sample, unsigned int width)
{
    unsigned char* x = (unsigned char*)buffer;
    unsigned char* y = ((unsigned char*)&sample)+sizeof(sample);
    while (width--) {
        *x++ = *--y;
    }
}

/*----------------------------------------------------------------------
|   BLT_Pcm_WriteSignedIntLE
+---------------------------------------------------------------------*/
static void
BLT_Pcm_WriteSignedIntLE(void* buffer, BLT_Int32 sample, unsigned int width)
{
    unsigned char* x = ((unsigned char*)buffer)+width;
    unsigned char* y = ((unsigned char*)&sample)+sizeof(sample);
    while (width--) {
        *--x = *--y;
    }
}

/*----------------------------------------------------------------------
|   BLT_Pcm_ReadFloatBE
+---------------------------------------------------------------------*/
static BLT_Int32
BLT_Pcm_ReadFloatBE(const void* buffer, unsigned int width)
{
    unsigned char* x = (unsigned char*)buffer;
    float v;
    unsigned char* y = (unsigned char*)&v;
    
    BLT_COMPILER_UNUSED(width);
    
    y[3] = x[0];
    y[2] = x[1];
    y[1] = x[2];
    y[0] = x[3];
    {
        float f = 2147483648.0f*v;
        if (f > 2147483647.0f) {
            f = 2147483647.0f;
        } else if (f < -2147483648.0f) {
            f = -2147483648.0f;
        }
        return (BLT_Int32)f;
    }
}

/*----------------------------------------------------------------------
|   BLT_Pcm_ReadFloatLE
+---------------------------------------------------------------------*/
static BLT_Int32
BLT_Pcm_ReadFloatLE(const void* buffer, unsigned int width)
{
	BLT_COMPILER_UNUSED(width);
	
    {
        float f = 2147483648.0f* (*(float*)buffer);
        if (f > 2147483647.0f) {
            f = 2147483647.0f;
        } else if (f < -2147483648.0f) {
            f = -2147483648.0f;
        }
        return (BLT_Int32)f;
    }
}

/*----------------------------------------------------------------------
|   BLT_Pcm_WriteFloatBE
+---------------------------------------------------------------------*/
static void
BLT_Pcm_WriteFloatBE(void* buffer, BLT_Int32 sample, unsigned int width)
{
    unsigned char* x = (unsigned char*)buffer;
    float f = ((float)sample)/32768.0f;
    unsigned char* y = (unsigned char*)&f;
	BLT_COMPILER_UNUSED(width);
	
    x[0] = y[3];
    x[1] = y[2];
    x[2] = y[1];
    x[3] = y[0];
}

/*----------------------------------------------------------------------
|   BLT_Pcm_WriteFloatLE
+---------------------------------------------------------------------*/
static void
BLT_Pcm_WriteFloatLE(void* buffer, BLT_Int32 sample, unsigned int width)
{
	BLT_COMPILER_UNUSED(width);
	
    *((float*)buffer) = ((float)sample)/32768.0f;
}

#else 
#error "BLT_CPU_BIG_ENDIAN not implemented yet"
#endif

/*----------------------------------------------------------------------
|   BLT_Pcm_CanConvert
+---------------------------------------------------------------------*/
BLT_Boolean
BLT_Pcm_CanConvert(const BLT_MediaType* from, const BLT_MediaType* to)
{
    const BLT_PcmMediaType* from_pcm = (const BLT_PcmMediaType*)from;
    const BLT_PcmMediaType* to_pcm   = (const BLT_PcmMediaType*)to;

    /* check that both types are PCM */
    if (from->id != BLT_MEDIA_TYPE_ID_AUDIO_PCM ||
        to->id   != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_FALSE;
    }

    /* check that the input format is valid */
    if (from_pcm->bits_per_sample == 0 ||
        from_pcm->channel_count == 0   ||
        from_pcm->sample_rate == 0) {
        return BLT_FALSE;
    }

    /* check that we support the format */
    if (from_pcm->sample_format != BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE &&
        from_pcm->sample_format != BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE &&
        from_pcm->sample_format != BLT_PCM_SAMPLE_FORMAT_FLOAT_BE      &&
        from_pcm->sample_format != BLT_PCM_SAMPLE_FORMAT_FLOAT_LE) {
        return BLT_FALSE;
    }

    /* we only support floats in 32-bit width */
    if (from_pcm->sample_format == BLT_PCM_SAMPLE_FORMAT_FLOAT_LE ||
        from_pcm->sample_format == BLT_PCM_SAMPLE_FORMAT_FLOAT_BE) {
        if (from_pcm->bits_per_sample != 32) return BLT_FALSE;
    }
    if (to_pcm->sample_format == BLT_PCM_SAMPLE_FORMAT_FLOAT_LE ||
        to_pcm->sample_format == BLT_PCM_SAMPLE_FORMAT_FLOAT_BE) {
        if (to_pcm->bits_per_sample != 32) return BLT_FALSE;
    }

    /* we do not support channel conversions yet */
    if (to_pcm->channel_count   != 0 && 
        from_pcm->channel_count != to_pcm->channel_count) {
        return BLT_FALSE;
    }

    /* we do not support sample rate conversions yet */
    if (to_pcm->sample_rate   != 0 && 
        from_pcm->sample_rate != to_pcm->sample_rate) {
        return BLT_FALSE;
    }

    return BLT_TRUE;
}

/*----------------------------------------------------------------------
|   BLT_Pcm_ConvertMediaPacket
+---------------------------------------------------------------------*/
BLT_Result
BLT_Pcm_ConvertMediaPacket(BLT_Core*         core,
                           BLT_MediaPacket*  in, 
                           BLT_PcmMediaType* out_type_spec, 
                           BLT_MediaPacket** out)
{
    const BLT_PcmMediaType* in_type;
    BLT_PcmMediaType        out_type;
    unsigned int            sample_count;
    unsigned int            packet_size;
    BLT_Int32               (*in_function)(const void* buffer, unsigned int width);
    void                    (*out_function)(void* buffer, BLT_Int32 sample, unsigned int width);
    BLT_Result              result;

    /* default */
    *out = NULL;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(in, (const BLT_MediaType**)(const void*)&in_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type */
    if (in_type->base.id  != BLT_MEDIA_TYPE_ID_AUDIO_PCM ||
        out_type_spec->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    
    /* do automatic setting of output parameters */
    out_type = *out_type_spec;
    if (out_type.bits_per_sample == 0) {
        out_type.bits_per_sample = in_type->bits_per_sample;
    }
    if (out_type.channel_count == 0) {
        out_type.channel_count = in_type->channel_count;
    }
    if (out_type.sample_rate == 0) {
        out_type.sample_rate = in_type->sample_rate;
    }

    /* select the approprite conversion routines */
    switch (in_type->sample_format) {
        case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE:
            in_function = BLT_Pcm_ReadSignedIntBE;
            break;

        case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE:
            in_function = BLT_Pcm_ReadSignedIntLE;
            break;

        case BLT_PCM_SAMPLE_FORMAT_FLOAT_BE:
            in_function = BLT_Pcm_ReadFloatBE;
            if (in_type->bits_per_sample != 32) return BLT_ERROR_INVALID_MEDIA_TYPE;
            break;

        case BLT_PCM_SAMPLE_FORMAT_FLOAT_LE:
            in_function = BLT_Pcm_ReadFloatLE;
            if (in_type->bits_per_sample != 32) return BLT_ERROR_INVALID_MEDIA_TYPE;
            break;

        default:
            return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    switch (out_type.sample_format) {
        case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE:
            out_function = BLT_Pcm_WriteSignedIntBE;
            break;

        case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE:
            out_function = BLT_Pcm_WriteSignedIntLE;
            break;

        case BLT_PCM_SAMPLE_FORMAT_FLOAT_BE:
            out_function = BLT_Pcm_WriteFloatBE;
            if (out_type.bits_per_sample != 32) return BLT_ERROR_INVALID_MEDIA_TYPE;
            break;

        case BLT_PCM_SAMPLE_FORMAT_FLOAT_LE:
            out_function = BLT_Pcm_WriteFloatLE;
            if (out_type.bits_per_sample != 32) return BLT_ERROR_INVALID_MEDIA_TYPE;
            break;

        default:
            return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* allocate the output packet */
    sample_count = BLT_MediaPacket_GetPayloadSize(in)/(in_type->bits_per_sample/8);
    packet_size = sample_count*(out_type.bits_per_sample/8);
    result = BLT_Core_CreateMediaPacket(core,
                                        packet_size,
                                        (const BLT_MediaType*)&out_type,
                                        out);
    if (BLT_FAILED(result)) return result;

    /* set the payload size */
    BLT_MediaPacket_SetPayloadSize(*out, packet_size);

    /* convert the samples */
    {
        const char*  in_buffer = (const char*)BLT_MediaPacket_GetPayloadBuffer(in);
        unsigned int in_width  = in_type->bits_per_sample/8;
        char*  out_buffer      = (char*)BLT_MediaPacket_GetPayloadBuffer(*out);
        unsigned int out_width  = out_type.bits_per_sample/8;
        while (sample_count--) {
            BLT_Int32 sample = in_function(in_buffer, in_width);
            out_function(out_buffer, sample, out_width);
            in_buffer += in_width;
            out_buffer += out_width;
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_Pcm_ParseMimeType
+---------------------------------------------------------------------*/
BLT_Result
BLT_Pcm_ParseMimeType(const char* mime_type, BLT_PcmMediaType** media_type)
{
    ATX_String main_type = ATX_EMPTY_STRING;
    ATX_List*  parameters = NULL;
    ATX_Result result;
    
    /* default */
    *media_type = NULL;
    
    /* parse the type and params */
    result = BLT_ParseMimeType(mime_type, &main_type, &parameters);
    if (BLT_FAILED(result)) goto end;
    
    /* check that this is a supported type */
    if (!ATX_String_Equals(&main_type, "audio/L16", ATX_TRUE)) {
        result = ATX_ERROR_INVALID_PARAMETERS;
        goto end;
    }
    
    /* allocate the type */
    *media_type = ATX_AllocateZeroMemory(sizeof(BLT_PcmMediaType));
    BLT_PcmMediaType_Init(*media_type);
    (*media_type)->bits_per_sample = 16;
    (*media_type)->sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE;
    
    /* parse the parameters */
    if (parameters) {
        ATX_ListItem* item = ATX_List_GetFirstItem(parameters);
        while (item) {
            unsigned int value = 0;
            BLT_MimeTypeParameter* parameter = (BLT_MimeTypeParameter*)ATX_ListItem_GetData(item);
            if (ATX_String_Equals(&parameter->name, "channels", ATX_TRUE)) {
                if (ATX_SUCCEEDED(ATX_ParseIntegerU(ATX_CSTR(parameter->value), &value, ATX_FALSE))) {
                    (*media_type)->channel_count = value;
                }
            } else if (ATX_String_Equals(&parameter->name, "rate", ATX_TRUE)) {
                if (ATX_SUCCEEDED(ATX_ParseIntegerU(ATX_CSTR(parameter->value), &value, ATX_FALSE))) {
                    (*media_type)->sample_rate = value;
                }
            }
            item = ATX_ListItem_GetNext(item);
        }
    }
    
end:
    ATX_String_Destruct(&main_type);
    if (parameters) ATX_List_Destroy(parameters);
    
    return result;
}
