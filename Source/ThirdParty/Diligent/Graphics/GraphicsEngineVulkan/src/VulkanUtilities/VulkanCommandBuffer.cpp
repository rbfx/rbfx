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
#include <sstream>

#include "VulkanUtilities/VulkanCommandBuffer.hpp"
#include "AdvancedMath.hpp"

namespace VulkanUtilities
{

namespace
{

static VkAccessFlags AccessMaskFromImageLayout(VkImageLayout Layout,
                                               bool          IsDstMask // false - source mask
                                                                       // true  - destination mask
)
{
    VkAccessFlags AccessMask = 0;
    switch (Layout)
    {
        // does not support device access. This layout must only be used as the initialLayout member
        // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
        // When transitioning out of this layout, the contents of the memory are not guaranteed to be preserved (11.4)
        case VK_IMAGE_LAYOUT_UNDEFINED:
            if (IsDstMask)
            {
                UNEXPECTED("The new layout used in a transition must not be VK_IMAGE_LAYOUT_UNDEFINED. "
                           "This layout must only be used as the initialLayout member of VkImageCreateInfo "
                           "or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
            }
            break;

        // supports all types of device access
        case VK_IMAGE_LAYOUT_GENERAL:
            // VK_IMAGE_LAYOUT_GENERAL must be used for image load/store operations (13.1.1, 13.2.4)
            AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;

        // must only be used as a color or resolve attachment in a VkFramebuffer (11.4)
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;

        // must only be used as a depth/stencil attachment in a VkFramebuffer (11.4)
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;

        // must only be used as a read-only depth/stencil attachment in a VkFramebuffer and/or as a read-only image in a shader (11.4)
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        // must only be used as a read-only image in a shader (which can be read as a sampled image,
        // combined image/sampler and/or input attachment) (11.4)
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            break;

        //  must only be used as a source image of a transfer command (11.4)
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            AccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            break;

        // must only be used as a destination image of a transfer command (11.4)
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            AccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;

        // does not support device access. This layout must only be used as the initialLayout member
        // of VkImageCreateInfo or VkAttachmentDescription, or as the oldLayout in an image transition.
        // When transitioning out of this layout, the contents of the memory are preserved. (11.4)
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            if (!IsDstMask)
            {
                AccessMask = VK_ACCESS_HOST_WRITE_BIT;
            }
            else
            {
                UNEXPECTED("The new layout used in a transition must not be VK_IMAGE_LAYOUT_PREINITIALIZED. "
                           "This layout must only be used as the initialLayout member of VkImageCreateInfo "
                           "or VkAttachmentDescription, or as the oldLayout in an image transition. (11.4)");
            }
            break;

        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            AccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            break;

        // When transitioning the image to VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR or VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        // there is no need to delay subsequent processing, or perform any visibility operations (as vkQueuePresentKHR
        // performs automatic visibility operations). To achieve this, the dstAccessMask member of the VkImageMemoryBarrier
        // should be set to 0, and the dstStageMask parameter should be set to VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT.
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            AccessMask = 0;
            break;

        case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
            AccessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
            break;

        case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
            AccessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
            break;

        default:
            UNEXPECTED("Unexpected image layout");
            break;
    }

    return AccessMask;
}

} // namespace


VulkanCommandBuffer::VulkanCommandBuffer() noexcept
{
    m_ImageBarriers.reserve(32);
}

void VulkanCommandBuffer::TransitionImageLayout(VkImage                        Image,
                                                VkImageLayout                  OldLayout,
                                                VkImageLayout                  NewLayout,
                                                const VkImageSubresourceRange& SubresRange,
                                                VkPipelineStageFlags           SrcStages,
                                                VkPipelineStageFlags           DstStages)
{
    if (m_State.RenderPass != VK_NULL_HANDLE)
    {
        // Image layout transitions within a render pass execute
        // dependencies between attachments
        EndRenderPass();
    }

    VERIFY_EXPR((SrcStages & m_Barrier.SupportedStagesMask) != 0);
    VERIFY_EXPR((DstStages & m_Barrier.SupportedStagesMask) != 0);

    if (OldLayout == NewLayout)
    {
        m_Barrier.MemorySrcStages |= SrcStages;
        m_Barrier.MemoryDstStages |= DstStages;

        m_Barrier.MemorySrcAccess |= AccessMaskFromImageLayout(OldLayout, false);
        m_Barrier.MemoryDstAccess |= AccessMaskFromImageLayout(NewLayout, true);
        return;
    }

    // Check overlapping subresources
    for (size_t i = 0; i < m_ImageBarriers.size(); ++i)
    {
        const auto& ImgBarrier = m_ImageBarriers[i];
        if (ImgBarrier.image != Image)
            continue;

        const auto& OtherRange = ImgBarrier.subresourceRange;

        const auto StartLayer0 = SubresRange.baseArrayLayer;
        const auto EndLayer0   = SubresRange.layerCount != VK_REMAINING_ARRAY_LAYERS ? (SubresRange.baseArrayLayer + SubresRange.layerCount) : ~0u;
        const auto StartLayer1 = OtherRange.baseArrayLayer;
        const auto EndLayer1   = OtherRange.layerCount != VK_REMAINING_ARRAY_LAYERS ? (OtherRange.baseArrayLayer + OtherRange.layerCount) : ~0u;

        const auto StartMip0 = SubresRange.baseMipLevel;
        const auto EndMip0   = SubresRange.levelCount != VK_REMAINING_MIP_LEVELS ? (SubresRange.baseMipLevel + SubresRange.levelCount) : ~0u;
        const auto StartMip1 = OtherRange.baseMipLevel;
        const auto EndMip1   = OtherRange.levelCount != VK_REMAINING_MIP_LEVELS ? (OtherRange.baseMipLevel + OtherRange.levelCount) : ~0u;

        const auto SlicesOverlap = Diligent::CheckLineSectionOverlap<true>(StartLayer0, EndLayer0, StartLayer1, EndLayer1);
        const auto MipsOverlap   = Diligent::CheckLineSectionOverlap<true>(StartMip0, EndMip0, StartMip1, EndMip1);

        // If the range overlaps with any of the existing barriers, we need to
        // flush them.
        if (SlicesOverlap && MipsOverlap)
        {
            FlushBarriers();
            break;
        }
    }

    m_Barrier.ImageSrcStages |= SrcStages;
    m_Barrier.ImageDstStages |= DstStages;

    VkImageMemoryBarrier ImgBarrier{};
    ImgBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImgBarrier.pNext               = nullptr;
    ImgBarrier.oldLayout           = OldLayout;
    ImgBarrier.newLayout           = NewLayout;
    ImgBarrier.image               = Image;
    ImgBarrier.subresourceRange    = SubresRange;
    ImgBarrier.srcAccessMask       = AccessMaskFromImageLayout(OldLayout, false) & m_Barrier.SupportedAccessMask;
    ImgBarrier.dstAccessMask       = AccessMaskFromImageLayout(NewLayout, true) & m_Barrier.SupportedAccessMask;
    ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // source queue family for a queue family ownership transfer.
    ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // destination queue family for a queue family ownership transfer.
    m_ImageBarriers.emplace_back(ImgBarrier);
}

void VulkanCommandBuffer::MemoryBarrier(VkAccessFlags        srcAccessMask,
                                        VkAccessFlags        dstAccessMask,
                                        VkPipelineStageFlags SrcStages,
                                        VkPipelineStageFlags DstStages)
{
    if (m_State.RenderPass != VK_NULL_HANDLE)
    {
        EndRenderPass();
    }

    VERIFY_EXPR((SrcStages & m_Barrier.SupportedStagesMask) != 0);
    VERIFY_EXPR((DstStages & m_Barrier.SupportedStagesMask) != 0);

    m_Barrier.MemorySrcStages |= SrcStages;
    m_Barrier.MemoryDstStages |= DstStages;

    m_Barrier.MemorySrcAccess |= srcAccessMask;
    m_Barrier.MemoryDstAccess |= dstAccessMask;
}

void VulkanCommandBuffer::FlushBarriers()
{
    if (m_Barrier.MemorySrcStages == 0 && m_Barrier.MemoryDstStages == 0 && m_ImageBarriers.empty())
        return;

    if (m_State.RenderPass != VK_NULL_HANDLE)
    {
        EndRenderPass();
    }

    VERIFY_EXPR(m_VkCmdBuffer != VK_NULL_HANDLE);

    VkMemoryBarrier vkMemBarrier{};
    vkMemBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    vkMemBarrier.pNext         = nullptr;
    vkMemBarrier.srcAccessMask = m_Barrier.MemorySrcAccess & m_Barrier.SupportedAccessMask;
    vkMemBarrier.dstAccessMask = m_Barrier.MemoryDstAccess & m_Barrier.SupportedAccessMask;

    const bool HasMemoryBarrier =
        m_Barrier.MemorySrcStages != 0 && m_Barrier.MemoryDstStages != 0 &&
        m_Barrier.MemorySrcAccess != 0 && m_Barrier.MemoryDstAccess != 0;

    const VkPipelineStageFlags SrcStages = (m_Barrier.ImageSrcStages | m_Barrier.MemorySrcStages) & m_Barrier.SupportedStagesMask;
    const VkPipelineStageFlags DstStages = (m_Barrier.ImageDstStages | m_Barrier.MemoryDstStages) & m_Barrier.SupportedStagesMask;
    VERIFY_EXPR(SrcStages != 0 && DstStages != 0);

    vkCmdPipelineBarrier(m_VkCmdBuffer,
                         SrcStages,
                         DstStages,
                         0,
                         HasMemoryBarrier ? 1 : 0,
                         HasMemoryBarrier ? &vkMemBarrier : nullptr,
                         0,
                         nullptr,
                         static_cast<uint32_t>(m_ImageBarriers.size()),
                         m_ImageBarriers.empty() ? nullptr : m_ImageBarriers.data());

    m_ImageBarriers.clear();
    m_Barrier.ImageSrcStages  = 0;
    m_Barrier.ImageDstStages  = 0;
    m_Barrier.MemorySrcStages = 0;
    m_Barrier.MemoryDstStages = 0;
    m_Barrier.MemorySrcAccess = 0;
    m_Barrier.MemoryDstAccess = 0;
    // Do not clear SupportedStagesMask and SupportedAccessMask
}

} // namespace VulkanUtilities
