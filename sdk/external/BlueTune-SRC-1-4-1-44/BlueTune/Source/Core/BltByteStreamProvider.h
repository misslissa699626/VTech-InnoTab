/*****************************************************************
|
|   BlueTune - InputStreamProvider & OutputStreamProvider
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_StreamProvider interface
 */

#ifndef _BLT_BYTE_STREAM_PROVIDER_H_
#define _BLT_BYTE_STREAM_PROVIDER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltTypes.h"
#include "BltMedia.h"

/*----------------------------------------------------------------------
|   BLT_InputStreamProvider
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_InputStreamProvider)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_InputStreamProvider)
    BLT_Result (*GetStream)(BLT_InputStreamProvider* self,
                            ATX_InputStream**        stream);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_InputStreamProvider_GetStream(object, stream) \
ATX_INTERFACE(object)->GetStream(object, stream)


/*----------------------------------------------------------------------
|   BLT_OutputStreamProvider
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_OutputStreamProvider)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_OutputStreamProvider)
    BLT_Result (*GetStream)(BLT_OutputStreamProvider* instance,
                            ATX_OutputStream**        stream,
                            const BLT_MediaType*      media_type);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_OutputStreamProvider_GetStream(object, stream, media_type) \
ATX_INTERFACE(object)->GetStream(object, stream, media_type)

#endif /* _BLT_BYTE_STREAM_PROVIDER_H_ */
