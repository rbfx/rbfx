// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Graphics/Model.h"

#include "Urho3D/SystemUI/ModelInspectorWidget.h"
#include "Urho3D/SystemUI/SystemUI.h"

namespace Urho3D
{

namespace
{

} // namespace

const ea::vector<ResourceInspectorWidget::PropertyDesc> ModelInspectorWidget::properties{};

ModelInspectorWidget::ModelInspectorWidget(Context* context, const ResourceVector& resources)
    : BaseClassName(context, resources, ea::span(properties.begin(), properties.end()))
{
}

ModelInspectorWidget::~ModelInspectorWidget()
{
}

} // namespace Urho3D
