// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/SceneViewTab.h"
#include "../../Project/ResourceFactory.h"

#include <Urho3D/Scene/PrefabResource.h>

namespace Urho3D
{

void Foundation_CreatePrefabFromNode(Context* context, SceneViewTab* sceneViewTab);

/// Intermediate class for request processing.
class PrefabFromNodeFactory : public BaseResourceFactory
{
    URHO3D_OBJECT(PrefabFromNodeFactory, BaseResourceFactory);

public:
    using WeakNodeVector = ea::vector<WeakPtr<Node>>;

    explicit PrefabFromNodeFactory(Context* context);
    void Setup(SceneViewTab* tab, const WeakNodeVector& nodes);

    /// Implement ResourceFactory.
    /// @{
    ea::string GetDefaultFileName() const override;
    bool IsFileNameEditable() const override;
    void Render(const FileNameChecker& checker, bool& canCommit, bool& shouldCommit) override;
    void RenderAuxilary() override;
    void CommitAndClose() override;
    /// @}

private:
    NodePrefab CreatePrefabBase() const;
    NodePrefab CreatePrefabFromNode(Node* node) const;

    ea::string FindBestFileName(Node* node, const ea::string& filePath) const;
    void SaveNodeAsPrefab(Node* node, const ea::string& resourceName, const ea::string& fileName);

    WeakPtr<SceneViewTab> tab_;
    WeakNodeVector nodes_;
    SharedPtr<PrefabResource> prefab_;
    bool replaceWithReference_{true};
};

/// Addon to manage scene selection with mouse and render debug geometry.
class CreatePrefabFromNode : public SceneViewAddon
{
    URHO3D_OBJECT(CreatePrefabFromNode, SceneViewAddon);

public:
    explicit CreatePrefabFromNode(SceneViewTab* owner);

    /// Implement SceneViewAddon.
    /// @{
    ea::string GetUniqueName() const override { return "CreatePrefabFrom"; }
    /// @}

private:
    void RenderMenu(SceneViewPage& page, Scene* scene, SceneSelection& selection);
    void CreatePrefabs(const SceneSelection& selection);

    SharedPtr<PrefabFromNodeFactory> factory_;
};

}
