/*****************************************************************
|
|   BlueTune - Interface Constants
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"

/*----------------------------------------------------------------------
|   interface constants
+---------------------------------------------------------------------*/
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_Module)               = {0x0101, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_Core)                 = {0x0102, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_Stream)               = {0x0103, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_MediaNode)            = {0x0104, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_MediaPort)            = {0x0105, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_Registry)             = {0x0106, 0x0002};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_PacketProducer)       = {0x0107, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_PacketConsumer)       = {0x0108, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_InputStreamProvider)  = {0x0109, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_OutputStreamProvider) = {0x010A, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_InputStreamUser)      = {0x010B, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_OutputStreamUser)     = {0x010C, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_EventListener)        = {0x010D, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_OutputNode)           = {0x010E, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_KeyManager)           = {0x010F, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_TimeSource)           = {0x0110, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_SyncSlave)            = {0x0111, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_VolumeControl)        = {0x0112, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_CipherFactory)        = {0x0113, 0x0001};
const ATX_InterfaceId ATX_INTERFACE_ID(BLT_Cipher)               = {0x0114, 0x0001};
