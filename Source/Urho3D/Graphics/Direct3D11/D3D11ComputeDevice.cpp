#include "../../Precompiled.h"

#include "../../Graphics/ComputeDevice.h"
#include "../../IO/Log.h"

#include "../../Graphics/Texture.h"
#include "../../Graphics/Texture2D.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../Graphics/TextureCube.h"

#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"

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
    if (shaderResourceViews_[unit] != texture->GetGPUObject())
    {
        samplerBindings_[unit] = (ID3D11SamplerState*)texture->GetSampler();
        shaderResourceViews_[unit] = (ID3D11ShaderResourceView*)texture->GetShaderResourceView();
        samplersDirty_ = true;
        texturesDirty_ = true;
    }

    return false;
}

bool ComputeDevice::SetConstantBuffer(ConstantBuffer* buffer, unsigned unit)
{
    if (constantBuffers_[unit] != buffer->GetGPUObject())
    {
        constantBuffers_[unit] = (ID3D11Buffer*)buffer->GetGPUObject();
        constantBuffersDirty_ = true;
    }
    return true;
}

bool ComputeDevice::SetWriteTexture(Texture* texture, unsigned unit, unsigned faceIndex, unsigned mipLevel)
{
    // If null then clear and mark.
    if (texture == nullptr)
    {
        uavs_[unit] = nullptr;
        uavsDirty_ = true;
        return true;
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
    if (auto tex2D = dynamic_cast<Texture2D*>(texture))
    {
        viewDesc.Texture2D.MipSlice = mipLevel;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    }
    else if (auto tex2DArray = dynamic_cast<Texture2DArray*>(texture))
    {
        viewDesc.Texture2DArray.ArraySize = faceIndex == UINT_MAX ? tex2DArray->GetLayers() : 1;
        viewDesc.Texture2DArray.FirstArraySlice = faceIndex == UINT_MAX ? 0 : faceIndex;
        viewDesc.Texture2DArray.MipSlice = mipLevel;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    }
    else if (auto texCube = dynamic_cast<TextureCube*>(texture))
    {
        viewDesc.Texture2DArray.ArraySize = faceIndex == UINT_MAX ? 6 : 1;
        viewDesc.Texture2DArray.FirstArraySlice = faceIndex == UINT_MAX ? 0 : faceIndex;
        viewDesc.Texture2DArray.MipSlice = mipLevel;
        viewDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
    }
    else
    {
        URHO3D_LOGERROR("Unsupported texture type for UAV");
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
    UAVBinding binding = { view, faceIndex, mipLevel };
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

/* JSandusky: not currently supported - writable buffers are a bit of a mess on DX-11 between R32_FLOAT and MISC_STRUCTURED ... not trivial to make generic.
bool ComputeDevice::SetWriteBuffer(ConstantBuffer* buffer, unsigned unit)
{
    if (buffer == nullptr)
    {
        uavs_[unit] = nullptr;
        uavsDirty_ = true;
        return true;
    }

    auto existingRecord = constructedUAVs_.find(WeakPtr<Object>(buffer));
    if (existingRecord != constructedUAVs_.end())
    {
        if (existingRecord->second.empty() == false)
        {
            uavs_[unit] = existingRecord->second[0].uav_;
            uavsDirty_ = true;
            return true;
        }
    }

    auto d3dDevice = graphics_->GetImpl()->GetDevice();

    D3D11_UNORDERED_ACCESS_VIEW_DESC viewDesc;
    ZeroMemory(&viewDesc, sizeof(viewDesc));

    viewDesc.Format = DXGI_FORMAT_R32_FLOAT;
    viewDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    viewDesc.Buffer.NumElements = buffer->GetSize() / sizeof(Vector4);

    return true;
}
*/

void ComputeDevice::ApplyBindings()
{
    auto d3dContext = graphics_->GetImpl()->GetDeviceContext();

    if (samplersDirty_)
        d3dContext->CSSetSamplers(0, MAX_TEXTURE_UNITS, samplerBindings_);

    if (texturesDirty_)
        d3dContext->CSSetShaderResources(0, MAX_TEXTURE_UNITS, shaderResourceViews_);

    if (constantBuffersDirty_)
        d3dContext->CSSetConstantBuffers(0, MAX_TEXTURE_UNITS, constantBuffers_);

    if (uavsDirty_)
    {
        unsigned deadCounts[16];
        ZeroMemory(deadCounts, sizeof(deadCounts));
        d3dContext->CSSetUnorderedAccessViews(0, MAX_TEXTURE_UNITS, uavs_, nullptr);
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
    {
        d3dContext->Dispatch(xDim, yDim, zDim);
    }
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
            uavsDirty_ |= ComputeDevice_ClearUAV(entry.uav_, uavs_, MAX_TEXTURE_UNITS);
            URHO3D_SAFE_RELEASE(entry.uav_);
        }

        constructedUAVs_.erase(foundUAV);
    }

    auto foundSRV = constructedBufferSRVs_.find(object);
    if (foundSRV != constructedBufferSRVs_.end())
    {
        for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
        {
            if (shaderResourceViews_[i] == gpuObject)
            {
                shaderResourceViews_[i] = nullptr;
                texturesDirty_ = true;
            }
        }

        URHO3D_SAFE_RELEASE(foundSRV->second);
        constructedBufferSRVs_.erase(foundSRV);
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

    for (auto& srv : constructedBufferSRVs_)
    {
        URHO3D_SAFE_RELEASE(srv.second);
    }
    constructedBufferSRVs_.clear();
}

}
