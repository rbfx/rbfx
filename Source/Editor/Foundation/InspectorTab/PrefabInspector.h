// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/InspectorTab.h"
#include "../../Foundation/Shared/InspectorWithPreview.h"

namespace Urho3D
{

class Drawable;

void Foundation_PrefabInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class PrefabInspector : public InspectorWithPreview
{
    URHO3D_OBJECT(PrefabInspector, InspectorWithPreview)

public:
    explicit PrefabInspector(Project* project);

protected:
    StringHash GetResourceType() const override;
    SharedPtr<ResourceInspectorWidget> MakeInspectorWidget(const ResourceVector& resources) override;
    SharedPtr<BaseWidget> MakePreviewWidget(Resource* resource) override;
};

} // namespace Urho3D
