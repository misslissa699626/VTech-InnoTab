/*****************************************************************
|
|    Melo - Utils
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

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "MloConfig.h"
#include "MloUtils.h"
#include "MloResults.h"

/*----------------------------------------------------------------------
|    MLO_BytesFromInt32Be
+---------------------------------------------------------------------*/
void 
MLO_BytesFromInt32Be(unsigned char* buffer, unsigned long value)
{
    buffer[0] = (unsigned char)(value>>24) & 0xFF;
    buffer[1] = (unsigned char)(value>>16) & 0xFF;
    buffer[2] = (unsigned char)(value>> 8) & 0xFF;
    buffer[3] = (unsigned char)(value    ) & 0xFF;
}

/*----------------------------------------------------------------------
|    MLO_BytesFromInt16Be
+---------------------------------------------------------------------*/
void 
MLO_BytesFromInt16Be(unsigned char* buffer, unsigned short value)
{
    buffer[0] = (unsigned char)((value>> 8) & 0xFF);
    buffer[1] = (unsigned char)((value    ) & 0xFF);
}

/*----------------------------------------------------------------------
|    MLO_BytesToInt32Be
+---------------------------------------------------------------------*/
unsigned long 
MLO_BytesToInt32Be(const unsigned char* buffer)
{
    return 
        ( ((unsigned long)buffer[0])<<24 ) |
        ( ((unsigned long)buffer[1])<<16 ) |
        ( ((unsigned long)buffer[2])<<8  ) |
        ( ((unsigned long)buffer[3])     );
}

/*----------------------------------------------------------------------
|    MLO_BytesToInt16Be
+---------------------------------------------------------------------*/
unsigned short 
MLO_BytesToInt16Be(const unsigned char* buffer)
{
    return 
        ( ((unsigned short)buffer[0])<<8  ) |
        ( ((unsigned short)buffer[1])     );
}

/*----------------------------------------------------------------------
|    MLO_BytesFromInt32Le
+---------------------------------------------------------------------*/
void 
MLO_BytesFromInt32Le(unsigned char* buffer, unsigned long value)
{
    buffer[3] = (unsigned char)(value>>24) & 0xFF;
    buffer[2] = (unsigned char)(value>>16) & 0xFF;
    buffer[1] = (unsigned char)(value>> 8) & 0xFF;
    buffer[0] = (unsigned char)(value    ) & 0xFF;
}

/*----------------------------------------------------------------------
|    MLO_BytesFromInt16Le
+---------------------------------------------------------------------*/
void 
MLO_BytesFromInt16Le(unsigned char* buffer, unsigned short value)
{
    buffer[1] = (unsigned char)((value>> 8) & 0xFF);
    buffer[0] = (unsigned char)((value    ) & 0xFF);
}

/*----------------------------------------------------------------------
|    MLO_BytesToInt32Le
+---------------------------------------------------------------------*/
unsigned long 
MLO_BytesToInt32Le(const unsigned char* buffer)
{
    return 
        ( ((unsigned long)buffer[3])<<24 ) |
        ( ((unsigned long)buffer[2])<<16 ) |
        ( ((unsigned long)buffer[1])<<8  ) |
        ( ((unsigned long)buffer[0])     );
}

/*----------------------------------------------------------------------
|    MLO_BytesToInt16Le
+---------------------------------------------------------------------*/
unsigned short 
MLO_BytesToInt16Le(const unsigned char* buffer)
{
    return 
        ( ((unsigned short)buffer[1])<<8  ) |
        ( ((unsigned short)buffer[0])     );
}
