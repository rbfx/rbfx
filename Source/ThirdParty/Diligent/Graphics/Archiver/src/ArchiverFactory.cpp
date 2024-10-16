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

#include "ArchiverFactory.h"
#include "ArchiverFactoryLoader.h"
#include "DefaultShaderSourceStreamFactory.h"

#include "DummyReferenceCounters.hpp"
#include "ArchiverImpl.hpp"
#include "SerializationDeviceImpl.hpp"
#include "EngineMemory.h"
#include "PlatformDebug.hpp"

namespace Diligent
{

DeviceObjectArchive::DeviceType ArchiveDeviceDataFlagToArchiveDeviceType(ARCHIVE_DEVICE_DATA_FLAGS DeviceFlag)
{
    using DeviceType = DeviceObjectArchive::DeviceType;
    VERIFY(IsPowerOfTwo(DeviceFlag), "Only single flag is expected");
    static_assert(ARCHIVE_DEVICE_DATA_FLAG_LAST == 1 << 7, "Please handle the new data type below");
    switch (DeviceFlag)
    {
        case ARCHIVE_DEVICE_DATA_FLAG_NONE:
            UNEXPECTED("Archive data type is undefined");
            return DeviceType::Count;

        case ARCHIVE_DEVICE_DATA_FLAG_D3D11:
            return DeviceType::Direct3D11;

        case ARCHIVE_DEVICE_DATA_FLAG_D3D12:
            return DeviceType::Direct3D12;

        case ARCHIVE_DEVICE_DATA_FLAG_GL:
        case ARCHIVE_DEVICE_DATA_FLAG_GLES:
            return DeviceType::OpenGL;

        case ARCHIVE_DEVICE_DATA_FLAG_VULKAN:
            return DeviceType::Vulkan;

        case ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS:
            return DeviceType::Metal_MacOS;

        case ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS:
            return DeviceType::Metal_iOS;

        case ARCHIVE_DEVICE_DATA_FLAG_WEBGPU:
            return DeviceType::WebGPU;

        default:
            UNEXPECTED("Unexpected data type");
            return DeviceType::Count;
    }
}

namespace
{

class ArchiverFactoryImpl final : public IArchiverFactory
{
public:
    static ArchiverFactoryImpl* GetInstance()
    {
        static ArchiverFactoryImpl TheFactory;
        return &TheFactory;
    }

    ArchiverFactoryImpl() :
        m_RefCounters{*this}
    {}

    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override final;

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE AddRef() override final
    {
        return m_RefCounters.AddStrongRef();
    }

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        return m_RefCounters.ReleaseStrongRef();
    }

    virtual IReferenceCounters* DILIGENT_CALL_TYPE GetReferenceCounters() const override final
    {
        return const_cast<IReferenceCounters*>(static_cast<const IReferenceCounters*>(&m_RefCounters));
    }

    virtual void DILIGENT_CALL_TYPE CreateArchiver(
        ISerializationDevice* pDevice,
        IArchiver**           ppArchiver) override final;

    virtual void DILIGENT_CALL_TYPE CreateSerializationDevice(
        const SerializationDeviceCreateInfo& CreateInfo,
        ISerializationDevice**               ppDevice) override final;

    virtual void DILIGENT_CALL_TYPE CreateDefaultShaderSourceStreamFactory(
        const Char*                              SearchDirectories,
        struct IShaderSourceInputStreamFactory** ppShaderSourceFactory) const override final;

    virtual Bool DILIGENT_CALL_TYPE RemoveDeviceData(
        const IDataBlob*          pSrcArchive,
        ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags,
        IDataBlob**               ppDstArchive) const override final;

    virtual Bool DILIGENT_CALL_TYPE AppendDeviceData(
        const IDataBlob*          pSrcArchive,
        ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags,
        const IDataBlob*          pDeviceArchive,
        IDataBlob**               ppDstArchive) const override final;

    virtual Bool DILIGENT_CALL_TYPE MergeArchives(
        const IDataBlob* ppSrcArchives[],
        Uint32           NumSrcArchives,
        IDataBlob**      ppDstArchive) const override final;

    virtual Bool DILIGENT_CALL_TYPE PrintArchiveContent(const IDataBlob* pArchive) const override final;

    virtual void DILIGENT_CALL_TYPE SetMessageCallback(DebugMessageCallbackType MessageCallback) const override final;

    virtual void DILIGENT_CALL_TYPE SetBreakOnError(bool BreakOnError) const override final;

private:
    DummyReferenceCounters<ArchiverFactoryImpl> m_RefCounters;
};


void ArchiverFactoryImpl::QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface)
{
    if (ppInterface == nullptr)
        return;

    *ppInterface = nullptr;
    if (IID == IID_Unknown || IID == IID_ArchiverFactory)
    {
        *ppInterface = this;
        (*ppInterface)->AddRef();
    }
}

void ArchiverFactoryImpl::CreateArchiver(ISerializationDevice* pDevice, IArchiver** ppArchiver)
{
    DEV_CHECK_ERR(ppArchiver != nullptr, "ppArchiver must not be null");
    if (!ppArchiver)
        return;

    *ppArchiver = nullptr;
    try
    {
        auto& RawMemAllocator = GetRawAllocator();
        auto* pArchiverImpl(NEW_RC_OBJ(RawMemAllocator, "Archiver instance", ArchiverImpl)(ClassPtrCast<SerializationDeviceImpl>(pDevice)));
        pArchiverImpl->QueryInterface(IID_Archiver, reinterpret_cast<IObject**>(ppArchiver));
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to create the archiver");
    }
}

void ArchiverFactoryImpl::CreateSerializationDevice(const SerializationDeviceCreateInfo& CreateInfo, ISerializationDevice** ppDevice)
{
    DEV_CHECK_ERR(ppDevice != nullptr, "ppDevice must not be null");
    if (!ppDevice)
        return;

    *ppDevice = nullptr;
    try
    {
        auto& RawMemAllocator = GetRawAllocator();
        auto* pDeviceImpl(NEW_RC_OBJ(RawMemAllocator, "Serialization device instance", SerializationDeviceImpl)(CreateInfo));
        pDeviceImpl->QueryInterface(IID_SerializationDevice, reinterpret_cast<IObject**>(ppDevice));
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to create the serialization device");
    }
}

void ArchiverFactoryImpl::CreateDefaultShaderSourceStreamFactory(const Char* SearchDirectories, struct IShaderSourceInputStreamFactory** ppShaderSourceFactory) const
{
    if (ppShaderSourceFactory == nullptr)
    {
        DEV_ERROR("ppShaderSourceFactory must not be null.");
        return;
    }
    DEV_CHECK_ERR(*ppShaderSourceFactory == nullptr, "*ppShaderSourceFactory is not null. Make sure the pointer is null to avoid memory leaks.");

    Diligent::CreateDefaultShaderSourceStreamFactory(SearchDirectories, ppShaderSourceFactory);
}

Bool ArchiverFactoryImpl::RemoveDeviceData(const IDataBlob*          pSrcArchive,
                                           ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags,
                                           IDataBlob**               ppDstArchive) const
{
    if (pSrcArchive == nullptr)
    {
        DEV_ERROR("pSrcArchive must not be null");
        return false;
    }
    if (ppDstArchive == nullptr)
    {
        DEV_ERROR("ppDstArchive must not be null");
        return false;
    }
    DEV_CHECK_ERR(*ppDstArchive == nullptr, "*ppDstArchive must be null");

    try
    {
        DeviceObjectArchive ObjectArchive{DeviceObjectArchive::CreateInfo{pSrcArchive}};

        while (DeviceFlags != ARCHIVE_DEVICE_DATA_FLAG_NONE)
        {
            const auto DataTypeFlag      = ExtractLSB(DeviceFlags);
            const auto ArchiveDeviceType = ArchiveDeviceDataFlagToArchiveDeviceType(DataTypeFlag);

            ObjectArchive.RemoveDeviceData(ArchiveDeviceType);
        }

        ObjectArchive.Serialize(ppDstArchive);
        return *ppDstArchive != nullptr;
    }
    catch (...)
    {
        return false;
    }
}

Bool ArchiverFactoryImpl::AppendDeviceData(const IDataBlob*          pSrcArchive,
                                           ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags,
                                           const IDataBlob*          pDeviceArchive,
                                           IDataBlob**               ppDstArchive) const
{
    if (pSrcArchive == nullptr)
    {
        DEV_ERROR("pSrcArchive must not be null");
        return false;
    }
    if (pDeviceArchive == nullptr)
    {
        DEV_ERROR("pDeviceArchive must not be null");
        return false;
    }
    if (ppDstArchive == nullptr)
    {
        DEV_ERROR("ppDstArchive must not be null");
        return false;
    }
    DEV_CHECK_ERR(*ppDstArchive == nullptr, "*ppDstArchive must be null");

    try
    {
        DeviceObjectArchive       ObjectArchive{DeviceObjectArchive::CreateInfo{pSrcArchive}};
        const DeviceObjectArchive DevObjectArchive{DeviceObjectArchive::CreateInfo{pDeviceArchive}};

        while (DeviceFlags != ARCHIVE_DEVICE_DATA_FLAG_NONE)
        {
            const auto DataTypeFlag      = ExtractLSB(DeviceFlags);
            const auto ArchiveDeviceType = ArchiveDeviceDataFlagToArchiveDeviceType(DataTypeFlag);

            ObjectArchive.AppendDeviceData(DevObjectArchive, ArchiveDeviceType);
        }

        ObjectArchive.Serialize(ppDstArchive);
        return *ppDstArchive != nullptr;
    }
    catch (...)
    {
        return false;
    }
}

Bool ArchiverFactoryImpl::MergeArchives(
    const IDataBlob* ppSrcArchives[],
    Uint32           NumSrcArchives,
    IDataBlob**      ppDstArchive) const
{
    if (NumSrcArchives == 0)
        return false;

    DEV_CHECK_ERR(ppSrcArchives != nullptr, "ppSrcArchives must not be null");
    DEV_CHECK_ERR(ppDstArchive != nullptr, "ppDstArchive must not be null");

    if (ppSrcArchives == nullptr || ppDstArchive == nullptr)
        return false;

    try
    {
        DeviceObjectArchive MergedArchive{DeviceObjectArchive::CreateInfo{ppSrcArchives[0]}};
        for (Uint32 i = 1; i < NumSrcArchives; ++i)
        {
            const DeviceObjectArchive Archive{DeviceObjectArchive::CreateInfo{ppSrcArchives[i]}};
            MergedArchive.Merge(Archive);
        }

        MergedArchive.Serialize(ppDstArchive);
        return *ppDstArchive != nullptr;
    }
    catch (...)
    {
        return false;
    }
}

Bool ArchiverFactoryImpl::PrintArchiveContent(const IDataBlob* pArchive) const
{
    try
    {
        DeviceObjectArchive ObjArchive{DeviceObjectArchive::CreateInfo{pArchive}};

        LOG_INFO_MESSAGE(ObjArchive.ToString());
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void ArchiverFactoryImpl::SetMessageCallback(DebugMessageCallbackType MessageCallback) const
{
    SetDebugMessageCallback(MessageCallback);
}

void ArchiverFactoryImpl::SetBreakOnError(bool BreakOnError) const
{
    PlatformDebug::SetBreakOnError(BreakOnError);
}

} // namespace


API_QUALIFIER
IArchiverFactory* GetArchiverFactory()
{
    return ArchiverFactoryImpl::GetInstance();
}

} // namespace Diligent

extern "C"
{
    API_QUALIFIER
    Diligent::IArchiverFactory* Diligent_GetArchiverFactory()
    {
        return Diligent::GetArchiverFactory();
    }
}
