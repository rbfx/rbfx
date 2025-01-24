/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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

#include "TextureWebGPUImpl.hpp"
#include "RenderDeviceWebGPUImpl.hpp"
#include "DeviceContextWebGPUImpl.hpp"
#include "WebGPUTypeConversions.hpp"
#include "GraphicsAccessories.hpp"

namespace Diligent
{

namespace
{

WGPUTextureDescriptor TextureDescToWGPUTextureDescriptor(const TextureDesc&              Desc,
                                                         RenderDeviceWebGPUImpl*         pRenderDevice,
                                                         std::vector<WGPUTextureFormat>& TextureViewFormats) noexcept
{
    WGPUTextureDescriptor wgpuTextureDesc{};

    if (Desc.Type == RESOURCE_DIM_TEX_CUBE)
        DEV_CHECK_ERR(Desc.ArraySize == 6, "Cube textures are expected to have exactly 6 array slices");
    if (Desc.Type == RESOURCE_DIM_TEX_CUBE_ARRAY)
        DEV_CHECK_ERR(Desc.ArraySize % 6 == 0, "Cube texture arrays are expected to have a number of array slices that is a multiple of 6");

    const TextureFormatInfoExt& FmtInfo = pRenderDevice->GetTextureFormatInfoExt(SRGBFormatToUnorm(Desc.Format));

    if (Desc.IsArray())
        wgpuTextureDesc.size.depthOrArrayLayers = Desc.ArraySize;
    else if (Desc.Is3D())
        wgpuTextureDesc.size.depthOrArrayLayers = Desc.Depth;
    else
        wgpuTextureDesc.size.depthOrArrayLayers = 1;

    if (Desc.Is1D())
        wgpuTextureDesc.dimension = WGPUTextureDimension_1D;
    else if (Desc.Is2D())
        wgpuTextureDesc.dimension = WGPUTextureDimension_2D;
    else if (Desc.Is3D())
        wgpuTextureDesc.dimension = WGPUTextureDimension_3D;
    else
        UNEXPECTED("Unknown texture type");

    wgpuTextureDesc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc;
    if (Desc.BindFlags & (BIND_RENDER_TARGET | BIND_DEPTH_STENCIL))
        wgpuTextureDesc.usage |= WGPUTextureUsage_RenderAttachment;
    if (Desc.BindFlags & BIND_UNORDERED_ACCESS)
        wgpuTextureDesc.usage |= WGPUTextureUsage_StorageBinding;
    if (Desc.BindFlags & BIND_SHADER_RESOURCE)
        wgpuTextureDesc.usage |= WGPUTextureUsage_TextureBinding;
    if (Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS)
    {
        if (FmtInfo.BindFlags & BIND_UNORDERED_ACCESS)
            wgpuTextureDesc.usage |= WGPUTextureUsage_StorageBinding;
        else if (FmtInfo.BindFlags & BIND_RENDER_TARGET)
            wgpuTextureDesc.usage |= WGPUTextureUsage_RenderAttachment;
        else
            LOG_ERROR_AND_THROW("Automatic mipmap generation isn't supported for ", GetTextureFormatAttribs(Desc.Format).Name, " as the format can't be used as render target or storage texture");

        if (!FmtInfo.Filterable)
            LOG_ERROR_AND_THROW("Automatic mipmap generation isn't supported for ", GetTextureFormatAttribs(Desc.Format).Name, " as the format doesn't support linear filtering");
    }

    const TextureFormatAttribs& FmtAttribs = GetTextureFormatAttribs(Desc.Format);

    std::unordered_set<TEXTURE_FORMAT> ViewFormatSet;
    if (FmtAttribs.IsTypeless)
    {
        auto InsertViewFormat = [&](TEXTURE_VIEW_TYPE ViewType) {
            TEXTURE_FORMAT Format = GetDefaultTextureViewFormat(Desc, ViewType);
            ViewFormatSet.insert(Format);
            if (ViewType == TEXTURE_VIEW_RENDER_TARGET || ViewType == TEXTURE_VIEW_SHADER_RESOURCE)
                ViewFormatSet.insert(UnormFormatToSRGB(Format));
        };

        if (Desc.BindFlags & BIND_DEPTH_STENCIL)
            LOG_ERROR_AND_THROW("Depth-stencil textures must have a specific format and cannot be typeless in WebGPU");
        if (Desc.BindFlags & BIND_UNORDERED_ACCESS)
            InsertViewFormat(TEXTURE_VIEW_UNORDERED_ACCESS);
        if (Desc.BindFlags & BIND_RENDER_TARGET)
            InsertViewFormat(TEXTURE_VIEW_RENDER_TARGET);
        if (Desc.BindFlags & BIND_SHADER_RESOURCE)
            InsertViewFormat(TEXTURE_VIEW_SHADER_RESOURCE);
    }

    const bool IsSRGBWithMipsAndStorageBinding =
        IsSRGBFormat(Desc.Format) &&
        (Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) &&
        (wgpuTextureDesc.usage & WGPUTextureUsage_StorageBinding);

    wgpuTextureDesc.format = IsSRGBWithMipsAndStorageBinding ?
        TextureFormatToWGPUFormat(SRGBFormatToUnorm(Desc.Format)) :
        TextureFormatToWGPUFormat(Desc.Format);

    if (IsSRGBWithMipsAndStorageBinding)
    {
        ViewFormatSet.insert(Desc.Format);
        ViewFormatSet.insert(SRGBFormatToUnorm(Desc.Format));
    }

    for (const auto& TextureViewFmt : ViewFormatSet)
        TextureViewFormats.push_back(TextureFormatToWGPUFormat(TextureViewFmt));

    wgpuTextureDesc.viewFormats     = TextureViewFormats.data();
    wgpuTextureDesc.viewFormatCount = TextureViewFormats.size();
    wgpuTextureDesc.mipLevelCount   = Desc.MipLevels;
    wgpuTextureDesc.sampleCount     = Desc.SampleCount;
    wgpuTextureDesc.size.width      = Desc.GetWidth();
    wgpuTextureDesc.size.height     = Desc.GetHeight();
    wgpuTextureDesc.label           = Desc.Name;

    return wgpuTextureDesc;
}

WGPUTextureViewDescriptor TextureViewDescToWGPUTextureViewDescriptor(const TextureDesc&            TexDesc,
                                                                     TextureViewDesc&              ViewDesc,
                                                                     const RenderDeviceWebGPUImpl* pRenderDevice) noexcept
{
    auto IsTextureArray = [](const TextureViewDesc& ViewDesc) {
        // clang-format off
        return ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D_ARRAY ||
               ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D_ARRAY ||
               ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE     ||
               ViewDesc.TextureDim == RESOURCE_DIM_TEX_CUBE_ARRAY;
        // clang-format on
    };

    if (ViewDesc.Format == TEX_FORMAT_UNKNOWN)
        ViewDesc.Format = TexDesc.Format;

    WGPUTextureViewDescriptor wgpuTextureViewDesc{};
    wgpuTextureViewDesc.dimension     = ResourceDimensionToWGPUTextureViewDimension(ViewDesc.TextureDim);
    wgpuTextureViewDesc.baseMipLevel  = ViewDesc.MostDetailedMip;
    wgpuTextureViewDesc.mipLevelCount = ViewDesc.NumMipLevels;
    if (!(TexDesc.BindFlags & BIND_DEPTH_STENCIL))
        wgpuTextureViewDesc.format = TextureFormatToWGPUFormat(ViewDesc.Format);

    if (IsTextureArray(ViewDesc))
    {
        wgpuTextureViewDesc.baseArrayLayer  = ViewDesc.FirstArraySlice;
        wgpuTextureViewDesc.arrayLayerCount = ViewDesc.NumArraySlices;
    }
    else
    {
        wgpuTextureViewDesc.baseArrayLayer  = ViewDesc.FirstArraySlice;
        wgpuTextureViewDesc.arrayLayerCount = 1;
    }

    const TextureFormatAttribs& FmtAttribs = GetTextureFormatAttribs(ViewDesc.Format);
    if (ViewDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL || ViewDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL)
    {
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            wgpuTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
            wgpuTextureViewDesc.aspect = WGPUTextureAspect_All;
        else
            UNEXPECTED("Unexpected component type for a depth-stencil view format");
    }
    else
    {
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH)
            wgpuTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
        else if (FmtAttribs.ComponentType == COMPONENT_TYPE_DEPTH_STENCIL)
        {
            if (ViewDesc.Format == TEX_FORMAT_R32_FLOAT_X8X24_TYPELESS || ViewDesc.Format == TEX_FORMAT_R24_UNORM_X8_TYPELESS)
                wgpuTextureViewDesc.aspect = WGPUTextureAspect_DepthOnly;
            else if (ViewDesc.Format == TEX_FORMAT_X32_TYPELESS_G8X24_UINT || ViewDesc.Format == TEX_FORMAT_X24_TYPELESS_G8_UINT)
                wgpuTextureViewDesc.aspect = WGPUTextureAspect_StencilOnly;
            else
                UNEXPECTED("Unexpected depth-stencil texture format");
        }
        else
            wgpuTextureViewDesc.aspect = WGPUTextureAspect_All;
    }

    return wgpuTextureViewDesc;
}

} // namespace

Uint64 TextureWebGPUImpl::GetStagingLocationOffset(const TextureDesc& TexDesc,
                                                   Uint32             ArraySlice,
                                                   Uint32             MipLevel,
                                                   Uint32             LocationX,
                                                   Uint32             LocationY,
                                                   Uint32             LocationZ)
{
    VERIFY_EXPR(TexDesc.MipLevels > 0 && TexDesc.GetArraySize() > 0 && TexDesc.Width > 0 && TexDesc.Height > 0 && TexDesc.Format != TEX_FORMAT_UNKNOWN);
    VERIFY_EXPR(ArraySlice < TexDesc.GetArraySize() && MipLevel < TexDesc.MipLevels || ArraySlice == TexDesc.GetArraySize() && MipLevel == 0);

    const TextureFormatAttribs& FmtAttribs = GetTextureFormatAttribs(TexDesc.Format);

    Uint64 Offset = 0;
    if (ArraySlice > 0)
    {
        Uint64 ArraySliceSize = 0;
        for (Uint32 MipIdx = 0; MipIdx < TexDesc.MipLevels; ++MipIdx)
        {
            const MipLevelProperties MipInfo        = GetMipLevelProperties(TexDesc, MipIdx);
            const Uint64             DepthSliceSize = AlignUp(MipInfo.RowSize, ImageCopyBufferRowAlignment) * (MipInfo.StorageHeight / FmtAttribs.BlockHeight);
            ArraySliceSize += DepthSliceSize * MipInfo.Depth;
        }

        Offset = ArraySliceSize;
        if (TexDesc.IsArray())
            Offset *= ArraySlice;
    }

    for (Uint32 MipIdx = 0; MipIdx < MipLevel; ++MipIdx)
    {
        const MipLevelProperties MipInfo        = GetMipLevelProperties(TexDesc, MipIdx);
        const Uint64             DepthSliceSize = AlignUp(MipInfo.RowSize, ImageCopyBufferRowAlignment) * (MipInfo.StorageHeight / FmtAttribs.BlockHeight);
        Offset += DepthSliceSize * MipInfo.Depth;
    }

    if (ArraySlice == TexDesc.GetArraySize())
    {
        VERIFY(LocationX == 0 && LocationY == 0 && LocationZ == 0,
               "Staging buffer size is requested: location must be (0,0,0).");
    }
    else if (LocationX != 0 || LocationY != 0 || LocationZ != 0)
    {
        MipLevelProperties MipLevelAttribs = GetMipLevelProperties(TexDesc, MipLevel);

        VERIFY(LocationX < MipLevelAttribs.LogicalWidth && LocationY < MipLevelAttribs.LogicalHeight && LocationZ < MipLevelAttribs.Depth,
               "Specified location is out of bounds");
        if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
        {
            VERIFY((LocationX % FmtAttribs.BlockWidth) == 0 && (LocationY % FmtAttribs.BlockHeight) == 0,
                   "For compressed texture formats, location must be a multiple of compressed block size.");
        }

        Offset += (LocationZ * MipLevelAttribs.StorageHeight + LocationY) / FmtAttribs.BlockHeight * AlignUp(MipLevelAttribs.RowSize, ImageCopyBufferRowAlignment);
        Offset += Uint64{LocationX / FmtAttribs.BlockWidth} * FmtAttribs.GetElementSize();
    }

    return Offset;
}


TextureWebGPUImpl::TextureWebGPUImpl(IReferenceCounters*        pRefCounters,
                                     FixedBlockMemoryAllocator& TexViewObjAllocator,
                                     RenderDeviceWebGPUImpl*    pDevice,
                                     const TextureDesc&         Desc,
                                     const TextureData*         pInitData,
                                     bool                       bIsDeviceInternal) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pDevice,
        Desc,
        bIsDeviceInternal
    },
    WebGPUResourceBase{*this, Desc.Usage == USAGE_STAGING ? ((Desc.CPUAccessFlags & CPU_ACCESS_READ) ? MaxStagingReadBuffers : 1): 0}
// clang-format on
{
    if (m_Desc.Usage == USAGE_IMMUTABLE && (pInitData == nullptr || pInitData->pSubResources == nullptr))
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time: pInitData can't be null");

    if (m_Desc.Usage == USAGE_STAGING && (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == (CPU_ACCESS_READ | CPU_ACCESS_WRITE))
        LOG_ERROR_AND_THROW("Read-write staging textures are not supported in WebGPU");

    if (m_Desc.Is1D() && m_Desc.IsArray())
        LOG_ERROR_AND_THROW("1D texture arrays are not supported in WebGPU");

    if (m_Desc.Is1D() && (m_Desc.BindFlags & (BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS | BIND_DEPTH_STENCIL)))
        LOG_ERROR_AND_THROW("1D textures cannot have bind flags for render target, unordered access, or depth stencil in WebGPU");

    if (m_Desc.Is1D() && (m_Desc.SampleCount > 1))
        LOG_ERROR_AND_THROW("1D textures cannot be multisampled in WebGPU");

    const TextureFormatAttribs& FmtAttribs        = GetTextureFormatAttribs(m_Desc.Format);
    const bool                  InitializeTexture = (pInitData != nullptr && pInitData->pSubResources != nullptr && pInitData->NumSubresources > 0);

    if (m_Desc.Usage == USAGE_IMMUTABLE || m_Desc.Usage == USAGE_DEFAULT || m_Desc.Usage == USAGE_DYNAMIC)
    {
        std::vector<WGPUTextureFormat> TextureViewFormats;
        WGPUTextureDescriptor          wgpuTextureDesc = TextureDescToWGPUTextureDescriptor(m_Desc, pDevice, TextureViewFormats);
        m_wgpuTexture.Reset(wgpuDeviceCreateTexture(pDevice->GetWebGPUDevice(), &wgpuTextureDesc));
        if (!m_wgpuTexture)
            LOG_ERROR_AND_THROW("Failed to create WebGPU texture ", " '", m_Desc.Name ? m_Desc.Name : "", '\'');

        if (InitializeTexture)
        {
            WGPUBufferDescriptor wgpuBufferDesc{};
            wgpuBufferDesc.usage            = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
            wgpuBufferDesc.size             = AlignUp(GetStagingLocationOffset(m_Desc, m_Desc.GetArraySize(), 0), Uint64{4});
            wgpuBufferDesc.mappedAtCreation = true;

            WebGPUBufferWrapper wgpuUploadBuffer{wgpuDeviceCreateBuffer(pDevice->GetWebGPUDevice(), &wgpuBufferDesc)};
            if (!wgpuUploadBuffer)
                LOG_ERROR_AND_THROW("Failed to create WebGPU texture upload buffer");

            // Do NOT use WGPU_WHOLE_MAP_SIZE due to https://github.com/emscripten-core/emscripten/issues/20538
            uint8_t* pUploadData = static_cast<uint8_t*>(wgpuBufferGetMappedRange(wgpuUploadBuffer.Get(), 0, static_cast<size_t>(wgpuBufferDesc.size)));

            WGPUCommandEncoderDescriptor wgpuEncoderDesc{};
            WebGPUCommandEncoderWrapper  wgpuCmdEncoder{wgpuDeviceCreateCommandEncoder(pDevice->GetWebGPUDevice(), &wgpuEncoderDesc)};

            Uint32 CurrSubRes = 0;
            for (Uint32 LayerIdx = 0; LayerIdx < m_Desc.GetArraySize(); ++LayerIdx)
            {
                for (Uint32 MipIdx = 0; MipIdx < m_Desc.MipLevels; ++MipIdx)
                {
                    const MipLevelProperties MipProps   = GetMipLevelProperties(m_Desc, MipIdx);
                    const TextureSubResData& SubResData = pInitData->pSubResources[CurrSubRes++];

                    const Uint64 DstSubResOffset = GetStagingLocationOffset(m_Desc, LayerIdx, MipIdx);
                    const Uint64 DstRawStride    = AlignUp(MipProps.RowSize, ImageCopyBufferRowAlignment);
                    const Uint64 DstDepthStride  = DstRawStride * (MipProps.StorageHeight / FmtAttribs.BlockHeight);

                    CopyTextureSubresource(SubResData,
                                           MipProps.StorageHeight / FmtAttribs.BlockHeight, // NumRows
                                           MipProps.Depth,
                                           MipProps.RowSize,
                                           pUploadData + DstSubResOffset,
                                           DstRawStride,  // DstRowStride
                                           DstDepthStride // DstDepthStride
                    );

                    WGPUImageCopyBuffer wgpuSourceCopyInfo{};
                    wgpuSourceCopyInfo.layout.offset       = DstSubResOffset;
                    wgpuSourceCopyInfo.layout.bytesPerRow  = static_cast<Uint32>(DstRawStride);
                    wgpuSourceCopyInfo.layout.rowsPerImage = static_cast<Uint32>(DstDepthStride / DstRawStride);
                    wgpuSourceCopyInfo.buffer              = wgpuUploadBuffer.Get();

                    WGPUImageCopyTexture wgpuDestinationCopyInfo{};
                    wgpuDestinationCopyInfo.texture  = m_wgpuTexture.Get();
                    wgpuDestinationCopyInfo.mipLevel = MipIdx;
                    wgpuDestinationCopyInfo.origin   = {0, 0, LayerIdx};
                    wgpuDestinationCopyInfo.aspect   = WGPUTextureAspect_All;

                    WGPUExtent3D wgpuCopySize{};
                    wgpuCopySize.width              = MipProps.LogicalWidth;
                    wgpuCopySize.height             = MipProps.LogicalHeight;
                    wgpuCopySize.depthOrArrayLayers = MipProps.Depth;

                    if (FmtAttribs.ComponentType == COMPONENT_TYPE_COMPRESSED)
                    {
                        wgpuCopySize.width  = AlignUp(wgpuCopySize.width, FmtAttribs.BlockWidth);
                        wgpuCopySize.height = AlignUp(wgpuCopySize.height, FmtAttribs.BlockHeight);
                    }

                    wgpuCommandEncoderCopyBufferToTexture(wgpuCmdEncoder, &wgpuSourceCopyInfo, &wgpuDestinationCopyInfo, &wgpuCopySize);
                }
            }

            wgpuBufferUnmap(wgpuUploadBuffer.Get());

            VERIFY_EXPR(pDevice->GetNumImmediateContexts() == 1);
            WGPUCommandBufferDescriptor wgpuCmdBufferDesc{};
            WebGPUCommandBufferWrapper  wgpuCmdBuffer{wgpuCommandEncoderFinish(wgpuCmdEncoder, &wgpuCmdBufferDesc)};
            DeviceContextWebGPUImpl*    pContext = pDevice->GetImmediateContext(0);
            wgpuQueueSubmit(pContext->GetWebGPUQueue(), 1, &wgpuCmdBuffer.Get());
        }
    }
    else if (m_Desc.Usage == USAGE_STAGING)
    {
        m_MappedData.resize(static_cast<size_t>(GetStagingLocationOffset(m_Desc, m_Desc.GetArraySize(), 0)));

        if (InitializeTexture)
        {
            Uint32 CurrSubRes = 0;
            for (Uint32 LayerIdx = 0; LayerIdx < m_Desc.GetArraySize(); ++LayerIdx)
            {
                for (Uint32 MipIdx = 0; MipIdx < m_Desc.MipLevels; ++MipIdx)
                {
                    const MipLevelProperties MipProps        = GetMipLevelProperties(m_Desc, MipIdx);
                    const TextureSubResData& SubResData      = pInitData->pSubResources[CurrSubRes++];
                    const Uint64             DstSubResOffset = GetStagingLocationOffset(m_Desc, LayerIdx, MipIdx);

                    CopyTextureSubresource(SubResData,
                                           MipProps.StorageHeight / FmtAttribs.BlockHeight, // NumRows
                                           MipProps.Depth,
                                           MipProps.RowSize,
                                           &m_MappedData[static_cast<size_t>(DstSubResOffset)],
                                           MipProps.RowSize,       // DstRowStride
                                           MipProps.DepthSliceSize // DstDepthStride
                    );
                }
            }
        }
    }
    else
    {
        UNSUPPORTED("Unsupported usage ", GetUsageString(m_Desc.Usage));
    }

    SetState(RESOURCE_STATE_UNDEFINED);
}

TextureWebGPUImpl::TextureWebGPUImpl(IReferenceCounters*        pRefCounters,
                                     FixedBlockMemoryAllocator& TexViewObjAllocator,
                                     RenderDeviceWebGPUImpl*    pDevice,
                                     const TextureDesc&         Desc,
                                     RESOURCE_STATE             InitialState,
                                     WGPUTexture                wgpuTextureHandle,
                                     bool                       bIsDeviceInternal) :
    // clang-format off
    TTextureBase
    {
        pRefCounters,
        TexViewObjAllocator,
        pDevice,
        Desc,
        bIsDeviceInternal
    },
    WebGPUResourceBase{*this, 0},
    m_wgpuTexture{wgpuTextureHandle, {true}}
// clang-format on
{
    DEV_CHECK_ERR(Desc.Usage != USAGE_STAGING, "Staging texture is not expected");

    SetState(InitialState);
}

Uint64 TextureWebGPUImpl::GetNativeHandle()
{
    return reinterpret_cast<Uint64>(GetWebGPUTexture());
}

WGPUTexture TextureWebGPUImpl::GetWebGPUTexture() const
{
    return m_wgpuTexture.Get();
}

TextureWebGPUImpl::StagingBufferInfo* TextureWebGPUImpl::GetStagingBuffer()
{
    VERIFY(m_Desc.Usage == USAGE_STAGING, "USAGE_STAGING texture is expected");
    return WebGPUResourceBase::GetStagingBuffer(m_pDevice->GetWebGPUDevice(), m_Desc.CPUAccessFlags);
}

void* TextureWebGPUImpl::Map(MAP_TYPE MapType, Uint64 Offset, Uint64 Size)
{
    VERIFY(m_Desc.Usage == USAGE_STAGING, "Map is only allowed for USAGE_STAGING textures");
    VERIFY(Offset + Size <= m_MappedData.size(), "Offset (", Offset, ") + size (", Size, ") exceeds the mapped data size (", m_MappedData.size(), ")");
    return WebGPUResourceBase::Map(MapType, Offset);
}

void TextureWebGPUImpl::Unmap()
{
    VERIFY(m_Desc.Usage == USAGE_STAGING, "Unmap is only allowed USAGE_STAGING textures");
    WebGPUResourceBase::Unmap();
}

void TextureWebGPUImpl::CreateViewInternal(const TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "View pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto& TexViewAllocator = m_pDevice->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        TextureViewDesc UpdatedViewDesc = ViewDesc;
        ValidatedAndCorrectTextureViewDesc(m_Desc, UpdatedViewDesc);
        WGPUTextureViewDescriptor wgpuTextureViewDesc = TextureViewDescToWGPUTextureViewDescriptor(m_Desc, UpdatedViewDesc, m_pDevice);

        WebGPUTextureViewWrapper wgpuTextureView{wgpuTextureCreateView(m_wgpuTexture.Get(), &wgpuTextureViewDesc)};
        if (!wgpuTextureView)
            LOG_ERROR_AND_THROW("Failed to create WebGPU texture view ", " '", ViewDesc.Name ? ViewDesc.Name : "", '\'');

        std::vector<WebGPUTextureViewWrapper> wgpuTextureMipSRVs;
        std::vector<WebGPUTextureViewWrapper> wgpuTextureMipUAVs;
        if (UpdatedViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION)
        {
            const auto& FmtInfo = m_pDevice->GetTextureFormatInfoExt(SRGBFormatToUnorm(UpdatedViewDesc.Format));
            VERIFY_EXPR((m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0 && m_Desc.Is2D());

            if (FmtInfo.BindFlags & BIND_UNORDERED_ACCESS)
            {
                for (Uint32 MipLevel = 0; MipLevel < UpdatedViewDesc.NumMipLevels; ++MipLevel)
                {
                    TextureViewDesc TexMipSRVDesc = UpdatedViewDesc;
                    TexMipSRVDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                    TexMipSRVDesc.ViewType        = TEXTURE_VIEW_SHADER_RESOURCE;
                    TexMipSRVDesc.MostDetailedMip = UpdatedViewDesc.MostDetailedMip + MipLevel;
                    TexMipSRVDesc.NumMipLevels    = 1;

                    WGPUTextureViewDescriptor wgpuTextureViewDescSRV = TextureViewDescToWGPUTextureViewDescriptor(m_Desc, TexMipSRVDesc, m_pDevice);
                    wgpuTextureMipSRVs.emplace_back(wgpuTextureCreateView(m_wgpuTexture.Get(), &wgpuTextureViewDescSRV));

                    if (!wgpuTextureMipSRVs.back())
                        LOG_ERROR_AND_THROW("Failed to create WebGPU texture view ", " '", UpdatedViewDesc.Name ? UpdatedViewDesc.Name : "", '\'');
                }

                for (Uint32 MipLevel = 0; MipLevel < UpdatedViewDesc.NumMipLevels; ++MipLevel)
                {
                    TextureViewDesc TexMipUAVDesc = UpdatedViewDesc;
                    TexMipUAVDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                    TexMipUAVDesc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
                    TexMipUAVDesc.MostDetailedMip = UpdatedViewDesc.MostDetailedMip + MipLevel;
                    TexMipUAVDesc.NumMipLevels    = 1;
                    TexMipUAVDesc.Format          = SRGBFormatToUnorm(TexMipUAVDesc.Format);

                    WGPUTextureViewDescriptor wgpuTextureViewDescUAV = TextureViewDescToWGPUTextureViewDescriptor(m_Desc, TexMipUAVDesc, m_pDevice);
                    wgpuTextureMipUAVs.emplace_back(wgpuTextureCreateView(m_wgpuTexture.Get(), &wgpuTextureViewDescUAV));

                    if (!wgpuTextureMipUAVs.back())
                        LOG_ERROR_AND_THROW("Failed to create WebGPU texture view ", " '", UpdatedViewDesc.Name ? UpdatedViewDesc.Name : "", '\'');
                }
            }
            else
            {
                for (Uint32 Slice = 0; Slice < UpdatedViewDesc.NumArraySlices; ++Slice)
                {
                    for (Uint32 MipLevel = 0; MipLevel < UpdatedViewDesc.NumMipLevels; ++MipLevel)
                    {
                        TextureViewDesc TexMipSRVDesc = UpdatedViewDesc;
                        TexMipSRVDesc.TextureDim      = RESOURCE_DIM_TEX_2D;
                        TexMipSRVDesc.ViewType        = TEXTURE_VIEW_SHADER_RESOURCE;
                        TexMipSRVDesc.MostDetailedMip = UpdatedViewDesc.MostDetailedMip + MipLevel;
                        TexMipSRVDesc.NumMipLevels    = 1;
                        TexMipSRVDesc.FirstArraySlice = UpdatedViewDesc.FirstArraySlice + Slice;

                        WGPUTextureViewDescriptor wgpuTextureViewDescSRV = TextureViewDescToWGPUTextureViewDescriptor(m_Desc, TexMipSRVDesc, m_pDevice);
                        wgpuTextureMipSRVs.emplace_back(wgpuTextureCreateView(m_wgpuTexture.Get(), &wgpuTextureViewDescSRV));

                        if (!wgpuTextureMipSRVs.back())
                            LOG_ERROR_AND_THROW("Failed to create WebGPU texture view ", " '", UpdatedViewDesc.Name ? UpdatedViewDesc.Name : "", '\'');
                    }

                    for (Uint32 MipLevel = 0; MipLevel < UpdatedViewDesc.NumMipLevels; ++MipLevel)
                    {
                        TextureViewDesc TexMipRTVDesc = UpdatedViewDesc;
                        TexMipRTVDesc.TextureDim      = RESOURCE_DIM_TEX_2D;
                        TexMipRTVDesc.ViewType        = TEXTURE_VIEW_RENDER_TARGET;
                        TexMipRTVDesc.MostDetailedMip = UpdatedViewDesc.MostDetailedMip + MipLevel;
                        TexMipRTVDesc.NumMipLevels    = 1;
                        TexMipRTVDesc.FirstArraySlice = UpdatedViewDesc.FirstArraySlice + Slice;

                        WGPUTextureViewDescriptor wgpuTextureViewDescRTV = TextureViewDescToWGPUTextureViewDescriptor(m_Desc, TexMipRTVDesc, m_pDevice);
                        wgpuTextureMipUAVs.emplace_back(wgpuTextureCreateView(m_wgpuTexture.Get(), &wgpuTextureViewDescRTV));

                        if (!wgpuTextureMipUAVs.back())
                            LOG_ERROR_AND_THROW("Failed to create WebGPU texture view ", " '", UpdatedViewDesc.Name ? UpdatedViewDesc.Name : "", '\'');
                    }
                }
            }
        }

        TextureViewWebGPUImpl* pViewWebGPU = NEW_RC_OBJ(TexViewAllocator, "TextureViewWebGPUImpl instance", TextureViewWebGPUImpl, bIsDefaultView ? this : nullptr)(
            GetDevice(), UpdatedViewDesc, this, std::move(wgpuTextureView), std::move(wgpuTextureMipSRVs), std::move(wgpuTextureMipUAVs), bIsDefaultView, m_bIsDeviceInternal);
        VERIFY(pViewWebGPU->GetDesc().ViewType == ViewDesc.ViewType, "Incorrect view type");

        if (bIsDefaultView)
            *ppView = pViewWebGPU;
        else
            pViewWebGPU->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));
    }
    catch (const std::runtime_error&)
    {
        const char* ViewTypeName = GetTexViewTypeLiteralName(ViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", ViewDesc.Name ? ViewDesc.Name : "", "\" (", ViewTypeName, ") for texture \"", m_Desc.Name ? m_Desc.Name : "", "\"");
    }
}

} // namespace Diligent
