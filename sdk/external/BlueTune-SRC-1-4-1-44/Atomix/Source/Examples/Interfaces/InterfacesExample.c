/*****************************************************************
|
|   Atomix Examples - Interfaces
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
|   includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include "AtxInterfaces.h"
#include "AtxResults.h"
#include "AtxUtils.h"

ATX_INTERFACE_ID_TYPE_MOD ATX_InterfaceId ATX_INTERFACE_ID__ATX_Polymorphic = {1,0};

/*----------------------------------------------------------------------
|   Foo
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(Foo)
ATX_BEGIN_INTERFACE_DEFINITION(Foo)
    ATX_Result (*FooMethod1)(Foo* self, int x);
    ATX_Result (*FooMethod2)(Foo* self, int x, int y);
    ATX_Result (*DoSomething)(Foo* self);
ATX_END_INTERFACE_DEFINITION

ATX_INTERFACE_ID_TYPE_MOD ATX_InterfaceId ATX_INTERFACE_ID__Foo = {1,2};

#define Foo_FooMethod1(self, x)    ATX_INTERFACE(self)->FooMethod1(self, x)
#define Foo_FooMethod2(self, x, y) ATX_INTERFACE(self)->FooMethod2(self, x, y)
#define Foo_DoSomething(self)      ATX_INTERFACE(self)->DoSomething(self)

/*----------------------------------------------------------------------
|   Bar
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(Bar)
ATX_BEGIN_INTERFACE_DEFINITION(Bar)
    ATX_Result (*BarMethod1)(Bar* self, int x);
    ATX_Result (*BarMethod2)(Bar* self, int x, int y);
    ATX_Result (*DoSomething)(Bar* self);
ATX_END_INTERFACE_DEFINITION

ATX_INTERFACE_ID_TYPE_MOD ATX_InterfaceId ATX_INTERFACE_ID__Bar = {1,3};

#define Bar_BarMethod1(self, x)    ATX_INTERFACE(self)->BarMethod1(self, x)
#define Bar_BarMethod2(self, x, y) ATX_INTERFACE(self)->BarMethod2(self, x, y)
#define Bar_DoSomething(self)      ATX_INTERFACE(self)->DoSomething(self)

/*----------------------------------------------------------------------
|   Yop
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE(Yop)
ATX_BEGIN_INTERFACE_DEFINITION(Yop)
ATX_Result (*YopMethod1)(Yop* self);
ATX_END_INTERFACE_DEFINITION

ATX_INTERFACE_ID_TYPE_MOD ATX_InterfaceId ATX_INTERFACE_ID__Yop = {1,4};

#define Yop_YopMethod1(self) ATX_INTERFACE(self)->YopMethod1(self)

/*----------------------------------------------------------------------
|   MyFooBar
+---------------------------------------------------------------------*/
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(Foo);
    ATX_IMPLEMENTS(Bar);

    /* members */
    int value;
} MyFooBar;

/*----------------------------------------------------------------------
|   MyYop
+---------------------------------------------------------------------*/
typedef struct {
    /* interfaces */
    ATX_IMPLEMENTS(Yop);

    /* base classes */
    ATX_EXTENDS(MyFooBar);

    /* members */
    int yep;
} MyYop;

/*----------------------------------------------------------------------
|   MyFooBar_FooMethod1
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_FooMethod1(Foo* _self, int x)
{
    MyFooBar* self = ATX_SELF(MyFooBar, Foo);
    printf("MyFooBar_FooMethod1 - x=%d, value=%d\n", x, self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_FooMethod2
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_FooMethod2(Foo* _self, int x, int y)
{
    MyFooBar* self = ATX_SELF(MyFooBar, Foo);
    printf("MyFooBar_FooMethod2 - x=%d, y=%d, value=%d\n", x, y, self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_Foo_DoSomething
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_Foo_DoSomething(Foo* _self)
{
    MyFooBar* self = ATX_SELF(MyFooBar, Foo);
    printf("MyFooBar_Foo_DoSomething - value=%d\n", self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MyFooBar)
    ATX_GET_INTERFACE_ACCEPT(MyFooBar, Foo)
    ATX_GET_INTERFACE_ACCEPT(MyFooBar, Bar)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   MyFooBar_FooInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MyFooBar, Foo)
    MyFooBar_FooMethod1,
    MyFooBar_FooMethod2,
    MyFooBar_Foo_DoSomething
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MyFooBar_BarMethod1
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_BarMethod1(Bar* _self, int x)
{
    MyFooBar* self = ATX_SELF(MyFooBar, Bar);
    printf("MyFooBar_BarMethod1 - x=%d, value=%d\n", x, self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_BarMethod2
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_BarMethod2(Bar* _self, int x, int y)
{
    MyFooBar* self = ATX_SELF(MyFooBar, Bar);
    printf("MyFooBar_BarMethod2 - x=%d, y=%d, value=%d\n", x, y, self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_Bar_DoSomething
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_Bar_DoSomething(Bar* _self)
{
    MyFooBar* self = ATX_SELF(MyFooBar, Bar);
    printf("MyFooBar_Bar_DoSomething - value=%d\n", self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_BarInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MyFooBar, Bar)
    MyFooBar_BarMethod1,
    MyFooBar_BarMethod2,
    MyFooBar_Bar_DoSomething
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MyFooBar_Construct
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_Construct(MyFooBar* self, int value)
{
    printf("MyFooBar_Construct - value = %d\n", value);

    /* construct the object */
    self->value = value;

    /* setup the interfaces */
    ATX_SET_INTERFACE(self, MyFooBar, Foo);
    ATX_SET_INTERFACE(self, MyFooBar, Bar);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_Destruct
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_Destruct(MyFooBar* self)
{
    printf("MyFooBar_Destruct - value = %d\n", self->value);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyFooBar_Create
+---------------------------------------------------------------------*/
ATX_METHOD
MyFooBar_Create(int value, Foo** instance)
{
    MyFooBar* object;
    
    printf("MyFooBar_Create - value = %d\n", value);

    /* allocate the object */
    object = (MyFooBar*)ATX_AllocateMemory(sizeof(MyFooBar));
    if (object == NULL) {
        *instance = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }
    *instance = &ATX_BASE(object, Foo);

    /* construct the object */
    return MyFooBar_Construct(object, value);
}


/*----------------------------------------------------------------------
|   MyYop_FooMethod1
+---------------------------------------------------------------------*/
ATX_METHOD
MyYop_FooMethod1(Foo* _self, int x)
{
    MyYop* self = ATX_SELF_EX(MyYop, MyFooBar, Foo);
    printf("MyYop_FooMethod1 - x=%d, value=%d, yep=%d\n", x, ATX_BASE(self, MyFooBar).value, self->yep);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyYop_BarMethod2
+---------------------------------------------------------------------*/
ATX_METHOD
MyYop_BarMethod2(Bar* _self, int x, int y)
{
    MyYop* self = ATX_SELF_EX(MyYop, MyFooBar, Bar);
    printf("MyYop_BarMethod2 - x=%d, y=%d, value=%d, yep=%d\n", x, y, ATX_BASE(self, MyFooBar).value, self->yep);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyYop_YopMethod1
+---------------------------------------------------------------------*/
ATX_METHOD
MyYop_YopMethod1(Yop* _self)
{
    MyYop* self = ATX_SELF(MyYop, Yop);
    printf("MyYop_YopMethod1 - value=%d, yep=%d\n", ATX_BASE(self, MyFooBar).value, self->yep);
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   MyYop_GetInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(MyYop)
    ATX_GET_INTERFACE_ACCEPT_EX(MyYop, MyFooBar, Foo)
    ATX_GET_INTERFACE_ACCEPT_EX(MyYop, MyFooBar, Bar)
    ATX_GET_INTERFACE_ACCEPT(MyYop, Yop)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   MyYop_FooInterface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_GET_INTERFACE_ADAPTER_EX(MyYop, MyFooBar, Foo)
ATX_INTERFACE_MAP(MyYop, Foo) = {
    MyYop_Foo_GetInterface,
    MyYop_FooMethod1,
    MyFooBar_FooMethod2,
    MyFooBar_Foo_DoSomething
};

/*----------------------------------------------------------------------
|   MyYop_BarInterface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_GET_INTERFACE_ADAPTER_EX(MyYop, MyFooBar, Bar)
ATX_INTERFACE_MAP(MyYop, Bar) = {
    MyYop_Bar_GetInterface,
    MyFooBar_BarMethod1,
    MyYop_BarMethod2,
    MyFooBar_Bar_DoSomething
};

/*----------------------------------------------------------------------
|   MyYop_YopInterface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(MyYop, Yop)
    MyYop_YopMethod1
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   MyYop_Create
+---------------------------------------------------------------------*/
ATX_METHOD
MyYop_Create(int value, int yep, Yop** instance)
{
    MyYop*     object;
    ATX_Result result;

    printf("MyYop_Create\n");

    /* allocate the object */
    object = (MyYop*)ATX_AllocateMemory(sizeof(MyYop));
    if (object == NULL) {
        *instance = NULL;
        return ATX_ERROR_OUT_OF_MEMORY;
    }

    /* construct the base classes */
    result = MyFooBar_Construct(&object->MyFooBar_Base, value);
    if (ATX_FAILED(result)) {
        MyFooBar_Destruct(&object->MyFooBar_Base);
        ATX_FreeMemory((void*)object);
        return result;
    }

    /* construct the object */
    object->yep = yep;

    /* setup the interfaces */
    ATX_SET_INTERFACE_EX(object, MyYop, MyFooBar, Foo);
    ATX_SET_INTERFACE_EX(object, MyYop, MyFooBar, Bar);
    ATX_SET_INTERFACE   (object, MyYop, Yop);
    *instance = &ATX_BASE(object, Yop);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    Foo* foo;
    Bar* bar;
    Yop* yop;


    ATX_CHECK(MyFooBar_Create(7, &foo));
    printf("foo = %x\n", ATX_POINTER_TO_LONG(foo));

    Foo_FooMethod1(foo, 9);
    Foo_FooMethod2(foo, 10, 11);
    Foo_DoSomething(foo);

    bar = ATX_CAST(foo, Bar);
    printf("bar = %x\n", ATX_POINTER_TO_LONG(bar));
    Bar_BarMethod1(bar, 12);
    Bar_BarMethod2(bar, 13, 14);
    Bar_DoSomething(bar);

    ATX_CHECK(MyYop_Create(22,23, &yop));
    printf("yop = %x\n", ATX_POINTER_TO_LONG(yop));
    Yop_YopMethod1(yop);

    foo = ATX_CAST(yop, Foo);
    printf("foo = %x\n", ATX_POINTER_TO_LONG(foo));
    Foo_FooMethod1(foo, 56);
    Foo_FooMethod2(foo, 57, 58);
    Foo_DoSomething(foo);

    bar = ATX_CAST(yop, Bar);
    printf("bar = %x\n", ATX_POINTER_TO_LONG(bar));
    Bar_BarMethod1(bar, 88);
    Bar_BarMethod2(bar, 87, 86);
    Bar_DoSomething(bar);

    /* verify that we can get back to yop from foo and bar */
    yop = ATX_CAST(foo, Yop);
    printf("yop = %x\n", ATX_POINTER_TO_LONG(yop));
    yop = ATX_CAST(bar, Yop);
    printf("yop = %x\n", ATX_POINTER_TO_LONG(yop));

    return 0;
}