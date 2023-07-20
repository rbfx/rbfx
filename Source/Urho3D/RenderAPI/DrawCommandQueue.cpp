// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/DrawCommandQueue.h"

#include "Urho3D/RenderAPI/RawBuffer.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderAPI/RenderPool.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

Diligent::VALUE_TYPE GetIndexType(RawBuffer* indexBuffer)
{
    return indexBuffer->GetStride() == 2 ? Diligent::VT_UINT16 : Diligent::VT_UINT32;
}

RawTexture* GetReadableTexture(
    RenderContext* renderContext, TextureType type, RawTexture* texture, RawTexture* backupTexture)
{
    if (texture && !renderContext->IsBoundAsRenderTarget(texture))
        return texture;
    else if (backupTexture && !renderContext->IsBoundAsRenderTarget(backupTexture))
        return backupTexture;
    else
        return renderContext->GetRenderDevice()->GetDefaultTexture(type);
}

}

DrawCommandQueue::DrawCommandQueue(RenderDevice* renderDevice)
    : renderDevice_(renderDevice)
{
}

void DrawCommandQueue::Reset()
{
    clipPlaneEnabled_ = false;

    // Reset state accumulators
    currentDrawCommand_ = {};
    currentShaderResourceGroup_ = {};
    currentUnorderedAccessViewGroup_ = {};
    currentShaderProgramReflection_ = nullptr;

    // Clear shader parameters
    constantBuffers_.collection_.ClearAndInitialize(renderDevice_->GetCaps().constantBufferOffsetAlignment_);
    constantBuffers_.currentData_ = nullptr;
    constantBuffers_.currentHashes_.fill(0);

    currentDrawCommand_.constantBuffers_.fill({});

    // Clear arrays and draw commands
    shaderResources_.clear();
    unorderedAccessViews_.clear();
    drawCommands_.clear();
    scissorRects_.clear();
    scissorRects_.push_back(IntRect::ZERO);
}

void DrawCommandQueue::ExecuteInContext(RenderContext* renderContext)
{
    if (drawCommands_.empty())
        return;

    RenderPool* renderPool = renderContext->GetRenderPool();
    Diligent::IDeviceContext* deviceContext = renderContext->GetHandle();
    RenderDeviceStats& stats = renderContext->GetMutableStats();

    const RenderBackend& backend = renderContext->GetRenderDevice()->GetBackend();
    const RenderDeviceCaps& caps = renderContext->GetRenderDevice()->GetCaps();

    // Update constant buffers to store all shader parameters for queue
    const unsigned numUniformBuffers = constantBuffers_.collection_.GetNumBuffers();
    temp_.uniformBuffers_.resize(numUniformBuffers);
    for (unsigned i = 0; i < numUniformBuffers; ++i)
    {
        const unsigned size = constantBuffers_.collection_.GetGPUBufferSize(i);
        RawBuffer* uniformBuffer = renderPool->GetUniformBuffer(i, size);
        uniformBuffer->Update(constantBuffers_.collection_.GetBufferData(i));
        temp_.uniformBuffers_[i] = uniformBuffer->GetHandle();
    }

    // Update shader resources
    temp_.shaderResourceViews_.resize(shaderResources_.size());
    for (unsigned i = 0; i < shaderResources_.size(); ++i)
    {
        const ShaderResourceData& data = shaderResources_[i];

        RawTexture* texture = GetReadableTexture(renderContext, data.type_, data.texture_, data.backupTexture_);
        if (texture->GetResolveDirty())
            texture->Resolve();
        if (texture->GetLevelsDirty())
            texture->GenerateLevels();

        temp_.shaderResourceViews_[i] = texture->GetHandles().srv_;
    }

    // Cached current state
    PipelineState* currentPipelineState = nullptr;
    Diligent::IShaderResourceBinding* currentShaderResourceBinding = nullptr;
    ShaderProgramReflection* currentShaderReflection = nullptr;
    RawBuffer* currentIndexBuffer = nullptr;
    RawVertexBufferArray currentVertexBuffers{};
    ShaderResourceRange currentShaderResources;
    ShaderResourceRange currentUnorderedAccessViews;
    unsigned currentScissorRect = M_MAX_UNSIGNED;
    ea::optional<unsigned> currentStencilRef;

    // Set common state
    const float blendFactors[] = {1.0f, 1.0f, 1.0f, 1.0f};
    deviceContext->SetBlendFactors(blendFactors);
    renderContext->SetClipPlaneEnabled(clipPlaneEnabled_);

    for (const DrawCommandDescription& cmd : drawCommands_)
    {
        if (cmd.baseVertexIndex_ != 0 && !caps.drawBaseVertex_)
        {
            URHO3D_LOGWARNING("Base vertex index is not supported by current graphics API");
            continue;
        }

        // Set pipeline state
        if (cmd.pipelineState_ != currentPipelineState)
        {
            if (!cmd.pipelineState_->GetHandle())
                continue; // We shouldn't get here

            // Skip this pipeline if something goes wrong.
            deviceContext->SetPipelineState(cmd.pipelineState_->GetHandle());

            currentPipelineState = cmd.pipelineState_;
            currentShaderResourceBinding = cmd.pipelineState_->GetShaderResourceBinding();
            currentShaderReflection = cmd.pipelineState_->GetReflection();

            // Reset current shader resources because mapping can be different
            currentShaderResources = {};
            currentUnorderedAccessViews = {};
        }

        // Set scissor
        if (cmd.scissorRect_ != currentScissorRect)
        {
            const IntRect& scissorRect = scissorRects_[cmd.scissorRect_];

            Diligent::Rect internalRect;
            internalRect.left = scissorRect.left_;
            internalRect.top = scissorRect.top_;
            internalRect.right = scissorRect.right_;
            internalRect.bottom = scissorRect.bottom_;

            deviceContext->SetScissorRects(1, &internalRect, 0, 0);
            currentScissorRect = cmd.scissorRect_;
        }

        // Set stencil ref
        if (cmd.stencilRef_ != currentStencilRef)
        {
            deviceContext->SetStencilRef(cmd.stencilRef_);
            currentStencilRef = cmd.stencilRef_;
        }

        // Set index buffer
        if (cmd.indexBuffer_ != currentIndexBuffer)
        {
            if (cmd.indexBuffer_)
                cmd.indexBuffer_->Resolve();

            Diligent::IBuffer* indexBufferHandle = cmd.indexBuffer_ ? cmd.indexBuffer_->GetHandle() : nullptr;
            deviceContext->SetIndexBuffer(indexBufferHandle, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            currentIndexBuffer = cmd.indexBuffer_;
        }

        // Set vertex buffers
        if (cmd.vertexBuffers_ != currentVertexBuffers
            || (cmd.instanceCount_ != 0 && !caps.drawBaseInstance_))
        {
            ea::array<Diligent::IBuffer*, MaxVertexStreams> vertexBufferHandles{};
            ea::array<Diligent::Uint64, MaxVertexStreams> vertexBufferOffsets{};

            for (unsigned i = 0; i < MaxVertexStreams; ++i)
            {
                RawBuffer* vertexBuffer = cmd.vertexBuffers_[i];
                if (!vertexBuffer)
                    continue;

                vertexBuffer->Resolve();

                const bool needInstanceOffset =
                    !caps.drawBaseInstance_ && vertexBuffer->GetFlags().Test(BufferFlag::PerInstanceData);

                vertexBufferHandles[i] = vertexBuffer->GetHandle();
                vertexBufferOffsets[i] = needInstanceOffset ? cmd.instanceStart_ * vertexBuffer->GetStride() : 0;
            }

            deviceContext->SetVertexBuffers(0, MaxVertexStreams, vertexBufferHandles.data(), vertexBufferOffsets.data(),
                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_NONE);

            currentVertexBuffers = cmd.vertexBuffers_;
        }

        // Set resources
        if (currentShaderResources != cmd.shaderResources_)
        {
            for (unsigned i = cmd.shaderResources_.first; i < cmd.shaderResources_.second; ++i)
                shaderResources_[i].variable_->Set(temp_.shaderResourceViews_[i]);

            currentShaderResources = cmd.shaderResources_;
        }

        if (currentUnorderedAccessViews != cmd.unorderedAccessViews_)
        {
            for (unsigned i = cmd.unorderedAccessViews_.first; i < cmd.unorderedAccessViews_.second; ++i)
            {
                const UnorderedAccessViewData& data = unorderedAccessViews_[i];

                RawTexture* texture = data.texture_;
                if (texture->GetResolveDirty())
                    texture->Resolve();
                if (texture->GetLevelsDirty())
                    texture->GenerateLevels();

                data.variable_->Set(data.view_);
            }

            currentUnorderedAccessViews = cmd.unorderedAccessViews_;
        }

        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        {
            const auto group = static_cast<ShaderParameterGroup>(i);
            const UniformBufferReflection* uniformBufferReflection = currentShaderReflection->GetUniformBuffer(group);
            if (!uniformBufferReflection)
                continue;

            Diligent::IBuffer* uniformBuffer = temp_.uniformBuffers_[cmd.constantBuffers_[i].index_];
            for (Diligent::IShaderResourceVariable* variable : uniformBufferReflection->variables_)
            {
                variable->SetBufferRange(uniformBuffer, cmd.constantBuffers_[i].offset_, cmd.constantBuffers_[i].size_);
            }
        }

        deviceContext->CommitShaderResources(
            currentShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (currentPipelineState->GetPipelineType() == PipelineStateType::Graphics)
        {
            if (currentIndexBuffer)
            {
                Diligent::DrawIndexedAttribs drawAttrs;
                drawAttrs.NumIndices = cmd.indexCount_;
                drawAttrs.NumInstances = ea::max(1u, cmd.instanceCount_);
                drawAttrs.FirstIndexLocation = cmd.indexStart_;
                drawAttrs.FirstInstanceLocation = caps.drawBaseInstance_ ? cmd.instanceStart_ : 0;
                drawAttrs.BaseVertex = cmd.baseVertexIndex_;
                drawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
                drawAttrs.IndexType = GetIndexType(currentIndexBuffer);

                deviceContext->DrawIndexed(drawAttrs);

                stats.numDraws_ += 1;
                stats.numPrimitives_ += drawAttrs.NumIndices * drawAttrs.NumInstances;
            }
            else
            {
                Diligent::DrawAttribs drawAttrs;
                drawAttrs.NumVertices = cmd.indexCount_;
                drawAttrs.NumInstances = ea::max(1u, cmd.instanceCount_);
                drawAttrs.StartVertexLocation = cmd.indexStart_;
                drawAttrs.FirstInstanceLocation = caps.drawBaseInstance_ ? cmd.instanceStart_ : 0;
                drawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

                deviceContext->Draw(drawAttrs);

                stats.numDraws_ += 1;
                stats.numPrimitives_ += drawAttrs.NumVertices * drawAttrs.NumInstances;
            }
        }
        else if (currentPipelineState->GetPipelineType() == PipelineStateType::Compute)
        {
            Diligent::DispatchComputeAttribs dispatchAttrs;
            dispatchAttrs.ThreadGroupCountX = cmd.numGroups_.x_;
            dispatchAttrs.ThreadGroupCountY = cmd.numGroups_.y_;
            dispatchAttrs.ThreadGroupCountZ = cmd.numGroups_.z_;
            deviceContext->DispatchCompute(dispatchAttrs);

            stats.numDispatches_ += 1;
        }
    }
}

}
