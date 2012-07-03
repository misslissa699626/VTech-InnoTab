/*****************************************************************
|
|      Cdda: BltNeutrinoCddaDevice.c
|
|      Neutrino Cdda Device Module
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <sys/cdrom.h>
#include <sys/dcmd_cam.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltCddaDevice.h"
#include "BltErrors.h"
#include "BltDebug.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct {
    int                     fd;
    BLT_CddaTableOfContents toc;
} NeutrinoCddaDevice;

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoCddaDevice)
static const BLT_CddaDeviceInterface NeutrinoCddaDevice_BLT_CddaDeviceInterface;

/*----------------------------------------------------------------------
|       NeutrinoCddaDevice_ReadTableOfContents
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoCddaDevice_ReadTableOfContents(NeutrinoCddaDevice* device)
{
    int              io_result;
    cdrom_read_toc_t toc;
    BLT_Ordinal      track;

    /* get the toc header */
    io_result = devctl(device->fd,  
                       DCMD_CAM_CDROMREADTOC,
                       &toc, sizeof(toc), 
                       NULL);
    BLT_Debug("DCMD_CAM_CDROMREADTOC: return %d, errno=%s\n", io_result, strerror(errno));
    if (io_result != 0) return BLT_FAILURE;

    device->toc.first_track_index = toc.first_track;
    device->toc.last_track_index = toc.last_track;
    device->toc.track_count = 1 + toc.last_track - toc.first_track;

    BLT_Debug("NeutrinoCddaDevice_ReadTableOfContents: first=%d, last=%d\n",
              device->toc.first_track_index, device->toc.last_track_index);

    /* allocate memory for the track infos */
    device->toc.tracks = 
        (BLT_CddaTrackInfo*)ATX_AllocateMemory(sizeof(BLT_CddaTrackInfo) *
                                               device->toc.track_count);
    if (device->toc.tracks == NULL) return BLT_ERROR_OUT_OF_MEMORY;

    /* get info for each track */
    for (track = 0; track < device->toc.track_count; track++) {
        cdrom_tocentry_t* entry           = &toc.toc_entry[track];
        device->toc.tracks[track].index   = entry->track_number;
        device->toc.tracks[track].address = entry->addr.lba;
        device->toc.tracks[track].type    = 
            entry->control_adr & CDROM_DATA_TRACK ?
            BLT_CDDA_TRACK_TYPE_DATA : BLT_CDDA_TRACK_TYPE_AUDIO;

        /* compute the duration of the previous track */
        if (track != 0) {
            device->toc.tracks[track-1].duration.frames = 
                device->toc.tracks[track  ].address -
                device->toc.tracks[track-1].address;
            BLT_Cdda_FramesToMsf(device->toc.tracks[track-1].duration.frames, 
                                 &device->toc.tracks[track-1].duration.msf);
        }
    }

    /* get info for the leadout track to compute last track's duration */
    device->toc.tracks[device->toc.track_count-1].duration.frames = 
        toc.toc_entry[device->toc.track_count  ].addr.lba - 
        toc.toc_entry[device->toc.track_count-1].addr.lba;
    BLT_Cdda_FramesToMsf(
        device->toc.tracks[device->toc.track_count-1].duration.frames,
        &device->toc.tracks[device->toc.track_count-1].duration.msf);

    for (track = 0; track < device->toc.track_count; track++) {
        BLT_Debug("track %02d: (%c) addr = %08ld, "
                  "duration = %ld [%02d:%02d:%02d]\n",
                  device->toc.tracks[track].index,
                  device->toc.tracks[track].type == BLT_CDDA_TRACK_TYPE_AUDIO ?
                  'A' : 'D',
                  device->toc.tracks[track].address,
                  device->toc.tracks[track].duration.frames,
                  device->toc.tracks[track].duration.msf.m,
                  device->toc.tracks[track].duration.msf.s,
                  device->toc.tracks[track].duration.msf.f);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoCddaDevice_Create
+---------------------------------------------------------------------*/
static BLT_Result
NeutrinoCddaDevice_Create(BLT_String name, BLT_CddaDevice* object)
{
    NeutrinoCddaDevice* device;
    BLT_Result       result;

    BLT_COMPILER_UNUSED(name);

    /* allocate memory for the object */
    device = (NeutrinoCddaDevice*)ATX_AllocateZeroMemory(sizeof(NeutrinoCddaDevice));
    if (device == NULL) {
        ATX_CLEAR_OBJECT(object);
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* initialize the object */
    device->fd = open("/dev/cd0", O_RDONLY/* | O_NONBLOCK*/);
    if (device->fd < 0) {
        BLT_Debug("open return %d, errno=%s\n", device->fd, strerror(errno));
        if (errno == ENOENT) {
            result = BLT_ERROR_NO_SUCH_DEVICE;
        } else if (errno == EACCES) {
            result = BLT_ERROR_ACCESS_DENIED;
        } else {
            result = BLT_FAILURE;
        }
        goto failure;
    }

    /* read the toc */
    result = NeutrinoCddaDevice_ReadTableOfContents(device);
    if (BLT_FAILED(result)) goto failure;

    /* construct the object reference */
    ATX_INSTANCE(object) = (BLT_CddaDeviceInstance*)device;
    ATX_INTERFACE(object) = &NeutrinoCddaDevice_BLT_CddaDeviceInterface;

    return BLT_SUCCESS;

failure:
    if (device->fd >= 0) close(device->fd);
    ATX_FreeMemory((void*)device);
    ATX_CLEAR_OBJECT(object);
    return result;
}

/*----------------------------------------------------------------------
|       NeutrinoCddaDevice_Destroy
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoCddaDevice_Destroy(ATX_DestroyableInstance* instance)
{
    NeutrinoCddaDevice* device = (NeutrinoCddaDevice*)instance;

    /* close the device */
    if (device->fd != -1) {
        close(device->fd);
    }

    /* release the toc memory */
    if (device->toc.tracks != NULL) {
        ATX_FreeMemory(device->toc.tracks);
    }

    /* free the memory */
    ATX_FreeMemory((void*)device);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoCddaDevice_GetTrackInfo
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoCddaDevice_GetTrackInfo(BLT_CddaDeviceInstance* instance,  
                             BLT_Ordinal             index,
                             BLT_CddaTrackInfo*      info)
{
    NeutrinoCddaDevice* device = (NeutrinoCddaDevice*)instance;

    /* check that the track is within range */
    if (index < device->toc.first_track_index ||
        index > device->toc.last_track_index) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* return the info */
    *info = device->toc.tracks[index-device->toc.first_track_index];

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NeutrinoCddaDevice_GetTableOfContents
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoCddaDevice_GetTableOfContents(BLT_CddaDeviceInstance*   instance, 
                                   BLT_CddaTableOfContents** toc)
{
    NeutrinoCddaDevice* device = (NeutrinoCddaDevice*)instance;
    *toc = &device->toc;
    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|       NeutrinoCddaDevice_ReadFrames
+---------------------------------------------------------------------*/
BLT_METHOD
NeutrinoCddaDevice_ReadFrames(BLT_CddaDeviceInstance* instance,
                              BLT_CddaLba             addr,
                              BLT_Cardinal            count,
                              BLT_Any                 buffer)
{
    NeutrinoCddaDevice*    device = (NeutrinoCddaDevice*)instance;
    struct _cdrom_raw_read request;
    iov_t                  send_iov[1];
    iov_t                  recv_iov[1];
    int                    io_result;

    // setup the request
    request.lba      = addr;
    request.nsectors = count;
    request.mode     = CDDA;

    // setup the parameters
    SETIOV(&send_iov[0], &request, sizeof(request));
    SETIOV(&recv_iov[0], buffer, count * BLT_CDDA_FRAME_SIZE);
    
    // read the sectors
    io_result = devctlv(device->fd, 
                        DCMD_CAM_CDROMREAD, 
                        1, 1,
                        send_iov,
                        recv_iov,
                        NULL);

    if (io_result != 0) return BLT_FAILURE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_CddaDevice interface
+---------------------------------------------------------------------*/
static const BLT_CddaDeviceInterface
NeutrinoCddaDevice_BLT_CddaDeviceInterface = {
    NeutrinoCddaDevice_GetInterface,
    NeutrinoCddaDevice_GetTrackInfo,
    NeutrinoCddaDevice_GetTableOfContents,
    NeutrinoCddaDevice_ReadFrames
};

/*----------------------------------------------------------------------
|       ATX_Destroyable interface
+---------------------------------------------------------------------*/
ATX_DEFINE_SIMPLE_DESTROYABLE_INTERFACE(NeutrinoCddaDevice)

/*----------------------------------------------------------------------
|       interface map
+---------------------------------------------------------------------*/
ATX_BEGIN_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoCddaDevice)
ATX_INTERFACE_MAP_ADD(NeutrinoCddaDevice, BLT_CddaDevice)
ATX_INTERFACE_MAP_ADD(NeutrinoCddaDevice, ATX_Destroyable)
ATX_END_SIMPLE_GET_INTERFACE_IMPLEMENTATION(NeutrinoCddaDevice)

/*----------------------------------------------------------------------
|       BLT_CddaDevice_Create
+---------------------------------------------------------------------*/
BLT_Result 
BLT_CddaDevice_Create(BLT_String name, BLT_CddaDevice* device)
{
    return NeutrinoCddaDevice_Create(name, device);
}


#/** PhEDIT attribute block
#-11:16777215
#0:10558:monaco09:-3:-3:0
#**  PhEDIT attribute block ends (-0000115)**/
