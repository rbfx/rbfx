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

#include "Environment.h"

#include "Helpers.h"
#include "ParticleGraphSystem.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{

NodePattern::NodePattern(UpdateFunction&& update, PinPattern&& pin0)
    : updateFunction_(ea::move(update))
    , in_{ea::move(pin0)}
{
}

bool NodePattern::Match(const ea::span<ParticleGraphPin>& pins) const {
    return false;
}

VariantType NodePattern::EvaluateOutputPinType(const ea::span<ParticleGraphPin>& pins,
    const ParticleGraphPin& outputPin) const
{
    return VAR_NONE;
}

AdaptiveGraphNode::Instance::Instance(AdaptiveGraphNode* op, const NodePattern& pattern)
    : node_(op)
    , pattern_(pattern)
{
}

void AdaptiveGraphNode::Instance::Update(UpdateContext& context)
{
    node_->Update(context, pattern_);
}

AdaptiveGraphNode::AdaptiveGraphNode(Context* context, const ea::vector<NodePattern>& patterns)
    : ParticleGraphNode(context)
    , patterns_(patterns)
{
}

unsigned AdaptiveGraphNode::GetNumPins() const { return static_cast<unsigned>(pins_.size()); }

ParticleGraphPin& AdaptiveGraphNode::GetPin(unsigned index) { return pins_[index]; }

unsigned AdaptiveGraphNode::EvaluateInstanceSize() { return sizeof(Instance); }

ParticleGraphNodeInstance* AdaptiveGraphNode::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    for (const NodePattern& p : patterns_)
    {
        if (p.Match(pins_))
        {
            return new (ptr) Instance(this, p);
        }
    }
    return nullptr;
}

VariantType AdaptiveGraphNode::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    for (const NodePattern& p : patterns_)
    {
        const VariantType type = p.EvaluateOutputPinType(pins_, pin);
        if (type != VAR_NONE)
            return type;
    }
    return VAR_NONE;
}

void AdaptiveGraphNode::Update(UpdateContext& context, const NodePattern& pattern)
{
    ea::fixed_vector<ParticleGraphPinRef, NodePattern::ExpectedNumberOfPins> pinRefs;
    for (auto& pin: pins_)
    {
        pinRefs.push_back(pin.GetMemoryReference());
    }

    return pattern.updateFunction_(context, pinRefs.data());
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
