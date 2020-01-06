//
// Copyright (c) 2008-2019 the Urho3D project.
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

/// \file

#pragma once

#include "../Math/Frustum.h"
#include "../Math/Vector3.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Light;
class LightProbeGroup;
class Node;
class StaticModel;
class Scene;
class Octree;

/// Lightmap scene collector interface.
/// Objects may be loaded and unloaded even if scene is locked.
class URHO3D_API LightmapSceneCollector
{
public:
    /// Destruct.
    virtual ~LightmapSceneCollector();

    /// Called before everything else. Scene objects must stay unchanged after this call.
    virtual void LockScene(Scene* scene, const Vector3& chunkSize) = 0;
    /// Return all scene chunks.
    virtual ea::vector<IntVector3> GetChunks() = 0;

    /// Return unique static models within chunk.
    virtual ea::vector<StaticModel*> GetUniqueStaticModels(const IntVector3& chunkIndex) = 0;
    /// Called when static models were changed externally.
    virtual void CommitStaticModels(const IntVector3& chunkIndex) = 0;
    /// Return unique light probe groups within chunk. Order of groups must stay the same for each call.
    virtual ea::vector<LightProbeGroup*> GetUniqueLightProbeGroups(const IntVector3& chunkIndex) = 0;
    /// Called when light probe groups were changed externally.
    virtual void CommitLightProbeGroups(const IntVector3& chunkIndex) = 0;

    /// Return bounding box of unique nodes of the chunk.
    virtual BoundingBox GetChunkBoundingBox(const IntVector3& chunkIndex) = 0;
    /// Return lights intersecting given volume.
    virtual ea::vector<Light*> GetLightsInBoundingBox(const IntVector3& chunkIndex, const BoundingBox& boundingBox) = 0;
    /// Return static models intersecting given volume.
    virtual ea::vector<StaticModel*> GetStaticModelsInBoundingBox(const IntVector3& chunkIndex, const BoundingBox& boundingBox) = 0;
    /// Return light probe groups intersecting given volume.
    virtual ea::vector<LightProbeGroup*> GetLightProbeGroupsInBoundingBox(const IntVector3& chunkIndex, const BoundingBox& boundingBox) = 0;
    /// Return static models intersecting given frustum. The frustum is guaranteed to contain specified chunk.
    virtual ea::vector<StaticModel*> GetStaticModelsInFrustum(const IntVector3& chunkIndex, const Frustum& frustum) = 0;

    /// Called after everything else. Scene objects must stay unchanged before this call.
    virtual void UnlockScene() = 0;
};

/// Standard scene collector.
class URHO3D_API DefaultLightmapSceneCollector : public LightmapSceneCollector
{
public:
    /// Construct.
    DefaultLightmapSceneCollector() {}

    /// Called before everything else. Scene objects must stay unchanged after this call.
    void LockScene(Scene* scene, const Vector3& chunkSize) override;
    /// Return all scene chunks.
    ea::vector<IntVector3> GetChunks() override;

    /// Return unique static models within chunk.
    ea::vector<StaticModel*> GetUniqueStaticModels(const IntVector3& chunkIndex) override;
    /// Called after static models are changed externally.
    void CommitStaticModels(const IntVector3& chunkIndex) override;
    /// Return unique light probe groups within chunk. Order of groups must stay the same for each call.
    ea::vector<LightProbeGroup*> GetUniqueLightProbeGroups(const IntVector3& chunkIndex) override;
    /// Called after light probe groups are changed externally.
    void CommitLightProbeGroups(const IntVector3& chunkIndex) override;

    /// Return bounding box of unique nodes of the chunk.
    BoundingBox GetChunkBoundingBox(const IntVector3& chunkIndex) override;
    /// Return lights intersecting given volume.
    ea::vector<Light*> GetLightsInBoundingBox(const IntVector3& chunkIndex, const BoundingBox& boundingBox) override;
    /// Return static models intersecting given volume.
    ea::vector<StaticModel*> GetStaticModelsInBoundingBox(const IntVector3& chunkIndex, const BoundingBox& boundingBox) override;
    /// Return light probe groups intersecting given volume.
    ea::vector<LightProbeGroup*> GetLightProbeGroupsInBoundingBox(const IntVector3& chunkIndex, const BoundingBox& boundingBox) override;
    /// Return static models intersecting given frustum. The frustum is guaranteed to contain specified chunk.
    ea::vector<StaticModel*> GetStaticModelsInFrustum(const IntVector3& chunkIndex, const Frustum& frustum) override;

    /// Called after everything else. Scene objects must stay unchanged before this call.
    void UnlockScene() override;

private:
    /// Chunk data.
    struct ChunkData
    {
        /// Unique static models.
        ea::vector<StaticModel*> staticModels_;
        /// Unique light probe groups.
        ea::vector<LightProbeGroup*> lightProbeGroups_;
        /// Bounding box.
        BoundingBox boundingBox_;
    };
    /// Scene.
    Scene* scene_{};
    /// Chunk size.
    Vector3 chunkSize_;
    /// Bounding box of the scene.
    BoundingBox boundingBox_;
    /// Dimensions of chunk grid.
    IntVector3 chunkGridDimension_;
    /// Scene Octree.
    Octree* octree_{};
    /// Indexed nodes.
    ea::unordered_map<IntVector3, ChunkData> chunks_;
    /// All light probe groups.
    ea::vector<LightProbeGroup*> lightProbeGroups_;
};

}
