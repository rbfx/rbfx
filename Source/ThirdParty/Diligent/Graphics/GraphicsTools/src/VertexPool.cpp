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

#include "VertexPool.h"

#include <mutex>
#include <atomic>
#include <memory>

#include "DebugUtilities.hpp"
#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "DynamicBuffer.hpp"
#include "VariableSizeAllocationsManager.hpp"
#include "Align.hpp"
#include "DefaultRawMemoryAllocator.hpp"
#include "FixedBlockMemoryAllocator.hpp"

namespace Diligent
{

class VertexPoolImpl;

class VertexPoolAllocationImpl final : public ObjectBase<IVertexPoolAllocation>
{
public:
    using TBase = ObjectBase<IVertexPoolAllocation>;
    VertexPoolAllocationImpl(IReferenceCounters*                          pRefCounters,
                             VertexPoolImpl*                              pParentPool,
                             Uint32                                       StartVertex,
                             Uint32                                       VertexCount,
                             VariableSizeAllocationsManager::Allocation&& Region) :
        // clang-format off
        TBase        {pRefCounters},
        m_pParentPool{pParentPool},
        m_Region     {std::move(Region)},
        m_StartVertex{StartVertex},
        m_VertexCount{VertexCount}
    // clang-format on
    {
        VERIFY_EXPR(m_pParentPool);
        VERIFY_EXPR(m_Region.IsValid());
    }

    ~VertexPoolAllocationImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_VertexPoolAllocation, TBase)

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        RefCntAutoPtr<VertexPoolImpl> pParent;
        return TBase::Release(
            [&]() //
            {
                // We must keep the pool alive while this object is being destroyed because
                // the pool keeps the memory allocator for this object.
                pParent = m_pParentPool;
            });
    }

    virtual Uint32 GetStartVertex() const override final
    {
        return m_StartVertex;
    }

    virtual Uint32 GetVertexCount() const override final
    {
        return m_VertexCount;
    }

    virtual IVertexPool* GetPool() override final;

    virtual IBuffer* Update(Uint32 Index, IRenderDevice* pDevice, IDeviceContext* pContext) override final;

    virtual IBuffer* GetBuffer(Uint32 Index) const override final;

    virtual void SetUserData(IObject* pUserData) override final
    {
        m_pUserData = pUserData;
    }

    virtual IObject* GetUserData() const override final
    {
        return m_pUserData;
    }

private:
    RefCntAutoPtr<VertexPoolImpl> m_pParentPool;

    VariableSizeAllocationsManager::Allocation m_Region;

    const Uint32 m_StartVertex;
    const Uint32 m_VertexCount;

    RefCntAutoPtr<IObject> m_pUserData;
};

class VertexPoolImpl final : public ObjectBase<IVertexPool>
{
public:
    using TBase = ObjectBase<IVertexPool>;

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_VertexPool, TBase)

    VertexPoolImpl(IReferenceCounters*         pRefCounters,
                   IRenderDevice*              pDevice,
                   const VertexPoolCreateInfo& CreateInfo) :
        // clang-format off
        TBase{pRefCounters},
        m_Name    {CreateInfo.Desc.Name != nullptr ? CreateInfo.Desc.Name : "Vertex pool"},
        m_Elements{CreateInfo.Desc.pElements, CreateInfo.Desc.pElements + CreateInfo.Desc.NumElements},
        m_Desc    {CreateInfo.Desc},
        m_Mgr
        {
            VariableSizeAllocationsManager::CreateInfo
            {
                DefaultRawMemoryAllocator::GetAllocator(),
                CreateInfo.Desc.VertexCount,
                CreateInfo.DisableDebugValidation
            }
        },
        m_MgrSize         {m_Mgr.GetMaxSize()},
        m_BufferSizes     (m_Desc.NumElements),
        m_ExtraVertexCount{CreateInfo.ExtraVertexCount},
        // clang-format on
        m_MaxVertexCount{
            [](Uint32 VertexCount, Uint32 MaxVertexCount) {
                if (MaxVertexCount != 0 && MaxVertexCount < VertexCount)
                {
                    LOG_WARNING_MESSAGE("MaxVertexCount (", MaxVertexCount, ") is less than VertexCount (", VertexCount, ").");
                    MaxVertexCount = VertexCount;
                }
                return MaxVertexCount;
            }(CreateInfo.Desc.VertexCount, CreateInfo.MaxVertexCount),
        },
        m_AllocationObjAllocator{
            DefaultRawMemoryAllocator::GetAllocator(),
            sizeof(VertexPoolAllocationImpl),
            1024u / Uint32{sizeof(VertexPoolAllocationImpl)} // Use 1 Kb pages.
        }
    {
        m_Desc.Name      = m_Name.c_str();
        m_Desc.pElements = m_Elements.data();

        m_Buffers.reserve(m_Desc.NumElements);
        for (Uint32 i = 0; i < m_Desc.NumElements; ++i)
        {
            const auto& VtxElem = m_Desc.pElements[i];

            std::string Name = m_Desc.Name;
            Name += " - buffer ";
            Name += std::to_string(i);

            DynamicBufferCreateInfo DynBuffCI;
            DynBuffCI.Desc.Name           = Name.c_str();
            DynBuffCI.Desc.Size           = Uint64{m_Desc.VertexCount} * Uint64{VtxElem.Size};
            DynBuffCI.Desc.BindFlags      = VtxElem.BindFlags;
            DynBuffCI.Desc.Usage          = VtxElem.Usage;
            DynBuffCI.Desc.CPUAccessFlags = VtxElem.CPUAccessFlags;
            DynBuffCI.Desc.Mode           = VtxElem.Mode;
            if (DynBuffCI.Desc.BindFlags & (BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS))
            {
                DynBuffCI.Desc.ElementByteStride = VtxElem.Size;
            }
            DynBuffCI.MemoryPageSize = CreateInfo.ExtraVertexCount != 0 ?
                CreateInfo.ExtraVertexCount * VtxElem.Size :
                static_cast<Uint32>(DynBuffCI.Desc.Size);

            DynBuffCI.VirtualSize = m_MaxVertexCount != 0 ?
                Uint64{m_MaxVertexCount} * Uint64{VtxElem.Size} :
                // Use 2GB as the default virtual size, but reserve at least 1 MB for alignment.
                // Resources above 2GB don't work in Direct3D11 (even though there are no errors).
                (Uint64{2} << Uint64{30}) - AlignUp(DynBuffCI.MemoryPageSize, 1u << 20u);

            m_Buffers.emplace_back(std::make_unique<DynamicBuffer>(pDevice, DynBuffCI));
            if (!m_Buffers.back())
                LOG_ERROR_AND_THROW("Failed to create dynamic buffer ", i);

            // NB: request the size from the buffer. It may be different from DynBuffCI.Desc.Size.
            m_BufferSizes[i].store(m_Buffers.back()->GetDesc().Size);
        }
    }

    ~VertexPoolImpl()
    {
        VERIFY_EXPR(m_AllocationCount.load() == 0);
    }

    virtual IBuffer* Update(Uint32 Index, IRenderDevice* pDevice, IDeviceContext* pContext) override final
    {
        if (Index >= m_Buffers.size())
        {
            UNEXPECTED("Index (", Index, ") is out of range: there are only ", m_Buffers.size(), " buffers.");
            return nullptr;
        }
        auto& BufferSize = m_BufferSizes[Index];
        auto& Buffer     = *m_Buffers[Index];

        // NB: mutex must not be locked here to avoid stalling render thread
        const auto MgrSize = m_MgrSize.load() * m_Elements[Index].Size;
        VERIFY_EXPR(BufferSize.load() == Buffer.GetDesc().Size);
        if (MgrSize > Buffer.GetDesc().Size)
        {
            Buffer.Resize(pDevice, pContext, MgrSize);
            // We must use atomic because this value is read in another thread,
            // while m_Buffer internally does not use mutex or other synchronization.
            BufferSize.store(Buffer.GetDesc().Size);

            UpdateCommittedMemorySize();
        }
        return Buffer.Update(pDevice, pContext);
    }

    virtual void UpdateAll(IRenderDevice* pDevice, IDeviceContext* pContext) override final
    {
        for (Uint32 i = 0; i < m_Buffers.size(); ++i)
            Update(i, pDevice, pContext);
    }

    virtual IBuffer* GetBuffer(Uint32 Index) const override final
    {
        if (Index >= m_Buffers.size())
        {
            UNEXPECTED("Index (", Index, ") is out of range: there are only ", m_Buffers.size(), " buffers.");
            return nullptr;
        }

        return m_Buffers[Index]->GetBuffer();
    }

    virtual void Allocate(Uint32                  NumVertices,
                          IVertexPoolAllocation** ppAllocation) override final
    {
        if (NumVertices == 0)
        {
            UNEXPECTED("Vertex count must not be zero");
            return;
        }

        if (ppAllocation == nullptr)
        {
            UNEXPECTED("ppAllocation must not be null");
            return;
        }

        DEV_CHECK_ERR(*ppAllocation == nullptr, "Overwriting reference to existing object may cause memory leaks");

        VariableSizeAllocationsManager::Allocation Region;
        {
            std::lock_guard<std::mutex> Lock{m_MgrMtx};

            {
                Uint64 ActualCapacity = ~Uint64{0};
                for (Uint32 i = 0; i < m_Desc.NumElements; ++i)
                {
                    const auto BufferCapacity = m_BufferSizes[i].load() / m_Elements[i].Size;
                    ActualCapacity            = std::min(ActualCapacity, BufferCapacity);
                }

                // After the resize, the actual buffer size may be larger due to alignment
                // requirements (for sparse buffers, the size is aligned by the memory page size).
                const auto MgrSize = m_Mgr.GetMaxSize();
                if (ActualCapacity > MgrSize)
                {
                    m_Mgr.Extend(StaticCast<size_t>(ActualCapacity - MgrSize));
                    VERIFY_EXPR(m_Mgr.GetMaxSize() == ActualCapacity);
                    m_MgrSize.store(m_Mgr.GetMaxSize());
                    m_Desc.VertexCount = static_cast<Uint32>(ActualCapacity);
                }
            }

            Region = m_Mgr.Allocate(NumVertices, 1);

            while (!Region.IsValid() && (m_MaxVertexCount == 0 || m_Mgr.GetMaxSize() < m_MaxVertexCount))
            {
                size_t ExtraSize = m_ExtraVertexCount != 0 ?
                    std::max(m_ExtraVertexCount, NumVertices) :
                    m_Mgr.GetMaxSize();

                if (m_MaxVertexCount != 0)
                    ExtraSize = std::min(ExtraSize, size_t{m_MaxVertexCount} - m_Mgr.GetMaxSize());

                m_Mgr.Extend(ExtraSize);
                m_MgrSize.store(m_Mgr.GetMaxSize());
                m_Desc.VertexCount = static_cast<Uint32>(m_Mgr.GetMaxSize());

                Region = m_Mgr.Allocate(NumVertices, 1);
            }

            UpdateUsageStats();
        }

        if (Region.IsValid())
        {
            // clang-format off
            VertexPoolAllocationImpl* pSuballocation{
                NEW_RC_OBJ(m_AllocationObjAllocator, "VertexPoolAllocationImpl instance", VertexPoolAllocationImpl)
                (
                    this,
                    static_cast<Uint32>(Region.UnalignedOffset),
                    NumVertices,
                    std::move(Region)
                )
            };
            // clang-format on

            pSuballocation->QueryInterface(IID_VertexPoolAllocation, reinterpret_cast<IObject**>(ppAllocation));
            m_AllocationCount.fetch_add(1);
        }
    }

    void Free(VariableSizeAllocationsManager::Allocation&& Region)
    {
        std::lock_guard<std::mutex> Lock{m_MgrMtx};
        m_Mgr.Free(std::move(Region));
        m_AllocationCount.fetch_add(-1);
        UpdateUsageStats();
    }

    virtual Uint32 GetVersion() const override final
    {
        Uint32 Version = 0;
        for (const auto& Buffer : m_Buffers)
            Version += Buffer->GetVersion();
        return Version;
    }

    virtual const VertexPoolDesc& GetDesc() const override final
    {
        return m_Desc;
    }

    virtual void GetUsageStats(VertexPoolUsageStats& UsageStats) override final
    {
        // NB: mutex must not be locked here to avoid stalling render thread
        UsageStats.TotalVertexCount     = m_TotalVertexCount.load();
        UsageStats.AllocatedVertexCount = m_AllocatedVertexCount.load();
        UsageStats.CommittedMemorySize  = m_CommittedMemorySize.load();

        Uint64 VertexSize = 0;
        for (size_t Elem = 0; Elem < m_Desc.NumElements; ++Elem)
            VertexSize += m_Desc.pElements[Elem].Size;
        UsageStats.UsedMemorySize = UsageStats.AllocatedVertexCount * VertexSize;

        UsageStats.AllocationCount = m_AllocationCount.load();
    }

private:
    void UpdateUsageStats()
    {
        m_AllocatedVertexCount.store(m_Mgr.GetUsedSize());
        m_TotalVertexCount.store(m_Mgr.GetMaxSize());
        UpdateCommittedMemorySize();
    }
    void UpdateCommittedMemorySize()
    {
        Uint64 CommittedMemorySize = 0;
        for (const auto& BuffSize : m_BufferSizes)
            CommittedMemorySize += BuffSize.load();
        m_CommittedMemorySize.store(CommittedMemorySize);
    }

private:
    const std::string                        m_Name;
    const std::vector<VertexPoolElementDesc> m_Elements;

    VertexPoolDesc m_Desc;

    std::mutex                     m_MgrMtx;
    VariableSizeAllocationsManager m_Mgr;

    std::atomic<VariableSizeAllocationsManager::OffsetType> m_MgrSize{0};

    std::vector<std::unique_ptr<DynamicBuffer>> m_Buffers;
    std::vector<std::atomic<Uint64>>            m_BufferSizes;

    const Uint32 m_ExtraVertexCount;
    const Uint32 m_MaxVertexCount;

    std::atomic<Int32>  m_AllocationCount{0};
    std::atomic<Uint64> m_AllocatedVertexCount{0};
    std::atomic<Uint64> m_CommittedMemorySize{0};
    std::atomic<Uint64> m_TotalVertexCount{0};

    FixedBlockMemoryAllocator m_AllocationObjAllocator;
};


VertexPoolAllocationImpl::~VertexPoolAllocationImpl()
{
    m_pParentPool->Free(std::move(m_Region));
}

IVertexPool* VertexPoolAllocationImpl::GetPool()
{
    return m_pParentPool;
}

IBuffer* VertexPoolAllocationImpl::Update(Uint32 Index, IRenderDevice* pDevice, IDeviceContext* pContext)
{
    return m_pParentPool->Update(Index, pDevice, pContext);
}

IBuffer* VertexPoolAllocationImpl::GetBuffer(Uint32 Index) const
{
    return m_pParentPool->GetBuffer(Index);
}

void CreateVertexPool(IRenderDevice*              pDevice,
                      const VertexPoolCreateInfo& CreateInfo,
                      IVertexPool**               ppVertexPool)
{
    try
    {
        auto* pPool = MakeNewRCObj<VertexPoolImpl>()(pDevice, CreateInfo);
        pPool->QueryInterface(IID_VertexPool, reinterpret_cast<IObject**>(ppVertexPool));
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to create buffer subPool");
    }
}

} // namespace Diligent
