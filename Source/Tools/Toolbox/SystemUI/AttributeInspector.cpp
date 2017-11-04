//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include "AttributeInspector.h"
#include "ImGuiDock.h"
#include "Widgets.h"

#include <IconFontCppHeaders/IconsFontAwesome.h>
#include <ImGui/imgui_internal.h>


namespace Urho3D
{

AttributeInspector::AttributeInspector(Urho3D::Context* context)
    : Object(context)
{
    filter_.front() = 0;
}

struct AttributeInspectorBuffer
{
    explicit AttributeInspectorBuffer(const String& defaultValue=String::EMPTY)
    {
        strncpy(buffer_, defaultValue.CString(), sizeof(buffer_));
    }

    char buffer_[0x1000]{};
};

void AttributeInspector::RenderAttributes(const PODVector<Serializable*>& items)
{
    /// If serializable changes clear value buffers so values from previous item do not appear when inspecting new item.
    if (lastSerializables_ != items)
    {
        maxWidth_ = 0;
        buffers_.Clear();
        lastSerializables_ = items;
    }

    ui::TextUnformatted("Filter");
    NextColumn();
    ui::PushID("FilterEdit");
    ui::PushItemWidth(-1);
    ui::InputText("", &filter_.front(), filter_.size() - 1);
    if (ui::IsItemActive() && ui::IsKeyPressed(ImGuiKey_Escape))
        filter_.front() = 0;
    ui::PopItemWidth();
    ui::PopID();

    for (Serializable* item: items)
    {
        if (item == nullptr)
            continue;

        ui::PushID(item);
        const char* modifiedThisFrame = nullptr;
        const auto& attributes = *item->GetAttributes();

        ui::TextUnformatted(item->GetTypeName().CString());
        ui::Separator();

        for (const AttributeInfo& info: attributes)
        {
            bool hidden = false;
            Color color = Color::WHITE;
            String tooltip;

            Variant value, oldValue;
            value = oldValue = item->GetAttribute(info.name_);

            if (value == info.defaultValue_)
                color = Color::GRAY;

            if (info.mode_ & AM_NOEDIT)
                hidden = true;
            else if (filter_.front() && !info.name_.Contains(&filter_.front(), false))
                hidden = true;

            // Customize attribute rendering
            {
                using namespace AttributeInspectorAttribute;
                VariantMap args;
                args[P_SERIALIZABLE] = item;
                args[P_ATTRIBUTEINFO] = (void*)&info;
                args[P_COLOR] = color;
                args[P_HIDDEN] = hidden;
                args[P_TOOLTIP] = tooltip;
                SendEvent(E_ATTRIBUTEINSPECTOATTRIBUTE, args);
                hidden = args[P_HIDDEN].GetBool();
                color = args[P_COLOR].GetColor();
                tooltip = args[P_TOOLTIP].GetString();
            }

            if (hidden)
                continue;

            ui::PushID(info.name_.CString());

            ui::TextColored(ToImGui(color), "%s", info.name_.CString());
            if (!tooltip.Empty() && ui::IsItemHovered())
                ui::SetTooltip("%s", tooltip.CString());

            if (ui::IsItemHovered() && ui::IsMouseClicked(2))
                ui::OpenPopup("Attribute Menu");

            bool expireBuffers = false;
            if (ui::BeginPopup("Attribute Menu"))
            {
                if (value == info.defaultValue_)
                {
                    ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    ui::MenuItem("Reset to default");
                    ui::PopStyleColor();
                }
                else
                {
                    if (ui::MenuItem("Reset to default"))
                    {
                        item->SetAttribute(info.name_, info.defaultValue_);
                        item->ApplyAttributes();
                        value = info.defaultValue_;     // For current frame to render correctly
                        expireBuffers = true;
                    }
                }

                // Allow customization of attribute menu.
                using namespace AttributeInspectorMenu;
                SendEvent(E_ATTRIBUTEINSPECTORMENU, P_SERIALIZABLE, item, P_ATTRIBUTEINFO, (void*)&info);

                ImGui::EndPopup();
            }

            // Buffers have to be expired outside of popup, because popup has it's own id stack. Careful when pushing
            // new IDs in code below, buffer expiring will break!
            if (expireBuffers)
                ui::ExpireUIState<AttributeInspectorBuffer>();

            NextColumn();

            bool modifiedLastFrame = modifiedLastFrame_ == info.name_.CString();
            ui::PushItemWidth(-1);
            if (RenderSingleAttribute(info, value))
            {
                assert(modifiedThisFrame == nullptr);
                modifiedLastFrame_ = info.name_.CString();

                // Just started changing value of the attribute. Save old value required for event on modification end.
                if (!modifiedLastFrame)
                    originalValue_ = oldValue;

                // Update attribute value and do nothing else for now.
                item->SetAttribute(info.name_, value);
                item->ApplyAttributes();
            }
            else if (modifiedLastFrame && !ui::IsAnyItemActive())
            {
                // This attribute was modified on last frame, but not on this frame. Continous attribute value modification
                // has ended and we can fire attribute modification event.
                using namespace AttributeInspectorValueModified;
                SendEvent(E_ATTRIBUTEINSPECTVALUEMODIFIED, P_SERIALIZABLE, item, P_ATTRIBUTEINFO, (void*)&info,
                    P_OLDVALUE, originalValue_, P_NEWVALUE, value);
            }
            ui::PopItemWidth();

            ui::PopID();
        }

        ui::PopID();
    }

    // Just finished modifying attribute.
    if (modifiedLastFrame_ && !ui::IsAnyItemActive())
        modifiedLastFrame_ = nullptr;
}

void AttributeInspector::RenderAttributes(Serializable* item)
{
    PODVector<Serializable*> items;
    items.Push(item);
    RenderAttributes(items);
}

bool AttributeInspector::RenderSingleAttribute(const AttributeInfo& info, Variant& value)
{
    const float floatMin = -14000.f;
    const float floatMax = 14000.f;
    const float floatStep = 0.01f;
    const float power = 3.0f;

    bool modified = false;
    const char** comboValues = nullptr;
    auto comboValuesNum = 0;
    if (info.enumNames_)
    {
        comboValues = info.enumNames_;
        for (; comboValues[++comboValuesNum];);
    }

    if (comboValues)
    {
        int current = value.GetInt();
        modified |= ui::Combo("", &current, comboValues, comboValuesNum);
        if (modified)
            value = current;
    }
    else
    {
        switch (info.type_)
        {
        case VAR_NONE:
            ui::TextUnformatted("None");
            break;
        case VAR_INT:
        {
            // TODO: replace this with custom control that properly handles int types.
            auto v = value.GetInt();
            modified |= ui::DragInt("", &v, 1, M_MIN_INT, M_MAX_INT);
            if (modified)
                value = v;
            break;
        }
        case VAR_BOOL:
        {
            auto v = value.GetBool();
            modified |= ui::Checkbox("", &v);
            if (modified)
                value = v;
            break;
        }
        case VAR_FLOAT:
        {
            auto v = value.GetFloat();
            modified |= ui::DragFloat("", &v, floatStep, floatMin, floatMax, "%.3f", power);
            if (modified)
                value = v;
            break;
        }
        case VAR_VECTOR2:
        {
            auto& v = value.GetVector2();
            modified |= ui::DragFloat2("xy", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            break;
        }
        case VAR_VECTOR3:
        {
            auto& v = value.GetVector3();
            modified |= ui::DragFloat3("xyz", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            break;
        }
        case VAR_VECTOR4:
        {
            auto& v = value.GetVector4();
            modified |= ui::DragFloat4("xyzw", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            break;
        }
        case VAR_QUATERNION:
        {
            auto v = value.GetQuaternion().EulerAngles();
            modified |= ui::DragFloat3("xyz", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            if (modified)
                value = Quaternion(v.x_, v.y_, v.z_);
            break;
        }
        case VAR_COLOR:
        {
            auto& v = value.GetColor();
            modified |= ui::ColorEdit4("rgba", const_cast<float*>(&v.r_));
            break;
        }
        case VAR_STRING:
        {
            auto& v = const_cast<String&>(value.GetString());
            AttributeInspectorBuffer* state = ui::GetUIState<AttributeInspectorBuffer>(v);
            bool dirty = v != state->buffer_;
            if (dirty)
                ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            modified |= ui::InputText("", state->buffer_, IM_ARRAYSIZE(state->buffer_), ImGuiInputTextFlags_EnterReturnsTrue);
            if (dirty)
            {
                ui::PopStyleColor();
                if (ui::IsItemHovered())
                    ui::SetTooltip("Press [Enter] to commit changes.");
            }
            if (modified)
                value = state->buffer_;
            break;
        }
//            case VAR_BUFFER:
        case VAR_VOIDPTR:
            ui::Text("%p", value.GetVoidPtr());
            break;
        case VAR_RESOURCEREF:
        {
            auto ref = value.GetResourceRef();
            String name = ref.name_;
            ui::InputText("", (char*)name.CString(), name.Length(), ImGuiInputTextFlags_AutoSelectAll |
                ImGuiInputTextFlags_ReadOnly);
            if (ui::DroppedOnItem())
            {
                Variant dragData = GetSystemUI()->GetDragData();
                SharedPtr<Resource> resource;

                if (dragData.GetType() == VAR_STRING)
                    resource = GetCache()->GetResource(ref.type_, dragData.GetString());
                else if (dragData.GetType() == VAR_RESOURCEREF)
                    resource = GetCache()->GetResource(ref.type_, dragData.GetResourceRef().name_);

                if (resource.NotNull())
                {
                    ref.name_ = resource->GetName();
                    value = ref;
                    modified = true;
                }
            }
            else if (ui::IsItemHovered())
                ui::SetTooltip("Drag resource here.");
            break;
        }
//            case VAR_RESOURCEREFLIST:
//            case VAR_VARIANTVECTOR:
//            case VAR_VARIANTMAP:
        case VAR_INTRECT:
        {
            auto& v = value.GetIntRect();
            modified |= ui::DragInt4("ltbr", const_cast<int*>(&v.left_), 1, M_MIN_INT, M_MAX_INT);
            break;
        }
        case VAR_INTVECTOR2:
        {
            auto& v = value.GetIntVector2();
            modified |= ui::DragInt2("xy", const_cast<int*>(&v.x_), 1, M_MIN_INT, M_MAX_INT);
            break;
        }
        case VAR_PTR:
            ui::Text("%p (Void Pointer)", value.GetPtr());
            break;
        case VAR_MATRIX3:
        {
            auto& v = value.GetMatrix3();
            modified |= ui::DragFloat3("m0", const_cast<float*>(&v.m00_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat3("m1", const_cast<float*>(&v.m10_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat3("m2", const_cast<float*>(&v.m20_), floatStep, floatMin, floatMax, "%.3f", power);
            break;
        }
        case VAR_MATRIX3X4:
        {
            auto& v = value.GetMatrix3x4();
            modified |= ui::DragFloat4("m0", const_cast<float*>(&v.m00_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat4("m1", const_cast<float*>(&v.m10_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat4("m2", const_cast<float*>(&v.m20_), floatStep, floatMin, floatMax, "%.3f", power);
            break;
        }
        case VAR_MATRIX4:
        {
            auto& v = value.GetMatrix4();
            modified |= ui::DragFloat4("m0", const_cast<float*>(&v.m00_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat4("m1", const_cast<float*>(&v.m10_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat4("m2", const_cast<float*>(&v.m20_), floatStep, floatMin, floatMax, "%.3f", power);
            modified |= ui::DragFloat4("m3", const_cast<float*>(&v.m30_), floatStep, floatMin, floatMax, "%.3f", power);
            break;
        }
        case VAR_DOUBLE:
        {
            // TODO: replace this with custom control that properly handles double types.
            float v = static_cast<float>(value.GetDouble());
            modified |= ui::DragFloat("", &v, floatStep, floatMin, floatMax, "%.3f", power);
            if (modified)
                value = (double)v;
            break;
        }
        case VAR_STRINGVECTOR:
        {
            auto& v = const_cast<StringVector&>(value.GetStringVector());

            // Insert new item.
            {
                AttributeInspectorBuffer* state = ui::GetUIState<AttributeInspectorBuffer>();
                if (ui::InputText("", state->buffer_, IM_ARRAYSIZE(state->buffer_), ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    v.Push(state->buffer_);
                    *state->buffer_ = 0;
                    modified = true;

                    // Expire buffer of this new item just in case other item already used it.
                    ui::PushID(v.Size());
                    ui::ExpireUIState<AttributeInspectorBuffer>();
                    ui::PopID();
                }
                if (ui::IsItemHovered())
                    ui::SetTooltip("Press [Enter] to insert new item.");
            }

            // List of current items.
            unsigned index = 0;
            for (auto it = v.Begin(); it != v.End();)
            {
                String& sv = *it;

                ui::PushID(++index);
                AttributeInspectorBuffer* state = ui::GetUIState<AttributeInspectorBuffer>(sv);
                if (ui::Button(ICON_FA_TRASH))
                {
                    it = v.Erase(it);
                    modified = true;
                    ui::ExpireUIState<AttributeInspectorBuffer>();
                }
                else if (modified)
                {
                    // After modification of the vector all buffers are expired and recreated because their indexes
                    // changed. Index is used as id in this loop.
                    ui::ExpireUIState<AttributeInspectorBuffer>();
                    ++it;
                }
                else
                {
                    ui::SameLine();

                    bool dirty = sv != state->buffer_;
                    if (dirty)
                        ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    modified |= ui::InputText("", state->buffer_, IM_ARRAYSIZE(state->buffer_),
                        ImGuiInputTextFlags_EnterReturnsTrue);
                    if (dirty)
                    {
                        ui::PopStyleColor();
                        if (ui::IsItemHovered())
                            ui::SetTooltip("Press [Enter] to commit changes.");
                    }
                    if (modified)
                        sv = state->buffer_;
                    ++it;
                }
                ui::PopID();
            }

            if (modified)
                value = StringVector(v);

            break;
        }
        case VAR_RECT:
        {
            auto& v = value.GetRect();
            modified |= ui::DragFloat2("min xy", const_cast<float*>(&v.min_.x_), floatStep, floatMin,
                                       floatMax, "%.3f", power);
            ui::SameLine();
            modified |= ui::DragFloat2("max xy", const_cast<float*>(&v.max_.x_), floatStep, floatMin,
                                       floatMax, "%.3f", power);
            break;
        }
        case VAR_INTVECTOR3:
        {
            auto& v = value.GetIntVector3();
            modified |= ui::DragInt3("xyz", const_cast<int*>(&v.x_), 1, M_MIN_INT, M_MAX_INT);
            break;
        }
        case VAR_INT64:
        {
            // TODO: replace this with custom control that properly handles int types.
            int v = static_cast<int>(value.GetInt64());
            if (value.GetInt64() > M_MAX_INT || value.GetInt64() < M_MIN_INT)
                URHO3D_LOGWARNINGF("AttributeInspector truncated 64bit integer value.");
            modified |= ui::DragInt("", &v, 1, M_MIN_INT, M_MAX_INT, "%d");
            if (modified)
                value = (long long)v;
            break;
        }
        default:
            ui::TextUnformatted("Unhandled attribute type.");
            break;
        }
    }
    return modified;
}

void AttributeInspector::NextColumn()
{
    ui::SameLine();
    maxWidth_ = Max(maxWidth_, ui::GetCursorPosX());
    ui::SameLine(maxWidth_);
}

AttributeInspectorWindow::AttributeInspectorWindow(Context* context) : AttributeInspector(context)
{

}

void AttributeInspectorWindow::SetEnabled(bool enabled)
{
    if (enabled && !IsEnabled())
        SubscribeToEvent(E_UPDATE, std::bind(&AttributeInspectorWindow::RenderUi, this));
    else if (!enabled && IsEnabled())
        UnsubscribeFromEvent(E_UPDATE);
}

void AttributeInspectorWindow::SetSerializable(Serializable* item)
{
    currentSerializable_ = item;
}

void AttributeInspectorWindow::RenderUi()
{
    if (ui::Begin("Inspector"))
    {
        if (currentSerializable_.NotNull())
        {
            RenderAttributes(currentSerializable_);
        }
    }
    ui::End();
}

bool AttributeInspectorWindow::IsEnabled() const
{
    return HasSubscribedToEvent(E_UPDATE);
}

AttributeInspectorDockWindow::AttributeInspectorDockWindow(Context* context)
    : AttributeInspectorWindow(context)
{

}

void AttributeInspectorDockWindow::RenderUi()
{
    if (ui::BeginDock("Inspector"))
    {
        if (currentSerializable_.NotNull())
        {
            RenderAttributes(currentSerializable_);
        }
    }
    ui::EndDock();
}

}
