/*****************************************************************
|
|   BlueTune - MediaNode Objects
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BlueTune MediaNode Objects
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltDefs.h"
#include "BltErrors.h"
#include "BltCore.h"
#include "BltMediaNode.h"

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Construct
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Construct(BLT_BaseMediaNode* node, 
                            BLT_Module*        module,
                            BLT_Core*          core)
{
    node->reference_count = 1;
    node->core            = core;

    /* setup the node info */
    ATX_SetMemory(&node->info, 0, sizeof(node->info));
    node->info.module = module;

    /* by default, use the module name as the node name */
    if (module) {
        BLT_ModuleInfo module_info;
        BLT_Result     result;
        
        /* keep a reference to the module */
        ATX_REFERENCE_OBJECT(module);

        /* get the module info */
        result = BLT_Module_GetInfo(module, &module_info);
        if (BLT_SUCCEEDED(result) && module_info.name) {
            node->info.name = ATX_DuplicateString(module_info.name);
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Destruct
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Destruct(BLT_BaseMediaNode* node)
{
    /* free the node name */
    if (node->info.name) {
        ATX_FreeMemory((void*)node->info.name);
    }

    /* release the reference to the module */
    ATX_RELEASE_OBJECT(node->info.module);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_GetInfo
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_GetInfo(BLT_MediaNode* _self, BLT_MediaNodeInfo* info)
{
    BLT_BaseMediaNode* self = ATX_SELF(BLT_BaseMediaNode, BLT_MediaNode);
    *info = self->info;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Activate
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    BLT_BaseMediaNode* self = ATX_SELF(BLT_BaseMediaNode, BLT_MediaNode);
    self->context = stream;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Deactivate
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Deactivate(BLT_MediaNode* _self)
{
    BLT_BaseMediaNode* self = ATX_SELF(BLT_BaseMediaNode, BLT_MediaNode);
    self->context = NULL;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Start
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Start(BLT_MediaNode* self)
{
    BLT_COMPILER_UNUSED(self);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Stop
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Stop(BLT_MediaNode* self)
{
    BLT_COMPILER_UNUSED(self);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Pause
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Pause(BLT_MediaNode* self)
{
    BLT_COMPILER_UNUSED(self);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Resume
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Resume(BLT_MediaNode* self)
{
    BLT_COMPILER_UNUSED(self);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    BLT_BaseMediaNode_Seek
+---------------------------------------------------------------------*/
BLT_Result
BLT_BaseMediaNode_Seek(BLT_MediaNode* self, 
                       BLT_SeekMode*  mode,
                       BLT_SeekPoint* point)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);
    return BLT_SUCCESS;
}

