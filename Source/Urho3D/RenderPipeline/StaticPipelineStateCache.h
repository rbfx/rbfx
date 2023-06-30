// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

/// Cache for static set of pipeline states.
/// It is possible to reuse same pipeline state for different outputs.
class URHO3D_API StaticPipelineStateCache : public Object
{
    URHO3D_OBJECT(StaticPipelineStateCache, Object);

public:
    /// Construct.
    explicit StaticPipelineStateCache(Context* context);

    /// Invalidate all pipeline states.
    void Invalidate();

    /// Create new pipeline state.
    StaticPipelineStateId CreateState(const GraphicsPipelineStateDesc& desc);
    /// Get or create pipeline state for given ID and output layout.
    PipelineState* GetState(StaticPipelineStateId id, const PipelineStateOutputDesc& outputDesc);

private:
    struct PerOutputCache
    {
        PipelineStateOutputDesc outputDesc_;
        ea::vector<SharedPtr<PipelineState>> pipelineStates_;
    };

    ea::vector<GraphicsPipelineStateDesc> desc_;
    ea::unordered_map<unsigned, PerOutputCache> caches_;
};

}
