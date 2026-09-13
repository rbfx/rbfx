// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/Geometry.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/GraphicsUtils.h"
#include "Urho3D/Graphics/Technique.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderPipeline/BatchStateCache.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

void BatchStateCache::Invalidate()
{
    cache_.clear();
}

void BatchStateCache::SetOutputDesc(const PipelineStateOutputDesc& outputDesc)
{
    if (outputDesc_ != outputDesc)
    {
        outputDesc_ = outputDesc;
        Invalidate();
    }
}

PipelineState* BatchStateCache::GetPipelineState(const BatchStateLookupKey& key) const
{
    URHO3D_ASSERT(outputDesc_);

    const auto iter = cache_.find(key);
    if (iter == cache_.end() || iter->second.invalidated_.load(std::memory_order_relaxed))
        return nullptr;

    const CachedBatchState& entry = iter->second;
    if (!entry.pipelineState_
        || key.geometry_->GetPipelineStateHash() != entry.geometryHash_
        || key.material_->GetPipelineStateHash() != entry.materialHash_
        || key.pass_->GetPipelineStateHash() != entry.passHash_)
    {
        entry.invalidated_.store(true, std::memory_order_relaxed);
        return nullptr;
    }

    return entry.pipelineState_;
}

PipelineState* BatchStateCache::GetOrCreatePipelineState(const BatchStateCreateKey& key,
    const BatchStateCreateContext& ctx, BatchStateCacheCallback* callback)
{
    URHO3D_ASSERT(outputDesc_);

    CachedBatchState& entry = cache_[key];
    if (!entry.pipelineState_ || entry.invalidated_.load(std::memory_order_relaxed)
        || key.geometry_->GetPipelineStateHash() != entry.geometryHash_
        || key.material_->GetPipelineStateHash() != entry.materialHash_
        || key.pass_->GetPipelineStateHash() != entry.passHash_)
    {
        entry.pipelineState_ = callback->CreateBatchPipelineState(key, ctx, *outputDesc_);
        entry.geometryHash_ = key.geometry_->GetPipelineStateHash();
        entry.materialHash_ = key.material_->GetPipelineStateHash();
        entry.passHash_ = key.pass_->GetPipelineStateHash();
        entry.invalidated_.store(false, std::memory_order_relaxed);
    }

    return entry.pipelineState_;
}

PipelineState* BatchStateCache::GetOrCreatePlaceholderPipelineState(
    unsigned vertexStride, BatchStateCacheCallback* callback)
{
    SharedPtr<PipelineState>& entry = placeholderCache_[vertexStride];
    if (!entry)
        entry = callback->CreateBatchPipelineStatePlaceholder(vertexStride, *outputDesc_);
    return entry && entry->IsValid() ? entry : nullptr;
}

void UIBatchStateCache::Invalidate()
{
    cache_.clear();
}

PipelineState* UIBatchStateCache::GetPipelineState(const UIBatchStateKey& key) const
{
    const auto iter = cache_.find(key);
    if (iter == cache_.end() || iter->second.invalidated_.load(std::memory_order_relaxed))
        return nullptr;

    const CachedUIBatchState& entry = iter->second;
    if (!entry.pipelineState_
        || key.material_->GetPipelineStateHash() != entry.materialHash_
        || key.pass_->GetPipelineStateHash() != entry.passHash_)
    {
        entry.invalidated_.store(true, std::memory_order_relaxed);
        return nullptr;
    }

    return entry.pipelineState_;
}

PipelineState* UIBatchStateCache::GetOrCreatePipelineState(const UIBatchStateKey& key,
    const UIBatchStateCreateContext& ctx, UIBatchStateCacheCallback* callback)
{
    CachedUIBatchState& entry = cache_[key];
    if (!entry.pipelineState_ || entry.invalidated_.load(std::memory_order_relaxed)
        || key.material_->GetPipelineStateHash() != entry.materialHash_
        || key.pass_->GetPipelineStateHash() != entry.passHash_)
    {
        entry.pipelineState_ = callback->CreateUIBatchPipelineState(key, ctx);
        entry.materialHash_ = key.material_->GetPipelineStateHash();
        entry.passHash_ = key.pass_->GetPipelineStateHash();
        entry.invalidated_.store(false, std::memory_order_relaxed);
    }

    return entry.pipelineState_;
}

PipelineState* DefaultUIBatchStateCache::GetOrCreatePipelineState(
    const UIBatchStateKey& key, const UIBatchStateCreateContext& ctx)
{
    return UIBatchStateCache::GetOrCreatePipelineState(key, ctx, this);
}

SharedPtr<PipelineState> DefaultUIBatchStateCache::CreateUIBatchPipelineState(
    const UIBatchStateKey& key, const UIBatchStateCreateContext& ctx)
{
    auto graphics = GetSubsystem<Graphics>();
    auto pipelineStateCache = GetSubsystem<PipelineStateCache>();

    GraphicsPipelineStateDesc desc;

    desc.debugName_ = Format("UI Batch for '{}'", key.material_->GetName());

    InitializeInputLayout(desc.inputLayout_, {ctx.vertexBuffer_});
    desc.primitiveType_ = TRIANGLE_LIST;
    desc.output_ = key.outputDesc_;
    desc.colorWriteEnabled_ = true;
    desc.cullMode_ = CULL_NONE;
    desc.depthCompareFunction_ = CMP_ALWAYS;
    desc.depthWriteEnabled_ = false;
    desc.fillMode_ = FILL_SOLID;
    desc.stencilTestEnabled_ = false;
    desc.blendMode_ = key.blendMode_;
    desc.scissorTestEnabled_ = true;

    for (const auto& [nameHash, texture] : key.material_->GetTextures())
    {
        if (texture.value_)
            desc.samplers_.Add(nameHash, texture.value_->GetSamplerStateDesc());
    }
    if (ctx.defaultSampler_)
        desc.samplers_.Add(ShaderResources::Albedo, *ctx.defaultSampler_);

    vertexShaderDefines_ = key.pass_->GetEffectiveVertexShaderDefines();
    pixelShaderDefines_ = key.pass_->GetEffectivePixelShaderDefines();

    if (key.linearOutput_)
    {
        vertexShaderDefines_ += " URHO3D_LINEAR_OUTPUT";
        pixelShaderDefines_ += " URHO3D_LINEAR_OUTPUT";
    }

    desc.vertexShader_ = graphics->GetShader(VS, key.pass_->GetVertexShader(), vertexShaderDefines_);
    desc.pixelShader_ = graphics->GetShader(PS, key.pass_->GetPixelShader(), pixelShaderDefines_);

    return pipelineStateCache->GetGraphicsPipelineState(desc);
}

}
