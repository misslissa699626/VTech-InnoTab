/*****************************************************************
|
|   BlueTune - Controller/Player
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "Atomix.h"
#include "NptUtils.h"
#include "Neptune.h"
#include "BlueTune.h"
#include "BtStreamController.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
class BtController : public BLT_Player
{
public:
    // methods
    BtController();
    virtual ~BtController();

    // BLT_DecoderClient_MessageHandler methods
    void OnAckNotification(BLT_DecoderServer_Message::CommandId id);
    void OnNackNotification(BLT_DecoderServer_Message::CommandId id,
                            BLT_Result                           result);
    void OnDecoderStateNotification(BLT_DecoderServer::State state);
    void OnDecoderEventNotification(BLT_DecoderServer::DecoderEvent& event);
    void OnStreamTimeCodeNotification(BLT_TimeCode time_code);
    void OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info);
    void OnPropertyNotification(BLT_PropertyScope        scope,
                                const char*              source,
                                const char*              name,
                                const ATX_PropertyValue* value);
    
    // command methods
    void DoSeekToTimeStamp(const char* time);

private:
    // members
    BtStreamController* m_ConsoleController;
};

/*----------------------------------------------------------------------
|    BtController::BtController
+---------------------------------------------------------------------*/
BtController::BtController()
{
    // create the command stream
    NPT_File standard_in(NPT_FILE_STANDARD_INPUT);
    NPT_Result result = standard_in.Open(NPT_FILE_OPEN_MODE_READ |
                                         NPT_FILE_OPEN_MODE_UNBUFFERED);
    if (NPT_FAILED(result)) return;

    // get the command stream
    NPT_InputStreamReference input_stream;
    standard_in.GetInputStream(input_stream);

    // create the controller
    m_ConsoleController = new BtStreamController(input_stream, *this);
    m_ConsoleController->Start();
}

/*----------------------------------------------------------------------
|    BtController::~BtController
+---------------------------------------------------------------------*/
BtController::~BtController()
{
    delete m_ConsoleController;
}

/*----------------------------------------------------------------------
|    BtController::OnAckNotification
+---------------------------------------------------------------------*/
void
BtController::OnAckNotification(BLT_DecoderServer_Message::CommandId id)
{
    ATX_Debug("BLT_Player::OnAckNotification (id=%d)\n", id);
}

/*----------------------------------------------------------------------
|    BtController::OnAckNotification
+---------------------------------------------------------------------*/
void
BtController::OnNackNotification(BLT_DecoderServer_Message::CommandId id,
                             BLT_Result                           result)
{
    ATX_Debug("BLT_Player::OnNackNotification (id=%d, result=%d:%s)\n", 
              id, result, BLT_ResultText(result));    
}

/*----------------------------------------------------------------------
|    BtController::OnDecoderStateNotification
+---------------------------------------------------------------------*/
void
BtController::OnDecoderStateNotification(BLT_DecoderServer::State state)
{
    ATX_ConsoleOutput("INFO: State = ");

    switch (state) {
      case BLT_DecoderServer::STATE_STOPPED:
        ATX_ConsoleOutput("[STOPPED]\n");
        break;

      case BLT_DecoderServer::STATE_PLAYING:
        ATX_ConsoleOutput("[PLAYING]\n");
        break;

      case BLT_DecoderServer::STATE_PAUSED:
        ATX_ConsoleOutput("[PAUSED]\n");
        break;

      case BLT_DecoderServer::STATE_EOS:
        ATX_ConsoleOutput("[END OF STREAM]\n");
        break;

      default:
        ATX_ConsoleOutput("[UNKNOWN]\n");
        break;
    }
}

/*----------------------------------------------------------------------
|    BtController::OnDecoderEventNotification
+---------------------------------------------------------------------*/
void 
BtController::OnDecoderEventNotification(BLT_DecoderServer::DecoderEvent& event)
{
    ATX_ConsoleOutput("INFO: Decoder Event - ");
    switch (event.m_Type) {
        case BLT_DecoderServer::DecoderEvent::EVENT_TYPE_DECODING_ERROR: {
            BLT_DecoderServer::DecoderEvent::ErrorDetails* details = 
                static_cast<BLT_DecoderServer::DecoderEvent::ErrorDetails*>(event.m_Details);
            ATX_ConsoleOutputF("[DECODING ERROR] result=%d (%s) message=%s\n",
                               details->m_ResultCode,
                               BLT_ResultText(details->m_ResultCode),
                               details->m_Message.GetChars());
            break;
        }
        
        default:
            ATX_ConsoleOutputF("[UNKNOWN] type=%d\n", (int)event.m_Type);
            break;
    }
}

/*----------------------------------------------------------------------
|    BtController::OnStreamTimeCodeNotification
+---------------------------------------------------------------------*/
void 
BtController::OnStreamTimeCodeNotification(BLT_TimeCode time_code)
{
    char time[32];
    ATX_FormatStringN(time, 32,
                      "%02d:%02d:%02d",
                      time_code.h,
                      time_code.m,
                      time_code.s);
    ATX_ConsoleOutputF("INFO: Time Code = %s\n", time);
}

/*----------------------------------------------------------------------
|    BtController::OnStreamInfoNotification
+---------------------------------------------------------------------*/
void 
BtController::OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info)
{       
    if (update_mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
        ATX_ConsoleOutputF("INFO: Nominal Bitrate = %ld\n", info.nominal_bitrate);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
        ATX_ConsoleOutputF("INFO: Average Bitrate = %ld\n", info.average_bitrate);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
        ATX_ConsoleOutputF("INFO: Instant Bitrate = %ld\n", info.instant_bitrate);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
        ATX_ConsoleOutputF("INFO: Sample Rate = %ld\n", info.sample_rate);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
        ATX_ConsoleOutputF("INFO: Channels = %d\n", info.channel_count);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_SIZE) {
        ATX_ConsoleOutputF("INFO: Stream Size = %" ATX_INT64_PRINTF_FORMAT "d\n", info.size);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_DURATION) {
        ATX_ConsoleOutputF("INFO: Stream Duration = %" ATX_INT64_PRINTF_FORMAT "d\n", info.duration);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
        ATX_ConsoleOutputF("INFO: Data Type = %s\n", info.data_type?info.data_type:"(null)");
    }
}

/*----------------------------------------------------------------------
|    BtController::OnPropertyNotification
+---------------------------------------------------------------------*/
void 
BtController::OnPropertyNotification(BLT_PropertyScope        scope,
                                     const char*              /* source */,
                                     const char*              name,
                                     const ATX_PropertyValue* value)
{
    const char* scope_name;
    switch (scope) {
        case BLT_PROPERTY_SCOPE_CORE: scope_name = "core"; break;
        case BLT_PROPERTY_SCOPE_STREAM: scope_name = "stream"; break;
        default: scope_name = "unknown";
    }
    
    // when the name is NULL or empty, it means that all the properties in that 
    // scope fo that source have been deleted 
    if (name == NULL || name[0] == '\0') {
        ATX_ConsoleOutputF("INFO: All properties in '%s' scope deleted\n", scope_name);
        return;
    }
    
    ATX_ConsoleOutputF("INFO: Property %s (%s) ", name, scope_name);
    if (value == NULL) {
        ATX_ConsoleOutputF("deleted]\n");
        return;
    }

    switch (value->type) {
        case ATX_PROPERTY_VALUE_TYPE_INTEGER:
            ATX_ConsoleOutputF("= [I] %d\n", value->data.integer);
            break;
            
        case ATX_PROPERTY_VALUE_TYPE_FLOAT:
            ATX_ConsoleOutputF("= [F] %f\n", value->data.fp);
            break;
            
        case ATX_PROPERTY_VALUE_TYPE_STRING:
            ATX_ConsoleOutputF("= [S] %s\n", value->data.string);
            break;
            
        case ATX_PROPERTY_VALUE_TYPE_BOOLEAN:
            ATX_ConsoleOutputF("= [B] %s\n", value->data.boolean == ATX_TRUE?"true":"false");
            break;
            
        case ATX_PROPERTY_VALUE_TYPE_RAW_DATA:
            ATX_ConsoleOutputF("= [R] %d bytes of data\n", value->data.raw_data.size);
            break;

        case ATX_PROPERTY_VALUE_TYPE_POINTER:
            ATX_ConsoleOutputF("= [P] pointer at %x\n", value->data.pointer);
            break;
    }
}

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    BLT_Result result;
    
    // create the controller
    BtController* controller = new BtController();

    // pump notification messages
    do {
        result =  controller->PumpMessage();
    } while (result == BLT_SUCCESS);
    ATX_ConsoleOutputF("INFO: message loop ended with status %d (%s)\n", result, BLT_ResultText(result));

    // delete the controller
    delete controller;

    return 0;
}
