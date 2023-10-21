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
/// Declaration of Diligent::PipelineResourceSignatureD3D11Impl class

#include <array>

#include "EngineD3D11ImplTraits.hpp"
#include "PipelineResourceSignatureBase.hpp"
#include "PipelineResourceAttribsD3D11.hpp"

// ShaderVariableManagerD3D11, ShaderResourceCacheD3D11, and ShaderResourceBindingD3D11Impl
// are required by PipelineResourceSignatureBase
#include "ShaderResourceCacheD3D11.hpp"
#include "ShaderVariableManagerD3D11.hpp"
#include "ShaderResourceBindingD3D11Impl.hpp"
#include "SamplerD3D11Impl.hpp"

#include "ResourceBindingMap.hpp"

namespace Diligent
{

struct PipelineResourceImmutableSamplerAttribsD3D11
{
public:
    Uint32                  ArraySize = 1;
    D3D11ResourceBindPoints BindPoints;

    PipelineResourceImmutableSamplerAttribsD3D11() noexcept {}

    bool IsAllocated() const { return !BindPoints.IsEmpty(); }
};
ASSERT_SIZEOF(PipelineResourceImmutableSamplerAttribsD3D11, 12, "The struct is used in serialization and must be tightly packed");


struct PipelineResourceSignatureInternalDataD3D11 : PipelineResourceSignatureInternalData
{
    const PipelineResourceAttribsD3D11*                 pResourceAttribs     = nullptr; // [NumResources]
    Uint32                                              NumResources         = 0;
    const PipelineResourceImmutableSamplerAttribsD3D11* pImmutableSamplers   = nullptr; // [NumImmutableSamplers]
    Uint32                                              NumImmutableSamplers = 0;

    std::unique_ptr<PipelineResourceImmutableSamplerAttribsD3D11[]> m_pImmutableSamplers;

    PipelineResourceSignatureInternalDataD3D11() noexcept
    {}

    explicit PipelineResourceSignatureInternalDataD3D11(const PipelineResourceSignatureInternalData& Serialized) noexcept :
        PipelineResourceSignatureInternalData{Serialized}
    {}
};

/// Implementation of the Diligent::PipelineResourceSignatureD3D11Impl class
class PipelineResourceSignatureD3D11Impl final : public PipelineResourceSignatureBase<EngineD3D11ImplTraits>
{
public:
    using TPipelineResourceSignatureBase = PipelineResourceSignatureBase<EngineD3D11ImplTraits>;

    using ResourceAttribs = TPipelineResourceSignatureBase::PipelineResourceAttribsType;

    PipelineResourceSignatureD3D11Impl(IReferenceCounters*                  pRefCounters,
                                       RenderDeviceD3D11Impl*               pDevice,
                                       const PipelineResourceSignatureDesc& Desc,
                                       SHADER_TYPE                          ShaderStages      = SHADER_TYPE_UNKNOWN,
                                       bool                                 bIsDeviceInternal = false);
    PipelineResourceSignatureD3D11Impl(IReferenceCounters*                               pRefCounters,
                                       RenderDeviceD3D11Impl*                            pDevice,
                                       const PipelineResourceSignatureDesc&              Desc,
                                       const PipelineResourceSignatureInternalDataD3D11& InternalData);
    ~PipelineResourceSignatureD3D11Impl();

    // sizeof(ImmutableSamplerAttribs) == 24, x64
    struct ImmutableSamplerAttribs : PipelineResourceImmutableSamplerAttribsD3D11
    {
        RefCntAutoPtr<SamplerD3D11Impl> pSampler;

        ImmutableSamplerAttribs() noexcept {}
        explicit ImmutableSamplerAttribs(const PipelineResourceImmutableSamplerAttribsD3D11& Attribs) noexcept :
            PipelineResourceImmutableSamplerAttribsD3D11{Attribs} {}
    };

    const ImmutableSamplerAttribs& GetImmutableSamplerAttribs(Uint32 SampIndex) const
    {
        VERIFY_EXPR(SampIndex < m_Desc.NumImmutableSamplers);
        return m_ImmutableSamplers[SampIndex];
    }

    // Shifts resource bindings by the number of resources in each shader stage and resource range.
    __forceinline void ShiftBindings(D3D11ShaderResourceCounters& Bindings) const
    {
        for (Uint32 r = 0; r < D3D11_RESOURCE_RANGE_COUNT; ++r)
            Bindings[r] += m_ResourceCounters[r];
    }

    void InitSRBResourceCache(ShaderResourceCacheD3D11& ResourceCache);

    void UpdateShaderResourceBindingMap(ResourceBinding::TMap& ResourceMap, SHADER_TYPE ShaderStage, const D3D11ShaderResourceCounters& BaseBindings) const;

    // Copies static resources from the static resource cache to the destination cache
    void CopyStaticResources(ShaderResourceCacheD3D11& ResourceCache) const;
    // Make the base class method visible
    using TPipelineResourceSignatureBase::CopyStaticResources;

    PipelineResourceSignatureInternalDataD3D11 GetInternalData() const;

#ifdef DILIGENT_DEVELOPMENT
    /// Verifies committed resource using the D3D resource attributes from the PSO.
    bool DvpValidateCommittedResource(const D3DShaderResourceAttribs& D3DAttribs,
                                      Uint32                          ResIndex,
                                      const ShaderResourceCacheD3D11& ResourceCache,
                                      const char*                     ShaderName,
                                      const char*                     PSOName) const;
#endif

    static D3D11_RESOURCE_RANGE ShaderResourceTypeToRange(SHADER_RESOURCE_TYPE Type);

private:
    void CreateLayout(bool IsSerialized);

    void Destruct();

private:
    D3D11ShaderResourceCounters m_ResourceCounters = {};

    static constexpr int NumShaderTypes = D3D11ResourceBindPoints::NumShaderTypes;

    // Indicates which constant buffer slots are allowed to contain buffers with dynamic offsets.
    std::array<Uint16, NumShaderTypes> m_DynamicCBSlotsMask{};
    static_assert(sizeof(m_DynamicCBSlotsMask[0]) * 8 >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, "Not enough bits for all dynamic buffer slots");

    ImmutableSamplerAttribs* m_ImmutableSamplers = nullptr; // [m_Desc.NumImmutableSamplers]
};

} // namespace Diligent
