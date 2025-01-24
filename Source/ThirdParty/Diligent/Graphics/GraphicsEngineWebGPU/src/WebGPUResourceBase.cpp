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

#include "WebGPUResourceBase.hpp"
#include "Align.hpp"

namespace Diligent
{

WebGPUResourceBase::WebGPUResourceBase(IDeviceObject& Owner, size_t MaxPendingBuffers) :
    m_Owner{Owner}
{
    m_StagingBuffers.reserve(MaxPendingBuffers);
}

WebGPUResourceBase::~WebGPUResourceBase()
{
}

WebGPUResourceBase::StagingBufferInfo* WebGPUResourceBase::FindStagingWriteBuffer(WGPUDevice wgpuDevice)
{
    if (m_StagingBuffers.empty())
    {
        String StagingBufferName = "Staging write buffer for '";
        StagingBufferName += m_Owner.GetDesc().Name;
        StagingBufferName += '\'';

        WGPUBufferDescriptor wgpuBufferDesc{};
        wgpuBufferDesc.label            = StagingBufferName.c_str();
        wgpuBufferDesc.size             = AlignUp(m_MappedData.size(), MappedRangeAlignment);
        wgpuBufferDesc.usage            = WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc;
        wgpuBufferDesc.mappedAtCreation = true;

        WebGPUBufferWrapper wgpuBuffer{wgpuDeviceCreateBuffer(wgpuDevice, &wgpuBufferDesc)};
        if (!wgpuBuffer)
        {
            LOG_ERROR("Failed to create WebGPU buffer '", StagingBufferName, '\'');
            return nullptr;
        }

        m_StagingBuffers.emplace_back(StagingBufferInfo{
            *this,
            std::move(wgpuBuffer),
        });
    }

    return &m_StagingBuffers.back();
}

WebGPUResourceBase::StagingBufferInfo* WebGPUResourceBase::FindStagingReadBuffer(WGPUDevice wgpuDevice)
{
    for (StagingBufferInfo& BufferInfo : m_StagingBuffers)
    {
        if (wgpuBufferGetMapState(BufferInfo.wgpuBuffer) == WGPUBufferMapState_Unmapped)
        {
            if (BufferInfo.pSyncPoint->IsTriggered())
            {
                // Create a new sync point since the old one can still be referenced by fences
                BufferInfo.pSyncPoint = MakeNewRCObj<SyncPointWebGPUImpl>()();
            }
            return &BufferInfo;
        }
    }

    if (m_StagingBuffers.size() == m_StagingBuffers.capacity())
    {
        LOG_ERROR("Unable to create a new staging read buffer: limit of ", m_StagingBuffers.capacity(), " buffers is reached");
        return nullptr;
    }

    String StagingBufferName = "Staging read buffer for '";
    StagingBufferName += m_Owner.GetDesc().Name;
    StagingBufferName += '\'';

    WGPUBufferDescriptor wgpuBufferDesc{};
    wgpuBufferDesc.label = StagingBufferName.c_str();
    wgpuBufferDesc.size  = AlignUp(m_MappedData.size(), MappedRangeAlignment);
    wgpuBufferDesc.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;

    WebGPUBufferWrapper wgpuBuffer{wgpuDeviceCreateBuffer(wgpuDevice, &wgpuBufferDesc)};
    if (!wgpuBuffer)
    {
        LOG_ERROR("Failed to create WebGPU buffer '", StagingBufferName, '\'');
        return nullptr;
    }

    m_StagingBuffers.emplace_back(
        StagingBufferInfo{
            *this,
            std::move(wgpuBuffer),
            RefCntAutoPtr<SyncPointWebGPUImpl>{MakeNewRCObj<SyncPointWebGPUImpl>()()},
        });
    return &m_StagingBuffers.back();
}

WebGPUResourceBase::StagingBufferInfo* WebGPUResourceBase::GetStagingBuffer(WGPUDevice wgpuDevice, CPU_ACCESS_FLAGS Access)
{
    VERIFY(m_StagingBuffers.capacity() != 0, "Resource is not initialized as staging");
    VERIFY(Access == CPU_ACCESS_READ || Access == CPU_ACCESS_WRITE, "Read or write access is expected");
    return Access == CPU_ACCESS_READ ?
        FindStagingReadBuffer(wgpuDevice) :
        FindStagingWriteBuffer(wgpuDevice);
}

void* WebGPUResourceBase::Map(MAP_TYPE MapType, Uint64 Offset)
{
    VERIFY(m_MapState == MapState::None, "Resource is already mapped");
    VERIFY(Offset < m_MappedData.size(), "Offset (", Offset, ") exceeds the mapped data size (", m_MappedData.size(), ")");

    if (MapType == MAP_READ)
    {
        m_MapState = MapState::Read;
        return &m_MappedData[static_cast<size_t>(Offset)];
    }
    else if (MapType == MAP_WRITE)
    {
        m_MapState = MapState::Write;
        return &m_MappedData[static_cast<size_t>(Offset)];
    }
    else if (MapType == MAP_READ_WRITE)
    {
        LOG_ERROR("MAP_READ_WRITE is not supported in WebGPU backend");
    }
    else
    {
        UNEXPECTED("Unknown map type");
    }

    return nullptr;
}

void WebGPUResourceBase::Unmap()
{
    DEV_CHECK_ERR(m_MapState != MapState::None, "Resource is not mapped");
    m_MapState = MapState::None;
}

void WebGPUResourceBase::FlushPendingWrites(StagingBufferInfo& Buffer)
{
    VERIFY_EXPR(m_StagingBuffers.size() == 1);
    VERIFY(!Buffer.pSyncPoint, "Staging write buffers do not need sync points");

    // Do NOT use WGPU_WHOLE_MAP_SIZE due to https://github.com/emscripten-core/emscripten/issues/20538
    const size_t MappedDataSize = m_MappedData.size();
    if (void* pData = wgpuBufferGetMappedRange(Buffer.wgpuBuffer, 0, AlignUp(MappedDataSize, MappedRangeAlignment)))
    {
        memcpy(pData, m_MappedData.data(), MappedDataSize);
    }
    else
    {
        UNEXPECTED("Mapped range is null");
    }
    wgpuBufferUnmap(Buffer.wgpuBuffer);

    // Clear staging buffers - we create a new write buffer that is mapped at creation each time.
    m_StagingBuffers.clear();
}

void WebGPUResourceBase::ProcessAsyncReadback(StagingBufferInfo& Buffer)
{
    auto MapAsyncCallback = [](WGPUBufferMapAsyncStatus MapStatus, void* pUserData) {
        VERIFY_EXPR(pUserData != nullptr);
        StagingBufferInfo& BufferInfo = *static_cast<StagingBufferInfo*>(pUserData);

        if (MapStatus == WGPUBufferMapAsyncStatus_Success)
        {
            // Do NOT use WGPU_WHOLE_MAP_SIZE due to https://github.com/emscripten-core/emscripten/issues/20538
            const size_t MappedDataSize = BufferInfo.Resource.m_MappedData.size();
            if (const void* pData = wgpuBufferGetConstMappedRange(BufferInfo.wgpuBuffer, 0, AlignUp(MappedDataSize, MappedRangeAlignment)))
            {
                memcpy(BufferInfo.Resource.m_MappedData.data(), pData, MappedDataSize);
            }
            else
            {
                UNEXPECTED("Mapped range is null");
            }
            wgpuBufferUnmap(BufferInfo.wgpuBuffer.Get());
        }

        BufferInfo.pSyncPoint->Trigger();

        // Release the reference to the resource
        BufferInfo.Resource.m_Owner.Release();
    };

    // Keep the resource alive until the callback is called
    m_Owner.AddRef();
    // Do NOT use WGPU_WHOLE_MAP_SIZE due to https://github.com/emscripten-core/emscripten/issues/20538
    const size_t MappedDataSize = Buffer.Resource.m_MappedData.size();
    wgpuBufferMapAsync(Buffer.wgpuBuffer, WGPUMapMode_Read, 0, AlignUp(MappedDataSize, MappedRangeAlignment), MapAsyncCallback, &Buffer);
}

} // namespace Diligent
