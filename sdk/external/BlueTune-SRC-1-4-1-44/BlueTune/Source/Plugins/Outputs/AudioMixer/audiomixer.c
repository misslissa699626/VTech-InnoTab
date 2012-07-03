/*****************************************************************
|
|      OSS Output Module
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"

#define USE_AUDIOMIXER

#ifdef USE_AUDIOMIXER
#include <audiomixer.h>
#define BUFFER_SIZE 32*1024
#define TRIGGER_SIZE 8*1024

#define outBufLen 1024*4 
unsigned char outBuf[outBufLen+1024];
int outBufPos=0;
#endif
/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.oss")

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AudioMixerModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(AudioMixer, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AudioMixer, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(AudioMixer, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(AudioMixer, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(AudioMixer, BLT_PacketConsumer)

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_OSS_OUTPUT_INVALID_HANDLE (-1)

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} AudioMixerModule;

typedef enum {
    BLT_OSS_OUTPUT_STATE_CLOSED,
    BLT_OSS_OUTPUT_STATE_OPEN,
    BLT_OSS_OUTPUT_STATE_CONFIGURED,
	BLT_OSS_OUTPUT_STATE_FORCEDSTOP
} AudioMixerState;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    AudioMixerState    state;
    ATX_String        device_name;
    #if defined(USE_AUDIOMIXER)
	audiomixer_handle_t* device_handle;
    #else
    int               device_handle;
    #endif
    BLT_Flags         device_flags;
    BLT_PcmMediaType  media_type;
    BLT_PcmMediaType  expected_media_type;
    BLT_Cardinal      bytes_before_trigger;
    EqualizerConvert  equalizer;
} AudioMixer;

static EqualizerConvert g_equalizer = 0;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER  0x01
#define BLT_OSS_OUTPUT_WRITE_WATCHDOG    100

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
static BLT_Result AudioMixer_Close(AudioMixer* output);

/*----------------------------------------------------------------------
|    AudioMixer_SetState
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_SetState(AudioMixer* self, AudioMixerState state)
{
    if (state != self->state) {
        ATX_LOG_FINER_2("AudioMixer::SetState - from %d to %d",
                        self->state, state);
    }
    self->state = state;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_GetCaps
+---------------------------------------------------------------------*/
static void
AudioMixer_GetCaps(AudioMixer* self)
{
#if defined(USE_AUDIOMIXER)
#else
#if defined(SNDCTL_DSP_GETCAPS) && defined(SNDCTL_DSP_SETTRIGGER)
    int caps;
    if (ioctl(self->device_handle, SNDCTL_DSP_GETCAPS, &caps) == 0) {
        if (caps & DSP_CAP_TRIGGER) {
            int enable = ~PCM_ENABLE_OUTPUT;
            self->device_flags |= BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER;
            ioctl(self->device_handle, SNDCTL_DSP_SETTRIGGER, &enable);
        } else {
            self->device_flags ^= BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER;
        }
    }
#endif
#endif
}

/*----------------------------------------------------------------------
|    AudioMixer_Open
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Open(AudioMixer* self)
{
    int io_result;

    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
#if defined(USE_AUDIOMIXER)
#else
        ATX_LOG_FINE_1("AudioMixer::Open - %s", self->device_name);
        io_result = open(ATX_CSTR(self->device_name), O_WRONLY);
        if (io_result < 0) {
            self->device_handle = BLT_OSS_OUTPUT_INVALID_HANDLE;
            switch (errno) {
              case ENOENT:
                return BLT_ERROR_NO_SUCH_DEVICE;

              case EACCES:
                return BLT_ERROR_ACCESS_DENIED;
                
              case EBUSY:
                return BLT_ERROR_DEVICE_BUSY;
                
              default:
                return BLT_FAILURE;
            }
        }
        self->device_handle = io_result;
        AudioMixer_GetCaps(self);
#endif
        break;

      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
        return BLT_FAILURE;
    }

    /* update the state */
    AudioMixer_SetState(self, BLT_OSS_OUTPUT_STATE_OPEN);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_Close
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Close(AudioMixer* self)
{
    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
        /* wait for buffers to finish */
#if defined(USE_AUDIOMIXER)
		if (self->device_handle) {
			audiomixer_flush(self->device_handle);
		}
#else		
        ATX_LOG_FINER("AudioMixer::Close (configured)");
        ioctl(self->device_handle, SNDCTL_DSP_SYNC, 0);
        /* FALLTHROUGH */
#endif
#if defined(USE_AUDIOMIXER)
	case BLT_OSS_OUTPUT_STATE_FORCEDSTOP:
         if(self->state==BLT_OSS_OUTPUT_STATE_FORCEDSTOP)
		   {
		    audiomixer_set_volume(self->device_handle,0);
			audiomixer_clear_input_buffer(self->device_handle);	
           }			
#endif
      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* close the device */
        ATX_LOG_FINER("AudioMixer::Close");
#if defined(USE_AUDIOMIXER)
		audiomixer_close(self->device_handle);
#else		
        close(self->device_handle);
#endif
        self->device_handle = BLT_OSS_OUTPUT_INVALID_HANDLE;
        break;
    }

    /* update the state */
    AudioMixer_SetState(self, BLT_OSS_OUTPUT_STATE_CLOSED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_Drain
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Drain(AudioMixer* self)
{
    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* flush samples buffered by the driver */
        ATX_LOG_FINER("AudioMixer::Drain");
#if defined(USE_AUDIOMIXER)
		if (self->device_handle) {
			audiomixer_flush(self->device_handle);
		}
#else
        ioctl(self->device_handle, SNDCTL_DSP_SYNC, 0);
#endif
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_SetEqualizerFunction
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_SetEqualizerFunction(AudioMixer* self, EqualizerConvert function)
{
    g_equalizer = function;
    //self->equalizer = function;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_Configure
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Configure(AudioMixer* self)
{
    BLT_Result result;
    int        io_result;
    int        param;

    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* first, we need to open the device */
        result = AudioMixer_Open(self);
        if (BLT_FAILED(result)) return result;

        /* FALLTHROUGH */

      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* configure the device */

        /* format */
#if defined(USE_AUDIOMIXER)
	{
			audiomixer_info_t info;
			/* bit per sample */
			switch (self->media_type.bits_per_sample) {
				case 8:
					info.format |= AUDIOMIXER_BITS8;
					break;
				case 16:
					info.format |= AUDIOMIXER_BITS16;
					break;
				default:
					return BLT_ERROR_INVALID_MEDIA_FORMAT;
			}

			/* sample rate */
			info.rate = self->media_type.sample_rate;

			/* channels */
			switch (self->media_type.channel_count) {
				case 1:
					info.input_channels |= AUDIOMIXER_MONO;
					break;
				case 2:
					info.input_channels |= AUDIOMIXER_STEREO;
					break;
				default:
					return BLT_ERROR_INVALID_MEDIA_FORMAT;
			}

			info.bufferSize = BUFFER_SIZE;
			info.triggerSize = TRIGGER_SIZE;
			info.host = NULL;
			info.name = NULL;
			info.volume = AUDIOMIXER_VOLUME_MAX;
                        info.output_channels = AUDIOMIXER_STEREO;
			self->device_handle = audiomixer_play(info);
			if (!self->device_handle) {
				return BLT_FAILURE;
			}
	}
#else
        switch (self->media_type.bits_per_sample) {
        	case  8: param = AFMT_U8;     break;
        	case 16: param = AFMT_S16_NE; break;
        	default: return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        io_result = ioctl(self->device_handle, SNDCTL_DSP_SETFMT, &param);
        if (io_result != 0) {
            ATX_LOG_WARNING_2("AudioMixer::Configure - SNDCTL_DSP_SETFMT(%d) failed (%d)",
                              param, io_result);
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        /* sample rate */
        param = self->media_type.sample_rate;
        io_result = ioctl(self->device_handle, SNDCTL_DSP_SPEED, &param);
        if (io_result != 0) {
            ATX_LOG_WARNING_2("AudioMixer::Configure - SNDCTL_DSP_SPEED(%d) failed (%d)", 
                              param, io_result);
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        /* channels */
        param = self->media_type.channel_count == 2 ? 1 : 0;
        io_result = ioctl(self->device_handle, SNDCTL_DSP_STEREO, &param);
        if (io_result != 0) {
            ATX_LOG_WARNING_2("AudioMixer::Configure - SNDCTL_DSP_STEREO(%d) failed (%d)", 
                              param, io_result);
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
        
        /* compute trigger */
        self->bytes_before_trigger = 
            (self->media_type.sample_rate *
             self->media_type.channel_count) / 4;

        /* set fragments */
        if (0) {
            int fragment = 0x7FFF000D;
            ioctl(self->device_handle, SNDCTL_DSP_SETFRAGMENT, &fragment);
        }
#endif
        break;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
        /* ignore */
        return BLT_SUCCESS;
    }

    /* update the state */
    AudioMixer_SetState(self, BLT_OSS_OUTPUT_STATE_CONFIGURED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_SetFormat
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_SetFormat(AudioMixer*              self,
                    const BLT_PcmMediaType* format)
{
    /* compare the media format with the current format */
    if (format->sample_rate     != self->media_type.sample_rate   ||
        format->channel_count   != self->media_type.channel_count ||
        format->bits_per_sample != self->media_type.bits_per_sample) {
        /* new format */

        /* check the format */
        if (format->sample_rate     == 0 ||
            format->channel_count   == 0 ||
            format->bits_per_sample == 0) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
        
        /* perform basic validity checks of the format */
        if (format->sample_format != BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        /* copy the format */
        self->media_type = *format;

        /* ensure that we can switch to the new format */
        switch (self->state) {
          case BLT_OSS_OUTPUT_STATE_CLOSED:
          case BLT_OSS_OUTPUT_STATE_OPEN:
            break;

          case BLT_OSS_OUTPUT_STATE_CONFIGURED:
            /* drain any pending samples */
            AudioMixer_Drain(self);
            AudioMixer_SetState(self, BLT_OSS_OUTPUT_STATE_OPEN);
            break;
        }
    }

    return BLT_SUCCESS;
}
/*---------------------------------------------------------------------
| AudioMixer_device
+----------------------------------------------------------------------*/
#if defined(USE_AUDIOMIXER)
int
OutPutToDevice(audiomixer_handle_t* handle,unsigned char * buf,int bufSize){

	if(bufSize+outBufPos < outBufLen){
		memcpy(outBuf+outBufPos,buf,bufSize);
		outBufPos+=bufSize;
		return bufSize;
	} else{
		int wLen = outBufLen-outBufPos ;
		int nb_written = 0;
		memcpy(outBuf+outBufPos,buf,outBufLen-outBufPos);
		nb_written = audiomixer_write(handle, outBuf, outBufLen);
		outBufPos = 0;
		if(nb_written == -1) return -1;
		while(bufSize - wLen > outBufLen){
			nb_written = audiomixer_write(handle, buf + wLen, outBufLen);
			wLen+=outBufLen;
			if(nb_written == -1) return -1;
		}
		if(bufSize - wLen){
			memcpy(outBuf,buf + wLen,bufSize - wLen);
			outBufPos = bufSize - wLen;
			wLen += bufSize - wLen;
		}
		return wLen;
	}
}
#endif
/*----------------------------------------------------------------------
|    AudioMixer_Write
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Write(AudioMixer* self, void* buffer, BLT_Size size)
{
    int        watchdog = BLT_OSS_OUTPUT_WRITE_WATCHDOG;
    BLT_Result result;
  
    /* ensure that the device is configured */
    result = AudioMixer_Configure(self);
    if (BLT_FAILED(result)) {
        /* reset the media type */
        BLT_PcmMediaType_Init(&self->media_type);
        return result;
    }
#if defined(USE_AUDIOMIXER)
#else
#if defined(SNDCTL_DSP_SETTRIGGER)
    if (self->device_flags & BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER) {
        if (self->bytes_before_trigger > size) {
            self->bytes_before_trigger -= size;
        } else {
            if (self->bytes_before_trigger != 0) {
                int enable = PCM_ENABLE_OUTPUT;
                ioctl(self->device_handle, SNDCTL_DSP_SETTRIGGER, &enable);
            }
            self->bytes_before_trigger = 0;
        }
    }
#endif
#endif
    while (size) {
#if defined(USE_AUDIOMIXER)
		/*nb_written = write(self->device_handle, buffer, size);*/
		/*int nb_written= OutPutToDevice(self->device_handle, buffer, size);*/
		int nb_written=audiomixer_write(self->device_handle,buffer,size);
#else		
        int nb_written = write(self->device_handle, buffer, size);
#endif		
        if (nb_written == -1) {
            if (errno == EAGAIN) {
#if defined(USE_AUDIOMIXER)
#else
#if defined(SNDCTL_DSP_SETTRIGGER)
                if (self->device_flags & BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER) {
                    /* we have set a trigger, and the buffer is full */
                    int enable = PCM_ENABLE_OUTPUT;
                    ioctl(self->device_handle, 
                          SNDCTL_DSP_SETTRIGGER,
                          &enable);
                }
#endif
#endif
                nb_written = 0;
            } else if (errno != EINTR) {
                return BLT_FAILURE;
            }
        }

        size -= nb_written;
        buffer = (void*)((char *)buffer + nb_written);
        if (watchdog-- == 0) return BLT_FAILURE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_PutPacket(BLT_PacketConsumer* _self,
                    BLT_MediaPacket*    packet)
{
    AudioMixer*              self = ATX_SELF(AudioMixer, BLT_PacketConsumer);
    const BLT_PcmMediaType* media_type;
    BLT_Size                size;
    BLT_Result              result;
    void *dst = NULL;

    /* check parameters */
    if (packet == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check the payload size */
    size = BLT_MediaPacket_GetPayloadSize(packet);
    if (size == 0) return BLT_SUCCESS;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)(const void*)&media_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* set the format of the samples */
    result = AudioMixer_SetFormat(self, media_type);
    if (BLT_FAILED(result)) return result;

    self->equalizer = g_equalizer;
    if ( media_type->channel_count == 2 &&
         media_type->bits_per_sample == 16 &&
         self->equalizer) {
	dst = self->equalizer(BLT_MediaPacket_GetPayloadBuffer(packet), size);
    }

    /* send the payload to the drvice */
    if ( dst != NULL ) {
        result = AudioMixer_Write(self, 
                                 dst, 
                                 size);
        ATX_FreeMemory(dst);
    }
    else {
        result = AudioMixer_Write(self, 
                                 BLT_MediaPacket_GetPayloadBuffer(packet), 
                                 size);
    }

    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_QueryMediaType(BLT_MediaPort*        _self,
				         BLT_Ordinal           index,
				         const BLT_MediaType** media_type)
{
    AudioMixer* self = ATX_SELF(AudioMixer, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    AudioMixer_Create
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    AudioMixer*                self;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;

    ATX_LOG_FINE("AudioMixer::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(AudioMixer));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    self->state                      = BLT_OSS_OUTPUT_STATE_CLOSED;
    self->device_handle              = BLT_OSS_OUTPUT_INVALID_HANDLE;
    self->device_flags               = 0;
    self->media_type.sample_rate     = 0;
    self->media_type.channel_count   = 0;
    self->media_type.bits_per_sample = 0;
    self->media_type.sample_format   = 0;

    /* parse the name */
    if (constructor->name) {
        int index = 0;
        if (ATX_StringLength(constructor->name) < 5) {
            ATX_FreeMemory(self);
            return BLT_ERROR_INVALID_PARAMETERS;
        }
        if (constructor->name[4] >= '0' &&
            constructor->name[4] <= '9') {
            /* name is a soundcard index */
            const char* c_index = &constructor->name[4];
            while (*c_index >= '0' && *c_index <= '9') {
                index = 10*index + *c_index++ -'0';
            }
            if (index == 0) {
                ATX_String_Assign(&self->device_name, "/dev/dsp");
            } else {
                char        device_name[32] = "/dev/dsp";
                const char* c_index = &constructor->name[4];
                char*       d_index = &device_name[8];
                while (*c_index >= '0' && *c_index <= '9') {
                    *d_index++ = *c_index++;
                }
                *d_index = '\0';
                ATX_String_Assign(&self->device_name, device_name);
            }
        } else {
            ATX_String_Assign(&self->device_name, constructor->name+4);
        }
    } else {
        ATX_String_Assign(&self->device_name, "/dev/dsp");
    }

    /* setup the expected media type */
    BLT_PcmMediaType_Init(&self->expected_media_type);
    self->expected_media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
    self->expected_media_type.bits_per_sample = 16;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, AudioMixer, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, AudioMixer, BLT_PacketConsumer);
    ATX_SET_INTERFACE(self, AudioMixer, BLT_OutputNode);
    ATX_SET_INTERFACE(self, AudioMixer, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AudioMixer_Destroy(AudioMixer* self)
{
    ATX_LOG_FINE("AudioMixer::Destroy");

    /* close the device */
    AudioMixer_Close(self);

    /* free the name */
    ATX_String_Destruct(&self->device_name);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AudioMixer_Start
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_Start(BLT_MediaNode* _self)
{
    AudioMixer* self = ATX_SELF_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);

    /* open the device */
    ATX_LOG_FINER("AudioMixer::Start");
    AudioMixer_Open(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       AudioMixer_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_Stop(BLT_MediaNode* _self)
{
    AudioMixer* self = ATX_SELF_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);

    /* close the device */
    ATX_LOG_FINER("AudioMixer::Stop");
    AudioMixer_Close(self);

    return BLT_SUCCESS;
}
 
/*----------------------------------------------------------------------
|    AudioMixer_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_Pause(BLT_MediaNode* _self)
{
#if defined(USE_AUDIOMIXER)
#else
    AudioMixer* self = ATX_SELF_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);

    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* reset the device (there is no IOCTL for pause */
        ATX_LOG_FINER("AudioMixer::Pause (configured)");
        ioctl(self->device_handle, SNDCTL_DSP_RESET, 0);
        return BLT_SUCCESS;
    }
#endif
    return BLT_SUCCESS;
}
                   
/*----------------------------------------------------------------------
|   AudioMixer_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    AudioMixer* self = ATX_SELF_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    AudioMixer_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_Seek(BLT_MediaNode* _self,
               BLT_SeekMode*  mode,
               BLT_SeekPoint* point)
{
#if defined(USE_AUDIOMIXER)
#else
    AudioMixer* self = ATX_SELF_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* reset the device */
    ioctl(self->device_handle, SNDCTL_DSP_RESET, 0);
#endif
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AudioMixer_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixer_GetStatus(BLT_OutputNode*       _self,
                    BLT_OutputNodeStatus* status)
{
#if defined(USE_AUDIOMIXER)
#else
    AudioMixer* self = ATX_SELF(AudioMixer, BLT_OutputNode);
    int        delay;
    int        io_result;

    /* ask the driver how much delay there is */
    io_result = ioctl(self->device_handle, SNDCTL_DSP_GETODELAY, &delay);
    if (io_result != 0) {
        return BLT_FAILURE;
    }

    /* convert delay from bytes to milliseconds */
    if (self->media_type.sample_rate &&
        self->media_type.bits_per_sample/8 &&
        self->media_type.channel_count) {
        unsigned long samples = delay/
            ((self->media_type.bits_per_sample/8)*
             self->media_type.channel_count);
        unsigned long delay_ms = 
            (samples*1000)/self->media_type.sample_rate;
        status->media_time.seconds = delay_ms/1000;
        delay_ms -= (status->media_time.seconds*1000);
        status->media_time.nanoseconds = delay_ms*1000000;
    } else {
        status->media_time.seconds = 0;
        status->media_time.nanoseconds = 0;
    }
#endif
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AudioMixer)
    ATX_GET_INTERFACE_ACCEPT_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AudioMixer, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(AudioMixer, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(AudioMixer, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AudioMixer, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AudioMixer, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(AudioMixer, BLT_MediaPort)
    AudioMixer_GetName,
    AudioMixer_GetProtocol,
    AudioMixer_GetDirection,
    AudioMixer_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AudioMixer, BLT_PacketConsumer)
    AudioMixer_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AudioMixer, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AudioMixer_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    AudioMixer_Start,
    AudioMixer_Stop,
    AudioMixer_Pause,
    BLT_BaseMediaNode_Resume,
    AudioMixer_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AudioMixer, BLT_OutputNode)
    AudioMixer_GetStatus,
    NULL,
     AudioMixer_SetEqualizerFunction
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AudioMixer, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|       AudioMixerModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AudioMixerModule_Probe(BLT_Module*              self, 
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

            /* the name should be 'oss:<name>' */
            if (constructor->name == NULL ||
                !ATX_StringsEqualN(constructor->name, "oss:", 4)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("AudioMixerModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AudioMixerModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AudioMixerModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AudioMixerModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AudioMixerModule, AudioMixer)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AudioMixerModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    AudioMixerModule_CreateInstance,
    AudioMixerModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AudioMixerModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AudioMixerModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_AudioMixerModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("OSS Output", NULL, 0, 
                                 &AudioMixerModule_BLT_ModuleInterface,
                                 &AudioMixerModule_ATX_ReferenceableInterface,
                                 object);
}
