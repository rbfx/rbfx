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

#pragma once

#include "Core/UndoManager.h"

#include <ImGuiNodeEditor/imgui_node_editor.h>
#include <EASTL/fixed_vector.h>

namespace Urho3D
{
class GraphViewTab;

class DeleteLinkAction : public EditorAction
{
    struct LinkPrototype
    {
        ax::NodeEditor::LinkId linkId_;
        ax::NodeEditor::PinId from_;
        ax::NodeEditor::PinId to_;
    };

public:
    DeleteLinkAction(GraphViewTab* graphTab, ax::NodeEditor::LinkId linkId);

    /// Redo this action. May fail if external state has unexpectedly changed.
    void Redo() const override;
    /// Undo this action. May fail if external state has unexpectedly changed.
    void Undo() const override;
    /// Try to merge this action with another. Return true if successfully merged.
    bool MergeWith(const EditorAction& other) override;

private:
    GraphViewTab* graphTab_;
    ea::fixed_vector<LinkPrototype, 1> links_;
};

} // namespace Urho3D
