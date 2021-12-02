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
#include "ParticleGraphPin.h"

#include "../../IO/ArchiveSerialization.h"


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

void ParticleGraphNode::SetPinSource(unsigned pinIndex, unsigned nodeIndex, unsigned nodePinIndex)
{
    if (pinIndex >= NumPins())
    {
        URHO3D_LOGERROR("Pin index out of bounds");
        return;
    }
    auto& pin = GetPin(pinIndex);
    pin.SetSource(nodeIndex, nodePinIndex);
}

ParticleGraphPin& ParticleGraphNode::GetPin(const ea::string& name)
{
    for (unsigned i=0; i<NumPins(); ++i)
    {
        ParticleGraphPin& pin = GetPin(i);
        if (pin.GetName() == name)
            return pin;
    }
    static ParticleGraphPin err;
    return err;
}

namespace
{
struct PinArrayAdapter
{
    typedef ParticleGraphPin value_type;
    PinArrayAdapter(ParticleGraphNode& node)
        : node_(node)
    {
    }
    size_t size() const { return node_.NumPins(); }
    ParticleGraphPin& operator[](size_t index) { return node_.GetPin(index); }

    ParticleGraphNode& node_;
};
} // namespace

bool ParticleGraphNode::Serialize(Archive& archive)
{
    PinArrayAdapter adapter(*this);
    return SerializeArrayAsObjects(archive, "pins", "pin", adapter);
}

VariantType ParticleGraphNode::EvaluateOutputPinType(ParticleGraphPin& pin) { return VAR_NONE; }

bool SerializeValue(Archive& archive, const char* name, SharedPtr<ParticleGraphNode>& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        // Serialize type
        StringHash type = value ? value->GetType() : StringHash{};
        const ea::string_view typeName = value ? ea::string_view{value->GetTypeName()} : "";
        if (!SerializeStringHash(archive, "type", type, typeName))
            return false;

        // Serialize empty object
        if (type == StringHash{})
        {
            value = nullptr;
            return true;
        }

        // Create instance if loading
        if (archive.IsInput())
        {
            Context* context = archive.GetContext();
            if (!context)
            {
                archive.SetError(Format("Context is required to serialize Serializable '{0}'", name));
                return false;
            }

            value.StaticCast(context->CreateParticleGraphNode(type));

            if (!value)
            {
                archive.SetError(Format("Failed to create instance of type '{0}'", type.Value()));
                return false;
            }
        }

        // Serialize object
        return value->Serialize(archive);
    }
    return false;
}

} // namespace Urho3D
