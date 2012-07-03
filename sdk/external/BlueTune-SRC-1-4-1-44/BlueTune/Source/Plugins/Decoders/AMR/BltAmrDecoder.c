
/*****************************************************************
|
|   BlueTune - Amr Decoder Module
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
#include "BltAmrDecoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"

#include "gp_avcodec.h"
#include "auddec.h"
#include "gp_acodec.h"
#include <dlfcn.h>

#include <unistd.h>

#if 0
#define DEBUG0 printf
#else
#define DEBUG0(...)
#endif

#if 0

#if 0
#define DEBUG0(args...) fprintf(stderr, args...)
#else
#define DEBUG0(...)
#endif

#endif

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.amr")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
	/* base class */
	ATX_EXTENDS(BLT_BaseModule);

	/* members */
	BLT_UInt32 amr_type_id;
} AmrDecoderModule;

typedef struct {
	/* interfaces */
	ATX_IMPLEMENTS(BLT_MediaPort);
	ATX_IMPLEMENTS(BLT_PacketConsumer);

	 /* members */
    BLT_Boolean eos;
	ATX_List* 	packets;
} AmrDecoderInput;

typedef struct {
	/* interfaces */
	ATX_IMPLEMENTS(BLT_MediaPort);
	ATX_IMPLEMENTS(BLT_PacketProducer);

	/* members */
	BLT_Boolean 	eos;
	BLT_PcmMediaType media_type;
	BLT_TimeStamp    time_stamp;
} AmrDecoderOutput;

typedef struct {
	/* base class */
	ATX_EXTENDS(BLT_BaseMediaNode);

	/* members */
	BLT_UInt32       amr_type_id;
	AmrDecoderInput  input;
	AmrDecoderOutput output;
 
} AmrDecoder;


typedef struct {
	BLT_MediaType   base;
	BLT_UInt32		CodecID;
  	BLT_Int32		nChannels;
	BLT_Int32		nSamplesRate;
	BLT_Int32		nBitRate;
}BLT_AmrAudioMediaType;




/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

#define AMR_ERROR_NO_MORE_SAMPLES (-100)

#define AMR_SAMPLE_COUNT 100
#define AMR_CHANNEL_COUNT 2
#define AMR_SAMPLE_RATE 16

#define RING_ALLOCATE_SIZE 	256 * 1024

static const  audec_interface_t*	audec = NULL;
static audec_param_t 				adp;
static BLT_UInt32 					seekable = 0;
static ATX_UInt8 					es_buf [RING_ALLOCATE_SIZE];
static ATX_Int32					RefillSize = 0;
static ATX_UInt32					ringCnt = 0;
static ATX_Int32					first_init = 1;
static const ATX_Any 				*hlib = NULL;
static BLT_UInt64 					pcm_size = 0;
static BLT_UInt32					out_samRate = 0;




/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AmrDecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(AmrDecoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AmrDecoder, ATX_Referenceable)



struct BLT_MediaPacket {
    BLT_Cardinal   reference_count;
    BLT_MediaType* type;
    BLT_Size       allocated_size;
    BLT_Size       payload_size;
    BLT_Offset     payload_offset;
    BLT_Any        payload;
    BLT_Flags      flags;
    BLT_TimeStamp  time_stamp;
    BLT_Time       duration;
};

/*----------------------------------------------------------------------
|   AmrDecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AmrDecoderInput_PutPacket(BLT_PacketConsumer* _self,
                                BLT_MediaPacket*    packet)
{
	DEBUG0("**** into AmrDecoderInput_PutPacket *****\n");

	AmrDecoder* self = ATX_SELF_M(input, AmrDecoder, BLT_PacketConsumer);
	ATX_Result result;

	/* check to see if this is the end of a stream */
	if (BLT_MediaPacket_GetFlags(packet) & 
		BLT_MEDIA_PACKET_FLAG_END_OF_STREAM) {
		DEBUG0("*** at end of file in AmrDecoderInput_PutPacket******\n");
		self->input.eos = BLT_TRUE;
	}

    /* add the packet to the input list */
    result = ATX_List_AddData(self->input.packets, packet);
    if (ATX_SUCCEEDED(result)) {
        BLT_MediaPacket_AddReference(packet);
    }
	DEBUG0("*** return from AmrDecoderInput_PutPacket ******\n");
    return result;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrDecoderInput)
    ATX_GET_INTERFACE_ACCEPT(AmrDecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AmrDecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AmrDecoderInput, BLT_PacketConsumer)
    AmrDecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AmrDecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(AmrDecoderInput, BLT_MediaPort)
    AmrDecoderInput_GetName,
    AmrDecoderInput_GetProtocol,
    AmrDecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AmrDecoder_UpdateInfo
+---------------------------------------------------------------------*/
static BLT_Result
AmrDecoder_UpdateInfo(AmrDecoder* self)
{
	ATX_ListItem* item;
    while ((item = ATX_List_GetFirstItem(self->input.packets))) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) BLT_MediaPacket_Release(packet);
        ATX_List_RemoveItem(self->input.packets, item);
    }
	
	return BLT_SUCCESS;
	
}


/*----------------------------------------------------------------------
|   AmrDecoderOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AmrDecoderOutput_GetPacket(BLT_PacketProducer* _self,
                                 BLT_MediaPacket**   packet)
{
	AmrDecoder*   self = ATX_SELF_M(output, AmrDecoder, BLT_PacketProducer);
	
	ATX_Result        	result;
	const ATX_UInt8*	packet_ptr = NULL;
	ATX_UInt32 			packet_size = 0;
	ATX_UInt16 			wBitsPerSample = 0;
	ATX_Int32 			out_size = RING_ALLOCATE_SIZE;
	ATX_UInt8 			pcm_buf[RING_ALLOCATE_SIZE];
	ATX_ListItem* 		packet_item;
	BLT_MediaPacket* 	input_packet = NULL;
	
	
	ATX_SetMemory(pcm_buf, 0, RING_ALLOCATE_SIZE);
	DEBUG0("**** into AmrDecoderOutput_GetPacket *****\n");

	/* check for EOS */
	if (self->output.eos) {
		return BLT_ERROR_EOS;
	}

	/* check if we have a packet available */
	packet_item = ATX_List_GetFirstItem(self->input.packets);
	if (packet_item) {
       	input_packet = (BLT_MediaPacket*)ATX_ListItem_GetData(packet_item);
       	//ATX_List_RemoveItem(self->input.packets, packet_item);
    }
	if ((self->input.eos == BLT_TRUE) && (ATX_List_GetItemCount(self->input.packets) == 1)){
	
		/*直接去解码，如果解码失败就返回no data*/	
		DEBUG0("############# at end of file  ######\n");
		DEBUG0("adp.RingWI = %u\n", adp.RingWI);
		DEBUG0("adp.RingRI = %u\n", adp.RingRI);
		DEBUG0("ringCnt = %u\n", ringCnt);
		result = audec->dec(&adp, pcm_buf, &out_size);
		DEBUG0("result = %u\n", result);
		if (result <= 0) {
			/*如果解码失败，直接返回*/
			DEBUG0(" decoder fail result = %d\n", result);

			pcm_size = 0;
			
			return result;
		}else{
			ringCnt -= result;
			*packet = NULL;
			DEBUG0("out_size = %u\n", out_size);
			/* create a PCM packet for the output */
			result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
														out_size,
														(BLT_MediaType*)&self->output.media_type,
														packet);
			if (BLT_FAILED(result)){
				return result;
			} 

			/*set payload size*/
			BLT_MediaPacket_SetPayloadSize(*packet, out_size);
			BLT_MediaPacket_SetAllocatedSize(*packet, out_size);

			pcm_size += out_size;
			self->output.time_stamp = BLT_TimeStamp_FromSeconds((double)pcm_size / (double)adp.out_sample_rate);
			
			/* copy the timestamp */
       		BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
			
			/*copy pcm data to out_packet*/
			DEBUG0("***********copy pcm data to out_packet**********\n" );
			ATX_CopyMemory((BLT_UInt8*)BLT_MediaPacket_GetPayloadBuffer(*packet), pcm_buf, out_size);
			
			return BLT_SUCCESS;
		}
	}else{
		if(input_packet == NULL && self->input.eos != BLT_TRUE){
			DEBUG0("############# at decoder output no data packet  ######\n");
			return BLT_ERROR_PORT_HAS_NO_DATA;	
		}else{
			DEBUG0("############# have data pcaket  ######\n");
			packet_ptr = BLT_MediaPacket_GetPayloadBuffer(input_packet);
			packet_size = BLT_MediaPacket_GetPayloadSize(input_packet);
			wBitsPerSample = 0;

			if (first_init || seekable) {
				const BLT_MediaType*			media_type;
				const BLT_AmrAudioMediaType* 	amr_media_type;
				const BLT_UInt8*				libName = NULL;
				RefillSize = 0;
				ringCnt = 0;
				ATX_SetMemory(es_buf, 0, RING_ALLOCATE_SIZE);
				DEBUG0("****************1111**************\n" );

        		BLT_MediaPacket_GetMediaType(input_packet, &media_type);
        		if (media_type == NULL || media_type->id != self->amr_type_id) {
           			 return BLT_ERROR_INVALID_MEDIA_TYPE;
        		}
				amr_media_type = (const BLT_AmrAudioMediaType*)media_type;

				if(amr_media_type->CodecID == CODEC_ID_AMR_NB){
					libName = "/system/lib/libamrnbdec.so";
				}
				if(amr_media_type->CodecID == CODEC_ID_AMR_WB){
					libName = "/system/lib/libamrwbdec.so";
				}

				DEBUG0("****************2222**************\n" );	
				if((hlib = dlopen(libName, RTLD_NOW))== NULL){
					perror("dlopen");
					return BLT_FAILURE;
				}
				audec = (audec_interface_t *)dlsym(hlib, "gp_aud_dec_api");
				if(audec == NULL){
					perror("dlsym");
					return BLT_FAILURE;
				}
				ATX_SetMemory(es_buf, 0, RING_ALLOCATE_SIZE);
				ATX_CopyMemory(es_buf, (BLT_UInt8 *)packet_ptr, packet_size);

				adp.RingWI = packet_size;
				adp.sample_rate = amr_media_type->nSamplesRate;
				adp.channels = amr_media_type->nChannels;	
				adp.Ring = es_buf; 
				adp.RingSize = RING_ALLOCATE_SIZE;
				adp.RingRI = 0;
				adp.hDecoder = ATX_AllocateZeroMemory(audec->instance_size());
				
				if(adp.hDecoder == NULL || adp.Ring == NULL){
					DEBUG0("adp.hDecoder is NULL\n");
					return BLT_FAILURE;
				}

				DEBUG0("&&&&&&&&&&&&&& check decoder init argument &&&&&&&&&&&\n");
				DEBUG0("adp.RingWI = %u\n", adp.RingWI);
				DEBUG0("adp.sample_rate = %u\n", adp.sample_rate);
				DEBUG0("adp.channels = %u\n", adp.channels);
		
				
				DEBUG0("\n\n\n");
				int i =0;
				for(i=0;i<packet_size;i++){
				if(i%16==0 && i!=0){
					DEBUG0("\n");
				}
					DEBUG0("%02x ", es_buf[i]);
				}
				DEBUG0("\n");
								
				DEBUG0("$$$$$$$$$$$$$ end check argument $$$$$$$$$$$$$$$$$$$$$$$\n");

	
				
				ringCnt += packet_size; 
				
				/*init decoder*/
				result = audec->init(&adp);
				DEBUG0("result = %d\n", result);
				if (result < 0) {
					DEBUG0("init decoder fail\n");
	
					return BLT_FAILURE;
				} 
				ATX_List_RemoveItem(self->input.packets, packet_item);
				out_samRate = adp.out_sample_rate;
				DEBUG0("**** init decoder success****\n");

			}/*end of if(!flag)*/

			/*decoder*/
			if(!first_init){ 	
				DEBUG0("****************4444**************\n" );
				RefillSize = adp.RingSize - ringCnt - 1;

				DEBUG0("RefillSize = %d\n", RefillSize);
				DEBUG0("adp.RingSize = %d\n", adp.RingSize);
				DEBUG0("ringCnt = %d\n", ringCnt);
				DEBUG0("packet_size = %d\n", packet_size);
		
				if(RefillSize < packet_size ) {
					DEBUG0("****************4444---1**************\n" );
					goto  AMR_DEC;
				}else{
					RefillSize = packet_size;
					ATX_List_RemoveItem(self->input.packets, packet_item);
				}
				DEBUG0("****************adp.RingWI = %d**************\n", adp.RingWI );
				DEBUG0("****adp.RingWI + RefillSize = %d****\n", adp.RingWI + RefillSize );
		
				if((adp.RingWI + RefillSize) >= (adp.RingSize)){
					ATX_Int32 cnt = adp.RingSize - adp.RingWI;
					ATX_CopyMemory(adp.Ring + adp.RingWI, packet_ptr, cnt);
					packet_ptr += cnt;
					adp.RingWI = 0;
					RefillSize -= cnt;
					ringCnt += cnt;
				}
				if(RefillSize){
					ATX_CopyMemory(adp.Ring + adp.RingWI, packet_ptr, RefillSize);
					adp.RingWI += RefillSize;
					ringCnt += RefillSize;
					RefillSize = 0;
					packet_ptr = NULL;
				}
			}
	AMR_DEC:
		DEBUG0("****************666666666**************\n" ); 
		
			result = audec->dec(&adp, pcm_buf, &out_size);

			DEBUG0("&&&&&& result = %d &&&&&&\n", result);
			//while(1);
			if (result < 0) {
				DEBUG0("first decoder fail\n");
				DEBUG0("\n ############## audec.dec fail ###############\n");
				return BLT_FAILURE;
			}
			#if 0
			int i=0;
			for(i=0;i<out_size ;i++){
				if(i%16==0 && i!=0){
					DEBUG0("\n");
				}
					DEBUG0("%02x ", pcm_buf[i]);
				}
				DEBUG0("\n\n\n");
			#endif
			ringCnt -= result;
			*packet = NULL;
			DEBUG0("****************77777**************\n" ); 
			DEBUG0("*****out_size = %u******\n", out_size ); 
			/* create a PCM packet for the output */
			result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
														out_size,
														(BLT_MediaType*)&self->output.media_type,
														packet);
			if (BLT_FAILED(result)){
				DEBUG0("****************create media packet fail**************\n" );
				return result;
			} 
			
			
			/*compute time stamp*/
			pcm_size += out_size;
			
			DEBUG0("pcm_size = %llu\n", pcm_size);
			DEBUG0("adp.out_sample_rate = %d\n", adp.out_sample_rate);


			self->output.time_stamp = BLT_TimeStamp_FromSeconds((double)pcm_size / (double)adp.out_sample_rate);
	
			/* copy the timestamp */
       		BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
			DEBUG0("self->output.time_stamp.seconds = %d\n", self->output.time_stamp.seconds);
			DEBUG0("self->output.time_stamp.nanoseconds = %d\n", self->output.time_stamp.nanoseconds);
			DEBUG0("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^time stamp^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
			
			

			/*set payload size*/
			BLT_MediaPacket_SetPayloadSize(*packet, out_size);
			BLT_MediaPacket_SetAllocatedSize(*packet, out_size);
			ATX_CopyMemory((BLT_UInt8*)BLT_MediaPacket_GetPayloadBuffer(*packet), pcm_buf, out_size);
			DEBUG0("****************88888**************\n" ); 
				
			if(self->output.media_type.channel_count == 0 || seekable){
				DEBUG0("****************100001**************\n" ); 
				/* first time, setup our media type */
				self->output.media_type.channel_count	= adp.out_channels;
				self->output.media_type.sample_rate 	= adp.out_sample_rate;
				self->output.media_type.bits_per_sample = 16; //wBitsPerSample; 
				self->output.media_type.sample_format	= BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
				self->output.media_type.channel_mask	= 0;
										
				/* update the packet media type */
				BLT_MediaPacket_SetMediaType(*packet, (BLT_MediaType*)&self->output.media_type);
				DEBUG0("(*packet)->type->id = %d\n", (*packet)->type->id);
				//while(1);
				DEBUG0("****************100003**************\n" );
				first_init = 0;
				seekable = 0;
			}
			return BLT_SUCCESS;
		}

	}
	//return BLT_ERROR_PORT_HAS_NO_DATA;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrDecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(AmrDecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AmrDecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AmrDecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AmrDecoderOutput, BLT_MediaPort)
    AmrDecoderOutput_GetName,
    AmrDecoderOutput_GetProtocol,
    AmrDecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AmrDecoderOutput, BLT_PacketProducer)
    AmrDecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AmrDecoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
AmrDecoder_SetupPorts(AmrDecoder* self, BLT_MediaTypeId amr_type_id)
{
    ATX_Result result;

    /* init the input port */
    self->amr_type_id = amr_type_id;
    self->input.eos = BLT_FALSE;

    /* create a list of input packets */
    result = ATX_List_Create(&self->input.packets);
    if (ATX_FAILED(result)) return result;
    
    /* setup the output port */
    BLT_PcmMediaType_Init(&self->output.media_type);

    return BLT_SUCCESS;
}



/*----------------------------------------------------------------------
|    AmrDecoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
AmrDecoder_Create(BLT_Module*              module,
                        BLT_Core*                core, 
                        BLT_ModuleParametersType parameters_type,
                        BLT_CString              parameters, 
                        BLT_MediaNode**          object)
{

	
	AmrDecoder* self;
	AmrDecoderModule* amr_decoder_module = (AmrDecoderModule*)module;
	BLT_Result        result;

	ATX_LOG_FINE("AmrDecoder::Create");

	/* check parameters */
	if (parameters == NULL || 
		parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
		return BLT_ERROR_INVALID_PARAMETERS;
	}

	/* allocate memory for the object */
	self = ATX_AllocateZeroMemory(sizeof(AmrDecoder));
	if (self == NULL) {
		*object = NULL;
		return BLT_ERROR_OUT_OF_MEMORY;
	}


	/* construct the inherited object */
	BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

	/* setup the input and output ports */
	result = AmrDecoder_SetupPorts(self, amr_decoder_module->amr_type_id);
	if (BLT_FAILED(result)) {
		ATX_FreeMemory(self);
		*object = NULL;
		return result;
	}

	/* setup interfaces */
	ATX_SET_INTERFACE_EX(self, AmrDecoder, BLT_BaseMediaNode, BLT_MediaNode);
	ATX_SET_INTERFACE_EX(self, AmrDecoder, BLT_BaseMediaNode, ATX_Referenceable);
	ATX_SET_INTERFACE(&self->input,  AmrDecoderInput,  BLT_MediaPort);
	ATX_SET_INTERFACE(&self->input,  AmrDecoderInput,  BLT_PacketConsumer);
	ATX_SET_INTERFACE(&self->output, AmrDecoderOutput, BLT_MediaPort);
	ATX_SET_INTERFACE(&self->output, AmrDecoderOutput, BLT_PacketProducer);
	*object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);


	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AmrDecoderInput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
AmrDecoderInput_Flush(AmrDecoder* self)
{
    ATX_ListItem* item;
    while ((item = ATX_List_GetFirstItem(self->input.packets))) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) BLT_MediaPacket_Release(packet);
        ATX_List_RemoveItem(self->input.packets, item);
    }

    return BLT_SUCCESS;
}


/*----------------------------------------------------------------------
|    AmrDecoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AmrDecoder_Destroy(AmrDecoder* self)
{ 
	
	ATX_LOG_FINE("AacDecoder::Destroy");
	BLT_Result result;
	
	   /* release any packet we may hold */
	   AmrDecoderInput_Flush(self);
	   ATX_List_Destroy(self->input.packets);
	   DEBUG0("***into AmrDecoder_Destroy*****\n");

	   result = audec->uninit(&adp);
	   if(BLT_FAILED(result)){
			return BLT_FAILURE;
	   }
	   if(hlib != NULL){
			dlclose(hlib);
	   }
	   
	   if(adp.hDecoder != NULL){
			ATX_FreeMemory(adp.hDecoder);
			adp.hDecoder = NULL;
		}

	   if(audec != NULL){
		   audec = NULL;
	   }
	
	   if(hlib != NULL){
			dlclose(hlib);
	   }

	   /* destroy the decoder */
	   DEBUG0("***  aaaaa *****\n");
	   seekable = 0;
	   ATX_SetMemory(es_buf, 0, RING_ALLOCATE_SIZE);
	   RefillSize = 0;
	   ringCnt = 0;
	   first_init = 1;
	   pcm_size = 0;
	   out_samRate = 0;

	   DEBUG0("***  bbbb *****\n");
	   /* destruct the inherited object */
	   BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));
	
	   /* free the object memory */
	   ATX_FreeMemory(self);
    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   AmrDecoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AmrDecoder_GetPortByName(BLT_MediaNode*  _self,
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
	AmrDecoder* self = ATX_SELF_EX(AmrDecoder, BLT_BaseMediaNode, BLT_MediaNode);


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
|   AmrDecoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AmrDecoder_Seek(BLT_MediaNode* _self,
                      BLT_SeekMode*  mode,
                      BLT_SeekPoint* point)
{
	AmrDecoder* self = ATX_SELF_EX(AmrDecoder, BLT_BaseMediaNode, BLT_MediaNode);
	BLT_UInt32	ms;

	/* flush pending input packets */
	AmrDecoderInput_Flush(self);

	/* clear the eos flag */
	self->input.eos  = BLT_FALSE;
	self->output.eos = BLT_FALSE;

	ms = point->time_stamp.seconds * 1000 + point->time_stamp.nanoseconds /1000000;
	pcm_size += 1000 * ms * out_samRate;

	seekable = 1;
	first_init = 1;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrDecoder)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrDecoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AmrDecoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AmrDecoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    AmrDecoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AmrDecoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   AmrDecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
AmrDecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{

	
	AmrDecoderModule* self = ATX_SELF_EX(AmrDecoderModule, BLT_BaseModule, BLT_Module);
	BLT_Registry*           registry;
	BLT_Result              result;

	DEBUG0("****** into AmrDecoderModule_Attach******\n");


	/* get the registry */
	result = BLT_Core_GetRegistry(core, &registry);
	if (BLT_FAILED(result)) return result;

	/* register the .amr file extensions */
	result = BLT_Registry_RegisterExtension(registry, 
	                                    ".amr",
	                                    "audio/amr");
	if (BLT_FAILED(result)) return result;

	/* register the "audio/amr" type */
	result = BLT_Registry_RegisterName(registry,
				BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
				"audio/amr",
				&self->amr_type_id);
	if (BLT_FAILED(result)) return result;

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AmrDecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AmrDecoderModule_Probe(BLT_Module*              _self, 
                             BLT_Core*                core,
                             BLT_ModuleParametersType parameters_type,
                             BLT_AnyConst             parameters,
                             BLT_Cardinal*            match)
{

    AmrDecoderModule* self = ATX_SELF_EX(AmrDecoderModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

	DEBUG0("****** into AmrDecoderModule_Probe******\n");
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
	
			/* the input type should be audio/amr */
            if (constructor->spec.input.media_type->id != 
                self->amr_type_id) {
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
                if (ATX_StringsEqual(constructor->name, "AmrDecoder")) {
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AmrDecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrDecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AmrDecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AmrDecoderModule, AmrDecoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AmrDecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    AmrDecoderModule_Attach,
    AmrDecoderModule_CreateInstance,
    AmrDecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AmrDecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AmrDecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(AmrDecoderModule,
                                         "Amr Decoder",
                                         "com.axiosys.decoder.amr",
                                         "0.0.1",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)

