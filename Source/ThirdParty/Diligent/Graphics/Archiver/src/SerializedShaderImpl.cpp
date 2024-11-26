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

#include "SerializedShaderImpl.hpp"

#include <cstring>

#include "SerializationDeviceImpl.hpp"
#include "EngineMemory.h"
#include "DataBlobImpl.hpp"
#include "PlatformMisc.hpp"
#include "BasicMath.hpp"
#include "PSOSerializer.hpp"

namespace Diligent
{

const INTERFACE_ID SerializedShaderImpl::IID_InternalImpl;

SerializedShaderImpl::SerializedShaderImpl(IReferenceCounters*      pRefCounters,
                                           SerializationDeviceImpl* pDevice,
                                           const ShaderCreateInfo&  ShaderCI,
                                           const ShaderArchiveInfo& ArchiveInfo,
                                           IDataBlob**              ppCompilerOutput) :
    TBase{pRefCounters},
    m_pDevice{pDevice}
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

    m_CreateInfo = ShaderCreateInfoWrapper{ShaderCI, GetRawAllocator()};

    if ((DeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_GL) != 0 && (DeviceFlags & ARCHIVE_DEVICE_DATA_FLAG_GLES) != 0)
    {
        // OpenGL and GLES use the same device data. Clear one flag to avoid shader duplication.
        DeviceFlags &= ~ARCHIVE_DEVICE_DATA_FLAG_GLES;
    }

    while (DeviceFlags != ARCHIVE_DEVICE_DATA_FLAG_NONE)
    {
        const ARCHIVE_DEVICE_DATA_FLAGS Flag = ExtractLSB(DeviceFlags);

        static_assert(ARCHIVE_DEVICE_DATA_FLAG_LAST == 1 << 7, "Please update the switch below to handle the new device data type");
        switch (Flag)
        {
#if D3D11_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_D3D11:
                CreateShaderD3D11(pRefCounters, ShaderCI, ppCompilerOutput);
                break;
#endif

#if D3D12_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_D3D12:
                CreateShaderD3D12(pRefCounters, ShaderCI, ppCompilerOutput);
                break;
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_GL:
            case ARCHIVE_DEVICE_DATA_FLAG_GLES:
                CreateShaderGL(pRefCounters, ShaderCI, Flag == ARCHIVE_DEVICE_DATA_FLAG_GL ? RENDER_DEVICE_TYPE_GL : RENDER_DEVICE_TYPE_GLES, ppCompilerOutput);
                break;
#endif

#if VULKAN_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_VULKAN:
                CreateShaderVk(pRefCounters, ShaderCI, ppCompilerOutput);
                break;
#endif

#if METAL_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS:
            case ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS:
                CreateShaderMtl(pRefCounters, ShaderCI, Flag == ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS ? DeviceType::Metal_MacOS : DeviceType::Metal_iOS, ppCompilerOutput);
                break;
#endif

#if WEBGPU_SUPPORTED
            case ARCHIVE_DEVICE_DATA_FLAG_WEBGPU:
                CreateShaderWebGPU(pRefCounters, ShaderCI, ppCompilerOutput);
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

SerializedShaderImpl::~SerializedShaderImpl()
{
    // Make sure that all asynchrous tasks are complete.
    GetStatus(/*WaitForCompletion = */ true);
}

void DILIGENT_CALL_TYPE SerializedShaderImpl::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;
    if (IID == IID_SerializedShader || IID == IID_Shader || IID == IID_InternalImpl)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
    else
    {
        TBase::QueryInterface(IID, ppInterface);
    }
}

bool SerializedShaderImpl::operator==(const SerializedShaderImpl& Rhs) const noexcept
{
    return m_CreateInfo.Get() == Rhs.m_CreateInfo.Get();
}

SerializedData SerializedShaderImpl::SerializeCreateInfo(const ShaderCreateInfo& CI)
{
    SerializedData ShaderData;

    {
        Serializer<SerializerMode::Measure> Ser;
        ShaderSerializer<SerializerMode::Measure>::SerializeCI(Ser, CI);
        ShaderData = Ser.AllocateData(GetRawAllocator());
    }

    {
        Serializer<SerializerMode::Write> Ser{ShaderData};
        ShaderSerializer<SerializerMode::Write>::SerializeCI(Ser, CI);
        VERIFY_EXPR(Ser.IsEnded());
    }

    return ShaderData;
}

SerializedData SerializedShaderImpl::GetDeviceData(DeviceType Type) const
{
    DEV_CHECK_ERR(!IsCompiling(), "Device data is not available until compilation is complete. Use GetStatus() to check the shader status.");

    const auto& pCompiledShader = m_Shaders[static_cast<size_t>(Type)];
    return pCompiledShader ?
        pCompiledShader->Serialize(GetCreateInfo()) :
        SerializedData{};
}

IShader* SerializedShaderImpl::GetDeviceShader(RENDER_DEVICE_TYPE Type) const
{
    const DeviceType ArchiveDeviceType = RenderDeviceTypeToArchiveDeviceType(Type);
    const auto&      pCompiledShader   = m_Shaders[static_cast<size_t>(ArchiveDeviceType)];
    return pCompiledShader ?
        pCompiledShader->GetDeviceShader() :
        nullptr;
}

SHADER_STATUS SerializedShaderImpl::GetStatus(bool WaitForCompletion)
{
    SHADER_STATUS OverallStatus = SHADER_STATUS_READY;
    for (size_t type = 0; type < static_cast<size_t>(DeviceType::Count); ++type)
    {
        const auto& pCompiledShader = m_Shaders[type];
        if (!pCompiledShader)
            continue;

        const SHADER_STATUS Status = pCompiledShader->GetStatus(WaitForCompletion);
        switch (Status)
        {
            case SHADER_STATUS_UNINITIALIZED:
                UNEXPECTED("Shader status must not be uninitialized");
                break;

            case SHADER_STATUS_COMPILING:
                OverallStatus = SHADER_STATUS_COMPILING;
                break;

            case SHADER_STATUS_READY:
                // Nothing to do
                break;

            case SHADER_STATUS_FAILED:
                return SHADER_STATUS_FAILED;

            default:
                UNEXPECTED("Unexpected shader status");
                break;
        }
    }

    return OverallStatus;
}

bool SerializedShaderImpl::IsCompiling() const
{
    for (const auto& pCompiledShader : m_Shaders)
    {
        if (pCompiledShader && pCompiledShader->IsCompiling())
            return true;
    }

    return false;
}

std::vector<RefCntAutoPtr<IAsyncTask>> SerializedShaderImpl::GetCompileTasks() const
{
    std::vector<RefCntAutoPtr<IAsyncTask>> Tasks;
    for (const auto& pCompiledShader : m_Shaders)
    {
        if (RefCntAutoPtr<IAsyncTask> pTask{pCompiledShader ? pCompiledShader->GetCompileTask() : RefCntAutoPtr<IAsyncTask>{}})
        {
            Tasks.emplace_back(std::move(pTask));
        }
    }
    return Tasks;
}

} // namespace Diligent
