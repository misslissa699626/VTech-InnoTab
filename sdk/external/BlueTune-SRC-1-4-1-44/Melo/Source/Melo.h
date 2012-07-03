/*****************************************************************
|
|    Melo - Top Level Header
|
|    Copyright 2004-2006 Axiomatic Systems LLC
|
|    This file is part of Melo (Melo AAC Decoder).
|
|    Unless you have obtained Melo under a difference license,
|    this version of Melo is Melo|GPL.
|    Melo|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Melo|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Melo|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
 ****************************************************************/
/** @file
 * Master Header file included by Melo clients
 */

/** 
@mainpage Melo AAC Decoder SDK

@section intro Introduction
 
The Melo AAC Decoder is a portable runtime library, written in ANSI C
that decodes AAC Low Complexity audio frames and produces PCM audio
samples.

@section header_files Header Files
Client applications that use the functions and data structures of the SDK
should only include the file Melo.h. This is the master include file that
includes a number of more specific include files containing the prototypes
and type definitions for all the client API functions and data structures
of the SDK.

@section decoder_api Decoder API
Client applications should use the Decoder API to create an @ref MLO_Decoder
object, use it to decode AAC audio frames into PCM audio samples, and 
destroy that decoder object when no longer needed. See the @ref MLO_Decoder
documentation for API details.

*/

#ifndef _MELO_H_
#define _MELO_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloTypes.h"
#include "MloResults.h"
#include "MloDecoder.h"
#include "MloBitStream.h"
#include "MloFrame.h"
#include "MloAdtsParser.h"

#endif /* _MELO_H_ */
