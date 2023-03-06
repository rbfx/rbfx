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
/// Declaration of Diligent::PipelineStateVkImpl class

#include <array>
#include <memory>

#include "EngineVkImplTraits.hpp"
#include "PipelineStateBase.hpp"
#include "PipelineResourceSignatureVkImpl.hpp" // Required by PipelineStateBase

#include "ShaderVariableManagerVk.hpp"
#include "FixedBlockMemoryAllocator.hpp"
#include "SRBMemoryAllocator.hpp"
#include "PipelineLayoutVk.hpp"
#include "VulkanUtilities/VulkanObjectWrappers.hpp"
#include "VulkanUtilities/VulkanCommandBuffer.hpp"

namespace Diligent
{

class DeviceContextVkImpl;

/// Pipeline state object implementation in Vulkan backend.
class PipelineStateVkImpl final : public PipelineStateBase<EngineVkImplTraits>
{
public:
    using TPipelineStateBase = PipelineStateBase<EngineVkImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0xdbac0281, 0x36de, 0x4550, {0x80, 0x2d, 0xa3, 0x8c, 0x6e, 0xfb, 0x92, 0x57}};

    PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const GraphicsPipelineStateCreateInfo& CreateInfo);
    PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const ComputePipelineStateCreateInfo& CreateInfo);
    PipelineStateVkImpl(IReferenceCounters* pRefCounters, RenderDeviceVkImpl* pDeviceVk, const RayTracingPipelineStateCreateInfo& CreateInfo);
    ~PipelineStateVkImpl();

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_PipelineStateVk, IID_InternalImpl, TPipelineStateBase)

    /// Implementation of IPipelineStateVk::GetRenderPass().
    virtual IRenderPassVk* DILIGENT_CALL_TYPE GetRenderPass() const override final { return GetRenderPassPtr().RawPtr<IRenderPassVk>(); }

    /// Implementation of IPipelineStateVk::GetVkPipeline().
    virtual VkPipeline DILIGENT_CALL_TYPE GetVkPipeline() const override final { return m_Pipeline; }

    const PipelineLayoutVk& GetPipelineLayout() const { return m_PipelineLayout; }

    struct ShaderStageInfo
    {
        ShaderStageInfo() {}
        explicit ShaderStageInfo(const ShaderVkImpl* pShader);

        void   Append(const ShaderVkImpl* pShader);
        size_t Count() const;

        // Shader stage type. All shaders in the stage must have the same type.
        SHADER_TYPE Type = SHADER_TYPE_UNKNOWN;

        std::vector<const ShaderVkImpl*>   Shaders;
        std::vector<std::vector<uint32_t>> SPIRVs;

        friend SHADER_TYPE GetShaderStageType(const ShaderStageInfo& Stage) { return Stage.Type; }
    };
    using TShaderStages = std::vector<ShaderStageInfo>;

#ifdef DILIGENT_DEVELOPMENT
    // Performs validation of SRB resource parameters that are not possible to validate
    // when resource is bound.
    using ShaderResourceCacheArrayType = std::array<ShaderResourceCacheVk*, MAX_RESOURCE_SIGNATURES>;
    void DvpVerifySRBResources(const DeviceContextVkImpl* pCtx, const ShaderResourceCacheArrayType& ResourceCaches) const;

    void DvpValidateResourceLimits() const;
#endif

    using TShaderResources         = std::vector<std::shared_ptr<const SPIRVShaderResources>>;
    using TResourceAttibutions     = std::vector<ResourceAttribution>;
    using TBindIndexToDescSetIndex = std::array<Uint32, MAX_RESOURCE_SIGNATURES>;
    static void RemapOrVerifyShaderResources(
        TShaderStages&                                       ShaderStages,
        const RefCntAutoPtr<PipelineResourceSignatureVkImpl> pSignatures[],
        Uint32                                               SignatureCount,
        const TBindIndexToDescSetIndex&                      BindIndexToDescSetIndex,
        bool                                                 bVerifyOnly,
        bool                                                 bStripReflection,
        const char*                                          PipelineName,
        TShaderResources*                                    pShaderResources     = nullptr,
        TResourceAttibutions*                                pResourceAttibutions = nullptr) noexcept(false);

    static PipelineResourceSignatureDescWrapper GetDefaultResourceSignatureDesc(
        const TShaderStages&              ShaderStages,
        const char*                       PSOName,
        const PipelineResourceLayoutDesc& ResourceLayout,
        Uint32                            SRBAllocationGranularity) noexcept(false);

private:
    template <typename PSOCreateInfoType>
    TShaderStages InitInternalObjects(const PSOCreateInfoType&                           CreateInfo,
                                      std::vector<VkPipelineShaderStageCreateInfo>&      vkShaderStages,
                                      std::vector<VulkanUtilities::ShaderModuleWrapper>& ShaderModules) noexcept(false);

    void InitPipelineLayout(const PipelineStateCreateInfo& CreateInfo,
                            TShaderStages&                 ShaderStages) noexcept(false);

    void Destruct();

    VulkanUtilities::PipelineWrapper m_Pipeline;
    PipelineLayoutVk                 m_PipelineLayout;

#ifdef DILIGENT_DEVELOPMENT
    // Shader resources for all shaders in all shader stages
    TShaderResources m_ShaderResources;
    // Resource attributions for every resource in m_ShaderResources, in the same order
    TResourceAttibutions m_ResourceAttibutions;
#endif
};

} // namespace Diligent
