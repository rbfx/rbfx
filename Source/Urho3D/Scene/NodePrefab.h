// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Scene/PrefabTypes.h>
#include <Urho3D/Scene/Serializable.h>

#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Attribute prefab.
/// Contains representation of attribute with value, type information and identifier.
/// At least one of the following identifiers should be present:
/// - Zero-based attribute ID optimized for variable-length encoding. Zero is invalid value.
/// - Full attribute name. May be empty if loaded from compacted binary archive.
/// - Attribute name hash. May be present even if attribute name is empty.
class URHO3D_API AttributePrefab
{
public:
    enum class IdentifierType : unsigned char
    {
        Id,
        Name,
        NameHash,
        Unused,
        // Don't add more types!
    };

    AttributePrefab() = default;
    explicit AttributePrefab(AttributeId id);
    explicit AttributePrefab(const ea::string& name);
    explicit AttributePrefab(const char* name);
    explicit AttributePrefab(StringHash nameHash);

    void SetValue(const Variant& value) { value_ = value; }
    void SetValue(Variant&& value) { value_ = ea::move(value); }

    void SerializeInBlock(Archive& archive, bool compactSave = false);

    IdentifierType GetIdentifierType() const;

    AttributeId GetId() const { return id_; }
    const ea::string& GetName() const { return name_; }
    StringHash GetNameHash() const { return nameHash_; }
    VariantType GetType() const { return value_.GetType(); }
    const Variant& GetValue() const { return value_; }

    bool operator==(const AttributePrefab& rhs) const;
    bool operator!=(const AttributePrefab& rhs) const { return !(*this == rhs); }

private:
    AttributeId id_{};
    ea::string name_;
    StringHash nameHash_;

    Variant value_{};
};

URHO3D_API void SerializeValue(Archive& archive, const char* name, AttributePrefab& value, bool compactSave = false);

/// Serializable prefab.
/// Contains a list of attributes.
class URHO3D_API SerializablePrefab
{
public:
    SerializablePrefab() = default;

    void SetType(const ea::string& typeName);
    void SetType(const char* typeName);
    void SetType(StringHash typeNameHash);
    void SetId(SerializableId id) { id_ = id; }

    void Import(const Serializable* serializable, PrefabSaveFlags flags = {});
    void Export(Serializable* serializable, PrefabLoadFlags flags = {}) const;

    void SerializeInBlock(Archive& archive, PrefabArchiveFlags flags = {}, bool compactSave = false);

    AttributeScopeHint GetEffectiveScopeHint(Context* context) const;

    const ea::string& GetTypeName() const { return typeName_; }
    StringHash GetTypeNameHash() const { return typeNameHash_; }
    SerializableId GetId() const { return id_; }
    const ea::vector<AttributePrefab>& GetAttributes() const { return attributes_; }
    ea::vector<AttributePrefab>& GetMutableAttributes() { return attributes_; }

    bool operator==(const SerializablePrefab& rhs) const;
    bool operator!=(const SerializablePrefab& rhs) const { return !(*this == rhs); }

private:
    ea::string typeName_;
    StringHash typeNameHash_;

    SerializableId id_{};
    bool temporary_{};

    ea::vector<AttributePrefab> attributes_;
};

URHO3D_API void SerializeValue(Archive& archive, const char* name, SerializablePrefab& value,
    PrefabArchiveFlags flags = {}, bool compactSave = false);

/// Scene prefab.
/// Contains node attributes, components and child nodes.
class URHO3D_API NodePrefab
{
public:
    static const NodePrefab Empty;

    void SerializeInBlock(Archive& archive, PrefabArchiveFlags flags = {}, bool compactSave = false);

    AttributeScopeHint GetEffectiveScopeHint(Context* context) const;

    void NormalizeIds(Context* context);

    const SerializablePrefab& GetNode() const { return node_; }
    SerializablePrefab& GetMutableNode() { return node_; }
    const ea::vector<SerializablePrefab>& GetComponents() const { return components_; }
    ea::vector<SerializablePrefab>& GetMutableComponents() { return components_; }
    const ea::vector<NodePrefab>& GetChildren() const { return children_; }
    ea::vector<NodePrefab>& GetMutableChildren() { return children_; }

    const ea::string& GetNodeName() const;
    const NodePrefab& FindChild(ea::string_view path) const;

    void Clear();
    bool IsEmpty() const;

    bool operator==(const NodePrefab& rhs) const;
    bool operator!=(const NodePrefab& rhs) const { return !(*this == rhs); }

private:
    SerializablePrefab node_;
    ea::vector<SerializablePrefab> components_;
    ea::vector<NodePrefab> children_;
};

URHO3D_API void SerializeValue(
    Archive& archive, const char* name, NodePrefab& value, PrefabArchiveFlags flags = {}, bool compactSave = false);

/// Utility class to remap and resolve prefab IDs. Similar to SceneResolver.
class PrefabNormalizer
{
public:
    explicit PrefabNormalizer(Context* context);

    void ScanNode(NodePrefab& node);
    void RemapAndPrune(NodePrefab& node);

private:
    void ScanSerializable(SerializablePrefab& prefab);
    void ScanAttribute(AttributePrefab& attributePrefab, const AttributeInfo& attr);

    void RemapReferencedIds();
    void PatchAttributes();
    void PruneUnreferencedIds(NodePrefab& node);
    void PruneUnreferencedId(SerializablePrefab& prefab, bool isNode);

    Context* context_{};

    ea::vector<AttributePrefab*> nodeIdAttributes_;
    ea::vector<AttributePrefab*> componentIdAttributes_;
    ea::vector<SerializableId> referencedNodeIds_;
    ea::vector<SerializableId> referencedComponentIds_;
    ea::unordered_map<SerializableId, SerializableId> nodeIdRemap_;
    ea::unordered_map<SerializableId, SerializableId> componentIdRemap_;
};

} // namespace Urho3D
