// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/ResourceBrowserTab.h"

namespace Urho3D
{

void Foundation_SceneFactory(Context* context, ResourceBrowserTab* resourceBrowserTab);

/// Camera controller used by Scene View.
class SceneFactory : public BaseResourceFactory
{
    URHO3D_OBJECT(SceneFactory, BaseResourceFactory);

public:
    SceneFactory(Context* context, bool isPrefab);

    /// Implement BaseResourceFactory.
    /// @{
    ea::string GetDefaultFileName() const override { return isPrefab_ ? "Prefab.prefab" : "Scene.scene"; }
    void RenderAuxilary() override;
    void CommitAndClose() override;
    /// @}

private:
    const bool isPrefab_{};
    bool highQuality_{true};
    bool defaultObjects_{true};
};

}
