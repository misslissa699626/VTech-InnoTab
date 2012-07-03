/*****************************************************************
|
|   BlueTune - Media Port Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_MediaPort interface.
 */

#ifndef _BLT_MEDIA_PORT_H_
#define _BLT_MEDIA_PORT_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltMedia.h"

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define BLT_ERROR_NO_SUCH_PORT       (BLT_ERROR_BASE_MEDIA_PORT - 0)
#define BLT_ERROR_PORT_HAS_NO_DATA   (BLT_ERROR_BASE_MEDIA_PORT - 1)
#define BLT_ERROR_PORT_HAS_NO_STREAM (BLT_ERROR_BASE_MEDIA_PORT - 2)

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_MEDIA_PORT_PROTOCOL_ANY,
    BLT_MEDIA_PORT_PROTOCOL_NONE,
    BLT_MEDIA_PORT_PROTOCOL_PACKET,
    BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH,
    BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL
} BLT_MediaPortProtocol;

typedef enum {
    BLT_MEDIA_PORT_DIRECTION_NONE,
    BLT_MEDIA_PORT_DIRECTION_IN,
    BLT_MEDIA_PORT_DIRECTION_OUT
} BLT_MediaPortDirection;

typedef struct {
    BLT_MediaPortProtocol protocol;
    const BLT_MediaType*  media_type;
} BLT_MediaPortInterfaceSpec;

/*----------------------------------------------------------------------
|   BLT_MediaPort Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_MediaPort)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_MediaPort)
    BLT_Result (*GetName)(BLT_MediaPort*  self, BLT_CString* name);
    BLT_Result (*GetProtocol)(BLT_MediaPort*         self, 
                              BLT_MediaPortProtocol* protocol);
    BLT_Result (*GetDirection)(BLT_MediaPort*          self, 
                               BLT_MediaPortDirection* direction);
    BLT_Result (*QueryMediaType)(BLT_MediaPort*        self,
                                 BLT_Ordinal           index,
                                 const BLT_MediaType** media_type);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_MediaPort_GetName(object, name)\
ATX_INTERFACE(object)->GetName(object, name)

#define BLT_MediaPort_GetProtocol(object, protocol)\
ATX_INTERFACE(object)->GetProtocol(object, protocol)

#define BLT_MediaPort_GetDirection(object, direction)\
ATX_INTERFACE(object)->GetDirection(object, direction)

#define BLT_MediaPort_QueryMediaType(object, index, media_type)\
ATX_INTERFACE(object)->QueryMediaType(object, index, media_type)

/*----------------------------------------------------------------------
|   templates
+---------------------------------------------------------------------*/
#define BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(port, n, proto, dir)   \
BLT_METHOD port##_GetName(BLT_MediaPort* self, BLT_CString* name)       \
{                                                                       \
    BLT_COMPILER_UNUSED(self);                                          \
    *name = (n);                                                        \
    return BLT_SUCCESS;                                                 \
}                                                                       \
BLT_METHOD port##_GetProtocol(BLT_MediaPort*         self,              \
                              BLT_MediaPortProtocol* protocol)          \
{                                                                       \
    BLT_COMPILER_UNUSED(self);                                          \
    *protocol = BLT_MEDIA_PORT_PROTOCOL_##proto;                        \
    return BLT_SUCCESS;                                                 \
}                                                                       \
BLT_METHOD port##_GetDirection(BLT_MediaPort*          self,            \
                               BLT_MediaPortDirection* direction)       \
{                                                                       \
    BLT_COMPILER_UNUSED(self);                                          \
    *direction = BLT_MEDIA_PORT_DIRECTION_##dir;                        \
    return BLT_SUCCESS;                                                 \
}


/*----------------------------------------------------------------------
|   functions
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
    
BLT_Result 
BLT_MediaPort_DefaultQueryMediaType(BLT_MediaPort*         self,
                                    BLT_Ordinal            index,
                                    const BLT_MediaType**  media_type);

#if defined(__cplusplus)
}
#endif

#endif /* _BLT_MEDIA_PORT_H_ */
