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
/// Declaration of Diligent::ShaderD3D12Impl class

#include "EngineD3D12ImplTraits.hpp"
#include "ShaderBase.hpp"
#include "ShaderD3DBase.hpp"
#include "ShaderResourcesD3D12.hpp"

namespace Diligent
{

/// Implementation of a shader object in Direct3D12 backend.
class ShaderD3D12Impl final : public ShaderBase<EngineD3D12ImplTraits>, public ShaderD3DBase
{
public:
    using TShaderBase = ShaderBase<EngineD3D12ImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0x98a800f1, 0x673, 0x4a39, {0xaf, 0x28, 0xa4, 0xa5, 0xd6, 0x3e, 0x84, 0xa2}};

    struct CreateInfo
    {
        IDXCompiler* const         pDXCompiler;
        const RenderDeviceInfo&    DeviceInfo;
        const GraphicsAdapterInfo& AdapterInfo;
        const ShaderVersion        MaxShaderVersion;
    };
    ShaderD3D12Impl(IReferenceCounters*     pRefCounters,
                    RenderDeviceD3D12Impl*  pRenderDeviceD3D12,
                    const ShaderCreateInfo& ShaderCI,
                    const CreateInfo&       D3D12ShaderCI,
                    bool                    IsDeviceInternal = false);
    ~ShaderD3D12Impl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IShader::GetResourceCount() in Direct3D12 backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final
    {
        return m_pShaderResources ? m_pShaderResources->GetTotalResources() : 0;
    }

    /// Implementation of IShader::GetResource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final
    {
        if (m_pShaderResources)
            ResourceDesc = m_pShaderResources->GetHLSLShaderResourceDesc(Index);
    }

    /// Implementation of IShader::GetConstantBufferDesc() in Direct3D12 backend.
    virtual const ShaderCodeBufferDesc* DILIGENT_CALL_TYPE GetConstantBufferDesc(Uint32 Index) const override final
    {
        return m_pShaderResources ?
            // Constant buffers always go first in the list of resources
            m_pShaderResources->GetConstantBufferDesc(Index) :
            nullptr;
    }

    /// Implementation of IShaderD3D::GetHLSLResource() in Direct3D12 backend.
    virtual void DILIGENT_CALL_TYPE GetHLSLResource(Uint32 Index, HLSLShaderResourceDesc& ResourceDesc) const override final
    {
        if (m_pShaderResources)
            ResourceDesc = m_pShaderResources->GetHLSLShaderResourceDesc(Index);
    }

    virtual void DILIGENT_CALL_TYPE GetBytecode(const void** ppBytecode,
                                                Uint64&      Size) const override final
    {
        ShaderD3DBase::GetBytecode(ppBytecode, Size);
    }

    const Char* GetEntryPoint() const { return m_EntryPoint.c_str(); }

    const std::shared_ptr<const ShaderResourcesD3D12>& GetShaderResources() const { return m_pShaderResources; }

private:
    // ShaderResources class instance must be referenced through the shared pointer, because
    // it is referenced by PipelineStateD3D12Impl class instances
    std::shared_ptr<const ShaderResourcesD3D12> m_pShaderResources;

    String m_EntryPoint;
};

} // namespace Diligent
