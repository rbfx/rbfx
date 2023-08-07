/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

/// \file
/// Definition of the Diligent::IRenderDeviceGLES interface

#include "RenderDeviceGL.h"
#if PLATFORM_ANDROID
#    include <android/native_window.h>
#    include <EGL/egl.h>
#elif PLATFORM_EMSCRIPTEN
#else
#    error Unsupported platform
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {F705A0D9-2023-4DE1-8B3C-C56E4CEB8DB7}
static const INTERFACE_ID IID_RenderDeviceGLES =
    {0xf705a0d9, 0x2023, 0x4de1, {0x8b, 0x3c, 0xc5, 0x6e, 0x4c, 0xeb, 0x8d, 0xb7}};

#define DILIGENT_INTERFACE_NAME IRenderDeviceGLES
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IRenderDeviceGLESInclusiveMethods \
    IRenderDeviceGLInclusiveMethods;      \
    IRenderDeviceGLESMethods RenderDeviceGLES

// clang-format off

/// Interface to the render device object implemented in OpenGLES
DILIGENT_BEGIN_INTERFACE(IRenderDeviceGLES, IRenderDeviceGL)
{
    VIRTUAL bool METHOD(Invalidate)(THIS) PURE;

    VIRTUAL void METHOD(Suspend)(THIS) PURE;

#if PLATFORM_ANDROID
    VIRTUAL EGLint METHOD(Resume)(THIS_
                                  ANativeWindow* window) PURE;
#endif

};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IRenderDeviceGLES_Invalidate(This)   CALL_IFACE_METHOD(RenderDeviceGLES, Invalidate, This)
#    define IRenderDeviceGLES_Suspend(This)      CALL_IFACE_METHOD(RenderDeviceGLES, Suspend,    This)
#    define IRenderDeviceGLES_Resume(This, ...)  CALL_IFACE_METHOD(RenderDeviceGLES, Resume,     This, __VA_ARGS__)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
