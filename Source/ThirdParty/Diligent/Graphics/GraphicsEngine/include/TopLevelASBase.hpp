/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Implementation of the Diligent::TopLevelASBase template class

#include <unordered_map>
#include <atomic>

#include "TopLevelAS.h"
#include "BottomLevelASBase.hpp"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"
#include "StringPool.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

/// Validates top-level AS description and throws an exception in case of an error.
void ValidateTopLevelASDesc(const TopLevelASDesc& Desc) noexcept(false);

/// Template class implementing base functionality of the top-level acceleration structure object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class TopLevelASBase : public DeviceObjectBase<typename EngineImplTraits::TopLevelASInterface, typename EngineImplTraits::RenderDeviceImplType, TopLevelASDesc>
{
private:
    // Base interface that this class inherits (ITopLevelASD3D12, ITopLevelASVk, etc.).
    using BaseInterface = typename EngineImplTraits::TopLevelASInterface;

    // Render device implementation type (RenderDeviceD3D12Impl, RenderDeviceVkImpl, etc.).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    // Bottom-level AS implementation type (BottomLevelASD3D12Impl, BottomLevelASVkImpl, etc.).
    using BottomLevelASImplType = typename EngineImplTraits::BottomLevelASImplType;

    struct InstanceDesc
    {
        Uint32                               ContributionToHitGroupIndex = 0;
        Uint32                               InstanceIndex               = 0;
        RefCntAutoPtr<BottomLevelASImplType> pBLAS;
#ifdef DILIGENT_DEVELOPMENT
        Uint32 dvpVersion = 0;
#endif
    };

public:
    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, TopLevelASDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this TLAS.
    /// \param pDevice           - Pointer to the device.
    /// \param Desc              - TLAS description.
    /// \param bIsDeviceInternal - Flag indicating if the TLAS is an internal device object and
    ///							   must not keep a strong reference to the device.
    TopLevelASBase(IReferenceCounters*   pRefCounters,
                   RenderDeviceImplType* pDevice,
                   const TopLevelASDesc& Desc,
                   bool                  bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {
        if (!this->GetDevice()->GetFeatures().RayTracing)
            LOG_ERROR_AND_THROW("Ray tracing is not supported by device");

        ValidateTopLevelASDesc(this->m_Desc);
    }

    ~TopLevelASBase()
    {
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_TopLevelAS, TDeviceObjectBase)

    bool SetInstanceData(const TLASBuildInstanceData* pInstances,
                         const Uint32                 InstanceCount,
                         const Uint32                 BaseContributionToHitGroupIndex,
                         const Uint32                 HitGroupStride,
                         const HIT_GROUP_BINDING_MODE BindingMode) noexcept
    {
        try
        {
            ClearInstanceData();

            size_t StringPoolSize = 0;
            for (Uint32 i = 0; i < InstanceCount; ++i)
            {
                VERIFY_EXPR(pInstances[i].InstanceName != nullptr);
                StringPoolSize += StringPool::GetRequiredReserveSize(pInstances[i].InstanceName);
            }

            this->m_StringPool.Reserve(StringPoolSize, GetRawAllocator());

            Uint32 InstanceOffset = BaseContributionToHitGroupIndex;

            for (Uint32 i = 0; i < InstanceCount; ++i)
            {
                const auto&  Inst     = pInstances[i];
                const char*  NameCopy = this->m_StringPool.CopyString(Inst.InstanceName);
                InstanceDesc Desc     = {};

                Desc.pBLAS                       = ClassPtrCast<BottomLevelASImplType>(Inst.pBLAS);
                Desc.ContributionToHitGroupIndex = Inst.ContributionToHitGroupIndex;
                Desc.InstanceIndex               = i;
                CalculateHitGroupIndex(Desc, InstanceOffset, HitGroupStride, BindingMode);

#ifdef DILIGENT_DEVELOPMENT
                Desc.dvpVersion = Desc.pBLAS->DvpGetVersion();
#endif
                bool IsUniqueName = this->m_Instances.emplace(NameCopy, Desc).second;
                if (!IsUniqueName)
                    LOG_ERROR_AND_THROW("Instance name must be unique!");
            }

            VERIFY_EXPR(this->m_StringPool.GetRemainingSize() == 0);

            InstanceOffset = InstanceOffset + (BindingMode == HIT_GROUP_BINDING_MODE_PER_TLAS ? HitGroupStride : 0) - 1;

            this->m_BuildInfo.HitGroupStride                   = HitGroupStride;
            this->m_BuildInfo.FirstContributionToHitGroupIndex = BaseContributionToHitGroupIndex;
            this->m_BuildInfo.LastContributionToHitGroupIndex  = InstanceOffset;
            this->m_BuildInfo.BindingMode                      = BindingMode;
            this->m_BuildInfo.InstanceCount                    = InstanceCount;

#ifdef DILIGENT_DEVELOPMENT
            this->m_DvpVersion.fetch_add(1);
#endif
            return true;
        }
        catch (...)
        {
#ifdef DILIGENT_DEVELOPMENT
            this->m_DvpVersion.fetch_add(1);
#endif
            ClearInstanceData();
            return false;
        }
    }

    bool UpdateInstances(const TLASBuildInstanceData* pInstances,
                         const Uint32                 InstanceCount,
                         const Uint32                 BaseContributionToHitGroupIndex,
                         const Uint32                 HitGroupStride,
                         const HIT_GROUP_BINDING_MODE BindingMode) noexcept
    {
        VERIFY_EXPR(this->m_BuildInfo.InstanceCount == InstanceCount);
#ifdef DILIGENT_DEVELOPMENT
        bool Changed = false;
#endif
        Uint32 InstanceOffset = BaseContributionToHitGroupIndex;

        for (Uint32 i = 0; i < InstanceCount; ++i)
        {
            const auto& Inst = pInstances[i];
            auto        Iter = this->m_Instances.find(Inst.InstanceName);

            if (Iter == this->m_Instances.end())
            {
                UNEXPECTED("Failed to find instance with name '", Inst.InstanceName, "' in instances from the previous build");
                return false;
            }

            auto&      Desc      = Iter->second;
            const auto PrevIndex = Desc.ContributionToHitGroupIndex;
            const auto pPrevBLAS = Desc.pBLAS;

            Desc.pBLAS                       = ClassPtrCast<BottomLevelASImplType>(Inst.pBLAS);
            Desc.ContributionToHitGroupIndex = Inst.ContributionToHitGroupIndex;
            //Desc.InstanceIndex             = i; // keep Desc.InstanceIndex unmodified
            CalculateHitGroupIndex(Desc, InstanceOffset, HitGroupStride, BindingMode);

#ifdef DILIGENT_DEVELOPMENT
            Changed         = Changed || (pPrevBLAS != Desc.pBLAS);
            Changed         = Changed || (PrevIndex != Desc.ContributionToHitGroupIndex);
            Desc.dvpVersion = Desc.pBLAS->DvpGetVersion();
#endif
        }

        InstanceOffset = InstanceOffset + (BindingMode == HIT_GROUP_BINDING_MODE_PER_TLAS ? HitGroupStride : 0) - 1;

#ifdef DILIGENT_DEVELOPMENT
        Changed = Changed || (this->m_BuildInfo.HitGroupStride != HitGroupStride);
        Changed = Changed || (this->m_BuildInfo.FirstContributionToHitGroupIndex != BaseContributionToHitGroupIndex);
        Changed = Changed || (this->m_BuildInfo.LastContributionToHitGroupIndex != InstanceOffset);
        Changed = Changed || (this->m_BuildInfo.BindingMode != BindingMode);
        if (Changed)
            this->m_DvpVersion.fetch_add(1);
#endif
        this->m_BuildInfo.HitGroupStride                   = HitGroupStride;
        this->m_BuildInfo.FirstContributionToHitGroupIndex = BaseContributionToHitGroupIndex;
        this->m_BuildInfo.LastContributionToHitGroupIndex  = InstanceOffset;
        this->m_BuildInfo.BindingMode                      = BindingMode;

        return true;
    }

    void CopyInstanceData(const TopLevelASBase& Src) noexcept
    {
        ClearInstanceData();

        this->m_StringPool.Reserve(Src.m_StringPool.GetReservedSize(), GetRawAllocator());
        this->m_BuildInfo = Src.m_BuildInfo;

        for (auto& SrcInst : Src.m_Instances)
        {
            const char* NameCopy = this->m_StringPool.CopyString(SrcInst.first.GetStr());
            this->m_Instances.emplace(NameCopy, SrcInst.second);
        }

        VERIFY_EXPR(this->m_StringPool.GetRemainingSize() == 0);

#ifdef DILIGENT_DEVELOPMENT
        this->m_DvpVersion.fetch_add(1);
#endif
    }

    /// Implementation of ITopLevelAS::GetInstanceDesc().
    virtual TLASInstanceDesc DILIGENT_CALL_TYPE GetInstanceDesc(const char* Name) const override final
    {
        VERIFY_EXPR(Name != nullptr && Name[0] != '\0');

        TLASInstanceDesc Result = {};

        auto Iter = this->m_Instances.find(Name);
        if (Iter != this->m_Instances.end())
        {
            const auto& Inst                   = Iter->second;
            Result.ContributionToHitGroupIndex = Inst.ContributionToHitGroupIndex;
            Result.InstanceIndex               = Inst.InstanceIndex;
            Result.pBLAS                       = Inst.pBLAS;
        }
        else
        {
            Result.ContributionToHitGroupIndex = INVALID_INDEX;
            Result.InstanceIndex               = INVALID_INDEX;
            LOG_ERROR_MESSAGE("Can't find instance with the specified name ('", Name, "')");
        }

        return Result;
    }

    /// Implementation of ITopLevelAS::GetBuildInfo().
    virtual TLASBuildInfo DILIGENT_CALL_TYPE GetBuildInfo() const override final
    {
        return m_BuildInfo;
    }

    /// Implementation of ITopLevelAS::SetState().
    virtual void DILIGENT_CALL_TYPE SetState(RESOURCE_STATE State) override final
    {
        VERIFY(State == RESOURCE_STATE_UNKNOWN || State == RESOURCE_STATE_BUILD_AS_READ || State == RESOURCE_STATE_BUILD_AS_WRITE || State == RESOURCE_STATE_RAY_TRACING,
               "Unsupported state for top-level acceleration structure");
        this->m_State = State;
    }

    /// Implementation of ITopLevelAS::GetState().
    virtual RESOURCE_STATE DILIGENT_CALL_TYPE GetState() const override final
    {
        return this->m_State;
    }

    /// Implementation of ITopLevelAS::GetScratchBufferSizes().
    virtual ScratchBufferSizes DILIGENT_CALL_TYPE GetScratchBufferSizes() const override final
    {
        return this->m_ScratchSize;
    }

    bool IsInKnownState() const
    {
        return this->m_State != RESOURCE_STATE_UNKNOWN;
    }

    bool CheckState(RESOURCE_STATE State) const
    {
        VERIFY((State & (State - 1)) == 0, "Single state is expected");
        VERIFY(IsInKnownState(), "TLAS state is unknown");
        return (this->m_State & State) == State;
    }

#ifdef DILIGENT_DEVELOPMENT
    bool ValidateContent() const
    {
        bool result = true;

        if (this->m_Instances.empty())
        {
            LOG_ERROR_MESSAGE("TLAS with name ('", this->m_Desc.Name, "') doesn't have instances, use IDeviceContext::BuildTLAS() or IDeviceContext::CopyTLAS() to initialize TLAS content");
            result = false;
        }

        // Validate instances
        for (const auto& NameAndInst : this->m_Instances)
        {
            const InstanceDesc& Inst = NameAndInst.second;

            if (Inst.dvpVersion != Inst.pBLAS->DvpGetVersion())
            {
                LOG_ERROR_MESSAGE("Instance with name '", NameAndInst.first.GetStr(), "' contains BLAS with name '", Inst.pBLAS->GetDesc().Name,
                                  "' that was changed after TLAS build, you must rebuild TLAS");
                result = false;
            }

            if (Inst.pBLAS->IsInKnownState() && Inst.pBLAS->GetState() != RESOURCE_STATE_BUILD_AS_READ)
            {
                LOG_ERROR_MESSAGE("Instance with name '", NameAndInst.first.GetStr(), "' contains BLAS with name '", Inst.pBLAS->GetDesc().Name,
                                  "' that must be in BUILD_AS_READ state, but current state is ",
                                  GetResourceStateFlagString(Inst.pBLAS->GetState()));
                result = false;
            }
        }
        return result;
    }

    Uint32 GetVersion() const
    {
        return this->m_DvpVersion.load();
    }
#endif // DILIGENT_DEVELOPMENT

private:
    void ClearInstanceData()
    {
        this->m_Instances.clear();
        this->m_StringPool.Clear();

        this->m_BuildInfo.BindingMode                      = HIT_GROUP_BINDING_MODE_LAST;
        this->m_BuildInfo.HitGroupStride                   = 0;
        this->m_BuildInfo.FirstContributionToHitGroupIndex = INVALID_INDEX;
        this->m_BuildInfo.LastContributionToHitGroupIndex  = INVALID_INDEX;
    }

    static void CalculateHitGroupIndex(InstanceDesc& Desc, Uint32& InstanceOffset, const Uint32 HitGroupStride, const HIT_GROUP_BINDING_MODE BindingMode)
    {
        static_assert(HIT_GROUP_BINDING_MODE_LAST == HIT_GROUP_BINDING_MODE_USER_DEFINED, "Please update the switch below to handle the new shader binding mode");

        if (Desc.ContributionToHitGroupIndex == TLAS_INSTANCE_OFFSET_AUTO)
        {
            Desc.ContributionToHitGroupIndex = InstanceOffset;
            switch (BindingMode)
            {
                // clang-format off
                case HIT_GROUP_BINDING_MODE_PER_GEOMETRY:     InstanceOffset += Desc.pBLAS->GetActualGeometryCount() * HitGroupStride;                            break;
                case HIT_GROUP_BINDING_MODE_PER_INSTANCE:     InstanceOffset += HitGroupStride;                                                                   break;
                case HIT_GROUP_BINDING_MODE_PER_TLAS:         /* InstanceOffset is constant */                                                                  break;
                case HIT_GROUP_BINDING_MODE_USER_DEFINED:     UNEXPECTED("TLAS_INSTANCE_OFFSET_AUTO is not compatible with HIT_GROUP_BINDING_MODE_USER_DEFINED"); break;
                default:                                      UNEXPECTED("Unknown ray tracing shader binding mode");
                    // clang-format on
            }
        }
        else
        {
            VERIFY(BindingMode == HIT_GROUP_BINDING_MODE_USER_DEFINED, "BindingMode must be HIT_GROUP_BINDING_MODE_USER_DEFINED");
        }

        constexpr Uint32 MaxIndex = (1u << 24);
        VERIFY(Desc.ContributionToHitGroupIndex < MaxIndex, "ContributionToHitGroupIndex must be less than ", MaxIndex);
    }

protected:
    RESOURCE_STATE     m_State = RESOURCE_STATE_UNKNOWN;
    TLASBuildInfo      m_BuildInfo;
    ScratchBufferSizes m_ScratchSize;

    std::unordered_map<HashMapStringKey, InstanceDesc> m_Instances;

    StringPool m_StringPool;

#ifdef DILIGENT_DEVELOPMENT
    std::atomic<Uint32> m_DvpVersion{0};
#endif
};

} // namespace Diligent
