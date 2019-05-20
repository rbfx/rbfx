//
// Copyright (c) 2017-2019 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#ifdef _WIN32

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#endif

#define URHO3D_TOOLBOX_EXPORT_API __declspec(dllexport)

#ifdef URHO3D_TOOLBOX_STATIC
#  define URHO3D_TOOLBOX_API
#  define URHO3D_TOOLBOX_NO_EXPORT
#else
#  ifndef URHO3D_TOOLBOX_API
#    ifdef URHO3D_TOOLBOX_EXPORTS
/* We are building this library */
#      define URHO3D_TOOLBOX_API URHO3D_TOOLBOX_EXPORT_API
#    else
/* We are using this library */
#      define URHO3D_TOOLBOX_API __declspec(dllimport)
#    endif
#  endif
#endif

#else

#define URHO3D_TOOLBOX_EXPORT_API __attribute__((visibility("default")))

#ifdef URHO3D_TOOLBOX_STATIC
#ifndef URHO3D_TOOLBOX_API
#  define URHO3D_TOOLBOX_API
#endif
#  define URHO3D_TOOLBOX_NO_EXPORT
#else
#  define URHO3D_TOOLBOX_API URHO3D_TOOLBOX_EXPORT_API
#endif

#endif

namespace Urho3D
{

class Context;

/// Register toolbox types with the engine.
URHO3D_TOOLBOX_API void RegisterToolboxTypes(Context* context);

};
