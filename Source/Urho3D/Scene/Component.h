//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// \file

#pragma once

#include "../Scene/Serializable.h"

namespace Urho3D
{

class DebugRenderer;
class Node;
class Scene;

/// Autoremove is used by some components for automatic removal from the scene hierarchy upon completion of an action, for example sound or particle effect.
enum AutoRemoveMode
{
    REMOVE_DISABLED = 0,
    REMOVE_COMPONENT,
    REMOVE_NODE
};

/// Base class for components. Components can be created to scene nodes.
/// @templateversion
class URHO3D_API Component : public Serializable
{
    URHO3D_OBJECT(Component, Serializable);

    friend class Node;
    friend class Scene;

public:
    /// Construct.
    explicit Component(Context* context);
    /// Destruct.
    ~Component() override;

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled() { }

    /// Evaluate effective attribute scope.
    /// It is a hint for the Editor to know what is affected by the component addition/removal
    /// so it can generate optimal undo/redo actions.
    AttributeScopeHint GetEffectiveScopeHint() const;

    /// Save as binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const override;
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const override;
    /// Return the depended on nodes to order network updates.
    virtual void GetDependencyNodes(ea::vector<Node*>& dest);
    /// Visualize the component as debug geometry.
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    /// Return whether the component provides auxiliary data.
    /// Only components directly attached to the Scene can have auxiliary data.
    /// Auxiliary data is not supported in prefabs.
    virtual bool HasAuxiliaryData() const { return false; }
    /// Serialize auxiliary data from/to the current block of the archive. May throw ArchiveException.
    virtual void SerializeAuxiliaryData(Archive& archive) {}

    /// Set enabled/disabled state.
    /// @property
    void SetEnabled(bool enable);
    /// Remove from the scene node. If no other shared pointer references exist, causes immediate deletion.
    void Remove();

    /// Return ID.
    /// @property{get_id}
    unsigned GetID() const { return id_; }

    /// Return scene node.
    /// @property
    Node* GetNode() const { return node_; }

    /// Return the scene the node belongs to.
    Scene* GetScene() const;

    /// Return whether is enabled.
    /// @property
    bool IsEnabled() const { return enabled_; }
    /// Return whether is effectively enabled (node is also enabled).
    /// @property
    bool IsEnabledEffective() const;

    /// Return full component name for debugging. Unique for each component in the scene. Slow!
    ea::string GetFullNameDebug() const;

    /// Return component in the same scene node by type. If there are several, returns the first.
    Component* GetComponent(StringHash type) const;
    /// Template version of returning a component in the same scene node by type.
    template <class T> T* GetComponent() const;
    /// Return index of this component in the node.
    unsigned GetIndexInParent() const;

protected:
    /// Handle scene node being assigned at creation.
    virtual void OnNodeSet(Node* previousNode, Node* currentNode);
    /// Handle scene being assigned. This may happen several times during the component's lifetime. Scene-wide subsystems and events are subscribed to here.
    virtual void OnSceneSet(Scene* scene);
    /// Handle scene node transform dirtied.
    virtual void OnMarkedDirty(Node* node);
    /// Handle scene node enabled status changing.
    virtual void OnNodeSetEnabled(Node* node);
    /// Set ID. Called by Scene.
    /// @property{set_id}
    void SetID(unsigned id);
    /// Set scene node. Called by Node when creating the component.
    void SetNode(Node* node);
    /// Return a component from the scene root that sends out fixed update events (either PhysicsWorld or PhysicsWorld2D). Return null if neither exists.
    Component* GetFixedUpdateSource();
    /// Perform autoremove. Called by subclasses. Caller should keep a weak pointer to itself to check whether was actually removed, and return immediately without further member operations in that case.
    void DoAutoRemove(AutoRemoveMode mode);

    /// Scene node.
    Node* node_;
    /// Unique ID within the scene.
    unsigned id_;
    /// Network update queued flag.
    bool networkUpdate_;
    /// Enabled flag.
    bool enabled_;
};

template <class T> T* Component::GetComponent() const { return static_cast<T*>(GetComponent(T::GetTypeStatic())); }

}
