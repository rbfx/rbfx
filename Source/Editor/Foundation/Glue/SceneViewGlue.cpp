// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/Glue/SceneViewGlue.h"
#include "../../Foundation/SceneViewTab/SceneHierarchy.h"

namespace Urho3D
{

void Foundation_SceneViewGlue(Context* context, SceneViewTab* sceneViewTab)
{
    auto project = sceneViewTab->GetProject();
    const WeakPtr<HierarchyBrowserTab> hierarchyBrowserTab{project->FindTab<HierarchyBrowserTab>()};
    const WeakPtr<SceneHierarchy> sceneHierarchy{sceneViewTab->GetAddon<SceneHierarchy>()};

    if (hierarchyBrowserTab && sceneHierarchy)
    {
        sceneViewTab->OnFocused.Subscribe(sceneHierarchy.Get(),
            [hierarchyBrowserTab](SceneHierarchy* sceneHierarchy)
        {
            if (hierarchyBrowserTab)
                hierarchyBrowserTab->ConnectToSource(sceneHierarchy);
        });
    }
}

}
