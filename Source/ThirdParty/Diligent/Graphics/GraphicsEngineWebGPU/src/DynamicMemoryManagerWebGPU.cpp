/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include "DynamicMemoryManagerWebGPU.hpp"
#include "Align.hpp"
#include "Cast.hpp"

namespace Diligent
{

DynamicMemoryManagerWebGPU::Page::Page(DynamicMemoryManagerWebGPU* _pMgr, size_t Size, size_t Offset) :
    m_pMgr{_pMgr},
    m_Size{Size},
    m_BufferOffset{Offset}
{
    VERIFY(IsPowerOfTwo(Size), "Page size must be power of two");
}

DynamicMemoryManagerWebGPU::Page::Page(Page&& RHS) noexcept :
    //clang-format off
    m_pMgr{RHS.m_pMgr},
    m_Size{RHS.m_Size},
    m_CurrOffset{RHS.m_CurrOffset},
    m_BufferOffset{RHS.m_BufferOffset}
// clang-format on
{
    RHS = Page{};
}

DynamicMemoryManagerWebGPU::Page& DynamicMemoryManagerWebGPU::Page::operator=(Page&& RHS) noexcept
{
    if (&RHS == this)
        return *this;

    m_pMgr         = RHS.m_pMgr;
    m_Size         = RHS.m_Size;
    m_CurrOffset   = RHS.m_CurrOffset;
    m_BufferOffset = RHS.m_BufferOffset;

    RHS.m_pMgr         = nullptr;
    RHS.m_Size         = 0;
    RHS.m_CurrOffset   = 0;
    RHS.m_BufferOffset = 0;

    return *this;
}

DynamicMemoryManagerWebGPU::Page::~Page()
{
    VERIFY(m_CurrOffset == 0, "Destroying a page that has not been recycled");
}

DynamicMemoryManagerWebGPU::Allocation DynamicMemoryManagerWebGPU::Page::Allocate(size_t Size, size_t Alignment)
{
    VERIFY(IsPowerOfTwo(Alignment), "Alignment size must be a power of two");

    size_t Offset    = AlignUp(m_CurrOffset, Alignment);
    size_t AllocSize = AlignUp(Size, Alignment);
    if (Offset + AllocSize <= m_Size)
    {
        size_t     MemoryOffset = m_BufferOffset + Offset;
        Allocation Alloc;
        Alloc.wgpuBuffer = m_pMgr->m_wgpuBuffer;
        Alloc.pData      = &m_pMgr->m_MappedData[MemoryOffset];
        Alloc.Offset     = MemoryOffset;
        Alloc.Size       = AllocSize;

        m_CurrOffset = Offset + AllocSize;
        return Alloc;
    }
    return Allocation{};
}

void DynamicMemoryManagerWebGPU::Page::FlushWrites(WGPUQueue wgpuQueue)
{
    if (m_CurrOffset > 0)
    {
        VERIFY_EXPR(m_pMgr != nullptr);
        wgpuQueueWriteBuffer(wgpuQueue, m_pMgr->m_wgpuBuffer, m_BufferOffset, &m_pMgr->m_MappedData[m_BufferOffset], m_CurrOffset);
    }
}

void DynamicMemoryManagerWebGPU::Page::Recycle()
{
    if (m_pMgr == nullptr)
    {
        UNEXPECTED("The page is empty.");
        return;
    }
    m_CurrOffset = 0;
    m_pMgr->RecyclePage(std::move(*this));
}

DynamicMemoryManagerWebGPU::DynamicMemoryManagerWebGPU(WGPUDevice wgpuDevice, size_t PageSize, size_t BufferSize) :
    m_PageSize{PageSize},
    m_BufferSize{BufferSize},
    m_CurrentOffset{0}
{
    WGPUBufferDescriptor wgpuBufferDesc{};
    wgpuBufferDesc.label = "Dynamic buffer";
    wgpuBufferDesc.size  = BufferSize;
    wgpuBufferDesc.usage =
        WGPUBufferUsage_CopyDst |
        WGPUBufferUsage_CopySrc |
        WGPUBufferUsage_Uniform |
        WGPUBufferUsage_Storage |
        WGPUBufferUsage_Vertex |
        WGPUBufferUsage_Index |
        WGPUBufferUsage_Indirect;
    m_wgpuBuffer.Reset(wgpuDeviceCreateBuffer(wgpuDevice, &wgpuBufferDesc));
    m_MappedData.resize(BufferSize);

    LOG_INFO_MESSAGE("Created dynamic buffer: ", BufferSize >> 10, " KB");
}

DynamicMemoryManagerWebGPU::~DynamicMemoryManagerWebGPU()
{
    LOG_INFO_MESSAGE("Dynamic memory manager usage stats:\n"
                     "                       Total size: ",
                     FormatMemorySize(m_BufferSize, 2),
                     ". Peak allocated size: ", FormatMemorySize(m_CurrentOffset, 2, m_BufferSize),
                     ". Peak utilization: ",
                     std::fixed, std::setprecision(1), static_cast<double>(m_CurrentOffset) / static_cast<double>(std::max(m_BufferSize, size_t{1})) * 100.0, '%');
}

DynamicMemoryManagerWebGPU::Page DynamicMemoryManagerWebGPU::GetPage(size_t Size)
{
    size_t PageSize = m_PageSize;
    while (PageSize < Size)
        PageSize *= 2;

    std::lock_guard Lock{m_AvailablePagesMtx};
    auto            Iter = m_AvailablePages.begin();
    while (Iter != m_AvailablePages.end())
    {
        if (PageSize <= Iter->GetSize())
        {
            auto Result = std::move(*Iter);
            m_AvailablePages.erase(Iter);
            return Result;
        }
        ++Iter;
    }

    if (m_CurrentOffset + PageSize > m_BufferSize)
    {
        LOG_ERROR("Requested dynamic allocation size ", m_CurrentOffset + PageSize, " exceeds maximum dynamic memory size ", m_BufferSize, ". The app should increase dynamic heap size.");
        return Page{};
    }

    size_t Offset = m_CurrentOffset;
    m_CurrentOffset += PageSize;

    return Page{this, PageSize, Offset};
}

void DynamicMemoryManagerWebGPU::RecyclePage(Page&& Item)
{
    std::lock_guard Lock{m_AvailablePagesMtx};
    m_AvailablePages.emplace_back(std::move(Item));
}

} // namespace Diligent
