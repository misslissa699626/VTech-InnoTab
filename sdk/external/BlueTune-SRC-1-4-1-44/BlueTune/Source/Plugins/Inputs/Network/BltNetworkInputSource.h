/*****************************************************************
|
|   Network Input Module - Private Header
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _BLT_NETWORK_INPUT__H_
#define _BLT_NETWORK_INPUT__H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltTypes.h"
#include "BltModule.h"
#include "AtxInterfaces.h"

/*----------------------------------------------------------------------
|   BLT_NetworkInputSource
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_NetworkInputSource)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_NetworkInputSource)
    BLT_Result (*Attach)(BLT_NetworkInputSource* self, BLT_Stream* stream);
    BLT_Result (*Detach)(BLT_NetworkInputSource* self);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   jump functions
+---------------------------------------------------------------------*/
BLT_Result 
BLT_NetworkInputSource_Attach(BLT_NetworkInputSource* self, 
                              BLT_Stream*             stream);
BLT_Result 
BLT_NetworkInputSource_Detach(BLT_NetworkInputSource* self);

#endif /* _BLT_NETWORK_INPUT__H_ */
