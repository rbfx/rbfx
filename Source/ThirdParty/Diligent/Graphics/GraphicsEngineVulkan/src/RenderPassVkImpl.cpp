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

#include "pch.h"
#include <vector>

#include "RenderPassVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "PlatformMisc.hpp"

namespace Diligent
{

RenderPassVkImpl::RenderPassVkImpl(IReferenceCounters*   pRefCounters,
                                   RenderDeviceVkImpl*   pDevice,
                                   const RenderPassDesc& Desc,
                                   bool                  IsDeviceInternal) :
    TRenderPassBase{pRefCounters, pDevice, Desc, IsDeviceInternal}
{
    const auto& ExtFeats = pDevice->GetLogicalDevice().GetEnabledExtFeatures();

    size_t RenderPassVersion = 1;
    if (ExtFeats.ShadingRate.attachmentFragmentShadingRate)
    {
        for (Uint32 i = 0; i < m_Desc.SubpassCount && RenderPassVersion < 2; ++i)
        {
            const auto& Subpass{m_Desc.pSubpasses[i]};
            if (Subpass.pShadingRateAttachment != nullptr)
                RenderPassVersion = 2;
        }
        DEV_CHECK_ERR(RenderPassVersion < 2 || ExtFeats.RenderPass2 != VK_FALSE,
                      "This render pass requires RenderPass2 Vulkan feature that is not enabled");
    }
    else if (ExtFeats.FragmentDensityMap.fragmentDensityMap)
    {
        // Fragment density map is defined through RenderPassCI.pNext
    }


    switch (RenderPassVersion)
    {
        case 1: CreateRenderPass<1>(); break;
        case 2: CreateRenderPass<2>(); break;
        default: UNSUPPORTED("Unsupported render pass version");
    }
}

inline void InitAttachmentDescription(VkAttachmentDescription&) {}
inline void InitAttachmentDescription(VkAttachmentDescription2& Attachment)
{
    Attachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
    Attachment.pNext = nullptr;
}

inline void InitAttachmentReference(VkAttachmentReference&, VkImageAspectFlags) {}
inline void InitAttachmentReference(VkAttachmentReference2& AttachmentRef, VkImageAspectFlags AspectMask)
{
    AttachmentRef.sType      = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
    AttachmentRef.pNext      = nullptr;
    AttachmentRef.aspectMask = AspectMask;
}

inline void InitSubpassDescription(VkSubpassDescription&) {}
inline void InitSubpassDescription(VkSubpassDescription2& Subpass)
{
    Subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
    Subpass.pNext = nullptr;
}

inline void SetSubpassDescriptionNext(VkSubpassDescription&, const void*) {}
inline void SetSubpassDescriptionNext(VkSubpassDescription2& Subpass, const void* pNext)
{
    Subpass.pNext = pNext;
}

inline void InitSubpassDependency(VkSubpassDependency&) {}
inline void InitSubpassDependency(VkSubpassDependency2& Dependency)
{
    Dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
    Dependency.pNext = nullptr;

    // For multiview
    Dependency.viewOffset = 0;
}

template <size_t RPVersion>
void RenderPassVkImpl::CreateRenderPass() noexcept(false)
{
    using RenderPassCIType          = std::conditional_t<RPVersion == 2, VkRenderPassCreateInfo2, VkRenderPassCreateInfo>;
    using SubpassDescriptionType    = std::conditional_t<RPVersion == 2, VkSubpassDescription2, VkSubpassDescription>;
    using AttachmentDescriptionType = std::conditional_t<RPVersion == 2, VkAttachmentDescription2, VkAttachmentDescription>;
    using AttachmentReferenceType   = std::conditional_t<RPVersion == 2, VkAttachmentReference2, VkAttachmentReference>;
    using SubpassDependencyType     = std::conditional_t<RPVersion == 2, VkSubpassDependency2, VkSubpassDependency>;

    const auto& LogicalDevice         = m_pDevice->GetLogicalDevice();
    const auto& ExtFeats              = LogicalDevice.GetEnabledExtFeatures();
    const bool  ShadingRateEnabled    = ExtFeats.ShadingRate.attachmentFragmentShadingRate != VK_FALSE;
    const bool  FragDensityMapEnabled = ExtFeats.FragmentDensityMap.fragmentDensityMap != VK_FALSE;

    RenderPassCIType RenderPassCI{};
    RenderPassCI.sType = RPVersion == 2 ? VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2 : VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    RenderPassCI.pNext = nullptr;
    RenderPassCI.flags = 0;

    std::vector<AttachmentDescriptionType> vkAttachments(m_Desc.AttachmentCount);
    for (Uint32 i = 0; i < m_Desc.AttachmentCount; ++i)
    {
        const auto& Attachment   = m_Desc.pAttachments[i];
        auto&       vkAttachment = vkAttachments[i];

        InitAttachmentDescription(vkAttachment);
        vkAttachment.flags          = 0;
        vkAttachment.format         = TexFormatToVkFormat(Attachment.Format);
        vkAttachment.samples        = static_cast<VkSampleCountFlagBits>(Attachment.SampleCount);
        vkAttachment.loadOp         = AttachmentLoadOpToVkAttachmentLoadOp(Attachment.LoadOp);
        vkAttachment.storeOp        = AttachmentStoreOpToVkAttachmentStoreOp(Attachment.StoreOp);
        vkAttachment.stencilLoadOp  = AttachmentLoadOpToVkAttachmentLoadOp(Attachment.StencilLoadOp);
        vkAttachment.stencilStoreOp = AttachmentStoreOpToVkAttachmentStoreOp(Attachment.StencilStoreOp);
        vkAttachment.initialLayout  = ResourceStateToVkImageLayout(Attachment.InitialState, /*IsInsideRenderPass = */ false, FragDensityMapEnabled);
        vkAttachment.finalLayout    = ResourceStateToVkImageLayout(Attachment.FinalState, /*IsInsideRenderPass = */ true, FragDensityMapEnabled);
    }
    RenderPassCI.attachmentCount = m_Desc.AttachmentCount;
    RenderPassCI.pAttachments    = vkAttachments.data();

    Uint32 TotalAttachmentReferencesCount   = 0;
    Uint32 TotalPreserveAttachmentsCount    = 0;
    Uint32 TotalShadingRateAttachmentsCount = 0;
    for (Uint32 i = 0; i < m_Desc.SubpassCount; ++i)
    {
        const auto& Subpass = m_Desc.pSubpasses[i];
        TotalAttachmentReferencesCount += Subpass.InputAttachmentCount;
        TotalAttachmentReferencesCount += Subpass.RenderTargetAttachmentCount;
        if (Subpass.pResolveAttachments != nullptr)
            TotalAttachmentReferencesCount += Subpass.RenderTargetAttachmentCount;
        if (Subpass.pDepthStencilAttachment != nullptr)
            TotalAttachmentReferencesCount += 1;
        if (Subpass.pShadingRateAttachment != nullptr && ShadingRateEnabled)
            TotalShadingRateAttachmentsCount += 1;
        TotalPreserveAttachmentsCount += Subpass.PreserveAttachmentCount;
    }

    std::vector<AttachmentReferenceType>                vkAttachmentReferences(size_t{TotalAttachmentReferencesCount} + size_t{TotalShadingRateAttachmentsCount});
    std::vector<Uint32>                                 vkPreserveAttachments(TotalPreserveAttachmentsCount);
    std::vector<VkFragmentShadingRateAttachmentInfoKHR> vkShadingRate{TotalShadingRateAttachmentsCount};
    const ShadingRateAttachment*                        pMainSRA = nullptr;

    Uint32 CurrAttachmentReferenceInd = 0;
    Uint32 CurrPreserveAttachmentInd  = 0;

    // State flags for every attachment in each subpass.
    // This array is used to detect attachments that are used as render target or depth-stencil,
    // but also as input attachment in the same subpass. Such attachments need to use GENERAL layout.
    std::vector<RESOURCE_STATE> AttachmentStates(m_Desc.AttachmentCount);

    std::vector<SubpassDescriptionType> vkSubpasses(m_Desc.SubpassCount);
    for (Uint32 i = 0, SRInd = 0; i < m_Desc.SubpassCount; ++i)
    {
        const auto& SubpassDesc = m_Desc.pSubpasses[i];
        auto&       vkSubpass   = vkSubpasses[i];

        InitSubpassDescription(vkSubpass);
        vkSubpass.flags             = 0;
        vkSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        std::fill(AttachmentStates.begin(), AttachmentStates.end(), RESOURCE_STATE_UNKNOWN);
        auto UpdateAttachmentsStates = [&AttachmentStates](Uint32 NumAttachments, const AttachmentReference* pSrcAttachments) //
        {
            if (pSrcAttachments == nullptr)
                return;
            for (Uint32 attachment = 0; attachment < NumAttachments; ++attachment)
            {
                const auto& SrcAttachmentRef = pSrcAttachments[attachment];
                if (SrcAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED)
                    AttachmentStates[SrcAttachmentRef.AttachmentIndex] |= SrcAttachmentRef.State;
            }
        };
        UpdateAttachmentsStates(SubpassDesc.InputAttachmentCount, SubpassDesc.pInputAttachments);
        UpdateAttachmentsStates(SubpassDesc.RenderTargetAttachmentCount, SubpassDesc.pRenderTargetAttachments);
        UpdateAttachmentsStates(1, SubpassDesc.pDepthStencilAttachment);

        auto ConvertAttachmentReferences = [&](Uint32 NumAttachments, const AttachmentReference* pSrcAttachments, VkImageAspectFlags AspectMask) //
        {
            auto* pCurrVkAttachmentReference = &vkAttachmentReferences[CurrAttachmentReferenceInd];
            for (Uint32 attachment = 0; attachment < NumAttachments; ++attachment, ++CurrAttachmentReferenceInd)
            {
                const auto& SrcAttachmentRef = pSrcAttachments[attachment];
                auto&       DstAttachmentRef = vkAttachmentReferences[CurrAttachmentReferenceInd];

                InitAttachmentReference(DstAttachmentRef, AspectMask);
                DstAttachmentRef.attachment = SrcAttachmentRef.AttachmentIndex;

                auto State = SrcAttachmentRef.AttachmentIndex != ATTACHMENT_UNUSED ?
                    AttachmentStates[SrcAttachmentRef.AttachmentIndex] :
                    SrcAttachmentRef.State;
                if (PlatformMisc::CountOneBits(State) >= 2)
                {
                    // The same attachment is used in different ways in this subpass (e.g. color and input attachments).
                    // It must use COMMON layout.
                    State = RESOURCE_STATE_COMMON;
                }
                else
                {
                    VERIFY_EXPR(State == RESOURCE_STATE_UNKNOWN || State == SrcAttachmentRef.State);
                    State = SrcAttachmentRef.State;
                }

                DstAttachmentRef.layout = ResourceStateToVkImageLayout(State, /*IsInsideRenderPass = */ true, FragDensityMapEnabled);
            }
            return pCurrVkAttachmentReference;
        };

        vkSubpass.inputAttachmentCount = SubpassDesc.InputAttachmentCount;
        if (SubpassDesc.InputAttachmentCount != 0)
        {
            vkSubpass.pInputAttachments = ConvertAttachmentReferences(SubpassDesc.InputAttachmentCount, SubpassDesc.pInputAttachments, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        vkSubpass.colorAttachmentCount = SubpassDesc.RenderTargetAttachmentCount;
        if (SubpassDesc.RenderTargetAttachmentCount != 0)
        {
            vkSubpass.pColorAttachments = ConvertAttachmentReferences(SubpassDesc.RenderTargetAttachmentCount, SubpassDesc.pRenderTargetAttachments, VK_IMAGE_ASPECT_COLOR_BIT);
            if (SubpassDesc.pResolveAttachments != nullptr)
            {
                vkSubpass.pResolveAttachments = ConvertAttachmentReferences(SubpassDesc.RenderTargetAttachmentCount, SubpassDesc.pResolveAttachments, VK_IMAGE_ASPECT_COLOR_BIT);
            }
        }

        if (SubpassDesc.pDepthStencilAttachment != nullptr)
        {
            vkSubpass.pDepthStencilAttachment = ConvertAttachmentReferences(1, SubpassDesc.pDepthStencilAttachment, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
        }

        vkSubpass.preserveAttachmentCount = SubpassDesc.PreserveAttachmentCount;
        if (SubpassDesc.PreserveAttachmentCount != 0)
        {
            vkSubpass.pPreserveAttachments = &vkPreserveAttachments[CurrPreserveAttachmentInd];
            for (Uint32 prsv_attachment = 0; prsv_attachment < SubpassDesc.PreserveAttachmentCount; ++prsv_attachment, ++CurrPreserveAttachmentInd)
            {
                vkPreserveAttachments[CurrPreserveAttachmentInd] = SubpassDesc.pPreserveAttachments[prsv_attachment];
            }
        }

        if (SubpassDesc.pShadingRateAttachment != nullptr)
        {
            if (ShadingRateEnabled)
            {
                const auto& SRAttachment   = *SubpassDesc.pShadingRateAttachment;
                auto&       vkSRAttachment = vkShadingRate[SRInd++];

                SetSubpassDescriptionNext(vkSubpass, &vkSRAttachment);
                vkSRAttachment.pNext                          = nullptr;
                vkSRAttachment.sType                          = VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
                vkSRAttachment.pFragmentShadingRateAttachment = reinterpret_cast<const VkAttachmentReference2*>(ConvertAttachmentReferences(1, &SRAttachment.Attachment, VK_IMAGE_ASPECT_COLOR_BIT));
                vkSRAttachment.shadingRateAttachmentTexelSize = {SRAttachment.TileSize[0], SRAttachment.TileSize[1]};
            }
            else
            {
                VERIFY_EXPR(FragDensityMapEnabled);
                pMainSRA = pMainSRA ? pMainSRA : SubpassDesc.pShadingRateAttachment;
            }
        }
    }

    if (FragDensityMapEnabled && pMainSRA != nullptr)
    {
        for (Uint32 i = 0; i < m_Desc.SubpassCount; ++i)
        {
            const auto& SubpassDesc = m_Desc.pSubpasses[i];

            if (SubpassDesc.pShadingRateAttachment == nullptr)
                LOG_ERROR_AND_THROW("Vk_EXT_fragment_density_map extension requires that shading rate attachment is specified for all subpasses");

            if (*pMainSRA != *SubpassDesc.pShadingRateAttachment)
                LOG_ERROR_AND_THROW("Vk_EXT_fragment_density_map extension requires that shading rate attachment is the same for all subpasses");
        }
    }

    VERIFY_EXPR(CurrAttachmentReferenceInd == vkAttachmentReferences.size());
    VERIFY_EXPR(CurrPreserveAttachmentInd == vkPreserveAttachments.size());
    RenderPassCI.subpassCount = m_Desc.SubpassCount;
    RenderPassCI.pSubpasses   = vkSubpasses.data();

    std::vector<SubpassDependencyType> vkDependencies(m_Desc.DependencyCount);
    for (Uint32 i = 0; i < m_Desc.DependencyCount; ++i)
    {
        const auto& DependencyDesc = m_Desc.pDependencies[i];
        auto&       vkDependency   = vkDependencies[i];

        InitSubpassDependency(vkDependency);
        vkDependency.srcSubpass    = DependencyDesc.SrcSubpass;
        vkDependency.dstSubpass    = DependencyDesc.DstSubpass;
        vkDependency.srcStageMask  = PipelineStageFlagsToVkPipelineStageFlags(DependencyDesc.SrcStageMask);
        vkDependency.dstStageMask  = PipelineStageFlagsToVkPipelineStageFlags(DependencyDesc.DstStageMask);
        vkDependency.srcAccessMask = AccessFlagsToVkAccessFlags(DependencyDesc.SrcAccessMask);
        vkDependency.dstAccessMask = AccessFlagsToVkAccessFlags(DependencyDesc.DstAccessMask);

        // VK_DEPENDENCY_BY_REGION_BIT specifies that dependencies will be framebuffer-local.
        // Framebuffer-local dependencies are more optimal for most architectures; particularly
        // tile-based architectures - which can keep framebuffer-regions entirely in on-chip registers
        // and thus avoid external bandwidth across such a dependency. Including a framebuffer-global
        // dependency in your rendering will usually force all implementations to flush data to memory,
        // or to a higher level cache, breaking any potential locality optimizations.
        vkDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    }
    RenderPassCI.dependencyCount = m_Desc.DependencyCount;
    RenderPassCI.pDependencies   = vkDependencies.data();

    // Enable fragment density map
    VkRenderPassFragmentDensityMapCreateInfoEXT FragDensityMapCI{};
    if (FragDensityMapEnabled && pMainSRA != nullptr)
    {
        RenderPassCI.pNext     = &FragDensityMapCI;
        FragDensityMapCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT;
        FragDensityMapCI.pNext = nullptr;

        FragDensityMapCI.fragmentDensityMapAttachment.attachment = pMainSRA->Attachment.AttachmentIndex;
        FragDensityMapCI.fragmentDensityMapAttachment.layout     = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
    }

    m_VkRenderPass = LogicalDevice.CreateRenderPass(RenderPassCI, m_Desc.Name);
    if (!m_VkRenderPass)
    {
        LOG_ERROR_AND_THROW("Failed to create Vulkan render pass");
    }
}

RenderPassVkImpl::~RenderPassVkImpl()
{
    m_pDevice->SafeReleaseDeviceObject(std::move(m_VkRenderPass), ~Uint64{0});
}

} // namespace Diligent
