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

#include <Toolbox/Common/UndoStack.h>

namespace Urho3D
{

class Tab;
class SceneTab;
class ResourceTab;

class UndoSetSelection : public UndoAction
{
public:
    ///
    UndoSetSelection(Tab* oldTab, ByteVector  oldSelection, Tab* newTab, ByteVector  newSelection);
    ///
    bool Undo(Context* context) override;
    ///
    bool Redo(Context* context) override;

protected:
    ///
    WeakPtr<Tab> oldTab_;
    ///
    ByteVector oldSelection_;
    ///
    WeakPtr<Tab> newTab_;
    ///
    ByteVector newSelection_;
};

}
