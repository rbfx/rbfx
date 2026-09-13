// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../SystemUI/ResourceInspectorWidget.h"

namespace Urho3D
{

/// SystemUI widget used to edit models.
class URHO3D_API ModelInspectorWidget : public ResourceInspectorWidget
{
    URHO3D_OBJECT(ModelInspectorWidget, ResourceInspectorWidget);

public:
    ModelInspectorWidget(Context* context, const ResourceVector& resources);
    ~ModelInspectorWidget() override;

    bool CanSave() const override { return false; }

private:
    static const ea::vector<PropertyDesc> properties;
};

} // namespace Urho3D
