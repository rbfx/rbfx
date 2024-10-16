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

#include "EngineGLImplTraits.hpp"
#include "ShaderBase.hpp"
#include "GLObjectWrapper.hpp"
#include "ShaderResourcesGL.hpp"

namespace Diligent
{

/// Shader object implementation in OpenGL backend.
class ShaderGLImpl final : public ShaderBase<EngineGLImplTraits>
{
public:
    using TShaderBase = ShaderBase<EngineGLImplTraits>;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0xa62b7e6a, 0x566b, 0x4c8d, {0xbd, 0xe0, 0x2f, 0x63, 0xcf, 0xca, 0x78, 0xc8}};

    struct CreateInfo
    {
        const RenderDeviceInfo&    DeviceInfo;
        const GraphicsAdapterInfo& AdapterInfo;
        IDataBlob** const          ppCompilerOutput;
    };

    ShaderGLImpl(IReferenceCounters*     pRefCounters,
                 RenderDeviceGLImpl*     pDeviceGL,
                 const ShaderCreateInfo& ShaderCI,
                 const CreateInfo&       GLShaderCI,
                 bool                    bIsDeviceInternal = false) noexcept(false);
    ~ShaderGLImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    /// Implementation of IShader::GetResourceCount() in OpenGL backend.
    virtual Uint32 DILIGENT_CALL_TYPE GetResourceCount() const override final;

    /// Implementation of IShader::GetResource() in OpenGL backend.
    virtual void DILIGENT_CALL_TYPE GetResourceDesc(Uint32 Index, ShaderResourceDesc& ResourceDesc) const override final;

    /// Implementation of IShader::GetConstantBufferDesc() in OpenGL backend.
    virtual const ShaderCodeBufferDesc* DILIGENT_CALL_TYPE GetConstantBufferDesc(Uint32 Index) const override final;

    /// Implementation of IShaderGL::GetGLShaderHandle() in OpenGL backend.
    virtual GLuint DILIGENT_CALL_TYPE GetGLShaderHandle() const override final { return m_GLShaderObj; }

    const std::shared_ptr<const ShaderResourcesGL>& GetShaderResources() const { return m_pShaderResources; }

    SHADER_SOURCE_LANGUAGE GetSourceLanguage() const { return m_SourceLanguage; }

    virtual void DILIGENT_CALL_TYPE GetBytecode(const void** ppData,
                                                Uint64&      DataSize) const override final
    {
        *ppData  = !m_GLSLSourceString.empty() ? m_GLSLSourceString.c_str() : nullptr;
        DataSize = m_GLSLSourceString.length();
    }

    virtual SHADER_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion) override final;

private:
    void CompileShader() noexcept;
    bool GetCompileStatus(IDataBlob** ppCompilerOutput, bool ThrowOnError) noexcept(false);

private:
    SHADER_SOURCE_LANGUAGE                   m_SourceLanguage = SHADER_SOURCE_LANGUAGE_DEFAULT;
    std::string                              m_GLSLSourceString;
    GLObjectWrappers::GLShaderObj            m_GLShaderObj;
    std::shared_ptr<const ShaderResourcesGL> m_pShaderResources;

    class ShaderBuilder;
    std::unique_ptr<ShaderBuilder> m_Builder;
};

} // namespace Diligent
