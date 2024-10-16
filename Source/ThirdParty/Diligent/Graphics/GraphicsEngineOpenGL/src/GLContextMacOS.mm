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

#import <AppKit/AppKit.h>

#include "GLContextMacOS.hpp"
#include "GraphicsTypes.h"
#include "GLTypeConversions.hpp"

static void glDrawArraysInstancedBaseInstance_stub(GLenum mode, GLint first, GLsizei count, GLsizei primcount, GLuint baseinstance)
{
    LOG_ERROR_MESSAGE("glDrawArraysInstancedBaseInstance is not supported on MacOS");
}

static void glDrawElementsInstancedBaseInstance_stub(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount, GLuint baseinstance)
{
    LOG_ERROR_MESSAGE("glDrawElementsInstancedBaseInstance is not supported on MacOS");
}

static void  glDrawElementsInstancedBaseVertexBaseInstance_stub(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount, GLint basevertex, GLuint baseinstance)
{
    LOG_ERROR_MESSAGE("glDrawElementsInstancedBaseVertexBaseInstance is not supported on MacOS");
}


namespace Diligent
{
    GLContext::GLContext(const EngineGLCreateInfo& InitAttribs, RENDER_DEVICE_TYPE& DevType, struct Version& APIVersion, const struct SwapChainDesc* /*pSCDesc*/)
    {
        if (GetCurrentNativeGLContext() == nullptr)
        {
            LOG_ERROR_AND_THROW("No current GL context found!");
        }

        // Initialize GLEW
        glewExperimental = true; // This is required on MacOS
        GLenum err = glewInit();
        if( GLEW_OK != err )
            LOG_ERROR_AND_THROW( "Failed to initialize GLEW" );
        if(glDrawArraysInstancedBaseInstance == nullptr)
            glDrawArraysInstancedBaseInstance = glDrawArraysInstancedBaseInstance_stub;
        if(glDrawElementsInstancedBaseInstance == nullptr)
            glDrawElementsInstancedBaseInstance = glDrawElementsInstancedBaseInstance_stub;
        if(glDrawElementsInstancedBaseVertexBaseInstance == nullptr)
            glDrawElementsInstancedBaseVertexBaseInstance = glDrawElementsInstancedBaseVertexBaseInstance_stub;

        //Checking GL version
        const GLubyte *GLVersionString = glGetString( GL_VERSION );
        const GLubyte *GLRenderer = glGetString(GL_RENDERER);

        Int32 MajorVersion = 0, MinorVersion = 0;
        //Or better yet, use the GL3 way to get the version number
        glGetIntegerv( GL_MAJOR_VERSION, &MajorVersion );
        glGetIntegerv( GL_MINOR_VERSION, &MinorVersion );
        LOG_INFO_MESSAGE(InitAttribs.Window.pNSView != nullptr ? "Initialized OpenGL " : "Attached to OpenGL ", MajorVersion, '.', MinorVersion, " context (", GLVersionString, ", ", GLRenderer, ')');

        DevType          = RENDER_DEVICE_TYPE_GL;
        APIVersion.Major = static_cast<Uint32>(MajorVersion);
        APIVersion.Minor = static_cast<Uint32>(MinorVersion);
    }

    GLContext::NativeGLContextType GLContext::GetCurrentNativeGLContext()
    {
        NSOpenGLContext* CurrentCtx = [NSOpenGLContext currentContext];
        return (__bridge void*) CurrentCtx;
    }
}
