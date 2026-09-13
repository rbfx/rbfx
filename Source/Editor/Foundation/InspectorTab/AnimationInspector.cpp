// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/AnimationInspector.h"

#include <Urho3D/SystemUI/AnimationInspectorWidget.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

void Foundation_AnimationInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<AnimationInspector>(inspectorTab->GetProject());
}

AnimationInspector::AnimationInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash AnimationInspector::GetResourceType() const
{
    return Animation::GetTypeStatic();
}


SharedPtr<ResourceInspectorWidget> AnimationInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<AnimationInspectorWidget>(context_, resources);
}

} // namespace Urho3D
