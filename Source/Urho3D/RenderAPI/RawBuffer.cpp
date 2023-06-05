// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RawBuffer.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>

#include <EASTL/shared_array.h>

namespace Urho3D
{

RawBuffer::RawBuffer(Context* context)
    : Object(context)
    , DeviceObject(context, this)
{
}

RawBuffer::~RawBuffer()
{
    Destroy();
}

void RawBuffer::Invalidate()
{
    URHO3D_ASSERT(!IsLocked());

    handle_ = nullptr;
}

void RawBuffer::Restore()
{
    URHO3D_ASSERT(!IsLocked());

    if (shadowData_)
    {
        CreateGPU(shadowData_.get());
        dataLost_ = false;
    }
    else
    {
        CreateGPU(nullptr);
        dataLost_ = true;
    }
}

void RawBuffer::Destroy()
{
    URHO3D_ASSERT(!IsLocked());

    handle_ = nullptr;
    shadowData_ = nullptr;
}

bool RawBuffer::Create(BufferType type, unsigned size, unsigned stride, BufferFlags flags, const void* data)
{
    Destroy();

    type_ = type;
    size_ = size;
    stride_ = stride;
    flags_ = flags;
    needResolve_ = false;
    if (size_ == 0)
        return true;

    if (!renderDevice_)
    {
        // If there's no render device, buffer must be shadowed
        flags_ |= BufferFlag::Shadowed;
    }
    else
    {
        // If buffer is dynamic, next-gen backend is used, and Discard is not set, shadow buffer is required
        const RenderBackend backend = renderDevice_->GetBackend();
        const bool isNextGen = backend != RenderBackend::D3D11 && backend != RenderBackend::OpenGL;
        if (isNextGen && flags_.Test(BufferFlag::Dynamic) && !flags_.Test(BufferFlag::Discard))
        {
            flags_ |= BufferFlag::Shadowed;
            needResolve_ = true;
        }
    }

    // Dynamic buffer cannot be bound as UAV
    if (flags_.Test(BufferFlag::BindUnorderedAccess))
    {
        URHO3D_ASSERTLOG(!flags_.Test(BufferFlag::Dynamic), "Dynamic buffer cannot be bound as UAV and is demoted");
        flags_ &= ~BufferFlag::Dynamic;
    }

    // Dynamic buffer cannot have initial data
    if (flags_.Test(BufferFlag::Dynamic))
    {
        URHO3D_ASSERTLOG(!data, "Dynamic buffer cannot have initial data");
        data = nullptr;
    }

    // Dynamic buffers on OpenGL are weird, don't use them
    if (renderDevice_ && renderDevice_->GetBackend() == RenderBackend::OpenGL)
        flags_ &= ~BufferFlag::Dynamic;

    // Create CPU buffer
    if (flags_.Test(BufferFlag::Shadowed))
    {
        shadowData_ = ea::shared_array<unsigned char>(new unsigned char[size_]);
        if (data)
            memcpy(shadowData_.get(), data, size_);
    }

    // Create GPU buffer
    if (renderDevice_)
    {
        if (!CreateGPU(data))
            return false;
    }

    ClearDataLost();
    return true;
}

bool RawBuffer::CreateGPU(const void* data)
{
    static const EnumArray<Diligent::BIND_FLAGS, BufferType> bufferTypeToBindFlag{{
        Diligent::BIND_VERTEX_BUFFER,
        Diligent::BIND_INDEX_BUFFER,
        Diligent::BIND_UNIFORM_BUFFER,
    }};

    Diligent::BufferDesc bufferDesc;
    bufferDesc.Name = debugName_.c_str();

    bufferDesc.BindFlags = bufferTypeToBindFlag[type_];
    if (flags_.Test(BufferFlag::BindUnorderedAccess))
        bufferDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

    // TODO: Revisit this place if we add other usages
    bufferDesc.Usage = Diligent::USAGE_DEFAULT;
    bufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
    if (renderDevice_->GetBackend() != RenderBackend::OpenGL && flags_.Test(BufferFlag::Dynamic))
    {
        bufferDesc.Usage = Diligent::USAGE_DYNAMIC;
        bufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    }
    internalUsage_ = bufferDesc.Usage;

    bufferDesc.Mode = Diligent::BUFFER_MODE_UNDEFINED;
    bufferDesc.Size = size_;
    bufferDesc.ElementByteStride = stride_;

    Diligent::IRenderDevice* device = renderDevice_->GetRenderDevice();
    Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();

    Diligent::BufferData bufferData;
    bufferData.pData = data;
    bufferData.DataSize = size_;
    bufferData.pContext = immediateContext;

    device->CreateBuffer(bufferDesc, data ? &bufferData : nullptr, &handle_);
    if (!handle_)
    {
        URHO3D_LOGERROR("Failed to create buffer: type={} size={} stride={} flags=0b{:b}", type_, size_, stride_,
            flags_.AsInteger());
        return false;
    }

    return true;
}

void RawBuffer::Update(const void* data, unsigned size)
{
    const unsigned dataSize = size ? size : size_;
    UpdateRange(data, 0, dataSize);
}

void RawBuffer::UpdateRange(const void* data, unsigned offset, unsigned size)
{
    URHO3D_ASSERT(!IsLocked());
    URHO3D_ASSERT(data, "Data must not be null");
    URHO3D_ASSERT(offset + size <= size_, "Range must be within buffer size");
    URHO3D_ASSERT(offset == 0 || !flags_.Test(BufferFlag::Dynamic), "Dynamic buffer cannot be partially updated");

    if (size == 0)
    {
        URHO3D_LOGWARNING("RawBuffer::UpdateRange is called with zero size for buffer '{}'", debugName_);
        return;
    }

    if (flags_.Test(BufferFlag::Shadowed))
    {
        void* cpuBuffer = &shadowData_[offset];
        if (cpuBuffer != data)
            memcpy(cpuBuffer, data, size);
    }

    if (renderDevice_)
    {
        Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();
        if (internalUsage_ == Diligent::USAGE_DYNAMIC)
        {
            void* gpuBuffer = nullptr;
            immediateContext->MapBuffer(handle_, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, gpuBuffer);
            if (gpuBuffer)
            {
                memcpy(gpuBuffer, data, size);
                immediateContext->UnmapBuffer(handle_, Diligent::MAP_WRITE);
            }
        }
        else
        {
            immediateContext->UpdateBuffer(
                handle_, offset, size, data, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }

        lastUpdateFrameIndex_ = renderDevice_->GetFrameIndex();
    }
}

void* RawBuffer::Map()
{
    URHO3D_ASSERT(!IsLocked());

    // If shadowed, return shadow data and upload it on unlock
    if (flags_.Test(BufferFlag::Shadowed))
    {
        unlockImpl_ = [=]() { Update(shadowData_.get()); };
        return shadowData_.get();
    }

    // If hardware dynamic buffer, map it
    if (internalUsage_ == Diligent::USAGE_DYNAMIC)
    {
        Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();
        URHO3D_ASSERT(immediateContext);

        void* gpuBuffer = nullptr;
        immediateContext->MapBuffer(handle_, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD, gpuBuffer);
        if (!gpuBuffer)
            return nullptr;

        unlockImpl_ = [=]() { immediateContext->UnmapBuffer(handle_, Diligent::MAP_WRITE); };
        return gpuBuffer;
    }

    // If hardware static buffer, return temporary buffer
    // TODO(diligent): Use allocator here
    auto cpuBufferHolder = ea::shared_array<unsigned char>(new unsigned char[size_]);
    unlockImpl_ = [=]() { Update(cpuBufferHolder.get()); };
    return cpuBufferHolder.get();
}

void RawBuffer::Unmap()
{
    URHO3D_ASSERT(IsLocked());
    auto callback = ea::move(unlockImpl_);
    unlockImpl_ = nullptr;
    callback();

    if (renderDevice_)
        lastUpdateFrameIndex_ = renderDevice_->GetFrameIndex();
}

void RawBuffer::Resolve()
{
    if (!needResolve_)
        return;
    if (lastUpdateFrameIndex_ != renderDevice_->GetFrameIndex())
        Update(shadowData_.get());
}

} // namespace Urho3D
