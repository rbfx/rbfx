// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/ConstantBufferCollection.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RawTexture.h"
#include "Urho3D/RenderAPI/ShaderProgramReflection.h"

namespace Urho3D
{

class RawBuffer;
class RenderContext;

/// Shader resource group, range in array.
using ShaderResourceRange = ea::pair<unsigned, unsigned>;

/// Description of draw command.
struct DrawCommandDescription
{
    PipelineState* pipelineState_{};
    RawVertexBufferArray vertexBuffers_{};
    RawBuffer* indexBuffer_{};

    ea::array<ConstantBufferCollectionRef, MAX_SHADER_PARAMETER_GROUPS> constantBuffers_;

    ShaderResourceRange shaderResources_;
    ShaderResourceRange unorderedAccessViews_;

    /// Index of scissor rectangle. 0 if disabled.
    unsigned scissorRect_{};
    /// Stencil reference value.
    unsigned stencilRef_{};

    /// Draw call parameters
    /// @{
    unsigned indexStart_{};
    unsigned indexCount_{};
    unsigned baseVertexIndex_{};
    unsigned instanceStart_{};
    unsigned instanceCount_{};
    IntVector3 numGroups_{};
    /// @}
};

/// Queue of draw commands.
class DrawCommandQueue : public RefCounted
{
public:
    /// Construct.
    DrawCommandQueue(RenderDevice* renderDevice);

    /// Reset queue.
    void Reset();

    /// Set clip plane enabled for all draw commands in the queue.
    void SetClipPlaneMask(unsigned mask) { clipPlaneMask_ = mask; }

    /// Set pipeline state. Must be called first.
    void SetPipelineState(PipelineState* pipelineState)
    {
        URHO3D_ASSERT(pipelineState);

        currentDrawCommand_.pipelineState_ = pipelineState;
        currentShaderProgramReflection_ = pipelineState->GetReflection();
    }

    /// Set stencil reference value.
    void SetStencilRef(unsigned ref)
    {
        currentDrawCommand_.stencilRef_ = ref;
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

    /// Add non-null shader resource.
    void AddShaderResource(StringHash name, RawTexture* texture, RawTexture* backupTexture = nullptr)
    {
        AddNullableShaderResource(name, texture->GetParams().type_, texture, backupTexture);
    }

    /// Add nullable shader resource.
    void AddNullableShaderResource(
        StringHash name, TextureType type, RawTexture* texture, RawTexture* backupTexture = nullptr)
    {
        const ShaderResourceReflection* shaderParameter = currentShaderProgramReflection_->GetShaderResource(name);
        if (!shaderParameter || !shaderParameter->variable_)
            return;

        shaderResources_.push_back(ShaderResourceData{shaderParameter->variable_, texture, backupTexture, type});
        ++currentShaderResourceGroup_.second;
    }

    /// Commit shader resources added since previous commit.
    void CommitShaderResources()
    {
        currentDrawCommand_.shaderResources_ = currentShaderResourceGroup_;
        currentShaderResourceGroup_.first = shaderResources_.size();
        currentShaderResourceGroup_.second = currentShaderResourceGroup_.first;
    }

    /// Add unordered access view.
    void AddUnorderedAccessView(StringHash name, RawTexture* texture, const RawTextureUAVKey& key)
    {
        const ShaderResourceReflection* uav = currentShaderProgramReflection_->GetUnorderedAccessView(name);
        if (!uav || !uav->variable_)
            return;

        Diligent::ITextureView* view = texture->GetUAV(key);
        if (!view)
        {
            URHO3D_ASSERTLOG(false, "Requested UAV for texture does not exist");
            return;
        }

        unorderedAccessViews_.push_back(UnorderedAccessViewData{uav->variable_, texture, view});
        ++currentUnorderedAccessViewGroup_.second;
    }

    /// Commit unordered access views added since previous commit.
    void CommitUnorderedAccessViews()
    {
        currentDrawCommand_.unorderedAccessViews_ = currentUnorderedAccessViewGroup_;
        currentUnorderedAccessViewGroup_.first = unorderedAccessViews_.size();
        currentUnorderedAccessViewGroup_.second = currentUnorderedAccessViewGroup_.first;
    }

    /// Set vertex buffers.
    void SetVertexBuffers(RawVertexBufferArray buffers)
    {
        currentDrawCommand_.vertexBuffers_ = buffers;
    }

    /// Set index buffer.
    void SetIndexBuffer(RawBuffer* buffer)
    {
        currentDrawCommand_.indexBuffer_ = buffer;
    }

    /// Enqueue draw non-indexed geometry.
    void Draw(unsigned vertexStart, unsigned vertexCount)
    {
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Graphics);
        URHO3D_ASSERT(!currentDrawCommand_.indexBuffer_);
        URHO3D_ASSERT(vertexCount > 0);

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
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Graphics);
        URHO3D_ASSERT(currentDrawCommand_.indexBuffer_);
        URHO3D_ASSERT(indexCount > 0);

        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = 0;
        currentDrawCommand_.instanceStart_ = 0;
        currentDrawCommand_.instanceCount_ = 0;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw indexed geometry with vertex index offset.
    void DrawIndexed(unsigned indexStart, unsigned indexCount, unsigned baseVertexIndex)
    {
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Graphics);
        URHO3D_ASSERT(currentDrawCommand_.indexBuffer_);
        URHO3D_ASSERT(indexCount > 0);

        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = baseVertexIndex;
        currentDrawCommand_.instanceStart_ = 0;
        currentDrawCommand_.instanceCount_ = 0;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw instanced geometry.
    void DrawInstanced(unsigned vertexStart, unsigned vertexCount, unsigned instanceStart, unsigned instanceCount)
    {
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Graphics);
        URHO3D_ASSERT(!currentDrawCommand_.indexBuffer_);
        URHO3D_ASSERT(vertexCount > 0);

        currentDrawCommand_.indexStart_ = vertexStart;
        currentDrawCommand_.indexCount_ = vertexCount;
        currentDrawCommand_.baseVertexIndex_ = 0;
        currentDrawCommand_.instanceStart_ = instanceStart;
        currentDrawCommand_.instanceCount_ = instanceCount;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Enqueue draw indexed, instanced geometry.
    void DrawIndexedInstanced(unsigned indexStart, unsigned indexCount, unsigned instanceStart, unsigned instanceCount)
    {
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Graphics);
        URHO3D_ASSERT(currentDrawCommand_.indexBuffer_);
        URHO3D_ASSERT(indexCount > 0);

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
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Graphics);
        URHO3D_ASSERT(currentDrawCommand_.indexBuffer_);
        URHO3D_ASSERT(indexCount > 0);

        currentDrawCommand_.indexStart_ = indexStart;
        currentDrawCommand_.indexCount_ = indexCount;
        currentDrawCommand_.baseVertexIndex_ = baseVertexIndex;
        currentDrawCommand_.instanceStart_ = instanceStart;
        currentDrawCommand_.instanceCount_ = instanceCount;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Dispatch compute shader.
    void Dispatch(const IntVector3& numGroups)
    {
        URHO3D_ASSERT(currentDrawCommand_.pipelineState_->GetPipelineType() == PipelineStateType::Compute);

        currentDrawCommand_.numGroups_ = numGroups;
        drawCommands_.push_back(currentDrawCommand_);
    }

    /// Execute commands in the queue.
    void ExecuteInContext(RenderContext* renderContext);

private:
    RenderDevice* renderDevice_{};

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
        RawTexture* texture_{};
        RawTexture* backupTexture_{};
        TextureType type_{};
    };

    struct UnorderedAccessViewData
    {
        Diligent::IShaderResourceVariable* variable_{};
        RawTexture* texture_{};
        Diligent::ITextureView* view_{};
    };

    /// Whether to enable clip plane.
    unsigned clipPlaneMask_{};

    /// Shader resources.
    ea::vector<ShaderResourceData> shaderResources_;
    /// Unordered access views.
    ea::vector<UnorderedAccessViewData> unorderedAccessViews_;
    /// Scissor rects.
    ea::vector<IntRect> scissorRects_;
    /// Draw operations.
    ea::vector<DrawCommandDescription> drawCommands_;

    /// Current draw operation.
    DrawCommandDescription currentDrawCommand_;
    /// Current shader resource group.
    ShaderResourceRange currentShaderResourceGroup_;
    /// Current unordered access view group.
    ShaderResourceRange currentUnorderedAccessViewGroup_;
    /// Current shader program reflection.
    ShaderProgramReflection* currentShaderProgramReflection_{};

    /// Temporary storage for executing draw commands.
    struct Temporary
    {
        ea::vector<Diligent::IBuffer*> uniformBuffers_;
        ea::vector<Diligent::ITextureView*> shaderResourceViews_;
    } temp_;
};

}
