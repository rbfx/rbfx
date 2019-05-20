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

#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui_stdlib.h>
#include <Urho3D/IO/Log.h>
#include "Tabs/Scene/SceneTab.h"
#include "EditorEvents.h"
#include "MaterialInspector.h"
#include "MaterialInspectorUndo.h"
#include "Editor.h"

using namespace ImGui::litterals;

namespace Urho3D
{

MaterialInspector::MaterialInspector(Context* context, Material* material)
    : PreviewInspector(context)
    , inspectable_(new Inspectable::Material(material))
    , attributeInspector_(context)
{
    auto autoSave = [this](StringHash, VariantMap&) {
        // Auto-save material on modification
        auto* material = inspectable_->GetMaterial();
        GetCache()->IgnoreResourceReload(material);
        material->SaveFile(GetCache()->GetResourceFileName(material->GetName()));
    };
    SubscribeToEvent(&attributeInspector_, E_ATTRIBUTEINSPECTVALUEMODIFIED, autoSave);
    SubscribeToEvent(&attributeInspector_, E_INSPECTORRENDERSTART, [this](StringHash, VariantMap&) { RenderPreview(); });
    SubscribeToEvent(&attributeInspector_, E_INSPECTORRENDERATTRIBUTE, [this](StringHash, VariantMap& args) { RenderCustomWidgets(args); });

    undo_.Connect(&attributeInspector_);

    ToggleModel();
}

void MaterialInspector::RenderInspector(const char* filter)
{
    RenderAttributes(inspectable_, filter, &attributeInspector_);
}

void MaterialInspector::ToggleModel()
{
    const char* currentModel = figures_[figureIndex_];
    SetModel(ToString("Models/%s.mdl", currentModel));
    figureIndex_ = ++figureIndex_ % figures_.size();

    StaticModel* model = node_->GetComponent<StaticModel>();
    model->SetMaterial(inspectable_->GetMaterial());

    auto bb = model->GetBoundingBox();
    auto scale = 1.f / Max(bb.Size().x_, Max(bb.Size().y_, bb.Size().z_));
    if (strcmp(currentModel, "Box") == 0)            // Box is rather big after autodetecting scale, but other
        scale *= 0.7f;                               // figures are ok. Patch the box then.
    else if (strcmp(currentModel, "TeaPot") == 0)    // And teapot is rather small.
        scale *= 1.2f;
    node_->SetScale(scale);
}

void MaterialInspector::Save()
{
    inspectable_->GetMaterial()->SaveFile(GetCache()->GetResourceFileName(inspectable_->GetMaterial()->GetName()));
}

void MaterialInspector::RenderPreview()
{
    PreviewInspector::RenderPreview();
    ui::SetHelpTooltip("Click to switch object.");
    if (ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_LEFT))
        ToggleModel();
    else
        HandleInput();

    if (Material* material = inspectable_->GetMaterial())
    {
        const char* resourceName = material->GetName().c_str();
        ui::SetCursorPosX((ui::GetContentRegionMax().x - ui::CalcTextSize(resourceName).x) / 2);
        ui::TextUnformatted(resourceName);
        ui::Separator();
    }
}

void MaterialInspector::RenderCustomWidgets(VariantMap& args)
{
    using namespace InspectorRenderAttribute;
    AttributeInfo& info = *reinterpret_cast<AttributeInfo*>(args[P_ATTRIBUTEINFO].GetVoidPtr());
    Material* material = (static_cast<Inspectable::Material*>(args[P_SERIALIZABLE].GetPtr()))->GetMaterial();

    if (info.name_ == "Techniques")
    {
        ui::NextColumn();
        auto secondColWidth = ui::GetColumnWidth(1) - ui::GetStyle().ItemSpacing.x * 2;

        bool modified = false;
        for (unsigned i = 0; i < material->GetNumTechniques(); i++)
        {
            if (i > 0)
                ui::Separator();

            ui::IdScope idScope(i);

            auto tech = material->GetTechniqueEntry(i);
            auto* modification = ui::GetUIState<ModifiedStateTracker<TechniqueEntry>>();

            ui::Columns();
            ea::string techName = tech.technique_->GetName();
            UI_ITEMWIDTH(material->GetNumTechniques() > 1 ? -44_dpx : -22_dpx)
                ui::InputText("###techniqueName_", const_cast<char*>(techName.c_str()), techName.length(),
                              ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_ReadOnly);

            if (ui::BeginDragDropTarget())
            {
                const Variant& payload = ui::AcceptDragDropVariant("path");
                if (!payload.IsEmpty())
                {
                    auto* technique = GetCache()->GetResource<Technique>(payload.GetString());
                    if (technique)
                    {
                        material->SetTechnique(i, technique, tech.qualityLevel_, tech.lodDistance_);
                        undo_.Track<Undo::TechniqueChangedAction>(material, i, &tech, &material->GetTechniqueEntry(i));
                        modified = true;
                    }
                }
                ui::EndDragDropTarget();
            }
            ui::SetHelpTooltip("Drag resource here.");

            ui::SameLine(VAR_NONE);
            if (ui::IconButton(ICON_FA_CROSSHAIRS))
            {
                SendEvent(E_INSPECTORLOCATERESOURCE, InspectorLocateResource::P_NAME, material->GetTechnique(i)->GetName());
            }
            ui::SetHelpTooltip("Locate resource");

            if (material->GetNumTechniques() > 1)
            {
                ui::SameLine(VAR_NONE);
                if (ui::IconButton(ICON_FA_TRASH))
                {
                    for (auto j = i + 1; j < material->GetNumTechniques(); j++)
                        material->SetTechnique(j - 1, material->GetTechnique(j));
                    undo_.Track<Undo::TechniqueChangedAction>(material, i, &tech, nullptr);
                    // Technique removed possibly from the middle. Shift existing techniques back to the front and remove last one.
                    for (auto j = i + 1; j < material->GetNumTechniques(); j++)
                    {
                        const auto& entry = material->GetTechniqueEntry(j);
                        material->SetTechnique(j - 1, entry.original_.Get(), entry.qualityLevel_, entry.lodDistance_);
                    }
                    material->SetNumTechniques(material->GetNumTechniques() - 1);
                    modified = true;
                    break;
                }
            }

            UI_UPIDSCOPE(3)            // Technique, attribute and serializable scopes
                ui::Columns(2);

            bool modifiedField = false;
            secondColWidth = ui::GetColumnWidth(1) - ui::GetStyle().ItemSpacing.x * 2;

            // ---------------------------------------------------------------------------------------------------------

            ui::NewLine();
            ui::SameLine(20_dpx);
            ui::TextUnformatted("LOD Distance");
            ui::NextColumn();
            UI_ITEMWIDTH(secondColWidth)
                modifiedField |= ui::DragFloat("###LOD Distance", &tech.lodDistance_);
            ui::NextColumn();

            // ---------------------------------------------------------------------------------------------------------

            static const char* qualityNames[] = {
                "Low", "Medium", "High", "Ultra", "Max"
            };

            ui::NewLine();
            ui::SameLine(20_dpx);
            ui::TextUnformatted("Quality");
            ui::NextColumn();
            UI_ITEMWIDTH(secondColWidth)
                modifiedField |= ui::Combo("###Quality", reinterpret_cast<int*>(&tech.qualityLevel_), qualityNames, SDL_arraysize(qualityNames));
            ui::NextColumn();

            if (modification->TrackModification(modifiedField, [material, i]() { return material->GetTechniqueEntry(i); }))
                undo_.Track<Undo::TechniqueChangedAction>(material, i, &modification->GetInitialValue(), &tech);

            if (modifiedField)
                material->SetTechnique(i, tech.original_.Get(), tech.qualityLevel_, tech.lodDistance_);

            modified |= modifiedField;
        }

        ui::Columns();
        const char addNewTechnique[]{"Add New Technique"};
        UI_ITEMWIDTH(-1)
        {
            ui::InputText("###Add Technique", const_cast<char*>(addNewTechnique), sizeof(addNewTechnique),
                          ImGuiInputTextFlags_ReadOnly);
        }
        if (ui::BeginDragDropTarget())
        {
            const Variant& payload = ui::AcceptDragDropVariant("path");
            if (!payload.IsEmpty())
            {
                auto* technique = GetCache()->GetResource<Technique>(payload.GetString());
                if (technique)
                {
                    auto index = material->GetNumTechniques();
                    material->SetNumTechniques(index + 1);
                    material->SetTechnique(index, technique);
                    undo_.Track<Undo::TechniqueChangedAction>(material, index, nullptr, &material->GetTechniqueEntry(index));
                    modified = true;
                }
            }
            ui::EndDragDropTarget();
        }
        ui::SetHelpTooltip("Drag and drop technique resource here.");

        UI_UPIDSCOPE(2)            // Attribute and serializable scopes
        {
            ui::Columns(2);
            ui::NextColumn();      // Custom widget must end rendering in second column
        }

        args[P_HANDLED] = true;
        args[P_MODIFIED] = modified;
    }
    else if (info.name_ == "Shader Parameters")
    {
        struct ShaderParameterState
        {
            ea::string fieldName_;
            int variantTypeIndex_ = 0;
            bool insertingNew_ = false;
        };

        auto* paramState = ui::GetUIState<ShaderParameterState>();
        if (ui::Button(ICON_FA_PLUS))
            paramState->insertingNew_ = true;
        ui::SetHelpTooltip("Add new shader parameter.");
        ui::NextColumn();

        bool modified = false;

        const auto& parameters = material->GetShaderParameters();
        unsigned parametersLeft = parameters.size();
        for (const auto& pair : parameters)
        {
            const ea::string& parameterName = pair.second.name_;
            ui::IdScope id(parameterName.c_str());
            auto* modification = ui::GetUIState<ModifiedStateTracker<Variant>>();

            ui::NewLine();
            ui::SameLine(20_dpx);
            ui::TextUnformatted(parameterName.c_str());
            ui::NextColumn();
            Variant value = pair.second.value_;

            UI_ITEMWIDTH(-22_dpx)
            {
                bool modifiedNow = RenderSingleAttribute(value);
                if (modification->TrackModification(modifiedNow, [material, &parameterName]() { return material->GetShaderParameter(parameterName); }))
                {
                    undo_.Track<Undo::ShaderParameterChangedAction>(material, parameterName, modification->GetInitialValue(), value);
                    modified = true;
                }
                if (modifiedNow)
                    material->SetShaderParameter(parameterName, value);
            }

            ui::SameLine(value.GetType());
            if (ui::Button(ICON_FA_TRASH))
            {
                undo_.Track<Undo::ShaderParameterChangedAction>(material, parameterName, pair.second.value_, Variant{});
                material->RemoveShaderParameter(parameterName);
                modified = true;
                break;
            }

            if (--parametersLeft > 0)
                ui::NextColumn();
        }

        if (paramState->insertingNew_)
        {
            static const VariantType shaderParameterVariantTypes[] = {
                VAR_FLOAT,
                VAR_VECTOR2,
                VAR_VECTOR3,
                VAR_VECTOR4,
                VAR_COLOR,
                VAR_RECT,
            };

            static const char* shaderParameterVariantNames[] = {
                "Float",
                "Vector2",
                "Vector3",
                "Vector4",
                "Color",
                "Rect",
            };

            ui::NextColumn();
            UI_ITEMWIDTH(-1)
                ui::InputText("###Name", &paramState->fieldName_);
            ui::SetHelpTooltip("Shader parameter name.");

            ui::NextColumn();
            UI_ITEMWIDTH(-22_dpx) // Space for OK button
                ui::Combo("###Type", &paramState->variantTypeIndex_, shaderParameterVariantNames, SDL_arraysize(shaderParameterVariantTypes));
            ui::SetHelpTooltip("Shader parameter type.");

            ui::SameLine(0, 4_dpx);
            if (ui::Button(ICON_FA_CHECK))
            {
                if (!paramState->fieldName_.empty() && material->GetShaderParameter(paramState->fieldName_.c_str()).GetType() == VAR_NONE)   // TODO: Show warning about duplicate name
                {
                    Variant value{shaderParameterVariantTypes[paramState->variantTypeIndex_]};
                    undo_.Track<Undo::ShaderParameterChangedAction>(material, paramState->fieldName_.c_str(), Variant{}, value);
                    material->SetShaderParameter(paramState->fieldName_.c_str(), value);
                    modified = true;
                    paramState->fieldName_.clear();
                    paramState->variantTypeIndex_ = 0;
                    paramState->insertingNew_ = false;
                }
            }
        }

        args[P_HANDLED] = true;
        args[P_MODIFIED] = modified;
    }
}

Inspectable::Material::Material(Urho3D::Material* material)
    : Serializable(material->GetContext())
    , material_(material)
{
}

void Inspectable::Material::RegisterObject(Context* context)
{
    // Cull Mode
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)       { value = inspectable.GetMaterial()->GetCullMode(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value) { inspectable.GetMaterial()->SetCullMode(static_cast<CullMode>(value.GetUInt())); };
        URHO3D_CUSTOM_ENUM_ATTRIBUTE("Cull", getter, setter, cullModeNames, CULL_NONE, AM_EDIT);
    }

    // Shadow Cull Mode
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)       { value = inspectable.GetMaterial()->GetShadowCullMode(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value) { inspectable.GetMaterial()->SetShadowCullMode(static_cast<CullMode>(value.GetUInt())); };
        URHO3D_CUSTOM_ENUM_ATTRIBUTE("Shadow Cull", getter, setter, cullModeNames, CULL_NONE, AM_EDIT);
    }

    // Fill Mode
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)       { value = inspectable.GetMaterial()->GetFillMode(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value) { inspectable.GetMaterial()->SetFillMode(static_cast<FillMode>(value.GetUInt())); };
        URHO3D_CUSTOM_ENUM_ATTRIBUTE("Fill", getter, setter, fillModeNames, FILL_SOLID, AM_EDIT);
    }

    // Alpha To Coverage
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)       { value = inspectable.GetMaterial()->GetAlphaToCoverage(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value) { inspectable.GetMaterial()->SetAlphaToCoverage(value.GetBool()); };
        URHO3D_CUSTOM_ATTRIBUTE("Alpha To Coverage", getter, setter, bool, false, AM_EDIT);
    }

    // Line Anti Alias
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)       { value = inspectable.GetMaterial()->GetLineAntiAlias(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value) { inspectable.GetMaterial()->SetLineAntiAlias(value.GetBool()); };
        URHO3D_CUSTOM_ATTRIBUTE("Line Anti Alias", getter, setter, bool, false, AM_EDIT);
    }

    // Render Order
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)       { value = inspectable.GetMaterial()->GetRenderOrder(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value) { inspectable.GetMaterial()->SetRenderOrder(static_cast<unsigned char>(value.GetUInt())); };
        URHO3D_CUSTOM_ATTRIBUTE("Render Order", getter, setter, unsigned, DEFAULT_RENDER_ORDER, AM_EDIT);
    }

    // Occlusion
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)        { value = inspectable.GetMaterial()->GetOcclusion(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)  { inspectable.GetMaterial()->SetOcclusion(value.GetBool()); };
        URHO3D_CUSTOM_ATTRIBUTE("Occlusion", getter, setter, bool, false, AM_EDIT);
    }

    // Occlusion
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)        { value = inspectable.GetMaterial()->GetOcclusion(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)  { inspectable.GetMaterial()->SetOcclusion(value.GetBool()); };
        URHO3D_CUSTOM_ATTRIBUTE("Occlusion", getter, setter, bool, false, AM_EDIT);
    }

    // Occlusion
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)        { value = inspectable.GetMaterial()->GetOcclusion(); };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)  { inspectable.GetMaterial()->SetOcclusion(value.GetBool()); };
        URHO3D_CUSTOM_ATTRIBUTE("Occlusion", getter, setter, bool, false, AM_EDIT);
    }

    // Constant Bias
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)        { value = inspectable.GetMaterial()->GetDepthBias().constantBias_; };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)  {
            auto bias = inspectable.GetMaterial()->GetDepthBias();
            bias.constantBias_ = value.GetFloat();
            inspectable.GetMaterial()->SetDepthBias(bias);
        };
        URHO3D_CUSTOM_ATTRIBUTE("Constant Bias", getter, setter, float, 0.0f, AM_EDIT);
    }

    // Slope Scaled Bias
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)        { value = inspectable.GetMaterial()->GetDepthBias().slopeScaledBias_; };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)  {
            auto bias = inspectable.GetMaterial()->GetDepthBias();
            bias.slopeScaledBias_ = value.GetFloat();
            inspectable.GetMaterial()->SetDepthBias(bias);
        };
        URHO3D_CUSTOM_ATTRIBUTE("Slope Scaled Bias", getter, setter, float, 0.0f, AM_EDIT);
    }

    // Normal Offset
    {
        auto getter = [](const Inspectable::Material& inspectable, Variant& value)        { value = inspectable.GetMaterial()->GetDepthBias().normalOffset_; };
        auto setter = [](const Inspectable::Material& inspectable, const Variant& value)  {
            auto bias = inspectable.GetMaterial()->GetDepthBias();
            bias.normalOffset_ = value.GetFloat();
            inspectable.GetMaterial()->SetDepthBias(bias);
        };
        URHO3D_CUSTOM_ATTRIBUTE("Normal Offset", getter, setter, float, 0.0f, AM_EDIT);
    }

    // Dummy attributes used for rendering custom widgets in inspector.
    URHO3D_CUSTOM_ATTRIBUTE("Techniques", [](const Inspectable::Material&, Variant&){}, [](const Inspectable::Material&, const Variant&){}, unsigned, Variant{}, AM_EDIT);
    URHO3D_CUSTOM_ATTRIBUTE("Shader Parameters", [](const Inspectable::Material&, Variant&){}, [](const Inspectable::Material&, const Variant&){}, unsigned, Variant{}, AM_EDIT);

    for (auto i = 0; i < MAX_MATERIAL_TEXTURE_UNITS; i++)
    {
        ea::string finalName = ToString("%s Texture", textureUnitNames[i]);
        ReplaceUTF8(finalName, 0, ToUpper(AtUTF8(finalName, 0)));
        auto textureUnit = static_cast<TextureUnit>(i);

        auto getter = [textureUnit](const Inspectable::Material& inspectable, Variant& value)
        {
            if (auto* texture = inspectable.GetMaterial()->GetTexture(textureUnit))
                value = ResourceRef(Texture2D::GetTypeStatic(), texture->GetName());
            else
                value = ResourceRef{Texture2D::GetTypeStatic()};
        };
        auto setter = [textureUnit](const Inspectable::Material& inspectable, const Variant& value)
        {
            const auto& ref = value.GetResourceRef();
            if (auto* texture = inspectable.GetCache()->GetResource(ref.type_, ref.name_)->Cast<Texture>())
                inspectable.GetMaterial()->SetTexture(textureUnit, texture);
        };
        URHO3D_CUSTOM_ATTRIBUTE(finalName.c_str(), getter, setter, ResourceRef, ResourceRef{Texture2D::GetTypeStatic()}, AM_EDIT);
    }
}

}
