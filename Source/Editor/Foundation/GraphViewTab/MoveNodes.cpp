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

#include "MoveNodes.h"

#include "GraphViewTab.h"

namespace Urho3D
{

MoveNodesAction::MoveNodesAction(Detail::GraphView* graphView)
    : graphView_(graphView)
{
}

void MoveNodesAction::Add(ax::NodeEditor::NodeId id, const Vector2& oldPos, const Vector2& newPos)
{
    auto existing = nodes_.find(id);
    if (existing != nodes_.end())
    {
        nodes_[id] = ea::make_tuple(ea::get<0>(existing->second), newPos);
    }
    else
    {
        nodes_[id] = ea::make_tuple(oldPos, newPos);
    }
}

void MoveNodesAction::Redo() const
{
    for (auto& kv : nodes_)
    {
        auto it = graphView_->nodes_.find(kv.first);
        if (it != graphView_->nodes_.end())
        {
            it->second.SetPosition(ea::get<1>(kv.second));
        }
    }
}

void MoveNodesAction::Undo() const
{
    auto graphView = graphView_;

    for (auto& kv : nodes_)
    {
        auto it = graphView->nodes_.find(kv.first);
        if (it != graphView->nodes_.end())
        {
            it->second.SetPosition(ea::get<0>(kv.second));
        }
    }
}

bool MoveNodesAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const MoveNodesAction*>(&other);
    if (!otherAction)
        return false;
    if (otherAction->graphView_ != graphView_)
        return false;

    for (auto& kv: otherAction->nodes_)
    {
        Add(kv.first, ea::get<0>(kv.second), ea::get<1>(kv.second));
    }
       
    return true;
}

} // namespace Urho3D
