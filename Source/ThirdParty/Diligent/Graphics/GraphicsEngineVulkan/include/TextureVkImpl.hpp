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

#pragma once

/// \file
/// Declaration of Diligent::TextureVkImpl class

#include "EngineVkImplTraits.hpp"
#include "TextureBase.hpp"
#include "TextureViewVkImpl.hpp"
#include "VulkanUtilities/VulkanMemoryManager.hpp"

namespace Diligent
{

/// Texture object implementation in Vulkan backend.
class TextureVkImpl final : public TextureBase<EngineVkImplTraits>
{
public:
    using TTextureBase = TextureBase<EngineVkImplTraits>;
    using ViewImplType = TextureViewVkImpl;

    // Creates a new Vk resource
    TextureVkImpl(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  RenderDeviceVkImpl*        pDeviceVk,
                  const TextureDesc&         TexDesc,
                  const TextureData*         pInitData = nullptr);

    // Attaches to an existing Vk resource
    TextureVkImpl(IReferenceCounters*        pRefCounters,
                  FixedBlockMemoryAllocator& TexViewObjAllocator,
                  class RenderDeviceVkImpl*  pDeviceVk,
                  const TextureDesc&         TexDesc,
                  RESOURCE_STATE             InitialState,
                  VkImage                    VkImageHandle);

    ~TextureVkImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_TextureVk, TTextureBase)

    /// Implementation of ITextureVk::GetVkImage().
    virtual VkImage DILIGENT_CALL_TYPE GetVkImage() const override final { return m_VulkanImage; }

    /// Implementation of ITexture::GetNativeHandle() in Vulkan backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final { return BitCast<Uint64>(GetVkImage()); }

    /// Implementation of ITextureVk::SetLayout().
    virtual void DILIGENT_CALL_TYPE SetLayout(VkImageLayout Layout) override final;

    /// Implementation of ITextureVk::GetLayout().
    virtual VkImageLayout DILIGENT_CALL_TYPE GetLayout() const override final;

    VkBuffer GetVkStagingBuffer() const
    {
        return m_StagingBuffer;
    }

    uint8_t* GetStagingDataCPUAddress() const
    {
        auto* StagingDataCPUAddress = reinterpret_cast<uint8_t*>(m_MemoryAllocation.Page->GetCPUMemory());
        VERIFY_EXPR(StagingDataCPUAddress != nullptr);
        StagingDataCPUAddress += m_StagingDataAlignedOffset;
        return StagingDataCPUAddress;
    }

    void InvalidateStagingRange(VkDeviceSize Offset, VkDeviceSize Size);

    // For non-compressed color format buffer, the offset must be a multiple of the format's texel block size.
    // For compressed format buffer, the offset must be a multiple of the compressed texel block size in bytes.
    // For depth-stencil format buffer, the offset must be a multiple of 4.
    // If command buffer does not support graphics or compute commands, then the buffer offset must be a multiple of 4.
    // ("Copying Data Between Buffers and Images")
    static constexpr Uint32 StagingBufferOffsetAlignment = 16; // max texel size - 16 bytes (RGBA32F), max texel block size - 16 bytes.

protected:
    void CreateViewInternal(const struct TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView) override;

    void InitializeTextureContent(const TextureData&          InitData,
                                  const TextureFormatAttribs& FmtAttribs,
                                  const VkImageCreateInfo&    ImageCI) noexcept(false);
    void CreateStagingTexture(const TextureData*          pInitData,
                              const TextureFormatAttribs& FmtAttribs);

    VulkanUtilities::ImageViewWrapper CreateImageView(TextureViewDesc& ViewDesc);

    void InitSparseProperties() noexcept(false);

    VulkanUtilities::ImageWrapper           m_VulkanImage;
    VulkanUtilities::BufferWrapper          m_StagingBuffer;
    VulkanUtilities::VulkanMemoryAllocation m_MemoryAllocation;
    VkDeviceSize                            m_StagingDataAlignedOffset = 0;
};

VkImageCreateInfo TextureDescToVkImageCreateInfo(const TextureDesc& Desc, const RenderDeviceVkImpl* pDevice) noexcept;

} // namespace Diligent
