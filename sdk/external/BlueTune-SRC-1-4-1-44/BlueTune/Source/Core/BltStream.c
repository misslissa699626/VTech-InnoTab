/*****************************************************************
|
|   BlueTune - Stream Objects
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune Stream Objects
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltCore.h"
#include "BltStream.h"
#include "BltStreamPriv.h"
#include "BltMediaNode.h"
#include "BltByteStreamProvider.h"
#include "BltByteStreamUser.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltBuiltins.h"
#include "BltEvent.h"
#include "BltEventListener.h"
#include "BltOutputNode.h"
#include "BltPcm.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.core.stream")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct StreamNode {
    BLT_Stream*        stream;
    BLT_MediaNode*     media_node;
    BLT_MediaNodeState state;
    BLT_Flags          flags;
    struct {
        BLT_MediaPortProtocol protocol;
        BLT_MediaPort*        port;
        BLT_Boolean           connected;
        union {
            ATX_Object*               any;
            BLT_InputStreamUser*      stream_user;
            BLT_OutputStreamProvider* stream_provider;
            BLT_PacketConsumer*       packet_consumer;
        }                     iface;
    }                  input;
    struct {
        BLT_MediaPortProtocol protocol;
        BLT_MediaPort*        port;
        BLT_Boolean           connected;
        union {
            ATX_Object*              any;
            BLT_OutputStreamUser*    stream_user;
            BLT_InputStreamProvider* stream_provider;
            BLT_PacketProducer*      packet_producer;
        }                     iface;
    }                  output;
    struct StreamNode* next;
    struct StreamNode* prev;
} StreamNode;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_Stream);
    ATX_IMPLEMENTS(BLT_EventListener);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    BLT_Cardinal reference_count;
    BLT_Core*    core;
    struct {
        StreamNode* head;
        StreamNode* tail;
    }            nodes;
    struct {
        BLT_CString name;
        StreamNode* node;
    }            input;
    struct {
        BLT_CString     name;
        StreamNode*     node;
        BLT_OutputNode* output_node;
        BLT_TimeStamp   last_time_stamp;
        BLT_TimeStamp   next_time_stamp;
    }                  output;
    ATX_Properties*    properties;
    BLT_StreamInfo     info;
    BLT_EventListener* event_listener;
} Stream;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(Stream, BLT_Stream)
ATX_DECLARE_INTERFACE_MAP(Stream, BLT_EventListener)
ATX_DECLARE_INTERFACE_MAP(Stream, ATX_Referenceable)
static BLT_Result Stream_ResetInfo(Stream* self);
static BLT_Result Stream_RemoveNode(Stream* self, StreamNode* stream_node);
static BLT_Result StreamNode_Activate(StreamNode* self);
static BLT_Result StreamNode_Deactivate(StreamNode* self);
static BLT_Result StreamNode_Start(StreamNode* self);
static BLT_Result StreamNode_Stop(StreamNode* self);

/*----------------------------------------------------------------------
|    StreamNode_Create
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Create(BLT_Stream*    stream,
                  BLT_MediaNode* media_node, 
                  BLT_CString    input_port,
                  BLT_CString    output_port,
                  StreamNode**   stream_node)
{
    StreamNode* node;
    BLT_Result  result;
    
    /* set the return value to NULL in case we exit early */
    *stream_node = NULL;

    /* activate the node */
    result = BLT_MediaNode_Activate(media_node, stream);
    if (BLT_FAILED(result)) {
        return result;
    }

    /* allocate memory for the node */
    node = (StreamNode*)ATX_AllocateZeroMemory(sizeof(StreamNode));
    if (node == NULL) return BLT_ERROR_OUT_OF_MEMORY;

    /* construct the node */
    node->stream     = stream;
    node->media_node = media_node;
    node->state      = BLT_MEDIA_NODE_STATE_IDLE;
    node->next       = NULL;
    node->prev       = NULL;
    node->flags      = 0;

    /* get the input port of the node */
    node->input.port = NULL;
    node->input.connected = BLT_FALSE;
    node->input.protocol  = BLT_MEDIA_PORT_PROTOCOL_NONE;
    result = BLT_MediaNode_GetPortByName(media_node, 
                                         input_port?input_port:"input",
                                         &node->input.port);
    if (BLT_SUCCEEDED(result)) {
        /* this is a valid input */
        BLT_MediaPort_GetProtocol(node->input.port, &node->input.protocol);
        
        switch (node->input.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            node->input.iface.packet_consumer = 
                ATX_CAST(node->input.port, BLT_PacketConsumer);
            break;

          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            node->input.iface.stream_provider = 
                ATX_CAST(node->input.port, BLT_OutputStreamProvider);
            break;

          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
            node->input.iface.stream_user = 
                ATX_CAST(node->input.port, BLT_InputStreamUser);
            break;

          default:
            ATX_ASSERT(0);
            ATX_FreeMemory((void*)node);
            return BLT_ERROR_INVALID_PARAMETERS;
            break;
        }
        if (node->input.iface.any == NULL) {
            ATX_FreeMemory((void*)node);
            return result;
        }
    }

    /* get the output port of the node */
    node->output.port = NULL;
    node->output.connected = BLT_FALSE;
    node->output.protocol  = BLT_MEDIA_PORT_PROTOCOL_NONE;
    result = BLT_MediaNode_GetPortByName(media_node, 
                                         output_port?output_port:"output",
                                         &node->output.port);
    if (BLT_SUCCEEDED(result)) {
        /* this is a valid output */
        BLT_MediaPort_GetProtocol(node->output.port, &node->output.protocol);
        
        switch (node->output.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            node->output.iface.packet_producer = 
                ATX_CAST(node->output.port, BLT_PacketProducer);
            break;

          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            node->output.iface.stream_user = 
                ATX_CAST(node->output.port, BLT_OutputStreamUser);
            break;

          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
            node->output.iface.stream_provider = 
                ATX_CAST(node->output.port, BLT_InputStreamProvider);
            break;

          default:
            ATX_ASSERT(0);
            ATX_FreeMemory((void*)node);
            return BLT_ERROR_INVALID_PARAMETERS;
            break;
        }
        if (node->output.iface.any == NULL) {
            ATX_FreeMemory((void*)node);
            return result;
        }
    }

    /* keep a reference to the media node */
    ATX_REFERENCE_OBJECT(media_node);

    /* return the node */
    *stream_node = node;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamNode_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Destroy(StreamNode* self)
{
    /* check parameters */
    if (self == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* deactivate the node */
    StreamNode_Deactivate(self);

    /* release the reference to the media node */
    ATX_RELEASE_OBJECT(self->media_node);

    /* free the memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

#if defined(ATX_CONFIG_ENABLE_LOGGING)
/*----------------------------------------------------------------------
|    Stream_GetProtocolName
+---------------------------------------------------------------------*/
static const char*
Stream_GetProtocolName(BLT_MediaPortProtocol protocol)
{
    switch (protocol) {
      case BLT_MEDIA_PORT_PROTOCOL_NONE:
        return "NONE";
      case BLT_MEDIA_PORT_PROTOCOL_ANY:
        return "ANY";
      case BLT_MEDIA_PORT_PROTOCOL_PACKET:
        return "PACKET";
      case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
        return "STREAM_PULL";
      case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
        return "STREAM_PUSH";
      default:
        return "UNKNOWN";
    }
}

/*----------------------------------------------------------------------
|    Stream_GetTypeName
+---------------------------------------------------------------------*/
static const char*
Stream_GetTypeName(Stream* self, const BLT_MediaType* type)
{
    switch (type->id) {
      case BLT_MEDIA_TYPE_ID_NONE:
        return "none";
      case BLT_MEDIA_TYPE_ID_UNKNOWN:
        return "unknown";
      case BLT_MEDIA_TYPE_ID_AUDIO_PCM:
        return "audio/pcm";
      default: {
          BLT_Result    result;
          BLT_Registry* registry;
          BLT_CString   name;
          result = BLT_Core_GetRegistry(self->core, &registry);
          if (BLT_FAILED(result)) return "no-reg";
          result = BLT_Registry_GetNameForId(
              registry, 
              BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
              type->id,
              &name);
          if (BLT_FAILED(result)) return "not-found";
          return name;
      }
    }
}
#endif /* defined(ATX_CONFIG_ENABLE_LOGGING) */

/*----------------------------------------------------------------------
|    Stream_TopologyChanged
+---------------------------------------------------------------------*/
static void
Stream_TopologyChanged(Stream*                     self, 
                       BLT_StreamTopologyEventType type,
                       StreamNode*                 node)
{
    BLT_StreamTopologyEvent event;

    /* quick check */
    if (self->event_listener == NULL) return;

    /* send an event to the listener */
    event.type = type;
    event.node = node->media_node;
    BLT_EventListener_OnEvent(self->event_listener,
                              (ATX_Object*)self, 
                              BLT_EVENT_TYPE_STREAM_TOPOLOGY,
                              (const BLT_Event*)(const void*)&event);
}

/*----------------------------------------------------------------------
|    StreamNode_Activate
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Activate(StreamNode* self)
{
    BLT_Result result;

    switch (self->state) {
      case BLT_MEDIA_NODE_STATE_RESET:
        /* activate the node */
        result = BLT_MediaNode_Activate(self->media_node, self->stream);
        if (BLT_FAILED(result)) return result;
        break;

      case BLT_MEDIA_NODE_STATE_IDLE:
      case BLT_MEDIA_NODE_STATE_RUNNING:
      case BLT_MEDIA_NODE_STATE_PAUSED:
        /* already activated */
        break;
    }

    /* the node is now activated */
    self->state = BLT_MEDIA_NODE_STATE_IDLE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamNode_Deactivate
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Deactivate(StreamNode* self)
{
    BLT_Result result;

    switch (self->state) {
      case BLT_MEDIA_NODE_STATE_RESET:
        /* already deactivated */
        break;

      case BLT_MEDIA_NODE_STATE_RUNNING:
      case BLT_MEDIA_NODE_STATE_PAUSED:
        /* first, stop the node */
        result = StreamNode_Stop(self);
        if (BLT_FAILED(result)) return result;

        /* then deactivate the node (FALLTHROUGH) */

      case BLT_MEDIA_NODE_STATE_IDLE:
        /* deactivate the node */
        result = BLT_MediaNode_Deactivate(self->media_node);
        if (BLT_FAILED(result)) return result;
        break;
    }

    /* the node is now deactivated */
    self->state = BLT_MEDIA_NODE_STATE_RESET;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamNode_Start
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Start(StreamNode* self)
{
    BLT_Result result;

    switch (self->state) {
      case BLT_MEDIA_NODE_STATE_RESET:
        /* first, activate the node */
        result = StreamNode_Activate(self);
        if (BLT_FAILED(result)) return result;

        /* then start the node (FALLTHROUGH) */

      case BLT_MEDIA_NODE_STATE_IDLE:
        /* start the node */
        result = BLT_MediaNode_Start(self->media_node);
        if (BLT_FAILED(result)) return result;
        break;

      case BLT_MEDIA_NODE_STATE_RUNNING:
        /* the node is already running, do nothing */
        return BLT_SUCCESS;

      case BLT_MEDIA_NODE_STATE_PAUSED:
        /* resume the node */
        result = BLT_MediaNode_Resume(self->media_node);
        if (BLT_FAILED(result)) return result;
        break;
    }

    /* the node is now running */
    self->state = BLT_MEDIA_NODE_STATE_RUNNING;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamNode_Stop
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Stop(StreamNode* self)
{
    BLT_Result result;

    switch (self->state) {
      case BLT_MEDIA_NODE_STATE_RESET:
      case BLT_MEDIA_NODE_STATE_IDLE:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_MEDIA_NODE_STATE_RUNNING:
      case BLT_MEDIA_NODE_STATE_PAUSED:
        /* stop the node */
        result = BLT_MediaNode_Stop(self->media_node);
        if (BLT_FAILED(result)) return result;
        break;
    }

    /* the node is now idle */
    self->state = BLT_MEDIA_NODE_STATE_IDLE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamNode_Pause
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Pause(StreamNode* self)
{
    BLT_Result result;

    switch (self->state) {
      case BLT_MEDIA_NODE_STATE_RESET:
      case BLT_MEDIA_NODE_STATE_IDLE:
        /* ignore */
        return BLT_SUCCESS;

      case BLT_MEDIA_NODE_STATE_RUNNING:
        /* pause the node */
        result = BLT_MediaNode_Pause(self->media_node);
        if (BLT_FAILED(result)) return result;
        break;

      case BLT_MEDIA_NODE_STATE_PAUSED:
        /* the node is already paused, do nothing */
        break;
    }

    /* the node is now paused */
    self->state = BLT_MEDIA_NODE_STATE_PAUSED;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    StreamNode_Seek
+---------------------------------------------------------------------*/
static BLT_Result
StreamNode_Seek(StreamNode*    self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    BLT_Result result;

    switch (self->state) {
      case BLT_MEDIA_NODE_STATE_RESET:
        /* first, activate the node */
        result = StreamNode_Activate(self);
        if (BLT_FAILED(result)) return result;

        /* then seek (FALLTHROUGH) */

      case BLT_MEDIA_NODE_STATE_IDLE:
      case BLT_MEDIA_NODE_STATE_RUNNING:
      case BLT_MEDIA_NODE_STATE_PAUSED:
        /* seek */
        return BLT_MediaNode_Seek(self->media_node, mode, point);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_Create
+---------------------------------------------------------------------*/
BLT_Result
Stream_Create(BLT_Core* core, BLT_Stream** object)
{
    Stream* stream;

    ATX_LOG_FINE("Stream::Create");
 
    /* allocate memory for the object */
    stream = ATX_AllocateZeroMemory(sizeof(Stream));
    if (stream == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    stream->reference_count = 1;
    stream->core            = core;
    ATX_Properties_Create(&stream->properties);

    /* setup interfaces */
    ATX_SET_INTERFACE(stream, Stream, BLT_Stream);
    ATX_SET_INTERFACE(stream, Stream, BLT_EventListener);
    ATX_SET_INTERFACE(stream, Stream, ATX_Referenceable);
    *object = &ATX_BASE(stream, BLT_Stream);    

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Stream_Destroy(Stream* self)
{
    ATX_LOG_FINE("Stream::Destroy");
    
    /* deactivate the nodes */
    {
        StreamNode* node = self->nodes.head;
        while (node) {
            StreamNode* next = node->next;
            StreamNode_Deactivate(node);
            node = next;
        }
    }

    /* remove and destroy the nodes */
    {
        StreamNode* node = self->nodes.head;
        while (node) {
            StreamNode* next = node->next;
            Stream_RemoveNode(self, node);
            node = next;
        }
    }

    /* free input name */
    if (self->input.name) {
        ATX_FreeMemory((void*)self->input.name);
    }

    /* free output name */
    if (self->output.name) {
        ATX_FreeMemory((void*)self->output.name);
    }

    /* free the stream info data */
    Stream_ResetInfo(self);

    /* destroy the properties object */
    ATX_DESTROY_OBJECT(self->properties);

    /* free the object memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_InsertChain
+---------------------------------------------------------------------*/
static BLT_Result 
Stream_InsertChain(Stream*     self, 
                   StreamNode* where, 
                   StreamNode* chain)
{
    StreamNode* tail = chain;

    /* find the tail of the chain */
    while (tail->next) {
        tail = tail->next;
    }
    
    /* insert 'chain' after the 'where' node */
    if (where == NULL) {
        /* this node chain becomes the head */
        tail->next = self->nodes.head;
        if (tail->next) {
            tail->next->prev = tail;
        }
        self->nodes.head = chain;
    } else {
        chain->prev = where;
        tail->next  = where->next;
        if (tail->next) {
            tail->next->prev = chain;
        }
        where->next = chain;
    }
    if (where == self->nodes.tail) {
        /* this is the new tail */
        self->nodes.tail = tail;
    }

    /* notify that a node was added */
    Stream_TopologyChanged(self, 
                           BLT_STREAM_TOPOLOGY_NODE_ADDED,
                           chain);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_CleanupChain
+---------------------------------------------------------------------*/
static BLT_Result 
Stream_CleanupChain(Stream* self) 
{
    /* remove all transient nodes */
    StreamNode* node = self->nodes.head;
    while (node) {
        StreamNode* next = node->next;
        if (node->flags & BLT_STREAM_NODE_FLAG_TRANSIENT) {
            Stream_RemoveNode(self, node);
        }
        node = next;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_RemoveNode
+---------------------------------------------------------------------*/
static BLT_Result 
Stream_RemoveNode(Stream* self, StreamNode* node)
{
    /* quick check */
    if (self == NULL) {
        return BLT_SUCCESS;
    }

    /* relink the chain */
    if (node->prev) {
        node->prev->next = node->next;
        node->prev->output.connected = BLT_FALSE;
    } 
    if (node->next) {
        node->next->prev = node->prev;
        node->next->input.connected = BLT_FALSE;
    }
    if (self->nodes.head == node) {
        self->nodes.head =  node->next;
    }
    if (self->nodes.tail == node) {
        self->nodes.tail =  node->prev;
    }

    /* notify that the topology has changed */
    node->next = node->prev = NULL;
    Stream_TopologyChanged(self, 
                           BLT_STREAM_TOPOLOGY_NODE_REMOVED, 
                           node);

    /* destroy the removed node */
    StreamNode_Destroy(node);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_ResetInfo
+---------------------------------------------------------------------*/
static BLT_Result
Stream_ResetInfo(Stream* self)
{
    /* clear the stream info */
    if (self->info.data_type) {
        ATX_FreeMemory((void*)self->info.data_type);
	    self->info.data_type = NULL;
    }
    ATX_SetMemory(&self->info, 0, sizeof(self->info));

    /* reset the time stamp */
    BLT_TimeStamp_Set(self->output.last_time_stamp, 0, 0);
    BLT_TimeStamp_Set(self->output.next_time_stamp, 0, 0);

    return BLT_SUCCESS;
} 

/*----------------------------------------------------------------------
|    Stream_ResetProperties
+---------------------------------------------------------------------*/
static BLT_Result
Stream_ResetProperties(Stream* self)
{
    /* reset the properies object */
    return ATX_Properties_Clear(self->properties);
} 

/*----------------------------------------------------------------------
|    Stream_SetEventListener
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_SetEventListener(BLT_Stream*        _self, 
                        BLT_EventListener* listener)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* keep the object reference */
    self->event_listener = listener;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_ResetInputNode
+---------------------------------------------------------------------*/
static BLT_Result
Stream_ResetInputNode(Stream* self)
{
    /* reset the stream info */
    Stream_ResetInfo(self);

    /* reset the properties */
    Stream_ResetProperties(self);

    /* clear the name */
    if (self->input.name) {
        ATX_FreeMemory((void*)self->input.name);
        self->input.name = NULL;
    }

    /* remove the current input */
    if (self->input.node) {
        Stream_RemoveNode(self, self->input.node);
        self->input.node = NULL;

        /* cleanup the chain */
        Stream_CleanupChain(self);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_ResetInput
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_ResetInput(BLT_Stream* _self)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* reset the input node */
    Stream_ResetInputNode(self);

    /* notify of this new info */
    if (self->event_listener) {
        BLT_StreamInfoEvent event;

        event.update_mask = BLT_STREAM_INFO_MASK_ALL;
        event.info        = self->info;
        
        BLT_EventListener_OnEvent(self->event_listener, 
                                  (ATX_Object*)self, 
                                  BLT_EVENT_TYPE_STREAM_INFO,
                                  (const BLT_Event*)(const void*)&event);
    }   

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_SetInputNode
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_SetInputNode(BLT_Stream*    _self, 
                    BLT_CString    name,
                    BLT_CString    port,
                    BLT_MediaNode* node)

{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* stream_node;
    BLT_Result  result;

    /* check parameters */
    if (name == NULL || node == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* reset the current stream */
    Stream_ResetInputNode(self);

    /* create a stream node to represent the media node */
    result = StreamNode_Create(_self, node, NULL, port, &stream_node);
    if (BLT_FAILED(result)) return result;

    /* copy the name */
    self->input.name = ATX_DuplicateString(name);

    /* install the new input */
    self->input.node = stream_node;
    Stream_InsertChain(self, NULL, stream_node);
        
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_SetInput
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_SetInput(BLT_Stream* _self, 
                BLT_CString name, 
                BLT_CString type)
{
    Stream*                  self = ATX_SELF(Stream, BLT_Stream);
    BLT_MediaNode*           media_node;
    BLT_MediaType            input_media_type;
    BLT_MediaType            output_media_type;
    BLT_MediaNodeConstructor constructor;
    BLT_Result               result;

    /* check parameters */
    if (name == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* normalize type */
    if (type && type[0] == '\0') type = NULL;

    ATX_LOG_FINE_1("input name=%s", name);

    /* ask the core to create the corresponding input node */
    constructor.spec.input.protocol  = BLT_MEDIA_PORT_PROTOCOL_NONE;
    constructor.spec.output.protocol = BLT_MEDIA_PORT_PROTOCOL_ANY;
    constructor.name                 = name;
    BLT_MediaType_Init(&input_media_type,  BLT_MEDIA_TYPE_ID_NONE);
    BLT_MediaType_Init(&output_media_type, BLT_MEDIA_TYPE_ID_UNKNOWN);
    constructor.spec.output.media_type = &output_media_type;
    constructor.spec.input.media_type  = &input_media_type;
    if (type != NULL) {
        BLT_MediaType* media_type;
        result = BLT_Core_ParseMimeType(self->core, type, &media_type);
        constructor.spec.output.media_type = media_type;
        if (BLT_FAILED(result)) return result;
    }

    /* create the input media node */
    result = BLT_Core_CreateCompatibleMediaNode(self->core, 
                                                &constructor, 
                                                &media_node);
    if (constructor.spec.output.media_type != &output_media_type) {
        BLT_MediaType_Free((BLT_MediaType*)constructor.spec.output.media_type);
    }
    if (BLT_FAILED(result)) return result;

    /* set the media node as the new input */
    result = Stream_SetInputNode(_self, name, NULL, media_node);
    ATX_RELEASE_OBJECT(media_node);
    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetInputNode
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_GetInputNode(BLT_Stream* _self, BLT_MediaNode** node)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* check parameters */
    if (node == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* return the input media node */
    if (self->input.node != NULL) {
        *node = self->input.node->media_node;
        ATX_REFERENCE_OBJECT(*node);
    } else {
        *node = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_ResetOutput
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_ResetOutput(BLT_Stream* _self)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* clear the name */
    if (self->output.name) {
        ATX_FreeMemory((void*)self->output.name);
        self->output.name = NULL;
    }

    /* remove the previous output */
    if (self->output.node) {
        Stream_RemoveNode(self, self->output.node);
        self->output.node = NULL;

        /* cleanup the chain */
        Stream_CleanupChain(self);
    }

    /* reset the time stamp */
    BLT_TimeStamp_Set(self->output.last_time_stamp, 0, 0);
    BLT_TimeStamp_Set(self->output.next_time_stamp, 0, 0);

    return BLT_SUCCESS;
} 

/*----------------------------------------------------------------------
|    Stream_SetOutputNode
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_SetOutputNode(BLT_Stream*    _self, 
		     BLT_CString    name,
                     BLT_MediaNode* node)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* stream_node;
    BLT_Result  result;

    /* reset the current output */
    Stream_ResetOutput(_self);

    /* create a stream node to represent the media node */
    result = StreamNode_Create(_self, node, NULL, "", &stream_node);
    if (BLT_FAILED(result)) return result;

    /* get the BLT_OutputNode interface */
    self->output.output_node = ATX_CAST(stream_node->media_node, 
                                        BLT_OutputNode);

    /* copy the name */
    self->output.name = ATX_DuplicateString(name);

    /* install the new output */
    self->output.node = stream_node;
    Stream_InsertChain(self, self->nodes.tail, stream_node);
        
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_SetOutput
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_SetOutput(BLT_Stream* _self, 
                 BLT_CString name,
                 BLT_CString type)
{
    Stream*                  self = ATX_SELF(Stream, BLT_Stream);
    BLT_MediaNode*           media_node;
    BLT_MediaType            input_media_type;
    BLT_MediaType            output_media_type;
    BLT_MediaNodeConstructor constructor;
    BLT_Result               result;

    /* check parameters */
    if (name == NULL) return BLT_ERROR_INVALID_PARAMETERS;
    
    /* ask the core to create the corresponding output node */
    constructor.spec.input.protocol  = BLT_MEDIA_PORT_PROTOCOL_ANY;
    constructor.spec.output.protocol = BLT_MEDIA_PORT_PROTOCOL_NONE;
    constructor.name                 = name;
    BLT_MediaType_Init(&input_media_type,  BLT_MEDIA_TYPE_ID_UNKNOWN);
    BLT_MediaType_Init(&output_media_type, BLT_MEDIA_TYPE_ID_NONE);
    constructor.spec.output.media_type = &output_media_type;
    constructor.spec.input.media_type  = &input_media_type;
    if (type && type[0]) {
        BLT_MediaType* media_type;
        result = BLT_Core_ParseMimeType(self->core, type, &media_type);
        constructor.spec.input.media_type = media_type;
        if (BLT_FAILED(result)) return result;
    }
    result = BLT_Core_CreateCompatibleMediaNode(self->core, 
                                                &constructor, 
                                                &media_node);
    if (constructor.spec.input.media_type != &input_media_type) {
        BLT_MediaType_Free((BLT_MediaType*)constructor.spec.input.media_type);
    }
    if (BLT_FAILED(result)) return result;

    /* set the media node as the new output node */
    result = Stream_SetOutputNode(_self, name, media_node);
    ATX_RELEASE_OBJECT(media_node);
    if (BLT_FAILED(result)) return result;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetOutputNode
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_GetOutputNode(BLT_Stream* _self, BLT_MediaNode** node)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* check parameters */
    if (node == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* return the output media node */
    if (self->output.node != NULL) {
        *node = self->output.node->media_node;
        ATX_REFERENCE_OBJECT(*node);
    } else {
        *node = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_FindNode
+---------------------------------------------------------------------*/
static StreamNode*
Stream_FindNode(Stream* stream, const BLT_MediaNode* media_node)
{
    StreamNode* node = stream->nodes.head;
    while (node) {
        if (node->media_node == media_node) {
            return node;
        }
        node = node->next;
    }

    return NULL;
}

/*----------------------------------------------------------------------
|    Stream_AddNode
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_AddNode(BLT_Stream*    _self, 
               BLT_MediaNode* where,
               BLT_MediaNode* node)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* stream_node;
    StreamNode* insert_point;
    BLT_Result  result;

    /* check parameters */
    if (node == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* find a place to insert the node */
    if (where) {
        insert_point = Stream_FindNode(self, where);
    } else {
        /* a NULL insert point means just before the output */
        if (self->output.node) {
            insert_point = self->output.node->prev;
        } else {
            insert_point = NULL;
        }
    }

    /* create a stream node to represent the media node */
    result = StreamNode_Create(_self, node, NULL, NULL, &stream_node);
    if (BLT_FAILED(result)) return result;

    /* insert the node in the chain */
    return Stream_InsertChain(self, insert_point, stream_node);
}

/*----------------------------------------------------------------------
|    Stream_AddNodeByName
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_AddNodeByName(BLT_Stream*    _self, 
                     BLT_MediaNode* where,
                     BLT_CString    name)
{
    Stream*                  self = ATX_SELF(Stream, BLT_Stream);
    BLT_MediaNode*           media_node;
    BLT_MediaType            input_media_type;
    BLT_MediaType            output_media_type;
    BLT_MediaNodeConstructor constructor;
    BLT_Result               result;

    /* check parameters */
    if (name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* ask the core to create the media node */
    constructor.spec.input.protocol  = BLT_MEDIA_PORT_PROTOCOL_ANY;
    constructor.spec.output.protocol = BLT_MEDIA_PORT_PROTOCOL_ANY;
    constructor.name                 = name;
    BLT_MediaType_Init(&input_media_type,  BLT_MEDIA_TYPE_ID_UNKNOWN);
    BLT_MediaType_Init(&output_media_type, BLT_MEDIA_TYPE_ID_UNKNOWN);
    constructor.spec.output.media_type = &output_media_type;
    constructor.spec.input.media_type  = &input_media_type;
    result = BLT_Core_CreateCompatibleMediaNode(self->core, 
                                                &constructor, 
                                                &media_node);
    if (BLT_FAILED(result)) return result;

    /* add the node to the stream */
    result = Stream_AddNode(_self, where, media_node);
    ATX_RELEASE_OBJECT(media_node);
    if (BLT_FAILED(result)) {
        return result;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetStreamNodeInfo
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_GetStreamNodeInfo(BLT_Stream*          _self,
                         const BLT_MediaNode* node,
                         BLT_StreamNodeInfo*  info)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* stream_node;

    /* check parameters */
    if (node == NULL || info == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* find the node */
    stream_node = Stream_FindNode(self, node);
    if (stream_node == NULL) {
        return BLT_ERROR_NO_SUCH_MEDIA_NODE;
    }

    /* return the info */
    info->media_node       = stream_node->media_node;
    info->flags            = stream_node->flags;
    info->input.connected  = stream_node->input.connected;
    info->input.protocol   = stream_node->input.protocol;
    info->output.connected = stream_node->output.connected;
    info->output.protocol  = stream_node->output.protocol;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetFirstNode
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_GetFirstNode(BLT_Stream* _self, BLT_MediaNode** node)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* check parameters */
    if (node == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* return the first media node, if any */
    if (self->nodes.head) {
        *node = self->nodes.head->media_node;
    } else {
        *node = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetNextNode
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_GetNextNode(BLT_Stream*     _self,
                   BLT_MediaNode*  node,
                   BLT_MediaNode** next)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* stream_node;

    /* check parameters */
    if (node == NULL || next == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* find the node */
    stream_node = Stream_FindNode(self, node);
    if (stream_node == NULL) {
        *next = NULL;
        return BLT_ERROR_NO_SUCH_MEDIA_NODE;
    }

    /* return the next media node */
    if (stream_node->next) {
        *next = stream_node->next->media_node;
    } else {
        *next = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_ConnectNodes
+---------------------------------------------------------------------*/
static void
Stream_ConnectNodes(Stream* self, StreamNode* from, StreamNode* to)
{
    ATX_LOG_FINE("Stream::ConnectNodes - connected");
    from->output.connected = BLT_TRUE;
    to->input.connected    = BLT_TRUE;
    /* only notify of the connection of the 'from' node, as the other */
    /* one can be found by following the chain                        */
    Stream_TopologyChanged(self, BLT_STREAM_TOPOLOGY_NODE_CONNECTED, from);
}

#define DBG_TRYING												      \
ATX_LOG_FINE_4(                                                       \
    "  Trying from %s:%s to %s:%s", 				    	          \
    Stream_GetProtocolName(constructor.spec.input.protocol),	      \
    Stream_GetTypeName(self, constructor.spec.input.media_type),	  \
    Stream_GetProtocolName(constructor.spec.output.protocol),	      \
    Stream_GetTypeName(self, constructor.spec.output.media_type))     \
        
/*----------------------------------------------------------------------
|    Stream_CreateCompatibleMediaNode
+---------------------------------------------------------------------*/
static BLT_Result
Stream_CreateCompatibleMediaNode(Stream*              self, 
                                 StreamNode*          from_node,
                                 const BLT_MediaType* from_type,
                                 StreamNode*          to_node,
                                 BLT_MediaNode**      media_node)
{
    BLT_MediaNodeConstructor constructor;
    BLT_MediaType            output_media_type;
    BLT_Result               result;
    
    /* setup the constructor */
    constructor.spec.input.protocol   = from_node->output.protocol;
    constructor.spec.input.media_type = from_type;
    constructor.name                  = NULL;

    ATX_LOG_FINE("trying to create compatible node:");

    /* first, try to join the to_node */
    if (to_node) {
		/* first try with an expected type and the requested protocol */
        BLT_Ordinal index = 0;
        constructor.spec.output.protocol = to_node->input.protocol;
        for (;;) {
            /* get the 'nth' media type expected by the port */
            constructor.spec.output.media_type = from_type;
            result = BLT_MediaPort_QueryMediaType(
                to_node->input.port, index, 
                &constructor.spec.output.media_type);
            if (BLT_FAILED(result)) break;

			DBG_TRYING;
            result = BLT_Core_CreateCompatibleMediaNode(self->core,
                                                        &constructor,
                                                        media_node);
            if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;

            /* try the next type */
            index++;
        }
        
		/* next try with an expected type and any protocol */
        index = 0;
        constructor.spec.output.protocol = BLT_MEDIA_PORT_PROTOCOL_ANY;
        for (;;) {
            /* get the 'nth' media type expected by the port */
            constructor.spec.output.media_type = from_type;
            result = BLT_MediaPort_QueryMediaType(
                to_node->input.port, index, 
                &constructor.spec.output.media_type);
            if (BLT_FAILED(result)) break;

			DBG_TRYING;                  
            result = BLT_Core_CreateCompatibleMediaNode(self->core,
                                                        &constructor,
                                                        media_node);
            if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;

            /* try the next type */
            index++;
        }
    }

	/* use a locally-allocated media type */
    constructor.spec.output.media_type = &output_media_type;

    /* try with any type and any protocol */
    constructor.spec.output.protocol = BLT_MEDIA_PORT_PROTOCOL_ANY;
    BLT_MediaType_Init(&output_media_type, BLT_MEDIA_TYPE_ID_UNKNOWN);
	DBG_TRYING;                  
    result = BLT_Core_CreateCompatibleMediaNode(self->core,
                                                &constructor,
                                                media_node);
    if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;

    return BLT_ERROR_STREAM_NO_COMPATIBLE_NODE;
}
                      
/*----------------------------------------------------------------------
|    Stream_InterpolateChain
+---------------------------------------------------------------------*/
static BLT_Result
Stream_InterpolateChain(Stream*              self, 
                        StreamNode*          from_node, 
                        const BLT_MediaType* from_type,
                        StreamNode*          to_node,
                        StreamNode**         new_node)
{
    BLT_MediaNode* media_node;
    BLT_Result     result;
        
    /* default value */
    *new_node = NULL;

    /* find a compatible media node to be next */
    result = Stream_CreateCompatibleMediaNode(self, 
                                              from_node, 
                                              from_type,
                                              to_node,
                                              &media_node);
    if (BLT_FAILED(result)) return result;

    /* create a stream node to represent the media node */
    result = StreamNode_Create(&ATX_BASE(self, BLT_Stream), media_node, NULL, NULL, new_node);
    ATX_RELEASE_OBJECT(media_node);
    if (BLT_FAILED(result)) return result;
    (*new_node)->flags |= BLT_STREAM_NODE_FLAG_TRANSIENT;

    /* add the new node to the tail of the chain */
    return Stream_InsertChain(self, from_node, *new_node);
}

/*----------------------------------------------------------------------
|    Stream_DeliverPacket
+---------------------------------------------------------------------*/
static BLT_Result
Stream_DeliverPacket(Stream*          self, 
                     BLT_MediaPacket* packet, 
                     StreamNode*      from_node)
{
    StreamNode*          new_node;
    StreamNode*          to_node;
    const BLT_MediaType* media_type;
    unsigned int         watchdog;
    BLT_Result           result;

    if (BLT_MediaPacket_GetFlags(packet)) {
        ATX_LOG_FINE_1("Stream::DeliverPacket - flags = %x --------------------", 
                       BLT_MediaPacket_GetFlags(packet));
    }
    
    /* set the recipient node */
    to_node = from_node->next;

    for (watchdog=0; watchdog<16; watchdog++) {
        /* if we're connected, we can only try once */
        if (from_node->output.connected == BLT_TRUE) {
            result = BLT_PacketConsumer_PutPacket(to_node->input.iface.packet_consumer, packet);
            break;
        } 
        
        /* check if the recipient uses the PACKET protocol */
        if (to_node->input.protocol == BLT_MEDIA_PORT_PROTOCOL_PACKET) {
            /* try to deliver the packet to the recipient */
            result = BLT_PacketConsumer_PutPacket(to_node->input.iface.packet_consumer, packet);
            if (BLT_SUCCEEDED(result) || result != BLT_ERROR_INVALID_MEDIA_TYPE) {
                /* success, or fatal error */
                break;
            }
        } 

        /* get the media type of the packet */
        BLT_MediaPacket_GetMediaType(packet, &media_type);

        /* try to create a compatible node to receive the packet */
        result = Stream_InterpolateChain(self, 
                                         from_node, 
                                         media_type, 
                                         to_node, 
                                         &new_node);
        if (BLT_FAILED(result)) break;

        /* check that the new node has the right interface */
        if (new_node->input.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET) {
            result = BLT_ERROR_INTERNAL;
            break;
        }

        /* use the interpolated node */
        to_node = new_node;
    }
    
    if (BLT_SUCCEEDED(result)) {
        if (to_node == self->output.node) {
            /* if the packet has been delivered to the output, keep its timestamp */
            BLT_TimeStamp ts = BLT_MediaPacket_GetTimeStamp(packet);
            BLT_Time      duration = BLT_MediaPacket_GetDuration(packet);
            if (ts.seconds == 0 && ts.nanoseconds == 0) {
                /* this packet has a zero timestamp, which means that   */
                /* the timing information was probably not set by any   */
                /* of the media nodes, so we compute the real timestamp */
                /* from the previous one plus the packet duration       */
                self->output.last_time_stamp = self->output.next_time_stamp;
            } else {
                self->output.last_time_stamp = ts;
            }

            /* if the duration of the packet is 0, assume it was not set  */
            /* by any of the media nodes, so try to compute it if we know */
            /* it is PCM                                                  */
            if (duration.seconds == 0 && duration.nanoseconds == 0) {
                const BLT_MediaType* media_type = NULL;
                BLT_MediaPacket_GetMediaType(packet, &media_type);
                if (media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
                    const BLT_PcmMediaType* pcm_type = (const BLT_PcmMediaType*)media_type;
                    if (pcm_type->channel_count && pcm_type->bits_per_sample) {
                        unsigned int sample_count = BLT_MediaPacket_GetPayloadSize(packet)/
                                                    (pcm_type->channel_count*pcm_type->bits_per_sample/8);
                        duration = BLT_TimeStamp_FromSamples(sample_count, pcm_type->sample_rate);
                    }
                }
            }
            self->output.next_time_stamp = BLT_TimeStamp_Add(self->output.last_time_stamp, duration);
        }
        if (from_node->output.connected == BLT_FALSE) {
            /* we're now connected */
            Stream_ConnectNodes(self, from_node, to_node);
        }
    }
        
    /* do not keep the reference to the packet */
    BLT_MediaPacket_Release(packet);

    return result;
}

/*----------------------------------------------------------------------
|    Stream_SetupByteStreams
+---------------------------------------------------------------------*/
static BLT_Result
Stream_SetupByteStreams(Stream*     self, 
                        StreamNode* from_node, 
                        StreamNode* to_node)
{
    ATX_InputStream*     input_stream  = NULL;
    ATX_OutputStream*    output_stream = NULL;
    BLT_MediaType        unknown_media_type;
    const BLT_MediaType* media_type = NULL;
    BLT_Result           result = BLT_SUCCESS;

    /* get the 'from' node's stream type */
    result = BLT_MediaPort_QueryMediaType(
        from_node->output.port,
        0,
        &media_type);
    if (BLT_FAILED(result)) {
        BLT_MediaType_Init(&unknown_media_type, BLT_MEDIA_TYPE_ID_UNKNOWN);
        media_type = &unknown_media_type;
    }

    switch (from_node->output.protocol) {
      case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
        /* get the stream from the from_node */
        result = BLT_InputStreamProvider_GetStream(
            from_node->output.iface.stream_provider, 
            &input_stream);
        if (BLT_FAILED(result)) goto done;

        /* connect the stream to the to_node */
        result = BLT_InputStreamUser_SetStream(
            to_node->input.iface.stream_user,
            input_stream,
            media_type);
        break;

      case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
        /* get the stream from to_node */
        result = BLT_OutputStreamProvider_GetStream(
            to_node->input.iface.stream_provider,
            &output_stream,
            media_type);
        if (BLT_FAILED(result)) goto done;

        /* connect the stream to from_node */
        result = BLT_OutputStreamUser_SetStream(
            from_node->output.iface.stream_user,
            output_stream);
        break;

      default:
        return BLT_ERROR_INTERNAL;
    }

 done:
    /* release the streams */
    ATX_RELEASE_OBJECT(input_stream);
    ATX_RELEASE_OBJECT(output_stream);

    /* if we were successful, we're connected */
    if (BLT_SUCCEEDED(result)) {
        Stream_ConnectNodes(self, from_node, to_node);
    }

    return result;
}

/*----------------------------------------------------------------------
|    Stream_ConnectPort
+---------------------------------------------------------------------*/
static BLT_Result
Stream_ConnectPort(Stream*     self, 
                   StreamNode* from_node, 
                   StreamNode* to_node)
{
    StreamNode*          new_node;
    ATX_InputStream*     input_stream  = NULL;
    ATX_OutputStream*    output_stream = NULL;
    const BLT_MediaType* media_type = NULL;
    BLT_Result           result = BLT_SUCCESS;

    /* if the protocols match, try to do a stream setup */
    if (from_node->output.protocol == to_node->input.protocol) {
        result = Stream_SetupByteStreams(self, from_node, to_node);
        if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;
        if (result != BLT_ERROR_INVALID_MEDIA_TYPE) return result;
    }
    
    /* get the media type of the 'from' node's output port */
    result = BLT_MediaPort_QueryMediaType(
        from_node->output.port,
        0,
        &media_type);
    if (BLT_FAILED(result)) goto done;

    /* create a new node by interpolating the chain */
    result = Stream_InterpolateChain(self, 
                                     from_node, 
                                     media_type, 
                                     to_node, 
                                     &new_node);
    if (BLT_FAILED(result)) goto done;

    /* use the new node as the to_node */
    to_node = new_node;

    /* check that the protocols match */
    if (from_node->output.protocol != to_node->input.protocol) {
        StreamNode_Destroy(new_node);
        result = BLT_ERROR_INTERNAL;
        goto done;
    }

    /* perform the connection if we're ready for it */
    switch (from_node->output.protocol) {
      case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
        /* get the stream from the from_node */
        result = BLT_InputStreamProvider_GetStream(
            from_node->output.iface.stream_provider,
            &input_stream);
        if (BLT_FAILED(result)) goto done;

        /* connect the stream to the to_node */
        result = BLT_InputStreamUser_SetStream(
            to_node->input.iface.stream_user,
            input_stream,
            media_type);
        if (BLT_FAILED(result)) goto done;
        break;

      case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
        /* get the stream from the to_node */
        result = BLT_OutputStreamProvider_GetStream(
            to_node->input.iface.stream_provider,
            &output_stream,
            media_type);
        if (BLT_FAILED(result)) goto done;

        /* connect the stream to from_node */
        result = BLT_OutputStreamUser_SetStream(
            from_node->output.iface.stream_user,
            output_stream);
        if (BLT_FAILED(result)) goto done;
        break;

      default:
        result = BLT_ERROR_INTERNAL;
        goto done;
    }

    /* we're connected ! */
    Stream_ConnectNodes(self, from_node, to_node);

 done:
    ATX_RELEASE_OBJECT(input_stream);
    ATX_RELEASE_OBJECT(output_stream);
    return result;
}

/*----------------------------------------------------------------------
|    Stream_PumpPacket
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_PumpPacket(BLT_Stream* _self)
{
    Stream*          self = ATX_SELF(Stream, BLT_Stream);
    StreamNode*      node;
    BLT_MediaPacket* packet;
    BLT_Result       result = BLT_FAILURE;

    /* check that we have an input and an output */
    if (self->input.node  == NULL ||
        self->output.node == NULL) {
        ATX_LOG_WARNING("no input or output node");
        return BLT_FAILURE;
    }

    /* get the next available packet */
    node = self->nodes.tail;
    while (node) {
        /* ensure that the node and its sources (if any) are started */
        StreamNode* target = node;
        while (target) {
            result = StreamNode_Start(target);
            if (BLT_FAILED(result)) return result;
            if (!target->input.connected || 
                target->input.protocol != BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) {
                break;
            }
            target = target->prev;
        }

        switch (node->output.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_NONE:
            /* skip that node */
            break;

          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            /* get a packet from the node's output port */
            result = BLT_PacketProducer_GetPacket(
                node->output.iface.packet_producer,
                &packet);
            if (BLT_SUCCEEDED(result) && packet != NULL) {
                return Stream_DeliverPacket(self, packet, node);
            } else {
                if (result != BLT_ERROR_PORT_HAS_NO_DATA) {
                    return result;
                } else {
                    result = BLT_SUCCESS;
                } 
            }
            break;

          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            /* ensure that the node's port is connected */
            if (!node->output.connected) {
                result = Stream_ConnectPort(self, node, node->next);
                if (result != BLT_ERROR_PORT_HAS_NO_DATA &&
                    result != BLT_ERROR_PORT_HAS_NO_STREAM) {
                    return result;
                }
            }
            break;

          default:
            return BLT_ERROR_INTERNAL;
        }

        /* move to the previous node */
        node = node->prev;
    }

    return result;
}

/*----------------------------------------------------------------------
|    Stream_Start
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_Start(BLT_Stream* _self)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* node = self->nodes.head;

    /* start all the nodes */
    while (node) {
        StreamNode_Start(node);
        node = node->next;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_Stop
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_Stop(BLT_Stream* _self)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* node = self->nodes.head;

    /* stop all the nodes */
    while (node) {
        StreamNode_Stop(node);
        node = node->next;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_Pause
+---------------------------------------------------------------------*/
BLT_METHOD 
Stream_Pause(BLT_Stream* _self)
{
    Stream*     self = ATX_SELF(Stream, BLT_Stream);
    StreamNode* node   = self->nodes.head;

    /* pause all the nodes */
    while (node) {
        StreamNode_Pause(node);
        node = node->next;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_SetInfo
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_SetInfo(BLT_Stream* _self, const BLT_StreamInfo* info)
{
    Stream*  self = ATX_SELF(Stream, BLT_Stream);
    BLT_Mask update_mask = 0;

    self->info.mask |= info->mask;
    if ((info->mask & BLT_STREAM_INFO_MASK_TYPE) &&
        self->info.type != info->type) {
        self->info.type  = info->type;
        update_mask |= BLT_STREAM_INFO_MASK_TYPE;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_ID) &&
        self->info.id != info->id) {
        self->info.id  = info->id;
        update_mask |= BLT_STREAM_INFO_MASK_ID;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) &&
        self->info.nominal_bitrate != info->nominal_bitrate) {
        self->info.nominal_bitrate  = info->nominal_bitrate;
        update_mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) &&
        self->info.average_bitrate != info->average_bitrate) {
        self->info.average_bitrate  = info->average_bitrate;
        update_mask |= BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) &&
        self->info.instant_bitrate != info->instant_bitrate) {
        self->info.instant_bitrate  = info->instant_bitrate;
        update_mask |= BLT_STREAM_INFO_MASK_INSTANT_BITRATE;
    }   
    if ((info->mask & BLT_STREAM_INFO_MASK_SIZE) &&
        self->info.size != info->size) {
        self->info.size  = info->size;
        update_mask |= BLT_STREAM_INFO_MASK_SIZE;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_DURATION) &&
        self->info.duration != info->duration) {
        self->info.duration  = info->duration;
        update_mask |= BLT_STREAM_INFO_MASK_DURATION;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) &&
        self->info.sample_rate != info->sample_rate) {
        self->info.sample_rate  = info->sample_rate;
        update_mask |= BLT_STREAM_INFO_MASK_SAMPLE_RATE;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) &&
        self->info.channel_count != info->channel_count) {
        self->info.channel_count  = info->channel_count;
        update_mask |= BLT_STREAM_INFO_MASK_CHANNEL_COUNT;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_WIDTH) &&
        self->info.width != info->width) {
        self->info.width  = info->width;
        update_mask |= BLT_STREAM_INFO_MASK_WIDTH;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_HEIGHT) &&
        self->info.height != info->height) {
        self->info.height  = info->height;
        update_mask |= BLT_STREAM_INFO_MASK_HEIGHT;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_FLAGS) &&
        self->info.flags != info->flags) {
        self->info.flags  = info->flags;
        update_mask |= BLT_STREAM_INFO_MASK_FLAGS;
    }
    if ((info->mask & BLT_STREAM_INFO_MASK_DATA_TYPE) &&
        (self->info.data_type == NULL ||
         info->data_type == NULL        ||
         !ATX_StringsEqual(self->info.data_type, info->data_type))) {
        if (self->info.data_type != NULL) {
            ATX_FreeMemory((void*)self->info.data_type);
            self->info.data_type = NULL;
        }
        if (info->data_type) {
            self->info.data_type = ATX_DuplicateString(info->data_type);
        }
        update_mask |= BLT_STREAM_INFO_MASK_DATA_TYPE;
    }

    if (update_mask && self->event_listener != NULL) {
        BLT_StreamInfoEvent event;

        event.update_mask = update_mask;
        event.info        = self->info;
        
        BLT_EventListener_OnEvent(self->event_listener, 
                                  (ATX_Object*)self, 
                                  BLT_EVENT_TYPE_STREAM_INFO,
                                  (const BLT_Event*)(const void*)&event);
    }   
        
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetInfo
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_GetInfo(BLT_Stream* _self, BLT_StreamInfo* info)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    *info = self->info;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_GetStatus(BLT_Stream* _self, BLT_StreamStatus* status)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    /* set the stream status */
    status->time_stamp = self->output.next_time_stamp;
    if (self->info.duration) {
        /* estimate the position from the time stamp and duration */
        status->position.offset = BLT_TimeStamp_ToMillis(status->time_stamp);
        status->position.range  = self->info.duration;
    } else {
        status->position.offset = 0;
        status->position.range  = 0;
    }
    
    /* compute the output node status */
    BLT_TimeStamp_Set(status->output_status.media_time, 0, 0);
    if (self->output.output_node) {
        /* get the output status from the output node */
        BLT_OutputNode_GetStatus(self->output.output_node, 
                                 &status->output_status);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_GetProperties
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_GetProperties(BLT_Stream* _self, ATX_Properties** properties)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);
    *properties = self->properties;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_EstimateSeekPoint
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_EstimateSeekPoint(BLT_Stream*    _self, 
                         BLT_SeekMode   mode,
                         BLT_SeekPoint* point)
{
    Stream* self = ATX_SELF(Stream, BLT_Stream);

    switch (mode) {
      case BLT_SEEK_MODE_IGNORE:
        return BLT_SUCCESS;

      case BLT_SEEK_MODE_BY_TIME_STAMP:
        /* estimate from the timestamp */
        if ((point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP) &&
            self->info.duration) {
            /* estimate the offset from the time stamp and duration */
            ATX_UInt64 offset = BLT_TimeStamp_ToMillis(point->time_stamp);
            offset *= self->info.size;
            offset /= self->info.duration;
            if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
                point->offset = (ATX_Offset)offset;
                point->mask |= BLT_SEEK_POINT_MASK_OFFSET;
            }
                
            /* estimate the position from the offset and size */
            if (!(point->mask & BLT_SEEK_POINT_MASK_POSITION)) {
                point->mask |= BLT_SEEK_POINT_MASK_POSITION;
                point->position.offset = (ATX_Offset)(offset);
                point->position.range  = self->info.size;
                point->mask |= BLT_SEEK_POINT_MASK_POSITION;
            }
            break;
        } else {
            return BLT_FAILURE;
        }

      case BLT_SEEK_MODE_BY_POSITION:
        /* estimate from the position */
        if (point->mask & BLT_SEEK_POINT_MASK_POSITION) {
            /* estimate the offset from the position and size */
            ATX_UInt64 offset = (((ATX_UInt64)point->position.offset) *
                                self->info.size)/point->position.range;
            if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
                point->offset = (ATX_Offset)offset;
                point->mask |= BLT_SEEK_POINT_MASK_OFFSET;
            }

            /* estimate the time stamp from the position and duration */
            if (!(point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP) &&
                point->position.range) {
                ATX_UInt64 time_stamp_ms = ((ATX_UInt64)self->info.duration*point->position.offset)/point->position.range;
                point->time_stamp = BLT_TimeStamp_FromMillis(time_stamp_ms);
                point->mask |= BLT_SEEK_POINT_MASK_TIME_STAMP;
            }
            break;
        } else {
            return BLT_FAILURE; 
        }

      case BLT_SEEK_MODE_BY_OFFSET:
        /* estimate from the offset */
        if (point->mask & BLT_SEEK_POINT_MASK_OFFSET) {
            if (!(point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP) &&
                self->info.size) {
                /* estimate the time stamp from offset, size and duration */
                ATX_UInt64 time_stamp_ms = (((ATX_UInt64)self->info.duration)*point->offset)/self->info.size;
                point->time_stamp = BLT_TimeStamp_FromMillis(time_stamp_ms);
                point->mask |= BLT_SEEK_POINT_MASK_TIME_STAMP;
            }
            if (!(point->mask & BLT_SEEK_POINT_MASK_POSITION) &&
                self->info.size) {
                point->position.offset = point->offset;
                point->position.range  = self->info.size;
                point->mask |= BLT_SEEK_POINT_MASK_POSITION;
            }
            break;
        } else {
            return BLT_FAILURE;
        }

      case BLT_SEEK_MODE_BY_SAMPLE:
        /* estimate from the sample offset */
        if (point->mask & BLT_SEEK_POINT_MASK_SAMPLE) {
            if (self->info.duration && self->info.sample_rate) {
                /* compute position from duration and sample rate */
                ATX_UInt64 duration_samples = (((ATX_UInt64)self->info.duration)*self->info.sample_rate)/1000;
                point->position.offset = (ATX_Offset)(point->sample/100);
                point->position.range  = (ATX_Range)(duration_samples/100);
                point->mask |= BLT_SEEK_POINT_MASK_POSITION;

                /* compute offset from size and duration */
                point->offset = (((ATX_UInt64)self->info.size*(ATX_UInt64)point->sample)/duration_samples);
                point->mask |= BLT_SEEK_POINT_MASK_OFFSET;
                
                /* compute time stamp */
                point->time_stamp = BLT_TimeStamp_FromSamples(point->sample, self->info.sample_rate);
                point->mask |= BLT_SEEK_POINT_MASK_TIME_STAMP;
            }
            break;
        } else {
            return BLT_FAILURE;
        }
    }

    /* estimate the sample offset from the time stamp and sample rate */
    if (point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP &&
        !(point->mask & BLT_SEEK_POINT_MASK_SAMPLE) && self->info.sample_rate) {
        ATX_UInt64 time_stamp_ms = BLT_TimeStamp_ToMillis(point->time_stamp);
        point->sample = (time_stamp_ms*self->info.sample_rate)/1000;
        point->mask |= BLT_SEEK_POINT_MASK_SAMPLE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Stream_Seek
+---------------------------------------------------------------------*/
static BLT_Result
Stream_Seek(Stream*        self, 
            BLT_SeekMode*  mode,
            BLT_SeekPoint* point)
{
    StreamNode* node = self->nodes.tail;
    BLT_Result  result;

    /* check parameters */
    if (mode == NULL || point == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* go through all the nodes in reverse order */
    while (node) {
        /* tell the node to seek */
        result = StreamNode_Seek(node, mode, point);
        if (BLT_FAILED(result)) return result;

        /* move to the previous node */
        node = node->prev;
    }

    /* keep a copy of the time stamp as our last time stamp */
    if (!(point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP)) {
        Stream_EstimateSeekPoint(&ATX_BASE(self, BLT_Stream), *mode, point);
    }
    self->output.last_time_stamp = point->time_stamp;
    self->output.next_time_stamp = point->time_stamp;

    return BLT_SUCCESS;
}

/*---------------------------------------------------------------------
|    Stream_SeekToTime
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_SeekToTime(BLT_Stream* _self, BLT_UInt64 time)
{
    Stream*       self = ATX_SELF(Stream, BLT_Stream);
    BLT_SeekPoint point;
    BLT_SeekMode  mode;

    /* setup the seek request */
    ATX_SetMemory(&point, 0, sizeof(point));
    mode       = BLT_SEEK_MODE_BY_TIME_STAMP;
    point.mask = BLT_SEEK_POINT_MASK_TIME_STAMP;
    point.time_stamp.seconds     = (BLT_Int32)(time/1000);
    point.time_stamp.nanoseconds = 
        (BLT_Int32)
        ((time-(point.time_stamp.seconds*1000))*1000000);

    /* fill in the other seek point fields */
    Stream_EstimateSeekPoint(_self, mode, &point);

    /* perform the seek */
    return Stream_Seek(self, &mode, &point);
}

/*----------------------------------------------------------------------
|    Stream_SeekToPosition
+---------------------------------------------------------------------*/
BLT_METHOD
Stream_SeekToPosition(BLT_Stream* _self, 
                      BLT_UInt64  offset,
                      BLT_UInt64  range)
{
    Stream*       self = ATX_SELF(Stream, BLT_Stream);
    BLT_SeekPoint point;
    BLT_SeekMode  mode;

    /* sanitize the parameters */
    if (offset > range) offset = range;

    /* setup the seek request */
    ATX_SetMemory(&point, 0, sizeof(point));
    mode       = BLT_SEEK_MODE_BY_POSITION;
    point.mask = BLT_SEEK_POINT_MASK_POSITION;
    point.position.offset = offset;
    point.position.range  = range;

    /* fill in the other seek point fields */
    Stream_EstimateSeekPoint(_self, mode, &point);

    /* perform the seek */
    return Stream_Seek(self, &mode, &point);
}

/*----------------------------------------------------------------------
|    Stream_OnEvent
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
Stream_OnEvent(BLT_EventListener* _self,
               ATX_Object*        source,
               BLT_EventType      type,
               const BLT_Event*   event)
{
    Stream* self = ATX_SELF(Stream, BLT_EventListener);

    if (self->event_listener) {
        BLT_EventListener_OnEvent(self->event_listener, 
                                  source, type, event);
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Stream)
    ATX_GET_INTERFACE_ACCEPT(Stream, BLT_Stream)
    ATX_GET_INTERFACE_ACCEPT(Stream, BLT_EventListener)
    ATX_GET_INTERFACE_ACCEPT(Stream, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_Stream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Stream, BLT_Stream)
    Stream_SetEventListener,
    Stream_ResetInput,
    Stream_SetInput,
    Stream_SetInputNode,
    Stream_GetInputNode,
    Stream_ResetOutput,
    Stream_SetOutput,
    Stream_SetOutputNode,
    Stream_GetOutputNode,
    Stream_AddNode,
    Stream_AddNodeByName,
    Stream_GetStreamNodeInfo,
    Stream_GetFirstNode,
    Stream_GetNextNode,
    Stream_PumpPacket,
    Stream_Start,
    Stream_Stop,
    Stream_Pause,
    Stream_SetInfo,
    Stream_GetInfo,
    Stream_GetStatus,
    Stream_GetProperties,
    Stream_EstimateSeekPoint,
    Stream_SeekToTime,
    Stream_SeekToPosition
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_EventListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Stream, BLT_EventListener)
    Stream_OnEvent
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(Stream, reference_count)



