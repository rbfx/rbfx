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
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Graphics/IndexBuffer.h"
#include "../../Graphics/Texture.h"
#include "../../Graphics/Texture2D.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../Graphics/Texture3D.h"
#include "../../Graphics/TextureCube.h"
#include "../../Graphics/VertexBuffer.h"
#include "../../IO/Log.h"

#pragma optimize("", off)

namespace Urho3D
{

using namespace Diligent;
bool ComputeDevice_ClearResource(IDeviceObject* view, ea::unordered_map<ea::string, RefCntAutoPtr<IDeviceObject>>& resources)
{
    bool changedAny = false;
    for (auto it : resources)
    {
        if (it.second == view)
        {
            changedAny = true;
            resources.erase(it.first);
        }
    }
    return changedAny;
}

void ComputeDevice::Init()
{
    SubscribeToEvent(E_ENGINEINITIALIZED, URHO3D_HANDLER(ComputeDevice, HandleEngineInitialization));
    SubscribeToEvent(E_DEVICELOST, &ComputeDevice::ReleaseLocalState);
}
void ComputeDevice::HandleEngineInitialization(StringHash eventType, VariantMap& eventData)
{
    psoCache_ = GetSubsystem<PipelineStateCache>();
    resourcesDirty_ = true;
}

bool ComputeDevice::IsSupported() const
{
    return true;
}

bool ComputeDevice::SetReadTexture(Texture* texture, CD_UNIT textureSlot)
{
    if (texture == nullptr)
    {
        if (resources_.contains(textureSlot))
            resourcesDirty_ = true;

        resources_[textureSlot] = nullptr;
        return true;
    }

    ITextureView* textureObj = texture->GetHandles().srv_;

    auto foundRes = resources_.find(textureSlot);
    if (foundRes != resources_.end() && foundRes->second == textureObj)
        return true;

    resources_[textureSlot] = textureObj;

    resourcesDirty_ = true;
    return true;
}

bool ComputeDevice::SetConstantBuffer(ConstantBuffer* buffer, CD_UNIT cbufferSlot)
{
    if (buffer == nullptr)
    {
        if (resources_.contains(cbufferSlot))
            resourcesDirty_ = true;

        resources_[cbufferSlot] = nullptr;
        return true;
    }

    IBuffer* bufferObj = buffer->GetHandle();

    auto foundRes = resources_.find(cbufferSlot.c_str());
    if (foundRes != resources_.end() && foundRes->second == bufferObj)
        return true;

    resources_[cbufferSlot] = bufferObj;
    resourcesDirty_ = true;

    return true;
}

bool ComputeDevice::SetWriteTexture(Texture* texture, CD_UNIT textureSlot, unsigned faceIndex, unsigned mipLevel)
{
    // If null then clear and mark.
    if (texture == nullptr)
    {
        if (resources_.contains(textureSlot))
            resourcesDirty_ = true;

        resources_[textureSlot] = nullptr;
        return true;
    }

    if (!texture->IsUnorderedAccess())
    {
        URHO3D_LOGERROR(
            "ComputeDevice::SetWriteTexture, provided texture of format {} is not writeable", texture->GetFormat());
        return false;
    }

    RawTextureUAVKey key;
    key.canWrite_ = true;
    key.firstLevel_ = mipLevel;
    if (faceIndex != UINT_MAX)
    {
        key.firstSlice_ = faceIndex;
        key.numSlices_ = 1;
    }

    resources_[textureSlot] = texture->CreateUAV(key);
    resourcesDirty_ = true;
    return true;
}

bool ComputeDevice::SetWritableBuffer(Object* object, CD_UNIT slot)
{
    // If null then clear and mark.
    if (object == nullptr)
    {
        if (resources_.contains(slot))
            resourcesDirty_ = true;

        resources_[slot] = nullptr;
        return true;
    }

    // Easy case, it's a structured-buffer and thus manages the UAV itself
    if (auto structuredBuffer = object->Cast<ComputeBuffer>())
    {
        auto uav = structuredBuffer->GetUAV();
        auto foundRes = resources_.find(slot);
        if (foundRes != resources_.end() && foundRes->second == uav)
            return true;
        resources_[slot] = structuredBuffer->GetUAV();
        return resourcesDirty_ = true;
    }

    auto found = constructedBufferUAVs_.find(WeakPtr(object));
    if (found != constructedBufferUAVs_.end())
    {
        auto foundRes = resources_.find(slot);
        if (foundRes != resources_.end() && foundRes->second == found->second)
            return true;
        resources_[slot] = found->second;
        return resourcesDirty_ = true;
    }

    BufferViewDesc viewDesc;
    viewDesc.ViewType = BUFFER_VIEW_UNORDERED_ACCESS;
#ifdef URHO3D_DEBUG
    ea::string dbgName;
#endif
    RefCntAutoPtr<IBuffer> buffer;

    if (auto cbuffer = object->Cast<ConstantBuffer>())
    {
        buffer = cbuffer->GetHandle();
        viewDesc.Format.ValueType = VT_FLOAT32;
        viewDesc.Format.NumComponents = 4;
        viewDesc.Format.IsNormalized = false;
        viewDesc.ByteWidth = cbuffer->GetSize();
    }
    else if (auto vbuffer = object->Cast<VertexBuffer>())
    {
        buffer = vbuffer->GetHandle();
        viewDesc.Format.ValueType = VT_FLOAT32;
        viewDesc.Format.NumComponents = 4;
        viewDesc.Format.IsNormalized = false;
        viewDesc.ByteWidth = (vbuffer->GetElements().size() * vbuffer->GetVertexCount()) * vbuffer->GetElements().size();
    }
    else if (auto ibuffer = object->Cast<IndexBuffer>())
    {
        buffer = ibuffer->GetHandle();
        viewDesc.Format.IsNormalized = false;
        viewDesc.Format.NumComponents = 1;
        if (ibuffer->GetIndexSize() == sizeof(unsigned short))
            viewDesc.Format.ValueType = VT_UINT16;
        else
            viewDesc.Format.ValueType = VT_UINT32;

        viewDesc.ByteWidth = ibuffer->GetIndexCount();
    }
    else
    {
        URHO3D_LOGERROR("ComputeDevice::SetWritableBuffer, cannot bind {} as write target", object->GetTypeName());
        return false;
    }

    RefCntAutoPtr<IBufferView> view;
    buffer->CreateView(viewDesc, &view);
    if (!view)
    {
        URHO3D_LOGERROR("Failed to create UAV for buffer");
        return false;
    }

    // Subscribe for the clean-up opportunity
    SubscribeToEvent(object, E_GPURESOURCERELEASED, URHO3D_HANDLER(ComputeDevice, HandleGPUResourceRelease));

    constructedBufferUAVs_.insert({WeakPtr(object), view});

    resources_[slot] = view;

    return resourcesDirty_ = true;
}


bool ComputeDevice::BuildPipeline()
{
    if (!programDirty_ && pipeline_ && srb_)
        return true;

    IShader* computeShader = computeShader_->GetHandle();
    unsigned hash = MakeHash((void*)computeShader);
    auto cacheEntry = cachedPipelines_.find(hash);
    if (cacheEntry != cachedPipelines_.end())
    {
        pipeline_ = cacheEntry->second.pipeline_;
        srb_ = cacheEntry->second.srb_;
        resourcesDirty_ = true; // Force resource binding update.
        return true;
    }

    pipeline_ = nullptr;
    srb_ = nullptr;

    ComputePipelineStateCreateInfo ci;
#ifdef URHO3D_DEBUG
    ea::string dbgName = Format("{}(Compute)", computeShader_->GetDebugName());
    ci.PSODesc.Name = dbgName.c_str();
#endif
    ci.PSODesc.PipelineType = PIPELINE_TYPE_COMPUTE;
    ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;

    // TODO(diligent): Revisit this
    ea::vector<ImmutableSamplerDesc> immutableSamplers;
    immutableSamplers.reserve(resources_.size());
    for (const auto& [resource, _] : resources_)
    {
        ImmutableSamplerDesc& desc = immutableSamplers.emplace_back();
        desc.SamplerOrTextureName = resource.c_str();
        desc.ShaderStages = SHADER_TYPE_COMPUTE;
        desc.Desc.MinFilter = FILTER_TYPE_LINEAR;
        desc.Desc.MagFilter = FILTER_TYPE_LINEAR;
        desc.Desc.MipFilter = FILTER_TYPE_LINEAR;
        desc.Desc.AddressU = TEXTURE_ADDRESS_WRAP;
        desc.Desc.AddressV = TEXTURE_ADDRESS_WRAP;
        desc.Desc.AddressW = TEXTURE_ADDRESS_WRAP;
    }
    ci.PSODesc.ResourceLayout.NumImmutableSamplers = resources_.size();
    ci.PSODesc.ResourceLayout.ImmutableSamplers = immutableSamplers.data();

    ci.pCS = computeShader;
    // Use PSO Cache if was created.
    if(psoCache_->GetGPUPipelineCache())
        ci.pPSOCache = psoCache_->GetGPUPipelineCache().Cast<IPipelineStateCache>(IID_PipelineStateCache);

    auto device = graphics_->GetImpl()->GetDevice();
    device->CreateComputePipelineState(ci, &pipeline_);

    if (!pipeline_) {
        URHO3D_LOGERROR("Failed to create Compute Pipeline State.");
        return false;
    }

    pipeline_->CreateShaderResourceBinding(&srb_, true);
    if (!srb_) {
        URHO3D_LOGERROR("Failed to create SRB for Compute Pipeline State");
        pipeline_ = nullptr;
        return false;
    }

    cachedPipelines_.insert(ea::make_pair(hash, CacheEntry{ pipeline_, srb_ }));
    return true;
}

void ComputeDevice::ApplyBindings()
{
    auto ctx = graphics_->GetImpl()->GetDeviceContext();
    auto device = graphics_->GetImpl()->GetDevice();

    if (resourcesDirty_) {
        RefCntAutoPtr<IResourceMapping> resMapping;
        device->CreateResourceMapping(ResourceMappingDesc{}, &resMapping);

        URHO3D_ASSERTLOG(resMapping, "Can create resource mapping object.");
        for (auto it : resources_)
            resMapping->AddResource(it.first.c_str(), it.second, false);

        srb_->BindResources(SHADER_TYPE_COMPUTE, resMapping, BIND_SHADER_RESOURCES_UPDATE_ALL | BIND_SHADER_RESOURCES_ALLOW_OVERWRITE);
    }
    resourcesDirty_ = false;

    ctx->SetPipelineState(pipeline_);
    ctx->CommitShaderResources(srb_, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

    programDirty_ = false;
}

void ComputeDevice::Dispatch(unsigned xDim, unsigned yDim, unsigned zDim)
{
    if (!IsSupported())
    {
        URHO3D_LOGERROR("Attempted to dispatch compute with a D3D feature level below 11_0");
        return;
    }

//    if (computeShader_ && !computeShader_->GetGPUObject())
//    {
//        if (computeShader_->GetCompilerOutput().empty())
//        {
//            bool success = computeShader_->Create();
//            if (!success)
//                URHO3D_LOGERROR("Failed to compile compute shader " + computeShader_->GetFullName() + ":\n"
//                    + computeShader_->GetCompilerOutput());
//        }
//        else
//            computeShader_ = nullptr;
//    }

    if (!computeShader_)
        return;

    if (!BuildPipeline())
        return;

    ApplyBindings();

    if (!pipeline_ || !srb_)
        return;

    DispatchComputeAttribs attribs;
    attribs.ThreadGroupCountX = xDim;
    attribs.ThreadGroupCountY = yDim;
    attribs.ThreadGroupCountZ = zDim;

    auto device = graphics_->GetImpl()->GetDeviceContext();
    device->DispatchCompute(attribs);
}

void ComputeDevice::HandleGPUResourceRelease(StringHash eventID, VariantMap& eventData)
{
    SharedPtr<Object> object(dynamic_cast<Object*>(eventData["GPUObject"].GetPtr()));
    if (object == nullptr)
        return;
    void* gpuObject = ((GPUObject*)object.Get())->GetGPUObject();

    auto foundBuffUAV = constructedBufferUAVs_.find(object);
    if (foundBuffUAV != constructedBufferUAVs_.end())
    {
        resourcesDirty_ |= ComputeDevice_ClearResource(foundBuffUAV->second, resources_);
        constructedBufferUAVs_.erase(foundBuffUAV);
    }

    UnsubscribeFromEvent(object.Get(), E_GPURESOURCERELEASED);

}

void ComputeDevice::ReleaseLocalState()
{
    constructedBufferUAVs_.clear();
    cachedPipelines_.clear();
    resources_.clear();

    pipeline_ = nullptr;
    srb_ = nullptr;
}

} // namespace Urho3D
