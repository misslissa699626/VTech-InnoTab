/*****************************************************************
|
|   BlueTune - A/V Synch Support
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Audio/Video Synchronization
 */

#ifndef _BLT_SYNCHRONIZATION_H_
#define _BLT_SYNCHRONIZATION_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_SYNC_MODE_TIME_SOURCE
} BLT_SyncMode;

/*----------------------------------------------------------------------
|   BLT_TimeSource Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_TimeSource)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_TimeSource)
    BLT_Result (*GetTime)(BLT_TimeSource* self, BLT_TimeStamp* time);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   BLT_SyncSlave Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_SyncSlave)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_SyncSlave)
    BLT_Result (*SetTimeSource)(BLT_SyncSlave* self, BLT_TimeSource* source);
    BLT_Result (*SetSyncMode)(BLT_SyncSlave* self, BLT_SyncMode mode);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_TimeSource_GetTime(object, time) \
ATX_INTERFACE(object)->GetTime(object, time)

#define BLT_SyncSlave_SetTimeSource(object, source) \
ATX_INTERFACE(object)->SetTimeSource(object, source)

#define BLT_SyncSlave_SetSyncMode(object, mode) \
ATX_INTERFACE(object)->SetSyncMode(object, mode)

#endif /* _BLT_SYNCHRONIZATION_H_ */
