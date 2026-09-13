// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/ResourceBrowserTab/SceneFactory.h"

#include "../../Project/CreateDefaultScene.h"

namespace Urho3D
{

void Foundation_SceneFactory(Context* context, ResourceBrowserTab* resourceBrowserTab)
{
    resourceBrowserTab->AddFactory(MakeShared<SceneFactory>(context, true));
    resourceBrowserTab->AddFactory(MakeShared<SceneFactory>(context, false));
}

SceneFactory::SceneFactory(Context* context, bool isPrefab)
    : BaseResourceFactory(context, 0, isPrefab ? "Prefab" : "Scene")
    , isPrefab_(isPrefab)
{
}

void SceneFactory::RenderAuxilary()
{
    if (!isPrefab_)
    {
        ui::Separator();

        ui::Checkbox("High Quality", &highQuality_);
        if (ui::IsItemHovered())
            ui::SetTooltip("Use renderer settings for high picture quality");

        ui::Checkbox("Default Objects", &defaultObjects_);
        if (ui::IsItemHovered())
            ui::SetTooltip("Add default light, environment and teapot to the scene.");
    }

    ui::Separator();
}

void SceneFactory::CommitAndClose()
{
    DefaultSceneParameters params;
    params.highQuality_ = highQuality_;
    params.createObjects_ = defaultObjects_;
    params.isPrefab_ = isPrefab_;

    CreateDefaultScene(context_, GetFinalFileName(), params);
}

}
