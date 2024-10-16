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

#include "BufferD3D11Impl.hpp"

#include "RenderDeviceD3D11Impl.hpp"
#include "DeviceContextD3D11Impl.hpp"
#include "BufferViewD3D11Impl.hpp"

#include "D3D11TypeConversions.hpp"
#include "GraphicsAccessories.hpp"
#include "EngineMemory.h"
#include "Align.hpp"

namespace Diligent
{

BufferD3D11Impl::BufferD3D11Impl(IReferenceCounters*        pRefCounters,
                                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                                 RenderDeviceD3D11Impl*     pRenderDeviceD3D11,
                                 const BufferDesc&          BuffDesc,
                                 const BufferData*          pBuffData /*= nullptr*/) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pRenderDeviceD3D11,
        BuffDesc,
        false
    }
// clang-format on
{
    ValidateBufferInitData(BuffDesc, pBuffData);

    if (m_Desc.Usage == USAGE_UNIFIED)
    {
        LOG_ERROR_AND_THROW("Unified resources are not supported in Direct3D11");
    }

    if (m_Desc.BindFlags & BIND_UNIFORM_BUFFER)
    {
        // Note that Direct3D11 does not allow partial updates of constant buffers with UpdateSubresource().
        // Only the entire buffer may be updated.
        static constexpr Uint32 Alignment{16};
        m_Desc.Size = AlignUp(m_Desc.Size, Alignment);
    }

    VERIFY_EXPR(m_Desc.Size <= std::numeric_limits<Uint32>::max()); // duplicates check in ValidateBufferDesc()

    D3D11_BUFFER_DESC D3D11BuffDesc{};
    D3D11BuffDesc.BindFlags = BindFlagsToD3D11BindFlags(m_Desc.BindFlags);
    D3D11BuffDesc.ByteWidth = StaticCast<UINT>(m_Desc.Size);
    D3D11BuffDesc.MiscFlags = 0;
    if (m_Desc.BindFlags & BIND_INDIRECT_DRAW_ARGS)
    {
        D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
    }
    if (m_Desc.Usage == USAGE_SPARSE)
    {
        D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_TILED;
    }
    D3D11BuffDesc.Usage = UsageToD3D11Usage(m_Desc.Usage);

    // Size of each element in the buffer structure (in bytes) when the buffer represents a structured buffer, or
    // the size of the format that is used for views of the buffer.
    D3D11BuffDesc.StructureByteStride = m_Desc.ElementByteStride;
    if ((m_Desc.BindFlags & BIND_UNORDERED_ACCESS) || (m_Desc.BindFlags & BIND_SHADER_RESOURCE))
    {
        if (m_Desc.Mode == BUFFER_MODE_STRUCTURED)
        {
            D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            VERIFY(D3D11BuffDesc.StructureByteStride != 0, "StructureByteStride cannot be zero for a structured buffer");
        }
        else if (m_Desc.Mode == BUFFER_MODE_FORMATTED)
        {
            VERIFY(D3D11BuffDesc.StructureByteStride != 0, "StructureByteStride cannot not be zero for a formatted buffer");
        }
        else if (m_Desc.Mode == BUFFER_MODE_RAW)
        {
            D3D11BuffDesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
        }
        else
        {
            UNEXPECTED("Unexpected buffer mode");
        }
    }

    D3D11BuffDesc.CPUAccessFlags = CPUAccessFlagsToD3D11CPUAccessFlags(m_Desc.CPUAccessFlags);

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem          = pBuffData ? pBuffData->pData : nullptr;
    InitData.SysMemPitch      = 0;
    InitData.SysMemSlicePitch = 0;

    auto* pDeviceD3D11 = pRenderDeviceD3D11->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pDeviceD3D11->CreateBuffer(&D3D11BuffDesc, InitData.pSysMem ? &InitData : nullptr, &m_pd3d11Buffer),
                           "Failed to create the Direct3D11 buffer");

    if (*m_Desc.Name != 0)
    {
        auto hr = m_pd3d11Buffer->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(strlen(m_Desc.Name)), m_Desc.Name);
        DEV_CHECK_ERR(SUCCEEDED(hr), "Failed to set buffer name");
    }

    SetState(m_Desc.Usage == USAGE_DYNAMIC ? RESOURCE_STATE_GENERIC_READ : RESOURCE_STATE_UNDEFINED);

    // The memory is always coherent in Direct3D11
    m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;
}

static BufferDesc BuffDescFromD3D11Buffer(ID3D11Buffer* pd3d11Buffer, BufferDesc BuffDesc)
{
    D3D11_BUFFER_DESC D3D11BuffDesc;
    pd3d11Buffer->GetDesc(&D3D11BuffDesc);

    VERIFY(BuffDesc.Size == 0 || BuffDesc.Size == D3D11BuffDesc.ByteWidth,
           "The buffer size specified by the BufferDesc (", BuffDesc.Size,
           ") does not match the d3d11 buffer size (", D3D11BuffDesc.ByteWidth, ")");
    BuffDesc.Size = Uint32{D3D11BuffDesc.ByteWidth};

    auto BindFlags = D3D11BindFlagsToBindFlags(D3D11BuffDesc.BindFlags);
    if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS)
        BindFlags |= BIND_INDIRECT_DRAW_ARGS;
    VERIFY(BuffDesc.BindFlags == 0 || BuffDesc.BindFlags == BindFlags,
           "Bind flags specified by the BufferDesc (", GetBindFlagsString(BuffDesc.BindFlags),
           ") do not match the bind flags recovered from d3d11 buffer desc (", GetBindFlagsString(BindFlags), ")");
    BuffDesc.BindFlags = BindFlags;

    auto Usage = D3D11UsageToUsage(D3D11BuffDesc.Usage);
    if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_TILED)
    {
        VERIFY_EXPR(Usage == USAGE_DEFAULT);
        Usage = USAGE_SPARSE;

        // In Direct3D11, sparse resources are always aliased
        BuffDesc.MiscFlags |= MISC_BUFFER_FLAG_SPARSE_ALIASING;
    }
    VERIFY(BuffDesc.Usage == 0 || BuffDesc.Usage == Usage,
           "Usage specified by the BufferDesc (", GetUsageString(BuffDesc.Usage),
           ") does not match the buffer usage recovered from d3d11 buffer desc (", GetUsageString(Usage), ")");
    BuffDesc.Usage = Usage;

    auto CPUAccessFlags = D3D11CPUAccessFlagsToCPUAccessFlags(D3D11BuffDesc.CPUAccessFlags);
    VERIFY(BuffDesc.CPUAccessFlags == 0 || BuffDesc.CPUAccessFlags == CPUAccessFlags,
           "CPU access flags specified by the BufferDesc (", GetCPUAccessFlagsString(BuffDesc.CPUAccessFlags),
           ") do not match CPU access flags recovered from d3d11 buffer desc (", GetCPUAccessFlagsString(CPUAccessFlags), ")");
    BuffDesc.CPUAccessFlags = CPUAccessFlags;

    if ((BuffDesc.BindFlags & BIND_UNORDERED_ACCESS) || (BuffDesc.BindFlags & BIND_SHADER_RESOURCE))
    {
        if (D3D11BuffDesc.StructureByteStride != 0)
        {
            VERIFY(BuffDesc.ElementByteStride == 0 || BuffDesc.ElementByteStride == D3D11BuffDesc.StructureByteStride,
                   "Element byte stride specified by the BufferDesc (", BuffDesc.ElementByteStride,
                   ") does not match the structured byte stride recovered from d3d11 buffer desc (", D3D11BuffDesc.StructureByteStride, ")");
            BuffDesc.ElementByteStride = Uint32{D3D11BuffDesc.StructureByteStride};
        }
        if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
        {
            VERIFY(BuffDesc.Mode == BUFFER_MODE_UNDEFINED || BuffDesc.Mode == BUFFER_MODE_STRUCTURED,
                   "Inconsistent buffer mode: BUFFER_MODE_STRUCTURED or BUFFER_MODE_UNDEFINED is expected");
            BuffDesc.Mode = BUFFER_MODE_STRUCTURED;
        }
        else if (D3D11BuffDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
        {
            VERIFY(BuffDesc.Mode == BUFFER_MODE_UNDEFINED || BuffDesc.Mode == BUFFER_MODE_RAW,
                   "Inconsistent buffer mode: BUFFER_MODE_RAW or BUFFER_MODE_UNDEFINED is expected");
            BuffDesc.Mode = BUFFER_MODE_RAW;
        }
        else
        {
            if (BuffDesc.ElementByteStride != 0)
            {
                VERIFY(BuffDesc.Mode == BUFFER_MODE_UNDEFINED || BuffDesc.Mode == BUFFER_MODE_FORMATTED,
                       "Inconsistent buffer mode: BUFFER_MODE_FORMATTED or BUFFER_MODE_UNDEFINED is expected");
                BuffDesc.Mode = BUFFER_MODE_FORMATTED;
            }
            else
                BuffDesc.Mode = BUFFER_MODE_UNDEFINED;
        }
    }

    return BuffDesc;
}
BufferD3D11Impl::BufferD3D11Impl(IReferenceCounters*          pRefCounters,
                                 FixedBlockMemoryAllocator&   BuffViewObjMemAllocator,
                                 class RenderDeviceD3D11Impl* pDeviceD3D11,
                                 const BufferDesc&            BuffDesc,
                                 RESOURCE_STATE               InitialState,
                                 ID3D11Buffer*                pd3d11Buffer) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pDeviceD3D11,
        BuffDescFromD3D11Buffer(pd3d11Buffer, BuffDesc),
        false
    }
// clang-format on
{
    m_pd3d11Buffer = pd3d11Buffer;
    SetState(InitialState);

    // The memory is always coherent in Direct3D11
    m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;
}

BufferD3D11Impl::~BufferD3D11Impl()
{
}

IMPLEMENT_QUERY_INTERFACE(BufferD3D11Impl, IID_BufferD3D11, TBufferBase)


void BufferD3D11Impl::CreateViewInternal(const BufferViewDesc& OrigViewDesc, IBufferView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "Null pointer provided");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto* pDeviceD3D11Impl  = GetDevice();
        auto& BuffViewAllocator = pDeviceD3D11Impl->GetBuffViewObjAllocator();
        VERIFY(&BuffViewAllocator == &m_dbgBuffViewAllocator, "Buff view allocator does not match allocator provided at buffer initialization");

        BufferViewDesc ViewDesc = OrigViewDesc;
        if (ViewDesc.ViewType == BUFFER_VIEW_UNORDERED_ACCESS)
        {
            CComPtr<ID3D11UnorderedAccessView> pUAV;
            CreateUAV(ViewDesc, &pUAV);
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewD3D11Impl instance", BufferViewD3D11Impl, bIsDefaultView ? this : nullptr)(pDeviceD3D11Impl, ViewDesc, this, pUAV, bIsDefaultView);
        }
        else if (ViewDesc.ViewType == BUFFER_VIEW_SHADER_RESOURCE)
        {
            CComPtr<ID3D11ShaderResourceView> pSRV;
            CreateSRV(ViewDesc, &pSRV);
            *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewD3D11Impl instance", BufferViewD3D11Impl, bIsDefaultView ? this : nullptr)(pDeviceD3D11Impl, ViewDesc, this, pSRV, bIsDefaultView);
        }

        if (!bIsDefaultView && *ppView)
            (*ppView)->AddRef();
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetBufferViewTypeLiteralName(OrigViewDesc.ViewType);
        LOG_ERROR("Failed to create view \"", OrigViewDesc.Name ? OrigViewDesc.Name : "", "\" (", ViewTypeName, ") for buffer \"", m_Desc.Name, "\"");
    }
}

void BufferD3D11Impl::CreateUAV(BufferViewDesc& UAVDesc, ID3D11UnorderedAccessView** ppD3D11UAV)
{
    ValidateAndCorrectBufferViewDesc(m_Desc, UAVDesc, GetDevice()->GetAdapterInfo().Buffer.StructuredBufferOffsetAlignment);

    D3D11_UNORDERED_ACCESS_VIEW_DESC D3D11_UAVDesc;
    BufferViewDesc_to_D3D11_UAV_DESC(m_Desc, UAVDesc, D3D11_UAVDesc);

    auto* pd3d11Device = GetDevice()->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pd3d11Device->CreateUnorderedAccessView(m_pd3d11Buffer, &D3D11_UAVDesc, ppD3D11UAV),
                           "Failed to create D3D11 unordered access view");
}

void BufferD3D11Impl::CreateSRV(struct BufferViewDesc& SRVDesc, ID3D11ShaderResourceView** ppD3D11SRV)
{
    ValidateAndCorrectBufferViewDesc(m_Desc, SRVDesc, m_pDevice->GetAdapterInfo().Buffer.StructuredBufferOffsetAlignment);

    D3D11_SHADER_RESOURCE_VIEW_DESC D3D11_SRVDesc;
    BufferViewDesc_to_D3D11_SRV_DESC(m_Desc, SRVDesc, D3D11_SRVDesc);

    auto* pd3d11Device = GetDevice()->GetD3D11Device();
    CHECK_D3D_RESULT_THROW(pd3d11Device->CreateShaderResourceView(m_pd3d11Buffer, &D3D11_SRVDesc, ppD3D11SRV),
                           "Failed to create D3D11 shader resource view");
}

SparseBufferProperties BufferD3D11Impl::GetSparseProperties() const
{
    DEV_CHECK_ERR(m_Desc.Usage == USAGE_SPARSE,
                  "IBuffer::GetSparseProperties() should only be used for sparse buffer");

    auto* pd3d11Device2 = m_pDevice->GetD3D11Device2();

    UINT             NumTilesForEntireResource = 0;
    D3D11_TILE_SHAPE StandardTileShapeForNonPackedMips{};
    pd3d11Device2->GetResourceTiling(m_pd3d11Buffer,
                                     &NumTilesForEntireResource,
                                     nullptr,
                                     &StandardTileShapeForNonPackedMips,
                                     nullptr,
                                     0,
                                     nullptr);

    VERIFY(StandardTileShapeForNonPackedMips.WidthInTexels == D3D11_2_TILED_RESOURCE_TILE_SIZE_IN_BYTES,
           "Expected to be a standard block size");

    SparseBufferProperties Props;
    Props.AddressSpaceSize = Uint64{NumTilesForEntireResource} * StandardTileShapeForNonPackedMips.WidthInTexels;
    Props.BlockSize        = StandardTileShapeForNonPackedMips.WidthInTexels;
    return Props;
}

} // namespace Diligent
