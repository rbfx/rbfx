// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/Texture2DInspector.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Texture2DWidget.h>
#include <Urho3D/SystemUI/Texture2DInspectorWidget.h>

namespace Urho3D
{

void Foundation_Texture2DInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<Texture2DInspector>(inspectorTab->GetProject());
}

Texture2DInspector::Texture2DInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash Texture2DInspector::GetResourceType() const
{
    return Texture2D::GetTypeStatic();
}

SharedPtr<BaseWidget> Texture2DInspector::MakePreviewWidget(Resource* resource)
{
    return MakeShared<Texture2DWidget>(context_, static_cast<Texture2D*>(resource));
}

SharedPtr<ResourceInspectorWidget> Texture2DInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<Texture2DInspectorWidget>(context_, resources);
}

} // namespace Urho3D
