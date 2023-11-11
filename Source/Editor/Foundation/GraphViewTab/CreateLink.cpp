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

#include "GraphViewTab.h"
#include "CreateLink.h"

namespace Urho3D
{

CreateLinkAction::CreateLinkAction(Detail::GraphView* graphView, ax::NodeEditor::PinId from, ax::NodeEditor::PinId to)
    : graphView_(graphView)
{
    auto* view = graphView_;
    ax::NodeEditor::LinkId id(view->nextUniqueId_++);
    links_.emplace_back(LinkPrototype{id, from, to});
}

void CreateLinkAction::Redo() const
{
    auto* view = graphView_;
    for (auto& link: links_)
    {
        view->AddLink(link.linkId_, link.from_, link.to_);
    }
}

void CreateLinkAction::Undo() const
{
    auto* view = graphView_;
    for (auto& link : links_)
    {
        view->DeleteLink(link.linkId_);
    }
}

bool CreateLinkAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const CreateLinkAction*>(&other);
    if (!otherAction)
        return false;
    if (otherAction->graphView_ != graphView_)
        return false;

    links_.reserve(links_.size() + otherAction->links_.size());
    links_.insert(links_.end(), otherAction->links_.begin(), otherAction->links_.end());
    return true;
}
} // namespace Urho3D
