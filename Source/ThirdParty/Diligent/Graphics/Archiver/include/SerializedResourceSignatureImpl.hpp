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

#include <atomic>
#include <array>
#include <memory>
#include <string>

#include "PipelineResourceSignature.h"
#include "ObjectBase.hpp"
#include "STDAllocator.hpp"
#include "Serializer.hpp"
#include "DeviceObjectArchive.hpp"
#include "SerializationEngineImplTraits.hpp"

namespace Diligent
{

// {A4AC2D45-50FF-44EE-A218-5388CA6BF432}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SerializedResourceSignature =
    {0xa4ac2d45, 0x50ff, 0x44ee, {0xa2, 0x18, 0x53, 0x88, 0xca, 0x6b, 0xf4, 0x32}};

class SerializedResourceSignatureImpl final : public ObjectBase<IPipelineResourceSignature>
{
public:
    using TBase = ObjectBase<IPipelineResourceSignature>;

    using DeviceType                    = DeviceObjectArchive::DeviceType;
    static constexpr Uint32 DeviceCount = static_cast<Uint32>(DeviceType::Count);

    SerializedResourceSignatureImpl(IReferenceCounters*                  pRefCounters,
                                    SerializationDeviceImpl*             pDevice,
                                    const PipelineResourceSignatureDesc& Desc,
                                    const ResourceSignatureArchiveInfo&  ArchiveInfo,
                                    SHADER_TYPE                          ShaderStages = SHADER_TYPE_UNKNOWN);

    SerializedResourceSignatureImpl(IReferenceCounters* pRefCounters, const char* Name) noexcept;

    ~SerializedResourceSignatureImpl() override;

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_SerializedResourceSignature, IID_PipelineResourceSignature, TBase)

    virtual const PipelineResourceSignatureDesc& DILIGENT_CALL_TYPE GetDesc() const override final;

    // clang-format off
    UNSUPPORTED_METHOD      (void, CreateShaderResourceBinding, IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
    UNSUPPORTED_METHOD      (void, BindStaticResources,         SHADER_TYPE ShaderStages, IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
    UNSUPPORTED_METHOD      (IShaderResourceVariable*, GetStaticVariableByName, SHADER_TYPE ShaderType, const Char* Name)
    UNSUPPORTED_METHOD      (IShaderResourceVariable*, GetStaticVariableByIndex, SHADER_TYPE ShaderType, Uint32 Index)
    UNSUPPORTED_CONST_METHOD(Uint32,   GetStaticVariableCount,       SHADER_TYPE ShaderType)
    UNSUPPORTED_CONST_METHOD(void,     InitializeStaticSRBResources, IShaderResourceBinding* pShaderResourceBinding)
    UNSUPPORTED_CONST_METHOD(void,     CopyStaticResources,          IPipelineResourceSignature* pPRS)
    UNSUPPORTED_CONST_METHOD(bool,     IsCompatibleWith,             const IPipelineResourceSignature* pPRS)
    UNSUPPORTED_CONST_METHOD(Int32,    GetUniqueID)
    UNSUPPORTED_METHOD      (void,     SetUserData, IObject* pUserData)
    UNSUPPORTED_CONST_METHOD(IObject*, GetUserData)
    // clang-format on

    bool IsCompatible(const SerializedResourceSignatureImpl& Rhs, ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags) const;
    bool operator==(const SerializedResourceSignatureImpl& Rhs) const noexcept;
    bool operator!=(const SerializedResourceSignatureImpl& Rhs) const noexcept
    {
        return !(*this == Rhs);
    }
    size_t CalcHash() const;

    const SerializedData& GetCommonData() const { return m_CommonData; }

    const SerializedData* GetDeviceData(DeviceType Type) const
    {
        VERIFY_EXPR(static_cast<Uint32>(Type) < DeviceCount);
        const auto& Wrpr = m_pDeviceSignatures[static_cast<size_t>(Type)];
        return Wrpr ? &Wrpr->Data : nullptr;
    }

    template <typename SignatureType>
    struct SignatureTraits;

    template <typename SignatureType>
    SignatureType* GetDeviceSignature(DeviceType Type) const
    {
        constexpr auto TraitsType = SignatureTraits<SignatureType>::Type;
        VERIFY_EXPR(Type == TraitsType || (Type == DeviceType::Metal_iOS && TraitsType == DeviceType::Metal_MacOS));

        return ClassPtrCast<SignatureType>(GetDeviceSignature(Type));
    }

    const char* GetName() const { return m_Name.c_str(); }

    template <typename SignatureImplType>
    void CreateDeviceSignature(DeviceType                           Type,
                               const PipelineResourceSignatureDesc& Desc,
                               SHADER_TYPE                          ShaderStages);

private:
    IPipelineResourceSignature* GetDeviceSignature(DeviceType Type) const
    {
        VERIFY_EXPR(static_cast<Uint32>(Type) < DeviceCount);
        const auto& Wrpr = m_pDeviceSignatures[static_cast<size_t>(Type)];
        return Wrpr ? Wrpr->GetPRS() : nullptr;
    }

    void InitCommonData(const PipelineResourceSignatureDesc& Desc);

private:
    const std::string m_Name;

    const PipelineResourceSignatureDesc* m_pDesc = nullptr;

    SerializedData m_CommonData;

    struct PRSWapperBase
    {
        virtual ~PRSWapperBase() {}
        virtual IPipelineResourceSignature* GetPRS() = 0;

        SerializedData Data;
    };

    template <typename ImplType> struct TPRS;

    std::array<std::unique_ptr<PRSWapperBase>, DeviceCount> m_pDeviceSignatures;

    mutable std::atomic<size_t> m_Hash{0};
};

#define INSTANTIATE_GET_DEVICE_SIGNATURE(PRSImplType) template PRSImplType* SerializedResourceSignatureImpl::GetDeviceSignature<PRSImplType>(DeviceType Type) const
#define DECLARE_GET_DEVICE_SIGNATURE(PRSImplType)     extern INSTANTIATE_GET_DEVICE_SIGNATURE(PRSImplType)

#define INSTANTIATE_CREATE_DEVICE_SIGNATURE(PRSImplType)                               \
    template void SerializedResourceSignatureImpl::CreateDeviceSignature<PRSImplType>( \
        DeviceType                           Type,                                     \
        const PipelineResourceSignatureDesc& Desc,                                     \
        SHADER_TYPE                          ShaderStages)
#define DECLARE_CREATE_DEVICE_SIGNATURE(PRSImplType) extern INSTANTIATE_CREATE_DEVICE_SIGNATURE(PRSImplType)

#define DECLARE_DEVICE_SIGNATURE_METHODS(PRSImplType) \
    class PRSImplType;                                \
    DECLARE_GET_DEVICE_SIGNATURE(PRSImplType);        \
    DECLARE_CREATE_DEVICE_SIGNATURE(PRSImplType);

#define INSTANTIATE_DEVICE_SIGNATURE_METHODS(PRSImplType) \
    INSTANTIATE_GET_DEVICE_SIGNATURE(PRSImplType);        \
    INSTANTIATE_CREATE_DEVICE_SIGNATURE(PRSImplType);

#if D3D11_SUPPORTED
DECLARE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureD3D11Impl)
#endif

#if D3D12_SUPPORTED
DECLARE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureD3D12Impl)
#endif

#if GL_SUPPORTED || GLES_SUPPORTED
DECLARE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureGLImpl)
#endif

#if VULKAN_SUPPORTED
DECLARE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureVkImpl)
#endif

#if METAL_SUPPORTED
DECLARE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureMtlImpl)
#endif

#if WEBGPU_SUPPORTED
DECLARE_DEVICE_SIGNATURE_METHODS(PipelineResourceSignatureWebGPUImpl)
#endif

} // namespace Diligent
