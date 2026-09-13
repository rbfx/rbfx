// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Context.h"

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
