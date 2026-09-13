// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Exception.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>

#include <EASTL/optional.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Exception thrown when UndoManager stack is desynchronized with editor state.
class UndoException : public RuntimeException
{
public:
    using RuntimeException::RuntimeException;
};

/// ID corresponding to the temporal order of undo actions.
using EditorActionFrame = unsigned long long;

/// Abstract undoable and redoable action.
class EditorAction : public RefCounted
{
public:
    /// Return whether the action should be completely removed from stack on undo.
    /// Useful for injecting callback on undoing. Don't change any important state if true!
    virtual bool RemoveOnUndo() const { return false; }
    /// Return whether the action is incomplete, e.g. "redo" state is not saved. Useful for heavy actions.
    virtual bool IsComplete() const { return true; }
    /// Return if action is transparent, i.e. it can be pushed to stack or ignored without desynchronization.
    virtual bool IsTransparent() const { return false; }
    /// Action is pushed to the stack.
    virtual void OnPushed(EditorActionFrame frame) {}
    /// Complete action if needed. Called after merge attempt but before stack modification.
    /// Called with force=false periodically.
    virtual void Complete(bool force) {}
    /// Return whether the action can be redone and undone.
    virtual bool CanUndoRedo() const { return true; }
    /// Return whether the action can be redone.
    virtual bool CanRedo() const { return CanUndoRedo(); }
    /// Redo this action. May fail if external state has unexpectedly changed.
    virtual void Redo() const = 0;
    /// Return whether the action can be undone.
    virtual bool CanUndo() const { return CanUndoRedo(); }
    /// Undo this action. May fail if external state has unexpectedly changed.
    virtual void Undo() const = 0;
    /// Try to merge this action with another. Return true if successfully merged.
    virtual bool MergeWith(const EditorAction& other) { return false; }
};

/// Base class for action wrappers.
class BaseEditorActionWrapper : public EditorAction
{
public:
    explicit BaseEditorActionWrapper(SharedPtr<EditorAction> action);

    /// Implement EditorAction.
    /// @{
    bool RemoveOnUndo() const override;
    bool IsComplete() const override;
    bool IsTransparent() const override;
    void OnPushed(EditorActionFrame frame) override;
    void Complete(bool force) override;
    bool CanRedo() const override;
    void Redo() const override;
    bool CanUndo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

protected:
    SharedPtr<EditorAction> action_;
};

using EditorActionPtr = SharedPtr<EditorAction>;

/// Manages undo stack and actions.
class UndoManager : public Object
{
    URHO3D_OBJECT(UndoManager, Object);

public:
    explicit UndoManager(Context* context);

    /// Force new frame. Call it on any resource save.
    void NewFrame();
    /// Push new action. May be merged with the top of the stack. Drops redo stack.
    EditorActionFrame PushAction(const EditorActionPtr& action);
    /// Try to undo action. May fail if external state changed.
    bool Undo();
    /// Try to redo action. May fail if external state changed.
    bool Redo();

    /// Return whether can undo.
    bool CanUndo() const;
    /// Return whether can redo.
    bool CanRedo() const;

private:
    struct ActionGroup
    {
        EditorActionFrame frame_{};
        ea::vector<EditorActionPtr> actions_;

        bool CanRedo() const;
        bool CanUndo() const;
    };

    void ClearCanUndoRedo();
    void Update();
    bool NeedNewGroup() const;
    void CommitIncompleteAction(bool force);

    const unsigned actionCompletionTimeoutMs_{1000};

    ea::vector<ActionGroup> undoStack_;
    ea::vector<ActionGroup> redoStack_;
    EditorActionFrame frame_{};

    EditorActionPtr incompleteAction_;
    Timer incompleteActionTimer_;

    mutable ea::optional<bool> canUndo_;
    mutable ea::optional<bool> canRedo_;
};

}
