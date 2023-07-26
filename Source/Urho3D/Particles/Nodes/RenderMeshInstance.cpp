//
// Copyright (c) 2021-2022 the rbfx project.
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

#include "../../Precompiled.h"

#include "RenderMeshInstance.h"

#include "../../Graphics/Camera.h"
#include "../../Graphics/Octree.h"
#include "../../Graphics/StaticModel.h"
#include "../../Scene/Scene.h"
#include "../ParticleGraphLayerInstance.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

RenderMeshDrawable::RenderMeshDrawable(Context* context)
    : StaticModel(context)
{
}

void RenderMeshDrawable::UpdateBatches(const FrameInfo& frame)
{
    // Getting the world bounding box ensures the transforms are updated
    const BoundingBox& worldBoundingBox = GetWorldBoundingBox();
    const Matrix3x4& worldTransform = node_->GetWorldTransform();
    distance_ = frame.camera_->GetDistance(worldBoundingBox.Center());
    if (batches_.size() > 1)
    {
        for (unsigned i = 0; i < batches_.size(); ++i)
        {
            batches_[i].distance_ = frame.camera_->GetDistance(worldTransform * geometryData_[i].center_);
            batches_[i].worldTransform_ = transforms_.empty() ? &Matrix3x4::IDENTITY : &transforms_[0];
            batches_[i].numWorldTransforms_ = transforms_.size();
        }
    }
    else if (batches_.size() == 1)
    {
        batches_[0].distance_ = distance_;
        batches_[0].worldTransform_ = transforms_.empty() ? &Matrix3x4::IDENTITY : &transforms_[0];
        batches_[0].numWorldTransforms_ = transforms_.size();
    }

    float scale = worldBoundingBox.Size().DotProduct(DOT_SCALE);
    float newLodDistance = frame.camera_->GetLodDistance(distance_, scale, lodBias_);

    if (newLodDistance != lodDistance_)
    {
        lodDistance_ = newLodDistance;
        CalculateLodLevels();
    }
}


void RenderMeshInstance::Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
{
    InstanceBase::Init(node, layer);

    auto graphNode = static_cast<RenderMesh*>(node_);
    sceneNode_ = MakeShared<Node>(GetContext());
    drawable_ = MakeShared<RenderMeshDrawable>(GetContext());
    sceneNode_->AddComponent(drawable_, 0);
    drawable_->SetModelAttr(graphNode->GetModel());
    drawable_->SetMaterialsAttr(graphNode->GetMaterial());
    UpdateDrawableAttributes();
    OnSceneSet(GetScene());
}

void RenderMeshInstance::OnSceneSet(Scene* scene)
{
    if (octree_)
    {
        octree_->RemoveManualDrawable(drawable_);
        octree_.Reset();
    }
    if (scene)
    {
        octree_ = scene->GetOrCreateComponent<Octree>();
        octree_->AddManualDrawable(drawable_);
    }
}

void RenderMeshInstance::UpdateDrawableAttributes()
{
    CopyDrawableAttributes(drawable_, GetEmitter());
}

RenderMeshInstance::~RenderMeshInstance()
{
    OnSceneSet(nullptr);
}

ea::vector<Matrix3x4>& RenderMeshInstance::Prepare(unsigned numParticles)
{
    drawable_->transforms_.resize(numParticles);
    sceneNode_->SetWorldTransform(GetNode()->GetWorldTransform());
    // if (node_->material_ != drawable_->GetMaterial(0))
    //    drawable_->SetMaterial(node_->material_);
    return drawable_->transforms_;
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
