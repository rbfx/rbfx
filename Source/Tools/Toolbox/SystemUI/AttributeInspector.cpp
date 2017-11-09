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
#include "ImGuiDock.h"
#include "Widgets.h"

#include <IconFontCppHeaders/IconsFontAwesome.h>
#include <ImGui/imgui_internal.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Graphics/SceneView.h>


namespace Urho3D
{

// Should these be exported by the engine?
static const char* cullModeNames[] =
{
    "none",
    "ccw",
    "cw",
    nullptr
};
static const char* fillModeNames[] =
{
    "solid",
    "wireframe",
    "point",
    nullptr
};
#define MAX_FILLMODES (IM_ARRAYSIZE(fillModeNames) - 1)

const float attributeIndentLevel = 15.f;

/// Renders material preview in attribute inspector.
class MaterialView : public SceneView
{
    URHO3D_OBJECT(MaterialView, SceneView);
public:
    explicit MaterialView(Context* context, Material* material, Viewport* effectSource)
        : SceneView(context, {0, 0, 200, 200})
        , material_(material)
    {
        // Workaround: for some reason this overriden method of our class does not get called by SceneView constructor.
        CreateObjects();

        // Scene viewport renderpath must be same as material viewport renderpath
        auto path = effectSource->GetRenderPath();
        viewport_->SetRenderPath(path);
        auto light = camera_->GetComponent<Light>();
        for (auto& command: path->commands_)
        {
            if (command.pixelShaderName_ == "PBRDeferred")
            {
                // Lights in PBR scenes need modifications, otherwise obects in material preview look very dark
                light->SetUsePhysicalValues(true);
                light->SetBrightness(5000);
                light->SetShadowCascade(CascadeParameters(10, 20, 30, 40, 10));
                break;
            }
        }
    }

    void Render()
    {
        int size = static_cast<int>(ui::GetWindowWidth() - ui::GetCursorPosX());
        SetSize({0, 0, size, size});
        ui::Image(texture_.Get(), ToImGui(rect_.Size()));
        Input* input = camera_->GetInput();
        bool rightMouseButtonDown = input->GetMouseButtonDown(MOUSEB_RIGHT);
        if (ui::IsItemHovered())
        {
            if (rightMouseButtonDown)
                SetGrab(true);
            else if (input->GetMouseButtonPress(MOUSEB_LEFT))
                ToggleModel();
        }

        if (mouseGrabbed_)
        {
            if (rightMouseButtonDown)
            {
                if (input->GetKeyPress(KEY_ESCAPE))
                {
                    camera_->SetPosition(Vector3::BACK * distance_);
                    camera_->LookAt(Vector3::ZERO);
                }
                else
                {
                    IntVector2 delta = input->GetMouseMove();
                    camera_->RotateAround(Vector3::ZERO,
                                          Quaternion(delta.x_ * 0.1f, camera_->GetUp()) *
                                          Quaternion(delta.y_ * 0.1f, camera_->GetRight()), TS_WORLD);
                }
            }
            else
                SetGrab(false);
        }
    }

    void ToggleModel()
    {
        auto model = node_->GetOrCreateComponent<StaticModel>();

        model->SetModel(GetCache()->GetResource<Model>(ToString("Models/%s.mdl", figures_[figureIndex_])));
        model->SetMaterial(material_);
        auto bb = model->GetBoundingBox();
        auto scale = 1.f / Max(bb.Size().x_, Max(bb.Size().y_, bb.Size().z_));
        if (figures_[figureIndex_] == "Box")    // Box is rather big after autodetecting scale, but other figures
            scale *= 0.7f;                      // are ok. Patch the box then.
        else if (figures_[figureIndex_] == "TeaPot")    // And teapot is rather small.
            scale *= 1.2f;
        node_->SetScale(scale);
        node_->SetWorldPosition(node_->GetWorldPosition() - model->GetWorldBoundingBox().Center());

        figureIndex_ = ++figureIndex_ % figures_.Size();
    }

    void SetGrab(bool enable)
    {
        if (mouseGrabbed_ == enable)
            return;

        mouseGrabbed_ = enable;
        Input* input = camera_->GetInput();
        if (enable && input->IsMouseVisible())
            input->SetMouseVisible(false);
        else if (!enable && !input->IsMouseVisible())
            input->SetMouseVisible(true);
    }

protected:
    void CreateObjects() override
    {
        SceneView::CreateObjects();
        node_ = scene_->CreateChild("Sphere");
        ToggleModel();
        camera_->CreateComponent<Light>();
        camera_->SetPosition(Vector3::BACK * distance_);
        camera_->LookAt(Vector3::ZERO);
    }

    /// Material which is being previewed.
    SharedPtr<Material> material_;
    /// Node holding figure to which material is applied.
    WeakPtr<Node> node_;
    /// Flag indicating if this widget grabbed mouse for rotating material node.
    bool mouseGrabbed_ = false;
    /// Index of current figure displaying material.
    int figureIndex_ = 0;
    /// A list of figures between which material view can be toggled.
    PODVector<const char*> figures_{"Sphere", "Box", "Torus", "TeaPot"};
    /// Distance from camera to figure.
    float distance_ = 1.5f;
};

struct AttributeInspectorBuffer
{
    explicit AttributeInspectorBuffer(const String& defaultValue=String::EMPTY)
    {
        strncpy(buffer_, defaultValue.CString(), sizeof(buffer_) - 1);
        buffer_[sizeof(buffer_) - 1] = 0;
    }

    char buffer_[0x1000]{};
};

AttributeInspector::AttributeInspector(Urho3D::Context* context)
    : Object(context)
{
    filter_.front() = 0;
}

void AttributeInspector::RenderAttributes(const PODVector<Serializable*>& items)
{
    /// If serializable changes clear value buffers so values from previous item do not appear when inspecting new item.
    if (lastSerializables_ != items)
    {
        maxWidth_ = 0;
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

        if (ui::CollapsingHeader(item->GetTypeName().CString(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            ui::PushID(item);
            const char* modifiedThisFrame = nullptr;
            const auto& attributes = *item->GetAttributes();

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

                bool modified = false;
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
                            modified = true;
                        }
                    }

                    if (value.GetType() == VAR_INT && info.name_.EndsWith(" Mask"))
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
                    SendEvent(E_ATTRIBUTEINSPECTORMENU, P_SERIALIZABLE, item, P_ATTRIBUTEINFO, (void*)&info);

                    ImGui::EndPopup();
                }

                // Buffers have to be expired outside of popup, because popup has it's own id stack. Careful when pushing
                // new IDs in code below, buffer expiring will break!
                if (expireBuffers)
                    ui::ExpireUIState<AttributeInspectorBuffer>();

                bool expanded = true;
                bool expandable = false;
                if (value.GetType() == VAR_RESOURCEREF)
                {
                    if (value.GetResourceRef().type_ == Material::GetTypeStatic())
                        expandable = true;
                }
                else if (value.GetType() == VAR_RESOURCEREFLIST)
                {
                    if (value.GetResourceRefList().type_ == Material::GetTypeStatic())
                        expandable = true;
                }

                expanded = RenderAttributeLabel(info, color, expandable);

                if (!tooltip.Empty() && ui::IsItemHovered())
                    ui::SetTooltip("%s", tooltip.CString());

                if (ui::IsItemHovered() && ui::IsMouseClicked(2))
                    ui::OpenPopup("Attribute Menu");

                NextColumn();

                bool modifiedLastFrame = modifiedLastFrame_ == info.name_.CString();
                ui::PushItemWidth(-1);
                modified |= RenderSingleAttribute(info, value, expanded);
                ui::PopItemWidth();
                ui::PopID();

                if (modified)
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
                    // This attribute was modified on last frame, but not on this frame. Continuous attribute value modification
                    // has ended and we can fire attribute modification event.
                    using namespace AttributeInspectorValueModified;
                    SendEvent(E_ATTRIBUTEINSPECTVALUEMODIFIED, P_SERIALIZABLE, item, P_ATTRIBUTEINFO, (void*)&info,
                        P_OLDVALUE, originalValue_, P_NEWVALUE, value);
                }
            }

            ui::PopID();
        }
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

bool AttributeInspector::RenderSingleAttribute(const AttributeInfo& info, Variant& value, bool expanded)
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
            if (info.name_.EndsWith(" Mask"))
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
            const auto& ref = value.GetResourceRef();
            String result;
            if (RenderResourceRef(ref.type_, ref.name_, result, expanded))
            {
                value = ResourceRef(ref.type_, result);
                modified = true;
            }
            break;
        }
        case VAR_RESOURCEREFLIST:
        {
            auto& refList = value.GetResourceRefList();
            for (auto i = 0; i < refList.names_.Size(); i++)
            {
                ui::PushID(i);
                String result;
                if (RenderResourceRef(refList.type_, refList.names_[i], result, expanded))
                {
                    ResourceRefList newRefList(refList);
                    newRefList.names_[i] = result;
                    value = newRefList;
                    modified = true;
                    ui::PopID();
                    break;
                }
                ui::PopID();

                // Render labels for multiple resources
                if (i < refList.names_.Size() - 1)
                {
                    ui::PushID(i + 1);
                    expanded = RenderAttributeLabel(info, Color::WHITE, value.GetResourceRefList().type_ == Material::GetTypeStatic());
                    NextColumn();
                    ui::PopID();
                }
            }
            if (refList.names_.Empty())
                ui::NewLine();
            break;
        }
//            case VAR_VARIANTVECTOR:
//            case VAR_VARIANTMAP:
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
        case VAR_PTR:
            ui::Text("%p (Void Pointer)", value.GetPtr());
            break;
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
            modified |= ui::DragFloat2("###min xy", const_cast<float*>(&v.min_.x_), floatStep, floatMin,
                                       floatMax, "%.3f", power);
            ui::SetHelpTooltip("min xy");
            ui::SameLine();
            modified |= ui::DragFloat2("###max xy", const_cast<float*>(&v.max_.x_), floatStep, floatMin,
                                       floatMax, "%.3f", power);
            ui::SetHelpTooltip("max xy");
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

bool AttributeInspector::RenderResourceRef(StringHash type, const String& name, String& result, bool expanded)
{
    auto handleDragAndDrop = [&](StringHash resourceType, SharedPtr<Resource>& resource)
    {
        if (ui::DroppedOnItem())
        {
            Variant dragData = GetSystemUI()->GetDragData();

            if (dragData.GetType() == VAR_STRING)
                resource = GetCache()->GetResource(resourceType, dragData.GetString());
            else if (dragData.GetType() == VAR_RESOURCEREF)
                resource = GetCache()->GetResource(resourceType, dragData.GetResourceRef().name_);

            return resource.NotNull();
        }
        else
            ui::SetHelpTooltip("Drag resource here.");
    };

    SharedPtr<Resource> resource;
    ui::PushItemWidth(ui::ScaleX(-30));
    ui::InputText("", (char*)name.CString(), name.Length(), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
    ui::PopItemWidth();
    if (handleDragAndDrop(type, resource))
    {
        result = resource->GetName();
        ui::ExpireUIState<MaterialView>();
        return true;
    }

    ui::SameLine();
    if (ui::IconButton(ICON_FA_TRASH))
    {
        result.Clear();
        return true;
    }

    if (!expanded || name.Empty())
        return false;

    if (type == Material::GetTypeStatic())
    {
        Material* material = GetCache()->GetResource<Material>(name);
        if (material == nullptr)
            return false;

        MaterialView* state = ui::GetUIState<MaterialView>(context_, material, effectSource_);
        ui::Indent(attributeIndentLevel);

        state->Render();
        if (handleDragAndDrop(type, resource))
        {
            result = resource->GetName();
            ui::ExpireUIState<MaterialView>();
            return true;
        }
        ui::SetHelpTooltip("Drag resource here.\nClick to switch object.");

        ui::TextUnformatted("Cull");
        NextColumn();
        int valueInt = material->GetCullMode();
        if (ui::Combo("###cull", &valueInt, cullModeNames, (int)MAX_CULLMODES))
        {
            material->SetCullMode(static_cast<CullMode>(valueInt));
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Shadow Cull");
        NextColumn();
        valueInt = material->GetShadowCullMode();
        if (ui::Combo("###shadowCull", &valueInt, cullModeNames, (int)MAX_CULLMODES))
        {
            material->SetShadowCullMode(static_cast<CullMode>(valueInt));
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Fill");
        NextColumn();
        valueInt = material->GetFillMode();
        if (ui::Combo("###fill", &valueInt, fillModeNames, (int)MAX_FILLMODES))
        {
            material->SetFillMode(static_cast<FillMode>(valueInt));
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        auto bias = material->GetDepthBias();
        ui::TextUnformatted("Constant Bias");
        NextColumn();
        if (ui::DragFloat("###constantBias_", &bias.constantBias_, 0.1f, -1, 1))
        {
            material->SetDepthBias(bias);
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Slope Scaled Bias");
        NextColumn();
        if (ui::DragFloat("###slopeScaledBias_", &bias.slopeScaledBias_, 1, -16, 16))
        {
            material->SetDepthBias(bias);
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Normal Offset");
        NextColumn();
        if (ui::DragFloat("###normalOffset_", &bias.normalOffset_, 1, 0))
        {
            material->SetDepthBias(bias);
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Alpha To Coverage");
        NextColumn();
        bool valueBool = material->GetAlphaToCoverage();
        if (ui::Checkbox("###alphaToCoverage_", &valueBool))
        {
            material->SetAlphaToCoverage(valueBool);
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Line Anti-Alias");
        NextColumn();
        valueBool = material->GetLineAntiAlias();
        if (ui::Checkbox("###lineAntiAlias_", &valueBool))
        {
            material->SetLineAntiAlias(valueBool);
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Occlusion");
        NextColumn();
        valueBool = material->GetOcclusion();
        if (ui::Checkbox("###occlusion_", &valueBool))
        {
            material->SetOcclusion(valueBool);
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        ui::TextUnformatted("Render Order");
        NextColumn();
        valueInt = material->GetRenderOrder();
        if (ui::DragInt("###renderOrder_", &valueInt, 1, 0, 0xFF))
        {
            material->SetRenderOrder(static_cast<unsigned char>(valueInt));
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }

        for (unsigned i = 0; i < material->GetNumTechniques(); i++)
        {
            ui::PushID(i);
            TechniqueEntry& tech = const_cast<TechniqueEntry&>(material->GetTechniqueEntry(i));

            bool open = ui::CollapsingHeaderSimple(ToString("Technique %d", i).CString());
            NextColumn();
            String techName = tech.technique_->GetName();
            if (material->GetNumTechniques() > 1)
                ui::PushItemWidth(ui::ScaleX(-30));
            ui::InputText("###techniqueName_", (char*)techName.CString(), techName.Length(),
                ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);
            if (material->GetNumTechniques() > 1)
                ui::PopItemWidth();
            if (handleDragAndDrop(Technique::GetTypeStatic(), resource))
            {
                material->SetTechnique(i, DynamicCast<Technique>(resource), tech.qualityLevel_, tech.lodDistance_);
                material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
                resource.Reset();
            }

            if (material->GetNumTechniques() > 1)
            {
                ui::SameLine();
                if (ui::IconButton(ICON_FA_TRASH))
                {
                    for (auto j = i + 1; j < material->GetNumTechniques(); j++)
                        material->SetTechnique(j - 1, material->GetTechnique(j));
                    material->SetNumTechniques(material->GetNumTechniques() - 1);
                    material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
                    ui::PopID();
                    break;
                }
            }

            if (open)
            {
                ui::Indent(attributeIndentLevel);

                ui::TextUnformatted("LOD Distance");
                NextColumn();
                if (ui::DragFloat("###lodDistance_", &tech.lodDistance_))
                    material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));

                ui::TextUnformatted("Quality");
                NextColumn();
                if (ui::DragInt("###qualityLevel_", &tech.qualityLevel_))
                    material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));

                ui::Unindent(attributeIndentLevel);
            }
            ui::PopID();
        }

        const char* newTechnique = "Add new technique";
        ui::InputText("###newTechnique_", (char*)newTechnique, strlen(newTechnique), ImGuiInputTextFlags_ReadOnly);
        if (handleDragAndDrop(Technique::GetTypeStatic(), resource))
        {
            material->SetNumTechniques(material->GetNumTechniques() + 1);
            material->SetTechnique(material->GetNumTechniques() - 1, dynamic_cast<Technique*>(resource.Get()));
            material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
        }
        ui::Unindent(attributeIndentLevel);
    }

    return false;
}

void AttributeInspector::CopyEffectsFrom(Viewport* source)
{
    effectSource_ = source;
}

bool AttributeInspector::RenderAttributeLabel(const AttributeInfo& info, Color color, bool expandable)
{
    bool expanded = false;
    if (expandable)
    {
        ui::PushStyleColor(ImGuiCol_Text, ToImGui(color));
        expanded = ui::CollapsingHeaderSimple(info.name_.CString());
        ui::PopStyleColor();
    }
    else
        ui::TextColored(ToImGui(color), "%s", info.name_.CString());
    return expanded;
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
