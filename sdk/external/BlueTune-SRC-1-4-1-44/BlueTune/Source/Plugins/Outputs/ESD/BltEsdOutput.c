/*****************************************************************
|
|      Esd Output Module
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
#include "BltEsdOutput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"

#include <esd.h>
#define outBufLen 1024*4 
unsigned char outBuf[outBufLen+1024];
int outBufPos=0;

#if 1
#define DEBUG0(args...) fprintf(stderr, args...)
#else
#define DEBUG0(...)
#endif
/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.esd")

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(EsdOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(EsdOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(EsdOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(EsdOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(EsdOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(EsdOutput, BLT_PacketConsumer)

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_ESD_OUTPUT_INVALID_HANDLE (-1)

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} EsdOutputModule;

typedef enum {
    BLT_ESD_OUTPUT_STATE_CLOSED,
    BLT_ESD_OUTPUT_STATE_OPEN,
    BLT_ESD_OUTPUT_STATE_CONFIGURED
} EsdOutputState;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    EsdOutputState    state;
    ATX_String        device_name;
    int               device_handle;
    BLT_Flags         device_flags;
    BLT_PcmMediaType  media_type;
    BLT_PcmMediaType  expected_media_type;
    BLT_Cardinal      bytes_before_trigger;
    EqualizerConvert  equalizer;
} EsdOutput;

static EqualizerConvert g_equalizer = 0;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_ESD_OUTPUT_FLAG_CAN_TRIGGER  0x01
#define BLT_ESD_OUTPUT_WRITE_WATCHDOG    100

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
static BLT_Result EsdOutput_Close(EsdOutput* output);

/*----------------------------------------------------------------------
|    EsdOutput_SetState
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_SetState(EsdOutput* self, EsdOutputState state)
{
    if (state != self->state) {
        ATX_LOG_FINER_2("EsdOutput::SetState - from %d to %d",
                        self->state, state);
    }
    self->state = state;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_Open
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Open(EsdOutput* self)
{
    int io_result;
	
    switch (self->state) {
      case BLT_ESD_OUTPUT_STATE_CLOSED:
        ATX_LOG_FINE_1("EsdOutput::Open - %s", self->device_name);
        io_result =esd_open_sound(NULL);
        if (io_result < 0) {
				self->device_handle = BLT_ESD_OUTPUT_INVALID_HANDLE;
                return BLT_ERROR_NO_SUCH_DEVICE;
		}
		esd_close(io_result);
        break;
      case BLT_ESD_OUTPUT_STATE_OPEN:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ESD_OUTPUT_STATE_CONFIGURED:
        return BLT_FAILURE;
    }

    /* update the state */
    EsdOutput_SetState(self, BLT_ESD_OUTPUT_STATE_OPEN);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_Close
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Close(EsdOutput* self)
{
    switch (self->state) {
      case BLT_ESD_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ESD_OUTPUT_STATE_CONFIGURED:

      case BLT_ESD_OUTPUT_STATE_OPEN:
        /* close the device */
        ATX_LOG_FINER("EsdOutput::Close");
        esd_close(self->device_handle);
        self->device_handle = BLT_ESD_OUTPUT_INVALID_HANDLE;
        break;
    }

    /* update the state */
    EsdOutput_SetState(self, BLT_ESD_OUTPUT_STATE_CLOSED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_Drain
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Drain(EsdOutput* self)
{
    switch (self->state) {
      case BLT_ESD_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_ESD_OUTPUT_STATE_CONFIGURED:
      case BLT_ESD_OUTPUT_STATE_OPEN:
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_SetEqualizerFunction
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_SetEqualizerFunction(EsdOutput* self, EqualizerConvert function)
{
    g_equalizer = function;
    //self->equalizer = function;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_Configure
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Configure(EsdOutput* self)
{
    BLT_Result result;
    int        io_result;
    int        param;

    switch (self->state) {
      case BLT_ESD_OUTPUT_STATE_CLOSED:
        /* first, we need to open the device */
        result = EsdOutput_Open(self);
        if (BLT_FAILED(result)) return result;

        /* FALLTHROUGH */

      case BLT_ESD_OUTPUT_STATE_OPEN:
        /* configure the device */
		{
			esd_format_t format;
			int io_result;
			
			format = (ESD_STREAM | ESD_PLAY);
			/* bit per sample */
			switch (self->media_type.bits_per_sample) {
				case 8:
					format |= ESD_BITS8;
					break;
				case 16:
					format |= ESD_BITS16;
					break;
				default:
					return BLT_ERROR_INVALID_MEDIA_TYPE;
			}

			/* sample rate */
			param = self->media_type.sample_rate;

			/* channels */
			switch (self->media_type.channel_count) {
				case 1:
					format |= ESD_STEREO;/*1 Channel got some problem*/
					break;
				case 2:
					format |= ESD_STEREO;
					break;
				default:
					return BLT_ERROR_INVALID_MEDIA_TYPE;
			}
			io_result = esd_play_stream(format, self->media_type.sample_rate, NULL, NULL);
			if ( io_result < 0 ) {
				self->device_handle = BLT_ESD_OUTPUT_INVALID_HANDLE;
                return BLT_ERROR_NO_SUCH_DEVICE;
			}
			self->device_handle = io_result;
		}
        break;

      case BLT_ESD_OUTPUT_STATE_CONFIGURED:
        /* ignore */
        return BLT_SUCCESS;
    }

    /* update the state */
    EsdOutput_SetState(self, BLT_ESD_OUTPUT_STATE_CONFIGURED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_SetFormat
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_SetFormat(EsdOutput*              self,
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
          case BLT_ESD_OUTPUT_STATE_CLOSED:
          case BLT_ESD_OUTPUT_STATE_OPEN:
            break;

          case BLT_ESD_OUTPUT_STATE_CONFIGURED:
            /* drain any pending samples */
            EsdOutput_Drain(self);
            EsdOutput_SetState(self, BLT_ESD_OUTPUT_STATE_OPEN);
            break;
        }
    }

    return BLT_SUCCESS;
}

int
OutPutToDevice(int handle,unsigned char * buf,int bufSize){
	
	if(bufSize+outBufPos < outBufLen){
		memcpy(outBuf+outBufPos,buf,bufSize);
		outBufPos+=bufSize;
		return bufSize;
	} else{
		int wLen = outBufLen-outBufPos ;
		int nb_written = 0;
		memcpy(outBuf+outBufPos,buf,outBufLen-outBufPos);
		nb_written = write(handle, outBuf, outBufLen);
		outBufPos = 0;
		if(nb_written == -1) return -1;
		while(bufSize - wLen > outBufLen){
				nb_written = write(handle, buf + wLen, outBufLen);
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

/*----------------------------------------------------------------------
|    EsdOutput_Write
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Write(EsdOutput* self, void* buffer, BLT_Size size)
{
    int        watchdog = BLT_ESD_OUTPUT_WRITE_WATCHDOG;
    BLT_Result result;

    /* ensure that the device is configured */
    result = EsdOutput_Configure(self);
    if (BLT_FAILED(result)) {
        /* reset the media type */
        BLT_PcmMediaType_Init(&self->media_type);
        return result;
    }
	
    while (size) {
		int nb_written = 0;
		if (self->media_type.channel_count == 1 ) {
			unsigned long i = 0;
			unsigned short *resampleBuf;
			unsigned short * srcBuf = (unsigned short *)buffer;
			resampleBuf = (unsigned short *)malloc(2*size);
			for(i=0;i<size/2;i++){
				resampleBuf[i*2]=srcBuf[i];
				resampleBuf[(i*2)+1]=srcBuf[i];
			}
			
			/*nb_written = write(self->device_handle, resampleBuf, 2*size);	*/
			nb_written = OutPutToDevice(self->device_handle, resampleBuf, 2*size);
			nb_written/=2;
			free(resampleBuf);

		}else{
			/*nb_written = write(self->device_handle, buffer, size);*/
			nb_written= OutPutToDevice(self->device_handle, buffer, size);
		}

        if (nb_written == -1) {
            if (errno == EAGAIN) {
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
|    EsdOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_PutPacket(BLT_PacketConsumer* _self,
                    BLT_MediaPacket*    packet)
{
    EsdOutput*              self = ATX_SELF(EsdOutput, BLT_PacketConsumer);
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
    result = EsdOutput_SetFormat(self, media_type);
    if (BLT_FAILED(result)) return result;

    self->equalizer = g_equalizer;
    if ( media_type->channel_count == 2 &&
         media_type->bits_per_sample == 16 &&
         self->equalizer) {
	dst = self->equalizer(BLT_MediaPacket_GetPayloadBuffer(packet), size);
    }

    /* send the payload to the drvice */
    if ( dst != NULL ) {
        result = EsdOutput_Write(self, 
                                 dst, 
                                 size);
        ATX_FreeMemory(dst);
    }
    else {
        result = EsdOutput_Write(self, 
                                 BLT_MediaPacket_GetPayloadBuffer(packet), 
                                 size);
    }

    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_QueryMediaType(BLT_MediaPort*        _self,
				         BLT_Ordinal           index,
				         const BLT_MediaType** media_type)
{
    EsdOutput* self = ATX_SELF(EsdOutput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    EsdOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    EsdOutput*                self;
	int connection;
	
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;

    ATX_LOG_FINE("EsdOutput::Create");

	/*Check daemon started or not*/
	connection = esd_open_sound(NULL);
	if ( connection < 0 ) {
		return BLT_ERROR_NO_SUCH_DEVICE;
	}
	esd_close(connection);


    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(EsdOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    self->state                      = BLT_ESD_OUTPUT_STATE_CLOSED;
    self->device_handle              = BLT_ESD_OUTPUT_INVALID_HANDLE;
    self->device_flags               = 0;
    self->media_type.sample_rate     = 0;
    self->media_type.channel_count   = 0;
    self->media_type.bits_per_sample = 0;
    self->media_type.sample_format   = 0;

    /* setup the expected media type */
    BLT_PcmMediaType_Init(&self->expected_media_type);
    self->expected_media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
    self->expected_media_type.bits_per_sample = 16;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, EsdOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, EsdOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, EsdOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE(self, EsdOutput, BLT_OutputNode);
    ATX_SET_INTERFACE(self, EsdOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
EsdOutput_Destroy(EsdOutput* self)
{
    ATX_LOG_FINE("EsdOutput::Destroy");

    /* close the device */
    EsdOutput_Close(self);

    /* free the name */
    ATX_String_Destruct(&self->device_name);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       EsdOutput_Start
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_Start(BLT_MediaNode* _self)
{
    EsdOutput* self = ATX_SELF_EX(EsdOutput, BLT_BaseMediaNode, BLT_MediaNode);

    /* open the device */
    ATX_LOG_FINER("EsdOutput::Start");
    EsdOutput_Open(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       EsdOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_Stop(BLT_MediaNode* _self)
{
    EsdOutput* self = ATX_SELF_EX(EsdOutput, BLT_BaseMediaNode, BLT_MediaNode);

    /* close the device */
    ATX_LOG_FINER("EsdOutput::Stop");
    EsdOutput_Close(self);

    return BLT_SUCCESS;
}
 
/*----------------------------------------------------------------------
|    EsdOutput_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_Pause(BLT_MediaNode* _self)
{
    return BLT_SUCCESS;
}
                   
/*----------------------------------------------------------------------
|   EsdOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    EsdOutput* self = ATX_SELF_EX(EsdOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    EsdOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_Seek(BLT_MediaNode* _self,
               BLT_SeekMode*  mode,
               BLT_SeekPoint* point)
{
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    EsdOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutput_GetStatus(BLT_OutputNode*       _self,
                    BLT_OutputNodeStatus* status)
{
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(EsdOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(EsdOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(EsdOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(EsdOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(EsdOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(EsdOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(EsdOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(EsdOutput, BLT_MediaPort)
    EsdOutput_GetName,
    EsdOutput_GetProtocol,
    EsdOutput_GetDirection,
    EsdOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(EsdOutput, BLT_PacketConsumer)
    EsdOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(EsdOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    EsdOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    EsdOutput_Start,
    EsdOutput_Stop,
    EsdOutput_Pause,
    BLT_BaseMediaNode_Resume,
    EsdOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(EsdOutput, BLT_OutputNode)
    EsdOutput_GetStatus,
    NULL,
     EsdOutput_SetEqualizerFunction
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(EsdOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|       EsdOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
EsdOutputModule_Probe(BLT_Module*              self, 
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

            /* the name should be 'esd:<name>' */
            if (constructor->name == NULL ||
                !ATX_StringsEqualN(constructor->name, "esd:", 4)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("EsdOutputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(EsdOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(EsdOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(EsdOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(EsdOutputModule, EsdOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(EsdOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    EsdOutputModule_CreateInstance,
    EsdOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define EsdOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(EsdOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_EsdOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("ESD Output", NULL, 0, 
                                 &EsdOutputModule_BLT_ModuleInterface,
                                 &EsdOutputModule_ATX_ReferenceableInterface,
                                 object);
}
