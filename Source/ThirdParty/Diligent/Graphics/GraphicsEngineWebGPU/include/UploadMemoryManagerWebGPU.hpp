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

#pragma once

/// \file
/// Declaration of Diligent::UploadMemoryManagerWebGPU class

#include <mutex>
#include <vector>

#include "WebGPUObjectWrappers.hpp"
#include "BasicTypes.h"

namespace Diligent
{

// Upload memory is used by device context to upload data to GPU resources in:
// - UpdateBuffer
// - UpdateTexture
// - MapTextureSubresource
//
// The data is first written to the upload memory and the copy command is added to the command list.
// Upload data is flushed to the GPU memory before the command list is submitted to the queue.
class UploadMemoryManagerWebGPU
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
    };

    class Page
    {
    public:
        Page() noexcept = default;
        Page(UploadMemoryManagerWebGPU& Mgr, size_t Size);

        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;

        Page(Page&& RHS) noexcept;
        Page& operator=(Page&& RHS) noexcept;

        ~Page();

        Allocation Allocate(size_t Size, size_t Alignment = 16);

        void FlushWrites(WGPUQueue wgpuQueue);
        void Recycle();

        size_t GetSize() const
        {
            return m_Data.size();
        }

    private:
        UploadMemoryManagerWebGPU* m_pMgr = nullptr;
        WebGPUBufferWrapper        m_wgpuBuffer;
        std::vector<Uint8>         m_Data;
        size_t                     m_CurrOffset = 0;
    };

    UploadMemoryManagerWebGPU(WGPUDevice wgpuDevice, size_t PageSize);
    ~UploadMemoryManagerWebGPU();

    Page GetPage(size_t Size);

private:
    void RecyclePage(Page&& page);

private:
    const size_t m_PageSize;
    WGPUDevice   m_wgpuDevice;

    std::mutex        m_AvailablePagesMtx;
    std::vector<Page> m_AvailablePages;

#if DILIGENT_DEBUG
    std::atomic<uint32_t> m_DbgPageCounter{0};
#endif
};

} // namespace Diligent
