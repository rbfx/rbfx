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

class Node;
class Scene;
class Octree;

/// Lightmap scene collector interface.
class LightmapSceneCollector
{
public:
    /// Called before everything else. Scene objects must stay unchanged after this call.
    virtual void LockScene(Scene* scene, float chunkSize) = 0;
    /// Return all scene chunks. Chunk with index (0, 0, 0) starts at (0, 0, 0) and ends at (size, size, size).
    virtual ea::vector<IntVector3> GetChunks() = 0;
    /// Return nodes within chunk. Every node should be returned exactly once.
    virtual ea::vector<Node*> GetUniqueNodes(const IntVector3& chunkIndex) = 0;
    /// Return nodes overlapping given chunk with given padding.
    virtual ea::vector<Node*> GetOverlappingNodes(const IntVector3& chunkIndex, const Vector3& padding) = 0;
    /// Return nodes within given frustum. The frustum is guaranteed to contain specified chunk.
    virtual ea::vector<Node*> GetNodesInFrustum(const IntVector3& chunkIndex, const Frustum& frustum) = 0;
    /// Called after everything else. Scene objects must stay unchanged before this call.
    virtual void UnlockScene() = 0;
};

/// Standard scene collector.
class DefaultLightmapSceneCollector : public LightmapSceneCollector
{
public:
    /// Construct.
    DefaultLightmapSceneCollector() {}

    /// Called before everything else. Scene objects must stay unchanged after this call.
    void LockScene(Scene* scene, float chunkSize) override;
    /// Return all scene chunks. Chunk with index (0, 0, 0) starts at (0, 0, 0) and ends at (size, size, size).
    ea::vector<IntVector3> GetChunks() override;
    /// Return nodes within chunk. Every node should be returned exactly once.
    ea::vector<Node*> GetUniqueNodes(const IntVector3& chunkIndex) override;
    /// Return nodes overlapping given chunk with given padding.
    ea::vector<Node*> GetOverlappingNodes(const IntVector3& chunkIndex, const Vector3& padding) override;
    /// Return nodes within given frustum. The frustum is guaranteed to contain specified chunk.
    ea::vector<Node*> GetNodesInFrustum(const IntVector3& chunkIndex, const Frustum& frustum) override;
    /// Called after everything else. Scene objects must stay unchanged before this call.
    void UnlockScene() override;

private:
    /// Scene.
    Scene* scene_{};
    /// Chunk size.
    float chunkSize_{};
    /// Scene Octree.
    Octree* octree_{};
    /// Indexed nodes.
    ea::unordered_map<IntVector3, ea::vector<Node*>> indexedNodes_;
};

}
