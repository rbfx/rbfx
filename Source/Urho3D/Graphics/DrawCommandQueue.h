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

#pragma once

#include "../Core/Object.h"
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/ConstantBufferCollection.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../RenderAPI/PipelineState.h"
#include "../IO/Log.h"
#include "Urho3D/RenderAPI/ShaderProgramReflection.h"

namespace Urho3D
{

class Graphics;
class Texture;

/// Reference to input shader resource. Only textures are supported now.
struct ShaderResourceDesc
{
    StringHash name_{};
    Texture* texture_{};
};

/// Generic description of shader parameter.
/// Beware of Variant allocations for types larger than Vector4!
struct ShaderParameterDesc
{
    StringHash name_;
    Variant value_;
};

/// Shader resource group, range in array.
using ShaderResourceRange = ea::pair<unsigned, unsigned>;

/// Set of input buffers with vertex and index data.
struct GeometryBufferArray
{
    IndexBuffer* indexBuffer_{};
    ea::array<VertexBuffer*, MAX_VERTEX_STREAMS> vertexBuffers_{};

    GeometryBufferArray() = default;

    GeometryBufferArray(std::initializer_list<VertexBuffer*> vertexBuffers,
        IndexBuffer* indexBuffer, VertexBuffer* instancingBuffer)
    {
        ea::span<VertexBuffer* const> tempVertexBuffers(vertexBuffers.begin(), static_cast<unsigned>(vertexBuffers.size()));
        Initialize(tempVertexBuffers, indexBuffer, instancingBuffer);
    }

    template <class Container>
    GeometryBufferArray(Container&& vertexBuffers, IndexBuffer* indexBuffer, VertexBuffer* instancingBuffer)
    {
        Initialize(vertexBuffers, indexBuffer, instancingBuffer);
    }

    explicit GeometryBufferArray(const Geometry* geometry, VertexBuffer* instancingBuffer = nullptr);

private:
    template <class Container>
    void Initialize(Container&& vertexBuffers, IndexBuffer* indexBuffer, VertexBuffer* instancingBuffer)
    {
        using eastl::size;
        using eastl::begin;
        using eastl::end;

        const unsigned numVertexBuffers = size(vertexBuffers);
        assert(numVertexBuffers + !!instancingBuffer <= MAX_VERTEX_STREAMS);

        const auto iter = ea::copy(begin(vertexBuffers), end(vertexBuffers), vertexBuffers_.begin());
        if (instancingBuffer)
            *iter = instancingBuffer;

        indexBuffer_ = indexBuffer;
    }
};

/// Description of draw command.
struct DrawCommandDescription
{
    PipelineState* pipelineState_{};
    GeometryBufferArray inputBuffers_;

    ea::array<ConstantBufferCollectionRef, MAX_SHADER_PARAMETER_GROUPS> constantBuffers_;

    ShaderResourceRange shaderResources_;

    /// Index of scissor rectangle. 0 if disabled.
    unsigned scissorRect_{};

    /// Draw call parameters
    /// @{
    unsigned indexStart_{};
    unsigned indexCount_{};
    unsigned baseVertexIndex_{};
    unsigned instanceStart_{};
    unsigned instanceCount_{};
    /// @}
};

/// Queue of draw commands.
class DrawCommandQueue : public RefCounted
{
public:
    /// Construct.
    DrawCommandQueue(Graphics* graphics);

    /// Reset queue.
    void Reset();

    /// Set pipeline state. Must be called first.
    void SetPipelineState(PipelineState* pipelineState)
    {
        assert(pipelineState);
        currentDrawCommand_.pipelineState_ = pipelineState;
        currentShaderProgramReflection_ = pipelineState->GetReflection();
    }

    /// Set scissor rect.
    void SetScissorRect(const IntRect& scissorRect)
    {
        if (scissorRects_.size() > 1 && scissorRects_.back() == scissorRect)
            return;

        currentDrawCommand_.scissorRect_ = scissorRects_.size();
        scissorRects_.push_back(scissorRect);
    }

    /// Begin shader parameter group. All parameters shall be set for each draw command.
    bool BeginShaderParameterGroup(ShaderParameterGroup group, bool differentFromPrevious = false)
    {
        const UniformBufferReflection* uniformBuffer = currentShaderProgramReflection_->GetUniformBuffer(group);
        if (!uniformBuffer)
        {
            // If contents changed, forget cached constant buffer
            if (differentFromPrevious)
                constantBuffers_.currentHashes_[group] = 0;
            return false;
        }

        // If data and/or layout changed, rebuild block
        if (differentFromPrevious || uniformBuffer->hash_ != constantBuffers_.currentHashes_[group])
        {
            const auto& refAndData = constantBuffers_.collection_.AddBlock(uniformBuffer->size_);

            currentDrawCommand_.constantBuffers_[group] = refAndData.first;
            constantBuffers_.currentData_ = refAndData.second;
            constantBuffers_.currentHashes_[group] = uniformBuffer->hash_;
            constantBuffers_.currentGroup_ = group;
            return true;
        }

        return false;
    }

    /// Add shader parameter. Shall be called only if BeginShaderParameterGroup returned true.
    template <class T>
    void AddShaderParameter(StringHash name, const T& value)
    {
        const auto* paramInfo = currentShaderProgramReflection_->GetUniform(name);
        if (paramInfo)
        {
            if (constantBuffers_.currentGroup_ != paramInfo->group_)
            {
                URHO3D_LOGERROR("Shader parameter #{} '{}' shall be stored in group {} instead of group {}",
                    name.Value(), name.Reverse(), constantBuffers_.currentGroup_, paramInfo->group_);
                return;
            }

            if (!ConstantBufferCollection::StoreParameter(constantBuffers_.currentData_ + paramInfo->offset_,
                paramInfo->size_, value))
            {
                URHO3D_LOGERROR("Shader parameter #{} '{}' has unexpected type, {} bytes expected",
                    name.Value(), name.Reverse(), paramInfo->size_);
            }
        }
    }

    /// Commit shader parameter group. Shall be called only if BeginShaderParameterGroup returned true.
    void CommitShaderParameterGroup(ShaderParameterGroup group)
    {
        // All data is already stored, nothing to do
        constantBuffers_.currentGroup_ = MAX_SHADER_PARAMETER_GROUPS;
    }

    /// Add shader resource.
    void AddShaderResource(StringHash name, Texture* texture)
    {
        const ShaderResourceReflection* shaderParameter = currentShaderProgramReflection_->GetShaderResource(name);
        if (!shaderParameter || !shaderParameter->variable_)
            return;

        shaderResources_.push_back(ShaderResourceData{shaderParameter->variable_, texture});
        ++currentShaderResourceGroup_.second;
    }

    /// Commit shader resources added since previous commit.
    void CommitShaderResources()
    {
        currentDrawCommand_.shaderResources_ = currentShaderResourceGroup_;
        currentShaderResourceGroup_.first = shaderResources_.size();
        currentShaderResourceGroup_.second = currentShaderResourceGroup_.first;
    }

    /// Set vertex and index buffers.
    void SetBuffers(const GeometryBufferArray& buffers)
    {
        currentDrawCommand_.inputBuffers_ = buffers;
    }

    /// Enqueue draw non-indexed geometry.
    void Draw(unsigned vertexStart, unsigned vertexCount)
    {
        currentDrawCommand_.inputBuffers_.indexBuffer_ = nullptr;
        currentDrawCommand_.indexStart_ = vertexStart;
        currentDrawCommand_.indexCount_ = vertexCount;
        currentDrawCommand_.baseVertexIndex_ = 0;
        currentDrawCommand_.instanceStart_ = 0;
        currentDrawCommand_.instanceCount_ = 0;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw indexed geometry.
    void DrawIndexed(unsigned indexStart, unsigned indexCount)
    {
        assert(currentDrawCommand_.inputBuffers_.indexBuffer_);
        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = 0;
        currentDrawCommand_.instanceStart_ = 0;
        currentDrawCommand_.instanceCount_ = 0;
        // TODO(diligent): Revisit this
        // fix: don't push command if index start and index count is 0
        if (indexStart == 0 && indexCount == 0)
            return;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw indexed geometry with vertex index offset.
    void DrawIndexed(unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex)
    {
        assert(currentDrawCommand_.inputBuffers_.indexBuffer_);
        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = baseVertexIndex;
        currentDrawCommand_.instanceStart_ = 0;
        currentDrawCommand_.instanceCount_ = 0;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw indexed, instanced geometry.
    void DrawIndexedInstanced(unsigned indexStart, unsigned indexCount, unsigned instanceStart, unsigned instanceCount)
    {
        assert(currentDrawCommand_.inputBuffers_.indexBuffer_);
        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = 0;
        currentDrawCommand_.instanceStart_ = instanceStart;
        currentDrawCommand_.instanceCount_ = instanceCount;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw indexed, instanced geometry with vertex index offset.
    void DrawIndexedInstanced(unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex,
        unsigned instanceStart, unsigned instanceCount)
    {
        assert(currentDrawCommand_.inputBuffers_.indexBuffer_);
        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = baseVertexIndex;
        currentDrawCommand_.instanceStart_ = instanceStart;
        currentDrawCommand_.instanceCount_ = instanceCount;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Execute commands in the queue.
    void Execute();

private:
    /// Cached pointer to Graphics.
    Graphics* graphics_{};

    /// Shader parameters data when constant buffers are used.
    struct ConstantBuffersData
    {
        /// Constant buffers.
        ConstantBufferCollection collection_;

        /// Current constant buffer group.
        ShaderParameterGroup currentGroup_{ MAX_SHADER_PARAMETER_GROUPS };
        /// Current pointer to constant buffer data.
        unsigned char* currentData_{};
        /// Current constant buffer layout hashes.
        ea::array<unsigned, MAX_SHADER_PARAMETER_GROUPS> currentHashes_{};
    } constantBuffers_;

    struct ShaderResourceData
    {
        Diligent::IShaderResourceVariable* variable_{};
        Texture* texture_{};
    };

    /// Shader resources.
    ea::vector<ShaderResourceData> shaderResources_;
    /// Scissor rects.
    ea::vector<IntRect> scissorRects_;
    /// Draw operations.
    ea::vector<DrawCommandDescription> drawCommands_;

    /// Current draw operation.
    DrawCommandDescription currentDrawCommand_;
    /// Current shader resource group.
    ShaderResourceRange currentShaderResourceGroup_;
    ShaderProgramReflection* currentShaderProgramReflection_{};

};

}
