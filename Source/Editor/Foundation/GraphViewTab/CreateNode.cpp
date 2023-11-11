//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "CreateNode.h"
#include "GraphViewTab.h"

namespace Urho3D
{
CreateNodeAction::CreateNodeAction(Detail::GraphView* graphView, const Detail::GraphNodeView* node)
    : graphView_(graphView)
{
    if (node)
    {
        nodes_.push_back(*node);
    }
}

void CreateNodeAction::Redo() const
{
    for (auto& node: nodes_)
    {
        graphView_->AddNode(node);
    }
}

void CreateNodeAction::Undo() const
{
    for (auto& node : nodes_)
    {
        graphView_->DeleteNode(node.id_);
    }
}

bool CreateNodeAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const CreateNodeAction*>(&other);
    if (!otherAction)
        return false;
    if (otherAction->graphView_ != graphView_)
        return false;

    nodes_.reserve(nodes_.size() + otherAction->nodes_.size());
    nodes_.insert(nodes_.end(), otherAction->nodes_.begin(), otherAction->nodes_.end());
    return true;
}
} // namespace Urho3D
