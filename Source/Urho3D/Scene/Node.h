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

#include "../IO/VectorBuffer.h"
#include "../Math/Matrix3x4.h"
#include "../Math/Transform.h"
#include "../Scene/Component.h"
#include "../Scene/PrefabTypes.h"
#include "../Scene/Serializable.h"

#include <EASTL/type_traits.h>

#include <atomic>

namespace Urho3D
{

class Connection;
class Node;
class PrefabReader;
class PrefabWriter;
class Scene;
class NodePrefab;
class SceneResolver;
class SerializablePrefab;
class PrefabResource;

enum SceneLookupFlag
{
    None = 0x0,
    /// Whether to do recursive search in the scene subtree.
    Recursive = 0x1,
    /// Whether to ignore temporary nodes and components.
    //IgnoreTemporary = 0x2,
    /// Used for lazy node lookup. Whether to validate the existing node name.
    ValidateName = 0x4,
    /// Used for lazy node lookup. Whether to validate that the existing node is a child of the queried node.
    ValidateRelation = 0x8,
};
URHO3D_FLAGSET(SceneLookupFlag, SceneLookupFlags);

/// Transform space for translations and rotations.
enum TransformSpace
{
    TS_LOCAL = 0,
    TS_PARENT,
    TS_WORLD
};

/// Internal implementation structure for less performance-critical Node variables.
struct URHO3D_API NodeImpl
{
    /// Nodes this node depends on for network updates.
    ea::vector<Node*> dependencyNodes_;
    /// Name.
    ea::string name_;
    /// Tag strings.
    StringVector tags_;
    /// Name hash.
    StringHash nameHash_;
    /// Attribute buffer for network updates.
    mutable VectorBuffer attrBuffer_;
};

/// %Scene node that may contain components and child nodes.
class URHO3D_API Node : public Serializable
{
    URHO3D_OBJECT(Node, Serializable);

    friend class Connection;

public:
    /// Construct.
    explicit Node(Context* context);
    /// Destruct. Any child nodes are detached.
    ~Node() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// For given set of components, return all nodes they belong to.
    static ea::vector<Node*> GetNodes(const ea::vector<Component*>& components);
    /// For given set of nodes, exclude all children nodes and return all "root" nodes only.
    static ea::vector<Node*> GetParentNodes(const ea::vector<Node*>& nodes);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;
    void SerializeInBlock(Archive& archive, bool serializeTemporary, PrefabSaveFlags saveFlags,
        PrefabLoadFlags loadFlags = PrefabLoadFlag::None);

    /// Load from prefab without resolving IDs and applying attributes. May throw ArchiveException.
    void LoadInternal(const SerializablePrefab& nodePrefab, PrefabReader& reader, SceneResolver& resolver,
        PrefabLoadFlags flags = {});
    /// Load from prefab. Return true on success. Discard PrefabReader after calling this.
    bool Load(PrefabReader& reader, PrefabLoadFlags flags = {});
    /// Write to prefab. May throw ArchiveException.
    void SaveInternal(PrefabWriter& writer) const;
    /// Write to prefab. Return true on success. Discard PrefabWriter after calling this.
    bool Save(PrefabWriter& writer) const;

    /// Instantiate scene content from prefab. Return root node if successful.
    Node* InstantiatePrefab(const PrefabResource* prefabResource, const Vector3& position = Vector3::ZERO,
        const Quaternion& rotation = Quaternion::IDENTITY);

    /// Instantiate scene content from prefab. Return root node if successful.
    Node* InstantiatePrefab(const NodePrefab& prefab, const Vector3& position = Vector3::ZERO,
        const Quaternion& rotation = Quaternion::IDENTITY);
    /// Generate prefab from scene content.
    void GeneratePrefab(NodePrefab& prefab) const;
    NodePrefab GeneratePrefab() const;

    /// Evaluate effective attribute scope.
    /// It is a hint for the Editor to know what is affected by the node addition/removal
    /// so it can generate optimal undo/redo actions.
    AttributeScopeHint GetEffectiveScopeHint() const;

    /// Load from binary data. Return true if successful.
    bool Load(Deserializer& source) override;
    /// Load from XML data. Return true if successful.
    bool LoadXML(const XMLElement& source) override;
    /// Load from JSON data. Return true if successful.
    bool LoadJSON(const JSONValue& source) override;
    /// Save as binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Save as XML data. Return true if successful.
    bool SaveXML(XMLElement& dest) const override;
    /// Save as JSON data. Return true if successful.
    bool SaveJSON(JSONValue& dest) const override;
    /// Apply attribute changes that can not be applied immediately recursively to child nodes and components.
    void ApplyAttributes() override;

    /// Return whether should save default-valued attributes into XML. Always save node transforms for readability, even if identity.
    bool SaveDefaultAttributes(const AttributeInfo& attr) const override { return true; }

    /// Save to an XML file. Return true if successful.
    bool SaveXML(Serializer& dest, const ea::string& indentation = "\t") const;
    /// Save to a JSON file. Return true if successful.
    bool SaveJSON(Serializer& dest, const ea::string& indentation = "\t") const;
    /// Set name of the scene node. Names are not required to be unique.
    /// @property
    void SetName(const ea::string& name);

    /// Set tags. Old tags are overwritten.
    void SetTags(const StringVector& tags);
    /// Add a tag.
    void AddTag(const ea::string& tag);
    /// Add tags with the specified separator (; by default).
    void AddTags(const ea::string& tags, char separator = ';');
    /// Add tags.
    void AddTags(const StringVector& tags);
    /// Remove tag. Return true if existed.
    bool RemoveTag(const ea::string& tag);
    /// Remove all tags.
    void RemoveAllTags();

    /// Set position in parent space. If the scene node is on the root level (is child of the scene itself), this is same as world space.
    /// @property
    void SetPosition(const Vector3& position);

    /// Set position in parent space (for Urho2D).
    /// @property
    void SetPosition2D(const Vector2& position) {SetPosition(position.ToVector3()); }

    /// Set position in parent space (for Urho2D).
    void SetPosition2D(float x, float y) { SetPosition(Vector3(x, y, 0.0f)); }

    /// Set rotation in parent space.
    /// @property
    void SetRotation(const Quaternion& rotation);

    /// Set rotation in parent space (for Urho2D).
    /// @property
    void SetRotation2D(float rotation) { SetRotation(Quaternion(rotation)); }

    /// Set forward direction in parent space. Positive Z axis equals identity rotation.
    /// @property
    void SetDirection(const Vector3& direction);
    /// Set uniform scale in parent space.
    void SetScale(float scale);
    /// Set scale in parent space.
    /// @property
    void SetScale(const Vector3& scale);

    /// Set scale in parent space (for Urho2D).
    /// @property
    void SetScale2D(const Vector2& scale) {SetScale(scale.ToVector3(1.0f)); }

    /// Set scale in parent space (for Urho2D).
    void SetScale2D(float x, float y) { SetScale(Vector3(x, y, 1.0f)); }

    /// Set both position and rotation in parent space as an atomic operation. This is faster than setting position and rotation separately.
    void SetTransform(const Vector3& position, const Quaternion& rotation);
    /// Set position, rotation, and uniform scale in parent space as an atomic operation.
    void SetTransform(const Vector3& position, const Quaternion& rotation, float scale);
    /// Set position, rotation, and scale in parent space as an atomic operation.
    void SetTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);
    /// Set node transformation in parent space as an atomic operation.
    void SetTransformMatrix(const Matrix3x4& matrix);
    /// Set node transformation in parent space as an atomic operation.
    void SetTransform(const Transform& transform);

    /// Set both position and rotation in parent space as an atomic operation (for Urho2D).
    void SetTransform2D(const Vector2& position, float rotation)
    {
        SetTransform(position.ToVector3(), Quaternion(rotation));
    }

    /// Set position, rotation, and uniform scale in parent space as an atomic operation (for Urho2D).
    void SetTransform2D(const Vector2& position, float rotation, float scale)
    {
        SetTransform(position.ToVector3(), Quaternion(rotation), scale);
    }

    /// Set position, rotation, and scale in parent space as an atomic operation (for Urho2D).
    void SetTransform2D(const Vector2& position, float rotation, const Vector2& scale)
    {
        SetTransform(position.ToVector3(), Quaternion(rotation), scale.ToVector3(1.0f));
    }

    /// Set position in world space.
    /// @property
    void SetWorldPosition(const Vector3& position);

    /// Set position in world space (for Urho2D).
    /// @property
    void SetWorldPosition2D(const Vector2& position) {SetWorldPosition(position.ToVector3()); }

    /// Set position in world space (for Urho2D).
    void SetWorldPosition2D(float x, float y) { SetWorldPosition(Vector3(x, y, 0.0f)); }

    /// Set rotation in world space.
    /// @property
    void SetWorldRotation(const Quaternion& rotation);

    /// Set rotation in world space (for Urho2D).
    /// @property
    void SetWorldRotation2D(float rotation) { SetWorldRotation(Quaternion(rotation)); }

    /// Set forward direction in world space.
    /// @property
    void SetWorldDirection(const Vector3& direction);
    /// Set uniform scale in world space.
    void SetWorldScale(float scale);
    /// Set scale in world space.
    /// @property
    void SetWorldScale(const Vector3& scale);

    /// Set scale in world space (for Urho2D).
    /// @property
    void SetWorldScale2D(const Vector2& scale) {SetWorldScale(scale.ToVector3(1.0f)); }

    /// Set scale in world space (for Urho2D).
    void SetWorldScale2D(float x, float y) { SetWorldScale(Vector3(x, y, 1.0f)); }

    /// Set both position and rotation in world space as an atomic operation.
    void SetWorldTransform(const Vector3& position, const Quaternion& rotation);
    /// Set position, rotation, and uniform scale in world space as an atomic operation.
    void SetWorldTransform(const Vector3& position, const Quaternion& rotation, float scale);
    /// Set position, rotation, and scale in world space as an atomic opration.
    void SetWorldTransform(const Vector3& position, const Quaternion& rotation, const Vector3& scale);
    /// Set position, rotation, and scale in world space as an atomic operation from a transformation matrix.
    void SetWorldTransform(const Matrix3x4& worldTransform);

    /// Set both position and rotation in world space as an atomic operation (for Urho2D).
    void SetWorldTransform2D(const Vector2& position, float rotation)
    {
        SetWorldTransform(position.ToVector3(), Quaternion(rotation));
    }

    /// Set position, rotation, and uniform scale in world space as an atomic operation (for Urho2D).
    void SetWorldTransform2D(const Vector2& position, float rotation, float scale)
    {
        SetWorldTransform(position.ToVector3(), Quaternion(rotation), scale);
    }

    /// Set position, rotation, and scale in world space as an atomic opration (for Urho2D).
    void SetWorldTransform2D(const Vector2& position, float rotation, const Vector2& scale)
    {
        SetWorldTransform(position.ToVector3(), Quaternion(rotation), scale.ToVector3(1.0f));
    }

    /// Move the scene node in the chosen transform space.
    void Translate(const Vector3& delta, TransformSpace space = TS_LOCAL);

    /// Move the scene node in the chosen transform space (for Urho2D).
    void Translate2D(const Vector2& delta, TransformSpace space = TS_LOCAL) {Translate(delta.ToVector3(), space); }

    /// Rotate the scene node in the chosen transform space.
    void Rotate(const Quaternion& delta, TransformSpace space = TS_LOCAL);

    /// Rotate the scene node in the chosen transform space (for Urho2D).
    void Rotate2D(float delta, TransformSpace space = TS_LOCAL) { Rotate(Quaternion(delta), space); }

    /// Rotate around a point in the chosen transform space.
    void RotateAround(const Vector3& point, const Quaternion& delta, TransformSpace space = TS_LOCAL);

    /// Rotate around a point in the chosen transform space (for Urho2D).
    void RotateAround2D(const Vector2& point, float delta, TransformSpace space = TS_LOCAL)
    {
        RotateAround(point.ToVector3(), Quaternion(delta), space);
    }

    /// Rotate around the X axis.
    void Pitch(float angle, TransformSpace space = TS_LOCAL);
    /// Rotate around the Y axis.
    void Yaw(float angle, TransformSpace space = TS_LOCAL);
    /// Rotate around the Z axis.
    void Roll(float angle, TransformSpace space = TS_LOCAL);
    /// Look at a target position in the chosen transform space. Note that the up vector is always specified in world space. Return true if successful, or false if resulted in an illegal rotation, in which case the current rotation remains.
    bool LookAt(const Vector3& target, const Vector3& up = Vector3::UP, TransformSpace space = TS_WORLD);
    /// Modify scale in parent space uniformly.
    void Scale(float scale);
    /// Modify scale in parent space.
    void Scale(const Vector3& scale);

    /// Scale around a point in the chosen transform space.
    void ScaleAround(const Vector3& point, const Vector3& scale, TransformSpace space = TS_LOCAL);

    /// Modify scale in parent space (for Urho2D).
    void Scale2D(const Vector2& scale) {Scale(scale.ToVector3(1.0f)); }

    /// Set enabled/disabled state without recursion. Components in a disabled node become effectively disabled regardless of their own enable/disable state.
    /// @property
    void SetEnabled(bool enable);
    /// Set enabled state on self and child nodes. Nodes' own enabled state is remembered (IsEnabledSelf) and can be restored.
    void SetDeepEnabled(bool enable);
    /// Reset enabled state to the node's remembered state prior to calling SetDeepEnabled.
    void ResetDeepEnabled();
    /// Set enabled state on self and child nodes. Unlike SetDeepEnabled this does not remember the nodes' own enabled state, but overwrites it.
    void SetEnabledRecursive(bool enable);
    /// Mark node and child nodes to need world transform recalculation. Notify listener components.
    void MarkDirty();
    /// Create a child scene node (with specified ID if provided).
    Node* CreateChild(const ea::string& name = EMPTY_STRING, unsigned id = 0, bool temporary = false);
    /// Create a temporary child scene node (with specified ID if provided).
    Node* CreateTemporaryChild(const ea::string& name = EMPTY_STRING, unsigned id = 0);
    /// Add a child scene node at a specific index. If index is not explicitly specified or is greater than current children size, append the new child at the end.
    void AddChild(Node* node, unsigned index = M_MAX_UNSIGNED);
    /// Remove a child scene node.
    void RemoveChild(Node* node);
    /// Remove all child scene nodes.
    void RemoveAllChildren();
    /// Remove child scene nodes that match criteria.
    void RemoveChildren(bool recursive);
    /// Create a component to this node (with specified ID if provided).
    Component* CreateComponent(StringHash type, unsigned id = 0);
    /// Create a component to this node if it does not exist already.
    Component* GetOrCreateComponent(StringHash type, unsigned id = 0);
    /// Clone a component from another node using its create mode. Return the clone if successful or null on failure.
    Component* CloneComponent(Component* component, unsigned id = 0);
    /// Remove a component from this node.
    void RemoveComponent(Component* component);
    /// Remove the first component of specific type from this node.
    void RemoveComponent(StringHash type);
    /// Remove components that match criteria.
    void RemoveComponents();
    /// Remove all components of specific type.
    void RemoveComponents(StringHash type);
    /// Remove all components from this node.
    void RemoveAllComponents();
    /// Adjust index order of an existing child node in this node.
    void ReorderChild(Node* child, unsigned index);
    /// Adjust index order of an existing component in this node.
    void ReorderComponent(Component* component, unsigned index);
    /// Clone scene node, components and child nodes. Return the clone.
    Node* Clone(Node* parent = nullptr);
    /// Remove from the parent node. If no other shared pointer references exist, causes immediate deletion.
    void Remove();
    /// Assign to a new parent scene node. Retains the world transform.
    /// @property
    void SetParent(Node* parent);
    /// Set a user variable.
    void SetVar(const ea::string& key, const Variant& value);
    void SetVarByHash(StringHash hash, const Variant& value);
    /// Add listener component that is notified of node being dirtied. Can either be in the same node or another.
    void AddListener(Component* component);
    /// Remove listener component.
    void RemoveListener(Component* component);
    /// Template version of creating a component.
    template <class T> T* CreateComponent(unsigned id = 0);
    /// Template version of getting or creating a component.
    template <class T> T* GetOrCreateComponent(unsigned id = 0);
    /// Template version of removing a component.
    template <class T> void RemoveComponent();
    /// Template version of removing all components of specific type.
    template <class T> void RemoveComponents();

    /// Return ID.
    /// @property{get_id}
    unsigned GetID() const { return id_; }

    /// Return name.
    /// @property
    const ea::string& GetName() const { return impl_->name_; }

    /// Return name hash.
    StringHash GetNameHash() const { return impl_->nameHash_; }

    /// Return full node name for debugging. Unique for each node in the scene. Slow!
    ea::string GetFullNameDebug() const;

    /// Return all tags.
    /// @property
    const StringVector& GetTags() const { return impl_->tags_; }

    /// Return whether has a specific tag.
    bool HasTag(const ea::string& tag) const;

    /// Return parent scene node.
    /// @property
    Node* GetParent() const { return parent_; }

    /// Return scene.
    /// @property
    Scene* GetScene() const { return scene_; }

    /// Return whether is a direct or indirect child of specified node.
    bool IsChildOf(const Node* node) const;

    /// Return whether the node is effectively temporary, i.e. is temporary or is a child of temporary node.
    bool IsTemporaryEffective() const;

    /// Return direct child of this node which contains specified indirect child.
    Node* GetDirectChildFor(Node* indirectChild) const;

    /// Return whether the node is the root of the transform hierarchy. If true, local transform and world transform are the same.
    bool IsTransformHierarchyRoot() const;

    /// Return whether is enabled. Disables nodes effectively disable all their components.
    /// @property
    bool IsEnabled() const { return enabled_; }

    /// Return the node's last own enabled state. May be different than the value returned by IsEnabled when SetDeepEnabled has been used.
    /// @property
    bool IsEnabledSelf() const { return enabledPrev_; }

    /// Return position in parent space.
    /// @property
    const Vector3& GetPosition() const { return position_; }

    /// Return position in parent space (for Urho2D).
    /// @property
    Vector2 GetPosition2D() const { return Vector2(position_.x_, position_.y_); }

    /// Return rotation in parent space.
    /// @property
    const Quaternion& GetRotation() const { return rotation_; }

    /// Return rotation in parent space (for Urho2D).
    /// @property
    float GetRotation2D() const { return rotation_.RollAngle(); }

    /// Return forward direction in parent space. Positive Z axis equals identity rotation.
    /// @property
    Vector3 GetDirection() const { return rotation_ * Vector3::FORWARD; }

    /// Return up direction in parent space. Positive Y axis equals identity rotation.
    /// @property
    Vector3 GetUp() const { return rotation_ * Vector3::UP; }

    /// Return right direction in parent space. Positive X axis equals identity rotation.
    /// @property
    Vector3 GetRight() const { return rotation_ * Vector3::RIGHT; }

    /// Return scale in parent space.
    /// @property
    const Vector3& GetScale() const { return scale_; }

    /// Return scale in parent space (for Urho2D).
    /// @property
    Vector2 GetScale2D() const { return Vector2(scale_.x_, scale_.y_); }

    /// Return local-to-parent space transform matrix.
    /// @property
    Matrix3x4 GetTransformMatrix() const { return Matrix3x4(position_, rotation_, scale_); }

    /// Return local-to-parent space transform components.
    Transform GetTransform() const { return Transform{position_, rotation_, scale_}; }

    /// Return position in world space.
    /// @property
    Vector3 GetWorldPosition() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldTransform_.Translation();
    }

    /// Return position in world space (for Urho2D).
    /// @property
    Vector2 GetWorldPosition2D() const
    {
        Vector3 worldPosition = GetWorldPosition();
        return Vector2(worldPosition.x_, worldPosition.y_);
    }

    /// Return rotation in world space.
    /// @property
    Quaternion GetWorldRotation() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldRotation_;
    }

    /// Return rotation in world space (for Urho2D).
    /// @property
    float GetWorldRotation2D() const
    {
        return GetWorldRotation().RollAngle();
    }

    /// Return direction in world space.
    /// @property
    Vector3 GetWorldDirection() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldRotation_ * Vector3::FORWARD;
    }

    /// Return node's up vector in world space.
    /// @property
    Vector3 GetWorldUp() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldRotation_ * Vector3::UP;
    }

    /// Return node's right vector in world space.
    /// @property
    Vector3 GetWorldRight() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldRotation_ * Vector3::RIGHT;
    }

    /// Return scale in world space.
    /// @property
    Vector3 GetWorldScale() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldTransform_.Scale();
    }

    /// Return signed scale in world space. Utilized for Urho2D physics.
    /// @property
    Vector3 GetSignedWorldScale() const;

    /// Return scale in world space (for Urho2D).
    /// @property
    Vector2 GetWorldScale2D() const
    {
        Vector3 worldScale = GetWorldScale();
        return Vector2(worldScale.x_, worldScale.y_);
    }

    /// Return world space transform matrix.
    /// @property
    const Matrix3x4& GetWorldTransform() const
    {
        if (dirty_)
            UpdateWorldTransform();

        return worldTransform_;
    }

    /// Convert a local space position to world space.
    Vector3 LocalToWorld(const Vector3& position) const;
    /// Convert a local space position or rotation to world space.
    Vector3 LocalToWorld(const Vector4& vector) const;
    /// Convert a local space position or rotation to world space (for Urho2D).
    Vector2 LocalToWorld2D(const Vector2& vector) const;
    /// Convert a world space position to local space.
    Vector3 WorldToLocal(const Vector3& position) const;
    /// Convert a world space position or rotation to local space.
    Vector3 WorldToLocal(const Vector4& vector) const;
    /// Convert a world space position or rotation to local space (for Urho2D).
    Vector2 WorldToLocal2D(const Vector2& vector) const;

    /// Return whether transform has changed and world transform needs recalculation.
    bool IsDirty() const { return dirty_; }

    /// Return number of child scene nodes.
    unsigned GetNumChildren(bool recursive = false) const;

    /// Return immediate child scene nodes.
    const ea::vector<SharedPtr<Node> >& GetChildren() const { return children_; }

    /// Return child scene nodes, optionally recursive.
    void GetChildren(ea::vector<Node*>& dest, bool recursive = false) const;
    /// Return child scene nodes, optionally recursive.
    ea::vector<Node*> GetChildren(bool recursive) const;
    /// Return child scene nodes with a specific component.
    void GetChildrenWithComponent(ea::vector<Node*>& dest, StringHash type, bool recursive = false) const;
    /// Return child scene nodes with a specific component.
    ea::vector<Node*> GetChildrenWithComponent(StringHash type, bool recursive = false) const;
    /// Return child scene nodes with a specific tag.
    void GetChildrenWithTag(ea::vector<Node*>& dest, const ea::string& tag, bool recursive = false) const;
    /// Return child scene nodes with a specific tag.
    ea::vector<Node*> GetChildrenWithTag(const ea::string& tag, bool recursive = false) const;

    /// Return index of direct child or M_MAX_UNSIGNED if not found.
    unsigned GetChildIndex(const Node* child) const;
    /// Return index of this node in the parent.
    unsigned GetIndexInParent() const;
    /// Return child scene node by index.
    Node* GetChild(unsigned index) const;
    /// Return child scene node by name hash.
    Node* GetChild(StringHash nameHash, bool recursive = false) const;
    /// Find child node by path string in format "Parent Name/Child Name/Grandchild Name/..."
    /// Node index may be used instead of name: ".../#10/..."
    /// Node name '**' may be used to match any path. For example, "Parent Name/**/Target Name" will match either of:
    /// - "Parent Name/Target Name"
    /// - "Parent Name/Child Name/Target Name"
    /// - "Parent Name/Child Name/Grandchild Name/Target Name"
    /// - etc.
    Node* FindChild(ea::string_view path, bool firstRecursive = false) const;
    /// Find attribute of itself or owned component by path string in format "@ComponentName/Attribute Name".
    /// If component name is not specified, attribute is searched in the node itself: "@/Position".
    ea::pair<Serializable*, unsigned> FindComponentAttribute(ea::string_view path) const;

    /// Return number of components.
    /// @property
    unsigned GetNumComponents() const { return components_.size(); }

    /// Return all components.
    const ea::vector<SharedPtr<Component> >& GetComponents() const { return components_; }

    /// Return all components of type. Optionally recursive.
    void GetComponents(ea::vector<Component*>& dest, StringHash type, bool recursive = false) const;
    /// Return index of owned component or M_MAX_UNSIGNED if not found.
    unsigned GetComponentIndex(const Component* component) const;
    /// Return component by type. If there are several, returns the first.
    Component* GetComponent(StringHash type, bool recursive = false) const;
    /// Return N-th component of given type.
    Component* GetNthComponent(StringHash type, unsigned index) const;
    /// Return component in parent node. If there are several, returns the first. May optional traverse up to the root node.
    Component* GetParentComponent(StringHash type, bool fullTraversal = false) const;
    /// Return whether has a specific component.
    bool HasComponent(StringHash type) const;
    /// Return listener components.
    const ea::vector<WeakPtr<Component> > GetListeners() const { return listeners_; }

    /// Return a user variable.
    const Variant& GetVar(const ea::string& key) const;
    const Variant& GetVarByHash(StringHash key) const;

    /// Return all user variables.
    const StringVariantMap& GetVars() const { return vars_; }

    /// Return all components derived from class. Optionally recursive.
    void GetDerivedComponents(ea::vector<Component*>& dest, StringHash type, bool recursive = false) const;
    /// Return first component derived from class.
    Component* GetDerivedComponent(StringHash type, bool recursive = false) const;
    /// Return first component derived from class.
    Component* GetParentDerivedComponent(StringHash type, bool fullTraversal = false) const;
    /// Return first component derived from class.
    template <class T> T* GetDerivedComponent(bool recursive = false) const;
    /// Return first component derived from class in the parent node, or if fully traversing then the first node up the tree with one.
    template <class T> T* GetParentDerivedComponent(bool fullTraversal = false) const;
    /// Return components derived from class.
    template <class T, class U> void GetDerivedComponents(U& destVector, bool recursive = false, bool clearVector = true) const;
    template <class U> void GetDerivedComponents(U& destVector, bool recursive = false, bool clearVector = true) const;
    /// Template version of returning child nodes with a specific component.
    template <class T> void GetChildrenWithComponent(ea::vector<Node*>& dest, bool recursive = false) const;
    /// Template version of returning a component by type.
    template <class T> T* GetComponent(bool recursive = false) const;
    /// Return N-th component of given type.
    template <class T> T* GetNthComponent(unsigned index) const;
    /// Template version of returning a parent's component by type.
    template <class T> T* GetParentComponent(bool fullTraversal = false) const;
    /// Template version of returning all components of type.
    template <class T, class U> void GetComponents(U& destVector, bool recursive = false, bool clearVector = true) const;
    template <class U> void GetComponents(U& destVector, bool recursive = false, bool clearVector = true) const;
    /// Template version of checking whether has a specific component.
    template <class T> bool HasComponent() const;

    /// Find and return child node inplace if pointer is null, do nothing if pointer is already initialized.
    /// Return true if child node is found or is already initialized.
    /// This function is optimized for the case when the child node is expected to be found.
    bool GetChildLazy(
        WeakPtr<Node>& childNode, StringHash nameHash, SceneLookupFlags flags = SceneLookupFlag::None) const;
    /// Find and return component inplace if pointer is null, do nothing if pointer is already initialized.
    /// Return true if component is found or is already initialized.
    /// This function is optimized for the case when the component is expected to be found.
    template <class T> bool GetNthComponentLazy(WeakPtr<T>& childComponent, unsigned index = 0) const;

    /// Traverse all components and child nodes recursively depth-first.
    /// Return `false` from `nodeCallback` to prevent traversal of the node.
    template <class T, class U> void TraverseDepthFirst(const T& nodeCallback, const U& componentCallback)
    {
        for (const auto& component : components_)
            componentCallback(component);

        for (const auto& child : children_)
        {
            if (nodeCallback(child))
                child->TraverseDepthFirst(nodeCallback, componentCallback);
        }
    }

    /// Set ID. Called by Scene.
    /// @property{set_id}
    void SetID(unsigned id);
    /// Set scene. Called by Scene.
    void SetScene(Scene* scene);
    /// Reset scene, ID and owner. Called by Scene.
    void ResetScene();
    /// Load components and optionally load child nodes.
    bool Load(Deserializer& source, SceneResolver& resolver, bool loadChildren = true, bool rewriteIDs = false);
    /// Load components from XML data and optionally load child nodes.
    bool LoadXML(const XMLElement& source, SceneResolver& resolver, bool loadChildren = true, bool rewriteIDs = false,
        bool removeComponents = true);
    /// Load components from XML data and optionally load child nodes.
    bool LoadJSON(const JSONValue& source, SceneResolver& resolver, bool loadChildren = true, bool rewriteIDs = false);
    /// Return the depended on nodes to order network updates.
    const ea::vector<Node*>& GetDependencyNodes() const { return impl_->dependencyNodes_; }

    /// Create a child node with specific ID.
    Node* CreateChild(unsigned id, bool temporary = false);
    /// Add a pre-created component. Using this function from application code is discouraged, as component operation without an owner node may not be well-defined in all cases. Prefer CreateComponent() instead.
    void AddComponent(Component* component, unsigned id);
    /// Calculate number of non-temporary child nodes.
    unsigned GetNumPersistentChildren() const;
    /// Calculate number of non-temporary components.
    unsigned GetNumPersistentComponents() const;

    /// Set position in parent space silently without marking the node & child nodes dirty. Used by animation code.
    void SetPositionSilent(const Vector3& position) { position_ = position; }

    /// Set position in parent space silently without marking the node & child nodes dirty. Used by animation code.
    void SetRotationSilent(const Quaternion& rotation) { rotation_ = rotation; }

    /// Set scale in parent space silently without marking the node & child nodes dirty. Used by animation code.
    void SetScaleSilent(const Vector3& scale) { scale_ = scale; }

    /// Set local transform silently without marking the node & child nodes dirty. Used by animation code.
    void SetTransformSilent(const Vector3& position, const Quaternion& rotation, const Vector3& scale);

    /// Set local transform silently without marking the node & child nodes dirty. Used by animation code.
    void SetTransformMatrixSilent(const Matrix3x4& matrix);

private:
    /// Set enabled/disabled state with optional recursion. Optionally affect the remembered enable state.
    void SetEnabled(bool enable, bool recursive, bool storeSelf);
    /// Create component, allowing UnknownComponent if actual type is not supported. Leave typeName empty if not known.
    Component* SafeCreateComponent(const ea::string& typeName, StringHash type, unsigned id);
    /// Recalculate the world transform.
    void UpdateWorldTransform() const;
    /// Remove child node by iterator.
    void RemoveChild(ea::vector<SharedPtr<Node> >::iterator i);
    /// Return child nodes recursively.
    void GetChildrenRecursive(ea::vector<Node*>& dest) const;
    /// Return child nodes with a specific component recursively.
    void GetChildrenWithComponentRecursive(ea::vector<Node*>& dest, StringHash type) const;
    /// Return child nodes with a specific tag recursively.
    void GetChildrenWithTagRecursive(ea::vector<Node*>& dest, const ea::string& tag) const;
    /// Return specific components recursively.
    void GetComponentsRecursive(ea::vector<Component*>& dest, StringHash type) const;
    /// Return components derived from type recursively.
    void GetDerivedComponentsRecursive(ea::vector<Component*>& dest, StringHash type) const;
    /// Clone node recursively.
    Node* CloneRecursive(Node* parent, SceneResolver& resolver);
    /// Remove a component from this node with the specified iterator.
    void RemoveComponent(ea::vector<SharedPtr<Component> >::iterator i);
    /// Find child node by index if name is an integer starting with "#" (like "#12" or "#0").
    /// Find child by name otherwise (optionally recursive). Empty name is considered invalid.
    Node* GetChildByNameOrIndex(ea::string_view name, bool recursive = false) const;
    /// Find component by name. If name is empty, returns the owner node itself.
    Serializable* GetSerializableByName(ea::string_view name) const;

    /// World-space transform matrix.
    mutable Matrix3x4 worldTransform_;
    /// World transform needs update flag.
    mutable std::atomic_bool dirty_;
    /// Enabled flag.
    bool enabled_;
    /// Last SetEnabled flag before any SetDeepEnabled.
    bool enabledPrev_;

private:
    /// Parent scene node.
    Node* parent_;
    /// Scene (root node).
    Scene* scene_;
    /// Unique ID within the scene.
    unsigned id_;
    /// Position.
    Vector3 position_;
    /// Rotation.
    Quaternion rotation_;
    /// Scale.
    Vector3 scale_;
    /// World-space rotation.
    mutable Quaternion worldRotation_;
    /// Components.
    ea::vector<SharedPtr<Component> > components_;
    /// Child scene nodes.
    ea::vector<SharedPtr<Node> > children_;
    /// Node listeners.
    ea::vector<WeakPtr<Component> > listeners_;
    /// Pointer to implementation.
    ea::unique_ptr<NodeImpl> impl_;

protected:
    /// User variables.
    StringVariantMap vars_;
};

template <class T> T* Node::CreateComponent(unsigned id)
{
    return static_cast<T*>(CreateComponent(T::GetTypeStatic(), id));
}

template <class T> T* Node::GetOrCreateComponent(unsigned id)
{
    return static_cast<T*>(GetOrCreateComponent(T::GetTypeStatic(), id));
}

template <class T> void Node::RemoveComponent() { RemoveComponent(T::GetTypeStatic()); }

template <class T> void Node::RemoveComponents() { RemoveComponents(T::GetTypeStatic()); }

template <class T> void Node::GetChildrenWithComponent(ea::vector<Node*>& dest, bool recursive) const
{
    GetChildrenWithComponent(dest, T::GetTypeStatic(), recursive);
}

template <class T> T* Node::GetComponent(bool recursive) const { return static_cast<T*>(GetComponent(T::GetTypeStatic(), recursive)); }

template <class T> T* Node::GetNthComponent(unsigned index) const { return static_cast<T*>(GetNthComponent(T::GetTypeStatic(), index)); }

template <class T> T* Node::GetParentComponent(bool fullTraversal) const { return static_cast<T*>(GetParentComponent(T::GetTypeStatic(), fullTraversal)); }

template <class T, class U> void Node::GetComponents(U& destVector, bool recursive, bool clearVector) const
{
    using PointerType = ea::remove_reference_t<decltype(destVector[0])>;

    if (clearVector)
        destVector.clear();

    for (const auto& component : components_)
    {
        if constexpr (ea::is_same_v<T, Component>)
            destVector.push_back(PointerType{component.Get()});
        else if (component->GetType() == T::GetTypeStatic())
            destVector.push_back(PointerType{static_cast<T*>(component.Get())});
    }

    if (recursive)
    {
        for (const auto& child : children_)
            child->GetComponents<T>(destVector, true, false);
    }
}

template <class U> void Node::GetComponents(U& destVector, bool recursive, bool clearVector) const
{
    using ComponentType = ea::remove_reference_t<decltype(*destVector[0])>;
    GetComponents<ComponentType>(destVector, recursive, clearVector);
}

template <class T> bool Node::HasComponent() const { return HasComponent(T::GetTypeStatic()); }

template <class T> T* Node::GetDerivedComponent(bool recursive) const
{
    for (auto i = components_.begin(); i != components_.end(); ++i)
    {
        auto* component = dynamic_cast<T*>(i->Get());
        if (component)
            return component;
    }

    if (recursive)
    {
        for (auto i = children_.begin(); i != children_.end(); ++i)
        {
            T* component = (*i)->GetDerivedComponent<T>(true);
            if (component)
                return component;
        }
    }

    return nullptr;
}

template <class T> T* Node::GetParentDerivedComponent(bool fullTraversal) const
{
    Node* current = GetParent();
    while (current)
    {
        T* soughtComponent = current->GetDerivedComponent<T>();
        if (soughtComponent)
            return soughtComponent;

        if (fullTraversal)
            current = current->GetParent();
        else
            break;
    }
    return 0;
}

template <class T, class U> void Node::GetDerivedComponents(U& destVector, bool recursive, bool clearVector) const
{
    using PointerType = ea::remove_reference_t<decltype(destVector[0])>;

    if (clearVector)
        destVector.clear();

    for (const auto& component : components_)
    {
        if (auto derivedComponent = dynamic_cast<T*>(component.Get()))
            destVector.push_back(PointerType{derivedComponent});
    }

    if (recursive)
    {
        for (const auto& child : children_)
            child->GetDerivedComponents<T>(destVector, true, false);
    }
}

template <class U> void Node::GetDerivedComponents(U& destVector, bool recursive, bool clearVector) const
{
    using ComponentType = ea::remove_reference_t<decltype(*destVector[0])>;
    GetDerivedComponents<ComponentType>(destVector, recursive, clearVector);
}

template <class T> bool Node::GetNthComponentLazy(WeakPtr<T>& childComponent, unsigned index) const
{
    // Try to use existing weak pointer
    if (childComponent)
        return true;

    // Try to find and cache the component.
    if (auto component = GetNthComponent<T>(index))
    {
        childComponent = component;
        return true;
    }

    return false;
}

}
