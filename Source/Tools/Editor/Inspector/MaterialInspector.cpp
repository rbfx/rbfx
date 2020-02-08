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
#include <Urho3D/Core/Function.h>
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
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <EASTL/sort.h>

#include <Toolbox/Common/UndoManager.h>
#include <Toolbox/SystemUI/Widgets.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <ImGui/imgui_stdlib.h>
#include "Tabs/Scene/SceneTab.h"
#include "MaterialInspector.h"
#include "MaterialInspectorUndo.h"
#include "Editor.h"


namespace Urho3D
{

MaterialInspector::MaterialInspector(Context* context)
    : PreviewInspector(context)
    , attributeInspector_(context)
{
}

void MaterialInspector::SetInspected(Object* inspected)
{
    assert(inspected->IsInstanceOf<Material>());
    BaseClassName::SetInspected(inspected);

    material_ = static_cast<Material*>(inspected);
    // if (auto* material = )
    // {
    //     inspectable_ = new Inspectable::Material(material);
    //     auto autoSave = [this](StringHash, VariantMap&) {
    //         // Auto-save material on modification
    //         auto* cache = GetSubsystem<ResourceCache>();
    //         Material* material = inspectable_->GetMaterial();
    //         cache->IgnoreResourceReload(material);
    //         material->SaveFile(context_->GetCache()->GetResourceFileName(material->GetName()));
    //     };
    //     SubscribeToEvent(&attributeInspector_, E_ATTRIBUTEINSPECTVALUEMODIFIED, autoSave);
    //     SubscribeToEvent(&attributeInspector_, E_INSPECTORRENDERSTART, [this](StringHash, VariantMap&) { RenderPreview(); });
    //     SubscribeToEvent(&attributeInspector_, E_INSPECTORRENDERATTRIBUTE, [this](StringHash, VariantMap& args) { RenderCustomWidgets(args); });
    //
    //     // TODO: We need to look up asset by it's byproduct.
    //     auto* editor = GetSubsystem<Editor>();
    //     Asset* asset = nullptr;
    //     for (Object* object : editor->GetInspected())
    //     {
    //         asset = object ? object->Cast<Asset>() : nullptr;
    //         if (asset)
    //             break;
    //     }
    //     assert(asset != nullptr);
    //     asset_ = asset;
    //     asset_->GetUndo().Connect(&attributeInspector_);
    //
        ToggleModel();
    // }
}

void MaterialInspector::RenderInspector(const char* filter)
{
    ImGuiStyle& style = ui::GetStyle();

    if (!ui::CollapsingHeader(material_->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        return;

    float w = ui::CalcItemWidth();
    auto undo = GetSubsystem<UndoStack>();

    ui::PushID(material_);
    // Cull
    {
        int value = material_->GetCullMode(), valuePrev = value;
        if (ui::Combo("Cull", &value, cullModeNames, MAX_CULLMODES))
        {
            material_->SetCullMode(static_cast<CullMode>(value));
            undo->Add<UndoResourceSetter<Material, CullMode>>(material_->GetName(), static_cast<CullMode>(valuePrev),
                static_cast<CullMode>(value), &Material::SetCullMode);
        }
    }
    // Shadow Cull
    {
        int value = material_->GetShadowCullMode(), valuePrev = value;
        if (ui::Combo("Shadow Cull", &value, cullModeNames, MAX_CULLMODES))
        {
            material_->SetShadowCullMode(static_cast<CullMode>(value));
            undo->Add<UndoResourceSetter<Material, CullMode>>(material_->GetName(), static_cast<CullMode>(valuePrev),
                static_cast<CullMode>(value), &Material::SetShadowCullMode);
        }
    }
    // Fill Mode
    {
        int value = material_->GetFillMode(), valuePrev = value;
        if (ui::Combo("Fill Mode", &value, fillModeNames, MAX_FILLMODES))
        {
            material_->SetFillMode(static_cast<FillMode>(value));
            undo->Add<UndoResourceSetter<Material, FillMode>>(material_->GetName(), static_cast<FillMode>(valuePrev),
                static_cast<FillMode>(value), &Material::SetFillMode);
        }
    }
    // Alpha To Coverage
    {
        bool value = material_->GetAlphaToCoverage();
        ui::ItemAlign(ui::GetFrameHeight());
        if (ui::Checkbox("Alpha To Coverage", &value))
        {
            material_->SetAlphaToCoverage(value);
            undo->Add<UndoResourceSetter<Material, bool>>(material_->GetName(), !value, value, &Material::SetAlphaToCoverage);
        }
    }
    // Line Anti Alias
    {
        bool value = material_->GetLineAntiAlias();
        ui::ItemAlign(ui::GetFrameHeight());
        if (ui::Checkbox("Line Anti Alias", &value))
        {
            material_->SetLineAntiAlias(value);
            undo->Add<UndoResourceSetter<Material, bool>>(material_->GetName(), !value, value, &Material::SetLineAntiAlias);
        }
    }
    // Render Order
    {
        ui::PushID("Render Order");
        unsigned char value = material_->GetRenderOrder();
        auto& valuePrev = *ui::GetUIState<unsigned char>(value);
        if (ui::DragScalar("Render Order", ImGuiDataType_U8, &value, 0.01f))
            material_->SetRenderOrder(value);
        if (value != valuePrev && !ui::IsItemActive())
        {
            undo->Add<UndoResourceSetter<Material, unsigned char>>(material_->GetName(), valuePrev, value, &Material::SetRenderOrder);
            valuePrev = value;
        }
        ui::PopID();
    }
    // Occlusion
    {
        bool value = material_->GetOcclusion();
        ui::ItemAlign(ui::GetFrameHeight());
        if (ui::Checkbox("Occlusion", &value))
        {
            material_->SetOcclusion(value);
            undo->Add<UndoResourceSetter<Material, bool>>(material_->GetName(), !value, value, &Material::SetOcclusion);
        }
    }
    // Specular
    {
        bool value = material_->GetSpecular();
        ui::ItemAlign(ui::GetFrameHeight());
        if (ui::Checkbox("Specular", &value))
        {
            material_->SetSpecular(value);
            undo->Add<UndoResourceSetter<Material, bool>>(material_->GetName(), !value, value, &Material::SetSpecular);
        }
    }
    // Constant Bias
    {
        ui::PushID("Constant Bias");
        auto&& setDepthBias = [name=material_->GetName()](Context* context, float value)
        {
            auto* cache = context->GetSubsystem<ResourceCache>();
            if (auto* material = cache->GetResource<Material>(name))
            {
                auto bias = material->GetDepthBias();
                bias.constantBias_ = value;
                material->SetDepthBias(bias);
            }
        };
        if (auto mod = undo->Track<UndoCustomAction<float>>(material_->GetDepthBias().constantBias_, setDepthBias))
            mod.SetModified(ui::DragScalar("Constant Bias", ImGuiDataType_Float, &mod.value_, 0.01f));
        ui::PopID();
    }
    // Slope Scaled Bias
    {
        ui::PushID("Slope Scaled Bias");
        auto&& setSlopeScaledBias = [name=material_->GetName()](Context* context, float value)
        {
            auto* cache = context->GetSubsystem<ResourceCache>();
            if (auto* material = cache->GetResource<Material>(name))
            {
                auto bias = material->GetDepthBias();
                bias.slopeScaledBias_ = value;
                material->SetDepthBias(bias);
            }
        };
        if (auto mod = undo->Track<UndoCustomAction<float>>(material_->GetDepthBias().slopeScaledBias_, setSlopeScaledBias))
            mod.SetModified(ui::DragScalar("Slope Scaled Bias", ImGuiDataType_Float, &mod.value_, 0.01f));
        ui::PopID();
    }
    // Normal Offset
    {
        ui::PushID("Normal Offset");
        auto&& setNormalOffset = [name=material_->GetName()](Context* context, float value)
        {
            auto* cache = context->GetSubsystem<ResourceCache>();
            if (auto* material = cache->GetResource<Material>(name))
            {
                auto bias = material->GetDepthBias();
                bias.normalOffset_ = value;
                material->SetDepthBias(bias);
            }
        };
        if (auto mod = undo->Track<UndoCustomAction<float>>(material_->GetDepthBias().normalOffset_, setNormalOffset))
            mod.SetModified(ui::DragScalar("Normal Offset", ImGuiDataType_Float, &mod.value_, 0.01f));
        ui::PopID();
    }
    // Techniques
    {
        ui::TextCentered("Techniques");
        ui::Separator();

        struct TechniquesCache
        {
            Context* context_;
            StringVector techniques_;
            unsigned lastScan_ = 0;

            explicit TechniquesCache(Context* context) : context_(context) { }

            void Update()
            {
                unsigned now = Time::GetSystemTime();
                if (now - lastScan_ < 1000)
                    return;
                lastScan_ = now;
                auto* cache = context_->GetSubsystem<ResourceCache>();
                techniques_.clear();
                cache->Scan(techniques_, "Techniques", "*.xml", SCAN_FILES, true);
                for (ea::string& name : techniques_)
                    name.insert(0, "Techniques/");
                ea::quick_sort(techniques_.begin(), techniques_.end());
            }
        };

        auto* techniquesCache = ui::GetUIState<TechniquesCache>(context_);
        techniquesCache->Update();

        for (unsigned i = 0; i < material_->GetNumTechniques(); i++)
        {
            if (i > 0)
                ui::Separator();

            ui::IdScope idScope(i);

            auto& tech = material_->GetTechniqueEntry(i);
            auto* techName = ui::GetUIState<ea::string>(tech.technique_->GetName());
            bool modifiedInput = *techName != tech.technique_->GetName();

            // Input
            if (modifiedInput)
                ui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
            bool modified = ui::InputText("##techniqueName", techName, ImGuiInputTextFlags_EnterReturnsTrue);
            if (modifiedInput)
                ui::PopStyleColor();
            ui::SetHelpTooltip("Drag resource here.");
            // Autocomplete
            modified |= ui::Autocomplete(ui::GetID("##techniqueName"), techName, &techniquesCache->techniques_);
            // Drop target
            if (ui::BeginDragDropTarget())
            {
                const Variant& payload = ui::AcceptDragDropVariant("path");
                if (!payload.IsEmpty())
                {
                    *techName = payload.GetString();
                    modified = true;
                }
                ui::EndDragDropTarget();
            }
            // Undo
            if (modified)
            {
                auto* cache = GetSubsystem<ResourceCache>();
                if (auto* technique = cache->GetResource<Technique>(*techName))
                {
                    // Track
                    undo->Add<UndoCustomAction<const ea::string>>(tech.technique_->GetName(), *techName,
                        [name=material_->GetName(), index=i](Context* context, const ea::string& value)
                        {
                            auto* cache = context->GetSubsystem<ResourceCache>();
                            if (auto* material = cache->GetResource<Material>(name))
                            {
                                if (auto* technique = cache->GetResource<Technique>(value))
                                {
                                    const TechniqueEntry& entry = material->GetTechniqueEntry(index);
                                    material->SetTechnique(index, technique, entry.qualityLevel_, entry.lodDistance_);
                                }
                            }
                        });
                    // Update
                    const TechniqueEntry& entry = material_->GetTechniqueEntry(i);
                    material_->SetTechnique(i, technique, entry.qualityLevel_, entry.lodDistance_);
                }
            }
            else if (*techName != tech.technique_->GetName() && !ui::IsItemActive() && !ui::IsItemFocused())
                *techName = tech.technique_->GetName();     // Apply change from undo to current buffer

            // Locate
            ui::SameLine();
            if (ui::IconButton(ICON_FA_CROSSHAIRS))
                SendEvent(E_INSPECTORLOCATERESOURCE, InspectorLocateResource::P_NAME, material_->GetTechnique(i)->GetName());
            ui::SetHelpTooltip("Locate resource");
            // Delete
            ui::SameLine();
            if (ui::IconButton(ICON_FA_TRASH))
            {
                struct TechniqueInfo
                {
                    /// Technique resource name.
                    ea::string name_;
                    /// Quality level.
                    MaterialQuality qualityLevel_;
                    /// LOD distance.
                    float lodDistance_;
                    /// Index in technique stack of material.
                    int index_;

                    TechniqueInfo(Context* context, const TechniqueEntry& entry, int index)
                        : name_(entry.original_->GetName()), qualityLevel_(entry.qualityLevel_)
                        , lodDistance_(entry.lodDistance_), index_(index)
                    {
                    }
                };

                undo->Add<UndoCustomAction<const TechniqueInfo>>(TechniqueInfo{context_, tech, (int)i}, TechniqueInfo{context_, tech, (int)i},
                    [name=material_->GetName()](Context* context, const TechniqueInfo& info)
                    {
                        // Re-add
                        auto* cache = context->GetSubsystem<ResourceCache>();
                        auto* material = cache->GetResource<Material>(name);
                        auto* technique = cache->GetResource<Technique>(info.name_);
                        if (material == nullptr || technique == nullptr)
                            return;

                        material->SetNumTechniques(material->GetNumTechniques() + 1);
                        // Shift techniques towars the end by 1 to make space for insertion.
                        for (int i = material->GetNumTechniques() - 2; i >= info.index_; i--)
                        {
                            const TechniqueEntry& entry = material->GetTechniqueEntry(i);
                            material->SetTechnique(i + 1, entry.original_.Get(), entry.qualityLevel_, entry.lodDistance_);
                        }
                        // Insert back
                        material->SetTechnique(info.index_, technique, info.qualityLevel_, info.lodDistance_);
                    }, [name=material_->GetName()](Context* context, const TechniqueInfo& info)
                    {
                        // Remove again
                        auto* cache = context->GetSubsystem<ResourceCache>();
                        auto* material = cache->GetResource<Material>(name);
                        if (material == nullptr)
                            return;
                        for (int j = info.index_ + 1; j < material->GetNumTechniques(); j++)
                        {
                            const TechniqueEntry& entry = material->GetTechniqueEntry(j);
                            material->SetTechnique(j - 1, entry.original_.Get(), entry.qualityLevel_, entry.lodDistance_);
                        }
                        material->SetNumTechniques(material->GetNumTechniques() - 1);
                    });

                // Technique removed possibly from the middle. Shift existing techniques back to the front and remove last one.
                for (auto j = i + 1; j < material_->GetNumTechniques(); j++)
                {
                    const TechniqueEntry& entry = material_->GetTechniqueEntry(j);
                    material_->SetTechnique(j - 1, entry.original_.Get(), entry.qualityLevel_, entry.lodDistance_);
                }
                material_->SetNumTechniques(material_->GetNumTechniques() - 1);
            }
            // LOD Distance
            auto&& setLodDistance = [name=material_->GetName(), index=i](Context* context, float value)
            {
                auto* cache = context->GetSubsystem<ResourceCache>();
                if (auto* material = cache->GetResource<Material>(name))
                {
                    const TechniqueEntry& entry = material->GetTechniqueEntry(index);
                    material->SetTechnique(index, entry.original_, entry.qualityLevel_, value);
                }
            };
            if (auto mod = undo->Track<UndoCustomAction<float>>(tech.lodDistance_, setLodDistance))
                mod.SetModified(ui::DragFloat("LOD Distance", &mod.value_));
            // Quality
            static const char* qualityNames[] = {"low", "medium", "high", "max"};
            int quality = tech.qualityLevel_;
            if (ui::Combo("Quality", &quality, qualityNames,URHO3D_ARRAYSIZE(qualityNames)))
            {
                undo->Add<UndoCustomAction<const int>>(static_cast<int>(tech.qualityLevel_), quality,
                    [name=material_->GetName(), index=i](Context* context, int value)
                    {
                        auto* cache = context->GetSubsystem<ResourceCache>();
                        if (auto* material = cache->GetResource<Material>(name))
                        {
                            const TechniqueEntry& entry = material->GetTechniqueEntry(index);
                            material->SetTechnique(index, entry.original_, static_cast<MaterialQuality>(value), entry.lodDistance_);
                        }
                    });
            }
        }
        ui::Separator();
        ui::PushID("Add Technique");
        auto* newTechniqueName = ui::GetUIState<ea::string>();
        // New technique name
        ui::SetNextItemWidth(-1);
        bool modified = ui::InputTextWithHint("##Add Technique", "Add Technique", newTechniqueName, ImGuiInputTextFlags_EnterReturnsTrue);
        ui::SetHelpTooltip("Drag and drop technique resource here.");
        // Autocomplete
        modified |= ui::Autocomplete(ui::GetID("##techniqueName"), newTechniqueName, &techniquesCache->techniques_);
        // Drag and drop
        if (ui::BeginDragDropTarget())
        {
            const Variant& payload = ui::AcceptDragDropVariant("path");
            if (!payload.IsEmpty())
            {
                *newTechniqueName = payload.GetString();
                modified = true;
            }
            ui::EndDragDropTarget();
        }
        // Save
        if (modified)
        {
            auto* cache = GetSubsystem<ResourceCache>();
            if (auto* technique = cache->GetResource<Technique>(*newTechniqueName))
            {
                auto index = material_->GetNumTechniques();
                material_->SetNumTechniques(index + 1);
                material_->SetTechnique(index, technique);
                undo->Add<UndoCustomAction<const ea::string>>("", *newTechniqueName,
                    [name=material_->GetName()](Context* context, const ea::string& techniqueName)
                    {
                        auto* cache = context->GetSubsystem<ResourceCache>();
                        if (auto* material = cache->GetResource<Material>(name))
                        {
                            if (techniqueName.empty())  // delete last
                                material->SetNumTechniques(material->GetNumTechniques() - 1);
                            else if (auto* technique = cache->GetResource<Technique>(techniqueName))
                            {
                                auto index = material->GetNumTechniques();
                                material->SetNumTechniques(index + 1);
                                material->SetTechnique(index, technique);
                            }
                        }
                    });
                newTechniqueName->clear();
            }
        }
        ui::PopID();
        ui::Separator();
        ui::Separator();
    }
    // Shader Parameters
    {
        struct ShaderParameterState
        {
            ea::string fieldName_;
            int variantTypeIndex_ = 0;
        };

        ui::TextCentered("Shader parameters");
        ui::Separator();

        auto* paramState = ui::GetUIState<ShaderParameterState>();
        const auto& parameters = material_->GetShaderParameters();
        for (const auto& pair : parameters)
        {
            const ea::string& parameterName = pair.second.name_;
            ui::IdScope id(parameterName.c_str());
            auto& value = ValueHistory<Variant>::Get(pair.second.value_);

            if (ui::Button(ICON_FA_TRASH))
            {
                undo->Add<UndoShaderParameterChanged>(material_, parameterName, pair.second.value_, Variant{});
                material_->RemoveShaderParameter(parameterName);
                break;
            }
            ui::SameLine();

            UI_ITEMWIDTH(w - ui::IconButtonSize() - style.ItemSpacing.x) // Space for OK button
            {
                if (RenderSingleAttribute(value.current_, parameterName.c_str()))
                    material_->SetShaderParameter(parameterName, value.current_);
            }

            if (value.IsModified())
                undo->Add<UndoShaderParameterChanged>(material_, parameterName, value.initial_, value.current_);
        }

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

            ui::SetNextItemWidth(w * 0.3 - style.ItemSpacing.x );
            ui::Combo("###Type", &paramState->variantTypeIndex_, shaderParameterVariantNames, SDL_arraysize(shaderParameterVariantTypes));
            ui::SameLine();
            ui::SetHelpTooltip("Shader parameter type.");

            ui::SetNextItemWidth(w * 0.7 - style.ItemSpacing.x - ui::IconButtonSize());
            bool addNew = ui::InputTextWithHint("###Name", "Parameter name", &paramState->fieldName_, ImGuiInputTextFlags_EnterReturnsTrue);
            ui::SameLine();
            ui::SetHelpTooltip("Shader parameter name.");

            addNew |= ui::Button(ICON_FA_CHECK);
            if (addNew)
            {
                if (!paramState->fieldName_.empty() && material_->GetShaderParameter(paramState->fieldName_.c_str()).GetType() == VAR_NONE)   // TODO: Show warning about duplicate name
                {
                    Variant value{shaderParameterVariantTypes[paramState->variantTypeIndex_]};
                    undo->Add<UndoShaderParameterChanged>(material_, paramState->fieldName_.c_str(), Variant{}, value);
                    material_->SetShaderParameter(paramState->fieldName_.c_str(), value);
                    paramState->fieldName_.clear();
                    paramState->variantTypeIndex_ = 0;
                }
            }

            ui::SameLine();
            ui::TextUnformatted("Add new");
        }
    }

    for (auto i = 0; i < MAX_MATERIAL_TEXTURE_UNITS; i++)
    {
        ea::string finalName = Format("{} Texture", textureUnitNames[i]);
        ReplaceUTF8(finalName, 0, ToUpper(AtUTF8(finalName, 0)));
        auto textureUnit = static_cast<TextureUnit>(i);

        ResourceRef resourceRef;
        if (auto* texture = material_->GetTexture(textureUnit))
            resourceRef = ResourceRef(texture->GetType(), texture->GetName());
        else
            resourceRef = ResourceRef(Texture::GetTypeStatic());

        Variant resource(resourceRef);
        if (RenderSingleAttribute(resource, finalName.c_str()))
        {
            const auto& ref = resource.GetResourceRef();
            auto cache = GetSubsystem<ResourceCache>();
            if (auto texture = dynamic_cast<Texture*>(cache->GetResource(ref.type_, ref.name_)))
                material_->SetTexture(textureUnit, texture);

            undo->Add<UndoCustomAction<const ResourceRef>>(resourceRef, ref,
                [name=material_->GetName(), textureUnit=textureUnit](Context* context, const ResourceRef& ref) {
                    auto cache = context->GetSubsystem<ResourceCache>();
                    if (auto material = cache->GetResource<Material>(name))
                    {
                        if (auto texture = dynamic_cast<Texture*>(cache->GetResource(ref.type_, ref.name_)))
                            material->SetTexture(textureUnit, texture);
                    }
                });
        }
    }
    ui::PopID();    // material_
}

void MaterialInspector::ToggleModel()
{
    const char* currentModel = figures_[figureIndex_];
    SetModel(ToString("Models/%s.mdl", currentModel));
    figureIndex_ = ++figureIndex_ % figures_.size();

    auto* model = node_->GetComponent<StaticModel>();
    model->SetMaterial(material_);

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
    material_->SaveFile(context_->GetCache()->GetResourceFileName(material_->GetName()));
}

void MaterialInspector::RenderPreview()
{
    if (material_.Expired())
        return;

    PreviewInspector::RenderPreview();
    ui::SetHelpTooltip("Click to switch object.");
    if (ui::IsItemHovered() && ui::IsMouseClicked(MOUSEB_LEFT))
        ToggleModel();

    const char* resourceName = material_->GetName().c_str();
    ui::TextCentered(resourceName);
    ui::Separator();
}

}
