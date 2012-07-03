/*****************************************************************
|
|   BlueTune - Event Listener Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_EventListener interface
 */

#ifndef _BLT_EVENT_LISTENER_H_
#define _BLT_EVENT_LISTENER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltEvent.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_EventListener)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_EventListener)
    void (*OnEvent)(BLT_EventListener* self,
                    ATX_Object*        source,
                    BLT_EventType      type,
                    const BLT_Event*   event);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
/**
 * Convenience macro used to call the OnEvent() method on objects 
 * that implement the BLT_EventListener interface 
 */
#define BLT_EventListener_OnEvent(object, source, type, event) \
ATX_INTERFACE(object)->OnEvent(object, source, type, event)


#endif /* _BLT_EVENT_LISTENER_H_ */
