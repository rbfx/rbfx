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

#include "../Precompiled.h"

#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsImpl.h"
#include "../Graphics/DrawCommandQueue.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Texture.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include "../DebugNew.h"

namespace Urho3D
{

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

    Diligent::IDeviceContext* deviceContext = graphics_->GetImpl()->GetDeviceContext();

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
    ShaderProgramLayout* currentShaderReflection = nullptr;
    IndexBuffer* currentIndexBuffer = nullptr;
    ea::array<VertexBuffer*, MAX_VERTEX_STREAMS> currentVertexBuffers{};
    ShaderResourceRange currentShaderResources;
    PrimitiveType currentPrimitiveType{};
    unsigned currentScissorRect = M_MAX_UNSIGNED;

    // Temporary collections
    ea::vector<VertexBuffer*> tempVertexBuffers;

    for (const DrawCommandDescription& cmd : drawCommands_)
    {
        // Set pipeline state
        if (cmd.pipelineState_ != currentPipelineState)
        {
            // Skip this pipeline if something goes wrong.
            if (!cmd.pipelineState_->Apply(graphics_))
                continue;
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
            const bool scissorEnabled = currentPipelineState->GetDesc().scissorTestEnabled_;
            graphics_->SetScissorTest(scissorEnabled, scissorRects_[cmd.scissorRect_]);
            currentScissorRect = cmd.scissorRect_;
        }

        // Set index buffer
        if (cmd.inputBuffers_.indexBuffer_ != currentIndexBuffer)
        {
            graphics_->SetIndexBuffer(cmd.inputBuffers_.indexBuffer_);
            currentIndexBuffer = cmd.inputBuffers_.indexBuffer_;
        }

        // Set vertex buffers
        if (cmd.inputBuffers_.vertexBuffers_ != currentVertexBuffers || cmd.instanceCount_ != 0)
        {
            tempVertexBuffers.clear();
            tempVertexBuffers.assign(cmd.inputBuffers_.vertexBuffers_.begin(), cmd.inputBuffers_.vertexBuffers_.end());
            for (VertexBuffer* vertexBuffer : tempVertexBuffers)
            {
                if (vertexBuffer)
                    vertexBuffer->Resolve();
            }
            // If something goes wrong here, skip current command
            if (!graphics_->SetVertexBuffers(tempVertexBuffers, cmd.instanceStart_))
                continue;
            currentVertexBuffers = cmd.inputBuffers_.vertexBuffers_;
        }

        // Set resources
        for (unsigned i = cmd.shaderResources_.first; i < cmd.shaderResources_.second; ++i)
        {
            const ShaderResourceData& data = shaderResources_[i];
            Texture* texture = data.texture_;

            // TODO(diligent): Revisit this place
            RenderSurface* currRT = graphics_->GetRenderTarget(0);
            if (currRT && currRT->GetParentTexture() == texture)
                texture = texture->GetBackupTexture(); // TODO(diligent): We should have default backup texture!
            if (!texture)
                continue;
            if (texture->GetLevelsDirty())
                texture->RegenerateLevels();

            data.variable_->Set(texture->GetShaderResourceView());
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

        // Invoke appropriate draw command
        const unsigned vertexStart = 0;
        const unsigned vertexCount = 0;
        if (cmd.instanceCount_ != 0)
        {
            if (cmd.baseVertexIndex_ == 0)
            {
                graphics_->DrawInstanced(currentPrimitiveType, cmd.indexStart_, cmd.indexCount_,
                    vertexStart, vertexCount, cmd.instanceCount_);
            }
            else
            {
                graphics_->DrawInstanced(currentPrimitiveType, cmd.indexStart_, cmd.indexCount_,
                    cmd.baseVertexIndex_, vertexStart, vertexCount, cmd.instanceCount_);
            }
        }
        else
        {
            if (!currentIndexBuffer)
            {
                graphics_->Draw(currentPrimitiveType, cmd.indexStart_, cmd.indexCount_);
            }
            else if (cmd.baseVertexIndex_ == 0)
            {
                graphics_->Draw(currentPrimitiveType, cmd.indexStart_, cmd.indexCount_,
                    vertexStart, vertexCount);
            }
            else
            {
                graphics_->Draw(currentPrimitiveType, cmd.indexStart_, cmd.indexCount_,
                    cmd.baseVertexIndex_, vertexStart, vertexCount);
            }
        }
    }
}

}
