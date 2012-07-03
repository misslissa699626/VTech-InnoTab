/*****************************************************************
|
|   Win32 Audio Output Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#ifndef STRICT
#define STRICT
#endif

#include <windows.h>
#include <assert.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltWin32AudioOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"
#include "BltVolumeControl.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.win32.audio")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#if !defined(_WIN32_WCE)
#define BLT_WIN32_AUDIO_OUTPUT_USE_WAVEFORMATEXTENSIBLE 
#endif

#if !defined(WAVE_FORMAT_DOLBY_AC3_SPDIF)
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092
#endif

#if defined(BLT_WIN32_AUDIO_OUTPUT_USE_WAVEFORMATEXTENSIBLE)
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>
static const GUID  BLT_WIN32_OUTPUT_KSDATAFORMAT_SUBTYPE_PCM = 
    {0x00000001,                  0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID BLT_WIN32_OUTPUT_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF = 
    {WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
#endif

#define BLT_WIN32_AUDIO_OUTPUT_AC3_SPDIF_FRAME_SIZE 6144

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(Win32AudioOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(Win32AudioOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(Win32AudioOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(Win32AudioOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(Win32AudioOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(Win32AudioOutput, BLT_PacketConsumer)
ATX_DECLARE_INTERFACE_MAP(Win32AudioOutput, BLT_VolumeControl)

BLT_METHOD Win32AudioOutput_Resume(BLT_MediaNode* self);

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_MediaPacket* media_packet;
    WAVEHDR          wave_header;
} QueueBuffer;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_MediaTypeId ac3_type_id;
} Win32AudioOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_VolumeControl);

    /* members */
    UINT              device_id;
    HWAVEOUT          device_handle;
    BLT_PcmMediaType  expected_media_types[2];
    BLT_PcmMediaType  media_type;
    BLT_MediaTypeId   ac3_type_id;
    BLT_Boolean       use_passthrough;
    BLT_Boolean       paused;
    BLT_UInt64        nb_samples_written;
    BLT_UInt64        timestamp_after_buffer;
    struct {
        ATX_List* packets;
    }                 free_queue;
    struct {
        ATX_List* packets;
        BLT_Size  buffered;
        BLT_Size  max_buffered;
    }                 pending_queue;
} Win32AudioOutput;

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
#define BLT_WIN32_OUTPUT_MAX_QUEUE_DURATION     1    /* seconds */
#define BLT_WIN32_OUTPUT_MAX_OPEN_RETRIES       10
#define BLT_WIN32_OUTPUT_OPEN_RETRY_SLEEP       30   /* milliseconds */
#define BLT_WIN32_OUTPUT_QUEUE_WAIT_SLEEP       100  /* milliseconds */ 
#define BLT_WIN32_OUTPUT_QUEUE_REQUEST_WATCHDOG 100
#define BLT_WIN32_OUTPUT_QUEUE_WAIT_WATCHDOG    50

/*----------------------------------------------------------------------
|    Win32AudioOutput_DestroyListItem
+---------------------------------------------------------------------*/
static void 
Win32AudioOutput_DestroyListItem(ATX_ListDataDestructor* self, ATX_Any data, ATX_UInt32 type)
{
    QueueBuffer* queue_buffer = (QueueBuffer*)data;
    if (queue_buffer->media_packet) {
        BLT_MediaPacket_Release(queue_buffer->media_packet);
    }
    ATX_FreeMemory(queue_buffer);

	BLT_COMPILER_UNUSED(self);	
	BLT_COMPILER_UNUSED(type);	
}

/*----------------------------------------------------------------------
|    Win32AudioOutputListItemDestructor
+---------------------------------------------------------------------*/
static void Win32AudioOutput_DestroyListItem(ATX_ListDataDestructor* self, ATX_Any data, ATX_UInt32 type);
ATX_ListDataDestructor 
Win32AudioOutputListItemDestructor = {
    NULL,
    Win32AudioOutput_DestroyListItem
};

/*----------------------------------------------------------------------
|    Win32AudioOutput_FreeQueueItem
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_FreeQueueItem(Win32AudioOutput* self, ATX_ListItem* item)
{
    QueueBuffer* queue_buffer = ATX_ListItem_GetData(item);

    /* unprepare the header */
    if (queue_buffer->wave_header.dwFlags & WHDR_PREPARED) {
        assert(queue_buffer->wave_header.dwFlags & WHDR_DONE);
        waveOutUnprepareHeader(self->device_handle, 
                               &queue_buffer->wave_header,
                               sizeof(WAVEHDR));
    }

    /* clear the header */
    queue_buffer->wave_header.dwFlags         = 0;
    queue_buffer->wave_header.dwBufferLength  = 0;
    queue_buffer->wave_header.dwBytesRecorded = 0;
    queue_buffer->wave_header.dwLoops         = 0;
    queue_buffer->wave_header.dwUser          = 0;
    queue_buffer->wave_header.lpData          = NULL;
    queue_buffer->wave_header.lpNext          = NULL;
    queue_buffer->wave_header.reserved        = 0;

    /* put the item on the free queue */
    ATX_List_AddItem(self->free_queue.packets, item);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_ReleaseQueueItem
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_ReleaseQueueItem(Win32AudioOutput* self, ATX_ListItem* item)
{
    QueueBuffer* queue_buffer = ATX_ListItem_GetData(item);

    /* free the queue item first */
    Win32AudioOutput_FreeQueueItem(self, item);

    /* release the media packet */
    /* NOTE: this needs to be done after the call to wavUnprepareHeader    */
    /* because the header's lpData field need to be valid when unpreparing */
    /* the header                                                          */
    if (queue_buffer->media_packet) {
        BLT_MediaPacket_Release(queue_buffer->media_packet);
        queue_buffer->media_packet = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_WaitForQueueItem
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_WaitForQueueItem(Win32AudioOutput* self, ATX_ListItem** item)
{
    ATX_Cardinal watchdog = BLT_WIN32_OUTPUT_QUEUE_WAIT_WATCHDOG;

    *item = ATX_List_GetFirstItem(self->pending_queue.packets);
    if (*item) {
        QueueBuffer* queue_buffer = (QueueBuffer*)ATX_ListItem_GetData(*item);
        DWORD volatile* flags = &queue_buffer->wave_header.dwFlags;
        assert(queue_buffer->wave_header.dwFlags & WHDR_PREPARED);
        while ((*flags & WHDR_DONE) == 0) {
            if (watchdog-- == 0) return BLT_FAILURE;
            Sleep(BLT_WIN32_OUTPUT_QUEUE_WAIT_SLEEP);
        }

        /* pop the item from the pending queue */
        ATX_List_DetachItem(self->pending_queue.packets, *item);
        self->pending_queue.buffered -= queue_buffer->wave_header.dwBufferLength;

        /*BLT_Debug("WaitForQueueItem: pending = %d (%d/%d buff), free = %d\n",
                  ATX_List_GetItemCount(self->pending_queue.packets),
                  self->pending_queue.buffered,
                  self->pending_queue.max_buffered,
                  ATX_List_GetItemCount(self->free_queue.packets));*/
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_RequestQueueItem
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_RequestQueueItem(Win32AudioOutput* self, ATX_ListItem** item)
{
    ATX_Cardinal watchdog = BLT_WIN32_OUTPUT_QUEUE_REQUEST_WATCHDOG;

    /* wait to the total pending buffers to be less than the max duration */
    while (self->pending_queue.buffered > 
           self->pending_queue.max_buffered) {
        BLT_Result    result;

        /* wait for the head of the queue to free up */
        result = Win32AudioOutput_WaitForQueueItem(self, item);
        if (BLT_FAILED(result)) {
            *item = NULL;
            return result;
        }
        /* the item should not be NULL */
        if (item == NULL) return BLT_ERROR_INTERNAL;

        Win32AudioOutput_ReleaseQueueItem(self, *item);

        if (watchdog-- == 0) return BLT_ERROR_INTERNAL;
    }

    /* if there is a buffer available in the free queue, return it */
    *item = ATX_List_GetLastItem(self->free_queue.packets);
    if (*item) {
        ATX_List_DetachItem(self->free_queue.packets, *item);
        return BLT_SUCCESS;
    }

    /* we get here is there ware no buffer in the free queue */
    {
        QueueBuffer* queue_buffer;
        queue_buffer = (QueueBuffer*)ATX_AllocateZeroMemory(sizeof(QueueBuffer));
        if (queue_buffer == NULL) {
            *item = NULL;
            return BLT_ERROR_OUT_OF_MEMORY;
        }
        *item = ATX_List_CreateItem(self->free_queue.packets);
        if (*item == NULL) {
            ATX_FreeMemory(queue_buffer);
            return BLT_ERROR_OUT_OF_MEMORY;
        }
        ATX_ListItem_SetData(*item, (ATX_Any)queue_buffer);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Drain
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_Drain(Win32AudioOutput* self)
{
    ATX_ListItem* item;
    BLT_Result    result;
    
    /* make sure we're not paused */
    Win32AudioOutput_Resume(&ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode));

    do {
        result = Win32AudioOutput_WaitForQueueItem(self, &item);
        if (BLT_SUCCEEDED(result) && item != NULL) {
            Win32AudioOutput_ReleaseQueueItem(self, item);
        }
    } while (BLT_SUCCEEDED(result) && item != NULL);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_OpenDevice
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_OpenDevice(UINT                    device_id, 
                            const BLT_PcmMediaType* media_type,
                            HWAVEOUT*               device_handle)

{
    MMRESULT     mm_result;
    BLT_Cardinal retry;

#if defined(BLT_WIN32_AUDIO_OUTPUT_USE_WAVEFORMATEXTENSIBLE)
    /* used for 24 and 32 bits per sample as well as multichannel */
    WAVEFORMATEXTENSIBLE format;
#else
    WAVEFORMATEX format;
#endif

    /* fill in format structure */
    ATX_SetMemory(&format, 0, sizeof(format));
#if defined(BLT_WIN32_AUDIO_OUTPUT_USE_WAVEFORMATEXTENSIBLE)
    if (media_type->base.id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        format.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
        format.Format.nChannels            = media_type->channel_count;
        format.Format.nSamplesPerSec       = media_type->sample_rate;
        format.Format.nBlockAlign          = media_type->channel_count * media_type->bits_per_sample/8;
        format.Format.nAvgBytesPerSec      = format.Format.nBlockAlign * format.Format.nSamplesPerSec;
        format.Format.wBitsPerSample       = media_type->bits_per_sample;
        format.Format.cbSize               = 22;
        format.Samples.wValidBitsPerSample = media_type->bits_per_sample;
        if (media_type->channel_mask && media_type->channel_count > 2) {
            format.dwChannelMask = media_type->channel_mask;
        } else {
            switch (media_type->channel_count) {
            case 1:
                format.dwChannelMask = KSAUDIO_SPEAKER_MONO;
                break;

            case 2:
                format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
                break;

            case 3:
                format.dwChannelMask = KSAUDIO_SPEAKER_STEREO |
                                       SPEAKER_FRONT_CENTER;
                break;

            case 4:
                format.dwChannelMask = KSAUDIO_SPEAKER_QUAD;
                break;

            case 6:
                format.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
                break;

            case 8:
                format.dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
                break;

            default:
                format.dwChannelMask = SPEAKER_ALL;
            }
        }
        format.SubFormat = BLT_WIN32_OUTPUT_KSDATAFORMAT_SUBTYPE_PCM; 
    } else {
        format.Format.wFormatTag           = WAVE_FORMAT_DOLBY_AC3_SPDIF;
        format.Format.nChannels            = 2;
        format.Format.nSamplesPerSec       = 48000;
        format.Format.nBlockAlign          = 4;
        format.Format.nAvgBytesPerSec      = 4*48000;
        format.Format.wBitsPerSample       = 16;
        format.Format.cbSize               = 22;
        format.Samples.wValidBitsPerSample = 16;
        format.SubFormat = BLT_WIN32_OUTPUT_KSDATAFORMAT_SUBTYPE_DOLBY_AC3_SPDIF;
        format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
    }
#else
    format.wFormatTag      = WAVE_FORMAT_PCM;
    format.nChannels       = media_type->channel_count; 
    format.nSamplesPerSec  = media_type->sample_rate;
    format.nBlockAlign     = media_type->channel_count *
                             media_type->bits_per_sample/8;
    format.nAvgBytesPerSec = format.nBlockAlign*format.nSamplesPerSec;
    format.wBitsPerSample  = media_type->bits_per_sample;
    format.cbSize          = 0;
#endif

    /* try to open the device */
    if (device_handle) *device_handle = NULL;
    for (retry = 0; retry < BLT_WIN32_OUTPUT_MAX_OPEN_RETRIES; retry++) {
        mm_result = waveOutOpen(device_handle, 
                                device_id, 
                                (const struct tWAVEFORMATEX*)&format,
                                0, 0, WAVE_ALLOWSYNC | (device_handle?0:WAVE_FORMAT_QUERY));
        if (mm_result != MMSYSERR_ALLOCATED) break;
        Sleep(BLT_WIN32_OUTPUT_OPEN_RETRY_SLEEP);
    }

    if (mm_result == MMSYSERR_ALLOCATED) {
        return BLT_ERROR_DEVICE_BUSY;
    }
    if (mm_result == MMSYSERR_BADDEVICEID || 
        mm_result == MMSYSERR_NODRIVER) {
        return BLT_ERROR_NO_SUCH_DEVICE;
    }
    if (mm_result == WAVERR_BADFORMAT) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }
    if (mm_result != MMSYSERR_NOERROR) {
        return BLT_ERROR_OPEN_FAILED;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_CheckDeviceCapabilities
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_CheckDeviceCapabilities(Win32AudioOutput* self)
{
    BLT_Result       result;
    BLT_PcmMediaType ac3_type;

    /* check AC-3 over SPDIF */
    BLT_MediaType_Init(&ac3_type.base, self->ac3_type_id);
    ac3_type.bits_per_sample = 16;
    ac3_type.channel_count   = 2;
    ac3_type.channel_mask    = 0;
    ac3_type.sample_format   = 0;
    ac3_type.sample_rate     = 48000;
    result = Win32AudioOutput_OpenDevice(self->device_id, 
                                         &ac3_type,
                                         NULL);
    if (result == BLT_ERROR_INVALID_MEDIA_TYPE) {
        self->use_passthrough = ATX_FALSE;
    }
    if (result == BLT_ERROR_NO_SUCH_DEVICE) {
        return result;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Open
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_Open(Win32AudioOutput* self)
{
    /* check current state */
    if (self->device_handle) {
        /* the device is already open */
        return BLT_SUCCESS;
    }

    /* reset some fields */
    self->nb_samples_written = 0;

    return Win32AudioOutput_OpenDevice(self->device_id,
                                       &self->media_type,
                                       &self->device_handle);
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Close
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_Close(Win32AudioOutput* self)
{
    /* shortcut */
    if (self->device_handle == NULL) {
        return BLT_SUCCESS;
    }

    /* wait for all buffers to be played */
    Win32AudioOutput_Drain(self);

    /* reset device */
    waveOutReset(self->device_handle);

    /* close the device */
    waveOutClose(self->device_handle);

    /* release all queued packets */
    {
        ATX_ListItem* item;
        while ((item = ATX_List_GetFirstItem(self->pending_queue.packets))) {
            ATX_List_DetachItem(self->pending_queue.packets, item);
            Win32AudioOutput_ReleaseQueueItem(self, item);
        }
    }

    /* reset counters */
    self->nb_samples_written = 0;

    /* clear the device handle */
    self->device_handle = NULL;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_PutPacket(BLT_PacketConsumer* _self,
                           BLT_MediaPacket*    packet)
{
    Win32AudioOutput* self = ATX_SELF(Win32AudioOutput, BLT_PacketConsumer);
    BLT_MediaType*    media_type;
    QueueBuffer*      queue_buffer = NULL;
    ATX_ListItem*     queue_item = NULL;
    BLT_Result        result;
    MMRESULT          mm_result;

    /* check parameters */
    if (packet == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* get the media type */
    result = BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
    if (BLT_FAILED(result)) goto failed;

    /* check the media type */
    if (media_type->id == self->ac3_type_id) {
        if (!self->use_passthrough) {
            ATX_LOG_FINE("refusing AC-3 media, because we have not selected the mode or it is not supported");
            return BLT_ERROR_INVALID_MEDIA_TYPE;
        }
    } else if (media_type->id != BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        result = BLT_ERROR_INVALID_MEDIA_TYPE;
        goto failed;
    }

    /* for PCM, compare the media format with the current format */
    if (media_type->id == BLT_MEDIA_TYPE_ID_AUDIO_PCM) {
        BLT_PcmMediaType* pcm_type = (BLT_PcmMediaType*)media_type;
        if (pcm_type->sample_rate     != self->media_type.sample_rate   ||
            pcm_type->channel_count   != self->media_type.channel_count ||
            pcm_type->bits_per_sample != self->media_type.bits_per_sample) {
            /* new format */

            /* check the format */
            if (pcm_type->sample_rate     == 0 ||
                pcm_type->channel_count   == 0 ||
                pcm_type->bits_per_sample == 0) {
                return BLT_ERROR_INVALID_MEDIA_FORMAT;
            }
            
            /* perform basic validity checks of the format */
            if (pcm_type->sample_format != BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE) {
                return BLT_ERROR_INVALID_MEDIA_TYPE;
            }
            if (pcm_type->bits_per_sample !=  8 &&
                pcm_type->bits_per_sample != 16 &&
                pcm_type->bits_per_sample != 24 &&
                pcm_type->bits_per_sample != 32) {
                return BLT_ERROR_INVALID_MEDIA_TYPE;
            }

            /* copy the format */
            self->media_type = *pcm_type;

            /* recompute the max queue buffer size */
            self->pending_queue.max_buffered = 
                BLT_WIN32_OUTPUT_MAX_QUEUE_DURATION *
                pcm_type->sample_rate *
                pcm_type->channel_count *
                (pcm_type->bits_per_sample/8);

            /* close the device */
            result = Win32AudioOutput_Close(self);
            if (BLT_FAILED(result)) goto failed;
        }
    } else if (media_type->id != self->media_type.base.id) {
        self->media_type.base = *media_type;
        if (media_type->id == self->ac3_type_id) {
            self->pending_queue.max_buffered = 
                BLT_WIN32_OUTPUT_MAX_QUEUE_DURATION*48000*4;
            self->media_type.bits_per_sample = 16;
            self->media_type.channel_count   = 2;
            self->media_type.sample_rate     = 48000;
        }
    }

    /* for AC-3/SPDIF we need to create an SPDIF buffer */
    if (media_type->id == self->ac3_type_id) {
        unsigned char*   ac3_payload      = BLT_MediaPacket_GetPayloadBuffer(packet);
        unsigned int     ac3_payload_size = BLT_MediaPacket_GetPayloadSize(packet);
        unsigned char*   spdif_payload    = NULL;
        BLT_MediaPacket* spdif_packet     = NULL;
        result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core, 
                                            BLT_WIN32_AUDIO_OUTPUT_AC3_SPDIF_FRAME_SIZE,
                                            media_type,
                                            &spdif_packet);
        if (BLT_FAILED(result)) goto failed;
        BLT_MediaPacket_SetPayloadSize(spdif_packet, BLT_WIN32_AUDIO_OUTPUT_AC3_SPDIF_FRAME_SIZE);
        spdif_payload = BLT_MediaPacket_GetPayloadBuffer(spdif_packet);
        
        /* setup the SPDIF header */
        spdif_payload[0] = 0x72;
        spdif_payload[1] = 0xF8;
        spdif_payload[2] = 0x1F;
        spdif_payload[3] = 0x4E;
        spdif_payload[4] = 0x01;
        spdif_payload[5] = ac3_payload[5] & 0x07; /* bsmod */
        spdif_payload[6] = (unsigned char)((ac3_payload_size*8)     ); /* frame size in bits, LSB */
        spdif_payload[7] = (unsigned char)((ac3_payload_size*8) >> 8); /* frame size in bits, MSB */

        if (ac3_payload_size <= BLT_WIN32_AUDIO_OUTPUT_AC3_SPDIF_FRAME_SIZE-8) {
            ATX_CopyMemory(&spdif_payload[8], ac3_payload, ac3_payload_size);
            ATX_SetMemory(&spdif_payload[8+ac3_payload_size], 0, BLT_WIN32_AUDIO_OUTPUT_AC3_SPDIF_FRAME_SIZE-8-ac3_payload_size);

            /* swap bytes if needed (to make the byte order little endian) */
            if (ac3_payload[0] == 0x0B) {
                unsigned int i;
                for (i=0; i<ac3_payload_size/2; i++) {
                    unsigned short word = spdif_payload[8+i];
                    spdif_payload[8+i] = (word>>8)|(word<<8);
                }
            }
        }

        packet = spdif_packet;
    }

    /* ensure that the device is open */
    result = Win32AudioOutput_Open(self);
    if (BLT_FAILED(result)) goto failed;

    /* ensure we're not paused */
    Win32AudioOutput_Resume(&ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode));

    /* wait for space in the queue */
    result = Win32AudioOutput_RequestQueueItem(self, &queue_item);
    if (BLT_FAILED(result)) goto failed;
    queue_buffer = ATX_ListItem_GetData(queue_item);

    /* setup the queue element */
    queue_buffer->wave_header.lpData         = BLT_MediaPacket_GetPayloadBuffer(packet);
    queue_buffer->wave_header.dwBufferLength = BLT_MediaPacket_GetPayloadSize(packet);
    assert((queue_buffer->wave_header.dwFlags & WHDR_PREPARED) == 0);
    mm_result = waveOutPrepareHeader(self->device_handle, 
                                     &queue_buffer->wave_header, 
                                     sizeof(WAVEHDR));
    if (mm_result != MMSYSERR_NOERROR) {
        goto failed;
    }
    queue_buffer->media_packet = packet;

    /* send the sample buffer to the driver */
    assert((queue_buffer->wave_header.dwFlags & WHDR_DONE) == 0);
    assert(queue_buffer->wave_header.dwFlags & WHDR_PREPARED);
    mm_result = waveOutWrite(self->device_handle, 
                             &queue_buffer->wave_header,
                             sizeof(WAVEHDR));
    if (mm_result != MMSYSERR_NOERROR) {
        goto failed;
    }

    /* keep a count of the number of samples written               */
    /* keep track of the timestamp after the *last* sample written */
    {
        BLT_UInt64 packet_samples  = BLT_MediaPacket_GetPayloadSize(packet)/(self->media_type.channel_count*(self->media_type.bits_per_sample/8));
        BLT_UInt64 packet_duration = (packet_samples*1000000000)/self->media_type.sample_rate;
        BLT_UInt64 packet_ts       = BLT_TimeStamp_ToNanos(BLT_MediaPacket_GetTimeStamp(packet));
        self->nb_samples_written += packet_samples;
        self->timestamp_after_buffer = packet_ts+packet_duration;
        ATX_LOG_FINE_5("packet_ts=%lld, packet_samples=%lld, packet_duration=%lld, total_written=%lld, ts after buffer=%lld",
                       packet_ts,
                       packet_samples, 
                       packet_duration,
                       self->nb_samples_written,
                       self->timestamp_after_buffer);
    }

    /* queue the packet */
    ATX_List_AddItem(self->pending_queue.packets, queue_item);
    self->pending_queue.buffered += queue_buffer->wave_header.dwBufferLength;

    /* keep a reference to the packet */
    BLT_MediaPacket_AddReference(packet);

    return BLT_SUCCESS;

failed:
    if (queue_item) {
        Win32AudioOutput_FreeQueueItem(self, queue_item);
    }
    return result;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    Win32AudioOutput* self = ATX_SELF(Win32AudioOutput, BLT_MediaPort);

    if (index == 0) {
        if (self->use_passthrough) {
            /* prefer AC-3 by default */
            *media_type = (const BLT_MediaType*)&self->expected_media_types[1];
        } else {
            *media_type = (const BLT_MediaType*)&self->expected_media_types[0];
        }
    } else if (index == 1 && self->use_passthrough) {
        *media_type = (const BLT_MediaType*)&self->expected_media_types[0];
    } else {
        *media_type = NULL;
        return ATX_ERROR_NO_SUCH_ITEM;
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_Create(BLT_Module*              module,
                        BLT_Core*                core, 
                        BLT_ModuleParametersType parameters_type,
                        BLT_CString              parameters, 
                        BLT_MediaNode**          object)
{
    Win32AudioOutput*         self;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    const char*               device_name;
    BLT_Result                result;

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(Win32AudioOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* parse the device name */
    self->device_id = WAVE_MAPPER;
    device_name = constructor->name+5;
    if (device_name[0] == '+') {
        self->use_passthrough = ATX_TRUE;
        ++device_name;
    }
    if (ATX_SUCCEEDED(ATX_ParseIntegerU(device_name, &self->device_id, ATX_FALSE))) {
        if (self->device_id > 0) --self->device_id;
    } else {
        ATX_FreeMemory(self);
        return BLT_ERROR_NO_SUCH_DEVICE;
    }
    ATX_LOG_FINE_1("selected device id %d", self->device_id);

    /* construct the object */
    self->device_handle              = NULL;
    self->media_type.sample_rate     = 0;
    self->media_type.channel_count   = 0;
    self->media_type.bits_per_sample = 0;
    self->ac3_type_id                = ((Win32AudioOutputModule*)module)->ac3_type_id;
    self->pending_queue.buffered     = 0;
    self->pending_queue.max_buffered = 0;
    self->nb_samples_written         = 0;

    /* check if the device exists and if it supports passthrough */
    result = Win32AudioOutput_CheckDeviceCapabilities(self);
    if (BLT_FAILED(result)) {
        ATX_FreeMemory(self);
        return result;
    }

    /* create the packet queues */
    ATX_List_CreateEx(&Win32AudioOutputListItemDestructor, &self->free_queue.packets);
    ATX_List_CreateEx(&Win32AudioOutputListItemDestructor, &self->pending_queue.packets);

    /* setup the expected media type */
    BLT_PcmMediaType_Init(&self->expected_media_types[0]);
    self->expected_media_types[0].sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;
    BLT_PcmMediaType_Init(&self->expected_media_types[1]);
    self->expected_media_types[1].base.id = self->ac3_type_id;

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, Win32AudioOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (self, Win32AudioOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE   (self, Win32AudioOutput, BLT_OutputNode);
    ATX_SET_INTERFACE   (self, Win32AudioOutput, BLT_MediaPort);
    ATX_SET_INTERFACE   (self, Win32AudioOutput, BLT_VolumeControl);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Win32AudioOutput_Destroy(Win32AudioOutput* self)
{
    /* close the handle */
    Win32AudioOutput_Close(self);

    /* free resources */
    ATX_List_Destroy(self->free_queue.packets);
    ATX_List_Destroy(self->pending_queue.packets);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                
/*----------------------------------------------------------------------
|   Win32AudioOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_GetPortByName(BLT_MediaNode*  _self,
                               BLT_CString     name,
                               BLT_MediaPort** port)
{
    Win32AudioOutput* self = ATX_SELF_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_Seek(BLT_MediaNode* _self,
                      BLT_SeekMode*  mode,
                      BLT_SeekPoint* point)
{
    Win32AudioOutput* self = ATX_SELF_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    /* reset the device */
    waveOutReset(self->device_handle);

    /* reset counters */
    self->nb_samples_written = 0;
    if (point->mask & BLT_SEEK_POINT_MASK_TIME_STAMP) {
        self->timestamp_after_buffer = BLT_TimeStamp_ToNanos(point->time_stamp);
    } else {
        self->timestamp_after_buffer = 0;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_GetStatus(BLT_OutputNode*       _self,
                           BLT_OutputNodeStatus* status)
{
    Win32AudioOutput* self = ATX_SELF(Win32AudioOutput, BLT_OutputNode);
    MMRESULT          result;
    MMTIME            position;
	
    /* default value */
    status->flags = 0;

	/* get the output position from the device */
    position.wType = TIME_SAMPLES;
    result = waveOutGetPosition(self->device_handle,  
                                &position, sizeof(position)); 
    if (result == MMSYSERR_NOERROR     && 
        position.wType == TIME_SAMPLES && 
        self->media_type.sample_rate != 0) {
        /* compute the buffer delay (taking into account the fact that */
        /* position.u.sample only has 32-bit of presision              */
        BLT_UInt64 delay_samples = ((BLT_UInt64)(self->nb_samples_written - position.u.sample))%0x100000000;
        BLT_UInt64 delay_nanos = (delay_samples * 1000000000) / self->media_type.sample_rate;
        ATX_LOG_FINER_2("audio buffer delay=%lld nanoseconds, %lld samples", delay_nanos, delay_samples);
        if (self->timestamp_after_buffer >= delay_nanos) {
            ATX_LOG_FINER_2("timestamp after buffer=%lld - media_time=%lld", 
                            self->timestamp_after_buffer, 
                            self->timestamp_after_buffer-delay_nanos);
            status->media_time = BLT_TimeStamp_FromNanos(self->timestamp_after_buffer-delay_nanos);
        } else {
            ATX_LOG_FINER_1("timestamp after buffer=%lld -> less than delay!", 
                            self->timestamp_after_buffer);
            status->media_time.seconds = status->media_time.nanoseconds = 0;
        }
    } else {
        /* can't measure the buffer delay, use some estimate */
        status->media_time = BLT_TimeStamp_FromNanos(self->timestamp_after_buffer);
    } 

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_Stop(BLT_MediaNode* _self)
{
    Win32AudioOutput* self = ATX_SELF_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode);
    waveOutReset(self->device_handle);
    self->nb_samples_written = 0;
    self->timestamp_after_buffer = 0;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Pause
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_Pause(BLT_MediaNode* _self)
{
    Win32AudioOutput* self = ATX_SELF_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode);
    if (!self->paused) {
        waveOutPause(self->device_handle);
        self->paused = BLT_TRUE;
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_Resume
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_Resume(BLT_MediaNode* _self)
{
    Win32AudioOutput* self = ATX_SELF_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode);
    if (self->paused) {
        waveOutRestart(self->device_handle);
        self->paused = BLT_FALSE;
    }
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_SetVolume
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_SetVolume(BLT_VolumeControl* _self, float volume)
{
    Win32AudioOutput* self = ATX_SELF(Win32AudioOutput, BLT_VolumeControl);
    DWORD ivolume = (DWORD)(volume*65535.0f);
    MMRESULT result;
    
    result = waveOutSetVolume(self->device_handle, ivolume | (ivolume<<16));
    if (result != MMSYSERR_NOERROR) {
        ATX_LOG_WARNING_1("waveOutSetVolume() failed (%x)", result);
        return BLT_FAILURE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Win32AudioOutput_GetVolume
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutput_GetVolume(BLT_VolumeControl* _self, float* volume)
{
    Win32AudioOutput* self = ATX_SELF(Win32AudioOutput, BLT_VolumeControl);
    DWORD ivolume = 0;
    MMRESULT result;

    *volume = 0.0f; /* default value */

    result = waveOutGetVolume(self->device_handle, &ivolume);
    if (result != MMSYSERR_NOERROR) {
        ATX_LOG_WARNING_1("waveOutGetVolume() failed (%x)", result);
        return BLT_FAILURE;
    } else {
        unsigned short left  =  (unsigned short)((ivolume>>16)&0xFFFF);
        unsigned short right =  (unsigned short)(ivolume&0xFFFF);
        *volume = ((float)(left+right))/(2.0f*65535.0f);
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Win32AudioOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(Win32AudioOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(Win32AudioOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT(Win32AudioOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(Win32AudioOutput, BLT_PacketConsumer)
    ATX_GET_INTERFACE_ACCEPT(Win32AudioOutput, BLT_VolumeControl)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(Win32AudioOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(Win32AudioOutput, BLT_MediaPort)
    Win32AudioOutput_GetName,
    Win32AudioOutput_GetProtocol,
    Win32AudioOutput_GetDirection,
    Win32AudioOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Win32AudioOutput, BLT_PacketConsumer)
    Win32AudioOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(Win32AudioOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    Win32AudioOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    Win32AudioOutput_Stop,
    Win32AudioOutput_Pause,
    Win32AudioOutput_Resume,
    Win32AudioOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Win32AudioOutput, BLT_OutputNode)
    Win32AudioOutput_GetStatus,
    NULL
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_VolumeControl interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Win32AudioOutput, BLT_VolumeControl)
    Win32AudioOutput_GetVolume,
    Win32AudioOutput_SetVolume
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(Win32AudioOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   Win32AudioOutputModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutputModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    Win32AudioOutputModule* self = ATX_SELF_EX(Win32AudioOutputModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*           registry;
    BLT_Result              result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/ac3" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/ac3",
        &self->ac3_type_id);
    if (BLT_FAILED(result)) return result;
     
    ATX_LOG_FINE_1("(audio/ac3 type = %d)", self->ac3_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Win32AudioOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
Win32AudioOutputModule_Probe(BLT_Module*              _self, 
                             BLT_Core*                core,
                             BLT_ModuleParametersType parameters_type,
                             BLT_AnyConst             parameters,
                             BLT_Cardinal*            match)
{
    Win32AudioOutputModule* self = ATX_SELF_EX(Win32AudioOutputModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the input type should be unknown, audio/pcm or audio/ac3 */
            if (constructor->spec.input.media_type->id != BLT_MEDIA_TYPE_ID_AUDIO_PCM &&
                constructor->spec.input.media_type->id != self->ac3_type_id &&
                constructor->spec.input.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN) {
                return BLT_FAILURE;
            }

            /* the name should be 'wave:<n>' or 'wave:+<n>' (+ for spdif/hdmi passthrough) */
            if (constructor->name == NULL) return BLT_FAILURE;
            if (!ATX_StringsEqualN(constructor->name, "wave:", 5)) return BLT_FAILURE;
            if (ATX_StringLength(constructor->name) < 6) return BLT_FAILURE;
            if (constructor->name[5] != '+') {
                /* we cannot do AC-3 over plain wave */
                if (constructor->spec.input.media_type->id == self->ac3_type_id) return BLT_FAILURE;
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Win32AudioOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(Win32AudioOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(Win32AudioOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(Win32AudioOutputModule, Win32AudioOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(Win32AudioOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    Win32AudioOutputModule_Attach,
    Win32AudioOutputModule_CreateInstance,
    Win32AudioOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define Win32AudioOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(Win32AudioOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   node constructor
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_CONSTRUCTOR(Win32AudioOutputModule, "Win32 Audio Output", 0)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_Win32AudioOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return Win32AudioOutputModule_Create(object);
}
