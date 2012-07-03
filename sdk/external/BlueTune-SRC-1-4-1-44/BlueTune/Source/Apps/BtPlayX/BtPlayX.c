/*****************************************************************
|
|   BlueTune - Command-Line Player
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file 
 * Main code for BtPlay
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Atomix.h"
#include "BlueTune.h"
#include "BltDecoderX.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    ATX_String    name;
    unsigned char value[16];
} BLTP_Key;

typedef struct {
    BLT_CString  audio_output_name;
    BLT_CString  audio_output_type;
    BLT_CString  video_output_name;
    BLT_CString  video_output_type;
    unsigned int duration;
    unsigned int verbosity;
    ATX_List*    keys;
} BLTP_Options;

typedef struct  {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_EventListener);
    ATX_IMPLEMENTS(ATX_PropertyListener);
    ATX_IMPLEMENTS(BLT_KeyManager);
} BLTP;

/*----------------------------------------------------------------------
|    globals
+---------------------------------------------------------------------*/
BLTP_Options Options;

/*----------------------------------------------------------------------
|    flags
+---------------------------------------------------------------------*/
#define BLTP_VERBOSITY_STREAM_TOPOLOGY      1
#define BLTP_VERBOSITY_STREAM_INFO          2
#define BLTP_VERBOSITY_MISC                 4

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/
#define BLTP_CHECK(result)                                      \
do {                                                            \
    if (BLT_FAILED(result)) {                                   \
        fprintf(stderr, "runtime error on line %d\n", __LINE__);\
        exit(1);                                                \
    }                                                           \
} while(0)

/*----------------------------------------------------------------------
|    BLTP_PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
BLTP_PrintUsageAndExit(int exit_code)
{
    ATX_ConsoleOutput(
        "--- BlueTune command line player (extended)\n"
        "--- BlueTune version " BLT_BLUETUNE_SDK_VERSION_STRING " build " BLT_SVN_VERSION_STRING "\n\n"
        "usage: btplayx [options] <input-spec> [<input-spec>..]\n"
        "  each <input-spec> is either an input name (file or URL), or an input type\n"
        "  (--input-type=<type>) followed by an input name\n"
        "\n");
    ATX_ConsoleOutput(
        "options:\n"
        "  -h\n"
        "  --help\n" 
        "  --audio-output=<name>\n"
        "  --audio-output-type=<type>\n"
        "  --video-output=<name>\n"
        "  --video-output-type=<type>\n"
        "  --duration=<n> (seconds)\n"
        "  --verbose=<name> : print messages related to <name>, where name is\n"
        "                     'stream-topology', 'stream-info', or 'all'\n"
        "                     (multiple --verbose= options can be specified)\n"
        "  --key=<name>:<value> : content decryption key for content ID <name>.\n"
        "                         The key value is in hexadecimal\n");
    exit(exit_code);
}

/*----------------------------------------------------------------------
|    BLTP_ParseHexNibble
+---------------------------------------------------------------------*/
static int
BLTP_ParseHexNibble(char nibble)
{
    switch (nibble) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a':
        case 'A': return 0x0A;
        case 'b':
        case 'B': return 0x0B;
        case 'c':
        case 'C': return 0x0C;
        case 'd':
        case 'D': return 0x0D;
        case 'e':
        case 'E': return 0x0E;
        case 'f':
        case 'F': return 0x0F;
        default: return -1;
    }
}

/*----------------------------------------------------------------------
|    BLTP_ParseKey
+---------------------------------------------------------------------*/
static void
BLTP_ParseKey(const char* name_and_value)
{
    BLTP_Key*    key;
    unsigned int length = ATX_StringLength(name_and_value);
    
    /* we need at least a ':' followed by 32 hex chars */
    if (length < 33) {
        fprintf(stderr, "ERROR: invalid syntax for --key argument\n");
        return;
    }
    
    key = (BLTP_Key*)ATX_AllocateZeroMemory(sizeof(BLTP_Key));
    ATX_String_AssignN(&key->name, name_and_value, length-33);
    {
        unsigned int i;
        unsigned int x = 0;
        for (i=length-32; i<length; i+=2) {
            int nib1 = BLTP_ParseHexNibble(name_and_value[i  ]);
            int nib2 = BLTP_ParseHexNibble(name_and_value[i+1]);
            if (nib1 < 0 || nib2 < 0) {
                fprintf(stderr, "ERROR: invalid syntax for --key argument\n");
            }
            key->value[x++] = (nib1<<4) | nib2;
        }
    }
    
    ATX_List_AddData(Options.keys, key);
}

/*----------------------------------------------------------------------
|    BLTP_ParseCommandLine
+---------------------------------------------------------------------*/
static char**
BLTP_ParseCommandLine(char** args)
{
    char* arg;

    /* setup default values for options */
    Options.audio_output_name = BLT_DECODER_DEFAULT_OUTPUT_NAME;
    Options.audio_output_type = NULL;
    Options.video_output_name = BLT_DECODER_DEFAULT_OUTPUT_NAME;
    Options.video_output_type = NULL;
    Options.duration    = 0;
    Options.verbosity   = 0;
    ATX_List_Create(&Options.keys);
    
    while ((arg = *args)) {
        if (ATX_StringsEqual(arg, "-h") ||
            ATX_StringsEqual(arg, "--help")) {
            BLTP_PrintUsageAndExit(0);
        } else if (ATX_StringsEqualN(arg, "--audio-output=", 15)) {
            Options.audio_output_name = arg+15;
        } else if (ATX_StringsEqualN(arg, "--audio-output-type=",20)) {
            Options.audio_output_type = arg+20;
        } else if (ATX_StringsEqualN(arg, "--video-output=", 15)) {
            Options.video_output_name = arg+15;
        } else if (ATX_StringsEqualN(arg, "--video-output-type=", 20)) {
            Options.video_output_type = arg+20;
        } else if (ATX_StringsEqualN(arg, "--duration=", 11)) {
            int duration = 0;
            ATX_ParseInteger(arg+11, &duration, ATX_FALSE);
            Options.duration = duration;
        } else if (ATX_StringsEqualN(arg, "--verbose=", 10)) {
            if (ATX_StringsEqual(arg+10, "stream-topology")) {
                Options.verbosity |= BLTP_VERBOSITY_STREAM_TOPOLOGY;
            } else if (ATX_StringsEqual(arg+10, "stream-info")) {
                Options.verbosity |= BLTP_VERBOSITY_STREAM_INFO;
            } else if (ATX_StringsEqual(arg+10, "all")) {
                Options.verbosity = 0xFFFFFFFF;
            }
        } else if (ATX_StringsEqualN(arg, "--key=", 6)) {
            BLTP_ParseKey(arg+6);
        } else {
            return args;
        }
        ++args;
    }

    return args;
}

/*----------------------------------------------------------------------
|    BLTP_OnStreamPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
BLTP_OnStreamPropertyChanged(ATX_PropertyListener*    self,
                             ATX_CString              name,
                             const ATX_PropertyValue* value)    
{
    BLT_COMPILER_UNUSED(self);

    if (!(Options.verbosity & BLTP_VERBOSITY_STREAM_INFO)) return;
    
    if (name == NULL) {
        ATX_ConsoleOutput("BLTP::OnStreamPropertyChanged - All Properties Cleared\n");
    } else {
        if (value == NULL) {
            ATX_ConsoleOutputF("BLTP::OnStreamPropertyChanged - Property %s cleared\n", name);
        } else {
            ATX_ConsoleOutputF("BLTP::OnStreamPropretyChanged - %s = ", name);
            switch (value->type) {
              case ATX_PROPERTY_VALUE_TYPE_STRING:
                ATX_ConsoleOutputF("%s\n", value->data.string);
                break;

              case ATX_PROPERTY_VALUE_TYPE_INTEGER:
                ATX_ConsoleOutputF("%d\n", value->data.integer);
                break;

              default:
                ATX_ConsoleOutput("\n");
            }
        }
    }
}

/*----------------------------------------------------------------------
|    BLTP_GetKeyByName
+---------------------------------------------------------------------*/
BLT_METHOD
BLTP_GetKeyByName(BLT_KeyManager* self,
                  const char*     name,
                  unsigned char*  key, 
                  unsigned int*   key_size)
{
    ATX_ListItem* item = ATX_List_GetFirstItem(Options.keys);

    ATX_COMPILER_UNUSED(self);

    /* check the key size */
    if (*key_size < 16) {
        *key_size = 16;
        return BLT_ERROR_BUFFER_TOO_SMALL;
    }
    
    for (; item; item = ATX_ListItem_GetNext(item)) {
        BLTP_Key* key_info = (BLTP_Key*)ATX_ListItem_GetData(item);
        if (ATX_String_Equals(&key_info->name, name, ATX_FALSE)) {
            /* found a match */
            ATX_CopyMemory(key, key_info->value, 16);
            *key_size = 16;
            return BLT_SUCCESS;
        }
    }
    
    return ATX_ERROR_NO_SUCH_ITEM;
}
                               
/*----------------------------------------------------------------------
|    BLTP_ShowStreamTopology
+---------------------------------------------------------------------*/
static void
BLTP_ShowStreamTopology(ATX_Object* source)
{
    BLT_Stream*        stream;
    BLT_MediaNode*     node;
    BLT_StreamNodeInfo s_info;
    BLT_Result         result;

    /* cast the source object to a stream object */
    stream = ATX_CAST(source, BLT_Stream);
    if (stream == NULL) return;

    result = BLT_Stream_GetFirstNode(stream, &node);
    if (BLT_FAILED(result)) return;
    while (node) {
        const char* name;
        BLT_MediaNodeInfo n_info;
        result = BLT_Stream_GetStreamNodeInfo(stream, node, &s_info);
        if (BLT_FAILED(result)) break;
        result = BLT_MediaNode_GetInfo(node, &n_info);
        if (BLT_SUCCEEDED(result)) {
            name = n_info.name ? n_info.name : "?";
        } else {
            name = "UNKNOWN";
        }

        if (s_info.input.connected) {
            ATX_ConsoleOutput("-");
        } else {
            ATX_ConsoleOutput(".");
        }

        switch (s_info.input.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_NONE:
            ATX_ConsoleOutput("!"); break;
          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            ATX_ConsoleOutput("#"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            ATX_ConsoleOutput(">"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
            ATX_ConsoleOutput("<"); break;
          default:
            ATX_ConsoleOutput("@"); break;
        }

        ATX_ConsoleOutputF("[%s]", name);

        switch (s_info.output.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_NONE:
            ATX_ConsoleOutput("!"); break;
          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            ATX_ConsoleOutput("#"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            ATX_ConsoleOutput(">"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
            ATX_ConsoleOutput("<"); break;
          default:
            ATX_ConsoleOutput("@"); break;
        }

        if (s_info.output.connected) {
            ATX_ConsoleOutput("-");
        } else {
            ATX_ConsoleOutput(".");
        }

        result = BLT_Stream_GetNextNode(stream, node, &node);
        if (BLT_FAILED(result)) break;
    }
    ATX_ConsoleOutput("\n");
}

/*----------------------------------------------------------------------
|    BLTP_OnEvent
+---------------------------------------------------------------------*/
BLT_VOID_METHOD 
BLTP_OnEvent(BLT_EventListener* self,
             ATX_Object*        source,
             BLT_EventType      type,
             const BLT_Event*   event)
{
    BLT_COMPILER_UNUSED(self);
    if (type == BLT_EVENT_TYPE_STREAM_INFO && 
        Options.verbosity & BLTP_VERBOSITY_STREAM_INFO) {
        const BLT_StreamInfoEvent* e = (BLT_StreamInfoEvent*)event;
        ATX_ConsoleOutputF("BLTP::OnEvent - info update=%x\n", e->update_mask);

        if (e->update_mask & BLT_STREAM_INFO_MASK_TYPE) {
            ATX_ConsoleOutputF("  stream type     = %ld (", (int)e->info.type);
            switch (e->info.type) {
                case BLT_STREAM_TYPE_UNKNOWN:     ATX_ConsoleOutputF("UNKNOWN)\n");     break;
                case BLT_STREAM_TYPE_MULTIPLEXED: ATX_ConsoleOutputF("MULTIPLEXED)\n"); break;
                case BLT_STREAM_TYPE_AUDIO:       ATX_ConsoleOutputF("AUDIO)\n");       break;
                case BLT_STREAM_TYPE_VIDEO:       ATX_ConsoleOutputF("VIDEO)\n");       break;
                default:                          ATX_ConsoleOutputF("OTHER)\n");       break;
            }
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_ID) {
            ATX_ConsoleOutputF("  id              = %ld\n", e->info.id);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
            ATX_ConsoleOutputF("  nominal_bitrate = %ld\n", e->info.nominal_bitrate);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
            ATX_ConsoleOutputF("  average_bitrate = %ld\n", e->info.average_bitrate);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
            ATX_ConsoleOutputF("  instant_bitrate = %ld\n", e->info.instant_bitrate);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_SIZE) {
            ATX_ConsoleOutputF("  size            = %ld\n", e->info.size);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_DURATION) {
            ATX_ConsoleOutputF("  duration        = %ld\n", e->info.duration);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
            ATX_ConsoleOutputF("  sample_rate     = %ld\n", e->info.sample_rate);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
            ATX_ConsoleOutputF("  channel_count   = %ld\n", e->info.channel_count);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_WIDTH) {
            ATX_ConsoleOutputF("  width           = %ld\n", e->info.width);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_HEIGHT) {
            ATX_ConsoleOutputF("  height          = %ld\n", e->info.height);
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_FLAGS) {
            ATX_ConsoleOutputF("  flags           = %x", e->info.flags);
            if (e->info.flags & BLT_STREAM_INFO_FLAG_VBR) {
                ATX_ConsoleOutputF(" (VBR)\n");
            } else {
                ATX_ConsoleOutput("\n");
            }
        }
        if (e->update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
            ATX_ConsoleOutputF("  data_type       = %s\n", 
                               e->info.data_type ? e->info.data_type : "");
        }
    } else if (type == BLT_EVENT_TYPE_STREAM_TOPOLOGY &&
               Options.verbosity & BLTP_VERBOSITY_STREAM_TOPOLOGY) {
        const BLT_StreamTopologyEvent* e = (BLT_StreamTopologyEvent*)event;
        switch (e->type) {
          case BLT_STREAM_TOPOLOGY_NODE_ADDED:
            ATX_ConsoleOutput("STREAM TOPOLOGY: node added\n");  
            break;
          case BLT_STREAM_TOPOLOGY_NODE_REMOVED:
            ATX_ConsoleOutput("STREAM TOPOLOGY: node removed\n");
            break;
          case BLT_STREAM_TOPOLOGY_NODE_CONNECTED:
            ATX_ConsoleOutput("STREAM TOPOLOGY: node connected\n");
            break;
          default:
            break;
        }
        BLTP_ShowStreamTopology(source);
    }
}

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(BLTP)
    ATX_GET_INTERFACE_ACCEPT(BLTP, BLT_EventListener)
    ATX_GET_INTERFACE_ACCEPT(BLTP, ATX_PropertyListener)
    ATX_GET_INTERFACE_ACCEPT(BLTP, BLT_KeyManager)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLTP, ATX_PropertyListener)
    BLTP_OnStreamPropertyChanged
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_EventListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLTP, BLT_EventListener)
    BLTP_OnEvent
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_KeyManager interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLTP, BLT_KeyManager)
    BLTP_GetKeyByName
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLTP_CheckElapsedTime
+---------------------------------------------------------------------*/
static BLT_Result
BLTP_CheckElapsedTime(BLT_DecoderX* decoder, unsigned int duration)
{
    BLT_DecoderStatus status;
    
    if (duration == 0) return BLT_SUCCESS;
    BLT_DecoderX_GetStatus(decoder, &status);
    if (status.time_stamp.seconds > (int)duration) {
        ATX_ConsoleOutput("END of specified duration\n");
        return BLT_FAILURE;
    }
    return BLT_SUCCESS;
}

/* optional plugins */
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_FFMPEG_DECODER)
#include "BltFfmpegDecoder.h"
#endif
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_IPP_DECODER)
#include "BltIppDecoder.h"
#endif
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_OSX_VIDEO_OUTPUT)
#include "BltOsxVideoOutput.h"
#endif
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_DX9_VIDEO_OUTPUT)
#include "BltDx9VideoOutput.h"
#endif
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_SDL_VIDEO_OUTPUT)
#include "BltSdlVideoOutput.h"
#endif

/*----------------------------------------------------------------------
|    BLTP_RegisterPlugins
+---------------------------------------------------------------------*/
static void
BLTP_RegisterPlugins(BLT_DecoderX* decoder)
{
    BLT_Module* module = NULL;
    BLT_Result  result;
    
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_FFMPEG_DECODER)
    result = BLT_FfmpegDecoderModule_GetModuleObject(&module);
    if (BLT_SUCCEEDED(result)) BLT_DecoderX_RegisterModule(decoder, module);
#endif
    
#if defined(BLT_CONFIG_BTPLAYX_ENABLE_IPP_DECODER)
    result = BLT_IppDecoderModule_GetModuleObject(&module);
    if (BLT_SUCCEEDED(result)) BLT_DecoderX_RegisterModule(decoder, module);
#endif

#if defined(BLT_CONFIG_BTPLAYX_ENABLE_OSX_VIDEO_OUTPUT)
    result = BLT_OsxVideoOutputModule_GetModuleObject(&module);
    if (BLT_SUCCEEDED(result)) BLT_DecoderX_RegisterModule(decoder, module);
#endif

#if defined(BLT_CONFIG_BTPLAYX_ENABLE_DX9_VIDEO_OUTPUT)
    result = BLT_Dx9VideoOutputModule_GetModuleObject(&module);
    if (BLT_SUCCEEDED(result)) BLT_DecoderX_RegisterModule(decoder, module);
#endif

#if defined(BLT_CONFIG_BTPLAYX_ENABLE_SDL_VIDEO_OUTPUT)
    result = BLT_SdlVideoOutputModule_GetModuleObject(&module);
    if (BLT_SUCCEEDED(result)) BLT_DecoderX_RegisterModule(decoder, module);
#endif
}

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    BLT_DecoderX* decoder;
    BLT_CString   input_name;
    BLT_CString   input_type = NULL;
    BLTP          player;
    BLT_Result    result;

    /*mtrace();*/
    BLT_COMPILER_UNUSED(argc);

    /* parse command line */
    if (argc < 2) BLTP_PrintUsageAndExit(0);
    argv = BLTP_ParseCommandLine(argv+1);

    /* create a decoder */
    result = BLT_DecoderX_Create(&decoder);
    BLTP_CHECK(result);

    /* setup our interfaces */
    ATX_SET_INTERFACE(&player, BLTP, BLT_EventListener);
    ATX_SET_INTERFACE(&player, BLTP, ATX_PropertyListener);
    ATX_SET_INTERFACE(&player, BLTP, BLT_KeyManager);

    /* listen to stream events */
    BLT_DecoderX_SetEventListener(decoder, &ATX_BASE(&player, BLT_EventListener));
             
    /* listen to audio stream properties events */
    {
        ATX_Properties* properties;
        BLT_DecoderX_GetAudioStreamProperties(decoder, &properties);
        ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&player, ATX_PropertyListener), NULL);
    }

    /* listen to video stream properties events */
    {
        ATX_Properties* properties;
        BLT_DecoderX_GetVideoStreamProperties(decoder, &properties);
        ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&player, ATX_PropertyListener), NULL);
    }

    /* listen to input stream properties events */
    {
        ATX_Properties*   properties;
        BLT_DecoderX_GetInputStreamProperties(decoder, &properties);
        ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&player, ATX_PropertyListener), NULL);
    }

    /* register as the key manager */
    {
        ATX_Properties*   properties;
        ATX_PropertyValue key_manager_prop;
        BLT_DecoderX_GetProperties(decoder, &properties);
        ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&player, ATX_PropertyListener), NULL);
        
        key_manager_prop.type = ATX_PROPERTY_VALUE_TYPE_POINTER;
        key_manager_prop.data.pointer = &ATX_BASE(&player, BLT_KeyManager);
        ATX_Properties_SetProperty(properties, BLT_KEY_MANAGER_PROPERTY, &key_manager_prop);
    }
    
    /* register builtin modules */
    result = BLT_DecoderX_RegisterBuiltins(decoder);
    BLTP_CHECK(result);

    /* register some optional plugins */
    BLTP_RegisterPlugins(decoder);
    
    /* set the audio output */
    result = BLT_DecoderX_SetAudioOutput(decoder, Options.audio_output_name, Options.audio_output_type);
    if (BLT_FAILED(result)) {
        fprintf(stderr, "SetAudioOutput failed: %d (%s)\n", result, BLT_ResultText(result));
        exit(1);
    }
    /* set the video output */
    result = BLT_DecoderX_SetVideoOutput(decoder, Options.video_output_name, Options.video_output_type);
    if (BLT_FAILED(result)) {
        fprintf(stderr, "SetVideoOutput failed: %d (%s)\n", result, BLT_ResultText(result));
        exit(1);
    }

    /* process each input in turn */
    while ((input_name = *argv++)) {
        if (ATX_StringsEqualN(input_name, "--input-type=", 13)) {
            input_type = input_name+13;
            continue;
        }

        /* set the input name */
        result = BLT_DecoderX_SetInput(decoder, input_name, input_type);
        if (BLT_FAILED(result)) {
            ATX_ConsoleOutputF("SetInput failed: %d (%s)\n", result, BLT_ResultText(result));
            input_type = NULL;
            continue;
        }

        /* pump the packets */
        do {
            /* process one packet */
            result = BLT_DecoderX_PumpPacket(decoder);
            
            /* if a duration is specified, check if we have exceeded it */
            if (BLT_SUCCEEDED(result)) result = BLTP_CheckElapsedTime(decoder, Options.duration);
        } while (BLT_SUCCEEDED(result));
        if (Options.verbosity & BLTP_VERBOSITY_MISC) {
            ATX_ConsoleOutputF("final result = %d (%s)\n", result, BLT_ResultText(result));
        }

        /* reset input type */
        input_type = NULL;
    }

    /* destroy the decoder */
    BLT_DecoderX_Destroy(decoder);

    return 0;
}
