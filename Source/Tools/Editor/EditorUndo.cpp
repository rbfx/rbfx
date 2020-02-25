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
#include "Tabs/Scene/SceneTab.h"
#include "EditorUndo.h"


namespace Urho3D
{

UndoSetSelection::UndoSetSelection(Tab* oldTab, ByteVector oldSelection, Tab* newTab, ByteVector newSelection)
    : oldTab_(oldTab)
    , oldSelection_(std::move(oldSelection))
    , newTab_(newTab)
    , newSelection_(std::move(newSelection))
{
}

bool UndoSetSelection::Undo(Context* context)
{
    bool success = false;

    if (!oldTab_.Expired())
    {
        success |= oldTab_->DeserializeSelection(oldSelection_);
        oldTab_->Activate();
    }

    if (!newTab_.Expired() && oldTab_ != newTab_)
    {
        newTab_->ClearSelection();
        success = true;
    }

    return success;
}

bool UndoSetSelection::Redo(Context* context)
{
    bool success = false;

    if (!newTab_.Expired())
    {
        success |= newTab_->DeserializeSelection(newSelection_);
        newTab_->Activate();
    }

    if (!oldTab_.Expired() && oldTab_ != newTab_)
    {
        oldTab_->ClearSelection();
        success = true;
    }

    return success;
}

}
