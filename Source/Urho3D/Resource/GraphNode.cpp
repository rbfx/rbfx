//
// Copyright (c) 2021 the Urho3D project.
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

#include "GraphNode.h"
#include "Graph.h"
#include "GraphPin.h"

#include "../IO/ArchiveSerialization.h"

namespace Urho3D
{
namespace
{

static char* DirectionEnumConstants[]{"In", "Out", "Enter", "Exit", nullptr};

template <typename Iterator> bool SerializePins(Archive& archive, Iterator begin, Iterator end)
{
    while (begin != end)
    {
        if (ArchiveBlock block = archive.OpenUnorderedBlock("pin"))
        {
            GraphPinDirection dir = begin->GetDirection();
            if (dir != PINDIR_INPUT)
            {
                if (!SerializeEnum(archive, "direction", DirectionEnumConstants, dir))
                    return false;
            }
            if (!begin->Serialize(archive))
                return false;
            ++begin;
        }
        else
        {
            return false;
        }
    }
    return true;
}

}

GraphNode::GraphNode(Context* context)
    : Object(context)
    , graph_(nullptr)
    , id_(0)
    , name_()
    , nameHash_()
{
}

GraphNode::~GraphNode() = default;

void GraphNode::RegisterObject(Context* context)
{
    context->RegisterFactory<GraphNode>();
}


GraphInPin& GraphNode::GetOrAddInput(const ea::string_view name)
{
    for (auto& pin : inputPins_)
    {
        if (pin.GetName() == name)
        {
            return pin;
        }
    }
    inputPins_.push_back(GraphInPin(this, PINDIR_INPUT));
    auto& pin = inputPins_.back();
    pin.SetName(name);
    return pin;
}

GraphOutPin& GraphNode::GetOrAddOutput(const ea::string_view name)
{
    for (auto& pin : outputPins_)
    {
        if (pin.GetName() == name)
        {
            return pin;
        }
    }
    outputPins_.push_back(GraphOutPin(this, PINDIR_OUTPUT));
    auto& pin = outputPins_.back();
    pin.SetName(name);
    return pin;
}

GraphInPin& GraphNode::GetOrAddExit(const ea::string_view name)
{
    for (auto& pin : exitPins_)
    {
        if (pin.GetName() == name)
        {
            return pin;
        }
    }
    exitPins_.push_back(GraphInPin(this, PINDIR_EXIT));
    auto& pin = exitPins_.back();
    pin.SetName(name);
    return pin;
}

GraphOutPin& GraphNode::GetOrAddEnter(const ea::string_view name)
{
    for (auto& pin : enterPins_)
    {
        if (pin.GetName() == name)
        {
            return pin;
        }
    }
    enterPins_.push_back(GraphOutPin(this, PINDIR_ENTER));
    auto& pin = enterPins_.back();
    pin.SetName(name);
    return pin;
}

void GraphNode::SetName(const ea::string& name)
{
    if (name_ != name)
    {
        name_ = name;
        nameHash_ = name;
    }
}

bool GraphNode::Serialize(Archive& archive)
{
    unsigned numPins = archive.IsInput() ? 0 : enterPins_.size() + exitPins_.size() + inputPins_.size() + outputPins_.size();

    if (archive.IsInput())
    {
        // TODO: check if properties present
        SerializeValue(archive, "properties", properties_);
    }
    else
    {
        if (!properties_.empty())
        {
            SerializeValue(archive, "properties", properties_);
        }
    }

    if (auto block = archive.OpenArrayBlock("pins", numPins))
    {
        if (archive.IsInput())
        {
            enterPins_.clear();
            exitPins_.clear();
            inputPins_.clear();
            outputPins_.clear();

            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                if (ArchiveBlock block = archive.OpenUnorderedBlock("pin"))
                {
                    GraphPinDirection direction = PINDIR_INPUT;

                    //TODO: handle error
                    SerializeEnum(archive, "direction", DirectionEnumConstants, direction);

                    switch (direction)
                    {
                    case PINDIR_INPUT:
                    {
                        GraphInPin pin(this, direction);
                        inputPins_.emplace_back(std::move(pin));
                        pin.Serialize(archive);
                        break;
                    }
                    case PINDIR_OUTPUT:
                    {
                        GraphOutPin pin(this, direction);
                        outputPins_.emplace_back(std::move(pin));
                        pin.Serialize(archive);
                        break;
                    }
                    case PINDIR_ENTER:
                    {
                        GraphOutPin pin(this, direction);
                        enterPins_.emplace_back(std::move(pin));
                        pin.Serialize(archive);
                        break;
                    }
                    case PINDIR_EXIT:
                    {
                        GraphInPin pin(this, direction);
                        exitPins_.emplace_back(std::move(pin));
                        pin.Serialize(archive);
                        break;
                    }
                    }
                }
            }
            return true;
        }
        else
        {
            if (!SerializePins(archive, inputPins_.begin(), inputPins_.end()))
                return false;
            if (!SerializePins(archive, outputPins_.begin(), outputPins_.end()))
                return false;
            if (!SerializePins(archive, enterPins_.begin(), enterPins_.end()))
                return false;
            if (!SerializePins(archive, exitPins_.begin(), exitPins_.end()))
                return false;
            return true;
        }
    }
    return false;
}

void GraphNode::SetGraph(Graph* scene, unsigned id)
{
    graph_ = scene;
    id_ = id;
}

} // namespace Urho3D
