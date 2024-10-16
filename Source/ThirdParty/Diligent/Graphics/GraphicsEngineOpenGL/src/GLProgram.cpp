/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "GLProgram.hpp"
#include "ShaderGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"

namespace Diligent
{

GLProgram::GLProgram(ShaderGLImpl* const* ppShaders,
                     Uint32               NumShaders,
                     bool                 IsSeparableProgram) noexcept :
    m_AttachedShaders{ppShaders, ppShaders + NumShaders}
{
    VERIFY(!IsSeparableProgram || NumShaders == 1, "Number of shaders must be 1 when separable program is created");

    // GL_PROGRAM_SEPARABLE parameter must be set before linking!
    if (IsSeparableProgram)
    {
        glProgramParameteri(m_GLProg, GL_PROGRAM_SEPARABLE, GL_TRUE);
        DEV_CHECK_GL_ERROR("glProgramParameteri(GL_PROGRAM_SEPARABLE) failed");
    }

    for (Uint32 i = 0; i < NumShaders; ++i)
    {
        auto* pCurrShader = ppShaders[i];
        glAttachShader(m_GLProg, pCurrShader->GetGLShaderHandle());
        DEV_CHECK_GL_ERROR("glAttachShader() failed");
    }

    //With separable program objects, interfaces between shader stages may
    //involve the outputs from one program object and the inputs from a
    //second program object. For such interfaces, it is not possible to
    //detect mismatches at link time, because the programs are linked
    //separately. When each such program is linked, all inputs or outputs
    //interfacing with another program stage are treated as active. The
    //linker will generate an executable that assumes the presence of a
    //compatible program on the other side of the interface. If a mismatch
    //between programs occurs, no GL error will be generated, but some or all
    //of the inputs on the interface will be undefined.
    glLinkProgram(m_GLProg);
    DEV_CHECK_GL_ERROR("glLinkProgram() failed");

    // Note: according to the spec, shaders can be detached immediately after glLinkProgram call.
    //       However, on NVidia GPUs this completely disables the GL_KHR_parallel_shader_compile
    //       extension, which barely works already. So we keep shaders attached until the program
    //       is linked.

    m_LinkStatus = LinkStatus::InProgress;
}

GLProgram::~GLProgram()
{
}

GLProgram::LinkStatus GLProgram::GetLinkStatus(bool WaitForCompletion) noexcept
{
    VERIFY_EXPR(m_LinkStatus != LinkStatus::Undefined);
    if (m_LinkStatus != LinkStatus::InProgress)
        return m_LinkStatus;

    if (!WaitForCompletion)
    {
        GLint LinkingComplete = GL_FALSE;
        glGetProgramiv(m_GLProg, GL_COMPLETION_STATUS_KHR, &LinkingComplete);
        DEV_CHECK_GL_ERROR("glGetProgramiv(GL_COMPLETION_STATUS_KHR) failed");
        if (!LinkingComplete)
            return LinkStatus::InProgress;
    }

    int IsLinked = GL_FALSE;
    glGetProgramiv(m_GLProg, GL_LINK_STATUS, &IsLinked);
    DEV_CHECK_GL_ERROR("glGetProgramiv(GL_LINK_STATUS) failed");

    if (IsLinked)
    {
        m_LinkStatus = LinkStatus::Succeeded;
    }
    else
    {
        int LengthWithNull = 0, Length = 0;
        // Notice that glGetProgramiv is used to get the length for a shader program, not glGetShaderiv.
        // The length of the info log includes a null terminator.
        glGetProgramiv(m_GLProg, GL_INFO_LOG_LENGTH, &LengthWithNull);
        DEV_CHECK_GL_ERROR("glGetProgramiv(GL_INFO_LOG_LENGTH) failed");

        // The maxLength includes the NULL character
        std::vector<char> shaderProgramInfoLog(LengthWithNull);

        // Notice that glGetProgramInfoLog  is used, not glGetShaderInfoLog.
        glGetProgramInfoLog(m_GLProg, LengthWithNull, &Length, shaderProgramInfoLog.data());
        DEV_CHECK_GL_ERROR("glGetProgramInfoLog() failed");

        VERIFY(Length == LengthWithNull - 1, "Incorrect program info log len");
        m_InfoLog.assign(shaderProgramInfoLog.data(), Length);

        m_LinkStatus = LinkStatus::Failed;
    }

    for (const ShaderGLImpl* pShader : m_AttachedShaders)
    {
        glDetachShader(m_GLProg, pShader->GetGLShaderHandle());
        DEV_CHECK_GL_ERROR("glDetachShader() failed");
    }

    std::vector<const ShaderGLImpl*> Null{};
    m_AttachedShaders.swap(Null);

    return m_LinkStatus;
}

std::shared_ptr<const ShaderResourcesGL>& GLProgram::LoadResources(SHADER_TYPE             ShaderStages,
                                                                   PIPELINE_RESOURCE_FLAGS SamplerResourceFlag,
                                                                   GLContextState&         State,
                                                                   bool                    LoadUniformBufferReflection,
                                                                   SHADER_SOURCE_LANGUAGE  SourceLang)
{
    DEV_CHECK_ERR(m_LinkStatus == LinkStatus::Succeeded, "Program must be successfully linked to load resources");
    VERIFY(m_pResources == nullptr, "Resources have already been loaded");

    std::unique_ptr<ShaderResourcesGL> pResources = std::make_unique<ShaderResourcesGL>();
    pResources->LoadUniforms(
        {
            ShaderStages,
            SamplerResourceFlag,
            m_GLProg,
            State,
            LoadUniformBufferReflection,
            SourceLang,
        });
    m_pResources.reset(pResources.release());

    return m_pResources;
}

void GLProgram::SetResources(std::shared_ptr<const ShaderResourcesGL> pResources)
{
    DEV_CHECK_ERR(m_LinkStatus == LinkStatus::Succeeded, "Program must be successfully linked to set resources");
    if (!m_pResources)
    {
        m_pResources = std::move(pResources);
    }
    else
    {
        VERIFY(m_pResources == pResources, "Assigning different resources. This should not happen as cached programs are keyed by shader IDs");
    }
}

void GLProgram::ApplyBindings(const PipelineResourceSignatureGLImpl*            pSignature,
                              GLContextState&                                   State,
                              const PipelineResourceSignatureGLImpl::TBindings& BaseBindings)
{
    // Since we check that assigned resources and base bindings are always the same, the same bindings
    // will be applied by all pipelines that use this program.

    DEV_CHECK_ERR(m_LinkStatus == LinkStatus::Succeeded, "Program must be successfully linked to apply bindings");
    VERIFY(m_pResources, "Resources have not been loaded or assigned");
    if (!m_BindingsApplied)
    {
        pSignature->ApplyBindings(m_GLProg, *m_pResources, State, BaseBindings);
#ifdef DILIGENT_DEBUG
        m_DbgBaseBindings = BaseBindings;
#endif
    }
    else
    {
#ifdef DILIGENT_DEBUG
        VERIFY(m_DbgBaseBindings == BaseBindings, "Base bindings have changed since the last time they were applied. "
                                                  "This should not happen as cached programs are keyed by pipeline resource signature IDs or resource layout.");
#endif
    }
}

} // namespace Diligent
