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

#include "../Precompiled.h"

#include "ParticleGraph.h"

#include "../IO/Log.h"
#include "../Resource/Graph.h"
#include "Nodes/Constant.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphSystem.h"

namespace Urho3D
{

ParticleGraph::ParticleGraph(Context* context)
    : Object(context)
{
}

ParticleGraph::~ParticleGraph() = default;

unsigned ParticleGraph::Add(const SharedPtr<ParticleGraphNode> node)
{
    if (!node)
    {
        URHO3D_LOGERROR("Can't add empty node");
        return 0;
    }
    const auto index = static_cast<unsigned>(nodes_.size());
    nodes_.push_back(node);
    node->SetGraph(this, index);
    return index;
}

unsigned ParticleGraph::GetNumNodes() const { return static_cast<unsigned>(nodes_.size()); }

SharedPtr<ParticleGraphNode> ParticleGraph::GetNode(unsigned index) const
{
    if (index >= nodes_.size())
    {
        URHO3D_LOGERROR("Node index out of bounds");
        return {};
    }
    return nodes_[index];
}

bool ParticleGraph::LoadGraph(Graph& graph)
{
    ParticleGraphReader reader(*this, graph);
    return reader.Read();
}

bool ParticleGraph::SaveGraph(Graph& graph)
{
    graph.Clear();
    ParticleGraphWriter writer(*this, graph);
    return writer.Write();
}

void ParticleGraph::SerializeInBlock(Archive& archive)
{
    Graph graph(context_);
    if (archive.IsInput())
    {
        graph.SerializeInBlock(archive);
        LoadGraph(graph);
    }
    else
    {
        SaveGraph(graph);
        graph.SerializeInBlock(archive);
    }
}

ParticleGraphWriter::ParticleGraphWriter(ParticleGraph& particleGraph, Graph& graph)
    : particleGraph_(particleGraph)
    , graph_(graph)
    , system_(particleGraph.GetContext()->GetSubsystem<ParticleGraphSystem>())
{
    nodes_.resize(particleGraph_.GetNumNodes());
    for (unsigned i = 0; i < particleGraph_.GetNumNodes(); ++i)
    {
        nodes_[i] = 0;
    }
}

bool ParticleGraphWriter::Write()
{
    for (unsigned i=0; i<particleGraph_.GetNumNodes(); ++i)
    {
        const auto id = WriteNode(i);
        if (!id)
            return false;
    }
    return true;
}

unsigned ParticleGraphWriter::WriteNode(unsigned index)
{
    if (nodes_[index])
    {
        return nodes_[index];
    }
    const auto node = particleGraph_.GetNode(index);
    const auto outNode = graph_.Create(node->GetTypeName());
    nodes_[index] = outNode->GetID();
    if (!node->Save(*this, *outNode))
        return false;
    return true;
}

GraphPinRef<GraphOutPin> ParticleGraphWriter::GetSourcePin(unsigned nodeIndex, unsigned pinIndex)
{
    const auto node = particleGraph_.GetNode(nodeIndex);
    const auto outNode = nodes_[nodeIndex];
    const auto pin = node->GetPin(pinIndex);
    return graph_.GetNode(outNode)->GetOrAddOutput(pin.GetName());
}

ParticleGraphReader::ParticleGraphReader(ParticleGraph& particleGraph, Graph& graph)
    : particleGraph_(particleGraph)
    , graph_(graph)
    , system_(particleGraph.GetContext()->GetSubsystem<ParticleGraphSystem>())
{
    graph.GetNodeIds(ids_);
}

unsigned ParticleGraphReader::ReadNode(unsigned id)
{
    {
        const auto it = nodes_.find(id);
        if (it != nodes_.end())
        {
            if (it->second == ParticleGraph::INVALID_NODE_INDEX)
            {
                URHO3D_LOGERROR("Loop detected at particle graph");
                return it->second;
            }
            return it->second;
        }
    }
    nodes_[id] = ParticleGraph::INVALID_NODE_INDEX;

    auto *srcNode = graph_.GetNode(id);
    auto newNode = system_->CreateObject(srcNode->GetNameHash());
    if (!newNode)
    {
        URHO3D_LOGERROR(Format("Unknown node type {}", srcNode->GetName()));
        return ParticleGraph::INVALID_NODE_INDEX;
    }
    SharedPtr<ParticleGraphNode> dstNode;
    dstNode.StaticCast(newNode);
    if (!dstNode->Load(*this, *srcNode))
        return ParticleGraph::INVALID_NODE_INDEX;

    for (unsigned i=0; i<dstNode->GetNumPins(); ++i)
    {
        auto pin = dstNode->GetPin(i);
        if (pin.IsInput() && pin.GetRequestedType() != VAR_NONE)
        {
            if (pin.GetConnectedNodeIndex() == ParticleGraph::INVALID_NODE_INDEX)
            {
                auto constNode = MakeShared<ParticleGraphNodes::Constant>(system_->GetContext());
                constNode->SetValue(Variant(pin.GetRequestedType()));
                auto constIndex = particleGraph_.Add(constNode);
                dstNode->SetPinSource(i, constIndex, 0);
            }
        }
    }

    auto dstIndex =  particleGraph_.Add(dstNode);
    nodes_[id] = dstIndex;
    return dstIndex;
}
unsigned ParticleGraphReader::GetOrAddConstant(const Variant& constValue)
{
    const auto it = constants_.find(constValue);
    if (it == constants_.end())
    {
        const auto constNode = MakeShared<ParticleGraphNodes::Constant>(particleGraph_.GetContext());
        constNode->SetValue(constValue);
        return constants_[constValue] = particleGraph_.Add(constNode);
    }
    return it->second;

}
unsigned ParticleGraphReader::GetInputPinIndex(unsigned nodeIndex, const ea::string& string)
{
    const auto node = particleGraph_.GetNode(nodeIndex);
    if (!node)
        return ParticleGraphNode::INVALID_PIN;
    return node->GetPinIndex(string);
}

bool ParticleGraphReader::Read()
{
    for (const unsigned id : ids_)
    {
        if (ReadNode(id) == ParticleGraph::INVALID_NODE_INDEX)
            return false;
    }
    return true;
}

} // namespace Urho3D
