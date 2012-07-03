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
#include "BltOssOutput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.oss")

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(OssOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(OssOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(OssOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(OssOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(OssOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(OssOutput, BLT_PacketConsumer)

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
} OssOutputModule;

typedef enum {
    BLT_OSS_OUTPUT_STATE_CLOSED,
    BLT_OSS_OUTPUT_STATE_OPEN,
    BLT_OSS_OUTPUT_STATE_CONFIGURED
} OssOutputState;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    OssOutputState    state;
    ATX_String        device_name;
    int               device_handle;
    BLT_Flags         device_flags;
    BLT_PcmMediaType  media_type;
    BLT_PcmMediaType  expected_media_type;
    BLT_Cardinal      bytes_before_trigger;
    EqualizerConvert  equalizer;
} OssOutput;

static EqualizerConvert g_equalizer = 0;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER  0x01
#define BLT_OSS_OUTPUT_WRITE_WATCHDOG    100

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
static BLT_Result OssOutput_Close(OssOutput* output);

/*----------------------------------------------------------------------
|    OssOutput_SetState
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_SetState(OssOutput* self, OssOutputState state)
{
    if (state != self->state) {
        ATX_LOG_FINER_2("OssOutput::SetState - from %d to %d",
                        self->state, state);
    }
    self->state = state;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_GetCaps
+---------------------------------------------------------------------*/
static void
OssOutput_GetCaps(OssOutput* self)
{
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
}

/*----------------------------------------------------------------------
|    OssOutput_Open
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Open(OssOutput* self)
{
    int io_result;

    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        ATX_LOG_FINE_1("OssOutput::Open - %s", self->device_name);
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
        OssOutput_GetCaps(self);
        break;

      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
        return BLT_FAILURE;
    }

    /* update the state */
    OssOutput_SetState(self, BLT_OSS_OUTPUT_STATE_OPEN);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_Close
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Close(OssOutput* self)
{
    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
        /* wait for buffers to finish */
        ATX_LOG_FINER("OssOutput::Close (configured)");
        ioctl(self->device_handle, SNDCTL_DSP_SYNC, 0);
        /* FALLTHROUGH */

      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* close the device */
        ATX_LOG_FINER("OssOutput::Close");
        close(self->device_handle);
        self->device_handle = BLT_OSS_OUTPUT_INVALID_HANDLE;
        break;
    }

    /* update the state */
    OssOutput_SetState(self, BLT_OSS_OUTPUT_STATE_CLOSED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_Drain
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Drain(OssOutput* self)
{
    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* flush samples buffered by the driver */
        ATX_LOG_FINER("OssOutput::Drain");
        ioctl(self->device_handle, SNDCTL_DSP_SYNC, 0);
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_SetEqualizerFunction
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_SetEqualizerFunction(OssOutput* self, EqualizerConvert function)
{
    g_equalizer = function;
    //self->equalizer = function;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_Configure
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Configure(OssOutput* self)
{
    BLT_Result result;
    int        io_result;
    int        param;

    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* first, we need to open the device */
        result = OssOutput_Open(self);
        if (BLT_FAILED(result)) return result;

        /* FALLTHROUGH */

      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* configure the device */

        /* format */
        switch (self->media_type.bits_per_sample) {
        	case  8: param = AFMT_U8;     break;
        	case 16: param = AFMT_S16_NE; break;
        	default: return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        io_result = ioctl(self->device_handle, SNDCTL_DSP_SETFMT, &param);
        if (io_result != 0) {
            ATX_LOG_WARNING_2("OssOutput::Configure - SNDCTL_DSP_SETFMT(%d) failed (%d)",
                              param, io_result);
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        /* sample rate */
        param = self->media_type.sample_rate;
        io_result = ioctl(self->device_handle, SNDCTL_DSP_SPEED, &param);
        if (io_result != 0) {
            ATX_LOG_WARNING_2("OssOutput::Configure - SNDCTL_DSP_SPEED(%d) failed (%d)", 
                              param, io_result);
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        /* channels */
        param = self->media_type.channel_count == 2 ? 1 : 0;
        io_result = ioctl(self->device_handle, SNDCTL_DSP_STEREO, &param);
        if (io_result != 0) {
            ATX_LOG_WARNING_2("OssOutput::Configure - SNDCTL_DSP_STEREO(%d) failed (%d)", 
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
        break;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
        /* ignore */
        return BLT_SUCCESS;
    }

    /* update the state */
    OssOutput_SetState(self, BLT_OSS_OUTPUT_STATE_CONFIGURED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_SetFormat
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_SetFormat(OssOutput*              self,
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
            OssOutput_Drain(self);
            OssOutput_SetState(self, BLT_OSS_OUTPUT_STATE_OPEN);
            break;
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_Write
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Write(OssOutput* self, void* buffer, BLT_Size size)
{
    int        watchdog = BLT_OSS_OUTPUT_WRITE_WATCHDOG;
    BLT_Result result;

    /* ensure that the device is configured */
    result = OssOutput_Configure(self);
    if (BLT_FAILED(result)) {
        /* reset the media type */
        BLT_PcmMediaType_Init(&self->media_type);
        return result;
    }

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

    while (size) {
        int nb_written = write(self->device_handle, buffer, size);
        if (nb_written == -1) {
            if (errno == EAGAIN) {
#if defined(SNDCTL_DSP_SETTRIGGER)
                if (self->device_flags & BLT_OSS_OUTPUT_FLAG_CAN_TRIGGER) {
                    /* we have set a trigger, and the buffer is full */
                    int enable = PCM_ENABLE_OUTPUT;
                    ioctl(self->device_handle, 
                          SNDCTL_DSP_SETTRIGGER,
                          &enable);
                }
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
|    OssOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_PutPacket(BLT_PacketConsumer* _self,
                    BLT_MediaPacket*    packet)
{
    OssOutput*              self = ATX_SELF(OssOutput, BLT_PacketConsumer);
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
    result = OssOutput_SetFormat(self, media_type);
    if (BLT_FAILED(result)) return result;

    self->equalizer = g_equalizer;
    if ( media_type->channel_count == 2 &&
         media_type->bits_per_sample == 16 &&
         self->equalizer) {
	dst = self->equalizer(BLT_MediaPacket_GetPayloadBuffer(packet), size);
    }

    /* send the payload to the drvice */
    if ( dst != NULL ) {
        result = OssOutput_Write(self, 
                                 dst, 
                                 size);
        ATX_FreeMemory(dst);
    }
    else {
        result = OssOutput_Write(self, 
                                 BLT_MediaPacket_GetPayloadBuffer(packet), 
                                 size);
    }

    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_QueryMediaType(BLT_MediaPort*        _self,
				         BLT_Ordinal           index,
				         const BLT_MediaType** media_type)
{
    OssOutput* self = ATX_SELF(OssOutput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    OssOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    OssOutput*                self;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;

    ATX_LOG_FINE("OssOutput::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(OssOutput));
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
    ATX_SET_INTERFACE_EX(self, OssOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, OssOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, OssOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE(self, OssOutput, BLT_OutputNode);
    ATX_SET_INTERFACE(self, OssOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
OssOutput_Destroy(OssOutput* self)
{
    ATX_LOG_FINE("OssOutput::Destroy");

    /* close the device */
    OssOutput_Close(self);

    /* free the name */
    ATX_String_Destruct(&self->device_name);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       OssOutput_Start
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_Start(BLT_MediaNode* _self)
{
    OssOutput* self = ATX_SELF_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode);

    /* open the device */
    ATX_LOG_FINER("OssOutput::Start");
    OssOutput_Open(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       OssOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_Stop(BLT_MediaNode* _self)
{
    OssOutput* self = ATX_SELF_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode);

    /* close the device */
    ATX_LOG_FINER("OssOutput::Stop");
    OssOutput_Close(self);

    return BLT_SUCCESS;
}
 
/*----------------------------------------------------------------------
|    OssOutput_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_Pause(BLT_MediaNode* _self)
{
    OssOutput* self = ATX_SELF_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode);

    switch (self->state) {
      case BLT_OSS_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_OSS_OUTPUT_STATE_CONFIGURED:
      case BLT_OSS_OUTPUT_STATE_OPEN:
        /* reset the device (there is no IOCTL for pause */
        ATX_LOG_FINER("OssOutput::Pause (configured)");
        ioctl(self->device_handle, SNDCTL_DSP_RESET, 0);
        return BLT_SUCCESS;
    }

    return BLT_SUCCESS;
}
                   
/*----------------------------------------------------------------------
|   OssOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    OssOutput* self = ATX_SELF_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    OssOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_Seek(BLT_MediaNode* _self,
               BLT_SeekMode*  mode,
               BLT_SeekPoint* point)
{
    OssOutput* self = ATX_SELF_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* reset the device */
    ioctl(self->device_handle, SNDCTL_DSP_RESET, 0);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OssOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutput_GetStatus(BLT_OutputNode*       _self,
                    BLT_OutputNodeStatus* status)
{
    OssOutput* self = ATX_SELF(OssOutput, BLT_OutputNode);
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

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OssOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(OssOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(OssOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(OssOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(OssOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(OssOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(OssOutput, BLT_MediaPort)
    OssOutput_GetName,
    OssOutput_GetProtocol,
    OssOutput_GetDirection,
    OssOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OssOutput, BLT_PacketConsumer)
    OssOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OssOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    OssOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    OssOutput_Start,
    OssOutput_Stop,
    OssOutput_Pause,
    BLT_BaseMediaNode_Resume,
    OssOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OssOutput, BLT_OutputNode)
    OssOutput_GetStatus,
    NULL,
     OssOutput_SetEqualizerFunction
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OssOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|       OssOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
OssOutputModule_Probe(BLT_Module*              self, 
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

            ATX_LOG_FINE_1("OssOutputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OssOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(OssOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(OssOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(OssOutputModule, OssOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OssOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    OssOutputModule_CreateInstance,
    OssOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define OssOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OssOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_OssOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("OSS Output", NULL, 0, 
                                 &OssOutputModule_BLT_ModuleInterface,
                                 &OssOutputModule_ATX_ReferenceableInterface,
                                 object);
}
