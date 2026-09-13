// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/SystemUI/ResourceInspectorWidget.h"

#include "Urho3D/SystemUI/SystemUI.h"

namespace Urho3D
{

namespace
{

} // namespace

ResourceInspectorWidget::ResourceInspectorWidget(
    Context* context, const ResourceVector& resources, ea::span<const PropertyDesc> properties)
    : BaseClassName(context)
    , resources_(resources)
    , properties(properties)
{
    URHO3D_ASSERT(!resources_.empty());
}

ResourceInspectorWidget::~ResourceInspectorWidget()
{
}

void ResourceInspectorWidget::RenderTitle()
{
    if (resources_.size() == 1)
        ui::Text("%s", resources_[0]->GetName().c_str());
    else
        ui::Text("%d %s", resources_.size(), resources_[0]->GetTypeInfo()->GetTypeName().c_str());
}

void ResourceInspectorWidget::RenderContent()
{
    pendingSetProperties_.clear();

    const IdScopeGuard guard("RenderProperties");

    if (!ui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (const PropertyDesc& property : properties)
        RenderProperty(property);

    ui::Separator();

    if (!pendingSetProperties_.empty())
    {
        OnEditBegin(this);
        for (Resource* material : resources_)
        {
            for (const auto& [desc, value] : pendingSetProperties_)
                desc->setter_(material, value);
        }
        OnEditEnd(this);
    }
}

void ResourceInspectorWidget::RenderProperty(const PropertyDesc& desc)
{
    const IdScopeGuard guard(desc.name_.c_str());

    Variant value = desc.getter_(resources_[0]);
    const bool isUndefined = ea::any_of(resources_.begin() + 1, resources_.end(),
        [&](const Resource* resource) { return value != desc.getter_(resource); });

    Widgets::ItemLabel(desc.name_, Widgets::GetItemLabelColor(isUndefined, value == desc.defaultValue_));
    if (!desc.hint_.empty() && ui::IsItemHovered())
        ui::SetTooltip("%s", desc.hint_.c_str());

    const ColorScopeGuard guardBackgroundColor{
        ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(isUndefined), isUndefined};

    ui::BeginDisabled(!CanSave());
    if (Widgets::EditVariant(value, desc.options_))
        pendingSetProperties_.emplace_back(&desc, value);
    ui::EndDisabled();
}

} // namespace Urho3D
