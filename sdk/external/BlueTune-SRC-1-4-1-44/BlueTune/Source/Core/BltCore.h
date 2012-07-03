/*****************************************************************
|
|   BlueTune - Core API
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_Core interface
 */

#ifndef _BLT_CORE_H_
#define _BLT_CORE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltRegistry.h"
#include "BltMediaPacket.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_MODULE_CATEGORY_INPUT     0x01
#define BLT_MODULE_CATEGORY_PARSER    0x02
#define BLT_MODULE_CATEGORY_FORMATTER 0x04
#define BLT_MODULE_CATEGORY_DECODER   0x08
#define BLT_MODULE_CATEGORY_ENCODER   0x10
#define BLT_MODULE_CATEGORY_FILTER    0x20
#define BLT_MODULE_CATEGORY_OUTPUT    0x40

/*----------------------------------------------------------------------
|   references
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_Stream)
ATX_DECLARE_INTERFACE(BLT_Module)
ATX_DECLARE_INTERFACE(BLT_MediaNode)
typedef struct BLT_MediaNodeConstructor BLT_MediaNodeConstructor;

/*----------------------------------------------------------------------
|   BLT_Core Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_Core)
/**
 * @brief Interface implemented by the core of the BlueTune system
 *  
 */
ATX_BEGIN_INTERFACE_DEFINITION(BLT_Core)
    BLT_Result (*CreateStream)(BLT_Core* self, BLT_Stream** stream);
    BLT_Result (*RegisterModule)(BLT_Core* self, BLT_Module* module);
    BLT_Result (*UnRegisterModule)(BLT_Core* self, BLT_Module* module);
    BLT_Result (*EnumerateModules)(BLT_Core* self, ATX_List** modules);
    BLT_Result (*GetRegistry)(BLT_Core* self, BLT_Registry** registry);
    BLT_Result (*GetProperties)(BLT_Core* self, ATX_Properties** properties);
    BLT_Result (*CreateCompatibleMediaNode)(BLT_Core*                 self,
                                            BLT_MediaNodeConstructor* constructor,
                                            BLT_MediaNode**           node);
    BLT_Result (*CreateMediaPacket)(BLT_Core*            self, 
                                    BLT_Size             size, 
                                    const BLT_MediaType* type,
                                    BLT_MediaPacket**    packet);
    BLT_Result (*ParseMimeType)(BLT_Core*       self, 
                                const char*     mime_type, 
                                BLT_MediaType** media_type);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_Core_CreateStream(object, stream) \
ATX_INTERFACE(object)->CreateStream(object, stream)

#define BLT_Core_RegisterModule(object, module) \
ATX_INTERFACE(object)->RegisterModule(object, module)

#define BLT_Core_UnRegisterModule(object, module) \
ATX_INTERFACE(object)->UnRegisterModule(object, module)

#define BLT_Core_EnumerateModules(object, modules) \
ATX_INTERFACE(object)->EnumerateModules(object, modules)

#define BLT_Core_GetRegistry(object, registry) \
ATX_INTERFACE(object)->GetRegistry(object, registry)

#define BLT_Core_GetProperties(object, settings) \
ATX_INTERFACE(object)->GetProperties(object, settings)

#define BLT_Core_CreateCompatibleMediaNode(object, constructor, node) \
ATX_INTERFACE(object)->CreateCompatibleMediaNode(object, constructor, node)

#define BLT_Core_CreateMediaPacket(object, size, type, packet)\
ATX_INTERFACE(object)->CreateMediaPacket(object, size, type, packet)

#define BLT_Core_ParseMimeType(object, mime_type, media_type)\
ATX_INTERFACE(object)->ParseMimeType(object, mime_type, media_type)

#define BLT_Core_Destroy(object) ATX_DESTROY_OBJECT(object)

#endif /* _BLT_CORE_H_ */
