#ifndef __GLATTER_PLATFORM_HEADERS_H__
#define __GLATTER_PLATFORM_HEADERS_H__

// When introducing a new platform, this file needs to be modified.

// WARNING: This file is processed by glatter.py, which makes certain
// assumptions about its format.
// A platform's header set must be contained within a single #if/ifdef block which
// begins with #define GLATTER_PLATFORM_DIR the_name_of_the_platform_directory
// It is OK to have other/irrelevant statements inside that block.


#if defined(GLATTER_WINDOWS_WGL_GL) +\
    defined(GLATTER_MESA_GLX_GL) 	+\
    defined(GLATTER_EGL_GLES_1_1) 	+\
    defined(GLATTER_EGL_GLES2_2_0) 	+\
    defined(GLATTER_EGL_GLES_3_0) 	+\
    defined(GLATTER_EGL_GLES_3_1) 	+\
    defined(GLATTER_EGL_GLES_3_2)   > 1
    
    #error Multiple platforms defined.
    
#endif


#if defined(_WIN32)
    #include <windows.h>
#elif defined (__linux__)
    #include <pthread.h>
    #include <dlfcn.h>
#else
    #error This platform is not supported
#endif


#if defined(GLATTER_WINDOWS_WGL_GL)

    #define GLATTER_PLATFORM_DIR glatter_windows_wgl_gl
    #include "headers/windows_gl_basic/GL.h"
    #include "headers/khronos_gl/glext.h"
    #include "headers/khronos_gl/wglext.h"
    #if defined(GLATTER_GLU)
        #include "headers/windows_gl_basic/GLU.h"
    #endif
    
    #if defined(GLATTER_GLX) || defined(GLATTER_EGL)
        #error One of the wrappers defined are not relevant to the selected platform. Please review your glatter_config.h.
    #endif

#elif defined(GLATTER_MESA_GLX_GL)

    #define GLATTER_PLATFORM_DIR glatter_mesa_glx_gl
    #define GL_GLEXT_LEGACY   // i.e. do not pull glext.h  from the system
    #include "headers/mesa_gl_basic/gl.h"
    #define GLX_GLXEXT_LEGACY // i.e. do not pull glxext.h from the system
    #include "headers/mesa_gl_basic/glx.h"
    #include "headers/khronos_gl/glext.h"
    #include "headers/khronos_gl/glxext.h"
    #if defined(GLATTER_GLU)
        #include "headers/sgi_glu/glu.h"
    #endif

    #if defined(GLATTER_WGL) || defined(GLATTER_EGL)
        #error One of the wrappers defined are not relevant to the selected platform. Please review your glatter_config.h.
    #endif
    
#else

    #if defined(GLATTER_GLU)
        #error Glatter does not support a GLU implementation on this platform. Please do not define GLATTER_GLU.
    #endif

    #if defined(GLATTER_EGL_GLES_1_1)

        #define GLATTER_PLATFORM_DIR glatter_egl_gles_1_1
        #include "headers/khronos_egl/egl.h"
        #include "headers/khronos_egl/eglext.h"
        #include "headers/khronos_gles/gl.h"
        #include "headers/khronos_gles/glext.h"
        #include "headers/khronos_gles/glplatform.h"

    #elif defined(GLATTER_EGL_GLES2_2_0)

        #define GLATTER_PLATFORM_DIR glatter_egl_gles2_2_0
        #include "headers/khronos_egl/egl.h"
        #include "headers/khronos_egl/eglext.h"
        #include "headers/khronos_gles2/gl2platform.h"
        #include "headers/khronos_gles2/gl2.h"
        #include "headers/khronos_gles2/gl2ext.h"

    #elif defined(GLATTER_EGL_GLES_3_0)

        #define GLATTER_PLATFORM_DIR glatter_egl_gles_3_0
        #include "headers/khronos_egl/egl.h"
        #include "headers/khronos_egl/eglext.h"
        #include "headers/khronos_gles3/gl3platform.h"
        #include "headers/khronos_gles3/gl3.h"
        #include "headers/khronos_gles2/gl2ext.h"

    #elif defined(GLATTER_EGL_GLES_3_1)

        #define GLATTER_PLATFORM_DIR glatter_egl_gles_3_1
        #include "headers/khronos_egl/egl.h"
        #include "headers/khronos_egl/eglext.h"
        #include "headers/khronos_gles3/gl3platform.h"
        #include "headers/khronos_gles3/gl31.h"
        #include "headers/khronos_gles2/gl2ext.h"

    #elif defined(GLATTER_EGL_GLES_3_2)

        #define GLATTER_PLATFORM_DIR glatter_egl_gles_3_2
        #include "headers/khronos_gles3/gl3platform.h"
        #include "headers/khronos_gles3/gl32.h"
        #include "headers/khronos_gles2/gl2ext.h"

    #endif
    
    #if defined(GLATTER_GLX) || defined(GLATTER_WGL)
        #error One of the wrappers defined are not relevant to the selected platform. Please review your glatter_config.h.
    #endif
    
#endif


#endif
