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

#include "ParticleGraphNode.h"

#include "ParticleGraph.h"
#include "ParticleGraphPin.h"
#include "ParticleGraphSystem.h"

#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"


namespace Urho3D
{

ParticleGraphNode::ParticleGraphNode(Context* context)
    : Object(context)
    , graph_(nullptr)
    , index_(0)
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

unsigned ParticleGraphNode::GetPinIndex(const ea::string& name)
{
    for (unsigned i = 0; i < NumPins(); ++i)
    {
        ParticleGraphPin& pin = GetPin(i);
        if (pin.GetName() == name)
            return i;
    }
    return INVALID_PIN;
}

ParticleGraphPin* ParticleGraphNode::GetPin(const ea::string& name)
{
    auto index = GetPinIndex(name);
    if (index == INVALID_PIN)
        return nullptr;
    return &GetPin(index);
}

bool ParticleGraphNode::Load(ParticleGraphReader& reader, GraphNode& node)
{
    if (!LoadProperties(reader, node))
        return false;
    if (!LoadPins(reader, node))
        return false;

    return true;
}

bool ParticleGraphNode::Save(ParticleGraphWriter& writer, GraphNode& node)
{
    node.SetName(GetTypeName());

    if (!SaveProperties(writer, node))
        return false;
    if (!SavePins(writer, node))
        return false;
    
    return true;
}

bool ParticleGraphNode::LoadPins(ParticleGraphReader& reader, GraphNode& node)
{
    // First pass: validate and connect pins input pins.
    for (auto& inputPin : node.GetInputs())
    {
        auto pin = GetPin(inputPin.GetName());
        if (pin)
        {
            pin->SetValueType(inputPin.type_);
        }
        else
        {
            URHO3D_LOGERROR(Format("Unknown input pin {}", inputPin.GetName()));
            return false;
        }

        if (inputPin.GetConnected())
        {
            auto source = inputPin.GetConnectedPin();
            if (source == nullptr)
            {
                URHO3D_LOGERROR(Format("Can't resolve connected pin for {}", inputPin.GetName()));
                return false;
            }
            auto connectedNode = reader.ReadNode(source->GetNode()->GetID());
            if (connectedNode == ParticleGraph::INVALID_NODE_INDEX)
            {
                return false;
            }
            unsigned pinIndex = reader.GetInputPinIndex(connectedNode, source->GetName());
            pin->SetSource(connectedNode, pinIndex);
        }
    }
    // Second pass: create constant nodes.
    for (auto& inputPin : node.GetInputs())
    {
        auto pin = GetPin(inputPin.GetName());
        if (!inputPin.GetConnected())
        {
            auto constValue = inputPin.GetDefaultValue();
            if (constValue.GetType() == VAR_NONE)
            {
                URHO3D_LOGERROR(Format("Pin {} is not connected and doesn't have default value.", inputPin.GetName()));
                return false;
            }
            auto connectedNode = reader.GetOrAddConstant(constValue);
            pin->SetSource(connectedNode, 0);
        }
    }

    for (auto& outputPin : node.GetOutputs())
    {
        auto pin = GetPin(outputPin.GetName());
        if (pin)
        {
            pin->SetValueType(outputPin.type_);
        }
        else
        {
            URHO3D_LOGERROR(Format("Unknown output pin {}", outputPin.GetName()));
        }
    }
    return true;
}

bool ParticleGraphNode::LoadProperties(ParticleGraphReader& reader, GraphNode& node)
{
    return true;
}

bool ParticleGraphNode::SavePins(ParticleGraphWriter& writer, GraphNode& node)
{
    for (unsigned i = 0; i < NumPins(); ++i)
    {
        ParticleGraphPin& pin = GetPin(i);
        if (pin.GetIsInput())
        {
            auto& inputPin = node.GetOrAddInput(pin.GetName());
            inputPin.type_ = pin.GetValueType();

            if (pin.GetConnected())
            {
                auto connectedNode = pin.GetConnectedNodeIndex();
                inputPin.ConnectTo(writer.GetSourcePin(connectedNode, pin.GetConnectedPinIndex()));
            }
        }
        else
        {
            auto outputPin = node.GetOrAddOutput(pin.GetName());
            outputPin.type_ = pin.GetValueType();
        }
    }
    return true;
}

bool ParticleGraphNode::SaveProperties(ParticleGraphWriter& writer, GraphNode& node)
{
    return true;
}

VariantType ParticleGraphNode::EvaluateOutputPinType(ParticleGraphPin& pin) { return VAR_NONE; }

void ParticleGraphNode::SetGraph(ParticleGraph* graph, unsigned index)
{
    graph_ = graph;
    index_ = index;
}
//
//bool SerializeValue(Archive& archive, const char* name, SharedPtr<ParticleGraphNode>& value)
//{
//    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
//    {
//        // Serialize type
//        StringHash type = value ? value->GetType() : StringHash{};
//        const ea::string_view typeName = value ? ea::string_view{value->GetTypeName()} : "";
//        if (!SerializeStringHash(archive, "type", type, typeName))
//            return false;
//
//        // Serialize empty object
//        if (type == StringHash{})
//        {
//            value = nullptr;
//            return true;
//        }
//
//        // Create instance if loading
//        if (archive.IsInput())
//        {
//            Context* context = archive.GetContext();
//            if (!context)
//            {
//                archive.SetError(Format("Context is required to serialize Serializable '{0}'", name));
//                return false;
//            }
//            auto system = context->GetSubsystem<ParticleGraphSystem>();
//            
//            value = system->CreateParticleGraphNode(type);
//
//            if (!value)
//            {
//                archive.SetError(Format("Failed to create instance of type '{0}'", type.Value()));
//                return false;
//            }
//        }
//
//        // Serialize object
//        return value->Serialize(archive);
//    }
//    return false;
//}

} // namespace Urho3D
