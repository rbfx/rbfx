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

#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"

namespace Urho3D
{

GraphPin::GraphPin(GraphNode* node)
    : node_(node)
{
}

GraphPin::~GraphPin() = default;

bool GraphPin::Serialize(Archive& archive, ArchiveBlock& block)
{
    //TODO: make proper optional serialization.
    SerializeOptional(archive, name_.empty(),
                             [&](bool loading) { return SerializeValue(archive, "name", name_); });
    return true;
}

GraphDataPin::GraphDataPin(GraphNode* node)
    : GraphPin(node)
{
}

bool GraphDataPin::Serialize(Archive& archive, ArchiveBlock& block)
{
    if (!GraphPin::Serialize(archive, block))
        return false;
    // TODO: check if element is missing properly
    SerializeOptional(archive, type_ == VAR_NONE,
                      [&](bool loading) { return SerializeEnum(archive, "type", Variant::GetTypeNameList(), type_); });
    return true;
}

GraphOutPin::GraphOutPin(GraphNode* node)
    :GraphDataPin(node)
{
}

GraphInPin::GraphInPin(GraphNode* node)
    : GraphDataPin(node)
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
    targetNode_ = pin.GetNode()->GetID();
    targetPin_ = pin.GetName();
    return true;
}

void GraphInPin::Disconnect()
{
    targetNode_ = 0;
    targetPin_.clear();
}

GraphOutPin* GraphInPin::GetConnectedPin() const
{
    if (!targetNode_)
        return nullptr;

    if (const auto node = GetNode())
    {
        if (const auto graph = node->GetGraph())
        {
            if (auto target = graph->GetNode(targetNode_))
            {
                return node->GetOutput(targetPin_);
            }
        }
    }
    return nullptr;
}


bool GraphInPin::Serialize(Archive& archive, ArchiveBlock& block)
{
    if (!GraphDataPin::Serialize(archive, block))
        return false;

            // TODO: check if element is missing properly
    SerializeOptional(archive, targetNode_ != 0,
                      [&](bool loading) { return SerializeValue(archive, "node", targetNode_); });
    SerializeOptional(archive, !targetPin_.empty(),
                      [&](bool loading) { return SerializeValue(archive, "pin", targetPin_); });

    SerializeOptional(archive, !value_.IsEmpty(),
                      [&](bool loading) { return SerializeVariantValue(archive, type_, "value", value_); });

    return true;
}

void GraphInPin::SetValue(const Variant& variant)
{
    value_ = variant;
    Disconnect();
}

GraphEnterPin* GraphExitPin::GetConnectedPin() const
{
    if (!targetNode_)
        return nullptr;

    if (const auto node = GetNode())
    {
        if (const auto graph = node->GetGraph())
        {
            if (auto target = graph->GetNode(targetNode_))
            {
                return node->GetEnter(targetPin_);
            }
        }
    }
    return nullptr;
}

GraphEnterPin::GraphEnterPin(GraphNode* node)
    :GraphPin(node)
{
}

GraphExitPin::GraphExitPin(GraphNode* node)
    : GraphPin(node)
{
}

bool GraphExitPin::Serialize(Archive& archive, ArchiveBlock& block)
{
    if (!GraphPin::Serialize(archive, block))
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

bool GraphExitPin::ConnectTo(GraphEnterPin& pin)
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
    targetNode_ = pin.GetNode()->GetID();
    targetPin_ = pin.GetName();
    return true;
}

void GraphExitPin::Disconnect()
{
    targetNode_ = 0;
    targetPin_.clear();
}

}
