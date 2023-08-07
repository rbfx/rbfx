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

/// \file
/// Implementation of the Diligent::DearchiverBase class

#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>

#include "Dearchiver.h"
#include "RenderDevice.h"
#include "Shader.h"

#include "ObjectBase.hpp"
#include "EngineMemory.h"
#include "RefCntAutoPtr.hpp"
#include "DeviceObjectArchive.hpp"
#include "DynamicLinearAllocator.hpp"

namespace Diligent
{

struct DearchiverCreateInfo;

/// Class implementing base functionality of the dearchiver
class DearchiverBase : public ObjectBase<IDearchiver>
{
public:
    using TObjectBase = ObjectBase<IDearchiver>;

    DearchiverBase(IReferenceCounters* pRefCounters, const DearchiverCreateInfo& CI) noexcept :
        TObjectBase{pRefCounters}
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_Dearchiver, TObjectBase)

    /// Implementation of IDearchiver::LoadArchive().
    virtual bool DILIGENT_CALL_TYPE LoadArchive(const IDataBlob* pArchiveData, Uint32 ContentVersion, bool MakeCopy) override final;

    /// Implementation of IDearchiver::UnpackShader().
    virtual void DILIGENT_CALL_TYPE UnpackShader(const ShaderUnpackInfo& UnpackInfo,
                                                 IShader**               ppShader) override final;

    /// Implementation of IDearchiver::UnpackPipelineState().
    virtual void DILIGENT_CALL_TYPE UnpackPipelineState(const PipelineStateUnpackInfo& DeArchiveInfo,
                                                        IPipelineState**               ppPSO) override final;

    /// Implementation of IDearchiver::UnpackResourceSignature().
    virtual void DILIGENT_CALL_TYPE UnpackResourceSignature(const ResourceSignatureUnpackInfo& DeArchiveInfo,
                                                            IPipelineResourceSignature**       ppSignature) override final;

    /// Implementation of IDearchiver::UnpackRenderPass().
    virtual void DILIGENT_CALL_TYPE UnpackRenderPass(const RenderPassUnpackInfo& DeArchiveInfo,
                                                     IRenderPass**               ppRP) override final;

    /// Implementation of IDearchiver::Store().
    virtual bool DILIGENT_CALL_TYPE Store(IDataBlob** ppArchive) const override final;

    /// Implementation of IDearchiver::Reset().
    virtual void DILIGENT_CALL_TYPE Reset() override final;

    /// Implementation of IDearchiver::GetContentVersion().
    virtual Uint32 DILIGENT_CALL_TYPE GetContentVersion() const override final;

protected:
    template <typename RenderDeviceImplType, typename PRSSerializerType>
    RefCntAutoPtr<IPipelineResourceSignature> UnpackResourceSignatureImpl(
        const ResourceSignatureUnpackInfo& DeArchiveInfo,
        bool                               IsImplicit);

    virtual RefCntAutoPtr<IPipelineResourceSignature> UnpackResourceSignature(const ResourceSignatureUnpackInfo& DeArchiveInfo,
                                                                              bool                               IsImplicit) = 0;

    virtual RefCntAutoPtr<IShader> UnpackShader(const ShaderCreateInfo& ShaderCI,
                                                IRenderDevice*          pDevice);

protected:
    using ResourceType         = DeviceObjectArchive::ResourceType;
    using DeviceType           = DeviceObjectArchive::DeviceType;
    using SerializedPSOAuxData = DeviceObjectArchive::SerializedPSOAuxData;
    using TPRSNames            = DeviceObjectArchive::TPRSNames;
    using ResourceKey          = DeviceObjectArchive::NamedResourceKey;

    template <typename ResType>
    class NamedResourceCache
    {
    public:
        NamedResourceCache() noexcept {};

        // clang-format off
        NamedResourceCache           (const NamedResourceCache&) = delete;
        NamedResourceCache& operator=(const NamedResourceCache&) = delete;
        NamedResourceCache           (NamedResourceCache&&)      = default;
        NamedResourceCache& operator=(NamedResourceCache&&)      = default;
        // clang-format on

        bool Get(ResourceType Type, const char* Name, ResType** ppResource);
        void Set(ResourceType Type, const char* Name, ResType* pResource);

        void Clear() { m_Map.clear(); }

    private:
        std::mutex m_Mtx;
        // Keep weak resource references in the cache
        std::unordered_map<ResourceKey, RefCntWeakPtr<ResType>, ResourceKey::Hasher> m_Map;
    };

    struct ResourceCache
    {
        NamedResourceCache<IPipelineResourceSignature> Sign;
        NamedResourceCache<IRenderPass>                RenderPass;
        NamedResourceCache<IPipelineState>             PSO;
    } m_Cache;

    struct PRSData
    {
        DynamicLinearAllocator        Allocator;
        PipelineResourceSignatureDesc Desc;

        static constexpr ResourceType ArchiveResType = ResourceType::ResourceSignature;

        explicit PRSData(IMemoryAllocator& Allocator, Uint32 BlockSize = 1 << 10) :
            Allocator{Allocator, BlockSize}
        {}

        bool Deserialize(const char* Name, Serializer<SerializerMode::Read>& Ser);
    };

    static DeviceType GetArchiveDeviceType(const IRenderDevice* pDevice);

private:
    template <typename CreateInfoType>
    struct PSOData;

    struct RPData;

    struct ShaderCacheData
    {
        std::mutex Mtx;

        std::vector<RefCntAutoPtr<IShader>> Shaders;

        ShaderCacheData() = default;
        ShaderCacheData(ShaderCacheData&& rhs) noexcept :
            Shaders{std::move(rhs.Shaders)}
        {}
        // clang-format off
        ShaderCacheData           (const ShaderCacheData&)  = delete;
        ShaderCacheData& operator=(const ShaderCacheData&)  = delete;
        ShaderCacheData& operator=(      ShaderCacheData&&) = delete;
        // clang-format on
    };

    struct ArchiveData
    {
        explicit ArchiveData(std::unique_ptr<DeviceObjectArchive>&& _pObjArchive) noexcept :
            pObjArchive{std::move(_pObjArchive)}
        {}
        // clang-format off
        ArchiveData           (const ArchiveData&)  = delete;
        ArchiveData           (      ArchiveData&&) = default;
        ArchiveData& operator=(const ArchiveData&)  = delete;
        ArchiveData& operator=(      ArchiveData&&) = delete;
        // clang-format on

        std::unique_ptr<const DeviceObjectArchive> pObjArchive;

        std::array<ShaderCacheData, static_cast<size_t>(DeviceType::Count)> CachedShaders;
    };

    template <typename CreateInfoType>
    bool UnpackPSOSignatures(PSOData<CreateInfoType>& PSO, IRenderDevice* pDevice);

    template <typename CreateInfoType>
    bool UnpackPSORenderPass(PSOData<CreateInfoType>& PSO, IRenderDevice* pDevice) { return true; }

    template <typename CreateInfoType>
    bool UnpackPSOShaders(ArchiveData&             Archive,
                          PSOData<CreateInfoType>& PSO,
                          IRenderDevice*           pDevice);

    template <typename CreateInfoType>
    void UnpackPipelineStateImpl(const PipelineStateUnpackInfo& UnpackInfo, IPipelineState** ppPSO);

    ArchiveData* FindArchive(ResourceType ResType, const char* ResName);

private:
    // Resource type and name -> archive index that contains this resource.
    // Names must be unique for each resource type.
    using NamedResourceKey = DeviceObjectArchive::NamedResourceKey;
    std::unordered_map<NamedResourceKey, size_t, NamedResourceKey::Hasher> m_ResNameToArchiveIdx;

    std::vector<ArchiveData> m_Archives;
};


template <typename RenderDeviceImplType, typename PRSSerializerType>
RefCntAutoPtr<IPipelineResourceSignature> DearchiverBase::UnpackResourceSignatureImpl(
    const ResourceSignatureUnpackInfo& DeArchiveInfo,
    bool                               IsImplicit)
{
    RefCntAutoPtr<IPipelineResourceSignature> pSignature;
    // Do not reuse implicit signatures
    if (!IsImplicit)
    {
        // Since signature names must be unique, we use a single cache for all
        // loaded archives.
        if (m_Cache.Sign.Get(PRSData::ArchiveResType, DeArchiveInfo.Name, pSignature.RawDblPtr()))
            return pSignature;
    }

    // Find the archive that contains this signature
    auto archive_idx_it = m_ResNameToArchiveIdx.find(NamedResourceKey{PRSData::ArchiveResType, DeArchiveInfo.Name});
    if (archive_idx_it == m_ResNameToArchiveIdx.end())
        return {};

    const auto& pObjArchive = m_Archives[archive_idx_it->second].pObjArchive;
    if (!pObjArchive)
    {
        UNEXPECTED("Null object archives should never be added to the list. This is a bug.");
        return {};
    }

    PRSData PRS{GetRawAllocator()};
    if (!pObjArchive->LoadResourceCommonData(PRSData::ArchiveResType, DeArchiveInfo.Name, PRS))
        return {};

    PRS.Desc.SRBAllocationGranularity = DeArchiveInfo.SRBAllocationGranularity;

    const auto  DevType = GetArchiveDeviceType(DeArchiveInfo.pDevice);
    const auto& Data    = pObjArchive->GetDeviceSpecificData(PRSData::ArchiveResType, DeArchiveInfo.Name, DevType);
    if (!Data)
        return {};

    Serializer<SerializerMode::Read> Ser{Data};

    bool SpecialDesc = false;
    if (!Ser(SpecialDesc))
    {
        LOG_ERROR_MESSAGE("Failed to deserialize SpecialDesc flag. Archive file may be corrupted or invalid.");
        return {};
    }

    if (SpecialDesc)
    {
        // The signature uses a special description that differs from the common
        const auto* Name = PRS.Desc.Name;
        PRS.Desc         = {};
        if (!PRS.Deserialize(Name, Ser))
        {
            LOG_ERROR_MESSAGE("Failed to deserialize PRS description. Archive file may be corrupted or invalid.");
            return {};
        }
    }

    typename PRSSerializerType::InternalDataType InternalData;
    if (!PRSSerializerType::SerializeInternalData(Ser, InternalData, &PRS.Allocator))
    {
        LOG_ERROR_MESSAGE("Failed to deserialize PRS internal data. Archive file may be corrupted or invalid.");
        return {};
    }
    VERIFY_EXPR(Ser.IsEnded());

    auto* pRenderDevice = ClassPtrCast<RenderDeviceImplType>(DeArchiveInfo.pDevice);
    pRenderDevice->CreatePipelineResourceSignature(PRS.Desc, InternalData, &pSignature);

    if (!IsImplicit)
        m_Cache.Sign.Set(PRSData::ArchiveResType, DeArchiveInfo.Name, pSignature.RawPtr());

    return pSignature;
}

} // namespace Diligent
