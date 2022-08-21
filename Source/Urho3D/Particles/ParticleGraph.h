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

#include "ParticleGraphPin.h"
#include "../Resource/Graph.h"
#include "../Resource/GraphPin.h"

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
