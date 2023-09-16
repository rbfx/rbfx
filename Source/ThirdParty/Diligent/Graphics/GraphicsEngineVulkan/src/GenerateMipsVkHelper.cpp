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

#include "GenerateMipsVkHelper.hpp"

#include "DeviceContextVkImpl.hpp"
#include "TextureViewVkImpl.hpp"
#include "TextureVkImpl.hpp"

#include "VulkanTypeConversions.hpp"

namespace Diligent
{

namespace GenerateMipsVkHelper
{

void GenerateMips(TextureViewVkImpl& TexView, DeviceContextVkImpl& Ctx)
{
    auto* pTexVk = TexView.GetTexture<TextureVkImpl>();
    if (!pTexVk->IsInKnownState())
    {
        LOG_ERROR_MESSAGE("Unable to generate mips for texture '", pTexVk->GetDesc().Name, "' because the texture state is unknown");
        return;
    }

    const auto  OriginalState  = pTexVk->GetState();
    const auto  OriginalLayout = pTexVk->GetLayout();
    const auto  OldStages      = ResourceStateFlagsToVkPipelineStageFlags(OriginalState);
    const auto& TexDesc        = pTexVk->GetDesc();
    const auto& ViewDesc       = TexView.GetDesc();

    DEV_CHECK_ERR(ViewDesc.NumMipLevels > 1, "Number of mip levels in the view must be greater than 1");
    DEV_CHECK_ERR(OriginalState != RESOURCE_STATE_UNDEFINED,
                  "Attempting to generate mipmaps for texture '", TexDesc.Name,
                  "' which is in RESOURCE_STATE_UNDEFINED state ."
                  "This is not expected in Vulkan backend as textures are transition to a defined state when created.");

    const auto& FmtAttribs = GetTextureFormatAttribs(ViewDesc.Format);

    VkImageSubresourceRange SubresRange{};
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
        SubresRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
    {
        // If image has a depth / stencil format with both depth and stencil components, then the
        // aspectMask member of subresourceRange must include both VK_IMAGE_ASPECT_DEPTH_BIT and
        // VK_IMAGE_ASPECT_STENCIL_BIT (6.7.3)
        SubresRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        SubresRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    SubresRange.baseArrayLayer = ViewDesc.FirstArraySlice;
    SubresRange.layerCount     = ViewDesc.NumArraySlices;
    SubresRange.baseMipLevel   = ViewDesc.MostDetailedMip;
    SubresRange.levelCount     = 1;

    VkImageBlit BlitRegion{};
    BlitRegion.srcSubresource.baseArrayLayer = ViewDesc.FirstArraySlice;
    BlitRegion.srcSubresource.layerCount     = ViewDesc.NumArraySlices;
    BlitRegion.srcSubresource.aspectMask     = SubresRange.aspectMask;
    BlitRegion.dstSubresource.baseArrayLayer = BlitRegion.srcSubresource.baseArrayLayer;
    BlitRegion.dstSubresource.layerCount     = BlitRegion.srcSubresource.layerCount;
    BlitRegion.dstSubresource.aspectMask     = BlitRegion.srcSubresource.aspectMask;
    BlitRegion.srcOffsets[0]                 = VkOffset3D{0, 0, 0};
    BlitRegion.dstOffsets[0]                 = VkOffset3D{0, 0, 0};

    SubresRange.baseMipLevel = ViewDesc.MostDetailedMip;
    SubresRange.levelCount   = 1;

    auto&      CmdBuffer = Ctx.GetCommandBuffer();
    const auto vkImage   = pTexVk->GetVkImage();
    if (OriginalState != RESOURCE_STATE_COPY_SOURCE)
        CmdBuffer.TransitionImageLayout(vkImage, OriginalLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, SubresRange, OldStages, VK_PIPELINE_STAGE_TRANSFER_BIT);

    for (uint32_t mip = ViewDesc.MostDetailedMip + 1; mip < ViewDesc.MostDetailedMip + ViewDesc.NumMipLevels; ++mip)
    {
        BlitRegion.srcSubresource.mipLevel = mip - 1;
        BlitRegion.dstSubresource.mipLevel = mip;

        BlitRegion.srcOffsets[1] =
            VkOffset3D //
            {
                static_cast<int32_t>(std::max(TexDesc.Width >> (mip - 1), 1u)),
                static_cast<int32_t>(std::max(TexDesc.Height >> (mip - 1), 1u)),
                1 //
            };
        BlitRegion.dstOffsets[1] =
            VkOffset3D //
            {
                static_cast<int32_t>(std::max(TexDesc.Width >> mip, 1u)),
                static_cast<int32_t>(std::max(TexDesc.Height >> mip, 1u)),
                1 //
            };
        if (TexDesc.Type == RESOURCE_DIM_TEX_3D)
        {
            BlitRegion.srcOffsets[1].z = std::max(TexDesc.Depth >> (mip - 1), 1u);
            BlitRegion.dstOffsets[1].z = std::max(TexDesc.Depth >> mip, 1u);
        }

        SubresRange.baseMipLevel = mip;
        if (OriginalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            CmdBuffer.TransitionImageLayout(vkImage, OriginalLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                            SubresRange, OldStages, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }

        // For sRGB source formats, nonlinear RGB values are converted to linear representation prior to filtering.
        // In case of sRGB destination format, linear RGB values are converted to nonlinear representation before writing the pixel to the image.
        CmdBuffer.BlitImage(vkImage,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //  must be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
                            vkImage,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //  must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL
                            1,
                            &BlitRegion,
                            VK_FILTER_LINEAR);
        CmdBuffer.TransitionImageLayout(vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                        SubresRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    }

    const auto AffectedMipLevelLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    // All affected mip levels are now in VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL state
    if (AffectedMipLevelLayout != OriginalLayout)
    {
        bool IsAllSlices = (TexDesc.Type != RESOURCE_DIM_TEX_1D_ARRAY &&
                            TexDesc.Type != RESOURCE_DIM_TEX_2D_ARRAY &&
                            TexDesc.Type != RESOURCE_DIM_TEX_CUBE_ARRAY) ||
            TexDesc.ArraySize == ViewDesc.NumArraySlices;
        bool IsAllMips = ViewDesc.NumMipLevels == TexDesc.MipLevels;
        if (IsAllSlices && IsAllMips)
        {
            pTexVk->SetLayout(AffectedMipLevelLayout);
        }
        else
        {
            VERIFY(OriginalLayout != VK_IMAGE_LAYOUT_UNDEFINED, "Original layout must not be undefined");
            SubresRange.baseMipLevel = ViewDesc.MostDetailedMip;
            SubresRange.levelCount   = ViewDesc.NumMipLevels;
            // Transition all affected subresources back to original layout
            CmdBuffer.FlushBarriers();
            CmdBuffer.TransitionImageLayout(vkImage, AffectedMipLevelLayout, OriginalLayout,
                                            SubresRange, VK_PIPELINE_STAGE_TRANSFER_BIT, OldStages);
            VERIFY_EXPR(pTexVk->GetLayout() == OriginalLayout);
        }
    }
}

} // namespace GenerateMipsVkHelper

} // namespace Diligent
