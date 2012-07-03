/*****************************************************************
|
|   BlueTune - Test Input Module
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTestInput.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltModule.h"
#include "BltPacketProducer.h"
#include "BltCommonMediaTypes.h"
#include "BltPixels.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.inputs.test")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} TestInputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);
    
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_MediaType* media_type;
    BLT_UInt32     frame_count;
} TestInput;

/*----------------------------------------------------------------------
|    forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(TestInputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(TestInput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(TestInput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(TestInput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(TestInput, BLT_PacketProducer)
static BLT_Result TestInput_Destroy(TestInput* self);

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
static const struct {
    unsigned char pixels[8][8];
} TestDigits[10] = { 
    {{
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0}
    }},
    {{
        {0,0,0,0,1,0,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,1,0,1,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,1,1,1,0,0}
    }},
    {{
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,0,0,1,0,0},
        {0,0,0,1,1,0,0,0},
        {0,0,1,0,0,0,0,0},
        {0,1,0,0,0,0,0,0},
        {0,1,1,1,1,1,1,0},
    }},
    {{
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,1,1,1,0,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0}
    }},
    {{
        {0,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0},
        {0,0,1,0,0,0,0,0},
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0}
    }},
    {{
        {0,1,1,1,1,1,1,0},
        {0,1,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,0},
        {0,1,1,1,1,1,0,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0}
    }},
    {{
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,0,0},
        {0,1,0,0,0,0,0,0},
        {0,1,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0}
    }},
    {{
        {0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,0,0,1,0,0},
        {0,0,0,0,0,1,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,0,1,0,0,0},
        {0,0,0,1,0,0,0,0},
        {0,0,0,1,0,0,0,0}
    }},
    {{
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0}
    }},
    {{
        {0,0,1,1,1,1,0,0},
        {0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,1,0},
        {0,0,0,0,0,0,1,0},
        {0,0,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0},
        {0,0,1,1,1,1,0,0}
    }}
};

/*----------------------------------------------------------------------
|    TestInput_Create
+---------------------------------------------------------------------*/
static BLT_Result
TestInput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_AnyConst             parameters, 
                 BLT_MediaNode**          object)
{
    TestInput*                input;
    BLT_MediaNodeConstructor* constructor = 
        (BLT_MediaNodeConstructor*)parameters;
    BLT_Result                result;

    ATX_LOG_FINE("TestInput::Create");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    input = (TestInput*)ATX_AllocateZeroMemory(sizeof(TestInput));
    if (input == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(input, BLT_BaseMediaNode), module, core);
    
    /* setup the media type */
    {
        BLT_RawVideoMediaType* media_type = (BLT_RawVideoMediaType*)ATX_AllocateMemory(sizeof(*media_type));
        if (media_type == NULL) {
            result = BLT_ERROR_OUT_OF_MEMORY;
            goto failure;
        }
        BLT_RawVideoMediaType_Init(media_type);
        media_type->width  = 320;
        media_type->height = 240;
        media_type->format = BLT_PIXEL_FORMAT_YV12;
        media_type->planes[0].bytes_per_line = 320;
        media_type->planes[0].offset         = 0;
        media_type->planes[1].bytes_per_line = 160;
        media_type->planes[1].offset         = 320*240;
        media_type->planes[2].bytes_per_line = 160;
        media_type->planes[2].offset         = media_type->planes[1].offset+160*120;
        input->media_type = &media_type->base;        
    }
    
    /* construct reference */
    ATX_SET_INTERFACE_EX(input, TestInput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(input, TestInput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (input, TestInput, BLT_MediaPort);
    ATX_SET_INTERFACE   (input, TestInput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(input, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;

failure:
    TestInput_Destroy(input);
    object = NULL;
    return result;
}

/*----------------------------------------------------------------------
|    TestInput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
TestInput_Destroy(TestInput* self)
{
    ATX_LOG_FINE("TestInput::Destroy");

    /* free the media type extensions */
    BLT_MediaType_Free(self->media_type);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   TestInput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
TestInput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    TestInput* self = ATX_SELF_EX(TestInput, BLT_BaseMediaNode, BLT_MediaNode);
    if (ATX_StringsEqual(name, "output")) {
        /* we implement the BLT_MediaPort interface ourselves */
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    TestInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
TestInput_QueryMediaType(BLT_MediaPort*        _self,
                         BLT_Ordinal           index,
                         const BLT_MediaType** media_type)
{
    TestInput* self = ATX_SELF(TestInput, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = self->media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   TestInput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
TestInput_GetPacket(BLT_PacketProducer* _self,
                    BLT_MediaPacket**   packet)
{
    TestInput*     self = ATX_SELF(TestInput, BLT_PacketProducer);
    BLT_Size       packet_size;
    unsigned char* packet_payload;
    BLT_Result     result;

    /* create an empty packet */
    *packet = NULL;
    packet_size = (320*240)*2;
    result = BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core, 
                                        packet_size,
                                        self->media_type,
                                        packet);
    if (BLT_FAILED(result)) return result;
    BLT_MediaPacket_SetPayloadSize(*packet, packet_size);
    packet_payload = BLT_MediaPacket_GetPayloadBuffer(*packet);
    
    /* fill the packet */
    ATX_SetMemory(packet_payload, 0, packet_size);
    {
        unsigned int row,col;
        unsigned int frame_count = self->frame_count;
        unsigned char* y = packet_payload;
        unsigned char* u = packet_payload+(320*240);
        unsigned char* v = u+(160*120);
        unsigned int   digit_offset = 0;
        for (row=0; row<240; row++) {
            for (col=0; col<320; col++) {
                unsigned char py = row+col+frame_count;
                unsigned char pu = frame_count+col+2*row;
                unsigned char pv = frame_count+col+3*row;
                y[col+row*320] = py;
                if ((col%2 == 0) && (row%2 == 0)) {
                    u[col/2+(row/2)*160] = pu;
                    v[col/2+(row/2)*160] = pv;
                }
            }
        }
        digit_offset = 6;
        do {
            unsigned int digit = frame_count%10;
            unsigned int dx, dy;
            for (dy=0; dy<8; dy++) {
                for (dx=0; dx<8; dx++) {
                    if (TestDigits[digit].pixels[dy][dx]) {
                        for (row=32+4*dy; row<32+4*(dy+1); row++) {
                            for (col=32+4*(digit_offset*8+dx); col<32+4*(digit_offset*8+dx+1); col++) {
                                y[col+row*320] = 0;
                                if ((col%2 == 0) && (row%2 == 0)) {
                                    u[col/2+(row/2)*160] = 0;
                                    v[col/2+(row/2)*160] = 0;
                                }
                            }
                        }
                    }
                }
            }
            digit_offset--;
            frame_count /= 10;
        } while (frame_count);
    }
    self->frame_count++;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(TestInput)
    ATX_GET_INTERFACE_ACCEPT_EX(TestInput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(TestInput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (TestInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (TestInput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(TestInput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    TestInput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(TestInput, 
                                         "output", 
                                         PACKET, 
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(TestInput, BLT_MediaPort)
    TestInput_GetName,
    TestInput_GetProtocol,
    TestInput_GetDirection,
    TestInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(TestInput, BLT_PacketProducer)
    TestInput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(TestInput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   TestInputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
TestInputModule_Probe(BLT_Module*              self, 
                      BLT_Core*                core,
                      BLT_ModuleParametersType parameters_type,
                      BLT_AnyConst             parameters,
                      BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

            /* we need a file name */
            if (constructor->name == NULL) return BLT_FAILURE;

            /* the input protocol should be NONE, and the output */
            /* protocol should be PACKET                         */
            if ((constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* check the name */
            if (ATX_StringsEqualN(constructor->name, "test:", 5)) {
                /* this is an exact match for us */
                *match = BLT_MODULE_PROBE_MATCH_EXACT;
            } else {
                return BLT_FAILURE;
            }

            ATX_LOG_FINE_1("TestInputModule::Probe - Ok [%d]", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(TestInputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(TestInputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(TestInputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(TestInputModule, TestInput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(TestInputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    TestInputModule_CreateInstance,
    TestInputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define TestInputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(TestInputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_TestInputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("Test Input", NULL, 0,
                                 &TestInputModule_BLT_ModuleInterface,
                                 &TestInputModule_ATX_ReferenceableInterface,
                                 object);
}
