// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/TextureCubeInspector.h"

#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/TextureCube.h>
#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/SystemUI/TextureCubeInspectorWidget.h>

namespace Urho3D
{

void Foundation_TextureCubeInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<TextureCubeInspector>(inspectorTab->GetProject());
}

TextureCubeInspector::TextureCubeInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash TextureCubeInspector::GetResourceType() const
{
    return TextureCube::GetTypeStatic();
}

SharedPtr<ResourceInspectorWidget> TextureCubeInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<TextureCubeInspectorWidget>(context_, resources);
}

SharedPtr<BaseWidget> TextureCubeInspector::MakePreviewWidget(Resource* resource)
{
    auto sceneWidget = MakeShared<SceneWidget>(context_);
    sceneWidget->CreateDefaultScene();
    sceneWidget->SetSkyboxTexture(static_cast<Texture*>(resource));
    return sceneWidget;
}
} // namespace Urho3D
