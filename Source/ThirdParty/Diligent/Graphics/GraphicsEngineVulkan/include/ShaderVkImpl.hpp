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
/// Declaration of Diligent::ShaderVkImpl class

#include "EngineVkImplTraits.hpp"
#include "ShaderBase.hpp"
#include "SPIRVShaderResources.hpp"
#include "ThreadPool.h"
#include "RefCntAutoPtr.hpp"

namespace Diligent
{
class IDXCompiler;

/// Shader object object implementation in Vulkan backend.
class ShaderVkImpl final : public ShaderBase<EngineVkImplTraits>
{
public:
    using TShaderBase = ShaderBase<EngineVkImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0x17523656, 0x19a6, 0x4874, {0x8c, 0x48, 0x74, 0xf5, 0xb7, 0x2, 0x31, 0x1}};

    struct CreateInfo
    {
        IDXCompiler* const         pDXCompiler;
        const RenderDeviceInfo&    DeviceInfo;
        const GraphicsAdapterInfo& AdapterInfo;
        const Uint32               VkVersion;
        const bool                 HasSpirv14;
        IDataBlob** const          ppCompilerOutput;
        IThreadPool* const         pCompilationThreadPool;
    };
    ShaderVkImpl(IReferenceCounters*     pRefCounters,
                 RenderDeviceVkImpl*     pRenderDeviceVk,
                 const ShaderCreateInfo& ShaderCI,
                 const CreateInfo&       VkShaderCI,
                 bool                    IsDeviceInternal = false);
    ~ShaderVkImpl();

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_ShaderVk, IID_InternalImpl, TShaderBase)

    /// Implementation of IShader::GetResourceCount() in Vulkan backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final
    {
        DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");
        return m_pShaderResources ? m_pShaderResources->GetTotalResources() : 0;
    }

    /// Implementation of IShader::GetResource() in Vulkan backend.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final;

    /// Implementation of IShader::GetConstantBufferDesc() in Vulkan backend.
    virtual const ShaderCodeBufferDesc* DILIGENT_CALL_TYPE GetConstantBufferDesc(Uint32 Index) const override final;

    /// Implementation of IShaderVk::GetSPIRV().
    virtual const std::vector<uint32_t>& DILIGENT_CALL_TYPE GetSPIRV() const override final
    {
        static const std::vector<uint32_t> NullSPIRV;
        // NOTE: while shader is compiled asynchronously, m_SPIRV may be modified by
        //       another thread and thus can't be accessed.
        return !IsCompiling() ? m_SPIRV : NullSPIRV;
    }

    const std::shared_ptr<const SPIRVShaderResources>& GetShaderResources() const
    {
        DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");
        return m_pShaderResources;
    }

    const char* GetEntryPoint() const
    {
        DEV_CHECK_ERR(!IsCompiling(), "Shader resources are not available until the shader is compiled. Use GetStatus() to check the shader status.");
        return m_EntryPoint.c_str();
    }

    virtual void DILIGENT_CALL_TYPE GetBytecode(const void** ppBytecode,
                                                Uint64&      Size) const override final
    {
        DEV_CHECK_ERR(!IsCompiling(), "Shader byte code is not available until the shader is compiled. Use GetStatus() to check the shader status.");
        *ppBytecode = !m_SPIRV.empty() ? m_SPIRV.data() : nullptr;
        Size        = m_SPIRV.size() * sizeof(m_SPIRV[0]);
    }

private:
    void Initialize(const ShaderCreateInfo& ShaderCI,
                    const CreateInfo&       VkShaderCI) noexcept(false);

private:
    std::shared_ptr<const SPIRVShaderResources> m_pShaderResources;

    std::string           m_EntryPoint;
    std::vector<uint32_t> m_SPIRV;
};

} // namespace Diligent
