// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RawBuffer.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderAPI/RenderPool.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>

namespace Urho3D
{

RawBuffer::RawBuffer(Context* context)
    : Object(context)
    , DeviceObject(context)
{
}

RawBuffer::RawBuffer(Context* context, const RawBufferParams& params, const void* data)
    : Object(context)
    , DeviceObject(context)
{
    Create(params, data);
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

    if (params_.size_ == 0)
    {
        dataLost_ = false;
    }
    else if (shadowData_)
    {
        const bool isDynamic = params_.flags_.Test(BufferFlag::Dynamic);
        if (params_.flags_.Test(BufferFlag::Dynamic))
        {
            CreateGPU(nullptr);
            Update(shadowData_.get());
        }
        else
        {
            CreateGPU(shadowData_.get());
        }
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

bool RawBuffer::Create(const RawBufferParams& params, const void* data)
{
    Destroy();

    params_ = params;
    needResolve_ = false;
    if (params_.size_ == 0)
        return true;

    // TODO: This is a workaround for a bug in Vulkan driver, maybe it should be configured in real-time.
    static const bool disableDefaultBuffers = false;

    if (disableDefaultBuffers && !params_.flags_.Test(BufferFlag::Dynamic))
        params_.flags_.Set(BufferFlag::Immutable);

    if (params_.flags_.Test(BufferFlag::Dynamic) && params_.flags_.Test(BufferFlag::Immutable))
    {
        URHO3D_ASSERTLOG(false, "Dynamic buffer cannot be immutable");
        return false;
    }

    if (!renderDevice_)
    {
        // If there's no render device, buffer must be shadowed
        params_.flags_.Set(BufferFlag::Shadowed);
    }
    else
    {
        // If buffer is dynamic, next-gen backend is used, and Discard is not set, shadow buffer is required
        const RenderBackend backend = renderDevice_->GetBackend();
        const bool isNextGen = backend != RenderBackend::D3D11 && backend != RenderBackend::OpenGL;
        if (isNextGen && params_.flags_.Test(BufferFlag::Dynamic) && !params_.flags_.Test(BufferFlag::Discard))
        {
            params_.flags_.Set(BufferFlag::Shadowed);
            needResolve_ = true;
        }
        else if (params_.flags_.Test(BufferFlag::Immutable))
        {
            // Immutable buffer is always shadowed
            params_.flags_.Set(BufferFlag::Shadowed);
        }
    }

    // Dynamic buffer cannot be bound as UAV
    if (params_.flags_.Test(BufferFlag::BindUnorderedAccess) && params_.flags_.Test(BufferFlag::Dynamic))
    {
        URHO3D_ASSERTLOG(false, "Dynamic buffer cannot be bound as UAV and is demoted");
        return false;
    }

    // Dynamic buffer cannot have initial data
    if (params_.flags_.Test(BufferFlag::Dynamic) && data)
    {
        URHO3D_ASSERTLOG(false, "Dynamic buffer cannot have initial data");
        return false;
    }

    // Dynamic buffers on OpenGL are weird, don't use them
    if (renderDevice_ && renderDevice_->GetBackend() == RenderBackend::OpenGL)
        params_.flags_.Unset(BufferFlag::Dynamic);

    // Create CPU buffer
    if (params_.flags_.Test(BufferFlag::Shadowed))
    {
        shadowData_ = ea::shared_array<unsigned char>(new unsigned char[params_.size_]);
        if (data)
            memcpy(shadowData_.get(), data, params_.size_);
    }

    // Create GPU buffer, postpone if Immutable and no data
    if (renderDevice_ && (!params_.flags_.Test(BufferFlag::Immutable) || data != nullptr))
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

    bufferDesc.BindFlags = bufferTypeToBindFlag[params_.type_];
    if (params_.flags_.Test(BufferFlag::BindUnorderedAccess))
        bufferDesc.BindFlags |= Diligent::BIND_UNORDERED_ACCESS;

    // TODO: Revisit this place if we add other usages
    bufferDesc.Usage = params_.flags_.Test(BufferFlag::Immutable) ? Diligent::USAGE_IMMUTABLE : Diligent::USAGE_DEFAULT;
    bufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_NONE;
    if (renderDevice_->GetBackend() != RenderBackend::OpenGL && params_.flags_.Test(BufferFlag::Dynamic))
    {
        bufferDesc.Usage = Diligent::USAGE_DYNAMIC;
        bufferDesc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    }
    internalUsage_ = bufferDesc.Usage;

    bufferDesc.Mode = Diligent::BUFFER_MODE_UNDEFINED;
    bufferDesc.Size = params_.size_;
    bufferDesc.ElementByteStride = params_.stride_;

    Diligent::IRenderDevice* device = renderDevice_->GetRenderDevice();
    Diligent::IDeviceContext* immediateContext = renderDevice_->GetImmediateContext();

    Diligent::BufferData bufferData;
    bufferData.pData = data;
    bufferData.DataSize = params_.size_;
    bufferData.pContext = immediateContext;

    device->CreateBuffer(bufferDesc, data ? &bufferData : nullptr, &handle_);
    if (!handle_)
    {
        URHO3D_LOGERROR("Failed to create buffer: type={} size={} stride={} flags=0b{:b}", params_.type_, params_.size_,
            params_.stride_, params_.flags_.AsInteger());
        return false;
    }

    return true;
}

void RawBuffer::Update(const void* data, unsigned size)
{
    const unsigned dataSize = size ? size : params_.size_;
    UpdateRange(data, 0, dataSize);
}

void RawBuffer::UpdateRange(const void* data, unsigned offset, unsigned size)
{
    URHO3D_ASSERT(!IsLocked());
    URHO3D_ASSERT(data, "Data must not be null");
    URHO3D_ASSERT(offset + size <= params_.size_, "Range must be within buffer size");
    URHO3D_ASSERT(
        offset == 0 || (!params_.flags_.Test(BufferFlag::Dynamic) && !params_.flags_.Test(BufferFlag::Immutable)),
        "Dynamic and immutable buffers cannot be partially updated");

    if (params_.flags_.Test(BufferFlag::Immutable) && renderDevice_ && handle_)
        URHO3D_LOGWARNING("Recreating immutable buffer '{}' due to RawBuffer::UpdateRange call", debugName_);

    if (size == 0)
    {
        URHO3D_LOGWARNING("RawBuffer::UpdateRange is called with zero size for buffer '{}'", debugName_);
        return;
    }

    if (params_.flags_.Test(BufferFlag::Shadowed))
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
        else if (internalUsage_ == Diligent::USAGE_IMMUTABLE)
        {
            CreateGPU(data);
        }
        else
        {
            immediateContext->UpdateBuffer(
                handle_, offset, size, data, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }

        lastUpdateFrameIndex_ = renderDevice_->GetFrameIndex();
    }

    ClearDataLost();
}

void* RawBuffer::Map()
{
    URHO3D_ASSERT(!IsLocked());

    // If shadowed, return shadow data and upload it on unlock
    if (params_.flags_.Test(BufferFlag::Shadowed))
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
    RenderPool* renderPool = renderDevice_->GetRenderPool();

    const auto temporaryBuffer = reinterpret_cast<unsigned char*>(renderPool->AllocateScratchBuffer(params_.size_));
    const auto deleter = [=](unsigned char* ptr) { renderPool->ReleaseScratchBuffer(ptr); };
    auto cpuBufferHolder = ea::shared_ptr<unsigned char>(temporaryBuffer, deleter);

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

    ClearDataLost();
}

void RawBuffer::Resolve()
{
    if (!needResolve_)
        return;
    if (lastUpdateFrameIndex_ != renderDevice_->GetFrameIndex())
        Update(shadowData_.get());
}

} // namespace Urho3D
