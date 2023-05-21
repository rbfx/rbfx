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

#include "pch.h"
#include <algorithm>
#include "FixedBlockMemoryAllocator.hpp"
#include "Align.hpp"

namespace Diligent
{

#ifdef DILIGENT_DEBUG
inline void FillWithDebugPattern(void* ptr, Uint8 Pattern, size_t NumBytes)
{
    memset(ptr, Pattern, NumBytes);
}
#else
#    define FillWithDebugPattern(...)
#endif

FixedBlockMemoryAllocator::MemoryPage::MemoryPage(FixedBlockMemoryAllocator& OwnerAllocator) :
    // clang-format off
    m_NumFreeBlocks       {OwnerAllocator.m_NumBlocksInPage},
    m_NumInitializedBlocks{0},
    m_pOwnerAllocator     {&OwnerAllocator}
// clang-format on
{
    const auto PageSize = OwnerAllocator.m_BlockSize * OwnerAllocator.m_NumBlocksInPage;
    VERIFY_EXPR(PageSize > 0);
    m_pPageStart = reinterpret_cast<Uint8*>(
        OwnerAllocator.m_RawMemoryAllocator.Allocate(PageSize, "FixedBlockMemoryAllocator page", __FILE__, __LINE__));
    m_pNextFreeBlock = m_pPageStart;
    FillWithDebugPattern(m_pPageStart, NewPageMemPattern, PageSize);
}

FixedBlockMemoryAllocator::MemoryPage::MemoryPage(MemoryPage&& Page) noexcept :
    // clang-format off
    m_NumFreeBlocks       {Page.m_NumFreeBlocks       },
    m_NumInitializedBlocks{Page.m_NumInitializedBlocks},
    m_pPageStart          {Page.m_pPageStart          },
    m_pNextFreeBlock      {Page.m_pNextFreeBlock      },
    m_pOwnerAllocator     {Page.m_pOwnerAllocator     }
// clang-format on
{
    Page.m_NumFreeBlocks        = 0;
    Page.m_NumInitializedBlocks = 0;
    Page.m_pPageStart           = nullptr;
    Page.m_pNextFreeBlock       = nullptr;
    Page.m_pOwnerAllocator      = nullptr;
}

FixedBlockMemoryAllocator::MemoryPage::~MemoryPage()
{
    if (m_pOwnerAllocator)
        m_pOwnerAllocator->m_RawMemoryAllocator.Free(m_pPageStart);
}

void* FixedBlockMemoryAllocator::MemoryPage::GetBlockStartAddress(Uint32 BlockIndex) const
{
    VERIFY_EXPR(m_pOwnerAllocator != nullptr);
    VERIFY(BlockIndex < m_pOwnerAllocator->m_NumBlocksInPage, "Invalid block index");
    return reinterpret_cast<Uint8*>(m_pPageStart) + BlockIndex * m_pOwnerAllocator->m_BlockSize;
}

#ifdef DILIGENT_DEBUG
void FixedBlockMemoryAllocator::MemoryPage::dbgVerifyAddress(const void* pBlockAddr) const
{
    size_t Delta = reinterpret_cast<const Uint8*>(pBlockAddr) - reinterpret_cast<Uint8*>(m_pPageStart);
    VERIFY(Delta % m_pOwnerAllocator->m_BlockSize == 0, "Invalid address");
    Uint32 BlockIndex = static_cast<Uint32>(Delta / m_pOwnerAllocator->m_BlockSize);
    VERIFY(BlockIndex < m_pOwnerAllocator->m_NumBlocksInPage, "Invalid block index");
}
#else
#    define dbgVerifyAddress(...)
#endif

void* FixedBlockMemoryAllocator::MemoryPage::Allocate()
{
    VERIFY_EXPR(m_pOwnerAllocator != nullptr);

    if (m_NumFreeBlocks == 0)
    {
        VERIFY_EXPR(m_NumInitializedBlocks == m_pOwnerAllocator->m_NumBlocksInPage);
        return nullptr;
    }

    // Initialize the next block
    if (m_NumInitializedBlocks < m_pOwnerAllocator->m_NumBlocksInPage)
    {
        // Link next uninitialized block to the end of the list:

        //
        //                            ___________                      ___________
        //                           |           |                    |           |
        //                           | 0xcdcdcd  |                 -->| 0xcdcdcd  |   m_NumInitializedBlocks
        //                           |-----------|                |   |-----------|
        //                           |           |                |   |           |
        //  m_NumInitializedBlocks   | 0xcdcdcd  |      ==>        ---|           |
        //                           |-----------|                    |-----------|
        //
        //                           ~           ~                    ~           ~
        //                           |           |                    |           |
        //                       0   |           |                    |           |
        //                            -----------                      -----------
        //
        auto* pUninitializedBlock = GetBlockStartAddress(m_NumInitializedBlocks);
        FillWithDebugPattern(pUninitializedBlock, InitializedBlockMemPattern, m_pOwnerAllocator->m_BlockSize);
        void** ppNextBlock = reinterpret_cast<void**>(pUninitializedBlock);
        ++m_NumInitializedBlocks;
        if (m_NumInitializedBlocks < m_pOwnerAllocator->m_NumBlocksInPage)
            *ppNextBlock = GetBlockStartAddress(m_NumInitializedBlocks);
        else
            *ppNextBlock = nullptr;
    }

    void* res = m_pNextFreeBlock;
    dbgVerifyAddress(res);
    // Move pointer to the next free block
    m_pNextFreeBlock = *reinterpret_cast<void**>(m_pNextFreeBlock);
    --m_NumFreeBlocks;
    if (m_NumFreeBlocks != 0)
        dbgVerifyAddress(m_pNextFreeBlock);
    else
        VERIFY_EXPR(m_pNextFreeBlock == nullptr);

    FillWithDebugPattern(res, AllocatedBlockMemPattern, m_pOwnerAllocator->m_BlockSize);
    return res;
}

void FixedBlockMemoryAllocator::MemoryPage::DeAllocate(void* p)
{
    VERIFY_EXPR(m_pOwnerAllocator != nullptr);

    dbgVerifyAddress(p);
    FillWithDebugPattern(p, DeallocatedBlockMemPattern, m_pOwnerAllocator->m_BlockSize);
    // Add block to the beginning of the linked list
    *reinterpret_cast<void**>(p) = m_pNextFreeBlock;
    m_pNextFreeBlock             = p;
    ++m_NumFreeBlocks;
}


static size_t AdjustBlockSize(size_t BlockSize)
{
    return AlignUp(BlockSize, sizeof(void*));
}

FixedBlockMemoryAllocator::FixedBlockMemoryAllocator(IMemoryAllocator& RawMemoryAllocator,
                                                     size_t            BlockSize,
                                                     Uint32            NumBlocksInPage) :
    // clang-format off
    m_PagePool          (STD_ALLOCATOR_RAW_MEM(MemoryPage, RawMemoryAllocator, "Allocator for vector<MemoryPage>")),
    m_AvailablePages    (STD_ALLOCATOR_RAW_MEM(size_t, RawMemoryAllocator, "Allocator for unordered_set<size_t>") ),
    m_AddrToPageId      (STD_ALLOCATOR_RAW_MEM(AddrToPageIdMapElem, RawMemoryAllocator, "Allocator for unordered_map<void*, size_t>")),
    m_RawMemoryAllocator{RawMemoryAllocator        },
    m_BlockSize         {AdjustBlockSize(BlockSize)},
    m_NumBlocksInPage   {NumBlocksInPage           }
// clang-format on
{
    // Allocate one page
    if (m_BlockSize > 0)
    {
        CreateNewPage();
    }
}

FixedBlockMemoryAllocator::~FixedBlockMemoryAllocator()
{
#ifdef DILIGENT_DEBUG
    for (size_t p = 0; p < m_PagePool.size(); ++p)
    {
        VERIFY(!m_PagePool[p].HasAllocations(), "Memory leak detected: memory page has allocated block");
        VERIFY(m_AvailablePages.find(p) != m_AvailablePages.end(), "Memory page is not in the available page pool");
    }
#endif
}

void FixedBlockMemoryAllocator::CreateNewPage()
{
    VERIFY_EXPR(m_BlockSize > 0);
    m_PagePool.emplace_back(*this);
    m_AvailablePages.insert(m_PagePool.size() - 1);
    m_AddrToPageId.reserve(m_PagePool.size() * m_NumBlocksInPage);
}

void* FixedBlockMemoryAllocator::Allocate(size_t Size, const Char* dbgDescription, const char* dbgFileName, const Int32 dbgLineNumber)
{
    VERIFY_EXPR(Size > 0);

    Size = AdjustBlockSize(Size);
    VERIFY(m_BlockSize == Size, "Requested size (", Size, ") does not match the block size (", m_BlockSize, ")");

    std::lock_guard<std::mutex> LockGuard(m_Mutex);

    if (m_AvailablePages.empty())
    {
        CreateNewPage();
    }

    auto  PageId = *m_AvailablePages.begin();
    auto& Page   = m_PagePool[PageId];
    auto* Ptr    = Page.Allocate();
    m_AddrToPageId.insert(std::make_pair(Ptr, PageId));
    if (!Page.HasSpace())
    {
        m_AvailablePages.erase(m_AvailablePages.begin());
    }

    return Ptr;
}

void FixedBlockMemoryAllocator::Free(void* Ptr)
{
    std::lock_guard<std::mutex> LockGuard(m_Mutex);
    auto                        PageIdIt = m_AddrToPageId.find(Ptr);
    if (PageIdIt != m_AddrToPageId.end())
    {
        auto PageId = PageIdIt->second;
        VERIFY_EXPR(PageId < m_PagePool.size());
        m_PagePool[PageId].DeAllocate(Ptr);
        m_AvailablePages.insert(PageId);
        m_AddrToPageId.erase(PageIdIt);
        if (m_AvailablePages.size() > 1 && !m_PagePool[PageId].HasAllocations())
        {
            // In current implementation pages are never released!
            // Note that if we delete a page, all indices past it will be invalid

            //m_PagePool.erase(m_PagePool.begin() + PageId);
            //m_AvailablePages.erase(PageId);
        }
    }
    else
    {
        UNEXPECTED("Address not found in the allocations list - double freeing memory?");
    }
}

} // namespace Diligent
