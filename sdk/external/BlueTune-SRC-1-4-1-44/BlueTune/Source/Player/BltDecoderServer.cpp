/*****************************************************************
|
|   BlueTune - Decoder Server
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/
/** @file
* BlueTune Async Layer
*/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltDecoder.h"
#include "BltDecoderServer.h"
#include "BltDecoderClient.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.player.decoder-server")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int BLT_PLAYER_LOOP_WAIT_DURATION = 50; // milliseconds

/*----------------------------------------------------------------------
|   BLT_DecoderServer_Message::MessageType
+---------------------------------------------------------------------*/
NPT_Message::Type 
BLT_DecoderServer_Message::MessageType = "BLT_DecoderServer Message";

/*----------------------------------------------------------------------
|    forward references
+---------------------------------------------------------------------*/
BLT_VOID_METHOD 
BLT_DecoderServer_EventListenerWrapper_OnEvent(
    BLT_EventListener* self,
    ATX_Object*        source,
    BLT_EventType      type,
    const BLT_Event*   event);

BLT_VOID_METHOD 
BLT_DecoderServer_PropertyListenerWrapper_OnPropertyChanged(
    ATX_PropertyListener*    self,
    ATX_CString              name,
    const ATX_PropertyValue* value);

/*----------------------------------------------------------------------
|    BLT_EventListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(BLT_DecoderServer_EventListenerWrapper)
    ATX_GET_INTERFACE_ACCEPT(BLT_DecoderServer_EventListenerWrapper, BLT_EventListener)
ATX_END_GET_INTERFACE_IMPLEMENTATION

ATX_BEGIN_INTERFACE_MAP(BLT_DecoderServer_EventListenerWrapper, BLT_EventListener)
    BLT_DecoderServer_EventListenerWrapper_OnEvent
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(BLT_DecoderServer_PropertyListenerWrapper)
    ATX_GET_INTERFACE_ACCEPT(BLT_DecoderServer_PropertyListenerWrapper, ATX_PropertyListener)
ATX_END_GET_INTERFACE_IMPLEMENTATION

ATX_BEGIN_INTERFACE_MAP(BLT_DecoderServer_PropertyListenerWrapper, ATX_PropertyListener)
    BLT_DecoderServer_PropertyListenerWrapper_OnPropertyChanged
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_DecoderServer_Message::Dispatch
+---------------------------------------------------------------------*/
NPT_Result
BLT_DecoderServer_Message::Dispatch(NPT_MessageHandler* handler) 
{
    BLT_DecoderServer_MessageHandler* specific =
        dynamic_cast<BLT_DecoderServer_MessageHandler*>(handler);
    if (specific) {
        return Deliver(specific);
    } else {
        return DefaultDeliver(handler);
    }
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::BLT_DecoderServer
+---------------------------------------------------------------------*/
BLT_DecoderServer::BLT_DecoderServer(NPT_MessageReceiver* client) :
    m_Decoder(NULL),
    m_Client(client),
    m_TimeStampUpdateQuantum(1000),
    m_PositionUpdateRange(BLT_DECODER_SERVER_DEFAULT_POSITION_UPDATE_RANGE),
    m_State(STATE_STOPPED)
{
    // create a queue to receive messages
    m_MessageQueue = new NPT_SimpleMessageQueue();

    // attach the queue as the receiving queue
    SetQueue(m_MessageQueue);
    
    // register ourselves as the message handler
    SetHandler(this);

    // reset some fields
    m_DecoderStatus.position.range  = m_PositionUpdateRange;
    m_DecoderStatus.position.offset = 0;

    // setup our event listener interface
    m_EventListener.outer = this;
    ATX_SET_INTERFACE(&m_EventListener, 
                      BLT_DecoderServer_EventListenerWrapper, 
                      BLT_EventListener);

    // setup our core property listener interface
    m_CorePropertyListener.scope = BLT_PROPERTY_SCOPE_CORE;
    m_CorePropertyListener.source = NULL;
    m_CorePropertyListener.outer = this;
    ATX_SET_INTERFACE(&m_CorePropertyListener, 
                      BLT_DecoderServer_PropertyListenerWrapper, 
                      ATX_PropertyListener);

    // setup our stream property listener interface
    m_StreamPropertyListener.scope = BLT_PROPERTY_SCOPE_STREAM;
    m_StreamPropertyListener.source = NULL;
    m_StreamPropertyListener.outer = this;
    ATX_SET_INTERFACE(&m_StreamPropertyListener, 
                      BLT_DecoderServer_PropertyListenerWrapper, 
                      ATX_PropertyListener);

    // start the thread
    Start();
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::~BLT_DecoderServer
+---------------------------------------------------------------------*/
BLT_DecoderServer::~BLT_DecoderServer()
{
    ATX_LOG_FINE("enter");

    // send a message to our thread to make it terminate
    PostMessage(new NPT_TerminateMessage);
    
    // wait for the thread to terminate
    Wait();

    // delete the message queue
    delete m_MessageQueue;
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::WaitForTerminateMessage
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::WaitForTerminateMessage()
{
    BLT_Result result;
    do {
        result = m_MessageQueue->PumpMessage(NPT_TIMEOUT_INFINITE);
    } while (BLT_SUCCEEDED(result));

    SetState(STATE_TERMINATED);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::Run
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::Run()
{
    BLT_Result result;

    // create the decoder
    result = BLT_Decoder_Create(&m_Decoder);
    if (BLT_FAILED(result)) {
        m_Client->PostMessage(new BLT_DecoderClient_DecoderEventNotificationMessage(
            BLT_DecoderServer::DecoderEvent::EVENT_TYPE_INIT_ERROR,
            result, 
            "error from BLT_Decoder_Create"));
        WaitForTerminateMessage();
        return;
    }
        
    // register as the event handler
    BLT_Decoder_SetEventListener(m_Decoder, 
                                 &ATX_BASE(&m_EventListener, 
                                           BLT_EventListener));

    // listen to core property changes
    {
        ATX_Properties* properties;
        BLT_Decoder_GetProperties(m_Decoder, &properties);
        ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&m_CorePropertyListener, ATX_PropertyListener), NULL);
    }

    // listen to stream property changes
    {
        ATX_Properties* properties;
        BLT_Decoder_GetStreamProperties(m_Decoder, &properties);
        ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&m_StreamPropertyListener, ATX_PropertyListener), NULL);
    }

    // register builtins 
    result = BLT_Decoder_RegisterBuiltins(m_Decoder);
    if (BLT_FAILED(result)) {
        m_Client->PostMessage(new BLT_DecoderClient_DecoderEventNotificationMessage(
            BLT_DecoderServer::DecoderEvent::EVENT_TYPE_INIT_ERROR,
            result, 
            "error from BLT_Decoder_RegisterBuiltins"));
        WaitForTerminateMessage();
        return;
    }

    // set default output, default type
    result = BLT_Decoder_SetOutput(m_Decoder, 
                                   BLT_DECODER_DEFAULT_OUTPUT_NAME, 
                                   NULL);
    if (BLT_FAILED(result)) {
        m_Client->PostMessage(new BLT_DecoderClient_DecoderEventNotificationMessage(
            BLT_DecoderServer::DecoderEvent::EVENT_TYPE_INIT_ERROR,
            result, 
            "error from BLT_Decoder_SetOutput"));
        WaitForTerminateMessage();
        return;
    }
    
    // notify the client of the initial state
    m_Client->PostMessage(
        new BLT_DecoderClient_DecoderStateNotificationMessage(STATE_STOPPED));

    // initial status
    BLT_Decoder_GetStatus(m_Decoder, &m_DecoderStatus);
    m_DecoderStatus.position.range = m_PositionUpdateRange;
    NotifyTimeCode();
    NotifyPosition();

    // initial volume
    float volume=0.0f;
    result = BLT_Decoder_GetVolume(m_Decoder, &volume);
    if (BLT_SUCCEEDED(result)) {
        m_Client->PostMessage(new BLT_DecoderClient_VolumeNotificationMessage(volume));
    }

    SetupIsComplete();

    // decoding loop
    do {
        do {
            result = m_MessageQueue->PumpMessage(0); // non-blocking
        } while (BLT_SUCCEEDED(result));
        
        if (result != NPT_ERROR_LIST_EMPTY) {
            break;
        }

        if (m_State == STATE_PLAYING) {
            result = BLT_Decoder_PumpPacketWithOptions(m_Decoder, BLT_DECODER_PUMP_OPTION_NON_BLOCKING);
            if (BLT_FAILED(result)) {
                if (result == BLT_ERROR_WOULD_BLOCK || result == BLT_ERROR_PORT_HAS_NO_DATA) {
                    /* not fatal, just wait and try again later */
                    ATX_LOG_FINER("pump would block, waiting a short time");
                    result = m_MessageQueue->PumpMessage(BLT_PLAYER_LOOP_WAIT_DURATION);
                } else {
                    ATX_LOG_FINE_1("stopped on %d", result);
                    if (result != BLT_ERROR_EOS) {
                        m_Client->PostMessage(new BLT_DecoderClient_DecoderEventNotificationMessage(
                            BLT_DecoderServer::DecoderEvent::EVENT_TYPE_DECODING_ERROR,
                            result, 
                            "error from BLT_Decoder_PumpPacketWithOptions"));
                    }
                    SetState(STATE_EOS);
                    result = BLT_SUCCESS;
                }
            } else {
                UpdateStatus();
            }
        } else {
            ATX_LOG_FINE("waiting for message");
            result = m_MessageQueue->PumpMessage(NPT_TIMEOUT_INFINITE);
            ATX_LOG_FINE("got message");
        }
    } while (BLT_SUCCEEDED(result) || result == NPT_ERROR_TIMEOUT);

    ATX_LOG_FINE("received Terminate Message");

    // unregister as an event listener
    BLT_Decoder_SetEventListener(m_Decoder, NULL);

    // destroy the decoder
    if (m_Decoder != NULL) {
        BLT_Decoder_Destroy(m_Decoder);
    }  
    
    // we're done
    SetState(STATE_TERMINATED);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::DecoderEvent::DecoderEvent
+---------------------------------------------------------------------*/
BLT_DecoderServer::DecoderEvent::DecoderEvent(BLT_DecoderServer::DecoderEvent::Type type, BLT_Result result_code, const char* message) :
    m_Type(type)
{
    m_Details = new BLT_DecoderServer::DecoderEvent::ErrorDetails(result_code, message);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::SendReply
+---------------------------------------------------------------------*/
BLT_Result 
BLT_DecoderServer::SendReply(BLT_DecoderServer_Message::CommandId id, 
                             BLT_Result                           result)
{
    BLT_DecoderClient_Message* reply;

    if (BLT_SUCCEEDED(result)) {
        reply = new BLT_DecoderClient_AckNotificationMessage(id);
    } else {
        reply = new BLT_DecoderClient_NackNotificationMessage(id, result);
    }

    return m_Client->PostMessage(reply);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::NotifyTimeCode
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::NotifyTimeCode()
{
    BLT_TimeCode time_code;
    BLT_Cardinal seconds = m_DecoderStatus.time_stamp.seconds;
    time_code.h = (BLT_UInt8)(seconds/(60*60));
    seconds -= time_code.h*(60*60);
    time_code.m = (BLT_UInt8)(seconds/60);
    seconds -= time_code.m*60;
    time_code.s = (BLT_UInt8)seconds;
    time_code.f = (BLT_UInt8)(m_DecoderStatus.time_stamp.nanoseconds/10000000);
    return m_Client->PostMessage(
        new BLT_DecoderClient_StreamTimeCodeNotificationMessage(time_code));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::NotifyPosition
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::NotifyPosition()
{
    return m_Client->PostMessage(
        new BLT_DecoderClient_StreamPositionNotificationMessage(
            m_DecoderStatus.position));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::UpdateStatus
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::UpdateStatus()
{
    BLT_DecoderStatus status;
    BLT_Result        result;

    // get the decoder status
    result = BLT_Decoder_GetStatus(m_Decoder, &status);
    if (BLT_FAILED(result)) return result;

    // notify if the time has changed by more than the update threshold
    // NOTE: current and previous here are measured in milliseconds
    ATX_UInt64 previous = BLT_TimeStamp_ToNanos(m_DecoderStatus.time_stamp)/1000000;
    ATX_UInt64 current  = BLT_TimeStamp_ToNanos(status.time_stamp)/1000000;
    if (m_TimeStampUpdateQuantum) {
        // make the new time stamp a multiple of the update quantum
        current /= m_TimeStampUpdateQuantum;
        current *= m_TimeStampUpdateQuantum;
    }
    if (current != previous) {
        m_DecoderStatus.time_stamp = BLT_TimeStamp_FromMillis(current);
        NotifyTimeCode();
    }

    // convert the stream position into a decoder position 
    if (m_PositionUpdateRange != 0) {
        ATX_UInt64 ratio = status.position.range/m_PositionUpdateRange;
        ATX_UInt64 offset;
        if (ratio == 0) {
            offset = 0;
        } else {
            offset = status.position.offset/ratio;
        }
        if (offset != m_DecoderStatus.position.offset) {
            m_DecoderStatus.position.offset = offset;
            NotifyPosition();
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::SetState
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::SetState(State state)
{
    // shortcut
    if (state == m_State) return BLT_SUCCESS;

    ATX_LOG_FINE_2("state change from %d to %d", m_State, state);

    m_State = state;

    // notify the client
    m_Client->PostMessage(
        new BLT_DecoderClient_DecoderStateNotificationMessage(state));

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::SetInput
+---------------------------------------------------------------------*/
BLT_Result 
BLT_DecoderServer::SetInput(BLT_CString name, BLT_CString type)
{
    return PostMessage(
        new BLT_DecoderServer_SetInputCommandMessage(name, type));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnSetInputCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnSetInputCommand(BLT_CString name, BLT_CString type)
{
    BLT_Result result;

    ATX_LOG_FINE_2("set input (%s / %s)",
                   BLT_SAFE_STRING(name), BLT_SAFE_STRING(type));
    result = BLT_Decoder_SetInput(m_Decoder, name, type);

    // update the state if we were in the STATE_EOS state
    if (m_State == STATE_EOS) SetState(STATE_STOPPED);
    
    UpdateStatus();
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_SET_INPUT, result);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::SetOutput
+---------------------------------------------------------------------*/
BLT_Result 
BLT_DecoderServer::SetOutput(BLT_CString name, BLT_CString type)
{
    return PostMessage(
        new BLT_DecoderServer_SetOutputCommandMessage(name, type));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnSetOutputCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnSetOutputCommand(BLT_CString name, BLT_CString type)
{
    BLT_Result result;

    ATX_LOG_FINE_2("set output (%s / %s",
                   BLT_SAFE_STRING(name), BLT_SAFE_STRING(type));
    result = BLT_Decoder_SetOutput(m_Decoder, name, type);
    if (BLT_SUCCEEDED(result)) {
        // notify of the new volume
        float volume=0.0f;
        result = BLT_Decoder_GetVolume(m_Decoder, &volume);
        if (BLT_SUCCEEDED(result)) {
            m_Client->PostMessage(new BLT_DecoderClient_VolumeNotificationMessage(volume));
        }
        result = BLT_SUCCESS;
    }
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_SET_OUTPUT, result);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::Play
+---------------------------------------------------------------------*/
BLT_Result 
BLT_DecoderServer::Play()
{
    return PostMessage(new BLT_DecoderServer_PlayCommandMessage);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnPlayCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnPlayCommand()
{
    ATX_LOG_FINE("enter");

    SetState(STATE_PLAYING);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_PLAY, BLT_SUCCESS);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::Stop
+---------------------------------------------------------------------*/
BLT_Result BLT_DecoderServer::Stop()
{
    return PostMessage(new BLT_DecoderServer_StopCommandMessage);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnStopCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnStopCommand()
{
    ATX_LOG_FINE("enter");
    BLT_Decoder_Stop(m_Decoder);
    SetState(STATE_STOPPED);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_STOP, BLT_SUCCESS);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::Pause
+---------------------------------------------------------------------*/
BLT_Result 
BLT_DecoderServer::Pause()
{
    return PostMessage(new BLT_DecoderServer_PauseCommandMessage);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnPauseCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnPauseCommand()
{
    ATX_LOG_FINE("enter");
    BLT_Decoder_Pause(m_Decoder);
    SetState(STATE_PAUSED);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_PAUSE, BLT_SUCCESS);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::Ping
+---------------------------------------------------------------------*/
BLT_Result 
BLT_DecoderServer::Ping(const void* cookie)
{
    return PostMessage(new BLT_DecoderServer_PingCommandMessage(cookie));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnPingCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnPingCommand(const void* cookie)
{
    ATX_LOG_FINE("enter");
    BLT_DecoderClient_Message* pong;
    pong = new BLT_DecoderClient_PongNotificationMessage(cookie);
    m_Client->PostMessage(pong);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_PING, BLT_SUCCESS);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::SeekToTime
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::SeekToTime(BLT_UInt64 time)
{
    return PostMessage(new BLT_DecoderServer_SeekToTimeCommandMessage(time));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnSeekToTimeCommand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnSeekToTimeCommand(BLT_UInt64 time)
{
    BLT_Result result;
    ATX_LOG_FINE_1("[%d]", (int)time);
    result = BLT_Decoder_SeekToTime(m_Decoder, time);
    if (BLT_SUCCEEDED(result)) {
        UpdateStatus();

        // update the state we were in the STATE_EOS state
        if (m_State == STATE_EOS) SetState(STATE_STOPPED);
    }

    SendReply(BLT_DecoderServer_Message::COMMAND_ID_SEEK_TO_TIME, result);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::SeekToPosition
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::SeekToPosition(BLT_UInt64 offset, BLT_UInt64 range)
{
    return PostMessage(
        new BLT_DecoderServer_SeekToPositionCommandMessage(offset, range));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnSeekToPositionCommnand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnSeekToPositionCommand(BLT_UInt64 offset, BLT_UInt64 range)
{
    BLT_Result result;
    ATX_LOG_FINE_2("[%d:%d]", (int)offset, (int)range);
    result = BLT_Decoder_SeekToPosition(m_Decoder, offset, range);
    if (BLT_SUCCEEDED(result)) {
        UpdateStatus();

        // update the state we were in the STATE_EOS state
        if (m_State == STATE_EOS) SetState(STATE_STOPPED);
    }

    SendReply(BLT_DecoderServer_Message::COMMAND_ID_SEEK_TO_POSITION, result);
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::RegisterModule
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::RegisterModule(BLT_Module* module)
{
    return PostMessage(
        new BLT_DecoderServer_RegisterModuleCommandMessage(module));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer::OnRegisterModuleCommnand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnRegisterModuleCommand(BLT_Module* module)
{
    BLT_Result result;
    ATX_LOG_FINE("enter");
    result = BLT_Decoder_RegisterModule(m_Decoder, module);
    ATX_RELEASE_OBJECT(module);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_REGISTER_MODULE, result);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::AddNode
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::AddNode(BLT_CString name)
{
    return PostMessage(
        new BLT_DecoderServer_AddNodeCommandMessage(name));
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::OnAddNodeCommnand
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnAddNodeCommand(BLT_CString name)
{
    BLT_Result result;
    ATX_LOG_FINE_1("node name = %s", name);
    result = BLT_Decoder_AddNodeByName(m_Decoder, NULL, name);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_ADD_NODE, result);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::SetVolume
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::SetVolume(float volume)
{
    return PostMessage(new BLT_DecoderServer_SetVolumeCommandMessage(volume));
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::OnSetVolumeCommand
+---------------------------------------------------------------------*/
void 
BLT_DecoderServer::OnSetVolumeCommand(float volume)
{
    BLT_Result result;
    ATX_LOG_FINE_1("volume=%f", volume);

    result = BLT_Decoder_SetVolume(m_Decoder, volume);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_SET_VOLUME, result);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::SetProperty
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::SetProperty(BLT_PropertyScope        scope,
                               const char*              target,
                               const char*              name,
                               const ATX_PropertyValue* value)
{
    return PostMessage(
        new BLT_DecoderServer_SetPropertyCommandMessage(scope, target, name, value));
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::OnSetPropertyCommand
+---------------------------------------------------------------------*/
void 
BLT_DecoderServer::OnSetPropertyCommand(BLT_PropertyScope        scope,
                                        const NPT_String&        /*target*/,
                                        const NPT_String&        name,
                                        const ATX_PropertyValue* value)
{
    BLT_Result result;
    ATX_LOG_FINE_1("[%s]", name.GetChars());

    ATX_Properties* properties = NULL;
    switch (scope) {
        case BLT_PROPERTY_SCOPE_CORE:
            result = BLT_Decoder_GetProperties(m_Decoder, &properties);
            break;
            
        case BLT_PROPERTY_SCOPE_STREAM:
            result = BLT_Decoder_GetStreamProperties(m_Decoder, &properties);
            break;
            
        default:
            // not handled yet
            result = BLT_ERROR_NOT_SUPPORTED;
    }
    if (ATX_SUCCEEDED(result) && properties != NULL) {
        result = ATX_Properties_SetProperty(properties, name.GetChars(), value);
    }
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_SET_PROPERTY, result);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::LoadPlugin
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::LoadPlugin(const char* name, BLT_Flags search_flags)
{
    return PostMessage(
        new BLT_DecoderServer_LoadPluginCommandMessage(name, search_flags));
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::OnLoadPluginCommand
+---------------------------------------------------------------------*/
void 
BLT_DecoderServer::OnLoadPluginCommand(const NPT_String& name, BLT_Flags search_flags)
{
    BLT_Result result = BLT_Decoder_LoadPlugin(m_Decoder, name, search_flags);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_LOAD_PLUGIN, result);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::LoadPlugins
+---------------------------------------------------------------------*/
BLT_Result
BLT_DecoderServer::LoadPlugins(const char* directory, const char* file_extension)
{
    return PostMessage(
        new BLT_DecoderServer_LoadPluginsCommandMessage(directory, file_extension));
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::OnLoadPluginsCommand
+---------------------------------------------------------------------*/
void 
BLT_DecoderServer::OnLoadPluginsCommand(const NPT_String& directory, 
                                        const NPT_String& file_extension)
{
    BLT_Result result = BLT_Decoder_LoadPlugins(m_Decoder, directory, file_extension);
    SendReply(BLT_DecoderServer_Message::COMMAND_ID_LOAD_PLUGINS, result);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer::OnEvent
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnEvent(const ATX_Object* /*source*/,
                           BLT_EventType     type,
                           const BLT_Event*  event)
{
    switch (type) {
      case BLT_EVENT_TYPE_STREAM_INFO: {
          BLT_StreamInfoEvent* e = (BLT_StreamInfoEvent*)event;
          m_Client->PostMessage(
              new BLT_DecoderClient_StreamInfoNotificationMessage(
                  e->update_mask,
                  e->info));
          break;
      }

      case BLT_EVENT_TYPE_DECODING_ERROR: {
          BLT_DecodingErrorEvent* e = (BLT_DecodingErrorEvent*)event;
          m_Client->PostMessage(
              new BLT_DecoderClient_DecoderEventNotificationMessage(
                BLT_DecoderServer::DecoderEvent::EVENT_TYPE_DECODING_ERROR,
                e->result,
                e->message));
          break;
      }

      default:
          break;
    }
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer_EventListenerWrapper_OnEvent
+---------------------------------------------------------------------*/
BLT_VOID_METHOD 
BLT_DecoderServer_EventListenerWrapper_OnEvent(
    BLT_EventListener* _self,
    ATX_Object*        source,
    BLT_EventType      type,
    const BLT_Event*   event)
{
    BLT_DecoderServer_EventListenerWrapper* self = ATX_SELF(BLT_DecoderServer_EventListenerWrapper, BLT_EventListener);
    self->outer->OnEvent(source, type, event);
}

/*----------------------------------------------------------------------
|    BLTP_DecoderServer::OnPropertyChanged
+---------------------------------------------------------------------*/
void
BLT_DecoderServer::OnPropertyChanged(BLT_PropertyScope        scope,
                                     const char*              source,
                                     const char*              name, 
                                     const ATX_PropertyValue* value)
{
    m_Client->PostMessage(new BLT_DecoderClient_PropertyNotificationMessage(scope,
                                                                            source,
                                                                            name,
                                                                            value));
}

/*----------------------------------------------------------------------
|    BLT_DecoderServer_PropertyListenerWrapper_OnPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
BLT_DecoderServer_PropertyListenerWrapper_OnPropertyChanged(
    ATX_PropertyListener*    _self,
    ATX_CString              name,
    const ATX_PropertyValue* value)    
{
    BLT_DecoderServer_PropertyListenerWrapper* self = ATX_SELF(BLT_DecoderServer_PropertyListenerWrapper, ATX_PropertyListener);
    self->outer->OnPropertyChanged(self->scope, self->source, name, value);
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PropertyValueWrapper::BLT_DecoderServer_PropertyValueWrapper
+---------------------------------------------------------------------*/
BLT_DecoderServer_PropertyValueWrapper::BLT_DecoderServer_PropertyValueWrapper(
    const ATX_PropertyValue* value)
{
    if (value == NULL) {
        m_Value = NULL;
        return;
    }
    
    m_Value = new ATX_PropertyValue();
    m_Value->type = value->type;
    switch (value->type) {
        case ATX_PROPERTY_VALUE_TYPE_BOOLEAN:
        case ATX_PROPERTY_VALUE_TYPE_FLOAT:
        case ATX_PROPERTY_VALUE_TYPE_INTEGER:
        case ATX_PROPERTY_VALUE_TYPE_POINTER:
            m_Value->data = value->data;
            break;

        case ATX_PROPERTY_VALUE_TYPE_STRING:
            if (value->data.string) {
                char* copy = new char[ATX_StringLength(value->data.string)+1];
                ATX_CopyString(copy, value->data.string);
                m_Value->data.string = copy;
            } else {
                m_Value->data.string = NULL;
            }
            break;

        case ATX_PROPERTY_VALUE_TYPE_RAW_DATA:
            if (value->data.raw_data.data &&
                value->data.raw_data.size) {
                m_Value->data.raw_data.size = value->data.raw_data.size;
                m_Value->data.raw_data.data = new unsigned char[value->data.raw_data.size];
                ATX_CopyMemory(m_Value->data.raw_data.data, value->data.raw_data.data, value->data.raw_data.size);
            } else {
                m_Value->data.raw_data.size = 0;
                m_Value->data.raw_data.data = NULL;
            }
            break;
    }
}

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PropertyValueWrapper::~BLT_DecoderServer_PropertyValueWrapper
+---------------------------------------------------------------------*/
BLT_DecoderServer_PropertyValueWrapper::~BLT_DecoderServer_PropertyValueWrapper()
{
    if (m_Value == NULL) return;
    
    switch (m_Value->type) {
        case ATX_PROPERTY_VALUE_TYPE_STRING:
            delete[] m_Value->data.string;
            break;

        case ATX_PROPERTY_VALUE_TYPE_RAW_DATA:
            delete[] static_cast<unsigned char*>(m_Value->data.raw_data.data);
            break;

        case ATX_PROPERTY_VALUE_TYPE_BOOLEAN:
        case ATX_PROPERTY_VALUE_TYPE_FLOAT:
        case ATX_PROPERTY_VALUE_TYPE_INTEGER:
        case ATX_PROPERTY_VALUE_TYPE_POINTER:
            break;
    }
}
