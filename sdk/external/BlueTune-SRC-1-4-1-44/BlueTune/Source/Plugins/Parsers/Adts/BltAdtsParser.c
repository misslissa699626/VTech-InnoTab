/*****************************************************************
|
|   ADTS Parser Module
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltAdtsParser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"

#if 0
#define DEBUG0 printf
#else
#define DEBUG0(...)
#endif


/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.adts")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_ADTS_PARSER_MAX_FRAME_SIZE           8192
#define BLT_AAC_DECODER_OBJECT_TYPE_MPEG2_AAC_LC 0x67
#define BLT_AAC_DECODER_OBJECT_TYPE_MPEG4_AUDIO  0x40

static const unsigned int AacSamplingFreqTable[13] =
{
	96000, 88200, 64000, 48000, 
    44100, 32000, 24000, 22050, 
    16000, 12000, 11025, 8000, 
    7350
};


/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 adts_type_id;
    BLT_UInt32 mp4es_type_id;
} AdtsParserModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType    media_type;
    ATX_InputStream* stream;
    BLT_Boolean      eos;
} AdtsParserInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_Mp4AudioMediaType* media_type;
} AdtsParserOutput;

typedef enum {
    BLT_ADTS_PARSER_STATE_NEED_SYNC,
    BLT_ADTS_PARSER_STATE_IN_FRAME
} AdtsParserState;

typedef struct {
    /* fixed part */
    unsigned char id;
    unsigned char layer;
    unsigned char protection_absent;
    unsigned char profile_object_type;
    unsigned char sampling_frequency_index;
    unsigned char private_bit;
    unsigned char channel_configuration;
    unsigned char original;
    unsigned char home;
    
    /* variable part */
    unsigned char copyright_identification_bit;
    unsigned char copyright_identification_start;
    unsigned int  aac_frame_length;
    unsigned int  adts_buffer_fullness;
    unsigned char number_of_raw_data_blocks;
} AdtsHeader;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    AdtsParserInput  input;
    AdtsParserOutput output;
    AdtsParserState  state;
    unsigned char    buffer[BLT_ADTS_PARSER_MAX_FRAME_SIZE];
    unsigned int     buffer_fullness;
    AdtsHeader       frame_header;
} AdtsParser;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(AdtsParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(AdtsParser, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(AdtsParser, ATX_Referenceable)

/*----------------------------------------------------------------------
|   AdtsParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParserInput_SetStream(BLT_InputStreamUser* _self,
                          ATX_InputStream*     stream,
                          const BLT_MediaType* media_type)
{
    AdtsParserInput* self = ATX_SELF(AdtsParserInput, BLT_InputStreamUser);
	static BLT_UInt32 first_enter = 1;
	BLT_Position where;
	BLT_LargeSize stream_size;
	BLT_StreamInfo info;

	/* check media type */
    if (media_type == NULL || media_type->id != self->media_type.id) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a stream, release it */
    ATX_RELEASE_OBJECT(self->stream);
    
    /* keep a reference to the stream */
    self->stream = stream;
    ATX_REFERENCE_OBJECT(stream);

	AdtsParser* pParser   = ATX_SELF_M(input, AdtsParser, BLT_InputStreamUser);
	/*count file size*/
	if(first_enter){
		/* update the stream info */
		ATX_InputStream_GetSize(pParser->input.stream, &stream_size);
		info.size = stream_size;
        if (ATX_BASE(pParser, BLT_BaseMediaNode).context) {
        	info.mask = BLT_STREAM_INFO_MASK_SIZE;
            BLT_Stream_SetInfo(ATX_BASE(pParser, BLT_BaseMediaNode).context, &info);
           }
		first_enter = 0;
	}
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParserInput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    AdtsParserInput* self = ATX_SELF(AdtsParserInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AdtsParserInput)
    ATX_GET_INTERFACE_ACCEPT(AdtsParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AdtsParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AdtsParserInput, BLT_InputStreamUser)
    AdtsParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AdtsParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(AdtsParserInput, BLT_MediaPort)
    AdtsParserInput_GetName,
    AdtsParserInput_GetProtocol,
    AdtsParserInput_GetDirection,
    AdtsParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AdtsParser_MakeEosPacket
+---------------------------------------------------------------------*/
static BLT_Result
AdtsParser_MakeEosPacket(AdtsParser* self, BLT_MediaPacket** packet)
{
    BLT_Result result;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
                                        0,
                                        (BLT_MediaType*)self->output.media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;

    BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsParser_ParseHeader
+---------------------------------------------------------------------*/
static BLT_Result
AdtsHeader_Parse(AdtsHeader* h, unsigned char* buffer)
{
    h->id                             =  (buffer[1]>>4)&0x01;
    h->layer                          =  (buffer[1]>>1)&0x03;
    h->protection_absent              =  (buffer[1]   )&0x01;
    h->profile_object_type            =  (buffer[2]>>6)&0x03;
    h->sampling_frequency_index       =  (buffer[2]>>2)&0x0F;
    h->private_bit                    =  (buffer[2]>>1)&0x01;
    h->channel_configuration          = ((buffer[2]<<1)&0x04) | 
                                        ((buffer[3]>>6)&0x03);
    h->original                       =  (buffer[3]>>5)&0x01;
    h->home                           =  (buffer[3]>>4)&0x01;
    h->copyright_identification_bit   =  (buffer[3]>>3)&0x01;
    h->copyright_identification_start =  (buffer[3]>>2)&0x01;
    h->aac_frame_length               = (((unsigned int)buffer[3]<<11)&0x1FFF) |
                                        (((unsigned int)buffer[4]<< 3)       ) |
                                        (((unsigned int)buffer[5]>> 5)&0x07  );

	h->adts_buffer_fullness           = (((unsigned int)buffer[5]<< 6)&0x3FF ) |
                                        (((unsigned int)buffer[6]>> 2)&0x03  );
    h->number_of_raw_data_blocks      = (buffer[6]    )&0x03;

    /* check the validity of the header */
    if (h->layer != 0) return BLT_FAILURE;
    if (h->id == 1 && h->profile_object_type == 3) return BLT_FAILURE;
    if (h->sampling_frequency_index > 12) return BLT_FAILURE;
    if (h->aac_frame_length < 7) return BLT_FAILURE;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsHeader_Match
+---------------------------------------------------------------------*/
static BLT_Boolean
AdtsHeader_Match(const AdtsHeader* h1, const AdtsHeader* h2)
{
    return h1->id                       == h2->id && 
           h1->layer                    == h2->layer &&
           h1->protection_absent        == h2->protection_absent &&
           h1->profile_object_type      == h2->profile_object_type &&
           h1->sampling_frequency_index == h2->sampling_frequency_index &&
           h1->private_bit              == h2->private_bit &&
           h1->channel_configuration    == h2->channel_configuration &&
           h1->original                 == h2->original &&
           h1->home                     == h2->home;
  }

/*----------------------------------------------------------------------
|   AdtsParser_FindHeader
+---------------------------------------------------------------------*/
static BLT_Result
AdtsParser_FindHeader(AdtsParser* self, AdtsHeader* header)
{
    BLT_Result   result;
    unsigned int i;

    /* refill the buffer to have 7 bytes */
    result = ATX_InputStream_ReadFully(self->input.stream, 
                                       &self->buffer[self->buffer_fullness], 
                                       7-self->buffer_fullness);
    if (BLT_FAILED(result)) return result;

		self->buffer_fullness = 7;
    
    /* look for a sync pattern */
    for (i=0; i<6; i++) {
        if (self->buffer[i] == 0xFF && (self->buffer[i+1]&0xF0) == 0xF0) {
            /* sync pattern found, get a full header */
            if (i != 0) {
                /* refill the header to a full 7 bytes */
                unsigned int j;
                for (j=i; j<7; j++) {
                    self->buffer[j-i] = self->buffer[j];
                }
                result = ATX_InputStream_ReadFully(self->input.stream, 
                                                   &self->buffer[7-i], 
                                                   i);
                if (BLT_FAILED(result)) return result;
            }
            
            result = AdtsHeader_Parse(header, self->buffer);
            if (BLT_FAILED(result)) {
                /* it looked like a header, but wasn't one */
                /* skip two bytes and try again            */
                unsigned int j;
                for (j=2; j<7; j++) {
                    self->buffer[j-2] = self->buffer[j];
                }
                self->buffer_fullness = 5;
                return BLT_ERROR_PORT_HAS_NO_DATA;
            }
            
            self->buffer_fullness = 7;

            /* found a valid header */
            return BLT_SUCCESS;
        }
    }

    /* sync pattern not found, keep the last byte for the next time around */
    self->buffer[0] = self->buffer[6];
    self->buffer_fullness = 1;
    
    return BLT_ERROR_PORT_HAS_NO_DATA;
}

/*----------------------------------------------------------------------
|   AdtsParser_ReadHeader
+---------------------------------------------------------------------*/
static BLT_Result
AdtsParser_ReadHeader(AdtsParser* self, AdtsHeader* header, unsigned char *buf_header) 
{
    BLT_Result    result;
    unsigned char buffer[7];

    /* fill the buffer to have 7 bytes */
    result = ATX_InputStream_ReadFully(self->input.stream, buffer, 7);
	
    if (BLT_FAILED(result)) return result;

    /* look for a sync pattern */
    if (buffer[0] != 0xFF || (buffer[1]&0xF0) != 0xF0) {
        /* not a sync pattern */
        return BLT_FAILURE;
    } 
    /*add by junp.zhang at 4/22/2011*/
	if(buf_header != NULL){
		ATX_CopyMemory(buf_header,buffer, sizeof(buffer));
	}
	
    
    /* parse the buffer into a header struct */
    result = AdtsHeader_Parse(header, buffer);
    if (BLT_FAILED(result)) return result;
        
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsParser_UpdateMediaType
+---------------------------------------------------------------------*/
static void
AdtsParser_UpdateMediaType(AdtsParser* self)
{
    unsigned int dsi = 0;
    
    self->output.media_type->base.format_or_object_type_id = 
        self->frame_header.id == 0 ?
        BLT_AAC_DECODER_OBJECT_TYPE_MPEG4_AUDIO :
        BLT_AAC_DECODER_OBJECT_TYPE_MPEG2_AAC_LC;
        
    dsi = ((self->frame_header.profile_object_type+1)   << 11) |
          ( self->frame_header.sampling_frequency_index << 7 ) |
          ( self->frame_header.channel_configuration    << 3 );
	
    self->output.media_type->decoder_info[0] = (unsigned char)(dsi>>8);
    self->output.media_type->decoder_info[1] = (unsigned char)dsi;  
}

/*----------------------------------------------------------------------
|   AdtsParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    AdtsParserOutput* self = ATX_SELF(AdtsParserOutput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (BLT_MediaType*)self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AdtsParser_ReadHeader
+---------------------------------------------------------------------*/
static BLT_Result
AdtsParser_GetFrameSize(AdtsParser* self, BLT_UInt32* size) 
{
    BLT_Result    result;
    unsigned char buffer[7];

    /* fill the buffer to have 7 bytes */
    result = ATX_InputStream_ReadFully(self->input.stream, buffer, 7);
	
    if (BLT_FAILED(result)) return result;

    /* look for a sync pattern */
    if (buffer[0] != 0xFF || (buffer[1]&0xF0) != 0xF0) {
        /* not a sync pattern */
        return BLT_FAILURE;
    } 
  
    /* parse the buffer into a header struct */
    *size           = (((unsigned int)buffer[3]<<11)&0x1FFF) |
                                        (((unsigned int)buffer[4]<< 3)       ) |
                                        (((unsigned int)buffer[5]>> 5)&0x07  );
        
    return BLT_SUCCESS;
}


/*----------------------------------------------------------------------
|   AdtsParserOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParserOutput_GetPacket(BLT_PacketProducer* _self,
                           BLT_MediaPacket**   packet)
{
    AdtsParser* self   = ATX_SELF_M(output, AdtsParser, BLT_PacketProducer);
    BLT_Result  result = BLT_SUCCESS;
	int i = 0;
	unsigned char buf_header[7];	
	BLT_Position where;

	#if 0
	if(first_enter){
		/* update the stream info */
 		BLT_StreamInfo info;
		ATX_InputStream_Tell(self->input.stream , &where);
		ATX_InputStream_Seek(self->input.stream, 0);
		ATX_InputStream_GetSize(self->input.stream, &stream_size);
		ATX_InputStream_Seek(self->input.stream, where);
		info.size = stream_size;
        if (ATX_BASE(self, BLT_BaseMediaNode).context) {
        	info.mask = BLT_STREAM_INFO_MASK_SIZE;
            BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
           }
		first_enter = 0;
	}
	#endif
    
    /* default value */
    *packet = NULL;
    
    /* check for EOS */
    if (self->input.eos) return BLT_ERROR_EOS;
    
    /* do the next step of the state machine */
    switch (self->state) {
        case BLT_ADTS_PARSER_STATE_NEED_SYNC: {
            result = AdtsParser_FindHeader(self, &self->frame_header);
            if (BLT_SUCCEEDED(result)) {
                AdtsParser_UpdateMediaType(self);
                self->state = BLT_ADTS_PARSER_STATE_IN_FRAME;
				
            }
        }
        break;
                        
        case BLT_ADTS_PARSER_STATE_IN_FRAME: {
			/*快速获取每一个frame的大小*/
			BLT_UInt32 			size;
			BLT_Position 		where;
			BLT_UInt32 			count = 1;
			BLT_Position    	back=0;
			static BLT_UInt32	count_frame = 1;
			if(count_frame)
			{
			//	DEBUG0("*************\n");
				ATX_InputStream_Tell(self->input.stream, &where);
			//	DEBUG0("where = %llu\n", where);
			//	DEBUG0("self->frame_header.aac_frame_length = %d\n", self->frame_header.aac_frame_length);
				//while(1);
				back = where;
				ATX_InputStream_Seek(self->input.stream, self->frame_header.aac_frame_length);
			
				while(1)
				{
					result = AdtsParser_GetFrameSize(self, &size);
					if(result == BLT_ERROR_EOS){
						break;
					}
				
					count++;
					//DEBUG0("size = %u\n", size);
					//DEBUG0("count = %u\n", count);
					ATX_InputStream_Tell(self->input.stream, &where);
					ATX_InputStream_Seek(self->input.stream, where - 7 + size);
					
				}
				self->output.media_type->frame_count = count;
				ATX_InputStream_Seek(self->input.stream, back);
				count_frame = 0;
				//DEBUG0("self->output.media_type->frame_count = %u", self->output.media_type->frame_count);
				//while(1);
			}
			
			unsigned int needed = self->frame_header.aac_frame_length - 
                                  self->buffer_fullness;
			
            if (needed) {
                ATX_Size bytes_read = 0;
                result = ATX_InputStream_Read(self->input.stream, 
                                              &self->buffer[self->buffer_fullness],
                                              needed,
                                              &bytes_read);
                if (BLT_FAILED(result)) break;
                self->buffer_fullness += bytes_read;
            }
            if (self->buffer_fullness == self->frame_header.aac_frame_length) {
                /* the frame is complete, look for the next header */
                AdtsHeader next_header = self->frame_header;
                result = AdtsParser_ReadHeader(self, &next_header, buf_header);
                if (BLT_FAILED(result)) {
                    /* at the end of the stream, it is ok not to have */
                    /* a next header.                                 */
                    if (result != BLT_ERROR_EOS) {
                        self->state = BLT_ADTS_PARSER_STATE_NEED_SYNC;
                        self->buffer_fullness = 0;
                        return BLT_ERROR_PORT_HAS_NO_DATA;
                    }
                }

                /* do as if we had read the header at the start of the buffer */
                self->buffer_fullness = 7;
                
                /* compare with the current header */
                if (!AdtsHeader_Match(&next_header, &self->frame_header)) {
                    /* not the same header, look for a new frame */
                    self->frame_header = next_header;
                    AdtsParser_UpdateMediaType(self);
                    return BLT_ERROR_PORT_HAS_NO_DATA;
                }

                /* this frame looks good, create a media packet for it */
                {
                    //unsigned int payload_offset = self->frame_header.protection_absent?7:9;
					unsigned int payload_offset = 0;
                    unsigned int payload_size = self->frame_header.aac_frame_length-payload_offset;
                    
                    if (self->frame_header.aac_frame_length < payload_offset) {
                        /* something is terrible wrong here */
                        self->state = BLT_ADTS_PARSER_STATE_NEED_SYNC;
                        self->buffer_fullness = 0;
                        return BLT_ERROR_PORT_HAS_NO_DATA;                        
                    }
                    
                    result = BLT_Core_CreateMediaPacket(
                        ATX_BASE(self, BLT_BaseMediaNode).core,
                        payload_size,
                        (BLT_MediaType*)self->output.media_type,
                        packet);
                    if (BLT_FAILED(result)) return result;
					
                    ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(*packet),
                                   &self->buffer[payload_offset],
                                   payload_size);
										/*add by junp.zhang at 2011.4.22*/
										for(i=0;i<7;i++){
											if(self->buffer[i]!= buf_header[i]){
													ATX_CopyMemory(self->buffer,buf_header,7);
											}
										}
                    BLT_MediaPacket_SetPayloadSize(*packet, payload_size);
                }

                /* update the header for next time */
                self->frame_header = next_header;
            }
        }
        break;
    }
    
    if (*packet == NULL) {
        if (result == BLT_ERROR_EOS) {
            self->input.eos = BLT_TRUE;
            return AdtsParser_MakeEosPacket(self, packet);
        } else {
            return result;
        }
    } else {
        return BLT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AdtsParserOutput)
    ATX_GET_INTERFACE_ACCEPT(AdtsParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(AdtsParserOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(AdtsParserOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(AdtsParserOutput, BLT_MediaPort)
    AdtsParserOutput_GetName,
    AdtsParserOutput_GetProtocol,
    AdtsParserOutput_GetDirection,
    AdtsParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(AdtsParserOutput, BLT_PacketProducer)
    AdtsParserOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   AdtsParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
AdtsParser_Create(BLT_Module*              module,
                  BLT_Core*                core, 
                  BLT_ModuleParametersType parameters_type,
                  BLT_CString              parameters, 
                  BLT_MediaNode**          object)
{
    AdtsParser* self;

    ATX_LOG_FINE("AdtsParser::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(AdtsParser));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    BLT_MediaType_Init(&self->input.media_type,
                       ((AdtsParserModule*)module)->adts_type_id);
    self->input.stream = NULL;
    self->output.media_type = (BLT_Mp4AudioMediaType*)ATX_AllocateZeroMemory(sizeof(BLT_Mp4AudioMediaType)+1);
    BLT_MediaType_InitEx(&self->output.media_type->base.base, ((AdtsParserModule*)module)->mp4es_type_id, sizeof(BLT_Mp4AudioMediaType)+1);
    self->output.media_type->base.stream_type              = BLT_MP4_STREAM_TYPE_AUDIO;
    self->output.media_type->base.format_or_object_type_id = 0; 
    self->output.media_type->decoder_info_length           = 2;

    self->state = BLT_ADTS_PARSER_STATE_NEED_SYNC;
    self->buffer_fullness = 0;
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, AdtsParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, AdtsParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  AdtsParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  AdtsParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, AdtsParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, AdtsParserOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
AdtsParser_Destroy(AdtsParser* self)
{
    ATX_LOG_FINE("AdtsParser::Destroy");

    /* release the byte stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    /* free the media type extensions */
    BLT_MediaType_Free((BLT_MediaType*)self->output.media_type);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    AdtsParser_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParser_Deactivate(BLT_MediaNode* _self)
{
    AdtsParser* self = ATX_SELF_EX(AdtsParser, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("AdtsParser::Deactivate");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    /* release the stream */
    ATX_RELEASE_OBJECT(self->input.stream);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    AdtsParser* self = ATX_SELF_EX(AdtsParser, BLT_BaseMediaNode, BLT_MediaNode);

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
|   AdtsParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParser_Seek(BLT_MediaNode* _self,
                BLT_SeekMode*  mode,
                BLT_SeekPoint* point)
{
    AdtsParser* self = ATX_SELF_EX(AdtsParser, BLT_BaseMediaNode, BLT_MediaNode);
	BLT_Result result;
	BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);
	BLT_UInt64 where;

	DEBUG0("*** into AdtsParser_Seek **\n");

	 /* estimate the seek offset from the other stream parameters */
    result = BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (BLT_FAILED(result)) return result;
    if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
        return BLT_FAILURE;
    }
	
    /* we need to reset the state machine */
    self->state = BLT_ADTS_PARSER_STATE_NEED_SYNC;
    self->buffer_fullness = 0;
    self->input.eos = BLT_FALSE;
	DEBUG0("*** point->offset = %llu **\n", point->offset);
    /* seek into the input stream (ignore return value) */
	 ATX_InputStream_Tell(self->input.stream, &where);
	DEBUG0("where = %llu\n", where);
    ATX_InputStream_Seek(self->input.stream, point->offset);
	ATX_InputStream_Tell(self->input.stream, &where);
	DEBUG0("where = %llu\n", where);

    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                 */
    *mode = BLT_SEEK_MODE_IGNORE;

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AdtsParser)
    ATX_GET_INTERFACE_ACCEPT_EX(AdtsParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(AdtsParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AdtsParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    AdtsParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    AdtsParser_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    AdtsParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AdtsParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   AdtsParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    AdtsParserModule* self = ATX_SELF_EX(AdtsParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*     registry;
    BLT_Result        result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".aac" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".aac",
                                            "audio/aac");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/aac" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/aac",
        &self->adts_type_id);
    if (BLT_FAILED(result)) return result;
 
    /* register an alias for the same mime type */
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/aacp", self->adts_type_id);

    /* register the type id for BLT_MP4_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_MP4_AUDIO_ES_MIME_TYPE,
        &self->mp4es_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("ADTS Parser Module::Attach (" BLT_MP4_AUDIO_ES_MIME_TYPE " type = %d)", self->mp4es_type_id);
    ATX_LOG_FINE_1("ADTS Parser Module::Attach (audio/aac type = %d)", self->adts_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   AdtsParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
AdtsParserModule_Probe(BLT_Module*              _self, 
                       BLT_Core*                core,
                       BLT_ModuleParametersType parameters_type,
                       BLT_AnyConst             parameters,
                       BLT_Cardinal*            match)
{
    AdtsParserModule* self = ATX_SELF_EX(AdtsParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need the input protocol to be STREAM_PULL and the output */
            /* protocol to be STREAM_PULL                                  */
             if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != 
                 BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* we need the input media type to be 'audio/aac' */
            if (constructor->spec.input.media_type->id != self->adts_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unknown or an AAC elementary stream at this point */
            if (constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN &&
                constructor->spec.output.media_type->id != self->mp4es_type_id) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "AdtsParser")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not out name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("AdtsParserModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(AdtsParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(AdtsParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(AdtsParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(AdtsParserModule, AdtsParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(AdtsParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    AdtsParserModule_Attach,
    AdtsParserModule_CreateInstance,
    AdtsParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define AdtsParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(AdtsParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(AdtsParserModule,
                                         "ADTS Parser",
                                         "com.axiosys.parser.adts",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
