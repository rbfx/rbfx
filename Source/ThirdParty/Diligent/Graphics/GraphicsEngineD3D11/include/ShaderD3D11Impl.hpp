/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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
/// Declaration of Diligent::ShaderD3D11Impl class

#include <mutex>
#include <unordered_map>
#include <cstring>

#include <atlbase.h>

#include "EngineD3D11ImplTraits.hpp"
#include "ShaderBase.hpp"
#include "ShaderD3DBase.hpp"
#include "ShaderResourcesD3D11.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

/// Shader implementation in Direct3D11 backend.
class ShaderD3D11Impl final : public ShaderD3DBase<EngineD3D11ImplTraits, ShaderResourcesD3D11>
{
public:
    using TShaderBase = ShaderD3DBase<EngineD3D11ImplTraits, ShaderResourcesD3D11>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0xc6e1e44d, 0xb9d7, 0x4793, {0xb3, 0x8f, 0x4c, 0x2e, 0xb3, 0x9f, 0x20, 0xb0}};

    struct CreateInfo : TShaderBase::CreateInfo
    {
        const D3D_FEATURE_LEVEL FeatureLevel;

        CreateInfo(const TShaderBase::CreateInfo& _BaseCreateInfo, D3D_FEATURE_LEVEL _FeatureLevel) :
            TShaderBase::CreateInfo{_BaseCreateInfo},
            FeatureLevel{_FeatureLevel}
        {}
    };
    ShaderD3D11Impl(IReferenceCounters*          pRefCounters,
                    class RenderDeviceD3D11Impl* pRenderDeviceD3D11,
                    const ShaderCreateInfo&      ShaderCI,
                    const CreateInfo&            D3D11ShaderCI,
                    bool                         IsDeviceInternal = false);
    ~ShaderD3D11Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;


    /// Implementation of IShaderD3D11::GetD3D11Shader() method.
    virtual ID3D11DeviceChild* DILIGENT_CALL_TYPE GetD3D11Shader() override final
    {
        DEV_CHECK_ERR(!IsCompiling(), "Shader bytecode is not available until compilation is complete. Use GetStatus() to check the shader status.");
        return GetD3D11Shader(m_pShaderByteCode);
    }

    ID3D11DeviceChild* GetD3D11Shader(IDataBlob* pBytecode) noexcept(false);

private:
    struct BlobHashKey
    {
        const size_t             Hash;
        RefCntAutoPtr<IDataBlob> pBlob;

        explicit BlobHashKey(IDataBlob* _pBlob) :
            Hash{ComputeHashRaw(_pBlob->GetConstDataPtr(), _pBlob->GetSize())},
            pBlob{_pBlob}
        {}

        struct Hasher
        {
            size_t operator()(const BlobHashKey& Key) const noexcept
            {
                return Key.Hash;
            }
        };

        bool operator==(const BlobHashKey& rhs) const noexcept
        {
            if (Hash != rhs.Hash)
                return false;

            const auto Size0 = pBlob->GetSize();
            const auto Size1 = rhs.pBlob->GetSize();
            if (Size0 != Size1)
                return false;

            return memcmp(pBlob->GetConstDataPtr(), rhs.pBlob->GetConstDataPtr(), Size0) == 0;
        }
    };

    std::mutex                                                                       m_d3dShaderCacheMtx;
    std::unordered_map<BlobHashKey, CComPtr<ID3D11DeviceChild>, BlobHashKey::Hasher> m_d3dShaderCache;
};

} // namespace Diligent
