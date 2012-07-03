/*****************************************************************
|
|      Atomix - Helpers: SymbianOS Implementation
|
| Copyright (c) 2002-2010, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "AtxSymbianHelper.h"
#include "AtxSymbianUtils.h"

#include "AtxTypes.h"
#include "AtxResults.h"
#include "AtxDebug.h"
#include "AtxFile.h"
#include "AtxUtils.h"

#include "e32def.h"
#include "e32math.h"

#include "f32file.h"

#include "string.h" // symbian's string.h



/*----------------------------------------------------------------------
|       ATX_Symbian_GetRandomBytes
+---------------------------------------------------------------------*/
unsigned long
ATX_Symbian_GetRandomBytes(unsigned char *buf, unsigned long len)
{
    TUint32 acc = 0;
    for (unsigned long i = 0; i < len; ++i) {
      if (len % 4 == 0)
          acc = Math::Random();
      buf[i] = acc;
      acc >>= 8;
   }
    
    return len;
}


/*----------------------------------------------------------------------
|       ATX_Heap_Available
+---------------------------------------------------------------------*/
ATX_Int32
ATX_Heap_Available()
{
    RHeap& heap = User::Heap();
    TInt biggestblock = 0;
    /*TInt freemem =*/ 
    heap.Available(biggestblock);
    return biggestblock;
}
