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

#include "../Glow/LightmapSceneCollector.h"

#include "../Glow/EmbreeScene.h"
#include "../Graphics/Octree.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Terrain.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

void DefaultLightmapSceneCollector::LockScene(Scene* scene, const Vector3& chunkSize)
{
    scene_ = scene;
    chunkSize_ = chunkSize;
    octree_ = scene_->GetComponent<Octree>();

    const ea::vector<Node*> children = scene_->GetChildren(true);
    boundingBox_ = CalculateBoundingBoxOfNodes(children, true);
    chunkGridDimension_ = VectorMax(IntVector3::ONE, VectorRoundToInt(boundingBox_.Size() / chunkSize_));
    const IntVector3 maxChunk = chunkGridDimension_ - IntVector3::ONE;

    for (Node* node : children)
    {
        auto staticModel = node->GetComponent<StaticModel>();
        if (staticModel)
        {
            const Vector3 position = node->GetWorldPosition();
            const Vector3 index = (position - boundingBox_.min_) / boundingBox_.Size() * Vector3(chunkGridDimension_);
            const IntVector3 chunk = VectorMin(VectorMax(IntVector3::ZERO, VectorFloorToInt(index)), maxChunk);
            indexedNodes_[chunk].push_back(node);
        }
    }
}

ea::vector<IntVector3> DefaultLightmapSceneCollector::GetChunks()
{
    return indexedNodes_.keys();
}

ea::vector<Node*> DefaultLightmapSceneCollector::GetUniqueNodes(const IntVector3& chunkIndex)
{
    auto iter = indexedNodes_.find(chunkIndex);
    if (iter != indexedNodes_.end())
        return iter->second;
    return {};
}

ea::vector<Node*> DefaultLightmapSceneCollector::GetOverlappingNodes(const IntVector3& chunkIndex, const Vector3& padding)
{
    return {};
}

ea::vector<Node*> DefaultLightmapSceneCollector::GetNodesInFrustum(const IntVector3& chunkIndex, const Frustum& frustum)
{
    return {};
}

void DefaultLightmapSceneCollector::UnlockScene()
{
    scene_ = nullptr;
    chunkSize_ = Vector3::ZERO;
    boundingBox_ = BoundingBox{};
    chunkGridDimension_ = IntVector3::ZERO;
    octree_ = nullptr;
    indexedNodes_.clear();
}

}
