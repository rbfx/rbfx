// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/SystemUI/ResourceInspectorWidget.h"

namespace Urho3D
{

/// SystemUI widget used to edit animation.
class URHO3D_API AnimationInspectorWidget : public ResourceInspectorWidget
{
    URHO3D_OBJECT(AnimationInspectorWidget, ResourceInspectorWidget);

public:
    AnimationInspectorWidget(Context* context, const ResourceVector& resources);
    ~AnimationInspectorWidget() override;

    bool CanSave() const override { return false; }

private:
    static const ea::vector<PropertyDesc> properties;
};

} // namespace Urho3D
