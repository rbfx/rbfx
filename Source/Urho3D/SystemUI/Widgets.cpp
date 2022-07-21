//
// Copyright (c) 2008-2017 the Urho3D project.
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
#include "../SystemUI/Widgets.h"

#include "../Input/Input.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../SystemUI/DragDropPayload.h"
#include "../SystemUI/SystemUI.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

namespace Widgets
{

namespace
{

ea::string GetFormatStringForStep(double step)
{
    if (step >= 1.0 || step <= 0.0)
        return "%.0f";
    else
    {
        const auto numDigits = Clamp(RoundToInt(-std::log10(step)), 1, 8);
        return Format("%.{}f", numDigits);
    }
}

ea::optional<StringHash> GetMatchingType(const ResourceFileDescriptor& desc, StringHash currentType, const StringVector* allowedTypes)
{
    if (!allowedTypes)
    {
        if (desc.HasObjectType(currentType))
            return currentType;
        return ea::nullopt;
    }

    if (allowedTypes->empty())
        return StringHash{desc.mostDerivedType_};

    const auto iter = ea::find_if(allowedTypes->begin(), allowedTypes->end(),
        [&](const ea::string& type) { return desc.HasObjectType(type); });
    if (iter != allowedTypes->end())
        return StringHash{*iter};

    return ea::nullopt;
};

}

float GetSmallButtonSize()
{
    ImGuiContext& g = *GImGui;
    return g.FontSize + g.Style.FramePadding.y * 2.0f;
}

bool ToolbarButton(const char* label, const char* tooltip, bool active)
{
    const auto& g = *ui::GetCurrentContext();
    const float dimension = GetSmallButtonSize();

    const ColorScopeGuard guardColor{ImGuiCol_Button, g.Style.Colors[ImGuiCol_ButtonActive], active};
    ui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{});

    const bool result = ui::ButtonEx(label, {dimension, dimension}, ImGuiButtonFlags_PressedOnClick);

    ui::PopStyleVar();

    ui::SameLine(0, 0);

    if (ui::IsItemHovered() && tooltip)
        ui::SetTooltip("%s", tooltip);

    return result;
}

void ToolbarSeparator()
{
    ImGuiContext& g = *GImGui;
    ui::SetCursorPosX(ui::GetCursorPosX() + g.Style.FramePadding.x);
}

void ItemLabel(ea::string_view title, const ea::optional<Color>& color, bool isLeft)
{
    ImGuiWindow& window = *ui::GetCurrentWindow();
    const ImGuiStyle& style = ui::GetStyle();

    const ImVec2 lineStart = ui::GetCursorScreenPos();
    const float fullWidth = ui::GetContentRegionAvail().x;
    const float itemWidth = ui::CalcItemWidth() + style.ItemSpacing.x;
    const ImVec2 textSize = ui::CalcTextSize(title.begin(), title.end());

    ImRect textRect;
    textRect.Min = ui::GetCursorScreenPos();
    if (!isLeft)
        textRect.Min.x = textRect.Min.x + itemWidth;
    textRect.Max = textRect.Min;
    textRect.Max.x += fullWidth - itemWidth;
    textRect.Max.y += textSize.y;

    ui::SetCursorScreenPos(textRect.Min);

    ImGui::AlignTextToFramePadding();
    // Adjust text rect manually because we render it directly into a drawlist instead of using public functions.
    textRect.Min.y += window.DC.CurrLineTextBaseOffset;
    textRect.Max.y += window.DC.CurrLineTextBaseOffset;

    ui::ItemSize(textRect);
    if (ui::ItemAdd(textRect, window.GetID(title.data(), title.data() + title.size())))
    {
        const ColorScopeGuard guardColor{ImGuiCol_Text, color.value_or(Color::BLACK).ToUInt(), color.has_value()};

        ui::RenderTextEllipsis(ui::GetWindowDrawList(), textRect.Min, textRect.Max, textRect.Max.x,
            textRect.Max.x, title.data(), title.data() + title.size(), &textSize);

        if (textRect.GetWidth() < textSize.x && ui::IsItemHovered())
            ui::SetTooltip("%.*s", (int)title.size(), title.data());
    }
    if (isLeft)
    {
        ui::SetCursorScreenPos(textRect.Max - ImVec2{0, textSize.y + window.DC.CurrLineTextBaseOffset});
        ui::SameLine();
    }
    else if (!isLeft)
        ui::SetCursorScreenPos(lineStart);
}

Color GetItemLabelColor(bool canEdit, bool defaultValue)
{
    const auto& style = ui::GetStyle();
    if (!canEdit)
        return ToColor(style.Colors[ImGuiCol_TextDisabled]);
    else if (defaultValue)
        return {0.85f, 0.85f, 0.85f, 1.0f};
    else
        return {1.0f, 1.0f, 0.75f, 1.0f};
}

bool EditResourceRef(StringHash& type, ea::string& name, const StringVector* allowedTypes)
{
    bool modified = false;

    if (allowedTypes != nullptr && !allowedTypes->empty())
    {
        if (ui::Button(ICON_FA_LIST))
            ui::OpenPopup("##SelectType");
        if (ui::IsItemHovered())
            ui::SetTooltip("Select resource type (%u allowed)", allowedTypes->size());
        ui::SameLine();

        if (ui::BeginPopup("##SelectType"))
        {
            for (const ea::string& allowedType : *allowedTypes)
            {
                if (ui::Selectable(allowedType.c_str(), type == StringHash{allowedType}))
                {
                    type = StringHash{allowedType};
                    modified = true;
                }
            }
            ui::EndPopup();
        }
    }

    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::InputText("##Name", &name, ImGuiInputTextFlags_EnterReturnsTrue))
        modified = true;

    if (allowedTypes != nullptr)
    {
        if (ui::IsItemHovered())
        {
            if (allowedTypes->empty())
            {
                ui::SetTooltip("Resource: any type");
            }
            else
            {
                const auto iter = ea::find_if(allowedTypes->begin(), allowedTypes->end(),
                    [&](const ea::string& allowedType) { return type == StringHash{allowedType}; });
                ea::string tooltip = "Resource: ";
                if (iter != allowedTypes->end())
                    tooltip += *iter;
                else
                    tooltip += "Unknown";
                ui::SetTooltip("%s", tooltip.c_str());
            }
        }
    }

    if (ui::BeginDragDropTarget())
    {
        auto payload = dynamic_cast<ResourceDragDropPayload*>(DragDropPayload::Get());
        if (payload && payload->resources_.size() == 1 && !payload->resources_[0].isDirectory_)
        {
            const ResourceFileDescriptor& desc = payload->resources_[0];
            if (const auto matchingType = GetMatchingType(desc, type, allowedTypes))
            {
                if (ui::AcceptDragDropPayload(DragDropPayloadType.c_str()))
                {
                    name = desc.resourceName_;
                    type = *matchingType;
                    modified = true;
                }
            }
        }
        ui::EndDragDropTarget();
    }

    return modified;
}

bool EditVariantColor(Variant& var, const EditVariantOptions& options)
{
    const bool isColor = var.GetType() == VAR_COLOR;
    const bool hasAlpha = var.GetType() == VAR_VECTOR4;

    ImGuiColorEditFlags flags{};
    if (!hasAlpha)
        flags |= ImGuiColorEditFlags_NoAlpha;

    auto color = isColor ? var.GetColor() : hasAlpha ? Color{var.GetVector4()} : Color{var.GetVector3()};
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::ColorEdit4("", &color.r_, flags))
    {
        var = isColor ? Variant{color} : hasAlpha ? Variant{color.ToVector4()} : Variant{color.ToVector3()};
        return true;
    }

    return false;
}

bool EditVariantFloat(Variant& var, const EditVariantOptions& options)
{
    float value = var.GetFloat();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat("", &value, options.step_, options.min_, options.max_, GetFormatStringForStep(options.step_).c_str()))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantVector2(Variant& var, const EditVariantOptions& options)
{
    Vector2 value = var.GetVector2();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat2("", &value.x_, options.step_, options.min_, options.max_, GetFormatStringForStep(options.step_).c_str()))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantVector3(Variant& var, const EditVariantOptions& options)
{
    Vector3 value = var.GetVector3();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat3("", &value.x_, options.step_, options.min_, options.max_, GetFormatStringForStep(options.step_).c_str()))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantVector4(Variant& var, const EditVariantOptions& options)
{
    Vector4 value = var.GetVector4();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat4("", &value.x_, options.step_, options.min_, options.max_, GetFormatStringForStep(options.step_).c_str()))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantBool(Variant& var, const EditVariantOptions& options)
{
    bool value = var.GetBool();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::Checkbox("", &value))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantInt(Variant& var, const EditVariantOptions& options)
{
    int value = var.GetInt();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragInt("", &value, ea::max(1.0, options.step_), options.min_, options.max_))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantString(Variant& var, const EditVariantOptions& options)
{
    ea::string value = var.GetString();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::InputText("", &value, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantEnum(Variant& var, const EditVariantOptions& options)
{
    const auto& items = *options.intToString_;
    const auto maxEnumValue = static_cast<int>(items.size() - 1);
    bool valueChanged = false;

    int value = Clamp(var.GetInt(), 0, maxEnumValue);
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::BeginCombo("", items[value].c_str()))
    {
        for (int index = 0; index <= maxEnumValue; ++index)
        {
            if (ui::Selectable(items[index].c_str(), value == index))
            {
                var = index;
                break;
            }
        }
        ui::EndCombo();
        return true;
    }
    return false;
}

bool EditVariantResourceRef(Variant& var, const EditVariantOptions& options)
{
    ResourceRef value = var.GetResourceRef();

    // TODO(editor): Hack for Light["Light Shape Texture"]
    static const StringVector lightShapeTypes = {Texture2D::GetTypeNameStatic(), TextureCube::GetTypeNameStatic()};
    const StringVector* allowedTypes = value.type_ == Texture::GetTypeNameStatic() ? &lightShapeTypes : options.resourceTypes_;

    if (EditResourceRef(value.type_, value.name_, allowedTypes))
    {
        var = value;
        return true;
    }

    return false;
}

bool EditVariant(Variant& var, const EditVariantOptions& options)
{
    switch (var.GetType())
    {
    case VAR_NONE:
    case VAR_PTR:
    case VAR_VOIDPTR:
    case VAR_CUSTOM:
        ui::Text("Unsupported type");
        return false;

    case VAR_INT:
        if (options.intToString_ && !options.intToString_->empty())
            return EditVariantEnum(var, options);
        else
            return EditVariantInt(var, options);

    case VAR_BOOL:
        return EditVariantBool(var, options);

    case VAR_FLOAT:
        return EditVariantFloat(var, options);

    case VAR_VECTOR2:
        return EditVariantVector2(var, options);

    case VAR_VECTOR3:
        if (options.asColor_)
            return EditVariantColor(var, options);
        else
            return EditVariantVector3(var, options);

    case VAR_VECTOR4:
        if (options.asColor_)
            return EditVariantColor(var, options);
        else
            return EditVariantVector4(var, options);

    // case VAR_QUATERNION:

    case VAR_COLOR:
        return EditVariantColor(var, options);

    case VAR_STRING:
        return EditVariantString(var, options);

    // case VAR_STRING:
    // case VAR_BUFFER:

    case VAR_RESOURCEREF:
        return EditVariantResourceRef(var, options);

    // case VAR_RESOURCEREFLIST:
    // case VAR_VARIANTVECTOR:
    // case VAR_VARIANTMAP:
    // case VAR_INTRECT:
    // case VAR_INTVECTOR2:
    // case VAR_MATRIX3:
    // case VAR_MATRIX3X4:
    // case VAR_MATRIX4:
    // case VAR_DOUBLE:
    // case VAR_STRINGVECTOR:
    // case VAR_RECT:
    // case VAR_INTVECTOR3:
    // case VAR_INT64:
    // case VAR_VARIANTCURVE:

    default:
        ui::Button("TODO: Implement");
        return false;
    }
}

}

}

bool ui::SetDragDropVariant(const ea::string& types, const Urho3D::Variant& variant, ImGuiCond cond)
{
    if (SetDragDropPayload(types.c_str(), nullptr, 0, cond))
    {
        auto* systemUI = static_cast<Urho3D::SystemUI*>(GetIO().UserData);
        systemUI->GetContext()->SetGlobalVar("SystemUI_Drag&Drop_Value", variant);
        return true;
    }
    return false;
}

const Urho3D::Variant& ui::AcceptDragDropVariant(const ea::string& types, ImGuiDragDropFlags flags)
{
    using namespace Urho3D;

    if (const ImGuiPayload* payload = GetDragDropPayload())
    {
        bool accepted = false;
        for (const ea::string& type : types.split(','))
        {
            const char* t = payload->DataType;
            while (t < payload->DataType + URHO3D_ARRAYSIZE(payload->DataType))
            {
                t = strstr(t, type.c_str());
                if (t == nullptr)
                    break;

                const char* tEnd = strstr(t, ",");
                tEnd = tEnd ? Min(t + strlen(t), tEnd) : t + strlen(t);
                if ((t == payload->DataType || t[-1] == ',') && (*tEnd == 0 || *tEnd == ','))
                    accepted = true;
                t = tEnd;
            }
        }

        if (AcceptDragDropPayload(accepted ? payload->DataType : "Smth that won't be accepted.", flags))
        {
            SystemUI* systemUI = static_cast<SystemUI*>(GetIO().UserData);
            return systemUI->GetContext()->GetGlobalVar("SystemUI_Drag&Drop_Value");
        }
    }

    return Urho3D::Variant::EMPTY;
}

void ui::Image(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    auto* systemUI = static_cast<Urho3D::SystemUI*>(GetIO().UserData);
    systemUI->ReferenceTexture(user_texture_id);
#if URHO3D_D3D11
    void* texture_id = user_texture_id->GetShaderResourceView();
#else
    void* texture_id = user_texture_id->GetGPUObject();
#endif
    Image(texture_id, size, uv0, uv1, tint_col, border_col);
}

void ui::ImageItem(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col, const ImVec4& border_col)
{
    ImGuiWindow* window = ui::GetCurrentWindow();
    ImGuiID id = window->GetID(user_texture_id);
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    ui::Image(user_texture_id, size, uv0, uv1, tint_col, border_col);
    ui::ItemAdd(bb, id);
}

bool ui::ImageButton(Urho3D::Texture2D* user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int frame_padding, const ImVec4& bg_col, const ImVec4& tint_col)
{
    auto* systemUI = static_cast<Urho3D::SystemUI*>(GetIO().UserData);
    systemUI->ReferenceTexture(user_texture_id);
#if URHO3D_D3D11
    void* texture_id = user_texture_id->GetShaderResourceView();
#else
    void* texture_id = user_texture_id->GetGPUObject();
#endif
    return ImageButton(texture_id, size, uv0, uv1, frame_padding, bg_col, tint_col);
}

bool ui::ItemMouseActivation(Urho3D::MouseButton button, unsigned flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    bool activated = !ui::IsItemActive() && ui::IsItemHovered();
    if (flags == ImGuiItemMouseActivation_Dragging)
        activated &= ui::IsMouseDragging(button);
    else
        activated &= ui::IsMouseClicked(button);

    if (activated)
        ui::SetActiveID(g.LastItemData.ID, window);
    else if (ui::IsItemActive() && !ui::IsMouseDown(button))
        ui::ClearActiveID();
    return ui::IsItemActive();
}

void ui::HideCursorWhenActive(Urho3D::MouseButton button, bool on_drag)
{
    using namespace Urho3D;
    ImGuiContext& g = *GImGui;
    SystemUI* systemUI = reinterpret_cast<SystemUI*>(g.IO.UserData);
    if (ui::IsItemActive())
    {
        if (!on_drag || ui::IsMouseDragging(button))
        {
            Input* input = systemUI->GetSubsystem<Input>();
            if (input->IsMouseVisible())
            {
                systemUI->SetRelativeMouseMove(true, true);
                input->SetMouseVisible(false);
            }
        }
    }
    else if (ui::IsItemDeactivated())
    {
        systemUI->SetRelativeMouseMove(false, true);
        systemUI->GetSubsystem<Input>()->SetMouseVisible(true);
    }
}
