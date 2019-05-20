//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <limits>

#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Technique.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include "AttributeInspector.h"
#include "Widgets.h"

#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui_internal.h>
#include <ImGui/imgui_stdlib.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Graphics/SceneView.h>

using namespace ui::litterals;

namespace Urho3D
{

VariantType supportedVariantTypes[] = {
    VAR_INT,
    VAR_BOOL,
    VAR_FLOAT,
    VAR_VECTOR2,
    VAR_VECTOR3,
    VAR_VECTOR4,
    VAR_QUATERNION,
    VAR_COLOR,
    VAR_STRING,
    //VAR_BUFFER,
    //VAR_RESOURCEREF,
    //VAR_RESOURCEREFLIST,
    //VAR_VARIANTVECTOR,
    //VAR_VARIANTMAP,
    VAR_INTRECT,
    VAR_INTVECTOR2,
    VAR_MATRIX3,
    VAR_MATRIX3X4,
    VAR_MATRIX4,
    VAR_DOUBLE,
    //VAR_STRINGVECTOR,
    VAR_RECT,
    VAR_INTVECTOR3,
    VAR_INT64,
};
const int MAX_SUPPORTED_VAR_TYPES = SDL_arraysize(supportedVariantTypes);

const char* supportedVariantNames[] = {
    "Int",
    "Bool",
    "Float",
    "Vector2",
    "Vector3",
    "Vector4",
    "Quaternion",
    "Color",
    "String",
    //"Buffer",
    //"ResourceRef",
    //"ResourceRefList",
    //"VariantVector",
    //"VariantMap",
    "IntRect",
    "IntVector2",
    "Matrix3",
    "Matrix3x4",
    "Matrix4",
    "Double",
    //"StringVector",
    "Rect",
    "IntVector3",
    "Int64",
};

static const float buttonWidth()
{
    return 26_dpx;  // TODO: this should not exist
}

bool RenderResourceRef(Object* eventNamespace, StringHash type, const ea::string& name, ea::string& result)
{
    SharedPtr<Resource> resource;
    auto returnValue = false;

    UI_ITEMWIDTH((eventNamespace != nullptr ? 2 : 1) * (-buttonWidth()))
        ui::InputText("", const_cast<char*>(name.c_str()), name.length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

    if (eventNamespace != nullptr)
    {
        bool dropped = false;
        if (ui::BeginDragDropTarget())
        {
            const Variant& payload = ui::AcceptDragDropVariant("path");
            if (!payload.IsEmpty())
            {
                resource = eventNamespace->GetCache()->GetResource(type, payload.GetString());
                dropped = resource != nullptr;
            }
            ui::EndDragDropTarget();
        }
        ui::SetHelpTooltip("Drag resource here.");

        if (dropped)
        {
            result = resource->GetName();
            returnValue = true;
        }

        ui::SameLine(VAR_RESOURCEREF);
        if (ui::IconButton(ICON_FA_CROSSHAIRS))
        {
            eventNamespace->SendEvent(E_INSPECTORLOCATERESOURCE, InspectorLocateResource::P_NAME, name);
        }
        ui::SetHelpTooltip("Locate resource.");
    }

    ui::SameLine(VAR_RESOURCEREF);
    if (ui::IconButton(ICON_FA_TRASH))
    {
        result.clear();
        returnValue = true;
    }
    ui::SetHelpTooltip("Stop using resource.");

    return returnValue;
}

bool RenderSingleAttribute(Object* eventNamespace, const AttributeInfo* info, Variant& value)
{
    const float floatMin = -std::numeric_limits<float>::infinity();
    const float floatMax = std::numeric_limits<float>::infinity();
    const double doubleMin = -std::numeric_limits<double>::infinity();
    const double doubleMax = std::numeric_limits<double>::infinity();
    const float floatStep = 0.01f;
    const float power = 3.0f;

    bool modified = false;
    auto comboValuesNum = 0;
    if (info != nullptr)
    {
        for (; info->enumNames_ && info->enumNames_[++comboValuesNum];);
    }

    if (comboValuesNum > 0 && info != nullptr)
    {
        int current = 0;
        if (info->type_ == VAR_INT)
            current = value.GetInt();
        else if (info->type_ == VAR_STRING)
            current = GetStringListIndex(value.GetString().c_str(), info->enumNames_, 0);
        else
            assert(false);

        modified |= ui::Combo("", &current, info->enumNames_, comboValuesNum);
        if (modified)
        {
            if (info->type_ == VAR_INT)
                value = current;
            else if (info->type_ == VAR_STRING)
                value = info->enumNames_[current];
        }
    }
    else
    {
        switch (info ? info->type_ : value.GetType())
        {
        case VAR_NONE:
            ui::TextUnformatted("None");
            break;
        case VAR_INT:
        {
            if (info && (info->name_.ends_with(" Mask") || info->name_.ends_with(" Bits")))
            {
                auto v = value.GetUInt();
                modified |= ui::MaskSelector(&v);
                if (modified)
                    value = v;
            }
            else
            {
                auto v = value.GetInt();
                modified |= ui::DragInt("", &v, 1, M_MIN_INT, M_MAX_INT);
                if (modified)
                    value = v;
            }
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
            modified |= ui::DragFloat2("", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("xy");
            break;
        }
        case VAR_VECTOR3:
        {
            auto& v = value.GetVector3();
            modified |= ui::DragFloat3("", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("xyz");
            break;
        }
        case VAR_VECTOR4:
        {
            auto& v = value.GetVector4();
            modified |= ui::DragFloat4("", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("xyzw");
            break;
        }
        case VAR_QUATERNION:
        {
            auto v = value.GetQuaternion().EulerAngles();
            modified |= ui::DragFloat3("", const_cast<float*>(&v.x_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("xyz");
            if (modified)
                value = Quaternion(v.x_, v.y_, v.z_);
            break;
        }
        case VAR_COLOR:
        {
            auto& v = value.GetColor();
            modified |= ui::ColorEdit4("", const_cast<float*>(&v.r_));
            ui::SetHelpTooltip("rgba");
            break;
        }
        case VAR_STRING:
        {
            auto& v = const_cast<ea::string&>(value.GetString());
            auto* buffer = ui::GetUIState<ea::string>(v.c_str());
            bool dirty = v.compare(buffer->c_str()) != 0;
            if (dirty)
                ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            modified |= ui::InputText("", buffer, ImGuiInputTextFlags_EnterReturnsTrue);
            if (dirty)
            {
                ui::PopStyleColor();
                if (ui::IsItemHovered())
                    ui::SetTooltip("Press [Enter] to commit changes.");
            }
            if (modified)
                value = *buffer;
            break;
        }
//        case VAR_BUFFER:
        case VAR_RESOURCEREF:
        {
            const auto& ref = value.GetResourceRef();
            auto refType = ref.type_;

            if (refType == StringHash::ZERO && info)
                refType = info->defaultValue_.GetResourceRef().type_;

            ea::string result;
            if (RenderResourceRef(eventNamespace, refType, ref.name_, result))
            {
                value = ResourceRef(refType, result);
                modified = true;
            }
            break;
        }
        case VAR_RESOURCEREFLIST:
        {
            auto& refList = value.GetResourceRefList();
            for (auto i = 0; i < refList.names_.size(); i++)
            {
                UI_ID(i)
                {
                    ea::string result;

                    auto refType = refList.type_;
                    if (refType == StringHash::ZERO && info)
                        refType = info->defaultValue_.GetResourceRef().type_;

                    modified |= RenderResourceRef(eventNamespace, refType, refList.names_[i], result);
                    if (modified)
                    {
                        ResourceRefList newRefList(refList);
                        newRefList.names_[i] = result;
                        value = newRefList;
                        break;
                    }
                }

            }
            if (refList.names_.empty())
            {
                ui::SetCursorPosY(ui::GetCursorPosY() + 5_dpy);
                ui::TextUnformatted("...");
            }
            break;
        }
//            case VAR_VARIANTVECTOR:
        case VAR_VARIANTMAP:
        {
            struct VariantMapState
            {
                ea::string fieldName;
                int variantTypeIndex = 0;
                bool insertingNew = false;
            };

            ui::IdScope idScope(VAR_VARIANTMAP);

            auto* mapState = ui::GetUIState<VariantMapState>();
            auto* map = value.GetVariantMapPtr();
            if (ui::Button(ICON_FA_PLUS))
                mapState->insertingNew = true;

            if (!map->empty())
                ui::NextColumn();

            unsigned index = 0;
            unsigned size = map->size();
            for (auto it = map->begin(); it != map->end(); it++)
            {
                VariantType type = it->second.GetType();
                if (type == VAR_RESOURCEREFLIST || type == VAR_VARIANTMAP || type == VAR_VARIANTVECTOR ||
                    type == VAR_BUFFER || type == VAR_VOIDPTR || type == VAR_PTR)
                    // TODO: Support nested collections.
                    continue;

#if URHO3D_HASH_DEBUG
                const ea::string& name = StringHash::GetGlobalStringHashRegister()->GetString(it->first);
                // Column-friendly indent
                ui::NewLine();
                ui::SameLine(20_dpx);
                ui::TextUnformatted((name.empty() ? it->first.ToString() : name).c_str());
#else
                // Column-friendly indent
                ui::NewLine();
                ui::SameLine(20_dpx);
                ui::TextUnformatted(it->first_.ToString().c_str());
#endif

                ui::NextColumn();
                ui::IdScope entryIdScope(index++);
                UI_ITEMWIDTH(-buttonWidth()) // Space for trashcan button. TODO: trashcan goes out of screen a little for matrices.
                    modified |= RenderSingleAttribute(eventNamespace, nullptr, it->second);
                ui::SameLine(it->second.GetType());
                if (ui::Button(ICON_FA_TRASH))
                {
                    it = map->erase(it);
                    modified |= true;
                    break;
                }

                if (--size > 0)
                    ui::NextColumn();
            }

            if (mapState->insertingNew)
            {
                ui::NextColumn();
                UI_ITEMWIDTH(-1)
                    ui::InputText("###Key", &mapState->fieldName);
                ui::NextColumn();
                UI_ITEMWIDTH(-buttonWidth()) // Space for OK button
                    ui::Combo("###Type", &mapState->variantTypeIndex, supportedVariantNames, MAX_SUPPORTED_VAR_TYPES);
                ui::SameLine(0, 4_dpx);
                if (ui::Button(ICON_FA_CHECK))
                {
                    if (map->find(mapState->fieldName.c_str()) == map->end())   // TODO: Show warning about duplicate name
                    {
                        map->insert(
                            {mapState->fieldName.c_str(), Variant{supportedVariantTypes[mapState->variantTypeIndex]}});
                        mapState->fieldName.clear();
                        mapState->variantTypeIndex = 0;
                        mapState->insertingNew = false;
                        modified = true;
                    }
                }
            }
            break;
        }
        case VAR_INTRECT:
        {
            auto& v = value.GetIntRect();
            modified |= ui::DragInt4("", const_cast<int*>(&v.left_), 1, M_MIN_INT, M_MAX_INT);
            ui::SetHelpTooltip("ltbr");
            break;
        }
        case VAR_INTVECTOR2:
        {
            auto& v = value.GetIntVector2();
            modified |= ui::DragInt2("", const_cast<int*>(&v.x_), 1, M_MIN_INT, M_MAX_INT);
            ui::SetHelpTooltip("xy");
            break;
        }
        case VAR_MATRIX3:
        {
            auto& v = value.GetMatrix3();
            modified |= ui::DragFloat3("###m0", const_cast<float*>(&v.m00_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m0");
            modified |= ui::DragFloat3("###m1", const_cast<float*>(&v.m10_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m1");
            modified |= ui::DragFloat3("###m2", const_cast<float*>(&v.m20_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m2");
            break;
        }
        case VAR_MATRIX3X4:
        {
            auto& v = value.GetMatrix3x4();
            modified |= ui::DragFloat4("###m0", const_cast<float*>(&v.m00_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m0");
            modified |= ui::DragFloat4("###m1", const_cast<float*>(&v.m10_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m1");
            modified |= ui::DragFloat4("###m2", const_cast<float*>(&v.m20_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m2");
            break;
        }
        case VAR_MATRIX4:
        {
            auto& v = value.GetMatrix4();
            modified |= ui::DragFloat4("###m0", const_cast<float*>(&v.m00_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m0");
            modified |= ui::DragFloat4("###m1", const_cast<float*>(&v.m10_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m1");
            modified |= ui::DragFloat4("###m2", const_cast<float*>(&v.m20_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m2");
            modified |= ui::DragFloat4("###m3", const_cast<float*>(&v.m30_), floatStep, floatMin, floatMax, "%.3f", power);
            ui::SetHelpTooltip("m3");
            break;
        }
        case VAR_DOUBLE:
        {
            auto v = value.GetDouble();
            modified |= ui::DragScalar("", ImGuiDataType_Double, &v, floatStep, &doubleMin, &doubleMax, "%.3f", power);
            if (modified)
                value = v;
            break;
        }
        case VAR_STRINGVECTOR:
        {
            auto& v = const_cast<StringVector&>(value.GetStringVector());

            // Insert new item.
            {
                auto* buffer = ui::GetUIState<ea::string>();
                if (ui::InputText("", buffer, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    v.push_back(*buffer);
                    buffer->clear();
                    modified = true;

                    // Expire buffer of this new item just in case other item already used it.
                    UI_ID(v.size())
                        ui::ExpireUIState<ea::string>();
                }
                if (ui::IsItemHovered())
                    ui::SetTooltip("Press [Enter] to insert new item.");
            }

            // List of current items.
            unsigned index = 0;
            for (auto it = v.begin(); it != v.end();)
            {
                ea::string& sv = *it;

                ui::IdScope idScope(++index);
                auto* buffer = ui::GetUIState<ea::string>(sv.c_str());
                if (ui::Button(ICON_FA_TRASH))
                {
                    it = v.erase(it);
                    modified = true;
                    ui::ExpireUIState<ea::string>();
                }
                else if (modified)
                {
                    // After modification of the vector all buffers are expired and recreated because their indexes
                    // changed. Index is used as id in this loop.
                    ui::ExpireUIState<ea::string>();
                    ++it;
                }
                else
                {
                    ui::SameLine();

                    bool dirty = sv.compare(buffer->c_str()) != 0;
                    if (dirty)
                        ui::PushStyleColor(ImGuiCol_Text, ui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    modified |= ui::InputText("", buffer, ImGuiInputTextFlags_EnterReturnsTrue);
                    if (dirty)
                    {
                        ui::PopStyleColor();
                        if (ui::IsItemHovered())
                            ui::SetTooltip("Press [Enter] to commit changes.");
                    }
                    if (modified)
                        sv = *buffer;
                    ++it;
                }
            }

            if (modified)
                value = StringVector(v);

            break;
        }
        case VAR_RECT:
        {
            auto& v = value.GetRect();
            modified |= ui::DragFloat4("###minmax", const_cast<float*>(&v.min_.x_), floatStep, floatMin,
                                       floatMax, "%.3f", power);
            ui::SetHelpTooltip("min xy, max xy");
            break;
        }
        case VAR_INTVECTOR3:
        {
            auto& v = value.GetIntVector3();
            modified |= ui::DragInt3("xyz", const_cast<int*>(&v.x_), 1, M_MIN_INT, M_MAX_INT);
            ui::SetHelpTooltip("xyz");
            break;
        }
        case VAR_INT64:
        {
            auto minVal = std::numeric_limits<long long int>::min();
            auto maxVal = std::numeric_limits<long long int>::max();
            auto v = value.GetInt64();
            modified |= ui::DragScalar("", ImGuiDataType_S64, &v, 1, &minVal, &maxVal);
            if (modified)
                value = v;
            break;
        }
        default:
            break;
        }
    }
    return modified;
}

bool RenderAttributes(Serializable* item, const char* filter, Object* eventNamespace)
{
    if (item->GetNumAttributes() == 0)
        return false;

    if (eventNamespace == nullptr)
        eventNamespace = ui::GetSystemUI();

    auto isOpen = ui::CollapsingHeader(item->GetTypeName().c_str(), ImGuiTreeNodeFlags_DefaultOpen);
    if (isOpen)
    {
        const ea::vector<AttributeInfo>* attributes = item->GetAttributes();
        if (attributes == nullptr)
            return false;

        ui::PushID(item);
        eventNamespace->SendEvent(E_INSPECTORRENDERSTART, InspectorRenderStart::P_SERIALIZABLE, item);

        UI_UPIDSCOPE(1)
        {
            // Show columns after custom widgets at inspector start, but have them in a global
            // context. Columns of all components will be resized simultaneously.
            // [/!\ WARNING /!\]
            // Adding new ID scopes here will break code in custom inspector widgets if that code
            // uses ui::Columns() calls.
            // [/!\ WARNING /!\]
            ui::Columns(2);
        }

        for (const AttributeInfo& info: *attributes)
        {
            if (info.mode_ & AM_NOEDIT)
                continue;

            bool hidden = false;
            Color color = Color::WHITE;
            ea::string tooltip;

            Variant value = item->GetAttribute(info.name_);

            if (info.defaultValue_.GetType() != VAR_NONE && value == info.defaultValue_)
                color = Color::GRAY;

            if (info.mode_ & AM_NOEDIT)
                hidden = true;
            else if (filter != nullptr && *filter && !info.name_.contains(filter, false))
                hidden = true;

            if (info.type_ == VAR_BUFFER || info.type_ == VAR_VARIANTVECTOR || info.type_ == VAR_VOIDPTR || info.type_ == VAR_PTR)
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
                eventNamespace->SendEvent(E_ATTRIBUTEINSPECTOATTRIBUTE, args);
                hidden = args[P_HIDDEN].GetBool();
                color = args[P_COLOR].GetColor();
                tooltip = args[P_TOOLTIP].GetString();
            }

            if (hidden)
                continue;

            ui::PushID(info.name_.c_str());

            ui::TextColored(ToImGui(color), "%s", info.name_.c_str());

            if (!tooltip.empty() && ui::IsItemHovered())
                ui::SetTooltip("%s", tooltip.c_str());

            if (ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_RIGHT))
                ui::OpenPopup("Attribute Menu");

            bool modified = false;
            bool expireBuffers = false;
            if (ui::BeginPopup("Attribute Menu"))
            {
                if (info.defaultValue_.GetType() != VAR_NONE)
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
                            value = info.defaultValue_;     // For current frame to render correctly
                            expireBuffers = true;
                            modified = true;
                        }
                    }
                }

                if (value.GetType() == VAR_INT && info.name_.ends_with(" Mask"))
                {
                    if (ui::MenuItem("Enable All"))
                    {
                        value = M_MAX_UNSIGNED;
                        modified = true;
                    }
                    if (ui::MenuItem("Disable All"))
                    {
                        value = 0;
                        modified = true;
                    }
                    if (ui::MenuItem("Toggle"))
                    {
                        value = value.GetUInt() ^ M_MAX_UNSIGNED;
                        modified = true;
                    }
                }

                // Allow customization of attribute menu.
                using namespace AttributeInspectorMenu;
                eventNamespace->SendEvent(E_ATTRIBUTEINSPECTORMENU, P_SERIALIZABLE, item, P_ATTRIBUTEINFO, (void*)&info);

                ImGui::EndPopup();
            }

            // Buffers have to be expired outside of popup, because popup has it's own id stack. Careful when pushing
            // new IDs in code below, buffer expiring will break!
            if (expireBuffers)
                ui::ExpireUIState<ea::string>();

            ui::NextColumn();

            ui::PushItemWidth(-1);

            // Value widget rendering
            bool nonVariantValue{};
            {
                using namespace InspectorRenderAttribute;
                VariantMap args{ };
                args[P_ATTRIBUTEINFO] = (void*) &info;
                args[P_SERIALIZABLE] = item;
                args[P_HANDLED] = false;
                args[P_MODIFIED] = false;
                // Rendering of custom widgets for values that do not map to Variant.
                eventNamespace->SendEvent(E_INSPECTORRENDERATTRIBUTE, args);
                nonVariantValue = args[P_HANDLED].GetBool();
                if (nonVariantValue)
                    modified |= args[P_MODIFIED].GetBool();
                else
                    // Rendering of default widgets for Variant values.
                    modified |= RenderSingleAttribute(eventNamespace, &info, value);
            }

            // Normal attributes
            auto* modification = ui::GetUIState<ModifiedStateTracker<Variant>>();
            if (modification->TrackModification(modified, [item, &info]() {
                auto previousValue = item->GetAttribute(info.name_);
                if (previousValue.GetType() == VAR_NONE)
                    return info.defaultValue_;
                return previousValue;
            }))
            {
                // This attribute was modified on last frame, but not on this frame. Continuous attribute value modification
                // has ended and we can fire attribute modification event.
                using namespace AttributeInspectorValueModified;
                eventNamespace->SendEvent(E_ATTRIBUTEINSPECTVALUEMODIFIED, P_SERIALIZABLE, item, P_ATTRIBUTEINFO,
                                          (void*)&info, P_OLDVALUE, modification->GetInitialValue(), P_NEWVALUE, value);
            }

            if (!nonVariantValue && modified)
            {
                // Update attribute value and do nothing else for now.
                item->SetAttribute(info.name_, value);
                item->ApplyAttributes();
            }

            ui::PopItemWidth();
            ui::PopID();

            ui::NextColumn();
        }
        ui::Columns();
        eventNamespace->SendEvent(E_INSPECTORRENDEREND);
        ui::PopID();
    }

    return isOpen;
}

bool RenderSingleAttribute(Variant& value)
{
    return RenderSingleAttribute(nullptr, nullptr, value);
}

}

void ImGui::SameLine(Urho3D::VariantType type)
{
    using namespace Urho3D;

    float spacingFix;
    switch (type)
    {
    case VAR_VECTOR2:
    case VAR_VECTOR3:
    case VAR_VECTOR4:
    case VAR_QUATERNION:
    case VAR_COLOR:
    case VAR_INTRECT:
    case VAR_INTVECTOR2:
    case VAR_MATRIX3:
    case VAR_MATRIX3X4:
    case VAR_MATRIX4:
    case VAR_RECT:
    case VAR_INTVECTOR3:
        spacingFix = 0;
        break;
    default:
        spacingFix = 4_dpx;
        break;
    }

    ui::SameLine(0, spacingFix);
}
