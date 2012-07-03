/*****************************************************************
|
|   BlueTune - MP3 Decoder Module
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
#include "BltMP3Decoder.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltStream.h"
#include "BltReplayGain.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/ioctl.h>
#include <mach/gp_chunkmem.h>
#include <mach/audio/soundcard.h>
#include "gp_avcodec.h"
#include "auddec.h"
#include "gp_acodec.h"
#include <dlfcn.h>


/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.decoders.mpeg-audio")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 mpeg_audio_type_id;
} MP3DecoderModule;

typedef struct {
    unsigned char id;
    unsigned char layer;
    unsigned char protection_bit;
    unsigned char bitrate_index;
    unsigned char sampling_frequency;
    unsigned char padding_bit;
    unsigned char private_bit;
    unsigned char mode;
    unsigned char mode_extension;
    unsigned char copyright;
    unsigned char original;
    unsigned char emphasis;
} FrameHeader;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketConsumer);

    /* members */
    BLT_Boolean eos;
    BLT_Boolean is_continous_stream;
    ATX_List*   packets;
} MP3DecoderInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_Boolean      eos;
    BLT_PcmMediaType media_type;
    BLT_TimeStamp    time_stamp;
    ATX_Int64        sample_count;
} MP3DecoderOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    BLT_Module             module;
    MP3DecoderInput  input;
    MP3DecoderOutput output;
    //FLO_Decoder*           fluo;
    struct {
        BLT_Cardinal nominal_bitrate;
        BLT_Cardinal average_bitrate;
        ATX_Int64    average_bitrate_accumulator;
        BLT_Cardinal instant_bitrate;
        ATX_Int64    instant_bitrate_accumulator;
    }                      stream_info;
    struct {
        ATX_Flags flags;
        ATX_Int32 track_gain;
        ATX_Int32 album_gain;
    }                      replay_gain_info;
    struct {
        unsigned int level;
        unsigned int layer;
    }                      mpeg_info;
} MP3Decoder;

const unsigned short MpegBitrates[3][3][16] = {
    /* MPEG2 */
    {
        /* layer I */
        {  
              0,  32,  48,  56,  64,  80,  96, 112, 
            128, 144, 160, 176, 192, 224, 256,   0
        },
        /* layer II */
        {  
              0,   8,  16,  24,  32,  40,  48,  56, 
             64,  80,  96, 112, 128, 144, 160,   0
        },
        /* layer III */
        {  
              0,   8,  16,  24,  32,  40,  48,  56, 
             64,  80,  96, 112, 128, 144, 160,   0
        }
    },
    /* MPEG1 */
    {
        /* layer I */
        {
              0,  32,  64,  96, 128, 160, 192, 224, 
            256, 288, 320, 352, 384, 416, 448,   0
        },
        /* layer II */
        {
              0,  32,  48,  56,  64,  80,  96, 112, 
            128, 160, 192, 224, 256, 320, 384,   0
        },        
        /* layer III */
        {
              0,  32,  40,  48,  56,  64,  80,  96, 
            112, 128, 160, 192, 224, 256, 320,   0
        },
    },
    /* MPEG2.5 */
    {
        /* layer I */
        {
              0,  32,  48,  56,  64,  80,  96, 112, 
            128, 144, 160, 176, 192, 224, 256,   0
        },
        /* layer II */
        {
              0,   8,  16,  24,  32,  40,  48,  56, 
             64,  80,  96, 112, 128, 144, 160,   0
        },
        /* layer III */
        {
              0,   8,  16,  24,  32,  40,  48,  56, 
             64,  80,  96, 112, 128, 144, 160,   0
        }
    }
};

const unsigned MpegSamplingFrequencies[3][4] = {
    {22050, 24000, 16000, 0}, /* MPEG2   */
    {44100, 48000, 32000, 0}, /* MPEG1   */
    {11025, 12000,  8000, 0}  /* MPEG2.5 */
};


/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_BITRATE_AVERAGING_SHORT_SCALE     7
#define BLT_BITRATE_AVERAGING_SHORT_WINDOW    32
#define BLT_BITRATE_AVERAGING_LONG_SCALE      7
#define BLT_BITRATE_AVERAGING_LONG_WINDOW     4096
#define BLT_BITRATE_AVERAGING_PRECISION       4000
#define RING_ALLOCATE_SIZE  64*1024
#define MPEG_MODE_SINGLE_CHANNEL  3
#define MPEG_LAYER_I    3
#define MPEG_LAYER_I_PCM_SAMPLES_PER_FRAME      384
#define MPEG_LAYER_II                           2
#define MPEG_LAYER_III                          1
#define MPEG_LAYER_II_PCM_SAMPLES_PER_FRAME     1152
#define MPEG_ID_MPEG_1                          1
#define MPEG_LAYER_III_MPEG1_PCM_SAMPLES_PER_FRAME    1152
#define MPEG_LAYER_III_MPEG2_PCM_SAMPLES_PER_FRAME    576
#define MPEG_LAYER_I_BYTES_PER_SLOT             4
#define MPEG_ID_MPEG_2_5                        2
#define MPEG_ID_MPEG_2                          0
#define END_OF_STREAM     0x04
#define MPEG_SYNC_WORD_BIT_LENGTH               11
#define MPEG_SYNC_WORD                          0x000007FFL
#define BYTE_STREAM_POINTER_VAL(offset)   ((offset)&(RING_ALLOCATE_SIZE-1))

#define BYTE_STREAM_POINTER_OFFSET(pointer, offset)     (BYTE_STREAM_POINTER_VAL((pointer)+(offset)))

#define BYTE_STREAM_POINTER_ADD(pointer, offset)     ((pointer) = BYTE_STREAM_POINTER_OFFSET(pointer, offset))
/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(MP3DecoderModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(MP3Decoder, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(MP3Decoder, ATX_Referenceable)

/*----------------------------------------------------------------------
|   MP3DecoderInput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
MP3DecoderInput_Flush(MP3Decoder* self)
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
|   MpegAudioDecoder_UpdateBitrateAverage
+---------------------------------------------------------------------*/
static BLT_Cardinal
MpegAudioDecoder_UpdateBitrateAverage(BLT_Cardinal previous_bitrate,
                                      BLT_Cardinal current_bitrate,
                                      ATX_Int64*   accumulator,
                                      BLT_Cardinal window,
                                      BLT_Cardinal scale)
{
    BLT_Cardinal new_bitrate;
    long         diff_bitrate;

    if (previous_bitrate == 0) {
        *accumulator = current_bitrate << scale;
        return current_bitrate;
    }

    *accumulator *= window-1;
    *accumulator += current_bitrate << scale;
    *accumulator /= window;
    
    new_bitrate = (ATX_UInt32)(*accumulator);
    new_bitrate = (new_bitrate + (1<<(scale-1))) >> scale;
    new_bitrate = new_bitrate /
        BLT_BITRATE_AVERAGING_PRECISION *
        BLT_BITRATE_AVERAGING_PRECISION;

    /* only update if the difference is more than 1/32th of the previous one */
    diff_bitrate = new_bitrate - previous_bitrate;
    if (diff_bitrate < 0) diff_bitrate = -diff_bitrate;
    if (diff_bitrate > (long)(previous_bitrate>>5)) {
        return new_bitrate;
    } else {
        return previous_bitrate;
    }
}

ATX_UInt8  pcm_buf[RING_ALLOCATE_SIZE];
audec_interface_t  *audec;
audec_param_t  adp;
 int init_flag=1,seek_flag=0;
/*----------------------------------------------------------------------
|    MP3_Mp3Init
+---------------------------------------------------------------------*/
ATX_Result
MP3_Mp3Init(void){
	int dec_ptr;
       void *g_dlhandle= NULL;
	if((g_dlhandle = dlopen("/system/lib/libmp3dec.so", RTLD_NOW))== NULL){
			perror("dlopen");
			exit(1);
		}
	 audec = (audec_interface_t *)dlsym(g_dlhandle, "gp_aud_dec_api");
	 if(audec == NULL){
		perror("dlsym");
		exit(1);
	 }
	 dec_ptr = (int)ATX_AllocateZeroMemory(audec->instance_size() + 4);
        if(dec_ptr<0){
	        perror("dec_ptr");
		 exit(1);
        }
	ATX_SetMemory(pcm_buf, 0, RING_ALLOCATE_SIZE);
  	ATX_SetMemory(&adp, 0, sizeof(adp)); 
	adp.codec_id=CODEC_ID_MP3;
	adp.hDecoder=ATX_AllocateZeroMemory(audec->instance_size());
	adp.audec_buf = (char *)((dec_ptr + 3) & ~3);
	adp.Ring=(char*)ATX_AllocateZeroMemory(RING_ALLOCATE_SIZE);
	adp.RingSize=RING_ALLOCATE_SIZE;
	adp.RingWI=adp.RingRI=0;	
	return 0;
}
/*----------------------------------------------------------------------
|   GetBytesFree
+---------------------------------------------------------------------*/
unsigned int 
GetBytesFree(audec_param_t* adp)
{
    return  
        (adp->RingWI < adp->RingRI) ? 
        (adp->RingRI - adp->RingWI - 1):
        (RING_ALLOCATE_SIZE  + (adp->RingRI - adp->RingWI) - 1);
}
/*----------------------------------------------------------------------
|   WriteBytes
+---------------------------------------------------------------------*/

ATX_Result
WriteBytes(audec_param_t* adp, 
                         const unsigned char* bytes, 
                         unsigned int         byte_count)
{
    if (adp->RingWI< adp->RingRI) {
        ATX_CopyMemory(adp->Ring+adp->RingWI, bytes, byte_count);
        BYTE_STREAM_POINTER_ADD(adp->RingWI, byte_count);		
    } else {
        unsigned int chunk = RING_ALLOCATE_SIZE - adp->RingWI;
        if (chunk > byte_count) chunk = byte_count;
        ATX_CopyMemory(adp->Ring+adp->RingWI, bytes, chunk);
        BYTE_STREAM_POINTER_ADD(adp->RingWI, chunk);
        if (chunk != byte_count) {
            ATX_CopyMemory(adp->Ring+adp->RingWI,  bytes+chunk, byte_count-chunk);
            BYTE_STREAM_POINTER_ADD(adp->RingWI, byte_count-chunk);
        }
    }

    return 0;
}
/*----------------------------------------------------------------------
|   FGetBytesAvailable
+---------------------------------------------------------------------*/
unsigned int 
GetBytesAvailable(audec_param_t* adp)
{
    return 
	 (adp->RingRI<=adp->RingWI)?
	 (adp->RingWI-adp->RingRI):
	 (adp->RingWI+(RING_ALLOCATE_SIZE-adp->RingRI));
}
/*----------------------------------------------------------------------
|   MP3DecoderInput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
MP3DecoderInput_PutPacket(BLT_PacketConsumer* _self,
                                 struct BLT_MediaPacket *    packet)
{
    MP3Decoder* self = ATX_SELF_M(input, MP3Decoder, BLT_PacketConsumer);
    ATX_Result        result;
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MP3DecoderInput)
    ATX_GET_INTERFACE_ACCEPT(MP3DecoderInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(MP3DecoderInput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MP3DecoderInput, BLT_PacketConsumer)
    MP3DecoderInput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(MP3DecoderInput, 
                                         "input",
                                         PACKET,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(MP3DecoderInput, BLT_MediaPort)
    MP3DecoderInput_GetName,
    MP3DecoderInput_GetProtocol,
    MP3DecoderInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MP3Decoder_DecodeFrame
+---------------------------------------------------------------------*/

static BLT_Result
MP3Decoder_DecodeFrame(MP3Decoder* self,
                             BLT_MediaPacket** packet)
{      
	ATX_Result        result,ret;
	static ATX_Result level,mode,layer;
	static unsigned long packed;
	ATX_Any samples;
	ATX_Int32 	out_size = RING_ALLOCATE_SIZE;
	static FrameHeader frame_header;
	static ATX_UInt32  sample_rate,bitrate,channel_count,sample_count;
	if(init_flag){  
		unsigned int  current   = adp.RingRI;
		int available=GetBytesAvailable(&adp);
		packed=adp.Ring[current];
		BYTE_STREAM_POINTER_ADD(current, 1);
		packed = (packed << 8) | adp.Ring[current];
		BYTE_STREAM_POINTER_ADD(current, 1);
		packed = (packed << 8) | adp.Ring[current];
		BYTE_STREAM_POINTER_ADD(current, 1);
		packed = (packed << 8) | adp.Ring[current];
		BYTE_STREAM_POINTER_ADD(current, 1);
		available -= 4;
		for (;/* ever */;) {
		/* check if we have a sync word */
			if (((packed >> (32 - MPEG_SYNC_WORD_BIT_LENGTH)) & ((1<<MPEG_SYNC_WORD_BIT_LENGTH)-1)) == MPEG_SYNC_WORD) {
			    /* rewind to start of header */
			      adp.RingRI = BYTE_STREAM_POINTER_OFFSET(current, -4);
			break;
		}
		/* move on to the next byte */
		if (available > 0) {
			packed = (packed << 8) | adp.Ring[current];
			BYTE_STREAM_POINTER_ADD(current, 1);
			available --;
		} else {
		       break;
		}
	}			
	       /*unpack header*/
		frame_header.emphasis            = (unsigned char)(packed & 0x3); packed >>= 2;
		frame_header.original            = (unsigned char)(packed & 0x1); packed >>= 1;
		frame_header.copyright           = (unsigned char)(packed & 0x1); packed >>= 1;
		frame_header.mode_extension      = (unsigned char)(packed & 0x3); packed >>= 2;
		frame_header.mode                = (unsigned char)(packed & 0x3); packed >>= 2;
		frame_header.private_bit         = (unsigned char)(packed & 0x1); packed >>= 1;
		frame_header.padding_bit         = (unsigned char)(packed & 0x1); packed >>= 1;
		frame_header.sampling_frequency  = (unsigned char)(packed & 0x3); packed >>= 2;
		frame_header.bitrate_index       = (unsigned char)(packed & 0xF); packed >>= 4;
		frame_header.protection_bit      = (unsigned char)(packed & 0x1); packed >>= 1;
		frame_header.layer               = (unsigned char)(packed & 0x3); packed >>= 2;
		frame_header.id                  = (unsigned char)(packed & 0x1); packed >>= 1;
		if ((packed & 0x1) == 0) {
		    /* this is FHG's MPEG 2.5 extension */
		    frame_header.id += 2;
		}
	       /*get frameinfo*/
		level       = 2-frame_header.id;
		layer       = 4-frame_header.layer ;
		mode        = frame_header.mode;
		bitrate     =  MpegBitrates[frame_header.id][3-frame_header.layer][frame_header.bitrate_index] *1000;
		sample_rate = MpegSamplingFrequencies[frame_header.id][frame_header.sampling_frequency];
		if (frame_header.mode== MPEG_MODE_SINGLE_CHANNEL) {
		    channel_count = 1;
		} else {
		    channel_count = 2;
		}
		if (frame_header.layer == MPEG_LAYER_I) {
		    sample_count = MPEG_LAYER_I_PCM_SAMPLES_PER_FRAME;
		} else if (frame_header.layer == MPEG_LAYER_II) {
		    sample_count = MPEG_LAYER_II_PCM_SAMPLES_PER_FRAME;
		} else {
		    if (frame_header.id  == MPEG_ID_MPEG_1) {
		        sample_count = MPEG_LAYER_III_MPEG1_PCM_SAMPLES_PER_FRAME;
		    } else {
		        sample_count = MPEG_LAYER_III_MPEG2_PCM_SAMPLES_PER_FRAME;
		    }
		}
	       /*update the stream info*/
		if (sample_rate   != self->output.media_type.sample_rate   ||
		    channel_count != self->output.media_type.channel_count ||
		    level         != self->mpeg_info.level                 ||
		    layer         != self->mpeg_info.layer) {
		    /* keep the new info */
		    self->mpeg_info.layer = layer;
		    self->mpeg_info.level = level;
		    /* set the output type extensions */
		    BLT_PcmMediaType_Init(&self->output.media_type);
		    self->output.media_type.channel_count   = (BLT_UInt16)channel_count;
		    self->output.media_type.sample_rate     = sample_rate;
		    self->output.media_type.bits_per_sample = 16;
		    self->output.media_type.sample_format   = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
		}
		{
		    BLT_StreamInfo info;
		    char           data_type[32] = "MPEG-X Layer X";
		    data_type[ 5] = '0' + (level > 0 ?level : 2);
		    data_type[13] = '0' + layer;
		    info.data_type       = data_type;
		    info.sample_rate     = sample_rate;
		    info.channel_count   = (BLT_UInt16)channel_count;
		    if (ATX_BASE(self, BLT_BaseMediaNode).context) {
		        info.mask = 
		            BLT_STREAM_INFO_MASK_DATA_TYPE    |
		            BLT_STREAM_INFO_MASK_SAMPLE_RATE  |
		            BLT_STREAM_INFO_MASK_CHANNEL_COUNT;
		    BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
		    }
		}
	}
       /*update Duration and Bitratte*/

	{
		BLT_StreamInfo info;
		BLT_Stream_GetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
		info.mask = BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
		/* update the duration unless this is a continuous stream */
		if ((info.flags & BLT_STREAM_INFO_FLAG_CONTINUOUS) == 0) {
			info.mask |= BLT_STREAM_INFO_MASK_DURATION;
		}
		if (self->stream_info.nominal_bitrate == 0) {
			info.nominal_bitrate = bitrate;
			self->stream_info.nominal_bitrate = bitrate;
			info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
		} 
	       /* average bitrate */
		{
			info.average_bitrate = MpegAudioDecoder_UpdateBitrateAverage(
			self->stream_info.average_bitrate,
			bitrate,
			&self->stream_info.average_bitrate_accumulator,
			BLT_BITRATE_AVERAGING_LONG_WINDOW,
			BLT_BITRATE_AVERAGING_LONG_SCALE);
			self->stream_info.average_bitrate = info.average_bitrate;
		}
	       /* compute the duration from the size and the average bitrate */
		if (info.size && info.average_bitrate) {
			ATX_UInt64 duration_ms = (8*1000*(ATX_UInt64)info.size)/info.average_bitrate;
			info.duration = duration_ms;
		} else {
			info.duration = 0;
		}
	       /* the instant bitrate is a short window average */
		{
			info.instant_bitrate = MpegAudioDecoder_UpdateBitrateAverage(
			self->stream_info.instant_bitrate,
			bitrate,
			&self->stream_info.instant_bitrate_accumulator,
			BLT_BITRATE_AVERAGING_SHORT_WINDOW,
			BLT_BITRATE_AVERAGING_SHORT_SCALE);
			self->stream_info.instant_bitrate = info.instant_bitrate;
			info.mask |= BLT_STREAM_INFO_MASK_INSTANT_BITRATE;
		}
	       /* update the stream info */
		BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &info);
	}
       /* decode the frame */
	if(init_flag||seek_flag){   
			ret = audec->init(&adp);
			if(ret<0){
			 perror("audec->init");
			 return ret;
			}
			init_flag=0;
			seek_flag=0;
	}
	ret=audec->dec(&adp, pcm_buf, &out_size);
	if(ret<0){
		perror("audec->dec");
		return ret;
	}
	       /* get a packet from the core */
	*packet = NULL;
	result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
	                                   out_size,
	                                    (const BLT_MediaType*)&self->output.media_type,
	                                    packet);
	if (BLT_FAILED(result)) return result;
	
       /* get the address of the packet payload */
	samples = BLT_MediaPacket_GetPayloadBuffer(*packet);

	ATX_CopyMemory(samples,pcm_buf,out_size);
       /* update the sample count */
	self->output.sample_count += sample_count;
	BLT_MediaPacket_SetPayloadSize(*packet, out_size);
	/* set start of stream packet flags */
	{
		if (self->output.sample_count == 0) {
			BLT_MediaPacket_SetFlags(*packet, 
			BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);
		}
	}
       /* update the timestamp */
	if (channel_count             != 0 && 
	    sample_rate               != 0 ){
	    /* compute time stamp */
		self->output.time_stamp = 
		BLT_TimeStamp_FromSamples(self->output.sample_count, sample_rate);
		BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
	} 
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MP3DecoderOutput_GetPacket add by zheng.wang 2011.5.23
+---------------------------------------------------------------------*/
BLT_METHOD
MP3DecoderOutput_GetPacket(BLT_PacketProducer* _self,
                                 BLT_MediaPacket**   packet)
{
	MP3Decoder* self = ATX_SELF_M(output, MP3Decoder, BLT_PacketProducer);
	ATX_ListItem*     item;
	BLT_Any           payload_buffer=NULL;
	BLT_Size          payload_size,size;
	BLT_Boolean       try_again;
	BLT_Result        result;	
	BLT_MediaPacket* input;
	/* default return */
	*packet = NULL;
	/* check for EOS */
	if (self->output.eos) {
		return BLT_ERROR_EOS;
	}
	size=GetBytesFree(&adp);
	if ((item = ATX_List_GetFirstItem(self->input.packets))) {
			input = ATX_ListItem_GetData(item);
			ATX_Flags        flags = 0;
			/* get the packet payload */
			payload_buffer = BLT_MediaPacket_GetPayloadBuffer(input);
			payload_size   = BLT_MediaPacket_GetPayloadSize(input);		
		}
	if(size>payload_size && payload_buffer!=NULL){
		if(RING_ALLOCATE_SIZE-size-1<1024){
			result=WriteBytes(&adp, payload_buffer, payload_size);
					if(ATX_FAILED(result)){
						perror("WriteBytes");
						return result;
					}
					ATX_List_RemoveItem(self->input.packets, item);
					BLT_MediaPacket_Release(input);
			}
	}
	if(self->input.eos&&((RING_ALLOCATE_SIZE-size-1)>0)){
			result = MP3Decoder_DecodeFrame(self, packet);
			if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;
			if (FLO_ERROR_IS_FATAL(result)) {
				return result;
			}    
	}else if(self->input.eos) {
		result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core,
		                                    0,
		                                    (const BLT_MediaType*)&self->output.media_type,
		                                    packet);
		if (BLT_FAILED(result)) return result;
		BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_END_OF_STREAM);
		BLT_MediaPacket_SetTimeStamp(*packet, self->output.time_stamp);
		self->output.eos = BLT_TRUE;
        	return BLT_SUCCESS;
	}else if(RING_ALLOCATE_SIZE-size-1>1024){
			result = MP3Decoder_DecodeFrame(self, packet);
			if (BLT_SUCCEEDED(result)) return BLT_SUCCESS;
			if (FLO_ERROR_IS_FATAL(result)) {
				return result;
			}             
	}else
    		return BLT_ERROR_PORT_HAS_NO_DATA;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MP3DecoderOutput)
    ATX_GET_INTERFACE_ACCEPT(MP3DecoderOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(MP3DecoderOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(MP3DecoderOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(MP3DecoderOutput, BLT_MediaPort)
    MP3DecoderOutput_GetName,
    MP3DecoderOutput_GetProtocol,
    MP3DecoderOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MP3DecoderOutput, BLT_PacketProducer)
    MP3DecoderOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MP3Decoder_SetupPorts
+---------------------------------------------------------------------*/
static BLT_Result
MP3Decoder_SetupPorts(MP3Decoder* self)
{
    ATX_Result result;

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
|    MP3Decoder_Create
+---------------------------------------------------------------------*/
static BLT_Result
MP3Decoder_Create(BLT_Module*              module,
                        BLT_Core*                core, 
                        BLT_ModuleParametersType parameters_type,
                        BLT_CString              parameters, 
                        BLT_MediaNode**          object)
{
    MP3Decoder* self;
    BLT_Result        result;
    ATX_LOG_FINE("MpegAudioDecoder::Create");
    result=MP3_Mp3Init();
    if(ATX_FAILED(result)){
		perror("MpegAudio_Mp3Init");
		return result;
	}
    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(MP3Decoder));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* create the fluo decoder */
#if 0
    result = FLO_Decoder_Create(&self->fluo);
    if (FLO_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }
#endif
    /* setup the input and output ports */
    result = MP3Decoder_SetupPorts(self);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        *object = NULL;
        return result;
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, MP3Decoder, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, MP3Decoder, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  MP3DecoderInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  MP3DecoderInput,  BLT_PacketConsumer);
    ATX_SET_INTERFACE(&self->output, MP3DecoderOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, MP3DecoderOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    MP3Decoder_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
MP3Decoder_Destroy(MP3Decoder* self)
{ 
    ATX_ListItem* item;
    BLT_Result        result;
    ATX_LOG_FINE("MpegAudioDecoder::Destroy");
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
    
    /* destroy the fluo decoder */
//FLO_Decoder_Destroy(self->fluo);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));
    ATX_FreeMemory(adp.hDecoder);
    ATX_FreeMemory(adp.Ring);
    ATX_FreeMemory(adp.audec_buf);
    result=audec->uninit(&adp);
    if(BLT_FAILED(result)){
		return BLT_FAILURE;
    }
    if(audec != NULL){
	   audec = NULL;
    }
   // ATX_FreeMemory(pcm_buf);
   ATX_SetMemory(pcm_buf, 0, RING_ALLOCATE_SIZE);
    init_flag=1;
    seek_flag=0;
 /* free the object memory */
	ATX_FreeMemory(self);
 printf("--------destroy----------\n");
    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   MP3Decoder_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
MP3Decoder_GetPortByName(BLT_MediaNode*  _self,
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
    MP3Decoder* self = ATX_SELF_EX(MP3Decoder, BLT_BaseMediaNode, BLT_MediaNode);
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
|    MP3Decoder_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
MP3Decoder_Seek(BLT_MediaNode* _self,
                      BLT_SeekMode*  mode,
                      BLT_SeekPoint* point)
{
	MP3Decoder* self = ATX_SELF_EX(MP3Decoder, BLT_BaseMediaNode, BLT_MediaNode);
	/* flush pending input packets */
	MP3DecoderInput_Flush(self);
	ATX_SetMemory(pcm_buf, 0, RING_ALLOCATE_SIZE);
	ATX_SetMemory(adp.Ring, 0, RING_ALLOCATE_SIZE);
	adp.RingWI=adp.RingRI=0;	
	/* clear the eos flag */
	self->input.eos  = BLT_FALSE;
	self->output.eos = BLT_FALSE;
	/* estimate the seek point in time_stamp mode */
	if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
	BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
	if (!(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
	    return BLT_FAILURE;
	}
	/* update the decoder's sample position */
	self->output.sample_count = point->sample;
	self->output.time_stamp = point->time_stamp;
	seek_flag=1;
	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MP3Decoder)
    ATX_GET_INTERFACE_ACCEPT_EX(MP3Decoder, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(MP3Decoder, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(MP3Decoder, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    MP3Decoder_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    MP3Decoder_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(MP3Decoder, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   MP3DecoderModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
MP3DecoderModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    MP3DecoderModule* self = ATX_SELF_EX(MP3DecoderModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*           registry;
    BLT_Result              result;
    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the .mp2, .mp1, .mp3 .mpa and .mpg file extensions */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp3",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp2",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp1",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mpa",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mpg",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;

    /* register the "audio/mpeg" type */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mpeg",
        &self->mpeg_audio_type_id);
    if (BLT_FAILED(result)) return result;
    /* register mime type aliases */
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mp3", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mp3", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mpg", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpg", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpeg", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mpeg3", self->mpeg_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpeg3", self->mpeg_audio_type_id);

    ATX_LOG_FINE_1("MP3DecoderModule::Attach (audio/mpeg type = %d)", self->mpeg_audio_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   MP3DecoderModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
MP3DecoderModule_Probe(BLT_Module*              _self, 
                             BLT_Core*                core,
                             BLT_ModuleParametersType parameters_type,
                             BLT_AnyConst             parameters,
                             BLT_Cardinal*            match)
{
    MP3DecoderModule* self = ATX_SELF_EX(MP3DecoderModule, BLT_BaseModule, BLT_Module);
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

            /* the input type should be audio/mpeg */
            if (constructor->spec.input.media_type->id != 
                self->mpeg_audio_type_id) {
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
                if (ATX_StringsEqual(constructor->name, "MP3Decoder")) {
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

            ATX_LOG_FINE_1("MpegAudioDecoderModule::Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MP3DecoderModule)
    ATX_GET_INTERFACE_ACCEPT_EX(MP3DecoderModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(MP3DecoderModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(MP3DecoderModule, MP3Decoder)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(MP3DecoderModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    MP3DecoderModule_Attach,
    MP3DecoderModule_CreateInstance,
    MP3DecoderModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define MP3DecoderModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(MP3DecoderModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(MP3DecoderModule,
                                         "MP3 Decoder",
                                         "com.axiosys.decoder.mpeg-audio",
                                         "1.4.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
