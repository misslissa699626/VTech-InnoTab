/*****************************************************************
|
|      File: AtxStdcDebug.c
|
|      Atomix - Debug Support: Android Implementation
|
|      (c) 2002-2010 Gilles Boccon-Gibod
|      Author: Edin Hodzic (dino@concisoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <android/log.h>

#include "AtxConfig.h"
#include "AtxDefs.h"
#include "AtxTypes.h"
#include "AtxDebug.h"

/*----------------------------------------------------------------------
|       ATX_DebugOuput
+---------------------------------------------------------------------*/
void
ATX_DebugOutput(const char* message)
{
    __android_log_write(ANDROID_LOG_DEBUG, "Atomix", message);
}
