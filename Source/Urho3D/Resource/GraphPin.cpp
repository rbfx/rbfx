// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "GraphPin.h"

#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"
#include "Graph.h"
#include "GraphNode.h"

namespace Urho3D
{

GraphPin::~GraphPin() = default;

void GraphPin::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "name", name_);
}

void GraphDataPin::SerializeInBlock(Archive& archive)
{
    GraphPin::SerializeInBlock(archive);

    SerializeOptionalValue(archive, "type", type_);
}

bool GraphInPin::ConnectTo(GraphPinRef<GraphOutPin> pin)
{
    if (pin)
    {
        targetNode_ = pin.GetNode()->GetID();
        targetPin_ = pin.GetPin()->GetName();
        return true;
    }
    else
    {
        Disconnect();
        return false;
    }
}

void GraphInPin::Disconnect()
{
    targetNode_ = 0;
    targetPin_.clear();
}

void GraphInPin::SerializeInBlock(Archive& archive)
{
    GraphDataPin::SerializeInBlock(archive);

    SerializeOptionalValue(archive, "node", targetNode_);
    SerializeOptionalValue(archive, "pin", targetPin_);
    SerializeOptionalValue(archive, "value", value_, Variant::EMPTY,
        [&](Archive& archive, const char* name, Variant& value)
    {
        SerializeVariantAsType(archive, name, value, type_);
    });
}

void GraphInPin::SetValue(const Variant& variant)
{
    value_ = variant;
    Disconnect();
}

void GraphExitPin::SerializeInBlock(Archive& archive)
{
    GraphPin::SerializeInBlock(archive);

    SerializeOptionalValue(archive, "node", targetNode_);
    SerializeOptionalValue(archive, "pin", targetPin_);

    if (archive.IsInput())
    {
        if (!targetNode_ || targetPin_.empty())
        {
            targetNode_ = 0;
            targetPin_ = "";
        }
    }
}

bool GraphExitPin::ConnectTo(GraphPinRef<GraphEnterPin> pin)
{
    if (pin)
    {
        targetNode_ = pin.GetNode()->GetID();
        targetPin_ = pin.GetPin()->GetName();
        return true;
    }
    else
    {
        Disconnect();
        return false;
    }
}

void GraphExitPin::Disconnect()
{
    targetNode_ = 0;
    targetPin_.clear();
}

} // namespace Urho3D
