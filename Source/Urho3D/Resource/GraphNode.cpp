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
template <typename T, size_t nodeCount> struct GraphMapHelper
{
    /// Construct.
    GraphMapHelper(ea::fixed_vector<T, nodeCount>& vector)
        : vector_(vector)
    {
    }

    /// Get item by name.
    T* Get(const ea::string_view name)
    {
        for (T& val : vector_)
        {
            if (val.GetName() == name)
                return &val;
        }
        return nullptr;
    }

    /// Get or create new item.
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

    /// Get or create new item.
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

    /// Add new item. Returns false if value is already present.
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

static const char* DirectionEnumConstants[]{"In", "Out", "Enter", "Exit", nullptr};

template <typename T, size_t nodeCount>
GraphMapHelper<T, nodeCount> MakeMapHelper(ea::fixed_vector<T, nodeCount>& vector)
{
    return GraphMapHelper<T, nodeCount>(vector);
}

}

bool SerializeValue(Archive& archive, const char* name, GraphNodeProperty& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        return value.Serialize(archive);
    }
    return false;
}

void GraphNodeProperty::SetName(const ea::string_view name)
{
    name_ = name;
    nameHash_ = name;
}

bool GraphNodeProperty::Serialize(Archive& archive)
{
    if (!SerializeValue(archive, "name", name_))
        return false;

    VariantType variantType = value_.GetType();
    if (!SerializeValue(archive, "type", variantType))
        return false;

    return SerializeVariantValue(archive, variantType, "value", value_);
}

GraphNode::GraphNode(Context* context)
    : Object(context)
    , graph_(nullptr)
    , id_(0)
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
            auto type = begin->type_;
            if (type != VAR_NONE)
            {
                if (!SerializeEnum(archive, "type", Variant::GetTypeNameList(), type))
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

Variant* GraphNode::GetProperty(const ea::string_view name)
{
    const auto res  = MakeMapHelper(properties_).Get(name);
    if (res)
        return &res->value_;
    return nullptr;
}

GraphNode* GraphNode::WithProperty(const ea::string_view name, const Variant& value)
{
    auto& prop = GetOrAddProperty(name);
    prop = value;
    return this;
}

GraphDataInPin* GraphNode::GetInput(const ea::string_view name)
{
    return MakeMapHelper(inputPins_).Get(name);
}

GraphDataInPin& GraphNode::GetOrAddInput(const ea::string_view name)
{
    return MakeMapHelper(inputPins_).GetOrAdd(name, [this]() { return GraphDataInPin(this, PINDIR_INPUT); });
}

GraphNode* GraphNode::WithInput(const ea::string_view name, VariantType type)
{
    auto& pin = GetOrAddInput(name);
    pin.SetName(name);
    pin.type_ = type;
    return this;
}

GraphNode* GraphNode::WithInput(const ea::string_view name, const Variant& value)
{
    auto& pin = GetOrAddInput(name);
    pin.SetName(name);
    pin.type_ = value.GetType();
    pin.SetDefaultValue(value);
    return this;
}

GraphNode* GraphNode::WithInput(const ea::string_view name, GraphOutPin* outputPin, VariantType type)
{
    auto& pin = GetOrAddInput(name);
    pin.SetName(name);
    pin.type_ = type;
    if (outputPin)
    {
        pin.ConnectTo(*outputPin);
    }
    return this;
}

GraphOutPin& GraphNode::GetOrAddOutput(const ea::string_view name)
{
    return MakeMapHelper(outputPins_).GetOrAdd(name, [this]() { return GraphOutPin(this, PINDIR_OUTPUT); });
}

GraphNode* GraphNode::WithOutput(const ea::string_view name, VariantType type)
{
    auto& pin = GetOrAddOutput(name);
    pin.SetName(name);
    pin.type_ = type;
    return this;
}

GraphOutPin* GraphNode::GetOutput(const ea::string_view name)
{
    return MakeMapHelper(outputPins_).Get(name);
}

GraphInPin* GraphNode::GetExit(const ea::string_view name)
{
    return MakeMapHelper(exitPins_).Get(name);
}

GraphInPin& GraphNode::GetOrAddExit(const ea::string_view name)
{
    return MakeMapHelper(exitPins_).GetOrAdd(name, [this]() { return GraphInPin(this, PINDIR_EXIT); });
}

GraphOutPin& GraphNode::GetOrAddEnter(const ea::string_view name)
{
    return MakeMapHelper(enterPins_).GetOrAdd(name, [this]() { return GraphOutPin(this, PINDIR_ENTER); });
}

GraphOutPin* GraphNode::GetEnter(const ea::string_view name)
{
    return MakeMapHelper(enterPins_).Get(name);
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

    SerializeValue(archive, "name", name_);

    if (archive.IsInput())
    {
        nameHash_ = name_;

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
    return true;
}

void GraphNode::SetGraph(Graph* scene, unsigned id)
{
    graph_ = scene;
    id_ = id;
}

} // namespace Urho3D
