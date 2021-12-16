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

#include "GraphPin.h"

#include "Graph.h"
#include "GraphNode.h"

#include "GraphNode.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"

namespace Urho3D
{

GraphPin::GraphPin(GraphNode* node, GraphPinDirection direction)
    : node_(node)
    , direction_(direction)
    , type_(VAR_NONE)
{
}

void GraphPin::SetType(VariantType type)
{
    type_ = type;
}

bool GraphPin::Serialize(Archive& archive)
{
    if (!SerializeValue(archive, "name", name_))
    {
        return false;
    }
    //TODO: check if element is missing properly
    SerializeEnum(archive, "type", Variant::GetTypeNameList(), type_);
    return true;
}

void GraphPin::SetName(const ea::string_view name)
{
    name_ = name;
}

GraphInPin::GraphInPin(GraphNode* node, GraphPinDirection direction)
    : GraphPin(node, direction)
    , targetNode_(0)
{
}

bool GraphInPin::ConnectTo(GraphOutPin& pin)
{
    if (!GetNode()->GetGraph())
    {
        URHO3D_LOGERROR("Can't connect pin from detached node.");
        return false;
    }
    if (pin.GetNode()->GetGraph() != GetNode()->GetGraph())
    {
        URHO3D_LOGERROR("Can't connect to pin in a different graph.");
        return false;
    }
    if (direction_ == PINDIR_INPUT && pin.GetDirection() != PINDIR_OUTPUT)
    {
        URHO3D_LOGERROR("Input pin can only be connected to output pin.");
        return false;
    }
    if (direction_ == PINDIR_EXIT && pin.GetDirection() != PINDIR_ENTER)
    {
        URHO3D_LOGERROR("Exit pin can only be connected to enter pin.");
        return false;
    }
    targetNode_ = pin.GetNode()->GetID();
    targetPin_ = pin.GetName();
    return true;
}

GraphOutPin* GraphInPin::GetConnectedPin() const
{
    if (!targetNode_)
        return nullptr;

    auto node = GetNode()->GetGraph()->GetNode(targetNode_);
    if (!node)
        return nullptr;

    if (direction_ == PINDIR_INPUT)
        return node->GetOutput(targetPin_);
    return node->GetEnter(targetPin_);
}


bool GraphInPin::Serialize(Archive& archive)
{
    if (!GraphPin::Serialize(archive))
        return false;
    if (archive.IsInput())
    {
        unsigned id = 0;
        ea::string pin;
        SerializeValue(archive, "node", id);
        SerializeValue(archive, "pin", pin);

        if (id != 0 && !pin.empty())
        {
            targetNode_ = id;
            if (targetNode_)
            {
                targetPin_ = pin;
            }
        }
    }
    else
    {
        if (targetNode_)
        {
            SerializeValue(archive, "node", targetNode_);
            SerializeValue(archive, "pin", targetPin_);
        }
    }
    return true;
}

bool GraphDataInPin::Serialize(Archive& archive)
{
    if (!GraphInPin::Serialize(archive))
        return false;
    if (archive.IsInput())
    {
        SerializeVariantValue(archive, type_, "value", defaultValue_);
    }
    else
    {
        if (defaultValue_.GetType() != VAR_NONE)
        {
            if (!SerializeVariantValue(archive, defaultValue_.GetType(), "value", defaultValue_))
                return false;
        }
    }
    return true;
}

const Variant& GraphDataInPin::GetDefaultValue()
{
    return defaultValue_;
}

void GraphDataInPin::SetDefaultValue(const Variant& variant)
{
    defaultValue_ = variant;
}


GraphDataInPin::GraphDataInPin(GraphNode* node, GraphPinDirection direction)
    : GraphInPin(node, direction)
{
}

GraphOutPin::GraphOutPin(GraphNode* node, GraphPinDirection direction)
    : GraphPin(node, direction)
{
}
}
