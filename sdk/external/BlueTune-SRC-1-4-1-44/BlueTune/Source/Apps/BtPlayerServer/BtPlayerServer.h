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
#include "Atomix.h"
#include "Neptune.h"
#include "BlueTune.h"

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
const unsigned int BT_HTTP_SERVER_DEFAULT_PORT = 8927;

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
class BtPlayerServer : public NPT_HttpRequestHandler,
                       public BLT_Player::EventListener,
                       public NPT_Runnable
{
public:
    // methods
    BtPlayerServer(const char* web_root, unsigned int port = BT_HTTP_SERVER_DEFAULT_PORT);
    virtual ~BtPlayerServer();

    // methods
    NPT_Result Loop();
     
    // NPT_HttpResponseHandler methods
    virtual NPT_Result SetupResponse(NPT_HttpRequest&              request,
                                     const NPT_HttpRequestContext& context,
                                     NPT_HttpResponse&             response);

    // BLT_DecoderClient_MessageHandler methods
    void OnDecoderStateNotification(BLT_DecoderServer::State state);
    void OnStreamPositionNotification(BLT_StreamPosition& position);
    void OnStreamTimeCodeNotification(BLT_TimeCode time_code);
    void OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info);
    void OnPropertyNotification(BLT_PropertyScope        scope,
                                const char*              source,
                                const char*              name,
                                const ATX_PropertyValue* value);
    
    // NPT_Runnable methods
    void Run();
    
    // members
    NPT_Mutex m_Lock;

private:
    // methods
    NPT_Result SendControlForm(NPT_HttpResponse& response, const char* msg);
    NPT_Result SendStatus(NPT_HttpResponse& response, NPT_UrlQuery& query);
    void       DoSeekToTimecode(const char* time);
    
    // members
    NPT_String               m_WebRoot;
    BLT_Player               m_Player;
    NPT_HttpServer*          m_HttpServer;
    BLT_StreamInfo           m_StreamInfo;
    ATX_Properties*          m_CoreProperties;
    ATX_Properties*          m_StreamProperties;
    BLT_DecoderServer::State m_DecoderState;
    BLT_TimeCode             m_DecoderTimecode;
    float                    m_DecoderPosition;
};

