/*
Copyright 2018 Ioannis Makris

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "glatter_config.h"
#include "glatter_platform_headers.h"


#include <assert.h>
#include <inttypes.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifdef GLATTER_HEADER_ONLY
    #define GLATTER_INLINE_OR_NOT inline

    #ifndef GLATTER_PDIR
        #error When GLATTER_HEADER_ONLY is defined, glatter.c should not be compiled.
    #endif

#else
    #define GLATTER_INLINE_OR_NOT
#endif




GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address(const char* function_name)
{
    void* ptr = 0;
#if defined(GLATTER_EGL)
    ptr = (void*) eglGetProcAddress(function_name);
    if (ptr == 0) {
#if defined(_WIN32)
        ptr = (void*) GetProcAddress(GetModuleHandle(TEXT("libEGL.dll")), function_name);
    }
    if (ptr == 0) {
        ptr = (void*) GetProcAddress(GetModuleHandle(TEXT("libGLES_CM.dll")), function_name);
#elif defined (__linux__)
        ptr = (void*) dlsym(dlopen("libEGL.so", RTLD_LAZY), function_name);
    }
    if (ptr == 0) {
        ptr = (void*) dlsym(dlopen("libGLES_CM.so", RTLD_LAZY), function_name);
#else
#error There is no implementation for your platform. Your contribution would be greatly appreciated!
#endif
    }
#elif defined(GLATTER_WGL)
    ptr = (void*) wglGetProcAddress(function_name);
    if (ptr == 0)
        ptr = (void*) GetProcAddress(GetModuleHandle(TEXT("opengl32.dll")), function_name);
#elif defined(GLATTER_GLX)
    ptr = (void*) glXGetProcAddress((const GLubyte*)function_name);
    if (ptr == 0)
        ptr = (void*) dlsym(dlopen("libGL.so", RTLD_LAZY), function_name);
#endif
    return ptr;
}



///////////////////////////
#if defined(GLATTER_GL)  //
///////////////////////////

GLATTER_INLINE_OR_NOT
const char* enum_to_string_GL(GLenum e);

GLATTER_INLINE_OR_NOT
void glatter_check_error_GL(const char* file, int line)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        printf("GLATTER: in '%s'(%d):\n", file, line);
        printf("GLATTER: OpenGL call produced %s error.\n", enum_to_string_GL(err));
    }
}


GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address_GL(const char* function_name)
{
    return glatter_get_proc_address(function_name);
}


////////////////////////////////
#endif // defined(GLATTER_GL) //
////////////////////////////////



////////////////////////////
#if defined(GLATTER_GLX)  //
////////////////////////////

GLATTER_INLINE_OR_NOT
const char* enum_to_string_GLX(GLenum e);

#if !defined(GLATTER_DO_NOT_INSTALL_X_ERROR_HANDLER)
GLATTER_INLINE_OR_NOT
int x_error_handler(Display *dsp, XErrorEvent *error)
{
    char error_string[128];
    XGetErrorText(dsp, error->error_code, error_string, 128);
    printf("X Error: %s\n", error_string);
    return 0;
}
#endif //!defined(GLATTER_DO_NOT_INSTALL_X_ERROR_HANDLER)


GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address_GLX_init(const char* function_name);


GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address_GLX(const char* function_name)
{
#if !defined(GLATTER_DO_NOT_INSTALL_X_ERROR_HANDLER)
    static int initialized = 0;
    if (!initialized) {
        XSetErrorHandler(x_error_handler);
        initialized = 1;
    }
#endif //!defined(GLATTER_DO_NOT_INSTALL_X_ERROR_HANDLER)

    return glatter_get_proc_address(function_name);
}


GLATTER_INLINE_OR_NOT
void glatter_check_error_GLX(const char* file, int line)
{
    // TODO: decide if implementing further wrappers to be able
    // to call XSync here, is within the scope of this library.
}

//////////////////////////////////
#endif // defined(GLATTER_GLX)  //
//////////////////////////////////


////////////////////////////
#if defined(GLATTER_WGL)  //
////////////////////////////

GLATTER_INLINE_OR_NOT
const char* enum_to_string_WGL(GLenum e);

GLATTER_INLINE_OR_NOT
void glatter_check_error_WGL(const char* file, int line)
{
    DWORD eid = GetLastError();
    if(eid == 0)
        return;

    LPSTR buffer = 0;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        eid, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buffer, 0, NULL);

    printf("GLATTER: WGL call produced the following error in %s(%d):\n%s\t", file, line, buffer);

    LocalFree(buffer);
}


GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address_WGL(const char* function_name)
{
    return glatter_get_proc_address(function_name);
}

//////////////////////////////////
#endif // defined(GLATTER_WGL)  //
//////////////////////////////////


////////////////////////////
#if defined(GLATTER_EGL)  //
////////////////////////////

GLATTER_INLINE_OR_NOT
const char* enum_to_string_EGL(GLenum e);


GLATTER_INLINE_OR_NOT
void glatter_check_error_EGL(const char* file, int line)
{
    EGLint err = eglGetError();
    if (err != EGL_SUCCESS) {
        printf("GLATTER: EGL call produced %s error in %s(%d)\n",
            enum_to_string_EGL(err), file, line);
    }
}


GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address_EGL(const char* function_name)
{
    return glatter_get_proc_address(function_name);
}

//////////////////////////////////
#endif // defined(GLATTER_EGL)  //
//////////////////////////////////

////////////////////////////
#if defined(GLATTER_GLU)  //
////////////////////////////

GLATTER_INLINE_OR_NOT
const char* enum_to_string_GLU(GLenum e);


GLATTER_INLINE_OR_NOT
void glatter_check_error_GLU(const char* file, int line)
{
    glatter_check_error_GL(file, line);
}


GLATTER_INLINE_OR_NOT
void* glatter_get_proc_address_GLU(const char* function_name)
{
    void* ptr = 0;

#if defined(_WIN32)
    ptr = (void*) GetProcAddress(GetModuleHandle(TEXT("glu32.dll")), function_name);
#elif defined (__linux__)
    ptr = (void*) dlsym(dlopen("libGLU.so", RTLD_LAZY), function_name);
#else
    #error There is no implementation for your platform. Your contribution would be greatly appreciated!
#endif

    return ptr;
}

//////////////////////////////////
#endif // defined(GLATTER_GLU)  //
//////////////////////////////////


GLATTER_INLINE_OR_NOT
void glatter_pre_callback(const char* file, int line)
{
    static int initialized = 0;
#if defined(_WIN32)
    static __declspec(thread) DWORD thread_id;
    if (!initialized) {
        thread_id = GetCurrentThreadId();
        initialized = 1;
    }
    DWORD current_thread = GetCurrentThreadId();
#elif defined(__linux__)
    static __thread pthread_t thread_id;
    if (!initialized) {
        thread_id = pthread_self();
        initialized = 1;
    }
    pthread_t current_thread = pthread_self();
#endif
    if (current_thread != thread_id) {
        printf("GLATTER: Calling OpenGL from a different thread, in %s(%d)\n", file, line);
    }
}


typedef struct
{
    char data[3*16+2];
}
Printable;


GLATTER_INLINE_OR_NOT
Printable get_prs(size_t sz, void* obj)
{
    Printable ret;
    
    if (sz > 16)
        sz = 16;

    for (size_t i=0; i<sz; i++) {
		#ifdef GLATTER_WGL
			_snprintf(&ret.data[i*3], sizeof(ret)-3*i-1," %02x", (unsigned char) *(((char*)obj)+i) );
		#else 
			snprintf(&ret.data[i*3], sizeof(ret)-3*i-1," %02x", (unsigned char) *(((char*)obj)+i) );
		#endif
    }
    
    ret.data[0     ] = '[';
    ret.data[3*sz  ] = ']';
    ret.data[3*sz+1] =  0 ;

    return ret;
}



GLATTER_INLINE_OR_NOT
uint32_t glatter_djb2(const uint8_t *str)
{
    uint32_t hash = 5381;

    for (int c = *str; c; c = *++str)
        hash = ((hash << 5) + hash) + c;

    return hash;
}


typedef struct glatter_es_record_struct
{
    uint32_t hash;
    uint16_t index;
} glatter_es_record_t;


// This value is hardoded inside glatter.py. To change it, both must be changed.
#define GLATTER_LOOKUP_SIZE 0x4000


//==================

#ifdef GLATTER_HEADER_ONLY


#define GLATTER_FBLOCK(return_or_not, family, cder, rtype, cconv, name, cargs, dargs)\
    typedef rtype (cconv *glatter_##name##_t) dargs;\
    inline rtype cconv glatter_##name dargs\
    {\
        static glatter_##name##_t s_f_ptr =\
            (glatter_##name##_t) glatter_get_proc_address_##family(#name);\
        return_or_not s_f_ptr cargs;\
    }


#else


#define GLATTER_FBLOCK(return_or_not, family, cder, rtype, cconv, name, cargs, dargs)\
    cder rtype cconv name dargs;\
    typedef rtype (cconv *glatter_##name##_t) dargs;\
    extern glatter_##name##_t glatter_##name ;\
    extern rtype cconv glatter_##name##_init dargs;\
    rtype cconv glatter_##name##_init dargs\
    {\
        glatter_##name = (glatter_##name##_t) glatter_get_proc_address_##family(#name);\
        return_or_not glatter_##name cargs;\
    }\
    glatter_##name##_t glatter_##name = glatter_##name##_init;


#endif

//==================

#if defined(GLATTER_LOG_CALLS)

    #define GLATTER_DBLOCK(file, line, name, printf_fmt, ...) \
        glatter_pre_callback(file, line);\
        printf("GLATTER: in '%s'(%d):\n", file, line);\
        printf("GLATTER: " #name printf_fmt "\n",##__VA_ARGS__);

    #define GLATTER_RBLOCK(...)\
        printf("GLATTER: returned " __VA_ARGS__);

#else

    #define GLATTER_DBLOCK(file, line, name, printf_fmt, ...) \
        glatter_pre_callback(file, line);

    #define GLATTER_RBLOCK(...)

#endif


#if defined (GLATTER_LOG_ERRORS)

    #define GLATTER_CHECK_ERROR(family, file, line) glatter_check_error_##family(file, line);

#else

    #define GLATTER_CHECK_ERROR(...)

#endif


#define GET_PRS(v)\
    (get_prs(sizeof(v), (void*)(&(v))).data)


#if defined(__llvm__) || defined (__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-value"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wsizeof-array-argument"
#endif


#ifndef GLATTER_HEADER_ONLY
    #define GLATTER_str(s) #s
    #define GLATTER_xstr(s) GLATTER_str(s)
    #define GLATTER_PDIR(pd) platforms/pd
#endif


#if defined(GLATTER_LOG_ERRORS) || defined(GLATTER_LOG_CALLS)

    #if defined(GLATTER_GL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_d_def.h)
    #endif

    #if defined(GLATTER_GLX)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_d_def.h)
    #endif

    #if defined(GLATTER_EGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_d_def.h)
    #endif

    #if defined(GLATTER_WGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_d_def.h)
    #endif

    #if defined(GLATTER_GLU)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLU_d_def.h)
    #endif

#else

    #if defined(GLATTER_GL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_r_def.h)
    #endif

    #if defined(GLATTER_GLX)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_r_def.h)
    #endif

    #if defined(GLATTER_EGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_r_def.h)
    #endif

    #if defined(GLATTER_WGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_r_def.h)
    #endif

    #if defined(GLATTER_GLU)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLU_r_def.h)
    #endif

#endif



#ifndef GLATTER_HEADER_ONLY

#if defined(GLATTER_GL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_ges_decl.h)
#endif

#if defined(GLATTER_GLX)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_ges_decl.h)
#endif

#if defined(GLATTER_EGL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_ges_decl.h)
#endif

#if defined(GLATTER_WGL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_ges_decl.h)
#endif

#endif



#if defined(GLATTER_GL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_ges_def.h)
#endif

#if defined(GLATTER_GLX)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_ges_def.h)
#endif

#if defined(GLATTER_EGL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_ges_def.h)
#endif

#if defined(GLATTER_WGL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_ges_def.h)
#endif



#if defined(GLATTER_GL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_e2s_def.h)
#endif

#if defined(GLATTER_GLX)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_e2s_def.h)
#endif

#if defined(GLATTER_EGL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_e2s_def.h)
#endif

#if defined(GLATTER_WGL)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_e2s_def.h)
#endif

#if defined(GLATTER_GLU)
    #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLU_e2s_def.h)
#endif


#if defined(__llvm__) || defined (__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif


#ifdef __cplusplus
}
#endif
