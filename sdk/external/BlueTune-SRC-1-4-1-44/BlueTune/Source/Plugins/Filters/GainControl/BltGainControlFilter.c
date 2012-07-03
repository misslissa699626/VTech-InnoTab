/*****************************************************************
|
|   Gain Control Filter Module
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <math.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltCore.h"
#include "BltGainControlFilter.h"
#include "BltReplayGain.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.filters.gain-control")

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_GAIN_CONTROL_FILTER_MODULE_NAME "com.axiosys.filter.gain-control"

#define BLT_GAIN_CONTROL_FILTER_FACTOR_RANGE 1024
#define BLT_GAIN_CONTROL_FILTER_MAX_FACTOR   16*1024

#define BLT_GAIN_CONTROL_REPLAY_GAIN_TRACK_VALUE_SET 1
#define BLT_GAIN_CONTROL_REPLAY_GAIN_ALBUM_VALUE_SET 2

#define BLT_GAIN_CONTROL_REPLAY_GAIN_MIN (-2300)
#define BLT_GAIN_CONTROL_REPLAY_GAIN_MAX (1700)

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef BLT_BaseModule GainControlFilterModule;

typedef enum {
    BLT_GAIN_CONTROL_FILTER_MODE_INACTIVE,
    BLT_GAIN_CONTROL_FILTER_MODE_AMPLIFY,
    BLT_GAIN_CONTROL_FILTER_MODE_ATTENUATE
} GainControlMode;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);
} GainControlFilterInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_MediaPacket* packet;
} GainControlFilterOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(ATX_PropertyListener);

    /* members */
    GainControlFilterInput  input;
    GainControlFilterOutput output;
    GainControlMode         mode;
    unsigned short          factor;
    struct {
        BLT_Flags flags;
        int       track_gain;
        int       album_gain;
    } replay_gain_info;
    ATX_PropertyListenerHandle track_gain_listener_handle;
    ATX_PropertyListenerHandle album_gain_listener_handle;
} GainControlFilter;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(GainControlFilterModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(GainControlFilter, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(GainControlFilter, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(GainControlFilter, ATX_PropertyListener)

/*----------------------------------------------------------------------
|    GainControlFilterInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilterInput_PutPacket(BLT_PacketConsumer* _self,
                                 BLT_MediaPacket*    packet)
{
    GainControlFilter* self = ATX_SELF_M(input, GainControlFilter, BLT_PacketConsumer);
    BLT_PcmMediaType*  media_type;
    BLT_Cardinal       sample_count;
    short*             pcm;
    BLT_Result         result;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)(const void*)&media_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* keep the packet */
    self->output.packet = packet;
    BLT_MediaPacket_AddReference(packet);

    /* exit now if we're inactive */
    if (self->mode == BLT_GAIN_CONTROL_FILTER_MODE_INACTIVE ||
        self->factor == 0) {
        return BLT_SUCCESS;
    }

    /* for now, we only support 16-bit */
    if (media_type->bits_per_sample != 16) {
        return BLT_SUCCESS;
    }

    /* adjust the gain */
    pcm = (short*)BLT_MediaPacket_GetPayloadBuffer(packet);
    sample_count = BLT_MediaPacket_GetPayloadSize(packet)/2;
    if (self->mode == BLT_GAIN_CONTROL_FILTER_MODE_AMPLIFY) {
        register unsigned short factor = self->factor;
        while (sample_count--) {
            *pcm = (*pcm * factor) / BLT_GAIN_CONTROL_FILTER_FACTOR_RANGE;
            pcm++;
        }
    } else {
        register unsigned short factor = self->factor;
        while (sample_count--) {
            *pcm = (*pcm * BLT_GAIN_CONTROL_FILTER_FACTOR_RANGE)/factor;
            pcm++;
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GainControlFilterInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilterInput_QueryMediaType(BLT_MediaPort*         self,
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(GainControlFilterInput)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilterInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilterInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(GainControlFilterInput, BLT_PacketConsumer)
    GainControlFilterInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(GainControlFilterInput,
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(GainControlFilterInput, BLT_MediaPort)
    GainControlFilterInput_GetName,
    GainControlFilterInput_GetProtocol,
    GainControlFilterInput_GetDirection,
    GainControlFilterInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    GainControlFilterOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilterOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    GainControlFilter* self = ATX_SELF_M(output, GainControlFilter, BLT_PacketProducer);

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
|   GainControlFilterOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilterOutput_QueryMediaType(BLT_MediaPort*         self,
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(GainControlFilterOutput)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilterOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilterOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(GainControlFilterOutput,
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(GainControlFilterOutput, BLT_MediaPort)
    GainControlFilterOutput_GetName,
    GainControlFilterOutput_GetProtocol,
    GainControlFilterOutput_GetDirection,
    GainControlFilterOutput_QueryMediaType
ATX_END_INTERFACE_MAP


/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(GainControlFilterOutput, BLT_PacketProducer)
    GainControlFilterOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    GainControlFilter_Create
+---------------------------------------------------------------------*/
static BLT_Result
GainControlFilter_Create(BLT_Module*              module,
                         BLT_Core*                core, 
                         BLT_ModuleParametersType parameters_type,
                         BLT_AnyConst             parameters, 
                         BLT_MediaNode**          object)
{
    GainControlFilter* self;

    ATX_LOG_FINE("GainControlFilter::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(GainControlFilter));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, GainControlFilter, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(self, GainControlFilter, ATX_PropertyListener);
    ATX_SET_INTERFACE(&self->input,  GainControlFilterInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  GainControlFilterInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, GainControlFilterOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, GainControlFilterOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GainControlFilter_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
GainControlFilter_Destroy(GainControlFilter* self)
{ 
    ATX_LOG_FINE("GainControlFilter::Destroy");

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
|   GainControlFilter_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilter_GetPortByName(BLT_MediaNode*  _self,
                                BLT_CString     name,
                                BLT_MediaPort** port)
{
    GainControlFilter* self = ATX_SELF_EX(GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode);

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
|    GainControlFilter_DbToFactor
|
|    The input parameter is the gain expressed in 100th of decibels
+---------------------------------------------------------------------*/
static unsigned short
GainControlFilter_DbToFactor(int gain)
{
    double f = 
        (double)BLT_GAIN_CONTROL_FILTER_FACTOR_RANGE *
        pow(10.0, ((double)gain)/2000);
    unsigned int factor = (unsigned int)f; 
    if (f >= BLT_GAIN_CONTROL_FILTER_FACTOR_RANGE && 
        f <= BLT_GAIN_CONTROL_FILTER_MAX_FACTOR) {
        return (unsigned short)factor;    
    } else {
        return BLT_GAIN_CONTROL_FILTER_FACTOR_RANGE;
    }
}

/*----------------------------------------------------------------------
|    GainControlFilter_UpdateReplayGain
+---------------------------------------------------------------------*/
static void
GainControlFilter_UpdateReplayGain(GainControlFilter* self)
{
    int gain_value = 0;
    if (self->replay_gain_info.flags & BLT_GAIN_CONTROL_REPLAY_GAIN_ALBUM_VALUE_SET) {
        gain_value = self->replay_gain_info.album_gain;   
    } else if (self->replay_gain_info.flags & BLT_GAIN_CONTROL_REPLAY_GAIN_TRACK_VALUE_SET) {
        gain_value = self->replay_gain_info.track_gain;
    } else {
        gain_value = 0;
    }
    
    /* if the gain is 0, deactivate the filter */
    if (gain_value == 0) {
        /* disable the filter */
        if (self->factor != 0) {
            ATX_LOG_FINE("GainControlFilter::UpdateReplayGain - filter now inactive");
        }
        self->factor = 0;
        self->mode = BLT_GAIN_CONTROL_FILTER_MODE_INACTIVE;
        return;
    }

    /* convert the gain value into a mode and a factor */
    if (gain_value > 0) {
        self->mode = BLT_GAIN_CONTROL_FILTER_MODE_AMPLIFY;
        self->factor = GainControlFilter_DbToFactor(gain_value);
        ATX_LOG_FINE_1("GainControlFilter::UpdateReplayGain - filter amplification = %d", self->factor);
    } else {
        self->mode = BLT_GAIN_CONTROL_FILTER_MODE_ATTENUATE;
        self->factor = GainControlFilter_DbToFactor(-gain_value);
        ATX_LOG_FINE_1("GainControlFilter::UpdateReplayGain - filter attenuation = %d", self->factor);
    }
}

/*----------------------------------------------------------------------
|    GainControlFilter_UpdateReplayGainTrackValue
+---------------------------------------------------------------------*/
static void
GainControlFilter_UpdateReplayGainTrackValue(GainControlFilter*           self,
                                             const ATX_PropertyValueData* value)
{
    if (value) {
        self->replay_gain_info.track_gain = value->integer;
        self->replay_gain_info.flags |= BLT_GAIN_CONTROL_REPLAY_GAIN_TRACK_VALUE_SET;
    } else {
        self->replay_gain_info.track_gain = 0;
        self->replay_gain_info.flags &= ~BLT_GAIN_CONTROL_REPLAY_GAIN_TRACK_VALUE_SET;
    }
    GainControlFilter_UpdateReplayGain(self);
}

/*----------------------------------------------------------------------
|    GainControlFilter_UpdateReplayGainAlbumValue
+---------------------------------------------------------------------*/
static void
GainControlFilter_UpdateReplayGainAlbumValue(GainControlFilter*           self,
                                             const ATX_PropertyValueData* value)
{
    if (value) {
        self->replay_gain_info.album_gain = value->integer;
        self->replay_gain_info.flags |= BLT_GAIN_CONTROL_REPLAY_GAIN_ALBUM_VALUE_SET;
    } else {
        self->replay_gain_info.album_gain = 0;
        self->replay_gain_info.flags &= ~BLT_GAIN_CONTROL_REPLAY_GAIN_ALBUM_VALUE_SET;
    }
    GainControlFilter_UpdateReplayGain(self);
}

/*----------------------------------------------------------------------
|    GainControlFilter_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilter_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    GainControlFilter* self = ATX_SELF_EX(GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode);

    /* keep a reference to the stream */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    /* listen to settings on the new stream */
    if (stream) {
        ATX_Properties* properties;
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, 
                                                   &properties))) {
            ATX_PropertyValue property;
            ATX_Properties_AddListener(properties, 
                                       BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                                       &ATX_BASE(self, ATX_PropertyListener),
                                       &self->track_gain_listener_handle);
            ATX_Properties_AddListener(properties, 
                                       BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE,
                                       &ATX_BASE(self, ATX_PropertyListener),
                                       &self->album_gain_listener_handle);

            /* read the initial values of the replay gain info */
            self->replay_gain_info.flags = 0;
            self->replay_gain_info.track_gain = 0;
            self->replay_gain_info.album_gain = 0;

            if (ATX_SUCCEEDED(ATX_Properties_GetProperty(
                    properties,
                    BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                    &property)) &&
                property.type == ATX_PROPERTY_VALUE_TYPE_INTEGER) {
                GainControlFilter_UpdateReplayGainTrackValue(self, &property.data);
            }
            if (ATX_SUCCEEDED(ATX_Properties_GetProperty(
                    properties,
                    BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                    &property)) &&
                property.type == ATX_PROPERTY_VALUE_TYPE_INTEGER) {
                GainControlFilter_UpdateReplayGainAlbumValue(self, &property.data);
            }
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GainControlFilter_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilter_Deactivate(BLT_MediaNode* _self)
{
    GainControlFilter* self = ATX_SELF_EX(GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode);

    /* reset the replay gain info */
    self->replay_gain_info.flags      = 0;
    self->replay_gain_info.album_gain = 0;
    self->replay_gain_info.track_gain = 0;

    /* remove our listener */
    if (ATX_BASE(self, BLT_BaseMediaNode).context) {
        ATX_Properties* properties;
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, 
                                                   &properties))) {
            ATX_Properties_RemoveListener(properties, 
                                          &self->track_gain_listener_handle);
            ATX_Properties_RemoveListener(properties, 
                                          &self->album_gain_listener_handle);
        }
    }

    /* we're detached from the stream */
    ATX_BASE(self, BLT_BaseMediaNode).context = NULL;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GainControlFilter_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilter_Seek(BLT_MediaNode* _self,
                       BLT_SeekMode*  mode,
                       BLT_SeekPoint* point)
{
    GainControlFilter* self = ATX_SELF_EX(GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode);

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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(GainControlFilter)
    ATX_GET_INTERFACE_ACCEPT_EX(GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(GainControlFilter, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilter, ATX_PropertyListener)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(GainControlFilter, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    GainControlFilter_GetPortByName,
    GainControlFilter_Activate,
    GainControlFilter_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    GainControlFilter_Seek
};

/*----------------------------------------------------------------------
|    GainControlFilter_OnPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
GainControlFilter_OnPropertyChanged(ATX_PropertyListener*    _self,
                                    ATX_CString              name,
                                    const ATX_PropertyValue* value)
{
    GainControlFilter* self = ATX_SELF(GainControlFilter, ATX_PropertyListener);

    if (name && 
        (value == NULL || value->type == ATX_PROPERTY_VALUE_TYPE_INTEGER)) {
        if (ATX_StringsEqual(name, BLT_REPLAY_GAIN_TRACK_GAIN_VALUE)) {
            GainControlFilter_UpdateReplayGainTrackValue(self, value?&value->data:NULL);
        } else if (ATX_StringsEqual(name, BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE)) {
            GainControlFilter_UpdateReplayGainAlbumValue(self, value?&value->data:NULL);
        }
    }
}

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(GainControlFilter, ATX_PropertyListener)
    GainControlFilter_OnPropertyChanged,
};

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(GainControlFilter, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   GainControlFilterModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
GainControlFilterModule_Probe(BLT_Module*              self,  
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
                !ATX_StringsEqual(constructor->name, BLT_GAIN_CONTROL_FILTER_MODULE_NAME)) {
                return BLT_FAILURE;
            }

            /* the input and output protocols should be PACKET */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be unspecified, or audio/pcm */
            if (!(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or audio/pcm */
            if (!(constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* match level is always exact */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            ATX_LOG_FINE_1("GainControlFilterModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(GainControlFilterModule)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilterModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT(GainControlFilterModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(GainControlFilterModule, GainControlFilter)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(GainControlFilterModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    GainControlFilterModule_CreateInstance,
    GainControlFilterModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define GainControlFilterModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(GainControlFilterModule, reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(GainControlFilterModule,
                                         "Gain Control Filter",
                                         BLT_GAIN_CONTROL_FILTER_MODULE_NAME,
                                         "1.3.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
