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

/// Container implementation that stores elements in a fixed vector.
template <typename T, size_t nodeCount> struct GraphNodeMapHelper
{
    GraphNodeMapHelper(ea::fixed_vector<T, nodeCount>& vector)
        : vector_(vector)
    {
    }

    T& GetOrAdd(const ea::string_view name, ea::function<T()> add)
    {
        for (T& val : vector_)
        {
            if (val.GetName() == name)
                return val;
        }
        T value = add();
        value.SetName(name);
        vector_.push_back(ea::move(value));
        return vector_.back();
    }

    T& GetOrAdd(const ea::string_view name)
    {
        for (T& val : vector_)
        {
            if (val.GetName() == name)
                return val;
        }
        vector_.push_back();
        T& value = vector_.back();
        value.SetName(name);
        return value;
    }

    bool Add(T&& value)
    {
        for (T& val : vector_)
        {
            if (val.GetName() == value.GetName())
                return false;
        }
        vector_.push_back(ea::move(value));
        return true;
    }

    ea::fixed_vector<T, nodeCount>& vector_;
};

namespace
{

static char* DirectionEnumConstants[]{"In", "Out", "Enter", "Exit", nullptr};

template <typename T, size_t nodeCount>
GraphNodeMapHelper<T, nodeCount> MakeMapHelper(ea::fixed_vector<T, nodeCount>& vector)
{
    return GraphNodeMapHelper<T, nodeCount>(vector);
}


bool SerializeValue(Archive& archive, const char* name, GraphNodeProperty& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        return value.Serialize(archive);
    }
    return false;
}


}

bool GraphNodeProperty::Serialize(Archive& archive)
{
    if (!SerializeValue(archive, "name", name_))
        return false;

    if (archive.IsInput())
    {
        ea::string customHint;
        SerializeValue(archive, "hint", customHint);
        if (customHint == "...")
        {
            // value_.SetCustom()
        }
    }
    else
    {
        if (value_.GetType() == VAR_CUSTOM)
        {
            //TODO: add custom type hints
            ea::string customHint = "...";
            SerializeValue(archive, "hint", customHint);
        }
    }
    SerializeValue(archive, "value", value_);

    return true;
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


template <typename Iterator>
bool GraphNode::SerializePins(Archive& archive, Iterator begin, Iterator end)
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

Variant& GraphNode::GetOrAddProperty(const ea::string_view name)
{
    return MakeMapHelper(properties_).GetOrAdd(name).value_;
}

GraphDataInPin& GraphNode::GetOrAddInput(const ea::string_view name)
{
    return MakeMapHelper(inputPins_).GetOrAdd(name, [this]() { return GraphDataInPin(this, PINDIR_INPUT); });
}

GraphOutPin& GraphNode::GetOrAddOutput(const ea::string_view name)
{
    return MakeMapHelper(outputPins_).GetOrAdd(name, [this]() { return GraphOutPin(this, PINDIR_OUTPUT); });
}

GraphInPin& GraphNode::GetOrAddExit(const ea::string_view name)
{
    return MakeMapHelper(exitPins_).GetOrAdd(name, [this]() { return GraphInPin(this, PINDIR_EXIT); });
}

GraphOutPin& GraphNode::GetOrAddEnter(const ea::string_view name)
{
    return MakeMapHelper(enterPins_).GetOrAdd(name, [this]() { return GraphOutPin(this, PINDIR_ENTER); });
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
        SerializeVectorAsObjects(archive, "properties", "property", properties_);
    }
    else
    {
        if (!properties_.empty())
        {
            SerializeVectorAsObjects(archive, "properties", "property", properties_);
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
                        GraphDataInPin pin(this, direction);
                        pin.Serialize(archive);
                        MakeMapHelper(inputPins_).Add(ea::move(pin));
                        break;
                    }
                    case PINDIR_OUTPUT:
                    {
                        GraphOutPin pin(this, direction);
                        pin.Serialize(archive);
                        MakeMapHelper(outputPins_).Add(ea::move(pin));
                        break;
                    }
                    case PINDIR_ENTER:
                    {
                        GraphOutPin pin(this, direction);
                        pin.Serialize(archive);
                        MakeMapHelper(enterPins_).Add(ea::move(pin));
                        break;
                    }
                    case PINDIR_EXIT:
                    {
                        GraphInPin pin(this, direction);
                        pin.Serialize(archive);
                        MakeMapHelper(exitPins_).Add(ea::move(pin));
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
