/*****************************************************************
|
|   Silence Remover Module
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
#include "BltCore.h"
#include "BltSilenceRemover.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.general.silence-remover")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef BLT_BaseModule SilenceRemoverModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    BLT_MediaPacket* pending;
} SilenceRemoverInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    ATX_List* packets;
} SilenceRemoverOutput;

typedef enum {
    SILENCE_REMOVER_STATE_START_OF_STREAM,
    SILENCE_REMOVER_STATE_IN_STREAM
} SilenceRemoverState;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    SilenceRemoverState  state;
    SilenceRemoverInput  input;
    SilenceRemoverOutput output;
} SilenceRemover;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_SILENCE_REMOVER_THRESHOLD 64

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(SilenceRemoverModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(SilenceRemover, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(SilenceRemover, ATX_Referenceable)

/*----------------------------------------------------------------------
|    ScanPacket
+---------------------------------------------------------------------*/
static BLT_Result
ScanPacket(BLT_MediaPacket* packet, 
           BLT_Cardinal*    zero_head, 
           BLT_Cardinal*    zero_tail)
{
    BLT_PcmMediaType* media_type;
    short*            pcm;
    BLT_Cardinal      sample_count;
    BLT_Ordinal       sample;
    BLT_Cardinal      zero_head_count = 0;
    BLT_Cardinal      non_zero_run = 0;

    /* default values */
    *zero_head = 0;
    *zero_tail = 0;

    /* get the media type */
    BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)(const void*)&media_type);
    /* check the media type */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* for now, we only support 16-bit, stereo, PCM */
    if (media_type->bits_per_sample != 16 || media_type->channel_count != 2) {
        return BLT_SUCCESS;
    }

    /* check for zero samples */
    sample_count = BLT_MediaPacket_GetPayloadSize(packet)/4;
    if (sample_count == 0) return BLT_SUCCESS;
    pcm = BLT_MediaPacket_GetPayloadBuffer(packet);
    for (sample = 0; sample < sample_count; sample++, pcm+=2) { 
        if (pcm[0] > -BLT_SILENCE_REMOVER_THRESHOLD && 
            pcm[0] <  BLT_SILENCE_REMOVER_THRESHOLD && 
            pcm[1] > -BLT_SILENCE_REMOVER_THRESHOLD && 
            pcm[1] <  BLT_SILENCE_REMOVER_THRESHOLD) {
            if (sample == zero_head_count) {
                zero_head_count++;
            }
        } else {
            non_zero_run = sample+1;
        }
    }
    *zero_head = zero_head_count*4;
    if (non_zero_run > 0) {
        *zero_tail = (sample_count-non_zero_run)*4;
    }

    return BLT_SUCCESS;     
}

/*----------------------------------------------------------------------
|    SilenceRemover_TrimPending
+---------------------------------------------------------------------*/
static void
SilenceRemover_TrimPending(SilenceRemover* self)
{
    BLT_MediaPacket* packet = self->input.pending;
    short*           pcm;
    BLT_Cardinal     sample_count;
    BLT_Cardinal     skip = 0;
    int              sample;

    /* quick check */
    if (!packet) return;

    ATX_LOG_FINER("SilenceRemover: trimming pending packet");

    /* remove silence at the end of the packet */
    pcm = (short*)BLT_MediaPacket_GetPayloadBuffer(packet);
    sample_count = BLT_MediaPacket_GetPayloadSize(packet)/4;
    pcm += sample_count*2;
    for (sample = sample_count-1; sample >= 0; sample--, pcm-=2) {
        if (pcm[0] > -BLT_SILENCE_REMOVER_THRESHOLD && 
            pcm[0] <  BLT_SILENCE_REMOVER_THRESHOLD && 
            pcm[1] > -BLT_SILENCE_REMOVER_THRESHOLD && 
            pcm[1] <  BLT_SILENCE_REMOVER_THRESHOLD) {
            skip++;
        }
    }
    BLT_MediaPacket_SetPayloadSize(packet, (sample_count-skip)*4);
}

/*----------------------------------------------------------------------
|    SilenceRemover_AcceptPending
+---------------------------------------------------------------------*/
static void
SilenceRemover_AcceptPending(SilenceRemover* self)
{
    BLT_MediaPacket* packet = self->input.pending;
    BLT_Result       result;

    if (packet != NULL) {
        result = ATX_List_AddData(self->output.packets, packet);
        if (ATX_FAILED(result)) {
            BLT_MediaPacket_Release(packet);
        }
        self->input.pending = NULL;
        ATX_LOG_FINER("SilenceRemover: accepting pending packet");
    } else {
        ATX_LOG_FINER("SilenceRemover: no pending packet");
    }
}

/*----------------------------------------------------------------------
|    SilenceRemover_HoldPacket
+---------------------------------------------------------------------*/
static void
SilenceRemover_HoldPacket(SilenceRemover* self, BLT_MediaPacket* packet)
{
    ATX_LOG_FINER("SilenceRemover: holding packet");

    /* accept the previously pending packet if any */
    SilenceRemover_AcceptPending(self);

    /* hold the packet as a pending input */
    self->input.pending = packet;
    BLT_MediaPacket_AddReference(packet);
}

/*----------------------------------------------------------------------
|    SilenceRemover_AcceptPacket
+---------------------------------------------------------------------*/
static void
SilenceRemover_AcceptPacket(SilenceRemover* self, BLT_MediaPacket* packet)
{
    BLT_Result result;
    ATX_LOG_FINER("SilenceRemover: accepting packet");

    /* first, use any pending packet */
    SilenceRemover_AcceptPending(self);

    /* add the packet to the output list */
    result = ATX_List_AddData(self->output.packets, packet);
    if (ATX_SUCCEEDED(result)) {
        BLT_MediaPacket_AddReference(packet);
    }
}

/*----------------------------------------------------------------------
|    SilenceRemoverInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemoverInput_PutPacket(BLT_PacketConsumer* _self,
                              BLT_MediaPacket*    packet)
{
    SilenceRemover* self = ATX_SELF_M(input, SilenceRemover, BLT_PacketConsumer);
    BLT_Flags       packet_flags;
    BLT_Cardinal    zero_head = 0;
    BLT_Cardinal    zero_tail = 0;
    BLT_Offset      payload_offset;
    BLT_Size        payload_size;
    ATX_Result      result;

    ATX_LOG_FINER("SilenceRemoverInput::PutPacket");

    /* get the packet info */
    packet_flags   = BLT_MediaPacket_GetFlags(packet);
    payload_offset = BLT_MediaPacket_GetPayloadOffset(packet);
    payload_size   = BLT_MediaPacket_GetPayloadSize(packet);

    /* scan the packet for zeros */
    if (payload_size != 0) {
        result = ScanPacket(packet, &zero_head, &zero_tail);    
        if (BLT_FAILED(result)) return result;
        if (zero_head || zero_tail) {
            ATX_LOG_FINER_2("SilenceRemoverInput::PutPacket zero_head=%ld, zero_tail=%ld",
                            zero_head, zero_tail);
        }
    }

    /* decide how to process the packet */
    if (self->state == SILENCE_REMOVER_STATE_START_OF_STREAM) {
        if (zero_head == payload_size) {
            /* packet is all silence */
            if (packet_flags != 0) {
                /* packet has flags, don't discard it, just empty it */
                ATX_LOG_FINER("SilenceRemover: emptying packet");
                BLT_MediaPacket_SetPayloadSize(packet, 0);
                SilenceRemover_AcceptPacket(self, packet);
            } else {
                ATX_LOG_FINER("SilenceRemover: dropping packet");
            }
        } else {
            /* remove silence at the start of the packet */
            BLT_MediaPacket_SetPayloadOffset(packet, payload_offset+zero_head);
            SilenceRemover_AcceptPacket(self, packet);

            /* we're now in the stream unless this is also the end */
            if (!(packet_flags & BLT_MEDIA_PACKET_FLAG_END_OF_STREAM)) {
                ATX_LOG_FINER("SilenceRemover: new state = IN_STREAM");
                self->state =  SILENCE_REMOVER_STATE_IN_STREAM;
            }
        }
    } else {
        /* in stream */
        if (zero_head == payload_size) {
            /* packet is all silence */
            ATX_LOG_FINER("SilenceRemover: packet is all silence");
            if (packet_flags) {
                /* packet has flags, don't discard it, just empty it */
                SilenceRemover_TrimPending(self);
                BLT_MediaPacket_SetPayloadSize(packet, 0);
                SilenceRemover_AcceptPacket(self, packet);
            } else {
                ATX_LOG_FINER("SilenceRemover: dropping packet");
            }
        } else {
            /* accept the pending packet */
            SilenceRemover_AcceptPending(self);
            if (zero_tail) {
                /* packet has some silence at the end */
                ATX_LOG_FINER("SilenceRemover: packet has silence at end");
                SilenceRemover_HoldPacket(self, packet);
            } else {
                /* packet has no silence at the end */
                ATX_LOG_FINER("SilenceRemover: packet has no silence at end");
                SilenceRemover_AcceptPacket(self, packet);
            }
        }
        if (packet_flags & BLT_MEDIA_PACKET_FLAG_END_OF_STREAM ||
            packet_flags & BLT_MEDIA_PACKET_FLAG_START_OF_STREAM) {
            ATX_LOG_FINER("SilenceRemover: new state = START_OF_STREAM");
            self->state = SILENCE_REMOVER_STATE_START_OF_STREAM;
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   SilenceRemoverInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemoverInput_QueryMediaType(BLT_MediaPort*         self,
                               BLT_Ordinal            index,
                               const BLT_MediaType**  media_type)
{
    BLT_COMPILER_UNUSED(self);
    if (index == 0) {
        *media_type = &BLT_GenericPcmMediaType;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(SilenceRemoverInput)
    ATX_GET_INTERFACE_ACCEPT(SilenceRemoverInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(SilenceRemoverInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(SilenceRemoverInput, BLT_PacketConsumer)
    SilenceRemoverInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(SilenceRemoverInput,
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(SilenceRemoverInput, BLT_MediaPort)
    SilenceRemoverInput_GetName,
    SilenceRemoverInput_GetProtocol,
    SilenceRemoverInput_GetDirection,
    SilenceRemoverInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    SilenceRemoverOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemoverOutput_GetPacket(BLT_PacketProducer* _self,
                               BLT_MediaPacket**   packet)
{
    SilenceRemover* self = ATX_SELF_M(output, SilenceRemover, BLT_PacketProducer);
    ATX_ListItem*   item;

    item = ATX_List_GetFirstItem(self->output.packets);
    if (item) {
        *packet = ATX_ListItem_GetData(item);
        ATX_List_RemoveItem(self->output.packets, item);
        return BLT_SUCCESS;
    } else {
        *packet = NULL;
        return BLT_ERROR_PORT_HAS_NO_DATA;
    }
}

/*----------------------------------------------------------------------
|   SilenceRemoverOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemoverOutput_QueryMediaType(BLT_MediaPort*        self,
                                    BLT_Ordinal           index,
                                    const BLT_MediaType** media_type)
{
    BLT_COMPILER_UNUSED(self);
    if (index == 0) {
        *media_type = &BLT_GenericPcmMediaType;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(SilenceRemoverOutput)
    ATX_GET_INTERFACE_ACCEPT(SilenceRemoverOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(SilenceRemoverOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(SilenceRemoverOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(SilenceRemoverOutput, BLT_MediaPort)
    SilenceRemoverOutput_GetName,
    SilenceRemoverOutput_GetProtocol,
    SilenceRemoverOutput_GetDirection,
    SilenceRemoverOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(SilenceRemoverOutput, BLT_PacketProducer)
    SilenceRemoverOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    SilenceRemover_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
SilenceRemover_SetupPorts(SilenceRemover* self)
{
    ATX_Result result;

    /* no pending packet yet */
    self->input.pending = NULL;

    /* create a list of output packets */
    result = ATX_List_Create(&self->output.packets);
    if (ATX_FAILED(result)) return result;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    SilenceRemover_Create
+---------------------------------------------------------------------*/
static BLT_Result
SilenceRemover_Create(BLT_Module*              module,
                      BLT_Core*                core, 
                      BLT_ModuleParametersType parameters_type,
                      BLT_AnyConst             parameters, 
                      BLT_MediaNode**          object)
{
    SilenceRemover* self;
    BLT_Result  result;

    ATX_LOG_FINE("SilenceRemover::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(SilenceRemover));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    self->state = SILENCE_REMOVER_STATE_START_OF_STREAM;

    /* setup the input and output ports */
    result = SilenceRemover_SetupPorts(self);
    if (BLT_FAILED(result)) {
        BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, SilenceRemover, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, SilenceRemover, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  SilenceRemoverInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  SilenceRemoverInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, SilenceRemoverOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, SilenceRemoverOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    SilenceRemover_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
SilenceRemover_Destroy(SilenceRemover* self)
{ 
    ATX_ListItem* item;

    ATX_LOG_FINE("SilenceRemover::Destroy");

    /* release any input packet we may hold */
    if (self->input.pending) {
        BLT_MediaPacket_Release(self->input.pending);
    }

    /* release any output packet we may hold */
    item = ATX_List_GetFirstItem(self->output.packets);
    while (item) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) {
            BLT_MediaPacket_Release(packet);
        }
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Destroy(self->output.packets);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   SilenceRemover_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemover_GetPortByName(BLT_MediaNode*  _self,
                             BLT_CString     name,
                             BLT_MediaPort** port)
{
    SilenceRemover* self = ATX_SELF_EX(SilenceRemover, BLT_BaseMediaNode, BLT_MediaNode);

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
|    SilenceRemover_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemover_Seek(BLT_MediaNode* instance,
                    BLT_SeekMode*  mode,
                    BLT_SeekPoint* point)
{
    SilenceRemover* self = (SilenceRemover*)instance;
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);
    
    /* flush the pending packet */
    if (self->input.pending) {
        BLT_MediaPacket_Release(self->input.pending);
    } 
    self->input.pending = NULL;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(SilenceRemover)
    ATX_GET_INTERFACE_ACCEPT_EX(SilenceRemover, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(SilenceRemover, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(SilenceRemover, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    SilenceRemover_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    SilenceRemover_Seek
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(SilenceRemover, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   SilenceRemoverModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
SilenceRemoverModule_Probe(BLT_Module*              self,  
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

            /* we need a name */
            if (constructor->name == NULL ||
                !ATX_StringsEqual(constructor->name, "SilenceRemover")) {
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

            ATX_LOG_FINE_1("SilenceRemoverModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(SilenceRemoverModule)
    ATX_GET_INTERFACE_ACCEPT(SilenceRemoverModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT(SilenceRemoverModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(SilenceRemoverModule, SilenceRemover)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(SilenceRemoverModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    SilenceRemoverModule_CreateInstance,
    SilenceRemoverModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define SilenceRemoverModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(SilenceRemoverModule, reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(SilenceRemoverModule,
                                         "Silence Remover",
                                         "com.axiosys.general.silence-remover",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
