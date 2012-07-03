/*****************************************************************
|
|   BlueTune - Volume Control Interface
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_VolumeControl interface
 */

#ifndef _BLT_VOLUME_CONTROL_H_
#define _BLT_VOLUME_CONTROL_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   BLT_VolumeControl Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_VolumeControl)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_VolumeControl)
    BLT_Result (*GetVolume)(BLT_VolumeControl* self, float* volume);
    BLT_Result (*SetVolume)(BLT_VolumeControl* self, float volume);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_VolumeControl_GetVolume(object, volume) \
ATX_INTERFACE(object)->GetVolume(object, volume)

#define BLT_VolumeControl_SetVolume(object, volume) \
ATX_INTERFACE(object)->SetVolume(object, volume)

#endif /* _BLT_VOLUME_CONTROL_H_ */
