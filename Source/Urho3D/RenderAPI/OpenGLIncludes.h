// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

#if GL_SUPPORTED || GLES_SUPPORTED
    #if defined(IOS) || defined(TVOS)
        #include <OpenGLES/ES3/gl.h>
        #include <OpenGLES/ES3/glext.h>
    #elif defined(__ANDROID__) || defined(__arm__) || defined(__aarch64__)
        #include <GLES3/gl3.h>
        #include <GLES3/gl3ext.h>
    #elif defined(__EMSCRIPTEN__)
        #include <GLES3/gl32.h>
    #else
        #include <GL/glew.h>
    #endif
#endif
