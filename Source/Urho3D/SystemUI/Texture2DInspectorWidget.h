// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/Signal.h"
#include "../Graphics/Animation.h"
#include "../SystemUI/ResourceInspectorWidget.h"
#include "../SystemUI/Widgets.h"

namespace Urho3D
{

/// SystemUI widget used to edit 2D texture.
class URHO3D_API Texture2DInspectorWidget : public ResourceInspectorWidget
{
    URHO3D_OBJECT(Texture2DInspectorWidget, ResourceInspectorWidget);

public:
    Texture2DInspectorWidget(Context* context, const ResourceVector& resources);
    ~Texture2DInspectorWidget() override;

    bool CanSave() const override { return false; }

private:
    static const ea::vector<PropertyDesc> properties;
};

} // namespace Urho3D
