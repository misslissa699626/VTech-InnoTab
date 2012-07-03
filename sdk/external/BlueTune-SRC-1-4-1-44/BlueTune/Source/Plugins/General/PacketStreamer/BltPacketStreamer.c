/*****************************************************************
|
|   Packet Streamer Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltPacketStreamer.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPacketConsumer.h"
#include "BltByteStreamUser.h"
#include "BltPcm.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.general.packet-streamer")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} PacketStreamerModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    ATX_List* packets;
} PacketStreamerInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_OutputStreamUser);

    /* members */
    ATX_OutputStream* stream;
} PacketStreamerOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    PacketStreamerInput  input;
    PacketStreamerOutput output;
    BLT_MediaType*       media_type;
} PacketStreamer;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(PacketStreamerModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(PacketStreamer, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(PacketStreamer, ATX_Referenceable)

/*----------------------------------------------------------------------
|    PacketStreamerInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamerInput_PutPacket(BLT_PacketConsumer* _self,
                              BLT_MediaPacket*    packet)
{
    PacketStreamer*      self = ATX_SELF_M(input, PacketStreamer, BLT_PacketConsumer);
    const BLT_MediaType* media_type;
    BLT_Size             size;
    BLT_Any              payload;

    /* check the media type */
    BLT_MediaPacket_GetMediaType(packet, &media_type);
    if (self->media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN) {
        BLT_MediaType_Free(self->media_type);
        BLT_MediaType_Clone(media_type, &self->media_type);
    } else {
        if (self->media_type->id != media_type->id) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        } else {
            if (self->media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM &&
                self->media_type->extension_size == media_type->extension_size &&
                self->media_type->extension_size == sizeof(BLT_PcmMediaType)-sizeof(BLT_MediaType)) {
                const BLT_PcmMediaType* pcm_in  = (const BLT_PcmMediaType*)media_type;
                const BLT_PcmMediaType* pcm_out = (const BLT_PcmMediaType*)self->media_type;
                if (pcm_in->channel_count   != pcm_out->channel_count ||
                    pcm_in->sample_format   != pcm_out->sample_format ||
                    pcm_in->bits_per_sample != pcm_out->bits_per_sample) {
                    return BLT_ERROR_INVALID_MEDIA_TYPE;
                }
            } 
        }
    }

    /* just buffer the packets if we have no stream yet */
    if (self->output.stream == NULL) {
        /* add the packet to the input list */
        BLT_Result result = ATX_List_AddData(self->input.packets, packet);
        if (ATX_SUCCEEDED(result)) {
            BLT_MediaPacket_AddReference(packet);
        }
        ATX_LOG_FINER("PacketStreamerInputPort_PutPacket - buffer");
        return BLT_SUCCESS;
    }

    /* flush any pending packets */
    {
        ATX_ListItem* item;
        while ((item = ATX_List_GetFirstItem(self->input.packets))) {
            BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
            if (packet) {
                size    = BLT_MediaPacket_GetPayloadSize(packet);
                payload = BLT_MediaPacket_GetPayloadBuffer(packet);
                if (size != 0 && payload != NULL) {
                    ATX_OutputStream_Write(self->output.stream, 
                                           payload, 
                                           size, 
                                           NULL);
                }
                BLT_MediaPacket_Release(packet);
            }
            ATX_List_RemoveItem(self->input.packets, item);
        }
    }

    /* get payload and size of this packet */
    size    = BLT_MediaPacket_GetPayloadSize(packet);
    payload = BLT_MediaPacket_GetPayloadBuffer(packet);
    if (size == 0 || payload == NULL) return BLT_SUCCESS;

    /* write packet to the output stream */
    return ATX_OutputStream_Write(self->output.stream, payload, size, NULL);
}

/*----------------------------------------------------------------------
|    PacketStreamerInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamerInput_QueryMediaType(BLT_MediaPort*        _self,
                                   BLT_Ordinal           index,
                                   const BLT_MediaType** media_type)
{
    PacketStreamer* self = ATX_SELF_M(input, PacketStreamer, BLT_MediaPort);
    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PacketStreamerInput)
    ATX_GET_INTERFACE_ACCEPT(PacketStreamerInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(PacketStreamerInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(PacketStreamerInput,
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(PacketStreamerInput, BLT_MediaPort)
    PacketStreamerInput_GetName,
    PacketStreamerInput_GetProtocol,
    PacketStreamerInput_GetDirection,
    PacketStreamerInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(PacketStreamerInput, BLT_PacketConsumer)
    PacketStreamerInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    PacketStreamerOutput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamerOutput_SetStream(BLT_OutputStreamUser* _self,
                               ATX_OutputStream*     stream)
{
    PacketStreamer* self = ATX_SELF_M(output, PacketStreamer, BLT_OutputStreamUser);

    /* keep a reference to the stream */
    self->output.stream = stream;
    ATX_REFERENCE_OBJECT(stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    PacketStreamerOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamerOutput_QueryMediaType(BLT_MediaPort*        _self,
                                    BLT_Ordinal           index,
                                    const BLT_MediaType** media_type)
{
    PacketStreamer* self = ATX_SELF_M(output, PacketStreamer, BLT_MediaPort);
    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PacketStreamerOutput)
    ATX_GET_INTERFACE_ACCEPT(PacketStreamerOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(PacketStreamerOutput, BLT_OutputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(PacketStreamerOutput,
                                         "output",
                                         STREAM_PUSH,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(PacketStreamerOutput, BLT_MediaPort)
    PacketStreamerOutput_GetName,
    PacketStreamerOutput_GetProtocol,
    PacketStreamerOutput_GetDirection,
    PacketStreamerOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_OutputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(PacketStreamerOutput, BLT_OutputStreamUser)
    PacketStreamerOutput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    PacketStreamer_Create
+---------------------------------------------------------------------*/
static BLT_Result
PacketStreamer_Create(BLT_Module*              module,
                      BLT_Core*                core, 
                      BLT_ModuleParametersType parameters_type,
                      BLT_CString              parameters, 
                      BLT_MediaNode**          object)
{
    PacketStreamer*           self;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;

    ATX_LOG_FINE("PacketStreamer::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(PacketStreamer));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* create a list of input packets */
    result = ATX_List_Create(&self->input.packets);
    if (ATX_FAILED(result)) return result;

    /* keep the media type info */
    BLT_MediaType_Clone(constructor->spec.output.media_type, &self->media_type);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, PacketStreamer, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, PacketStreamer, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  PacketStreamerInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  PacketStreamerInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, PacketStreamerOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, PacketStreamerOutput, BLT_OutputStreamUser);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);


    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    PacketStreamer_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
PacketStreamer_Destroy(PacketStreamer* self)
{
    ATX_ListItem* item;

    ATX_LOG_FINE("PacketStreamer::Destroy");

    /* release the stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    /* destroy the input packet list */
    item = ATX_List_GetFirstItem(self->input.packets);
    while (item) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) {
            BLT_MediaPacket_Release(packet);
        }
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Destroy(self->input.packets);

    /* free the media type extensions */
    BLT_MediaType_Free(self->media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    PacketStreamer_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamer_Deactivate(BLT_MediaNode* _self)
{
    PacketStreamer* self = ATX_SELF_EX(PacketStreamer, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("PacketStreamer::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the output stream */
    ATX_RELEASE_OBJECT(self->output.stream);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   PacketStreamer_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamer_GetPortByName(BLT_MediaNode*  _self,
                             BLT_CString     name,
                             BLT_MediaPort** port)
{
    PacketStreamer* self = ATX_SELF_EX(PacketStreamer, BLT_BaseMediaNode, BLT_MediaNode);

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
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PacketStreamer)
ATX_GET_INTERFACE_ACCEPT_EX(PacketStreamer, BLT_BaseMediaNode, BLT_MediaNode)
ATX_GET_INTERFACE_ACCEPT_EX(PacketStreamer, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(PacketStreamer, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    PacketStreamer_GetPortByName,
    BLT_BaseMediaNode_Activate,
    PacketStreamer_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(PacketStreamer, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   PacketStreamerModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
PacketStreamerModule_Probe(BLT_Module*              self, 
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

            *match = BLT_MODULE_PROBE_MATCH_DEFAULT;

            /* media types must match */
            if (constructor->spec.input.media_type->id  != BLT_MEDIA_TYPE_ID_UNKNOWN &&
                constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN &&
                constructor->spec.input.media_type->id  != constructor->spec.output.media_type->id) {
                return BLT_FAILURE;
            }

            /* compute match based on specified name */
            if (constructor->name == NULL) {
                /* the input protocol must be PACKET */
                if (constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET) {
                    return BLT_FAILURE;
                }

                /* the output protocol must be STREAM_PUSH */
                if (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH) {
                    return BLT_FAILURE;
                }

                *match = BLT_MODULE_PROBE_MATCH_DEFAULT+10;
            } else {
                /* if a name is a specified, it needs to match exactly */
                if (!ATX_StringsEqual(constructor->name, "PacketStreamer")) {
                    return BLT_FAILURE;
                } else {
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                }

                /* the input protocol must be PACKET or ANY */
                if (constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                    constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET) {
                    return BLT_FAILURE;
                }

                /* the output protocol must be STREAM_PUSH or ANY */
                if (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                    constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH) {
                    return BLT_FAILURE;
                }
            }

            ATX_LOG_FINE_1("PacketStreamerModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(PacketStreamerModule)
ATX_GET_INTERFACE_ACCEPT_EX(PacketStreamerModule, BLT_BaseModule, BLT_Module)
ATX_GET_INTERFACE_ACCEPT_EX(PacketStreamerModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(PacketStreamerModule, PacketStreamer)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(PacketStreamerModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    PacketStreamerModule_CreateInstance,
    PacketStreamerModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define PacketStreamerModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(PacketStreamerModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(PacketStreamerModule,
                                         "Packet Streamer",
                                         "com.axiosys.general.packet-streamer",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
