/*****************************************************************
|
|   BlueTune - Builtins Object
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune Builtins
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltBuiltins.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.common")

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#if !defined(BLT_CONFIG_MODULES_DEFAULT_AUDIO_OUTPUT_NAME)
#define BLT_BUILTINS_DEFAULT_AUDIO_OUTPUT_NAME "null"
#endif
#if !defined(BLT_CONFIG_MODULES_DEFAULT_AUDIO_OUTPUT_TYPE)
#define BLT_BUILTINS_DEFAULT_AUDIO_OUTPUT_TYPE "audio/pcm"
#endif

#if !defined(BLT_CONFIG_MODULES_DEFAULT_VIDEO_OUTPUT_NAME)
#define BLT_BUILTINS_DEFAULT_VIDEO_OUTPUT_NAME "null"
#endif
#if !defined(BLT_CONFIG_MODULES_DEFAULT_VIDEO_OUTPUT_TYPE)
#define BLT_BUILTINS_DEFAULT_VIDEO_OUTPUT_TYPE "video/raw"
#endif

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/
#define BLT_REGISTER_BUILTIN(x)                                               \
{                                                                             \
    extern BLT_Result BLT_##x##Module_GetModuleObject(BLT_Module** module);   \
    BLT_Module* module = NULL;                                                \
    BLT_Result result;                                                        \
    ATX_LOG_FINE("registering BLT_" #x "Module");                             \
    result = BLT_##x##Module_GetModuleObject(&module);                        \
    if (BLT_SUCCEEDED(result)) {                                              \
        BLT_Core_RegisterModule(core, module);                                \
        ATX_RELEASE_OBJECT(module);                                           \
    } else {                                                                  \
        ATX_LOG_WARNING_2("BLT_" #x "Module_GetModuleObject returned %d (%s)",\
                          result, BLT_ResultText(result));                    \
    }                                                                         \
}

/*----------------------------------------------------------------------
|    BLT_Builtins_RegisterModules
+---------------------------------------------------------------------*/
BLT_Result
BLT_Builtins_RegisterModules(BLT_Core* core)
{
    ATX_LOG_FINE("registering builtin modules");
    /* in the case where there are no builtin modules, avoid compiler warnings */
    ATX_COMPILER_UNUSED(core);
#if 1

#if defined(BLT_CONFIG_MODULES_ENABLE_TEST_INPUT)
    BLT_REGISTER_BUILTIN(TestInput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_FILE_INPUT)
    BLT_REGISTER_BUILTIN(FileInput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_NETWORK_INPUT)
    BLT_REGISTER_BUILTIN(NetworkInput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_CDDA_INPUT)
    BLT_REGISTER_BUILTIN(CddaInput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_ALSA_INPUT)
    BLT_REGISTER_BUILTIN(AlsaInput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_STREAM_PACKETIZER)
    BLT_REGISTER_BUILTIN(StreamPacketizer)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_PACKET_STREAMER)
    BLT_REGISTER_BUILTIN(PacketStreamer)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_CROSS_FADER)
    BLT_REGISTER_BUILTIN(CrossFader)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_SILENCE_REMOVER)
    BLT_REGISTER_BUILTIN(SilenceRemover)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_WAVE_PARSER)
    BLT_REGISTER_BUILTIN(WaveParser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_AIFF_PARSER)
    BLT_REGISTER_BUILTIN(AiffParser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_TAG_PARSER)
    BLT_REGISTER_BUILTIN(TagParser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_MP4_PARSER)
    BLT_REGISTER_BUILTIN(Mp4Parser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_DCF_PARSER)
    BLT_REGISTER_BUILTIN(DcfParser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_ADTS_PARSER)
    BLT_REGISTER_BUILTIN(AdtsParser)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_WMA_PARSER)
    BLT_REGISTER_BUILTIN(WmaParser)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_M4A_PARSER)
    BLT_REGISTER_BUILTIN(M4aParser)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_AMR_PARSER)
    BLT_REGISTER_BUILTIN(AmrParser)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_DDPLUS_PARSER)
    BLT_REGISTER_BUILTIN(DolbyDigitalPlusParser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_OSX_AUDIO_FILE_STREAM_PARSER)
    BLT_REGISTER_BUILTIN(OsxAudioFileStreamParser)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_WAVE_FORMATTER)
    BLT_REGISTER_BUILTIN(WaveFormatter)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_GAIN_CONTROL_FILTER)
    BLT_REGISTER_BUILTIN(GainControlFilter)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_FINGERPRINT_FILTER)
    BLT_REGISTER_BUILTIN(FingerprintFilter)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_FILTER_HOST)
    BLT_REGISTER_BUILTIN(FilterHost)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_PCM_ADAPTER)
    BLT_REGISTER_BUILTIN(PcmAdapter)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_MP3_DECODER)
    BLT_REGISTER_BUILTIN(MP3Decoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_MPEG_AUDIO_DECODER)
    BLT_REGISTER_BUILTIN(MpegAudioDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_VORBIS_DECODER)
    BLT_REGISTER_BUILTIN(VorbisDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_FLAC_DECODER)
    BLT_REGISTER_BUILTIN(FlacDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_ALAC_DECODER)
    BLT_REGISTER_BUILTIN(AlacDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_AAC_DECODER)
    BLT_REGISTER_BUILTIN(AacDecoder)
#endif
#if 1
#if defined(BLT_CONFIG_MODULES_ENABLE_WMA_DECODER)
    BLT_REGISTER_BUILTIN(WmaDecoder)
#endif
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_M4A_DECODER)
    BLT_REGISTER_BUILTIN(M4aDecoder)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_AMR_DECODER)
    BLT_REGISTER_BUILTIN(AmrDecoder)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_DEMO_DECODER)
    BLT_REGISTER_BUILTIN(DemoDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_FFMPEG_DECODER)
    BLT_REGISTER_BUILTIN(FfmpegDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_DDPLUS_DECODER)
    BLT_REGISTER_BUILTIN(DolbyDigitalPlusDecoder)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_WMS_PROTOCOL)
    BLT_REGISTER_BUILTIN(WmsProtocol)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_WIN32_AUDIO_OUTPUT)
    BLT_REGISTER_BUILTIN(Win32AudioOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_DX9_VIDEO_OUTPUT)
    BLT_REGISTER_BUILTIN(Dx9VideoOutput)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_AudioMixer_OUTPUT)
    BLT_REGISTER_BUILTIN(AudioMixer)
#endif
#if defined(BLT_CONFIG_MODULES_ENABLE_OSS_OUTPUT)
    BLT_REGISTER_BUILTIN(OssOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_ALSA_OUTPUT)
    BLT_REGISTER_BUILTIN(AlsaOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_ESD_OUTPUT)
    BLT_REGISTER_BUILTIN(EsdOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_OSX_AUDIO_UNITS_OUTPUT)
    BLT_REGISTER_BUILTIN(OsxAudioUnitsOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_OSX_AUDIO_QUEUE_OUTPUT)
    BLT_REGISTER_BUILTIN(OsxAudioQueueOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_OSX_VIDEO_OUTPUT)
    BLT_REGISTER_BUILTIN(OsxVideoOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_SDL_OUTPUT)
    BLT_REGISTER_BUILTIN(SdlOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_NEUTRINO_OUTPUT)
    BLT_REGISTER_BUILTIN(NeutrinoOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_DEBUG_OUTPUT)
    BLT_REGISTER_BUILTIN(DebugOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_FILE_OUTPUT)
    BLT_REGISTER_BUILTIN(FileOutput)
#endif

#if defined(BLT_CONFIG_MODULES_ENABLE_NULL_OUTPUT)
    BLT_REGISTER_BUILTIN(NullOutput)
#endif
#endif
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_Builtins_GetDefaultOutput
+---------------------------------------------------------------------*/
#define BLT_STRINGIFY_1(x) #x
#define BLT_STRINGIFY(x) BLT_STRINGIFY_1(x)

BLT_Result
BLT_Builtins_GetDefaultAudioOutput(BLT_CString* name, BLT_CString* type)
{
#if defined(BLT_CONFIG_MODULES_DEFAULT_AUDIO_OUTPUT_NAME)
    if (name) *name = BLT_STRINGIFY(BLT_CONFIG_MODULES_DEFAULT_AUDIO_OUTPUT_NAME);
#else
    if (name) *name = BLT_BUILTINS_DEFAULT_AUDIO_OUTPUT_NAME;
#endif

#if defined(BLT_CONFIG_MODULES_DEFAULT_AUDIO_OUTPUT_TYPE)
    if (type) *type = BLT_STRINGIFY(BLT_CONFIG_MODULES_DEFAULT_AUDIO_OUTPUT_TYPE);
#else
    if (type) *type = BLT_BUILTINS_DEFAULT_AUDIO_OUTPUT_TYPE;
#endif

    return BLT_SUCCESS;
}

BLT_Result
BLT_Builtins_GetDefaultVideoOutput(BLT_CString* name, BLT_CString* type)
{
#if defined(BLT_CONFIG_MODULES_DEFAULT_VIDEO_OUTPUT_NAME)
    if (name) *name = BLT_STRINGIFY(BLT_CONFIG_MODULES_DEFAULT_VIDEO_OUTPUT_NAME);
#else
    if (name) *name = BLT_BUILTINS_DEFAULT_VIDEO_OUTPUT_NAME;
#endif

#if defined(BLT_CONFIG_MODULES_DEFAULT_VIDEO_OUTPUT_TYPE)
    if (type) *type = BLT_STRINGIFY(BLT_CONFIG_MODULES_DEFAULT_VIDEO_OUTPUT_TYPE);
#else
    if (type) *type = BLT_BUILTINS_DEFAULT_VIDEO_OUTPUT_TYPE;
#endif

    return BLT_SUCCESS;
}
