//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/DrawCommandQueue.h"

#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/GraphicsImpl.h"
#include "Urho3D/Graphics/RenderSurface.h"
#include "Urho3D/Graphics/Texture.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/RenderAPI/RenderAPIUtils.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

Diligent::VALUE_TYPE GetIndexType(IndexBuffer* indexBuffer)
{
    return indexBuffer->GetIndexSize() == 2 ? Diligent::VT_UINT16 : Diligent::VT_UINT32;
}

}

GeometryBufferArray::GeometryBufferArray(const Geometry* geometry, VertexBuffer* instancingBuffer)
    : GeometryBufferArray(geometry->GetVertexBuffers(), geometry->GetIndexBuffer(), instancingBuffer)
{
}

DrawCommandQueue::DrawCommandQueue(Graphics* graphics)
    : graphics_(graphics)
{
}

void DrawCommandQueue::Reset()
{
    // Reset state accumulators
    currentDrawCommand_ = {};
    currentShaderResourceGroup_ = {};
    currentShaderProgramReflection_ = nullptr;

    // Clear shadep parameters
    constantBuffers_.collection_.ClearAndInitialize(graphics_->GetCaps().constantBufferOffsetAlignment_);
    constantBuffers_.currentData_ = nullptr;
    constantBuffers_.currentHashes_.fill(0);

    currentDrawCommand_.constantBuffers_.fill({});

    // Clear arrays and draw commands
    shaderResources_.clear();
    drawCommands_.clear();
    scissorRects_.clear();
    scissorRects_.push_back(IntRect::ZERO);
}

void DrawCommandQueue::Execute()
{
    if (drawCommands_.empty())
        return;

    RenderContext* renderContext = graphics_->GetRenderContext();
    Diligent::IDeviceContext* deviceContext = graphics_->GetImpl()->GetDeviceContext();

    const RenderBackend& backend = renderContext->GetRenderDevice()->GetBackend();
    const bool isBaseVertexAndInstanceSupported = !IsOpenGLESBackend(backend);

    // Constant buffers to store all shader parameters for queue
    ea::vector<Diligent::IBuffer*> uniformBuffers;
    const unsigned numUniformBuffers = constantBuffers_.collection_.GetNumBuffers();
    uniformBuffers.resize(numUniformBuffers);
    for (unsigned i = 0; i < numUniformBuffers; ++i)
    {
        const unsigned size = constantBuffers_.collection_.GetGPUBufferSize(i);
        ConstantBuffer* uniformBuffer = graphics_->GetOrCreateConstantBuffer(VS, i, size);
        uniformBuffer->Update(constantBuffers_.collection_.GetBufferData(i));
        uniformBuffers[i] = uniformBuffer->GetHandle();
    }

    // Cached current state
    PipelineState* currentPipelineState = nullptr;
    Diligent::IShaderResourceBinding* currentShaderResourceBinding = nullptr;
    ShaderProgramReflection* currentShaderReflection = nullptr;
    IndexBuffer* currentIndexBuffer = nullptr;
    ea::array<VertexBuffer*, MAX_VERTEX_STREAMS> currentVertexBuffers{};
    ShaderResourceRange currentShaderResources;
    PrimitiveType currentPrimitiveType{};
    unsigned currentScissorRect = M_MAX_UNSIGNED;

    // Set common state
    const float blendFactors[] = {1.0f, 1.0f, 1.0f, 1.0f};
    deviceContext->SetBlendFactors(blendFactors);

    for (const DrawCommandDescription& cmd : drawCommands_)
    {
        if (cmd.baseVertexIndex_ != 0 && !isBaseVertexAndInstanceSupported)
        {
            URHO3D_LOGWARNING("Base vertex index is not supported by current graphics API");
            continue;
        }

        // Set pipeline state
        if (cmd.pipelineState_ != currentPipelineState)
        {
            // TODO(diligent): This is used for shader reloading. Make better?
            cmd.pipelineState_->Restore();

            // TODO(diligent): Revisit error checking. Use default pipeline?
            if (!cmd.pipelineState_->GetHandle())
                continue;

            // Skip this pipeline if something goes wrong.
            deviceContext->SetPipelineState(cmd.pipelineState_->GetHandle());
            deviceContext->SetStencilRef(cmd.pipelineState_->GetDesc().stencilReferenceValue_);

            currentPipelineState = cmd.pipelineState_;
            currentShaderResourceBinding = cmd.pipelineState_->GetShaderResourceBinding();
            currentShaderReflection = cmd.pipelineState_->GetReflection();
            currentPrimitiveType = currentPipelineState->GetDesc().primitiveType_;

            // Reset current shader resources because mapping can be different
            currentShaderResources = {};
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

        // Set index buffer
        if (cmd.inputBuffers_.indexBuffer_ != currentIndexBuffer)
        {
            Diligent::IBuffer* indexBufferHandle =
                cmd.inputBuffers_.indexBuffer_ ? cmd.inputBuffers_.indexBuffer_->GetHandle() : nullptr;
            deviceContext->SetIndexBuffer(indexBufferHandle, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

            currentIndexBuffer = cmd.inputBuffers_.indexBuffer_;
        }

        // Set vertex buffers
        if (cmd.inputBuffers_.vertexBuffers_ != currentVertexBuffers || cmd.instanceCount_ != 0)
        {
            ea::array<Diligent::IBuffer*, MaxVertexStreams> vertexBufferHandles{};
            ea::array<Diligent::Uint64, MaxVertexStreams> vertexBufferOffsets{};

            for (unsigned i = 0; i < MaxVertexStreams; ++i)
            {
                VertexBuffer* vertexBuffer = cmd.inputBuffers_.vertexBuffers_[i];
                if (!vertexBuffer)
                    continue;

                vertexBuffer->Resolve();

                const ea::vector<VertexElement>& vertexElements = vertexBuffer->GetElements();
                const bool needInstanceOffset =
                    !isBaseVertexAndInstanceSupported && !vertexElements.empty() && vertexElements[0].perInstance_;

                vertexBufferHandles[i] = vertexBuffer->GetHandle();
                vertexBufferOffsets[i] = needInstanceOffset ? cmd.instanceStart_ * vertexBuffer->GetVertexSize() : 0;
            }

            deviceContext->SetVertexBuffers(0, MaxVertexStreams, vertexBufferHandles.data(), vertexBufferOffsets.data(),
                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_NONE);

            currentVertexBuffers = cmd.inputBuffers_.vertexBuffers_;
        }

        // Set resources
        for (unsigned i = cmd.shaderResources_.first; i < cmd.shaderResources_.second; ++i)
        {
            const ShaderResourceData& data = shaderResources_[i];
            Texture* texture = data.texture_;

            if (renderContext->IsBoundAsRenderTarget(texture))
                texture = texture->GetBackupTexture(); // TODO(diligent): We should have default backup texture!
            if (!texture)
                continue;
            if (texture->GetResolveDirty())
                texture->Resolve();
            if (texture->GetLevelsDirty())
                texture->GenerateLevels();

            data.variable_->Set(texture->GetHandles().srv_);
        }

        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i)
        {
            const auto group = static_cast<ShaderParameterGroup>(i);
            const UniformBufferReflection* uniformBufferReflection = currentShaderReflection->GetUniformBuffer(group);
            if (!uniformBufferReflection)
                continue;

            Diligent::IBuffer* uniformBuffer = uniformBuffers[cmd.constantBuffers_[i].index_];
            for (Diligent::IShaderResourceVariable* variable : uniformBufferReflection->variables_)
            {
                variable->SetBufferRange(uniformBuffer, cmd.constantBuffers_[i].offset_, cmd.constantBuffers_[i].size_);
            }
        }

        deviceContext->CommitShaderResources(
            currentShaderResourceBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        if (currentIndexBuffer)
        {
            Diligent::DrawIndexedAttribs drawAttrs;
            drawAttrs.NumIndices = cmd.indexCount_;
            drawAttrs.NumInstances = ea::max(1u, cmd.instanceCount_);
            drawAttrs.FirstIndexLocation = cmd.indexStart_;
            drawAttrs.FirstInstanceLocation = isBaseVertexAndInstanceSupported ? cmd.instanceStart_ : 0;
            drawAttrs.BaseVertex = cmd.baseVertexIndex_;
            drawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            drawAttrs.IndexType = GetIndexType(currentIndexBuffer);

            deviceContext->DrawIndexed(drawAttrs);
        }
        else
        {
            Diligent::DrawAttribs drawAttrs;
            drawAttrs.NumVertices = cmd.indexCount_;
            drawAttrs.NumInstances = ea::max(1u, cmd.instanceCount_);
            drawAttrs.StartVertexLocation = cmd.indexStart_;
            drawAttrs.FirstInstanceLocation = isBaseVertexAndInstanceSupported ? cmd.instanceStart_ : 0;
            drawAttrs.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;

            deviceContext->Draw(drawAttrs);
        }
    }
}

}
