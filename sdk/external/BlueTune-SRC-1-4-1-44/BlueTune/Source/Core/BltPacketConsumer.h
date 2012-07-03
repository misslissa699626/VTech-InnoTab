/*****************************************************************
|
|   BlueTune - Packet Consumer Interface
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_PacketConsumer interface
 */

#ifndef _BLT_PACKET_CONSUMER_H_
#define _BLT_PACKET_CONSUMER_H_

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

/*----------------------------------------------------------------------
|   BLT_PacketConsumer Interface
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(BLT_PacketConsumer)
ATX_BEGIN_INTERFACE_DEFINITION(BLT_PacketConsumer)
    BLT_Result (*PutPacket)(BLT_PacketConsumer* self, 
                            BLT_MediaPacket*    packet);
ATX_END_INTERFACE_DEFINITION

/*----------------------------------------------------------------------
|   convenience macros
+---------------------------------------------------------------------*/
#define BLT_PacketConsumer_PutPacket(object, packet) \
ATX_INTERFACE(object)->PutPacket(object, packet)

#endif /* _BLT_PACKET_CONSUMER_H_ */
