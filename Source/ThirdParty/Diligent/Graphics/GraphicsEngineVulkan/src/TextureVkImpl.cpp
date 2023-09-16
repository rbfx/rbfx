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
#include "TextureVkImpl.hpp"
#include "RenderDeviceVkImpl.hpp"
#include "DeviceContextVkImpl.hpp"
#include "TextureViewVkImpl.hpp"
#include "VulkanTypeConversions.hpp"
#include "EngineMemory.h"
#include "StringTools.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

VkImageCreateInfo TextureDescToVkImageCreateInfo(const TextureDesc& Desc, const RenderDeviceVkImpl* pRenderDeviceVk) noexcept
{
    const auto  IsMemoryless         = (Desc.MiscFlags & MISC_TEXTURE_FLAG_MEMORYLESS) != 0;
    const auto& FmtAttribs           = GetTextureFormatAttribs(Desc.Format);
    const auto  ImageView2DSupported = !Desc.Is3D() || pRenderDeviceVk->GetAdapterInfo().Texture.TextureView2DOn3DSupported;
    const auto& ExtFeatures          = pRenderDeviceVk->GetLogicalDevice().GetEnabledExtFeatures();

    VkImageCreateInfo ImageCI = {};

    ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageCI.pNext = nullptr;
    ImageCI.flags = 0;
    if (Desc.Type == RESOURCE_DIM_TEX_CUBE || Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        ImageCI.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    if (FmtAttribs.IsTypeless)
        ImageCI.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT; // Specifies that the image can be used to create a
                                                             // VkImageView with a different format from the image.

    if (Desc.Is1D())
        ImageCI.imageType = VK_IMAGE_TYPE_1D;
    else if (Desc.Is2D())
        ImageCI.imageType = VK_IMAGE_TYPE_2D;
    else if (Desc.Is3D())
    {
        ImageCI.imageType = VK_IMAGE_TYPE_3D;
        if (ImageView2DSupported)
            ImageCI.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
    }
    else
    {
        LOG_ERROR_AND_THROW("Unknown texture type");
    }

    TEXTURE_FORMAT InternalTexFmt = Desc.Format;
    if (FmtAttribs.IsTypeless)
    {
        TEXTURE_VIEW_TYPE PrimaryViewType;
        if (Desc.BindFlags & BIND_DEPTH_STENCIL)
            PrimaryViewType = TEXTURE_VIEW_DEPTH_STENCIL;
        else if (Desc.BindFlags & BIND_UNORDERED_ACCESS)
            PrimaryViewType = TEXTURE_VIEW_UNORDERED_ACCESS;
        else if (Desc.BindFlags & BIND_RENDER_TARGET)
            PrimaryViewType = TEXTURE_VIEW_RENDER_TARGET;
        else
            PrimaryViewType = TEXTURE_VIEW_SHADER_RESOURCE;
        InternalTexFmt = GetDefaultTextureViewFormat(Desc, PrimaryViewType);
    }

    ImageCI.format = TexFormatToVkFormat(InternalTexFmt);

    ImageCI.extent.width  = Desc.GetWidth();
    ImageCI.extent.height = Desc.GetHeight();
    ImageCI.extent.depth  = Desc.GetDepth();
    ImageCI.mipLevels     = Desc.MipLevels;
    ImageCI.arrayLayers   = Desc.GetArraySize();

    ImageCI.samples = static_cast<VkSampleCountFlagBits>(Desc.SampleCount);
    ImageCI.tiling  = VK_IMAGE_TILING_OPTIMAL;

    ImageCI.usage = BindFlagsToVkImageUsage(Desc.BindFlags, IsMemoryless, ExtFeatures.FragmentDensityMap.fragmentDensityMap != VK_FALSE);
    // TRANSFER_SRC_BIT and TRANSFER_DST_BIT are required by CopyTexture
    ImageCI.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (Desc.BindFlags & (BIND_DEPTH_STENCIL | BIND_RENDER_TARGET))
        DEV_CHECK_ERR(ImageView2DSupported, "imageView2DOn3DImage in VkPhysicalDevicePortabilitySubsetFeaturesKHR is not enabled, can not create depth-stencil target with 2D image view");

    if (Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS)
    {
        VERIFY_EXPR(!IsMemoryless);
#ifdef DILIGENT_DEVELOPMENT
        {
            const auto& PhysicalDevice = pRenderDeviceVk->GetPhysicalDevice();
            const auto  FmtProperties  = PhysicalDevice.GetPhysicalDeviceFormatProperties(ImageCI.format);
            DEV_CHECK_ERR((FmtProperties.optimalTilingFeatures & (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT)) == (VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT),
                          "Automatic mipmap generation is not supported for ", GetTextureFormatAttribs(InternalTexFmt).Name,
                          " as the format does not support blitting.");

            DEV_CHECK_ERR((FmtProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0,
                          "Automatic mipmap generation is not supported for ", GetTextureFormatAttribs(InternalTexFmt).Name,
                          " as the format does not support linear filtering.");
        }
#endif
    }
    if (Desc.MiscFlags & MISC_TEXTURE_FLAG_SUBSAMPLED)
    {
        ImageCI.usage &= ~(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
        ImageCI.flags |= VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
    }

    ImageCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    ImageCI.queueFamilyIndexCount = 0;
    ImageCI.pQueueFamilyIndices   = nullptr;

    if (Desc.Usage == USAGE_SPARSE)
    {
        ImageCI.flags &= ~VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT; // not compatible
        ImageCI.flags |=
            VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
            VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT |
            (Desc.MiscFlags & MISC_TEXTURE_FLAG_SPARSE_ALIASING ? VK_IMAGE_CREATE_SPARSE_ALIASED_BIT : 0);
    }

    return ImageCI;
}


TextureVkImpl::TextureVkImpl(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceVkImpl*        pRenderDeviceVk,
                             const TextureDesc&         TexDesc,
                             const TextureData*         pInitData /*= nullptr*/) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pRenderDeviceVk,
        TexDesc
    }
// clang-format on
{
    if (m_Desc.Usage == USAGE_IMMUTABLE && (pInitData == nullptr || pInitData->pSubResources == nullptr))
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time: pInitData can't be null");

    const auto IsMemoryless = (m_Desc.MiscFlags & MISC_TEXTURE_FLAG_MEMORYLESS) != 0;
    if (IsMemoryless && pInitData != nullptr && pInitData->pSubResources != nullptr)
        LOG_ERROR_AND_THROW("Memoryless textures can't be initialized");

    if (m_Desc.Usage == USAGE_SPARSE && m_Desc.Is3D() && (m_Desc.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL)) != 0)
        LOG_ERROR_AND_THROW("Sparse 3D texture with BIND_RENDER_TARGET or BIND_DEPTH_STENCIL is not supported in Vulkan");

    const auto& FmtAttribs    = GetTextureFormatAttribs(m_Desc.Format);
    const auto& LogicalDevice = pRenderDeviceVk->GetLogicalDevice();

    if (m_Desc.Usage == USAGE_IMMUTABLE || m_Desc.Usage == USAGE_DEFAULT || m_Desc.Usage == USAGE_DYNAMIC || m_Desc.Usage == USAGE_SPARSE)
    {
        VERIFY(m_Desc.Usage != USAGE_DYNAMIC || PlatformMisc::CountOneBits(m_Desc.ImmediateContextMask) <= 1,
               "ImmediateContextMask must contain single set bit, this error should've been handled in ValidateTextureDesc()");

        VkImageCreateInfo ImageCI = TextureDescToVkImageCreateInfo(m_Desc, pRenderDeviceVk);

        const auto QueueFamilyIndices = PlatformMisc::CountOneBits(m_Desc.ImmediateContextMask) > 1 ?
            GetDevice()->ConvertCmdQueueIdsToQueueFamilies(m_Desc.ImmediateContextMask) :
            std::vector<uint32_t>{};
        if (QueueFamilyIndices.size() > 1)
        {
            // If sharingMode is VK_SHARING_MODE_CONCURRENT, queueFamilyIndexCount must be greater than 1
            ImageCI.sharingMode           = VK_SHARING_MODE_CONCURRENT;
            ImageCI.pQueueFamilyIndices   = QueueFamilyIndices.data();
            ImageCI.queueFamilyIndexCount = static_cast<uint32_t>(QueueFamilyIndices.size());
        }

        // initialLayout must be either VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED (11.4)
        // If it is VK_IMAGE_LAYOUT_PREINITIALIZED, then the image data can be preinitialized by the host
        // while using this layout, and the transition away from this layout will preserve that data.
        // If it is VK_IMAGE_LAYOUT_UNDEFINED, then the contents of the data are considered to be undefined,
        // and the transition away from this layout is not guaranteed to preserve that data.
        ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (m_Desc.Usage == USAGE_SPARSE)
        {
            m_VulkanImage = LogicalDevice.CreateImage(ImageCI, m_Desc.Name);

            SetState(RESOURCE_STATE_UNDEFINED);

            InitSparseProperties();
        }
        else
        {
            m_VulkanImage = LogicalDevice.CreateImage(ImageCI, m_Desc.Name);

            VkMemoryRequirements MemReqs = LogicalDevice.GetImageMemoryRequirements(m_VulkanImage);

            const auto ImageMemoryFlags = IsMemoryless ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VERIFY(IsPowerOfTwo(MemReqs.alignment), "Alignment is not power of 2!");
            m_MemoryAllocation = pRenderDeviceVk->AllocateMemory(MemReqs, ImageMemoryFlags);
            if (!m_MemoryAllocation)
                LOG_ERROR_AND_THROW("Failed to allocate memory for texture '", m_Desc.Name, "'.");

            auto AlignedOffset = AlignUp(m_MemoryAllocation.UnalignedOffset, MemReqs.alignment);
            VERIFY_EXPR(m_MemoryAllocation.Size >= MemReqs.size + (AlignedOffset - m_MemoryAllocation.UnalignedOffset));
            auto Memory = m_MemoryAllocation.Page->GetVkMemory();
            auto err    = LogicalDevice.BindImageMemory(m_VulkanImage, Memory, AlignedOffset);
            CHECK_VK_ERROR_AND_THROW(err, "Failed to bind image memory");

            if (pInitData != nullptr && pInitData->pSubResources != nullptr && pInitData->NumSubresources > 0)
                InitializeTextureContent(*pInitData, FmtAttribs, ImageCI);
            else
                SetState(RESOURCE_STATE_UNDEFINED);
        }
    }
    else if (m_Desc.Usage == USAGE_STAGING)
    {
        CreateStagingTexture(pInitData, FmtAttribs);
    }
    else
    {
        UNSUPPORTED("Unsupported usage ", GetUsageString(m_Desc.Usage));
    }

    VERIFY_EXPR(IsInKnownState());
}

void TextureVkImpl::InitializeTextureContent(const TextureData&          InitData,
                                             const TextureFormatAttribs& FmtAttribs,
                                             const VkImageCreateInfo&    ImageCI) noexcept(false)
{
    const auto& LogicalDevice = GetDevice()->GetLogicalDevice();

    const auto CmdQueueInd = InitData.pContext ?
        ClassPtrCast<DeviceContextVkImpl>(InitData.pContext)->GetCommandQueueId() :
        SoftwareQueueIndex{PlatformMisc::GetLSB(m_Desc.ImmediateContextMask)};

    // Vulkan validation layers do not like uninitialized memory, so if no initial data
    // is provided, we will clear the memory

    VulkanUtilities::CommandPoolWrapper  CmdPool;
    VulkanUtilities::VulkanCommandBuffer CmdBuffer;
    GetDevice()->AllocateTransientCmdPool(CmdQueueInd, CmdPool, CmdBuffer, "Transient command pool to copy staging data to a device buffer");

    VkImageAspectFlags aspectMask = 0;
    if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
    {
        UNSUPPORTED("Initializing depth-stencil texture is not currently supported");
        // Only single aspect bit must be specified when copying texture data

        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    // For either clear or copy command, dst layout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    VkImageSubresourceRange SubresRange;
    SubresRange.aspectMask     = aspectMask;
    SubresRange.baseArrayLayer = 0;
    SubresRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    SubresRange.baseMipLevel   = 0;
    SubresRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    CmdBuffer.TransitionImageLayout(m_VulkanImage, ImageCI.initialLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubresRange, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    SetState(RESOURCE_STATE_COPY_DEST);
    const auto CurrentLayout = GetLayout();
    VERIFY_EXPR(CurrentLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    Uint32 ExpectedNumSubresources = ImageCI.mipLevels * ImageCI.arrayLayers;
    if (InitData.NumSubresources != ExpectedNumSubresources)
        LOG_ERROR_AND_THROW("Incorrect number of subresources in init data. ", ExpectedNumSubresources, " expected, while ", InitData.NumSubresources, " provided");

    std::vector<VkBufferImageCopy> Regions(InitData.NumSubresources);

    Uint64 uploadBufferSize = 0;
    Uint32 subres           = 0;
    for (Uint32 layer = 0; layer < ImageCI.arrayLayers; ++layer)
    {
        for (Uint32 mip = 0; mip < ImageCI.mipLevels; ++mip)
        {
            const auto& SubResData = InitData.pSubResources[subres];
            (void)SubResData;
            auto& CopyRegion = Regions[subres];

            auto MipInfo = GetMipLevelProperties(m_Desc, mip);

            CopyRegion.bufferOffset = uploadBufferSize; // offset in bytes from the start of the buffer object
            // bufferRowLength and bufferImageHeight specify the data in buffer memory as a subregion
            // of a larger two- or three-dimensional image, and control the addressing calculations of
            // data in buffer memory. If either of these values is zero, that aspect of the buffer memory
            // is considered to be tightly packed according to the imageExtent. (18.4)
            CopyRegion.bufferRowLength   = 0;
            CopyRegion.bufferImageHeight = 0;
            // For block-compression formats, all parameters are still specified in texels rather than compressed texel blocks (18.4.1)
            CopyRegion.imageOffset = VkOffset3D{0, 0, 0};
            CopyRegion.imageExtent = VkExtent3D{MipInfo.LogicalWidth, MipInfo.LogicalHeight, MipInfo.Depth};

            CopyRegion.imageSubresource.aspectMask     = aspectMask;
            CopyRegion.imageSubresource.mipLevel       = mip;
            CopyRegion.imageSubresource.baseArrayLayer = layer;
            CopyRegion.imageSubresource.layerCount     = 1;

            VERIFY(SubResData.Stride == 0 || SubResData.Stride >= MipInfo.RowSize, "Stride is too small");
            // For compressed-block formats, MipInfo.RowSize is the size of one row of blocks
            VERIFY(SubResData.DepthStride == 0 || SubResData.DepthStride >= (MipInfo.StorageHeight / FmtAttribs.BlockHeight) * MipInfo.RowSize, "Depth stride is too small");

            // bufferOffset must be a multiple of 4 (18.4)
            // If the calling command's VkImage parameter is a compressed image, bufferOffset
            // must be a multiple of the compressed texel block size in bytes (18.4). This
            // is automatically guaranteed as MipWidth and MipHeight are rounded to block size
            uploadBufferSize += (MipInfo.MipSize + 3) & (~3);
            ++subres;
        }
    }
    VERIFY_EXPR(subres == InitData.NumSubresources);

    VkBufferCreateInfo VkStagingBuffCI    = {};
    VkStagingBuffCI.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    VkStagingBuffCI.pNext                 = nullptr;
    VkStagingBuffCI.flags                 = 0;
    VkStagingBuffCI.size                  = uploadBufferSize;
    VkStagingBuffCI.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkStagingBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    VkStagingBuffCI.queueFamilyIndexCount = 0;
    VkStagingBuffCI.pQueueFamilyIndices   = nullptr;

    std::string StagingBufferName = "Upload buffer for '";
    StagingBufferName += m_Desc.Name;
    StagingBufferName += '\'';
    VulkanUtilities::BufferWrapper StagingBuffer = LogicalDevice.CreateBuffer(VkStagingBuffCI, StagingBufferName.c_str());

    VkMemoryRequirements StagingBufferMemReqs = LogicalDevice.GetBufferMemoryRequirements(StagingBuffer);
    VERIFY(IsPowerOfTwo(StagingBufferMemReqs.alignment), "Alignment is not power of 2!");
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
    // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible
    // to the host (10.2)
    auto StagingMemoryAllocation = GetDevice()->AllocateMemory(StagingBufferMemReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (!StagingMemoryAllocation)
        LOG_ERROR_AND_THROW("Failed to allocate staging memory for texture '", m_Desc.Name, "'.");

    auto StagingBufferMemory     = StagingMemoryAllocation.Page->GetVkMemory();
    auto AlignedStagingMemOffset = AlignUp(StagingMemoryAllocation.UnalignedOffset, StagingBufferMemReqs.alignment);
    VERIFY_EXPR(StagingMemoryAllocation.Size >= StagingBufferMemReqs.size + (AlignedStagingMemOffset - StagingMemoryAllocation.UnalignedOffset));

    auto* StagingData = reinterpret_cast<uint8_t*>(StagingMemoryAllocation.Page->GetCPUMemory());
    VERIFY_EXPR(StagingData != nullptr);
    StagingData += AlignedStagingMemOffset;

    subres = 0;
    for (Uint32 layer = 0; layer < ImageCI.arrayLayers; ++layer)
    {
        for (Uint32 mip = 0; mip < ImageCI.mipLevels; ++mip)
        {
            const auto& SubResData = InitData.pSubResources[subres];
            const auto& CopyRegion = Regions[subres];

            auto MipInfo = GetMipLevelProperties(m_Desc, mip);

            VERIFY_EXPR(MipInfo.LogicalWidth == CopyRegion.imageExtent.width);
            VERIFY_EXPR(MipInfo.LogicalHeight == CopyRegion.imageExtent.height);
            VERIFY_EXPR(MipInfo.Depth == CopyRegion.imageExtent.depth);

            VERIFY(SubResData.Stride == 0 || SubResData.Stride >= MipInfo.RowSize, "Stride is too small");
            // For compressed-block formats, MipInfo.RowSize is the size of one row of blocks
            VERIFY(SubResData.DepthStride == 0 || SubResData.DepthStride >= (MipInfo.StorageHeight / FmtAttribs.BlockHeight) * MipInfo.RowSize, "Depth stride is too small");

            for (Uint32 z = 0; z < MipInfo.Depth; ++z)
            {
                for (Uint32 y = 0; y < MipInfo.StorageHeight; y += FmtAttribs.BlockHeight)
                {
                    memcpy(StagingData + CopyRegion.bufferOffset + ((y + z * MipInfo.StorageHeight) / FmtAttribs.BlockHeight) * MipInfo.RowSize,
                           // SubResData.Stride must be the stride of one row of compressed blocks
                           reinterpret_cast<const uint8_t*>(SubResData.pData) + (y / FmtAttribs.BlockHeight) * SubResData.Stride + z * SubResData.DepthStride,
                           StaticCast<size_t>(MipInfo.RowSize));
                }
            }

            ++subres;
        }
    }
    VERIFY_EXPR(subres == InitData.NumSubresources);

    auto err = LogicalDevice.BindBufferMemory(StagingBuffer, StagingBufferMemory, AlignedStagingMemOffset);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to bind staging buffer memory");

    CmdBuffer.MemoryBarrier(VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Copy commands MUST be recorded outside of a render pass instance. This is OK here
    // as copy will be the only command in the cmd buffer
    CmdBuffer.CopyBufferToImage(StagingBuffer, m_VulkanImage,
                                CurrentLayout, // dstImageLayout must be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL or VK_IMAGE_LAYOUT_GENERAL (18.4)
                                static_cast<uint32_t>(Regions.size()), Regions.data());

    GetDevice()->ExecuteAndDisposeTransientCmdBuff(CmdQueueInd, CmdBuffer.GetVkCmdBuffer(), std::move(CmdPool));

    // After command buffer is submitted, safe-release resources. This strategy
    // is little overconservative as the resources will be released after the first
    // command buffer submitted through the immediate context will be completed
    GetDevice()->SafeReleaseDeviceObject(std::move(StagingBuffer), Uint64{1} << Uint64{CmdQueueInd});
    GetDevice()->SafeReleaseDeviceObject(std::move(StagingMemoryAllocation), Uint64{1} << Uint64{CmdQueueInd});
}

void TextureVkImpl::CreateStagingTexture(const TextureData* pInitData, const TextureFormatAttribs& FmtAttribs)
{
    const bool  bInitializeTexture = (pInitData != nullptr && pInitData->pSubResources != nullptr && pInitData->NumSubresources > 0);
    const auto& LogicalDevice      = GetDevice()->GetLogicalDevice();

    VkBufferCreateInfo VkStagingBuffCI = {};

    VkStagingBuffCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    VkStagingBuffCI.pNext = nullptr;
    VkStagingBuffCI.flags = 0;
    VkStagingBuffCI.size  = GetStagingTextureSubresourceOffset(m_Desc, m_Desc.GetArraySize(), 0, StagingBufferOffsetAlignment);

    // clang-format off
        DEV_CHECK_ERR((m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_READ ||
                      (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_WRITE,
                      "Exactly one of CPU_ACCESS_READ or CPU_ACCESS_WRITE flags must be specified");
    // clang-format on
    VkMemoryPropertyFlags MemProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    if (m_Desc.CPUAccessFlags & CPU_ACCESS_READ)
    {
        DEV_CHECK_ERR(!bInitializeTexture, "Readback textures should not be initialized with data");

        VkStagingBuffCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        MemProperties |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        SetState(RESOURCE_STATE_COPY_DEST);

        // We do not set HOST_COHERENT bit, so we will have to use InvalidateMappedMemoryRanges,
        // which requires the ranges to be aligned by nonCoherentAtomSize.
        const auto& DeviceLimits = GetDevice()->GetPhysicalDevice().GetProperties().limits;
        // Align the buffer size to ensure that any aligned range is always in bounds.
        VkStagingBuffCI.size = AlignUp(VkStagingBuffCI.size, DeviceLimits.nonCoherentAtomSize);
    }
    else if (m_Desc.CPUAccessFlags & CPU_ACCESS_WRITE)
    {
        VkStagingBuffCI.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT bit specifies that the host cache management commands vkFlushMappedMemoryRanges
        // and vkInvalidateMappedMemoryRanges are NOT needed to flush host writes to the device or make device writes visible
        // to the host (10.2)
        MemProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        SetState(RESOURCE_STATE_COPY_SOURCE);
    }
    else
        UNEXPECTED("Unexpected CPU access");

    VkStagingBuffCI.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    VkStagingBuffCI.queueFamilyIndexCount = 0;
    VkStagingBuffCI.pQueueFamilyIndices   = nullptr;

    std::string StagingBufferName = "Staging buffer for '";
    StagingBufferName += m_Desc.Name;
    StagingBufferName += '\'';
    m_StagingBuffer = LogicalDevice.CreateBuffer(VkStagingBuffCI, StagingBufferName.c_str());

    VkMemoryRequirements StagingBufferMemReqs = LogicalDevice.GetBufferMemoryRequirements(m_StagingBuffer);
    VERIFY(IsPowerOfTwo(StagingBufferMemReqs.alignment), "Alignment is not power of 2!");

    m_MemoryAllocation = GetDevice()->AllocateMemory(StagingBufferMemReqs, MemProperties);
    if (!m_MemoryAllocation)
        LOG_ERROR_AND_THROW("Failed to allocate memory for staging texture '", m_Desc.Name, "'.");

    auto StagingBufferMemory     = m_MemoryAllocation.Page->GetVkMemory();
    auto AlignedStagingMemOffset = AlignUp(m_MemoryAllocation.UnalignedOffset, StagingBufferMemReqs.alignment);
    VERIFY_EXPR(m_MemoryAllocation.Size >= StagingBufferMemReqs.size + (AlignedStagingMemOffset - m_MemoryAllocation.UnalignedOffset));

    auto err = LogicalDevice.BindBufferMemory(m_StagingBuffer, StagingBufferMemory, AlignedStagingMemOffset);
    CHECK_VK_ERROR_AND_THROW(err, "Failed to bind staging buffer memory");

    m_StagingDataAlignedOffset = AlignedStagingMemOffset;

    if (bInitializeTexture)
    {
        uint8_t* const pStagingData = GetStagingDataCPUAddress();

        Uint32 subres = 0;
        for (Uint32 layer = 0; layer < m_Desc.GetArraySize(); ++layer)
        {
            for (Uint32 mip = 0; mip < m_Desc.MipLevels; ++mip)
            {
                const auto& SubResData = pInitData->pSubResources[subres++];
                const auto  MipProps   = GetMipLevelProperties(m_Desc, mip);

                const auto DstSubresOffset =
                    GetStagingTextureSubresourceOffset(m_Desc, layer, mip, StagingBufferOffsetAlignment);

                CopyTextureSubresource(SubResData,
                                       MipProps.StorageHeight / FmtAttribs.BlockHeight, // NumRows
                                       MipProps.Depth,
                                       MipProps.RowSize,
                                       pStagingData + DstSubresOffset,
                                       MipProps.RowSize,       // DstRowStride
                                       MipProps.DepthSliceSize // DstDepthStride
                );
            }
        }
    }
}

TextureVkImpl::TextureVkImpl(IReferenceCounters*        pRefCounters,
                             FixedBlockMemoryAllocator& TexViewObjAllocator,
                             RenderDeviceVkImpl*        pDeviceVk,
                             const TextureDesc&         TexDesc,
                             RESOURCE_STATE             InitialState,
                             VkImage                    VkImageHandle) :
    TTextureBase{pRefCounters, TexViewObjAllocator, pDeviceVk, TexDesc},
    m_VulkanImage{VkImageHandle}
{
    SetState(InitialState);

    if (m_Desc.Usage == USAGE_SPARSE)
        InitSparseProperties();
}

void TextureVkImpl::CreateViewInternal(const TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "View pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto& TexViewAllocator = m_pDevice->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        auto UpdatedViewDesc = ViewDesc;
        ValidatedAndCorrectTextureViewDesc(m_Desc, UpdatedViewDesc);

        VulkanUtilities::ImageViewWrapper ImgView = CreateImageView(UpdatedViewDesc);
        auto                              pViewVk = NEW_RC_OBJ(TexViewAllocator, "TextureViewVkImpl instance", TextureViewVkImpl, bIsDefaultView ? this : nullptr)(GetDevice(), UpdatedViewDesc, this, std::move(ImgView), bIsDefaultView);
        VERIFY(pViewVk->GetDesc().ViewType == ViewDesc.ViewType, "Incorrect view type");

        if (bIsDefaultView)
            *ppView = pViewVk;
        else
            pViewVk->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetTexViewTypeLiteralName(ViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", ViewDesc.Name ? ViewDesc.Name : "", "\" (", ViewTypeName, ") for texture \"", m_Desc.Name ? m_Desc.Name : "", "\"");
    }
}

TextureVkImpl::~TextureVkImpl()
{
    // Vk object can only be destroyed when it is no longer used by the GPU
    // Wrappers for external texture will not be destroyed as they are created with null device pointer
    if (m_VulkanImage)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_VulkanImage), m_Desc.ImmediateContextMask);
    if (m_StagingBuffer)
        m_pDevice->SafeReleaseDeviceObject(std::move(m_StagingBuffer), m_Desc.ImmediateContextMask);
    m_pDevice->SafeReleaseDeviceObject(std::move(m_MemoryAllocation), m_Desc.ImmediateContextMask);
}

VulkanUtilities::ImageViewWrapper TextureVkImpl::CreateImageView(TextureViewDesc& ViewDesc)
{
    // clang-format off
    VERIFY(ViewDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE ||
           ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET ||
           ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL ||
           ViewDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL ||
           ViewDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS ||
           ViewDesc.ViewType == TEXTURE_VIEW_SHADING_RATE,
           "Unexpected view type");
    // clang-format on
    if (ViewDesc.Format == TEX_FORMAT_UNKNOWN)
    {
        ViewDesc.Format = m_Desc.Format;
    }

    VkImageViewCreateInfo ImageViewCI = {};

    ImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ImageViewCI.pNext = nullptr;
    ImageViewCI.flags = 0; // reserved for future use.
    ImageViewCI.image = m_VulkanImage;

    switch (ViewDesc.TextureDim)
    {
        case RESOURCE_DIM_TEX_1D:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_1D;
            break;

        case RESOURCE_DIM_TEX_1D_ARRAY:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;

        case RESOURCE_DIM_TEX_2D:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;

        case RESOURCE_DIM_TEX_2D_ARRAY:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;

        case RESOURCE_DIM_TEX_3D:
            if (ViewDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET ||
                ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL ||
                ViewDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL)
            {
                VERIFY_EXPR(m_pDevice->GetAdapterInfo().Texture.TextureView2DOn3DSupported);
                VERIFY(m_Desc.Usage != USAGE_SPARSE, "Can not create 2D texture view on a 3D sparse texture");

                ViewDesc.TextureDim  = RESOURCE_DIM_TEX_2D_ARRAY;
                ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            }
            else
            {
                ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_3D;
                Uint32 MipDepth      = std::max(m_Desc.Depth >> ViewDesc.MostDetailedMip, 1U);
                if (ViewDesc.FirstDepthSlice != 0 || ViewDesc.NumDepthSlices != MipDepth)
                {
                    LOG_ERROR("3D texture view '", (ViewDesc.Name ? ViewDesc.Name : ""), "' (most detailed mip: ", Uint32{ViewDesc.MostDetailedMip},
                              "; mip levels: ", Uint32{ViewDesc.NumMipLevels}, "; first slice: ", ViewDesc.FirstDepthSlice,
                              "; num depth slices: ", ViewDesc.NumDepthSlices, ") of texture '", m_Desc.Name,
                              "' does not references all depth slices (", MipDepth,
                              ") in the mip level. 3D texture views in Vulkan must address all depth slices.");
                    ViewDesc.FirstDepthSlice = 0;
                    ViewDesc.NumDepthSlices  = MipDepth;
                }
            }
            break;

        case RESOURCE_DIM_TEX_CUBE:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;

        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            ImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;

        default: UNEXPECTED("Unexpected view dimension");
    }

    TEXTURE_FORMAT CorrectedViewFormat = ViewDesc.Format;
    if (m_Desc.BindFlags & BIND_DEPTH_STENCIL)
        CorrectedViewFormat = GetDefaultTextureViewFormat(CorrectedViewFormat, TEXTURE_VIEW_DEPTH_STENCIL, m_Desc.BindFlags);
    ImageViewCI.format = TexFormatToVkFormat(CorrectedViewFormat);
    if (ViewDesc.Format == TEX_FORMAT_A8_UNORM)
    {
        auto GetA8Swizzle = [](TEXTURE_COMPONENT_SWIZZLE Component, TEXTURE_COMPONENT_SWIZZLE Swizzle) {
            if (Swizzle == TEXTURE_COMPONENT_SWIZZLE_ZERO || Swizzle == TEXTURE_COMPONENT_SWIZZLE_ONE)
                return TextureComponentSwizzleToVkComponentSwizzle(Swizzle);

            if (Swizzle == TEXTURE_COMPONENT_SWIZZLE_A || (Component == TEXTURE_COMPONENT_SWIZZLE_A && Swizzle == TEXTURE_COMPONENT_SWIZZLE_IDENTITY))
                return VK_COMPONENT_SWIZZLE_R;

            return VK_COMPONENT_SWIZZLE_ZERO;
        };

        ImageViewCI.components = {
            GetA8Swizzle(TEXTURE_COMPONENT_SWIZZLE_R, ViewDesc.Swizzle.R),
            GetA8Swizzle(TEXTURE_COMPONENT_SWIZZLE_G, ViewDesc.Swizzle.G),
            GetA8Swizzle(TEXTURE_COMPONENT_SWIZZLE_B, ViewDesc.Swizzle.B),
            GetA8Swizzle(TEXTURE_COMPONENT_SWIZZLE_A, ViewDesc.Swizzle.A) //
        };
    }
    else
    {
        ImageViewCI.components = TextureComponentMappingToVkComponentMapping(ViewDesc.Swizzle);
    }

    ImageViewCI.subresourceRange.baseMipLevel = ViewDesc.MostDetailedMip;
    ImageViewCI.subresourceRange.levelCount   = ViewDesc.NumMipLevels;
    if (m_Desc.IsArray())
    {
        ImageViewCI.subresourceRange.baseArrayLayer = ViewDesc.FirstArraySlice;
        ImageViewCI.subresourceRange.layerCount     = ViewDesc.NumArraySlices;
    }
    else
    {
        ImageViewCI.subresourceRange.baseArrayLayer = 0;
        ImageViewCI.subresourceRange.layerCount     = 1;
    }

    const auto& FmtAttribs = GetTextureFormatAttribs(CorrectedViewFormat);

    if (ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL || ViewDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL)
    {
        // When an imageView of a depth/stencil image is used as a depth/stencil framebuffer attachment,
        // the aspectMask is ignored and both depth and stencil image subresources are used. (11.5)
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        else
            UNEXPECTED("Unexpected component type for a depth-stencil view format");
    }
    else
    {
        // aspectMask must be only VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT
        // if format is a color, depth-only or stencil-only format, respectively.  (11.5)
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
        {
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (ViewDesc.Format == TEX_FORMAT_D32_FLOAT_S8X24_UINT ||
                ViewDesc.Format == TEX_FORMAT_D24_UNORM_S8_UINT)
            {
                ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else if (ViewDesc.Format == TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS ||
                     ViewDesc.Format == TEX_FORMAT_R24_UNORM_X8_TYPELESS)
            {
                ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            else if (ViewDesc.Format == TEX_FORMAT_X32_TYPELESS_G8X24_UINT ||
                     ViewDesc.Format == TEX_FORMAT_X24_TYPELESS_G8_UINT)
            {
                ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else
                UNEXPECTED("Unexpected depth-stencil texture format");
        }
        else
            ImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();

    if (ViewDesc.ViewType == TEXTURE_VIEW_SHADING_RATE)
    {
        if (LogicalDevice.GetEnabledExtFeatures().FragmentDensityMap.fragmentDensityMap != VK_FALSE)
        {
            const auto ShadingRateTexAccess = m_pDevice->GetAdapterInfo().ShadingRate.ShadingRateTextureAccess;
            switch (ShadingRateTexAccess)
            {
                case SHADING_RATE_TEXTURE_ACCESS_ON_GPU:
                    ImageViewCI.flags |= VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DYNAMIC_BIT_EXT;
                    break;

                case SHADING_RATE_TEXTURE_ACCESS_ON_SUBMIT:
                    ImageViewCI.flags |= VK_IMAGE_VIEW_CREATE_FRAGMENT_DENSITY_MAP_DEFERRED_BIT_EXT;
                    break;

                case SHADING_RATE_TEXTURE_ACCESS_ON_SET_RTV:
                    break;

                default:
                    UNEXPECTED("Unexpected shading rate access type");
            }
        }
    }

    std::string ViewName = "Image view for \'";
    ViewName += m_Desc.Name;
    ViewName += '\'';
    return LogicalDevice.CreateImageView(ImageViewCI, ViewName.c_str());
}

void TextureVkImpl::SetLayout(VkImageLayout Layout)
{
    SetState(VkImageLayoutToResourceState(Layout));
}

VkImageLayout TextureVkImpl::GetLayout() const
{
    const auto fragmentDensityMap = m_pDevice->GetLogicalDevice().GetEnabledExtFeatures().FragmentDensityMap.fragmentDensityMap;
    return ResourceStateToVkImageLayout(GetState(), /*IsInsideRenderPass = */ false, fragmentDensityMap != VK_FALSE);
}

void TextureVkImpl::InvalidateStagingRange(VkDeviceSize Offset, VkDeviceSize Size)
{
    const auto& LogicalDevice    = m_pDevice->GetLogicalDevice();
    const auto& PhysDeviceLimits = m_pDevice->GetPhysicalDevice().GetProperties().limits;

    VkMappedMemoryRange InvalidateRange{};
    InvalidateRange.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    InvalidateRange.pNext  = nullptr;
    InvalidateRange.memory = m_MemoryAllocation.Page->GetVkMemory();

    Offset += m_StagingDataAlignedOffset;
    auto AlignedOffset = AlignDown(Offset, PhysDeviceLimits.nonCoherentAtomSize);
    Size += Offset - AlignedOffset;
    auto AlignedSize = AlignUp(Size, PhysDeviceLimits.nonCoherentAtomSize);

    InvalidateRange.offset = AlignedOffset;
    InvalidateRange.size   = AlignedSize;

    auto err = LogicalDevice.InvalidateMappedMemoryRanges(1, &InvalidateRange);
    DEV_CHECK_ERR(err == VK_SUCCESS, "Failed to invalidated mapped texture memory range");
    (void)err;
}

void TextureVkImpl::InitSparseProperties() noexcept(false)
{
    VERIFY_EXPR(m_Desc.Usage == USAGE_SPARSE);
    VERIFY_EXPR(m_pSparseProps == nullptr);

    m_pSparseProps = std::make_unique<SparseTextureProperties>();

    const auto& LogicalDevice = m_pDevice->GetLogicalDevice();
    const auto  MemReq        = LogicalDevice.GetImageMemoryRequirements(GetVkImage());

    // If the image was not created with VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT, then pSparseMemoryRequirementCount will be set to zero.
    uint32_t SparseReqCount = 0;
    vkGetImageSparseMemoryRequirements(LogicalDevice.GetVkDevice(), GetVkImage(), &SparseReqCount, nullptr);
    if (SparseReqCount != 1)
        LOG_ERROR_AND_THROW("Sparse memory requirements for texture must be 1");

    // Texture with depth-stencil format may be implemented with two memory blocks per tile.
    VkSparseImageMemoryRequirements SparseReq[2] = {};
    vkGetImageSparseMemoryRequirements(LogicalDevice.GetVkDevice(), GetVkImage(), &SparseReqCount, SparseReq);

    auto& Props{*m_pSparseProps};
    Props.MipTailOffset  = SparseReq[0].imageMipTailOffset;
    Props.MipTailSize    = SparseReq[0].imageMipTailSize;
    Props.MipTailStride  = SparseReq[0].imageMipTailStride;
    Props.FirstMipInTail = SparseReq[0].imageMipTailFirstLod;
    Props.TileSize[0]    = SparseReq[0].formatProperties.imageGranularity.width;
    Props.TileSize[1]    = SparseReq[0].formatProperties.imageGranularity.height;
    Props.TileSize[2]    = SparseReq[0].formatProperties.imageGranularity.depth;
    Props.Flags          = VkSparseImageFormatFlagsToSparseTextureFlags(SparseReq[0].formatProperties.flags);

    if (m_Desc.GetArraySize() == 1)
    {
        VERIFY_EXPR(Props.MipTailOffset < MemReq.size || (Props.MipTailOffset == MemReq.size && Props.MipTailSize == 0));
        VERIFY_EXPR(Props.MipTailOffset + Props.MipTailSize <= MemReq.size);
        Props.MipTailStride = 0;
    }
    else
    {
        if (Props.Flags & SPARSE_TEXTURE_FLAG_SINGLE_MIPTAIL)
            Props.MipTailStride = 0;
        else
            VERIFY_EXPR(Props.MipTailStride > 0);

        VERIFY_EXPR(Props.MipTailStride * m_Desc.GetArraySize() == MemReq.size);
        VERIFY_EXPR(Props.MipTailStride % MemReq.alignment == 0);
        VERIFY_EXPR(Props.MipTailOffset < Props.MipTailStride);
        VERIFY_EXPR(Props.MipTailOffset + Props.MipTailSize <= Props.MipTailStride);
    }

    Props.AddressSpaceSize = MemReq.size;
    Props.BlockSize        = StaticCast<Uint32>(MemReq.alignment);

#ifdef DILIGENT_DEBUG
    const auto&  FmtAttribs    = GetTextureFormatAttribs(m_Desc.Format);
    const Uint32 BytesPerBlock = FmtAttribs.GetElementSize();
    const auto   BytesPerTile =
        (Props.TileSize[0] / FmtAttribs.BlockWidth) *
        (Props.TileSize[1] / FmtAttribs.BlockHeight) *
        Props.TileSize[2] * m_Desc.SampleCount * BytesPerBlock;

    VERIFY(BytesPerTile == Props.BlockSize, "Expected that memory alignment equals to block size");
#endif
}

} // namespace Diligent
