// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/InspectorTab.h"
#include "../../Foundation/Shared/InspectorWithPreview.h"

namespace Urho3D
{

void Foundation_AnimationInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class AnimationInspector
    : public InspectorWithPreview
{
    URHO3D_OBJECT(AnimationInspector, InspectorWithPreview)

public:
    explicit AnimationInspector(Project* project);

protected:
    StringHash GetResourceType() const override;
    SharedPtr<ResourceInspectorWidget> MakeInspectorWidget(const ResourceVector& resources) override;

};

} // namespace Urho3D
