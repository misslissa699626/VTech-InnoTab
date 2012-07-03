/*****************************************************************
|
|   BlueTune - Player Web Service
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Atomix.h"
#include "Neptune.h"
#include "BlueTune.h"
#include "BtPlayerServer.h"

/*----------------------------------------------------------------------
|    logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("bluetune.player.web")

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
const char* BT_CONTROL_FORM = 
"<form action='/control/form' method='get'><input type='submit' value='Refresh Status'/></form>"
"<form action='/player/set-input' method='get'><input type='text' name='name'/><input type='submit' value='Set Input'/></form>"
"<form action='/player/seek' method='get'><input type='text' name='timecode'/><input type='submit' value='Seek To Timecode'/></form>"
"<form action='/player/play' method='get'><input type='submit' value='Play'/></form>"
"<form action='/player/stop' method='get'><input type='submit' value='Stop'/></form>"
"<form action='/player/pause' method='get'><input type='submit' value='Pause'/></form>";

/*----------------------------------------------------------------------
|    BtPlayerServer::BtPlayerServer
+---------------------------------------------------------------------*/
BtPlayerServer::BtPlayerServer(const char* web_root, unsigned int port) :
    m_WebRoot(web_root),
    m_DecoderState(BLT_DecoderServer::STATE_STOPPED)
{
    // initialize status fields
    ATX_SetMemory(&m_StreamInfo, 0, sizeof(m_StreamInfo));
    ATX_Properties_Create(&m_CoreProperties);
    ATX_Properties_Create(&m_StreamProperties);
    
    // set ourselves as the player event listener
    m_Player.SetEventListener(this);
    
    // create the http server
    m_HttpServer = new NPT_HttpServer(port);
    
    // attach a file handler for the ajax player
    m_HttpServer->AddRequestHandler(new NPT_HttpFileRequestHandler("/control/ajax", web_root, true, "index.html"), "/control/ajax", true);
    
    // attach ourselves as a dynamic handler for the form control
    m_HttpServer->AddRequestHandler(this, "/control/form", false);
    
    // attach ourselves as a dynamic handler for commands
    m_HttpServer->AddRequestHandler(this, "/player", true);
}

/*----------------------------------------------------------------------
|    BtPlayerServer::~BtPlayerServer
+---------------------------------------------------------------------*/
BtPlayerServer::~BtPlayerServer()
{
    delete m_HttpServer;
    ATX_DESTROY_OBJECT(m_CoreProperties);
    ATX_DESTROY_OBJECT(m_StreamProperties);
}

/*----------------------------------------------------------------------
|    BtPlayerServer::Run
+---------------------------------------------------------------------*/
void
BtPlayerServer::Run()
{
    NPT_Result result;
    do {
        result = m_Player.PumpMessage();
    } while (NPT_SUCCEEDED(result));
}

/*----------------------------------------------------------------------
|    BtPlayerServer::Loop
+---------------------------------------------------------------------*/
NPT_Result
BtPlayerServer::Loop()
{
    // create a thread to handle notifications
    NPT_Thread notification_thread(*this);
    notification_thread.Start();

    NPT_Result result = m_HttpServer->Loop();

    // wait for the notification thread to end
    notification_thread.Wait();
    
    return result;
}

/*----------------------------------------------------------------------
|    BtPlayerServer::DoSeekToTimecode
+---------------------------------------------------------------------*/
void
BtPlayerServer::DoSeekToTimecode(const char* time)
{
    BLT_UInt8    val[4] = {0,0,0,0};
    ATX_Size     length = ATX_StringLength(time);
    unsigned int val_c = 0;
    bool         has_dot = false;
    
    if (length != 11 && length != 8 && length != 5 && length != 2) return;
    
    do {
        if ( time[0] >= '0' && time[0] <= '9' && 
             time[1] >= '0' && time[0] <= '9' &&
            (time[2] == ':' || time[2] == '.' || time[2] == '\0')) {
            if (time[2] == '.') {
                if (length != 5) return; // dots only on the last part
                has_dot = true;
            } else {
                if (val_c == 3) return; // too many parts
            }
            val[val_c++] = (time[0]-'0')*10 + (time[1]-'0');
            length -= (time[2]=='\0')?2:3;
            time += 3;
        } else {
            return;
        }
    } while (length >= 2);
    
    BLT_UInt8 h,m,s,f;
    if (has_dot) --val_c;    
    h = val[(val_c+1)%4];
    m = val[(val_c+2)%4];
    s = val[(val_c+3)%4];
    f = val[(val_c  )%4];

    m_Player.SeekToTimeStamp(h,m,s,f);
}

/*----------------------------------------------------------------------
|    BtPlayerServer::SendStatus
+---------------------------------------------------------------------*/
NPT_Result
BtPlayerServer::SendStatus(NPT_HttpResponse& response, NPT_UrlQuery& query)
{
    NPT_String json;
    
    // json start
    const char* json_callback = query.GetField("callback");
    if (json_callback) {
        json += NPT_UrlQuery::UrlDecode(json_callback)+'(';
    }
    json += '{';
    
    // state
    json += "\"state\": ";
    switch (m_DecoderState) {
      case BLT_DecoderServer::STATE_STOPPED:
        json += "\"STOPPED\"";
        break;

      case BLT_DecoderServer::STATE_PLAYING:
        json += "\"PLAYING\"";
        break;

      case BLT_DecoderServer::STATE_PAUSED:
        json += "\"PAUSED\"";
        break;

      case BLT_DecoderServer::STATE_EOS:
        json += "\"END OF STREAM\"";
        break;

      default:
        json += "\"UNKNOWN\"";
        break;
    }
    json += ',';
    
    // timecode
    json += NPT_String::Format("\"timecode\": \"%02d:%02d:%02d\", ",
                               m_DecoderTimecode.h,
                               m_DecoderTimecode.m,
                               m_DecoderTimecode.s);
    
    // position
    json += NPT_String::Format("\"position\": %f, ", m_DecoderPosition);

    // stream info
    json += "\"streamInfo\": {";
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
        json += NPT_String::Format("\"nominalBitrate\": %d, ", m_StreamInfo.nominal_bitrate);
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
        json += NPT_String::Format("\"averageBitrate\": %d, ", m_StreamInfo.average_bitrate);
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
        json += NPT_String::Format("\"instantBitrate\": %d, ", m_StreamInfo.instant_bitrate);
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_SIZE) {
        json += NPT_String::Format("\"size\": %d, ", m_StreamInfo.size);
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_DURATION) {
        unsigned int seconds = m_StreamInfo.duration/1000;
        json += NPT_String::Format("\"duration\": \"%02d:%02d:%02d\", ", 
                                   (seconds)/36000,
                                   (seconds%3600)/60,
                                   (seconds%60));
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
        json += NPT_String::Format("\"sampleRate\": %d, ", m_StreamInfo.sample_rate);
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
        json += NPT_String::Format("\"channelCount\": %d, ", m_StreamInfo.channel_count);
    }
    if (m_StreamInfo.mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
        json += NPT_String::Format("\"dataType\": \"%s\", ", m_StreamInfo.data_type);
    }
    json += "},";
    
    // stream properties
    
    // json end
    json += '}';
    if (json_callback) {
        json += ')';
    }
    
    // send the html document
    NPT_HttpEntity* entity = response.GetEntity();
    entity->SetContentType("application/json");
    entity->SetInputStream(json);        
    
    NPT_LOG_FINE_1("status: %s", json.GetChars());
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BtPlayerServer::SendControlForm
+---------------------------------------------------------------------*/
NPT_Result
BtPlayerServer::SendControlForm(NPT_HttpResponse& response, const char* msg)
{
    // create the html document
    NPT_String html = "<html>";
    
    // optional message
    if (msg && msg[0]) {
        html += "<";
        html += msg;
        html += "><p>";
    }
    
    // status
    html += "<p><b>State: </b>";
    switch (m_DecoderState) {
      case BLT_DecoderServer::STATE_STOPPED:
        html += "[STOPPED]";
        break;

      case BLT_DecoderServer::STATE_PLAYING:
        html += "[PLAYING]";
        break;

      case BLT_DecoderServer::STATE_PAUSED:
        html += "[PAUSED]";
        break;

      case BLT_DecoderServer::STATE_EOS:
        html += "[END OF STREAM]";
        break;

      default:
        html += "[UNKNOWN]";
        break;
    }
    html += "</p><p><b>Time Code: </b>";

    html += NPT_String::Format("\"%02d:%02d:%02d\"",
                               m_DecoderTimecode.h,
                               m_DecoderTimecode.m,
                               m_DecoderTimecode.s);
    html += "</p>";
    
    html += "<p>";
    html += "<b>Content Format: </b>";
    if (m_StreamInfo.data_type) {
        html += m_StreamInfo.data_type;
    }
    html += "<br>";
    
    
    // control form
    html += BT_CONTROL_FORM;
    html += "</html>";
    
    // send the html document
    NPT_HttpEntity* entity = response.GetEntity();
    entity->SetContentType("text/html");
    entity->SetInputStream(html);        

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BtPlayerServer::SetupResponse
+---------------------------------------------------------------------*/
NPT_Result 
BtPlayerServer::SetupResponse(NPT_HttpRequest&              request,
                              const NPT_HttpRequestContext& /*context*/,
                              NPT_HttpResponse&             response)
{
    const NPT_Url&    url  = request.GetUrl();
    const NPT_String& path = url.GetPath();
    NPT_UrlQuery      query;
    
    // parse the query part, if any
    if (url.HasQuery()) {
        query.Parse(url.GetQuery());
    }
    
    // lock the player 
    NPT_AutoLock lock(m_Lock);
    
    // handle form requests
    if (path == "/control/form") {
        return SendControlForm(response, NULL);
    }
    
    // handle status requests
    if (path == "/player/status") {
        return SendStatus(response, query);
    }
    
    // handle commands
    const char* mode_field = query.GetField("mode");
    const char* form_msg = "OK";
    bool use_form = false;
    if (mode_field && NPT_StringsEqual(mode_field, "form")) {
        use_form = true;
    }
    if (path == "/player/set-input") {
        const char* name_field = query.GetField("name");
        if (name_field) {
            NPT_String name = NPT_UrlQuery::UrlDecode(name_field);
            printf("BtPlayerServer::SetupResponse - set-input %s\n", name.GetChars());
            m_Player.SetInput(name);
        } else {
            form_msg = "INVALID PARAMETERS";
        }
    } else if (path == "/player/play") {
        m_Player.Play();
    } else if (path == "/player/pause") {
        m_Player.Pause();
    } else if (path == "/player/stop") {
        m_Player.Stop();
    } else if (path == "/player/seek") {
        const char* timecode_field = query.GetField("timecode");
        const char* position_field = query.GetField("position");
        if (timecode_field) {
            NPT_String timecode = NPT_UrlQuery::UrlDecode(timecode_field);
            DoSeekToTimecode(timecode);
        } else if (position_field) {
            unsigned int position;
            if (NPT_SUCCEEDED(NPT_ParseInteger(position_field, position))) {
                m_Player.SeekToPosition(position, 100);
            }
        } else {
            form_msg = "INVALID PARAMETER";
        }
    } else if (path == "/player/set-volume") {
        const char* volume_field = query.GetField("volume");
        if (volume_field) {
            unsigned int volume;
            if (NPT_SUCCEEDED(NPT_ParseInteger(volume_field, volume))) {
                m_Player.SetVolume((float)volume/100.0f);
            }
        } else {
            form_msg = "INVALID PARAMETER";
        }
    }
    
    if (use_form) {
        return SendControlForm(response, form_msg);
    } else {
        NPT_HttpEntity* entity = response.GetEntity();
        entity->SetContentType("application/json");
        entity->SetInputStream("{}");
        return NPT_SUCCESS;
    }

    printf("BtPlayerServer::SetupResponse - command not found\n");
    
    response.SetStatus(404, "Command Not Found");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BtPlayerServer::OnDecoderStateNotification
+---------------------------------------------------------------------*/
void
BtPlayerServer::OnDecoderStateNotification(BLT_DecoderServer::State state)
{
    NPT_AutoLock lock(m_Lock);
    m_DecoderState = state;
}

/*----------------------------------------------------------------------
|    BtPlayerServer::OnStreamTimeCodeNotification
+---------------------------------------------------------------------*/
void 
BtPlayerServer::OnStreamTimeCodeNotification(BLT_TimeCode time_code)
{
    NPT_AutoLock lock(m_Lock);
    m_DecoderTimecode = time_code;
}

/*----------------------------------------------------------------------
|    BtPlayerServer::OnStreamPositionNotification
+---------------------------------------------------------------------*/
void 
BtPlayerServer::OnStreamPositionNotification(BLT_StreamPosition& position)
{
    NPT_AutoLock lock(m_Lock);
    if (position.range) {
        m_DecoderPosition = (float)position.offset/(float)position.range;
    } else {
        m_DecoderPosition = 0.0f;
    }
}

/*----------------------------------------------------------------------
|    BtPlayerServer::OnStreamInfoNotification
+---------------------------------------------------------------------*/
void 
BtPlayerServer::OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info)
{       
    NPT_AutoLock lock(m_Lock);
    unsigned int mask = m_StreamInfo.mask|update_mask;
    ATX_DESTROY_CSTRING(m_StreamInfo.data_type);
    m_StreamInfo = info;
    m_StreamInfo.mask = mask;
    m_StreamInfo.data_type = NULL;
    ATX_SET_CSTRING(m_StreamInfo.data_type, info.data_type);
}

/*----------------------------------------------------------------------
|    BtPlayerServer::OnPropertyNotification
+---------------------------------------------------------------------*/
void 
BtPlayerServer::OnPropertyNotification(BLT_PropertyScope        scope,
                                       const char*              /* source */,
                                       const char*              name,
                                       const ATX_PropertyValue* value)
{
    ATX_Properties* properties = NULL;
    switch (scope) {
        case BLT_PROPERTY_SCOPE_CORE:   properties = m_CoreProperties;   break;
        case BLT_PROPERTY_SCOPE_STREAM: properties = m_StreamProperties; break;
        default: return;
    }
    
    // when the name is NULL or empty, it means that all the properties in that 
    // scope fo that source have been deleted 
    if (name == NULL || name[0] == '\0') {
        ATX_Properties_Clear(properties);
        return;
    }
    
    ATX_Properties_SetProperty(properties, name, value);
}
