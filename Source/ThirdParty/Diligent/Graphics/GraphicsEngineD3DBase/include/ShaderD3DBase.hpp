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

#include "WinHPreface.h"
#include <d3dcommon.h>
#include "WinHPostface.h"

#include <functional>
#include <memory>

#include "ShaderD3D.h"
#include "DataBlob.h"
#include "ShaderBase.hpp"
#include "ThreadPool.h"
#include "RefCntAutoPtr.hpp"

/// \file
/// Base implementation of a D3D shader

namespace Diligent
{

class IDXCompiler;

// AddRef/Release methods of ID3DBlob are not thread safe, so use Diligent::IDataBlob.
RefCntAutoPtr<IDataBlob> CompileD3DBytecode(const ShaderCreateInfo& ShaderCI,
                                            const ShaderVersion     ShaderModel,
                                            IDXCompiler*            DxCompiler,
                                            IDataBlob**             ppCompilerOutput) noexcept(false);

/// Base implementation of a D3D shader
template <typename EngineImplTraits, typename ShaderResourcesType>
class ShaderD3DBase : public ShaderBase<EngineImplTraits>
{
public:
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    struct CreateInfo
    {
        const RenderDeviceInfo&    DeviceInfo;
        const GraphicsAdapterInfo& AdapterInfo;
        IDXCompiler* const         pDXCompiler;
        IDataBlob** const          ppCompilerOutput;
        IThreadPool* const         pShaderCompilationThreadPool;
    };

    using InitResourcesFuncType = std::function<std::shared_ptr<const ShaderResourcesType>(const ShaderDesc&, IDataBlob*)>;

    ShaderD3DBase(IReferenceCounters*     pRefCounters,
                  RenderDeviceImplType*   pDevice,
                  const ShaderCreateInfo& ShaderCI,
                  const CreateInfo&       D3DShaderCI,
                  bool                    bIsDeviceInternal,
                  const ShaderVersion     ShaderModel,
                  InitResourcesFuncType   InitResources) :
        ShaderBase<EngineImplTraits>{
            pRefCounters,
            pDevice,
            ShaderCI.Desc,
            D3DShaderCI.DeviceInfo,
            D3DShaderCI.AdapterInfo,
            bIsDeviceInternal,
        }
    {
        this->m_Status.store(SHADER_STATUS_COMPILING);
        if (D3DShaderCI.pShaderCompilationThreadPool == nullptr || (ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_ASYNCHRONOUS) == 0 || ShaderCI.ByteCode != nullptr)
        {
            Initialize(ShaderCI, ShaderModel, D3DShaderCI.pDXCompiler, D3DShaderCI.ppCompilerOutput, InitResources);
        }
        else
        {
            this->m_AsyncInitializer = AsyncInitializer::Start(
                D3DShaderCI.pShaderCompilationThreadPool,
                [this,
                 ShaderCI = ShaderCreateInfoWrapper{ShaderCI, GetRawAllocator()},
                 ShaderModel,
                 pDXCompiler      = D3DShaderCI.pDXCompiler,
                 ppCompilerOutput = D3DShaderCI.ppCompilerOutput,
                 InitResources](Uint32 ThreadId) mutable //
                {
                    try
                    {
                        Initialize(ShaderCI, ShaderModel, pDXCompiler, ppCompilerOutput, InitResources);
                    }
                    catch (...)
                    {
                        this->m_Status.store(SHADER_STATUS_FAILED);
                    }
                    ShaderCI = ShaderCreateInfoWrapper{};
                });
        }
    }

    /// Implementation of IShader::GetResourceCount() method.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final
    {
        DEV_CHECK_ERR(!this->IsCompiling(), "Shader resources are not available until compilation is complete. Use GetStatus() to check the shader status.");
        return m_pShaderResources ? m_pShaderResources->GetTotalResources() : 0;
    }

    /// Implementation of IShader::GetResource() method.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final
    {
        DEV_CHECK_ERR(!this->IsCompiling(), "Shader resources are not available until compilation is complete. Use GetStatus() to check the shader status.");
        if (m_pShaderResources)
        {
            ResourceDesc = m_pShaderResources->GetHLSLShaderResourceDesc(Index);
        }
    }

    /// Implementation of IShader::GetConstantBufferDesc() method.
    virtual const ShaderCodeBufferDesc* DILIGENT_CALL_TYPE GetConstantBufferDesc(Uint32 Index) const override final
    {
        DEV_CHECK_ERR(!this->IsCompiling(), "Shader resources are not available until compilation is complete. Use GetStatus() to check the shader status.");
        return m_pShaderResources ?
            // Constant buffers always go first in the list of resources
            m_pShaderResources->GetConstantBufferDesc(Index) :
            nullptr;
    }

    /// Implementation of IShaderD3D::GetHLSLResource() method.
    virtual void DILIGENT_CALL_TYPE GetHLSLResource(Uint32 Index, HLSLShaderResourceDesc& ResourceDesc) const override final
    {
        DEV_CHECK_ERR(!this->IsCompiling(), "Shader resources are not available until compilation is complete. Use GetStatus() to check the shader status.");
        if (m_pShaderResources)
        {
            ResourceDesc = m_pShaderResources->GetHLSLShaderResourceDesc(Index);
        }
    }

    virtual void DILIGENT_CALL_TYPE GetBytecode(const void** ppBytecode,
                                                Uint64&      Size) const override final
    {
        DEV_CHECK_ERR(!this->IsCompiling(), "Shader resources are not available until compilation is complete. Use GetStatus() to check the shader status.");
        if (m_pShaderByteCode)
        {
            *ppBytecode = m_pShaderByteCode->GetConstDataPtr();
            Size        = m_pShaderByteCode->GetSize();
        }
        else
        {
            *ppBytecode = nullptr;
            Size        = 0;
        }
    }

    IDataBlob* GetD3DBytecode() const
    {
        // NOTE: while shader is compiled asynchronously, m_pShaderByteCode may be modified by
        //       another thread and thus can't be accessed.
        return !this->IsCompiling() ? m_pShaderByteCode.RawPtr() : nullptr;
    }

    const std::shared_ptr<const ShaderResourcesType>& GetShaderResources() const
    {
        DEV_CHECK_ERR(!this->IsCompiling(), "Shader resources are not available until compilation is complete. Use GetStatus() to check the shader status.");
        return m_pShaderResources;
    }

private:
    void Initialize(const ShaderCreateInfo& ShaderCI,
                    const ShaderVersion     ShaderModel,
                    IDXCompiler*            pDxCompiler,
                    IDataBlob**             ppCompilerOutput,
                    InitResourcesFuncType   InitResources) noexcept(false)
    {
        m_pShaderByteCode = CompileD3DBytecode(ShaderCI, ShaderModel, pDxCompiler, ppCompilerOutput);
        if ((ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_SKIP_REFLECTION) == 0)
        {
            m_pShaderResources = InitResources(this->m_Desc, m_pShaderByteCode);
        }
        this->m_Status.store(SHADER_STATUS_READY);
    }

protected:
    // AddRef/Release methods of ID3DBlob are not thread safe, so use Diligent::IDataBlob.
    RefCntAutoPtr<IDataBlob> m_pShaderByteCode;

    // ShaderResources class instance must be referenced through the shared pointer, because
    // it is referenced by PipelineStateD3DXXImpl class instances
    std::shared_ptr<const ShaderResourcesType> m_pShaderResources;
};

} // namespace Diligent
