/*****************************************************************
|
|   BlueTune - Media Definitions
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_MediaType API
 */

#ifndef _BLT_MEDIA_H_
#define _BLT_MEDIA_H_

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
typedef unsigned int BLT_MediaTypeId;

typedef struct {
    BLT_MediaTypeId id;
    BLT_Flags       flags;
    BLT_Size        extension_size;
    /* extended types are subclassed, and contain 'extension_size' */
    /* bytes following this                                        */
} BLT_MediaType;

typedef struct {
    ATX_String name;
    ATX_String value;
} BLT_MimeTypeParameter;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_MEDIA_TYPE_ID_NONE             0 /** no type           */
#define BLT_MEDIA_TYPE_ID_UNKNOWN          1 /** unknown type      */
#define BLT_MEDIA_TYPE_ID_AUDIO_PCM        2 /** see BltPcm.h      */
#define BLT_MEDIA_TYPE_ID_VIDEO_RAW        3 /** see BltPixels.h   */
#define BLT_MEDIA_TYPE_ID_AUDIO            4 /** unspecified audio */
#define BLT_MEDIA_TYPE_ID_VIDEO            5 /** unspecified video */

/*----------------------------------------------------------------------
|   error codes
+---------------------------------------------------------------------*/
#define BLT_ERROR_INVALID_MEDIA_TYPE             (BLT_ERROR_BASE_MEDIA - 0)
#define BLT_ERROR_INVALID_MEDIA_FORMAT           (BLT_ERROR_BASE_MEDIA - 1)
#define BLT_ERROR_UNSUPPORTED_CODEC              (BLT_ERROR_BASE_MEDIA - 2)
#define BLT_ERROR_UNSUPPORTED_FORMAT             (BLT_ERROR_BASE_MEDIA - 3)
#define BLT_ERROR_NO_MEDIA_KEY                   (BLT_ERROR_BASE_MEDIA - 4)
#define BLT_ERROR_INCOMPLETE_MEDIA               (BLT_ERROR_BASE_MEDIA - 5)

#if defined(__cplusplus)
extern "C" {
#endif

/*----------------------------------------------------------------------
|   shared data
+---------------------------------------------------------------------*/
extern const BLT_MediaType BLT_MediaType_None;
extern const BLT_MediaType BLT_MediaType_Unknown;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
BLT_Result  BLT_MediaType_Init(BLT_MediaType* type, BLT_MediaTypeId id);
BLT_Result  BLT_MediaType_InitEx(BLT_MediaType* type, BLT_MediaTypeId id, BLT_Size type_struct_size);
BLT_Result  BLT_MediaType_Free(BLT_MediaType* type);
BLT_Result  BLT_MediaType_Clone(const BLT_MediaType* from, BLT_MediaType** to);
BLT_Boolean BLT_MediaType_Equals(const BLT_MediaType* self, const BLT_MediaType* other);
                               
BLT_Result BLT_ParseMimeType(const char* mime_type, ATX_String* main_type, ATX_List** parameters);

#if defined(__cplusplus)
}
#endif

#endif /* _BLT_MEDIA_H_ */
