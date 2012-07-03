/*****************************************************************
|
|   Fingerprint Filter Module
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltFingerprintFilter.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.filters.fingerprint")

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_FINGERPRINT_FILTER_MODE_DISABLED  0
#define BLT_FINGERPRINT_FILTER_MODE_ENABLED   1

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef BLT_BaseModule FingerprintFilterModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);
} FingerprintFilterInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_MediaPacket* packet;
} FingerprintFilterOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(ATX_PropertyListener);

    /* members */
    FingerprintFilterInput     input;
    FingerprintFilterOutput    output;
    ATX_UInt32                 mode;
    ATX_PropertyListenerHandle property_listener_handle;
    ATX_UInt32                 fingerprint_value;
} FingerprintFilter;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(FingerprintFilterModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(FingerprintFilter, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(FingerprintFilter, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(FingerprintFilter, ATX_PropertyListener)

/*----------------------------------------------------------------------
|    FingerprintFilterInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilterInput_PutPacket(BLT_PacketConsumer* _self,
                                 BLT_MediaPacket*    packet)
{
    FingerprintFilter* self = ATX_SELF_M(input, FingerprintFilter, BLT_PacketConsumer);
    BLT_PcmMediaType*  media_type;
    BLT_Flags          packet_flags;
    BLT_Result         result;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)(const void*)&media_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type, we can only filter PCM */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* keep the packet */
    self->output.packet = packet;
    BLT_MediaPacket_AddReference(packet);

    /* exit now if we're not enabled */
    if (self->mode == BLT_FINGERPRINT_FILTER_MODE_DISABLED) {
        return BLT_SUCCESS;
    }

    /* for now, we only support 16-bit */
    if (media_type->bits_per_sample != 16) {
        return BLT_SUCCESS;
    }

    /* process the PCM data */
    /* TODO: implement actual fingerprint here */
    packet_flags = BLT_MediaPacket_GetFlags(packet);
    ATX_LOG_FINE_1("processing packet - flags=%d", packet_flags);
    if (packet_flags & BLT_MEDIA_PACKET_FLAG_START_OF_STREAM) {
        /* reset the fingerprint value */
        self->fingerprint_value = 0;
    }
    /* dummy example: fingerprint value = total number of PCM samples */
    if (media_type->channel_count) {
        ATX_UInt32 sample_count = BLT_MediaPacket_GetPayloadSize(packet)/(media_type->channel_count*media_type->bits_per_sample/8);
        self->fingerprint_value += sample_count;
    }
    if (packet_flags & BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
        /* emit the fingerprint as a stream property */
        /* as a dummy example, the fingerprint value is the string 'FP:XXXX' where XXXX is the PCM sample count */
        /* measured during the fingerprint measuring phase */
        ATX_Properties*   stream_properties = NULL;
        ATX_PropertyValue fingerprint_property;
        char              fingerprint_string[64];
        ATX_FormatStringN(fingerprint_string, sizeof(fingerprint_string), "FP:%d", self->fingerprint_value);
        BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_properties);
        fingerprint_property.type        = ATX_PROPERTY_VALUE_TYPE_STRING;
        fingerprint_property.data.string = fingerprint_string;
        ATX_Properties_SetProperty(stream_properties, "dummy.fingerprint", &fingerprint_property);
        self->fingerprint_value = 0;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   FingerprintFilterInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilterInput_QueryMediaType(BLT_MediaPort*        self,
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FingerprintFilterInput)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilterInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilterInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FingerprintFilterInput, BLT_PacketConsumer)
    FingerprintFilterInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FingerprintFilterInput,
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(FingerprintFilterInput, BLT_MediaPort)
    FingerprintFilterInput_GetName,
    FingerprintFilterInput_GetProtocol,
    FingerprintFilterInput_GetDirection,
    FingerprintFilterInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    FingerprintFilterOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilterOutput_GetPacket(BLT_PacketProducer* _self,
                                  BLT_MediaPacket**   packet)
{
    FingerprintFilter* self = ATX_SELF_M(output, FingerprintFilter, BLT_PacketProducer);

    if (self->output.packet) {
        *packet = self->output.packet;
        self->output.packet = NULL;
        return BLT_SUCCESS;
    } else {
        *packet = NULL;
        return BLT_ERROR_PORT_HAS_NO_DATA;
    }
}

/*----------------------------------------------------------------------
|   FingerprintFilterOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilterOutput_QueryMediaType(BLT_MediaPort*        self,
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FingerprintFilterOutput)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilterOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilterOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(FingerprintFilterOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(FingerprintFilterOutput, BLT_MediaPort)
    FingerprintFilterOutput_GetName,
    FingerprintFilterOutput_GetProtocol,
    FingerprintFilterOutput_GetDirection,
    FingerprintFilterOutput_QueryMediaType
ATX_END_INTERFACE_MAP


/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FingerprintFilterOutput, BLT_PacketProducer)
    FingerprintFilterOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    FingerprintFilter_Create
+---------------------------------------------------------------------*/
static BLT_Result
FingerprintFilter_Create(BLT_Module*              module,
                         BLT_Core*                core, 
                         BLT_ModuleParametersType parameters_type,
                         BLT_AnyConst             parameters, 
                         BLT_MediaNode**          object)
{
    FingerprintFilter* self;

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(FingerprintFilter));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, FingerprintFilter, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, FingerprintFilter, ATX_PropertyListener);
    ATX_SET_INTERFACE(&self->input,  FingerprintFilterInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  FingerprintFilterInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, FingerprintFilterOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, FingerprintFilterOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FingerprintFilter_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
FingerprintFilter_Destroy(FingerprintFilter* self)
{ 
    /* release any input packet we may hold */
    if (self->output.packet) {
        BLT_MediaPacket_Release(self->output.packet);
    }

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   FingerprintFilter_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilter_GetPortByName(BLT_MediaNode*  _self,
                                BLT_CString     name,
                                BLT_MediaPort** port)
{
    FingerprintFilter* self = ATX_SELF_EX(FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode);

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
|    FingerprintFilter_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilter_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    FingerprintFilter* self = ATX_SELF_EX(FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode);

    /* keep a reference to the stream */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    /* listen to core properties */
    if (stream) {
        ATX_Properties* properties;
        if (BLT_SUCCEEDED(BLT_Core_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).core, &properties))) {
            ATX_PropertyValue property;
            ATX_Properties_AddListener(properties, 
                                       BLT_FINGERPRINT_FILTER_MODE,
                                       &ATX_BASE(self, ATX_PropertyListener),
                                       &self->property_listener_handle);

            /* read the initial value of the property */
            self->mode = BLT_FINGERPRINT_FILTER_MODE_DISABLED;

            if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties,
                                                         BLT_FINGERPRINT_FILTER_MODE,
                                                         &property)) &&
                property.type == ATX_PROPERTY_VALUE_TYPE_INTEGER) {
                self->mode = property.data.integer;
            }
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FingerprintFilter_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilter_Deactivate(BLT_MediaNode* _self)
{
    FingerprintFilter* self = ATX_SELF_EX(FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode);

    /* reset */
    self->mode = BLT_FINGERPRINT_FILTER_MODE_DISABLED;

    /* remove our listener */
    if (ATX_BASE(self, BLT_BaseMediaNode).context) {
        ATX_Properties* properties;
        if (BLT_SUCCEEDED(BLT_Core_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).core, &properties))) {
            ATX_Properties_RemoveListener(properties, &self->property_listener_handle);
        }
    }

    /* we're detached from the stream */
    ATX_BASE(self, BLT_BaseMediaNode).context = NULL;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    FingerprintFilter_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilter_Seek(BLT_MediaNode* _self,
                       BLT_SeekMode*  mode,
                       BLT_SeekPoint* point)
{
    FingerprintFilter* self = ATX_SELF_EX(FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode);

    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    if (self->output.packet) {
        BLT_MediaPacket_Release(self->output.packet);
        self->output.packet = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FingerprintFilter)
    ATX_GET_INTERFACE_ACCEPT_EX(FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(FingerprintFilter, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilter, ATX_PropertyListener)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(FingerprintFilter, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    FingerprintFilter_GetPortByName,
    FingerprintFilter_Activate,
    FingerprintFilter_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    FingerprintFilter_Seek
};

/*----------------------------------------------------------------------
|    FingerprintFilter_OnPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
FingerprintFilter_OnPropertyChanged(ATX_PropertyListener*    _self,
                                    ATX_CString              name,
                                    const ATX_PropertyValue* value)
{
    FingerprintFilter* self = ATX_SELF(FingerprintFilter, ATX_PropertyListener);

    if (name && 
        (value == NULL || value->type == ATX_PROPERTY_VALUE_TYPE_INTEGER)) {
        if (ATX_StringsEqual(name, BLT_FINGERPRINT_FILTER_MODE)) {
            self->mode = value?value->data.integer:BLT_FINGERPRINT_FILTER_MODE_DISABLED;
        }
    }
}

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FingerprintFilter, ATX_PropertyListener)
    FingerprintFilter_OnPropertyChanged,
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(FingerprintFilter, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   FingerprintFilterModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
FingerprintFilterModule_Probe(BLT_Module*              self,  
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
                !ATX_StringsEqual(constructor->name, "com.axiosys.filter.fingerprint")) {
                return BLT_FAILURE;
            }

            /* the input and output protocols should be PACKET */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* match level is always exact */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(FingerprintFilterModule)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilterModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT(FingerprintFilterModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(FingerprintFilterModule, FingerprintFilter)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(FingerprintFilterModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    FingerprintFilterModule_CreateInstance,
    FingerprintFilterModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define FingerprintFilterModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(FingerprintFilterModule, reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(FingerprintFilterModule,
                                         "Fingerprint Filter",
                                         "com.axiosys.filter.fingerprint",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
