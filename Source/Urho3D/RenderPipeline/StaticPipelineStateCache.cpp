// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/StaticPipelineStateCache.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

StaticPipelineStateCache::StaticPipelineStateCache(Context* context)
    : Object(context)
{
}

void StaticPipelineStateCache::Invalidate()
{
    caches_.clear();
}

StaticPipelineStateId StaticPipelineStateCache::CreateState(const GraphicsPipelineStateDesc& desc)
{
    const auto iter = descToIndex_.find(desc);
    if (iter != descToIndex_.end())
        return iter->second;

    const auto index = static_cast<StaticPipelineStateId>(desc_.size() + 1);
    desc_.push_back(desc);
    descToIndex_.emplace(desc, index);
    return index;
}

PipelineState* StaticPipelineStateCache::GetState(StaticPipelineStateId id, const PipelineStateOutputDesc& outputDesc)
{
    const unsigned index = static_cast<unsigned>(id) - 1;
    if (id == StaticPipelineStateId::Invalid || index >= desc_.size())
    {
        URHO3D_ASSERT(false, "StaticPipelineStateCache::GetState is called with invalid id");
        return nullptr;
    }

    const unsigned outputHash = outputDesc.ToHash();

    // Try to find cached state
    const auto iter = caches_.find(outputHash);
    if (iter != caches_.end())
    {
        auto& states = iter->second.pipelineStates_;
        if (index < states.size() && states[index])
            return states[index];
    }

    // Create new state
    PerOutputCache& cache = caches_[outputHash];
    cache.outputDesc_ = outputDesc;

    GraphicsPipelineStateDesc pipelineDesc = desc_[index];
    pipelineDesc.output_ = outputDesc;

    auto pipelineStateCache = GetSubsystem<PipelineStateCache>();
    auto pipelineState = pipelineStateCache->GetGraphicsPipelineState(pipelineDesc);

    auto& states = cache.pipelineStates_;
    if (index >= states.size())
        states.resize(index + 1);
    states[index] = pipelineState;

    return pipelineState;
}

}
