/*****************************************************************
|
|   BlueTune - InputStreamUser & OutputStreamUser
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_InputStreamUser and BLT_OutputStreamUser interfaces
 */

#ifndef _BLT_BYTE_STREAM_USER_H_
#define _BLT_BYTE_STREAM_USER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltMedia.h"

/*----------------------------------------------------------------------
|   BLT_InputStreamUser
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_InputStreamUser)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_InputStreamUser)
    BLT_Result (*SetStream)(BLT_InputStreamUser* self,
                            ATX_InputStream*     stream,
                            const BLT_MediaType* media_type);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_InputStreamUser_SetStream(object, stream, media_type) \
ATX_INTERFACE(object)->SetStream(object, stream, media_type)

/*----------------------------------------------------------------------
|   BLT_OutputStreamUser
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_OutputStreamUser)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_OutputStreamUser)
    BLT_Result (*SetStream)(BLT_OutputStreamUser* instance,
                            ATX_OutputStream*     stream);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_OutputStreamUser_SetStream(object, stream) \
ATX_INTERFACE(object)->SetStream(object, stream)

#endif /* _BLT_BYTE_STREAM_USER_H_ */
