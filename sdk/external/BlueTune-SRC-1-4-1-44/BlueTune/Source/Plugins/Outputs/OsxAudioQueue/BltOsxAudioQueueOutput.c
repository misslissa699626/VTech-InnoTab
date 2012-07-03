/*****************************************************************
|
|   OSX Audio Queue Output Module
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <AvailabilityMacros.h>
#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5) || (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_2_0)
#include <AudioToolbox/AudioQueue.h>
#include <AudioToolbox/AudioFormat.h>
#endif
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltOsxAudioQueueOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"
#include "BltVolumeControl.h"

#if (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5) || (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_2_0)

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.osx.audio-queue")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT             8
#define BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFERS_TOTAL_DURATION   1000 /* milliseconds */
#define BLT_OSX_AUDIO_QUEUE_OUTPUT_PACKET_DESCRIPTION_COUNT 512
#define BLT_OSX_AUDIO_QUEUE_OUTPUT_DEFAULT_BUFFER_SIZE      32768
#define BLT_OSX_AUDIO_QUEUE_OUTPUT_MAX_WAIT                 3 /* seconds */

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_MediaType               base;
    AudioStreamBasicDescription asbd;
    unsigned int                magic_cookie_size;
    unsigned char               magic_cookie[1];
    // followed by zero or more magic_cookie bytes
} AsbdMediaType;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
    
    /* members */
    BLT_MediaTypeId asbd_media_type_id;
} OsxAudioQueueOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_VolumeControl);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    pthread_mutex_t              lock;
    AudioQueueRef                audio_queue;
    BLT_Boolean                  audio_queue_started;
    BLT_Boolean                  audio_queue_paused;
    pthread_cond_t               audio_queue_stopped_cond;
    BLT_Boolean                  waiting_for_stop;
    AudioStreamBasicDescription  audio_format;
    Float32                      volume;
    AudioQueueBufferRef          buffers[BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT];
    BLT_Ordinal                  buffer_index;
    pthread_cond_t               buffer_released_cond;
    AudioStreamPacketDescription packet_descriptions[BLT_OSX_AUDIO_QUEUE_OUTPUT_PACKET_DESCRIPTION_COUNT];
    BLT_Ordinal                  packet_count;
    BLT_Cardinal                 packet_count_max;
    struct {
        BLT_PcmMediaType pcm;
        AsbdMediaType    asbd;
    }                            expected_media_types;
} OsxAudioQueueOutput;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutput, BLT_VolumeControl)
ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(OsxAudioQueueOutput, BLT_PacketConsumer)

BLT_METHOD OsxAudioQueueOutput_Resume(BLT_MediaNode* self);
BLT_METHOD OsxAudioQueueOutput_Stop(BLT_MediaNode* self);
BLT_METHOD OsxAudioQueueOutput_Drain(BLT_OutputNode* self);

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_BufferCallback
+---------------------------------------------------------------------*/
static void
OsxAudioQueueOutput_BufferCallback(void*               _self, 
                                   AudioQueueRef       queue, 
                                   AudioQueueBufferRef buffer)
{
    OsxAudioQueueOutput* self = (OsxAudioQueueOutput*)_self;
    BLT_COMPILER_UNUSED(queue);
    
    /* mark the buffer as free */
    pthread_mutex_lock(&self->lock);
    buffer->mUserData = NULL;
    buffer->mAudioDataByteSize = 0;
    pthread_cond_signal(&self->buffer_released_cond);
    pthread_mutex_unlock(&self->lock);
    
    ATX_LOG_FINER_1("callback for buffer %p", buffer);
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_PropertyCallback
+---------------------------------------------------------------------*/
static void 
OsxAudioQueueOutput_PropertyCallback(void*                _self, 
                                     AudioQueueRef        queue, 
                                     AudioQueuePropertyID property_id)
{
    OsxAudioQueueOutput* self = (OsxAudioQueueOutput*)_self;
    UInt32               is_running = false;
    UInt32               property_size = sizeof(UInt32);
    OSStatus             status;
    
    status = AudioQueueGetProperty(queue, kAudioQueueProperty_IsRunning, &is_running, &property_size);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueGetProperty failed (%x)", status);
        return;
    }
    ATX_LOG_FINE_1("is_running property = %d", is_running);
    
    if (!is_running) {
        pthread_mutex_lock(&self->lock);
        if (self->waiting_for_stop) {
            self->waiting_for_stop = BLT_FALSE;
            pthread_cond_signal(&self->audio_queue_stopped_cond);
        }
        pthread_mutex_unlock(&self->lock);
    }
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_ConvertFormat
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_ConvertFormat(OsxAudioQueueOutput*         self,
                                  const BLT_MediaType*         media_type, 
                                  AudioStreamBasicDescription* audio_format)
{
    if (media_type->id == self->expected_media_types.pcm.base.id) {
        const BLT_PcmMediaType* pcm_type = (const BLT_PcmMediaType*) media_type;
        audio_format->mFormatID          = kAudioFormatLinearPCM;
        audio_format->mFormatFlags       = kAudioFormatFlagIsPacked;
        audio_format->mFramesPerPacket   = 1;
        audio_format->mSampleRate        = pcm_type->sample_rate;
        audio_format->mChannelsPerFrame  = pcm_type->channel_count;
        audio_format->mBitsPerChannel    = pcm_type->bits_per_sample;
        audio_format->mBytesPerFrame     = (audio_format->mBitsPerChannel * audio_format->mChannelsPerFrame) / 8;
        audio_format->mBytesPerPacket    = audio_format->mBytesPerFrame * audio_format->mFramesPerPacket;
        audio_format->mReserved          = 0;
        
        /* select the sample format */
        switch (pcm_type->sample_format) {
            case BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_BE:
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
                break;
            
            case BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE:
                break;
                
            case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE:
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
                break;
                
            case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE:
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
                break;
                
            case BLT_PCM_SAMPLE_FORMAT_FLOAT_BE:
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsFloat;
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
                break;
                
            case BLT_PCM_SAMPLE_FORMAT_FLOAT_LE:
                audio_format->mFormatFlags |= kLinearPCMFormatFlagIsFloat;
                break;
                
            default:
                return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
    } else if (media_type->id == self->expected_media_types.asbd.base.id) {
        const AsbdMediaType* asbd_type = (const AsbdMediaType*)media_type;
        *audio_format = asbd_type->asbd;
    } else {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_UpdateStreamFormat
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_UpdateStreamFormat(OsxAudioQueueOutput* self, 
                                       const BLT_MediaType* media_type)
{
    AudioStreamBasicDescription audio_format;
    BLT_Result                  result;
    OSStatus                    status;
    
    /* convert the media type into an audio format */
    result = OsxAudioQueueOutput_ConvertFormat(self, media_type, &audio_format);
    if (BLT_FAILED(result)) return result;
    
    /* do nothing if the format has not changed */
    if (self->audio_format.mFormatID         == audio_format.mFormatID         &&
        self->audio_format.mFormatFlags      == audio_format.mFormatFlags      &&
        self->audio_format.mFramesPerPacket  == audio_format.mFramesPerPacket  && 
        self->audio_format.mSampleRate       == audio_format.mSampleRate       && 
        self->audio_format.mChannelsPerFrame == audio_format.mChannelsPerFrame && 
        self->audio_format.mBitsPerChannel   == audio_format.mBitsPerChannel   && 
        self->audio_format.mBytesPerFrame    == audio_format.mBytesPerFrame    && 
        self->audio_format.mBytesPerPacket   == audio_format.mBytesPerPacket) {
        return BLT_SUCCESS;
    }
        
    /* reset any existing queue before we create a new one */
    if (self->audio_queue) {
        /* drain any pending packets before we switch */
        OsxAudioQueueOutput_Drain(&ATX_BASE(self, BLT_OutputNode));
        
        /* destroy the queue (this will also free the buffers) */
        AudioQueueDispose(self->audio_queue, true);
        self->audio_queue = NULL;
    }

    /* create an audio queue */
    status = AudioQueueNewOutput(&audio_format, 
                                 OsxAudioQueueOutput_BufferCallback,
                                 self,
                                 NULL,
                                 kCFRunLoopCommonModes,
                                 0,
                                 &self->audio_queue);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueNewOutput returned %x", status);
        self->audio_queue = NULL;
        return BLT_ERROR_UNSUPPORTED_FORMAT;
    }
    
    /* listen for property changes */
    status = AudioQueueAddPropertyListener(self->audio_queue, 
                                           kAudioQueueProperty_IsRunning, 
                                           OsxAudioQueueOutput_PropertyCallback, 
                                           self);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueAddPropertyListener returned %x", status);
        AudioQueueDispose(self->audio_queue, true);
        return BLT_FAILURE;
    }
    
    /* if this is an ASBD type, set the magic cookie and look at the buffer sizes */
    if (media_type->id == self->expected_media_types.asbd.base.id) {
        AsbdMediaType* asbd_media_type = (AsbdMediaType*)(media_type);
        if (asbd_media_type->magic_cookie_size) {
            status = AudioQueueSetProperty(self->audio_queue, 
                                           kAudioQueueProperty_MagicCookie, 
                                           asbd_media_type->magic_cookie, 
                                           asbd_media_type->magic_cookie_size);
            if (status != noErr) return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
        if (audio_format.mFramesPerPacket) {
            self->packet_count_max = 
                (((((UInt64)BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFERS_TOTAL_DURATION * 
                    (UInt64)audio_format.mSampleRate)/audio_format.mFramesPerPacket)/1000) +
                    BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT/2)/
                    BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT;
        } else {
            self->packet_count_max = 0;
        }
    } else {
        self->packet_count_max = 0;
    }
    self->packet_count = 0;
    ATX_LOG_FINE_1("max packets per buffer = %d", self->packet_count_max);
    
    /* create the buffers */
    {
        unsigned int i;
        unsigned int buffer_size;
        if (audio_format.mBytesPerFrame && ((UInt32)audio_format.mSampleRate != 0)) {
            buffer_size = (((UInt64)BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFERS_TOTAL_DURATION * 
                            (UInt64)audio_format.mBytesPerFrame*(UInt64)audio_format.mSampleRate)/1000)/
                            BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT;
        } else {
            buffer_size = BLT_OSX_AUDIO_QUEUE_OUTPUT_DEFAULT_BUFFER_SIZE;
        }
        ATX_LOG_FINE_1("buffer size = %d", buffer_size);
        for (i=0; i<BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT; i++) {
            AudioQueueAllocateBuffer(self->audio_queue, buffer_size, &self->buffers[i]);                
            self->buffers[i]->mUserData = NULL;
            self->buffers[i]->mAudioDataByteSize = 0;
        }
    }
            
    /* copy the format */
    self->audio_format = audio_format;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_WaitForCondition
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_WaitForCondition(OsxAudioQueueOutput* self, pthread_cond_t* condition)
{
    struct timespec timeout;
    struct timeval  now;
    gettimeofday(&now, NULL);
    timeout.tv_sec  = now.tv_sec+BLT_OSX_AUDIO_QUEUE_OUTPUT_MAX_WAIT;
    timeout.tv_nsec = now.tv_usec*1000;
    ATX_LOG_FINE("waiting for buffer...");
    int result = pthread_cond_timedwait(condition, &self->lock, &timeout);
    return result==0?BLT_SUCCESS:ATX_ERROR_TIMEOUT;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_WaitForBuffer
+---------------------------------------------------------------------*/
static AudioQueueBufferRef
OsxAudioQueueOutput_WaitForBuffer(OsxAudioQueueOutput* self)
{
    AudioQueueBufferRef buffer = self->buffers[self->buffer_index];
    
    /* check that we have a queue and buffers */
    if (self->audio_queue == NULL) return NULL;
    
    /* wait for the next buffer to be released */
    pthread_mutex_lock(&self->lock);
    while (buffer->mUserData) {
        /* the buffer is locked, wait for it to be released */
        BLT_Result result = OsxAudioQueueOutput_WaitForCondition(self, &self->buffer_released_cond);
        if (BLT_FAILED(result)) {
            ATX_LOG_WARNING("timeout while waiting for buffer");
            buffer = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&self->lock);
    
    return buffer;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Flush
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_Flush(OsxAudioQueueOutput* self)
{
    unsigned int i;
    
    /* check that we have a queue and buffers */
    if (self->audio_queue == NULL) return BLT_SUCCESS;

    /* reset the buffers */
    pthread_mutex_lock(&self->lock);
    for (i=0; i<BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT; i++) {
        self->buffers[i]->mAudioDataByteSize = 0;
        self->buffers[i]->mUserData = NULL;
    }
    pthread_mutex_unlock(&self->lock);
    self->packet_count = 0;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_EnqueueBuffer
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_EnqueueBuffer(OsxAudioQueueOutput* self)
{
    OSStatus            status;
    unsigned int        buffer_index = self->buffer_index;
    AudioQueueBufferRef buffer = self->buffers[self->buffer_index];
    AudioTimeStamp      start_time;
    AudioTimeStamp      current_time;
    unsigned int        packet_count = self->packet_count;
     
    /* reset the packet count */
    self->packet_count = 0;

    /* check that the buffer has data */
    if (buffer->mAudioDataByteSize == 0) return BLT_SUCCESS;
    
    /* mark this buffer as 'in queue' and move on to the next buffer */
    buffer->mUserData = self;
    self->buffer_index++;
    if (self->buffer_index == BLT_OSX_AUDIO_QUEUE_OUTPUT_BUFFER_COUNT) {
        self->buffer_index = 0;
    }
    
    /* automatically start the queue if it is not already running */
    if (!self->audio_queue_started) {
        OSStatus status = AudioQueueStart(self->audio_queue, NULL);
        ATX_LOG_FINE("auto-starting the queue");
        if (status != noErr) {
            ATX_LOG_WARNING_1("AudioQueueStart failed (%x)", status);
            return BLT_ERROR_INTERNAL;
        } 
        self->audio_queue_started = BLT_TRUE;
    }
    
    /* queue the buffer */
    ATX_LOG_FINE_2("enqueuing buffer %d, %d packets", buffer_index, packet_count);
    status = AudioQueueEnqueueBufferWithParameters(self->audio_queue, 
                                                   buffer, 
                                                   packet_count, 
                                                   packet_count?self->packet_descriptions:NULL, 
                                                   0, 
                                                   0, 
                                                   0, 
                                                   NULL, 
                                                   NULL, 
                                                   &start_time);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueEnqueueBuffer returned %x", status);
        buffer->mUserData = NULL;
        buffer->mAudioDataByteSize = 0;
        return BLT_FAILURE;
    }
    if (start_time.mFlags & kAudioTimeStampSampleTimeValid) {
        ATX_LOG_FINER_1("start_time = %lf", start_time.mSampleTime);
    } else {
        ATX_LOG_FINER("start_time not available");
        start_time.mSampleTime = 0;
    }
    status = AudioQueueGetCurrentTime(self->audio_queue, NULL, &current_time, NULL);
    if (status == noErr) {
        ATX_LOG_FINER_1("current time = %lf", current_time.mSampleTime);
        if ((current_time.mFlags & kAudioTimeStampSampleTimeValid) && 
            (start_time.mFlags & kAudioTimeStampSampleTimeValid)   &&
            (start_time.mSampleTime != 0)) {
            if (current_time.mSampleTime > start_time.mSampleTime) {
                ATX_LOG_FINE_1("audio queue underflow (%f)", (float)(current_time.mSampleTime - start_time.mSampleTime));
                AudioQueueStop(self->audio_queue, TRUE);
                AudioQueueStart(self->audio_queue, NULL /*&start_time*/);
            }
        }
    } else {
        ATX_LOG_FINER("no timestamp available");
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_PutPacket(BLT_PacketConsumer* _self,
                              BLT_MediaPacket*    packet)
{
    OsxAudioQueueOutput* self = ATX_SELF(OsxAudioQueueOutput, BLT_PacketConsumer);
    const BLT_MediaType* media_type;
    BLT_Result           result;

    /* check parameters */
    if (packet == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, &media_type);
    if (BLT_FAILED(result)) return result;

    /* update the media type */
    result = OsxAudioQueueOutput_UpdateStreamFormat(self, media_type);
    if (BLT_FAILED(result)) return result;

    /* exit early if the packet is empty */
    if (BLT_MediaPacket_GetPayloadSize(packet) == 0) return BLT_SUCCESS;
        
    /* ensure we're not paused */
    OsxAudioQueueOutput_Resume(&ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode));

    /* queue the packet */
    {
        const unsigned char* payload = (const unsigned char*)BLT_MediaPacket_GetPayloadBuffer(packet);
        BLT_Size             payload_size = BLT_MediaPacket_GetPayloadSize(packet);
        unsigned int         buffer_fullness = 0;
        
        /* wait for a buffer */
        AudioQueueBufferRef buffer = OsxAudioQueueOutput_WaitForBuffer(self);
        if (buffer == NULL) return BLT_ERROR_INTERNAL;
            
        /* check if there is enough space in the buffer */
        if (buffer->mAudioDataBytesCapacity-buffer->mAudioDataByteSize < payload_size) {
            /* not enough space, enqueue this buffer and wait for the next one */
            ATX_LOG_FINER("buffer full");
            OsxAudioQueueOutput_EnqueueBuffer(self);
            buffer = OsxAudioQueueOutput_WaitForBuffer(self);
            if (buffer == NULL) return BLT_ERROR_INTERNAL;
        }
        
        /* we should always have enough space at this point (unless the buffers are too small) */
        if (buffer->mAudioDataBytesCapacity-buffer->mAudioDataByteSize < payload_size) {
            ATX_LOG_WARNING_1("buffer too small! (%d needed)", payload_size);
            return BLT_ERROR_INTERNAL;
        }
        
        /* copy the data into the buffer */
        buffer_fullness = buffer->mAudioDataByteSize;
        ATX_CopyMemory((unsigned char*)buffer->mAudioData+buffer->mAudioDataByteSize,
                       payload,
                       payload_size);
        buffer->mAudioDataByteSize += payload_size;
        
        /* for ASBD types, update the packet descriptions */
        if (media_type->id == self->expected_media_types.asbd.base.id) {
            AudioStreamPacketDescription* packet_description = &self->packet_descriptions[self->packet_count++];
            packet_description->mDataByteSize           = payload_size;
            packet_description->mStartOffset            = buffer_fullness;
            packet_description->mVariableFramesInPacket = self->audio_format.mFramesPerPacket;
            
            /* enqueue now if we're used all the packet descriptions or there's enough in the buffer */
            if (self->packet_count == BLT_OSX_AUDIO_QUEUE_OUTPUT_PACKET_DESCRIPTION_COUNT ||
                (self->packet_count_max && self->packet_count >= self->packet_count_max)) {
                ATX_LOG_FINE_1("reached max packets in buffer (%d), enqueuing", self->packet_count);
                OsxAudioQueueOutput_EnqueueBuffer(self);
            }
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_QueryMediaType(BLT_MediaPort*        _self,
                                   BLT_Ordinal           index,
                                   const BLT_MediaType** media_type)
{
    OsxAudioQueueOutput* self = ATX_SELF(OsxAudioQueueOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_types.pcm;
        return BLT_SUCCESS;
    } else if (index == 1) {
        *media_type = (const BLT_MediaType*)&self->expected_media_types.asbd;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_Create(BLT_Module*              _module,
                           BLT_Core*                core, 
                           BLT_ModuleParametersType parameters_type,
                           BLT_CString              parameters, 
                           BLT_MediaNode**          object)
{
    OsxAudioQueueOutput*       self;
    OsxAudioQueueOutputModule* module = (OsxAudioQueueOutputModule*)_module;
    
    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(OsxAudioQueueOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), _module, core);

    /* create a lock and conditions */
    pthread_mutex_init(&self->lock, NULL);
    pthread_cond_init(&self->buffer_released_cond, NULL);
    pthread_cond_init(&self->audio_queue_stopped_cond, NULL);
    
    /* setup the expected media types */
    BLT_PcmMediaType_Init(&self->expected_media_types.pcm);
    self->expected_media_types.pcm.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
    BLT_MediaType_Init(&self->expected_media_types.asbd.base, module->asbd_media_type_id);
    
    /* initialize fields */
    self->volume = 1.0;
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, OsxAudioQueueOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (self, OsxAudioQueueOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE   (self, OsxAudioQueueOutput, BLT_OutputNode);
    ATX_SET_INTERFACE   (self, OsxAudioQueueOutput, BLT_VolumeControl);
    ATX_SET_INTERFACE   (self, OsxAudioQueueOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioQueueOutput_Destroy(OsxAudioQueueOutput* self)
{
    /* drain the queue */
    OsxAudioQueueOutput_Drain(&ATX_BASE(self, BLT_OutputNode));

    /* stop the audio pump */
    OsxAudioQueueOutput_Stop(&ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode));
    
    /* close the audio queue */
    if (self->audio_queue) {
        AudioQueueDispose(self->audio_queue, true);
    }
        
    /* destroy the lock and conditions */
    pthread_cond_destroy(&self->buffer_released_cond);
    pthread_cond_destroy(&self->audio_queue_stopped_cond);
    pthread_mutex_destroy(&self->lock);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                
/*----------------------------------------------------------------------
|   OsxAudioQueueOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_GetPortByName(BLT_MediaNode*  _self,
                                  BLT_CString     name,
                                  BLT_MediaPort** port)
{
    OsxAudioQueueOutput* self = ATX_SELF_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_Seek(BLT_MediaNode* _self,
                         BLT_SeekMode*  mode,
                         BLT_SeekPoint* point)
{
    OsxAudioQueueOutput* self = ATX_SELF_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode);
    
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* return now if no queue has been created */
    if (self->audio_queue == NULL) return BLT_SUCCESS;
    
    /* reset the queue */
    AudioQueueReset(self->audio_queue);

    /* flush any pending buffer */
    OsxAudioQueueOutput_Flush(self);
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_GetStatus(BLT_OutputNode*       _self,
                              BLT_OutputNodeStatus* status)
{
    OsxAudioQueueOutput* self = ATX_SELF(OsxAudioQueueOutput, BLT_OutputNode);
    
    /* default value */
    status->media_time.seconds     = 0;
    status->media_time.nanoseconds = 0;
    status->flags = 0;
    
    /* check if we're full */
    if (self->audio_queue) {
        pthread_mutex_lock(&self->lock);
        if (self->buffers[self->buffer_index]->mUserData) {
            /* buffer is busy */
            status->flags |= BLT_OUTPUT_NODE_STATUS_QUEUE_FULL;
        }
        pthread_mutex_unlock(&self->lock);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Drain
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_Drain(BLT_OutputNode* _self)
{
    OsxAudioQueueOutput* self = ATX_SELF(OsxAudioQueueOutput, BLT_OutputNode);
    OSStatus             status;
    
    /* if we're paused, there's nothing to drain */
    if (self->audio_queue_paused) return BLT_SUCCESS;
    
    /* in case we have a pending buffer, enqueue it now */
    AudioQueueBufferRef buffer = OsxAudioQueueOutput_WaitForBuffer(self);
    if (buffer && buffer->mAudioDataByteSize) OsxAudioQueueOutput_EnqueueBuffer(self);
    
    /* flush anything that may be in the queue */
    ATX_LOG_FINE("flusing queued buffers");
    status = AudioQueueFlush(self->audio_queue);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueFlush failed (%x)", status);
    }
    
    /* if the queue is not started, we're done */
    if (!self->audio_queue_started) return BLT_SUCCESS;
    
    /* wait for the queue to be stopped */
    pthread_mutex_lock(&self->lock);
    status = AudioQueueStop(self->audio_queue, false);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueStop failed (%x)", status);
    }
    self->waiting_for_stop = BLT_TRUE;
    OsxAudioQueueOutput_WaitForCondition(self, &self->audio_queue_stopped_cond);
    pthread_mutex_unlock(&self->lock);

    /* we're really stopped now */
    self->audio_queue_started = BLT_FALSE;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Start
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_Start(BLT_MediaNode* _self)
{
    /* do nothing here, because the queue is auto-started when packets arrive */
    BLT_COMPILER_UNUSED(_self);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_Stop(BLT_MediaNode* _self)
{
    OsxAudioQueueOutput* self = ATX_SELF_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (self->audio_queue == NULL) return BLT_SUCCESS;

    /* stop the audio queue */
    OSStatus status = AudioQueueStop(self->audio_queue, true);
    if (status != noErr) {
        ATX_LOG_WARNING_1("AudioQueueStop failed (%x)", status);
    }
    self->audio_queue_started = BLT_FALSE;

    /* flush any pending buffer */
    OsxAudioQueueOutput_Flush(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_Pause(BLT_MediaNode* _self)
{
    OsxAudioQueueOutput* self = ATX_SELF_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode);
    
    if (self->audio_queue && !self->audio_queue_paused) {
        OSStatus status = AudioQueuePause(self->audio_queue);
        if (status != noErr) {
            ATX_LOG_WARNING_1("AudioQueuePause failed (%x)", status);
        }
        self->audio_queue_paused = BLT_TRUE;
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_Resume
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_Resume(BLT_MediaNode* _self)
{
    OsxAudioQueueOutput* self = ATX_SELF_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (self->audio_queue && self->audio_queue_paused) {
        OSStatus status = AudioQueueStart(self->audio_queue, NULL);
        if (status != noErr) {
            ATX_LOG_WARNING_1("AudioQueueStart failed (%x)", status);
        }
        self->audio_queue_paused = BLT_FALSE;
        self->audio_queue_started = BLT_TRUE;
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_SetVolume
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_SetVolume(BLT_VolumeControl* _self, float volume)
{
    OsxAudioQueueOutput* self = ATX_SELF(OsxAudioQueueOutput, BLT_VolumeControl);
    
    self->volume = volume;
    if (self->audio_queue) {
        OSStatus status = AudioQueueSetParameter(self->audio_queue, kAudioQueueParam_Volume, volume);
        if (status != noErr) return BLT_FAILURE;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioQueueOutput_GetVolume
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutput_GetVolume(BLT_VolumeControl* _self, float* volume)
{
    OsxAudioQueueOutput* self = ATX_SELF(OsxAudioQueueOutput, BLT_VolumeControl);

    if (self->audio_queue) {
        OSStatus status = AudioQueueGetParameter(self->audio_queue, kAudioQueueParam_Volume, &self->volume);
        if (status == noErr) {
            *volume = self->volume;
            return BLT_SUCCESS;
        } else {
            *volume = 1.0f;
            return BLT_FAILURE;
        }
    } else {
        *volume = self->volume;
        return BLT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioQueueOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioQueueOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioQueueOutput, BLT_VolumeControl)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioQueueOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioQueueOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(OsxAudioQueueOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(OsxAudioQueueOutput, BLT_MediaPort)
    OsxAudioQueueOutput_GetName,
    OsxAudioQueueOutput_GetProtocol,
    OsxAudioQueueOutput_GetDirection,
    OsxAudioQueueOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioQueueOutput, BLT_PacketConsumer)
    OsxAudioQueueOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OsxAudioQueueOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    OsxAudioQueueOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    OsxAudioQueueOutput_Start,
    OsxAudioQueueOutput_Stop,
    OsxAudioQueueOutput_Pause,
    OsxAudioQueueOutput_Resume,
    OsxAudioQueueOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioQueueOutput, BLT_OutputNode)
    OsxAudioQueueOutput_GetStatus,
    OsxAudioQueueOutput_Drain
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_VolumeControl interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioQueueOutput, BLT_VolumeControl)
    OsxAudioQueueOutput_GetVolume,
    OsxAudioQueueOutput_SetVolume
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OsxAudioQueueOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   OsxAudioQueueOutputModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutputModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    OsxAudioQueueOutputModule* self = ATX_SELF_EX(OsxAudioQueueOutputModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*              registry;
    BLT_Result                 result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the audio/x-apple-asbd type id */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/x-apple-asbd",
        &self->asbd_media_type_id);
    if (BLT_FAILED(result)) return result;
    
    ATX_LOG_FINE_1("audio/x-apple-asbd type = %d", self->asbd_media_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   OsxAudioQueueOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioQueueOutputModule_Probe(BLT_Module*              _self, 
                                BLT_Core*                core,
                                BLT_ModuleParametersType parameters_type,
                                BLT_AnyConst             parameters,
                                BLT_Cardinal*            match)
{
    OsxAudioQueueOutputModule* self = (OsxAudioQueueOutputModule*)_self;
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the input type should be unknown, or audio/pcm */
            if (!(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.input.media_type->id == self->asbd_media_type_id) &&
                !(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* the name should be 'osxaq:<n>' */
            if (constructor->name == NULL ||
                !ATX_StringsEqualN(constructor->name, "osxaq:", 6)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioQueueOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioQueueOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioQueueOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(OsxAudioQueueOutputModule, OsxAudioQueueOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OsxAudioQueueOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    OsxAudioQueueOutputModule_Attach,
    OsxAudioQueueOutputModule_CreateInstance,
    OsxAudioQueueOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define OsxAudioQueueOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OsxAudioQueueOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(OsxAudioQueueOutputModule,
                                         "OSX Audio Queue Output",
                                         "com.axiosys.output.osx-audio-queue",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
#else 
/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_OsxAudioQueueOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;
    *object = NULL;
    return BLT_ERROR_NOT_SUPPORTED;
}
#endif

