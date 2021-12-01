//
// Copyright (c) 2021 the rbfx project.
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

#include "RenderBillboard.h"
#include "ParticleGraphLayerInstance.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{

RenderBillboard::Instance::Instance(RenderBillboard* node, ParticleGraphLayerInstance* layer): node_(node)
{
    auto emitter = layer->GetEmitter();
    auto scene = emitter->GetScene();

    sceneNode_ = MakeShared<Node>(scene->GetContext());
    billboardSet_ = sceneNode_->CreateComponent<BillboardSet>();
    octree_ = scene->GetOrCreateComponent<Octree>();
    octree_->AddManualDrawable(billboardSet_);
}

RenderBillboard::Instance::~Instance()
{
    octree_->RemoveManualDrawable(billboardSet_);
}

void RenderBillboard::Instance::Update(UpdateContext& context)
{
    const unsigned numParticles = context.indices_.size();
    unsigned numBillboards = billboardSet_->GetNumBillboards();
    if (numBillboards < numParticles)
    {
        billboardSet_->SetNumBillboards(numBillboards);
        numBillboards = numParticles;
    }
    auto& billboards = billboardSet_->GetBillboards();
    unsigned i; 
    for (i = 0; i < numParticles; ++i)
    {
        billboards[i].enabled_ = true;
        //TODO: Copy values
    }
    for (; i < numBillboards; ++i)
    {
        billboards[i].enabled_ = false;
    }
}

}

} // namespace Urho3D
