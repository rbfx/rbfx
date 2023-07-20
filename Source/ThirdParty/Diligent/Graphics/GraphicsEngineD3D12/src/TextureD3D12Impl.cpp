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

#include "TextureD3D12Impl.hpp"

#include "RenderDeviceD3D12Impl.hpp"
#include "DeviceContextD3D12Impl.hpp"
#include "TextureViewD3D12Impl.hpp"

#include "D3D12TypeConversions.hpp"
#include "DXGITypeConversions.hpp"
#include "d3dx12_win.h"
#include "EngineMemory.h"
#include "StringTools.hpp"

namespace Diligent
{

static DXGI_FORMAT GetClearFormat(DXGI_FORMAT Fmt, D3D12_RESOURCE_FLAGS Flags)
{
    if (Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
        // clang-format off
        switch (Fmt)
        {
            case DXGI_FORMAT_R32_TYPELESS:      return DXGI_FORMAT_D32_FLOAT;
            case DXGI_FORMAT_R16_TYPELESS:      return DXGI_FORMAT_D16_UNORM;
            case DXGI_FORMAT_R24G8_TYPELESS:    return DXGI_FORMAT_D24_UNORM_S8_UINT;
            case DXGI_FORMAT_R32G8X24_TYPELESS: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        }
        // clang-format on
    }
    else if (Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
        // clang-format off
        switch (Fmt)
        {
            case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
            case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
            case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
            case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;
            case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
            case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_FLOAT;
            case DXGI_FORMAT_R32_TYPELESS:          return DXGI_FORMAT_R32_FLOAT;
            case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;
            case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_FLOAT;
            case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return DXGI_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_B8G8R8X8_TYPELESS:     return DXGI_FORMAT_B8G8R8X8_UNORM;
        }
        // clang-format on
    }
    return Fmt;
}

D3D12_RESOURCE_DESC TextureD3D12Impl::GetD3D12TextureDesc() const
{
    D3D12_RESOURCE_DESC Desc = {};

    Desc.Alignment = 0;
    if (m_Desc.IsArray())
        Desc.DepthOrArraySize = StaticCast<UINT16>(m_Desc.ArraySize);
    else if (m_Desc.Is3D())
        Desc.DepthOrArraySize = StaticCast<UINT16>(m_Desc.Depth);
    else
        Desc.DepthOrArraySize = 1;

    if (m_Desc.Is1D())
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    else if (m_Desc.Is2D())
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    else if (m_Desc.Is3D())
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    else
    {
        LOG_ERROR_AND_THROW("Unknown texture type");
    }

    Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (m_Desc.BindFlags & BIND_RENDER_TARGET)
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    if (m_Desc.BindFlags & BIND_DEPTH_STENCIL)
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    if ((m_Desc.BindFlags & BIND_UNORDERED_ACCESS) || (m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS))
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    if ((m_Desc.BindFlags & (BIND_SHADER_RESOURCE | BIND_INPUT_ATTACHMENT)) == 0 && (m_Desc.BindFlags & BIND_DEPTH_STENCIL) != 0)
        Desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

    auto Format = TexFormatToDXGI_Format(m_Desc.Format, m_Desc.BindFlags);
    if (Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB && (Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        Desc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    else
        Desc.Format = Format;

    Desc.Height             = UINT{m_Desc.Height};
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.MipLevels          = StaticCast<UINT16>(m_Desc.MipLevels);
    Desc.SampleDesc.Count   = m_Desc.SampleCount;
    Desc.SampleDesc.Quality = 0;
    Desc.Width              = UINT64{m_Desc.Width};

    return Desc;
}

TextureD3D12Impl::TextureD3D12Impl(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& TexViewObjAllocator,
                                   RenderDeviceD3D12Impl*     pRenderDeviceD3D12,
                                   const TextureDesc&         TexDesc,
                                   const TextureData*         pInitData /*= nullptr*/) :
    TTextureBase{pRefCounters, TexViewObjAllocator, pRenderDeviceD3D12, TexDesc}
{
    if (m_Desc.Usage == USAGE_IMMUTABLE && (pInitData == nullptr || pInitData->pSubResources == nullptr))
        LOG_ERROR_AND_THROW("Immutable textures must be initialized with data at creation time: pInitData can't be null");

    if ((m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0)
    {
        if (!m_Desc.Is2D())
        {
            LOG_ERROR_AND_THROW("Mipmap generation is currently only supported for 2D and cube textures/texture arrays in d3d12 backend");
        }
    }

    D3D12_RESOURCE_DESC d3d12TexDesc       = GetD3D12TextureDesc();
    const bool          bInitializeTexture = (pInitData != nullptr && pInitData->pSubResources != nullptr && pInitData->NumSubresources > 0);

    const auto CmdQueueInd = pInitData != nullptr && pInitData->pContext != nullptr ?
        ClassPtrCast<DeviceContextD3D12Impl>(pInitData->pContext)->GetCommandQueueId() :
        SoftwareQueueIndex{PlatformMisc::GetLSB(m_Desc.ImmediateContextMask)};

    const auto d3d12StateMask = bInitializeTexture ?
        GetSupportedD3D12ResourceStatesForCommandList(pRenderDeviceD3D12->GetCommandQueueType(CmdQueueInd)) :
        static_cast<D3D12_RESOURCE_STATES>(~0u);

    D3D12_CLEAR_VALUE  ClearValue  = {};
    D3D12_CLEAR_VALUE* pClearValue = nullptr;
    if (d3d12TexDesc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
    {
        if (m_Desc.ClearValue.Format != TEX_FORMAT_UNKNOWN)
            ClearValue.Format = TexFormatToDXGI_Format(m_Desc.ClearValue.Format);
        else
        {
            auto Format       = TexFormatToDXGI_Format(m_Desc.Format, m_Desc.BindFlags);
            ClearValue.Format = GetClearFormat(Format, d3d12TexDesc.Flags);
        }

        if (d3d12TexDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
        {
            for (int i = 0; i < 4; ++i)
                ClearValue.Color[i] = m_Desc.ClearValue.Color[i];
        }
        else if (d3d12TexDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
        {
            ClearValue.DepthStencil.Depth   = m_Desc.ClearValue.DepthStencil.Depth;
            ClearValue.DepthStencil.Stencil = m_Desc.ClearValue.DepthStencil.Stencil;
        }
        pClearValue = &ClearValue;
    }

    auto* pd3d12Device = pRenderDeviceD3D12->GetD3D12Device();
    if (m_Desc.Usage == USAGE_SPARSE)
    {
        d3d12TexDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;

#ifdef DILIGENT_ENABLE_D3D_NVAPI
        if (IsUsingNVApi())
        {
            auto err = NvAPI_D3D12_CreateReservedResource(pd3d12Device, &d3d12TexDesc, D3D12_RESOURCE_STATE_COMMON, pClearValue, __uuidof(m_pd3d12Resource),
                                                          reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)),
                                                          true, m_pDevice->GetDummyNVApiHeap());
            if (err != NVAPI_OK)
                LOG_ERROR_AND_THROW("Failed to create D3D12 texture using NVApi");
        }
        else
#endif
        {
            auto hr = pd3d12Device->CreateReservedResource(&d3d12TexDesc, D3D12_RESOURCE_STATE_COMMON, pClearValue, __uuidof(m_pd3d12Resource),
                                                           reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create D3D12 texture");
        }

        if (*m_Desc.Name != 0)
            m_pd3d12Resource->SetName(WidenString(m_Desc.Name).c_str());

        SetState(RESOURCE_STATE_UNDEFINED);

        InitSparseProperties();
    }
    else if (m_Desc.Usage == USAGE_IMMUTABLE || m_Desc.Usage == USAGE_DEFAULT || m_Desc.Usage == USAGE_DYNAMIC)
    {
        VERIFY(m_Desc.Usage != USAGE_DYNAMIC || PlatformMisc::CountOneBits(m_Desc.ImmediateContextMask) <= 1,
               "ImmediateContextMask must contain single set bit, this error should've been handled in ValidateTextureDesc()");

        D3D12_HEAP_PROPERTIES HeapProps{};
        HeapProps.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        HeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProps.CreationNodeMask     = 1;
        HeapProps.VisibleNodeMask      = 1;

        auto InitialState = bInitializeTexture ? RESOURCE_STATE_COPY_DEST : RESOURCE_STATE_UNDEFINED;
        SetState(InitialState);

        const auto d3d12State = ResourceStateFlagsToD3D12ResourceStates(InitialState) & d3d12StateMask;

        // By default, committed resources and heaps are almost always zeroed upon creation.
        // CREATE_NOT_ZEROED flag allows this to be elided in some scenarios to lower the overhead
        // of creating the heap. No need to zero the resource if we initialize it.
        const auto d3d12HeapFlags = bInitializeTexture ?
            D3D12_HEAP_FLAG_CREATE_NOT_ZEROED :
            D3D12_HEAP_FLAG_NONE;

        auto hr = pd3d12Device->CreateCommittedResource(
            &HeapProps, d3d12HeapFlags, &d3d12TexDesc, d3d12State, pClearValue, __uuidof(m_pd3d12Resource),
            reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create D3D12 texture");

        if (*m_Desc.Name != 0)
            m_pd3d12Resource->SetName(WidenString(m_Desc.Name).c_str());

        if (bInitializeTexture)
        {
            Uint32 ExpectedNumSubresources = Uint32{d3d12TexDesc.MipLevels} * (d3d12TexDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : Uint32{d3d12TexDesc.DepthOrArraySize});
            if (pInitData->NumSubresources != ExpectedNumSubresources)
                LOG_ERROR_AND_THROW("Incorrect number of subresources in init data. ", ExpectedNumSubresources, " expected, while ", pInitData->NumSubresources, " provided");

            UINT64 uploadBufferSize = 0;
            pd3d12Device->GetCopyableFootprints(&d3d12TexDesc, 0, pInitData->NumSubresources, 0, nullptr, nullptr, nullptr, &uploadBufferSize);

            D3D12_HEAP_PROPERTIES UploadHeapProps{};
            UploadHeapProps.Type                 = D3D12_HEAP_TYPE_UPLOAD;
            UploadHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            UploadHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            UploadHeapProps.CreationNodeMask     = 1;
            UploadHeapProps.VisibleNodeMask      = 1;

            D3D12_RESOURCE_DESC UploadBuffDesc{};
            UploadBuffDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
            UploadBuffDesc.Alignment          = 0;
            UploadBuffDesc.Width              = uploadBufferSize;
            UploadBuffDesc.Height             = 1;
            UploadBuffDesc.DepthOrArraySize   = 1;
            UploadBuffDesc.MipLevels          = 1;
            UploadBuffDesc.Format             = DXGI_FORMAT_UNKNOWN;
            UploadBuffDesc.SampleDesc.Count   = 1;
            UploadBuffDesc.SampleDesc.Quality = 0;
            UploadBuffDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            UploadBuffDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

            CComPtr<ID3D12Resource> UploadBuffer;
            hr = pd3d12Device->CreateCommittedResource(
                &UploadHeapProps,
                D3D12_HEAP_FLAG_CREATE_NOT_ZEROED, // Do not zero the heap
                &UploadBuffDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, __uuidof(UploadBuffer),
                reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&UploadBuffer)));
            if (FAILED(hr))
                LOG_ERROR_AND_THROW("Failed to create committed resource in an upload heap");

            const auto UploadBufferName = std::wstring{L"Upload buffer for texture '"} + WidenString(m_Desc.Name) + L'\'';
            UploadBuffer->SetName(UploadBufferName.c_str());

            auto InitContext = pRenderDeviceD3D12->AllocateCommandContext(CmdQueueInd);
            // copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
            VERIFY_EXPR(CheckState(RESOURCE_STATE_COPY_DEST));
            std::vector<D3D12_SUBRESOURCE_DATA, STDAllocatorRawMem<D3D12_SUBRESOURCE_DATA>> D3D12SubResData(pInitData->NumSubresources, D3D12_SUBRESOURCE_DATA(), STD_ALLOCATOR_RAW_MEM(D3D12_SUBRESOURCE_DATA, GetRawAllocator(), "Allocator for vector<D3D12_SUBRESOURCE_DATA>"));
            for (size_t subres = 0; subres < D3D12SubResData.size(); ++subres)
            {
                D3D12SubResData[subres].pData      = pInitData->pSubResources[subres].pData;
                D3D12SubResData[subres].RowPitch   = static_cast<LONG_PTR>(pInitData->pSubResources[subres].Stride);
                D3D12SubResData[subres].SlicePitch = static_cast<LONG_PTR>(pInitData->pSubResources[subres].DepthStride);
            }
            auto UploadedSize = UpdateSubresources(InitContext->GetCommandList(), m_pd3d12Resource, UploadBuffer, 0, 0, pInitData->NumSubresources, D3D12SubResData.data());
            VERIFY(UploadedSize == uploadBufferSize, "Incorrect uploaded data size (", UploadedSize, "). ", uploadBufferSize, " is expected");

            // Command list fence should only be signaled when submitting cmd list
            // from the immediate context, otherwise the basic requirement will be violated
            // as in the scenario below
            // See http://diligentgraphics.com/diligent-engine/architecture/d3d12/managing-resource-lifetimes/
            //
            //  Signaled Fence  |        Immediate Context               |            InitContext            |
            //                  |                                        |                                   |
            //    N             |  Draw(ResourceX)                       |                                   |
            //                  |  Release(ResourceX)                    |                                   |
            //                  |   - (ResourceX, N) -> Release Queue    |                                   |
            //                  |                                        | CopyResource()                    |
            //   N+1            |                                        | CloseAndExecuteCommandContext()   |
            //                  |                                        |                                   |
            //   N+2            |  CloseAndExecuteCommandContext()       |                                   |
            //                  |   - Cmd list is submitted with number  |                                   |
            //                  |     N+1, but resource it references    |                                   |
            //                  |     was added to the delete queue      |                                   |
            //                  |     with value N                       |                                   |
            pRenderDeviceD3D12->CloseAndExecuteTransientCommandContext(CmdQueueInd, std::move(InitContext));

            // We MUST NOT call TransitionResource() from here, because
            // it will call AddRef() and potentially Release(), while
            // the object is not constructed yet
            // Add reference to the object to the release queue to keep it alive
            // until copy operation is complete.  This must be done after
            // submitting command list for execution!
            pRenderDeviceD3D12->SafeReleaseDeviceObject(std::move(UploadBuffer), Uint64{1} << CmdQueueInd);
        }
    }
    else if (m_Desc.Usage == USAGE_STAGING)
    {
        // Create staging buffer
        D3D12_HEAP_PROPERTIES StaginHeapProps{};
        DEV_CHECK_ERR((m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_READ ||
                          (m_Desc.CPUAccessFlags & (CPU_ACCESS_READ | CPU_ACCESS_WRITE)) == CPU_ACCESS_WRITE,
                      "Exactly one of CPU_ACCESS_READ or CPU_ACCESS_WRITE flags must be specified");

        RESOURCE_STATE InitialState = RESOURCE_STATE_UNKNOWN;
        if (m_Desc.CPUAccessFlags & CPU_ACCESS_READ)
        {
            DEV_CHECK_ERR(!bInitializeTexture, "Readback textures should not be initialized with data");
            StaginHeapProps.Type = D3D12_HEAP_TYPE_READBACK;
            InitialState         = RESOURCE_STATE_COPY_DEST;
        }
        else if (m_Desc.CPUAccessFlags & CPU_ACCESS_WRITE)
        {
            StaginHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            InitialState         = RESOURCE_STATE_GENERIC_READ;
        }
        else
            UNEXPECTED("Unexpected CPU access");

        SetState(InitialState);
        auto d3d12State = ResourceStateFlagsToD3D12ResourceStates(InitialState) & d3d12StateMask;

        StaginHeapProps.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        StaginHeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        StaginHeapProps.CreationNodeMask     = 1;
        StaginHeapProps.VisibleNodeMask      = 1;

        UINT64 stagingBufferSize = 0;
        Uint32 NumSubresources   = Uint32{d3d12TexDesc.MipLevels} * (d3d12TexDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : Uint32{d3d12TexDesc.DepthOrArraySize});
        m_StagingFootprints      = ALLOCATE(GetRawAllocator(), "Memory for staging footprints", D3D12_PLACED_SUBRESOURCE_FOOTPRINT, size_t{NumSubresources} + 1);
        pd3d12Device->GetCopyableFootprints(&d3d12TexDesc, 0, NumSubresources, 0, m_StagingFootprints, nullptr, nullptr, &stagingBufferSize);
        m_StagingFootprints[NumSubresources] = D3D12_PLACED_SUBRESOURCE_FOOTPRINT{stagingBufferSize};

        D3D12_RESOURCE_DESC BufferDesc{};
        BufferDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
        BufferDesc.Alignment          = 0;
        BufferDesc.Width              = stagingBufferSize;
        BufferDesc.Height             = 1;
        BufferDesc.DepthOrArraySize   = 1;
        BufferDesc.MipLevels          = 1;
        BufferDesc.Format             = DXGI_FORMAT_UNKNOWN;
        BufferDesc.SampleDesc.Count   = 1;
        BufferDesc.SampleDesc.Quality = 0;
        BufferDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        BufferDesc.Flags              = D3D12_RESOURCE_FLAG_NONE;

        // Resources on D3D12_HEAP_TYPE_READBACK heaps do not support persistent map. Map() and Unmap() must be
        // called between CPU and GPU accesses to the same memory address on some system architectures, when the
        // page caching behavior is write-back. Map() and Unmap() invalidate and flush the last level CPU cache
        // on some ARM systems, to marshal data between the CPU and GPU through memory addresses with write-back behavior.
        // https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map
        auto hr = pd3d12Device->CreateCommittedResource(&StaginHeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc, d3d12State,
                                                        nullptr, __uuidof(m_pd3d12Resource),
                                                        reinterpret_cast<void**>(static_cast<ID3D12Resource**>(&m_pd3d12Resource)));
        if (FAILED(hr))
            LOG_ERROR_AND_THROW("Failed to create staging buffer");

        if (bInitializeTexture)
        {
            const auto FmtAttribs = GetTextureFormatAttribs(m_Desc.Format);

            void* pStagingData = nullptr;
            m_pd3d12Resource->Map(0, nullptr, &pStagingData);
            DEV_CHECK_ERR(pStagingData != nullptr, "Failed to map staging buffer");
            if (pStagingData != nullptr)
            {
                for (Uint32 Subres = 0; Subres < NumSubresources; ++Subres)
                {
                    const auto Mip      = Subres % m_Desc.MipLevels;
                    const auto MipProps = GetMipLevelProperties(m_Desc, Mip);

                    const auto& SrcSubresData = pInitData->pSubResources[Subres];
                    const auto& DstFootprint  = GetStagingFootprint(Subres);

                    VERIFY_EXPR(MipProps.StorageWidth == DstFootprint.Footprint.Width);
                    VERIFY_EXPR(MipProps.StorageHeight == DstFootprint.Footprint.Height);
                    VERIFY_EXPR(MipProps.Depth == DstFootprint.Footprint.Depth);

                    CopyTextureSubresource(SrcSubresData,
                                           MipProps.StorageHeight / FmtAttribs.BlockHeight, // NumRows
                                           MipProps.Depth,
                                           MipProps.RowSize,
                                           reinterpret_cast<Uint8*>(pStagingData) + DstFootprint.Offset,
                                           DstFootprint.Footprint.RowPitch,
                                           DstFootprint.Footprint.RowPitch * DstFootprint.Footprint.Height / FmtAttribs.BlockHeight // DstDepthStride
                    );
                }
            }
            D3D12_RANGE FlushRange{0, StaticCast<SIZE_T>(stagingBufferSize)};
            m_pd3d12Resource->Unmap(0, &FlushRange);
        }
    }
    else
    {
        UNEXPECTED("Unexpected usage");
    }
}


static TextureDesc InitTexDescFromD3D12Resource(ID3D12Resource* pTexture, const TextureDesc& SrcTexDesc)
{
    const auto d3d12Desc = pTexture->GetDesc();

    TextureDesc TexDesc = SrcTexDesc;
    if (TexDesc.Format == TEX_FORMAT_UNKNOWN)
        TexDesc.Format = DXGI_FormatToTexFormat(d3d12Desc.Format);
    else
    {
        auto RefFormat = DXGI_FormatToTexFormat(d3d12Desc.Format);
        DEV_CHECK_ERR(RefFormat == TexDesc.Format, "The format specified by texture description (", GetTextureFormatAttribs(TexDesc.Format).Name,
                      ") does not match the D3D12 resource format (",
                      GetTextureFormatAttribs(RefFormat).Name, ")");
        (void)RefFormat;
    }

    TexDesc.Width     = StaticCast<Uint32>(d3d12Desc.Width);
    TexDesc.Height    = Uint32{d3d12Desc.Height};
    TexDesc.ArraySize = Uint32{d3d12Desc.DepthOrArraySize};
    TexDesc.MipLevels = Uint32{d3d12Desc.MipLevels};
    switch (d3d12Desc.Dimension)
    {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D: TexDesc.Type = TexDesc.ArraySize == 1 ? RESOURCE_DIM_TEX_1D : RESOURCE_DIM_TEX_1D_ARRAY; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D: TexDesc.Type = TexDesc.ArraySize == 1 ? RESOURCE_DIM_TEX_2D : RESOURCE_DIM_TEX_2D_ARRAY; break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D: TexDesc.Type = RESOURCE_DIM_TEX_3D; break;
    }

    TexDesc.SampleCount = d3d12Desc.SampleDesc.Count;

    TexDesc.Usage     = USAGE_DEFAULT;
    TexDesc.BindFlags = BIND_NONE;
    if ((d3d12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0)
        TexDesc.BindFlags |= BIND_RENDER_TARGET;
    if ((d3d12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0)
        TexDesc.BindFlags |= BIND_DEPTH_STENCIL;
    if ((d3d12Desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) != 0)
        TexDesc.BindFlags |= BIND_UNORDERED_ACCESS;
    if ((d3d12Desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == 0)
    {
        const auto& FormatAttribs = GetTextureFormatAttribs(TexDesc.Format);
        if (FormatAttribs.IsTypeless ||
            (FormatAttribs.ComponentType != COMPONENT_TYPE_DEPTH &&
             FormatAttribs.ComponentType != COMPONENT_TYPE_DEPTH_STENCIL))
        {
            TexDesc.BindFlags |= BIND_SHADER_RESOURCE;
        }
        if ((d3d12Desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) != 0)
            TexDesc.BindFlags |= BIND_INPUT_ATTACHMENT;
    }

    if (d3d12Desc.Layout == D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE)
    {
        TexDesc.Usage = USAGE_SPARSE;
        TexDesc.MiscFlags |= MISC_TEXTURE_FLAG_SPARSE_ALIASING;
    }

    return TexDesc;
}

TextureD3D12Impl::TextureD3D12Impl(IReferenceCounters*        pRefCounters,
                                   FixedBlockMemoryAllocator& TexViewObjAllocator,
                                   RenderDeviceD3D12Impl*     pDeviceD3D12,
                                   const TextureDesc&         TexDesc,
                                   RESOURCE_STATE             InitialState,
                                   ID3D12Resource*            pTexture) :
    TTextureBase{pRefCounters, TexViewObjAllocator, pDeviceD3D12, InitTexDescFromD3D12Resource(pTexture, TexDesc)}
{
    m_pd3d12Resource = pTexture;
    SetState(InitialState);

    if (m_Desc.Usage == USAGE_SPARSE)
        InitSparseProperties();
}

void TextureD3D12Impl::CreateViewInternal(const TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "View pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto* pDeviceD3D12Impl = GetDevice();
        auto& TexViewAllocator = pDeviceD3D12Impl->GetTexViewObjAllocator();
        VERIFY(&TexViewAllocator == &m_dbgTexViewObjAllocator, "Texture view allocator does not match allocator provided during texture initialization");

        auto UpdatedViewDesc = ViewDesc;
        ValidatedAndCorrectTextureViewDesc(m_Desc, UpdatedViewDesc);

        if (m_Desc.IsArray() && (ViewDesc.TextureDim == RESOURCE_DIM_TEX_1D || ViewDesc.TextureDim == RESOURCE_DIM_TEX_2D))
        {
            if (ViewDesc.FirstArraySlice != 0)
                LOG_ERROR_AND_THROW("FirstArraySlice must be 0, offset is not supported for non-array view in Direct3D12");
        }

        DescriptorHeapAllocation ViewDescriptor;
        switch (ViewDesc.ViewType)
        {
            case TEXTURE_VIEW_SHADER_RESOURCE:
            {
                VERIFY(m_Desc.BindFlags & BIND_SHADER_RESOURCE, "BIND_SHADER_RESOURCE flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                CreateSRV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_RENDER_TARGET:
            {
                VERIFY(m_Desc.BindFlags & BIND_RENDER_TARGET, "BIND_RENDER_TARGET flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
                CreateRTV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_DEPTH_STENCIL:
            case TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL:
            {
                VERIFY(m_Desc.BindFlags & BIND_DEPTH_STENCIL, "BIND_DEPTH_STENCIL flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
                CreateDSV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_UNORDERED_ACCESS:
            {
                VERIFY(m_Desc.BindFlags & BIND_UNORDERED_ACCESS, "BIND_UNORDERED_ACCESS flag is not set");
                ViewDescriptor = pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                CreateUAV(UpdatedViewDesc, ViewDescriptor.GetCpuHandle());
            }
            break;

            case TEXTURE_VIEW_SHADING_RATE:
            {
                // In Direct3D12 there is no special shading rate view, so use SRV instead because it is enabled by default
                VERIFY(m_Desc.BindFlags & BIND_SHADING_RATE, "BIND_SHADING_RATE flag is not set");
                // Descriptor handle is not needed
            }
            break;

            default: UNEXPECTED("Unknown view type"); break;
        }

        DescriptorHeapAllocation TexArraySRVDescriptor, MipUAVDescriptors;
        if (UpdatedViewDesc.Flags & TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION)
        {
            VERIFY_EXPR((m_Desc.MiscFlags & MISC_TEXTURE_FLAG_GENERATE_MIPS) != 0 && m_Desc.Is2D());

            {
                TexArraySRVDescriptor           = pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
                TextureViewDesc TexArraySRVDesc = UpdatedViewDesc;
                // Create texture array SRV
                TexArraySRVDesc.TextureDim = RESOURCE_DIM_TEX_2D_ARRAY;
                TexArraySRVDesc.ViewType   = TEXTURE_VIEW_SHADER_RESOURCE;
                CreateSRV(TexArraySRVDesc, TexArraySRVDescriptor.GetCpuHandle());
            }

            MipUAVDescriptors = pDeviceD3D12Impl->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_Desc.MipLevels);
            for (Uint32 MipLevel = 0; MipLevel < m_Desc.MipLevels; ++MipLevel)
            {
                TextureViewDesc UAVDesc = UpdatedViewDesc;
                // Always create texture array UAV
                UAVDesc.TextureDim      = RESOURCE_DIM_TEX_2D_ARRAY;
                UAVDesc.ViewType        = TEXTURE_VIEW_UNORDERED_ACCESS;
                UAVDesc.MostDetailedMip = MipLevel;
                UAVDesc.NumMipLevels    = 1;
                if (UAVDesc.Format == TEX_FORMAT_RGBA8_UNORM_SRGB)
                    UAVDesc.Format = TEX_FORMAT_RGBA8_UNORM;
                else if (UAVDesc.Format == TEX_FORMAT_BGRA8_UNORM_SRGB)
                    UAVDesc.Format = TEX_FORMAT_BGRA8_UNORM;
                CreateUAV(UAVDesc, MipUAVDescriptors.GetCpuHandle(MipLevel));
            }
        }
        auto pViewD3D12 = NEW_RC_OBJ(TexViewAllocator, "TextureViewD3D12Impl instance", TextureViewD3D12Impl, bIsDefaultView ? this : nullptr)(
            GetDevice(), UpdatedViewDesc, this, std::move(ViewDescriptor), std::move(TexArraySRVDescriptor), std::move(MipUAVDescriptors), bIsDefaultView);
        VERIFY(pViewD3D12->GetDesc().ViewType == ViewDesc.ViewType, "Incorrect view type");

        if (bIsDefaultView)
            *ppView = pViewD3D12;
        else
            pViewD3D12->QueryInterface(IID_TextureView, reinterpret_cast<IObject**>(ppView));
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetTexViewTypeLiteralName(ViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", ViewDesc.Name ? ViewDesc.Name : "", "\" (", ViewTypeName, ") for texture \"", m_Desc.Name ? m_Desc.Name : "", "\"");
    }
}

TextureD3D12Impl::~TextureD3D12Impl()
{
    // D3D12 object can only be destroyed when it is no longer used by the GPU
    GetDevice()->SafeReleaseDeviceObject(std::move(m_pd3d12Resource), m_Desc.ImmediateContextMask);
    if (m_StagingFootprints != nullptr)
    {
        FREE(GetRawAllocator(), m_StagingFootprints);
    }
}

void TextureD3D12Impl::CreateSRV(const TextureViewDesc& SRVDesc, D3D12_CPU_DESCRIPTOR_HANDLE SRVHandle)
{
    VERIFY(SRVDesc.ViewType == TEXTURE_VIEW_SHADER_RESOURCE, "Incorrect view type: shader resource is expected");
    VERIFY_EXPR(SRVDesc.Format != TEX_FORMAT_UNKNOWN);

    D3D12_SHADER_RESOURCE_VIEW_DESC D3D12_SRVDesc;
    TextureViewDesc_to_D3D12_SRV_DESC(SRVDesc, D3D12_SRVDesc, m_Desc.SampleCount);

    auto* pd3d12Device = GetDevice()->GetD3D12Device();
    pd3d12Device->CreateShaderResourceView(m_pd3d12Resource, &D3D12_SRVDesc, SRVHandle);
}

void TextureD3D12Impl::CreateRTV(const TextureViewDesc& RTVDesc, D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle)
{
    VERIFY(RTVDesc.ViewType == TEXTURE_VIEW_RENDER_TARGET, "Incorrect view type: render target is expected");
    VERIFY_EXPR(RTVDesc.Format != TEX_FORMAT_UNKNOWN);

    D3D12_RENDER_TARGET_VIEW_DESC D3D12_RTVDesc;
    TextureViewDesc_to_D3D12_RTV_DESC(RTVDesc, D3D12_RTVDesc, m_Desc.SampleCount);

    auto* pd3d12Device = GetDevice()->GetD3D12Device();
    pd3d12Device->CreateRenderTargetView(m_pd3d12Resource, &D3D12_RTVDesc, RTVHandle);
}

void TextureD3D12Impl::CreateDSV(const TextureViewDesc& DSVDesc, D3D12_CPU_DESCRIPTOR_HANDLE DSVHandle)
{
    VERIFY(DSVDesc.ViewType == TEXTURE_VIEW_DEPTH_STENCIL || DSVDesc.ViewType == TEXTURE_VIEW_READ_ONLY_DEPTH_STENCIL, "Incorrect view type: depth stencil is expected");
    VERIFY_EXPR(DSVDesc.Format != TEX_FORMAT_UNKNOWN);

    D3D12_DEPTH_STENCIL_VIEW_DESC D3D12_DSVDesc;
    TextureViewDesc_to_D3D12_DSV_DESC(DSVDesc, D3D12_DSVDesc, m_Desc.SampleCount);

    auto* pd3d12Device = GetDevice()->GetD3D12Device();
    pd3d12Device->CreateDepthStencilView(m_pd3d12Resource, &D3D12_DSVDesc, DSVHandle);
}

void TextureD3D12Impl::CreateUAV(const TextureViewDesc& UAVDesc, D3D12_CPU_DESCRIPTOR_HANDLE UAVHandle)
{
    VERIFY(UAVDesc.ViewType == TEXTURE_VIEW_UNORDERED_ACCESS, "Incorrect view type: unordered access is expected");
    VERIFY_EXPR(UAVDesc.Format != TEX_FORMAT_UNKNOWN);

    D3D12_UNORDERED_ACCESS_VIEW_DESC D3D12_UAVDesc;
    TextureViewDesc_to_D3D12_UAV_DESC(UAVDesc, D3D12_UAVDesc);

    auto* pd3d12Device = GetDevice()->GetD3D12Device();
    pd3d12Device->CreateUnorderedAccessView(m_pd3d12Resource, nullptr, &D3D12_UAVDesc, UAVHandle);
}

void TextureD3D12Impl::SetD3D12ResourceState(D3D12_RESOURCE_STATES state)
{
    SetState(D3D12ResourceStatesToResourceStateFlags(state));
}

D3D12_RESOURCE_STATES TextureD3D12Impl::GetD3D12ResourceState() const
{
    return ResourceStateFlagsToD3D12ResourceStates(GetState());
}

void TextureD3D12Impl::InitSparseProperties()
{
    VERIFY_EXPR(m_Desc.Usage == USAGE_SPARSE);
    VERIFY_EXPR(m_pSparseProps == nullptr);

    m_pSparseProps = std::make_unique<SparseTextureProperties>();

    if (IsUsingNVApi())
    {
        *m_pSparseProps = GetStandardSparseTextureProperties(m_Desc);
    }
    else
    {
        auto* pd3d12Device = m_pDevice->GetD3D12Device();

        UINT                  NumTilesForEntireResource = 0;
        D3D12_PACKED_MIP_INFO PackedMipDesc{};
        D3D12_TILE_SHAPE      StandardTileShapeForNonPackedMips{};
        UINT                  NumSubresourceTilings = 0;
        pd3d12Device->GetResourceTiling(GetD3D12Resource(),
                                        &NumTilesForEntireResource,
                                        &PackedMipDesc,
                                        &StandardTileShapeForNonPackedMips,
                                        &NumSubresourceTilings,
                                        0,
                                        nullptr);

        auto& Props{*m_pSparseProps};
        Props.AddressSpaceSize = Uint64{NumTilesForEntireResource} * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.BlockSize        = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.MipTailOffset    = Uint64{PackedMipDesc.StartTileIndexInOverallResource} * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.MipTailSize      = Uint64{PackedMipDesc.NumTilesForPackedMips} * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
        Props.FirstMipInTail   = PackedMipDesc.NumStandardMips;
        Props.TileSize[0]      = StandardTileShapeForNonPackedMips.WidthInTexels;
        Props.TileSize[1]      = StandardTileShapeForNonPackedMips.HeightInTexels;
        Props.TileSize[2]      = StandardTileShapeForNonPackedMips.DepthInTexels;
        Props.Flags            = SPARSE_TEXTURE_FLAG_NONE;

        // The number of overall tiles, packed or not, for a given array slice is simply the total number of tiles for the resource divided by the resource's array size
        Props.MipTailStride = m_Desc.IsArray() ? (NumTilesForEntireResource / m_Desc.ArraySize) * D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES : 0;
        VERIFY_EXPR(NumTilesForEntireResource % m_Desc.GetArraySize() == 0);
    }
}

} // namespace Diligent
