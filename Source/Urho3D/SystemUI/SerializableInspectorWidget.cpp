//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../SystemUI/SerializableInspectorWidget.h"

#include "../SystemUI/Widgets.h"
#include "../SystemUI/SystemUI.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

SerializableInspectorWidget::SerializableInspectorWidget(Context* context, const SerializableVector& objects)
    : Object(context)
    , objects_(objects)
{
    URHO3D_ASSERT(!objects_.empty());
}

SerializableInspectorWidget::~SerializableInspectorWidget()
{
}

void SerializableInspectorWidget::RenderTitle()
{
    // TODO(editor): Make better title
    ui::Text("%u objects", objects_.size());
}

void SerializableInspectorWidget::RenderContent()
{
    ea::erase_if(objects_, [](const WeakPtr<Serializable>& obj) { return !obj; });
    if (objects_.empty())
        return;

    const auto attributes = objects_[0]->GetAttributes();
    if (!attributes)
        return;

    pendingSetAttributes_.clear();
    for (const AttributeInfo& info : *attributes)
    {
        if (info.mode_ & AM_NOEDIT)
            continue;

        const IdScopeGuard guard{info.name_.c_str()};
        RenderAttribute(info);
    }

    if (!pendingSetAttributes_.empty())
    {
        for (const auto& [info, value] : pendingSetAttributes_)
        {
            OnEditAttributeBegin(this, objects_, info);
            for (Serializable* object : objects_)
            {
                if (object)
                    object->SetAttribute(info->name_, value);
            }
            OnEditAttributeEnd(this, objects_, info);
        }
    }
}

void SerializableInspectorWidget::RenderAttribute(const AttributeInfo& info)
{
    Variant value;
    info.accessor_->Get(objects_[0], value);

    Variant tempValue;
    const bool canEdit = ea::all_of(objects_.begin() + 1, objects_.end(),
        [&](const WeakPtr<Serializable>& obj) { info.accessor_->Get(obj, tempValue); return tempValue == value; });
    const bool isDefaultValue = value == info.defaultValue_;

    Widgets::ItemLabel(info.name_.c_str(), Widgets::GetItemLabelColor(canEdit, isDefaultValue));

    if (!canEdit)
    {
        if (ui::Button(ICON_FA_CODE_MERGE))
            pendingSetAttributes_.emplace_back(&info, value);
        if (ui::IsItemHovered())
            ui::SetTooltip("Override this attribute and enable editing");
        ui::SameLine();
    }

    ui::BeginDisabled(!canEdit);

    Widgets::EditVariantOptions options;
    if (Widgets::EditVariant(value, options))
        pendingSetAttributes_.emplace_back(&info, value);

    ui::EndDisabled();
}

}
