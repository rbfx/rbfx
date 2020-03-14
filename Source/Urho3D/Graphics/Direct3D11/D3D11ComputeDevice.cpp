//
// Copyright (c) 2017-2020 the Urho3D project.
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

#include "../../Precompiled.h"

#include "../../Graphics/ComputeDevice.h"
#include "../../IO/Log.h"

#include "../../Graphics/Texture.h"
#include "../../Graphics/Texture2D.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../Graphics/Texture3D.h"
#include "../../Graphics/TextureCube.h"

#include "../../Graphics/IndexBuffer.h"
#include "../../Graphics/VertexBuffer.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"

#pragma optimize("", off)

namespace Urho3D
{

bool ComputeDevice_ClearUAV(ID3D11UnorderedAccessView* view, ID3D11UnorderedAccessView** table, size_t tableSize)
{
    bool changedAny = false;
    for (size_t i = 0; i < tableSize; ++i)
    {
        if (table[i] == view)
        {
            table[i] = nullptr;
            changedAny = true;
        }
    }
    return changedAny;
}

void ComputeDevice::Init()
{
    ZeroMemory(shaderResourceViews_, sizeof(shaderResourceViews_));
    ZeroMemory(uavs_, sizeof(uavs_));
    ZeroMemory(samplerBindings_, sizeof(samplerBindings_));
    ZeroMemory(constantBuffers_, sizeof(constantBuffers_));
}

bool ComputeDevice::IsSupported() const
{
    auto device = graphics_->GetImpl()->GetDevice();

    // Although D3D10 *can optionally* support DirectCompute - I'm betting on it being too finnicky to trust for general exposure.
    return device->GetFeatureLevel() >= D3D_FEATURE_LEVEL_11_0;
}

bool ComputeDevice::SetReadTexture(Texture* texture, unsigned unit)
{
    if (unit >= MAX_TEXTURE_UNITS)
    {
        URHO3D_LOGERROR("ComputeDevice::SetReadTexture, invalid unit {} specified", unit);
        return false;
    }
    if (shaderResourceViews_[unit] != texture->GetGPUObject())
    {
        if (texture->GetParametersDirty())
            texture->UpdateParameters();

        samplerBindings_[unit] = (ID3D11SamplerState*)texture->GetSampler();
        shaderResourceViews_[unit] = (ID3D11ShaderResourceView*)texture->GetShaderResourceView();
        samplersDirty_ = true;
        texturesDirty_ = true;
    }

    return true;
}

bool ComputeDevice::SetConstantBuffer(ConstantBuffer* buffer, unsigned unit)
{
    if (unit >= MAX_SHADER_PARAMETER_GROUPS)
    {
        URHO3D_LOGERROR("ComputeDevice::SetConstantBuffer, invalid unit {} specified", unit);
        return false;
    }

    if (constantBuffers_[unit] != buffer->GetGPUObject())
    {
        constantBuffers_[unit] = (ID3D11Buffer*)buffer->GetGPUObject();
        constantBuffersDirty_ = true;
    }
    return true;
}

bool ComputeDevice::SetWriteTexture(Texture* texture, unsigned unit, unsigned faceIndex, unsigned mipLevel)
{
    if (unit >= MAX_COMPUTE_WRITE_TARGETS)
    {
        URHO3D_LOGERROR("ComputeDevice::SetWriteTexture, invalid unit {} specified", unit);
        return false;
    }

    // If null then clear and mark.
    if (texture == nullptr)
    {
        uavs_[unit] = nullptr;
        uavsDirty_ = true;
        return true;
    }

    if (!Texture::IsComputeWriteable(texture->GetFormat()))
    {
        URHO3D_LOGERROR("ComputeDevice::SetWriteTexture, provided texture of format {} is not writeable", texture->GetFormat());
        return false;
    }

    // First try to find a UAV that's already been constructed for this resource.
    auto& existingRecord = constructedUAVs_.find(WeakPtr<Object>(texture));
    if (existingRecord != constructedUAVs_.end())
    {
        for (auto& entry : existingRecord->second)
        {
            if (entry.face_ == faceIndex && entry.mipLevel_ == mipLevel)
            {
                uavs_[unit] = entry.uav_;
                uavsDirty_ = true;
                return true;
            }
        }
    }

    // Existing UAV wasn't found, so now a new one needs to be created.
    auto d3dDevice = graphics_->GetImpl()->GetDevice();

    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
    ZeroMemory(&viewDesc, sizeof(viewDesc));

    viewDesc.Format = (DXGI_FORMAT)texture->GetFormat();
    if (auto tex2D = texture->Cast<Texture2D>())
    {
        viewDesc.Texture2D.MipSlice = mipLevel;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    }
    else if (auto tex2DArray = texture->Cast<Texture2DArray>())
    {
        viewDesc.Texture2DArray.ArraySize = faceIndex == UINT_MAX ? tex2DArray->GetLayers() : 1;
        viewDesc.Texture2DArray.FirstArraySlice = faceIndex == UINT_MAX ? 0 : faceIndex;
        viewDesc.Texture2DArray.MipSlice = mipLevel;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    }
    else if (auto texCube = texture->Cast<TextureCube>())
    {
        viewDesc.Texture2DArray.ArraySize = faceIndex == UINT_MAX ? 6 : 1;
        viewDesc.Texture2DArray.FirstArraySlice = faceIndex == UINT_MAX ? 0 : faceIndex;
        viewDesc.Texture2DArray.MipSlice = mipLevel;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    }
    else if (auto tex3D = texture->Cast<Texture3D>())
    {
        viewDesc.Texture3D.MipSlice = mipLevel;
        viewDesc.Texture3D.FirstWSlice = 0;
        viewDesc.Texture3D.WSize = texture->GetLevelDepth(mipLevel);
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
    }
    else
    {
        URHO3D_LOGERROR("Unsupported texture type for UAV");
        return false;
    }

    ID3D11UnorderedAccessView* view = nullptr;
    auto result = d3dDevice->CreateUnorderedAccessView((ID3D11Resource*)texture->GetGPUObject(), &viewDesc, &view);
    if (FAILED(result))
    {
        URHO3D_LOGD3DERROR("Failed to create UAV for texture", result);
        URHO3D_SAFE_RELEASE(view);
        return false;
    }

    // Store the UAV now
    UAVBinding binding = { view, faceIndex, mipLevel, false };
    // List found, so append
    if (existingRecord != constructedUAVs_.end())
        existingRecord->second.push_back(binding);
    else
    {
        // list not found, create it
        eastl::vector<UAVBinding> bindingList = { binding };
        constructedUAVs_.insert({ WeakPtr<Object>(texture), bindingList });

        // Subscribe to the release event so the UAV can be cleaned up.
        SubscribeToEvent(texture, E_GPURESOURCERELEASED, URHO3D_HANDLER(ComputeDevice, HandleGPUResourceRelease));
    }

    uavs_[unit] = view;
    uavsDirty_ = true;

    return true;
}

bool ComputeDevice::SetWritableBuffer(Object* object, unsigned slot)
{
    // Null object is trivial
    if (object == nullptr)
    {
        uavsDirty_ = uavs_[slot] != nullptr;
        uavs_[slot] = nullptr;
        return true;
    }

    // Easy case, it's a structured-buffer and thus manages the UAV itself
    if (auto structuredBuffer = object->Cast<ComputeBuffer>())
    {
        uavs_[slot] = structuredBuffer->GetUAV();
        uavsDirty_ = true;
        return true;
    }

    auto found = constructedBufferUAVs_.find(WeakPtr(object));
    if (found != constructedBufferUAVs_.end())
    {
        uavs_[slot] = found->second;
        uavsDirty_ = true;
        return true;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
    ZeroMemory(&viewDesc, sizeof viewDesc);
    viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;

    if (auto cbuffer = object->Cast<ConstantBuffer>())
    {
        viewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        viewDesc.Buffer.NumElements = cbuffer->GetSize() / sizeof(Vector4);
    }
    else if (auto vbuffer = object->Cast<VertexBuffer>())
    {
        viewDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        viewDesc.Buffer.NumElements = vbuffer->GetElements().size() * vbuffer->GetVertexCount();
    }
    else if (auto ibuffer = object->Cast<IndexBuffer>())
    {
        if (ibuffer->GetIndexSize() == sizeof(unsigned short))
            viewDesc.Format = DXGI_FORMAT_R16_UINT;
        else
            viewDesc.Format = DXGI_FORMAT_R32_UINT;

        viewDesc.Buffer.NumElements = ibuffer->GetIndexCount();
    }

    ID3D11Buffer* buffer = (ID3D11Buffer*)((GPUObject*)object)->GetGPUObject();
    ID3D11UnorderedAccessView* uav = nullptr;
    auto hr = graphics_->GetImpl()->GetDevice()->CreateUnorderedAccessView(buffer, &viewDesc, &uav);
    if (FAILED(hr))
    {
        URHO3D_SAFE_RELEASE(uav);
        URHO3D_LOGD3DERROR("Failed to create UAV for buffer", hr);
    }

    // Subscribe for the clean-up opportunity
    SubscribeToEvent(object, E_GPURESOURCERELEASED, URHO3D_HANDLER(ComputeDevice, HandleGPUResourceRelease));

    constructedBufferUAVs_.insert({ WeakPtr(object), uav });
    uavs_[slot] = uav;
    uavsDirty_ = true;

    return true;
}

void ComputeDevice::ApplyBindings()
{
    auto d3dContext = graphics_->GetImpl()->GetDeviceContext();

    if (texturesDirty_)
    {
        // attempting to sample an active render-target doesn't work...
        graphics_->SetRenderTarget(0, (RenderSurface*)nullptr);
        // ...so make certain the deed is done.
        d3dContext->OMSetRenderTargets(0, nullptr, nullptr);

        d3dContext->CSSetShaderResources(0, MAX_TEXTURE_UNITS, shaderResourceViews_);
    }

    if (samplersDirty_)
        d3dContext->CSSetSamplers(0, MAX_TEXTURE_UNITS, samplerBindings_);

    if (constantBuffersDirty_)
        d3dContext->CSSetConstantBuffers(0, MAX_SHADER_PARAMETER_GROUPS, constantBuffers_);

    if (uavsDirty_)
    {
        unsigned deadCounts[16];
        ZeroMemory(deadCounts, sizeof(deadCounts));
        d3dContext->CSSetUnorderedAccessViews(0, MAX_COMPUTE_WRITE_TARGETS, uavs_, nullptr);
    }

    if (programDirty_)
    {
        if (computeShader_.NotNull())
            d3dContext->CSSetShader((ID3D11ComputeShader*)computeShader_->GetGPUObject(), nullptr, 0);
        else
            d3dContext->CSSetShader(nullptr, nullptr, 0);
    }

    constantBuffersDirty_ = false;
    samplersDirty_ = false;
    texturesDirty_ = false;
    uavsDirty_ = false;
    programDirty_ = false;
}

void ComputeDevice::Dispatch(unsigned xDim, unsigned yDim, unsigned zDim)
{
    if (!IsSupported())
    {
        URHO3D_LOGERROR("Attempted to dispatch compute with a D3D feature level below 11_0");
        return;
    }

    if (computeShader_ && !computeShader_->GetGPUObject())
    {
        if (computeShader_->GetCompilerOutput().empty())
        {
            bool success = computeShader_->Create();
            if (!success)
                URHO3D_LOGERROR("Failed to compile compute shader " + computeShader_->GetFullName() + ":\n" + computeShader_->GetCompilerOutput());
        }
        else
            computeShader_ = nullptr;
    }

    if (!computeShader_)
        return;

    ApplyBindings();

    auto d3dContext = graphics_->GetImpl()->GetDeviceContext();
    if (computeShader_.NotNull())
        d3dContext->Dispatch(xDim, yDim, zDim);
}

void ComputeDevice::HandleGPUResourceRelease(StringHash eventID, VariantMap& eventData)
{
    SharedPtr<Object> object(dynamic_cast<Object*>(eventData["GPUObject"].GetPtr()));
    if (object.Null())
        return;
    void* gpuObject = ((GPUObject*)object.Get())->GetGPUObject();

    auto foundUAV = constructedUAVs_.find(object);
    if (foundUAV != constructedUAVs_.end())
    {
        auto d3dDevice = graphics_->GetImpl()->GetDevice();

        for (auto& entry : foundUAV->second)
        {
            uavsDirty_ |= ComputeDevice_ClearUAV(entry.uav_, uavs_, MAX_COMPUTE_WRITE_TARGETS);
            URHO3D_SAFE_RELEASE(entry.uav_);
        }

        constructedUAVs_.erase(foundUAV);
    }

    auto foundBuffUAV = constructedBufferUAVs_.find(object);
    if (foundBuffUAV != constructedBufferUAVs_.end())
    {
        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (shaderResourceViews_[i] == gpuObject)
            {
                shaderResourceViews_[i] = nullptr;
                texturesDirty_ = true;
            }
        }

        for (unsigned i = 0; i < MAX_COMPUTE_WRITE_TARGETS; ++i)
        {
            if (uavs_[i] == foundBuffUAV->second)
            {
                uavs_[i] = nullptr;
                uavsDirty_ = true;
            }
        }

        URHO3D_SAFE_RELEASE(foundBuffUAV->second);
        constructedBufferUAVs_.erase(foundBuffUAV);
    }

    UnsubscribeFromEvent(object.Get(), E_GPURESOURCERELEASED);
}

void ComputeDevice::ReleaseLocalState()
{
    for (auto& uav : constructedUAVs_)
    {
        for (auto& item : uav.second)
        {
            URHO3D_SAFE_RELEASE(item.uav_);
        }
    }
    constructedUAVs_.clear();

    for (auto& srv : constructedBufferUAVs_)
    {
        URHO3D_SAFE_RELEASE(srv.second);
    }
    constructedBufferUAVs_.clear();
}

}
