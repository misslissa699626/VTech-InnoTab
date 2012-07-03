/*****************************************************************
|
|   MP4 Parser Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4.h"
#include "Ap4AtomixAdapters.h"
#include "Atomix.h"
#include "BltConfig.h"
#include "BltMp4Parser.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"
#include "BltCommonMediaTypes.h"
#include "BltKeyManager.h"
#include "BltBento4Adapters.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.mp4")

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
struct Mp4ParserModule {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 mp4_audio_type_id;
    BLT_UInt32 mp4_video_type_id;
    BLT_UInt32 mp4_audio_es_type_id;
    BLT_UInt32 mp4_video_es_type_id;
    BLT_UInt32 iso_base_audio_es_type_id;
    BLT_UInt32 iso_base_video_es_type_id;
};

// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
struct Mp4ParserInput {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType     audio_media_type;
    BLT_MediaType     video_media_type;
    AP4_File*         mp4_file;
    bool              slow_seek;
    AP4_LinearReader* reader;
};

struct Mp4Parser; // forward declaration

// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
struct Mp4ParserOutput {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    Mp4Parser*           parser;
    BLT_UInt32           mp4_es_type_id;
    BLT_UInt32           iso_base_es_type_id;
    BLT_MediaType*       media_type;
    AP4_Track*           track;
    BLT_Ordinal          sample;
    AP4_DataBuffer*      sample_buffer;
    AP4_SampleDecrypter* sample_decrypter;
    AP4_DataBuffer*      sample_decrypted_buffer;
    AP4_Ordinal          sample_description_index;
};

// it is important to keep this structure a POD (no methods)
// because the strict compilers will not like use using
// the offsetof() macro necessary when using ATX_SELF()
struct Mp4Parser {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    Mp4ParserInput          input;
    Mp4ParserOutput         audio_output;
    Mp4ParserOutput         video_output;
    BLT_KeyManager*         key_manager;
    AP4_BlockCipherFactory* cipher_factory;
};

/*----------------------------------------------------------------------
|   Mp4ParserInput_Construct
+---------------------------------------------------------------------*/
static void
Mp4ParserInput_Construct(Mp4ParserInput* self, BLT_Module* module)
{
    Mp4ParserModule* mp4_parser_module = (Mp4ParserModule*)module;
    BLT_MediaType_Init(&self->audio_media_type, mp4_parser_module->mp4_audio_type_id);
    BLT_MediaType_Init(&self->video_media_type, mp4_parser_module->mp4_video_type_id);
    self->mp4_file  = NULL;
    self->reader    = NULL;
    self->slow_seek = false;
}

/*----------------------------------------------------------------------
|   Mp4ParserInput_Destruct
+---------------------------------------------------------------------*/
static void
Mp4ParserInput_Destruct(Mp4ParserInput* self)
{
    delete self->reader;
    delete self->mp4_file;
}

/*----------------------------------------------------------------------
|   Mp4ParserOutput_ProcessCryptoInfo
+---------------------------------------------------------------------*/
static BLT_Result
Mp4ParserOutput_ProcessCryptoInfo(Mp4ParserOutput*        self, 
                                  AP4_SampleDescription*& sample_desc)
{
    // check if the track is encrypted
    if (sample_desc->GetType() == AP4_SampleDescription::TYPE_PROTECTED) {
        ATX_LOG_FINE("track is encrypted");
        AP4_ProtectedSampleDescription* prot_desc = dynamic_cast<AP4_ProtectedSampleDescription*>(sample_desc);
        if (prot_desc == NULL) {
            ATX_LOG_FINE("unable to obtain cipher info");
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }

        // obtain the key manager
        if (self->parser->key_manager == NULL) {
            ATX_Properties* properties = NULL;
            if (BLT_SUCCEEDED(BLT_Core_GetProperties(ATX_BASE(self->parser, BLT_BaseMediaNode).core, 
                                                              &properties))) {
                ATX_PropertyValue value;
                if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                             BLT_KEY_MANAGER_PROPERTY, 
                                                             &value))) {
                    if (value.type == ATX_PROPERTY_VALUE_TYPE_POINTER) {
                        self->parser->key_manager = (BLT_KeyManager*)value.data.pointer;
                    }
                } else {
                    ATX_LOG_FINE("no key manager");
                }
            }
        }
        if (self->parser->key_manager == NULL) return BLT_ERROR_NO_MEDIA_KEY;
        
        // check if we need to use a cipher factory
        if (self->parser->cipher_factory == NULL) {
            ATX_Properties* properties = NULL;
            if (BLT_SUCCEEDED(BLT_Core_GetProperties(ATX_BASE(self->parser, BLT_BaseMediaNode).core, 
                                                              &properties))) {
                ATX_PropertyValue value;
                if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                             BLT_CIPHER_FACTORY_PROPERTY, 
                                                             &value))) {
                    if (value.type == ATX_PROPERTY_VALUE_TYPE_POINTER) {
                        self->parser->cipher_factory = new BLT_Ap4CipherFactoryAdapter((BLT_CipherFactory*)value.data.pointer);
                    }
                }
            }
        }
        
        // figure out the content ID for this track
        // TODO: support different content ID schemes
        // for now, we just make up a content ID based on the track ID
        char content_id[32];
        NPT_FormatString(content_id, sizeof(content_id), "@track.%d", self->track->GetId());
        
        // get the key for this content
        unsigned int   key_size = 256;
        NPT_DataBuffer key(key_size);
        BLT_Result result = BLT_KeyManager_GetKeyByName(self->parser->key_manager, content_id, key.UseData(), &key_size);
        if (result == ATX_ERROR_NOT_ENOUGH_SPACE) {
            key.SetDataSize(key_size);
            result = BLT_KeyManager_GetKeyByName(self->parser->key_manager, content_id, key.UseData(), &key_size);
        }
        if (BLT_FAILED(result)) return BLT_ERROR_NO_MEDIA_KEY;
        key.SetDataSize(key_size);
        
        delete self->sample_decrypter;
        self->sample_decrypter = AP4_SampleDecrypter::Create(prot_desc, key.GetData(), key_size, self->parser->cipher_factory);
        if (self->sample_decrypter == NULL) {
            ATX_LOG_FINE("unable to create decrypter");
            return BLT_ERROR_CRYPTO_FAILURE;
        }
        
        // switch to the original sample description
        sample_desc = prot_desc->GetOriginalSampleDescription();
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Mp4ParserOutput_SetSampleDescription
+---------------------------------------------------------------------*/
static BLT_Result
Mp4ParserOutput_SetSampleDescription(Mp4ParserOutput* self, 
                                     unsigned int     indx)
{
    // if we had a decrypter before, release it now
    delete self->sample_decrypter;
    self->sample_decrypter = NULL;
    
    // check that the audio track is of the right type
    AP4_SampleDescription* sample_desc = self->track->GetSampleDescription(indx);
    if (sample_desc == NULL) {
        ATX_LOG_FINE("no sample description for track");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    // handle encrypted tracks
    BLT_Result result = Mp4ParserOutput_ProcessCryptoInfo(self, sample_desc);
    if (BLT_FAILED(result)) return result;
    
    // update the generic part of the stream info
    BLT_StreamInfo stream_info;
    stream_info.id            = self->track->GetId();
    stream_info.duration      = self->track->GetDurationMs();
    stream_info.mask = BLT_STREAM_INFO_MASK_ID |
                       BLT_STREAM_INFO_MASK_DURATION;
    
    // deal with audio details, if this is an audio track
    AP4_AudioSampleDescription* audio_desc = dynamic_cast<AP4_AudioSampleDescription*>(sample_desc);
    if (audio_desc) {
        ATX_LOG_FINE("sample description is audio");
        stream_info.type          = BLT_STREAM_TYPE_AUDIO;
        stream_info.channel_count = audio_desc->GetChannelCount();
        stream_info.sample_rate   = audio_desc->GetSampleRate();
        stream_info.mask |= BLT_STREAM_INFO_MASK_TYPE          |
                            BLT_STREAM_INFO_MASK_CHANNEL_COUNT |
                            BLT_STREAM_INFO_MASK_SAMPLE_RATE;
    } else if (self == &self->parser->audio_output) {
        ATX_LOG_FINE("expected audio sample description, but did not get one");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }

    AP4_VideoSampleDescription* video_desc = dynamic_cast<AP4_VideoSampleDescription*>(sample_desc);
    if (video_desc) {
        ATX_LOG_FINE("sample description is video");
        stream_info.type     = BLT_STREAM_TYPE_VIDEO;
        stream_info.width    = video_desc->GetWidth();
        stream_info.height   = video_desc->GetHeight();
        stream_info.mask |= BLT_STREAM_INFO_MASK_TYPE     |
                            BLT_STREAM_INFO_MASK_WIDTH    |
                            BLT_STREAM_INFO_MASK_HEIGHT;
    } else if (self == &self->parser->video_output) {
        ATX_LOG_FINE("expected video sample descriton, but did not get one");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    
    AP4_MpegSampleDescription* mpeg_desc = NULL;
    if (sample_desc->GetType() == AP4_SampleDescription::TYPE_MPEG) {
        ATX_LOG_FINE("sample description is of type MPEG");
        mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(sample_desc);
    }
    if (mpeg_desc) {
        stream_info.data_type       = mpeg_desc->GetObjectTypeString(mpeg_desc->GetObjectTypeId());
        stream_info.average_bitrate = mpeg_desc->GetAvgBitrate();
        stream_info.nominal_bitrate = mpeg_desc->GetAvgBitrate();
        stream_info.mask |= BLT_STREAM_INFO_MASK_AVERAGE_BITRATE |
                            BLT_STREAM_INFO_MASK_NOMINAL_BITRATE |
                            BLT_STREAM_INFO_MASK_DATA_TYPE;
    }
    
    // setup the output media type
    AP4_DataBuffer  decoder_info;
    BLT_MediaTypeId media_type_id = BLT_MEDIA_TYPE_ID_NONE;
    AP4_UI32        format_or_object_type_id = 0;
    if (mpeg_desc) {
        decoder_info.SetData(mpeg_desc->GetDecoderInfo().GetData(),
                             mpeg_desc->GetDecoderInfo().GetDataSize());
        media_type_id = self->mp4_es_type_id;
        format_or_object_type_id = mpeg_desc->GetObjectTypeId();
    } else {
        // here we have to be format-specific for the decoder info
        stream_info.data_type = AP4_GetFormatName(sample_desc->GetFormat());
        stream_info.mask |= BLT_STREAM_INFO_MASK_DATA_TYPE;
        format_or_object_type_id = sample_desc->GetFormat();
        if (sample_desc->GetFormat() == AP4_SAMPLE_FORMAT_AVC1) {
            // look for an 'avcC' atom
            AP4_AvccAtom* avcc = static_cast<AP4_AvccAtom*>(sample_desc->GetDetails().GetChild(AP4_ATOM_TYPE_AVCC));
            if (avcc) {
                // pass the avcc payload as the decoder info
                decoder_info.SetData(avcc->GetRawBytes().GetData(),
                                     avcc->GetRawBytes().GetDataSize());
            } 
        } else if (sample_desc->GetFormat() == AP4_SAMPLE_FORMAT_ALAC) {
            // look for an 'alac' atom (either top-level or inside a 'wave') 
            AP4_Atom* alac = sample_desc->GetDetails().GetChild(AP4_SAMPLE_FORMAT_ALAC);
            if (alac == NULL) {
                AP4_ContainerAtom* wave = dynamic_cast<AP4_ContainerAtom*>(sample_desc->GetDetails().GetChild(AP4_ATOM_TYPE_WAVE));
                if (wave) {
                    alac = wave->GetChild(AP4_SAMPLE_FORMAT_ALAC);
                }
            }
            if (alac) {
                // pass the alac payload as the decoder info
                AP4_MemoryByteStream* mbs = new AP4_MemoryByteStream((AP4_Size)alac->GetSize());
                alac->WriteFields(*mbs);
                decoder_info.SetData(mbs->GetData(), mbs->GetDataSize());                
                mbs->Release();
            } 
        }
        
        media_type_id = self->iso_base_es_type_id;
    }
    BLT_Mp4MediaType* media_type = NULL;
    unsigned int struct_size = decoder_info.GetDataSize()?decoder_info.GetDataSize()-1:0;
    if (audio_desc) {
        struct_size += sizeof(BLT_Mp4AudioMediaType);
        BLT_Mp4AudioMediaType* audio_type = (BLT_Mp4AudioMediaType*)ATX_AllocateZeroMemory(struct_size);;
        audio_type->base.stream_type    = BLT_MP4_STREAM_TYPE_AUDIO;
        audio_type->channel_count       = audio_desc->GetChannelCount();
        audio_type->sample_rate         = audio_desc->GetSampleRate();
        audio_type->decoder_info_length = decoder_info.GetDataSize();
        if (decoder_info.GetDataSize()) {
            ATX_CopyMemory(&audio_type->decoder_info[0], decoder_info.GetData(), decoder_info.GetDataSize());
        }
        media_type = &audio_type->base;
    } else {
        struct_size += sizeof(BLT_Mp4VideoMediaType);
        BLT_Mp4VideoMediaType* video_type = (BLT_Mp4VideoMediaType*)ATX_AllocateZeroMemory(struct_size);
        video_type->base.stream_type    = BLT_MP4_STREAM_TYPE_VIDEO;
        video_type->width               = video_desc->GetWidth();
        video_type->height              = video_desc->GetHeight();
        video_type->decoder_info_length = decoder_info.GetDataSize();
        if (decoder_info.GetDataSize()) {
            ATX_CopyMemory(&video_type->decoder_info[0], decoder_info.GetData(), decoder_info.GetDataSize());
        }
        media_type = &video_type->base;
    }
    media_type->base.id                  = media_type_id;
    media_type->base.extension_size      = struct_size-sizeof(BLT_MediaType); 
    media_type->format_or_object_type_id = format_or_object_type_id;
    self->media_type = &media_type->base;
    self->sample_description_index = indx;
    
    // final update to the stream info
    BLT_Stream_SetInfo(ATX_BASE(self->parser, BLT_BaseMediaNode).context, &stream_info);
    
    // enable the track in the linear reader if we have one
    if (self->parser->input.reader) {
        self->parser->input.reader->EnableTrack(self->track->GetId());
    }
    
    return BLT_SUCCESS;    
}

/*----------------------------------------------------------------------
|   Mp4Parser_SetupAudioOutput
+---------------------------------------------------------------------*/
static BLT_Result
Mp4Parser_SetupAudioOutput(Mp4Parser* self, AP4_Movie* movie)
{
    ATX_Properties* properties = NULL;
    
    // select the audio track
    if (BLT_SUCCEEDED(BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, &properties))) {
        ATX_PropertyValue value;
        bool selector_is_strict = false;
        if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                     BLT_STREAM_AUDIO_TRACK_SELECTOR_STRICT_PROPERTY, 
                                                     &value))) {
            if (value.type == ATX_PROPERTY_VALUE_TYPE_BOOLEAN) {
                selector_is_strict = value.data.boolean == ATX_TRUE;
            }
        }
        if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                     BLT_STREAM_AUDIO_TRACK_SELECTOR_INDEX_PROPERTY, 
                                                     &value))) {
            if (value.type == ATX_PROPERTY_VALUE_TYPE_INTEGER) {
                ATX_LOG_INFO_1("selecting audio track by index (%d)", value.data.integer);
                self->audio_output.track = movie->GetTrack(AP4_Track::TYPE_AUDIO, value.data.integer);
                if (self->audio_output.track == NULL) {
                    ATX_LOG_INFO("track not found");
                    if (selector_is_strict) return BLT_SUCCESS;
                }
            }
        } else if (ATX_SUCCEEDED(ATX_Properties_GetProperty(properties, 
                                                            BLT_STREAM_AUDIO_TRACK_SELECTOR_ID_PROPERTY, 
                                                            &value))) {
            if (value.type == ATX_PROPERTY_VALUE_TYPE_INTEGER) {
                ATX_LOG_INFO_1("selecting audio track by ID (%d)", value.data.integer);
                self->audio_output.track = movie->GetTrack((AP4_UI32)value.data.integer);
                if (self->audio_output.track == NULL) {
                    ATX_LOG_INFO("track not found");
                    if (selector_is_strict) return BLT_SUCCESS;
                } else if (self->audio_output.track->GetType() != AP4_Track::TYPE_AUDIO) {
                    ATX_LOG_INFO("track is not audio");
                    if (selector_is_strict) return BLT_SUCCESS;
                } 
            }
        }
    }
    
    // select the first audio track if no specific track was selected
    if (self->audio_output.track == NULL) {
        ATX_LOG_INFO("selecting first audio track");
        self->audio_output.track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    }
    
    // exit now if the track does not exist
    if (self->audio_output.track == NULL) return BLT_SUCCESS;

    ATX_LOG_INFO_1("found audio track (id=%d)", self->audio_output.track->GetId());
    
    // use the first sample description by default
    return Mp4ParserOutput_SetSampleDescription(&self->audio_output, 0);
}

/*----------------------------------------------------------------------
|   Mp4Parser_SetupVideoOutput
+---------------------------------------------------------------------*/
static BLT_Result
Mp4Parser_SetupVideoOutput(Mp4Parser* self, AP4_Movie* movie)
{
    // get the video track
    AP4_Track* track = movie->GetTrack(AP4_Track::TYPE_VIDEO);
    self->video_output.track = track;
    if (track == NULL) return BLT_SUCCESS;

    ATX_LOG_FINE("found video track");

    // use the first sample description by default
    return Mp4ParserOutput_SetSampleDescription(&self->video_output, 0);
}

/*----------------------------------------------------------------------
|   Mp4ParserInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4ParserInput_SetStream(BLT_InputStreamUser* _self,
                         ATX_InputStream*     stream,
                         const BLT_MediaType* stream_media_type)
{
    Mp4Parser* self = ATX_SELF_M(input, Mp4Parser, BLT_InputStreamUser);
    BLT_Result result = BLT_ERROR_INVALID_MEDIA_FORMAT;
    
    /* check media type */
    if (stream_media_type == NULL || 
        (stream_media_type->id != self->input.audio_media_type.id &&
         stream_media_type->id != self->input.video_media_type.id)) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* if we had a file before, release it now */
    delete self->input.mp4_file;
    self->input.mp4_file = NULL;
    self->input.slow_seek = false;
    
    /* create an adapter for the stream */
    AP4_ByteStream* stream_adapter = new ATX_InputStream_To_AP4_ByteStream_Adapter(stream);

    /* check if the source can seek quickly or not */
    {
        ATX_Properties* stream_properties = ATX_CAST(stream, ATX_Properties);
        if (stream_properties) {
            ATX_PropertyValue property_value;
            result = ATX_Properties_GetProperty(stream_properties, 
                                                ATX_INPUT_STREAM_PROPERTY_SEEK_SPEED,
                                                &property_value);
            if (ATX_SUCCEEDED(result) && 
                property_value.type == ATX_PROPERTY_VALUE_TYPE_INTEGER &&
                property_value.data.integer <= ATX_INPUT_STREAM_SEEK_SPEED_SLOW) {
                AP4_ByteStream* buffered = new AP4_BufferedInputStream(*stream_adapter);
                ATX_LOG_FINE("using no-seek mode, source is slow");
                stream_adapter->Release();
                stream_adapter = buffered;
                self->input.slow_seek = true;
            }
        }
    }

    /* parse the MP4 file */
    ATX_LOG_FINE("parsing MP4 file");
    self->input.mp4_file = new AP4_File(*stream_adapter, 
                                        AP4_DefaultAtomFactory::Instance,
                                        true); /* parse until moov only */
    stream_adapter->Release();

    // get the global file info
    AP4_Movie* movie = self->input.mp4_file->GetMovie();
    if (movie == NULL) {
        ATX_LOG_FINE("no movie in file");
        goto fail;
    }
    
    // update the stream info
    BLT_StreamInfo stream_info;
    stream_info.type     = BLT_STREAM_TYPE_MULTIPLEXED;
    stream_info.id       = 0;
    stream_info.duration = movie->GetDurationMs();
    stream_info.mask = BLT_STREAM_INFO_MASK_TYPE |
                       BLT_STREAM_INFO_MASK_ID   |
                       BLT_STREAM_INFO_MASK_DURATION;    
    BLT_Stream_SetInfo(ATX_BASE(self, BLT_BaseMediaNode).context, &stream_info);
    
    // create a linear reader if the source is slow-seeking
    if (self->input.slow_seek) {
        self->input.reader = new AP4_LinearReader(*movie);
    }
    
    // setup the tracks
    result = Mp4Parser_SetupAudioOutput(self, movie);
    if (BLT_FAILED(result)) goto fail;
    result = Mp4Parser_SetupVideoOutput(self, movie);
    if (BLT_FAILED(result)) goto fail;
    
    // check that we have at least one media track
    if (self->audio_output.track == NULL && 
        self->video_output.track == NULL) {
        ATX_LOG_FINE("no media track found");
        goto fail;
    }
    
    return BLT_SUCCESS;

fail:
    delete self->input.mp4_file;
    self->input.mp4_file = NULL;
    self->audio_output.track = NULL;
    self->video_output.track = NULL;
    
    return result;
}

/*----------------------------------------------------------------------
|   Mp4ParserInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4ParserInput_QueryMediaType(BLT_MediaPort*        _self,
                              BLT_Ordinal           index,
                              const BLT_MediaType** media_type)
{
    Mp4ParserInput* self = ATX_SELF(Mp4ParserInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->audio_media_type;
        return BLT_SUCCESS;
    } else if (index == 0) {
        *media_type = &self->video_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Mp4ParserInput)
    ATX_GET_INTERFACE_ACCEPT(Mp4ParserInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(Mp4ParserInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Mp4ParserInput, BLT_InputStreamUser)
    Mp4ParserInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(Mp4ParserInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(Mp4ParserInput, BLT_MediaPort)
    Mp4ParserInput_GetName,
    Mp4ParserInput_GetProtocol,
    Mp4ParserInput_GetDirection,
    Mp4ParserInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   Mp4ParserOutput_Construct
+---------------------------------------------------------------------*/
static void
Mp4ParserOutput_Construct(Mp4ParserOutput* self, Mp4Parser* parser)
{
    self->parser                  = parser;
    self->media_type              = NULL;
    self->sample                  = 0;
    self->sample_buffer           = new AP4_DataBuffer();
    self->sample_decrypted_buffer = new AP4_DataBuffer();
    self->sample_decrypter        = NULL;
}

/*----------------------------------------------------------------------
|   Mp4ParserOutput_Destruct
+---------------------------------------------------------------------*/
static void
Mp4ParserOutput_Destruct(Mp4ParserOutput* self)
{
    /* release the sample buffer */
    delete self->sample_buffer;
    delete self->sample_decrypted_buffer;

    /* free the media type extensions */
    BLT_MediaType_Free((BLT_MediaType*)self->media_type);
}

/*----------------------------------------------------------------------
|   Mp4ParserOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4ParserOutput_QueryMediaType(BLT_MediaPort*        _self,
                               BLT_Ordinal           index,
                               const BLT_MediaType** media_type)
{
    Mp4ParserOutput* self = ATX_SELF(Mp4ParserOutput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = (BLT_MediaType*)self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   Mp4ParserOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4ParserOutput_GetPacket(BLT_PacketProducer* _self,
                          BLT_MediaPacket**   packet)
{
    Mp4ParserOutput* self = ATX_SELF(Mp4ParserOutput, BLT_PacketProducer);

    *packet = NULL;
     
    // if we don't have an input yet, we can't produce packets
    //if (self->parser->input.mp4_file == NULL) {
    //    return BLT_ERROR_PORT_HAS_NO_DATA;
    //}
    
    if (self->track == NULL) {
        return BLT_ERROR_EOS;
    } else {
        // check for end-of-stream
        if (self->sample >= self->track->GetSampleCount()) {
            return BLT_ERROR_EOS;
        }

        // read one sample
        AP4_Sample sample;
        AP4_DataBuffer* sample_buffer = self->sample_buffer;
        AP4_Result result;
        if (self->parser->input.reader) {
            // linear reader mode
            result = self->parser->input.reader->ReadNextSample(self->track->GetId(), sample, *sample_buffer);
            if (AP4_SUCCEEDED(result)) self->sample++;
        } else {
            // normal mode
            result = self->track->ReadSample(self->sample++, sample, *sample_buffer);
        }
        if (AP4_FAILED(result)) {
            ATX_LOG_WARNING_1("ReadSample failed (%d)", result);
            if (result == AP4_ERROR_EOS || result == ATX_ERROR_OUT_OF_RANGE) {
                ATX_LOG_WARNING("incomplete media");
                return BLT_ERROR_INCOMPLETE_MEDIA;
            } else {
                return BLT_ERROR_PORT_HAS_NO_DATA;
            }
        }

        // update the sample description if it has changed
        if (sample.GetDescriptionIndex() != self->sample_description_index) {
            result = Mp4ParserOutput_SetSampleDescription(self, sample.GetDescriptionIndex());
            if (BLT_FAILED(result)) return result;
        }
        
        // decrypt the sample if needed
        if (self->sample_decrypter) {
            self->sample_decrypter->DecryptSampleData(*sample_buffer, *self->sample_decrypted_buffer);
            sample_buffer = self->sample_decrypted_buffer;
        }

        AP4_Size packet_size = sample_buffer->GetDataSize();
        result = BLT_Core_CreateMediaPacket(ATX_BASE(self->parser, BLT_BaseMediaNode).core,
                                            packet_size,
                                            (const BLT_MediaType*)self->media_type,
                                            packet);
        if (BLT_FAILED(result)) return result;
        BLT_MediaPacket_SetPayloadSize(*packet, packet_size);
        void* buffer = BLT_MediaPacket_GetPayloadBuffer(*packet);
        ATX_CopyMemory(buffer, sample_buffer->GetData(), packet_size);

        // set the timestamp
        AP4_UI32 media_timescale = self->track->GetMediaTimeScale();
        if (media_timescale) {
            AP4_UI64 ts = ((AP4_UI64)sample.GetCts())*1000000;
            ts /= media_timescale;
            BLT_TimeStamp bt_ts = {
                (BLT_Int32)(ts / 1000000),
                (BLT_Int32)((ts % 1000000)*1000)
            };
            BLT_MediaPacket_SetTimeStamp(*packet, bt_ts);
        }

        // set packet flags
        if (self->sample == 1) {
            BLT_MediaPacket_SetFlags(*packet, BLT_MEDIA_PACKET_FLAG_START_OF_STREAM);
        }

        return BLT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Mp4ParserOutput)
    ATX_GET_INTERFACE_ACCEPT(Mp4ParserOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(Mp4ParserOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(Mp4ParserOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(Mp4ParserOutput, BLT_MediaPort)
    Mp4ParserOutput_GetName,
    Mp4ParserOutput_GetProtocol,
    Mp4ParserOutput_GetDirection,
    Mp4ParserOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_InputStreamProvider interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Mp4ParserOutput, BLT_PacketProducer)
    Mp4ParserOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   Mp4Parser_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Mp4Parser_Destroy(Mp4Parser* self)
{
    ATX_LOG_FINE("start");

    /* destruct the members */
    Mp4ParserInput_Destruct(&self->input);
    Mp4ParserOutput_Destruct(&self->audio_output);
    Mp4ParserOutput_Destruct(&self->video_output);
    delete self->cipher_factory;

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    delete self;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Mp4Parser_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4Parser_GetPortByName(BLT_MediaNode*  _self,
                         BLT_CString     name,
                         BLT_MediaPort** port)
{
    Mp4Parser* self = ATX_SELF_EX(Mp4Parser, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(&self->input, BLT_MediaPort);
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "output") ||
               ATX_StringsEqual(name, "audio")) {
        *port = &ATX_BASE(&self->audio_output, BLT_MediaPort);
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "video")) {
        *port = &ATX_BASE(&self->video_output, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|   Mp4Parser_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4Parser_Seek(BLT_MediaNode* _self,
               BLT_SeekMode*  mode,
               BLT_SeekPoint* point)
{
    Mp4Parser* self = ATX_SELF_EX(Mp4Parser, BLT_BaseMediaNode, BLT_MediaNode);

    /* estimate the seek point */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP)) {
        return BLT_FAILURE;
    }

    /* seek to the estimated offset on all tracks */
    AP4_Ordinal sample_index = 0;
    AP4_UI32    ts_ms = point->time_stamp.seconds*1000+point->time_stamp.nanoseconds/1000000;
    if (self->video_output.track) {
        AP4_Result result = self->video_output.track->GetSampleIndexForTimeStampMs(ts_ms, sample_index);
        if (AP4_FAILED(result)) {
            ATX_LOG_WARNING_1("video GetSampleIndexForTimeStampMs failed (%d)", result);
            return BLT_FAILURE;
        }
        ATX_LOG_FINE_1("seeking to video time %d ms", ts_ms);
        
        // go to the nearest sync sample
        self->video_output.sample = self->video_output.track->GetNearestSyncSampleIndex(sample_index);
        if (self->input.reader) {
            self->input.reader->SetSampleIndex(self->video_output.track->GetId(), self->video_output.sample);
        }
        ATX_LOG_FINE_1("seeking to video sync sample %d", self->video_output.sample);
        
        // compute the timestamp of the video sample we're seeking to, so we can pick an audio
        // sample that is close in time (there are many more audio sync points than video)
        AP4_Sample sample;
        if (AP4_SUCCEEDED(self->video_output.track->GetSample(self->video_output.sample, sample))) {
            AP4_UI32 media_timescale = self->video_output.track->GetMediaTimeScale();
            if (media_timescale) {
                ts_ms = (AP4_UI32)((((AP4_UI64)sample.GetCts())*1000)/media_timescale);
                ATX_LOG_FINE_1("sync sample time is %d ms", ts_ms);
            }
        } else {
            ATX_LOG_FINE_1("unable to get sample info for sample %d", self->video_output.sample);
        }
    }
    if (self->audio_output.track) {
        AP4_Result result = self->audio_output.track->GetSampleIndexForTimeStampMs(ts_ms, sample_index);
        if (AP4_FAILED(result)) {
            ATX_LOG_WARNING_1("audio GetSampleIndexForTimeStampMs failed (%d)", result);
            return BLT_FAILURE;
        }
        self->audio_output.sample = sample_index;
        if (self->input.reader) {
            self->input.reader->SetSampleIndex(self->audio_output.track->GetId(), sample_index);
        }
    }
    
    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                 */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Mp4Parser)
    ATX_GET_INTERFACE_ACCEPT_EX(Mp4Parser, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(Mp4Parser, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(Mp4Parser, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    Mp4Parser_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    Mp4Parser_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(Mp4Parser, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   Mp4Parser_Construct
+---------------------------------------------------------------------*/
static void
Mp4Parser_Construct(Mp4Parser* self, BLT_Module* module, BLT_Core* core)
{
    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* initialize some fields */
    self->key_manager = NULL;
    
    /* construct the members */
    Mp4ParserInput_Construct(&self->input, module);
    Mp4ParserOutput_Construct(&self->audio_output, self);
    Mp4ParserOutput_Construct(&self->video_output, self);
    
    /* setup media types */
    Mp4ParserModule* mp4_parser_module = (Mp4ParserModule*)module;
    self->audio_output.mp4_es_type_id = mp4_parser_module->mp4_audio_es_type_id;
    self->audio_output.iso_base_es_type_id = mp4_parser_module->iso_base_audio_es_type_id;
    self->video_output.mp4_es_type_id = mp4_parser_module->mp4_video_es_type_id;
    self->video_output.iso_base_es_type_id = mp4_parser_module->iso_base_video_es_type_id;
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, Mp4Parser, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, Mp4Parser, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  Mp4ParserInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  Mp4ParserInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->audio_output, Mp4ParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->audio_output, Mp4ParserOutput, BLT_PacketProducer);
    ATX_SET_INTERFACE(&self->video_output, Mp4ParserOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->video_output, Mp4ParserOutput, BLT_PacketProducer);
}

/*----------------------------------------------------------------------
|   Mp4Parser_Create
+---------------------------------------------------------------------*/
static BLT_Result
Mp4Parser_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    Mp4Parser* self;

    ATX_LOG_FINE("start");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate the object */
    self = new Mp4Parser();
    Mp4Parser_Construct(self, module, core);

    /* return the object */
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Mp4ParserModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4ParserModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    Mp4ParserModule* self = ATX_SELF_EX(Mp4ParserModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*    registry;
    BLT_Result       result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".mp4" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mp4",
                                            "video/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the ".m4a" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".m4a",
                                            "audio/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the ".m4v" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".m4v",
                                            "video/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the ".m4p" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".m4p",
                                            "video/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the ".3gp" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".3gp",
                                            "video/mp4");
    if (BLT_FAILED(result)) return result;

    /* register the ".3gp" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".mov",
                                            "video/mp4");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/mp4" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/mp4",
        &self->mp4_audio_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1("audio/mp4 type = %d", self->mp4_audio_type_id);
    
    /* get the type id for "video/mp4" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "video/mp4",
        &self->mp4_video_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1("video/mp4 type = %d", self->mp4_video_type_id);

    /* register the type id for BLT_MP4_AUDIO_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_MP4_AUDIO_ES_MIME_TYPE,
        &self->mp4_audio_es_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1(BLT_MP4_AUDIO_ES_MIME_TYPE " type = %d", self->mp4_audio_es_type_id);

    /* register the type id for BLT_MP4_VIDEO_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_MP4_VIDEO_ES_MIME_TYPE,
        &self->mp4_video_es_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1(BLT_MP4_VIDEO_ES_MIME_TYPE " type = %d", self->mp4_video_es_type_id);

    /* register the type id for BLT_ISO_BASE_AUDIO_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_ISO_BASE_AUDIO_ES_MIME_TYPE,
        &self->iso_base_audio_es_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1(BLT_ISO_BASE_AUDIO_ES_MIME_TYPE " type = %d", self->iso_base_audio_es_type_id);

    /* register the type id for BLT_ISO_BASE_VIDEO_ES_MIME_TYPE */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        BLT_ISO_BASE_VIDEO_ES_MIME_TYPE,
        &self->iso_base_video_es_type_id);
    if (BLT_FAILED(result)) return result;
    ATX_LOG_FINE_1(BLT_ISO_BASE_VIDEO_ES_MIME_TYPE " type = %d", self->iso_base_video_es_type_id);

    /* register mime type aliases */
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "audio/m4a", self->mp4_audio_type_id);
    BLT_Registry_RegisterNameForId(registry, 
                                   BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
                                   "video/m4v", self->mp4_video_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Mp4ParserModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
Mp4ParserModule_Probe(BLT_Module*              _self, 
                      BLT_Core*                core,
                      BLT_ModuleParametersType parameters_type,
                      BLT_AnyConst             parameters,
                      BLT_Cardinal*            match)
{
    Mp4ParserModule* self = ATX_SELF_EX(Mp4ParserModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

            /* check if we're being probed by name */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "com.bluetune.parsers.mp4")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                    return BLT_SUCCESS;
                } else {
                    /* not out name */
                    *match = 0;
                    return BLT_FAILURE;
                }
            }
            
            /* we need the input protocol to be STREAM_PULL and the output */
            /* protocol to be PACKET                                       */
             if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                  constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                 (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                  constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* we need the input media type to be 'audio/mp4' or 'video/mp4' */
            if (constructor->spec.input.media_type->id != self->mp4_audio_type_id &&
                constructor->spec.input.media_type->id != self->mp4_video_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unknown at this point */
            if (constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN) {
                return BLT_FAILURE;
            }

            /* set the match level */
            *match = BLT_MODULE_PROBE_MATCH_MAX - 10;

            ATX_LOG_FINE_1("match %d", *match);
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Mp4ParserModule)
    ATX_GET_INTERFACE_ACCEPT_EX(Mp4ParserModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(Mp4ParserModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(Mp4ParserModule, Mp4Parser)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(Mp4ParserModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    Mp4ParserModule_Attach,
    Mp4ParserModule_CreateInstance,
    Mp4ParserModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define Mp4ParserModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(Mp4ParserModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(Mp4ParserModule,
                                         "MP4 Parser",
                                         "com.axiosys.parser.mp4",
                                         "1.2.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
