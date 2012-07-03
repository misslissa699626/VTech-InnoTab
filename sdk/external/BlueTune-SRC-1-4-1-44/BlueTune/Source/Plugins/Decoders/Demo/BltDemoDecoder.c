
/*****************************************************************
|
|   BlueTune - Demo Decoder Module
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
#include "BltDemoDecoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

#include <time.h>

#if 0
#define DEBUG0 printf
#else
#define DEBUG0(...)
#endif

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.demo")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
	/* base class */
	ATX_EXTENDS(BLT_BaseModule);

	/* members */
	BLT_UInt32 demo_type_id;
} DemoDecoderModule;

typedef struct {
	/* interfaces */
	ATX_IMPLEMENTS(BLT_MediaPort);
	ATX_IMPLEMENTS(BLT_PacketConsumer);

	/* members */
	BLT_Boolean eos;
	BLT_Boolean is_continous_stream;
	ATX_List*   packets;
} DemoDecoderInput;

typedef struct {
	/* interfaces */
	ATX_IMPLEMENTS(BLT_MediaPort);
	ATX_IMPLEMENTS(BLT_PacketProducer);

	/* members */
	BLT_Boolean      eos;
	BLT_PcmMediaType media_type;
	BLT_TimeStamp    time_stamp;
	ATX_Int64        sample_count;
} DemoDecoderOutput;

typedef struct {
	/* base class */
	ATX_EXTENDS(BLT_BaseMediaNode);

	/* members */
	BLT_Module             module;
	DemoDecoderInput  input;
	DemoDecoderOutput output;
 
} DemoDecoder;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

#define DEMO_ERROR_NO_MORE_SAMPLES (-100)

#define DEMO_SAMPLE_COUNT 100
#define DEMO_CHANNEL_COUNT 2
#define DEMO_SAMPLE_RATE 16

static BLT_UInt8 demo_data[DEMO_SAMPLE_COUNT*DEMO_CHANNEL_COUNT*2] = {0x55, 0xaa, 0x55, 0xaa};
static BLT_UInt32 demo_run_loop = 0;
static BLT_UInt64 start_time = 0;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(DemoDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(DemoDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(DemoDecoder, ATX_Referenceable)



#define GP_EPOCH 1156000000
static unsigned long getRawTime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	return ((tv.tv_sec- GP_EPOCH)*1000+tv.tv_usec/1000);
}

/*----------------------------------------------------------------------
|   DemoDecoderInput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
DemoDecoderInput_Flush(DemoDecoder* self)
{
	ATX_ListItem* item;
	DEBUG0("DemoDecoderInput_Flush:Enter \n");
	while ((item = ATX_List_GetFirstItem(self->input.packets))) {
		BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
		if (packet) BLT_MediaPacket_Release(packet);
		ATX_List_RemoveItem(self->input.packets, item);
	}

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DemoDecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
DemoDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                                BLT_MediaPacket*    packet)
{
	DemoDecoder* self = ATX_SELF_M(input, DemoDecoder, BLT_PacketConsumer);
	ATX_Result        result;

	DEBUG0("DemoDecoderInput_PutPacket:Enter \n");

	/* check to see if this is the end of a stream */
	if (BLT_MediaPacket_GetFlags(packet) & 
		BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
		self->input.eos = BLT_TRUE;
	}

	/* add the packet to the input list */
	result = ATX_List_AddData(self->input.packets, packet);
	if (ATX_SUCCEEDED(result)) {
		BLT_MediaPacket_AddReference(packet);
	}

	return result;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DemoDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(DemoDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(DemoDecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(DemoDecoderInput, BLT_PacketConsumer)
    DemoDecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(DemoDecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(DemoDecoderInput, BLT_MediaPort)
    DemoDecoderInput_GetName,
    DemoDecoderInput_GetProtocol,
    DemoDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   DemoDecoder_UpdateInfo
+---------------------------------------------------------------------*/
static BLT_Result
DemoDecoder_UpdateInfo(DemoDecoder* self,     
                            void*    frame_info)
{
	/* update the stream info */
	DEBUG0("DemoDecoder_UpdateInfo:Enter \n");
	self->output.sample_count = DEMO_SAMPLE_COUNT;
	if (ATX_BASE(self, BLT_BaseMediaNode).context) {
		BLT_StreamInfo info;
		/* start with no info */
		info.mask = 0;

		/* sample rate */
		info.sample_rate = DEMO_SAMPLE_RATE;
		info.mask |= BLT_STREAM_INFO_MASK_SAMPLE_RATE;

		/* channel count */
		info.channel_count = DEMO_CHANNEL_COUNT;
		info.mask |= BLT_STREAM_INFO_MASK_CHANNEL_COUNT;

		/* data type */
		info.data_type = "DEMO";
		info.mask |= BLT_STREAM_INFO_MASK_DATA_TYPE;

		/*TODO: update bitrate & duration etc...*/

		/* send update */
		BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
	}
	
	return BLT_SUCCESS;
	
}


/*----------------------------------------------------------------------
|   DemoDecoder_DecodeFrame
+---------------------------------------------------------------------*/
static BLT_Result
DemoDecoder_DecodeFrame(DemoDecoder* self,
                             BLT_MediaPacket** packet)
{
	BLT_Size buffer_szie;
	BLT_Result         result;
	BLT_Any buffer;
	BLT_UInt64 duration = 0;

	/* setup default return value */
	*packet = NULL;

	DEBUG0("DemoDecoder_DecodeFrame:Enter \n");

	/* update the stream info */
	result = DemoDecoder_UpdateInfo(self, NULL);
	if (BLT_FAILED(result)) return result;

	/* get a packet from the core */
	buffer_szie  = DEMO_SAMPLE_COUNT*DEMO_CHANNEL_COUNT*2;

	result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
	                    buffer_szie,
	                    (const BLT_MediaType*)&self->output.media_type,
	                    packet);
	if (BLT_FAILED(result)) return result;

	/* get the address of the packet payload */
	buffer = BLT_MediaPacket_GetPayloadBuffer(*packet);

	ATX_CopyMemory(buffer, demo_data, buffer_szie);

	/* set the packet payload size */
	BLT_MediaPacket_SetPayloadSize(*packet, buffer_szie);


	/* update the sample count */
	self->output.sample_count = demo_run_loop;

	/* set start of stream packet flags */
	{
		if (self->output.sample_count == 0) {
			BLT_MediaPacket_SetFlags(*packet, 
			                 BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);

			start_time = getRawTime();
		}
	}

	/* update the timestamp  */
	duration = getRawTime() - start_time;
	self->output.time_stamp = BLT_TimeStamp_FromMillis(duration);
	BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);

	/*check end*/
	if(demo_run_loop++ > 1000) {
		/* release the packet */
		BLT_MediaPacket_Release(*packet);
		*packet = NULL;
		return DEMO_ERROR_NO_MORE_SAMPLES;
	}
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DemoDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
DemoDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                                 BLT_MediaPacket**   packet)
{
	DemoDecoder* self = ATX_SELF_M(output, DemoDecoder, BLT_PacketProducer);
	BLT_Boolean       try_again;
	BLT_Result        result;

	/* default return */
	*packet = NULL;

	DEBUG0("DemoDecoderOutput_GetPacket:Enter \n");

	/* check for EOS */
	if (self->output.eos) {
		return BLT_ERROR_EOS;
	}

	do {
		/* try to decode a frame */
		result = DemoDecoder_DecodeFrame(self, packet);
		if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;

		self->output.eos = BLT_TRUE;

		try_again = BLT_FALSE;

	} while (try_again);

	/* if we've reached the end of stream, generate an empty packet with */
	/* a flag to indicate that situation                                 */
	if (self->input.eos) {
		result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
		                                    0,
		                                    (const BLT_MediaType*)&self->output.media_type,
		                                    packet);
		if (BLT_FAILED(result)) return result;
		BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
		BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
		self->output.eos = BLT_TRUE;
		return BLT_SUCCESS;
	}

	return BLT_ERROR_PORT_HAS_NO_DATA;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DemoDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(DemoDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(DemoDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(DemoDecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(DemoDecoderOutput, BLT_MediaPort)
    DemoDecoderOutput_GetName,
    DemoDecoderOutput_GetProtocol,
    DemoDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(DemoDecoderOutput, BLT_PacketProducer)
    DemoDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   DemoDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
DemoDecoder_SetupPorts(DemoDecoder* self)
{
	ATX_Result result;

	DEBUG0("DemoDecoder_SetupPorts:Enter \n");
	/* init the input port */
	self->input.eos = BLT_FALSE;

	/* create a list of input packets */
	result = ATX_List_Create(&self->input.packets);
	if (ATX_FAILED(result)) return result;

	/* setup the output port */
	self->output.eos = BLT_FALSE;
	BLT_PcmMediaType_Init(&self->output.media_type);
	self->output.sample_count = 0;
	BLT_TimeStamp_Set(self->output.time_stamp, 0, 0);

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    DemoDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
DemoDecoder_Create(BLT_Module*              module,
                        BLT_Core*                core, 
                        BLT_ModuleParametersType parameters_type,
                        BLT_CString              parameters, 
                        BLT_MediaNode**          object)
{
	DemoDecoder* self;
	BLT_Result        result;

	ATX_LOG_FINE("DemoDecoder::Create");
	DEBUG0("DemoDecoder_Create:Enter \n");
	/* check parameters */
	if (parameters == NULL || 
		parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
		return BLT_ERROR_INVALID_PARAMETERS;
	}

	/* allocate memory for the object */
	self = ATX_AllocateZeroMemory(sizeof(DemoDecoder));
	if (self == NULL) {
		*object = NULL;
		return BLT_ERROR_OUT_OF_MEMORY;
	}

	/* construct the inherited object */
	BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

	/* create the your decoder if needed*/

	/* setup the input and output ports */
	result = DemoDecoder_SetupPorts(self);
	if (BLT_FAILED(result)) {
		ATX_FreeMemory(self);
		*object = NULL;
		return result;
	}

	/* setup interfaces */
	ATX_SET_INTERFACE_EX(self, DemoDecoder, BLT_BaseMediaNode, BLT_MediaNode);
	ATX_SET_INTERFACE_EX(self, DemoDecoder, BLT_BaseMediaNode, ATX_Referenceable);
	ATX_SET_INTERFACE(&self->input,  DemoDecoderInput,  BLT_MediaPort);
	ATX_SET_INTERFACE(&self->input,  DemoDecoderInput,  BLT_PacketConsumer);
	ATX_SET_INTERFACE(&self->output, DemoDecoderOutput, BLT_MediaPort);
	ATX_SET_INTERFACE(&self->output, DemoDecoderOutput, BLT_PacketProducer);
	*object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

	demo_run_loop = 0;

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    DemoDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
DemoDecoder_Destroy(DemoDecoder* self)
{ 
	ATX_ListItem* item;

	ATX_LOG_FINE("DemoDecoder::Destroy");
	DEBUG0("DemoDecoder_Destroy:Enter \n");
	/* release any packet we may hold */
	item = ATX_List_GetFirstItem(self->input.packets);
	while (item) {
		BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
		if (packet) {
		    BLT_MediaPacket_Release(packet);
		}
		item = ATX_ListItem_GetNext(item);
	}
	ATX_List_Destroy(self->input.packets);

	/* destruct the inherited object */
	BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

	/* free the object memory */
	ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   DemoDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
DemoDecoder_GetPortByName(BLT_MediaNode*  _self,
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
	DemoDecoder* self = ATX_SELF_EX(DemoDecoder, BLT_BaseMediaNode, BLT_MediaNode);

	DEBUG0("DemoDecoder_GetPortByName:Enter \n");
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
|    DemoDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
DemoDecoder_Seek(BLT_MediaNode* _self,
                      BLT_SeekMode*  mode,
                      BLT_SeekPoint* point)
{
	DemoDecoder* self = ATX_SELF_EX(DemoDecoder, BLT_BaseMediaNode, BLT_MediaNode);

	DEBUG0("DemoDecoder_Seek:Enter \n");
	/* flush pending input packets */
	DemoDecoderInput_Flush(self);

	/* clear the eos flag */
	self->input.eos  = BLT_FALSE;
	self->output.eos = BLT_FALSE;

	/*TODO: flush and reset the decoder */

	/* estimate the seek point in time_stamp mode */
	if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
	BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
	if (!(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
	    return BLT_FAILURE;
	}

	/* update the decoder's sample position */
	self->output.sample_count = point->sample;
	self->output.time_stamp = point->time_stamp;
	/* Demo_Decoder_SetSample(self->fluo, point->sample);*/

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DemoDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(DemoDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(DemoDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(DemoDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    DemoDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    DemoDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(DemoDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   DemoDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
DemoDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
	DemoDecoderModule* self = ATX_SELF_EX(DemoDecoderModule, BLT_BaseModule, BLT_Module);
	BLT_Registry*           registry;
	BLT_Result              result;

	DEBUG0("DemoDecoderModule_Attach:Enter \n");

	/* get the registry */
	result = BLT_Core_GetRegistry(core, &registry);
	if (BLT_FAILED(result)) return result;

	/* register the .mp2, .mp1, .mp3 .mpa and .mpg file extensions */
	result = BLT_Registry_RegisterExtension(registry, 
	                                    ".demo",
	                                    "audio/demo");
	if (BLT_FAILED(result)) return result;

	/* register the "audio/demo" type */
	result = BLT_Registry_RegisterName(registry,
				BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
				"audio/demo",
				&self->demo_type_id);
	if (BLT_FAILED(result)) return result;

	/* register mime type aliases */
	BLT_Registry_RegisterNameForId(registry, 
	                           BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
	                           "audio/demo", self->demo_type_id);
	BLT_Registry_RegisterNameForId(registry, 
	                           BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
	                           "audio/x-demo", self->demo_type_id);

	ATX_LOG_FINE_1("DemoDecoderModule::Attach (audio/demo type = %d)", self->demo_type_id);

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   DemoDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
DemoDecoderModule_Probe(BLT_Module*              _self, 
                             BLT_Core*                core,
                             BLT_ModuleParametersType parameters_type,
                             BLT_AnyConst             parameters,
                             BLT_Cardinal*            match)
{
    DemoDecoderModule* self = ATX_SELF_EX(DemoDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
    
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

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

            /* the input type should be audio/demo */
            if (constructor->spec.input.media_type->id != 
                self->demo_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or audio/pcm */
            if (!(constructor->spec.output.media_type->id == 
                  BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.output.media_type->id ==
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "DemoDecoder")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }
		DEBUG0("DemoDecoder: Probe  ok \n");
            ATX_LOG_FINE_1("DemoDecoderModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(DemoDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(DemoDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(DemoDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(DemoDecoderModule, DemoDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(DemoDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    DemoDecoderModule_Attach,
    DemoDecoderModule_CreateInstance,
    DemoDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define DemoDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(DemoDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(DemoDecoderModule,
                                         "Demo Decoder",
                                         "com.axiosys.decoder.demo",
                                         "0.0.1",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)

