/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <memory>
#include <array>

#include "Shader.h"
#include "SerializationEngineImplTraits.hpp"
#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "STDAllocator.hpp"
#include "Serializer.hpp"
#include "DeviceObjectArchiveBase.hpp"

namespace Diligent
{

// {53A9A017-6A34-4AE9-AA23-C8E587023F9E}
static const INTERFACE_ID IID_SerializedShader =
    {0x53a9a017, 0x6a34, 0x4ae9, {0xaa, 0x23, 0xc8, 0xe5, 0x87, 0x2, 0x3f, 0x9e}};

class SerializedShaderImpl final : public ObjectBase<IShader>
{
public:
    using TBase      = ObjectBase<IShader>;
    using DeviceType = DeviceObjectArchiveBase::DeviceType;

    SerializedShaderImpl(IReferenceCounters*      pRefCounters,
                         SerializationDeviceImpl* pDevice,
                         const ShaderCreateInfo&  ShaderCI,
                         const ShaderArchiveInfo& ArchiveInfo);
    ~SerializedShaderImpl();

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    virtual const ShaderDesc& DILIGENT_CALL_TYPE GetDesc() const override final { return m_CreateInfo.Desc; }

    UNSUPPORTED_CONST_METHOD(Uint32, GetResourceCount)
    UNSUPPORTED_CONST_METHOD(void, GetResourceDesc, Uint32 Index, ShaderResourceDesc& ResourceDesc)
    UNSUPPORTED_CONST_METHOD(Int32, GetUniqueID)
    UNSUPPORTED_METHOD(void, SetUserData, IObject* pUserData)
    UNSUPPORTED_CONST_METHOD(IObject*, GetUserData)

    struct CompiledShader
    {
        virtual ~CompiledShader() {}
    };

    template <typename CompiledShaderType>
    CompiledShaderType* GetShader(DeviceType Type) const
    {
        return static_cast<CompiledShaderType*>(m_Shaders[static_cast<size_t>(Type)].get());
    }

    const ShaderCreateInfo& GetCreateInfo() const
    {
        return m_CreateInfo;
    }

private:
    void CopyShaderCreateInfo(const ShaderCreateInfo& ShaderCI) noexcept(false);

    SerializationDeviceImpl*                       m_pDevice;
    RefCntAutoPtr<IShaderSourceInputStreamFactory> m_pShaderSourceFactory;
    ShaderCreateInfo                               m_CreateInfo;
    std::unique_ptr<void, STDDeleterRawMem<void>>  m_pRawMemory;

    std::array<std::unique_ptr<CompiledShader>, static_cast<size_t>(DeviceType::Count)> m_Shaders;

    template <typename ShaderType, typename... ArgTypes>
    void CreateShader(DeviceType              Type,
                      IReferenceCounters*     pRefCounters,
                      const ShaderCreateInfo& ShaderCI,
                      const ArgTypes&... Args) noexcept(false);

#if D3D11_SUPPORTED
    void CreateShaderD3D11(IReferenceCounters* pRefCounters, const ShaderCreateInfo& ShaderCI) noexcept(false);
#endif

#if D3D12_SUPPORTED
    void CreateShaderD3D12(IReferenceCounters* pRefCounters, const ShaderCreateInfo& ShaderCI) noexcept(false);
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
    void CreateShaderGL(IReferenceCounters* pRefCounters, const ShaderCreateInfo& ShaderCI, RENDER_DEVICE_TYPE DeviceType) noexcept(false);
#endif

#if VULKAN_SUPPORTED
    void CreateShaderVk(IReferenceCounters* pRefCounters, const ShaderCreateInfo& ShaderCI) noexcept(false);
#endif

#if METAL_SUPPORTED
    void CreateShaderMtl(const ShaderCreateInfo& ShaderCI, DeviceType Type) noexcept(false);
#endif
};

} // namespace Diligent
