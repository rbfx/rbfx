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

#pragma once

#include <Urho3D/Core/Object.h>

#include <EASTL/vector.h>

namespace Urho3D
{

/// Abstract undoable and redoable action.
class EditorAction : public RefCounted
{
public:
    /// Redo this action. May fail if external state has unexpectedly changed.
    virtual bool Redo() const = 0;
    /// Undo this action. May fail if external state has unexpectedly changed.
    virtual bool Undo() const = 0;
    /// Try to merge this action with another. Return true if successfully merged.
    virtual bool MergeWith(const EditorAction& other) { return false; }
    /// Return whether the action should not reset redo stack on creation. Use with caution.
    virtual bool IsTransparent() const { return false; }
};

using EditorActionPtr = SharedPtr<EditorAction>;

/// Manages undo stack and actions.
class UndoManager : public Object
{
    URHO3D_OBJECT(UndoManager, Object);

public:
    explicit UndoManager(Context* context);

    /// Push new action. May be merged with the top of the stack. Drops redo stack.
    void PushAction(const EditorActionPtr& action);
    /// Try to undo action. May fail if external state changed.
    bool Undo();
    /// Try to redo action. May fail if external state changed.
    bool Redo();

private:
    ea::vector<EditorActionPtr> undoStack_;
    ea::vector<EditorActionPtr> redoStack_;
};

}
