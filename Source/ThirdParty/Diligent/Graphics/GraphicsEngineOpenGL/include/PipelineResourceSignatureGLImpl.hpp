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
/// Declaration of Diligent::PipelineResourceSignatureGLImpl class

#include <array>

#include "EngineGLImplTraits.hpp"
#include "PipelineResourceAttribsGL.hpp"
#include "PipelineResourceSignatureBase.hpp"
#include "ShaderResourcesGL.hpp"

// ShaderVariableManagerGL, ShaderResourceCacheGL, and ShaderResourceBindingGLImpl
// are required by PipelineResourceSignatureBase
#include "ShaderResourceCacheGL.hpp"
#include "ShaderVariableManagerGL.hpp"
#include "ShaderResourceBindingGLImpl.hpp"

namespace Diligent
{

enum BINDING_RANGE : Uint32
{
    BINDING_RANGE_UNIFORM_BUFFER = 0,
    BINDING_RANGE_TEXTURE,
    BINDING_RANGE_IMAGE,
    BINDING_RANGE_STORAGE_BUFFER,
    BINDING_RANGE_COUNT,
    BINDING_RANGE_UNKNOWN = ~0u
};
BINDING_RANGE PipelineResourceToBindingRange(const PipelineResourceDesc& Desc);
const char*   GetBindingRangeName(BINDING_RANGE Range);

struct PipelineResourceSignatureInternalDataGL : PipelineResourceSignatureInternalData
{
    const PipelineResourceAttribsGL* pResourceAttribs     = nullptr; // [NumResources]
    Uint32                           NumResources         = 0;
    const RefCntAutoPtr<ISampler>*   pImmutableSamplers   = nullptr; // unused
    Uint32                           NumImmutableSamplers = 0;       // unused

    PipelineResourceSignatureInternalDataGL() noexcept
    {}

    explicit PipelineResourceSignatureInternalDataGL(const PipelineResourceSignatureInternalData& InternalData) noexcept :
        PipelineResourceSignatureInternalData{InternalData}
    {}
};

/// Implementation of the Diligent::PipelineResourceSignatureGLImpl class
class PipelineResourceSignatureGLImpl final : public PipelineResourceSignatureBase<EngineGLImplTraits>
{
public:
    using TPipelineResourceSignatureBase = PipelineResourceSignatureBase<EngineGLImplTraits>;

    PipelineResourceSignatureGLImpl(IReferenceCounters*                  pRefCounters,
                                    RenderDeviceGLImpl*                  pDevice,
                                    const PipelineResourceSignatureDesc& Desc,
                                    SHADER_TYPE                          ShaderStages      = SHADER_TYPE_UNKNOWN,
                                    bool                                 bIsDeviceInternal = false);
    PipelineResourceSignatureGLImpl(IReferenceCounters*                            pRefCounters,
                                    RenderDeviceGLImpl*                            pDevice,
                                    const PipelineResourceSignatureDesc&           Desc,
                                    const PipelineResourceSignatureInternalDataGL& InternalData);
    ~PipelineResourceSignatureGLImpl();

    using ResourceAttribs = TPipelineResourceSignatureBase::PipelineResourceAttribsType;

    using TBindings = std::array<Uint16, BINDING_RANGE_COUNT>;

    // Applies bindings for resources in this signature to GLProgram.
    // The bindings are biased by BaseBindings.
    void ApplyBindings(GLObjectWrappers::GLProgramObj& GLProgram,
                       const ShaderResourcesGL&        Resources,
                       class GLContextState&           State,
                       const TBindings&                BaseBindings) const;

    __forceinline void ShiftBindings(TBindings& Bindings) const
    {
        for (Uint32 i = 0; i < Bindings.size(); ++i)
        {
            Bindings[i] += m_BindingCount[i];
        }
    }

    void InitSRBResourceCache(ShaderResourceCacheGL& ResourceCache);

#ifdef DILIGENT_DEVELOPMENT
    /// Verifies committed resource using the resource attributes from the PSO.
    bool DvpValidateCommittedResource(const ShaderResourcesGL::GLResourceAttribs& GLAttribs,
                                      RESOURCE_DIMENSION                          ResourceDim,
                                      bool                                        IsMultisample,
                                      Uint32                                      ResIndex,
                                      const ShaderResourceCacheGL&                ResourceCache,
                                      const char*                                 ShaderName,
                                      const char*                                 PSOName) const;
#endif

    // Copies static resources from the static resource cache to the destination cache
    void CopyStaticResources(ShaderResourceCacheGL& ResourceCache) const;
    // Make the base class method visible
    using TPipelineResourceSignatureBase::CopyStaticResources;

    Uint32 GetImmutableSamplerIdx(const ResourceAttribs& Res) const
    {
        auto ImtblSamIdx = InvalidImmutableSamplerIndex;
        if (Res.IsImmutableSamplerAssigned())
            ImtblSamIdx = Res.SamplerInd;
        else if (Res.IsSamplerAssigned())
        {
            VERIFY_EXPR(GetResourceDesc(Res.SamplerInd).ResourceType == SHADER_RESOURCE_TYPE_SAMPLER);
            const auto& SamAttribs = GetResourceAttribs(Res.SamplerInd);
            if (SamAttribs.IsImmutableSamplerAssigned())
                ImtblSamIdx = SamAttribs.SamplerInd;
        }
        VERIFY_EXPR(ImtblSamIdx == InvalidImmutableSamplerIndex || ImtblSamIdx < GetImmutableSamplerCount());
        return ImtblSamIdx;
    }

    PipelineResourceSignatureInternalDataGL GetInternalData() const;

private:
    void CreateLayout(bool IsSerialized);

    void Destruct();

private:
    TBindings m_BindingCount = {};

    // Indicates which UBO slots allow binding buffers with dynamic offsets
    Uint64 m_DynamicUBOMask = 0;
    // Indicates which SSBO slots allow binding buffers with dynamic offsets
    Uint64 m_DynamicSSBOMask = 0;

    using SamplerPtr                = RefCntAutoPtr<ISampler>;
    SamplerPtr* m_ImmutableSamplers = nullptr; // [m_Desc.NumImmutableSamplers]
};

} // namespace Diligent
