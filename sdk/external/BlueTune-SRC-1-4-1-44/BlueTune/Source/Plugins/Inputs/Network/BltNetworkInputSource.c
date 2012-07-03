/*****************************************************************
|
|   Network Input Source
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltNetworkInputSource.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_NetworkInputSource) = {0x0201, 0x0001};

/*----------------------------------------------------------------------
|   jump functions
+---------------------------------------------------------------------*/
BLT_Result 
BLT_NetworkInputSource_Attach(BLT_NetworkInputSource* self, 
                              BLT_Stream*             stream)
{
    return ATX_INTERFACE(self)->Attach(self, stream);
}

BLT_Result 
BLT_NetworkInputSource_Detach(BLT_NetworkInputSource* self)
{
    return ATX_INTERFACE(self)->Detach(self);
}

