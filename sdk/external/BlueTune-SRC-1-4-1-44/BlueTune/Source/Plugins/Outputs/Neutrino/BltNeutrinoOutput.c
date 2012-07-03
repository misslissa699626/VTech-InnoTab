/*****************************************************************
|
|      File: BltNeutrinoOutput.c
|
|      Neutrino Output Module
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#ifndef STRICT
#define STRICT
#endif

#include <sys/asoundlib.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltNeutrinoOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"
#include "BltDebug.h"

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputModule)
static const BLT_ModuleInterface NeutrinoOutputModule_BLT_ModuleInterface;

ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutput)
static const BLT_MediaNodeInterface NeutrinoOutput_BLT_MediaNodeInterface;

ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputInputPort)
static const BLT_MediaPortInterface NeutrinoOutputInputPort_BLT_MediaPortInterface;
static const BLT_PacketConsumerInterface NeutrinoOutputInputPort_BLT_PacketConsumerInterface;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE          4096
#define BLT_NEUTRINO_OUTPUT_DEFAULT_MIN_FRAGMENTS  1
#define BLT_NEUTRINO_OUTPUT_DEFAULT_MAX_FRAGMENTS  64

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_Cardinal reference_count;
} NeutrinoOutputModule;

typedef enum {
    BLT_NEUTRINO_OUTPUT_STATE_CLOSED,
    BLT_NEUTRINO_OUTPUT_STATE_OPEN,
    BLT_NEUTRINO_OUTPUT_STATE_PREPARED
} NeutrinoOutputState;

typedef struct {
    BLT_Cardinal                   reference_count;
    BLT_Core                       core;
    NeutrinoOutputState            state;
    int                            card_id;
    int                            device_id;
    snd_pcm_t*                     device_handle;
    BLT_MediaTypeStandardExtension media_format;
    BLT_Cardinal                   max_fragments;
    BLT_Cardinal                   min_fragments;
    struct {
        unsigned char buffer[BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE];
        BLT_Cardinal  valid_bytes;
    }                              fragment;
} NeutrinoOutput;

/*----------------------------------------------------------------------
|    prototypes
+---------------------------------------------------------------------*/
static BLT_Result NeutrinoOutput_Close(NeutrinoOutput* output);

/*----------------------------------------------------------------------
|    NeutrinoOutput_SetState
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_SetState(NeutrinoOutput* output, NeutrinoOutputState state)
{
    if (state != output->state) {
        BLT_Debug("NeutrinoOutput::SetState - from %d to %d\n",
                  output->state, state);
    }
    output->state = state;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NeutrinoOutput_Open
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Open(NeutrinoOutput* output)
{
    int io_result;

    switch (output->state) {
      case BLT_NEUTRINO_OUTPUT_STATE_CLOSED:
        BLT_Debug("NeutrinoOutput::Open - snd_pcm_open\n");
        output->fragment.valid_bytes = 0;
        if (output->card_id == 0) {
            io_result = snd_pcm_open_preferred(&output->device_handle,
                                               NULL,
                                               NULL,
                                               SND_PCM_OPEN_PLAYBACK);
        } else {
            io_result = snd_pcm_open(&output->device_handle,
                                     output->card_id,
                                     output->device_id,
                                     SND_PCM_OPEN_PLAYBACK);
        }
     
        if (io_result != 0) {
            output->device_handle = NULL;
            return BLT_FAILURE;
        }
        break;

      case BLT_NEUTRINO_OUTPUT_STATE_OPEN:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_NEUTRINO_OUTPUT_STATE_PREPARED:
        return BLT_FAILURE;
    }

    /* update the state */
    NeutrinoOutput_SetState(output, BLT_NEUTRINO_OUTPUT_STATE_OPEN);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NeutrinoOutput_Close
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Close(NeutrinoOutput* output)
{
    switch (output->state) {
      case BLT_NEUTRINO_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_NEUTRINO_OUTPUT_STATE_PREPARED:
        /* wait for buffers to finish */
        BLT_Debug("NeutrinoOutput::Close - snd_pcm_drain\n");
        snd_pcm_playback_drain(output->device_handle);
        /* FALLTHROUGH */

      case BLT_NEUTRINO_OUTPUT_STATE_OPEN:
        /* close the device */
        BLT_Debug("NeutrinoOutput::Close - snd_pcm_close\n");
        snd_pcm_close(output->device_handle);
        output->device_handle = NULL;
        output->fragment.valid_bytes = 0;
        break;
    }

    /* update the state */
    NeutrinoOutput_SetState(output, BLT_NEUTRINO_OUTPUT_STATE_CLOSED);

    return BLT_SUCCESS;
}

#if 0
/*----------------------------------------------------------------------
|    NeutrinoOutput_Reset
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Reset(NeutrinoOutput* output)
{
    switch (output->state) {
      case BLT_NEUTRINO_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_NEUTRINO_OUTPUT_STATE_PREPARED:
      case BLT_NEUTRINO_OUTPUT_STATE_OPEN:
        /* discard samples buffered by the driver */
        snd_pcm_playback_drain(output->device_handle);
        break;
    }

    /* update the state */
    NeutrinoOutput_SetState(output, BLT_NEUTRINO_OUTPUT_STATE_OPEN);

    return BLT_SUCCESS;
}
#endif

/*----------------------------------------------------------------------
|    NeutrinoOutput_Drain
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Drain(NeutrinoOutput* output)
{
    switch (output->state) {
      case BLT_NEUTRINO_OUTPUT_STATE_CLOSED:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_NEUTRINO_OUTPUT_STATE_PREPARED:
      case BLT_NEUTRINO_OUTPUT_STATE_OPEN:
        /* flush samples buffered by the driver */
        BLT_Debug("NeutrinoOutput::Drain - snd_pcm_flush\n");
        snd_pcm_channel_flush(output->device_handle,
                              SND_PCM_CHANNEL_PLAYBACK);
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NeutrinoOutput_Prepare
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Prepare(NeutrinoOutput* output)
{
    BLT_Result               result;
    snd_pcm_channel_params_t params;
    int                      io_result;

    switch (output->state) {
      case BLT_NEUTRINO_OUTPUT_STATE_CLOSED:
        /* first, we need to open the device */
        result = NeutrinoOutput_Open(output);
        if (BLT_FAILED(result)) return result;

        /* FALLTHROUGH */

      case BLT_NEUTRINO_OUTPUT_STATE_OPEN:
        /* configure the device */
        ATX_SetMemory((void*)&params, 0, sizeof(params));
        params.channel             = SND_PCM_CHANNEL_PLAYBACK;
        params.mode                = SND_PCM_MODE_BLOCK;
        params.format.interleave   = 1;
        params.format.rate         = output->media_format.sample_rate;
        params.format.voices       = output->media_format.channel_count;
        params.format.format       = SND_PCM_SFMT_S16;
        params.start_mode          = SND_PCM_START_FULL;
        params.stop_mode           = SND_PCM_STOP_STOP;
        params.buf.block.frag_size = BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE;
        params.buf.block.frags_min = output->min_fragments;
        params.buf.block.frags_max = output->max_fragments;
        BLT_Debug("NeutrinoOutput::Prepare - snd_pcm_channel_params\n");
        io_result = snd_pcm_channel_params(output->device_handle, &params);
        if (io_result != 0) {
            BLT_Debug("NeutrinoOutput::Prepare: - snd_pcm_channel_params failed (%d)\n",
                      io_result);
            return BLT_FAILURE;
        }

        /* prepare the device */
        BLT_Debug("NeutrinoOutput::Prepare - snd_pcm_playback_prepare\n");
        io_result = snd_pcm_playback_prepare(output->device_handle);
        if (io_result != 0) {
            BLT_Debug("NeutrinoOutput::Prepare: - snd_pcm_playback_prepare failed (%d)\n",
                      io_result);
            return BLT_FAILURE;
        }

        /* get the actual parameters */
        {
            snd_pcm_channel_setup_t setup;
            ATX_SetMemory(&setup, 0, sizeof(setup));
            setup.channel = SND_PCM_CHANNEL_PLAYBACK;
            io_result = snd_pcm_channel_setup(output->device_handle, &setup);
            if (io_result == 0) {
                BLT_Debug("NeutrinoOutput::Prepare - frag_size = %d\n", 
                          setup.buf.block.frag_size);
                BLT_Debug("NeutrinoOutput::Prepare - frags = %d\n", 
                          setup.buf.block.frags);
                BLT_Debug("NeutrinoOutput::Prepare - frags_min = %d\n", 
                          setup.buf.block.frags_min);
                BLT_Debug("NeutrinoOutput::Prepare - frags_max = %d\n",
                          setup.buf.block.frags_max);
            }
        }

        break;

      case BLT_NEUTRINO_OUTPUT_STATE_PREPARED:
        /* ignore */
        return BLT_SUCCESS;
    }

    /* update the state */
    NeutrinoOutput_SetState(output, BLT_NEUTRINO_OUTPUT_STATE_PREPARED);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NeutrinoOutput_Write
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Write(NeutrinoOutput* output, void* buffer, BLT_Size size)
{
    int        io_result;
    BLT_Result result;

    /* ensure that the device is prepared */
    result = NeutrinoOutput_Prepare(output);
    if (BLT_FAILED(result)) return result;

    /* write samples to the device */
    io_result = snd_pcm_write(output->device_handle, buffer, size);
    if (io_result == (int)size) return BLT_SUCCESS;

    /* we reach this point if the first write failed */
    {
        snd_pcm_channel_status_t status;
        ATX_SetMemory(&status, 0, sizeof(status));
        status.channel = SND_PCM_CHANNEL_PLAYBACK;

        io_result = snd_pcm_channel_status(output->device_handle, &status);
        if (io_result != 0) {
            return BLT_FAILURE;
        }
        if (status.status == SND_PCM_STATUS_READY ||
            status.status == SND_PCM_STATUS_UNDERRUN) {
            BLT_Debug("NeutrinoOutput::Write - **** UNDERRUN *****\n");
            
            /* re-prepare the channel */
            io_result = snd_pcm_playback_prepare(output->device_handle);
            if (io_result != 0) {
                return BLT_FAILURE;
            }
            
            /* write again */
            BLT_Debug("NeutrinoOutput::Write - **** RETRY *****\n");
            io_result = snd_pcm_write(output->device_handle, buffer, size);
            if (io_result != (int)size) {
                return BLT_FAILURE;
            } else {
                return BLT_SUCCESS;
            } 
        }
    }
    
    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|    NeutrinoOutputInputPort_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputInputPort_PutPacket(BLT_PacketConsumerInstance* instance,
                                  BLT_MediaPacket*            packet)
{
    NeutrinoOutput*                 output = (NeutrinoOutput*)instance;
    BLT_MediaTypeStandardExtension* std_ext;
    BLT_MediaType                   media_type;
    BLT_Size                        available;
    BLT_Size                        space;
    BLT_Size                        chunk;
    BLT_ByteBuffer                  payload;
    BLT_Result                      result;

    /* check parameters */
    if (packet == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* check the payload size */
    available = BLT_MediaPacket_GetPayloadSize(packet);
    if (available == 0) return BLT_SUCCESS;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, &media_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type */
    if (media_type.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* get the standard extension */
    result = BLT_MediaType_GetExtension(&media_type, 
                                        BLT_MEDIA_TYPE_EXTENSION_ID_STANDARD,
                                        (BLT_MediaTypeExtension**)&std_ext);
    if (BLT_FAILED(result)) return result;

    /* compare the media format with the current format */
    if (std_ext->sample_rate     != output->media_format.sample_rate   ||
        std_ext->channel_count   != output->media_format.channel_count ||
        std_ext->bits_per_sample != output->media_format.bits_per_sample) {
        /* new format */

        /* check the format */
        if (std_ext->sample_rate     == 0 ||
            std_ext->channel_count   == 0 ||
            std_ext->bits_per_sample == 0) {
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        
        /* copy the format */
        output->media_format = *std_ext;

        /* drain any pending samples */
        result = NeutrinoOutput_Drain(output);
        if (BLT_FAILED(result)) return result;
    }

    /* try to complete a fragment */
    payload = BLT_MediaPacket_GetPayloadBuffer(packet);
    space = BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE - output->fragment.valid_bytes;
    chunk = (available < space ? available : space);
    ATX_CopyMemory(&output->fragment.buffer[output->fragment.valid_bytes],
                   payload, chunk);
    output->fragment.valid_bytes += chunk;
    payload                      += chunk;
    if (available < space) {
        /* we could not fill a full fragment, just return */
        return BLT_SUCCESS;
    } else {
        /* update the number of available bytes */
        available -= chunk;
    }

    /* send the completed fragment */
    result = NeutrinoOutput_Write(output, output->fragment.buffer, 
                                  BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE);
    if (BLT_FAILED(result)) return BLT_FAILURE;
    output->fragment.valid_bytes = 0;

    /* transfer zero or more whole fragments */
    while (available >= BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE) {
        BLT_Size size = available;
        size -= available % BLT_NEUTRINO_OUTPUT_FRAGMENT_SIZE;
        result = NeutrinoOutput_Write(output, payload, size);
        if (BLT_FAILED(result)) return BLT_FAILURE;
        available -= size;
        payload += size;
    }

    /* copy any leftovers to the fragment buffer */
    if (available) {
        ATX_CopyMemory(output->fragment.buffer, payload, available);
        output->fragment.valid_bytes = available;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NeutrinoOutputInputPort_GetExpectedMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputInputPort_GetExpectedMediaType(BLT_MediaPortInstance* instance,
                                             BLT_Ordinal            index,
                                             BLT_MediaType*         media_type)
{
    BLT_COMPILER_UNUSED(instance);

    if (index == 0) {
        BLT_MediaType_Init(media_type, BLT_MEDIA_TYPE_ID_AUDIO_PCM);
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
static const BLT_MediaPortInterface
NeutrinoOutputInputPort_BLT_MediaPortInterface = {
    NeutrinoOutputInputPort_GetInterface,
    NeutrinoOutputInputPort_GetExpectedMediaType
};

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
static const BLT_PacketConsumerInterface
NeutrinoOutputInputPort_BLT_PacketConsumerInterface = {
    NeutrinoOutputInputPort_GetInterface,
    NeutrinoOutputInputPort_PutPacket
};

/*----------------------------------------------------------------------
|       standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputInputPort)
ATX_INTERFACE_MAP_ADD(NeutrinoOutputInputPort, BLT_MediaPort)
ATX_INTERFACE_MAP_ADD(NeutrinoOutputInputPort, BLT_PacketConsumer)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputInputPort)

/*----------------------------------------------------------------------
|    NeutrinoOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Create(BLT_Core*                core, 
                      BLT_ModuleParametersType parameters_type,
                      BLT_String               parameters, 
                      ATX_Object*              object)
{
    NeutrinoOutput*           output;
    /*BLT_MediaNodeConstructor* constructor = 
      (BLT_MediaNodeConstructor*)parameters;*/

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    output = ATX_AllocateZeroMemory(sizeof(NeutrinoOutput));
    if (output == NULL) {
        ATX_CLEAR_OBJECT(object);
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    output->reference_count              = 1;
    output->core                         = *core;
    output->state                        = BLT_NEUTRINO_OUTPUT_STATE_CLOSED;
    output->device_id                    = 0;
    output->device_handle                = NULL;
    output->media_format.sample_rate     = 0;
    output->media_format.channel_count   = 0;
    output->media_format.bits_per_sample = 0;
    output->min_fragments = BLT_NEUTRINO_OUTPUT_DEFAULT_MIN_FRAGMENTS;
    output->max_fragments = BLT_NEUTRINO_OUTPUT_DEFAULT_MAX_FRAGMENTS;
    output->fragment.valid_bytes         = 0;

    /* construct reference */
    ATX_INSTANCE(object)  = (ATX_Instance*)output;
    ATX_INTERFACE(object) = (ATX_Interface*)&NeutrinoOutput_BLT_MediaNodeInterface;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NeutrinoOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoOutput_Destroy(NeutrinoOutput* output)
{
    /* close the device */
    NeutrinoOutput_Close(output);

    /* free the object memory */
    ATX_FreeMemory(output);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoOutput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutput_Activate(BLT_MediaNodeInstance* instance, BLT_Stream* stream)
{
    NeutrinoOutput* output = (NeutrinoOutput*)instance;
	BLT_COMPILER_UNUSED(stream);
	
    BLT_Debug("NeutrinoOutput::Activate\n");

    /* open the device */
    NeutrinoOutput_Open(output);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoOutput_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutput_Deactivate(BLT_MediaNodeInstance* instance)
{
    NeutrinoOutput* output = (NeutrinoOutput*)instance;

    BLT_Debug("NeutrinoOutput::Deactivate\n");

    /* close the device */
    NeutrinoOutput_Close(output);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|       NeutrinoOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutput_GetPortByName(BLT_MediaNodeInstance* instance,
                          BLT_String             name,
                          BLT_MediaPort*         port)
{
    NeutrinoOutput* output = (NeutrinoOutput*)instance;

    if (ATX_StringsEqual(name, "input")) {
        ATX_INSTANCE(port)  = (BLT_MediaPortInstance*)output;
        ATX_INTERFACE(port) = &NeutrinoOutputInputPort_BLT_MediaPortInterface; 
        return BLT_SUCCESS;
    } else {
        ATX_CLEAR_OBJECT(port);
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    NeutrinoOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutput_Seek(BLT_MediaNodeInstance* instance,
                    BLT_SeekMode*          mode,
                    BLT_SeekPoint*         point)
{
    NeutrinoOutput* output = (NeutrinoOutput*)instance;
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* reset the device */
    snd_pcm_playback_drain(output->device_handle);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
static const BLT_MediaNodeInterface
NeutrinoOutput_BLT_MediaNodeInterface = {
    NeutrinoOutput_GetInterface,
    BLT_BaseMediaNode_GetInfo,
    NeutrinoOutput_GetPortByName,
    NeutrinoOutput_Activate,
    NeutrinoOutput_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    NeutrinoOutput_Seek
};

/*----------------------------------------------------------------------
|       ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_SIMPLE_REFERENCEABLE_INTERFACE(NeutrinoOutput, reference_count)

/*----------------------------------------------------------------------
|       standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutput)
ATX_INTERFACE_MAP_ADD(NeutrinoOutput, BLT_MediaNode)
ATX_INTERFACE_MAP_ADD(NeutrinoOutput, ATX_Referenceable)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutput)

/*----------------------------------------------------------------------
|       NeutrinoOutputModule_Create
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputModule_Create(BLT_Module* object)
{
    NeutrinoOutputModule* module;

    /* allocate memory for the object */
    module = (NeutrinoOutputModule*)
        ATX_AllocateMemory(sizeof(NeutrinoOutputModule));
    
    /* construct the object */
    module->reference_count = 1;

    /* create the object reference */
    ATX_INSTANCE(object)  = (BLT_ModuleInstance*)module;
    ATX_INTERFACE(object) = &NeutrinoOutputModule_BLT_ModuleInterface;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoOutputModule_Destroy
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputModule_Destroy(NeutrinoOutputModule* module)
{
    ATX_FreeMemory((void*)module);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoOutputModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputModule_Attach(BLT_ModuleInstance* instance, BLT_Core* core)
{
    BLT_COMPILER_UNUSED(instance);
    BLT_COMPILER_UNUSED(core);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoOutputModule_CreateInstance
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputModule_CreateInstance(BLT_ModuleInstance*      instance,
                                    BLT_Core*                core,
                                    BLT_ModuleParametersType parameters_type,
                                    BLT_AnyConst             parameters,
                                    const ATX_InterfaceId*   interface_id,
                                    ATX_Object*              object)
{
    BLT_COMPILER_UNUSED(instance);

    if (ATX_INTERFACE_IDS_EQUAL(interface_id, 
                                &ATX_INTERFACE_ID__BLT_MediaNode)) {
        return NeutrinoOutput_Create(core, 
                                     parameters_type, 
                                     parameters, 
                                     object);
    } else {
        return BLT_ERROR_INVALID_INTERFACE;
    }
}

/*----------------------------------------------------------------------
|       NeutrinoOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoOutputModule_Probe(BLT_ModuleInstance*      instance, 
                           BLT_Core*                core,
                           BLT_ModuleParametersType parameters_type,
                           BLT_AnyConst             parameters,
                           BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(instance);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if (!(constructor->spec.input.protocols &
                  BLT_MEDIA_NODE_PROTOCOL_PACKET) ||
                !(constructor->spec.output.protocols ==
                  BLT_MEDIA_NODE_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the input type should be unknown, or audio/pcm */
            if (!(constructor->spec.input.media_type.id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.input.media_type.id == 
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

            BLT_Debug("NeutrinoOutputModule::Probe - Ok [%d]\n", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|       BLT_Module interface
+---------------------------------------------------------------------*/
static const BLT_ModuleInterface NeutrinoOutputModule_BLT_ModuleInterface = {
    NeutrinoOutputModule_GetInterface,
    NeutrinoOutputModule_Attach,
    NeutrinoOutputModule_CreateInstance,
    NeutrinoOutputModule_Probe
};

/*----------------------------------------------------------------------
|       ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_SIMPLE_REFERENCEABLE_INTERFACE(NeutrinoOutputModule, 
                                             reference_count)

/*----------------------------------------------------------------------
|       standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputModule)
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputModule) 
ATX_INTERFACE_MAP_ADD(NeutrinoOutputModule, BLT_Module)
ATX_INTERFACE_MAP_ADD(NeutrinoOutputModule, ATX_Referenceable)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoOutputModule)

/*----------------------------------------------------------------------
|       module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_NeutrinoOutputModule_GetModuleObject(BLT_Module* object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return NeutrinoOutputModule_Create(object);
}

#/** PhEDIT attribute block
#-11:16777215
#0:6502:pcterm9:-3:-3:0
#6502:6599:pcterm9:0:-1:0
#6599:29377:pcterm9:-3:-3:0
#**  PhEDIT attribute block ends (-0000167)**/
