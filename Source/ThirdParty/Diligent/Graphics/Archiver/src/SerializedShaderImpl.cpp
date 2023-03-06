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

#include "SerializedShaderImpl.hpp"
#include "SerializationDeviceImpl.hpp"
#include "FixedLinearAllocator.hpp"
#include "EngineMemory.h"
#include "DataBlobImpl.hpp"
#include "PlatformMisc.hpp"
#include "BasicMath.hpp"

namespace Diligent
{

SerializedShaderImpl::SerializedShaderImpl(IReferenceCounters*      pRefCounters,
                                           SerializationDeviceImpl* pDevice,
                                           const ShaderCreateInfo&  ShaderCI,
                                           const ShaderArchiveInfo& ArchiveInfo) :
    TBase{pRefCounters},
    m_pDevice{pDevice},
    m_pShaderSourceFactory{ShaderCI.pShaderSourceStreamFactory},
    m_CreateInfo{ShaderCI}
{
    if (ShaderCI.Desc.Name == nullptr || ShaderCI.Desc.Name[0] == '\0')
    {
        LOG_ERROR_AND_THROW("Serialized shader name must not be null or empty string");
    }

    auto DeviceFlags = ArchiveInfo.DeviceFlags;
    if ((DeviceFlags & m_pDevice->GetSupportedDeviceFlags()) != DeviceFlags)
    {
        LOG_ERROR_AND_THROW("DeviceFlags contain unsupported device type");
    }

    if (ShaderCI.CompileFlags & SHADER_COMPILE_FLAG_SKIP_REFLECTION)
    {
        LOG_ERROR_AND_THROW("Serialized shader must not contain SHADER_COMPILE_FLAG_SKIP_REFLECTION flag");
    }

    CopyShaderCreateInfo(ShaderCI);

    while (DeviceFlags != ARCHIVE_DEVICE_DATA_FLAG_NONE)
    {
        const auto Flag = ExtractLSB(DeviceFlags);

        static_assert(ARCHIVE_DEVICE_DATA_FLAG_LAST == ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS, "Please update the switch below to handle the new device data type");
        switch (Flag)
        {
#if D3D11_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_D3D11:
                CreateShaderD3D11(pRefCounters, ShaderCI);
                break;
#endif

#if D3D12_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_D3D12:
                CreateShaderD3D12(pRefCounters, ShaderCI);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_GL:
            case ARCHIVE_DEVICE_DATA_FLAG_GLES:
                CreateShaderGL(pRefCounters, ShaderCI, Flag == ARCHIVE_DEVICE_DATA_FLAG_GL ? RENDER_DEVICE_TYPE_GL : RENDER_DEVICE_TYPE_GLES);
                break;
#endif

#if VULKAN_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_VULKAN:
                CreateShaderVk(pRefCounters, ShaderCI);
                break;
#endif

#if METAL_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS:
            case ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS:
                CreateShaderMtl(ShaderCI, Flag == ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS ? DeviceType::Metal_MacOS : DeviceType::Metal_iOS);
                break;
#endif

            case ARCHIVE_DEVICE_DATA_FLAG_NONE:
                UNEXPECTED("ARCHIVE_DEVICE_DATA_FLAG_NONE(0) should never occur");
                break;

            default:
                LOG_ERROR_MESSAGE("Unexpected render device type");
                break;
        }
    }
}

void SerializedShaderImpl::CopyShaderCreateInfo(const ShaderCreateInfo& ShaderCI) noexcept(false)
{
    m_CreateInfo.ppCompilerOutput = nullptr;

    auto&                RawAllocator = GetRawAllocator();
    FixedLinearAllocator Allocator{RawAllocator};

    Allocator.AddSpaceForString(ShaderCI.EntryPoint);
    Allocator.AddSpaceForString(ShaderCI.CombinedSamplerSuffix);
    Allocator.AddSpaceForString(ShaderCI.Desc.Name);

    if (ShaderCI.ByteCode && ShaderCI.ByteCodeSize > 0)
    {
        Allocator.AddSpace(ShaderCI.ByteCodeSize, alignof(Uint32));
    }
    else if (ShaderCI.Source != nullptr)
    {
        Allocator.AddSpaceForString(ShaderCI.Source, ShaderCI.SourceLength);
    }
    else if (ShaderCI.FilePath != nullptr && ShaderCI.pShaderSourceStreamFactory != nullptr)
    {
        Allocator.AddSpaceForString(ShaderCI.FilePath);
    }
    else
    {
        LOG_ERROR_AND_THROW("Shader create info must contain Source, Bytecode or FilePath with pShaderSourceStreamFactory");
    }

    size_t MacroCount = 0;
    if (ShaderCI.Macros)
    {
        for (auto* Macro = ShaderCI.Macros; Macro->Name != nullptr && Macro->Definition != nullptr; ++Macro, ++MacroCount)
        {}
        Allocator.AddSpace<ShaderMacro>(MacroCount + 1);

        for (size_t i = 0; i < MacroCount; ++i)
        {
            Allocator.AddSpaceForString(ShaderCI.Macros[i].Name);
            Allocator.AddSpaceForString(ShaderCI.Macros[i].Definition);
        }
    }

    Allocator.Reserve();

    m_pRawMemory = decltype(m_pRawMemory){Allocator.ReleaseOwnership(), STDDeleterRawMem<void>{RawAllocator}};

    m_CreateInfo.EntryPoint            = Allocator.CopyString(ShaderCI.EntryPoint);
    m_CreateInfo.CombinedSamplerSuffix = Allocator.CopyString(ShaderCI.CombinedSamplerSuffix);
    m_CreateInfo.Desc.Name             = Allocator.CopyString(ShaderCI.Desc.Name);

    if (m_CreateInfo.Desc.Name == nullptr)
        m_CreateInfo.Desc.Name = "";

    if (ShaderCI.ByteCode && ShaderCI.ByteCodeSize > 0)
    {
        m_CreateInfo.ByteCode = Allocator.Copy(ShaderCI.ByteCode, ShaderCI.ByteCodeSize, alignof(Uint32));
    }
    else if (ShaderCI.Source != nullptr)
    {
        m_CreateInfo.Source       = Allocator.CopyString(ShaderCI.Source, ShaderCI.SourceLength);
        m_CreateInfo.SourceLength = ShaderCI.SourceLength;
    }
    else
    {
        VERIFY_EXPR(ShaderCI.FilePath != nullptr && ShaderCI.pShaderSourceStreamFactory != nullptr);
        m_CreateInfo.FilePath = Allocator.CopyString(ShaderCI.FilePath);
    }

    if (MacroCount > 0)
    {
        auto* pMacros       = Allocator.ConstructArray<ShaderMacro>(MacroCount + 1);
        m_CreateInfo.Macros = pMacros;
        for (auto* Macro = ShaderCI.Macros; Macro->Name != nullptr && Macro->Definition != nullptr; ++Macro, ++pMacros)
        {
            pMacros->Name       = Allocator.CopyString(Macro->Name);
            pMacros->Definition = Allocator.CopyString(Macro->Definition);
        }
        pMacros->Name       = nullptr;
        pMacros->Definition = nullptr;
    }
}

SerializedShaderImpl::~SerializedShaderImpl()
{}

void DILIGENT_CALL_TYPE SerializedShaderImpl::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;
    if (IID == IID_SerializedShader || IID == IID_Shader)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
    else
    {
        TBase::QueryInterface(IID, ppInterface);
    }
}

} // namespace Diligent
