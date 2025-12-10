// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Scene/Component.h"

#include <EASTL/unordered_set.h>

namespace Urho3D
{

class RmlUI;
class RmlUIComponent;

/// Manager that keeps track of all RmlUIComponent instances in a scene.
class URHO3D_API RmlUIManager : public Component
{
    URHO3D_OBJECT(RmlUIManager, Component);

public:
    explicit RmlUIManager(Context* context);
    ~RmlUIManager() override;

    static void RegisterObject(Context* context);

    /// Enable or disable documents.
    void SetDocumentsEnabled(bool enabled);
    /// Update RmlUI instance that owns all the documents in this scene.
    void SetOwner(RmlUI* rmlUi);

    /// Return current owner instance.
    RmlUI* GetOwner() { return rmlUi_; }
    /// Return all tracked documents in the Scene.
    const ea::unordered_set<WeakPtr<RmlUIComponent>>& GetDocuments() const { return documents_; }

    /// Internal. Manage documents.
    /// @{
    void AddDocument(RmlUIComponent* component);
    void RemoveDocument(RmlUIComponent* component);
    /// @}

protected:
    void OnSceneSet(Scene* previousScene, Scene* scene) override;

private:
    WeakPtr<RmlUI> rmlUi_;
    ea::unordered_set<WeakPtr<RmlUIComponent>> documents_;
};

}
