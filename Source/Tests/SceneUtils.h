//
// Copyright (c) 2017-2021 the rbfx project.
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

#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;

namespace Tests
{

/// Serialize and deserialize Scene. Should preserve functional state of nodes and components.
void SerializeAndDeserializeScene(Scene* scene);

/// Return attribute value as variant.
Variant GetAttributeValue(const ea::pair<Serializable*, unsigned>& ref);

/// Weak reference to Scene node by name.
/// Useful for tests with serialization when actual objects are recreated.
struct NodeRef
{
    WeakPtr<Scene> scene_;
    ea::string name_;

    Node* GetNode() const;

    Node* operator*() const { return GetNode(); }
    Node* operator->() const { return GetNode(); }
    bool operator!() const { return GetNode() == nullptr; }
    explicit operator bool() const { return !!*this; }
};

/// Weak reference to Scene component by node name and component type.
/// Useful for tests with serialization when actual objects are recreated.
template <class T>
struct ComponentRef
{
    WeakPtr<Scene> scene_;
    ea::string name_;

    T* GetComponent() const
    {
        Node* node = scene_ ? scene_->GetChild(name_, true) : nullptr;
        return node ? node->GetComponent<T>() : nullptr;
    }

    T* operator*() const { return GetComponent(); }
    T* operator->() const { return GetComponent(); }
    bool operator!() const { return GetComponent() == nullptr; }
    explicit operator bool() const { return !!*this; }
};

}
