/*****************************************************************
|
|      Cdda: BltLinuxCddaDevice.c
|
|      Linux Cdda Device Module
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <sys/types.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltCddaDevice.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.cdda.linux")

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_CddaDevice);
    ATX_IMPLEMENTS(ATX_Destroyable);
    
    /* members */
    int                     fd;
    BLT_CddaTableOfContents toc;
} LinuxCddaDevice;

/*----------------------------------------------------------------------
|       forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(LinuxCddaDevice, BLT_CddaDevice)
ATX_DECLARE_INTERFACE_MAP(LinuxCddaDevice, ATX_Destroyable)

/*----------------------------------------------------------------------
|       LinuxCddaDevice_ReadTableOfContents
+---------------------------------------------------------------------*/
static BLT_Result
LinuxCddaDevice_ReadTableOfContents(LinuxCddaDevice* self)
{
    int                   io_result;
    struct cdrom_tochdr   toc_header;
    struct cdrom_tocentry toc_entry;
    BLT_CddaTrackInfo*    track_info;
    BLT_Ordinal           index;

    /* get the toc header */
    io_result = ioctl(self->fd, CDROMREADTOCHDR, &toc_header);
    ATX_LOG_WARNING_2("LinuxCddaDevice::ReadTableOfContents - CDROMREADTOCHDR return %d, errno=%s", io_result, strerror(errno));
    if (io_result != 0) return BLT_FAILURE;
    self->toc.first_track_index = toc_header.cdth_trk0;
    self->toc.last_track_index = toc_header.cdth_trk1;
    self->toc.track_count = 1 + toc_header.cdth_trk1 - toc_header.cdth_trk0;

    ATX_LOG_FINE_2("LinuxCddaDevice::ReadTableOfContents - first=%d, last=%d",
                   self->toc.first_track_index, self->toc.last_track_index);

    /* allocate memory for the track infos */
    self->toc.tracks = 
        (BLT_CddaTrackInfo*)ATX_AllocateZeroMemory(sizeof(BLT_CddaTrackInfo) *
                                                   self->toc.track_count);
    if (self->toc.tracks == NULL) return BLT_ERROR_OUT_OF_MEMORY;

    /* get info for each track */
    track_info = self->toc.tracks;
    for (index  = self->toc.first_track_index; 
         index <= self->toc.last_track_index;
         index++, track_info++) {
        ATX_SetMemory(&toc_entry, 0, sizeof(toc_entry));
        toc_entry.cdte_track  = index;
        toc_entry.cdte_format = CDROM_LBA; 
        io_result = ioctl(self->fd, CDROMREADTOCENTRY, &toc_entry);
        if (io_result != 0) return BLT_FAILURE;
        track_info->index = index;
        track_info->address = toc_entry.cdte_addr.lba;
        track_info->type = toc_entry.cdte_ctrl & CDROM_DATA_TRACK ?
            BLT_CDDA_TRACK_TYPE_DATA : BLT_CDDA_TRACK_TYPE_AUDIO;

        /* compute the duration of the previous track */
        if (index != self->toc.first_track_index) {
            track_info[-1].duration.frames = 
                track_info[0].address - track_info[-1].address;
            BLT_Cdda_FramesToMsf(track_info[-1].duration.frames, 
                                 &track_info[-1].duration.msf);
        }
    }

    /* get info for the leadout track to compute last track's duration */
    toc_entry.cdte_track  = CDROM_LEADOUT;
    toc_entry.cdte_format = CDROM_LBA; 
    io_result = ioctl(self->fd, CDROMREADTOCENTRY, &toc_entry);
    if (io_result != 0) return BLT_FAILURE;
    track_info[-1].duration.frames = 
        toc_entry.cdte_addr.lba - track_info[-1].address;
    BLT_Cdda_FramesToMsf(track_info[-1].duration.frames,
                         &track_info[-1].duration.msf);

    track_info = self->toc.tracks;
    for (index  = self->toc.first_track_index; 
         index <= self->toc.last_track_index;
         index++, track_info++) {
        ATX_LOG_FINE_7(
            "LinuxCddaDevice::ReadTableOfContents - track %02d: (%c) addr = %08ld, duration = %08ld [%02d:%02d:%02d]",
            track_info->index,
            track_info->type == BLT_CDDA_TRACK_TYPE_AUDIO ? 'A' : 'D',
            track_info->address,
            track_info->duration.frames,
            track_info->duration.msf.m,
            track_info->duration.msf.s,
            track_info->duration.msf.f);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       LinuxCddaDevice_Create
+---------------------------------------------------------------------*/
static BLT_Result
LinuxCddaDevice_Create(BLT_CString name, BLT_CddaDevice** object)
{
    LinuxCddaDevice* device;
    BLT_Result       result;

    BLT_COMPILER_UNUSED(name);

    /* allocate memory for the object */
    device = (LinuxCddaDevice*)ATX_AllocateZeroMemory(sizeof(LinuxCddaDevice));
    if (device == NULL) {
        object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* initialize the object */
    device->fd = open("/dev/cdrom", O_RDONLY | O_NONBLOCK);
    if (device->fd < 0) {
        ATX_LOG_FINER_2("LinuxCddaDevice::Create - open return %d, errno=%s", device->fd, strerror(errno));
        if (errno == ENOENT) {
            result = BLT_ERROR_NO_SUCH_DEVICE;
        } else if (errno == EACCES) {
            result = BLT_ERROR_ACCESS_DENIED;
        } else if (errno == ENOMEDIUM) {
            result = BLT_ERROR_NO_MEDIUM;
        } else {
            result = BLT_FAILURE;
        }
        goto failure;
    }

    /* read the toc */
    result = LinuxCddaDevice_ReadTableOfContents(device);
    if (BLT_FAILED(result)) goto failure;

    /* setup the interfaces */
    ATX_SET_INTERFACE(device, LinuxCddaDevice, BLT_CddaDevice);
    ATX_SET_INTERFACE(device, LinuxCddaDevice, ATX_Destroyable);
    *object = &ATX_BASE(device, BLT_CddaDevice);

    return BLT_SUCCESS;

failure:
    if (device->fd >= 0) close(device->fd);
    ATX_FreeMemory((void*)device);
    *object = NULL;
    return result;
}

/*----------------------------------------------------------------------
|       LinuxCddaDevice_Destroy
+---------------------------------------------------------------------*/
BLT_METHOD
LinuxCddaDevice_Destroy(ATX_Destroyable* _self)
{
    LinuxCddaDevice* self = ATX_SELF(LinuxCddaDevice, ATX_Destroyable);

    /* close the device */
    if (self->fd != -1) {
        close(self->fd);
    }

    /* release the toc memory */
    if (self->toc.tracks != NULL) {
        ATX_FreeMemory(self->toc.tracks);
    }

    /* free the memory */
    ATX_FreeMemory((void*)self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       LinuxCddaDevice_GetTrackInfo
+---------------------------------------------------------------------*/
BLT_METHOD
LinuxCddaDevice_GetTrackInfo(BLT_CddaDevice*    _self,  
                             BLT_Ordinal        index,
                             BLT_CddaTrackInfo* info)
{
    LinuxCddaDevice* self = ATX_SELF(LinuxCddaDevice, BLT_CddaDevice);

    /* check that the track is within range */
    if (index < self->toc.first_track_index ||
        index > self->toc.last_track_index) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* return the info */
    *info = self->toc.tracks[index-self->toc.first_track_index];

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|       LinuxCddaDevice_GetTableOfContents
+---------------------------------------------------------------------*/
BLT_METHOD
LinuxCddaDevice_GetTableOfContents(BLT_CddaDevice*           _self, 
                                   BLT_CddaTableOfContents** toc)
{
    LinuxCddaDevice* self = ATX_SELF(LinuxCddaDevice, BLT_CddaDevice);
    *toc = &self->toc;
    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|       LinuxCddaDevice_ReadFrames
+---------------------------------------------------------------------*/
BLT_METHOD
LinuxCddaDevice_ReadFrames(BLT_CddaDevice*         _self,
                           BLT_CddaLba             addr,
                           BLT_Cardinal            count,
                           BLT_Any                 buffer)
{
    LinuxCddaDevice*        self = ATX_SELF(LinuxCddaDevice, BLT_CddaDevice);
    struct cdrom_read_audio read_audio_cmd;
    int                     io_result;

    /* read audio from the device */
    read_audio_cmd.addr.lba    = addr;
    read_audio_cmd.addr_format = CDROM_LBA;
    read_audio_cmd.nframes     = count;
    read_audio_cmd.buf         = (unsigned char*)buffer;
    io_result = ioctl(self->fd, CDROMREADAUDIO, &read_audio_cmd);
    if (io_result != 0) return BLT_FAILURE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(LinuxCddaDevice)
    ATX_GET_INTERFACE_ACCEPT(LinuxCddaDevice, BLT_CddaDevice)
    ATX_GET_INTERFACE_ACCEPT(LinuxCddaDevice, ATX_Destroyable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_CddaDevice interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(LinuxCddaDevice, BLT_CddaDevice)
    LinuxCddaDevice_GetTrackInfo,
    LinuxCddaDevice_GetTableOfContents,
    LinuxCddaDevice_ReadFrames
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|       ATX_Destroyable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_DESTROYABLE_INTERFACE(LinuxCddaDevice)

/*----------------------------------------------------------------------
|       BLT_CddaDevice_Create
+---------------------------------------------------------------------*/
BLT_Result 
BLT_CddaDevice_Create(BLT_CString name, BLT_CddaDevice** device)
{
    return LinuxCddaDevice_Create(name, device);
}
