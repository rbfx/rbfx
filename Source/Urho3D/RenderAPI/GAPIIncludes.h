// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

#if GL_SUPPORTED || GLES_SUPPORTED
    #if URHO3D_PLATFORM_IOS || URHO3D_PLATFORM_TVOS
        #include <OpenGLES/ES3/gl.h>
        #include <OpenGLES/ES3/glext.h>
    #elif URHO3D_PLATFORM_MACOS
        #ifndef GLEW_STATIC
            #define GLEW_STATIC
        #endif
        #ifndef GLEW_NO_GLU
            #define GLEW_NO_GLU
        #endif
        #include <GL/glew.h>
    #elif URHO3D_PLATFORM_ANDROID
        #include <GLES3/gl3.h>
        #include <GLES3/gl3ext.h>
    #elif URHO3D_PLATFORM_WEB
        #include <GLES3/gl32.h>
    #else
        #include <GL/glew.h>
    #endif

    #ifndef GL_CLIP_DISTANCE0_EXT
        #define GL_CLIP_DISTANCE0_EXT 0x3000
    #endif

    #ifndef GL_PROGRAM_SEPARABLE
        #define GL_PROGRAM_SEPARABLE 0x8258
    #endif
#endif

#if VULKAN_SUPPORTED
    #if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS || URHO3D_PLATFORM_ANDROID
        #define DILIGENT_USE_VOLK 1
    #endif

    #if DILIGENT_USE_VOLK
        #define VK_NO_PROTOTYPES
    #endif

    #include <vulkan/vulkan.h>

    #if DILIGENT_USE_VOLK
        #include <Diligent/ThirdParty/volk/volk.h>
    #endif
#endif

#if D3D11_SUPPORTED
    #include <d3d11_1.h>
#endif

#if D3D12_SUPPORTED
    #include <d3d12.h>
#endif
