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
