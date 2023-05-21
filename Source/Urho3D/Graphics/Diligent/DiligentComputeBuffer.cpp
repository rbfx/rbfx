//
// Copyright (c) 2008-2020 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../../Graphics/ComputeBuffer.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsDefs.h"
#include "../../IO/Log.h"

namespace Urho3D
{
    using namespace Diligent;

void ComputeBuffer::OnDeviceLost()
{
    // nothing to do here on Diligent
}

void ComputeBuffer::OnDeviceReset()
{
    // nothing to do here on Diligent
}

void ComputeBuffer::Release()
{
    uav_ = nullptr;
    object_ = nullptr;
}

bool ComputeBuffer::SetSize(unsigned bytes, unsigned structureSize)
{
    Release();

    size_ = bytes;
    structureSize_ = structureSize;

    if (size_ == 0 || structureSize_ == 0)
    {
        URHO3D_LOGERROR("Unable to created ComputeBuffer with size: {} and struct-size: {}", size_, structureSize_);
        return false;
    }

    if (graphics_)
    {
        BufferDesc bufferDesc = {};
#ifdef URHO3D_DEBUG
        bufferDesc.Name = "Compute Buffer/UAV";
#endif
        bufferDesc.Usage = USAGE_DEFAULT;
        bufferDesc.BindFlags = BIND_UNORDERED_ACCESS;
        bufferDesc.CPUAccessFlags = CPU_ACCESS_NONE;
        bufferDesc.ElementByteStride = structureSize_;
        bufferDesc.Size = size_;
        bufferDesc.Mode = BUFFER_MODE_STRUCTURED;

        RefCntAutoPtr<IBuffer> buffer;
        graphics_->GetImpl()->GetDevice()->CreateBuffer(bufferDesc, nullptr, &buffer);

        if (!buffer)
        {
            URHO3D_LOGERROR("Failed to create a compute buffer.");
            return false;
        }

        object_ = buffer;

        uav_ = buffer->GetDefaultView(BUFFER_VIEW_UNORDERED_ACCESS);
    }

    return true;
}

bool ComputeBuffer::SetData(void* data, unsigned dataSize, unsigned structureSize)
{
    if (size_ != dataSize || structureSize_ != structureSize)
    {
        Release();
        if (!SetSize(dataSize, structureSize))
            return false;
    }


    graphics_->GetImpl()->GetDeviceContext()->UpdateBuffer(
        object_.Cast<IBuffer>(IID_Buffer),
        0,
        dataSize,
        data,
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );

    return true;
}

bool ComputeBuffer::GetData(void* writeInto, unsigned offset, unsigned readLength)
{
    if (!object_)
    {
        URHO3D_LOGERROR("Attempted to read ComputeBuffer data for invalid buffer");
        return false;
    }

    if (!graphics_)
        return false;

    BufferDesc bufferDesc;
#ifdef URHO3D_DEBUG
    bufferDesc.Name = "Compute Buffer/Staging Buffer";
#endif
    bufferDesc.Size = size_;
    bufferDesc.Usage = USAGE_STAGING;
    bufferDesc.CPUAccessFlags = CPU_ACCESS_READ;

    RefCntAutoPtr<IBuffer> stagingBuffer;
    graphics_->GetImpl()->GetDevice()->CreateBuffer(bufferDesc, nullptr, &stagingBuffer);

    if (stagingBuffer)
    {
        URHO3D_LOGERROR("Failed to create staging buffer for ComputeBuffer read");
        return false;
    }

    graphics_->GetImpl()->GetDeviceContext()->CopyBuffer(
        object_.Cast<IBuffer>(IID_Buffer),
        0,
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        stagingBuffer,
        0,
        size_,
        RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );

    void* mappedData = nullptr;
    graphics_->GetImpl()->GetDeviceContext()->MapBuffer(stagingBuffer, MAP_READ, MAP_FLAG_DISCARD, mappedData);

    memcpy(writeInto, ((unsigned char*)mappedData) + offset, readLength);

    graphics_->GetImpl()->GetDeviceContext()->UnmapBuffer(stagingBuffer, MAP_READ);
    return true;
}

} // namespace Urho3D
