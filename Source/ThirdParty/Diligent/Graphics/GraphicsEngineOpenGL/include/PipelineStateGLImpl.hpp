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

#include <vector>

#include "EngineGLImplTraits.hpp"
#include "PipelineStateBase.hpp"

#include "PipelineResourceSignatureGLImpl.hpp" // Required by PipelineStateBase
#include "ShaderGLImpl.hpp"

#include "GLObjectWrapper.hpp"
#include "GLContext.hpp"

namespace Diligent
{

/// Pipeline state object implementation in OpenGL backend.
class PipelineStateGLImpl final : public PipelineStateBase<EngineGLImplTraits>
{
public:
    using TPipelineStateBase = PipelineStateBase<EngineGLImplTraits>;

    PipelineStateGLImpl(IReferenceCounters*                    pRefCounters,
                        RenderDeviceGLImpl*                    pDeviceGL,
                        const GraphicsPipelineStateCreateInfo& CreateInfo,
                        bool                                   IsDeviceInternal = false);
    PipelineStateGLImpl(IReferenceCounters*                   pRefCounters,
                        RenderDeviceGLImpl*                   pDeviceGL,
                        const ComputePipelineStateCreateInfo& CreateInfo,
                        bool                                  IsDeviceInternal = false);
    ~PipelineStateGLImpl();

    /// Queries the specific interface, see IObject::QueryInterface() for details
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    void CommitProgram(GLContextState& State);

    using TBindings = PipelineResourceSignatureGLImpl::TBindings;
    const TBindings& GetBaseBindings(Uint32 Index) const
    {
        VERIFY_EXPR(Index < GetResourceSignatureCount());
        return m_BaseBindings[Index];
    }

    using TShaderStages = std::vector<ShaderGLImpl*>;

    static PipelineResourceSignatureDescWrapper GetDefaultResourceSignatureDesc(
        const TShaderStages&              ShaderStages,
        const char*                       PSOName,
        const PipelineResourceLayoutDesc& ResourceLayout,
        Uint32                            SRBAllocationGranularity) noexcept(false);

#ifdef DILIGENT_DEVELOPMENT
    using ShaderResourceCacheArrayType = std::array<ShaderResourceCacheGL*, MAX_RESOURCE_SIGNATURES>;
    using BaseBindingsArrayType        = std::array<TBindings, MAX_RESOURCE_SIGNATURES>;
    void DvpVerifySRBResources(const ShaderResourceCacheArrayType& ResourceCaches,
                               const BaseBindingsArrayType&        BaseBindings) const;
#endif

private:
    GLObjectWrappers::GLPipelineObj& GetGLProgramPipeline(GLContext::NativeGLContextType Context);

    template <typename PSOCreateInfoType>
    void InitInternalObjects(const PSOCreateInfoType& CreateInfo, const TShaderStages& ShaderStages);

    void InitResourceLayout(PSO_CREATE_INTERNAL_FLAGS InternalFlags,
                            const TShaderStages&      ShaderStages,
                            SHADER_TYPE               ActiveStages);

    PipelineResourceSignatureDescWrapper GetDefaultSignatureDesc(
        const TShaderStages& ShaderStages,
        SHADER_TYPE          ActiveStages);

    void Destruct();

    SHADER_TYPE GetShaderStageType(Uint32 Index) const;
    Uint32      GetNumShaderStages() const { return m_NumPrograms; }

    void ValidateShaderResources(std::shared_ptr<const ShaderResourcesGL> pShaderResources, const char* ShaderName, SHADER_TYPE ShaderStages);

    // Determines the required pipeline resource flag (NONE or COMBINED_SAMPLER) for the set of shaders.
    // Prints a warning in case of a conflict.
    PIPELINE_RESOURCE_FLAGS GetSamplerResourceFlag(const TShaderStages& Stages, bool SilenceWarning) const;

private:
    // Linked GL programs for every shader stage. Every pipeline needs to have its own programs
    // because resource bindings assigned by PipelineResourceSignatureGLImpl::ApplyBindings depend on other
    // shader stages.
    using GLProgramObj         = GLObjectWrappers::GLProgramObj;
    GLProgramObj* m_GLPrograms = nullptr; // [m_NumPrograms]

    Threading::SpinLock m_ProgPipelineLock;

    std::vector<std::pair<GLContext::NativeGLContextType, GLObjectWrappers::GLPipelineObj>> m_GLProgPipelines;

    Uint8        m_NumPrograms                = 0;
    bool         m_IsProgramPipelineSupported = false;
    SHADER_TYPE* m_ShaderTypes                = nullptr; // [m_NumPrograms]

    TBindings* m_BaseBindings = nullptr; // [m_SignatureCount]

#ifdef DILIGENT_DEVELOPMENT
    // Shader resources for all shaders in all shader stages in the pipeline.
    std::vector<std::shared_ptr<const ShaderResourcesGL>> m_ShaderResources;
    std::vector<String>                                   m_ShaderNames;

    // Shader resource attributions for every resource in m_ShaderResources, in the same order.
    std::vector<ResourceAttribution> m_ResourceAttibutions;
#endif
};

__forceinline SHADER_TYPE GetShaderStageType(const ShaderGLImpl* pShader)
{
    return pShader->GetDesc().ShaderType;
}

} // namespace Diligent
