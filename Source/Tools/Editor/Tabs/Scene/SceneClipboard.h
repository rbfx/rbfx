//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <EASTL/hash_set.h>
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
        nodes_.push_back(other.nodes_);
        components_.push_back(other.components_);
    }

    ///
    ea::vector<Node*> nodes_;
    ///
    ea::vector<Component*> components_;
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
    void Copy(const ea::vector<Node*>& nodes);
    ///
    void Copy(const ea::vector<Component*>& components);
    ///
    void Copy(const ea::vector<WeakPtr<Node>>& nodes);
    ///
    void Copy(const ea::hash_set<WeakPtr<Component>>& components);
    ///
    PasteResult Paste(Node* node);
    ///
    PasteResult Paste(const ea::vector<Node*>& nodes);
    ///
    PasteResult Paste(const ea::vector<WeakPtr<Node>>& nodes);
    ///
    bool HasNodes() const { return !nodes_.empty(); }
    ///
    bool HasComponents() const { return !components_.empty(); }

protected:
    ///
    ea::vector<VectorBuffer> nodes_;
    ///
    ea::vector<VectorBuffer> components_;
    ///
    Undo::Manager& undo_;
};


}
