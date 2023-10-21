/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include "GLContextEmscripten.hpp"
#include "GraphicsTypes.h"
#include "GLTypeConversions.hpp"

namespace Diligent
{

GLContext::GLContext(const EngineGLCreateInfo& InitAttribs, RENDER_DEVICE_TYPE& DevType, Version& APIVersion, const struct SwapChainDesc* /*pSCDesc*/)
{
    if (InitAttribs.Window.pCanvasId != nullptr)
    {
        EmscriptenWebGLContextAttributes ContextAttributes = {};
        emscripten_webgl_init_context_attributes(&ContextAttributes);

        // TODO: Initialization params
        ContextAttributes.depth        = true;
        ContextAttributes.majorVersion = 3;
        ContextAttributes.minorVersion = 0;

        m_GLContext = emscripten_webgl_create_context(InitAttribs.Window.pCanvasId, &ContextAttributes);
        if (m_GLContext == 0)
        {
            LOG_ERROR_AND_THROW("GL context isn't created");
        }

        auto EmResult = emscripten_webgl_make_context_current(m_GLContext);
        if (EmResult != EMSCRIPTEN_RESULT_SUCCESS)
        {
            LOG_ERROR_AND_THROW("Couldn't set the current GL context");
        }
        m_IsCreated = true;
    }
    else
    {
        m_GLContext = emscripten_webgl_get_current_context();
        if (m_GLContext == 0)
        {
            LOG_ERROR_AND_THROW("No current GL context found!");
        }
    }

    // Checking GL version
    const GLubyte* GLVersionString = glGetString(GL_VERSION);
    const GLubyte* GLRenderer      = glGetString(GL_RENDERER);

    Int32 MajorVersion = 0, MinorVersion = 0;
    // Or better yet, use the GL3 way to get the version number
    glGetIntegerv(GL_MAJOR_VERSION, &MajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &MinorVersion);
    LOG_INFO_MESSAGE(InitAttribs.Window.pCanvasId != nullptr ? "Initialized OpenGLES " : "Attached to OpenGLES ", MajorVersion, '.', MinorVersion, " context (", GLVersionString, ", ", GLRenderer, ')');

    // Under the standard filtering rules for cubemaps, filtering does not work across faces of the cubemap.
    // This results in a seam across the faces of a cubemap. This was a hardware limitation in the past, but
    // modern hardware is capable of interpolating across a cube face boundary.
    // GL_TEXTURE_CUBE_MAP_SEAMLESS is not defined in OpenGLES
    // glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // if (glGetError() != GL_NO_ERROR)
    //     LOG_ERROR_MESSAGE("Failed to enable seamless cubemap filtering");

    // When GL_FRAMEBUFFER_SRGB is enabled, and if the destination image is in the sRGB colorspace
    // then OpenGL will assume the shader's output is in the linear RGB colorspace. It will therefore
    // convert the output from linear RGB to sRGB.
    // Any writes to images that are not in the sRGB format should not be affected.
    // Thus this setting should be just set once and left that way
    glEnable(GL_FRAMEBUFFER_SRGB);
    if (glGetError() != GL_NO_ERROR)
        LOG_ERROR_MESSAGE("Failed to enable SRGB framebuffers");

    DevType    = RENDER_DEVICE_TYPE_GLES;
    APIVersion = Version{MajorVersion, MinorVersion};
}

GLContext::~GLContext()
{
    if (m_IsCreated)
    {
        auto EmResult = emscripten_webgl_destroy_context(m_GLContext);
        if (EmResult != EMSCRIPTEN_RESULT_SUCCESS)
        {
            LOG_INFO_MESSAGE("GL context isn't destroyed");
        }
        m_IsCreated = false;
    }
}

GLContext::NativeGLContextType GLContext::GetCurrentNativeGLContext()
{
    auto CurrentContext = emscripten_webgl_get_current_context();
    VERIFY(CurrentContext == m_GLContext, "These OpengGL contexts don't match");
    return CurrentContext;
}


void GLContext::Suspend()
{
    LOG_INFO_MESSAGE("Suspending GL context\n");
}

bool GLContext::Invalidate()
{
    LOG_INFO_MESSAGE("Invalidating GL context\n");
    return true;
}


} // namespace Diligent
