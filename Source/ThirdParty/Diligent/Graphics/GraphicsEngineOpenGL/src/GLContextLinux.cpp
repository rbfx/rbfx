/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

#include "pch.h"

#include "GLContextLinux.hpp"
#include "GraphicsTypes.h"
#include "GLTypeConversions.hpp"

namespace Diligent
{

GLContext::GLContext(const EngineGLCreateInfo& InitAttribs,
                     RENDER_DEVICE_TYPE&       DevType,
                     struct Version&           APIVersion,
                     const struct SwapChainDesc* /*pSCDesc*/) :
    m_WindowId(InitAttribs.Window.WindowId),
    m_pDisplay(InitAttribs.Window.pDisplay)
{
    auto CurrentCtx = glXGetCurrentContext();
    if (CurrentCtx == 0)
    {
        LOG_ERROR_AND_THROW("No current GL context found!");
    }

    // Initialize GLEW
    GLenum err = glewInit();
    if (GLEW_OK != err)
        LOG_ERROR_AND_THROW("Failed to initialize GLEW");

    //Checking GL version
    const GLubyte* GLVersionString = glGetString(GL_VERSION);
    const GLubyte* GLRenderer      = glGetString(GL_RENDERER);

    Int32 MajorVersion = 0, MinorVersion = 0;
    //Or better yet, use the GL3 way to get the version number
    glGetIntegerv(GL_MAJOR_VERSION, &MajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &MinorVersion);
    LOG_INFO_MESSAGE(InitAttribs.Window.WindowId != 0 ? "Initialized OpenGL " : "Attached to OpenGL ", MajorVersion, '.', MinorVersion, " context (", GLVersionString, ", ", GLRenderer, ')');

    DevType    = RENDER_DEVICE_TYPE_GL;
    APIVersion = Version{static_cast<Uint32>(MajorVersion), static_cast<Uint32>(MinorVersion)};
}

GLContext::~GLContext()
{
}

void GLContext::SwapBuffers(int SwapInterval)
{
    if (m_WindowId != 0 && m_pDisplay != nullptr)
    {
        auto wnd     = static_cast<Window>(m_WindowId);
        auto display = reinterpret_cast<Display*>(m_pDisplay);
#if GLX_EXT_swap_control
        if (glXSwapIntervalEXT != nullptr)
        {
            glXSwapIntervalEXT(display, wnd, SwapInterval);
        }
#    if GLX_MESA_swap_control
        else if (glXSwapIntervalMESA != nullptr)
        {
            glXSwapIntervalMESA(SwapInterval);
        }
#    endif
#endif
        glXSwapBuffers(display, wnd);
    }
    else
    {
        LOG_ERROR("Swap buffer failed because window and/or display handle is not initialized");
    }
}

GLContext::NativeGLContextType GLContext::GetCurrentNativeGLContext()
{
    return glXGetCurrentContext();
}

} // namespace Diligent
