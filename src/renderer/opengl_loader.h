#pragma once

typedef I8 GLbyte;
typedef F32 GLclampf;
typedef I16 GLshort;
typedef U16 GLushort;
typedef void GLvoid;
typedef U32 GLenum;
typedef F32 GLfloat;
typedef I32 GLfixed;
typedef U32 GLuint;
typedef U64 GLuint64;
typedef IMM GLsizeiptr;
typedef IMM GLintptr;
typedef U32 GLbitfield;
typedef I32 GLint;
typedef U8 GLubyte;
typedef U8 GLboolean;
typedef I32 GLsizei;
typedef I32 GLclampx;

#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

#define GL_FALSE                          0x0000
#define GL_TRUE                           0x0001
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_LINE_STRIP                     0x0003
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_NEVER                          0x0200
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408
#define GL_TEXTURE_2D                     0x0DE1
#define GL_CULL_FACE                      0x0B44
#define GL_ALPHA_TEST                     0x0BC0
#define GL_BLEND                          0x0BE2
#define GL_COLOR_LOGIC_OP                 0x0BF2
#define GL_DITHER                         0x0BD0
#define GL_STENCIL_TEST                   0x0B90
#define GL_DEPTH_TEST                     0x0B71
#define GL_NORMALIZE                      0x0BA1
#define GL_MULTISAMPLE                    0x809D
#define GL_NO_ERROR                       0x0000
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505
#define GL_CW                             0x0900
#define GL_CCW                            0x0901
#define GL_MAX_TEXTURE_SIZE               0x0D33
#define GL_DONT_CARE                      0x1100
#define GL_FASTEST                        0x1101
#define GL_NICEST                         0x1102
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_FLOAT                          0x1406
#define GL_FIXED                          0x140C
#define GL_CLEAR                          0x1500
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_PACK_ALIGNMENT                 0x0D05
#define GL_EXTENSIONS                     0x1F03
#define GL_NUM_EXTENSIONS                 0x821D

#define GL_TEXTURE_2D                     0x0DE1
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803

#define GL_RGBA16F                        0x881A
#define GL_RGB16F                         0x881B

#ifdef ANT_PLATFORM_WINDOWS
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_SWAP_METHOD_ARB               0x2007
#define WGL_SWAP_EXCHANGE_ARB             0x2028
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB  0x20A9

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_FLAGS_ARB             0x2094
#define WGL_CONTEXT_DEBUG_BIT_ARB         0x0001

#define ERROR_INVALID_VERSION_ARB         0x2095
#define ERROR_INVALID_PROFILE_ARB         0x2096

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001

#define GLAPI WINAPI

typedef HDC Device_Context;
typedef HGLRC GL_Context;

#else
#endif

#define EXTENSION_CONSTANTS
#include "renderer/opengl_extensions.inl"
#undef EXTENSION_CONSTANTS

//GL_FUNC(RETURN TYPE, NAME, PARAMETERS)
#define ListAllCoreGLFunctions()\
GL_FUNC(void, 	glGetIntegerv,    GLenum name, GLint* params)\
GL_FUNC(GLubyte*, glGetStringi,     GLenum name, GLuint index)\
GL_FUNC(void, 	glClearColor,     GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)\
GL_FUNC(void, 	glClear, 	     GLbitfield mask)\
GL_FUNC(void,     glClearStencil,   GLint s)\
GL_FUNC(void,     glClearDepthf,    GLfloat depth)\
GL_FUNC(void,     glGenTextures,    GLsizei n, GLuint * textures)\
GL_FUNC(void,     glDeleteTextures, GLsizei n, const GLuint* textures)\
GL_FUNC(void,     glBindTexture,    GLenum target, GLuint texture)\
GL_FUNC(void,     glTexParameteri,  GLenum target, GLenum pname, GLint param)\
GL_FUNC(void,     glTexStorage2D,   GLenum target, GLsizei levels, GLenum internalformat, GLsizei width,GLsizei height)\

#define GL_FUNC(ret, name, ...)\
typedef ret (GLAPI *PFN_##name) (__VA_ARGS__);

ListAllCoreGLFunctions();

#undef GL_FUNC

#define GL_EXTENSION_BEGIN(flag, name)
#define GL_EXTENSION_FUNC(ret, name, ...)\
typedef ret (GLAPI *PFN_##name) (__VA_ARGS__);
#define GL_EXTENSION_END()

#include "renderer/opengl_extensions.inl"

#undef GL_EXTENSION_BEGIN
#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_END

struct OpenGL_Extension_Info
{
#define GL_EXTENSION_NAME(flag, name) bool name;
#define FUNCTIONLESS_EXTENSIONS
    
#include "renderer/opengl_extensions.inl"
    
#undef FUNCTIONLESS_EXTENSIONS
#undef GL_EXTENSION_NAME
    
#define GL_EXTENSION_BEGIN(flag, name) bool name;
#define GL_EXTENSION_FUNC(ret, name, ...)
#define GL_EXTENSION_END()
    
#include "renderer/opengl_extensions.inl"
    
#undef GL_EXTENSION_BEGIN
#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_END
};

struct OpenGL_Capabilities
{
    U32 max_texture_array_layers;
};

struct OpenGL_Binding
{
    Module module;
    Process_Handle process_handle;
    Window_Handle window_handle;
    Device_Context device_context;
    GL_Context context;
    
    OpenGL_Extension_Info extension_info;
    OpenGL_Capabilities capabilities;
    
#define GL_FUNC(ret, name, ...) PFN_##name name;
    ListAllCoreGLFunctions()
#undef GL_FUNC
    
#define GL_EXTENSION_BEGIN(flag, name)
#define GL_EXTENSION_FUNC(ret, name, ...) PFN_##name name;
#define GL_EXTENSION_END()
    
#include "renderer/opengl_extensions.inl"
    
#undef GL_EXTENSION_BEGIN
#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_END
};

#ifdef ANT_PLATFORM_WINDOWS
inline bool
GLCreateContext (OpenGL_Binding* binding)
{
    bool succeeded = false;
    
    do
    {
        WNDCLASSEXA dummy_window_class = {};
        dummy_window_class.cbSize        = sizeof(WNDCLASSEXA);
        dummy_window_class.lpfnWndProc   = &DefWindowProcA;
        dummy_window_class.style         = CS_OWNDC;
        dummy_window_class.lpszClassName = "Dummy OpenGL extension loading window";
        dummy_window_class.hInstance     = binding->process_handle;
        
        if (RegisterClassExA(&dummy_window_class) == 0)
        {
            Error("Failed to register window class for the dummy OpenGL extension loading window. Win32 error code: %U.", GetLastError());
            break;
        }
        
        HWND dummy_window = CreateWindowExA(dummy_window_class.style, dummy_window_class.lpszClassName, "Dummy OpenGL extension loading window", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, binding->process_handle, NULL);
        
        if (dummy_window == NULL)
        {
            Error("Failed to create window for the dummy OpenGL extension loading window. Win32 error code: %U.", GetLastError());
            break;
        }
        
        HDC dummy_dc = GetDC(dummy_window);
        
        typedef HGLRC (GLAPI *PFN_wglCreateContext) (HDC arg1);
        PFN_wglCreateContext wglCreateContext = (PFN_wglCreateContext) GetProcAddress(binding->module, "wglCreateContext");
        if (wglCreateContext == NULL)
        {
            Error("Failed to load WGL function 'wglCreateContext'.");
            break;
        }
        
        typedef BOOL (GLAPI *PFN_wglMakeCurrent) (HDC arg1, HGLRC arg2);
        PFN_wglMakeCurrent wglMakeCurrent = (PFN_wglMakeCurrent) GetProcAddress(binding->module, "wglMakeCurrent");
        if (wglMakeCurrent == NULL)
        {
            Error("Failed to load WGL function 'wglMakeCurrent'.");
            break;
        }
        
        typedef BOOL (GLAPI *PFN_wglDeleteContext) (HGLRC arg1);
        PFN_wglDeleteContext wglDeleteContext = (PFN_wglDeleteContext) GetProcAddress(binding->module, "wglDeleteContext");
        if (wglDeleteContext == NULL)
        {
            Error("Failed to load WGL function 'wglDeleteContext'.");
            break;
        }
        
        typedef PROC (GLAPI *PFN_wglGetProcAddress) (LPCSTR arg1);
        PFN_wglGetProcAddress wglGetProcAddress = (PFN_wglGetProcAddress) GetProcAddress(binding->module, "wglGetProcAddress");
        if (wglGetProcAddress == NULL)
        {
            Error("Failed to load WGL function 'wglGetProcAddress'.");
            break;
        }
        
        PIXELFORMATDESCRIPTOR dummy_pixel_format = {};
        dummy_pixel_format.nSize      = sizeof(dummy_pixel_format);
        dummy_pixel_format.nVersion   = 1;
        dummy_pixel_format.iPixelType = PFD_TYPE_RGBA;
        dummy_pixel_format.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
        dummy_pixel_format.cColorBits = 32;
        dummy_pixel_format.cAlphaBits = 8;
        dummy_pixel_format.cDepthBits = 24;
        dummy_pixel_format.iLayerType = PFD_MAIN_PLANE;
        
        I32 dummy_pixel_format_index = ChoosePixelFormat(dummy_dc, &dummy_pixel_format);
        if (SetPixelFormat(dummy_dc, dummy_pixel_format_index, &dummy_pixel_format) != TRUE)
        {
            Error("Failed to set the pixel format for the dummy OpenGL extension loading window. Win32 error code: %U.", GetLastError());
            break;
        }
        
        HGLRC dummy_context = wglCreateContext(dummy_dc);
        if (dummy_context == NULL)
        {
            Error("Failed to create a dummy OpenGL context for extension loading. Win32 error code: %U.", GetLastError());
            break;
        }
        
        if (wglMakeCurrent(dummy_dc, dummy_context))
        {
            // TODO(soimn): query for the WGL_ARB_extension_string with glGetString before loading
            //              wglGetExtensionsStringARB.
            
            typedef const char* (GLAPI *PFN_wglGetExtensionsStringARB) (HDC hdc);
            PFN_wglGetExtensionsStringARB wglGetExtensionsStringARB = (PFN_wglGetExtensionsStringARB) wglGetProcAddress("wglGetExtensionsStringARB");
            
            if (wglGetExtensionsStringARB == NULL)
            {
                Error("Failed to load the WGL extension function 'wglGetExtensionsStringARB'. Win32 error code: %U.", GetLastError());
                break;
            }
            
            {
                bool supports_create_context_arb          = false;
                bool supports_pixel_format_arb            = false;
                bool supports_srgb_framebuffer            = false;
                // bool supports_create_context_no_error_arb = false;
                
                char* extension_string = (char*) wglGetExtensionsStringARB(dummy_dc);
                char* scan = extension_string;
                
                while (scan && *scan)
                {
                    if (supports_srgb_framebuffer 
                        && supports_create_context_arb 
                        // && supports_create_context_no_error_arb 
                        && supports_pixel_format_arb)
                    {
                        break;
                    }
                    
                    while (IsWhitespace(*scan)) ++scan;
                    
                    char* forward_scan = scan;
                    while (*forward_scan && !IsWhitespace(*forward_scan)) ++forward_scan;
                    
                    UMM cutoff = MAX(0, forward_scan - (scan + 1));
                    
                    if (StrCompare(scan, "WGL_EXT_framebuffer_sRGB", cutoff) ||StrCompare(scan, "WGL_ARB_framebuffer_sRGB", cutoff))
                    {
                        supports_srgb_framebuffer = true;
                    }
                    
                    else if (StrCompare(scan, "WGL_ARB_create_context", cutoff))
                    {
                        supports_create_context_arb = true;
                    }
                    
                    /*
                    else if (StrCompare(scan, "WGL_ARB_create_context_no_error", cutoff))
                    {
                    supports_create_context_no_error_arb = true;
                    }
                    */
                    
                    else if (StrCompare(scan, "WGL_ARB_pixel_format", cutoff))
                    {
                        supports_pixel_format_arb = true;
                    }
                    
                    scan = forward_scan;
                }
                
                if (!supports_create_context_arb)
                {
                    Error("Failed to find the WGL extension WGL_ARB_create_context.");
                    break;
                }
                
                else if (!supports_pixel_format_arb)
                {
                    Error("Failed to find the WGL extension WGL_ARB_pixel_format.");
                    break;
                }
                
                else if (!supports_srgb_framebuffer)
                {
                    Error("Failed to find the WGL extensions WGL_ARB_srgb_framebuffer and WGL_EXT_srgb_framebuffer.");
                    break;
                }
            }
            
            typedef HGLRC (GLAPI *PFN_wglCreateContextAttribsARB) (HDC hDC, HGLRC hShareContext,
                                                                   const int *attribList);
            
            PFN_wglCreateContextAttribsARB wglCreateContextAttribsARB = (PFN_wglCreateContextAttribsARB) wglGetProcAddress("wglCreateContextAttribsARB");
            
            if (wglCreateContextAttribsARB == NULL)
            {
                Error("Failed to load the WGL extension function 'wglCreateContextAttribsARB'. Win32 error code: %U.", GetLastError());
                break;
            }
            
            typedef BOOL (GLAPI *PFN_wglChoosePixelFormatARB) (HDC hdc,
                                                               const int *piAttribIList,
                                                               const FLOAT *pfAttribFList,
                                                               UINT nMaxFormats,
                                                               int *piFormats,
                                                               UINT *nNumFormats);
            
            PFN_wglChoosePixelFormatARB wglChoosePixelFormatARB = (PFN_wglChoosePixelFormatARB) wglGetProcAddress("wglChoosePixelFormatARB");
            
            if (wglChoosePixelFormatARB == NULL)
            {
                Error("Failed to load the WGL extension function 'wglChoosePixelFormatARB'. Win32 error code: %U.", GetLastError());
                break;
            }
            
            I32 attribute_list[] =
            {
                WGL_DRAW_TO_WINDOW_ARB,           GL_TRUE,
                WGL_ACCELERATION_ARB,             WGL_FULL_ACCELERATION_ARB,
                WGL_SUPPORT_OPENGL_ARB,           GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB,            GL_TRUE,
                WGL_PIXEL_TYPE_ARB,               WGL_TYPE_RGBA_ARB,
                WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
                WGL_SWAP_METHOD_ARB,              WGL_SWAP_EXCHANGE_ARB,
                0,
            };
            
            binding->device_context = GetDC(binding->window_handle);
            
            I32 chosen_format;
            U32 num_formats;
            if (wglChoosePixelFormatARB(binding->device_context, attribute_list, NULL,
                                        1, &chosen_format, &num_formats) != TRUE)
            {
                Error("Failed to choose an appropriate pixel format for the OpenGL WGL surface. Win32 error code: %U.", GetLastError());
                break;
            }
            
            if (num_formats == 0)
            {
                Error("Failed to choose an pixel format for the OpenGL WGL surface, as none were appropriate.");
                break;
            }
            
            PIXELFORMATDESCRIPTOR pixel_format = {};
            DescribePixelFormat(binding->device_context, chosen_format,
                                sizeof(PIXELFORMATDESCRIPTOR), &pixel_format);
            if (SetPixelFormat(binding->device_context, chosen_format, &pixel_format) != TRUE)
            {
                Error("Failed to set the pixel format for the OpenGL WGL surface. Win32 error code: %U.", GetLastError());
            }
            
            U32 minimum_major_version = 4;
            U32 minimum_minor_version = 5;
            
            I32 context_attributes[] =
            {
                WGL_CONTEXT_MAJOR_VERSION_ARB, (I32) minimum_major_version,
                WGL_CONTEXT_MINOR_VERSION_ARB, (I32) minimum_minor_version,
#ifdef ANT_DEBUG
                WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };
            
            
            // TODO(soimn): create a no_error context when supported
            binding->context = wglCreateContextAttribsARB(binding->device_context, NULL, context_attributes);
            if (binding->context == NULL)
            {
                U32 error_code = GetLastError();
                
                switch (error_code)
                {
                    case ERROR_INVALID_VERSION_ARB:
                    case ERROR_INVALID_PROFILE_ARB:
                    Error("Failed to create a modern OpenGL context. Old driver, minimum required: %U.%U Core.", minimum_major_version, minimum_minor_version);
                    break;
                    
                    default:
                    Error("Failed to create a modern OpenGL context. Win32 error code: %U.", error_code);
                    break;
                }
                
                break;
            }
            
            if (wglMakeCurrent(binding->device_context, binding->context) != TRUE)
            {
                Error("Failed to make th modern OpenGL context current. Win32 error code: %U.", GetLastError());
                break;
            }
            
            wglDeleteContext(dummy_context);
            ReleaseDC(dummy_window, dummy_dc);
            DestroyWindow(dummy_window);
            
            succeeded = true;
        }
        
        else
        {
            Error("Failed to make the dummy OpenGL extension loading context current. Win32 error code: %U.", GetLastError());
            break;
        }
        
    } while(0);
    
    return succeeded;
}

#else
inline bool
GLCreateContext (OpenGL_Binding* binding);
#endif

#ifdef ANT_PLATFORM_WINDOWS

inline bool
GLLoadFunctions (OpenGL_Binding* binding)
{
    bool succeeded = false;
    
    do
    {
        typedef PROC (GLAPI *PFN_wglGetProcAddress) (LPCSTR arg1);
        PFN_wglGetProcAddress wglGetProcAddress = (PFN_wglGetProcAddress) GetProcAddress(binding->module, "wglGetProcAddress");
        if (wglGetProcAddress == NULL)
        {
            Error("Failed to load WGL function 'wglGetProcAddress'.");
            break;
        }
        
        typedef const char* (GLAPI *PFN_wglGetExtensionsStringARB) (HDC hdc);
        PFN_wglGetExtensionsStringARB wglGetExtensionsStringARB = (PFN_wglGetExtensionsStringARB) wglGetProcAddress("wglGetExtensionsStringARB");
        
        if (wglGetExtensionsStringARB == NULL)
        {
            Error("Failed to load the WGL extension function 'wglGetExtensionsStringARB'. Win32 error code: %U.", GetLastError());
            break;
        }
        
        // Core functions
        
#define GL_FUNC(ret, name, ...)\
        binding->##name = (PFN_##name) wglGetProcAddress(#name);\
        if (binding->##name == NULL && (binding->##name = (PFN_##name) GetProcAddress(binding->module, #name)) == NULL)\
        {\
            Error("Failed to load the core OpenGL function '%s'. Win32 error code: %U.", #name, GetLastError());\
            break;\
        }
        
        ListAllCoreGLFunctions();
        
#undef GL_FUNC
        
        I32 extension_count;
        binding->glGetIntegerv(GL_NUM_EXTENSIONS, &extension_count);
        
        if (binding->glGetStringi(GL_EXTENSIONS, 0) == 0)
        {
            Error("Failed to query OpenGL extension string.");
            break;
        }
        
        for (I32 i = 0; i < extension_count; ++i)
        {
            const char* extension_name = (const char*) binding->glGetStringi(GL_EXTENSIONS, i);
            
#define GL_EXTENSION_NAME(flag, name)\
            if (StrCompare(#name, extension_name))\
            {\
                binding->extension_info.##name = true;\
                continue;\
            }
#define FUNCTIONLESS_EXTENSIONS
            
#include "renderer/opengl_extensions.inl"
            
#undef FUNCTIONLESS_EXTENSIONS
#undef GL_EXTENSION_NAME
            
#define GL_EXTENSION_BEGIN(flag, name)\
            if (StrCompare(#name, extension_name))\
            {\
                binding->extension_info.##name = true;\
                continue;\
            }
#define GL_EXTENSION_FUNC(ret, name, ...)
#define GL_EXTENSION_END()
            
#include "renderer/opengl_extensions.inl"
            
#undef GL_EXTENSION_BEGIN
#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_END
        }
        
#define EXT_REQUIRED 1
#define EXT_OPTIONAL 0
        
#define GL_EXTENSION_NAME(flag, name)\
        if (flag && binding->extension_info.##name == false)\
        {\
            Error("Required OpenGL extension '%s' not supported by the device.", #name);\
            break;\
        }
        
#define FUNCTIONLESS_EXTENSIONS
#include "renderer/opengl_extensions.inl"
#undef FUNCTIONLESS_EXTENSIONS
#undef GL_EXTENSION_NAME
        
#define GL_EXTENSION_BEGIN(flag, name)\
        if (flag && binding->extension_info.##name == false)\
        {\
            Error("Required OpenGL extension '%s' not supported by the device.", #name);\
            break;\
        }\
        \
        else\
        {\
            const char* extension_name = #name;
#define GL_EXTENSION_FUNC(ret, name, ...)\
            binding->##name = (PFN_##name) wglGetProcAddress(#name);\
            if (binding->##name == NULL)\
            {\
                Error("Failed to load the extended OpenGL function '%s', from the extension '%s'. Win32 error code: %U.", #name, extension_name, GetLastError());\
                break;\
            }
#define GL_EXTENSION_END()\
        }
        
#include "renderer/opengl_extensions.inl"
        
#undef EXT_REQUIRED
#undef EXT_OPTIONAL
#undef GL_EXTENSION_BEGIN
#undef GL_EXTENSION_FUNC
#undef GL_EXTENSION_END
        
        succeeded = true;
    } while(0);
    
    return succeeded;
}

#else
inline bool
GLLoadFunctions (OpenGL_Binding* binding);
#endif

inline bool
GLLoad (OpenGL_Binding* binding, Process_Handle process_handle, Window_Handle window_handle)
{
    bool succeeded = false;
    
    ZeroStruct(binding);
    
#ifdef ANT_PLATFORM_WINDOWS
    binding->module = LoadLibraryW(L"opengl32.dll");
#else
#endif
    
    binding->process_handle = process_handle;
    binding->window_handle  = window_handle;
    
    do
    {
        BREAK_ON_FALSE(GLCreateContext(binding));
        BREAK_ON_FALSE(GLLoadFunctions(binding));
        
        // Get capabilities
        binding->glGetIntegerv(0x88FF, (GLint*) &binding->capabilities.max_texture_array_layers);
        
        succeeded = true;
    } while(0);
    
    return succeeded;
}

#undef ListAllCoreGLFunctions