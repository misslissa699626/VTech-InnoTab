/*****************************************************************
|
|   BlueTune - Async Layer
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_DECODER_CLIENT_H_
#define _BLT_DECODER_CLIENT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "Atomix.h"
#include "BltDecoder.h"
#include "BltTime.h"
#include "BltDecoderServer.h"

/*----------------------------------------------------------------------
|   BLT_DecoderClient_MessageHandler
+---------------------------------------------------------------------*/
class BLT_DecoderClient_MessageHandler
{
public:
    // methods
    virtual ~BLT_DecoderClient_MessageHandler() {}

    virtual void OnAckNotification(BLT_DecoderServer_Message::CommandId /*id*/) {}
    virtual void OnNackNotification(BLT_DecoderServer_Message::CommandId /*id*/,
                                    BLT_Result                     /*result*/) {}
    virtual void OnPongNotification(const void* /*cookie*/) {}
    virtual void OnDecoderStateNotification(BLT_DecoderServer::State /*state*/) {}
    virtual void OnDecoderEventNotification(BLT_DecoderServer::DecoderEvent& /*event*/) {}
    virtual void OnVolumeNotification(float /*volume*/) {}
    virtual void OnStreamTimeCodeNotification(BLT_TimeCode /*timecode*/) {}
    virtual void OnStreamPositionNotification(BLT_StreamPosition& /*position*/) {}
    virtual void OnStreamInfoNotification(BLT_Mask        /*update_mask*/, 
                                          BLT_StreamInfo& /*info*/) {}
    virtual void OnPropertyNotification(BLT_PropertyScope        /* scope    */,
                                        const char*              /* source   */,
                                        const char*              /* name     */,
                                        const ATX_PropertyValue* /* value */) {}
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_Message
+---------------------------------------------------------------------*/
class BLT_DecoderClient_Message : public NPT_Message
{
public:
    // functions
    static NPT_Message::Type MessageType;
    NPT_Message::Type GetType() {
        return MessageType;
    }

    // methods
    virtual NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) = 0;
    virtual NPT_Result Dispatch(NPT_MessageHandler* handler);
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_AckNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_AckNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_AckNotificationMessage(
        BLT_DecoderServer_Message::CommandId id) :
        m_Id(id) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnAckNotification(m_Id);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_DecoderServer_Message::CommandId m_Id;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_NackNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_NackNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_NackNotificationMessage(
        BLT_DecoderServer_Message::CommandId id, BLT_Result result) :
        m_Id(id), m_Result(result) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnNackNotification(m_Id, m_Result);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_DecoderServer_Message::CommandId m_Id;
    BLT_Result                           m_Result;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_PongNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_PongNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_PongNotificationMessage(const void* cookie) :
        m_Cookie(cookie) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnPongNotification(m_Cookie);
        return NPT_SUCCESS;
    }

private:
    // members
    const void* m_Cookie;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_DecoderStateNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_DecoderStateNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_DecoderStateNotificationMessage(
        BLT_DecoderServer::State state) :
        m_State(state) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnDecoderStateNotification(m_State);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_DecoderServer::State m_State;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_DecoderEventNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_DecoderEventNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // constructor for decoder events of type
    // DECODER_EVENT_TYPE_INIT_ERROR
    // DECODER_EVENT_TYPE_DECODING_ERROR
    BLT_DecoderClient_DecoderEventNotificationMessage(BLT_DecoderServer::DecoderEvent::Type type,
                                                      BLT_Result                            result_code,
                                                      const char*                           message):
        m_Event(type, result_code, message) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnDecoderEventNotification(m_Event);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_DecoderServer::DecoderEvent m_Event;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_VolumeNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_VolumeNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_VolumeNotificationMessage(float volume) : m_Volume(volume) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnVolumeNotification(m_Volume);
        return NPT_SUCCESS;
    }

private:
    // members
    float m_Volume;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_StreamTimeCodeNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_StreamTimeCodeNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_StreamTimeCodeNotificationMessage(BLT_TimeCode time):
        m_TimeCode(time) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnStreamTimeCodeNotification(m_TimeCode);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_TimeCode m_TimeCode;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_StreamPositionNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_StreamPositionNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_StreamPositionNotificationMessage(
        BLT_StreamPosition& position): m_Position(position) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnStreamPositionNotification(m_Position);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_StreamPosition m_Position;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_StreamInfoNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_StreamInfoNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_StreamInfoNotificationMessage(BLT_Mask        update_mask,
                                                    BLT_StreamInfo& info):
        m_UpdateMask(update_mask), m_StreamInfo(info) {
        if ((update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) && info.data_type) {
            m_DataType = info.data_type;
        }
        m_StreamInfo.data_type = m_DataType.GetChars();
    }
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnStreamInfoNotification(m_UpdateMask, m_StreamInfo);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_Mask       m_UpdateMask;
    BLT_StreamInfo m_StreamInfo;
    NPT_String     m_DataType;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient_PropertyNotificationMessage
+---------------------------------------------------------------------*/
class BLT_DecoderClient_PropertyNotificationMessage :
    public BLT_DecoderClient_Message
{
public:
    // methods
    BLT_DecoderClient_PropertyNotificationMessage(BLT_PropertyScope        scope,
                                                  const char*              source,
                                                  const char*              name,
                                                  const ATX_PropertyValue* value) :
      m_Scope(scope),
      m_Source(source),
      m_Name(name),
      m_PropertyValueWarpper(value) {}
    NPT_Result Deliver(BLT_DecoderClient_MessageHandler* handler) {
        handler->OnPropertyNotification(m_Scope,
                                        m_Source,
                                        m_Name,
                                        m_PropertyValueWarpper.m_Value);
        return NPT_SUCCESS;
    }

private:
    // members
    BLT_PropertyScope                      m_Scope;
    NPT_String                             m_Source;
    NPT_String                             m_Name;
    BLT_DecoderServer_PropertyValueWrapper m_PropertyValueWarpper;
};

/*----------------------------------------------------------------------
|   BLT_DecoderClient
+---------------------------------------------------------------------*/
class BLT_DecoderClient : public NPT_MessageReceiver,
                          public NPT_MessageHandler,
                          public BLT_DecoderClient_MessageHandler
{
public:
    // methods
             BLT_DecoderClient(NPT_MessageQueue*   queue   = NULL,
                               NPT_MessageHandler* handler = NULL);
    virtual ~BLT_DecoderClient();

protected:
    // members
    NPT_MessageQueue* m_MessageQueue;
    bool              m_MessageQueueIsLocal;
};

#endif /* _BLT_DECODER_CLIENT_H_ */
