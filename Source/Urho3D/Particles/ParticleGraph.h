// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Particles/ParticleGraphPin.h"
#include "Urho3D/Resource/Graph.h"
#include "Urho3D/Resource/GraphPin.h"

namespace Urho3D
{

class ParticleGraphNode;
class ParticleGraphReader;
class ParticleGraphWriter;
class ParticleGraphSystem;

class URHO3D_API ParticleGraph : public Object
{
    URHO3D_OBJECT(ParticleGraph, Object);

public:
    static constexpr unsigned INVALID_NODE_INDEX = std::numeric_limits<unsigned>::max();

    /// Construct.
    explicit ParticleGraph(Context* context);
    /// Destruct.
    virtual ~ParticleGraph();

    /// Add node to the graph.
    /// Returns node index;
    unsigned Add(const SharedPtr<ParticleGraphNode> node);

    /// Get number of nodes.
    unsigned GetNumNodes() const;

    /// Get node by index.
    SharedPtr<ParticleGraphNode> GetNode(unsigned index) const;

    /// Load graph.
    bool LoadGraph(Graph& graph);

    /// Save graph.
    bool SaveGraph(Graph& graph);

    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive) override;

private:
    /// Nodes in the graph;
    ea::vector<SharedPtr<ParticleGraphNode>> nodes_;
};

class ParticleGraphWriter
{
public:
    ParticleGraphWriter(ParticleGraph& particleGraph, Graph& graph);

    /// Write graph.
    bool Write();

    /// Write graph node. Return node id.
    unsigned WriteNode(unsigned index);

    /// Get source pin reference.
    GraphPinRef<GraphOutPin> GetSourcePin(unsigned connected_node, unsigned get_connected_pin_index);

private:
    ParticleGraph& particleGraph_;
    Graph& graph_;
    ParticleGraphSystem* system_;
    ea::vector<unsigned> nodes_;
};

class ParticleGraphReader
{
public:
    ParticleGraphReader(ParticleGraph& particleGraph, Graph& graph);

    /// Load graph.
    bool Read();

    unsigned ReadNode(unsigned id);

    unsigned GetInputPinIndex(unsigned nodeIndex, const ea::string& string);

    unsigned GetOrAddConstant(const Variant& variant);

private:
    ParticleGraph& particleGraph_;
    Graph& graph_;
    ParticleGraphSystem* system_;
    ea::vector<unsigned> ids_;
    ea::unordered_map<unsigned, unsigned> nodes_;
    ea::unordered_map<Variant, unsigned> constants_;
};

} // namespace Urho3D
