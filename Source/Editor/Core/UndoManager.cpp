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

BaseEditorActionWrapper::BaseEditorActionWrapper(SharedPtr<EditorAction> action)
    : action_(action)
{
}

bool BaseEditorActionWrapper::RemoveOnUndo() const
{
    return action_->RemoveOnUndo();
}

bool BaseEditorActionWrapper::IsComplete() const
{
    return action_->IsComplete();
}

bool BaseEditorActionWrapper::IsTransparent() const
{
    return action_->IsTransparent();
}

void BaseEditorActionWrapper::OnPushed(EditorActionFrame frame)
{
    action_->OnPushed(frame);
}

void BaseEditorActionWrapper::Complete(bool force)
{
    action_->Complete(force);
}

bool BaseEditorActionWrapper::CanRedo() const
{
    return action_->CanRedo();
}

void BaseEditorActionWrapper::Redo() const
{
    action_->Redo();
}

bool BaseEditorActionWrapper::CanUndo() const
{
    return action_->CanUndo();
}

void BaseEditorActionWrapper::Undo() const
{
    action_->Undo();
}

bool BaseEditorActionWrapper::MergeWith(const EditorAction& other)
{
    if (typeid(*this) != typeid(other))
        return false;

    const auto& otherWrapper = static_cast<const BaseEditorActionWrapper&>(other);
    return action_->MergeWith(*otherWrapper.action_);
}

bool UndoManager::ActionGroup::CanRedo() const
{
    const auto canRedo = [](const EditorActionPtr& action) { return action->CanRedo(); };
    return ea::all_of(actions_.begin(), actions_.end(), canRedo);
}

bool UndoManager::ActionGroup::CanUndo() const
{
    const auto canUndo = [](const EditorActionPtr& action) { return action->CanUndo(); };
    return ea::all_of(actions_.begin(), actions_.end(), canUndo);
}

UndoManager::UndoManager(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_INPUTEND, &UndoManager::Update);
}

void UndoManager::NewFrame()
{
    ++frame_;
}

EditorActionFrame UndoManager::PushAction(const EditorActionPtr& action)
{
    ClearCanUndoRedo();
    action->OnPushed(frame_);

    if (!action->IsTransparent())
        redoStack_.clear();

    if (NeedNewGroup())
        undoStack_.push_back(ActionGroup{frame_});

    // Try merge first
    ActionGroup& group = undoStack_.back();
    if (!group.actions_.empty())
    {
        if (group.actions_.back()->MergeWith(*action))
            return frame_;
    }

    group.actions_.push_back(action);

    CommitIncompleteAction(true);
    if (!action->IsComplete())
    {
        incompleteAction_ = action;
        incompleteActionTimer_.Reset();
    }

    return frame_;
}

bool UndoManager::Undo()
{
    try
    {
        if (undoStack_.empty())
            return false;

        ClearCanUndoRedo();
        CommitIncompleteAction(true);

        ActionGroup& group = undoStack_.back();
        if (!group.CanUndo())
            return false;

        for (EditorAction* action : ea::reverse(group.actions_))
            action->Undo();

        ea::erase_if(group.actions_, [](const EditorActionPtr& action) { return action->RemoveOnUndo(); });

        if (!group.actions_.empty())
            redoStack_.push_back(ea::move(group));
        undoStack_.pop_back();
        return true;
    }
    catch (const UndoException& e)
    {
        URHO3D_ASSERTLOG(0, "Desynchronized on UndoManager::Undo: {}", e.what());
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

        ClearCanUndoRedo();
        CommitIncompleteAction(true);

        ActionGroup& group = redoStack_.back();
        if (!group.CanRedo())
            return false;

        for (EditorAction* action : group.actions_)
            action->Redo();

        undoStack_.push_back(ea::move(group));
        redoStack_.pop_back();
        return true;
    }
    catch (const UndoException& e)
    {
        URHO3D_ASSERTLOG(0, "Desynchronized on UndoManager::Redo: {}", e.what());
        redoStack_.clear();
        undoStack_.clear();
        return false;
    }
}

bool UndoManager::CanUndo() const
{
    if (!canUndo_)
        canUndo_ = !undoStack_.empty() && undoStack_.back().CanUndo();
    return *canUndo_;
}

bool UndoManager::CanRedo() const
{
    if (!canRedo_)
        canRedo_ = !redoStack_.empty() && redoStack_.back().CanRedo();
    return *canRedo_;
}

void UndoManager::ClearCanUndoRedo()
{
    canRedo_ = ea::nullopt;
    canUndo_ = ea::nullopt;
}

void UndoManager::Update()
{
    const bool mouseDown = ui::IsMouseDown(MOUSEB_LEFT)
        || ui::IsMouseDown(MOUSEB_RIGHT) || ui::IsMouseDown(MOUSEB_MIDDLE);
    if (!mouseDown)
        NewFrame();

    if (incompleteAction_ && NeedNewGroup() && incompleteActionTimer_.GetMSec(false) > actionCompletionTimeoutMs_)
        CommitIncompleteAction(false);
}

bool UndoManager::NeedNewGroup() const
{
    return undoStack_.empty() || undoStack_.back().frame_ != frame_;
}

void UndoManager::CommitIncompleteAction(bool force)
{
    if (!incompleteAction_)
        return;

    incompleteAction_->Complete(force);
    if (incompleteAction_->IsComplete())
        incompleteAction_ = nullptr;
    else if (force)
        URHO3D_LOGERROR("Incomplete action failed to complete when it was forced");
}

}
