/*****************************************************************
|
|   BlueTune - Decoder Client
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltCore.h"
#include "BltStreamPriv.h"
#include "BltMediaNode.h"
#include "BltRegistryPriv.h"
#include "BltMediaPacketPriv.h"
#include "BltDecoderClient.h"

/*----------------------------------------------------------------------
|   BLT_DecoderClient_Message::MessageType
+---------------------------------------------------------------------*/
NPT_Message::Type 
BLT_DecoderClient_Message::MessageType = "bluetune.messages.decoder.client";

/*----------------------------------------------------------------------
|   BLT_DecoderClient_Message::Dispatch
+---------------------------------------------------------------------*/
NPT_Result
BLT_DecoderClient_Message::Dispatch(NPT_MessageHandler* handler)
{
    BLT_DecoderClient_MessageHandler* specific =
        NPT_DYNAMIC_CAST(BLT_DecoderClient_MessageHandler, handler);
    if (specific) {
        return Deliver(specific);
    } else {
        return DefaultDeliver(handler);
    }
}

/*----------------------------------------------------------------------
|    BLT_DecoderClient::BLT_DecoderClient
+---------------------------------------------------------------------*/
BLT_DecoderClient::BLT_DecoderClient(NPT_MessageQueue*   queue,
                                     NPT_MessageHandler* handler) :
    m_MessageQueue(queue)
{
    // if no queue was specified, create one
    if (m_MessageQueue == NULL) {
        m_MessageQueue = new NPT_SimpleMessageQueue();
        m_MessageQueueIsLocal = BLT_TRUE;
    } else {
        m_MessageQueueIsLocal = BLT_FALSE;
    }

    // attach to the message queue
    SetQueue(m_MessageQueue);
    if (handler != NULL) {
        SetHandler(handler);
    } else {
        SetHandler(this);
    }
}

/*----------------------------------------------------------------------
|    BLT_DecoderClient::~BLT_DecoderClient
+---------------------------------------------------------------------*/
BLT_DecoderClient::~BLT_DecoderClient()
{
    // delete the message queue if we created it
    if (m_MessageQueueIsLocal) {
        delete m_MessageQueue;
    }
}

