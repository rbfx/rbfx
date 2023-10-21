/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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
/// Implementation of the Diligent::BottomLevelASBase template class

#include <unordered_map>
#include <atomic>

#include "BottomLevelAS.h"
#include "DeviceObjectBase.hpp"
#include "RenderDeviceBase.hpp"
#include "FixedLinearAllocator.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

struct BLASGeomIndex
{
    Uint32 IndexInDesc = INVALID_INDEX; // Geometry index in BottomLevelASDesc
    Uint32 ActualIndex = INVALID_INDEX; // Geometry index in build operation

    BLASGeomIndex() {}
    BLASGeomIndex(Uint32 _IndexInDesc,
                  Uint32 _ActualIndex) :
        IndexInDesc{_IndexInDesc},
        ActualIndex{_ActualIndex}
    {}
};
using BLASNameToIndex = std::unordered_map<HashMapStringKey, BLASGeomIndex>;

/// Validates bottom-level AS description and throws an exception in case of an error.
void ValidateBottomLevelASDesc(const BottomLevelASDesc& Desc) noexcept(false);

/// Copies bottom-level AS geometry description using MemPool to allocate required space.
void CopyBLASGeometryDesc(const BottomLevelASDesc& SrcDesc,
                          BottomLevelASDesc&       DstDesc,
                          FixedLinearAllocator&    MemPool,
                          const BLASNameToIndex*   pSrcNameToIndex,
                          BLASNameToIndex&         DstNameToIndex) noexcept(false);


/// Template class implementing base functionality of the bottom-level acceleration structure object.

/// \tparam EngineImplTraits - Engine implementation type traits.
template <typename EngineImplTraits>
class BottomLevelASBase : public DeviceObjectBase<typename EngineImplTraits::BottomLevelASInterface, typename EngineImplTraits::RenderDeviceImplType, BottomLevelASDesc>
{
public:
    // Base interface that this class inherits (IBottomLevelASD3D12 or IBottomLevelASVk).
    using BaseInterface = typename EngineImplTraits::BottomLevelASInterface;

    // Render device implementation type (RenderDeviceD3D12Impl or RenderDeviceVkImpl).
    using RenderDeviceImplType = typename EngineImplTraits::RenderDeviceImplType;

    using TDeviceObjectBase = DeviceObjectBase<BaseInterface, RenderDeviceImplType, BottomLevelASDesc>;

    /// \param pRefCounters      - Reference counters object that controls the lifetime of this BLAS.
    /// \param pDevice           - Pointer to the device.
    /// \param Desc              - BLAS description.
    /// \param bIsDeviceInternal - Flag indicating if the BLAS is an internal device object and
    ///							   must not keep a strong reference to the device.
    BottomLevelASBase(IReferenceCounters*      pRefCounters,
                      RenderDeviceImplType*    pDevice,
                      const BottomLevelASDesc& Desc,
                      bool                     bIsDeviceInternal = false) :
        TDeviceObjectBase{pRefCounters, pDevice, Desc, bIsDeviceInternal}
    {
        if (!this->GetDevice()->GetFeatures().RayTracing)
            LOG_ERROR_AND_THROW("Ray tracing is not supported by this device");

        ValidateBottomLevelASDesc(this->m_Desc);

        if (Desc.CompactedSize > 0)
        {
        }
        else
        {
            CopyGeometryDescriptionUnsafe(Desc, nullptr);
        }
    }

    ~BottomLevelASBase()
    {
        ClearGeometry();
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BottomLevelAS, TDeviceObjectBase)

    // Maps geometry that was used in a build operation to the geometry description.
    // Returns the geometry index in geometry description.
    Uint32 UpdateGeometryIndex(const char* Name, Uint32& ActualIndex, bool OnUpdate)
    {
        DEV_CHECK_ERR(Name != nullptr && Name[0] != '\0', "Geometry name must not be empty");

        auto iter = m_NameToIndex.find(Name);
        if (iter != m_NameToIndex.end())
        {
            if (OnUpdate)
                ActualIndex = iter->second.ActualIndex;
            else
                iter->second.ActualIndex = ActualIndex;
            return iter->second.IndexInDesc;
        }
        LOG_ERROR_MESSAGE("Can't find geometry with name '", Name, '\'');
        return INVALID_INDEX;
    }

    /// Implementation of IBottomLevelAS::GetGeometryDescIndex()
    virtual Uint32 DILIGENT_CALL_TYPE GetGeometryDescIndex(const char* Name) const override final
    {
        DEV_CHECK_ERR(Name != nullptr && Name[0] != '\0', "Geometry name must not be empty");

        auto iter = m_NameToIndex.find(Name);
        if (iter != m_NameToIndex.end())
            return iter->second.IndexInDesc;

        LOG_ERROR_MESSAGE("Can't find geometry with name '", Name, '\'');
        return INVALID_INDEX;
    }

    /// Implementation of IBottomLevelAS::GetGeometryIndex()
    virtual Uint32 DILIGENT_CALL_TYPE GetGeometryIndex(const char* Name) const override final
    {
        DEV_CHECK_ERR(Name != nullptr && Name[0] != '\0', "Geometry name must not be empty");

        auto iter = m_NameToIndex.find(Name);
        if (iter != m_NameToIndex.end())
        {
            VERIFY(iter->second.ActualIndex != INVALID_INDEX, "Geometry with name '", Name, "', exists, but was not enabled in the last build");
            return iter->second.ActualIndex;
        }
        LOG_ERROR_MESSAGE("Can't find geometry with name '", Name, '\'');
        return INVALID_INDEX;
    }

    /// Implementation of IBottomLevelAS::SetState()
    virtual void DILIGENT_CALL_TYPE SetState(RESOURCE_STATE State) override final
    {
        DEV_CHECK_ERR(State == RESOURCE_STATE_UNKNOWN || State == RESOURCE_STATE_BUILD_AS_READ || State == RESOURCE_STATE_BUILD_AS_WRITE,
                      "Unsupported state for a bottom-level acceleration structure");
        this->m_State = State;
    }

    /// Implementation of IBottomLevelAS::GetState()
    virtual RESOURCE_STATE DILIGENT_CALL_TYPE GetState() const override final
    {
        return this->m_State;
    }

    /// Implementation of IBottomLevelAS::GetScratchBufferSizes()
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
        DEV_CHECK_ERR((State & (State - 1)) == 0, "Single state is expected");
        DEV_CHECK_ERR(IsInKnownState(), "BLAS state is unknown");
        return (this->m_State & State) == State;
    }

#ifdef DILIGENT_DEVELOPMENT
    void DvpUpdateVersion()
    {
        this->m_DvpVersion.fetch_add(1);
    }

    Uint32 DvpGetVersion() const
    {
        return this->m_DvpVersion.load();
    }
#endif // DILIGENT_DEVELOPMENT

    void CopyGeometryDescription(const BottomLevelASBase& SrcBLAS) noexcept
    {
        ClearGeometry();

        try
        {
            CopyGeometryDescriptionUnsafe(SrcBLAS.GetDesc(), &SrcBLAS.m_NameToIndex);
        }
        catch (...)
        {
            ClearGeometry();
        }
    }

    void SetActualGeometryCount(Uint32 Count)
    {
        m_GeometryCount = Count;
    }

    virtual Uint32 DILIGENT_CALL_TYPE GetActualGeometryCount() const override final
    {
        return m_GeometryCount;
    }

private:
    void CopyGeometryDescriptionUnsafe(const BottomLevelASDesc& SrcDesc, const BLASNameToIndex* pSrcNameToIndex) noexcept(false)
    {
        FixedLinearAllocator MemPool{GetRawAllocator()};
        CopyBLASGeometryDesc(SrcDesc, this->m_Desc, MemPool, pSrcNameToIndex, this->m_NameToIndex);
        this->m_pRawPtr = MemPool.Release();
    }

    void ClearGeometry() noexcept
    {
        if (this->m_pRawPtr != nullptr)
        {
            GetRawAllocator().Free(this->m_pRawPtr);
            this->m_pRawPtr = nullptr;
        }

        // Keep Name, Flags, CompactedSize, ImmediateContextMask
        this->m_Desc.pTriangles    = nullptr;
        this->m_Desc.TriangleCount = 0;
        this->m_Desc.pBoxes        = nullptr;
        this->m_Desc.BoxCount      = 0;

        m_NameToIndex = decltype(m_NameToIndex){};
    }

protected:
    RESOURCE_STATE     m_State = RESOURCE_STATE_UNKNOWN;
    BLASNameToIndex    m_NameToIndex;
    void*              m_pRawPtr       = nullptr;
    Uint32             m_GeometryCount = 0;
    ScratchBufferSizes m_ScratchSize;

#ifdef DILIGENT_DEVELOPMENT
    std::atomic<Uint32> m_DvpVersion{0};
#endif
};

} // namespace Diligent
