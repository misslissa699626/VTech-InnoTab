/*****************************************************************
|
|   BlueTune - Network Stream
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/**
 * This module implements a buffered stream cache that allows some
 * amount of seeking backward and forward in a slow-to-seek source
 * stream such as a network stream.
 * The cache will try to limit the forward-filling of the buffer to 
 * a small amount to keep as much backward-seek buffer as possible.
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltNetworkStream.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_InputStream);
    ATX_IMPLEMENTS(ATX_Properties);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    ATX_Cardinal     reference_count;
    ATX_InputStream* source;
    ATX_Properties*  source_properties;
    ATX_RingBuffer*  buffer;
    ATX_Size         buffer_size;
    ATX_Size         back_store;
    ATX_Position     position;
    ATX_Boolean      eos;
    ATX_Result       eos_cause;
    ATX_Size         seek_as_read_threshold;
} BLT_NetworkStream;

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.network.stream")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
/**
 * Threshold below which a seek forward is implemented by reading from
 * the source instead of seeking in the source.
 */
#define BLT_NETWORK_STREAM_DEFAULT_SEEK_AS_READ_THRESHOLD 0     /* when seek is normal */ 
#define BLT_NETWORK_STREAM_SLOW_SEEK_AS_READ_THRESHOLD    32768 /* when seek is slow   */

/**
 * Try to keep this amount of data in the back store 
 */
#define BLT_NETWORK_STREAM_MIN_BACK_STORE 4096

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(BLT_NetworkStream, ATX_InputStream)
ATX_DECLARE_INTERFACE_MAP(BLT_NetworkStream, ATX_Properties)
ATX_DECLARE_INTERFACE_MAP(BLT_NetworkStream, ATX_Referenceable)

/*----------------------------------------------------------------------
|   BLT_NetworkStream_Create
+---------------------------------------------------------------------*/
BLT_Result 
BLT_NetworkStream_Create(BLT_Size          buffer_size,
                         ATX_InputStream*  source, 
                         ATX_InputStream** stream)
{
    ATX_Result result;

    /* allocate the object */
    BLT_NetworkStream* self = (BLT_NetworkStream*)ATX_AllocateZeroMemory(sizeof(BLT_NetworkStream));
    if (self == NULL) {
        *stream = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the object */
    self->reference_count = 1;
    result = ATX_RingBuffer_Create(buffer_size, &self->buffer);
    if (ATX_FAILED(result)) {
        *stream = NULL;
        return result;
    }
    self->buffer_size = buffer_size;
    self->eos_cause = ATX_ERROR_EOS;
    self->source = source;
    ATX_REFERENCE_OBJECT(source);
    
    /* get the properties interface of the source */
    self->source_properties = ATX_CAST(source, ATX_Properties);
    
    /* determine when we should read data instead issuing a seek when */
    /* the target position is close enough                            */
    self->seek_as_read_threshold = BLT_NETWORK_STREAM_SLOW_SEEK_AS_READ_THRESHOLD;
    
    /* setup the interfaces */
    ATX_SET_INTERFACE(self, BLT_NetworkStream, ATX_InputStream);
    ATX_SET_INTERFACE(self, BLT_NetworkStream, ATX_Properties);
    ATX_SET_INTERFACE(self, BLT_NetworkStream, ATX_Referenceable);
    *stream = &ATX_BASE(self, ATX_InputStream);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
BLT_NetworkStream_Destroy(BLT_NetworkStream* self)
{
    ATX_RELEASE_OBJECT(self->source);
    ATX_RingBuffer_Destroy(self->buffer);
    ATX_FreeMemory(self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_ClampBackStore
+---------------------------------------------------------------------*/
static void
BLT_NetworkStream_ClampBackStore(BLT_NetworkStream* self)
{
    /** clamp the back store to ensure it is not bigger than the max
     * possible value */
    ATX_Size max_back = ATX_RingBuffer_GetSpace(self->buffer);
    if (self->back_store > max_back) self->back_store = max_back;
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_GetProperty
+---------------------------------------------------------------------*/
ATX_METHOD
BLT_NetworkStream_GetProperty(ATX_Properties*    _self, 
                              const char*        name,
                              ATX_PropertyValue* value)
{
    BLT_NetworkStream* self = ATX_SELF(BLT_NetworkStream, ATX_Properties);
    
    if (self->source_properties && name != NULL &&
        ATX_StringsEqual(name, ATX_INPUT_STREAM_PROPERTY_SEEK_SPEED)) {        
        /* ask the source if it has the seek speed property */
        if (ATX_FAILED(ATX_Properties_GetProperty(self->source_properties, name, value))) {
            /* the source does not have the property, use a default value */
            value->type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
            value->data.integer = ATX_INPUT_STREAM_SEEK_SPEED_SLOW;
        }
        return ATX_SUCCESS;
    } else {
        return ATX_ERROR_NO_SUCH_PROPERTY;
    }
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_Read
+---------------------------------------------------------------------*/
static ATX_Result
BLT_NetworkStream_Read(ATX_InputStream* _self, 
                       void*            buffer,
                       ATX_Size         bytes_to_read,
                       ATX_Size*        bytes_read)
{
    BLT_NetworkStream* self = ATX_SELF(BLT_NetworkStream, ATX_InputStream);
    ATX_Size           buffered = ATX_RingBuffer_GetAvailable(self->buffer);
    ATX_Size           chunk;
    ATX_Size           bytes_read_storage = 0;

    /* default */
    if (bytes_read) {
        *bytes_read = 0;
    } else {
        bytes_read = &bytes_read_storage;
    }

    /* shortcut */
    if (bytes_to_read == 0) return ATX_SUCCESS;

    /* use all we can from the buffer */
    ATX_LOG_FINER_1("buffer available=%d", buffered);
    chunk = buffered > bytes_to_read ? bytes_to_read : buffered;
    if (chunk) {
        ATX_RingBuffer_Read(self->buffer, buffer, chunk);
        bytes_to_read -= chunk;
        *bytes_read += chunk;
        self->position += chunk;
        buffer = (void*)((char*)buffer+chunk);
        self->back_store += chunk;
        BLT_NetworkStream_ClampBackStore(self);
    }

    /* read what we can from the source */
    while (bytes_to_read && !self->eos) {
        ATX_Size       read_from_source = 0;
        ATX_Size       can_write = ATX_RingBuffer_GetSpace(self->buffer);
        ATX_Size       should_read = ATX_RingBuffer_GetContiguousSpace(self->buffer);
        unsigned char* in = ATX_RingBuffer_GetIn(self->buffer);
        ATX_Result     result;

        /* compute how much to read from the source */
        if (should_read > BLT_NETWORK_STREAM_MIN_BACK_STORE &&
            should_read - BLT_NETWORK_STREAM_MIN_BACK_STORE >= bytes_to_read) {
            /* leave some data in the back store */
            should_read -= BLT_NETWORK_STREAM_MIN_BACK_STORE;
        }

        /* read from the source */
        result = ATX_InputStream_Read(self->source, 
                                      in, 
                                      should_read, 
                                      &read_from_source);
        if (ATX_SUCCEEDED(result)) {
            ATX_LOG_FINER_1("read %d bytes from source", read_from_source);
            
            /* adjust the ring buffer */
            ATX_RingBuffer_MoveIn(self->buffer, read_from_source);

            /* transfer some of what was read */
            chunk = (bytes_to_read <= read_from_source)?bytes_to_read:read_from_source;
            ATX_RingBuffer_Read(self->buffer, buffer, chunk);

            /* adjust counters and pointers */
            *bytes_read += chunk;
            bytes_to_read -= chunk;
            self->position += chunk;
            buffer = (void*)((char*)buffer+chunk);

            /* compute how much back-store we have now */
            if (can_write-read_from_source < self->back_store) {
                /* we have reduced (written over) the back store */
                self->back_store = can_write-read_from_source;
            }
            self->back_store += chunk;
            BLT_NetworkStream_ClampBackStore(self);

        } else if (result == ATX_ERROR_EOS) {
            /* we can't continue further */
            ATX_LOG_FINE("reached EOS");
            self->eos = ATX_TRUE;
            self->eos_cause = ATX_ERROR_EOS;
            break;
        } else {
            ATX_LOG_FINE_2("read from source failed: %d (%S)", result, BLT_ResultText(result));
            return (*bytes_read == 0) ? result : ATX_SUCCESS;
        }

        /* don't loop if this was a short read */
        if (read_from_source != should_read) break;
    }

    if (self->eos && *bytes_read == 0) {
        return self->eos_cause;
    } else {
        return ATX_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_Seek
+---------------------------------------------------------------------*/
static ATX_Result 
BLT_NetworkStream_Seek(ATX_InputStream* _self, ATX_Position position)
{
    BLT_NetworkStream* self = ATX_SELF(BLT_NetworkStream, ATX_InputStream);
    ATX_Int64          move = position-self->position;
    ATX_Result         result;

    /* shortcut */
    if (move == 0) {
        if (position == 0) {
            /* force a call to the source, because some callers will  */
            /* use this to determine if the source is seekable or not */
            ATX_Position current = 0;
            result = ATX_InputStream_Tell(self->source, &current);
            if (ATX_FAILED(result)) return result;
            return ATX_InputStream_Seek(self->source, current);
        }
        return ATX_SUCCESS;
    }
    
    ATX_LOG_FINER_3("move by %ld, back_store=%ld, forward_store=%ld",
                    (long)move, (long)self->back_store, (long)ATX_RingBuffer_GetAvailable(self->buffer));
                  
    /* see if we can seek entirely within our buffer */
    if ((move < 0 && -move <= (ATX_Int64)self->back_store) ||
        (move > 0 && move < (ATX_Int64)ATX_RingBuffer_GetAvailable(self->buffer))) {
        ATX_RingBuffer_MoveOut(self->buffer, (ATX_Size)move);
        self->position = position;
        self->eos = ATX_FALSE;

        /* adjust the back-store counter */
        self->back_store += (ATX_Size)move;

        return ATX_SUCCESS;
    }

    /* we're seeking outside the buffered zone */
    self->eos = ATX_FALSE;
    self->eos_cause = ATX_ERROR_EOS;
    if (move > 0 && (unsigned int)move <= self->seek_as_read_threshold) {
        /* simulate a seek by reading data up to the position */
        char buffer[256];
        ATX_LOG_FINE_1("performing seek of %d as a read", move);
        while (move) {
            unsigned int chunk = ((unsigned int)move) > sizeof(buffer)?sizeof(buffer):(unsigned int)move;
            ATX_Size     bytes_read;
            result = BLT_NetworkStream_Read(_self, buffer, chunk, &bytes_read);
            if (ATX_FAILED(result)) return result;
            if (bytes_read != chunk) return ATX_ERROR_EOS;
            move -= chunk;
        }
    } else {
        /* perform a real seek in the source */
        ATX_LOG_FINE_2("performing seek of %ld as input seek(%ld)", move, (long)position);
        result = ATX_InputStream_Seek(self->source, position);
        if (ATX_FAILED(result)) return result;
        ATX_RingBuffer_Reset(self->buffer);
        self->back_store = 0;
        self->position = position;
    }
    
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_Tell
+---------------------------------------------------------------------*/
static ATX_Result 
BLT_NetworkStream_Tell(ATX_InputStream* _self, ATX_Position* offset)
{
    BLT_NetworkStream* self = ATX_SELF(BLT_NetworkStream, ATX_InputStream);
    *offset = self->position;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_GetSize
+---------------------------------------------------------------------*/
static ATX_Result 
BLT_NetworkStream_GetSize(ATX_InputStream* _self, ATX_LargeSize* size)
{
    BLT_NetworkStream* self = ATX_SELF(BLT_NetworkStream, ATX_InputStream);
    return ATX_InputStream_GetSize(self->source, size);
}

/*----------------------------------------------------------------------
|   BLT_NetworkStream_GetAvailable
+---------------------------------------------------------------------*/
static ATX_Result 
BLT_NetworkStream_GetAvailable(ATX_InputStream* _self, ATX_LargeSize* available)
{
    BLT_NetworkStream* self = ATX_SELF(BLT_NetworkStream, ATX_InputStream);
    ATX_LargeSize available_from_source = 0;
    ATX_InputStream_GetAvailable(self->source, &available_from_source);
    *available = available_from_source+ATX_RingBuffer_GetAvailable(self->buffer);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(BLT_NetworkStream)
    ATX_GET_INTERFACE_ACCEPT(BLT_NetworkStream, ATX_InputStream)
    ATX_GET_INTERFACE_ACCEPT(BLT_NetworkStream, ATX_Properties)
    ATX_GET_INTERFACE_ACCEPT(BLT_NetworkStream, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|       ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLT_NetworkStream, ATX_InputStream)
    BLT_NetworkStream_Read,
    BLT_NetworkStream_Seek,
    BLT_NetworkStream_Tell,
    BLT_NetworkStream_GetSize,
    BLT_NetworkStream_GetAvailable
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|       ATX_Properties interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_STATIC_PROPERTIES_INTERFACE(BLT_NetworkStream)

/*----------------------------------------------------------------------
|       ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(BLT_NetworkStream, reference_count)

