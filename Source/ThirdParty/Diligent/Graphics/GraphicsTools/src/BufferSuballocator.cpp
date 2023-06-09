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

#include "BufferSuballocator.h"

#include <mutex>
#include <atomic>

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

class BufferSuballocatorImpl;

class BufferSuballocationImpl final : public ObjectBase<IBufferSuballocation>
{
public:
    using TBase = ObjectBase<IBufferSuballocation>;
    BufferSuballocationImpl(IReferenceCounters*                          pRefCounters,
                            BufferSuballocatorImpl*                      pParentAllocator,
                            Uint32                                       Offset,
                            Uint32                                       Size,
                            VariableSizeAllocationsManager::Allocation&& Subregion) :
        // clang-format off
        TBase             {pRefCounters},
        m_pParentAllocator{pParentAllocator},
        m_Subregion       {std::move(Subregion)},
        m_Offset          {Offset},
        m_Size            {Size}
    // clang-format on
    {
        VERIFY_EXPR(m_pParentAllocator);
        VERIFY_EXPR(m_Subregion.IsValid());
    }

    ~BufferSuballocationImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BufferSuballocation, TBase)

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        RefCntAutoPtr<BufferSuballocatorImpl> pParent;
        return TBase::Release(
            [&]() //
            {
                // We must keep parent alive while this object is being destroyed because
                // the parent keeps the memory allocator for the object.
                pParent = m_pParentAllocator;
            });
    }

    virtual Uint32 GetOffset() const override final
    {
        return m_Offset;
    }

    virtual Uint32 GetSize() const override final
    {
        return m_Size;
    }

    virtual IBufferSuballocator* GetAllocator() override final;

    virtual IBuffer* GetBuffer(IRenderDevice* pDevice, IDeviceContext* pContext) override final;

    virtual void SetUserData(IObject* pUserData) override final
    {
        m_pUserData = pUserData;
    }

    virtual IObject* GetUserData() const override final
    {
        return m_pUserData;
    }

private:
    RefCntAutoPtr<BufferSuballocatorImpl> m_pParentAllocator;

    VariableSizeAllocationsManager::Allocation m_Subregion;

    const Uint32 m_Offset;
    const Uint32 m_Size;

    RefCntAutoPtr<IObject> m_pUserData;
};

class BufferSuballocatorImpl final : public ObjectBase<IBufferSuballocator>
{
public:
    using TBase = ObjectBase<IBufferSuballocator>;

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_BufferSuballocator, TBase)

    BufferSuballocatorImpl(IReferenceCounters*                 pRefCounters,
                           IRenderDevice*                      pDevice,
                           const BufferSuballocatorCreateInfo& CreateInfo) :
        // clang-format off
        TBase{pRefCounters},
        m_Mgr
        {
            VariableSizeAllocationsManager::CreateInfo
            {
                DefaultRawMemoryAllocator::GetAllocator(),
                StaticCast<size_t>(CreateInfo.Desc.Size),
                CreateInfo.DisableDebugValidation
            }
        },
        m_MgrSize{m_Mgr.GetMaxSize()},
        m_Buffer
        {
            pDevice,
            DynamicBufferCreateInfo
            {
                CreateInfo.Desc,
                CreateInfo.ExpansionSize != 0 ? CreateInfo.ExpansionSize : static_cast<Uint32>(CreateInfo.Desc.Size), // MemoryPageSize
                CreateInfo.VirtualSize
            }
        },
        m_BufferSize{m_Buffer.GetDesc().Size},
        m_ExpansionSize{CreateInfo.ExpansionSize},
        m_SuballocationsAllocator
        {
            DefaultRawMemoryAllocator::GetAllocator(),
            sizeof(BufferSuballocationImpl),
            1024u / Uint32{sizeof(BufferSuballocationImpl)} // Use 1 Kb pages.
        }
    // clang-format on
    {}

    ~BufferSuballocatorImpl()
    {
        VERIFY_EXPR(m_AllocationCount.load() == 0);
    }

    virtual IBuffer* GetBuffer(IRenderDevice* pDevice, IDeviceContext* pContext) override final
    {
        // NB: mutex must not be locked here to avoid stalling render thread
        const auto MgrSize = m_MgrSize.load();
        VERIFY_EXPR(m_BufferSize.load() == m_Buffer.GetDesc().Size);
        if (MgrSize > m_Buffer.GetDesc().Size)
        {
            m_Buffer.Resize(pDevice, pContext, MgrSize);
            // We must use atomic because this value is read in another thread,
            // while m_Buffer internally does not use mutex or other synchronization.
            m_BufferSize.store(m_Buffer.GetDesc().Size);
        }
        return m_Buffer.GetBuffer(pDevice, pContext);
    }

    virtual void Allocate(Uint32                 Size,
                          Uint32                 Alignment,
                          IBufferSuballocation** ppSuballocation) override final
    {
        if (Size == 0)
        {
            UNEXPECTED("Size must not be zero");
            return;
        }

        if (!IsPowerOfTwo(Alignment))
        {
            UNEXPECTED("Alignment (", Alignment, ") is not a power of two");
            return;
        }

        if (ppSuballocation == nullptr)
        {
            UNEXPECTED("ppSuballocation must not be null");
            return;
        }

        DEV_CHECK_ERR(*ppSuballocation == nullptr, "Overwriting reference to existing object may cause memory leaks");

        VariableSizeAllocationsManager::Allocation Subregion;
        {
            std::lock_guard<std::mutex> Lock{m_MgrMtx};

            {
                // After the resize, the actual buffer size may be larger due to alignment
                // requirements (for sparse buffers, the size is aligned by the memory page size).
                const auto BufferSize = m_BufferSize.load();
                const auto MgrSize    = m_Mgr.GetMaxSize();
                if (BufferSize > MgrSize)
                {
                    m_Mgr.Extend(StaticCast<size_t>(BufferSize - MgrSize));
                    VERIFY_EXPR(m_Mgr.GetMaxSize() == BufferSize);
                    m_MgrSize.store(m_Mgr.GetMaxSize());
                }
            }

            Subregion = m_Mgr.Allocate(Size, Alignment);

            while (!Subregion.IsValid())
            {
                auto ExtraSize = m_ExpansionSize != 0 ?
                    std::max(m_ExpansionSize, AlignUp(Size, Alignment)) :
                    m_Mgr.GetMaxSize();

                m_Mgr.Extend(ExtraSize);
                m_MgrSize.store(m_Mgr.GetMaxSize());

                Subregion = m_Mgr.Allocate(Size, Alignment);
            }

            UpdateUsageStats();
        }

        // clang-format off
        BufferSuballocationImpl* pSuballocation{
            NEW_RC_OBJ(m_SuballocationsAllocator, "BufferSuballocationImpl instance", BufferSuballocationImpl)
            (
                this,
                AlignUp(static_cast<Uint32>(Subregion.UnalignedOffset), Alignment),
                Size,
                std::move(Subregion)
            )
        };
        // clang-format on

        pSuballocation->QueryInterface(IID_BufferSuballocation, reinterpret_cast<IObject**>(ppSuballocation));
        m_AllocationCount.fetch_add(1);
    }

    void Free(VariableSizeAllocationsManager::Allocation&& Subregion)
    {
        std::lock_guard<std::mutex> Lock{m_MgrMtx};
        m_Mgr.Free(std::move(Subregion));
        m_AllocationCount.fetch_add(-1);
        UpdateUsageStats();
    }

    virtual Uint32 GetVersion() const override final
    {
        return m_Buffer.GetVersion();
    }

    virtual void GetUsageStats(BufferSuballocatorUsageStats& UsageStats) override final
    {
        // NB: mutex must not be locked here to avoid stalling render thread
        UsageStats.CommittedSize    = m_BufferSize.load();
        UsageStats.UsedSize         = m_UsedSize.load();
        UsageStats.MaxFreeChunkSize = m_MaxFreeBlockSize.load();
        UsageStats.AllocationCount  = m_AllocationCount.load();
    }

private:
    void UpdateUsageStats()
    {
        m_UsedSize.store(m_Mgr.GetUsedSize());
        m_MaxFreeBlockSize.store(m_Mgr.GetMaxFreeBlockSize());
    }

private:
    std::mutex                     m_MgrMtx;
    VariableSizeAllocationsManager m_Mgr;

    std::atomic<VariableSizeAllocationsManager::OffsetType> m_MgrSize{0};

    DynamicBuffer       m_Buffer;
    std::atomic<Uint64> m_BufferSize{0};

    const Uint32 m_ExpansionSize;

    std::atomic<Int32>  m_AllocationCount{0};
    std::atomic<Uint64> m_UsedSize{0};
    std::atomic<Uint64> m_MaxFreeBlockSize{0};

    FixedBlockMemoryAllocator m_SuballocationsAllocator;
};


BufferSuballocationImpl::~BufferSuballocationImpl()
{
    m_pParentAllocator->Free(std::move(m_Subregion));
}

IBufferSuballocator* BufferSuballocationImpl::GetAllocator()
{
    return m_pParentAllocator;
}

IBuffer* BufferSuballocationImpl::GetBuffer(IRenderDevice* pDevice, IDeviceContext* pContext)
{
    return m_pParentAllocator->GetBuffer(pDevice, pContext);
}


void CreateBufferSuballocator(IRenderDevice*                      pDevice,
                              const BufferSuballocatorCreateInfo& CreateInfo,
                              IBufferSuballocator**               ppBufferSuballocator)
{
    try
    {
        auto* pAllocator = MakeNewRCObj<BufferSuballocatorImpl>()(pDevice, CreateInfo);
        pAllocator->QueryInterface(IID_BufferSuballocator, reinterpret_cast<IObject**>(ppBufferSuballocator));
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to create buffer suballocator");
    }
}

} // namespace Diligent
