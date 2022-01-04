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

#pragma once

#include "RenderBillboardInstance.h"

#include "../Graphics/Camera.h"
#include "../Graphics/Octree.h"
#include "../Scene/Scene.h"
#include "Urho3D/Resource/ResourceCache.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
void RenderBillboardInstance::Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
{
    InstanceBase::Init(node, layer);

    auto* renderBillboard = static_cast<RenderBillboard*>(GetGraphNode());
    auto* context = node->GetContext();
    const auto scene = GetScene();

    sceneNode_ = MakeShared<Node>(GetContext());

    billboardSet_ = sceneNode_->CreateComponent<BillboardSet>();
    billboardSet_->SetMaterialAttr(renderBillboard->GetMaterial());
    //billboardSet_->SetFaceCameraMode(FC_NONE);
    octree_ = scene->GetOrCreateComponent<Octree>();
    octree_->AddManualDrawable(billboardSet_);
}


RenderBillboardInstance::~RenderBillboardInstance() { octree_->RemoveManualDrawable(billboardSet_); }

void RenderBillboardInstance::Prepare(unsigned numParticles)
{
    auto* renderBillboard = static_cast<RenderBillboard*>(GetGraphNode());

    if (!renderBillboard->GetIsWorldspace())
    {
        sceneNode_->SetWorldTransform(GetNode()->GetWorldTransform());
    }
    unsigned numBillboards = billboardSet_->GetNumBillboards();
    //if (numBillboards < numParticles)
    {
        billboardSet_->SetNumBillboards(numParticles);
        numBillboards = numParticles;
    }
    auto& billboards = billboardSet_->GetBillboards();
    for (unsigned i = numParticles; i < numBillboards; ++i)
    {
        billboards[i].enabled_ = false;
    }
    cols_ = Max(1, renderBillboard->GetColumns());
    rows_ = Max(1, renderBillboard->GetRows());
    uvTileSize_ = Vector2(1.0f / static_cast<float>(cols_), 1.0f / static_cast<float>(rows_));
}

void RenderBillboardInstance::UpdateParticle(
    unsigned index, const Vector3& pos, const Vector2& size, float frameIndex, Color& color, float rotation, Vector3& direction)
{
    auto* billboard = billboardSet_->GetBillboard(index);
    billboard->enabled_ = true;
    billboard->position_ = pos;
    billboard->size_ = size;
    billboard->color_ = color;
    billboard->rotation_ = rotation;
    billboard->direction_ = direction;
    const int frame = static_cast<int>(frameIndex);
    const unsigned x = frame % cols_;
    const unsigned y = (frame / cols_);
    const auto uvMin = Vector2(x, y) * uvTileSize_;
    const auto uvMax = uvMin + uvTileSize_;
    billboard->uv_ = Rect(uvMin, uvMax);
}

void RenderBillboardInstance::Commit() { billboardSet_->Commit(); }

} // namespace ParticleGraphNodes

} // namespace Urho3D
