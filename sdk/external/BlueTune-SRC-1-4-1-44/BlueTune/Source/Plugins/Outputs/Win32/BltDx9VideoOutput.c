/*****************************************************************
|
|   Dx9 Video Output Module
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
#include "BltDx9VideoOutput.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltCore.h"
#include "BltPacketConsumer.h"
#include "BltMediaPacket.h"
#include "BltPixels.h"
#include "BltTime.h"
#include "BltSynchronization.h"

#include <windows.h>
#if defined(BLT_DEBUG)
#define D3D_DEBUG_INFO
#endif
#include <d3d9.h>

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.outputs.win32.dx9")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_DX9_VIDEO_OUTPUT_WINDOW_CLASS       TEXT("BlueTune Video")
#define BLT_DX9_VIDEO_OUTPUT_WINDOW_NAME        TEXT("BlueTune Video")
#define BLT_DX9_VIDEO_OUTPUT_FULLSCREEN_WINDOW_CLASS       TEXT("BlueTune Fullscreen Video")
#define BLT_DX9_VIDEO_OUTPUT_FULLSCREEN_WINDOW_NAME        TEXT("BlueTune Fullscreen Video")
#define BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE 4
#define BLT_DX9_VIDEO_OUTPUT_FVF_CUSTOM_VERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
#define BLT_DX9_VIDEO_OUTPUT_MAX_DELAY           2000000000 /* 2 seconds */
#define BLT_DX9_VIDEO_DEFAULT_PRESENTATION_LATENCY  5000000 /* 5 ms      */
#define BLT_DX9_VIDEO_DEFAULT_SLEEP_TIME           10000000 /* 10 ms     */
#define BLT_DX9_VIDEO_MIN_SLEEP_TIME                5000000 /* 5 ms      */
#define BLT_DX9_VIDEO_MAX_SLEEP_TIME               10000000 /* 10 ms     */
#define BLT_DX9_VIDEO_UNDERFLOW_THRESHOLD         200000000 /* 200 ms    */

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);
} Dx9VideoOutputModule;

typedef struct {
	BLT_Boolean is_vista_or_later;
} Dx9VideoOutput_PlatformInfo;

typedef struct {
    LPDIRECT3DSURFACE9 d3d_surface;
    BLT_UInt64         display_time;
} Dx9VideoOutput_Picture;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(ATX_PropertyListener);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_SyncSlave);

    /* members */
	Dx9VideoOutput_PlatformInfo platform;
    HWND                        window;
    UINT                        picture_width;
    UINT                        picture_height;
    Dx9VideoOutput_Picture      pictures[BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE];
    volatile UINT               cur_picture, num_pictures;
    D3DFORMAT                   d3d_source_format;
    D3DFORMAT                   d3d_target_format;
    HMODULE                     d3d_library;
    LPDIRECT3D9                 d3d_object;
    LPDIRECT3DDEVICE9           d3d_device;
    LPDIRECT3DTEXTURE9          d3d_texture;
    LPDIRECT3DVERTEXBUFFER9     d3d_vertex_buffer;
    BLT_MediaType               expected_media_type;
    BLT_MediaType               media_type;

    /* fullscreen support */
    BLT_Boolean                 in_fullscreen;
    HWND                        fullscreen_window;

    /* property listener */
    ATX_PropertyListenerHandle  property_listener_handle;

    /* A/V sync support */
    BLT_TimeSource*             time_source;
    BLT_SyncMode                sync_mode;
    double                      timer_frequency;
    BLT_Boolean                 underflow;

    // video output thread vars
    HANDLE                      render_thread;
    volatile ATX_Boolean        time_to_quit;
    CRITICAL_SECTION            render_crit_section;
} Dx9VideoOutput;

typedef struct
{
    FLOAT    x,y,z;   /* vertex position     */
    FLOAT    rhw;     /* eye distance        */
    D3DCOLOR dcolor;  /* diffuse color       */
    FLOAT    u, v;    /* texture coordinates */
} BLT_Dx9VideoOutput_CustomVertex;

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutputModule, BLT_Module)

ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, BLT_MediaNode)
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, ATX_Referenceable)
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, BLT_OutputNode)
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, BLT_MediaPort)
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, BLT_PacketConsumer)
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, BLT_SyncSlave)
ATX_DECLARE_INTERFACE_MAP(Dx9VideoOutput, ATX_PropertyListener)


/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
static BLT_Result Dx9VideoOutput_Destroy(Dx9VideoOutput* self);
static BLT_Result Dx9VideoOutput_Reshape(Dx9VideoOutput* self,
                                         unsigned int    wnd_width,
                                         unsigned int    wnd_height,
                                         unsigned int    pic_width,
                                         unsigned int    pic_height);
static BLT_Result Dx9VideoOutput_ResetDevice(Dx9VideoOutput* self,
                                             unsigned int    wnd_width,
                                             unsigned int    wnd_height,
                                             unsigned int    pic_width,
                                             unsigned int    pic_height);

/*----------------------------------------------------------------------
|   Dx9VideoOutput_GetPlatformInfo
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_GetPlatformInfo(Dx9VideoOutput_PlatformInfo* info)
{
	OSVERSIONINFO os_info;
	os_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

	/* default values */
	ATX_SetMemory(info, 0, sizeof(*info));

	/* OS version */
	if (GetVersionEx(&os_info)) {
		if (os_info.dwMajorVersion > 5) {
			info->is_vista_or_later = BLT_TRUE;
		}
	} else {
		ATX_LOG_WARNING("Dx9VideoOutput::GetPlatformInfo - GetVersionEx failed");
		return BLT_FAILURE;
	}

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_WindowProc
+---------------------------------------------------------------------*/
static LRESULT CALLBACK 
Dx9VideoOutput_WindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    switch (message) {
      case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
      }
      case WM_WINDOWPOSCHANGED: {
        ATX_LOG_FINE("WM_WINDOWPOSCHANGED");
        break;
      }
      case WM_SIZE: {
          ATX_LOG_FINE("WM_SIZE");
          break;
      }
      case WM_SIZING: {
          ATX_LOG_FINE("WM_SIZING");
          break;
      }
    }

    return DefWindowProc(window, message, w_param, l_param);
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_CreateWindow
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_CreateWindow(Dx9VideoOutput* self)
{
    WNDCLASSEX window_class_info;
    HINSTANCE  module_instance = GetModuleHandle(NULL);

    /* register a class for our window */
    ZeroMemory(&window_class_info, sizeof(window_class_info));
    window_class_info.cbSize        = sizeof(WNDCLASSEX);
    window_class_info.style         = 0;
    window_class_info.lpfnWndProc   = Dx9VideoOutput_WindowProc;
    window_class_info.hInstance     = module_instance;
    window_class_info.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class_info.lpszClassName = BLT_DX9_VIDEO_OUTPUT_WINDOW_CLASS;
    window_class_info.lpszMenuName  = NULL;

    if (!RegisterClassEx(&window_class_info)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::CreateWindow - RegisterClassEx failed (%d)",
                          GetLastError());
        return BLT_FAILURE;
    }

    self->window = CreateWindowEx(0,
                                  BLT_DX9_VIDEO_OUTPUT_WINDOW_CLASS,
                                  BLT_DX9_VIDEO_OUTPUT_WINDOW_NAME,
                                  WS_OVERLAPPEDWINDOW|WS_SIZEBOX,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 
                                  720+8, 480+27, //720, 480,  // FIXME: change this default
                                  NULL,
                                  NULL,
                                  module_instance,
                                  NULL);
    if (self->window == NULL) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::CreateWindow - CreateWindowEx failed (%d)",
                          GetLastError());
        return BLT_FAILURE;
    }
    ShowWindow(self->window, SW_SHOW);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_FullscreenWindowProc
+---------------------------------------------------------------------*/
static LRESULT CALLBACK 
Dx9VideoOutput_FullscreenWindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    Dx9VideoOutput* self = (Dx9VideoOutput*)(long long)GetWindowLongPtr(window, GWLP_USERDATA);

    switch (message) {
      case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
      }
      case WM_KEYUP: {
        ATX_LOG_INFO_1("key %d", w_param);
                     }
      case WM_WINDOWPOSCHANGED: {
        ATX_LOG_FINE("Dx9VideoOutput_FullscreenWindowProc::WindowProc - WM_WINDOWPOSCHANGED");
        break;
      }
    }

    return DefWindowProc(window, message, w_param, l_param);
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_CreateFullscreenWindow
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_CreateFullscreenWindow(Dx9VideoOutput* self)
{
    WNDCLASSEX window_class_info;
    HINSTANCE  module_instance = GetModuleHandle(NULL);
    int window_width, window_height;
    window_width = GetSystemMetrics(SM_CXSCREEN);
    window_height = GetSystemMetrics(SM_CYSCREEN);

    /* register a class for our window */
    ZeroMemory(&window_class_info, sizeof(window_class_info));
    window_class_info.cbSize        = sizeof(WNDCLASSEX);
    window_class_info.style         = 0;
    window_class_info.lpfnWndProc   = Dx9VideoOutput_FullscreenWindowProc;
    window_class_info.hInstance     = module_instance;
    window_class_info.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    window_class_info.lpszClassName = BLT_DX9_VIDEO_OUTPUT_FULLSCREEN_WINDOW_CLASS;
    window_class_info.lpszMenuName  = NULL;

    if (!RegisterClassEx(&window_class_info)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::CreateFullscreenWindow - RegisterClassEx failed (%d)",
                          GetLastError());
        return BLT_FAILURE;
    }

    self->fullscreen_window = CreateWindowEx(WS_EX_TOOLWINDOW,
                                  BLT_DX9_VIDEO_OUTPUT_FULLSCREEN_WINDOW_CLASS,
                                  BLT_DX9_VIDEO_OUTPUT_FULLSCREEN_WINDOW_NAME,
                                  WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 
                                  window_width, window_height,
                                  NULL,
                                  NULL,
                                  module_instance,
                                  NULL);
    if (self->fullscreen_window == NULL) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::CreateFullscreenWindow - CreateWindowEx failed (%d)",
                          GetLastError());
        return BLT_FAILURE;
    }

    {
        // send pointer to self to new window
        LONG_PTR last_val;
        SetLastError(0); // so we can check error
        last_val = SetWindowLongPtr(self->fullscreen_window, GWLP_USERDATA, (LONG)(long long)self);
        if (last_val == 0 && GetLastError() != 0) {
            ATX_LOG_WARNING_1("SetWindowLongPtr failed %d", GetLastError());
        }
    }

    // bring the fullscreen window forward
    SetForegroundWindow(self->fullscreen_window);
    {
        BOOL worked = BringWindowToTop(self->fullscreen_window);
        if (!worked) {
            ATX_LOG_WARNING("no z order");
        }
    }
    ShowWindow(self->fullscreen_window, SW_SHOW);
    SetWindowPos(self->fullscreen_window, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE| SWP_NOMOVE | SWP_SHOWWINDOW);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_GetHostTime
+---------------------------------------------------------------------*/
static BLT_UInt64
Dx9VideoOutput_GetHostTime(Dx9VideoOutput* self)
{
    if (self->timer_frequency == 0.0) {
        // performance counter not available, use system clock
        ATX_TimeStamp now;
        BLT_TimeStamp bnow;
        ATX_System_GetCurrentTimeStamp(&now);
        bnow.seconds     = now.seconds;
        bnow.nanoseconds = now.nanoseconds;
        return BLT_TimeStamp_ToNanos(bnow);
    } else {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (BLT_UInt64)(1000000000.0*(((double)now.QuadPart)/self->timer_frequency));
    }
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_GetFormatName
+---------------------------------------------------------------------*/
static const char* 
Dx9VideoOutput_GetFormatName(D3DFORMAT format)
{
    switch (format) {
        case D3DFMT_UNKNOWN:      return "D3DFMT_UNKNOWN";
        case D3DFMT_R8G8B8:       return "D3DFMT_R8G8B8";
        case D3DFMT_A8R8G8B8:     return "D3DFMT_A8R8G8B8";
        case D3DFMT_X8R8G8B8:     return "D3DFMT_X8R8G8B8";
        case D3DFMT_R5G6B5:       return "D3DFMT_R5G6B5";
        case D3DFMT_X1R5G5B5:     return "D3DFMT_X1R5G5B5";
        case D3DFMT_A1R5G5B5:     return "D3DFMT_A1R5G5B5";
        case D3DFMT_A4R4G4B4:     return "D3DFMT_A4R4G4B4";
        case D3DFMT_R3G3B2:       return "D3DFMT_R3G3B2";
        case D3DFMT_A8:           return "D3DFMT_A8";
        case D3DFMT_A8R3G3B2:     return "D3DFMT_A8R3G3B2";
        case D3DFMT_X4R4G4B4:     return "D3DFMT_X4R4G4B4";
        case D3DFMT_A2B10G10R10:  return "D3DFMT_A2B10G10R10";
        case D3DFMT_A8B8G8R8:     return "D3DFMT_A8B8G8R8";
        case D3DFMT_X8B8G8R8:     return "D3DFMT_X8B8G8R8";
        case D3DFMT_G16R16:       return "D3DFMT_G16R16";
        case D3DFMT_A2R10G10B10:  return "D3DFMT_A2R10G10B10";
        case D3DFMT_A16B16G16R16: return "D3DFMT_A16B16G16R16";
        case D3DFMT_A8P8:         return "D3DFMT_A8P8";
        case D3DFMT_P8:           return "D3DFMT_P8";
        case D3DFMT_L8:           return "D3DFMT_L8";
        case D3DFMT_A8L8:         return "D3DFMT_A8L8";
        case D3DFMT_A4L4:         return "D3DFMT_A4L4";
        case D3DFMT_V8U8:         return "D3DFMT_V8U8";
        case D3DFMT_L6V5U5:       return "D3DFMT_L6V5U5";
        case D3DFMT_X8L8V8U8:     return "D3DFMT_X8L8V8U8";
        case D3DFMT_Q8W8V8U8:     return "D3DFMT_Q8W8V8U8";
        case D3DFMT_V16U16:       return "D3DFMT_V16U16";
        case D3DFMT_A2W10V10U10:  return "D3DFMT_A2W10V10U10";
        case D3DFMT_UYVY:         return "D3DFMT_UYVY";
        case D3DFMT_R8G8_B8G8:    return "D3DFMT_R8G8_B8G8";
        case D3DFMT_YUY2:         return "D3DFMT_YUY2";
        case D3DFMT_G8R8_G8B8:    return "D3DFMT_G8R8_G8B8";
        default:                  return "?";
    }
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_GetDirect3DParams
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_GetDirect3DParams(Dx9VideoOutput*        self, 
                                 D3DPRESENT_PARAMETERS* d3d_params,
                                 unsigned int           pic_width,
                                 unsigned int           pic_height)
{
    D3DDISPLAYMODE d3d_display_mode;
    HRESULT        result;

    /* default values */
    ZeroMemory(d3d_params, sizeof(*d3d_params));

    /* get the adapter's display mode */
    result = IDirect3D9_GetAdapterDisplayMode(self->d3d_object, D3DADAPTER_DEFAULT, &d3d_display_mode);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::GetDirect3DParams - IDirect3D9::GetAdapterDisplayMode() failed (%d)", result);
        return BLT_FAILURE;
    }
    ATX_LOG_FINE_5("Dx9VideoOutput::GetDirect3DParams - display mode size=%d*%d, format=%d:%s, refresh=%d",
                   d3d_display_mode.Width,
                   d3d_display_mode.Height,
                   d3d_display_mode.Format,
                   Dx9VideoOutput_GetFormatName(d3d_display_mode.Format),
                   d3d_display_mode.RefreshRate);

    /* setup the parameters */
    d3d_params->Flags                  = D3DPRESENTFLAG_VIDEO;
    d3d_params->Windowed               = TRUE;
    if (self->in_fullscreen) {
        //d3d_params->hDeviceWindow      = self->fullscreen_window;
        d3d_params->hDeviceWindow      = self->window;
    } else {
        d3d_params->hDeviceWindow      = self->window;
    }
    d3d_params->BackBufferWidth        = pic_width;
    d3d_params->BackBufferHeight       = pic_height;
    d3d_params->SwapEffect             = D3DSWAPEFFECT_COPY;
    d3d_params->MultiSampleType        = D3DMULTISAMPLE_NONE;
    d3d_params->PresentationInterval   = D3DPRESENT_INTERVAL_DEFAULT;
    d3d_params->BackBufferFormat       = D3DFMT_UNKNOWN;
    d3d_params->BackBufferCount        = 1;
    d3d_params->EnableAutoDepthStencil = FALSE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_SelectTextureFormat
+---------------------------------------------------------------------*/
static D3DFORMAT 
Dx9VideoOutput_SelectTextureFormat(Dx9VideoOutput* self)
{
    const D3DFORMAT formats[] = { D3DFMT_UYVY, D3DFMT_YUY2, D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8, D3DFMT_R8G8B8, D3DFMT_R5G6B5, D3DFMT_X1R5G5B5 };
    unsigned int    format_count = sizeof(formats)/sizeof(formats[0]);
    unsigned int    i;

    /* select the first available format from the list */
    for (i=0; i<format_count; i++) {
        HRESULT   result;
        D3DFORMAT format = formats[i];
        
        /* ask the device if it can create a surface with that format */
        result = IDirect3D9_CheckDeviceFormat(self->d3d_object,
                                              D3DADAPTER_DEFAULT,
                                              D3DDEVTYPE_HAL,
                                              self->d3d_target_format,
                                              0,
                                              D3DRTYPE_SURFACE,
                                              format);
        if (result == D3DERR_NOTAVAILABLE) continue;
        if (FAILED(result)) {
            ATX_LOG_WARNING_1("Dx9VideoOutput::SelectTextureFormat - IDirect3D9::CheckDeviceFormat failed (%d)", result);
            return D3DFMT_UNKNOWN;
        }

        /* check if the device can convert from that format to the target */
        result = IDirect3D9_CheckDeviceFormatConversion(self->d3d_object,
                                                        D3DADAPTER_DEFAULT, 
                                                        D3DDEVTYPE_HAL,
                                                        format, 
                                                        self->d3d_target_format);
        if (result == D3DERR_NOTAVAILABLE) continue;
        if (FAILED(result)) {
            ATX_LOG_WARNING_1("Dx9VideoOutput::SelectTextureFormat - IDirect3D9_CheckDeviceFormatConversion::CheckDeviceFormat failed (%d)", result);
            return D3DFMT_UNKNOWN;
        }

        return format;
    }

    /* not found */
    return D3DFMT_UNKNOWN;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_CreateVertexBuffer
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_CreateVertexBuffer(Dx9VideoOutput* self)
{
    BLT_Result hresult;

    /* create the vertex buffer */
    hresult = IDirect3DDevice9_CreateVertexBuffer(self->d3d_device,
        4*sizeof(BLT_Dx9VideoOutput_CustomVertex),
        D3DUSAGE_DYNAMIC|D3DUSAGE_WRITEONLY,
        BLT_DX9_VIDEO_OUTPUT_FVF_CUSTOM_VERTEX,
        D3DPOOL_DEFAULT,
        &self->d3d_vertex_buffer,
        NULL);
    if (FAILED(hresult)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::InitializeDirect3D - IDirect3DDevice9::CreateVertexBuffer failed (%d)", hresult);
        return hresult;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_InitializeDirect3D
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_InitializeDirect3D(Dx9VideoOutput* self)
{
    D3DCAPS9              d3d_caps;
    D3DPRESENT_PARAMETERS d3d_params;
    HRESULT               hresult;
    BLT_Result            bresult;
    LPDIRECT3D9 (WINAPI *d3d_factory)(UINT);

    ATX_LOG_FINE("Dx9VideoOutput::InitializeDirect3D - starting");

    /* load the DLL manually, so we can soft-fail if it is not available */
    self->d3d_library = LoadLibrary(TEXT("D3D9.DLL"));
    if (self->d3d_library == NULL) {
        ATX_LOG_FINE("Dx9VideoOutput::InitializeDirect3D - Direct3D 9 not available");
        return BLT_ERROR_NOT_SUPPORTED;
    }

    /* obtain the factory function */
    d3d_factory = (LPDIRECT3D9 (WINAPI *)(UINT))GetProcAddress(self->d3d_library, "Direct3DCreate9");
    if (d3d_factory == NULL) {
        ATX_LOG_WARNING("Dx9VideoOutput::InitializeDirect3D - cannot find Direct3DCreate9 function in D3D9.DLL");
        return BLT_FAILURE;
    }

    /* create the Direct3D object */
    self->d3d_object = d3d_factory(D3D_SDK_VERSION);
    if (self->d3d_object == NULL) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::InitializeDirect3D - Direct3DCreate9(%d) failed", D3D_SDK_VERSION);
        return BLT_FAILURE;
    }

    /* get Direct3D info and capabilities */
    ZeroMemory(&d3d_caps, sizeof(d3d_caps));
    hresult = IDirect3D9_GetDeviceCaps(self->d3d_object, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &d3d_caps);
    if (FAILED(hresult)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::InitializeDirect3D - IDirect3D9::GetDeviceCaps() failed", hresult);
        return hresult;
    }

    /* get the Direct3D parameters to create the device with */
    bresult = Dx9VideoOutput_GetDirect3DParams(self, &d3d_params, 0, 0);
    if (BLT_FAILED(bresult)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::InitializeDirect3D - Dx9VideoOutput::GetDirect3DParams() failed (%d)", bresult);
        return bresult;
    }
    ATX_LOG_FINE_2("3DParams backbufferwidth=%d, backbufferheight=%d", d3d_params.BackBufferWidth, d3d_params.BackBufferHeight);

    // don't try to create a device with 0 size -- this will make it fail.
    // let's choose some arbitrary number and let it get resized later when we have the correct window dimensions.
    if (d3d_params.BackBufferWidth == 0) {
        d3d_params.BackBufferWidth = 128;
        d3d_params.BackBufferHeight = 96;
    }

    /* create the Direct3D device */
    hresult = IDirect3D9_CreateDevice(self->d3d_object, 
                                      D3DADAPTER_DEFAULT,
                                      D3DDEVTYPE_HAL, 
                                      self->window,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED,
                                      &d3d_params, 
                                      &self->d3d_device);
    if (FAILED(hresult)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::InitializeDirect3D - IDirect3D9::CreateDevice failed (%d)", hresult);
        return hresult;
    }
    
    /* remember the target format */
    self->d3d_target_format = d3d_params.BackBufferFormat;

    /* find the best texture format */
    self->d3d_source_format = Dx9VideoOutput_SelectTextureFormat(self);
    if (self->d3d_source_format == D3DFMT_UNKNOWN) {
        ATX_LOG_WARNING("Dx9VideoOutput::InitializeDirect3D - no compatible format found");
        return BLT_FAILURE;
    }
    ATX_LOG_FINE_2("Dx9VideoOutput::InitializeDirect3D - selected format is %d:%s",
                   self->d3d_source_format,
                   Dx9VideoOutput_GetFormatName(self->d3d_source_format));

    /* set some of the device options */
    IDirect3DDevice9_SetSamplerState(self->d3d_device, 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
    IDirect3DDevice9_SetSamplerState(self->d3d_device, 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
    IDirect3DDevice9_SetSamplerState(self->d3d_device, 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    IDirect3DDevice9_SetSamplerState(self->d3d_device, 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_AMBIENT, D3DCOLOR_XRGB(255,255,255));
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_ZENABLE, D3DZB_FALSE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_LIGHTING, FALSE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_DITHERENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_STENCILENABLE, FALSE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_ALPHATESTENABLE,TRUE);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_ALPHAREF, 0x10);
    IDirect3DDevice9_SetRenderState(self->d3d_device, D3DRS_ALPHAFUNC,D3DCMP_GREATER);
    IDirect3DDevice9_SetTextureStageState(self->d3d_device, 0, D3DTSS_COLOROP,D3DTOP_MODULATE);
    IDirect3DDevice9_SetTextureStageState(self->d3d_device, 0, D3DTSS_COLORARG1,D3DTA_TEXTURE);
    IDirect3DDevice9_SetTextureStageState(self->d3d_device, 0, D3DTSS_COLORARG2,D3DTA_DIFFUSE);
    IDirect3DDevice9_SetTextureStageState(self->d3d_device, 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

    if (BLT_FAILED(hresult = Dx9VideoOutput_CreateVertexBuffer(self))) return hresult;

    ATX_LOG_FINE("Dx9VideoOutput::InitializeDirect3D - done");
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_ReleaseDirect3D
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_ReleaseDirect3D(Dx9VideoOutput* self)
{
    if (self->d3d_vertex_buffer) {
        IDirect3DVertexBuffer9_Release(self->d3d_vertex_buffer);
        self->d3d_vertex_buffer = NULL;
    }
    if (self->d3d_device) {
        IDirect3DDevice9_Release(self->d3d_device);
        self->d3d_device = NULL;
    }
    if (self->d3d_object) {
        IDirect3D9_Release(self->d3d_object);
 		self->d3d_object = NULL;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_DestroyPictures
+---------------------------------------------------------------------*/
static void 
Dx9VideoOutput_DestroyPictures(Dx9VideoOutput* self)
{
    unsigned int i;

    for (i=0; i<BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE; i++) {
        if (self->pictures[i].d3d_surface) {
            IDirect3DSurface9_Release(self->pictures[i].d3d_surface);
            self->pictures[i].d3d_surface = NULL;
        }
    }
    self->picture_width  = 0;
    self->picture_height = 0;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_CreatePictures
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_CreatePictures(Dx9VideoOutput* self,
                              unsigned int    width,
                              unsigned int    height)
{
    unsigned int i;
 
    EnterCriticalSection(&self->render_crit_section);

    /* create new pictures with the right size */
    for (i=0; i<BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE; i++) {
        HRESULT result;

        result = IDirect3DDevice9_CreateOffscreenPlainSurface(
            self->d3d_device,
            width,
            height,
            self->d3d_source_format,
            D3DPOOL_DEFAULT,
            &self->pictures[i].d3d_surface,
            NULL);
        if (FAILED(result)) {
            ATX_LOG_WARNING_1("Dx9VideoOutput::SetPictureSize - IDirect3DDevice9::CreateOffscreenPlainSurface failed (%d)", result);
            Dx9VideoOutput_DestroyPictures(self);
            return BLT_FAILURE;
        }

        /* clear the surface */
        IDirect3DDevice9_ColorFill(self->d3d_device, 
                                   self->pictures[i].d3d_surface, 
                                   NULL, 
                                   D3DCOLOR_ARGB(0xFF, 0, 0, 0));
    }

    /* remember the size */
    self->picture_width  = width;
    self->picture_height = height;
    self->cur_picture = self->num_pictures = 0;

    LeaveCriticalSection(&self->render_crit_section);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_CreateTexture
+---------------------------------------------------------------------*/
static BLT_Result 
Dx9VideoOutput_CreateTexture(Dx9VideoOutput* self, unsigned int width, unsigned int height)
{
    HRESULT result;

    /* create a texture */
    result = IDirect3DDevice9_CreateTexture(self->d3d_device,
                                            width,
                                            height,
                                            1, /* levels */
                                            D3DUSAGE_RENDERTARGET,
                                            self->d3d_target_format,
                                            D3DPOOL_DEFAULT,
                                            &self->d3d_texture,
                                            NULL);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::CreateTexture - IDirect3DDevice9::CreateTexture failed (%d)", result);
        return BLT_FAILURE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_DestroyTexture
+---------------------------------------------------------------------*/
static void 
Dx9VideoOutput_DestroyTexture(Dx9VideoOutput* self)
{
    if (self->d3d_texture) {
        IDirect3DTexture9_Release(self->d3d_texture);
        self->d3d_texture = NULL;
    }
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_Reshape
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_Reshape(Dx9VideoOutput* self,
                       unsigned int    wnd_width,
                       unsigned int    wnd_height,
                       unsigned int    pic_width,
                       unsigned int    pic_height)
{
    float                            dst_x      = 0.0f;
    float                            dst_y      = 0.0f;
    float                            dst_width  = 0.0f;
    float                            dst_height = 0.0f;
    BLT_Dx9VideoOutput_CustomVertex* vertices;
    HRESULT                          result;

    /* check if we have a buffer to work with */
    if (self->d3d_vertex_buffer == NULL) return BLT_SUCCESS;

    /* adjust to match the aspect ratio */
    if (pic_height > 0 && pic_width > 0 && wnd_width > 0 && wnd_height > 0) {
        /* compute the scaling factors and offsets so that the visible aspect ratio     */
        /* always remains the same as the picture aspect ratio.                         */
        /* this method assumes that the picture is composited on a texture of the       */
        /* same size (in pixels) as the picture itself, and the result is then rendered */
        /* on the screen.                                                               */
        float wnd_aspect_ratio = (float)wnd_width / (float)wnd_height;
        float pic_aspect_ratio = (float)pic_width / (float)pic_height;
        if (wnd_aspect_ratio > pic_aspect_ratio) {
            /* we need space on the left and right sides of the picture */
            dst_width  = (float)pic_width * (pic_aspect_ratio / wnd_aspect_ratio);
            dst_height = (float)pic_height;
            dst_x      = ((float)pic_width - dst_width) / 2.0f;
            dst_y      = 0.0f;
        } else {
            /* we need space on the top and bottom sides of the picture */
            dst_width  = (float)pic_width;
            dst_height = (float)(pic_height) * (wnd_aspect_ratio / pic_aspect_ratio);
            dst_x      = 0.0f;
            dst_y      = ((float)pic_height - dst_height) / 2.0f;
        }
        //dst_width = (float)pic_width;
        //dst_height = (float)pic_height;
    } 

    /* lock the vertices */
    result = IDirect3DVertexBuffer9_Lock(self->d3d_vertex_buffer, 0, 0, (void **)(&vertices), D3DLOCK_DISCARD);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::Reshape - IDirect3DVertexBuffer9::Lock failed (%d)", result);
        return BLT_FAILURE;
    }

    /* top-left */
    vertices[0].x       = dst_x;
    vertices[0].y       = dst_y;
    vertices[0].z       = 0.0f;
    vertices[0].dcolor = D3DCOLOR_ARGB(255, 255, 255, 255);
    vertices[0].rhw     = 1.0f;
    vertices[0].u       = 0.0f;
    vertices[0].v       = 0.0f;

    /* top-right */
    vertices[1].x       = dst_x+dst_width;
    vertices[1].y       = dst_y;
    vertices[1].z       = 0.0f;
    vertices[1].dcolor = D3DCOLOR_ARGB(255, 255, 255, 255);
    vertices[1].rhw     = 1.0f;
    vertices[1].u       = 1.0f;
    vertices[1].v       = 0.0f;

    /* bottom-right */
    vertices[2].x       = dst_x+dst_width;
    vertices[2].y       = dst_y+dst_height;
    vertices[2].z       = 0.0f;
    vertices[2].dcolor = D3DCOLOR_ARGB(255, 255, 255, 255);
    vertices[2].rhw     = 1.0f;
    vertices[2].u       = 1.0f;
    vertices[2].v       = 1.0f;

    /* bottom-left */
    vertices[3].x       = dst_x;
    vertices[3].y       = dst_y+dst_height;
    vertices[3].z       = 0.0f;
    vertices[3].dcolor = D3DCOLOR_ARGB(255, 255, 255, 255);
    vertices[3].rhw     = 1.0f;
    vertices[3].u       = 0.0f;
    vertices[3].v       = 1.0f;

    /* unlock the vertices */
    result = IDirect3DVertexBuffer9_Unlock(self->d3d_vertex_buffer);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::Reshape - IDirect3DVertexBuffer9::Lock failed (%d)", result);
        return BLT_FAILURE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_RenderPicture
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_RenderPicture(Dx9VideoOutput* self, Dx9VideoOutput_Picture* picture)
{
    LPDIRECT3DSURFACE9 d3d_dest_surface;
    HRESULT            result = 0;

    /* check that we have something to render */
    if (self->d3d_texture == NULL ||
        self->d3d_vertex_buffer == NULL || 
        picture->d3d_surface == NULL) {
        return BLT_SUCCESS;
    }

    /* then check if we can still use the device */
    result = IDirect3DDevice9_TestCooperativeLevel(self->d3d_device);
    if (FAILED(result)) {
        if (result == D3DERR_DEVICENOTRESET) {
            /* we need to reset the device */
            BLT_Result bresult = Dx9VideoOutput_ResetDevice(self, 0, 0, self->picture_width, self->picture_height);
            if (BLT_FAILED(result)) {
                ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - Dx9VideoOutput::ResetDevice failed (%d)", bresult);
                return bresult;
            }
        } else {
            ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::TestCooperativeLevel failed (%d)", result);
            return BLT_FAILURE;
        }
    }

    /* reshape to the current window */
    {
        RECT            wnd_rect;
        D3DSURFACE_DESC src_surf_desc;
        GetWindowRect(self->window, &wnd_rect);

        /* get dimensions of source surface (which will also be dims of dest surface) */
        result = IDirect3DSurface9_GetDesc(picture->d3d_surface, &src_surf_desc);
        if (FAILED(result)) {
            ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DSurface9::GetDesc failed(%d)", result);
            return BLT_FAILURE;
        }
        Dx9VideoOutput_Reshape(self, 
                               wnd_rect.right-wnd_rect.left, 
                               wnd_rect.bottom-wnd_rect.top,
                               src_surf_desc.Width,
                               src_surf_desc.Height);
    }

    /* clear the buffers */
    result = IDirect3DDevice9_Clear(self->d3d_device, 
                                    0, 
                                    NULL, 
                                    D3DCLEAR_TARGET, 
                                    D3DCOLOR_XRGB(0, 0, 0), 
                                    1.0f, 
                                    0);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::Clear failed (%d)", result);
        return BLT_FAILURE;
    }

    /* get the destination surface to render onto */
    result = IDirect3DTexture9_GetSurfaceLevel(self->d3d_texture, 0, &d3d_dest_surface);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DTexture9::GetSurfaceLevel failed (%d)", result);
        return BLT_FAILURE;
    }

    /* clear surface. TODO: not necessary if dest surface exact right dimensions */
    IDirect3DDevice9_ColorFill(self->d3d_device, 
                               d3d_dest_surface, 
                               NULL, 
                               D3DCOLOR_ARGB(0xFF, 0, 0, 0));

    /* blit the picture onto the destination surface */
    result = IDirect3DDevice9_StretchRect(self->d3d_device, 
                                          picture->d3d_surface, 
                                          NULL, 
                                          d3d_dest_surface, 
                                          NULL, 
                                          D3DTEXF_NONE);

    IDirect3DSurface9_Release(d3d_dest_surface);
    d3d_dest_surface = NULL;
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DTexture9::StretchRect failed (%d)", result);
        return BLT_FAILURE;
    }

    /* begin */
    result = IDirect3DDevice9_BeginScene(self->d3d_device);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::BeginScene failed (%d)", result);
        return BLT_FAILURE;
    }

    result = IDirect3DDevice9_SetTexture(self->d3d_device, 0, (LPDIRECT3DBASETEXTURE9)self->d3d_texture);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::SetTexture failed (%d)", result);
        goto end;
    }

    result = IDirect3DDevice9_SetStreamSource(self->d3d_device, 0, self->d3d_vertex_buffer, 0, sizeof(BLT_Dx9VideoOutput_CustomVertex));
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::SetStreamSource failed (%d)", result);
        goto end;
    }

    result = IDirect3DDevice9_SetVertexShader(self->d3d_device, NULL);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::SetVertexShader failed (%d)", result);
        goto end;
    }

    result = IDirect3DDevice9_SetFVF(self->d3d_device, BLT_DX9_VIDEO_OUTPUT_FVF_CUSTOM_VERTEX);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::SetFVF failed (%d)", result);
        goto end;
    }

    result = IDirect3DDevice9_DrawPrimitive(self->d3d_device, D3DPT_TRIANGLEFAN, 0, 2);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::RenderPicture - IDirect3DDevice9::DrawPrimitive failed (%d)", result);
        goto end;
    }

end:
    /* we're done with the scene */
    IDirect3DDevice9_EndScene(self->d3d_device);
    if (FAILED(result)) {
        return BLT_FAILURE;
    } else {
        return BLT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   Dx9VideoOutput_RenderingThread
+---------------------------------------------------------------------*/
static long WINAPI
Dx9VideoOutput_RenderingThread(Dx9VideoOutput* self)
{
    BLT_Result result;

    while (self->time_to_quit == ATX_FALSE) {
        BLT_UInt64   sleep_time = BLT_DX9_VIDEO_DEFAULT_SLEEP_TIME;
        unsigned int picture_count = 0;
        Dx9VideoOutput_Picture* picture = NULL;

        // only try to render if we have one in the bank
        EnterCriticalSection(&self->render_crit_section);
        picture_count = self->num_pictures;
        picture = &self->pictures[self->cur_picture];
        LeaveCriticalSection(&self->render_crit_section);

        // if a picture is available, check if it is time to show it
        if (picture_count > 0) {
            // get the next picture to display
            BLT_UInt64 now = Dx9VideoOutput_GetHostTime(self);
            ATX_LOG_FINEST_1("now=%lld", now);

            if (picture->display_time <= now+BLT_DX9_VIDEO_DEFAULT_PRESENTATION_LATENCY) {
                // this picture needs to be displayed
                ATX_LOG_FINER_2("rendering pic %d, num in bank: %d", self->cur_picture, self->num_pictures );
                result = Dx9VideoOutput_RenderPicture(self, picture);
                if (BLT_SUCCEEDED(result)) {
                    IDirect3DDevice9_Present(self->d3d_device, NULL, NULL, NULL, NULL);
                }

                // how long did it take us to render that?
                {
                    BLT_UInt64 then = Dx9VideoOutput_GetHostTime(self);
                    BLT_UInt64 delta = then-now;
                    ATX_LOG_FINER_1("render time: %lld", delta);
                }

                // move to the next picture
                EnterCriticalSection(&self->render_crit_section);
                self->cur_picture = (self->cur_picture + 1) % BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE;
                self->num_pictures--;
                LeaveCriticalSection(&self->render_crit_section);

                sleep_time = 0;
            } else {
                sleep_time = picture->display_time-now;
                if (sleep_time > BLT_DX9_VIDEO_DEFAULT_PRESENTATION_LATENCY) {
                    sleep_time -= BLT_DX9_VIDEO_DEFAULT_PRESENTATION_LATENCY;
                }
                if (sleep_time < BLT_DX9_VIDEO_MIN_SLEEP_TIME) {
                    sleep_time = BLT_DX9_VIDEO_MIN_SLEEP_TIME;
                }
            }
        }

        // sleep before looping 
        if (sleep_time) {
#if defined(ATX_CONFIG_ENABLE_LOGGING)
            BLT_UInt64 before = Dx9VideoOutput_GetHostTime(self);
            BLT_UInt64 after;
#endif
            ATX_LOG_FINEST_1("sleeping for %ldms", (DWORD)(sleep_time/1000000));
            Sleep((DWORD)(sleep_time/1000000));

#if defined(ATX_CONFIG_ENABLE_LOGGING)
            after = Dx9VideoOutput_GetHostTime(self);
            ATX_LOG_FINEST_1("slept for %lld nanos", after-before);
#endif
        }
    }

    ATX_LOG_INFO("exiting video output thread");

    return 0;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_Create
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_Create(BLT_Module*              module,
                      BLT_Core*                core,
                      BLT_ModuleParametersType parameters_type,
                      BLT_CString              parameters,
                      BLT_MediaNode**          object)
{
    Dx9VideoOutput*           self;
    BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;
    unsigned long             window_handle = 0;
    BLT_Result                result;

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR ||
        constructor->name == NULL) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }
    if (ATX_StringLength(constructor->name) < 4) {
        ATX_LOG_WARNING_1("invalid name (%s)", constructor->name);
    }

    /* compute the window handle */
    // TODO: 64-bit
    result = ATX_ParseIntegerU(constructor->name+4, &window_handle, ATX_FALSE);
    if (ATX_FAILED(result)) return result;

    /* allocate memory for the object */
    self = ATX_AllocateZeroMemory(sizeof(Dx9VideoOutput));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* setup the expected media type */
    BLT_MediaType_Init(&self->expected_media_type, BLT_MEDIA_TYPE_ID_VIDEO_RAW);

    /* get some info about the platform we are running on */
	Dx9VideoOutput_GetPlatformInfo(&self->platform);

    /* setup the window to render the video */
    if (window_handle) {
        self->window = (HWND)(long long)window_handle;
    } else {
        result = Dx9VideoOutput_CreateWindow(self);
        if (BLT_FAILED(result)) goto fail;
    }

    /* initialize Direct3D */
    result = Dx9VideoOutput_InitializeDirect3D(self);
    if (BLT_FAILED(result)) goto fail;

    /* setup the timer management */
    {
        LARGE_INTEGER timer_frequency;
        if (QueryPerformanceFrequency(&timer_frequency)) {
            self->timer_frequency = (double)timer_frequency.QuadPart;
        } else {
            self->timer_frequency = 0.0;
        }
    }

    // create rendering thread
    {
        BOOL bool_res;
        InitializeCriticalSection(&self->render_crit_section);
        self->time_to_quit = ATX_FALSE;
        self->render_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Dx9VideoOutput_RenderingThread, self, 0, 0);
        bool_res = SetThreadPriority(self->render_thread, THREAD_PRIORITY_HIGHEST);
        if (bool_res) {
            ATX_LOG_FINER("set vid out thread priority to high");
        } else {
            ATX_LOG_WARNING("failed to set thread priority");
        }
    }

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, Dx9VideoOutput, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE   (self, Dx9VideoOutput, BLT_PacketConsumer);
    ATX_SET_INTERFACE   (self, Dx9VideoOutput, BLT_OutputNode);
    ATX_SET_INTERFACE   (self, Dx9VideoOutput, BLT_MediaPort);
    ATX_SET_INTERFACE   (self, Dx9VideoOutput, BLT_SyncSlave);
    ATX_SET_INTERFACE   (self, Dx9VideoOutput, ATX_PropertyListener);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);
    
    return BLT_SUCCESS;

fail:
    Dx9VideoOutput_Destroy(self);   
    return result;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
Dx9VideoOutput_Destroy(Dx9VideoOutput* self)
{
    // kill our display thread
    self->time_to_quit = ATX_TRUE;
    WaitForSingleObject(self->render_thread, INFINITE);
    DeleteCriticalSection(&self->render_crit_section);

    /* close the window if we have one */

    /* destroy the pictures and texture */
    Dx9VideoOutput_DestroyPictures(self);
    Dx9VideoOutput_DestroyTexture(self);

    /* release Direct3D resources */
    Dx9VideoOutput_ReleaseDirect3D(self);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* unload the DLL */
    if (self->d3d_library) {
        FreeLibrary(self->d3d_library);
    }

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}
                
/*----------------------------------------------------------------------
|    Dx9VideoOutput_PutPacket
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_PutPacket(BLT_PacketConsumer* _self,
                         BLT_MediaPacket*    packet)
{
    Dx9VideoOutput*              self = ATX_SELF(Dx9VideoOutput, BLT_PacketConsumer);
    const BLT_RawVideoMediaType* media_type;
    Dx9VideoOutput_Picture*      picture;

    ATX_LOG_FINER("Dx9VideoOutput::PutPacket");

    //if (self->fullscreen_window != NULL)
    //BringWindowToTop(self->fullscreen_window);

    /* check the media type */
    BLT_MediaPacket_GetMediaType(packet, (const BLT_MediaType**)&media_type);
    if (media_type->base.id != BLT_MEDIA_TYPE_ID_VIDEO_RAW) return BLT_ERROR_INVALID_MEDIA_TYPE;
    if (media_type->format != BLT_PIXEL_FORMAT_YV12) return BLT_ERROR_INVALID_MEDIA_TYPE;

    if (media_type->width != self->picture_width || media_type->height != self->picture_height) {
        // inform stream listeners of our size
        ATX_Properties* properties;
        BLT_BaseMediaNode* bmn = &ATX_BASE(self, BLT_BaseMediaNode);
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(bmn->context, &properties))) {
            ATX_PropertyValue property_value;
            property_value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
            property_value.data.integer = media_type->width;
            ATX_Properties_SetProperty(properties,
                BLT_OUTPUT_NODE_WIDTH,
                &property_value);
            property_value.data.integer = media_type->height;
            ATX_Properties_SetProperty(properties,
                BLT_OUTPUT_NODE_HEIGHT,
                &property_value);
        }

        Dx9VideoOutput_ResetDevice(self, 0, 0, media_type->width, media_type->height);
    }

    // figure out where we are in the ring buffer
    EnterCriticalSection(&self->render_crit_section);

    if (self->num_pictures == BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE) {
        // we're full!
        ATX_LOG_WARNING("PutPacket is full!");
        LeaveCriticalSection(&self->render_crit_section);
        return BLT_SUCCESS;
    }

    // stick in next slot
    {
        int slot = (self->cur_picture + self->num_pictures) % BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE;
        picture = &self->pictures[slot];
        ATX_LOG_FINEST_1("new slot... %d", slot);
    }
    LeaveCriticalSection(&self->render_crit_section);

    if (picture->d3d_surface) {
        HRESULT        result;
        D3DLOCKED_RECT d3d_rect;
        result = IDirect3DSurface9_LockRect(picture->d3d_surface, &d3d_rect, NULL, 0);
        if (SUCCEEDED(result)) {
            const unsigned char* pixels = BLT_MediaPacket_GetPayloadBuffer(packet);
            const unsigned char* src_y = pixels+media_type->planes[0].offset;
            const unsigned char* src_u = pixels+media_type->planes[1].offset;
            const unsigned char* src_v = pixels+media_type->planes[2].offset;
            unsigned int row;
            unsigned int col;
        
            if (self->d3d_source_format == D3DFMT_UYVY) {
                unsigned char* dst_line = d3d_rect.pBits;
                for (row=0; row<media_type->height; row++) {
                    unsigned char* dst = dst_line;
                    for (col=0; col<(unsigned int)(media_type->width/2); col++) {
                        *dst++ = src_u[col];
                        *dst++ = src_y[2*col];
                        *dst++ = src_v[col];
                        *dst++ = src_y[2*col+1];
                    }
                    dst_line += d3d_rect.Pitch;
                    src_y += media_type->planes[0].bytes_per_line;
                    if (row&1) {
                        src_u += media_type->planes[1].bytes_per_line;
                        src_v += media_type->planes[2].bytes_per_line;
                    }
                }
            } else if (self->d3d_source_format == D3DFMT_X8R8G8B8) {
                unsigned char* dst_line = d3d_rect.pBits;
                for (row=0; row<media_type->height; row++) {
                    ATX_UInt32* dst = (ATX_UInt32*)dst_line;
                    for (col=0; col<media_type->width; col++) {
                        /* from http://msdn.microsoft.com/en-us/library/ms893078.aspx */
                        int r,g,b;
                        int y = src_y[col];
                        int u = src_u[col/2];
                        int v = src_v[col/2];
                        int C = y - 16;
                        int D = u - 128;
                        int E = v - 128;

                        r = (( 298 * C           + 409 * E + 128) >> 8);
                        g = (( 298 * C - 100 * D - 208 * E + 128) >> 8);
                        b = (( 298 * C + 516 * D           + 128) >> 8);

                        /* clamp values */
                        if (r > 255) r = 255;
                        if (g > 255) g = 255;
                        if (b > 255) b = 255;
                        if (r < 0) r = 0;
                        if (g < 0) g = 0;
                        if (b < 0) b = 0;

                        *dst++ = r<<16 | g<<8 | b;
                    }
                    dst_line += d3d_rect.Pitch;
                    src_y += media_type->planes[0].bytes_per_line;
                    if (row&1) {
                        src_u += media_type->planes[1].bytes_per_line;
                        src_v += media_type->planes[2].bytes_per_line;
                    }
                }
            }
        }

        /* we're done accessing the memory directly */
        result = IDirect3DSurface9_UnlockRect(picture->d3d_surface);

        /* compute the display time for this picture */
        {
            BLT_UInt64 delay = 0;
            if (self->time_source) {
                BLT_TimeStamp master_ts;
                BLT_TimeStamp packet_ts;
                BLT_TimeSource_GetTime(self->time_source, &master_ts);
                packet_ts = BLT_MediaPacket_GetTimeStamp(packet);
                if (BLT_TimeStamp_IsLater(packet_ts, master_ts)) {
                    delay = BLT_TimeStamp_ToNanos(packet_ts)-BLT_TimeStamp_ToNanos(master_ts);
                    self->underflow = BLT_FALSE;
                    ATX_LOG_FINER_3("video is early (%lld) master_ts=%lld, packet_ts=%lld", 
                                    delay, 
                                    BLT_TimeStamp_ToNanos(master_ts),
                                    BLT_TimeStamp_ToNanos(packet_ts)); 

                    /* clamp the delay to a safe maximum */
                    if (delay > BLT_DX9_VIDEO_OUTPUT_MAX_DELAY) {
                        delay = BLT_DX9_VIDEO_OUTPUT_MAX_DELAY;
                        ATX_LOG_FINER("video delay clamped to max");
                    }
                } else {
                    BLT_UInt64 late = BLT_TimeStamp_ToNanos(master_ts)-BLT_TimeStamp_ToNanos(packet_ts);
                    self->underflow =  late>BLT_DX9_VIDEO_UNDERFLOW_THRESHOLD ? BLT_TRUE:BLT_FALSE;
                    ATX_LOG_FINER_3("video is late (%lld) master_ts=%lld, packet_ts=%lld", 
                                    late,
                                    BLT_TimeStamp_ToNanos(master_ts),
                                    BLT_TimeStamp_ToNanos(packet_ts));  

                }
            }
            picture->display_time = Dx9VideoOutput_GetHostTime(self)+delay;
            ATX_LOG_FINER_2("picture display time=%lld (=now+%lld)", picture->display_time, delay);
        }

        /* make the picture available to the rendering thread */
        EnterCriticalSection(&self->render_crit_section);
        self->num_pictures++;
        LeaveCriticalSection(&self->render_crit_section);
    }

    /* FIXME: temporary */
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_QueryMediaType(BLT_MediaPort*        _self,
                              BLT_Ordinal           index,
                              const BLT_MediaType** media_type)
{
    Dx9VideoOutput* self = ATX_SELF(Dx9VideoOutput, BLT_MediaPort);

    if (index == 0) {
        *media_type = (const BLT_MediaType*)&self->expected_media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}
/*----------------------------------------------------------------------
|   Dx9VideoOutput_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_GetPortByName(BLT_MediaNode*  _self,
                             BLT_CString     name,
                             BLT_MediaPort** port)
{
    Dx9VideoOutput* self = ATX_SELF_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(self, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_Activate
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_Activate(BLT_MediaNode* _self, BLT_Stream* stream)
{
    Dx9VideoOutput* self = ATX_SELF_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);

    /* keep a reference to the stream */
    ATX_BASE(self, BLT_BaseMediaNode).context = stream;

    /* listen to settings on the new stream */
    if (stream) {
        ATX_Properties* properties;
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, 
                                                   &properties))) {
            ATX_PropertyValue property;
            ATX_Properties_AddListener(properties, 
                                       BLT_OUTPUT_NODE_FULLSCREEN,
                                       &ATX_BASE(self, ATX_PropertyListener),
                                       &self->property_listener_handle);
            ATX_LOG_FINER("added property listener for fullscreen");

            if (ATX_SUCCEEDED(ATX_Properties_GetProperty(
                    properties,
                    BLT_OUTPUT_NODE_FULLSCREEN,
                    &property)) &&
                property.type == ATX_PROPERTY_VALUE_TYPE_INTEGER) {
            }
        }
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_Deactivate(BLT_MediaNode* _self)
{
    Dx9VideoOutput* self = ATX_SELF_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);

    /* remove our listener */
    if (ATX_BASE(self, BLT_BaseMediaNode).context) {
        ATX_Properties* properties;
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(ATX_BASE(self, BLT_BaseMediaNode).context, 
                                                   &properties))) {
            ATX_Properties_RemoveListener(properties, 
                                          &self->property_listener_handle);
            ATX_LOG_FINER("removed property listener for fullscreen");
        }
    }

    /* we're detached from the stream */
    ATX_BASE(self, BLT_BaseMediaNode).context = NULL;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Dx9VideoOutput_Start
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_Start(BLT_MediaNode* _self)
{
    Dx9VideoOutput* self = ATX_SELF_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);
    (void)self;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Dx9VideoOutput_Stop
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_Stop(BLT_MediaNode* _self)
{
    Dx9VideoOutput* self = ATX_SELF_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);
    (void)self;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   Dx9VideoOutput_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_Seek(BLT_MediaNode* _self, BLT_SeekMode* mode, BLT_SeekPoint* point)
{
    Dx9VideoOutput* self = ATX_SELF_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode);
    BLT_COMPILER_UNUSED(mode);
    BLT_COMPILER_UNUSED(point);

    // flush the picture queue
    //EnterCriticalSection(&self->render_crit_section);
    //self->cur_picture = 0;
    //self->num_pictures = 0;
    //LeaveCriticalSection(&self->render_crit_section);

    // reset some status fields
    self->underflow = BLT_FALSE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_GetStatus
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_GetStatus(BLT_OutputNode*       _self,
                         BLT_OutputNodeStatus* status)
{
    Dx9VideoOutput* self = ATX_SELF(Dx9VideoOutput, BLT_OutputNode);
    
    /* default value */
    status->media_time.seconds     = 0;
    status->media_time.nanoseconds = 0;

    status->flags = 0;

    // don't let them push more pix to us if we are full
    EnterCriticalSection(&self->render_crit_section);
    if (self->num_pictures == BLT_DX9_VIDEO_OUTPUT_PICTURE_QUEUE_SIZE) {
        status->flags |= BLT_OUTPUT_NODE_STATUS_QUEUE_FULL;
    }
    LeaveCriticalSection(&self->render_crit_section);

    // check for underflow conditions
    if (self->underflow) {
        status->flags |= BLT_OUTPUT_NODE_STATUS_UNDERFLOW;
    }

	return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_SetTimeSource
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_SetTimeSource(BLT_SyncSlave*  _self,
                             BLT_TimeSource* source)
{
    Dx9VideoOutput* self = ATX_SELF(Dx9VideoOutput, BLT_SyncSlave);

    self->time_source = source;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_SetSyncMode
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutput_SetSyncMode(BLT_SyncSlave* _self,
                           BLT_SyncMode    mode)
{
    Dx9VideoOutput* self = ATX_SELF(Dx9VideoOutput, BLT_SyncSlave);

    self->sync_mode = mode;
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_ResetDevice
+---------------------------------------------------------------------*/
/*
* the idea here is to call reset on our device.
* this involves breaking down many existing structures and recreating them.
* this might be called switching to fullscreen and back, or when a device changes.
*/
static BLT_Result
Dx9VideoOutput_ResetDevice(Dx9VideoOutput* self, 
                           unsigned int    wnd_width, 
                           unsigned int    wnd_height,
                           unsigned int    pic_width,
                           unsigned int    pic_height)
{
    D3DPRESENT_PARAMETERS d3d_params;
    BLT_Result            result;
    
    /* release current resources */
    Dx9VideoOutput_DestroyPictures(self);
    Dx9VideoOutput_DestroyTexture(self);
    if (self->d3d_vertex_buffer) {
        IDirect3DVertexBuffer9_Release(self->d3d_vertex_buffer);
        self->d3d_vertex_buffer = NULL;
    }

    /* get the Direct3D parameters to update the device */
    result = Dx9VideoOutput_GetDirect3DParams(self, &d3d_params, pic_width, pic_height);
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::GetDirect3DParams() failed (%d)", result);
        return result;
    }
    ATX_LOG_FINE_2("3DParams backbufferwidth=%d, backbufferheight=%d", d3d_params.BackBufferWidth, d3d_params.BackBufferHeight);
    
    /* reset it: this fails unless all memory released as above. use dx in debug mode to trace. */
    result = IDirect3DDevice9_Reset(self->d3d_device, &d3d_params);
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("Dx9VideoOutput::IDirect3DDevice9_Reset() failed (%d)", result);
        return result;
    }

    /* the previous call may have updated the window width and height */
    //wnd_width  = d3d_params.BackBufferWidth;
    //wnd_height = d3d_params.BackBufferHeight;

    /* create a vertex buffer for the texture coordinates */
    result = Dx9VideoOutput_CreateVertexBuffer(self);
    if (FAILED(result)) {
        ATX_LOG_WARNING_1("IDirect3DDevice9::CreateVertexBuffer failed (%d)", result);
        return result;
    }

    /* allocate the new resources */
    Dx9VideoOutput_CreatePictures(self, pic_width, pic_height);
    Dx9VideoOutput_CreateTexture(self, pic_width, pic_height);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    Dx9VideoOutput_OnPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
Dx9VideoOutput_OnPropertyChanged(ATX_PropertyListener*    _self,
                                 ATX_CString              name,
                                 const ATX_PropertyValue* value)
{
    Dx9VideoOutput* self = ATX_SELF(Dx9VideoOutput, ATX_PropertyListener);
    //BLT_Result result;

    ATX_LOG_FINE_1("prop changed: %s", name);

    if (name && 
        (value == NULL || value->type == ATX_PROPERTY_VALUE_TYPE_INTEGER)) {
        if (ATX_StringsEqual(name, BLT_OUTPUT_NODE_FULLSCREEN)) {
            int going_full = 0;
            if (value != NULL) going_full = value->data.integer;
            ATX_LOG_FINE_1("fullscreen prop set to: %d", going_full);
            if (going_full) {
                if (self->in_fullscreen == BLT_TRUE) {
                    ATX_LOG_WARNING("cannot enter full screen mode: already in it");
                    return;
                }
                
                self->in_fullscreen = BLT_TRUE;

                //// open it up!
                //result = Dx9VideoOutput_CreateFullscreenWindow(self);
                //if (BLT_FAILED(result)) {
                //} else {
                //    self->in_fullscreen = BLT_TRUE;
                //    result = Dx9VideoOutput_ResetDevice(self, 
                //                                        0, 0,
                //                                        self->picture_width, 
                //                                        self->picture_height);
                //    if (FAILED(result)) {
                //        ATX_LOG_WARNING_1("reset device failed (%d)", result);
                //    }
                //}
            }
        }
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Dx9VideoOutput)
ATX_GET_INTERFACE_ACCEPT_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode)
ATX_GET_INTERFACE_ACCEPT_EX(Dx9VideoOutput, BLT_BaseMediaNode, ATX_Referenceable)
ATX_GET_INTERFACE_ACCEPT   (Dx9VideoOutput, BLT_OutputNode)
ATX_GET_INTERFACE_ACCEPT   (Dx9VideoOutput, BLT_MediaPort)
ATX_GET_INTERFACE_ACCEPT   (Dx9VideoOutput, BLT_PacketConsumer)
ATX_GET_INTERFACE_ACCEPT   (Dx9VideoOutput, ATX_PropertyListener)
ATX_GET_INTERFACE_ACCEPT   (Dx9VideoOutput, BLT_SyncSlave)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Dx9VideoOutput, ATX_PropertyListener)
    Dx9VideoOutput_OnPropertyChanged,
};

/*----------------------------------------------------------------------
|    BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(Dx9VideoOutput, "input", PACKET, IN)
ATX_BEGIN_INTERFACE_MAP(Dx9VideoOutput, BLT_MediaPort)
    Dx9VideoOutput_GetName,
    Dx9VideoOutput_GetProtocol,
    Dx9VideoOutput_GetDirection,
    Dx9VideoOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_PacketConsumer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Dx9VideoOutput, BLT_PacketConsumer)
    Dx9VideoOutput_PutPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(Dx9VideoOutput, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    Dx9VideoOutput_GetPortByName,
    Dx9VideoOutput_Activate,
    Dx9VideoOutput_Deactivate,
    Dx9VideoOutput_Start,
    Dx9VideoOutput_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    Dx9VideoOutput_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|    BLT_OutputNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Dx9VideoOutput, BLT_OutputNode)
    Dx9VideoOutput_GetStatus,
    NULL
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_SyncSlave interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(Dx9VideoOutput, BLT_SyncSlave)
    Dx9VideoOutput_SetTimeSource,
    Dx9VideoOutput_SetSyncMode
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(Dx9VideoOutput, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   Dx9VideoOutputModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
Dx9VideoOutputModule_Probe(BLT_Module*              self, 
                           BLT_Core*                core,
                           BLT_ModuleParametersType parameters_type,
                           BLT_AnyConst             parameters,
                           BLT_Cardinal*            match)
{
    BLT_COMPILER_UNUSED(self);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR: {
        BLT_MediaNodeConstructor* constructor = (BLT_MediaNodeConstructor*)parameters;

        /* the input protocol should be PACKET and the */
        /* output protocol should be NONE              */
        if ((constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_ANY &&
             constructor->spec.input.protocol  != BLT_MEDIA_PORT_PROTOCOL_PACKET) ||
            (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
             constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_NONE)) {
            return BLT_FAILURE;
        }

        /* the input type should be unknown, or video/raw */
        if (!(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_VIDEO_RAW) &&
            !(constructor->spec.input.media_type->id == BLT_MEDIA_TYPE_ID_UNKNOWN)) {
            return BLT_FAILURE;
        }

        /* the name should be 'dx9:<n>' */
        if (constructor->name == NULL || 
            !ATX_StringsEqualN(constructor->name, "dx9:", 4)) {
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
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(Dx9VideoOutputModule)
    ATX_GET_INTERFACE_ACCEPT_EX(Dx9VideoOutputModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(Dx9VideoOutputModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(Dx9VideoOutputModule, Dx9VideoOutput)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(Dx9VideoOutputModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    BLT_BaseModule_Attach,
    Dx9VideoOutputModule_CreateInstance,
    Dx9VideoOutputModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define Dx9VideoOutputModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(Dx9VideoOutputModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_Result 
BLT_Dx9VideoOutputModule_GetModuleObject(BLT_Module** object)
{
    if (object == NULL) return BLT_ERROR_INVALID_PARAMETERS;

    return BLT_BaseModule_Create("Dx9 Video Output", NULL, 0, 
                                 &Dx9VideoOutputModule_BLT_ModuleInterface,
                                 &Dx9VideoOutputModule_ATX_ReferenceableInterface,
                                 object);
}
