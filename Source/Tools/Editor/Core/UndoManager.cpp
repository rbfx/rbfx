//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Core/UndoManager.h"

#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <EASTL/bonus/adaptors.h>

namespace Urho3D
{

bool UndoManager::ActionGroup::IsAlive() const
{
    const auto isAlive = [](const EditorActionPtr& action) { return action->IsAlive(); };
    return ea::all_of(actions_.begin(), actions_.end(), isAlive);
}

UndoManager::UndoManager(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_INPUTEND, [this](StringHash, VariantMap&)
    {
        const bool mouseDown = ui::IsMouseDown(MOUSEB_LEFT)
            || ui::IsMouseDown(MOUSEB_RIGHT) || ui::IsMouseDown(MOUSEB_MIDDLE);
        if (!mouseDown)
            NewFrame();
    });
}

void UndoManager::NewFrame()
{
    ++frame_;
}

EditorActionFrame UndoManager::PushAction(const EditorActionPtr& action)
{
    action->OnPushed(frame_);

    if (NeedNewGroup())
        undoStack_.push_back(ActionGroup{frame_});

    ActionGroup& group = undoStack_.back();
    if (!group.actions_.empty())
    {
        if (group.actions_.back()->MergeWith(*action))
            return frame_;
    }
    group.actions_.push_back(action);
    return frame_;
}

bool UndoManager::Undo()
{
    try
    {
        if (undoStack_.empty())
            return false;

        ActionGroup& group = undoStack_.back();
        if (!group.IsAlive())
            return false;

        for (EditorAction* action : ea::reverse(group.actions_))
            action->Undo();

        redoStack_.push_back(ea::move(group));
        undoStack_.pop_back();
        return true;
    }
    catch (const UndoException& e)
    {
        URHO3D_ASSERTLOG(0, "Desynchronized on UndoManager::Undo: {}", e.GetMessage());
        redoStack_.clear();
        undoStack_.clear();
        return false;
    }
}

bool UndoManager::Redo()
{
    try
    {
        if (redoStack_.empty())
            return false;

        ActionGroup& group = redoStack_.back();
        if (!group.IsAlive())
            return false;

        for (EditorAction* action : group.actions_)
            action->Redo();

        undoStack_.push_back(ea::move(group));
        redoStack_.pop_back();
        return true;
    }
    catch (const UndoException& e)
    {
        URHO3D_ASSERTLOG(0, "Desynchronized on UndoManager::Redo: {}", e.GetMessage());
        redoStack_.clear();
        undoStack_.clear();
        return false;
    }
}

bool UndoManager::NeedNewGroup() const
{
    return undoStack_.empty() || undoStack_.back().frame_ != frame_;
}

}
