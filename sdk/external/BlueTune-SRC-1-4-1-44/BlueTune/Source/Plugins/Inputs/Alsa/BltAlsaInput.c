/*****************************************************************
|
|      Alsa: BltAlsaInput.c
|
|      ALSA Input Module
|
|      (c) 2002-2005 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <alsa/asoundlib.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltAlsaInput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltModule.h"
#include "BltByteStreamProvider.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.alsa")

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define BLT_ALSA_DEFAULT_BUFFER_SIZE 65536  /* frames */
#define BLT_ALSA_DEFAULT_PERIOD_SIZE 4096   /* frames */

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AlsaInputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(AlsaInputStream, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(AlsaInputStream, ATX_InputStream)

ATX_DECLARE_INTERFACE_MAP(AlsaInput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AlsaInput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(AlsaInput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(AlsaInput, BLT_InputStreamProvider)

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} AlsaInputModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_Referenceable);
    ATX_IMPLEMENTS(ATX_InputStream);

    /* members */
    ATX_Cardinal     reference_count;
    ATX_String       device_name;
    snd_pcm_t*       device_handle;
    BLT_PcmMediaType media_type;
    unsigned int     bytes_per_frame;
    ATX_Position     position;
    ATX_Int32        size;
} AlsaInputStream;

typedef struct {
     /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamProvider);

    /* members */
    AlsaInputStream* stream;
} AlsaInput;

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
|    AlsaInputStream_ParseName
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInputStream_ParseName(AlsaInputStream* self,
                          BLT_CString      name)
{
    /* the name format is: alsa:{option=value}[;{option=value}...] */

    const char*   device_name   = "default";
    unsigned int  channel_count = 2;     /* default = stereo */
    unsigned int  sample_rate   = 44100; /* default = cd sample rate */
    unsigned long duration      = 0;     /* no limit */
    
    /* make a copy of the name so that we can null-terminate options */
    char         name_buffer[1024]; 
    unsigned int name_length = ATX_StringLength(name);
    char*        option = name_buffer;
    char*        look = option;
    char         c;

    /* copy the name in the buffer */
    if (name_length+1 >= sizeof(name_buffer) || name_length < 5) {
        return ATX_ERROR_INVALID_PARAMETERS;
    }
    ATX_CopyString(name_buffer, name+5);

    /* parse the options */
    do {
        c = *look;
        if (*look == ';' || *look == '\0') {
            int value = 0;

            /* force-terminate the option */
            *look = '\0';
            
            /* parse option */
            if (ATX_StringsEqualN(option, "device=", 7)) {
                device_name = option+7;
            } else if (ATX_StringsEqualN(option, "ch=", 3)) {
                ATX_ParseInteger(option+3, &value, ATX_FALSE);
                channel_count = value;
            } else if (ATX_StringsEqualN(option, "sr=", 3)) {
                ATX_ParseInteger(option+3, &value, ATX_FALSE);
                sample_rate = value;
            } else if (ATX_StringsEqualN(option, "duration=", 9)) {
                ATX_ParseInteger(option+9, &value, ATX_FALSE);
                duration = value;
            } 
        }

        look++;
    } while (c != '\0');

    /* device name */
    ATX_String_Assign(&self->device_name, device_name);
    
    /* format */
    if (channel_count == 0) {
        ATX_LOG_SEVERE("AlsaInput - invalid channel count");
        return ATX_ERROR_INVALID_PARAMETERS;
    }
    if (sample_rate ==0) {
        ATX_LOG_SEVERE("AlsaInput - invalid sample rate");
        return ATX_ERROR_INVALID_PARAMETERS;
    }
    BLT_PcmMediaType_Init(&self->media_type);
    self->media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
    self->media_type.channel_count   = channel_count;
    self->media_type.sample_rate     = sample_rate;
    self->media_type.bits_per_sample = 16;

    /* stream size */
    {
        ATX_Int64 size;
        ATX_Int64_Set_Int32(size, duration);
        ATX_Int64_Mul_Int32(size, sample_rate);
        ATX_Int64_Mul_Int32(size, channel_count*2);
        ATX_Int64_Div_Int32(size, 1000);
        self->size = ATX_Int64_Get_Int32(size);
    }

    /* debug info */
    ATX_LOG_FINER_3("AlsaInput - recording from '%s': ch=%d, sr=%d", 
                    device_name,
                    channel_count,
                    sample_rate);
    if (duration != 0) {
        ATX_LOG_FINER_2("AlsaInput - duration=%dms (%d bytes)", duration, self->size);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Construct
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInputStream_Construct(AlsaInputStream* self, BLT_CString name)
{
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_sw_params_t* sw_params;
    unsigned int         rate;
    snd_pcm_uframes_t    buffer_size = BLT_ALSA_DEFAULT_BUFFER_SIZE;
    snd_pcm_uframes_t    period_size = BLT_ALSA_DEFAULT_PERIOD_SIZE;
    int                  ior;
    BLT_Result           result;

    /* set initial values */
    self->reference_count = 1;

    /* try to parse the name */
    result = AlsaInputStream_ParseName(self, name);
    if (BLT_FAILED(result)) {
        ATX_LOG_SEVERE_1("AlsaInput - invalid name %s", name);
        return result;
    }

    /* compute some parameters */
    self->bytes_per_frame = 
        self->media_type.channel_count *
        self->media_type.bits_per_sample/8;
        
    /* open the device */
    ior = snd_pcm_open(&self->device_handle,
                       ATX_CSTR(self->device_name),
                       SND_PCM_STREAM_CAPTURE,
                       0);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - snd_pcm_open failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* allocate a new blank configuration */
    snd_pcm_hw_params_alloca_no_assert(&hw_params);
    snd_pcm_hw_params_any(self->device_handle, hw_params);

    /* use interleaved access */
    ior = snd_pcm_hw_params_set_access(self->device_handle, hw_params, 
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - set 'access' failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* set the sample rate */
    rate = self->media_type.sample_rate;
    ior = snd_pcm_hw_params_set_rate_near(self->device_handle, 
                                          hw_params, 
                                          &rate, NULL);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AldaInput - set 'rate' failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* set the number of channels */
    ior = snd_pcm_hw_params_set_channels(self->device_handle, hw_params,
                                         self->media_type.channel_count);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - set 'channels' failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* set the sample format */
    ior = snd_pcm_hw_params_set_format(self->device_handle, hw_params,
                                       SND_PCM_FORMAT_S16);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - set 'format' failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* set the period size */
    ior = snd_pcm_hw_params_set_period_size_near(self->device_handle, 
						 hw_params,
						 &period_size,
						 NULL);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput::Configure - set 'period size' failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* set the buffer size */
    ior = snd_pcm_hw_params_set_buffer_size_near(self->device_handle,
                                                 hw_params, 
                                                 &buffer_size);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - set 'buffer size' failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* activate this configuration */
    ior = snd_pcm_hw_params(self->device_handle, hw_params);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - snd_pcm_hw_params failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* configure the software parameters */
    snd_pcm_sw_params_alloca_no_assert(&sw_params);
    snd_pcm_sw_params_current(self->device_handle, sw_params);

    /* set the buffer alignment */
    /* NOTE: this call is now obsolete */
    /* snd_pcm_sw_params_set_xfer_align(self->device_handle, sw_params, 1); */

    /* activate the sofware parameters */
    ior = snd_pcm_sw_params(self->device_handle, sw_params);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaInput - snd_pcm_sw_params failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* prepare the device */
    ior = snd_pcm_prepare(self->device_handle);
    if (ior != 0) {
        ATX_LOG_SEVERE_1("AlsaOutput::Prepare: - snd_pcm_prepare failed (%d)", ior);
        return BLT_FAILURE;
    }

    /* print status info */
    {
        snd_pcm_uframes_t val;
        if (rate != self->media_type.sample_rate) {
            ATX_LOG_FINER_1("AlsaOutput::Prepare - actual sample = %d", rate);
        }
        ATX_LOG_FINER_1("AlsaStream::Prepare - actual buffer size = %d", buffer_size);
        ATX_LOG_FINER_1("AlsaOutput::Prepare - buffer size = %d", buffer_size); 
        snd_pcm_sw_params_get_start_threshold(sw_params, &val);
        ATX_LOG_FINER_1("AlsaOutput::Prepare - start threshold = %d", val); 
        snd_pcm_sw_params_get_stop_threshold(sw_params, &val);
        ATX_LOG_FINER_1("AlsaOutput::Prepare - stop threshold = %d", val); 
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Create
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInputStream_Create(BLT_CString name, AlsaInputStream** stream)
{
    BLT_Result result;

    /* allocate the object */
    *stream = (AlsaInputStream*)
        ATX_AllocateZeroMemory(sizeof(AlsaInputStream));
    if (stream == NULL) {
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    result = AlsaInputStream_Construct(*stream, name);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory((void*)(stream));
        *stream = NULL;
        return result;
    }

    /* setup the interface */
    ATX_SET_INTERFACE(*stream, AlsaInputStream, ATX_Referenceable);
    ATX_SET_INTERFACE(*stream, AlsaInputStream, ATX_InputStream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Destruct
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInputStream_Destruct(AlsaInputStream* self)
{
    snd_pcm_drop(self->device_handle);
    snd_pcm_close(self->device_handle);
    ATX_String_Destruct(&self->device_name);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInputStream_Destroy(AlsaInputStream* self)
{
    /* destruct the stream */
    AlsaInputStream_Destruct(self);

    /* free the memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Read
+---------------------------------------------------------------------*/
ATX_METHOD
AlsaInputStream_Read(ATX_InputStream* _self, 
                     ATX_Any          buffer,
                     ATX_Size         bytes_to_read,
                     ATX_Size*        bytes_read)
{
    int               watchdog = 5; /* max number of retries */
    snd_pcm_sframes_t frames_to_read;
    snd_pcm_sframes_t frames_read;
    AlsaInputStream*  self = ATX_SELF(AlsaInputStream, ATX_InputStream);
    
    /* default values */
    if (bytes_read) *bytes_read = 0;

    /* check if we have a size limit */
    if (self->size != 0) {
        unsigned int max_read = self->size - self->position;
        if (bytes_to_read > max_read) {
            bytes_to_read = max_read;
        }
        if (bytes_to_read == 0) {
            /* no more bytes to read */
            return ATX_ERROR_EOS;
        }
    }

    /* read the samples from the input device */
    frames_to_read = bytes_to_read/self->bytes_per_frame;

    /* try to read and handle overruns */
    do {
        frames_read = snd_pcm_readi(self->device_handle, 
                                    buffer, frames_to_read);
        if (frames_read > 0) {
            unsigned int byte_count = frames_read*self->bytes_per_frame;
            if (bytes_read) *bytes_read = byte_count;
            self->position += byte_count;
            return ATX_SUCCESS;
        }
    
        /* if this failed, try to recover */
        {
            int               io_result;
            snd_pcm_status_t* status;
            snd_pcm_state_t   state;

            snd_pcm_status_alloca_no_assert(&status);
            io_result = snd_pcm_status(self->device_handle, status);
            if (io_result != 0) {
                return BLT_FAILURE;
            }
            state = snd_pcm_status_get_state(status);
            if (state == SND_PCM_STATE_XRUN) {
                ATX_LOG_WARNING("AlsaInput::Read - **** OVERRUN *****");
            
                /* re-prepare the channel */
                io_result = snd_pcm_prepare(self->device_handle);
                if (io_result != 0) return ATX_FAILURE;
            }
        }

        ATX_LOG_WARNING("AlsaInput::Read - **** RETRY ****");

    } while (watchdog--);

    
    ATX_LOG_SEVERE("AlsaInput::Read - **** THE WATCHDOG BIT US *****");

    /* nothing can be read */
    if (bytes_read) *bytes_read = 0;
    return ATX_FAILURE;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
AlsaInputStream_Seek(ATX_InputStream* self, ATX_Position  position)
{
    /* not seekable */
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(position);

    return ATX_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
AlsaInputStream_Tell(ATX_InputStream* _self, ATX_Position* position)
{
    AlsaInputStream* self = ATX_SELF(AlsaInputStream, ATX_InputStream);

    /* return the current position */
    *position = self->position;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD 
AlsaInputStream_GetSize(ATX_InputStream* _self, 
                        ATX_LargeSize*   size)
{
    AlsaInputStream*  self = ATX_SELF(AlsaInputStream, ATX_InputStream);

    *size = self->size;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInputStream_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD 
AlsaInputStream_GetAvailable(ATX_InputStream* self, 
                             ATX_LargeSize*   available)
{
    ATX_COMPILER_UNUSED(self);

    *available = 0;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlsaInputStream)
    ATX_GET_INTERFACE_ACCEPT(AlsaInputStream, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(AlsaInputStream, ATX_InputStream)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AlsaInputStream, ATX_InputStream)
    AlsaInputStream_Read,
    AlsaInputStream_Seek,
    AlsaInputStream_Tell,
    AlsaInputStream_GetSize,
    AlsaInputStream_GetAvailable
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(AlsaInputStream, reference_count) 

/*----------------------------------------------------------------------
|    AlsaInput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInput_Destroy(AlsaInput* self)
{
    ATX_LOG_FINE("AlsaInput::Destroy");

    /* release the byte stream */
    AlsaInputStream_Release(&ATX_BASE(self->stream, ATX_Referenceable));
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AlsaInput_Create
+---------------------------------------------------------------------*/
static BLT_Result
AlsaInput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    AlsaInput*                input;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;

    ATX_LOG_FINE("AlsaInput::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    input = ATX_AllocateZeroMemory(sizeof(AlsaInput));
    if (input == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(input, BLT_BaseMediaNode), module, core);
    
    /* create a stream for the input */
    result = AlsaInputStream_Create(constructor->name, &input->stream);
    if (ATX_FAILED(result)) {
        goto failure;
    }

    /* construct reference */
    ATX_SET_INTERFACE_EX(input, AlsaInput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(input, AlsaInput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (input, AlsaInput, BLT_MediaPort);
    ATX_SET_INTERFACE   (input, AlsaInput, BLT_InputStreamProvider);
    *object = &ATX_BASE_EX(input, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;

failure:
    AlsaInput_Destroy(input);
    *object = NULL;
    return result;
}

/*----------------------------------------------------------------------
|   AlsaInput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaInput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    AlsaInput* self = ATX_SELF_EX(AlsaInput, BLT_BaseMediaNode, BLT_MediaNode);
    if (ATX_StringsEqual(name, "output")) {
        /* we implement the BLT_MediaPort interface ourselves */
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    AlsaInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaInput_QueryMediaType(BLT_MediaPort*        _self,
                         BLT_Ordinal           index,
                         const BLT_MediaType** media_type)
{
    AlsaInput* self = ATX_SELF(AlsaInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->stream->media_type.base;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|       AlsaInput_GetStream
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaInput_GetStream(BLT_InputStreamProvider* _self,
                    ATX_InputStream**        stream)
{
    AlsaInput* self = ATX_SELF(AlsaInput, BLT_InputStreamProvider);
    
    /* return our stream object */
    *stream = &ATX_BASE(self->stream, ATX_InputStream);
    ATX_REFERENCE_OBJECT(*stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlsaInput)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaInput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaInput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (AlsaInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (AlsaInput, BLT_InputStreamProvider)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AlsaInput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AlsaInput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AlsaInput, 
                                         "output", 
                                         STREAM_PULL, 
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AlsaInput, BLT_MediaPort)
    AlsaInput_GetName,
    AlsaInput_GetProtocol,
    AlsaInput_GetDirection,
    AlsaInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AlsaInput, BLT_InputStreamProvider)
    AlsaInput_GetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AlsaInput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|       AlsaInputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AlsaInputModule_Probe(BLT_Module*              self, 
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

            /* we need a alsa name */
            if (constructor->name == NULL) return BLT_FAILURE;

            /* the input protocol should be NONE, and the output */
            /* protocol should be STREAM_PULL                    */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_NONE) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL)) {
                return BLT_FAILURE;
            }

            /* check the name */
            if (ATX_StringsEqualN(constructor->name, "alsa:", 5)) {
                /* this is an exact match for us */
                *match = BLT_MODULE_PROBE_MATCH_EXACT;
            } else {
                return BLT_FAILURE;
            }

            ATX_LOG_FINE_1("AlsaInputModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AlsaInputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaInputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AlsaInputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AlsaInputModule, AlsaInput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AlsaInputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    AlsaInputModule_CreateInstance,
    AlsaInputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AlsaInputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AlsaInputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_AlsaInputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("ALSA Input", NULL, 0,
                                 &AlsaInputModule_BLT_ModuleInterface,
                                 &AlsaInputModule_ATX_ReferenceableInterface,
                                 object);
}
