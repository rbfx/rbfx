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
#include "UpdatePinValue.h"

namespace Urho3D
{
UpdatePinValueAction::UpdatePinValueAction(GraphViewTab* graphTab, ax::NodeEditor::NodeId id, ax::NodeEditor::PinId pin,
    const Variant& oldValue, const Variant& newValue)
    : graphTab_(graphTab)
    , nodeId_(id)
    , pinId_(pin)
    , oldValue_(oldValue)
    , newValue_(newValue)
{
}

void UpdatePinValueAction::Redo() const
{
    const auto graphView = graphTab_->GetGraphView();
    const auto node = graphView->nodes_.find(nodeId_);
    if (node != graphView->nodes_.end())
    {
        for (auto& pin : node->second.inputPins_)
        {
            if (pin.id_ == pinId_)
            {
                pin.value_ = newValue_;
                pin.text_ = pin.value_.ToString();
                return;
            }
        }
    }
}

void UpdatePinValueAction::Undo() const
{
    const auto graphView = graphTab_->GetGraphView();
    const auto node = graphView->nodes_.find(nodeId_);
    if (node != graphView->nodes_.end())
    {
        for (auto& pin : node->second.inputPins_)
        {
            if (pin.id_ == pinId_)
            {
                pin.value_ = oldValue_;
                pin.text_ = pin.value_.ToString();
                return;
            }
        }
    }
}

bool UpdatePinValueAction::MergeWith(const EditorAction& other)
{
    const auto otherAction = dynamic_cast<const UpdatePinValueAction*>(&other);
    if (!otherAction)
        return false;
    if (otherAction->graphTab_ != graphTab_)
        return false;
    if (otherAction->nodeId_ != nodeId_)
        return false;
    if (otherAction->pinId_ != pinId_)
        return false;

    newValue_ = otherAction->newValue_;

    return true;
}

} // namespace Urho3D
