/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include <array>

#include "SerializedShader.h"
#include "SerializationEngineImplTraits.hpp"
#include "ShaderBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "STDAllocator.hpp"
#include "Serializer.hpp"
#include "DeviceObjectArchive.hpp"

namespace Diligent
{

class SerializedShaderImpl final : public ObjectBase<ISerializedShader>
{
public:
    using TBase      = ObjectBase<ISerializedShader>;
    using DeviceType = DeviceObjectArchive::DeviceType;

    static constexpr INTERFACE_ID IID_InternalImpl =
        {0x949bcae1, 0xb92c, 0x4f31, {0x88, 0x13, 0xec, 0x83, 0xa7, 0xe3, 0x89, 0x3}};

    SerializedShaderImpl(IReferenceCounters*      pRefCounters,
                         SerializationDeviceImpl* pDevice,
                         const ShaderCreateInfo&  ShaderCI,
                         const ShaderArchiveInfo& ArchiveInfo,
                         IDataBlob**              ppCompilerOutput);
    ~SerializedShaderImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    virtual const ShaderDesc& DILIGENT_CALL_TYPE GetDesc() const override final { return m_CreateInfo.Get().Desc; }

    UNSUPPORTED_CONST_METHOD(Uint32, GetResourceCount)
    UNSUPPORTED_CONST_METHOD(void, GetResourceDesc, Uint32 Index, ShaderResourceDesc& ResourceDesc)
    UNSUPPORTED_CONST_METHOD(const ShaderCodeBufferDesc*, GetConstantBufferDesc, Uint32 Index)
    UNSUPPORTED_CONST_METHOD(Int32, GetUniqueID)
    UNSUPPORTED_METHOD(void, SetUserData, IObject* pUserData)
    UNSUPPORTED_CONST_METHOD(IObject*, GetUserData)
    UNSUPPORTED_CONST_METHOD(void, GetBytecode, const void** ppBytecode, Uint64& Size);

    virtual SHADER_STATUS DILIGENT_CALL_TYPE GetStatus(bool WaitForCompletion) override final;
    virtual IShader* DILIGENT_CALL_TYPE      GetDeviceShader(RENDER_DEVICE_TYPE Type) const override final;

    bool IsCompiling() const;

    struct CompiledShader
    {
        virtual ~CompiledShader() {}
        virtual SerializedData Serialize(ShaderCreateInfo ShaderCI) const = 0;

        virtual IShader* GetDeviceShader() = 0;

        virtual SHADER_STATUS GetStatus(bool WaitForCompletion)
        {
            IShader* pShader = GetDeviceShader();
            return pShader != nullptr ? pShader->GetStatus(WaitForCompletion) : SHADER_STATUS_UNINITIALIZED;
        }

        virtual bool IsCompiling() const = 0;

        virtual RefCntAutoPtr<IAsyncTask> GetCompileTask() const = 0;
    };

    template <typename CompiledShaderType>
    CompiledShaderType* GetShader(DeviceType Type) const
    {
        return static_cast<CompiledShaderType*>(m_Shaders[static_cast<size_t>(Type)].get());
    }

    SerializedData GetCommonData() const { return SerializedData{}; }
    SerializedData GetDeviceData(DeviceType Type) const;

    const ShaderCreateInfo& GetCreateInfo() const
    {
        return m_CreateInfo;
    }

    static SerializedData SerializeCreateInfo(const ShaderCreateInfo& CI);

    bool operator==(const SerializedShaderImpl& Rhs) const noexcept;
    bool operator!=(const SerializedShaderImpl& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }

    std::vector<RefCntAutoPtr<IAsyncTask>> GetCompileTasks() const;

private:
    SerializationDeviceImpl* m_pDevice;
    ShaderCreateInfoWrapper  m_CreateInfo;

    std::array<std::unique_ptr<CompiledShader>, static_cast<size_t>(DeviceType::Count)> m_Shaders;

    template <typename ShaderType, typename... ArgTypes>
    void CreateShader(DeviceType              Type,
                      IReferenceCounters*     pRefCounters,
                      const ShaderCreateInfo& ShaderCI,
                      const ArgTypes&... Args) noexcept(false);

#if D3D11_SUPPORTED
    void CreateShaderD3D11(IReferenceCounters*     pRefCounters,
                           const ShaderCreateInfo& ShaderCI,
                           IDataBlob**             ppCompilerOutput) noexcept(false);
#endif

#if D3D12_SUPPORTED
    void CreateShaderD3D12(IReferenceCounters*     pRefCounters,
                           const ShaderCreateInfo& ShaderCI,
                           IDataBlob**             ppCompilerOutput) noexcept(false);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
    void CreateShaderGL(IReferenceCounters*     pRefCounters,
                        const ShaderCreateInfo& ShaderCI,
                        RENDER_DEVICE_TYPE      DeviceType,
                        IDataBlob**             ppCompilerOutput) noexcept(false);
#endif

#if VULKAN_SUPPORTED
    void CreateShaderVk(IReferenceCounters*     pRefCounters,
                        const ShaderCreateInfo& ShaderCI,
                        IDataBlob**             ppCompilerOutput) noexcept(false);
#endif

#if METAL_SUPPORTED
    void CreateShaderMtl(IReferenceCounters*     pRefCounters,
                         const ShaderCreateInfo& ShaderCI,
                         DeviceType              Type,
                         IDataBlob**             ppCompilerOutput) noexcept(false);
#endif

#if WEBGPU_SUPPORTED
    void CreateShaderWebGPU(IReferenceCounters*     pRefCounters,
                            const ShaderCreateInfo& ShaderCI,
                            IDataBlob**             ppCompilerOutput) noexcept(false);
#endif
};

} // namespace Diligent
