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

#include "../Precompiled.h"

#include "Helpers.h"

#include "ParticleGraphLayerInstance.h"
#include "ParticleGraphSystem.h"
#include "PatternMatchingNode.h"
#include "Span.h"
#include "UpdateContext.h"
#include "Urho3D/IO/Log.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
namespace
{
class NopInstance: public ParticleGraphNodeInstance
{
    void Update(UpdateContext& context) override{};
};
}

NodePattern::NodePattern(UpdateFunction&& update)
    : updateFunction_(ea::move(update))
{
}

NodePattern& NodePattern::WithPin(ParticleGraphPin&& pin)
{
    pins_.push_back(ea::move(pin));
    return *this;
}

bool NodePattern::Match(const ea::span<ParticleGraphPin>& pins) const {
    if (pins.size() != this->pins_.size())
    {
        return false;
    }
    for (unsigned i = 0; i < pins.size(); ++i)
    {
        if (pins[i].GetNameHash() != this->pins_[i].GetNameHash())
            return false;
        if (pins[i].GetValueType() != this->pins_[i].GetRequestedType())
            return false;
    }
    return true;
}

VariantType NodePattern::EvaluateOutputPinType(const ea::span<ParticleGraphPin>& pins,
    const ParticleGraphPin& outputPin) const
{
    if (pins.size() != this->pins_.size())
    {
        return VAR_NONE;
    }
    VariantType res = VAR_NONE;
    for (unsigned i = 0; i < pins.size(); ++i)
    {
        if (pins[i].GetNameHash() != this->pins_[i].GetNameHash())
            return VAR_NONE;
        if (pins[i].IsInput())
        {
            if (pins[i].GetValueType() != this->pins_[i].GetRequestedType())
                return VAR_NONE;
        }
        else
        {
            if (outputPin.GetNameHash() == this->pins_[i].GetNameHash())
            {
                res = this->pins_[i].GetRequestedType();
            }
        }
    }
    return res;
}

PatternMatchingNode::Instance::Instance(PatternMatchingNode* op, const NodePattern& pattern)
    : node_(op)
    , pattern_(pattern)
{
}

void PatternMatchingNode::Instance::Update(UpdateContext& context)
{
    node_->Update(context, pattern_);
}

PatternMatchingNode::PatternMatchingNode(Context* context, const ea::vector<NodePattern>& patterns)
    : ParticleGraphNode(context)
    , patterns_(patterns)
{
}

unsigned PatternMatchingNode::GetNumPins() const { return static_cast<unsigned>(pins_.size()); }

ParticleGraphPin& PatternMatchingNode::GetPin(unsigned index) { return pins_[index]; }

unsigned PatternMatchingNode::EvaluateInstanceSize() const { return sizeof(Instance); }

ParticleGraphNodeInstance* PatternMatchingNode::CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer)
{
    for (const NodePattern& p : patterns_)
    {
        if (p.Match(pins_))
        {
            return new (ptr) Instance(this, p);
        }
    }
    URHO3D_LOGERROR(Format("No matching pattern found for graph node {}", this->GetTypeName()));
    return new (ptr) NopInstance();
}

ParticleGraphPin* PatternMatchingNode::LoadInputPin(ParticleGraphReader& reader, GraphInPin& inputPin)
{
    const auto index = GetPinIndex(inputPin.GetName());
    if (index != INVALID_PIN)
    {
        if (inputPin.GetType() != VAR_NONE)
            SetPinValueType(index, inputPin.GetType());
        return &GetPin(index);
    }
    return &pins_.emplace_back(ParticleGraphPinFlag::Input, inputPin.GetName(), inputPin.GetType());
}

ParticleGraphPin* PatternMatchingNode::LoadOutputPin(ParticleGraphReader& reader, GraphOutPin& outputPin)
{
    const auto index = GetPinIndex(outputPin.GetName());
    if (index != INVALID_PIN)
    {
        if (outputPin.GetType() != VAR_NONE)
            SetPinValueType(index, outputPin.GetType());
        return &GetPin(index);
    }
    return &pins_.emplace_back(ParticleGraphPinFlag::Output, outputPin.GetName(), outputPin.GetType());
}

VariantType PatternMatchingNode::EvaluateOutputPinType(ParticleGraphPin& pin)
{
    for (const NodePattern& p : patterns_)
    {
        const VariantType type = p.EvaluateOutputPinType(pins_, pin);
        if (type != VAR_NONE)
            return type;
    }
    return VAR_NONE;
}

void PatternMatchingNode::Update(UpdateContext& context, const NodePattern& pattern)
{
    ea::fixed_vector<ParticleGraphPinRef, NodePattern::ExpectedNumberOfPins> pinRefs;
    bool allScalar = true;
    for (auto& pin: pins_)
    {
        pinRefs.push_back(pin.GetMemoryReference());
        allScalar &= pin.GetContainerType() == ParticleGraphContainerType::Scalar;
    }

    // Index optimization
    if (allScalar && context.indices_.size() > 1)
    {
        UpdateContext contextCopy = context;
        contextCopy.indices_ = contextCopy.indices_.subspan(0, 1);
        return pattern.updateFunction_(contextCopy, pinRefs.data());
    }

    return pattern.updateFunction_(context, pinRefs.data());
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
