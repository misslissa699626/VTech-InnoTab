/*****************************************************************
|
|   BlueTune - MediaPacket Objects
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune MediaPacket Objects
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltCore.h"
#include "BltMedia.h"
#include "BltMediaPacketPriv.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
struct BLT_MediaPacket {
    BLT_Cardinal   reference_count;
    BLT_MediaType* type;
    BLT_Size       allocated_size;
    BLT_Size       payload_size;
    BLT_Offset     payload_offset;
    BLT_Any        payload;
    BLT_Flags      flags;
    BLT_TimeStamp  time_stamp;
    BLT_Time       duration;
};

/*----------------------------------------------------------------------
|    BLT_MediaPacket_Create
+---------------------------------------------------------------------*/
BLT_Result
BLT_MediaPacket_Create(BLT_Size             size,
                       const BLT_MediaType* type,
                       BLT_MediaPacket**    packet)
{
    /* allocate memory for the packet object */
    *packet = (BLT_MediaPacket*)ATX_AllocateZeroMemory(sizeof(BLT_MediaPacket));
    if (*packet == NULL) {
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* allocate the payload buffer */
    if (size) {
        (*packet)->payload = ATX_AllocateMemory(size);
        if ((*packet)->payload == NULL) {
            ATX_FreeMemory(*packet);
            *packet = NULL;
            return BLT_ERROR_OUT_OF_MEMORY;
        }
    } else {
        (*packet)->payload = NULL;
    }

    /* initialize the non-zero fields */
    (*packet)->reference_count = 1;
    (*packet)->allocated_size  = size;

    /* set the media type */
    if (type) {
        BLT_MediaType_Clone(type, &(*packet)->type);
    } else {
        BLT_MediaType_Clone(&BLT_MediaType_None, &(*packet)->type);;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
BLT_MediaPacket_Destroy(BLT_MediaPacket* packet)
{
    /* free the packet payload */
    if (packet->payload) {
        ATX_FreeMemory(packet->payload);
    }

    /* free the media type extensions if any */
    BLT_MediaType_Free(packet->type);

    ATX_FreeMemory((void*)packet);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_Release
+---------------------------------------------------------------------*/
BLT_Result
BLT_MediaPacket_Release(BLT_MediaPacket* packet)
{
    /*BLT_Debug("MediaPacket [%x] - release (ref = %d)\n", 
      (int)packet, packet->reference_count);*/
    if (--packet->reference_count == 0) {
        return BLT_MediaPacket_Destroy(packet);
    } else {
        return BLT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_AddReference
+---------------------------------------------------------------------*/
BLT_Result
BLT_MediaPacket_AddReference(BLT_MediaPacket* packet)
{
    /*BLT_Debug("MediaPacket [%x] - reference (ref = %d)\n", 
      (int)packet, packet->reference_count);*/
    packet->reference_count++;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetPayloadBuffer
+---------------------------------------------------------------------*/
BLT_Any    
BLT_MediaPacket_GetPayloadBuffer(BLT_MediaPacket* packet)
{
    return (BLT_Any)(((char*)packet->payload) + packet->payload_offset);
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetPayloadWindow
+---------------------------------------------------------------------*/
BLT_Result   
BLT_MediaPacket_SetPayloadWindow(BLT_MediaPacket* packet,
                                 BLT_Offset       offset,
                                 BLT_Size         size)
{
    /* check that the window is in the current range */
    if (offset+size > packet->allocated_size) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* use the new offset and size */
    packet->payload_offset = offset;
    packet->payload_size = size;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetPayloadSize
+---------------------------------------------------------------------*/
BLT_Size   
BLT_MediaPacket_GetPayloadSize(BLT_MediaPacket* packet)
{
    return packet->payload_size;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetPayloadSize
+---------------------------------------------------------------------*/
BLT_Result
BLT_MediaPacket_SetPayloadSize(BLT_MediaPacket* packet, BLT_Size size)
{
    BLT_Size space_needed = size + packet->payload_offset;

    if (space_needed > packet->allocated_size) {
        /* it does not fit, reallocate */
        BLT_Result result;
        result = BLT_MediaPacket_SetAllocatedSize(packet, space_needed);
        if (BLT_FAILED(result)) return result;
    }

    /* use the new size */
    packet->payload_size = size;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetAllocatedSize
+---------------------------------------------------------------------*/
BLT_Size
BLT_MediaPacket_GetAllocatedSize(BLT_MediaPacket* packet)
{
    return packet->allocated_size;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetAllocatedSize
+---------------------------------------------------------------------*/
BLT_Result
BLT_MediaPacket_SetAllocatedSize(BLT_MediaPacket* packet, BLT_Size size)
{
    if (packet->allocated_size >= size) {
        /* it fits */
        return BLT_SUCCESS;
    } else {
        /* it does not fit, we need to reallocate */
        BLT_Any new_buffer;

        /* allocate a new buffer */
        new_buffer = ATX_AllocateMemory(size);
        if (new_buffer == NULL) {
            return BLT_ERROR_OUT_OF_MEMORY;
        }
        
        /* copy the memory from the previous buffer */
        if (packet->payload_size + packet->payload_offset && 
            packet->payload != NULL) {
            ATX_CopyMemory(new_buffer, packet->payload, 
                           packet->payload_size + packet->payload_offset);
        }

        /* free the previous buffer, if any */
        if (packet->payload) {
            ATX_FreeMemory(packet->payload);
        }
        
        /* use the new buffer */
        packet->payload        = new_buffer;
        packet->allocated_size = size;

        return BLT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetPayloadOffset
+---------------------------------------------------------------------*/
BLT_Offset   
BLT_MediaPacket_GetPayloadOffset(BLT_MediaPacket* packet)
{
    return packet->payload_offset;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetPayloadOffset
+---------------------------------------------------------------------*/
BLT_Result
BLT_MediaPacket_SetPayloadOffset(BLT_MediaPacket* packet, BLT_Offset offset)
{
    /* check that the offset is within bounds */
    if ((unsigned int)offset > packet->payload_offset+packet->payload_size) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* recompute the size */
    packet->payload_size -= offset - packet->payload_offset;

    /* store the offset */
    packet->payload_offset = offset;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetMediaType
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_GetMediaType(BLT_MediaPacket* packet, const BLT_MediaType** type)
{
    if (packet->type) {
        *type = packet->type;
    } else {
        *type = &BLT_MediaType_None;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetMediaType
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_SetMediaType(BLT_MediaPacket*     packet, 
                             const BLT_MediaType* type)
{
	if (packet->type != NULL && 
	    type != NULL &&
	    packet->type->extension_size >= type->extension_size) { 
		/* we have enough space for the type, just copy it */
		ATX_CopyMemory(packet->type, type, sizeof(*type)+type->extension_size);
		return BLT_SUCCESS;
	} else {
		/* replace the type with this new one */
	    BLT_MediaType_Free(packet->type);
    	return BLT_MediaType_Clone(type, &packet->type);
	}
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetTimeStamp
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_SetTimeStamp(BLT_MediaPacket* packet, BLT_TimeStamp time_stamp)
{
    packet->time_stamp = time_stamp;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetTimeStamp
+---------------------------------------------------------------------*/
BLT_TimeStamp 
BLT_MediaPacket_GetTimeStamp(BLT_MediaPacket* packet)
{
    return packet->time_stamp;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetDuration
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_SetDuration(BLT_MediaPacket* packet, BLT_Time duration)
{
    packet->duration = duration;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetDuration
+---------------------------------------------------------------------*/
BLT_Time
BLT_MediaPacket_GetDuration(BLT_MediaPacket* packet)
{
    return packet->duration;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_SetFlags
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_SetFlags(BLT_MediaPacket* packet, BLT_Flags flags)
{
    packet->flags |= flags;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_ClearFlags
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_ClearFlags(BLT_MediaPacket* packet, BLT_Flags flags)
{
    packet->flags ^= flags;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_ResetFlags
+---------------------------------------------------------------------*/
BLT_Result 
BLT_MediaPacket_ResetFlags(BLT_MediaPacket* packet)
{
    packet->flags = 0;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_MediaPacket_GetFlags
+---------------------------------------------------------------------*/
BLT_Flags  
BLT_MediaPacket_GetFlags(BLT_MediaPacket* packet)
{
    return packet->flags;
}
