// Copyright (c) 2017-2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Scene/PrefabResource.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;

namespace Tests
{

/// Serialize and deserialize Scene. Should preserve functional state of nodes and components.
void SerializeAndDeserializeScene(Scene* scene);

/// Convert Node to prefab.
SharedPtr<PrefabResource> ConvertNodeToPrefab(Node* node);

/// Return attribute value as variant.
Variant GetAttributeValue(const ea::pair<Serializable*, unsigned>& ref);

/// Compare aspects of the scene.
bool CompareAttributeValues(const Variant& lhs, const Variant& rhs);
bool CompareSerializables(const Serializable& lhs, const Serializable& rhs);
bool CompareNodes(const Node& lhs, const Node& rhs);

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
