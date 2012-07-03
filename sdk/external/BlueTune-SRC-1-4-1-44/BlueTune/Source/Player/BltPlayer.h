/*****************************************************************
|
|   BlueTune - Asynchronous API
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Asynchronous Player API
 */

#ifndef _BLT_PLAYER_H_
#define _BLT_PLAYER_H_

/** @defgroup BLT_Player BLT_Player Class
 * @{
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltDecoder.h"

#if defined(__cplusplus)
#include "Neptune.h"
#include "BltDecoderClient.h"
#include "BltDecoderServer.h"
#else
#include "Atomix.h"
#endif

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_TIMEOUT_INFINITE ((BLT_UInt32)(-1))

/*----------------------------------------------------------------------
|   BLT_Player
+---------------------------------------------------------------------*/
/**
 * The BLT_Player class is the asynchronous player API.
 * A BLT_Player player creates internally a thread in which a decoder 
 * (BLT_Decoder) runs in a decoding loop. The decoding loop receives
 * commands that are sent to it when calling some of the methods of this
 * class. Those commands perform basic player controls, such as the ability
 * to select the input, the output, to play, stop, pause, seek, etc...
 * 
 * An application can choose to either subclass this class and override
 * one or more of the virtual notification callbacks methods (the OnXXX
 * methods of the BLT_DecoderClient_MessageHandler interface class from which 
 * this class inherits), or create an instance of this class and pass an event 
 * listener to the SetEventListener() method to receive notification callbacks.
 */
#if defined(__cplusplus)
class BLT_Player : public BLT_DecoderClient
{
 public:
    /**
     * A class must derive from this interface class if it wants
     * to receive event notifications from the player.
     */
    typedef BLT_DecoderClient_MessageHandler EventListener;
     
    /**
     * Construct a new instance.
     * @param queue pointer to a message queue that should receive messages
     * from the decoder thread. If this parameter is NULL, a message queue
     * will be created automatically.
     * WARNING: it is important that this queue not be destroyed before this
     * player object is destroyed, because it may receive notification 
     * messages. If you need to destroy the queue in the destructor of a 
     * subclass of this class, call the Shutdown() method before destroying
     * the message queue to ensure that no more notification messages will 
     * be posted to the queue.
     */
    BLT_Player(NPT_MessageQueue* queue = NULL);

    /**
     * Destruct an instance.
     */
    virtual ~BLT_Player();

    /**
     * Set the event listener that will be called when notification
     * messages are received. By default, there is no event listener
     * when a player is created, so notification messages are just
     * ignored, unless a subclass of this class overrides the virtual
     * methods that handle notification messages (OnXXX virtual methods
     * of the BLT_DecoderClient_MessageHandler class).
     * @param listener Pointer to a listener, or NULL.
     */
    void SetEventListener(EventListener* listener);

    /**
     * Returns the event listener currently associated with this player.
     */
    EventListener* GetEventListener();
    
    /**
     * Dequeue and dispatch one message from the queue. 
     * @param blocking Indicate whether this call should block until a
     * message is available, or if it should return right away, regardless
     * of whether a message is available or not.
     */
    virtual BLT_Result PumpMessage(BLT_UInt32 timeout = BLT_TIMEOUT_INFINITE);

    /**
     * Set the input of the decoder.
     * @param name Name of the input
     * @param type Mime-type of the input, if known, or NULL
     */
    virtual BLT_Result SetInput(BLT_CString name, BLT_CString type = NULL);

    /**
     * Set the output of the decoder.
     * @param name Name of the output
     * @param type Mime-type of the output, if known.
     */
    virtual BLT_Result SetOutput(BLT_CString name, BLT_CString type = NULL);

    /**
     * Instruct the decoder to start decoding media packets and send them
     * to the output.
     */
    virtual BLT_Result Play();

    /**
     * Instruct the decoder to stop decoding and become idle.
     */
    virtual BLT_Result Stop();

    /**
     * Instruct the decoder to pause decoding and become idle.
     * The difference between pausing and stopping is that pausing can
     * be resumed without any gaps, whereas stopping may release internal
     * resources and flush decoding buffers, so a Play() after a Stop()
     * may result in an audible gap.
     */
    virtual BLT_Result Pause();

    /**
     * Instruct the decoder to seek to a specific time value.
     * @param time the time to seek to, expressed in milliseconds.
     */
    virtual BLT_Result SeekToTime(BLT_UInt64 time);

    /**
     * Instruct the decoder to seek to a specific time stamp.
     * @param h Hours
     * @param m Minutes
     * @param s Seconds
     * @param f Fractions
     */
    virtual BLT_Result SeekToTimeStamp(BLT_UInt8 h, 
                                       BLT_UInt8 m, 
                                       BLT_UInt8 s, 
                                       BLT_UInt8 f);

    /**
     * Instruct the decoder to seek to a specific position.
     * @param offset Offset between 0 and range
     * @param range Maximum value of offset. The range is an arbitrary
     * scale. For example, if offset=1 and range=2, this means that
     * the decoder should seek to exactly the middle point of the input.
     * Or if offset=25 and range=100, this means that the decoder should
     * seek to the point that is at 25/100 of the total input.
     */
    virtual BLT_Result SeekToPosition(BLT_UInt64 offset, BLT_UInt64 range);

    /**
     * Ping the decoder.
     * When the decoding thread processes the ping message, it
     * sends back a pong reply, which will invoke the OnPongNotification
     * notification method when received.
     * @param cookie Arbitrary pointer value that will be passed back
     * along with the pong notification message. This is typically used
     * by the caller to keep any state information associated with this
     * call.
     */
    virtual BLT_Result Ping(const void* cookie);

    /**
     * Register a module with the decoder.
     * This method transfers ownership of the reference held by the caller:
     * the module's reference count is not incremented, so the caller
     * should not call ATX_RELEASE() on the module object after this method
     * returns.
     * NOTE: the module's methods will be called in the context of
     * the decoding thread. The caller of this method should not
     * make any call to the module's methods after this method 
     * call returns unless the module's implementation is thread-safe.
     * @param module Pointer to a module object.
     */
    virtual BLT_Result RegisterModule(BLT_Module* module);

    /**
     * Instruct the decoder to instantiate a new node and add it to its
     * decoding stream.
     * @param name Name of the node to add.
     */
    virtual BLT_Result AddNode(BLT_CString name);

    /**
     * Set the audio volume.
     * @param volume Audio volume (between 0.0 and 1.0).
     */
    virtual BLT_Result SetVolume(float volume);

    /**
     * Set a property.
     * @param scope Scope enum specifying what type of property is being set.
     * @param target Name of the target to which this property applies. For
     * Core (BLT_PROPERTY_SCOPE_CORE) and Stream (BLT_PROPERTY_SCOPE_STREAM)
     * scopes this parameter must be NULL.
     * @param name Name of the property to set.
     * @param value Pointer to the property's value. If this parameter is
     * NULL, the property will be deleted.
     */
    virtual BLT_Result SetProperty(BLT_PropertyScope        scope,
                                   const char*              target,
                                   const char*              name,
                                   const ATX_PropertyValue* value);

    /**
     * Load and Register a plugin.
     * @param name Name of a plugin (filename of the plugin or special name)
     * @param search_flags Flags that select how the actual plugin file will
     * be located. It is an OR'ed combination or the BLT_PLUGIN_LOADER_FLAGS_XXX
     * constants defined in BltDynamicPlugins.h
     */
    BLT_Result LoadPlugin(const char* name, BLT_Flags search_flags);

    /**
     * Load and Register all plugins located in a directory.
     * @param directory Path of the directory that contains the plugins
     * @param file_extension File extension of the plugin files, or NULL to load
     * all files regardless of extension. The file extension includes the '.' 
     * characters (ex: ".plugin")
     */
    BLT_Result LoadPlugins(const char* directory, const char* file_extension);
    
    /**
     * Shutdown the player. 
     * Use this method before deleting the player if you need to ensure that
     * no more asynchronous event callbacks will be made. 
     * When this method returns, no other method can be called apart from
     * the destructor.
     */
    virtual BLT_Result Shutdown();

    /**
     * Send this player a NPT_TerminateMessage message that will cause any caller
     * waiting for a message on the incoming message queue to be
     * unblocked.
     * CAUTION: this is an advanced function. Only call this is you know 
     * exactly what you are doing (it is not often needed).
     */
    virtual BLT_Result Interrupt();

protected:
    /* BLT_DecoderClient_MessageHandler methods */
    virtual void OnAckNotification(BLT_DecoderServer_Message::CommandId id) {
        if (m_Listener) m_Listener->OnAckNotification(id);
    }
    virtual void OnNackNotification(BLT_DecoderServer_Message::CommandId id,
                                    BLT_Result                           result) {
        if (m_Listener) m_Listener->OnNackNotification(id, result);
    }
    virtual void OnPongNotification(const void* cookie) {
        if (m_Listener) m_Listener->OnPongNotification(cookie);
    }
    virtual void OnDecoderStateNotification(BLT_DecoderServer::State state) {
        if (m_Listener) m_Listener->OnDecoderStateNotification(state);
    }
    virtual void OnDecoderEventNotification(BLT_DecoderServer::DecoderEvent& event) {
        if (m_Listener) m_Listener->OnDecoderEventNotification(event);
    }
    virtual void OnStreamTimeCodeNotification(BLT_TimeCode timecode) {
        if (m_Listener) m_Listener->OnStreamTimeCodeNotification(timecode);
    }
    virtual void OnStreamPositionNotification(BLT_StreamPosition& position) {
        if (m_Listener) m_Listener->OnStreamPositionNotification(position);
    }
    virtual void OnStreamInfoNotification(BLT_Mask        update_mask, 
                                          BLT_StreamInfo& info) {
        if (m_Listener) m_Listener->OnStreamInfoNotification(update_mask, info);
    }
    virtual void OnPropertyNotification(BLT_PropertyScope        scope,
                                        const char*              source,
                                        const char*              name,
                                        const ATX_PropertyValue* value) {
        if (m_Listener) m_Listener->OnPropertyNotification(scope, source, name, value);
    }

private:
    /**
     * Instance of a decoding thread that implements the BLT_DecoderServer interface.
     * All player commands are delegated to this instance.
     */
    BLT_DecoderServer* m_Server;
    
    /**
     * Object that will handle notification callbacks, if not NULL.
     */
    BLT_DecoderClient_MessageHandler* m_Listener;
};
#endif

/*----------------------------------------------------------------------
|   BLT_Player C-style API
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#else
typedef struct BLT_Player BLT_Player;
#endif

typedef enum {
    BLT_PLAYER_EVENT_TYPE_ACK,
    BLT_PLAYER_EVENT_TYPE_NACK,
    BLT_PLAYER_EVENT_TYPE_PONG_NOTIFICATION,
    BLT_PLAYER_EVENT_TYPE_DECODER_STATE_NOTIFICATION,
    BLT_PLAYER_EVENT_TYPE_DECODING_ERROR_NOTIFICATION,
    BLT_PLAYER_EVENT_TYPE_STREAM_TIMECODE_NOTIFICATION,
    BLT_PLAYER_EVENT_TYPE_STREAM_POSITION_NOTIFICATION,
    BLT_PLAYER_EVENT_TYPE_STREAM_INFO_NOTIFICATION,
    BLT_PLAYER_EVENT_TYPE_PROPERTY_NOTIFICATION
} BLT_Player_EventType;

typedef enum {
    BLT_PLAYER_COMMAND_ID_SET_INPUT,
    BLT_PLAYER_COMMAND_ID_SET_OUTPUT,
    BLT_PLAYER_COMMAND_ID_SET_VOLUME,
    BLT_PLAYER_COMMAND_ID_PLAY,
    BLT_PLAYER_COMMAND_ID_PAUSE,
    BLT_PLAYER_COMMAND_ID_STOP,
    BLT_PLAYER_COMMAND_ID_PING,
    BLT_PLAYER_COMMAND_ID_SEEK_TO_TIME,
    BLT_PLAYER_COMMAND_ID_SEEK_TO_POSITION,
    BLT_PLAYER_COMMAND_ID_REGISTER_MODULE,
    BLT_PLAYER_COMMAND_ID_ADD_NODE,
    BLT_PLAYER_COMMAND_ID_SET_PROPERTY,
    BLT_PLAYER_COMMAND_ID_LOAD_PLUGIN,
    BLT_PLAYER_COMMAND_ID_LOAD_PLUGINS
} BLT_Player_CommandId;

typedef enum {
    BLT_PLAYER_DECODER_STATE_STOPPED,
    BLT_PLAYER_DECODER_STATE_PLAYING,
    BLT_PLAYER_DECODER_STATE_PAUSED,
    BLT_PLAYER_DECODER_STATE_EOS
} BLT_Player_DecoderState;

typedef struct {
    BLT_Result  result_code;
    const char* message;
} BLT_Player_DecodingError;

typedef struct {
    BLT_Player_EventType type;
} BLT_Player_Event;

typedef struct {
    BLT_Player_Event     base;
    BLT_Player_CommandId command;
} BLT_Player_AckEvent;

typedef struct {
    BLT_Player_Event     base;
    BLT_Player_CommandId command;
    BLT_Result          result;
} BLT_Player_NackEvent;

typedef struct {
    BLT_Player_Event base;
    const void*      cookie;
} BLT_Player_PongNotificationEvent;

typedef struct {
    BLT_Player_Event         base;
    BLT_Player_DecodingError error;
} BLT_Player_DecodingErrorNotificationEvent;

typedef struct {
    BLT_Player_Event        base;
    BLT_Player_DecoderState state;
} BLT_Player_DecoderStateNotificationEvent;

typedef struct {
    BLT_Player_Event base;
    BLT_TimeCode     timecode;
} BLT_Player_StreamTimeCodeNotificationEvent;

typedef struct {
    BLT_Player_Event   base;
    BLT_StreamPosition position;
} BLT_Player_StreamPositionNotificationEvent;

typedef struct {
    BLT_Player_Event base;
    BLT_Mask         update_mask;
    BLT_StreamInfo   info;
} BLT_Player_StreamInfoNotificationEvent;

typedef struct {
    BLT_Player_Event         base;
    BLT_PropertyScope        scope;
    const char*              source;
    const char*              name;
    const ATX_PropertyValue* value;
} BLT_Player_PropertyNotificationEvent;

typedef struct {
    void* instance;
    void  (*handler)(void* instance, const BLT_Player_Event* event);
} BLT_Player_EventListener;

BLT_Result BLT_Player_Create(BLT_Player_EventListener listener, 
                             BLT_Player**             player);
BLT_Result BLT_Player_Destroy(BLT_Player* player);
BLT_Result BLT_Player_PumpMessage(BLT_Player* player, BLT_UInt32 timeout);
BLT_Result BLT_Player_SetInput(BLT_Player* player,
                               BLT_CString name, 
                               BLT_CString mime_type);
BLT_Result BLT_Player_Play(BLT_Player* player);
BLT_Result BLT_Player_Stop(BLT_Player* player);
BLT_Result BLT_Player_Pause(BLT_Player* player);

#if defined(__cplusplus)
}

class BLT_PlayerApiMapper {
public:
    static BLT_Player_CommandId MapCommandId(BLT_DecoderServer_Message::CommandId id) {
        switch (id) {
          case BLT_DecoderServer_Message::COMMAND_ID_SET_INPUT:
            return BLT_PLAYER_COMMAND_ID_SET_INPUT;
          case BLT_DecoderServer_Message::COMMAND_ID_SET_OUTPUT:
            return BLT_PLAYER_COMMAND_ID_SET_OUTPUT;
          case BLT_DecoderServer_Message::COMMAND_ID_PLAY:
            return BLT_PLAYER_COMMAND_ID_PLAY;
          case BLT_DecoderServer_Message::COMMAND_ID_STOP:
            return BLT_PLAYER_COMMAND_ID_STOP;
          case BLT_DecoderServer_Message::COMMAND_ID_PAUSE:
            return BLT_PLAYER_COMMAND_ID_PAUSE;
          case BLT_DecoderServer_Message::COMMAND_ID_PING:
            return BLT_PLAYER_COMMAND_ID_PING;
          case BLT_DecoderServer_Message::COMMAND_ID_SEEK_TO_TIME:
            return BLT_PLAYER_COMMAND_ID_SEEK_TO_TIME;
          case BLT_DecoderServer_Message::COMMAND_ID_SEEK_TO_POSITION:
            return BLT_PLAYER_COMMAND_ID_SEEK_TO_POSITION;
          case BLT_DecoderServer_Message::COMMAND_ID_REGISTER_MODULE:
            return BLT_PLAYER_COMMAND_ID_REGISTER_MODULE;
          case BLT_DecoderServer_Message::COMMAND_ID_ADD_NODE:
            return BLT_PLAYER_COMMAND_ID_ADD_NODE;
          case BLT_DecoderServer_Message::COMMAND_ID_SET_VOLUME:
            return BLT_PLAYER_COMMAND_ID_SET_VOLUME;
          case BLT_DecoderServer_Message::COMMAND_ID_SET_PROPERTY:
            return BLT_PLAYER_COMMAND_ID_SET_PROPERTY;
          case BLT_DecoderServer_Message::COMMAND_ID_LOAD_PLUGIN:
            return BLT_PLAYER_COMMAND_ID_LOAD_PLUGIN;
          case BLT_DecoderServer_Message::COMMAND_ID_LOAD_PLUGINS:
            return BLT_PLAYER_COMMAND_ID_LOAD_PLUGINS;
        }
        return (BLT_Player_CommandId)(-1);
    }
    
    static BLT_Player_DecoderState MapDecoderState(BLT_DecoderServer::State state) {
        switch (state) {
          case BLT_DecoderServer::STATE_PLAYING: return BLT_PLAYER_DECODER_STATE_PLAYING;
          case BLT_DecoderServer::STATE_STOPPED: return BLT_PLAYER_DECODER_STATE_STOPPED;
          case BLT_DecoderServer::STATE_PAUSED:  return BLT_PLAYER_DECODER_STATE_PAUSED;
          case BLT_DecoderServer::STATE_EOS:     return BLT_PLAYER_DECODER_STATE_EOS;
          default: return BLT_PLAYER_DECODER_STATE_PLAYING;
        }
    }
};

#endif

/*----------------------------------------------------------------------
|   BLT_Player Objective C API
+---------------------------------------------------------------------*/
#if defined(__OBJC__)

#if TARGET_OS_IPHONE
#include <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif

@interface BLT_PlayerObject : NSObject
{
 @private
    BLT_Player* player;
    id          delegate;
    void*       messageQueue;
}

-(BLT_PlayerObject *) init;
-(id)                 delegate;
-(void)               setDelegate: (id) delegate;
-(BLT_Player*)        player;
-(BLT_Result)         setInput: (NSString*) name;
-(BLT_Result)         setInput: (NSString*) name withType: (NSString*) mime_type;
-(BLT_Result)         setOutput: (NSString*) name;
-(BLT_Result)         setOutput: (NSString*) name withType: (NSString*) mime_type;
-(BLT_Result)         play;
-(BLT_Result)         pause;
-(BLT_Result)         stop;
-(BLT_Result)         seekToTime: (UInt64) time;
-(BLT_Result)         seekToTimeStampHours: (UInt8) hours minutes: (UInt8)minutes seconds: (UInt8) seconds frames: (UInt8) frames;
-(BLT_Result)         seekToPosition: (UInt64) position range: (UInt64) range;
-(BLT_Result)         setVolume: (float) volume;
@end

@interface BLT_PlayerObjectDelegate : NSObject
{
}
-(void) ackWasReceived: (BLT_Player_CommandId) command_id;
-(void) nackWasReceived: (BLT_Player_CommandId) command_id result: (BLT_Result) result;
-(void) pongWasReceived: (const void*) cookie;
-(void) decoderStateDidChange: (BLT_Player_DecoderState) state;
-(void) streamTimecodeDidChange: (BLT_TimeCode) timecode;
-(void) streamPositionDidChange: (BLT_StreamPosition) position;
-(void) streamInfoDidChange: (const BLT_StreamInfo*) info updateMask: (BLT_Mask) mask;
-(void) propertyDidChange: (BLT_PropertyScope) scope source: (const char*) source name: (const char*) name value: (const ATX_PropertyValue*) value;
@end

#endif /* defined(__OBJC__) */

/** @} */

#endif /* _BLT_PLAYER_H_ */
