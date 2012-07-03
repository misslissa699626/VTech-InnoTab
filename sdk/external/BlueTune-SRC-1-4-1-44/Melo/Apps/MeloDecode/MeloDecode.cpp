/*****************************************************************
|
|    Melo - MP4 to PCM Decoder
|
|    Copyright 2003-2006 Gilles Boccon-Gibod & Julien Boeuf
|
|
|    This file is part of Melo.
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Ap4.h"
#include "Melo.h"
 
/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            "MeloDecode\n" 
            "\n\nusage: melodecode [options] <input> <output>\n");
    exit(1);
}

/*----------------------------------------------------------------------
|   BytesFromUInt32LE
+---------------------------------------------------------------------*/
static void
BytesFromUInt32LE(unsigned char* bytes, AP4_UI32 value)
{
    bytes[3] = (unsigned char)(value >> 24);
    bytes[2] = (unsigned char)(value >> 16);
    bytes[1] = (unsigned char)(value >>  8);
    bytes[0] = (unsigned char)(value      );
}

/*----------------------------------------------------------------------
|   AP4_BytesFromUInt16LE
+---------------------------------------------------------------------*/
static void
BytesFromUInt16LE(unsigned char* bytes, AP4_UI16 value)
{
    bytes[1] = (unsigned char)(value >> 8);
    bytes[0] = (unsigned char)(value     );
}

/*----------------------------------------------------------------------
|   WriteWaveHeader
+---------------------------------------------------------------------*/
static void
WriteWaveHeader(AP4_ByteStream* out, 
                unsigned int    stream_size,
                unsigned int    channel_count,
                unsigned int    sample_rate)
{
    unsigned char buffer[4];

    /* rewind */
    out->Seek(0);
    
    /* RIFF tag */
    out->Write("RIFF", 4);

    /* RIFF chunk size */
    BytesFromUInt32LE(buffer, stream_size + 8+16+12);
    out->Write(buffer, 4);

    /* WAVE format */
    out->Write("WAVE", 4);
    out->Write("fmt ", 4);
    BytesFromUInt32LE(buffer, 16L);
    out->Write(buffer, 4);
    BytesFromUInt16LE(buffer, 1);
    out->Write(buffer, 2);

    /* number of channels */
    BytesFromUInt16LE(buffer, channel_count);        
    out->Write(buffer, 2);

    /* sample rate */
    BytesFromUInt32LE(buffer, sample_rate);       
    out->Write(buffer, 4);

    /* bytes per second */
    BytesFromUInt32LE(buffer, sample_rate * channel_count * 2);
    out->Write(buffer, 4);

    /* alignment   */
    BytesFromUInt16LE(buffer, channel_count*2);     
    out->Write(buffer, 2);

    /* bits per sample */
    BytesFromUInt16LE(buffer, 16);               
    out->Write(buffer, 2);

    out->Write("data", 4);

    /* data size */
    BytesFromUInt32LE(buffer, stream_size);        
    out->Write(buffer, 4);
}

/*----------------------------------------------------------------------
|   WriteWaveSamples
+---------------------------------------------------------------------*/
static void
WriteWaveSamples(AP4_ByteStream* out, MLO_SampleBuffer* sample_buffer)
{
    unsigned int sample_count = MLO_SampleBuffer_GetSampleCount(sample_buffer);
    unsigned int channel_count = MLO_SampleBuffer_GetFormat(sample_buffer)->channel_count;
    const short* samples = (const short*)MLO_SampleBuffer_GetSamples(sample_buffer);
    
    for (unsigned int i=0; i<sample_count*channel_count; i++) {
        /* write the samples as 16-bit little endian (WAVE format) */
        unsigned char s[2];
        BytesFromUInt16LE(s, samples[i]);
        out->Write(s, 2);
    }
}

/*----------------------------------------------------------------------
|   WriteSamples
+---------------------------------------------------------------------*/
static void
WriteSamples(AP4_Track* track, AP4_ByteStream* output)
{
    // get the sample description
    AP4_SampleDescription* desc = track->GetSampleDescription(0);
    if (desc == NULL) {
        fprintf(stderr, "ERROR: unable to get sample description\n");
        return;
    }
    AP4_MpegSampleDescription* mpeg_desc;
    if (desc->GetType() != AP4_SampleDescription::TYPE_MPEG) {
        fprintf(stderr, "ERROR: this track does not contain MPEG audio data\n");
        return;
    }
    mpeg_desc = dynamic_cast<AP4_MpegSampleDescription*>(desc);
    if (!mpeg_desc) {
        fprintf(stderr, "ERROR: sample description is not of the right type\n");
        return;
    }
    printf("    Stream Type: %s\n", AP4_MpegSampleDescription::GetStreamTypeString(mpeg_desc->GetStreamType()));
    printf("    Object Type: %s\n", AP4_MpegSampleDescription::GetObjectTypeString(mpeg_desc->GetObjectTypeId()));
    printf("    Max Bitrate: %d\n", mpeg_desc->GetMaxBitrate());
    printf("    Avg Bitrate: %d\n", mpeg_desc->GetAvgBitrate());
    printf("    Buffer Size: %d\n", mpeg_desc->GetBufferSize());
    if (mpeg_desc->GetStreamType() != AP4_AUDIO_STREAM_TYPE) {
        fprintf(stderr, "ERROR: unsupported stream type\n");
        return;
    }
    if (mpeg_desc->GetObjectTypeId() != AP4_MPEG2_AAC_AUDIO_LC_OTI &&
        mpeg_desc->GetObjectTypeId() != AP4_MPEG4_AUDIO_OTI) {
        fprintf(stderr, "ERROR: unsupported object type\n");
        return;
    }
    if (desc->GetFormat() != AP4_SAMPLE_FORMAT_MP4A) {
        fprintf(stderr, "ERROR: the sample format is not 'mp4a'\n");
        return;
    }

    AP4_AudioSampleDescription* audio_desc = dynamic_cast<AP4_AudioSampleDescription*>(desc);
    if (!audio_desc) {
        fprintf(stderr, "ERROR: sample description is not of the right type\n");
        return;
    }
    unsigned int channel_count = audio_desc->GetChannelCount();
    unsigned int sample_rate   = audio_desc->GetSampleRate();
    printf("    Sample Rate: %d\n", audio_desc->GetSampleRate());
    printf("    Sample Size: %d\n", audio_desc->GetSampleSize());
    printf("    Channels:    %d\n", audio_desc->GetChannelCount());

    AP4_Sample     sample;
    AP4_DataBuffer data;
    AP4_Ordinal    index = 0;
    unsigned long  total_size = 0;

    // check that we have a decoder config
    const AP4_DataBuffer* encoded_config = mpeg_desc->GetDecoderInfo();
    if (encoded_config == NULL) {
        fprintf(stderr, "ERROR: no decoder config\n");
        return;
    }

    // parse the decoder config
    MLO_DecoderConfig decoder_config;
    MLO_Result result = MLO_DecoderConfig_Parse(encoded_config->GetData(), 
                                                encoded_config->GetDataSize(),
                                                &decoder_config);
    if (MLO_FAILED(result)) {
        fprintf(stderr, "ERROR: decoder config is unsupported or unsupported (%d)\n", result);
        return;
    }

    // print the object type (AAC stream type)
    const char* ots = "Unknown";
    switch (decoder_config.object_type) {
        case MLO_OBJECT_TYPE_AAC_MAIN:        ots = "AAC Main Profile"; break;
        case MLO_OBJECT_TYPE_AAC_LC:          ots = "AAC Low Complexity"; break;
        case MLO_OBJECT_TYPE_AAC_SSR:         ots = "AAC Scalable Sample Rate"; break;
        case MLO_OBJECT_TYPE_AAC_LTP:         ots = "AAC Long Term Prediction"; break;
        case MLO_OBJECT_TYPE_SBR:             ots = "Spectral Band Replication"; break;
        case MLO_OBJECT_TYPE_AAC_SCALABLE:    ots = "AAC Scalable"; break;
        case MLO_OBJECT_TYPE_ER_AAC_LC:       ots = "Error Resilient AAC Low Complexity"; break;
        case MLO_OBJECT_TYPE_ER_AAC_LTP:      ots = "Error Resilient AAC Long Term Prediction"; break;
        case MLO_OBJECT_TYPE_ER_AAC_SCALABLE: ots = "Error Resilient AAC Scalable"; break;
        case MLO_OBJECT_TYPE_ER_AAC_LD:       ots = "Error Resilient AAC Low Delay"; break;
    }
    printf("    Sub Type: [%d] %s\n", decoder_config.object_type, ots);
    
    // create the decoder and sample buffer
    MLO_Decoder* decoder = NULL;
    MLO_SampleBuffer* pcm_buffer = NULL;
    result = MLO_Decoder_Create(&decoder_config, &decoder);
    if (MLO_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to created MLO_Decoder (%d)\n", result);
        goto end;
    }
    result = MLO_SampleBuffer_Create(0, &pcm_buffer);
    if (MLO_FAILED(result)) goto end;

    // write a WAVE header
    WriteWaveHeader(output, total_size, channel_count, sample_rate);
    
    // decode and write all samples
    while (AP4_SUCCEEDED(track->ReadSample(index, sample, data))) {
        // decode one frame
        result = MLO_Decoder_DecodeFrame(decoder, data.GetData(), data.GetDataSize(), pcm_buffer);
        if (result != MLO_SUCCESS) {
            printf("MLO_Decoder_DecodeFrame return %d\n", result);
            break;
        }
        
        // check that the sample rate and channel count matches what was in the header
        // (sometimes it does not...)
        const MLO_SampleFormat* format = MLO_SampleBuffer_GetFormat(pcm_buffer);
        if (format->sample_rate != sample_rate) {
            fprintf(stderr, "WARNING: sample rate different from header (%d vs %d)\n",
                    format->sample_rate, sample_rate);
            sample_rate = format->sample_rate;
        }
        if (format->channel_count != channel_count) {
            fprintf(stderr, "WARNING: channel count different from header (%d vs %d)\n",
                    format->channel_count, channel_count);
            channel_count = format->channel_count;
        }
        
        // write the samples in WAVE format
        WriteWaveSamples(output, pcm_buffer);
        
        total_size += MLO_SampleBuffer_GetSize(pcm_buffer);
	    index++;
    }

    // update the WAVE header
    WriteWaveHeader(output, total_size, channel_count, sample_rate);
    
end:
    if (pcm_buffer) MLO_SampleBuffer_Destroy(pcm_buffer);
    if (decoder) MLO_Decoder_Destroy(decoder);
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 3) {
        PrintUsageAndExit();
    }
    
    // parse command line
    char** args = argv+1;

	// create the input stream
    AP4_ByteStream* input = 
        new AP4_FileByteStream(*args++,
                               AP4_FileByteStream::STREAM_MODE_READ);

	// create the output stream
    AP4_ByteStream* output =
        new AP4_FileByteStream(*args++,
                               AP4_FileByteStream::STREAM_MODE_WRITE);

	// open the file
    AP4_File* input_file = new AP4_File(*input);   

    // get the movie
    AP4_SampleDescription* sample_description;
    AP4_Track* audio_track;
    AP4_Movie* movie = input_file->GetMovie();
    if (movie == NULL) {
        fprintf(stderr, "ERROR: no movie in file\n");
        goto end;
    }

    // get the audio track
    audio_track = movie->GetTrack(AP4_Track::TYPE_AUDIO);
    if (audio_track == NULL) {
        fprintf(stderr, "ERROR: no audio track found\n");
        goto end;
    }

    // check that the track is of the right type
    sample_description = audio_track->GetSampleDescription(0);
    if (sample_description == NULL) {
        fprintf(stderr, "ERROR: unable to parse sample description\n");
        goto end;
    }

    // show info
    AP4_Debug("Audio Track:\n");
    AP4_Debug("  duration: %ld ms\n", audio_track->GetDurationMs());
    AP4_Debug("  sample count: %ld\n", audio_track->GetSampleCount());

    switch (sample_description->GetType()) {
        case AP4_SampleDescription::TYPE_MPEG:
            WriteSamples(audio_track, output);
            break;

        default:
            fprintf(stderr, "ERROR: unsupported sample type\n");
            break;
    }

end:
    delete input_file;
    input->Release();
    output->Release();

    return 0;
}

