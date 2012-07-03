/*****************************************************************
|
|   BlueTune - Media Node Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_MediaNode interface
 */

#ifndef _BLT_MEDIA_NODE_H_
#define _BLT_MEDIA_NODE_H_

/**
 * @defgroup plugin_input_modules Input Plugin Modules
 * Plugin modules that can be used as input. Input modules
 * are always positioned first in a chain.
 * Input modules product byte streams or media packets.
 *
 * @defgroup plugin_output_modules Output Plugin Modules
 * Plugin modules that can be used as output. Output modules
 * are always positioned last in a chain. Output modules consume
 * byte streams or media packets, and produce nothing.
 *
 * @defgroup plugin_parser_modules Parser Plugin Modules
 * Plugin modules that parse media formats. Typically, these modules
 * consume a byte stream at their input and product either media packets
 * or another byte stream.
 *
 * @defgroup plugin_formatter_modules Formatter Plugin Modules
 * Plugin modules that format media formats. Typically, these modules
 * consume a byte stream or media packets, and produce a formatted 
 * byte stream.
 *
 * @defgroup plugin_decoder_modules Decoder Plugin Modules
 * Plugin modules that implement some type of decoder that decodes
 * one format of media buffers and produces another format.
 *
 * @defgroup plugin_adapter_modules Adapter Plugin Modules
 * Plugin modules that adapt one media type to another similar media
 * type with slightly different characteristics.
 *
 * @defgroup plugin_filter_modules Filter Plugin Modules
 * Plugin modules that produce the same type of data as what they
 * consume, and perform some type of filter transformation on the
 * media data.
 *
 * @defgroup plugin_filter_modules General Plugin Modules
 * Plugin modules that act on all types of media types, and usually
 * just change their input type into a different output type, but 
 * without changing the media data.
 *
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltMedia.h"
#include "BltCore.h"
#include "BltMediaPort.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_ERROR_NO_SUCH_MEDIA_NODE (BLT_ERROR_BASE_MEDIA_NODE - 0)

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef enum {
    BLT_MEDIA_NODE_STATE_RESET,
    BLT_MEDIA_NODE_STATE_IDLE,
    BLT_MEDIA_NODE_STATE_RUNNING,
    BLT_MEDIA_NODE_STATE_PAUSED
} BLT_MediaNodeState;

typedef struct {
    BLT_Module* module;
    BLT_CString name;
    BLT_Flags   flags;
} BLT_MediaNodeInfo;

typedef struct {
    BLT_MediaPortInterfaceSpec input;
    BLT_MediaPortInterfaceSpec output;
} BLT_MediaNodeSpec;

struct BLT_MediaNodeConstructor {
    BLT_CString       name;
    BLT_MediaNodeSpec spec;
};

typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaNode);
    ATX_IMPLEMENTS(ATX_Referenceable);

    /* members */
    BLT_Cardinal      reference_count;
    BLT_Core*         core;
    BLT_Stream*       context;
    BLT_MediaNodeInfo info;
} BLT_BaseMediaNode;

/*----------------------------------------------------------------------
|   BLT_MediaNode Interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_DEFINITION(BLT_MediaNode)
    BLT_Result (*GetInfo)(BLT_MediaNode* self, BLT_MediaNodeInfo* info);
    BLT_Result (*GetPortByName)(BLT_MediaNode*  self, 
                                BLT_CString     name,
                                BLT_MediaPort** port);
    BLT_Result (*Activate)(BLT_MediaNode* self, BLT_Stream* stream);
    BLT_Result (*Deactivate)(BLT_MediaNode* self);
    BLT_Result (*Start)(BLT_MediaNode* self);
    BLT_Result (*Stop)(BLT_MediaNode* self);
    BLT_Result (*Pause)(BLT_MediaNode* self);
    BLT_Result (*Resume)(BLT_MediaNode* self);
    BLT_Result (*Seek)(BLT_MediaNode* self, 
                       BLT_SeekMode*  mode,
                       BLT_SeekPoint* point);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_MediaNode_GetInfo(object, info) \
ATX_INTERFACE(object)->GetInfo(object, info)

#define BLT_MediaNode_GetPortByName(object, name, port) \
ATX_INTERFACE(object)->GetPortByName(object, name, port)

#define BLT_MediaNode_Activate(object, stream) \
ATX_INTERFACE(object)->Activate(object, stream)

#define BLT_MediaNode_Deactivate(object) \
ATX_INTERFACE(object)->Deactivate(object)

#define BLT_MediaNode_Start(object) \
ATX_INTERFACE(object)->Start(object)

#define BLT_MediaNode_Stop(object) \
ATX_INTERFACE(object)->Stop(object)

#define BLT_MediaNode_Pause(object) \
ATX_INTERFACE(object)->Pause(object)

#define BLT_MediaNode_Resume(object) \
ATX_INTERFACE(object)->Resume(object)

#define BLT_MediaNode_Seek(object, mode, point) \
ATX_INTERFACE(object)->Seek(object, mode, point)

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif

BLT_Result BLT_BaseMediaNode_Construct(BLT_BaseMediaNode* self,
                                       BLT_Module*        module,
                                       BLT_Core*          core);
BLT_Result BLT_BaseMediaNode_Destruct(BLT_BaseMediaNode* self);
BLT_Result BLT_BaseMediaNode_GetInfo(BLT_MediaNode*     self,
                                     BLT_MediaNodeInfo* info);
BLT_Result BLT_BaseMediaNode_Activate(BLT_MediaNode* self,
                                      BLT_Stream*    stream);
BLT_Result BLT_BaseMediaNode_Deactivate(BLT_MediaNode* self);
BLT_Result BLT_BaseMediaNode_Start(BLT_MediaNode* self);
BLT_Result BLT_BaseMediaNode_Stop(BLT_MediaNode* self);
BLT_Result BLT_BaseMediaNode_Pause(BLT_MediaNode* self);
BLT_Result BLT_BaseMediaNode_Resume(BLT_MediaNode* self);
BLT_Result BLT_BaseMediaNode_Seek(BLT_MediaNode* self, 
                                  BLT_SeekMode*  mode,
                                  BLT_SeekPoint* point);

#if defined(__cplusplus)
}
#endif

/*----------------------------------------------------------------------
|   template macros
+---------------------------------------------------------------------*/
#define BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(_module_type, _class)    \
BLT_METHOD                                                              \
_module_type##_CreateInstance(BLT_Module*        self,                  \
                              BLT_Core*                core,            \
                              BLT_ModuleParametersType parameters_type, \
                              BLT_AnyConst             parameters,      \
                              const ATX_InterfaceId*   interface_id,    \
                              ATX_Object**             object)          \
{                                                                       \
    if (ATX_INTERFACE_IDS_EQUAL(interface_id,                           \
                                &ATX_INTERFACE_ID__BLT_MediaNode)) {    \
        return _class##_Create(self,                                    \
                               core,                                    \
                               parameters_type,                         \
                               parameters,                              \
                               (BLT_MediaNode**)object);                \
    } else {                                                            \
        return BLT_ERROR_INVALID_INTERFACE;                             \
    }                                                                   \
}                                                                       \

#endif /* _BLT_MEDIA_NODE_H_ */
