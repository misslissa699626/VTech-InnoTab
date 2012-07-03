/*****************************************************************
|
|   Atomix Tests - Logging Test
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
|  includes
+---------------------------------------------------------------------*/
#include "Atomix.h"

ATX_DEFINE_LOGGER(MyLogger, "atomix.test.my")
ATX_DEFINE_LOGGER(FooLogger, "atomix.test.foo")
ATX_SET_LOCAL_LOGGER("atomix.test")

/*----------------------------------------------------------------------
|  TestCheck functions
+---------------------------------------------------------------------*/
static ATX_Result TestCheck(void)
{
    ATX_CHECK_L(ATX_LOG_LEVEL_WARNING, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckSevere(void)
{
    ATX_CHECK_SEVERE(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckWarning(void)
{
    ATX_CHECK_WARNING(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckInfo(void)
{
    ATX_CHECK_INFO(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckFine(void)
{
    ATX_CHECK_FINE(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckFiner(void)
{
    ATX_CHECK_FINER(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckFinest(void)
{
    ATX_CHECK_FINEST(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}

static ATX_Result TestCheckL(void)
{
    ATX_CHECK_LL(FooLogger, ATX_LOG_LEVEL_WARNING, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckSevereL(void)
{
    ATX_CHECK_SEVERE(ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckWarningL(void)
{
    ATX_CHECK_WARNING_L(FooLogger, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckInfoL(void)
{
    ATX_CHECK_INFO_L(FooLogger, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckFineL(void)
{
    ATX_CHECK_FINE_L(FooLogger, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckFinerL(void)
{
    ATX_CHECK_FINER_L(FooLogger, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}
static ATX_Result TestCheckFinestL(void)
{
    ATX_CHECK_FINEST_L(FooLogger, ATX_FAILURE);
    ATX_LOG_SEVERE("###");
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|  TestLargeBuffer
+---------------------------------------------------------------------*/
static void
TestLargeBuffer(void)
{
    char* buffer = ATX_AllocateMemory(32768);
    int i;
    for (i=0; i<32768; i++) {
        buffer[i] = 'a';
    }
    buffer[32767] = 0;
    ATX_LOG_SEVERE(buffer);
    ATX_FreeMemory((void*)buffer);
}

/*----------------------------------------------------------------------
|  CheckWithLabels
+---------------------------------------------------------------------*/
static int
CheckWithLabels(void)
{
    ATX_CHECK_SEVERE(3==4);
    ATX_CHECK_SEVERE(1);
    ATX_CHECK_LABEL_WARNING(1, end);
    ATX_CHECK_LABEL_WARNING(4==5, end);
    ATX_LOG_SEVERE("you should not be reading this");
    
end:
    ATX_LOG_INFO("end");
    return 1;
}

/*----------------------------------------------------------------------
|  main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    ATX_COMPILER_UNUSED(argc);
    ATX_COMPILER_UNUSED(argv);
    
    ATX_LOG_L(MyLogger, ATX_LOG_LEVEL_WARNING, "blabla");
    ATX_LOG_L2(MyLogger, ATX_LOG_LEVEL_WARNING, "blabla %d %d", 8, 9);
    ATX_LOG(ATX_LOG_LEVEL_WARNING, "blabla");
    ATX_LOG_2(ATX_LOG_LEVEL_WARNING, "blabla %d %d", 8, 9);

    ATX_LOG(ATX_LOG_LEVEL_WARNING, "blibli");
    ATX_LOG_2(ATX_LOG_LEVEL_INFO, "fofo %d %d", 5, 7);
    ATX_LOG_L(MyLogger, ATX_LOG_LEVEL_WARNING, "coucou");
    ATX_LOG_L2(MyLogger, ATX_LOG_LEVEL_WARNING, "coucou %d %d", 8, 9);
    ATX_LOG(ATX_LOG_LEVEL_WARNING, "coucou");
    ATX_LOG_2(ATX_LOG_LEVEL_WARNING, "coucou %d %d", 8, 9);
    ATX_LOG_L(MyLogger, ATX_LOG_LEVEL_WARNING, "hello");
    ATX_LOG_L2(MyLogger, ATX_LOG_LEVEL_WARNING, "hello %d %d", 10, 11);
    
    ATX_LOG_SEVERE("this is severe!");
    ATX_LOG_SEVERE_1("this is severe (%d)", 9);

    ATX_LOG_SEVERE_L(MyLogger, "this is severe!");
    ATX_LOG_SEVERE_L1(MyLogger, "this is severe (%d)", 9);

    ATX_LOG_SEVERE_L(FooLogger, "this is severe!");
    ATX_LOG_SEVERE_L1(FooLogger, "this is severe (%d)", 9);

    ATX_LOG_SEVERE("severe");
    ATX_LOG_WARNING("warning");
    ATX_LOG_INFO("info");
    ATX_LOG_FINE("fine");
    ATX_LOG_FINER("finer");
    ATX_LOG_FINEST("finest");

    ATX_LOG_SEVERE_L(FooLogger, "severe");
    ATX_LOG_WARNING_L(FooLogger, "warning");
    ATX_LOG_INFO_L(FooLogger, "info");
    ATX_LOG_FINE_L(FooLogger, "fine");
    ATX_LOG_FINER_L(FooLogger, "finer");
    ATX_LOG_FINEST_L(FooLogger, "finest");

    CheckWithLabels();
    
    TestLargeBuffer();

    TestCheck();
    TestCheckSevere();
    TestCheckWarning();
    TestCheckInfo();
    TestCheckFine();
    TestCheckFiner();
    TestCheckFinest();

    TestCheckL();
    TestCheckSevereL();
    TestCheckWarningL();
    TestCheckInfoL();
    TestCheckFineL();
    TestCheckFinerL();
    TestCheckFinestL();

    return 0;
}

