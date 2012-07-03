/*****************************************************************
|
|      BlueTune - Top Level Header
|
|      (c) 2002-2008 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * Master Header file included by BlueTune client applications.
 *
 * Client Applications should only need to include this file, as it 
 * includes all the more specific include files required to use the API
 */

/** 
@mainpage BlueTune SDK

@section intro Introduction
 
The BlueTune SDK contains all the software components necessary to 
build and use the BlueTune Media Player Framework. This includes
the BlueTune code framework and plugins, the Neptune C++ runtime
library, the Atomix C runtime library, as well as other modules.

@section architecture Architecture

The BlueTune framework consists of a core media engine that manages the
processing of a stream of media data. To achieve this, the core uses several
components, called media nodes, that handle each part of the work
involve in getting the audio data from an input to an output.
The chain has at least two media nodes: an input and and output. The core
will create other media nodes based on the data produced by the input, so
that the output media nodes receives data in a mode (called a protocol) and
a format that is suitable.

@subsection media_nodes Media Nodes
Each media node implements the BLT_MediaNode interface. A media node has one
or two media ports. 

@subsection media_ports Media Ports
Media ports are the interface that allows media nodes to 
consume and produce media data. Media data can be exchanged either as byte
streams or media packets. The way a media port exchanges media data is called
a media port protocol. In addition, a media port has a direction, either input
(the media port receives data) or output (the media node produces data).
There are 3 protocols:
    @li The Packet protcol (#BLT_MEDIA_PORT_PROTOCOL_PACKET). For input 
    ports this means that the media port implements the BLT_PacketConsumer
    interface, through which it will receive media packets. For output ports
    this means that the media port implements the BLT_PacketProducer interface
    through, through which it will produce media packets.

    @li The Sream Push protcol (#BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH). For 
    input ports, this means that the media port implements the BLT_StreamProducer
    interface, through which it exposes a BLT_OutputStream object to
    which media data can be written. For output ports, this means that the media port
    implements the BLT_StreamConsumer interface through which it will receive a
    BLT_OutputStream object to which it will write media data.

    @li The Stream Pull protocol (#BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL). For
    input ports, this means that the media port implements the BLT_StreamConsumer
    interface, through which it will receive a BLT_InputStream from which it will
    read media data. For output ports, this means that the media port implements
    the BLT_StreamProducer interface, through which it exposes a BLT_InputStream
    object from which media data can be read.

@subsection media_packets Media Packets
Media packets are buffers of audio data. The media packet encapsulates the data 
and the data type, and provides memory management for the storage of the data.
See the @ref BLT_MediaPacket for details.

@subsection media_byte_streams Media Byte Streams
Media byte streams are either input streams (ATX_InputStream interface) or
output streams (ATX_OutputStream interface) and an associated mime type.

@section api API

There are two programming interfaces in the BlueTune SDK. The low-level
synchronous API, also called the Decoder API, and the high-level 
asynchronous API, also called the Player API.

@subsection low_level Low Level API
 
The low-level API provides a set of functions to do synchronous 
decoding/playback of media. With this API, the caller creates a BLT_Decoder
object, sets the input, output, and may register a number of plugin
components implementing Media Nodes. Then it can decode and output media 
packets one by one, until the end of the input has been reached.
See the @ref BLT_Decoder for details.

@subsection high_level High Level API
 
The high-level API is an interface to an asynchrous decoder built
on top of the low-level synchronous API. The decoder runs in its own
thread. The client application communicates with the decoder thread 
through a message queue.
See the BLT_Player class for details.

*/

#ifndef _BLUETUNE_H_
#define _BLUETUNE_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "BltConfig.h"
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltModule.h"
#include "BltRegistry.h"
#include "BltCore.h"
#include "BltStream.h"
#include "BltTime.h"
#include "BltMedia.h"
#include "BltMediaNode.h"
#include "BltMediaPort.h"
#include "BltMediaPacket.h"
#include "BltBuiltins.h"
#include "BltPacketProducer.h"
#include "BltPacketConsumer.h"
#include "BltByteStreamUser.h"
#include "BltByteStreamProvider.h"
#include "BltDecoder.h"
#include "BltEvent.h"
#include "BltEventListener.h"
#include "BltKeyManager.h"
#include "BltPcm.h"
#include "BltPlayer.h"
#include "BltVersion.h"
#include "BltSvnVersion.h"
#include "BltDynamicPlugins.h"

#endif /* _BLUETUNE_H_ */
