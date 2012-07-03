/*****************************************************************
|
|   Cross Fader Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "Fluo.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltCrossFader.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

#include <math.h>

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.general.cross-fader")

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderModule)
static const BLT_ModuleInterface CrossFaderModule_BLT_ModuleInterface;

ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFader)
static const BLT_MediaNodeInterface CrossFader_BLT_MediaNodeInterface;
static const ATX_PropertyListenerInterface 
CrossFader_ATX_PropertyListenerInterface;

ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderInputPort)

ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderOutputPort)

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define CROSS_FADER_PACKET_SIZE 4096

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_BaseModule base;
} CrossFaderModule;

typedef struct {
    BLT_PcmMediaType media_type;
    ATX_RingBuffer*  buffer;
    ATX_Size         position;
} CrossFaderInputPort;

typedef struct {
    ATX_List* packets;
} CrossFaderOutputPort;

typedef enum {
    CROSS_FADER_STATE_IN_START,
    CROSS_FADER_STATE_IN_MAIN
} CrossFaderState;

typedef struct {
    BLT_BaseMediaNode          base;
    BLT_Stream                 context;
    CrossFaderInputPort        input;
    CrossFaderOutputPort       output;
    CrossFaderState            state;
    ATX_PropertyListenerHandle listener_handle;
} CrossFader;

/*----------------------------------------------------------------------
|    CrossFader_SetupInput
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_SetupInput(CrossFader* fader, BLT_PcmMediaType* media_type)
{
    ATX_Result result;

    /* check the media type */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* clone the media type parameters */
    fader->input.media_type = *(BLT_PcmMediaType*)media_type;
    BLT_Debug("CrossFader_SetupInput - %dHz, %dch\n", 
              fader->input.media_type.sample_rate,
              fader->input.media_type.channel_count);

    /* reset the ring buffer */
    if (fader->input.buffer) {
        ATX_RingBuffer_Destroy(fader->input.buffer);
        fader->input.buffer = NULL;
    }
    result = ATX_RingBuffer_Create(44100*4*10, &fader->input.buffer);
    if (ATX_FAILED(result)) return result;
    
    /* reset other parameters */
    fader->input.position = 0;
    fader->state = CROSS_FADER_STATE_IN_START;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_PromotePacket
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_PromotePacket(CrossFader* fader, BLT_Size size) 
{
    BLT_MediaPacket* packet;
    unsigned char*   payload;
    BLT_Result       result;

    ATX_LOG_FINER_1("CrossFader::PromotePacket - size = %d", size);
    
    /* create a packet */
    result = BLT_Core_CreateMediaPacket(&fader->base.core,
                                        size,
                                        (const BLT_MediaType*)&fader->input.media_type,
                                        &packet);
    if (BLT_FAILED(result)) return result;

    /* get the addr of the buffer */
    payload = BLT_MediaPacket_GetPayloadBuffer(packet);

    /* read the data from the input ring buffer */
    result = ATX_RingBuffer_Read(fader->input.buffer, payload, size);
    if (BLT_FAILED(result)) {
        BLT_MediaPacket_Release(packet);
        return result;
    }

    /* update the size of the packet */
    BLT_MediaPacket_SetPayloadSize(packet, size);

    /* make the packet ready for the output */
    ATX_List_AddData(fader->output.packets, packet);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_Flush
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_Flush(CrossFader* fader)
{
    BLT_Size   buffered;
    BLT_Result result;

    /* shortcut */
    if (fader->input.buffer == NULL) return BLT_SUCCESS;

    /* see how much data we have */
    buffered = ATX_RingBuffer_GetAvailable(fader->input.buffer);
    
    /* output the data as packets */
    while (buffered) {
        BLT_Size packet_size;
        if (buffered >= CROSS_FADER_PACKET_SIZE) {
            packet_size = CROSS_FADER_PACKET_SIZE;
        } else {
            packet_size = buffered;
        }
        result = CrossFader_PromotePacket(fader, packet_size);
        if (BLT_FAILED(result)) return result;
        buffered -= packet_size;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_BufferPacket
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_BufferPacket(CrossFader* fader, BLT_MediaPacket* packet)
{
    BLT_Size   space;
    BLT_Size   payload_size;
    BLT_Any    payload_buffer;

    /* quick check */
    if (fader->input.buffer == NULL) return BLT_SUCCESS;

    /* see how much data we have */
    payload_size   = BLT_MediaPacket_GetPayloadSize(packet);
    payload_buffer = BLT_MediaPacket_GetPayloadBuffer(packet);

    /* shortcut */
    if (payload_size == 0) return BLT_SUCCESS;

    /* ensure that we have enough space in the buffer */
    space = ATX_RingBuffer_GetSpace(fader->input.buffer);
    if (space < payload_size) {
        /* not enough space */
        BLT_Size available = ATX_RingBuffer_GetAvailable(fader->input.buffer);
        if (available >= payload_size) {
            CrossFader_PromotePacket(fader, payload_size);
        } else {
            CrossFader_PromotePacket(fader, available);
        }
        space = ATX_RingBuffer_GetSpace(fader->input.buffer);
        if (space < payload_size) {
            /* we can't handle it */
            return BLT_FAILURE;
        }
    }

    /* copy the data to the input buffer */
    ATX_LOG_FINER_1("CrossFader::BufferPacket - buffering %d", payload_size);
    ATX_RingBuffer_Write(fader->input.buffer, payload_buffer, payload_size);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFaderInputPort_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFaderInputPort_PutPacket(BLT_PacketConsumerInstance* instance,
                              BLT_MediaPacket*            packet)
{
    CrossFader*       fader = (CrossFader*)instance;
    BLT_PcmMediaType* media_type;
    ATX_Result        result;

    ATX_LOG_FINER_1("CrossFaderInputPort::PutPacket - state = %s",
                    fader->state == CROSS_FADER_STATE_IN_START ? "START" :
                    fader->state == CROSS_FADER_STATE_IN_MAIN ? "MAIN" :
                    "???");

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
    if (BLT_FAILED(result)) return result;

    /* check the if media type is PCM */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* check if the media type has changed */
    if (media_type->sample_rate     != fader->input.media_type.sample_rate     ||
        media_type->channel_count   != fader->input.media_type.channel_count   ||
        media_type->bits_per_sample != fader->input.media_type.bits_per_sample ||
        media_type->sample_format   != fader->input.media_type.sample_format) {
        /* media type has changed */
        ATX_LOG_FINER("CrossFaderInputPort::PutPacket - new media type");
        CrossFader_Flush(fader);
        result = CrossFader_SetupInput(fader, media_type);
        if (BLT_FAILED(result)) return result;
    }

    /* decide what to do with the packet */
    switch (fader->state) {
      case CROSS_FADER_STATE_IN_START:
        {
            unsigned int sample;
            BLT_Size size = BLT_MediaPacket_GetPayloadSize(packet);
            short* samples = (short*)BLT_MediaPacket_GetPayloadBuffer(packet);
            float pos = (float)fader->input.position/(float)(44100*4*10);
            float factor = (float)pow(10.0f, -(30.0f-pos*30.0f)/20.0f);
            ATX_LOG_FINDER_1("CrossFaderInputPort::PutPacket - factor = %f", factor);
            for (sample = 0; sample < size/2; sample++) {
                *samples = (short)(((float)*samples)*factor);
                samples++;
            }
            fader->input.position += size;
            if (fader->input.position >= 44100*4*10) {
                fader->input.position = 0;
                fader->state = CROSS_FADER_STATE_IN_MAIN;
            }
        }
        CrossFader_BufferPacket(fader, packet);
        break;

      case CROSS_FADER_STATE_IN_MAIN:
        CrossFader_BufferPacket(fader, packet);
        break;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CrossFaderInputPort_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFaderInputPort_QueryMediaType(BLT_MediaPortInstance* instance,
                                   BLT_Ordinal            index,
                                   const BLT_MediaType**  media_type)
{
    BLT_COMPILER_UNUSED(instance);
    if (index == 0) {
        *media_type = &BLT_GenericPcmMediaType;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(CrossFaderInputPort,
                                         "input",
                                         PACKET,
                                         IN)
static const BLT_MediaPortInterface
CrossFaderInputPort_BLT_MediaPortInterface = {
    CrossFaderInputPort_GetInterface,
    CrossFaderInputPort_GetName,
    CrossFaderInputPort_GetProtocol,
    CrossFaderInputPort_GetDirection,
    CrossFaderInputPort_QueryMediaType
};

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
static const BLT_PacketConsumerInterface
CrossFaderInputPort_BLT_PacketConsumerInterface = {
    CrossFaderInputPort_GetInterface,
    CrossFaderInputPort_PutPacket
};

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderInputPort)
ATX_INTERFACE_MAP_ADD(CrossFaderInputPort, BLT_MediaPort)
ATX_INTERFACE_MAP_ADD(CrossFaderInputPort, BLT_PacketConsumer)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderInputPort)

/*----------------------------------------------------------------------
|    CrossFaderOutputPort_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFaderOutputPort_GetPacket(BLT_PacketProducerInstance* instance,
                               BLT_MediaPacket**           packet)
{
    CrossFader*   fader = (CrossFader*)instance;
    ATX_ListItem* item;

    item = ATX_List_GetFirstItem(fader->output.packets);
    if (item) {
        *packet = ATX_ListItem_GetData(item);
        ATX_List_RemoveItem(fader->output.packets, item);
        ATX_LOG_FINER("CrossFaderInputPort_GetPacket - got one");
        return BLT_SUCCESS;
    } else {
        *packet = NULL;
        ATX_LOG_FINER("CrossFaderInputPort_GetPacket - no more data");
        return BLT_ERROR_PORT_HAS_NO_DATA;
    }
}

/*----------------------------------------------------------------------
|   CrossFaderOutputPort_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFaderOutputPort_QueryMediaType(BLT_MediaPortInstance* instance,
                                    BLT_Ordinal            index,
                                    const BLT_MediaType**  media_type)
{
    BLT_COMPILER_UNUSED(instance);
    if (index == 0) {
        *media_type = &BLT_GenericPcmMediaType;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(CrossFaderOutputPort,
                                         "output",
                                         PACKET,
                                         OUT)
static const BLT_MediaPortInterface
CrossFaderOutputPort_BLT_MediaPortInterface = {
    CrossFaderOutputPort_GetInterface,
    CrossFaderOutputPort_GetName,
    CrossFaderOutputPort_GetProtocol,
    CrossFaderOutputPort_GetDirection,
    CrossFaderOutputPort_QueryMediaType
};

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
static const BLT_PacketProducerInterface
CrossFaderOutputPort_BLT_PacketProducerInterface = {
    CrossFaderOutputPort_GetInterface,
    CrossFaderOutputPort_GetPacket
};

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderOutputPort)
ATX_INTERFACE_MAP_ADD(CrossFaderOutputPort, BLT_MediaPort)
ATX_INTERFACE_MAP_ADD(CrossFaderOutputPort, BLT_PacketProducer)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderOutputPort)

/*----------------------------------------------------------------------
|    CrossFader_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_SetupPorts(CrossFader* fader)
{
    ATX_Result result;

    /* initialize the input */
    fader->input.buffer = NULL;
    BLT_PcmMediaType_Init(&fader->input.media_type);

    /* create a list of output packets */
    result = ATX_List_Create(&fader->output.packets);
    if (ATX_FAILED(result)) return result;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_Create
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  ATX_Object*              object)
{
    CrossFader* fader;
    BLT_Result  result;

    ATX_LOG_FINE("CrossFader::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    fader = ATX_AllocateZeroMemory(sizeof(CrossFader));
    if (fader == NULL) {
        ATX_CLEAR_OBJECT(object);
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&fader->base, module, core);

    /* construct the object */
    fader->state = CROSS_FADER_STATE_IN_START;

    /* setup the input and output ports */
    result = CrossFader_SetupPorts(fader);
    if (BLT_FAILED(result)) {
        BLT_BaseMediaNode_Destruct(&fader->base);
        ATX_FreeMemory(fader);
        ATX_CLEAR_OBJECT(object);
        return result;
    }

    /* construct reference */
    ATX_INSTANCE(object)  = (ATX_Instance*)fader;
    ATX_INTERFACE(object) = (ATX_Interface*)&CrossFader_BLT_MediaNodeInterface;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
CrossFader_Destroy(CrossFader* fader)
{ 
    ATX_ListItem* item;

    ATX_LOG_FINE("CrossFader::Destroy");

    /* release any packet we may hold */
    item = ATX_List_GetFirstItem(fader->output.packets);
    while (item) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) {
            BLT_MediaPacket_Release(packet);
        }
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Destroy(fader->output.packets);
    
    /* destroy the input buffer */
    if (fader->input.buffer) {
        ATX_RingBuffer_Destroy(fader->input.buffer);
    }

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&fader->base);

    /* free the object memory */
    ATX_FreeMemory(fader);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|    CrossFader_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFader_Activate(BLT_MediaNodeInstance* instance, BLT_Stream* stream)
{
    ATX_Properties settings;
    CrossFader*    fader = (CrossFader*)instance;

    /* stop listening for settings on the current stream, if any */
    if (!ATX_OBJECT_IS_NULL(&fader->context) &&
        fader->listener_handle != NULL) {
        if (BLT_SUCCEEDED(BLT_Core_GetSettings(&fader->base.core, 
                                               &settings))) {
            ATX_Properties_RemoveListener(&settings, fader->listener_handle);
        }
        fader->listener_handle = NULL;
    }

    /* keep a reference to the stream */
    fader->context = *stream;

    /* listen to settings on the new stream */
    if (BLT_SUCCEEDED(BLT_Core_GetSettings(&fader->base.core, 
                                            &settings))) {
        ATX_PropertyListener me;
        ATX_INSTANCE(&me)  = (ATX_PropertyListenerInstance*)fader;
        ATX_INTERFACE(&me) = &CrossFader_ATX_PropertyListenerInterface;
        ATX_Properties_AddListener(&settings, 
                                    "SomeSetting",
                                    &me,
                                    &fader->listener_handle);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFader_Deactivate(BLT_MediaNodeInstance* instance)
{
    ATX_Properties settings;
    CrossFader*    fader = (CrossFader*)instance;

    /* remove our listener */
    if (BLT_SUCCEEDED(BLT_Core_GetSettings(&fader->base.core, 
                                            &settings))) {
        ATX_Properties_RemoveListener(&settings, 
                                        &fader->listener_handle);
    }

    /* we're detached from the stream */
    ATX_CLEAR_OBJECT(&fader->context);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   CrossFader_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFader_GetPortByName(BLT_MediaNodeInstance* instance,
                         BLT_CString            name,
                         BLT_MediaPort*         port)
{
    CrossFader* fader = (CrossFader*)instance;

    if (ATX_StringsEqual(name, "input")) {
        ATX_INSTANCE(port)  = (BLT_MediaPortInstance*)fader;
        ATX_INTERFACE(port) = &CrossFaderInputPort_BLT_MediaPortInterface; 
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "output")) {
        ATX_INSTANCE(port)  = (BLT_MediaPortInstance*)fader;
        ATX_INTERFACE(port) = &CrossFaderOutputPort_BLT_MediaPortInterface; 
        return BLT_SUCCESS;
    } else {
        ATX_CLEAR_OBJECT(port);
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    CrossFader_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFader_Seek(BLT_MediaNodeInstance* instance,
                BLT_SeekMode*          mode,
                BLT_SeekPoint*         point)
{
    BLT_COMPILER_UNUSED(instance);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);
    /*CrossFader* fader = (CrossFader*)instance;*/
    
    /* flush pending input packets */
    /*CrossFaderInputPort_Flush(fader);*/

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    CrossFader_OnPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
CrossFader_OnPropertyChanged(ATX_PropertyListenerInstance* instance,
                             ATX_CString                   name,
                             ATX_PropertyType              type,
                             const ATX_PropertyValue*      value)
{
    /*CrossFader* fader = (CrossFader*)instance;*/
    BLT_COMPILER_UNUSED(instance);
    BLT_COMPILER_UNUSED(type);
    ATX_LOG_FINE_2("CrossFader::OnPropertyChanged - name=%s val=%d", 
                   name ? name : "*", value ? value->integer : 0);
}

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
static const BLT_MediaNodeInterface
CrossFader_BLT_MediaNodeInterface = {
    CrossFader_GetInterface,
    BLT_BaseMediaNode_GetInfo,
    CrossFader_GetPortByName,
    CrossFader_Activate,
    CrossFader_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    CrossFader_Seek
};

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
static const ATX_PropertyListenerInterface
CrossFader_ATX_PropertyListenerInterface = {
    CrossFader_GetInterface,
    CrossFader_OnPropertyChanged,
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_SIMPLE_REFERENCEABLE_INTERFACE(CrossFader, base.reference_count)

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFader)
ATX_INTERFACE_MAP_ADD(CrossFader, BLT_MediaNode)
ATX_INTERFACE_MAP_ADD(CrossFader, ATX_PropertyListener)
ATX_INTERFACE_MAP_ADD(CrossFader, ATX_Referenceable)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFader)

/*----------------------------------------------------------------------
|   CrossFaderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
CrossFaderModule_Probe(BLT_ModuleInstance*      instance, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(core);
    BLT_COMPILER_UNUSED(instance);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need a name */
            if (constructor->name == NULL ||
                !ATX_StringsEqual(constructor->name, "CrossFader")) {
                return BLT_FAILURE;
            }

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

            /* the input type should be unspecified, or audio/pcm */
            if (!(constructor->spec.input.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.input.media_type->id ==
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or audio/pcm */
            if (!(constructor->spec.output.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.output.media_type->id ==
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* match level is always exact */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("CrossFaderModule::Probe - Ok [%d]", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   template instantiations
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(CrossFader)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
static const BLT_ModuleInterface CrossFaderModule_BLT_ModuleInterface = {
    CrossFaderModule_GetInterface,
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    CrossFaderModule_CreateInstance,
    CrossFaderModule_Probe
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define CrossFaderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_SIMPLE_REFERENCEABLE_INTERFACE(CrossFaderModule, 
                                             base.reference_count)

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderModule)
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderModule) 
ATX_INTERFACE_MAP_ADD(CrossFaderModule, BLT_Module)
ATX_INTERFACE_MAP_ADD(CrossFaderModule, ATX_Referenceable)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(CrossFaderModule)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_CrossFaderModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("CrossFader", NULL, 0,
                                 &CrossFaderModule_BLT_ModuleInterface,
                                 &CrossFaderModule_ATX_ReferenceableInterface,
                                 object);
}
