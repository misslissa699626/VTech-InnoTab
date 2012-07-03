/*****************************************************************
|
|      CDDA Device Runtime Support
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltCddaDevice.h"

/*----------------------------------------------------------------------
|   logginf
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.cdda")

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define BLT_CDDA_BURST_SIZE 4

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
struct BLT_CddaTrack {
    /* interfaces */
    ATX_IMPLEMENTS(ATX_Referenceable);
    ATX_IMPLEMENTS(ATX_InputStream);
    
    /* members */
    BLT_Cardinal      reference_count;
    BLT_CddaDevice*   device;
    BLT_CddaTrackInfo info;
    ATX_Position      position;
    BLT_Size          size;
    struct {
        ATX_Position  position;
        BLT_Size      size;
        unsigned char data[BLT_CDDA_BURST_SIZE*BLT_CDDA_FRAME_SIZE];
    }                 cache;
};

/*----------------------------------------------------------------------
|       interface constants
+---------------------------------------------------------------------*/
const ATX_InterfaceId ATX_INTERFACE_ID__BLT_CddaDevice = {0x0201, 0x0001};

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(BLT_CddaTrack, ATX_InputStream)
ATX_DECLARE_INTERFACE_MAP(BLT_CddaTrack, ATX_Referenceable)

/*----------------------------------------------------------------------
|       BLT_Cdda_FramesToMsf
+---------------------------------------------------------------------*/
void
BLT_Cdda_FramesToMsf(BLT_CddaLba frames, BLT_CddaMsf* msf)
{
    msf->f = frames % 75;
    frames = (frames - msf->f)/75;
    msf->s = frames % 60;
    msf->m = (frames - msf->s)/60;
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_Destroy
+---------------------------------------------------------------------*/
static BLT_Result 
BLT_CddaTrack_Destroy(BLT_CddaTrack* self)
{
    ATX_LOG_FINE("BLT_CddaTrack::Destroy");
    
    ATX_FreeMemory((void*)self);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_Create
+---------------------------------------------------------------------*/
BLT_Result
BLT_CddaTrack_Create(BLT_CddaDevice*   device,
                     BLT_Ordinal       index,
                     ATX_InputStream** track)
{
    BLT_CddaTrack* self;
    BLT_Result     result;

    ATX_LOG_FINE_1("BLT_CddaTrack::Create - index=%d", index);
    
    /* allocate memory for the object */
    self = (BLT_CddaTrack*)ATX_AllocateZeroMemory(sizeof(BLT_CddaTrack));
    if (self == NULL) {
        *track = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* initialize the object */
    self->reference_count = 1;
    self->device = device;

    /* get the track info */
    result = BLT_CddaDevice_GetTrackInfo(device, index, &self->info);
    if (BLT_FAILED(result)) goto failure;

    /* compute track size */
    self->size = self->info.duration.frames * BLT_CDDA_FRAME_SIZE;
    
    /* construct reference */
    ATX_SET_INTERFACE(self, BLT_CddaTrack, ATX_Referenceable);
    ATX_SET_INTERFACE(self, BLT_CddaTrack, ATX_InputStream);
    *track = &ATX_BASE(self, ATX_InputStream);
    
    return BLT_SUCCESS;

 failure:
    BLT_CddaTrack_Destroy(self);
    *track = NULL;
    return result;
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_Read
+---------------------------------------------------------------------*/
ATX_METHOD
BLT_CddaTrack_Read(ATX_InputStream* _self, 
                   ATX_Any          buffer,
                   ATX_Size         bytes_to_read,
                   ATX_Size*        bytes_read)
{
    BLT_CddaTrack* self = ATX_SELF(BLT_CddaTrack, ATX_InputStream);
    BLT_Size       bytes_left_to_read;
    unsigned char* out = buffer;
    
    /* truncate to the max size we can read */
    if (self->position + bytes_to_read > self->size) {
        bytes_to_read = self->size - self->position;
    }

    /* default return values */
    if (bytes_read) *bytes_read = 0;

    /* if there is nothing to read, we've reached the end */
    if (bytes_to_read == 0) {
        return BLT_ERROR_EOS;
    }

    /* read all bytes through the cache */
    bytes_left_to_read = bytes_to_read;
    while (bytes_left_to_read != 0) {
        BLT_Size in_cache;
        if (self->position >= self->cache.position &&
            (BLT_Size)self->position < 
            self->cache.position + self->cache.size) {
            /* there is cached data available */
            in_cache = self->cache.position + self->cache.size - self->position;
            if (in_cache > bytes_left_to_read) {
                in_cache = bytes_left_to_read;
            }

            /* copy data from the cache */
            ATX_CopyMemory(out, 
                           &self->cache.data
                           [self->position-self->cache.position],
                           in_cache);
            out                += in_cache;
            self->position    += in_cache;
            bytes_left_to_read -= in_cache;
            /*BLT_Debug(">>> got %d bytes from cache (%d to read)\n", 
              in_cache, bytes_left_to_read);*/
        } else {
            /* refill the cache */
            BLT_Offset   frame_position;
            BLT_CddaLba  frame_addr;
            BLT_Cardinal burst = BLT_CDDA_BURST_SIZE;
            BLT_Result   result;

            /* compute frame addr and position */
            frame_position = self->position / BLT_CDDA_FRAME_SIZE;
            frame_addr     = self->info.address + frame_position;

            /* make sure that the burst is not too big */
            if (burst > self->info.duration.frames - frame_position) {
                burst = self->info.duration.frames - frame_position;
            }

            /* read the frames from the device */
            result = BLT_CddaDevice_ReadFrames(self->device,
                                               frame_addr,
                                               burst,
                                               self->cache.data);
                                               
            if (BLT_FAILED(result)) return result;

            /* update counters */
            self->cache.position = frame_position * BLT_CDDA_FRAME_SIZE;
            self->cache.size   = burst * BLT_CDDA_FRAME_SIZE;
        }
    }

    /* return the number of bytes read */
    if (bytes_read) *bytes_read = bytes_to_read;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_Seek
+---------------------------------------------------------------------*/
ATX_METHOD
BLT_CddaTrack_Seek(ATX_InputStream* _self, ATX_Position position)
{
    BLT_CddaTrack* self = ATX_SELF(BLT_CddaTrack, ATX_InputStream);

    /* align position to 4 bytes */
    position -= position%4;

    /* update the position */
    if ((BLT_Size)position <= self->size) {
        self->position = position;
        return BLT_SUCCESS;
    } else {
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_Tell
+---------------------------------------------------------------------*/
ATX_METHOD
BLT_CddaTrack_Tell(ATX_InputStream* _self, ATX_Position* position)
{
    BLT_CddaTrack* self = ATX_SELF(BLT_CddaTrack, ATX_InputStream);
    *position = self->position;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_GetSize
+---------------------------------------------------------------------*/
ATX_METHOD
BLT_CddaTrack_GetSize(ATX_InputStream* _self, ATX_LargeSize* size)
{
    BLT_CddaTrack* self = ATX_SELF(BLT_CddaTrack, ATX_InputStream);
    *size = self->size;
    return BLT_SUCCESS;    
}

/*----------------------------------------------------------------------
|       BLT_CddaTrack_GetAvailable
+---------------------------------------------------------------------*/
ATX_METHOD
BLT_CddaTrack_GetAvailable(ATX_InputStream* _self,
                           ATX_LargeSize*   available)
{
    BLT_CddaTrack* self = ATX_SELF(BLT_CddaTrack, ATX_InputStream);
    *available = self->size - self->position;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(BLT_CddaTrack)
    ATX_GET_INTERFACE_ACCEPT(BLT_CddaTrack, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT(BLT_CddaTrack, ATX_InputStream)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|       ATX_InputStream interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLT_CddaTrack, ATX_InputStream)
    BLT_CddaTrack_Read,
    BLT_CddaTrack_Seek,
    BLT_CddaTrack_Tell,
    BLT_CddaTrack_GetSize,
    BLT_CddaTrack_GetAvailable
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE(BLT_CddaTrack, reference_count)
