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

#include "Graph.h"
#include "GraphNode.h"
#include "XMLArchive.h"
#include "XMLFile.h"

#include "../IO/ArchiveSerialization.h"
#include "Urho3D/IO/MemoryBuffer.h"

namespace Urho3D
{

Graph::Graph(Context* context)
    : Object(context)
{
}

Graph::~Graph() { Clear(); }

void Graph::RegisterObject(Context* context) {}

void Graph::Clear()
{
    laskKnownNodeID_ = FIRST_ID;

    for (auto& i : nodes_)
    {
        i.second->SetGraph(nullptr, i.first);
    }
    nodes_.clear();
}
/// Connect exit pin to enter pin.
void Graph::Connect(GraphPinRef<GraphExitPin> pin, GraphPinRef<GraphEnterPin> target) {

}

/// Connect input pin to output pin.
void Graph::Connect(GraphPinRef<GraphInPin> pin, GraphPinRef<GraphOutPin> target) {
    
}

/// Get pin connected to the exit pin.
GraphPinRef<GraphEnterPin> Graph::GetConnectedPin(GraphExitPin& pin) const
{
    if (!pin.targetNode_)
        return {};

    if (auto target = GetNode(pin.targetNode_))
    {
        return target->GetEnter(pin.targetPin_);
    }
    return {};
}

/// Get pin connected to the input pin.
GraphPinRef<GraphOutPin> Graph::GetConnectedPin(GraphInPin& pin) const
{
    if (!pin.targetNode_)
        return {};

    if (auto target = GetNode(pin.targetNode_))
    {
        return target->GetOutput(pin.targetPin_);
    }
    return {};
}

void Graph::GetNodeIds(ea::vector<unsigned>& ids) const
{
    ids.clear();
    ids.reserve(nodes_.size());
    for (auto& node : nodes_)
    {
        ids.push_back(node.first);
    }
}

void Graph::SerializeInBlock(Archive& archive)
{
    unsigned lastNodeId{};
    SerializeMap(archive, "nodes", nodes_, "node",
        [&](Archive& archive, const char* name, auto& value)
    {
        using ValueType = ea::remove_reference_t<decltype(value)>;
        static constexpr bool isKey = ea::is_same_v<ValueType, unsigned>;
        static constexpr bool isValue = ea::is_same_v<ValueType, SharedPtr<GraphNode>>;
        static_assert(isKey != isValue, "Unexpected callback invocation");

        if constexpr (isKey)
        {
            SerializeValue(archive, "id", value);
            lastNodeId = value;
        }
        else if constexpr (isValue)
        {
            const bool loading = archive.IsInput();
            if (loading)
            {
                value = MakeShared<GraphNode>(context_);
                value->SetGraph(this, lastNodeId);
            }

            value->SerializeInBlock(archive);
        }
    });
}

GraphNode* Graph::GetNode(unsigned id) const
{
    const auto i = nodes_.find(id);
    if (i == nodes_.end())
    {
        return nullptr;
    }
    return i->second;
}

bool Graph::LoadXML(const ea::string_view xml)
{
    MemoryBuffer buffer(xml);
    XMLFile file(context_);
    if (!file.Load(buffer))
    {
        return false;
    }

    return file.LoadObject(*this);
}

GraphNode* Graph::Create(const ea::string& name)
{
    auto node = MakeShared<GraphNode>(context_);
    node->SetName(name);
    Add(node);
    return node;
}

void Graph::Add(GraphNode* node)
{
    // Skip if no node provided.
    if (!node)
        return;

    // Skip if already added.
    if (node->GetGraph() == this)
        return;

    // Reference node so we won't delete it in the process.
    SharedPtr<GraphNode> ptr = SharedPtr<GraphNode>(node);

    // Detach from previous graph.
    if (const auto prevGraph = node->GetGraph())
    {
        prevGraph->Remove(node);
    }

    // If the new node has an ID of zero (default), assign an ID now
    unsigned id = node->GetID();
    if (!id)
    {
        id = GetFreeNodeID();
        nodes_[id] = node;
        node->SetGraph(this, id);
        return;
    }

    auto i = nodes_.find(id);
    if (i == nodes_.end())
    {
        if (id >= laskKnownNodeID_)
        {
            if (id == MAX_ID - 1)
                laskKnownNodeID_ = FIRST_ID;
            else
                laskKnownNodeID_ = id + 1;
        }
        nodes_[id] = node;
        node->SetGraph(this, id);
        return;
    }

    id = GetFreeNodeID();
    nodes_[id] = node;
    node->SetGraph(this, id);
}

void Graph::Remove(GraphNode* node)
{
    // Skip if no node provided.
    if (!node)
        return;

    // Skip if already removed.
    if (node->GetGraph() != this)
        return;

    unsigned id = node->GetID();
    nodes_.erase(id);

    // Keep id after deletion.
    node->SetGraph(nullptr, id);
}

unsigned Graph::GetFreeNodeID()
{
    for (;;)
    {
        unsigned ret = laskKnownNodeID_;
        if (laskKnownNodeID_ < MAX_ID)
            ++laskKnownNodeID_;
        else
            laskKnownNodeID_ = FIRST_ID;

        if (!nodes_.contains(ret))
            return ret;
    }
}

} // namespace Urho3D
