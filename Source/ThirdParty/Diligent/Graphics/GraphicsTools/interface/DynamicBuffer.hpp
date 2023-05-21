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
/// Declaration of a DynamicBuffer class

#include <atomic>

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../GraphicsEngine/interface/DeviceMemory.h"
#include "../../GraphicsEngine/interface/Fence.h"
#include "../../../Common/interface/RefCntAutoPtr.hpp"

namespace Diligent
{

/// Dynamic buffer create information.
struct DynamicBufferCreateInfo
{
    /// Buffer description.
    BufferDesc Desc;

    /// The size of the memory page for the sparse buffer.

    /// \remarks
    ///     This value is only relevant when Desc.Usage == USAGE_SPARSE and
    ///     defines the memory page size of the device memory object that is
    ///     backing the buffer.
    ///
    ///     Memory page size should be a multiple of SparseResources.StandardBlockSize.
    ///     If it is not, the engine automatically aligns the value up to the closest
    ///     multiple of the block size.
    Uint32 MemoryPageSize = 64 << 10;

    /// When Desc.Usage == USAGE_SPARSE, the virtual size of the sparse buffer;
    /// ignored otherwise.
    Uint64 VirtualSize = Uint64{1} << Uint64{30};
};


/// Dynamically resizable buffer
class DynamicBuffer
{
public:
    /// Initializes the dynamic buffer.

    /// \param[in] pDevice    - Render device that will be used to create the buffer.
    ///                         This parameter may be null (see remarks).
    /// \param[in] CreateInfo - Create information, see Diligent::DynamicBufferCreateInfo.
    ///
    /// \remarks            If pDevice is null, internal buffer creation will be postponed
    ///                     until GetBuffer() or Resize() is called.
    DynamicBuffer(IRenderDevice* pDevice, const DynamicBufferCreateInfo& CreateInfo);

    // clang-format off
    DynamicBuffer           (const DynamicBuffer&)  = delete;
    DynamicBuffer& operator=(const DynamicBuffer&)  = delete;
    DynamicBuffer           (      DynamicBuffer&&) = delete;
    DynamicBuffer& operator=(      DynamicBuffer&&) = delete;
    // clang-format on


    /// Resizes the buffer to the new size.

    /// \param[in] pDevice        - Render device that will be used create the new internal buffer.
    ///                             This parameter may be null (see remarks).
    /// \param[in] pContext       - Device context that will be used to copy existing contents
    ///                             to the new buffer (for non-sparse buffer), or commit new memory
    ///                             pages (for sparse buffer). This parameter may be null (see remarks).
    /// \param[in] NewSize        - New buffer size. Can be zero.
    /// \param[in] DiscardContent - Whether to discard previous buffer content.
    ///
    /// \return     Pointer to the new buffer.
    ///
    /// \remarks    The method operation depends on which of pDevice and pContext parameters
    ///             are not null:
    ///             - Both pDevice and pContext are not null: the new internal buffer is created
    ///               and existing contents is copied (for non-sparse buffer), or memory pages
    ///               are committed (for sparse buffer). GetBuffer() may be called with
    ///               both pDevice and pContext being null.
    ///             - pDevice is not null, pContext is null: internal buffer is created,
    ///               but existing contents is not copied or memory is not committed. An
    ///               application must provide non-null device context when calling GetBuffer().
    ///             - Both pDevice and pContext are null: internal buffer is not created.
    ///               An application must provide non-null device and device context when calling
    ///               GetBuffer().
    ///
    ///             Typically pDevice and pContext should be null when the method is called from a worker thread.
    ///
    ///             If NewSize is zero, internal buffer will be released.
    IBuffer* Resize(IRenderDevice*  pDevice,
                    IDeviceContext* pContext,
                    Uint64          NewSize,
                    bool            DiscardContent = false);


    /// Returns the pointer to the buffer object, initializing it if necessary.

    /// \param[in] pDevice  - Render device that will be used to create the new buffer,
    ///                       if necessary (see remarks).
    /// \param[in] pContext - Device context that will be used to copy existing
    ///                       buffer contents or commit memory pages, if necessary
    ///                       (see remarks).
    /// \return               The pointer to the buffer object.
    ///
    /// \remarks    If the buffer has been resized, but internal buffer object has not been
    ///             initialized, pDevice and pContext must not be null.
    ///
    ///             If buffer does not need to be updated (PendingUpdate() returns false),
    ///             both pDevice and pContext may be null.
    IBuffer* GetBuffer(IRenderDevice*  pDevice,
                       IDeviceContext* pContext);


    /// Returns true if the buffer must be updated before use (e.g. it has been resized,
    /// but internal buffer has not been initialized or updated).
    /// When update is not pending, GetBuffer() may be called with null device and context.
    bool PendingUpdate() const
    {
        return m_PendingSize != m_Desc.Size;
    }


    /// Returns the buffer description.
    const BufferDesc& GetDesc() const
    {
        return m_Desc;
    }


    /// Returns the dynamic buffer version.
    /// The version is incremented every time a new internal buffer is created.
    Uint32 GetVersion() const
    {
        return m_Version.load();
    }

private:
    void InitBuffer(IRenderDevice* pDevice);
    void CreateSparseBuffer(IRenderDevice* pDevice);

    void CommitResize(IRenderDevice*  pDevice,
                      IDeviceContext* pContext,
                      bool            AllowNull);
    void ResizeSparseBuffer(IDeviceContext* pContext);
    void ResizeDefaultBuffer(IDeviceContext* pContext);

    const std::string m_Name;
    BufferDesc        m_Desc;

    std::atomic<Uint32> m_Version{0};

    RefCntAutoPtr<IBuffer>       m_pBuffer;
    RefCntAutoPtr<IBuffer>       m_pStaleBuffer;
    RefCntAutoPtr<IDeviceMemory> m_pMemory;

    Uint64 m_PendingSize = 0;
    Uint64 m_VirtualSize = 0;

    Uint32 m_MemoryPageSize = 0;

    Uint64 m_NextBeforeResizeFenceValue = 1;
    Uint64 m_NextAfterResizeFenceValue  = 1;
    Uint64 m_LastAfterResizeFenceValue  = 0;

    RefCntAutoPtr<IFence> m_pBeforeResizeFence;
    RefCntAutoPtr<IFence> m_pAfterResizeFence;
};

} // namespace Diligent
