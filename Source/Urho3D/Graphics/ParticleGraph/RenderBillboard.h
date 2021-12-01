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

#pragma once

#include "Helpers.h"
#include "Urho3D/Graphics/BillboardSet.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

/// Operation on attribute
class URHO3D_API RenderBillboard : public ParticleGraphNode
{
    URHO3D_OBJECT(RenderBillboard, ParticleGraphNode)
public:
    /// Construct.
    explicit RenderBillboard(Context* context)
        : ParticleGraphNode(context)
        , pins_{ParticleGraphNodePin(PGPIN_INPUT, "pos", VAR_VECTOR3)}
    {
    }

protected:
    class Instance : public ParticleGraphNodeInstance
    {
    public:
        Instance(RenderBillboard* node, ParticleGraphLayerInstance* layer);
        ~Instance();
        void Update(UpdateContext& context) override;

    protected:
        RenderBillboard* node_;
        SharedPtr<Urho3D::Node> sceneNode_;
        SharedPtr<Urho3D::BillboardSet> billboardSet_;
        SharedPtr<Urho3D::Octree> octree_;
    };

public:
    /// Get number of pins.
    unsigned NumPins() const override { return 1; }

    /// Get pin by index.
    ParticleGraphNodePin& GetPin(unsigned index) override { return pins_[index]; }

    /// Evaluate size required to place new node instance.
    unsigned EvalueInstanceSize() override { return sizeof(Instance); }

    /// Place new instance at the provided address.
    ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) override
    {
        return new (ptr) Instance(this, layer);
    }

protected:
    /// Pins
    ParticleGraphNodePin pins_[1];
};

} // namespace ParticleGraphNodes

} // namespace Urho3D
