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
#include "../Graphics/Light.h"
#include "../Graphics/Octree.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Terrain.h"
#include "../Graphics/Zone.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

LightmapSceneCollector::~LightmapSceneCollector() = default;

void DefaultLightmapSceneCollector::LockScene(Scene* scene, const Vector3& chunkSize)
{
    scene_ = scene;
    chunkSize_ = chunkSize;
    octree_ = scene_->GetComponent<Octree>();

    // Estimate dimensions
    const ea::vector<Node*> children = scene_->GetChildren(true);
    boundingBox_ = CalculateBoundingBoxOfNodes(children, true);
    chunkGridDimension_ = VectorMax(IntVector3::ONE, VectorRoundToInt(boundingBox_.Size() / chunkSize_));
    const IntVector3 maxChunk = chunkGridDimension_ - IntVector3::ONE;

    // Collect nodes
    for (Node* node : children)
    {
        auto staticModel = node->GetComponent<StaticModel>();
        if (staticModel && staticModel->GetBakeLightmap())
        {
            const Vector3 position = node->GetWorldPosition();
            const Vector3 index = (position - boundingBox_.min_) / boundingBox_.Size() * Vector3(chunkGridDimension_);
            const IntVector3 chunk = VectorMin(VectorMax(IntVector3::ZERO, VectorFloorToInt(index)), maxChunk);
            chunks_[chunk].nodes_.push_back(node);
        }
    }

    // Calculate chunks bounding boxes
    for (auto& elem : chunks_)
        elem.second.boundingBox_ = CalculateBoundingBoxOfNodes(elem.second.nodes_);
}

ea::vector<IntVector3> DefaultLightmapSceneCollector::GetChunks()
{
    return chunks_.keys();
}

ea::vector<Node*> DefaultLightmapSceneCollector::GetUniqueNodes(const IntVector3& chunkIndex)
{
    auto iter = chunks_.find(chunkIndex);
    if (iter != chunks_.end())
        return iter->second.nodes_;
    return {};
}

BoundingBox DefaultLightmapSceneCollector::GetChunkBoundingBox(const IntVector3& chunkIndex)
{
    auto iter = chunks_.find(chunkIndex);
    if (iter != chunks_.end())
        return iter->second.boundingBox_;
    return {};
}

ea::vector<Node*> DefaultLightmapSceneCollector::GetNodesInBoundingBox(const IntVector3& /*chunkIndex*/, const BoundingBox& boundingBox)
{
    // Query drawables
    ea::vector<Drawable*> drawables;
    BoxOctreeQuery query(drawables, boundingBox);
    octree_->GetDrawables(query);

    // Filter nodes
    ea::vector<Node*> nodes;
    for (Drawable* drawable : drawables)
    {
        if (Node* node = drawable->GetNode())
        {
            auto staticModel = dynamic_cast<StaticModel*>(drawable);
            if (staticModel && staticModel->GetBakeLightmap())
                nodes.push_back(node);

            auto light = dynamic_cast<Light*>(drawable);
            if (light && light->GetLightMode() != LM_REALTIME)
                nodes.push_back(node);

            if (auto zone = dynamic_cast<Zone*>(drawable))
                nodes.push_back(node);
        }
    }
    return nodes;
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
    chunks_.clear();
}

}
