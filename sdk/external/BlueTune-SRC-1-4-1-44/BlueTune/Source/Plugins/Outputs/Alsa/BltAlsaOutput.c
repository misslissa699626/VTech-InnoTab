/*****************************************************************
|
|      ALSA Output Module
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <alsa/asoundlib.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltAlsaOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.alsa")

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AlsaOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(AlsaOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AlsaOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(AlsaOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(AlsaOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(AlsaOutput, BLT_PacketConsumer)

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_ALSA_DEFAULT_BUFFER_TIME    500000 /* 0.5 secs */
#define BLT_ALSA_DEFAULT_PERIOD_SIZE    4096

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} AlsaOutputModule;

typedef enum {
    BLT_ALSA_OUTPUT_STATE_CLOSED,
    BLT_ALSA_OUTPUT_STATE_OPEN,
    BLT_ALSA_OUTPUT_STATE_CONFIGURED,
    BLT_ALSA_OUTPUT_STATE_PREPARED
} AlsaOutputState;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    AlsaOutputState  state;
    ATX_String       device_name;
    snd_pcm_t*       device_handle;
    BLT_PcmMediaType expected_media_type;
    BLT_PcmMediaType media_type;
    ATX_UInt64       media_time;
} AlsaOutput;

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
static BLT_Result AlsaOutput_Close(AlsaOutput* self);

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/

/* 
  we redefine some of the alsa macros here because the original alsa version
  of these macros have an assert() that will cause a warning with some versions
  of GCC.
*/
#define snd_pcm_status_alloca_no_assert(ptr)                     \
do {                                                             \
    *ptr = (snd_pcm_status_t *) alloca(snd_pcm_status_sizeof()); \
    memset(*ptr, 0, snd_pcm_status_sizeof());                    \
} while (0)

#define snd_pcm_hw_params_alloca_no_assert(ptr)                        \
do {                                                                   \
    *ptr = (snd_pcm_hw_params_t *) alloca(snd_pcm_hw_params_sizeof()); \
    memset(*ptr, 0, snd_pcm_hw_params_sizeof());                       \
} while (0)

#define snd_pcm_sw_params_alloca_no_assert(ptr)                        \
do {                                                                   \
    *ptr = (snd_pcm_sw_params_t *) alloca(snd_pcm_sw_params_sizeof()); \
    memset(*ptr, 0, snd_pcm_sw_params_sizeof());                       \
} while (0)

/*----------------------------------------------------------------------
|    AlsaOutput_SetState
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_SetState(AlsaOutput* self, AlsaOutputState state)
{
    if (state != self->state) {
        ATX_LOG_FINER_2("state changed from %d to %d", self->state, state);
    }
    self->state = state;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Open
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Open(AlsaOutput* self)
{
    int io_result;

    ATX_LOG_FINE_1("openning output - name=%s", ATX_CSTR(self->device_name));

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
        ATX_LOG_FINER("snd_pcm_open");
        io_result = snd_pcm_open(&self->device_handle,
                                 ATX_CSTR(self->device_name),
                                 SND_PCM_STREAM_PLAYBACK,
                                 0);
        if (io_result != 0) {
            self->device_handle = NULL;
            return BLT_FAILURE;
        }
        break;

      case BLT_ALSA_OUTPUT_STATE_OPEN:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        return BLT_FAILURE;
    }

    /* update the state */
    AlsaOutput_SetState(self, BLT_ALSA_OUTPUT_STATE_OPEN);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Close
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Close(AlsaOutput* self)
{
    ATX_LOG_FINER("closing output");

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        /* wait for buffers to finish */
        ATX_LOG_FINER("snd_pcm_drain");
        snd_pcm_drain(self->device_handle);
        /* FALLTHROUGH */

      case BLT_ALSA_OUTPUT_STATE_OPEN:
      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
        /* close the device */
        ATX_LOG_FINER("snd_pcm_close");
        snd_pcm_close(self->device_handle);
        self->device_handle = NULL;
        break;
    }

    /* update the state */
    AlsaOutput_SetState(self, BLT_ALSA_OUTPUT_STATE_CLOSED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Drain
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Drain(AlsaOutput* self)
{
    ATX_LOG_FINER("draining output");

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
      case BLT_ALSA_OUTPUT_STATE_OPEN:
      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        /* drain samples buffered by the driver (wait until they are played) */
        ATX_LOG_FINER("snd_pcm_drain");
        snd_pcm_drain(self->device_handle);
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Reset
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Reset(AlsaOutput* self)
{
    ATX_LOG_FINER("resetting output");

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
      case BLT_ALSA_OUTPUT_STATE_OPEN:
      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        ATX_LOG_FINER("snd_pcm_drop");
        snd_pcm_drop(self->device_handle);
        AlsaOutput_SetState(self, BLT_ALSA_OUTPUT_STATE_CONFIGURED);
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Prepare
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Prepare(AlsaOutput* self)
{
    int ior;

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
      case BLT_ALSA_OUTPUT_STATE_OPEN:
    /* we need to be configured already for 'prepare' to work */
    return BLT_FAILURE;

      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
        /* prepare the device */
        ATX_LOG_FINER("snd_pcm_prepare");

        ior = snd_pcm_prepare(self->device_handle);
        if (ior != 0) {
            ATX_LOG_FINER_2("snd_pcm_prepare() failed (%d)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }
        break;

      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        /* ignore */
        return BLT_SUCCESS;
    }

    /* update the state */
    AlsaOutput_SetState(self, BLT_ALSA_OUTPUT_STATE_PREPARED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Unprepare
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Unprepare(AlsaOutput* self)
{
    BLT_Result result;

    ATX_LOG_FINER("unpreparing output");

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
      case BLT_ALSA_OUTPUT_STATE_OPEN:
      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
        /* ignore */
        break;

      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        /* drain any pending samples */
        result = AlsaOutput_Drain(self);
        if (BLT_FAILED(result)) return result;
        
        /* update the state */
        AlsaOutput_SetState(self, BLT_ALSA_OUTPUT_STATE_CONFIGURED);
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Configure
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Configure(AlsaOutput*             self, 
                     const BLT_PcmMediaType* format)
{
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_sw_params_t* sw_params;
    unsigned int         rate = format->sample_rate;
    unsigned int         buffer_time = BLT_ALSA_DEFAULT_BUFFER_TIME;
    snd_pcm_uframes_t    buffer_size = 0;
    snd_pcm_uframes_t    period_size = BLT_ALSA_DEFAULT_PERIOD_SIZE;
    snd_pcm_format_t     pcm_format_id = SND_PCM_FORMAT_UNKNOWN;
    int                  ior;
    BLT_Result           result;

    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_CLOSED:
        /* first, we need to open the device */
        result = AlsaOutput_Open(self);
        if (BLT_FAILED(result)) return result;

        /* FALLTHROUGH */

      case BLT_ALSA_OUTPUT_STATE_CONFIGURED:
      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        /* check to see if the format has changed */
        if (format->sample_rate     != self->media_type.sample_rate   ||
            format->channel_count   != self->media_type.channel_count ||
            format->bits_per_sample != self->media_type.bits_per_sample) {
            /* new format */

            /* check the format */
            if (format->sample_rate     == 0 ||
                format->channel_count   == 0 ||
                format->bits_per_sample == 0) {
                return BLT_ERROR_INVALID_MEDIA_FORMAT;
            }
        
            /* unprepare (forget current settings) */
            result = AlsaOutput_Unprepare(self);
            if (BLT_FAILED(result)) return result;
        } else {
            /* same format, do nothing */
            return BLT_SUCCESS;
        }
        
        /* FALLTHROUGH */

      case BLT_ALSA_OUTPUT_STATE_OPEN:
        /* configure the device with the new format */
        ATX_LOG_FINER("configuring ALSA device");

        /* copy the format */
        self->media_type = *format;

        ATX_LOG_FINE_3("new format: sr=%d, ch=%d, bps=%d",
                       format->sample_rate,
                       format->channel_count,
                       format->bits_per_sample);

        /* allocate a new blank configuration */
        snd_pcm_hw_params_alloca_no_assert(&hw_params);
        snd_pcm_hw_params_any(self->device_handle, hw_params);

        /* use interleaved access */
        ior = snd_pcm_hw_params_set_access(self->device_handle, hw_params, 
                                           SND_PCM_ACCESS_RW_INTERLEAVED);
        if (ior != 0) {
            ATX_LOG_WARNING_2("snd_pcm_hw_params_set_access failed (%d:%s)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* set the sample rate */
        ior = snd_pcm_hw_params_set_rate_near(self->device_handle, 
                                              hw_params, 
                                              &rate, NULL);
        if (ior != 0) {
            ATX_LOG_WARNING_3("snd_pcm_hw_params_set_rate_near(%d) failed (%d:%s)", rate, ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* set the number of channels */
        ior = snd_pcm_hw_params_set_channels(self->device_handle, 
                                             hw_params,
                                             format->channel_count);
        if (ior != 0) {
            ATX_LOG_WARNING_3("snd_pcm_hw_params_set_channels(%d) failed (%d:%s)", format->channel_count, ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* set the sample format */
        switch (format->sample_format) {
        	case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE:
                ATX_LOG_FINE("sample format is BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE");
        		switch (format->bits_per_sample) {
        			case  8: pcm_format_id = SND_PCM_FORMAT_S8;      break;
        			case 16: pcm_format_id = SND_PCM_FORMAT_S16_LE;  break;
        			case 24: pcm_format_id = SND_PCM_FORMAT_S24_3LE; break;
        			case 32: pcm_format_id = SND_PCM_FORMAT_S32_LE;  break;
        		}
        		break;
        		
        	case BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE:
                ATX_LOG_FINE("sample format is BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE");
        		switch (format->bits_per_sample) {
        			case  8: pcm_format_id = SND_PCM_FORMAT_U8;      break;
        			case 16: pcm_format_id = SND_PCM_FORMAT_U16_LE;  break;
        			case 24: pcm_format_id = SND_PCM_FORMAT_U24_3LE; break;
        			case 32: pcm_format_id = SND_PCM_FORMAT_U32_LE;  break;
        		}
        		break;

        	case BLT_PCM_SAMPLE_FORMAT_FLOAT_LE:
                ATX_LOG_FINE("sample format is BLT_PCM_SAMPLE_FORMAT_FLOAT_LE");
        		switch (format->bits_per_sample) {
        			case 32: pcm_format_id = SND_PCM_FORMAT_FLOAT_LE; break;
        		}
        		break;

        	case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE:
                ATX_LOG_FINE("sample format is BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE");
        		switch (format->bits_per_sample) {
        			case  8: pcm_format_id = SND_PCM_FORMAT_S8;      break;
        			case 16: pcm_format_id = SND_PCM_FORMAT_S16_BE;  break;
        			case 24: pcm_format_id = SND_PCM_FORMAT_S24_3BE; break;
        			case 32: pcm_format_id = SND_PCM_FORMAT_S32_BE;  break;
        		}
        		break;
        		
        	case BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_BE:
                ATX_LOG_FINE("sample format is BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_BE");
        		switch (format->bits_per_sample) {
        			case  8: pcm_format_id = SND_PCM_FORMAT_U8;      break;
        			case 16: pcm_format_id = SND_PCM_FORMAT_U16_BE;  break;
        			case 24: pcm_format_id = SND_PCM_FORMAT_U24_3BE; break;
        			case 32: pcm_format_id = SND_PCM_FORMAT_U32_BE;  break;
        		}
        		break;

        	case BLT_PCM_SAMPLE_FORMAT_FLOAT_BE:
                ATX_LOG_FINE("sample format is BLT_PCM_SAMPLE_FORMAT_FLOAT_LE");
        		switch (format->bits_per_sample) {
        			case 32: pcm_format_id = SND_PCM_FORMAT_FLOAT_BE; break;
        		}
        		break;
        }

        if (pcm_format_id == SND_PCM_FORMAT_UNKNOWN) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
        ior = snd_pcm_hw_params_set_format(self->device_handle, 
                                           hw_params,
                                           pcm_format_id);
        if (ior != 0) {
            ATX_LOG_WARNING_2("snd_pcm_hw_params_set_format() failed (%d:%s)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* set the period size */
        ior = snd_pcm_hw_params_set_period_size_near(self->device_handle, 
                                                     hw_params,
                                                     &period_size,
                                                     NULL);
        if (ior != 0) {
            ATX_LOG_WARNING_2("snd_pcm_hw_params_set_period_size_near() failed (%d:%s)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }
        
                                                
        /* set the buffer time (duration) */
        ior = snd_pcm_hw_params_set_buffer_time_near(self->device_handle,
                                                     hw_params, 
                                                     &buffer_time,
                                                     NULL);
        if (ior != 0) {
            ATX_LOG_WARNING_2("snd_pcm_hw_params_set_buffer_time_near() failed (%d:%s)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* get the actual buffer size */
        snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);

        /* activate this configuration */
        ior = snd_pcm_hw_params(self->device_handle, hw_params);
        if (ior != 0) {
            ATX_LOG_WARNING_2("snd_pcm_hw_params() failed (%d:%s)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* configure the software parameters */
        snd_pcm_sw_params_alloca_no_assert(&sw_params);
        snd_pcm_sw_params_current(self->device_handle, sw_params);

        /* set the start threshold to 1/2 the buffer size */
        snd_pcm_sw_params_set_start_threshold(self->device_handle, 
                                              sw_params, 
                                              buffer_size/2);

        /* set the buffer alignment */
        /* NOTE: this call is now obsolete */
        /* snd_pcm_sw_params_set_xfer_align(self->device_handle, sw_params, 1); */

        /* activate the sofware parameters */
        ior = snd_pcm_sw_params(self->device_handle, sw_params);
        if (ior != 0) {
            ATX_LOG_SEVERE_2("snd_pcm_sw_params() failed (%d:%s)", ior, snd_strerror(ior));
            return BLT_FAILURE;
        }

        /* print status info */
        {
            snd_pcm_uframes_t val;
            ATX_LOG_FINER_1("sample type = %x", pcm_format_id);
            if (rate != format->sample_rate) {
                ATX_LOG_FINER_1("actual sample = %d", rate);
            }
            ATX_LOG_FINER_1("actual buffer time = %d", buffer_time);
            ATX_LOG_FINER_1("buffer size = %d", buffer_size); 
            snd_pcm_sw_params_get_start_threshold(sw_params, &val);
            ATX_LOG_FINER_1("start threshold = %d", val); 
            snd_pcm_sw_params_get_stop_threshold(sw_params, &val);
            ATX_LOG_FINER_1("stop threshold = %d", val); 
            snd_pcm_hw_params_get_period_size(hw_params, &val, NULL);
            ATX_LOG_FINER_1("period size = %d", val);
        }

        break;
    }

    /* update the state */
    AlsaOutput_SetState(self, BLT_ALSA_OUTPUT_STATE_CONFIGURED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Write
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Write(AlsaOutput* self, void* buffer, BLT_Size size)
{
    int          watchdog = 5;
    int          io_result;
    unsigned int sample_count;
    unsigned int sample_size;
    BLT_Result   result;

    /* ensure that the device is prepared */
    result = AlsaOutput_Prepare(self);
    if (BLT_FAILED(result)) return result;

    /* compute the number of samples */
    sample_size  = self->media_type.channel_count*self->media_type.bits_per_sample/8;
    sample_count = size / sample_size;
                           
    /* write samples to the device and handle underruns */       
    do {
        while (sample_count) {
            io_result = snd_pcm_writei(self->device_handle, 
                                       buffer, sample_count);
            if (io_result > 0) {
                buffer = (void*)((char*)buffer + io_result*sample_size);
                if ((unsigned int)io_result <= sample_count) {
                    sample_count -= io_result;
                } else {
                    /* strange, snd_pcm_writei returned more than we wrote */
                    sample_count = 0;
                }
            } else {
                break;
            }
        }        
        if (sample_count == 0) return BLT_SUCCESS;

        /* we reach this point if the first write failed */
        if (io_result < 0) {
            snd_pcm_status_t* status;
            snd_pcm_state_t   state;
            snd_pcm_status_alloca_no_assert(&status);

            io_result = snd_pcm_status(self->device_handle, status);
            if (io_result != 0) {
                return BLT_FAILURE;
            }
            state = snd_pcm_status_get_state(status);
            if (state == SND_PCM_STATE_XRUN) {
                ATX_LOG_FINE("**** UNDERRUN *****");
            
                /* re-prepare the channel */
                io_result = snd_pcm_prepare(self->device_handle);
                if (io_result != 0) {
                    return BLT_FAILURE;
                }
            } else {
               ATX_LOG_WARNING_1("**** STATE = %d ****", state);
            }
        } else {
            ATX_LOG_WARNING_1("snd_pcm_writei() returned %d", io_result); 
        }
        
        ATX_LOG_FINE("**** RETRY *****");

    } while(watchdog--);

    ATX_LOG_SEVERE("**** THE WATCHDOG BIT US ****");
    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|    AlsaOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_PutPacket(BLT_PacketConsumer* _self,
                     BLT_MediaPacket*    packet)
{
    AlsaOutput*             self = ATX_SELF(AlsaOutput, BLT_PacketConsumer);
    const BLT_PcmMediaType* media_type;
    BLT_ByteBuffer          buffer;
    BLT_Size                size;
    BLT_Result              result;

    /* check parameters */
    if (packet == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check the payload buffer and size */
    buffer = BLT_MediaPacket_GetPayloadBuffer(packet);
    size = BLT_MediaPacket_GetPayloadSize(packet);
    if (size == 0) return BLT_SUCCESS;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)(const void*)&media_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* configure the device for this format */
    result = AlsaOutput_Configure(self, media_type);
	if (BLT_FAILED(result)) return result;
	
    /* update the media time */
    {
        BLT_TimeStamp ts = BLT_MediaPacket_GetTimeStamp(packet);
        ATX_UInt64    ts_nanos = BLT_TimeStamp_ToNanos(ts);
        if (ts_nanos == 0 && 
            media_type->channel_count && 
            media_type->bits_per_sample) {
            BLT_TimeStamp duration;
            unsigned int sample_count = BLT_MediaPacket_GetPayloadSize(packet)/
                                        (media_type->channel_count*media_type->bits_per_sample/8);
            duration = BLT_TimeStamp_FromSamples(sample_count, media_type->sample_rate);            
            self->media_time += BLT_TimeStamp_ToNanos(duration);
        } else {
            self->media_time = ts_nanos;
        }
    }
    
    /* write the audio samples */
    return AlsaOutput_Write(self, buffer, size);
}

/*----------------------------------------------------------------------
|    AlsaOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_QueryMediaType(BLT_MediaPort*        _self,
                          BLT_Ordinal           index,
                          const BLT_MediaType** media_type)
{
    AlsaOutput* self = ATX_SELF(AlsaOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    AlsaOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_AnyConst             parameters, 
                  BLT_MediaNode**          object)
{
    AlsaOutput*               output;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;

    ATX_LOG_FINE("creating output");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    output = ATX_AllocateZeroMemory(sizeof(AlsaOutput));
    if (output == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(output, BLT_BaseMediaNode), module, core);

    /* construct the object */
    output->state                      = BLT_ALSA_OUTPUT_STATE_CLOSED;
    output->device_handle              = NULL;
    output->media_type.sample_rate     = 0;
    output->media_type.channel_count   = 0;
    output->media_type.bits_per_sample = 0;

    /* parse the name */
    if (constructor->name && ATX_StringLength(constructor->name) > 5) {
        output->device_name = ATX_String_Create(constructor->name+5);
    } else {
        output->device_name = ATX_String_Create("default");
    }
    
    /* setup the expected media type */
    BLT_PcmMediaType_Init(&output->expected_media_type);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(output, AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(output, AlsaOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(output, AlsaOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE(output, AlsaOutput, BLT_OutputNode);
    ATX_SET_INTERFACE(output, AlsaOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(output, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AlsaOutput_Destroy(AlsaOutput* self)
{
    ATX_LOG_FINE("destroying output");

    /* close the device */
    AlsaOutput_Close(self);

    /* free the name */
    ATX_String_Destruct(&self->device_name);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AlsaOutput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_COMPILER_UNUSED(stream);
        
    ATX_LOG_FINER("activating output");

    /* open the device */
    AlsaOutput_Open(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AlsaOutput_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_Deactivate(BLT_MediaNode* _self)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("deactivating output");

    /* close the device */
    AlsaOutput_Close(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AlsaOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_Stop(BLT_MediaNode* _self)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("stopping output");

    /* reset the device */
    AlsaOutput_Reset(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AlsaOutput_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_Pause(BLT_MediaNode* _self)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);
        
    ATX_LOG_FINER("pausing output");

    /* pause the device */
    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        snd_pcm_pause(self->device_handle, 1);
        break;

      default:
        /* ignore */
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AlsaOutput_Resume
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_Resume(BLT_MediaNode* _self)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);
        
    ATX_LOG_FINER("resuming output");

    /* pause the device */
    switch (self->state) {
      case BLT_ALSA_OUTPUT_STATE_PREPARED:
        snd_pcm_pause(self->device_handle, 0);
        break;

      default:
        /* ignore */
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AlsaOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_GetPortByName(BLT_MediaNode*  _self,
                          BLT_CString     name,
                          BLT_MediaPort** port)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    AlsaOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    AlsaOutput* self = ATX_SELF_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* ignore unless we're prepared */
    if (self->state != BLT_ALSA_OUTPUT_STATE_PREPARED) {
        return BLT_SUCCESS;
    }

    /* reset the device */
    AlsaOutput_Reset(self);

    /* update the media time */
    if (point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP) {
        self->media_time = BLT_TimeStamp_ToNanos(point->time_stamp);
    } else {
        self->media_time = 0;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutput_GetStatus(BLT_OutputNode*       _self,
                     BLT_OutputNodeStatus* status)
{
    AlsaOutput*       self = ATX_SELF(AlsaOutput, BLT_OutputNode);
    snd_pcm_status_t* pcm_status;
    snd_pcm_sframes_t delay = 0;
    ATX_UInt64        delay_ns = 0;
    ATX_UInt64        media_time = self->media_time;
    int               io_result;

    /* default values */
    status->media_time.seconds = 0;
    status->media_time.nanoseconds = 0;
    status->flags = 0;

    /* get the driver status */
    snd_pcm_status_alloca_no_assert(&pcm_status);
    io_result = snd_pcm_status(self->device_handle, pcm_status);
    if (io_result != 0) {
        return BLT_FAILURE;
    }
    if (delay == 0) {
        /* workaround buggy alsa drivers */
        io_result = snd_pcm_delay(self->device_handle, &delay);
        if (io_result != 0) {
            return BLT_FAILURE;
        }
    }
    if (delay > 0 && 
        self->media_type.sample_rate &&
        self->media_type.channel_count) {
        delay_ns = (delay * (ATX_UInt64)self->media_type.channel_count * (ATX_UInt64)1000000000)/self->media_type.sample_rate;
        if (delay_ns <= media_time) media_time -= delay_ns;
    }
    
    /* return the computed media time */
    status->media_time = BLT_TimeStamp_FromNanos(media_time);
    ATX_LOG_FINEST_2("delay = %lld, media time = %lld", delay_ns, media_time);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlsaOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(AlsaOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(AlsaOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AlsaOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AlsaOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(AlsaOutput, BLT_MediaPort)
    AlsaOutput_GetName,
    AlsaOutput_GetProtocol,
    AlsaOutput_GetDirection,
    AlsaOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AlsaOutput, BLT_PacketConsumer)
    AlsaOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AlsaOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AlsaOutput_GetPortByName,
    AlsaOutput_Activate,
    AlsaOutput_Deactivate,
    BLT_BaseMediaNode_Start,
    AlsaOutput_Stop,
    AlsaOutput_Pause,
    AlsaOutput_Resume,
    AlsaOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AlsaOutput, BLT_OutputNode)
    AlsaOutput_GetStatus,
    NULL
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AlsaOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|       AlsaOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaOutputModule_Probe(BLT_Module*              self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the input type should be unknown, or audio/pcm */
            if (!(constructor->spec.input.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.input.media_type->id == 
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* the name should be 'alsa:<name>' */
            if (constructor->name == NULL ||
                !ATX_StringsEqualN(constructor->name, "alsa:", 4)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("probe ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlsaOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AlsaOutputModule, AlsaOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AlsaOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    AlsaOutputModule_CreateInstance,
    AlsaOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AlsaOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AlsaOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_AlsaOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("ALSA Output", NULL, 0, 
                                 &AlsaOutputModule_BLT_ModuleInterface,
                                 &AlsaOutputModule_ATX_ReferenceableInterface,
                                 object);
}
