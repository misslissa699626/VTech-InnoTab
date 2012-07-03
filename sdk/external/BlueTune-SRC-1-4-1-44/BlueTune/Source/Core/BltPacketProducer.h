/*****************************************************************
|
|   BlueTune - Packet Producer Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_PacketProducer interface
 */

#ifndef _BLT_PACKET_PRODUCER_H_
#define _BLT_PACKET_PRODUCER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltCore.h"
#include "BltMedia.h"
#include "BltMediaPacket.h"

/*----------------------------------------------------------------------
|   BLT_PacketProducer Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_PacketProducer)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_PacketProducer)
    BLT_Result (*GetPacket)(BLT_PacketProducer* self, 
                            BLT_MediaPacket**   packet);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_PacketProducer_GetPacket(object, packet) \
ATX_INTERFACE(object)->GetPacket(object, packet)

#endif /* _BLT_PACKET_PRODUCER_H_ */
