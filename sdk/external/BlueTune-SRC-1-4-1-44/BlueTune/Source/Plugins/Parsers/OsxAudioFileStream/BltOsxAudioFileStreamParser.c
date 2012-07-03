/*****************************************************************
|
|   BlueTune - OSX AudioFileStream Parser Module
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
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"

#include <AudioToolbox/AudioToolbox.h>

/*----------------------------------------------------------------------
|  logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.audio-file-stream")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_OSX_AUDIO_FILE_STREAM_PARSER_FTYP_M4A 0x4d344120

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_MediaType               base;
    AudioStreamBasicDescription asbd;
    unsigned int                magic_cookie_size;
    unsigned char               magic_cookie[1];
    // followed by zero or more magic_cookie bytes
} AsbdMediaType;

typedef struct {
    BLT_MediaTypeId asbd;
    BLT_MediaTypeId audio_mp4;
    BLT_MediaTypeId audio_mp3;
} OsxAudioFileStreamParserMediaTypeIds;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    UInt32*                              supported_formats;
    unsigned int                         supported_format_count;
    OsxAudioFileStreamParserMediaTypeIds media_type_ids;
} OsxAudioFileStreamParserModule;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    ATX_InputStream* stream;
    BLT_Boolean      eos;
    UInt32           type_hint;
    ATX_DataBuffer*  ftyp;
    BLT_Boolean      ftyp_loaded;
} OsxAudioFileStreamParserInput;

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    AsbdMediaType* media_type;
    ATX_List*      packets;
} OsxAudioFileStreamParserOutput;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    OsxAudioFileStreamParserModule* module;
    OsxAudioFileStreamParserInput   input;
    OsxAudioFileStreamParserOutput  output;
    AudioFileStreamID               stream_parser;
    BLT_StreamInfo                  stream_info;
} OsxAudioFileStreamParser;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(OsxAudioFileStreamParserModule, BLT_Module)
ATX_DECLARE_INTERFACE_MAP(OsxAudioFileStreamParser,       BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(OsxAudioFileStreamParser,       ATX_Referenceable)

/*----------------------------------------------------------------------
|    OsxAudioFileStreamParserModule_FormatIsSupported
+---------------------------------------------------------------------*/
static BLT_Boolean
OsxAudioFileStreamParserModule_FormatIsSupported(OsxAudioFileStreamParserModule* self, 
                                                 UInt32                          format)
{
    unsigned int i;
    if (self->supported_formats == NULL) return BLT_TRUE; /* assume */
    for (i=0; i<self->supported_format_count; i++) {
        if (self->supported_formats[i] == format) return BLT_TRUE;
    }
    return BLT_FALSE;
}

/*----------------------------------------------------------------------
|    OsxAudioFileStreamParser_UpdateStreamInfo
+---------------------------------------------------------------------*/
static void
OsxAudioFileStreamParser_UpdateStreamInfo(OsxAudioFileStreamParser* self)
{
    UInt32 property_32 = 0;
    UInt64 property_64 = 0;
    UInt32 property_size = sizeof(property_32);
    if (!(self->stream_info.mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE)) {
        if (AudioFileStreamGetProperty(self->stream_parser, 
                                       kAudioFileStreamProperty_BitRate, 
                                       &property_size, 
                                       &property_32) == noErr) {
            self->stream_info.nominal_bitrate = property_32;
            self->stream_info.mask |= BLT_STREAM_INFO_MASK_NOMINAL_BITRATE;
        }
    }
    
    /* update the duration */
    property_size = sizeof(property_64);
    if (AudioFileStreamGetProperty(self->stream_parser, 
                                   kAudioFileStreamProperty_AudioDataPacketCount, 
                                   &property_size, 
                                   &property_64) == noErr) {
        UInt64 frame_count = property_64*(UInt64)self->output.media_type->asbd.mFramesPerPacket;
        if ((UInt64)self->output.media_type->asbd.mSampleRate) {
            self->stream_info.duration = ((UInt64)1000*frame_count)/(UInt64)self->output.media_type->asbd.mSampleRate;
            self->stream_info.mask |= BLT_STREAM_INFO_MASK_DURATION;
        }
    } else if ((self->stream_info.mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) &&
               (self->stream_info.mask & BLT_STREAM_INFO_MASK_SIZE)            &&
               self->stream_info.nominal_bitrate) {
        self->stream_info.duration = ((UInt64)8000*(UInt64)self->stream_info.size) /
                                      (UInt64)self->stream_info.nominal_bitrate;
        self->stream_info.mask |= BLT_STREAM_INFO_MASK_DURATION;
    }
    if ((self->stream_info.mask & BLT_STREAM_INFO_MASK_SIZE)     &&
        (self->stream_info.mask & BLT_STREAM_INFO_MASK_DURATION) &&
         self->stream_info.duration != 0) {
        self->stream_info.average_bitrate = 8000.0*(double)self->stream_info.size/(double)self->stream_info.duration;
        self->stream_info.mask |= BLT_STREAM_INFO_MASK_AVERAGE_BITRATE;
    }
}

/*----------------------------------------------------------------------
|    OsxAudioFileStreamParser_OnProperty
+---------------------------------------------------------------------*/
static void 
OsxAudioFileStreamParser_OnProperty(void*                     _self, 
                                    AudioFileStreamID         stream,
                                    AudioFileStreamPropertyID property_id,
                                    UInt32*                   flags)
{
    OsxAudioFileStreamParser* self = (OsxAudioFileStreamParser*)_self;
    UInt32                    property_size = 0;
    OSStatus                  result;
    ATX_COMPILER_UNUSED(flags);
    
    ATX_LOG_FINE_4("stream property %c%c%c%c", 
                   (property_id>>24)&0xFF, 
                   (property_id>>16)&0xFF,
                   (property_id>> 8)&0xFF, 
                   (property_id    )&0xFF);
                   
    switch (property_id) {
        case kAudioFileStreamProperty_FileFormat: {
            UInt32 property_value = 0;
            property_size = sizeof(property_value);
            if (AudioFileStreamGetProperty(stream, property_id, &property_size, &property_value) == noErr) {
                ATX_LOG_FINE_1("kAudioFileStreamProperty_FileFormat = %x", property_value);
            }
            break;
        }

        case kAudioFileStreamProperty_DataFormat: 
        case kAudioFileStreamProperty_FormatList:
        case kAudioFileStreamProperty_MagicCookieData:
        case kAudioFileStreamProperty_AudioDataByteCount:
        case kAudioFileStreamProperty_AudioDataPacketCount:
        case kAudioFileStreamProperty_BitRate:
            /* ask the parser to cache the property */
            *flags |= kAudioFileStreamPropertyFlag_CacheProperty;
            break;
            
        case kAudioFileStreamProperty_ReadyToProducePackets:
        {
            unsigned int media_type_size = sizeof(AsbdMediaType);
            
            /* get the magic cookie size */
            result = AudioFileStreamGetPropertyInfo(stream, kAudioFileStreamProperty_MagicCookieData, &property_size, NULL);
            if (result == noErr) {
                ATX_LOG_FINE_1("magic cookie size=%d", property_size);
            } else {
                ATX_LOG_FINE("no magic cookie");
                property_size = 0;
            }
            
            /* allocate the media type */
            if (property_size > 1) {
                media_type_size += property_size-1;
            }
            self->output.media_type = (AsbdMediaType*)ATX_AllocateZeroMemory(media_type_size);
            BLT_MediaType_InitEx(&self->output.media_type->base, self->module->media_type_ids.asbd, media_type_size);
            self->output.media_type->magic_cookie_size = property_size;
            
            /* copy the magic cookie if there is one */
            if (property_size) {
                result = AudioFileStreamGetProperty(stream, kAudioFileStreamProperty_MagicCookieData, &property_size, self->output.media_type->magic_cookie);
                if (result != noErr) {
                    ATX_LOG_WARNING_1("AudioFileStreamGetProperty failed (%d)", result);
                    ATX_FreeMemory(self->output.media_type);
                    self->output.media_type = NULL;
                    return;
                }
            }
            
            /* iterate the format list if one is available */
            result = AudioFileStreamGetPropertyInfo(stream, kAudioFileStreamProperty_FormatList, &property_size, NULL);
            if (result == noErr && property_size) {
                AudioFormatListItem* items = (AudioFormatListItem*)malloc(property_size);
                result = AudioFileStreamGetProperty(stream, kAudioFileStreamProperty_FormatList, &property_size, items);
                int item_count = property_size/sizeof(items[0]);
                int i;
                for (i=0; i<item_count; i++) {
                    ATX_LOG_FINE_7("format %d: %c%c%c%c, %d %d", 
                                   i,
                                   (items[i].mASBD.mFormatID>>24)&0xFF,
                                   (items[i].mASBD.mFormatID>>16)&0xFF,
                                   (items[i].mASBD.mFormatID>> 8)&0xFF,
                                   (items[i].mASBD.mFormatID    )&0xFF,
                                   (int)items[i].mASBD.mSampleRate,
                                   items[i].mASBD.mChannelsPerFrame);
                    if (items[i].mASBD.mFormatID == kAudioFormatMPEG4AAC_HE && 
                        OsxAudioFileStreamParserModule_FormatIsSupported(self->module, items[i].mASBD.mFormatID)) {
                        ATX_LOG_FINE("selecting kAudioFormatMPEG4AAC_HE");
                        self->output.media_type->asbd = items[i].mASBD;
                        break;
                    }
                    if (items[i].mASBD.mFormatID == kAudioFormatMPEG4AAC_HE_V2 &&
                        OsxAudioFileStreamParserModule_FormatIsSupported(self->module, items[i].mASBD.mFormatID)) {
                        ATX_LOG_FINE("selecting kAudioFormatMPEG4AAC_HE_V2");
                        self->output.media_type->asbd = items[i].mASBD;
                        break;
                    }
                }
                ATX_FreeMemory(items);
            }
            
            /* get the audio description if none was selected from the list previously */
            if (self->output.media_type->asbd.mFormatID == 0) {
                property_size = sizeof(AudioStreamBasicDescription);
                result = AudioFileStreamGetProperty(stream, kAudioFileStreamProperty_DataFormat, &property_size, &self->output.media_type->asbd);
                if (result != noErr) {
                    ATX_LOG_WARNING_1("AudioFileStreamGetProperty failed (%d)", result);
                    ATX_FreeMemory(self->output.media_type);
                    self->output.media_type = NULL;
                    return;
                }
                ATX_LOG_FINE_6("kAudioFileStreamProperty_DataFormat: %c%c%c%c, %d %d", 
                               (self->output.media_type->asbd.mFormatID>>24)&0xFF,
                               (self->output.media_type->asbd.mFormatID>>16)&0xFF,
                               (self->output.media_type->asbd.mFormatID>> 8)&0xFF,
                               (self->output.media_type->asbd.mFormatID    )&0xFF,
                               (int)self->output.media_type->asbd.mSampleRate,
                               self->output.media_type->asbd.mChannelsPerFrame);
            }
            
            /* get some info about the format */
            {
                CFStringRef name = NULL;
                char        buffer[1024];
                UInt64      property_64;
                
                self->stream_info.type          = BLT_STREAM_TYPE_AUDIO;
                self->stream_info.channel_count = self->output.media_type->asbd.mChannelsPerFrame;
                self->stream_info.sample_rate   = (ATX_UInt32)self->output.media_type->asbd.mSampleRate;
                self->stream_info.data_type     = "";
                self->stream_info.mask         |= BLT_STREAM_INFO_MASK_TYPE          |
                                                  BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
                                                  BLT_STREAM_INFO_MASK_SAMPLE_RATE   |
                                                  BLT_STREAM_INFO_MASK_DATA_TYPE;
                
                switch (self->output.media_type->asbd.mFormatID) {
                    case kAudioFormatAppleLossless:  self->stream_info.data_type = "Apple Lossless";       break;
                    case kAudioFormatAC3:            self->stream_info.data_type = "Dolby AC-3";           break;
                    case kAudioFormatMPEG4AAC:       self->stream_info.data_type = "AAC";                  break;
                    case kAudioFormatMPEG4AAC_HE:    self->stream_info.data_type = "He-AAC";               break;
                    case kAudioFormatMPEG4AAC_HE_V2: self->stream_info.data_type = "He-AAC v2";            break;
                    case kAudioFormatMPEG4AAC_LD:    self->stream_info.data_type = "AAC Low Delay";        break;
                    case kAudioFormatMPEGLayer1:     self->stream_info.data_type = "MPEG-1 Audio Layer 1"; break;
                    case kAudioFormatMPEGLayer2:     self->stream_info.data_type = "MPEG-1 Audio Layer 2"; break;
                    case kAudioFormatMPEGLayer3:     self->stream_info.data_type = "MPEG-1 Audio Layer 3"; break;
                    default:
                        property_size = sizeof(CFStringRef);
                        result = AudioFormatGetProperty(kAudioFormatProperty_FormatName, 
                                                        sizeof(self->output.media_type->asbd), 
                                                        &self->output.media_type->asbd, 
                                                        &property_size,
                                                        &name);
                        if (result == noErr) {
                            if (CFStringGetCString(name, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
                                self->stream_info.data_type = buffer;
                            }
                        }
                        break;
                }
                
                property_size = sizeof(UInt64);
                if (AudioFileStreamGetProperty(stream, kAudioFileStreamProperty_AudioDataByteCount, &property_size, &property_64) == noErr) {
                    self->stream_info.size = property_64;
                    self->stream_info.mask |= BLT_STREAM_INFO_MASK_SIZE;
                }
                                
                OsxAudioFileStreamParser_UpdateStreamInfo(self);
                BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &self->stream_info);
            }
            break;
        }
    }
}

/*----------------------------------------------------------------------
|    OsxAudioFileStreamParser_OnPacket
+---------------------------------------------------------------------*/
static void 
OsxAudioFileStreamParser_OnPacket(void*                         _self,
                                  UInt32                        number_of_bytes,
                                  UInt32                        number_of_packets,
                                  const void*                   data,
                                  AudioStreamPacketDescription* packet_descriptions)
{
    OsxAudioFileStreamParser* self = (OsxAudioFileStreamParser*)_self;
    
    ATX_LOG_FINER_2("new packet data, size=%d, count=%d", number_of_bytes, number_of_packets);
    if (self->output.media_type == NULL) return; 
    unsigned int i;
    for (i=0; i<number_of_packets; i++) {
        // create a media packet
        BLT_MediaPacket*     packet = NULL;
        const unsigned char* packet_data = data;
        BLT_Result           result;
        result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core, 
                                            packet_descriptions[i].mDataByteSize,
                                            &self->output.media_type->base,
                                            &packet);
        if (BLT_FAILED(result)) return;
        
        /* copy the packet payload */
        ATX_CopyMemory(BLT_MediaPacket_GetPayloadBuffer(packet), 
                       packet_data+packet_descriptions[i].mStartOffset,
                       packet_descriptions[i].mDataByteSize);
        BLT_MediaPacket_SetPayloadSize(packet, packet_descriptions[i].mDataByteSize);
        
        /* compute the packet duration */
        if (self->output.media_type->asbd.mFramesPerPacket &&
            self->output.media_type->asbd.mSampleRate != 0.0) {
            BLT_Time packet_duration = BLT_TimeStamp_FromSeconds(
                (double)self->output.media_type->asbd.mFramesPerPacket/
                        self->output.media_type->asbd.mSampleRate);
            BLT_MediaPacket_SetDuration(packet, packet_duration);
        }
                
        /* add the packet to the queue */
        ATX_List_AddData(self->output.packets, packet);
    }
    
    /* update the stream info */
    OsxAudioFileStreamParser_UpdateStreamInfo(self);
    BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &self->stream_info);
}

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserInput_Setup
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioFileStreamParserInput_Setup(OsxAudioFileStreamParser* self, 
                                    BLT_MediaTypeId           media_type_id)
{
    OSStatus status;
    
    /* reset the ftyp buffer */
    ATX_DataBuffer_SetDataSize(self->input.ftyp, 0);
    
    /* decide what the media format is */
    if (media_type_id == self->module->media_type_ids.audio_mp4) {
        ATX_LOG_FINE("packet type is audio/mp4");
        self->input.type_hint = kAudioFileM4AType;
    } else if (media_type_id == self->module->media_type_ids.audio_mp3) {
        ATX_LOG_FINE("packet type is audio/mp3");
        self->input.type_hint = kAudioFileMP3Type;
    } else {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    
    /* create the stream parser */
    status = AudioFileStreamOpen(self,
                                 OsxAudioFileStreamParser_OnProperty,
                                 OsxAudioFileStreamParser_OnPacket,
                                 self->input.type_hint,
                                 &self->stream_parser);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioFileStreamOpen failed (%d)", status);
        return BLT_FAILURE;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParserInput_SetStream(BLT_InputStreamUser* _self,
                                        ATX_InputStream*     stream,
                                        const BLT_MediaType* media_type)
{
    OsxAudioFileStreamParser* self = ATX_SELF_M(input, OsxAudioFileStreamParser, BLT_InputStreamUser);

    /* check the media type */
    if (media_type == NULL || 
        (media_type->id != self->module->media_type_ids.audio_mp4 &&
         media_type->id != self->module->media_type_ids.audio_mp3)) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    
    /* keep a reference to the stream */
    if (self->input.stream) ATX_RELEASE_OBJECT(self->input.stream);
    self->input.stream = stream;
    ATX_REFERENCE_OBJECT(stream);
    
    /* clear the current media type if we have one */
    if (self->output.media_type) {
        ATX_FreeMemory(self->output.media_type);
        self->output.media_type = NULL;
    }
    
    /* clear the stream info */
    ATX_SetMemory(&self->stream_info, 0, sizeof(self->stream_info));
    
    /* record the size */
    {
        ATX_LargeSize size = 0;
        if (ATX_SUCCEEDED(ATX_InputStream_GetSize(stream, &size))) {
            self->stream_info.size = size;
            self->stream_info.mask |= BLT_STREAM_INFO_MASK_SIZE;
        }
    }
    
    /* create the stream parser */
    return OsxAudioFileStreamParserInput_Setup(self, media_type->id);
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioFileStreamParserInput)
    ATX_GET_INTERFACE_ACCEPT(OsxAudioFileStreamParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(OsxAudioFileStreamParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioFileStreamParserInput, BLT_InputStreamUser)
    OsxAudioFileStreamParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(OsxAudioFileStreamParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(OsxAudioFileStreamParserInput, BLT_MediaPort)
    OsxAudioFileStreamParserInput_GetName,
    OsxAudioFileStreamParserInput_GetProtocol,
    OsxAudioFileStreamParserInput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserOutput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioFileStreamParserOutput_Flush(OsxAudioFileStreamParser* self)
{
    ATX_ListItem* item;
    while ((item = ATX_List_GetFirstItem(self->output.packets))) {
        BLT_MediaPacket* packet = ATX_ListItem_GetData(item);
        if (packet) BLT_MediaPacket_Release(packet);
        ATX_List_RemoveItem(self->output.packets, item);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParserOutput_GetPacket(BLT_PacketProducer* _self,
                                         BLT_MediaPacket**   packet)
{
    OsxAudioFileStreamParser* self = ATX_SELF_M(output, OsxAudioFileStreamParser, BLT_PacketProducer);
    ATX_ListItem*             packet_item;
    OSStatus                  status;

    /* default return */
    *packet = NULL;

    /* check that we have a stream */
    if (self->input.stream == NULL) return BLT_ERROR_INTERNAL;
        
    /* pump data from the source until packets start showing up */
    while ((packet_item = ATX_List_GetFirstItem(self->output.packets)) == NULL) {
        ATX_UInt8  buffer[4096];
        ATX_Size   bytes_read = 0;
        ATX_UInt8* payload = buffer; 
        ATX_Size   payload_size = 0;
        BLT_Result result = ATX_InputStream_Read(self->input.stream, buffer, sizeof(buffer), &bytes_read);
        if (BLT_FAILED(result)) return result;
        
        if (self->input.type_hint == kAudioFileM4AType && !self->input.ftyp_loaded) {
            /* we're still buffering the ftyp atom */
            payload_size = ATX_DataBuffer_GetDataSize(self->input.ftyp);
            ATX_DataBuffer_SetDataSize(self->input.ftyp, payload_size+bytes_read);
            payload = (ATX_UInt8*)ATX_DataBuffer_UseData(self->input.ftyp);
            ATX_CopyMemory(payload+payload_size, buffer, bytes_read);
            payload_size += bytes_read;
            if (payload_size >= 8) {
                if (payload[4] == 'f' && payload[5] == 't' && payload[6] == 'y' && payload[7] == 'p') {
                    ATX_UInt32 ftyp_size = ATX_BytesToInt32Be(payload);
                    if (ftyp_size < 8) {
                        /* 64-bit atom, just ignore it */
                        self->input.ftyp_loaded = BLT_TRUE;
                    } else if (payload_size >= ftyp_size) {
                        /* the entire atom has been buffered, we can parse an patch the 'ftyp' atom      */
                        /* this is a horrible hack that's designed to work around a problem with the     */
                        /* Apple parser that refuses to expose the packet count property when  the major */
                        /* brand is not 'M4A ' and when there is a compatible brand that's '3gXX' where  */
                        /* XX is not 'p2' or 'p3'... go figure!                                          */
                        ATX_UInt32 major_brand = ATX_BytesToInt32Be(payload+8);
                        if (major_brand != BLT_OSX_AUDIO_FILE_STREAM_PARSER_FTYP_M4A) {
                            ATX_LOG_FINE("major brand is not M4A");
                            payload[ 8] = 'M';
                            payload[ 9] = '4';
                            payload[10] = 'A';
                            payload[11] = ' ';
                            payload[12] = 0;
                            payload[13] = 0;
                            payload[14] = 0;
                            payload[15] = 0;
                        }
                        if (20 <= ftyp_size) {
                            ATX_UInt8* brands = payload+16;
                            ftyp_size -= 16;
                            for (; ftyp_size >= 4; ftyp_size -= 4, brands += 4) {
                                /* patch all '3gXX' compatible brands */
                                if (brands[0] == '3' && brands[1] == 'g') {
                                    ATX_LOG_FINE_2("patching 3g%c%c -> 'M4A '", brands[2], brands[3]);
                                    brands[0] = 'M';
                                    brands[1] = '4';
                                    brands[2] = 'A';
                                    brands[3] = ' ';
                                }
                            }
                        }
                        self->input.ftyp_loaded = BLT_TRUE;
                    }
                }
            } else {
                continue;
            }
        } else {
            payload_size = bytes_read;
        }
        status = AudioFileStreamParseBytes(self->stream_parser,
                                           payload_size,
                                           payload,
                                           0);
        if (status != noErr) {
            ATX_LOG_WARNING_1("AudioFileStreamParseBytes failed (%x)", status);
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }            
    }

    *packet = (BLT_MediaPacket*)ATX_ListItem_GetData(packet_item);
    ATX_List_RemoveItem(self->output.packets, packet_item);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioFileStreamParserOutput)
    ATX_GET_INTERFACE_ACCEPT(OsxAudioFileStreamParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(OsxAudioFileStreamParserOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(OsxAudioFileStreamParserOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(OsxAudioFileStreamParserOutput, BLT_MediaPort)
    OsxAudioFileStreamParserOutput_GetName,
    OsxAudioFileStreamParserOutput_GetProtocol,
    OsxAudioFileStreamParserOutput_GetDirection,
    BLT_MediaPort_DefaultQueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioFileStreamParserOutput, BLT_PacketProducer)
    OsxAudioFileStreamParserOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    OsxAudioFileStreamParser_Create
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioFileStreamParser_Create(BLT_Module*              module,
                                BLT_Core*                core, 
                                BLT_ModuleParametersType parameters_type,
                                BLT_CString              parameters, 
                                BLT_MediaNode**          object)
{
    OsxAudioFileStreamParser*       self;
    OsxAudioFileStreamParserModule* parser_module = (OsxAudioFileStreamParserModule*)module;
    BLT_Result                      result;
    
    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(OsxAudioFileStreamParser));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* keep a pointer to our module class */
    self->module = parser_module;
    
    /* setup the input and output ports */
    self->input.eos = BLT_FALSE;

    /* create a buffer for the ftyp atom */
    ATX_DataBuffer_Create(0, &self->input.ftyp);

    /* create a list of input packets */
    result = ATX_List_Create(&self->output.packets);
    if (ATX_FAILED(result)) {
        ATX_FreeMemory(self);
        return result;
    }
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, OsxAudioFileStreamParser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, OsxAudioFileStreamParser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  OsxAudioFileStreamParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  OsxAudioFileStreamParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, OsxAudioFileStreamParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, OsxAudioFileStreamParserOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioFileStreamParser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioFileStreamParser_Destroy(OsxAudioFileStreamParser* self)
{ 
    /* release any packet we may hold */
    OsxAudioFileStreamParserOutput_Flush(self);
    ATX_List_Destroy(self->output.packets);
    BLT_MediaType_Free((BLT_MediaType*)self->output.media_type);
    
    /* release the input stream if we have one */
    if (self->input.stream) ATX_RELEASE_OBJECT(self->input.stream);
    
    /* close the stream parser */
    if (self->stream_parser) AudioFileStreamClose(self->stream_parser);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                    
/*----------------------------------------------------------------------
|   OsxAudioFileStreamParser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParser_GetPortByName(BLT_MediaNode*  _self,
                                       BLT_CString     name,
                                       BLT_MediaPort** port)
{
    OsxAudioFileStreamParser* self = ATX_SELF_EX(OsxAudioFileStreamParser, BLT_BaseMediaNode, BLT_MediaNode);

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
|    OsxAudioFileStreamParser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParser_Seek(BLT_MediaNode* _self,
                              BLT_SeekMode*  mode,
                              BLT_SeekPoint* point)
{
    OsxAudioFileStreamParser* self = ATX_SELF_EX(OsxAudioFileStreamParser, BLT_BaseMediaNode, BLT_MediaNode);
    OSStatus                  status;
    BLT_Result                result;
    SInt64                    byte_offset = 0;
    SInt64                    audio_offset = 0;
    SInt64                    packet_offset;
    UInt32                    io_flags = 0;
    UInt32                    property_size;
    
    /* clear the eos flag */
    self->input.eos = BLT_FALSE;

    /* remove any packets in the output list */
    OsxAudioFileStreamParserOutput_Flush(self);

    /* estimate the seek point in sample mode */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_SAMPLE)) {
        return BLT_ERROR_NOT_SUPPORTED;
    }
    
    /* if we don't know the duration or the frames per packet, seek based on position */
    if (self->output.media_type->asbd.mFramesPerPacket == 0 ||
        (self->stream_info.mask & BLT_STREAM_INFO_MASK_DURATION) == 0) {
        /* perform the seek */
        result = ATX_InputStream_Seek(self->input.stream, point->offset);
        if (BLT_FAILED(result)) return result;
        
        /* set the mode so that the nodes down the chain know the seek has */
        /* already been done on the stream                                 */
        *mode = BLT_SEEK_MODE_IGNORE;        
        return BLT_SUCCESS;
    }
    
    /* compute the packet offset for the seek point */
    packet_offset = point->sample/self->output.media_type->asbd.mFramesPerPacket;
    
    /* compute the byte offset for the seek point */
    status = AudioFileStreamSeek(self->stream_parser, packet_offset, &byte_offset, &io_flags);
    if (status != noErr) return BLT_ERROR_NOT_SUPPORTED;
    
    /* the byte offset returned by the previous call is from the audio data origin */
    /* so we need to get that value from the stream                                */
    property_size = sizeof(audio_offset);
    status = AudioFileStreamGetProperty(self->stream_parser, kAudioFileStreamProperty_DataOffset, &property_size, &audio_offset);
    if (status != noErr) return BLT_ERROR_NOT_SUPPORTED;
    
    /* perform the seek */
    result = ATX_InputStream_Seek(self->input.stream, audio_offset+byte_offset);
    if (BLT_FAILED(result)) return result;
    
    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                 */
    *mode = BLT_SEEK_MODE_IGNORE;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioFileStreamParser)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioFileStreamParser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioFileStreamParser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OsxAudioFileStreamParser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    OsxAudioFileStreamParser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    OsxAudioFileStreamParser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OsxAudioFileStreamParser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserModule_Destroy
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParserModule_Destroy(OsxAudioFileStreamParserModule* self)
{
    if (self->supported_formats) ATX_FreeMemory(self->supported_formats);
    return BLT_BaseModule_Destroy(&ATX_BASE(self, BLT_BaseModule));
}

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    OsxAudioFileStreamParserModule* self = ATX_SELF_EX(OsxAudioFileStreamParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*                   registry;
    BLT_Result                      result;
    
    /* get a list of supported formats */
    {
        UInt32 property_size = 0;
        OSErr  status = AudioFormatGetPropertyInfo(kAudioFormatProperty_DecodeFormatIDs, 0, NULL, &property_size);
        if (status == noErr && property_size && (property_size%sizeof(UInt32)) == 0) {
            unsigned int i;
            self->supported_formats = (UInt32*)malloc(property_size);
            self->supported_format_count = property_size/sizeof(UInt32);
            status = AudioFormatGetProperty(kAudioFormatProperty_DecodeFormatIDs, 
                                            0, 
                                            NULL,
                                            &property_size, 
                                            self->supported_formats);
            if (status != noErr) {
                ATX_FreeMemory(self->supported_formats);
                self->supported_formats = NULL;
            }
            for (i=0; i<self->supported_format_count; i++) {
                ATX_LOG_FINE_5("supported format %d: %c%c%c%c", 
                               i, 
                               (self->supported_formats[i]>>24)&0xFF,
                               (self->supported_formats[i]>>16)&0xFF,
                               (self->supported_formats[i]>> 8)&0xFF,
                               (self->supported_formats[i]    )&0xFF);
            }
        }
    }
    
    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".mp4" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp4",
                                            "audio/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the ".m4a" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".m4a",
                                            "audio/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the "mp3" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp3",
                                            "audio/mpeg");
    if (BLT_FAILED(result)) return result;

    /* register the "audio/mpeg" type */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mpeg",
        &self->media_type_ids.audio_mp3);
    if (BLT_FAILED(result)) return result;
    
    /* register mime type aliases */
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mp3", self->media_type_ids.audio_mp3);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mp3", self->media_type_ids.audio_mp3);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mpg", self->media_type_ids.audio_mp3);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpg", self->media_type_ids.audio_mp3);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpeg", self->media_type_ids.audio_mp3);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/mpeg3", self->media_type_ids.audio_mp3);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/x-mpeg3", self->media_type_ids.audio_mp3);

    /* get the type id for "audio/mp4" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mp4",
        &self->media_type_ids.audio_mp4);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1("audio/mp4 type = %d", self->media_type_ids.audio_mp4);

    /* get the type id for "audio/mp3" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mp3",
        &self->media_type_ids.audio_mp3);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1("audio/mp3 type = %d", self->media_type_ids.audio_mp3);

    /* register the audio/x-apple-asbd type id */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/x-apple-asbd",
        &self->media_type_ids.asbd);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("audio/x-apple-asbd type = %d", self->media_type_ids.asbd);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   OsxAudioFileStreamParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioFileStreamParserModule_Probe(BLT_Module*              _self, 
                                     BLT_Core*                core,
                                     BLT_ModuleParametersType parameters_type,
                                     BLT_AnyConst             parameters,
                                     BLT_Cardinal*            match)
{
    OsxAudioFileStreamParserModule* self = ATX_SELF_EX(OsxAudioFileStreamParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);
    
    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

            /* the input must be PACKET or STREAM_PULL and the output must be PACKET */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* the input type should be one of the supported types */
            if (constructor->spec.input.media_type->id != self->media_type_ids.audio_mp4 &&
                constructor->spec.input.media_type->id != self->media_type_ids.audio_mp3) {
                return BLT_FAILURE;
            }

            /* the output type should be unspecified, or audio/x-apple-asbd */
            if (!(constructor->spec.output.media_type->id == self->media_type_ids.asbd) &&
                !(constructor->spec.output.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "OsxAudioFileStreamParser")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not our name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 9;
            }

            ATX_LOG_FINE_1("Probe - Ok [%d]", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioFileStreamParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioFileStreamParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioFileStreamParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(OsxAudioFileStreamParserModule, OsxAudioFileStreamParser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OsxAudioFileStreamParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    OsxAudioFileStreamParserModule_Attach,
    OsxAudioFileStreamParserModule_CreateInstance,
    OsxAudioFileStreamParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OsxAudioFileStreamParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(OsxAudioFileStreamParserModule,
                                         "OSX AudioFileStream Parser",
                                         "com.axiosys.parsers.osx-audio-file-stream",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
