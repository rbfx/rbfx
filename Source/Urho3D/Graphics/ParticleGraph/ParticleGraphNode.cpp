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

#include "ParticleGraphNode.h"
#include "ParticleGraphNodePin.h"

#include <Urho3D/IO/ArchiveSerialization.h>


namespace Urho3D
{

ParticleGraphNode::ParticleGraphNode(Context* context)
    : Object(context)
{
}

void ParticleGraphNode::SetPinName(unsigned pinIndex, const ea::string& name)
{
    if (pinIndex >= NumPins())
        return;
    auto& pin = GetPin(pinIndex);
    pin.SetName(name);
}

void ParticleGraphNode::SetPinValueType(unsigned pinIndex, VariantType type)
{
    if (pinIndex >= NumPins())
        return;
    auto& pin = GetPin(pinIndex);
    pin.SetValueType(type);
}

ParticleGraphNode::~ParticleGraphNode() = default;

namespace
{
struct PinArrayAdapter
{
    typedef ParticleGraphNodePin value_type;
    PinArrayAdapter(ParticleGraphNode& node)
        : node_(node)
    {
    }
    size_t size() const { return node_.NumPins(); }
    ParticleGraphNodePin& operator[](size_t index) { return node_.GetPin(index); }

    ParticleGraphNode& node_;
};
} // namespace

bool ParticleGraphNode::Serialize(Archive& archive)
{
    PinArrayAdapter adapter(*this);
    return SerializeArrayAsObjects(archive, "pins", "pin", adapter);
}

bool ParticleGraphNode::EvaluateOutputPinType(ParticleGraphNodePin& pin) { return false; }
} // namespace Urho3D
