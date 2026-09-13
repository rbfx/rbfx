// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"

#include <EASTL/string.h>
#include <EASTL/unordered_set.h>
#include <EASTL/vector.h>

namespace Urho3D
{

class Drawable;
class DrawableProcessor;
class Geometry;
class Light;
class Material;
class PipelineState;
class RawShader;
struct PipelineBatch;

struct URHO3D_API DebugFrameSnapshotBatch
{
    Drawable* drawable_{};
    Geometry* geometry_{};
    Material* material_{};
    Light* light_{};
    PipelineState* pipelineState_{};
    unsigned sourceBatchIndex_{};
    float distance_{};
    unsigned numVertices_{};
    unsigned numPrimitives_{};
    bool newInstancingGroup_{};

    DebugFrameSnapshotBatch() = default;
    DebugFrameSnapshotBatch(const DrawableProcessor& drawableProcessor,
        const PipelineBatch& pipelineBatch, bool newInstancingGroup);
    ea::string ToString() const;
};

struct URHO3D_API DebugFrameSnapshotQuad
{
    ea::string debugComment_;
    IntVector2 size_;

    ea::string ToString() const;
};

struct URHO3D_API DebugFrameSnapshotPass
{
    ea::string name_;
    ea::vector<DebugFrameSnapshotBatch> batches_;
    ea::vector<DebugFrameSnapshotQuad> quads_;

    ea::string ToString() const;
};

struct URHO3D_API DebugFrameSnapshot
{
    ea::vector<DebugFrameSnapshotPass> passes_;
    ea::unordered_set<PipelineState*> scenePipelineStates_{};
    ea::unordered_set<Material*> sceneMaterials_{};
    ea::unordered_set<RawShader*> sceneShaders_{};

    ea::string ToString() const;
    ea::string ScenePipelineStatesToString() const;
    ea::string SceneMaterialsToString() const;
    ea::string SceneShadersToString() const;
};;

/// Debug utility that takes snapshot of current frame.
class URHO3D_API RenderPipelineDebugger
{
public:
    RenderPipelineDebugger() {}

    /// Flow control
    /// @{
    void BeginSnapshot();
    void EndSnapshot();
    /// @}

    /// Debug info reporting. Should be called from main thread.
    /// @{
    void BeginPass(ea::string_view name);
    void ReportSceneBatch(const DebugFrameSnapshotBatch& sceneBatch);
    void ReportQuad(ea::string_view debugComment, const IntVector2& size = IntVector2::ZERO);
    void EndPass();
    /// @}

    bool IsSnapshotInProgress() const { return snapshotBuildingInProgress_; }
    static bool IsSnapshotInProgress(const RenderPipelineDebugger* debugger) { return debugger && debugger->IsSnapshotInProgress(); }
    const DebugFrameSnapshot& GetSnapshot() const { return snapshot_; }

private:
    bool snapshotBuildingInProgress_{};
    DebugFrameSnapshot snapshot_;

    bool passInProgress_{};
};

}
