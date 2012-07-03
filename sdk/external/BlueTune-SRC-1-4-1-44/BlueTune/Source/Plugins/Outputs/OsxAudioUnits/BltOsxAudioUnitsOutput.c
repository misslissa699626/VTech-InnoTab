/*****************************************************************
|
|   OSX Audio Units Output Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/

#include <TargetConditionals.h>
#include <AvailabilityMacros.h>
#if !defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AudioUnit.h>
#include <pthread.h>
#include <unistd.h>
#endif

#include "Atomix.h"
#include "BltConfig.h"
#include "BltOsxAudioUnitsOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"
#include "BltVolumeControl.h"

#if !defined(TARGET_OS_IPHONE) || !TARGET_OS_IPHONE

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.osx.audio-units")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_OSX_AUDIO_UNITS_OUTPUT_DEFAULT_PACKET_QUEUE_SIZE    32     /* packets */
#define BLT_OSX_AUDIO_UNITS_OUTPUT_SLEEP_INTERVAL       100000 /* us -> 0.1 secs */
#define BLT_OSX_AUDIO_UNITS_OUTPUT_MAX_QUEUE_WAIT_COUNT (5000000/BLT_OSX_AUDIO_UNITS_OUTPUT_SLEEP_INTERVAL) /* 5 secs */

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} OsxAudioUnitsOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_VolumeControl);

    /* members */
    unsigned int      audio_device_index;
    AudioDeviceID     audio_device_id;
    AudioUnit         audio_unit;
    pthread_mutex_t   lock;
    ATX_List*         packet_queue;
    BLT_Cardinal      max_packets_in_queue;
    BLT_PcmMediaType  expected_media_type;
    BLT_PcmMediaType  media_type;
    BLT_Boolean       paused;
    struct {
        BLT_TimeStamp rendered_packet_ts;
        UInt64        rendered_host_time;
        BLT_TimeStamp rendered_duration;
    }                 media_time_snapshot;
} OsxAudioUnitsOutput;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_PacketConsumer)
ATX_DECLARE_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_VolumeControl)

BLT_METHOD OsxAudioUnitsOutput_Resume(BLT_MediaNode* self);
BLT_METHOD OsxAudioUnitsOutput_Stop(BLT_MediaNode* self);
BLT_METHOD OsxAudioUnitsOutput_Drain(BLT_OutputNode* self);

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_RenderCallback
+---------------------------------------------------------------------*/
static OSStatus     
OsxAudioUnitsOutput_RenderCallback(void*						inRefCon,
                                   AudioUnitRenderActionFlags*	ioActionFlags,
                                   const AudioTimeStamp*		inTimeStamp,
                                   UInt32						inBusNumber,
                                   UInt32						inNumberFrames,
                                   AudioBufferList*			    ioData)
{
    OsxAudioUnitsOutput* self = (OsxAudioUnitsOutput*)inRefCon;
    ATX_ListItem*        item;
    unsigned int         requested = ioData->mBuffers[0].mDataByteSize;
    unsigned char*       out = (unsigned char*)ioData->mBuffers[0].mData;
    ATX_Boolean          timestamp_measured = ATX_FALSE;
    
    BLT_COMPILER_UNUSED(ioActionFlags);
    BLT_COMPILER_UNUSED(inTimeStamp);
    BLT_COMPILER_UNUSED(inBusNumber);
    BLT_COMPILER_UNUSED(inNumberFrames);
                
    /* sanity check on the parameters */
    if (ioData == NULL || ioData->mNumberBuffers == 0) return 0;
    
    /* in case we have a strange request with more than one buffer, just return silence */
    if (ioData->mNumberBuffers != 1) {
        unsigned int i;
        ATX_LOG_FINEST_1("strange request with %d buffers", 
                         ioData->mNumberBuffers);
        for (i=0; i<ioData->mNumberBuffers; i++) {
            ATX_SetMemory(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
        }
        return 0;
    }
    
    ATX_LOG_FINEST_2("request for %d bytes, %d frames", requested, inNumberFrames);
    
    /* lock the packet queue */
    pthread_mutex_lock(&self->lock);
    
    /* return now if we're paused */
    //if (self->paused) goto end;
    
    /* abort early if we have no packets */
    if (ATX_List_GetItemCount(self->packet_queue) == 0) goto end;
        
    /* fill as much as we can */
    while (requested && (item = ATX_List_GetFirstItem(self->packet_queue))) {
        BLT_MediaPacket*        packet = ATX_ListItem_GetData(item);
        const BLT_PcmMediaType* media_type;
        BLT_Size                payload_size;
        BLT_Size                chunk_size;
        BLT_TimeStamp           chunk_duration;
        BLT_TimeStamp           packet_ts;
        unsigned int            bytes_per_frame;
        unsigned int            sample_rate;
        
        /* get the packet info */
        BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
        packet_ts = BLT_MediaPacket_GetTimeStamp(packet);
        bytes_per_frame = media_type->channel_count*media_type->bits_per_sample/8;
        sample_rate = media_type->sample_rate;
        
        /* record the timestamp if we have not already done so */
        if (!timestamp_measured) {
            self->media_time_snapshot.rendered_packet_ts = packet_ts;
            self->media_time_snapshot.rendered_host_time = 
                AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
            BLT_TimeStamp_Set(self->media_time_snapshot.rendered_duration, 0, 0);
            timestamp_measured = ATX_TRUE;
            
            ATX_LOG_FINEST_2("rendered TS: packet ts=%lld, host ts=%lld",
                             BLT_TimeStamp_ToNanos(packet_ts),
                             self->media_time_snapshot.rendered_host_time);
        }
         
        /* compute how much to copy from this packet */
        payload_size = BLT_MediaPacket_GetPayloadSize(packet);
        if (payload_size <= requested) {
            /* copy the entire payload and remove the packet from the queue */
            chunk_size = payload_size;
            ATX_CopyMemory(out, BLT_MediaPacket_GetPayloadBuffer(packet), chunk_size);
            ATX_List_RemoveItem(self->packet_queue, item);
            packet = NULL;
            media_type = NULL;
            ATX_LOG_FINER_1("media packet fully consumed, %d left in queue",
                            ATX_List_GetItemCount(self->packet_queue));
        } else {
            /* only copy a portion of the payload */
            chunk_size = requested;
            ATX_CopyMemory(out, BLT_MediaPacket_GetPayloadBuffer(packet), chunk_size);            
        }
        
        /* update the counters */
        requested -= chunk_size;
        out       += chunk_size;
        
        /* update the media time snapshot */
        if (bytes_per_frame) {
            unsigned int frames_in_chunk = chunk_size/bytes_per_frame;
            chunk_duration = BLT_TimeStamp_FromSamples(frames_in_chunk, sample_rate);
        } else {
            BLT_TimeStamp_Set(chunk_duration, 0, 0);
        }
        self->media_time_snapshot.rendered_duration = 
            BLT_TimeStamp_Add(self->media_time_snapshot.rendered_duration, chunk_duration);
        
        /* update the packet unless we're done with it */
        if (packet) {
            /* update the packet offset and timestamp */
            BLT_MediaPacket_SetPayloadOffset(packet, BLT_MediaPacket_GetPayloadOffset(packet)+chunk_size);
            BLT_MediaPacket_SetTimeStamp(packet, BLT_TimeStamp_Add(packet_ts, chunk_duration));
        }
    }
   
end:
    /* fill whatever is left with silence */    
    if (requested) {
        ATX_LOG_FINEST_1("filling with %d bytes of silence", requested);
        ATX_SetMemory(out, 0, requested);
    }
    
    pthread_mutex_unlock(&self->lock);
        
    return 0;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_QueueItemDestructor
+---------------------------------------------------------------------*/
static void 
OsxAudioUnitsOutput_QueueItemDestructor(ATX_ListDataDestructor* self, 
                                        ATX_Any                 data, 
                                        ATX_UInt32              type)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(type);
    
    BLT_MediaPacket_Release((BLT_MediaPacket*)data);
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_QueuePacket
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioUnitsOutput_QueuePacket(OsxAudioUnitsOutput* self, BLT_MediaPacket* packet)
{
    BLT_Result   result   = BLT_SUCCESS;
    unsigned int watchdog = BLT_OSX_AUDIO_UNITS_OUTPUT_MAX_QUEUE_WAIT_COUNT;
    
    ATX_LOG_FINER("queuing packet");
    
    /* lock the queue */
    pthread_mutex_lock(&self->lock);
    
    /* wait for some space in the queue */
    while (ATX_List_GetItemCount(self->packet_queue) >= self->max_packets_in_queue) {
        pthread_mutex_unlock(&self->lock);
        usleep(BLT_OSX_AUDIO_UNITS_OUTPUT_SLEEP_INTERVAL);
        pthread_mutex_lock(&self->lock);
        
        if (--watchdog == 0) {
            ATX_LOG_WARNING("*** the watchdog bit us ***");
            goto end;
        }
    }
    
    /* add the packet to the queue */
    ATX_List_AddData(self->packet_queue, packet);
    ATX_LOG_FINER_1("packet queued, %d in queue", ATX_List_GetItemCount(self->packet_queue));
    
    /* keep a reference to the packet */
    BLT_MediaPacket_AddReference(packet);
    
end:
    /* unlock the queue */
    pthread_mutex_unlock(&self->lock);
    
    return result;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_SetStreamFormat
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioUnitsOutput_SetStreamFormat(OsxAudioUnitsOutput*    self, 
                                    const BLT_PcmMediaType* media_type)
{
    OSStatus                    result;
    AudioStreamBasicDescription audio_desc;
    
    /* setup the audio description */
    audio_desc.mFormatID         = kAudioFormatLinearPCM;
    audio_desc.mFormatFlags      = kAudioFormatFlagIsPacked;
    audio_desc.mFramesPerPacket  = 1;
    audio_desc.mSampleRate       = media_type->sample_rate;
    audio_desc.mChannelsPerFrame = media_type->channel_count;
    audio_desc.mBitsPerChannel   = media_type->bits_per_sample;
    audio_desc.mBytesPerFrame    = (audio_desc.mBitsPerChannel * audio_desc.mChannelsPerFrame) / 8;
    audio_desc.mBytesPerPacket   = audio_desc.mBytesPerFrame * audio_desc.mFramesPerPacket;
    audio_desc.mReserved         = 0;
    
    /* select the sample format */
    switch (media_type->sample_format) {
        case BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_BE:
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
            break;
        
        case BLT_PCM_SAMPLE_FORMAT_UNSIGNED_INT_LE:
            break;
            
        case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_BE:
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
            break;
            
        case BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_LE:
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
            break;
            
        case BLT_PCM_SAMPLE_FORMAT_FLOAT_BE:
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsBigEndian;
            break;
            
        case BLT_PCM_SAMPLE_FORMAT_FLOAT_LE:
            audio_desc.mFormatFlags |= kLinearPCMFormatFlagIsFloat;
            break;
            
        default:
            return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* drain any pending packets before we switch */
    result = OsxAudioUnitsOutput_Drain(&ATX_BASE(self, BLT_OutputNode));
    if (BLT_FAILED(result)) return result;

    /* set the audio unit property */
    result = AudioUnitSetProperty(self->audio_unit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  0,
                                  &audio_desc,
                                  sizeof(audio_desc));
    if (result != noErr) {
        ATX_LOG_WARNING_1("AudioUnitSetProperty failed (%d)", result);
        return BLT_FAILURE;
    }
    
    /* copy the format */
    self->media_type = *media_type;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_PutPacket(BLT_PacketConsumer* _self,
                              BLT_MediaPacket*    packet)
{
    OsxAudioUnitsOutput*    self = ATX_SELF(OsxAudioUnitsOutput, BLT_PacketConsumer);
    const BLT_PcmMediaType* media_type;
    BLT_Result              result;

    /* check parameters */
    if (packet == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
    if (BLT_FAILED(result)) return result;

    /* check the media type */
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* compare the media format with the current format */
    if (media_type->sample_rate     != self->media_type.sample_rate   ||
        media_type->channel_count   != self->media_type.channel_count ||
        media_type->bits_per_sample != self->media_type.bits_per_sample) {
        /* new format */
        
        /* check the format */
        if (media_type->sample_rate     == 0 ||
            media_type->channel_count   == 0 ||
            media_type->bits_per_sample == 0) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }

        /* check for the supported sample widths */
        if (media_type->bits_per_sample !=  8 &&
            media_type->bits_per_sample != 16 &&
            media_type->bits_per_sample != 24 &&
            media_type->bits_per_sample != 32) {
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
                        
        /* update the audio unit */
        result = OsxAudioUnitsOutput_SetStreamFormat(self, media_type);
        if (BLT_FAILED(result)) return result;
    }
    
    /* queue the packet */
    result = OsxAudioUnitsOutput_QueuePacket(self, packet);
    if (BLT_FAILED(result)) return result;
    
    /* ensure we're not paused */
    return OsxAudioUnitsOutput_Resume(&ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode));
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_QueryMediaType(BLT_MediaPort*        _self,
                                   BLT_Ordinal           index,
                                   const BLT_MediaType** media_type)
{
    OsxAudioUnitsOutput* self = ATX_SELF(OsxAudioUnitsOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   OsxAudioUnitsOutput_MapDeviceIndex
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioUnitsOutput_MapDeviceName(const char* name, AudioDeviceID* device_id)
{
    OSStatus       err;
    BLT_Result     result = BLT_ERROR_NO_SUCH_DEVICE;
    UInt32         prop_size = 0;
    AudioDeviceID* devices = NULL;
    unsigned int   device_count = 0;
    ATX_UInt32     device_index = 0;
    unsigned int   device_selector = 1;
    char*          device_name = NULL;
    unsigned int   i;
    
    /* setup a default value */
    *device_id = 0;
    
    /* check the parameters */
    if (name[0] == '\0') return BLT_ERROR_NO_SUCH_DEVICE;
    
    /* parse the name */
    if (name[0] == '#') {
        ++name;
        ATX_LOG_FINE_1("device name = %s", name);
    } else {
        if (BLT_FAILED(ATX_ParseInteger32U(name, &device_index, ATX_FALSE))) {
            return BLT_ERROR_NO_SUCH_DEVICE;
        }
        ATX_LOG_FINE_1("device index = %d", device_index);
        name = NULL;
        
        /* 0 means default */
        if (device_index == 0) {
            /* look at the device's streams */
            AudioDeviceID default_device_id = 0;
            prop_size = sizeof(AudioDeviceID);
            err = AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &prop_size, &default_device_id);
            if (err == noErr) {
                prop_size = 0;
                err = AudioDeviceGetPropertyInfo(default_device_id, 0, FALSE, kAudioDevicePropertyStreams, &prop_size, NULL);
                if (err == noErr) {
                    ATX_LOG_FINE_1("device has %d streams", prop_size/4);
                }
            }
            return BLT_SUCCESS;
        }
    }
    
    /* ask how many devices exist */
    err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &prop_size, NULL);
    if (err != noErr) {
        ATX_LOG_FINE_1("AudioHardwareGetPropertyInfo failed (%d)", err);
        return 0;
    }
    device_count = prop_size/sizeof(AudioDeviceID);
    ATX_LOG_FINE_1("found %d devices", device_count);
    
    /* allocate memory for the array */
    devices = (AudioDeviceID*)ATX_AllocateZeroMemory(sizeof(AudioDeviceID) * device_count);
    if (devices == NULL) return 0;
    
    /* retrieve the list of device IDs */
    err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices, &prop_size, devices);
    if (err != noErr) {
        ATX_LOG_FINE_1("AudioHardwareGetProperty(kAudioHardwarePropertyDevices) failed (%d)", err);
        return 0;
    }
    
    /* find the device ID we want */
    for (i=0; i<device_count; i++) {
        /* get the device name */
        err = AudioDeviceGetPropertyInfo(devices[i], 0, false,
                                         kAudioDevicePropertyDeviceName,
                                         &prop_size, NULL);
        if (err == noErr) {
            if (device_name) ATX_FreeMemory(device_name);
            device_name = (char*)ATX_AllocateZeroMemory(prop_size+1);
            err = AudioDeviceGetProperty(devices[i], 0, false,
                                         kAudioDevicePropertyDeviceName,
                                         &prop_size, device_name);
            if (err == noErr) {
                ATX_LOG_FINE_2("device name [%d] = %s", i, device_name);
            }
            /* cleanup the string */
            device_name[prop_size] = '\0'; /* NULL-terminate the string */
            if (prop_size > 0) {
                unsigned int x;
                for (x=prop_size-1; x; x--) {
                    if (device_name[x] == ' ') {
                        device_name[x] = '\0';
                    } else if (device_name[x]) {
                        break;
                    }
                }
            }
        } else {
            continue;
        }
        
        /* look at the device's streams */
        prop_size = 0;
        err = AudioDeviceGetPropertyInfo(devices[i], 0, FALSE, kAudioDevicePropertyStreams, &prop_size, NULL);
        if (err != noErr || prop_size == 0) {
            ATX_LOG_FINE("skipping device (not an output)");
            continue;
        }

        if (name) {
            /* look for a match by name */
            if (ATX_StringsEqual(device_name, name)) {
                *device_id = devices[i];
                ATX_LOG_FINE("device selected as output");
                result = BLT_SUCCESS;
                break;
            }
        } else {
            /* look for a match by index */
            if (device_selector++ == device_index) {
                *device_id = devices[i];
                ATX_LOG_FINE("device selected as output");
                result = BLT_SUCCESS;
                break;
            }
        }
    }
    
    if (device_name) ATX_FreeMemory(device_name);
    if (devices)     ATX_FreeMemory(devices);
    
    return result;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_CheckDownmix
+---------------------------------------------------------------------*/
static void
OsxAudioUnitsOutput_CheckDownmix(OsxAudioUnitsOutput* self)
{
    OSStatus err;
    UInt32   prop_size;
    
    err = AudioDeviceGetPropertyInfo(self->audio_device_id, 0, FALSE, kAudioDevicePropertyStreamConfiguration, &prop_size, NULL);
    if (err != noErr) return;

    AudioBufferList* buffers = (AudioBufferList *)ATX_AllocateZeroMemory(prop_size);
    err = AudioDeviceGetProperty(self->audio_device_id, 0, FALSE, kAudioDevicePropertyStreamConfiguration, &prop_size, buffers);
    if (err == noErr) {
        unsigned int i;
        unsigned int channel_count = 0;
        for (i = 0; i < buffers->mNumberBuffers; ++i) {
            channel_count += buffers->mBuffers[i].mNumberChannels;
        }
        ATX_LOG_FINE_1("device has %d channels", channel_count);
    }
    
    ATX_FreeMemory(buffers);
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioUnitsOutput_Create(BLT_Module*              module,
                           BLT_Core*                core, 
                           BLT_ModuleParametersType parameters_type,
                           const void*              parameters, 
                           BLT_MediaNode**          object)
{
    OsxAudioUnitsOutput*      self;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    AudioDeviceID             audio_device_id = 0;
    AudioUnit                 audio_unit = NULL;
    Component                 component;
    ComponentDescription      component_desc;
    ComponentResult           result;
    BLT_Result                blt_result;
    
    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* parse the name */
    if (!ATX_StringsEqualN(constructor->name, "osxau:", 6)) {
        return BLT_ERROR_INTERNAL;
    }

    /* map the name into a device ID */
    blt_result = OsxAudioUnitsOutput_MapDeviceName(constructor->name+6, &audio_device_id);
    if (BLT_FAILED(blt_result)) return blt_result;
    
    /* get the default output audio unit */
    ATX_SetMemory(&component_desc, 0, sizeof(component_desc));
    component_desc.componentType         = kAudioUnitType_Output;
    component_desc.componentSubType      = kAudioUnitSubType_HALOutput;
    component_desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    component_desc.componentFlags        = 0;
    component_desc.componentFlagsMask    = 0;
    component = FindNextComponent(NULL, &component_desc);
    if (component == NULL) {
        ATX_LOG_WARNING("FindNextComponent failed");
        return BLT_FAILURE;
    }
    
    /* open the audio unit (we will initialize it later) */
    result = OpenAComponent(component, &audio_unit);
    if (result != noErr) {
        ATX_LOG_WARNING_1("OpenAComponent failed (%d)", result);
        return BLT_FAILURE;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(OsxAudioUnitsOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    self->audio_device_id            = audio_device_id;
    self->audio_unit                 = audio_unit;
    self->media_type.sample_rate     = 0;
    self->media_type.channel_count   = 0;
    self->media_type.bits_per_sample = 0;

    /* create a lock */
    pthread_mutex_init(&self->lock, NULL);
    
    /* create the packet queue */
    {
        ATX_ListDataDestructor destructor = { NULL, OsxAudioUnitsOutput_QueueItemDestructor };
        ATX_List_CreateEx(&destructor, &self->packet_queue);
        self->max_packets_in_queue = BLT_OSX_AUDIO_UNITS_OUTPUT_DEFAULT_PACKET_QUEUE_SIZE;
    }
    
    /* setup the expected media type */
    BLT_PcmMediaType_Init(&self->expected_media_type);
    self->expected_media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, OsxAudioUnitsOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (self, OsxAudioUnitsOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE   (self, OsxAudioUnitsOutput, BLT_OutputNode);
    ATX_SET_INTERFACE   (self, OsxAudioUnitsOutput, BLT_MediaPort);
    ATX_SET_INTERFACE   (self, OsxAudioUnitsOutput, BLT_VolumeControl);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
OsxAudioUnitsOutput_Destroy(OsxAudioUnitsOutput* self)
{
    /* drain the queue */
    OsxAudioUnitsOutput_Drain(&ATX_BASE(self, BLT_OutputNode));

    /* stop the audio pump */
    OsxAudioUnitsOutput_Stop(&ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode));
    
    /* close the audio unit */
    if (self->audio_unit) {
        ComponentResult result;
        
        result = CloseComponent(self->audio_unit);
        if (result != noErr) {
            ATX_LOG_WARNING_1("CloseComponent failed (%d)", result);
        }
    }
    
    /* destroy the queue */
    ATX_List_Destroy(self->packet_queue);
    
    /* destroy the lock */
    pthread_mutex_destroy(&self->lock);
    
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                
/*----------------------------------------------------------------------
|   OsxAudioUnitsOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_GetPortByName(BLT_MediaNode*  _self,
                                  BLT_CString     name,
                                  BLT_MediaPort** port)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Seek(BLT_MediaNode* _self,
                         BLT_SeekMode*  mode,
                         BLT_SeekPoint* point)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult      result;
    
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);


    /* flush the queue */
    pthread_mutex_lock(&self->lock);
    ATX_List_Clear(self->packet_queue);
    pthread_mutex_unlock(&self->lock);
    
    /* reset the device */
    result = AudioUnitReset(self->audio_unit, kAudioUnitScope_Input, 0);
    if (result != noErr) {
        ATX_LOG_WARNING_1("AudioUnitReset failed (%d)", result);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_GetStatus(BLT_OutputNode*       _self,
                              BLT_OutputNodeStatus* status)
{
    OsxAudioUnitsOutput* self = ATX_SELF(OsxAudioUnitsOutput, BLT_OutputNode);
    
    /* default values */
    status->flags = 0;
    
    pthread_mutex_lock(&self->lock);
    
    /* check if the queue is full */
    if (ATX_List_GetItemCount(self->packet_queue) >= self->max_packets_in_queue) {
        ATX_LOG_FINER("packet queue is full");
        status->flags |= BLT_OUTPUT_NODE_STATUS_QUEUE_FULL;
    }

    /* compute the media time */
    BLT_TimeStamp_Set(status->media_time, 0, 0);
    if (self->media_time_snapshot.rendered_host_time) {
        UInt64 host_time  = AudioConvertHostTimeToNanos(AudioGetCurrentHostTime());
        UInt64 media_time = BLT_TimeStamp_ToNanos(self->media_time_snapshot.rendered_packet_ts); 
        ATX_LOG_FINER_3("host time = %lld, last rendered packet = %lld, rendered ts = %lld", 
                        host_time, 
                        self->media_time_snapshot.rendered_host_time,
                        media_time);
        if (host_time > self->media_time_snapshot.rendered_host_time) {
            media_time += host_time-self->media_time_snapshot.rendered_host_time;
        } 
        UInt64 max_media_time;
        max_media_time = BLT_TimeStamp_ToNanos(self->media_time_snapshot.rendered_packet_ts) +
                         BLT_TimeStamp_ToNanos(self->media_time_snapshot.rendered_duration);
        ATX_LOG_FINER_2("computed media time = %lld, max media time = %lld",
                        media_time, max_media_time);
        if (media_time > max_media_time) {
            ATX_LOG_FINER("media time clamped to max");
            media_time = max_media_time;
        }
        status->media_time = BLT_TimeStamp_FromNanos(media_time);
        ATX_LOG_FINER_1("media time = %lld", media_time);
    }
    
    pthread_mutex_unlock(&self->lock);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Drain
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Drain(BLT_OutputNode* _self)
{
    OsxAudioUnitsOutput* self = ATX_SELF(OsxAudioUnitsOutput, BLT_OutputNode);
    unsigned int watchdog = 20000000/BLT_OSX_AUDIO_UNITS_OUTPUT_SLEEP_INTERVAL;
    
    ATX_LOG_FINER("draining packets"); 

    /* lock the queue */
    pthread_mutex_lock(&self->lock);
    
    /* wait until there are no more packets in the queue */
    while (ATX_List_GetItemCount(self->packet_queue)) {
        pthread_mutex_unlock(&self->lock);
        ATX_LOG_FINER("waiting..."); 
        usleep(BLT_OSX_AUDIO_UNITS_OUTPUT_SLEEP_INTERVAL);
        pthread_mutex_lock(&self->lock);
        
        if (--watchdog == 0) {
            ATX_LOG_WARNING("*** the watchdog bit us ***");
            break;
        }
    }

    /* unlock the queue */
    pthread_mutex_unlock(&self->lock);
    
    ATX_LOG_FINER("end"); 

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Start
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Start(BLT_MediaNode* _self)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult      result;
    
    /* start the audio unit */
    result = AudioOutputUnitStart(self->audio_unit);
    if (result != noErr) {
        ATX_LOG_WARNING_1("AudioUnitOutputStart failed (%d)", result);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Stop(BLT_MediaNode* _self)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult      result;
    

    /* flush the queue */
    pthread_mutex_lock(&self->lock);
    ATX_List_Clear(self->packet_queue);
    pthread_mutex_unlock(&self->lock);

    /* stop the and reset audio unit */
    result = AudioOutputUnitStop(self->audio_unit);
    if (result != noErr) {
        ATX_LOG_WARNING_1("AudioUnitOutputStop failed (%d)", result);
    }
    result = AudioUnitReset(self->audio_unit, kAudioUnitScope_Input, 0);
    if (result != noErr) {
        ATX_LOG_WARNING_1("AudioUnitReset failed (%d)", result);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Pause(BLT_MediaNode* _self)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult      result;
    
    ATX_LOG_FINE("pausing output");
    if (!self->paused) {
        self->paused = BLT_TRUE;
        result = AudioOutputUnitStop(self->audio_unit);
        if (result != noErr) {
            ATX_LOG_WARNING_1("AudioUnitOutputStop failed (%d)", result);
        }
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_Resume
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Resume(BLT_MediaNode* _self)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult      result;

    if (self->paused) {
        ATX_LOG_FINE("resuming output");
        self->paused = BLT_FALSE;
        result = AudioOutputUnitStart(self->audio_unit);
        if (result != noErr) {
            ATX_LOG_WARNING_1("AudioUnitOutputStart failed (%d)", result);
        }
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_SetVolume
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_SetVolume(BLT_VolumeControl* _self, float volume)
{
    OsxAudioUnitsOutput* self = ATX_SELF(OsxAudioUnitsOutput, BLT_VolumeControl);
    ComponentResult      result;
    Float32              au_volume = volume;
    
    result = AudioUnitSetParameter(self->audio_unit, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, au_volume, 0);
    if (result == noErr) {
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }    
}

/*----------------------------------------------------------------------
|    OsxAudioUnitsOutput_GetVolume
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_GetVolume(BLT_VolumeControl* _self, float* volume)
{
    OsxAudioUnitsOutput* self = ATX_SELF(OsxAudioUnitsOutput, BLT_VolumeControl);
    ComponentResult      result;
    Float32              au_volume = 1.0f;
    
    result = AudioUnitGetParameter(self->audio_unit, kHALOutputParam_Volume, kAudioUnitScope_Global, 0, &au_volume);
    if (result == noErr) {
        *volume = au_volume;
        return BLT_SUCCESS;
    } else {
        *volume = 1.0f;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   OsxAudioUnitsOutput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    OsxAudioUnitsOutput*   self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult        result;
    AURenderCallbackStruct callback;
    
    BLT_COMPILER_UNUSED(stream);
        
    ATX_LOG_FINER("start");

    /* select the device */
    if (self->audio_device_id) {
        result = AudioUnitSetProperty(self->audio_unit,
                                      kAudioOutputUnitProperty_CurrentDevice,
                                      kAudioUnitScope_Global,
                                      0,
                                      &self->audio_device_id,
                                      sizeof(self->audio_device_id));
        if (result != noErr) {
            ATX_LOG_WARNING_1("AudioUnitSetProperty (kAudioOutputUnitProperty_CurrentDevice) failed (%d)", result);
        }
    }

    /* initialize the output */
    if (self->audio_unit) {
        result = AudioUnitInitialize(self->audio_unit);
        if (result != noErr) {
            ATX_LOG_WARNING_1("AudioUnitInitialize failed (%d)", result);
            return BLT_FAILURE;
        }
    }

    /* set some default audio format */
    {
        AudioStreamBasicDescription audio_desc;
        ATX_SetMemory(&audio_desc, 0, sizeof(audio_desc));
        
        /* setup the audio description */
        audio_desc.mFormatID         = kAudioFormatLinearPCM;
        audio_desc.mFormatFlags      = kAudioFormatFlagsNativeEndian | kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
        audio_desc.mFramesPerPacket  = 1;
        audio_desc.mSampleRate       = 44100;
        audio_desc.mChannelsPerFrame = 2;
        audio_desc.mBitsPerChannel   = 16;
        audio_desc.mBytesPerFrame    = (audio_desc.mBitsPerChannel * audio_desc.mChannelsPerFrame) / 8;
        audio_desc.mBytesPerPacket   = audio_desc.mBytesPerFrame * audio_desc.mFramesPerPacket;
        audio_desc.mReserved         = 0;
        result = AudioUnitSetProperty(self->audio_unit,
                                      kAudioUnitProperty_StreamFormat,
                                      kAudioUnitScope_Input,
                                      0,
                                      &audio_desc,
                                      sizeof(audio_desc));
        if (result != noErr) {
            ATX_LOG_WARNING_1("AudioUnitSetProperty failed (%d)", result);
            return BLT_FAILURE;
        }
    }

    /* check for downmix based on the number of supported channels */
    OsxAudioUnitsOutput_CheckDownmix(self);
    
    /* setup the callback */
    callback.inputProc = OsxAudioUnitsOutput_RenderCallback;
    callback.inputProcRefCon = _self;
    result = AudioUnitSetProperty(self->audio_unit, 
                                  kAudioUnitProperty_SetRenderCallback, 
                                  kAudioUnitScope_Input, 
                                  0,
                                  &callback, 
                                  sizeof(callback));
    if (result != noErr) {
        ATX_LOG_SEVERE_1("AudioUnitSetProperty failed when setting callback (%d)", result);
        return BLT_FAILURE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       OsxAudioUnitsOutput_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutput_Deactivate(BLT_MediaNode* _self)
{
    OsxAudioUnitsOutput* self = ATX_SELF_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ComponentResult      result;

    ATX_LOG_FINER("OsxAudioUnitsOutput::Deactivate");

    /* un-initialize the device */
    if (self->audio_unit) {
        result = AudioUnitUninitialize(self->audio_unit);
        if (result != noErr) {
            ATX_LOG_WARNING_1("AudioUnitUninitialize failed (%d)", result);
            return BLT_FAILURE;
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioUnitsOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioUnitsOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioUnitsOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioUnitsOutput, BLT_PacketConsumer)
    ATX_GET_INTERFACE_ACCEPT   (OsxAudioUnitsOutput, BLT_VolumeControl)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(OsxAudioUnitsOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_MediaPort)
    OsxAudioUnitsOutput_GetName,
    OsxAudioUnitsOutput_GetProtocol,
    OsxAudioUnitsOutput_GetDirection,
    OsxAudioUnitsOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_PacketConsumer)
    OsxAudioUnitsOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OsxAudioUnitsOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    OsxAudioUnitsOutput_GetPortByName,
    OsxAudioUnitsOutput_Activate,
    OsxAudioUnitsOutput_Deactivate,
    OsxAudioUnitsOutput_Start,
    OsxAudioUnitsOutput_Stop,
    OsxAudioUnitsOutput_Pause,
    OsxAudioUnitsOutput_Resume,
    OsxAudioUnitsOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_OutputNode)
    OsxAudioUnitsOutput_GetStatus,
    OsxAudioUnitsOutput_Drain
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_VolumeControl interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(OsxAudioUnitsOutput, BLT_VolumeControl)
    OsxAudioUnitsOutput_GetVolume,
    OsxAudioUnitsOutput_SetVolume
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OsxAudioUnitsOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   OsxAudioUnitsOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
OsxAudioUnitsOutputModule_Probe(BLT_Module*              self, 
                                BLT_Core*                core,
                                BLT_ModuleParametersType parameters_type,
                                BLT_AnyConst             parameters,
                                BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the input type should be unknown, or audio/pcm */
            if (!(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) &&
                !(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* the name should be 'osxau:<n>' */
            if (constructor->name == NULL ||
                !ATX_StringsEqualN(constructor->name, "osxau:", 6)) {
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(OsxAudioUnitsOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioUnitsOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(OsxAudioUnitsOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(OsxAudioUnitsOutputModule, OsxAudioUnitsOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(OsxAudioUnitsOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    OsxAudioUnitsOutputModule_CreateInstance,
    OsxAudioUnitsOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define OsxAudioUnitsOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(OsxAudioUnitsOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(OsxAudioUnitsOutputModule,
                                         "OSX Audio Units Output",
                                         "com.axiosys.output.osx-audio-units",
                                         "1.2.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
#else /* TARGET_OS_IPHONE */
/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_OsxAudioUnitsOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;
    *object = NULL;
    return BLT_ERROR_NOT_SUPPORTED;
}
#endif
