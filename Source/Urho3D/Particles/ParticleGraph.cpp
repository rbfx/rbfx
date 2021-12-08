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
#include "ParticleGraphNode.h"

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>


namespace Urho3D
{

/// Construct ParticleGraphLayer.
ParticleGraph::ParticleGraph(Context* context)
    : Object(context)
{
}

/// Destruct ParticleGraphLayer.
ParticleGraph::~ParticleGraph() = default;

/// Add node to the graph.
/// Returns node index;
unsigned ParticleGraph::Add(const SharedPtr<ParticleGraphNode> node)
{
    if (!node)
    {
        URHO3D_LOGERROR("Can't add empty node");
        return 0;
    }
    const auto index = static_cast<unsigned>(nodes_.size());
    nodes_.push_back(node);
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

/// Serialize from/to archive. Return true if successful.
bool ParticleGraph::Serialize(Archive& archive, const char* blockName)
{
    return SerializeVectorAsObjects(archive, blockName, "node", nodes_);
}

} // namespace Urho3D
