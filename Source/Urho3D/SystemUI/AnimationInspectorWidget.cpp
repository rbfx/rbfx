// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../SystemUI/AnimationInspectorWidget.h"

#include "../SystemUI/SystemUI.h"

namespace Urho3D
{

namespace
{

} // namespace

const ea::vector<ResourceInspectorWidget::PropertyDesc> AnimationInspectorWidget::properties{
    {
        "Length",
        Variant{0.0f},
        [](const Resource* resource) { return Variant{static_cast<const Animation*>(resource)->GetLength()}; },
        [](Resource* resource, const Variant& value) { static_cast<Animation*>(resource)->SetLength(value.GetFloat()); },
        "Length in seconds",
    },
};

AnimationInspectorWidget::AnimationInspectorWidget(Context* context, const ResourceVector& resources)
    : BaseClassName(context, resources, ea::span(properties.begin(), properties.end()))
{
}

AnimationInspectorWidget::~AnimationInspectorWidget()
{
}

} // namespace Urho3D
