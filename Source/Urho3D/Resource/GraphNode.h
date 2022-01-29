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

#pragma once

#include "../Core/Context.h"
#include "GraphPin.h"
#include <EASTL/fixed_vector.h>
#include <EASTL/span.h>

namespace Urho3D
{
class Graph;

class URHO3D_API GraphNodeProperty
{
public:
    /// Property value.
    Variant value_;

    /// Get property name.
    /// @property
    const ea::string& GetName() const { return name_; }

    /// Get property name hash.
    /// @property
    StringHash GetNameHash() const { return nameHash_; }

    /// Set property name.
    /// @property
    void SetName(const ea::string_view name)
    {
        name_ = name;
        nameHash_ = name;
    }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

private:
    /// Property name.
    ea::string name_;
    /// Property name hash.
    StringHash nameHash_;
};

/// Graph node.
class URHO3D_API GraphNode : public Object
{
    URHO3D_OBJECT(GraphNode, Object);

public:
    /// Construct.
    explicit GraphNode(Context* context);
    /// Destruct. Free all resources.
    ~GraphNode() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Return ID.
    /// @property{get_id}
    unsigned GetID() const { return id_; }

    /// Return Graph.
    /// @property{get_graph}
    Graph* GetGraph() const { return graph_; }

    /// Return name.
    /// @property
    const ea::string& GetName() const { return name_; }

    /// Return name hash.
    StringHash GetNameHash() const { return nameHash_; }

    /// Get or add node property.
    Variant& GetOrAddProperty(const ea::string_view name);

    /// Get node property. Returns nullptr if property is not found.
    Variant* GetProperty(const ea::string_view name);

    /// Get properties.
    ea::span<GraphNodeProperty> GetProperties() { return ea::span(properties_); }

    /// Add property with value.
    GraphNode* WithProperty(const ea::string_view name, const Variant& value);

    /// Get input pin by name.
    GraphPinRef<GraphInPin> GetInput(const ea::string_view name);

    /// Get input pin by index.
    GraphPinRef<GraphInPin> GetInput(unsigned index);

    /// Get number of input pins.
    unsigned GetNumInputs() const { return inputPins_.size(); }

    /// Get or add input pin.
    GraphPinRef<GraphInPin> GetOrAddInput(const ea::string_view name);

    /// Add input pin.
    GraphNode* WithInput(const ea::string_view name, VariantType type = VAR_NONE);

    /// Add input pin with value.
    GraphNode* WithInput(const ea::string_view name, const Variant& value);

    /// Add input pin connected to the output pin.
    GraphNode* WithInput(const ea::string_view name, GraphPinRef<GraphOutPin> pin, VariantType type = VAR_NONE);

    /// Get or add output pin.
    GraphPinRef<GraphOutPin> GetOrAddOutput(const ea::string_view name);

    /// Add output pin.
    GraphNode* WithOutput(const ea::string_view name, VariantType type = VAR_NONE);

    /// Get output pin by name.
    GraphPinRef<GraphOutPin> GetOutput(const ea::string_view name);

    /// Get output pin by index.
    GraphPinRef<GraphOutPin> GetOutput(unsigned index);

    /// Get number of output pins.
    unsigned GetNumOutputs() const { return outputPins_.size(); }

    /// Get or add enter pin.
    GraphPinRef<GraphEnterPin> GetOrAddEnter(const ea::string_view name);

    /// Get enter pin by name.
    GraphPinRef<GraphEnterPin> GetEnter(const ea::string_view name);

    /// Get enter pin by index.
    GraphPinRef<GraphEnterPin> GetEnter(unsigned index);

    /// Get number of enter pins.
    unsigned GetNumEnters() const { return enterPins_.size(); }

    /// Get exit pin.
    GraphPinRef<GraphExitPin> GetExit(const ea::string_view name);

    /// Get enter pin by index.
    GraphPinRef<GraphExitPin> GetExit(unsigned index);

    /// Get or add exit pin.
    GraphPinRef<GraphExitPin> GetOrAddExit(const ea::string_view name);

    /// Get number of exit pins.
    unsigned GetNumExits() const { return exitPins_.size(); }

    /// Set name of the graph node.
    /// @property
    void SetName(const ea::string& name);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Set graph and id. Called by Graph.
    void SetGraph(Graph* scene, unsigned id);

private:
    /// Name.
    ea::string name_;
    /// Name hash.
    StringHash nameHash_;

    /// Graph.
    Graph* graph_;
    /// Unique ID within the scene.
    unsigned id_;

    /// User defined properties of the node.
    ea::fixed_vector<GraphNodeProperty, 1> properties_;

    /// Enter pins. Define execution flow.
    ea::fixed_vector<GraphEnterPin, 1> enterPins_;
    /// Exit pins. Define next nodes in execution flow.
    ea::fixed_vector<GraphExitPin, 1> exitPins_;

    /// Input pins. Define source pin for the data flow.
    ea::fixed_vector<GraphInPin, 3> inputPins_;
    /// Output pins. Define data flow.
    ea::fixed_vector<GraphOutPin, 1> outputPins_;

    friend class Graph;
    friend class GraphNode;
};

template <typename PinType> template<typename OtherPinType> GraphPinRef<OtherPinType> GraphPinRef<PinType>::GetConnectedPin() const
{
    return node_->GetGraph()->GetConnectedPin(*pin_);
}

} // namespace Urho3D
