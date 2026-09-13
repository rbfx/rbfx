// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Resource/Resource.h"
#include "Urho3D/Scene/NodePrefab.h"

namespace Urho3D
{

class Node;

/// Prefab resource.
/// Constains representation of nodes and components with attributes, ready to be instantiated.
class URHO3D_API PrefabResource : public SimpleResource
{
    URHO3D_OBJECT(PrefabResource, SimpleResource)

public:
    explicit PrefabResource(Context* context);
    ~PrefabResource() override;

    static void RegisterObject(Context* context);

    /// Instantiate prefab into a scene or node as PrefabReference.
    /// If inplace, the prefab will be instantiated into the parent node directly, otherwise a new node will be created.
    Node* InstantiateReference(Node* parentNode, bool inplace = false);

    void NormalizeIds();

    void SerializeInBlock(Archive& archive) override;

    const NodePrefab& GetScenePrefab() const { return prefab_; }
    NodePrefab& GetMutableScenePrefab() { return prefab_; }

    const NodePrefab& GetNodePrefab() const;
    NodePrefab& GetMutableNodePrefab();

    const NodePrefab& GetNodePrefabSlice(ea::string_view path) const;

     /// Implement Resource.
    /// @{
    bool BeginLoad(Deserializer& source) override;
    /// @}

private:
    void BackgroundLoadResources(const NodePrefab& prefab);

    bool LoadLegacyXML(const XMLElement& source) override;

    NodePrefab prefab_;
};

} // namespace Urho3D
