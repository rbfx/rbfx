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
/// Declaration of BufferSuballocator interface and related data structures

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Buffer.h"

namespace Diligent
{

struct IBufferSuballocator;

// {562552DA-67F0-40C2-A4AF-F286DFCA1626}
static const INTERFACE_ID IID_BufferSuballocation =
    {0x562552da, 0x67f0, 0x40c2, {0xa4, 0xaf, 0xf2, 0x86, 0xdf, 0xca, 0x16, 0x26}};


// {71F59B50-7D13-49A7-A4F7-FC986715FFAC}
static const INTERFACE_ID IID_BufferSuballocator =
    {0x71f59b50, 0x7d13, 0x49a7, {0xa4, 0xf7, 0xfc, 0x98, 0x67, 0x15, 0xff, 0xac}};


/// Buffer suballocation.
struct IBufferSuballocation : public IObject
{
    /// Returns the start offset of the suballocation.
    virtual Uint32 GetOffset() const = 0;

    /// Returns the suballocation size.
    virtual Uint32 GetSize() const = 0;

    /// Returns the pointer to the parent allocator.
    virtual IBufferSuballocator* GetAllocator() = 0;

    /// Stores a pointer to the user-provided data object, which
    /// may later be retrieved through GetUserData().
    ///
    /// \param [in] pUserData - Pointer to the user data object to store.
    ///
    /// \note   The method is not thread-safe and an application
    ///         must externally synchronize the access.
    virtual void SetUserData(IObject* pUserData) = 0;

    /// Returns the pointer to the user data object previously
    /// set with SetUserData() method.
    ///
    /// \return     Pointer to the user data object
    virtual IObject* GetUserData() const = 0;
};


/// Buffer suballocator usage stats.
struct BufferSuballocatorUsageStats
{
    /// The size of the internal buffer, in bytes.
    Uint64 Size = 0;

    /// The total used size, in bytes.
    Uint64 UsedSize = 0;

    /// The maximum size of the continuous free chunk in the buffer, in bytes.
    Uint64 MaxFreeChunkSize = 0;

    /// The current number of allocations.
    Uint32 AllocationCount = 0;
};

/// Buffer suballocator.
struct IBufferSuballocator : public IObject
{
    /// Returns the pointer to the internal buffer object.

    /// \param[in]  pDevice  - Pointer to the render device that will be used to
    ///                        create a new internal buffer, if necessary.
    /// \param[in]  pContext - Pointer to the device context that will be used to
    ///                        copy existing contents to the new buffer, if necessary.
    ///
    /// \remarks    If the internal buffer needs to be resized, pDevice and pContext will
    ///             be used to create a new buffer and copy existing contents to the new buffer.
    ///             The method is not thread-safe and an application must externally synchronize the
    ///             access.
    virtual IBuffer* GetBuffer(IRenderDevice* pDevice, IDeviceContext* pContext) = 0;


    /// Performs suballocation from the buffer.

    /// \param[in]  Size            - Suballocation size.
    /// \param[in]  Alignment       - Required alignment.
    /// \param[out] ppSuballocation - Memory location where pointer to the new suballocation will be
    ///                               stored.
    ///
    /// \remarks    The method is thread-safe and can be called from multiple threads simultaneously.
    virtual void Allocate(Uint32                 Size,
                          Uint32                 Alignment,
                          IBufferSuballocation** ppSuballocation) = 0;


    /// Returns the suballocator usage stats, see Diligent::BufferSuballocatorUsageStats.
    virtual void GetUsageStats(BufferSuballocatorUsageStats& UsageStats) = 0;


    /// Returns internal buffer version. The version is incremented every time
    /// the buffer is expanded.
    virtual Uint32 GetVersion() const = 0;
};

/// Buffer suballocator create information.
struct BufferSuballocatorCreateInfo
{
    /// Buffer description
    BufferDesc Desc;


    /// Buffer expansion size, in bytes.

    /// When non-zero, the buffer will be expanded by the specified amount every time
    /// there is insufficient space. If zero, the buffer size will be doubled when
    /// more space is needed.
    Uint32 ExpansionSize = 0;


    /// Allocation granularity for IBufferSuballocator objects.

    /// Buffer suballocator uses FixedBlockMemoryAllocator to allocate instances
    /// of IBufferSuballocation implementation class. This member defines
    /// the number of objects in one page.
    Uint32 SuballocationObjAllocationGranularity = 64;


    /// If Desc.Usage == USAGE_SPARSE, the irtual buffer size; ignored otherwise.
    Uint64 VirtualSize = 0;
};

/// Creates a new buffer suballocator.

/// \param[in]  pDevice              - Pointer to the render device that will be used to initialize
///                                    internal buffer object. If this parameter is null, the
///                                    buffer will be created when GetBuffer() is called.
/// \param[in]  CreateInfo           - Suballocator create info, see Diligent::BufferSuballocatorCreateInfo.
/// \param[in]  ppBufferSuballocator - Memory location where pointer to the buffer suballocator will be stored.
void CreateBufferSuballocator(IRenderDevice*                      pDevice,
                              const BufferSuballocatorCreateInfo& CreateInfo,
                              IBufferSuballocator**               ppBufferSuballocator);

} // namespace Diligent
