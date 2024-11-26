/*
 *  Copyright 2023-2024 Diligent Graphics LLC
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

#include "Cast.hpp"
#include "Align.hpp"
#include "UploadMemoryManagerWebGPU.hpp"
#include "DebugUtilities.hpp"

namespace Diligent
{

UploadMemoryManagerWebGPU::Page::Page(UploadMemoryManagerWebGPU& Mgr, size_t Size) :
    m_pMgr{&Mgr},
    m_Data(Size)
{
    WGPUBufferDescriptor wgpuBufferDesc{};
    wgpuBufferDesc.label = "Upload memory page";
    wgpuBufferDesc.size  = Size;
    wgpuBufferDesc.usage =
        WGPUBufferUsage_CopyDst |
        WGPUBufferUsage_CopySrc |
        WGPUBufferUsage_Uniform |
        WGPUBufferUsage_Storage |
        WGPUBufferUsage_Vertex |
        WGPUBufferUsage_Index |
        WGPUBufferUsage_Indirect;
    m_wgpuBuffer.Reset(wgpuDeviceCreateBuffer(m_pMgr->m_wgpuDevice, &wgpuBufferDesc));
    LOG_INFO_MESSAGE("Created a new upload memory page, size: ", FormatMemorySize(Size));
}

UploadMemoryManagerWebGPU::Page::Page(Page&& RHS) noexcept :
    //clang-format off
    m_pMgr{RHS.m_pMgr},
    m_wgpuBuffer{std::move(RHS.m_wgpuBuffer)},
    m_Data{std::move(RHS.m_Data)},
    m_CurrOffset{RHS.m_CurrOffset}
// clang-format on
{
    RHS = Page{};
}

UploadMemoryManagerWebGPU::Page& UploadMemoryManagerWebGPU::Page::operator=(Page&& RHS) noexcept
{
    if (&RHS == this)
        return *this;

    m_pMgr       = RHS.m_pMgr;
    m_wgpuBuffer = std::move(RHS.m_wgpuBuffer);
    m_Data       = std::move(RHS.m_Data);
    m_CurrOffset = RHS.m_CurrOffset;

    RHS.m_pMgr       = nullptr;
    RHS.m_CurrOffset = 0;

    return *this;
}

UploadMemoryManagerWebGPU::Page::~Page()
{
    VERIFY(m_CurrOffset == 0, "Destroying a page that has not been recycled");
}

UploadMemoryManagerWebGPU::Allocation UploadMemoryManagerWebGPU::Page::Allocate(size_t Size, size_t Alignment)
{
    VERIFY(IsPowerOfTwo(Alignment), "Alignment size must be a power of two");
    Allocation Alloc;
    Alloc.Offset = AlignUp(m_CurrOffset, Alignment);
    Alloc.Size   = AlignUp(Size, Alignment);
    if (Alloc.Offset + Alloc.Size <= m_Data.size())
    {
        Alloc.wgpuBuffer = m_wgpuBuffer;
        Alloc.pData      = &m_Data[Alloc.Offset];
        m_CurrOffset     = Alloc.Offset + Alloc.Size;
        return Alloc;
    }
    return Allocation{};
}

void UploadMemoryManagerWebGPU::Page::FlushWrites(WGPUQueue wgpuQueue)
{
    if (m_CurrOffset > 0)
    {
        wgpuQueueWriteBuffer(wgpuQueue, m_wgpuBuffer, 0, m_Data.data(), m_CurrOffset);
    }
}

void UploadMemoryManagerWebGPU::Page::Recycle()
{
    if (m_pMgr == nullptr)
    {
        UNEXPECTED("The page is empty.");
        return;
    }

    m_CurrOffset = 0;
    m_pMgr->RecyclePage(std::move(*this));
}


UploadMemoryManagerWebGPU::UploadMemoryManagerWebGPU(WGPUDevice wgpuDevice, size_t PageSize) :
    m_PageSize{PageSize},
    m_wgpuDevice{wgpuDevice}
{
    VERIFY(IsPowerOfTwo(m_PageSize), "Page size must be power of two");
}

UploadMemoryManagerWebGPU::~UploadMemoryManagerWebGPU()
{
    VERIFY(m_DbgPageCounter == m_AvailablePages.size(),
           "Not all pages have been recycled. This may result in a crash if the page is recycled later.");
    size_t TotalSize = 0;
    for (const Page& page : m_AvailablePages)
        TotalSize += page.GetSize();
    LOG_INFO_MESSAGE("SharedMemoryManagerWebGPU: total allocated memory: ", FormatMemorySize(TotalSize));
}

UploadMemoryManagerWebGPU::Page UploadMemoryManagerWebGPU::GetPage(size_t Size)
{
    size_t PageSize = m_PageSize;
    while (PageSize < Size)
        PageSize *= 2;

    {
        std::lock_guard Lock{m_AvailablePagesMtx};

        auto Iter = m_AvailablePages.begin();
        while (Iter != m_AvailablePages.end())
        {
            if (PageSize <= Iter->GetSize())
            {
                Page Result = std::move(*Iter);
                m_AvailablePages.erase(Iter);
                return Result;
            }
            ++Iter;
        }
    }

#if DILIGENT_DEBUG
    m_DbgPageCounter.fetch_add(1);
#endif
    return Page{*this, PageSize};
}

void UploadMemoryManagerWebGPU::RecyclePage(Page&& Item)
{
    std::lock_guard Lock{m_AvailablePagesMtx};
    m_AvailablePages.emplace_back(std::move(Item));
}

} // namespace Diligent
