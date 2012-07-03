/*****************************************************************
|
|   BlueTune - Async Layer
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_DECODER_SERVER_H_
#define _BLT_DECODER_SERVER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "BltDecoder.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const BLT_Size BLT_DECODER_SERVER_DEFAULT_POSITION_UPDATE_RANGE = 400;

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PropertyValueWrapper
+---------------------------------------------------------------------*/
class BLT_DecoderServer_PropertyValueWrapper {
public:
    BLT_DecoderServer_PropertyValueWrapper(const ATX_PropertyValue* value);
   ~BLT_DecoderServer_PropertyValueWrapper();
   
   ATX_PropertyValue* m_Value;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_MessageHandler
+---------------------------------------------------------------------*/
class BLT_DecoderServer_MessageHandler
{
public:
    // methods
    virtual ~BLT_DecoderServer_MessageHandler() {}

    virtual void OnSetInputCommand(BLT_CString name, BLT_CString type) = 0;
    virtual void OnSetOutputCommand(BLT_CString name, BLT_CString type) = 0;
    virtual void OnSetVolumeCommand(float volume) = 0;
    virtual void OnPlayCommand() = 0;
    virtual void OnStopCommand() = 0;
    virtual void OnPauseCommand() = 0;
    virtual void OnPingCommand(const void* cookie) = 0;
    virtual void OnSeekToTimeCommand(BLT_UInt64 time) = 0;
    virtual void OnSeekToPositionCommand(BLT_UInt64 offset, BLT_UInt64 range) = 0;
    virtual void OnRegisterModuleCommand(BLT_Module* module) = 0;
    virtual void OnAddNodeCommand(BLT_CString name) = 0;
    virtual void OnSetPropertyCommand(BLT_PropertyScope        scope,
                                      const NPT_String&        target,
                                      const NPT_String&        name,
                                      const ATX_PropertyValue* value) = 0;
    virtual void OnLoadPluginCommand(const NPT_String& name, BLT_Flags search_flags) = 0;
    virtual void OnLoadPluginsCommand(const NPT_String& directory, const NPT_String& file_extension) = 0;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_Message
+---------------------------------------------------------------------*/
class BLT_DecoderServer_Message : public NPT_Message
{
public:
    // types
    typedef enum {
        COMMAND_ID_SET_INPUT,
        COMMAND_ID_SET_OUTPUT,
        COMMAND_ID_SET_VOLUME,
        COMMAND_ID_PLAY,
        COMMAND_ID_STOP,
        COMMAND_ID_PAUSE,
        COMMAND_ID_PING,
        COMMAND_ID_SEEK_TO_TIME,
        COMMAND_ID_SEEK_TO_POSITION,
        COMMAND_ID_REGISTER_MODULE,
        COMMAND_ID_ADD_NODE,
        COMMAND_ID_SET_PROPERTY,
        COMMAND_ID_LOAD_PLUGIN,
        COMMAND_ID_LOAD_PLUGINS
    } CommandId;

    // functions
    static NPT_Message::Type MessageType;
    NPT_Message::Type GetType() {
        return MessageType;
    }

    // methods
    BLT_DecoderServer_Message(CommandId id) : m_Id(id) {}
    virtual NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) = 0;
    virtual NPT_Result Dispatch(NPT_MessageHandler* handler);
    
 private:
    // members
    CommandId m_Id;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_SetInputCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_SetInputCommandMessage :
    public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_SetInputCommandMessage(BLT_CString name, 
                                             BLT_CString type) :
        BLT_DecoderServer_Message(COMMAND_ID_SET_INPUT),
        m_Name(BLT_SAFE_STRING(name)), m_Type(BLT_SAFE_STRING(type)) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnSetInputCommand(m_Name.GetChars(), m_Type.GetChars());
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_StringObject m_Name;
    BLT_StringObject m_Type;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_SetOutputCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_SetOutputCommandMessage :
    public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_SetOutputCommandMessage(BLT_CString name, 
                                              BLT_CString type) :
        BLT_DecoderServer_Message(COMMAND_ID_SET_OUTPUT),
        m_Name(BLT_SAFE_STRING(name)), m_Type(BLT_SAFE_STRING(type)) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnSetOutputCommand(m_Name.GetChars(), m_Type.GetChars());
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_StringObject m_Name;
    BLT_StringObject m_Type;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PlayCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_PlayCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_PlayCommandMessage() : 
        BLT_DecoderServer_Message(COMMAND_ID_PLAY) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnPlayCommand();
        return NPT_SUCCESS;
    }
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_StopCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_StopCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_StopCommandMessage() : 
        BLT_DecoderServer_Message(COMMAND_ID_STOP) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnStopCommand();
        return NPT_SUCCESS;
    }
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PauseCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_PauseCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_PauseCommandMessage() : 
        BLT_DecoderServer_Message(COMMAND_ID_PAUSE) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnPauseCommand();
        return NPT_SUCCESS;
    }
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PingCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_PingCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_PingCommandMessage(const void* cookie) : 
        BLT_DecoderServer_Message(COMMAND_ID_PING), m_Cookie(cookie) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnPingCommand(m_Cookie);
        return NPT_SUCCESS;
    }

private:
    // members
    const void* m_Cookie;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_SeekToTimeCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_SeekToTimeCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_SeekToTimeCommandMessage(BLT_UInt64 time) :
        BLT_DecoderServer_Message(COMMAND_ID_SEEK_TO_TIME),
        m_Time(time) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnSeekToTimeCommand(m_Time);
        return NPT_SUCCESS;
    }

 private:
    // members
    BLT_UInt64 m_Time;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_SeekToPositionCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_SeekToPositionCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_SeekToPositionCommandMessage(BLT_UInt64 offset,
                                                   BLT_UInt64 range) :
        BLT_DecoderServer_Message(COMMAND_ID_SEEK_TO_POSITION),
        m_Offset(offset), m_Range(range) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnSeekToPositionCommand(m_Offset, m_Range);
        return NPT_SUCCESS;
    }

 private:
    // members
    BLT_UInt64 m_Offset;
    BLT_UInt64 m_Range;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_RegisterModuleCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_RegisterModuleCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_RegisterModuleCommandMessage(BLT_Module* module) :
        BLT_DecoderServer_Message(COMMAND_ID_REGISTER_MODULE),
        m_Module(module) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnRegisterModuleCommand(m_Module);
        return NPT_SUCCESS;
    }

 private:
    // members
    BLT_Module* m_Module;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_AddNodeCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_AddNodeCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_AddNodeCommandMessage(BLT_CString name) :
        BLT_DecoderServer_Message(COMMAND_ID_ADD_NODE),
        m_NodeName(name) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnAddNodeCommand(m_NodeName);
        return NPT_SUCCESS;
    }

 private:
    // members
    BLT_StringObject m_NodeName;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_SetVolumeCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_SetVolumeCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_SetVolumeCommandMessage(float volume) :
      BLT_DecoderServer_Message(COMMAND_ID_SET_VOLUME),
      m_Volume(volume) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnSetVolumeCommand(m_Volume);
        return NPT_SUCCESS;
    }

 private:
    // members
    float m_Volume;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_SetPropertyCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_SetPropertyCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_SetPropertyCommandMessage(BLT_PropertyScope        scope,
                                                const char*              target,
                                                const char*              name,
                                                const ATX_PropertyValue* value) :
      BLT_DecoderServer_Message(COMMAND_ID_SET_PROPERTY),
      m_Scope(scope),
      m_Target(target),
      m_Name(name),
      m_PropertyValueWrapper(value) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnSetPropertyCommand(m_Scope, m_Target, m_Name, m_PropertyValueWrapper.m_Value);
        return NPT_SUCCESS;
    }

 private:
    // members
    BLT_PropertyScope                      m_Scope;
    NPT_String                             m_Target;
    NPT_String                             m_Name;
    BLT_DecoderServer_PropertyValueWrapper m_PropertyValueWrapper;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_LoadPluginCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_LoadPluginCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_LoadPluginCommandMessage(const char* name, BLT_Flags search_flags) :
      BLT_DecoderServer_Message(COMMAND_ID_LOAD_PLUGIN),
      m_Name(name),
      m_SearchFlags(search_flags) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnLoadPluginCommand(m_Name, m_SearchFlags);
        return NPT_SUCCESS;
    }

 private:
    // members
    NPT_String m_Name;
    NPT_Flags  m_SearchFlags;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_LoadPluginsCommandMessage
+---------------------------------------------------------------------*/
class BLT_DecoderServer_LoadPluginsCommandMessage : public BLT_DecoderServer_Message
{
public:
    // methods
    BLT_DecoderServer_LoadPluginsCommandMessage(const char* directory, const char* file_extension) :
      BLT_DecoderServer_Message(COMMAND_ID_LOAD_PLUGINS),
      m_Directory(directory),
      m_FileExtension(file_extension) {}
    NPT_Result Deliver(BLT_DecoderServer_MessageHandler* handler) {
        handler->OnLoadPluginsCommand(m_Directory, m_FileExtension);
        return NPT_SUCCESS;
    }

 private:
    // members
    NPT_String m_Directory;
    NPT_String m_FileExtension;
};

/*----------------------------------------------------------------------
|   BLT_DecoderServer_EventListenerWrapper
+---------------------------------------------------------------------*/
class BLT_DecoderServer;
typedef struct {
    // ATX-style interfaces
    ATX_IMPLEMENTS(BLT_EventListener);

    // back pointer
    BLT_DecoderServer* outer;
} BLT_DecoderServer_EventListenerWrapper;

/*----------------------------------------------------------------------
|   BLT_DecoderServer_PropertyListenerWrapper
+---------------------------------------------------------------------*/
typedef struct {
    // ATX-style interfaces
    ATX_IMPLEMENTS(ATX_PropertyListener);

    // back pointer and scoping
    BLT_PropertyScope  scope;
    const char*        source;
    BLT_DecoderServer* outer;
} BLT_DecoderServer_PropertyListenerWrapper;

/*----------------------------------------------------------------------
|   BLT_DecoderServer
+---------------------------------------------------------------------*/
class BLT_DecoderServer : public NPT_Thread,
                          public NPT_MessageReceiver,
                          public NPT_MessageHandler,
                          public BLT_DecoderServer_MessageHandler
{
 public:
    // types
    typedef enum {
        STATE_STOPPED,
        STATE_PLAYING,
        STATE_PAUSED,
        STATE_EOS,
        STATE_TERMINATED
    } State;
        
    struct DecoderEvent {
        // types
        typedef enum {
            EVENT_TYPE_INIT_ERROR,    // details point to an ErrorDetails
            EVENT_TYPE_DECODING_ERROR // details point to an ErrorDetails
        } Type;

        struct Details {
            virtual ~Details() {}
        };
        
        struct ErrorDetails : public Details {
            ErrorDetails(BLT_Result result_code, const char* message) :
                m_ResultCode(result_code), m_Message(message) {}
            BLT_Result m_ResultCode;
            NPT_String m_Message;
        };
        
        // constructor for events of types:
        // EVENT_TYPE_INIT_ERROR
        // DECODER_EVENT_TYPE_DECODING_ERROR
        DecoderEvent(Type type, BLT_Result result_code, const char* message);
    
        // destructor
        ~DecoderEvent() { delete m_Details; }
        
        // public members
        Type     m_Type;
        Details* m_Details;
    };
    
    // methods
    BLT_DecoderServer(NPT_MessageReceiver* client);
    virtual ~BLT_DecoderServer();
    virtual BLT_Result SetInput(BLT_CString name, BLT_CString type = NULL);
    virtual BLT_Result SetOutput(BLT_CString name, BLT_CString type = NULL);
    virtual BLT_Result SetVolume(float volume);
    virtual BLT_Result Play();
    virtual BLT_Result Stop();
    virtual BLT_Result Pause();
    virtual BLT_Result Ping(const void* cookie);
    virtual BLT_Result SeekToTime(BLT_UInt64 time);
    virtual BLT_Result SeekToPosition(BLT_UInt64 offset, BLT_UInt64 range);
    virtual BLT_Result RegisterModule(BLT_Module* module);
    virtual BLT_Result AddNode(BLT_CString name);
    virtual BLT_Result SetProperty(BLT_PropertyScope        scope,
                                   const char*              target,
                                   const char*              name,
                                   const ATX_PropertyValue* value);
    virtual BLT_Result LoadPlugin(const char* name, BLT_Flags search_flags);
    virtual BLT_Result LoadPlugins(const char* directory, const char* file_extension);
    virtual void       WaitForTerminateMessage();
    
    // NPT_Runnable methods
    void Run();

    // BLT_DecoderServer_MessageHandler methods
    virtual void OnSetInputCommand(BLT_CString name, BLT_CString type);
    virtual void OnSetOutputCommand(BLT_CString name, BLT_CString type);
    virtual void OnSetVolumeCommand(float volume);
    virtual void OnPlayCommand();
    virtual void OnStopCommand();
    virtual void OnPauseCommand();
    virtual void OnPingCommand(const void* cookie);
    virtual void OnSeekToTimeCommand(BLT_UInt64 time);
    virtual void OnSeekToPositionCommand(BLT_UInt64 offset, BLT_UInt64 range);
    virtual void OnRegisterModuleCommand(BLT_Module* module);
    virtual void OnAddNodeCommand(BLT_CString name);
    virtual void OnSetPropertyCommand(BLT_PropertyScope        scope,
                                      const NPT_String&        target,
                                      const NPT_String&        name,
                                      const ATX_PropertyValue* value);
    virtual void OnLoadPluginCommand(const NPT_String& name, BLT_Flags search_flags);
    virtual void OnLoadPluginsCommand(const NPT_String& directory, 
                                      const NPT_String& file_extension);

    // BLT_EventListener methods
    virtual void OnEvent(const ATX_Object* source,
                         BLT_EventType     type,
                         const BLT_Event*  event);

    // ATX_PropertyListener methods
    virtual void OnPropertyChanged(BLT_PropertyScope        scope,
                                   const char*              source,
                                   const char*              name,
                                   const ATX_PropertyValue* value);

 protected:
    // methods
    BLT_Result SendReply(BLT_DecoderServer_Message::CommandId id, 
                         BLT_Result                           result);
    BLT_Result SetState(State state);
    BLT_Result UpdateStatus();
    BLT_Result NotifyPosition();
    BLT_Result NotifyTimeCode();
    virtual void SetupIsComplete() {} // called when Run method of thread has finished setup but not yet entered loop

    // members
    BLT_Decoder*         m_Decoder;
    NPT_MessageReceiver* m_Client;
    NPT_MessageQueue*    m_MessageQueue;
    BLT_Cardinal         m_TimeStampUpdateQuantum;
    BLT_Size             m_PositionUpdateRange;
    BLT_DecoderStatus    m_DecoderStatus;
    State                m_State;

    BLT_DecoderServer_EventListenerWrapper    m_EventListener;
    BLT_DecoderServer_PropertyListenerWrapper m_CorePropertyListener;
    BLT_DecoderServer_PropertyListenerWrapper m_StreamPropertyListener;
};

#endif /* _BLT_DECODER_SERVER_H_ */
