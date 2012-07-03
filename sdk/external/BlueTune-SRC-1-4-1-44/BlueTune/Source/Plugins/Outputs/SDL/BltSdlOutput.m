/*****************************************************************
|
|   Sdl Output Module
|
|   (c) 2002-2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <SDL/SDL.h>
#import <Cocoa/Cocoa.h>

#include "Atomix.h"
#include "BltConfig.h"
#include "BltSdlOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"
#include "BltPixels.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.sdl")

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} SdlOutputModule;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    BLT_MediaType  expected_media_type;
    BLT_MediaType  media_type;

    SDL_Surface*   screen;
    SDL_Overlay*   yuv_overlay;
    ATX_TimeStamp  next_display_time;
} SdlOutput;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(SdlOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(SdlOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(SdlOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(SdlOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(SdlOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(SdlOutput, BLT_PacketConsumer)

/*----------------------------------------------------------------------
|    SdlOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
SdlOutput_PutPacket(BLT_PacketConsumer* _self,
                    BLT_MediaPacket*    packet)
{
    SdlOutput*                   self = ATX_SELF(SdlOutput, BLT_PacketConsumer);
    unsigned char*               pixel_data = (unsigned char*)BLT_MediaPacket_GetPayloadBuffer(packet);
    const BLT_RawVideoMediaType* media_type;
    unsigned int                 plane;
    
    /* check the media type */
    BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_VIDEO_RAW) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    if (media_type->format != BLT_PIXEL_FORMAT_YV12) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    
    /* resize/create the window and overlay if needed */
    if (self->yuv_overlay == NULL                 || 
        self->yuv_overlay->w != media_type->width ||
        self->yuv_overlay->h != media_type->height) {
        [NSApplication sharedApplication];
        NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
        self->screen = SDL_SetVideoMode(media_type->width, media_type->height, 24, SDL_HWSURFACE | SDL_RESIZABLE);
        if (self->screen == NULL) {
            ATX_LOG_WARNING("SdlOutput_PutPacket - SDL_SetVideoMode() failed");
            [pool release];
            return BLT_FAILURE;
        }
        self->yuv_overlay = SDL_CreateYUVOverlay(media_type->width, media_type->height, SDL_YV12_OVERLAY, self->screen);
        if (self->yuv_overlay == NULL) {
            ATX_LOG_WARNING("SdlOutput_PutPacket - SDL_CreateYUVOverlay() failed");
            [pool release];
            return BLT_FAILURE;
        }
        
        [pool release];
    }
    
    /* transfer the pixels */
    SDL_LockYUVOverlay(self->yuv_overlay);
    for (plane=0; plane<3; plane++) {
        unsigned int   plane_width  = (plane==0?media_type->width:(media_type->width/2));
        unsigned int   plane_height = (plane==0?media_type->height:(media_type->height/2));
        unsigned char* src          = pixel_data+media_type->planes[plane].offset;
        unsigned int   src_pitch    = media_type->planes[plane].bytes_per_line;
        unsigned char* dst          = self->yuv_overlay->pixels[plane==0?0:3-plane];
        unsigned int   dst_pitch    = self->yuv_overlay->pitches[plane==0?0:3-plane];
        while (plane_height--) {
            ATX_CopyMemory(dst, src,  plane_width);
            src += src_pitch;
            dst += dst_pitch;
        }
    }
    SDL_UnlockYUVOverlay(self->yuv_overlay);

	SDL_Rect rect;
    rect.x = 0;
	rect.y = 0;
	rect.w = self->screen->w;
	rect.h = self->screen->h;
	SDL_DisplayYUVOverlay(self->yuv_overlay, &rect);    
    
    {
        ATX_TimeStamp now;
        ATX_System_GetCurrentTimeStamp(&now);
        if (ATX_TimeStamp_IsLaterOrEqual(self->next_display_time, now)) { 
            ATX_TimeInterval delta;
            ATX_TimeStamp_Sub(delta, self->next_display_time, now);
            /* sanity check */
            if (delta.seconds == 0 && delta.nanoseconds > 1000000) {
                ATX_System_Sleep(&delta);
            }
        } else {
            self->next_display_time = now;
        }
        {
            ATX_TimeStamp frame_duration = {0, 41708000};
            ATX_TimeStamp_Add(self->next_display_time, self->next_display_time, frame_duration);
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    SdlOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
SdlOutput_QueryMediaType(BLT_MediaPort*        _self,
                            BLT_Ordinal           index,
                            const BLT_MediaType** media_type)
{
    SdlOutput* self = ATX_SELF(SdlOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|    SdlOutput_CreateWindow
+---------------------------------------------------------------------*/
static BLT_Result
SdlOutput_CreateWindow(SdlOutput* self)
{
    [NSApplication sharedApplication];
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        ATX_LOG_WARNING("SdlOutput_CreateWindow - cannot init SDL");
        [pool release];
        return BLT_FAILURE;
    }

    self->screen = SDL_SetVideoMode(1000, 600, 24, SDL_HWSURFACE | SDL_RESIZABLE);
    if (self->screen == NULL) {
        ATX_LOG_WARNING("SdlOutput_CreateWindow - SDL_SetVideoMode() failed");
        [pool release];
        return BLT_FAILURE;
    }
    self->yuv_overlay = SDL_CreateYUVOverlay(1000, 600, SDL_YV12_OVERLAY, self->screen);
    if (self->yuv_overlay == NULL) {
        ATX_LOG_WARNING("SdlOutput_CreateWindow - SDL_CreateYUVOverlay() failed");
        [pool release];
        return BLT_FAILURE;
    }
    
    [pool release];

    return BLT_SUCCESS;
}
        
/*----------------------------------------------------------------------
|    SdlOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
SdlOutput_Create(BLT_Module*              module,
                 BLT_Core*                core, 
                 BLT_ModuleParametersType parameters_type,
                 BLT_CString              parameters, 
                 BLT_MediaNode**          object)
{
    SdlOutput*        self;
    
    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(SdlOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */

    /* setup the expected media type */
    BLT_MediaType_Init(&self->expected_media_type, BLT_MEDIA_TYPE_ID_VIDEO_RAW);
    /*self->expected_media_type.sample_format = BLT_PCM_SAMPLE_FORMAT_SIGNED_INT_NE;*/

    SdlOutput_CreateWindow(self);
    
    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, SdlOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, SdlOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (self, SdlOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE   (self, SdlOutput, BLT_OutputNode);
    ATX_SET_INTERFACE   (self, SdlOutput, BLT_MediaPort);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    SdlOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
SdlOutput_Destroy(SdlOutput* self)
{
    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                
/*----------------------------------------------------------------------
|   SdlOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
SdlOutput_GetPortByName(BLT_MediaNode*  _self,
                        BLT_CString     name,
                        BLT_MediaPort** port)
{
    SdlOutput* self = ATX_SELF_EX(SdlOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    SdlOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
SdlOutput_GetStatus(BLT_OutputNode*       _self,
                    BLT_OutputNodeStatus* status)
{
    SdlOutput* self = ATX_SELF(SdlOutput, BLT_OutputNode);
    BLT_COMPILER_UNUSED(self);
    
    /* default value */
    status->media_time.seconds     = 0;
    status->media_time.nanoseconds = 0;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(SdlOutput)
    ATX_GET_INTERFACE_ACCEPT_EX(SdlOutput, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(SdlOutput, BLT_BaseMediaNode, ATX_Referenceable)
    ATX_GET_INTERFACE_ACCEPT   (SdlOutput, BLT_OutputNode)
    ATX_GET_INTERFACE_ACCEPT   (SdlOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT   (SdlOutput, BLT_PacketConsumer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(SdlOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(SdlOutput, BLT_MediaPort)
    SdlOutput_GetName,
    SdlOutput_GetProtocol,
    SdlOutput_GetDirection,
    SdlOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(SdlOutput, BLT_PacketConsumer)
    SdlOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(SdlOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    SdlOutput_GetPortByName,
    BLT_BaseMediaNode_Activate,
    BLT_BaseMediaNode_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    BLT_BaseMediaNode_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(SdlOutput, BLT_OutputNode)
    SdlOutput_GetStatus,
    NULL
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(SdlOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   SdlOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
SdlOutputModule_Probe(BLT_Module*              self, 
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
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* the input protocol should be PACKET and the */
            /* output protocol should be NONE              */
            if ((constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.input.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
                (constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol !=
                 BLT_MEDIA_PORT_PROTOCOL_NONE)) {
                return BLT_FAILURE;
            }

            /* the input type should be unknown, or video/raw */
            if (!(constructor->spec.input.media_type->id == 
                  BLT_MEDIA_TYPE_ID_VIDEO_RAW) &&
                !(constructor->spec.input.media_type->id == 
                  BLT_MEDIA_TYPE_ID_UNKNOWN)) {
                return BLT_FAILURE;
            }

            /* the name should be 'sdl:<n>' */
            if (constructor->name == NULL ||
                !ATX_StringsEqualN(constructor->name, "sdl:", 4)) {
                return BLT_FAILURE;
            }

            /* always an exact match, since we only respond to our name */
            *match = BLT_MODULE_PROBE_MATCH_EXACT;

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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(SdlOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(SdlOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(SdlOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(SdlOutputModule, SdlOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(SdlOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    SdlOutputModule_CreateInstance,
    SdlOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define SdlOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(SdlOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_SdlOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("SDL Output", NULL, 0, 
                                 &SdlOutputModule_BLT_ModuleInterface,
                                 &SdlOutputModule_ATX_ReferenceableInterface,
                                 object);
}
