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

#ifndef __GLATTER_H__
#define __GLATTER_H__

#include "glatter_config.h"
#include "glatter_platform_headers.h"

#ifdef __cplusplus

extern "C" {

#elif defined (GLATTER_HEADER_ONLY)

    #error GLATTER_HEADER_ONLY can only be used in C++

#endif //__cplusplus



#define GLATTER_str(s) #s
#define GLATTER_xstr(s) GLATTER_str(s)
#define GLATTER_PDIR(pd) platforms/pd



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



#ifdef GLATTER_HEADER_ONLY

    #include "glatter_def.h"

    #if defined(GLATTER_LOG_ERRORS) || defined(GLATTER_LOG_ERRORS)
        #define GLATTER_UBLOCK(rtype, cconv, name, dargs)\
            typedef rtype (cconv *glatter_##name##_t) dargs;\
            extern glatter_##name##_t glatter_##name;
    #else
        #define GLATTER_UBLOCK(...)
    #endif

#else

#define GLATTER_UBLOCK(rtype, cconv, name, dargs)\
    typedef rtype (cconv *glatter_##name##_t) dargs;\
    extern glatter_##name##_t glatter_##name;

#endif


#ifndef GLATTER_INLINE_OR_NOT
#define GLATTER_INLINE_OR_NOT
#endif


#if defined(GLATTER_GL)
    GLATTER_INLINE_OR_NOT glatter_extension_support_status_GL_t  glatter_get_extension_support_GL();
    GLATTER_INLINE_OR_NOT const char* enum_to_string_GL(GLenum e);
#endif

#if defined(GLATTER_GLX)
    GLATTER_INLINE_OR_NOT glatter_extension_support_status_GLX_t glatter_get_extension_support_GLX();
    GLATTER_INLINE_OR_NOT const char* enum_to_string_GLX(GLenum e);
#endif

#if defined(GLATTER_EGL)
    GLATTER_INLINE_OR_NOT glatter_extension_support_status_EGL_t glatter_get_extension_support_EGL();
    GLATTER_INLINE_OR_NOT const char* enum_to_string_WGL(GLenum e);
#endif

#if defined(GLATTER_WGL)
    GLATTER_INLINE_OR_NOT glatter_extension_support_status_WGL_t glatter_get_extension_support_WGL();
    GLATTER_INLINE_OR_NOT const char* enum_to_string_EGL(GLenum e);
#endif
    
#if defined (GLATTER_GLU)
    GLATTER_INLINE_OR_NOT const char* enum_to_string_GLU(GLenum e);
#endif


#if defined(GLATTER_LOG_ERRORS) || defined(GLATTER_LOG_CALLS)

    #if defined(GLATTER_GL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_d.h)
    #endif

    #if defined(GLATTER_GLX)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_d.h)
    #endif

    #if defined(GLATTER_EGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_d.h)
    #endif

    #if defined(GLATTER_WGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_d.h)
    #endif

    #if defined(GLATTER_GLU)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLU_d.h)
    #endif

#else

    #if defined(GLATTER_GL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GL_r.h)
    #endif

    #if defined(GLATTER_GLX)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLX_r.h)
    #endif

    #if defined(GLATTER_EGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_EGL_r.h)
    #endif

    #if defined(GLATTER_WGL)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_WGL_r.h)
    #endif

    #if defined(GLATTER_GLU)
        #include GLATTER_xstr(GLATTER_PDIR(GLATTER_PLATFORM_DIR)/glatter_GLU_r.h)
    #endif

#endif


#ifdef __cplusplus
}
#endif

#endif
