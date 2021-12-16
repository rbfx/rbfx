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

Graph::~Graph()
{
    Clear();
}

void Graph::RegisterObject(Context* context)
{
}

void Graph::Clear()
{
    laskKnownNodeID_ = FIRST_ID;

    for (auto& i: nodes_)
    {
        i.second->SetGraph(nullptr, i.first);
    }
    nodes_.clear();
}


void Graph::GetNodeIds(ea::vector<unsigned>& ids) const
{
    ids.clear();
    ids.reserve(nodes_.size());
    for (auto& node: nodes_)
    {
        ids.push_back(node.first);
    }
}

bool Graph::Serialize(Archive& archive)
{
    if (auto block = archive.OpenArrayBlock("nodes", GetNumNodes()))
    {
        if (archive.IsInput())
        {
            Clear();
            for (unsigned i = 0; i < block.GetSizeHint(); ++i)
            {
                if (ArchiveBlock block = archive.OpenUnorderedBlock("node"))
                {
                    unsigned id;
                    if (!SerializeValue(archive, "id", id))
                        return false;
                    SharedPtr<GraphNode> node = MakeShared<GraphNode>(context_);
                    node->SetGraph(nullptr, id);
                    Add(node);
                    if (!node->Serialize(archive, block))
                        return false;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            for (auto& value : nodes_)
            {
                if (ArchiveBlock block = archive.OpenUnorderedBlock("node"))
                {
                    unsigned id = value.first;
                    if (!SerializeValue(archive, "id", id))
                        return false;
                    if (!value.second->Serialize(archive, block))
                        return false;
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
    }
    return false;
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
    XMLInputArchive archive(&file);
    return Serialize(archive);
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
                laskKnownNodeID_ = id+1;
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
