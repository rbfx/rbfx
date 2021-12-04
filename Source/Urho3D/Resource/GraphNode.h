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

#pragma once

#include "GraphPin.h"
#include "../Core/Context.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{
class Graph;

/// Abstract graph to store connected nodes.
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

    /// Get or add input pin.
    GraphInPin& GetOrAddInput(const ea::string_view name);

    /// Get or add output pin.
    GraphOutPin& GetOrAddOutput(const ea::string_view name);

    /// Get or add enter pin.
    GraphOutPin& GetOrAddEnter(const ea::string_view name);

    /// Get or add exit pin.
    GraphInPin& GetOrAddExit(const ea::string_view name);

    /// Set name of the graph node.
    /// @property
    void SetName(const ea::string& name);

    /// Serialize from/to archive. Return true if successful.
    bool Serialize(Archive& archive);

    /// Get node properties.
    /// @property
    VariantMap& GetProperties() { return properties_; }

protected:

    /// Set graph and id. Called by Graph.
    void SetGraph(Graph* scene, unsigned id);

    template <typename Iterator> bool SerializePins(Archive& archive, Iterator begin, Iterator end);

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
    VariantMap properties_;

    /// Enter pins. Define execution flow.
    ea::fixed_vector<GraphOutPin, 1> enterPins_;
    /// Exit pins. Define next nodes in execution flow.
    ea::fixed_vector<GraphInPin, 1> exitPins_;

    /// Input pins. Define source pin for the data flow.
    ea::fixed_vector<GraphInPin, 3> inputPins_;
    /// Output pins. Define data flow.
    ea::fixed_vector<GraphOutPin, 1> outputPins_;

    friend class Graph;
};


} // namespace Urho3D
