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
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsDefs.h"
#include "../../Graphics/Direct3D11/D3D11GraphicsImpl.h"
#include "../../IO/Log.h"

namespace Urho3D
{

void ComputeBuffer::OnDeviceLost()
{
    // nothing to do here on DX11
}

void ComputeBuffer::OnDeviceReset()
{
    // nothing to do here on DX11
}

void ComputeBuffer::Release()
{
    URHO3D_SAFE_RELEASE(object_.ptr_);
    URHO3D_SAFE_RELEASE(uav_);
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
        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof bufferDesc);
        bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        bufferDesc.CPUAccessFlags = 0;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.StructureByteStride = structureSize_;
        bufferDesc.ByteWidth = size_;
        bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

        HRESULT hr = graphics_->GetImpl()->GetDevice()->CreateBuffer(&bufferDesc, nullptr, (ID3D11Buffer**)&object_.ptr_);
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(object_.ptr_);
            URHO3D_LOGD3DERROR("Failed to create compute buffer", hr);
            return false;
        }

        D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
        ZeroMemory(&viewDesc, sizeof viewDesc);
        viewDesc.Format = DXGI_FORMAT_UNKNOWN;
        viewDesc.Buffer.FirstElement = 0;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        viewDesc.Buffer.NumElements = GetNumElements();

        hr = graphics_->GetImpl()->GetDevice()->CreateUnorderedAccessView((ID3D11Resource*)object_.ptr_, &viewDesc, &uav_);
        if (FAILED(hr))
        {
            URHO3D_SAFE_RELEASE(uav_);
            URHO3D_SAFE_RELEASE(object_.ptr_);
            URHO3D_LOGD3DERROR("Failed to create UAV for compute buffer", hr);
            return false;
        }
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

    D3D11_BOX box;
    box.left = 0;
    box.right = dataSize;
    box.top = 0;
    box.bottom = 1;
    box.front = 0;
    box.back = 1;
    graphics_->GetImpl()->GetDeviceContext()->UpdateSubresource((ID3D11Resource*)object_.ptr_, 0, &box, data, 0, 0);
    return true;
}

bool ComputeBuffer::GetData(void* writeInto, unsigned offset, unsigned readLength)
{
    if (!object_.ptr_)
    {
        URHO3D_LOGERROR("Attempted to read ComputeBuffer data for invalid buffer");
        return false;
    }

    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof bufferDesc);
    bufferDesc.ByteWidth = size_;
    bufferDesc.Usage = D3D11_USAGE_STAGING;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* staging = nullptr;
    auto hr = graphics_->GetImpl()->GetDevice()->CreateBuffer(&bufferDesc, nullptr, &staging);
    if (FAILED(hr))
    {
        URHO3D_LOGD3DERROR("Failed to create staging buffer for ComputeBuffer read", hr);
        return false;
    }

    graphics_->GetImpl()->GetDeviceContext()->CopyResource(staging, (ID3D11Buffer*)object_.ptr_);

    D3D11_MAPPED_SUBRESOURCE mappedData;
    mappedData.pData = nullptr;

    hr = graphics_->GetImpl()->GetDeviceContext()->Map(staging, 0, D3D11_MAP_READ, 0, &mappedData);
    if (FAILED(hr) || !mappedData.pData)
    {
        URHO3D_LOGD3DERROR("Failed to map staging buffer for ComputeBuffer::GetData", hr);
        URHO3D_SAFE_RELEASE(staging);
        return false;
    }

    memcpy(writeInto, ((unsigned char*)mappedData.pData) + offset, readLength);

    graphics_->GetImpl()->GetDeviceContext()->Unmap(staging, 0);
    URHO3D_SAFE_RELEASE(staging);
    return true;
}

}
