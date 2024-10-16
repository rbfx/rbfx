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

#pragma once

/// \file
/// Declaration of Diligent::ShaderWebGPUImpl class

#include "EngineWebGPUImplTraits.hpp"
#include "ShaderBase.hpp"
#include "WGSLShaderResources.hpp"

namespace Diligent
{

/// Shader implementation in WebGPU backend.
class ShaderWebGPUImpl final : public ShaderBase<EngineWebGPUImplTraits>
{
public:
    using TShaderBase = ShaderBase<EngineWebGPUImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0x53C34f8A, 0x25F2, 0x46CD, {0x9C, 0x40, 0x87, 0x44, 0x26, 0x22, 0xA5, 0x4B}};

    struct CreateInfo
    {
        const RenderDeviceInfo&    DeviceInfo;
        const GraphicsAdapterInfo& AdapterInfo;
        IDataBlob** const          ppCompilerOutput;
        IThreadPool* const         pCompilationThreadPool;
    };

    ShaderWebGPUImpl(IReferenceCounters*     pRefCounters,
                     RenderDeviceWebGPUImpl* pDeviceWebGPU,
                     const ShaderCreateInfo& ShaderCI,
                     const CreateInfo&       WebGPUShaderCI,
                     bool                    IsDeviceInternal = false);

    ~ShaderWebGPUImpl() override;

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_ShaderWebGPU, IID_InternalImpl, TShaderBase)

    /// Implementation of IShader::GetResourceCount() in WebGPU backend.
    Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final;

    /// Implementation of IShader::GetResourceDesc() in WebGPU backend.
    void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final;

    /// Implementation of IShader::GetConstantBufferDesc() in WebGPU backend.
    const ShaderCodeBufferDesc* DILIGENT_CALL_TYPE GetConstantBufferDesc(Uint32 Index) const override final;

    /// Implementation of IShader::GetBytecode() in WebGPU backend.
    void DILIGENT_CALL_TYPE GetBytecode(const void** ppBytecode, Uint64& Size) const override final;

    /// Implementation of IShaderWebGPU::GetWGSL().
    const std::string& DILIGENT_CALL_TYPE GetWGSL() const override final;

    /// Implementation of IShaderWebGPU::GetEmulatedArrayIndexSuffix().
    const char* DILIGENT_CALL_TYPE GetEmulatedArrayIndexSuffix() const override final
    {
        return m_pShaderResources->GetEmulatedArrayIndexSuffix();
    }

    const char* GetEntryPoint() const;

    const std::shared_ptr<const WGSLShaderResources>& GetShaderResources() const
    {
        DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");
        return m_pShaderResources;
    }

private:
    void Initialize(const ShaderCreateInfo& ShaderCI,
                    const CreateInfo&       WebGPUShaderCI) noexcept(false);

    std::string m_WGSL;
    std::string m_EntryPoint;

    std::shared_ptr<const WGSLShaderResources> m_pShaderResources;
};

} // namespace Diligent
