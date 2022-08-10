//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../SystemUI/ResourceInspectorWidget.h"
#include "../SystemUI/SystemUI.h"

#include <EASTL/fixed_vector.h>

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

    if (Widgets::EditVariant(value, desc.options_))
        pendingSetProperties_.emplace_back(&desc, value);
}

} // namespace Urho3D
