//
// Copyright (c) 2018 Rokas Kupstys
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


#include <Urho3D/Container/HashSet.h>
#include <Urho3D/Core/Object.h>
#include <Toolbox/Common/UndoManager.h>


namespace Urho3D
{

class Node;
class Component;

struct PasteResult
{
    ///
    void Merge(const PasteResult& other)
    {
        nodes_.Push(other.nodes_);
        components_.Push(other.components_);
    }

    ///
    PODVector<Node*> nodes_;
    ///
    PODVector<Component*> components_;
};

class SceneClipboard : public Object
{
    URHO3D_OBJECT(SceneClipboard, Object);
public:
    ///
    explicit SceneClipboard(Context* context, Undo::Manager& undo);
    ///
    void Clear();
    ///
    void Copy(Node* node);
    ///
    void Copy(Component* component);
    ///
    void Copy(const PODVector<Node*>& nodes);
    ///
    void Copy(const PODVector<Component*>& components);
    ///
    void Copy(const Vector<WeakPtr<Node>>& nodes);
    ///
    void Copy(const HashSet<WeakPtr<Component>>& components);
    ///
    PasteResult Paste(Node* node);
    ///
    PasteResult Paste(const PODVector<Node*>& nodes);
    ///
    PasteResult Paste(const Vector<WeakPtr<Node>>& nodes);
    ///
    bool HasNodes() const { return !nodes_.Empty(); }
    ///
    bool HasComponents() const { return !components_.Empty(); }

protected:
    ///
    Vector<VectorBuffer> nodes_;
    ///
    Vector<VectorBuffer> components_;
    ///
    Undo::Manager& undo_;
};


}
