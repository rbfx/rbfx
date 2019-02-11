#ifndef __GLATTER_CONFIG_H__
#define __GLATTER_CONFIG_H__



/////////////////////////////////
// Explicit platform selection //
/////////////////////////////////
//
// NOTE: For GLES the platform must be specified explicitly.
//
// #define GLATTER_WINDOWS_WGL_GL
// #define GLATTER_MESA_GLX_GL
// #define GLATTER_EGL_GLES_1_1
// #define GLATTER_EGL_GLES2_2_0
// #define GLATTER_EGL_GLES_3_0
// #define GLATTER_EGL_GLES_3_1
// #define GLATTER_EGL_GLES_3_2

// If no platform is defined, it will be set according to the operating system.
#if !defined(GLATTER_WINDOWS_WGL_GL) && !defined(GLATTER_MESA_GLX_GL)   &&\
    !defined(GLATTER_EGL_GLES_1_1)   && !defined(GLATTER_EGL_GLES2_2_0) &&\
    !defined(GLATTER_EGL_GLES_3_0)   && !defined(GLATTER_EGL_GLES_3_1)  &&\
    !defined(GLATTER_EGL_GLES_3_2)
    
#if defined(_WIN32)
    #define GLATTER_WINDOWS_WGL_GL
#elif defined(__linux__)
    #define GLATTER_MESA_GLX_GL
#endif
    
#endif



////////////////////////////////
// Explicit wrapper selection //
////////////////////////////////
// #define GLATTER_GL
// #define GLATTER_EGL
// #define GLATTER_WGL
// #define GLATTER_GLX
// #define GLATTER_GLU

// If no wrapper is defined, GL is set, and one of ELG/GLX/WGL depending on the platform
#if !defined(GLATTER_GL) && !defined(GLATTER_EGL) && !defined(GLATTER_WGL) &&\
    !defined(GLATTER_GLX) && !defined(GLATTER_GLU)
    
    #define GLATTER_GL
    #if defined(GLATTER_WINDOWS_WGL_GL)
        #define GLATTER_WGL
    #elif defined (GLATTER_MESA_GLX_GL)
        #define GLATTER_GLX
    #else // GLES
        #define GLATTER_EGL
    #endif
    
    // #define GLATTER_GLU  // GLU is not enabled by default
    
#endif



////////////////////////////////////////
// Header-only switch for the library //
////////////////////////////////////////
//#define GLATTER_HEADER_ONLY



//////////////////////////////////////
// Debugging functionality switches //
//////////////////////////////////////
// #define GLATTER_LOG_ERRORS
// #define GLATTER_LOG_CALLS

// Unless specified otherwise, GL errors will be logged in debug builds
#if !defined(GLATTER_LOG_ERRORS) && !defined(GLATTER_LOG_CALLS)
    #ifndef NDEBUG
//        #define GLATTER_LOG_ERRORS
    #endif
#endif



///////////////////////////////////////////////////
// X error handler switch (only relevant to GLX) //
///////////////////////////////////////////////////
//#define GLATTER_DO_NOT_INSTALL_X_ERROR_HANDLER



#endif
