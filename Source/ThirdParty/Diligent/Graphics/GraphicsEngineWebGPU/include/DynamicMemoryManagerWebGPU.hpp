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

#pragma once

/// \file
/// Declaration of Diligent::DynamicMemoryManagerWebGPU class

#include <mutex>

#include "WebGPUObjectWrappers.hpp"
#include "BasicTypes.h"

namespace Diligent
{

// Dynamic memory manager provides dynamic memory allocations for dynamic buffers.
// The data is copied to the CPU memory and is flushed to the GPU memory before the
// command list is submitted to the queue.

class DynamicMemoryManagerWebGPU
{
public:
    struct Allocation
    {
        operator bool() const
        {
            return wgpuBuffer != nullptr;
        }

        WGPUBuffer wgpuBuffer = nullptr;
        size_t     Offset     = 0;
        size_t     Size       = 0;
        Uint8*     pData      = nullptr;
#ifdef DILIGENT_DEVELOPMENT
        Uint64 dvpFrameNumber = 0;
#endif
    };

    class Page
    {
    public:
        Page() noexcept = default;
        Page(DynamicMemoryManagerWebGPU* pMgr, size_t Size, size_t Offset);

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;

        Page(Page&& RHS) noexcept;
        Page& operator=(Page&& RHS) noexcept;

        ~Page();

        Allocation Allocate(size_t Size, size_t Alignment = 16);

        void FlushWrites(WGPUQueue wgpuQueue);
        void Recycle();

        size_t GetSize() const { return m_Size; }

    private:
        DynamicMemoryManagerWebGPU* m_pMgr = nullptr;

        size_t m_Size       = 0;
        size_t m_CurrOffset = 0;
        // Start offset in the buffer
        size_t m_BufferOffset = 0;
    };

    DynamicMemoryManagerWebGPU(WGPUDevice wgpuDevice, size_t PageSize, size_t BufferSize);

    ~DynamicMemoryManagerWebGPU();

    Page GetPage(size_t Size);

    WGPUBuffer GetWGPUBuffer() const
    {
        return m_wgpuBuffer;
    }

private:
    void RecyclePage(Page&& page);

private:
    const size_t        m_PageSize;
    const size_t        m_BufferSize;
    size_t              m_CurrentOffset = 0;
    WebGPUBufferWrapper m_wgpuBuffer;

    std::mutex         m_AvailablePagesMtx;
    std::vector<Page>  m_AvailablePages;
    std::vector<Uint8> m_MappedData;
};

} // namespace Diligent
