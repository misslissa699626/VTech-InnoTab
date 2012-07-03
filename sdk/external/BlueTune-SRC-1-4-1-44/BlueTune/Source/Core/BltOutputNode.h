/*****************************************************************
|
|   BlueTune - Output Node Interface
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_OutputNode interface
 */

#ifndef _BLT_OUTPUT_NODE_H_
#define _BLT_OUTPUT_NODE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltTime.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_OUTPUT_NODE_STATUS_QUEUE_FULL  1
#define BLT_OUTPUT_NODE_STATUS_UNDERFLOW   2

#define BLT_OUTPUT_NODE_WIDTH      "OutputNode.Width"
#define BLT_OUTPUT_NODE_HEIGHT     "OutputNode.Height"
#define BLT_OUTPUT_NODE_FULLSCREEN "OutputNode.FullScreen"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_TimeStamp media_time;
    BLT_Flags     flags;
} BLT_OutputNodeStatus;

typedef void* (*EqualizerConvert)(void *src, BLT_UInt32 length);

/*----------------------------------------------------------------------
|   BLT_OutputNode Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_OutputNode)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_OutputNode)
    BLT_Result (*GetStatus)(BLT_OutputNode*       self, 
                            BLT_OutputNodeStatus* status);
    BLT_Result (*Drain)(BLT_OutputNode* self);
    BLT_Result (*SetEqualizerFunction)(BLT_OutputNode* self,
                                   EqualizerConvert function);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_OutputNode_GetStatus(object, status) \
ATX_INTERFACE(object)->GetStatus(object, status)

#define BLT_OutputNode_Drain(object) \
    (ATX_INTERFACE(object)->Drain?ATX_INTERFACE(object)->Drain(object):BLT_SUCCESS)

#define BLT_OutputNode_SetEqualizerFunction(object, function) \
    (ATX_INTERFACE(object)->SetEqualizerFunction?ATX_INTERFACE(object)->SetEqualizerFunction(object, function):BLT_SUCCESS)
#endif /* _BLT_OUTPUT_NODE_H_ */
