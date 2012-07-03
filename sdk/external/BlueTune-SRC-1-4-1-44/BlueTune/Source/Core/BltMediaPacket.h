/*****************************************************************
|
|   BlueTune - Media Packet Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_MediaPacket object.
 */

#ifndef _BLT_MEDIA_PACKET_H_
#define _BLT_MEDIA_PACKET_H_

/**
 * @defgroup BLT_MediaPacket BLT_MediaPacket class
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltMedia.h"
#include "BltTime.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
/**
 * Media Packet object.
 * A media packet encapsulates a payload of media data and a data type.
 * The object manages an internal memory buffer. The payload is a portion
 * of the internal buffer.
 */
typedef struct BLT_MediaPacket BLT_MediaPacket;

/** @} */

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
/**
 * @defgroup BLT_MediaPacket_Flags BLT_MediaPacket flags
 * @ingroup BLT_MediaPacket
 * @{
 */

/**
 * This flag indicates that this packet contains the start of a stream
 * (the first byte of the payload must be the start of the stream, or the
 * payload must be empty)
 */
#define BLT_MEDIA_PACKET_FLAG_START_OF_STREAM           0x01

/**
 * This flag indicates that this packet contains the end of a stream
 * (the last byte of the payload must be the end of the stream, or the
 * payload must be empty)
 */
#define BLT_MEDIA_PACKET_FLAG_END_OF_STREAM             0x02

/**
 * This flag indicates that there has been a discontinuity in the stream
 * (packet loss, or seek)
 */
#define BLT_MEDIA_PACKET_FLAG_STREAM_DISCONTINUITY      0x04

/**
 * This flag indicates that the payload of the packet represents stream
 * metadata (such as a stream header). It is not required that the packet
 * payload only contains metadata.
 */
#define BLT_MEDIA_PACKET_FLAG_STREAM_METADATA           0x08

/** @} */

/** @addtogroup media_packet
 * @{
 */

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Increase the reference counter of a packet.
 */
BLT_Result BLT_MediaPacket_AddReference(BLT_MediaPacket* packet);

/**
 * Release a packet (decrement the reference counter).
 */
BLT_Result BLT_MediaPacket_Release(BLT_MediaPacket* packet);

/**
 * Returns a pointer to the packet's payload buffer.
 */
BLT_Any    BLT_MediaPacket_GetPayloadBuffer(BLT_MediaPacket* packet);

/**
 * Sets the packet's payload buffer to be a subset (window) of the
 * actual memory buffer managed by the packet.
 * @param offset Offset in bytes from the beginning of the internal buffer
 * to the packet's payload.
 * @param size Size in bytes of the packet's payload.
 */
BLT_Result BLT_MediaPacket_SetPayloadWindow(BLT_MediaPacket* packet,
                                            BLT_Offset       offset,
                                            BLT_Size         size);
/**
 * Returns the size of the packet's payload.
 */
BLT_Size   BLT_MediaPacket_GetPayloadSize(BLT_MediaPacket* packet);

/**
 * Sets the size of the packet's payload.
 * @param size Size in bytes of the payload. The internal buffer may
 * be reallocated if the current buffer is too small to accomodate the
 * requested size.
 */
BLT_Result BLT_MediaPacket_SetPayloadSize(BLT_MediaPacket* packet,
                                          BLT_Size         size);

/**
 * Returns the size of the internal buffer (not the payload size).
 */
BLT_Size   BLT_MediaPacket_GetAllocatedSize(BLT_MediaPacket* packet);

/**
 * Sets the minimum size of the internal buffer.
 * @param size Size of the internal buffer.
 */
BLT_Result BLT_MediaPacket_SetAllocatedSize(BLT_MediaPacket* packet,
                                            BLT_Size         size);

/**
 * Returns the offset of the payload from the beginning of the internal
 * buffer.
 */
BLT_Offset BLT_MediaPacket_GetPayloadOffset(BLT_MediaPacket* packet);

/**
 * Sets the offset of the payload.
 * @param offset Offset in bytes of the payload from the beginning
 * of the internal buffer.
 */
BLT_Result BLT_MediaPacket_SetPayloadOffset(BLT_MediaPacket* packet,
                                            BLT_Offset       offset);

/**
 * Returns the type of the media data in this packet.
 * @param type Pointer to a pointer that will, upon return, point
 * to a BLT_MediaType structure representing the type information.
 */
BLT_Result BLT_MediaPacket_GetMediaType(BLT_MediaPacket* packet,
                                        const BLT_MediaType** type);

/**
 * Sets the type of the media data in this packet.
 * This method makes an internal copy of the media type structure.
 * @param type Pointer to a BLT_MediaType structure representing the
 * type information.
 */
BLT_Result BLT_MediaPacket_SetMediaType(BLT_MediaPacket*     packet,
                                        const BLT_MediaType* type);

/**
 * Sets the timestamp associated with this media packet.
 * @param time_stamp Time stamp to associate with the packet.
 */
BLT_Result BLT_MediaPacket_SetTimeStamp(BLT_MediaPacket* packet,
                                        BLT_TimeStamp    time_stamp);

/**
 * Returns the time stamp associated with this media packet.
 */
BLT_TimeStamp BLT_MediaPacket_GetTimeStamp(BLT_MediaPacket* packet);

/**
 * Returns the duration of the media data in this packet. The duration
 * has a zero value if the duration of the data in this packet is not
 * known or cannot be established.
 */
BLT_Time      BLT_MediaPacket_GetDuration(BLT_MediaPacket* packet);

/**
 * Sets the duration of the media data in this packet.
 * @param duration Duration of the media data.
 */
BLT_Result    BLT_MediaPacket_SetDuration(BLT_MediaPacket* packet,
                                          BLT_Time         duration);

/**
 * Sets to 1 some flags associated with this packet.
 * @param flags Flags to set. All the bits set to 1 in this parameter value
 * will set the corresponding flag in the packet to 1. All the bits set to zero 
 * will leave the corresponding packet flag unchanged.
 * @sa @link media_packet_flags Media Packet Flags
 */
BLT_Result BLT_MediaPacket_SetFlags(BLT_MediaPacket* packet, BLT_Flags flags);

/**
 * Clears (sets to 0) some flags associated with this packet.
 * @param flags Flags to clear. All the bits set to 1 in this parameter value
 * will clear (set to 0) the corresponding flag in the packet. 
 * All the bits set to zero will leave the corresponding packet flag unchanged.
 * @sa @link media_packet_flags Media Packet Flags
 */
BLT_Result BLT_MediaPacket_ClearFlags(BLT_MediaPacket* packet, 
                                      BLT_Flags        flags);

/**
 * Reset the flags associated with this packet. All the packet flags will be
 * set to 0.
 * @sa @link media_packet_flags Media Packet Flags
 */
BLT_Result BLT_MediaPacket_ResetFlags(BLT_MediaPacket* packet);

/**
 * Returns the flags associated with this packet.
 * @sa @link media_packet_flags Media Packet Flags
 */
BLT_Flags  BLT_MediaPacket_GetFlags(BLT_MediaPacket* packet);

#if defined(__cplusplus)
}
#endif

/** @} */

#endif /* _BLT_MEDIA_PACKET_H_ */
