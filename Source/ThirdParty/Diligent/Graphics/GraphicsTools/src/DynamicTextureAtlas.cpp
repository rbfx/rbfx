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

#include "DynamicTextureAtlas.h"

#include <mutex>
#include <algorithm>
#include <atomic>
#include <unordered_map>
#include <map>
#include <set>

#include "DynamicAtlasManager.hpp"
#include "DynamicTextureArray.hpp"
#include "ObjectBase.hpp"
#include "RefCntAutoPtr.hpp"
#include "FixedBlockMemoryAllocator.hpp"
#include "DefaultRawMemoryAllocator.hpp"
#include "GraphicsAccessories.hpp"
#include "Align.hpp"

namespace Diligent
{

class DynamicTextureAtlasImpl;

class TextureAtlasSuballocationImpl final : public ObjectBase<ITextureAtlasSuballocation>
{
public:
    using TBase = ObjectBase<ITextureAtlasSuballocation>;
    TextureAtlasSuballocationImpl(IReferenceCounters*           pRefCounters,
                                  DynamicTextureAtlasImpl*      pParentAtlas,
                                  DynamicAtlasManager::Region&& Subregion,
                                  Uint32                        Slice,
                                  Uint32                        Alignment,
                                  const uint2&                  Size) noexcept :
        // clang-format off
        TBase         {pRefCounters},
        m_pParentAtlas{pParentAtlas},
        m_Subregion   {std::move(Subregion)},
        m_Slice       {Slice},
        m_Alignment   {Alignment},
        m_Size        {Size}
    // clang-format on
    {
        VERIFY_EXPR(m_pParentAtlas);
        VERIFY_EXPR(!m_Subregion.IsEmpty());
    }

    ~TextureAtlasSuballocationImpl();

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_TextureAtlasSuballocation, TBase)

    virtual ReferenceCounterValueType DILIGENT_CALL_TYPE Release() override final
    {
        RefCntAutoPtr<DynamicTextureAtlasImpl> pAtlas;
        return TBase::Release(
            [&]() //
            {
                // We must keep parent alive while this object is being destroyed because
                // the parent keeps the memory allocator for the object.
                pAtlas = m_pParentAtlas;
            });
    }

    virtual uint2 GetOrigin() const override final
    {
        return uint2 //
            {
                m_Subregion.x * m_Alignment,
                m_Subregion.y * m_Alignment //
            };
    }

    virtual Uint32 GetSlice() const override final
    {
        return m_Slice;
    }

    virtual uint2 GetSize() const override final
    {
        return m_Size;
    }

    virtual float4 GetUVScaleBias() const override final;

    virtual Uint32 GetAlignment() const override final
    {
        return m_Alignment;
    }

    virtual IDynamicTextureAtlas* GetAtlas() override final;

    virtual void SetUserData(IObject* pUserData) override final
    {
        m_pUserData = pUserData;
    }

    virtual IObject* GetUserData() const override final
    {
        return m_pUserData;
    }

private:
    RefCntAutoPtr<DynamicTextureAtlasImpl> m_pParentAtlas;

    DynamicAtlasManager::Region m_Subregion;

    const Uint32 m_Slice;
    const Uint32 m_Alignment;
    const uint2  m_Size;

    RefCntAutoPtr<IObject> m_pUserData;
};

namespace
{

class ThreadSafeAtlasManager
{
public:
    ThreadSafeAtlasManager(const uint2& Dim) noexcept :
        Mgr{Dim.x, Dim.y}
    {}

    // clang-format off
    ThreadSafeAtlasManager           (const ThreadSafeAtlasManager&)  = delete;
    ThreadSafeAtlasManager           (      ThreadSafeAtlasManager&&) = delete;
    ThreadSafeAtlasManager& operator=(const ThreadSafeAtlasManager&)  = delete;
    ThreadSafeAtlasManager& operator=(      ThreadSafeAtlasManager&&) = delete;
    // clang-format on

    class ManagerGuard
    {
    public:
        ManagerGuard() noexcept {}

        // clang-format off
        ManagerGuard           (const ManagerGuard&) = delete;
        ManagerGuard& operator=(const ManagerGuard&) = delete;
        // clang-format on

        ManagerGuard(ManagerGuard&& Other) noexcept :
            pAtlasMgr{Other.pAtlasMgr}
        {
            Other.pAtlasMgr = nullptr;
        }

        ManagerGuard& operator=(ManagerGuard&& Other) noexcept
        {
            pAtlasMgr       = Other.pAtlasMgr;
            Other.pAtlasMgr = nullptr;
            return *this;
        }

        int Release()
        {
            int UseCnt = -1;
            if (pAtlasMgr != nullptr)
            {
                UseCnt    = pAtlasMgr->ReleaseUse();
                pAtlasMgr = nullptr;
            }
            return UseCnt;
        }

        ~ManagerGuard()
        {
            Release();
        }

        explicit operator bool() const
        {
            return pAtlasMgr != nullptr;
        }

        DynamicAtlasManager::Region Allocate(Uint32 Width, Uint32 Height)
        {
            VERIFY_EXPR(pAtlasMgr != nullptr);
            VERIFY_EXPR(pAtlasMgr->UseCount > 0);
            std::lock_guard<std::mutex> Guard{pAtlasMgr->Mtx};
            return pAtlasMgr->Mgr.Allocate(Width, Height);
        }

        // Frees a region and returns true if the atlas is empty
        bool Free(DynamicAtlasManager::Region&& R)
        {
            VERIFY_EXPR(pAtlasMgr != nullptr);
            VERIFY_EXPR(pAtlasMgr->UseCount > 0);
            std::lock_guard<std::mutex> Guard{pAtlasMgr->Mtx};
            pAtlasMgr->Mgr.Free(std::move(R));
            return pAtlasMgr->Mgr.IsEmpty();
        }

        bool IsEmpty()
        {
            VERIFY_EXPR(pAtlasMgr != nullptr);
            VERIFY_EXPR(pAtlasMgr->UseCount > 0);
            std::lock_guard<std::mutex> Guard{pAtlasMgr->Mtx};
            return pAtlasMgr->Mgr.IsEmpty();
        }

    private:
        friend class ThreadSafeAtlasManager;

        explicit ManagerGuard(ThreadSafeAtlasManager& AtlasMg) :
            pAtlasMgr{&AtlasMg}
        {
            AtlasMg.AddUse();
        }

        ThreadSafeAtlasManager* pAtlasMgr = nullptr;
    };

    ManagerGuard Lock()
    {
        return ManagerGuard{*this};
    }

    int GetUseCount() const
    {
        return UseCount.load();
    }

private:
    friend ManagerGuard;

    int AddUse()
    {
        auto Uses = UseCount.fetch_add(1) + 1;
        VERIFY_EXPR(Uses > 0);
        return Uses;
    }

    int ReleaseUse()
    {
        auto Uses = UseCount.fetch_add(-1) - 1;
        VERIFY_EXPR(Uses >= 0);
        return Uses;
    }

private:
    std::mutex          Mtx;
    DynamicAtlasManager Mgr;

    std::atomic_int UseCount{0};
};


struct SliceBatch
{
    SliceBatch(const uint2 AtlasDim) noexcept :
        m_AtlasDim{AtlasDim}
    {}

    ~SliceBatch()
    {
        VERIFY(m_Slices.empty(), "Not all slice managers have been released.");
    }

    // clang-format off
    SliceBatch           (const SliceBatch&)  = delete;
    SliceBatch           (      SliceBatch&&) = delete;
    SliceBatch& operator=(const SliceBatch&)  = delete;
    SliceBatch& operator=(      SliceBatch&&) = delete;
    // clang-format on

    ThreadSafeAtlasManager::ManagerGuard LockSlice(Uint32 Slice)
    {
        std::lock_guard<std::mutex> Guard{m_Mtx};

        auto it = m_Slices.find(Slice);
        // NB: Lock() atomically increases the use count of the slice while we hold the mutex.
        return it != m_Slices.end() ? it->second.Lock() : ThreadSafeAtlasManager::ManagerGuard{};
    }

    ThreadSafeAtlasManager::ManagerGuard LockSliceAfter(Uint32& Slice)
    {
        std::lock_guard<std::mutex> Guard{m_Mtx};

        auto it = m_Slices.lower_bound(Slice);
        if (it != m_Slices.end())
        {
            Slice = it->first;
            // NB: Lock() atomically increases the use count of the slice while we hold the mutex.
            return it->second.Lock();
        }

        return {};
    }

    ThreadSafeAtlasManager::ManagerGuard AddSlice(Uint32 Slice)
    {
        std::lock_guard<std::mutex> Guard{m_Mtx};

        VERIFY(m_Slices.find(Slice) == m_Slices.end(), "Slice ", Slice, " already present in the batch.");
        auto it = m_Slices.emplace(Slice, m_AtlasDim).first;
        // NB: Lock() atomically increases the use count of the slice while we hold the mutex.
        return it->second.Lock();
    }

    bool Purge(Uint32 Slice)
    {
        std::lock_guard<std::mutex> Guard{m_Mtx};

        auto it = m_Slices.find(Slice);
        if (it == m_Slices.end())
            return false;

        // Use count may only be incremented under the mutex. If the count is zero,
        // no other thread may be accessing this slice since we hold the mutex.
        if (it->second.GetUseCount() != 0)
            return false;

        // Check that the slice is empty. It is very important to check this only
        // after we checked the use count.
        // If the slice is empty, but the use count is not zero, another thread may
        // allocate from this slice after we checked if it is empty.
        auto SliceMgr = it->second.Lock();
        VERIFY_EXPR(SliceMgr);
        if (!SliceMgr.IsEmpty())
            return false;

        const auto UseCnt = SliceMgr.Release();
        VERIFY(UseCnt == 0, "There must be no other uses of this slice since we checked the use count already.");
        m_Slices.erase(it);

        return true;
    }

private:
    const uint2 m_AtlasDim;

    std::mutex m_Mtx;
    // For every alignment, we keep a list of slice managers sorted by the slice index.
    std::map<Uint32, ThreadSafeAtlasManager> m_Slices;
};

} // namespace


class DynamicTextureAtlasImpl final : public ObjectBase<IDynamicTextureAtlas>
{
public:
    using TBase = ObjectBase<IDynamicTextureAtlas>;

    DynamicTextureAtlasImpl(IReferenceCounters*                  pRefCounters,
                            IRenderDevice*                       pDevice,
                            const DynamicTextureAtlasCreateInfo& CreateInfo) :
        TBase{pRefCounters},
        m_Name{CreateInfo.Desc.Name != nullptr ? CreateInfo.Desc.Name : "Dynamic texture atlas"},
        m_Desc //
        {
            [this](TextureDesc TexDesc) //
            {
                TexDesc.Name = m_Name.c_str();
                return TexDesc;
            }(CreateInfo.Desc) //
        },
        // clang-format off
        m_MinAlignment    {CreateInfo.MinAlignment},
        m_ExtraSliceCount {CreateInfo.ExtraSliceCount},
        m_MaxSliceCount   {CreateInfo.Desc.Type == RESOURCE_DIM_TEX_2D_ARRAY ? std::min(CreateInfo.MaxSliceCount, Uint32{2048}) : 1},
        m_Silent          {CreateInfo.Silent},
        m_SuballocationsAllocator
        {
            DefaultRawMemoryAllocator::GetAllocator(),
            sizeof(TextureAtlasSuballocationImpl),
            1024u / Uint32{sizeof(TextureAtlasSuballocationImpl)} // Use 1KB pages.
        }
    // clang-format on
    {
        if (m_Desc.Type != RESOURCE_DIM_TEX_2D && m_Desc.Type != RESOURCE_DIM_TEX_2D_ARRAY)
            LOG_ERROR_AND_THROW(GetResourceDimString(m_Desc.Type), " is not a valid resource dimension. Only 2D and 2D array textures are allowed");

        if (m_Desc.Format == TEX_FORMAT_UNKNOWN)
            LOG_ERROR_AND_THROW("Texture format must not be UNKNOWN");

        if (m_Desc.Width == 0)
            LOG_ERROR_AND_THROW("Texture width must not be zero");

        if (m_Desc.Height == 0)
            LOG_ERROR_AND_THROW("Texture height must not be zero");

        if (m_MinAlignment != 0)
        {
            if (!IsPowerOfTwo(m_MinAlignment))
                LOG_ERROR_AND_THROW("Minimum alignment (", m_MinAlignment, ") is not a power of two");

            if ((m_Desc.Width % m_MinAlignment) != 0)
                LOG_ERROR_AND_THROW("Texture width (", m_Desc.Width, ") is not a multiple of minimum alignment (", m_MinAlignment, ")");

            if ((m_Desc.Height % m_MinAlignment) != 0)
                LOG_ERROR_AND_THROW("Texture height (", m_Desc.Height, ") is not a multiple of minimum alignment (", m_MinAlignment, ")");
        }

        for (Uint32 i = 0; i < m_MaxSliceCount; ++i)
            m_AvailableSlices.insert(i);

        m_TexArraySize.store(m_Desc.ArraySize);
        if (m_Desc.Type == RESOURCE_DIM_TEX_2D)
        {
            if (pDevice != nullptr)
            {
                pDevice->CreateTexture(m_Desc, nullptr, &m_pTexture);
                VERIFY_EXPR(m_pTexture);
            }
        }
        else
        {
            DynamicTextureArrayCreateInfo DynTexArrCI;
            DynTexArrCI.Desc                  = m_Desc;
            DynTexArrCI.NumSlicesInMemoryPage = m_ExtraSliceCount != 0 ? m_ExtraSliceCount : m_Desc.ArraySize;
            m_DynamicTexArray                 = std::make_unique<DynamicTextureArray>(pDevice, DynTexArrCI);
        }
    }

    ~DynamicTextureAtlasImpl()
    {
        VERIFY_EXPR(m_AllocatedArea.load() == 0);
        VERIFY_EXPR(m_UsedArea.load() == 0);
        VERIFY_EXPR(m_AllocationCount.load() == 0);
        VERIFY_EXPR(m_AvailableSlices.size() == m_MaxSliceCount);
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_DynamicTextureAtlas, TBase)

    virtual ITexture* GetTexture(IRenderDevice* pDevice, IDeviceContext* pContext) override final
    {
        if (m_DynamicTexArray)
        {
            Uint32 ArraySize = m_TexArraySize.load();
            if (m_DynamicTexArray->GetDesc().ArraySize != ArraySize)
            {
                m_DynamicTexArray->Resize(pDevice, pContext, ArraySize);
            }

            return m_DynamicTexArray->GetTexture(pDevice, pContext);
        }
        else
        {
            VERIFY_EXPR(m_Desc.Type == RESOURCE_DIM_TEX_2D);
            if (!m_pTexture)
            {
                DEV_CHECK_ERR(pDevice != nullptr, "Texture must be created, but pDevice is null");
                pDevice->CreateTexture(m_Desc, nullptr, &m_pTexture);
                DEV_CHECK_ERR(m_pTexture, "Failed to create atlas texture");
            }

            return m_pTexture;
        }
    }

    virtual Uint32 GetAllocationAlignment(Uint32 Width, Uint32 Height) const override final
    {
        return ComputeTextureAtlasSuballocationAlignment(Width, Height, m_MinAlignment);
    }

    virtual void Allocate(Uint32                       Width,
                          Uint32                       Height,
                          ITextureAtlasSuballocation** ppSuballocation) override final
    {
        if (Width == 0 || Height == 0)
        {
            UNEXPECTED("Subregion size must not be zero");
            return;
        }

        if (Width > m_Desc.Width || Height > m_Desc.Height)
        {
            LOG_ERROR_MESSAGE("Requested region size ", Width, " x ", Height, " exceeds atlas dimensions ", m_Desc.Width, " x ", m_Desc.Height);
            return;
        }

        const auto Alignment     = GetAllocationAlignment(Width, Height);
        const auto AlignedWidth  = AlignUp(Width, Alignment);
        const auto AlignedHeight = AlignUp(Height, Alignment);

        auto* pBatch = GetSliceBatch(Alignment, m_Desc.Width / Alignment, m_Desc.Height / Alignment);
        VERIFY_EXPR(pBatch != nullptr);

        DynamicAtlasManager::Region Subregion;

        Uint32 Slice = 0;
        while (Slice < m_MaxSliceCount)
        {
            // Lock the first available slice with index >= Slice
            auto SliceMgr = pBatch->LockSliceAfter(Slice);
            if (!SliceMgr)
            {
                const auto NewSlice = GetNextAvailableSlice();
                if (NewSlice != ~Uint32{0})
                {
                    Slice    = NewSlice;
                    SliceMgr = pBatch->AddSlice(Slice);
                    VERIFY_EXPR(SliceMgr);
                }
                else
                {
                    // It is possible that another thread added a new slice while this thread failed
                    SliceMgr = pBatch->LockSliceAfter(Slice);
                    if (!SliceMgr)
                        break;
                }
            }

            if (SliceMgr)
            {
                Subregion = SliceMgr.Allocate(AlignedWidth / Alignment, AlignedHeight / Alignment);
                if (!Subregion.IsEmpty())
                    break;
            }

            // Failed to allocate the region - try the next slice
            ++Slice;
        }

        if (Subregion.IsEmpty())
        {
            if (!m_Silent)
            {
                LOG_ERROR_MESSAGE("Failed to suballocate texture subregion ", Width, " x ", Height, " from texture atlas");
            }
            return;
        }

        m_AllocatedArea.fetch_add(Int64{Width} * Int64{Height});
        m_UsedArea.fetch_add(Int64{AlignedWidth} * Int64{AlignedHeight});
        m_AllocationCount.fetch_add(1);

        // clang-format off
        TextureAtlasSuballocationImpl* pSuballocation{
            NEW_RC_OBJ(m_SuballocationsAllocator, "TextureAtlasSuballocationImpl instance", TextureAtlasSuballocationImpl)
            (
                this,
                std::move(Subregion),
                Slice,
                Alignment,
                uint2{Width, Height}
            )
        };
        // clang-format on

        pSuballocation->QueryInterface(IID_TextureAtlasSuballocation, reinterpret_cast<IObject**>(ppSuballocation));
    }

    void Free(Uint32 Slice, Uint32 Alignment, DynamicAtlasManager::Region&& Subregion, Uint32 Width, Uint32 Height)
    {
        const auto AllocatedArea = Int64{Width} * Int64{Height};
        const auto UsedArea      = (Int64{Subregion.width} * Int64{Alignment}) * (Int64{Subregion.height} * Int64{Alignment});

        auto* pBatch = GetSliceBatch(Alignment);
        if (pBatch == nullptr)
        {
            UNEXPECTED("There are no slices with alignment ", Alignment,
                       ". This may only happen when double-freeing the allocation or "
                       "freeing an allocation that was not allocated from this atlas.");
            return;
        }

        if (auto SliceMgr = pBatch->LockSlice(Slice))
        {
            // NB: do not hold the slice batch mutex while releasing the region.
            //     Different slices in the batch can be processed in parallel.
            SliceMgr.Free(std::move(Subregion));
        }
        else
        {
            UNEXPECTED("Slice ", Slice, " is not found in the batch of slices with alignment ", Alignment);
            return;
        }

        // NB: we need to always purge the batch as the call to Free is not
        //     protected by the slice batch mutex, so other threads may have
        //     accessed and changed the same slice.
        //
        //           Thread 1                            |         Thread 2
        //                                               |
        //  SliceMgr = pBatch->LockSlice                 |
        //  | UseCnt==1                                  |   SliceMgr = pBatch->LockSlice
        //  | SliceMgr.Free                              |   | UseCnt==2
        //  |   SliceMgr.IsEmpty==false                  |   | SliceMgr.Free
        //  |                                            |   |   SliceMgr.IsEmpty==true
        //  |                                            |   | SliceMgr.~ManagerGuard()
        //  |                                            |   | UseCnt==1
        //  |                                            |
        //  |                                            |   pBatch->Purge
        //  |                                            |   | UseCnt==1 -> No purge
        //  |                                            |
        //  | SliceMgr.~ManagerGuard()                   |
        //  | UseCnt==0                                  |
        //                                               |
        //  pBatch->Purge                                |
        //  | UseCnt==0, SliceMgr.IsEmpty==true -> Purge |
        //
        // Note that in the scenario above, Thread 1 purges the slice batch even though
        // the slice was not empty after freeing the region.
        if (pBatch->Purge(Slice))
        {
            RecycleSlice(Slice);
        }

        m_AllocatedArea.fetch_add(-AllocatedArea);
        m_UsedArea.fetch_add(-UsedArea);
        m_AllocationCount.fetch_add(-1);
    }

    virtual const TextureDesc& GetAtlasDesc() const override final
    {
        return m_DynamicTexArray ? m_DynamicTexArray->GetDesc() : m_Desc;
    }

    virtual Uint32 GetVersion() const override final
    {
        if (m_DynamicTexArray)
            return m_DynamicTexArray->GetVersion();
        else if (m_pTexture)
            return 1;
        else
            return 0;
    }

    void GetUsageStats(DynamicTextureAtlasUsageStats& Stats) const override final
    {
        if (m_DynamicTexArray)
        {
            Stats.CommittedSize = m_DynamicTexArray->GetMemoryUsage();
            const auto& Desc{m_DynamicTexArray->GetDesc()};
            Stats.TotalArea = Uint64{Desc.Width} * Uint64{Desc.Height} * Uint64{Desc.ArraySize};
        }
        else
        {
            VERIFY_EXPR(m_Desc.Type == RESOURCE_DIM_TEX_2D);
            Stats.CommittedSize = 0;
            for (Uint32 mip = 0; mip < m_Desc.MipLevels; ++mip)
                Stats.CommittedSize += GetMipLevelProperties(m_Desc, mip).MipSize;
            Stats.TotalArea = Uint64{m_Desc.Width} * Uint64{m_Desc.Height};
        }

        Stats.AllocationCount = m_AllocationCount.load();
        Stats.AllocatedArea   = m_AllocatedArea.load();
        Stats.UsedArea        = m_UsedArea.load();
    }

private:
    Uint32 GetNextAvailableSlice()
    {
        std::lock_guard<std::mutex> Guard{m_AvailableSlicesMtx};
        if (m_AvailableSlices.empty())
            return ~Uint32{0};

        auto FirstFreeSlice = *m_AvailableSlices.begin();
        VERIFY_EXPR(FirstFreeSlice < m_MaxSliceCount);
        m_AvailableSlices.erase(m_AvailableSlices.begin());

        while (m_TexArraySize <= FirstFreeSlice)
        {
            const auto ExtraSliceCount = m_ExtraSliceCount != 0 ?
                m_ExtraSliceCount :
                std::max(m_TexArraySize.load(), 1u);

            m_TexArraySize.store(std::min(m_TexArraySize + ExtraSliceCount, m_MaxSliceCount));
        }

        return FirstFreeSlice;
    }

    void RecycleSlice(Uint32 Slice)
    {
        std::lock_guard<std::mutex> Guard{m_AvailableSlicesMtx};
        VERIFY(m_AvailableSlices.find(Slice) == m_AvailableSlices.end(), "Slice ", Slice, " is already in the available slices list. This is a bug.");
        m_AvailableSlices.insert(Slice);
    }

    SliceBatch* GetSliceBatch(Uint32 Alignment, Uint32 AtlasWidth = 0, Uint32 AtlasHeight = 0)
    {
        std::lock_guard<std::mutex> Guard{m_SliceBatchesByAlignmentMtx};

        // Get the list of slices for this alignment
        auto BatchIt = m_SliceBatchesByAlignment.find(Alignment);
        if (BatchIt == m_SliceBatchesByAlignment.end() && AtlasWidth != 0 && AtlasHeight != 0)
            BatchIt = m_SliceBatchesByAlignment.emplace(Alignment, uint2{AtlasWidth, AtlasHeight}).first;

        return BatchIt != m_SliceBatchesByAlignment.end() ? &BatchIt->second : nullptr;
    }

private:
    const std::string m_Name;
    const TextureDesc m_Desc;

    const Uint32 m_MinAlignment;
    const Uint32 m_ExtraSliceCount;
    const Uint32 m_MaxSliceCount;
    const bool   m_Silent;

    std::unique_ptr<DynamicTextureArray> m_DynamicTexArray;
    RefCntAutoPtr<ITexture>              m_pTexture;

    std::atomic<Uint32> m_TexArraySize{0};

    FixedBlockMemoryAllocator m_SuballocationsAllocator;

    std::atomic<Int32> m_AllocationCount{0};

    std::atomic<Int64> m_AllocatedArea{0};
    std::atomic<Int64> m_UsedArea{0};

    std::mutex m_SliceBatchesByAlignmentMtx;
    // Alignment -> slice batch
    std::unordered_map<Uint32, SliceBatch> m_SliceBatchesByAlignment;

    // Keep available slice indices sorted.
    std::mutex       m_AvailableSlicesMtx;
    std::set<Uint32> m_AvailableSlices;
};


TextureAtlasSuballocationImpl::~TextureAtlasSuballocationImpl()
{
    m_pParentAtlas->Free(m_Slice, m_Alignment, std::move(m_Subregion), m_Size.x, m_Size.y);
}

IDynamicTextureAtlas* TextureAtlasSuballocationImpl::GetAtlas()
{
    return m_pParentAtlas;
}

float4 TextureAtlasSuballocationImpl::GetUVScaleBias() const
{
    const auto  Origin    = GetOrigin().Recast<float>();
    const auto  Size      = GetSize().Recast<float>();
    const auto& AtlasDesc = m_pParentAtlas->GetAtlasDesc();
    return float4 //
        {
            Size.x / static_cast<float>(AtlasDesc.Width),
            Size.y / static_cast<float>(AtlasDesc.Height),
            Origin.x / static_cast<float>(AtlasDesc.Width),
            Origin.y / static_cast<float>(AtlasDesc.Height) //
        };
}

Uint32 ComputeTextureAtlasSuballocationAlignment(Uint32 Width, Uint32 Height, Uint32 MinAlignment)
{
    Uint32 Alignment = MinAlignment;
    if (Alignment > 0)
    {
        while (std::min(Width, Height) > Alignment)
            Alignment *= 2;
    }
    else
    {
        Alignment = 1;
    }
    return Alignment;
}

void CreateDynamicTextureAtlas(IRenderDevice*                       pDevice,
                               const DynamicTextureAtlasCreateInfo& CreateInfo,
                               IDynamicTextureAtlas**               ppAtlas)
{
    try
    {
        auto* pAllocator = MakeNewRCObj<DynamicTextureAtlasImpl>()(pDevice, CreateInfo);
        pAllocator->QueryInterface(IID_DynamicTextureAtlas, reinterpret_cast<IObject**>(ppAtlas));
    }
    catch (...)
    {
        LOG_ERROR_MESSAGE("Failed to create buffer suballocator");
    }
}

} // namespace Diligent
