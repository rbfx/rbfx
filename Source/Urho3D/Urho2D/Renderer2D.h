//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Graphics/Drawable.h"
#include "../Graphics/Texture2D.h"
#include "../Math/Frustum.h"

namespace Urho3D
{

class Drawable2D;
class IndexBuffer;
class Material;
class Technique;
class VertexBuffer;
struct FrameInfo;
struct SourceBatch2D;

/// 2D view batch info.
/// @nobind
struct ViewBatchInfo2D
{
    /// Construct.
    ViewBatchInfo2D();

    /// Vertex buffer update frame number.
    unsigned vertexBufferUpdateFrameNumber_;
    /// Index count.
    unsigned indexCount_;
    /// Vertex count.
    unsigned vertexCount_;
    /// Vertex buffer.
    SharedPtr<VertexBuffer> vertexBuffer_;
    /// Batch updated frame number.
    unsigned batchUpdatedFrameNumber_;
    /// Source batches.
    ea::vector<const SourceBatch2D*> sourceBatches_;
    /// Batch count.
    unsigned batchCount_;
    /// Distances.
    ea::vector<float> distances_;
    /// Materials.
    ea::vector<SharedPtr<Material> > materials_;
    /// Geometries.
    ea::vector<SharedPtr<Geometry> > geometries_;
};

/// 2D renderer component.
class URHO3D_API Renderer2D : public Drawable
{
    URHO3D_OBJECT(Renderer2D, Drawable);

public:
    /// Construct.
    explicit Renderer2D(Context* context);
    /// Destruct.
    ~Renderer2D() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Process octree raycast. May be called from a worker thread.
    void ProcessRayQuery(const RayOctreeQuery& query, ea::vector<RayQueryResult>& results) override;
    /// Calculate distance and prepare batches for rendering. May be called from worker thread(s), possibly re-entrantly.
    void UpdateBatches(const FrameInfo& frame) override;
    /// Batch update from main thread. Called on demand only if RequestUpdateBatchesDelayed() is called from UpdateBatches().
    void UpdateBatchesDelayed(const FrameInfo& frame) override;

    /// Add Drawable2D.
    void AddDrawable(Drawable2D* drawable);
    /// Remove Drawable2D.
    void RemoveDrawable(Drawable2D* drawable);
    /// Return material by texture and blend mode.
    Material* GetMaterial(Texture2D* texture, BlendMode blendMode);

    /// Check visibility.
    bool CheckVisibility(Drawable2D* drawable) const;

private:
    /// Recalculate the world-space bounding box.
    void OnWorldBoundingBoxUpdate() override;
    /// Create material by texture and blend mode.
    SharedPtr<Material> CreateMaterial(Texture2D* texture, BlendMode blendMode);
    /// Handle view update begin event. Determine Drawable2D's and their batches here.
    void HandleBeginViewUpdate(StringHash eventType, VariantMap& eventData);
    /// Get all drawables in node.
    void GetDrawables(ea::vector<Drawable2D*>& drawables, Node* node);
    /// Update view batch info.
    void UpdateViewBatchInfo(ViewBatchInfo2D& viewBatchInfo, Camera* camera);
    /// Add view batch.
    void AddViewBatch(ViewBatchInfo2D& viewBatchInfo, Material* material,
        unsigned indexStart, unsigned indexCount, unsigned vertexStart, unsigned vertexCount, float distance);

    /// Index buffer.
    SharedPtr<IndexBuffer> indexBuffer_;
    /// Material.
    SharedPtr<Material> material_;
    /// Drawables.
    ea::vector<Drawable2D*> drawables_;
    /// View frame info for current frame.
    FrameInfo frame_;
    /// View batch info.
    ea::unordered_map<Camera*, ViewBatchInfo2D> viewBatchInfos_;
    /// Frustum for current frame.
    Frustum frustum_;
    /// View mask of current camera for visibility checking.
    unsigned viewMask_;
    /// Cached materials.
    ea::unordered_map<Texture2D*, ea::unordered_map<int, SharedPtr<Material> > > cachedMaterials_;
    /// Cached techniques per blend mode.
    ea::unordered_map<int, SharedPtr<Technique> > cachedTechniques_;
};

}
