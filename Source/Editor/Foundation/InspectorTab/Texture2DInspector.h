// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/InspectorTab.h"
#include "../../Foundation/Shared/InspectorWithPreview.h"

namespace Urho3D
{

void Foundation_Texture2DInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class Texture2DInspector : public InspectorWithPreview
{
    URHO3D_OBJECT(Texture2DInspector, InspectorWithPreview)

public:
    explicit Texture2DInspector(Project* project);

protected:
    StringHash GetResourceType() const override;
    SharedPtr<ResourceInspectorWidget> MakeInspectorWidget(const ResourceVector& resources) override;
    SharedPtr<BaseWidget> MakePreviewWidget(Resource* resource) override;
};

} // namespace Urho3D

