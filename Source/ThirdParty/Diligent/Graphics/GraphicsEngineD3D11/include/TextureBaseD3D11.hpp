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
/// Declaration of Diligent::TextureBaseD3D11 class

#include "EngineD3D11ImplTraits.hpp"
#include "TextureBase.hpp"
#include "TextureViewD3D11Impl.hpp"
#include "RenderDeviceD3D11Impl.hpp"
#include "ResourceD3D11Base.hpp"

namespace Diligent
{

class FixedBlockMemoryAllocator;

/// Base implementation of a texture object in Direct3D11 backend.
class TextureBaseD3D11 : public TextureBase<EngineD3D11ImplTraits>, public ResourceD3D11Base
{
public:
    using TTextureBase = TextureBase<EngineD3D11ImplTraits>;
    using ViewImplType = TextureViewD3D11Impl;

    TextureBaseD3D11(IReferenceCounters*          pRefCounters,
                     FixedBlockMemoryAllocator&   TexViewObjAllocator,
                     class RenderDeviceD3D11Impl* pDeviceD3D11,
                     const TextureDesc&           TexDesc,
                     const TextureData*           pInitData = nullptr);
    ~TextureBaseD3D11();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of ITextureD3D11::GetD3D11Texture().
    virtual ID3D11Resource* DILIGENT_CALL_TYPE GetD3D11Texture() const override final { return m_pd3d11Texture; }

    /// Implementation of ITexture::GetNativeHandle().
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final { return BitCast<Uint64>(GetD3D11Texture()); }

    void AddState(RESOURCE_STATE State)
    {
        m_State &= ~(RESOURCE_STATE_COMMON | RESOURCE_STATE_UNDEFINED);
        m_State |= State;
    }

    void ClearState(RESOURCE_STATE State)
    {
        VERIFY_EXPR(IsInKnownState());
        m_State &= ~State;
        if (m_State == RESOURCE_STATE_UNKNOWN)
            m_State = RESOURCE_STATE_UNDEFINED;
    }

    bool IsUsingNVApi() const
    {
        return m_Desc.Usage == USAGE_SPARSE && m_Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY && m_pDevice->IsNvApiEnabled();
    }

protected:
    void CreateViewInternal(const struct TextureViewDesc& ViewDesc, ITextureView** ppView, bool bIsDefaultView) override final;
    void PrepareD3D11InitData(const TextureData*                                                               pInitData,
                              Uint32                                                                           NumSubresources,
                              std::vector<D3D11_SUBRESOURCE_DATA, STDAllocatorRawMem<D3D11_SUBRESOURCE_DATA>>& D3D11InitData);

    void InitSparseProperties();

    // clang-format off
    virtual void CreateSRV(const TextureViewDesc& SRVDesc, ID3D11ShaderResourceView**  ppD3D11SRV) = 0;
    virtual void CreateRTV(const TextureViewDesc& RTVDesc, ID3D11RenderTargetView**    ppD3D11RTV) = 0;
    virtual void CreateDSV(const TextureViewDesc& DSVDesc, ID3D11DepthStencilView**    ppD3D11DSV) = 0;
    virtual void CreateUAV(const TextureViewDesc& UAVDesc, ID3D11UnorderedAccessView** ppD3D11UAV) = 0;
    // clang-format on

    friend class RenderDeviceD3D11Impl;
    /// D3D11 texture
    CComPtr<ID3D11Resource> m_pd3d11Texture;
};

} // namespace Diligent
