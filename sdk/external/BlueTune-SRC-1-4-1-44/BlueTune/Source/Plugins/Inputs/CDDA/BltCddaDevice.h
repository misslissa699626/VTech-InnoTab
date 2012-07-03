/*****************************************************************
|
|      Cdda: BltCddaDevice.h
|
|      Cdda Device Module
|
|      (c) 2002-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_CDDA_DEVICE_H_
#define _BLT_CDDA_DEVICE_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"

/*----------------------------------------------------------------------
|       types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_CDDA_TRACK_TYPE_AUDIO,
    BLT_CDDA_TRACK_TYPE_DATA
} BLT_CddaTrackType;

typedef unsigned long BLT_CddaLba;
typedef struct {
    unsigned int m;
    unsigned int s;
    unsigned int f;
} BLT_CddaMsf;

typedef struct {
    unsigned short left;
    unsigned short right;
} BLT_CddaSample;

typedef struct {
    BLT_Ordinal       index;
    BLT_CddaTrackType type;
    BLT_CddaLba       address;
    struct {
        BLT_CddaLba frames;
        BLT_CddaMsf msf;
    }                 duration;
} BLT_CddaTrackInfo;

typedef struct {
    BLT_Cardinal       track_count;
    BLT_Ordinal        first_track_index;
    BLT_Ordinal        last_track_index;
    BLT_CddaTrackInfo* tracks;
} BLT_CddaTableOfContents;

typedef struct BLT_CddaTrack BLT_CddaTrack;

/*----------------------------------------------------------------------
|       constants
+---------------------------------------------------------------------*/
#define BLT_CDDA_FRAME_SIZE        2352
#define BLT_CDDA_FRAMES_PER_SECOND 75
#define BLT_CDDA_MSF_FRAME_OFFSET  150

/*----------------------------------------------------------------------
|       BLT_CddaDevice interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_CddaDevice)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_CddaDevice)
    BLT_Result (*GetTrackInfo)(BLT_CddaDevice*    self, 
                               BLT_Ordinal        index,
                               BLT_CddaTrackInfo* info);
    BLT_Result (*GetTableOfContents)(BLT_CddaDevice*           self,
                                     BLT_CddaTableOfContents** toc);
    BLT_Result (*ReadFrames)(BLT_CddaDevice* self,
                             BLT_CddaLba     addr,
                             BLT_Cardinal    count,
                             BLT_Any         buffer);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|       convenience macros
+---------------------------------------------------------------------*/
#define BLT_CddaDevice_GetTrackInfo(self, index, info) \
ATX_INTERFACE(self)->GetTrackInfo(self, index, info)

#define BLT_CddaDevice_GetTableOfContents(self, toc) \
ATX_INTERFACE(self)->GetTableOfContents(self, toc)

#define BLT_CddaDevice_ReadFrames(self, addr, count, samples) \
ATX_INTERFACE(self)->ReadFrames(self, addr, count, samples)


/*----------------------------------------------------------------------
|       prototypes
+---------------------------------------------------------------------*/
void       BLT_Cdda_FramesToMsf(BLT_CddaLba frames, BLT_CddaMsf* msf);
BLT_Result BLT_CddaDevice_Create(BLT_CString name, BLT_CddaDevice** device);
BLT_Result BLT_CddaTrack_Create(BLT_CddaDevice*   device,
                                BLT_Ordinal       index,
                                ATX_InputStream** track);

#endif /* _BLT_CDDA_DEVICE_H_ */
