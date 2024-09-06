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

#include "../Precompiled.h"

#include "../SystemUI/Widgets.h"

#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../IO/FileSystem.h"
#include "../Input/Input.h"
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

struct QuaternionCachedInfo
{
    unsigned time_{};
    Quaternion value_;
    Vector3 angles_{Quaternion::IDENTITY.EulerAngles()};
};

static ea::unordered_map<ImGuiID, QuaternionCachedInfo> quaternionCache;

void PruneQuaternionCache()
{
    static const unsigned expireTimeMs = 1000;
    const unsigned currentTime = Time::GetSystemTime();
    ea::erase_if(quaternionCache, [&](const auto& pair) { return currentTime - pair.second.time_ > expireTimeMs; });
}

Vector3 GetQuaternionAngles(ImGuiID id, const Quaternion& quaternion)
{
    QuaternionCachedInfo& info = quaternionCache[id];

    info.time_ = Time::GetSystemTime();
    if (info.value_ == quaternion)
        return info.angles_;

    info.value_ = quaternion;
    info.angles_ = quaternion.EulerAngles();
    return info.angles_;
}

void UpdateQuaternionAngles(ImGuiID id, const Quaternion& quaternion, const Vector3& angles)
{
    QuaternionCachedInfo& info = quaternionCache[id];
    info.value_ = quaternion;
    info.angles_ = angles;
}

unsigned GetWrappedIndex(unsigned index, unsigned numLabels)
{
    if (index == 0)
        return 0;

    const unsigned wrappedIndex = (index - 1) % (numLabels - 1);
    return wrappedIndex + 1;
}

const char* StripSpaces(const char* str)
{
    while (*str && *str == ' ')
        str++;
    return str;
}

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

    ui::AlignTextToFramePadding();
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

Color GetItemLabelColor(bool isUndefined, bool defaultValue)
{
    const auto& style = ui::GetStyle();
    if (isUndefined)
        return ToColor(style.Colors[ImGuiCol_TextDisabled]);
    else if (defaultValue)
        return {0.85f, 0.85f, 0.85f, 1.0f};
    else
        return {1.0f, 1.0f, 0.75f, 1.0f};
}

Color GetItemBackgroundColor(bool isUndefined)
{
    const auto& style = ui::GetStyle();
    if (isUndefined)
        return {0.09f, 0.09f, 0.09f, 1.0f};
    else
        return ToColor(style.Colors[ImGuiCol_FrameBg]);
}

void Underline(const Color& color)
{
    const ImVec2 min = ui::GetItemRectMin();
    const ImVec2 max = ui::GetItemRectMax();
    ui::GetWindowDrawList()->AddLine({min.x, max.y}, max, color.ToUInt(), 1.0f);
}

void TextURL(const ea::string& label, const ea::string& url)
{
    auto context = Context::GetInstance();
    auto fs = context->GetSubsystem<FileSystem>();

    const auto& style = ui::GetStyle();

    ui::Text("%s", label.c_str());
    Underline(ToColor(style.Colors[ImGuiCol_Text]));

    const bool isHovered = ui::IsItemHovered();
    const bool isOpened = isHovered && ui::IsMouseClicked(MOUSEB_LEFT);

    if (isHovered)
    {
        ui::SetTooltip("%s", url.c_str());
        ui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    if (isOpened)
        fs->SystemOpen(url);
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
    if (ui::InputText("##Name", &name, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_NoUndoRedo))
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

bool EditResourceRefList(StringHash& type, StringVector& names, const StringVector* allowedTypes, bool resizable,
    const StringVector* elementNames)
{
    bool modified = false;
    ea::optional<unsigned> pendingRemove;

    unsigned index = 0;
    for (ea::string& name : names)
    {
        const IdScopeGuard guardElement{index};
        if (resizable)
        {
            if (ui::Button(ICON_FA_TRASH_CAN))
                pendingRemove = index;
            ui::SameLine();
            if (ui::IsItemHovered())
                ui::SetTooltip("Remove item");
        }
        else if (elementNames && elementNames->size() > 0)
        {
            const unsigned wrappedIndex = index % elementNames->size();
            if (wrappedIndex == 0)
                ui::Separator();
            Widgets::ItemLabel(StripSpaces((*elementNames)[wrappedIndex].c_str()));
        }

        if (EditResourceRef(type, name, allowedTypes))
            modified = true;

        ++index;
    }

    if (pendingRemove && *pendingRemove < names.size())
    {
        names.erase_at(*pendingRemove);
        modified = true;
    }

    if (resizable)
    {
        if (ui::Button(ICON_FA_SQUARE_PLUS " Add item"))
        {
            names.emplace_back();
            modified = true;
        }

        if (ui::IsItemHovered())
            ui::SetTooltip("Add item");
    }
    else if (names.empty())
        ui::NewLine();

    return modified;
}

bool EditBitmask(unsigned& value)
{
    bool modified = false;

    static const unsigned groupWidth = 8;
    static const unsigned groupHeight = 2;
    static const unsigned numGroups = 2;

    const ImGuiStyle& style = ui::GetStyle();
    const ImVec2 buttonSize = [&]
    {
        const float availableWidth = ui::CalcItemWidth();
        const float width = Round(availableWidth / ((groupWidth + 2) * numGroups));
        const float height = Round(GImGui->FontSize * 0.5f + style.ItemSpacing.y);
        return ImVec2{width, height};
    }();

    if (ui::Button(ICON_FA_ELLIPSIS_VERTICAL))
        ui::OpenPopup("##Action");
    ui::SameLine();

    if (ui::BeginPopup("##Action"))
    {
        if (ui::Selectable("Reset all to 0"))
        {
            modified = value != 0;
            value = 0;
        }

        if (ui::Selectable("Set all to 1"))
        {
            modified = value != 0xffffffffu;
            value = 0xffffffffu;
        }

        if (ui::Selectable("Invert all"))
        {
            modified = true;
            value = ~value;
        }
        ui::EndPopup();
    }

    const ImVec2 baseCursorPos = ui::GetCursorPos();
    ui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    for (unsigned row = 0; row < groupHeight; row++)
    {
        for (unsigned col = 0; col < groupWidth * numGroups; col++)
        {
            const unsigned groupIndex = col / groupWidth;
            const unsigned columnInGroup = col % groupWidth;

            const unsigned bitIndex = columnInGroup + groupWidth * (row + groupIndex * groupHeight);
            const unsigned bitMask = 1u << bitIndex;
            const bool selected = (value & bitMask) != 0;

            const ImVec4 currentColor = selected ? style.Colors[ImGuiCol_ButtonActive] : style.Colors[ImGuiCol_Button];
            const ColorScopeGuard guardColor{
                {ea::make_pair(ImGuiCol_Button, currentColor), ea::make_pair(ImGuiCol_ButtonActive, currentColor)}};
            const IdScopeGuard guardId{bitIndex};

            if (ui::Button("", buttonSize))
            {
                modified = true;
                value ^= bitMask;
            }
            if (ui::IsItemHovered())
                ui::SetTooltip("Bit %d", bitIndex);
            ui::SameLine(0, style.PointSize + (columnInGroup == groupWidth - 1 ? buttonSize.x : 0));
        }
        ui::NewLine();
        if (row != groupHeight - 1)
            ui::SetCursorPos({baseCursorPos.x, baseCursorPos.y + buttonSize.y + style.PointSize});
    }
    ui::PopStyleVar();

    return modified;
}

bool EditVariantType(VariantType& value, const char* button)
{
    static const ea::vector<VariantType> allowedTypes{
        VAR_INT,
        VAR_BOOL,
        VAR_FLOAT,
        VAR_VECTOR2,
        VAR_VECTOR3,
        VAR_VECTOR4,
        VAR_QUATERNION,
        VAR_COLOR,
        VAR_STRING,
        VAR_BUFFER,
        VAR_RESOURCEREF,
        VAR_RESOURCEREFLIST,
        VAR_VARIANTVECTOR,
        VAR_VARIANTMAP,
        VAR_INTRECT,
        VAR_INTVECTOR2,
        VAR_MATRIX3,
        VAR_MATRIX3X4,
        VAR_MATRIX4,
        VAR_DOUBLE,
        VAR_STRINGVECTOR,
        VAR_RECT,
        VAR_INTVECTOR3,
        VAR_INT64,
        VAR_VARIANTCURVE,
        VAR_STRINGVARIANTMAP,
    };

    bool modified = false;

    if (ui::Button(button ? button : ICON_FA_LIST))
        ui::OpenPopup("##SelectType");
    if (ui::IsItemHovered())
        ui::SetTooltip("Select variant type");

    if (ui::BeginPopup("##SelectType"))
    {
        for (const VariantType allowedType : allowedTypes)
        {
            if (ui::Selectable(Variant::GetTypeName(allowedType).c_str(), value == allowedType))
            {
                value = allowedType;
                modified = true;
            }
        }
        ui::EndPopup();
    }

    return modified;
}

bool EditVariantValue(Variant& value)
{
    static const auto options = EditVariantOptions{}.AllowResize().AllowTypeChange();
    return EditVariant(value, options);
}

bool EditVariantVector(VariantVector& value, bool resizable, bool dynamicTypes, const StringVector* elementNames)
{
    bool modified = false;
    ea::optional<unsigned> pendingRemove;

    unsigned index = 0;
    for (Variant& element : value)
    {
        const IdScopeGuard guardElement{index};

        if (elementNames && elementNames->size() > 1)
        {
            const unsigned wrappedIndex = GetWrappedIndex(index, elementNames->size());
            if (wrappedIndex == 1)
                ui::Separator();
            Widgets::ItemLabel(StripSpaces((*elementNames)[wrappedIndex].c_str()));
        }

        if (resizable)
        {
            if (ui::Button(ICON_FA_TRASH_CAN))
                pendingRemove = index;
            ui::SameLine();
            if (ui::IsItemHovered())
                ui::SetTooltip("Remove item");
        }

        VariantType elementType = element.GetType();
        if (dynamicTypes)
        {
            if (EditVariantType(elementType))
            {
                element = Variant{elementType};
                modified = true;
            }
            ui::SameLine();
        }

        if (EditVariantValue(element))
            modified = true;

        ++index;
    }

    if (pendingRemove && *pendingRemove < value.size())
    {
        value.erase_at(*pendingRemove);
        modified = true;
    }

    if (resizable)
    {
        const IdScopeGuard guardAddElement{"##AddElement"};

        VariantType newElementType = VAR_NONE;
        if (EditVariantType(newElementType, ICON_FA_SQUARE_PLUS " Add item"))
        {
            value.push_back(Variant{newElementType});
            modified = true;
        }

        if (ui::IsItemHovered())
            ui::SetTooltip("Add item");
    }

    return modified;
}

bool EditStringVector(StringVector& value, bool resizable)
{
    bool modified = false;
    ea::optional<unsigned> pendingRemove;

    unsigned index = 0;
    for (ea::string& element : value)
    {
        const IdScopeGuard guardElement{index};
        if (resizable)
        {
            if (ui::Button(ICON_FA_TRASH_CAN))
                pendingRemove = index;
            ui::SameLine();
            if (ui::IsItemHovered())
                ui::SetTooltip("Remove item");
        }

        ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
        if (ui::InputText("", &element, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_NoUndoRedo))
            return true;

        ++index;
    }

    if (pendingRemove && *pendingRemove < value.size())
    {
        value.erase_at(*pendingRemove);
        modified = true;
    }

    if (resizable)
    {
        // TODO(editor): this "static" is bad in theory
        static ea::string newElement;

        const IdScopeGuard guardAddElement{"##AddElement"};

        const bool isButtonClicked = ui::Button(ICON_FA_SQUARE_PLUS " Add item");
        ui::SameLine();

        ui::SetNextItemWidth(ui::GetContentRegionAvail().x);

        const bool isTextClicked =
            ui::InputText("", &newElement, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_NoUndoRedo);

        if (isButtonClicked || isTextClicked)
        {
            value.push_back(newElement);
            modified = true;
        }

        if (ui::IsItemHovered())
            ui::SetTooltip("Add item");
    }

    return modified;
}

bool EditStringVariantMap(StringVariantMap& value, bool resizable, bool dynamicTypes, bool dynamicMetadata)
{
    bool modified = false;
    ea::optional<ea::string> pendingRemove;

    StringVector sortedKeys = value.keys();
    ea::sort(sortedKeys.begin(), sortedKeys.end());

    for (const auto& key : sortedKeys)
    {
        if (dynamicMetadata && key.ends_with("@"))
            continue;

        const IdScopeGuard guardKey{key.c_str()};

        Widgets::ItemLabel(key.c_str());

        if (resizable)
        {
            if (ui::Button(ICON_FA_TRASH_CAN))
                pendingRemove = key;
            ui::SameLine();
            if (ui::IsItemHovered())
                ui::SetTooltip("Remove item");
        }

        VariantType elementType = value[key].GetType();
        if (dynamicTypes)
        {
            if (EditVariantType(elementType))
            {
                value[key] = Variant{elementType};
                modified = true;
            }
            ui::SameLine();
        }

        if (dynamicMetadata)
        {
            const ea::string metadataKey = key + "@";
            const auto iter = value.find(metadataKey);
            if (iter != value.end())
            {
                // Only enum names are currently supported.
                if (elementType == VAR_INT && iter->second.GetType() == VAR_STRINGVECTOR)
                {
                    const auto options = EditVariantOptions{}.Enum(iter->second.GetStringVector());
                    if (EditVariant(value[key], options))
                        modified = true;
                    continue;
                }
            }
        }

        if (EditVariantValue(value[key]))
            modified = true;
    }

    if (pendingRemove && value.contains(*pendingRemove))
    {
        value.erase(*pendingRemove);
        modified = true;
    }

    if (resizable)
    {
        // TODO(editor): this "static" is bad in theory
        static ea::string newKey;
        static VariantType newElementType = VAR_STRING;

        const IdScopeGuard guardAddElement{"##AddElement"};

        const ea::string addItemTitle = Format(ICON_FA_SQUARE_PLUS " Add new {}", Variant::GetTypeName(newElementType));
        const bool isButtonClicked = ui::Button(addItemTitle.c_str());
        if (ui::IsItemHovered())
            ui::SetTooltip("Add new item to the map");
        ui::SameLine();

        EditVariantType(newElementType);
        ui::SameLine();

        ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
        const bool isTextClicked = ui::InputText("", &newKey, ImGuiInputTextFlags_EnterReturnsTrue);

        if (isButtonClicked || isTextClicked)
        {
            value[newKey] = Variant{newElementType};
            modified = true;
        }

        if (ui::IsItemHovered())
            ui::SetTooltip("Item name");
    }

    return modified;
}

bool EditVariantColor(Variant& var, const EditVariantOptions& options)
{
    const bool isColor = var.GetType() == VAR_COLOR;
    const bool hasAlpha = isColor || var.GetType() == VAR_VECTOR4;

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

bool EditVariantIntVector2(Variant& var, const EditVariantOptions& options)
{
    IntVector2 value = var.GetIntVector2();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragInt2("", &value.x_, options.step_, static_cast<int>(options.min_), static_cast<int>(options.max_)))
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

bool EditVariantIntVector3(Variant& var, const EditVariantOptions& options)
{
    IntVector3 value = var.GetIntVector3();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragInt3("", &value.x_, options.step_, static_cast<int>(options.min_), static_cast<int>(options.max_)))
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

bool EditVariantRect(Variant& var, const EditVariantOptions& options)
{
    Rect value = var.GetRect();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat4("", &value.min_.x_, options.step_, options.min_, options.max_,
            GetFormatStringForStep(options.step_).c_str()))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantQuaternion(Variant& var, const EditVariantOptions& options)
{
    const ImGuiID id = ui::GetID("Quaternion");
    PruneQuaternionCache();

    const Quaternion value = var.GetQuaternion();
    Vector3 angles = GetQuaternionAngles(id, value);

    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    const float maxValue = 360.0f;
    if (ui::DragFloat3("", &angles.x_, 1.0f, -maxValue * 100, maxValue * 100, "%.2f"))
    {
        const Quaternion newValue{angles};
        UpdateQuaternionAngles(id, newValue, angles);

        var = newValue;
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
    const bool isCommitted = ui::InputText("", &value,
        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_NoUndoRedo | ImGuiInputTextFlags_CallbackAlways);
    const bool isDeactivated = ui::IsItemDeactivatedAfterEdit();
    if (isCommitted || isDeactivated)
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
                valueChanged = true;
                break;
            }
        }
        ui::EndCombo();
    }
    return valueChanged;
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

bool EditVariantResourceRefList(Variant& var, const EditVariantOptions& options)
{
    ResourceRefList value = var.GetResourceRefList();
    const unsigned effectiveLines = value.names_.size() + (options.allowResize_ ? 1 : 0);
    if (effectiveLines > 1)
    {
        ui::NewLine();
        ui::Indent();
    }
    if (EditResourceRefList(value.type_, value.names_, options.resourceTypes_, options.allowResize_,
            options.sizedStructVectorElements_))
    {
        var = value;
        return true;
    }
    if (effectiveLines > 1)
        ui::Unindent();
    return false;
}

bool EditVariantVector(Variant& var, const EditVariantOptions& options)
{
    if (!ui::CollapsingHeader("##Elements"))
        return false;

    VariantVector* value = var.GetVariantVectorPtr();
    URHO3D_ASSERT(value);

    bool modified = false;
    ui::Indent();
    if (EditVariantVector(*value, options.allowResize_, options.allowTypeChange_, options.sizedStructVectorElements_))
    {
        modified = true;
    }
    ui::Unindent();
    return modified;
}

bool EditVariantStringVector(Variant& var, const EditVariantOptions& options)
{
    if (!ui::CollapsingHeader("##Elements"))
        return false;

    StringVector* value = var.GetStringVectorPtr();
    URHO3D_ASSERT(value);

    bool modified = false;
    ui::Indent();
    if (EditStringVector(*value, options.allowResize_))
    {
        modified = true;
    }
    ui::Unindent();
    return modified;
}

bool EditVariantStringVariantMap(Variant& var, const EditVariantOptions& options)
{
    if (!ui::CollapsingHeader("##Elements"))
        return false;

    StringVariantMap* value = var.GetStringVariantMapPtr();
    URHO3D_ASSERT(value);

    bool modified = false;
    ui::Indent();
    if (EditStringVariantMap(*value, options.allowResize_, options.allowTypeChange_, options.dynamicMetadata_))
    {
        modified = true;
    }
    ui::Unindent();
    return modified;
}

bool EditVariantBitmask(Variant& var, const EditVariantOptions& options)
{
    unsigned value = var.GetUInt();
    if (EditBitmask(value))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariant(Variant& var, const EditVariantOptions& options)
{
    // TODO(editor): Implement all types
    switch (var.GetType())
    {
    case VAR_NONE:
    case VAR_PTR:
    case VAR_VOIDPTR:
    case VAR_CUSTOM:
        ui::Text("Unsupported type");
        return false;

    case VAR_INT:
        if (options.asBitmask_)
            return EditVariantBitmask(var, options);
        else if (options.intToString_ && !options.intToString_->empty())
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

    case VAR_QUATERNION:
        return EditVariantQuaternion(var, options);

    case VAR_COLOR:
        return EditVariantColor(var, options);

    case VAR_STRING:
        return EditVariantString(var, options);

    // case VAR_BUFFER:

    case VAR_RESOURCEREF:
        return EditVariantResourceRef(var, options);

    case VAR_RESOURCEREFLIST:
        return EditVariantResourceRefList(var, options);

    case VAR_VARIANTVECTOR:
        return EditVariantVector(var, options);

    // case VAR_VARIANTMAP:
    // case VAR_INTRECT:

    case VAR_INTVECTOR2:
        return EditVariantIntVector2(var, options);
    case VAR_INTVECTOR3:
        return EditVariantIntVector3(var, options);

    // case VAR_MATRIX3:
    // case VAR_MATRIX3X4:
    // case VAR_MATRIX4:
    // case VAR_DOUBLE:

    case VAR_STRINGVECTOR:
        return EditVariantStringVector(var, options);

    case VAR_RECT:
        return EditVariantRect(var, options);

    // case VAR_INTVECTOR3:
    // case VAR_INT64:
    // case VAR_VARIANTCURVE:

    case VAR_STRINGVARIANTMAP:
        return EditVariantStringVariantMap(var, options);

    default:
        ui::Button("TODO: Implement");
        return false;
    }
}

ImVec2 FitContent(const ImVec2& contentArea, const ImVec2& originalSize)
{
    const float eps = ea::numeric_limits<float>::epsilon();
    if (contentArea.x <= eps || contentArea.y <= eps || originalSize.x <= eps || originalSize.y <= eps)
    {
        return ImVec2(0.0f, 0.0f);
    }
    const float contentAspect = contentArea.x / contentArea.y;
    const float imageAspect = originalSize.x / originalSize.y;
    if (contentAspect > imageAspect)
    {
        return ImVec2(contentArea.y * imageAspect, contentArea.y);
    }
    else
    {
        return ImVec2(contentArea.x, contentArea.x / imageAspect);
    }
}

void Image(Texture2D* texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tintCol, const ImVec4& borderCol)
{
    auto context = Context::GetInstance();
    auto systemUI = context->GetSubsystem<SystemUI>();

    systemUI->ReferenceTexture(texture);

    ui::Image(ToImTextureID(texture), size, uv0, uv1, tintCol, borderCol);
}

void ImageItem(Texture2D* texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tintCol, const ImVec4& borderCol)
{
    ImGuiWindow* window = ui::GetCurrentWindow();
    ImGuiID id = window->GetID(texture);
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    Image(texture, size, uv0, uv1, tintCol, borderCol);
    ui::ItemAdd(bb, id);
}

bool ImageButton(Texture2D* texture, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, int framePadding, const ImVec4& bgCol, const ImVec4& tintCol)
{
    auto context = Context::GetInstance();
    auto systemUI = context->GetSubsystem<SystemUI>();

    systemUI->ReferenceTexture(texture);

    ImGuiWindow* window = ui::GetCurrentWindow();
    const ImGuiStyle& style = ui::GetStyle();

    ui::PushID(texture);
    const ImGuiID id = window->GetID("#image");
    ui::PopID();

    const auto framePaddingFloat = static_cast<float>(framePadding);
    const ImVec2 padding = (framePadding >= 0) ? ImVec2(framePaddingFloat, framePaddingFloat) : style.FramePadding;
    return ui::ImageButtonEx(id, ToImTextureID(texture), size, uv0, uv1, padding, bgCol, tintCol);
}

}

}
