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

namespace Urho3D
{
class GraphNode;
class GraphExitPin;
class GraphEnterPin;
class GraphInPin;
class GraphOutPin;
template <typename PinType> class GraphPinRef;

/// Abstract graph to store connected nodes.
class URHO3D_API Graph : public Object
{
    URHO3D_OBJECT(Graph, Object);

public:
    static constexpr unsigned MAX_ID = std::numeric_limits<unsigned>::max();
    static constexpr unsigned FIRST_ID = 1;

    /// Construct.
    explicit Graph(Context* context);
    /// Destruct. Free all resources.
    ~Graph() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Get number of nodes in the graph.
    unsigned GetNumNodes() const { return nodes_.size(); }

    /// Get node id that is not present in the graph.
    unsigned GetNextNodeId() const { return laskKnownNodeID_; }

    /// Get node ids.
    void GetNodeIds(ea::vector<unsigned>& ids) const;

    /// Get node by id.
    GraphNode* GetNode(unsigned id) const;

    /// Load graph from xml. Return true if successful.
    bool LoadXML(const ea::string_view xml);

    /// Load graph from json. Return true if successful.
    bool LoadJSON(const ea::string_view json);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

    /// Add node to the graph.
    void Add(GraphNode*);

    /// Create node at the graph.
    GraphNode* Create(const ea::string& name);

    /// Add node to the graph.
    void Remove(GraphNode*);

    /// Remove all nodes.
    void Clear();

    /// Connect exit pin to enter pin.
    void Connect(GraphPinRef<GraphExitPin> pin, GraphPinRef<GraphEnterPin> target);

    /// Connect input pin to output pin.
    void Connect(GraphPinRef<GraphInPin> pin, GraphPinRef<GraphOutPin> target);

    /// Get pin connected to the exit pin.
    GraphPinRef<GraphEnterPin> GetConnectedPin(GraphExitPin& pin) const;

    /// Get pin connected to the input pin.
    GraphPinRef<GraphOutPin> GetConnectedPin(GraphInPin& pin) const;

private:
    /// Get free node ID.
    unsigned GetFreeNodeID();

    /// Last known node ID.
    unsigned laskKnownNodeID_{FIRST_ID};

    /// Replicated scene nodes by ID.
    ea::unordered_map<unsigned, SharedPtr<GraphNode>> nodes_;
};

} // namespace Urho3D
