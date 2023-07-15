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

#include "../Precompiled.h"

#include "../SystemUI/MaterialInspectorWidget.h"

#include "../Graphics/Texture2D.h"
#include "../Graphics/Texture2DArray.h"
#include "../Graphics/Texture3D.h"
#include "../Graphics/TextureCube.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../SystemUI/SystemUI.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

namespace
{

const ea::unordered_map<ea::string, Variant>& GetDefaultShaderParameterValues(Context* context)
{
    static const auto value = [&]()
    {
        Material material(context);
        ea::unordered_map<ea::string, Variant> result;
        for (const auto& [_, desc] : material.GetShaderParameters())
            result[desc.name_] = desc.value_;
        return result;
    }();
    return value;
}

bool IsDefaultValue(Context* context, const ea::string& name, const Variant& value)
{
    const auto& defaultValues = GetDefaultShaderParameterValues(context);
    const auto iter = defaultValues.find(name);
    return iter != defaultValues.end() && iter->second == value;
}

const ea::pair<ea::string, Variant> shaderParameterTypes[] = {
    {"vec4 or rgba", Variant(Color::WHITE.ToVector4())},
    {"vec3 or rgb", Variant(Vector3::ZERO)},
    {"vec2", Variant(Vector2::ZERO)},
    {"float", Variant(0.0f)},
};

const StringVector cullModes{"Cull None", "Cull Back Faces", "Cull Front Faces"};

const StringVector fillModes{"Solid", "Wireframe", "Points"};

}

const ea::vector<MaterialInspectorWidget::TextureUnitDesc> MaterialInspectorWidget::textureUnits{
    {false, TU_DIFFUSE,     "Albedo",       "TU_DIFFUSE: Albedo map or Diffuse texture with optional alpha channel"},
    {false, TU_NORMAL,      "Normal",       "TU_NORMAL: Normal map"},
    {false, TU_SPECULAR,    "Specular",     "TU_SPECULAR: Metallic-Roughness-Occlusion map or Specular texture"},
    {false, TU_EMISSIVE,    "Emissive",     "TU_EMISSIVE: Emissive map or light map"},
    {false, TU_ENVIRONMENT, "Environment",  "TU_ENVIRONMENT: Texture with environment reflection"},
    {true,  TU_VOLUMEMAP,   "* Volume",     "TU_VOLUMEMAP: Desktop only, custom unit"},
    {true,  TU_CUSTOM1,     "* Custom 1",   "TU_CUSTOM1: Desktop only, custom unit"},
    {true,  TU_CUSTOM2,     "* Custom 2",   "TU_CUSTOM2: Desktop only, custom unit"},
};

const ea::vector<MaterialInspectorWidget::PropertyDesc> MaterialInspectorWidget::properties{
    {
        "Vertex Defines",
        Variant{EMPTY_STRING},
        [](const Material* material) { return Variant{material->GetVertexShaderDefines()}; },
        [](Material* material, const Variant& value) { material->SetVertexShaderDefines(value.GetString()); },
        "Additional shader defines applied to vertex shader. Should be space-separated list of DEFINES. Example: VOLUMETRIC SOFTPARTICLES",
    },
    {
        "Pixel Defines",
        Variant{EMPTY_STRING},
        [](const Material* material) { return Variant{material->GetPixelShaderDefines()}; },
        [](Material* material, const Variant& value) { material->SetPixelShaderDefines(value.GetString()); },
        "Additional shader defines applied to pixel shader. Should be space-separated list of DEFINES. Example: VOLUMETRIC SOFTPARTICLES",
    },

    {
        "Cull Mode",
        Variant{CULL_CCW},
        [](const Material* material) { return Variant{static_cast<int>(material->GetCullMode())}; },
        [](Material* material, const Variant& value) { material->SetCullMode(static_cast<CullMode>(value.GetInt())); },
        "Cull mode used to render primary geometry with this material",
        Widgets::EditVariantOptions{}.Enum(cullModes),
    },
    {
        "Shadow Cull Mode",
        Variant{CULL_CCW},
        [](const Material* material) { return Variant{static_cast<int>(material->GetShadowCullMode())}; },
        [](Material* material, const Variant& value) { material->SetShadowCullMode(static_cast<CullMode>(value.GetInt())); },
        "Cull mode used to render shadow geometry with this material",
        Widgets::EditVariantOptions{}.Enum(cullModes),
    },
    {
        "Fill Mode",
        Variant{FILL_SOLID},
        [](const Material* material) { return Variant{static_cast<int>(material->GetFillMode())}; },
        [](Material* material, const Variant& value) { material->SetFillMode(static_cast<FillMode>(value.GetInt())); },
        "Geometry fill mode. Mobiles support only Solid fill mode!",
        Widgets::EditVariantOptions{}.Enum(fillModes),
    },

    {
        "Alpha To Coverage",
        Variant{false},
        [](const Material* material) { return Variant{material->GetAlphaToCoverage()}; },
        [](Material* material, const Variant& value) { material->SetAlphaToCoverage(value.GetBool()); },
        "Whether to treat output alpha as MSAA coverage. It can be used by custom shaders for antialiased alpha cutout.",
    },
    {
        "Line Anti Alias",
        Variant{false},
        [](const Material* material) { return Variant{material->GetLineAntiAlias()}; },
        [](Material* material, const Variant& value) { material->SetLineAntiAlias(value.GetBool()); },
        "Whether to enable alpha-based line anti-aliasing for materials applied to line geometry",
    },
    {
        "Render Order",
        Variant{static_cast<int>(DEFAULT_RENDER_ORDER)},
        [](const Material* material) { return Variant{static_cast<int>(material->GetRenderOrder())}; },
        [](Material* material, const Variant& value) { material->SetRenderOrder(static_cast<unsigned char>(value.GetInt())); },
        "Global render order of the material. Materials with lower order are rendered first.",
        Widgets::EditVariantOptions{}.Range(0, 255),
    },
    {
        "Occlusion",
        Variant{true},
        [](const Material* material) { return Variant{material->GetOcclusion()}; },
        [](Material* material, const Variant& value) { material->SetOcclusion(value.GetBool()); },
        "Whether to render geometry with this material to occlusion buffer",
    },

    {
        "Constant Bias",
        Variant{0.0f},
        [](const Material* material) { return Variant{material->GetDepthBias().constantBias_}; },
        [](Material* material, const Variant& value) { auto temp = material->GetDepthBias(); temp.constantBias_ = value.GetFloat(); material->SetDepthBias(temp); },
        "Constant value added to pixel depth affecting geometry visibility behind or in front of obstacles",
        Widgets::EditVariantOptions{}.Step(0.000001).Range(-1.0f, 1.0f),
    },
    {
        "Slope Scaled Bias",
        Variant{0.0f},
        [](const Material* material) { return Variant{material->GetDepthBias().slopeScaledBias_}; },
        [](Material* material, const Variant& value) { auto temp = material->GetDepthBias(); temp.slopeScaledBias_ = value.GetFloat(); material->SetDepthBias(temp); },
        "You probably don't want to change this",
    },
    {
        "Normal Offset",
        Variant{0.0f},
        [](const Material* material) { return Variant{material->GetDepthBias().normalOffset_}; },
        [](Material* material, const Variant& value) { auto temp = material->GetDepthBias(); temp.normalOffset_ = value.GetFloat(); material->SetDepthBias(temp); },
        "You probably don't want to change this",
    },
};

bool MaterialInspectorWidget::TechniqueDesc::operator<(const TechniqueDesc& rhs) const
{
    return ea::tie(deprecated_, displayName_) < ea::tie(rhs.deprecated_, rhs.displayName_);
}

MaterialInspectorWidget::MaterialInspectorWidget(Context* context, const MaterialVector& materials)
    : Object(context)
    , materials_(materials)
{
    URHO3D_ASSERT(!materials_.empty());
}

MaterialInspectorWidget::~MaterialInspectorWidget()
{
}

void MaterialInspectorWidget::UpdateTechniques(const ea::string& path)
{
    auto cache = GetSubsystem<ResourceCache>();
    StringVector techniques;
    cache->Scan(techniques, path, "*.xml", SCAN_FILES | SCAN_RECURSIVE);

    techniques_.clear();
    sortedTechniques_.clear();

    for (const ea::string& relativeName : techniques)
    {
        auto desc = ea::make_shared<TechniqueDesc>();
        desc->resourceName_ = AddTrailingSlash(path) + relativeName;
        desc->displayName_ = relativeName.substr(0, relativeName.size() - 4);
        desc->technique_ = cache->GetResource<Technique>(desc->resourceName_);
        desc->deprecated_ = IsTechniqueDeprecated(desc->resourceName_);
        if (desc->technique_)
        {
            techniques_[desc->resourceName_] = desc;
            sortedTechniques_.push_back(desc);
        }
    }

    ea::sort(sortedTechniques_.begin(), sortedTechniques_.end(),
        [](const TechniqueDescPtr& lhs, const TechniqueDescPtr& rhs) { return *lhs < *rhs; });

    defaultTechnique_ = nullptr;
    if (const auto iter = techniques_.find(defaultTechniqueName_); iter != techniques_.end())
        defaultTechnique_ = iter->second;
    else if (!sortedTechniques_.empty())
    {
        URHO3D_LOGWARNING("Could not find default technique '{}'", defaultTechniqueName_);
        defaultTechnique_ = sortedTechniques_.front();
    }
}

void MaterialInspectorWidget::RenderTitle()
{
    if (materials_.size() == 1)
        ui::Text("%s", materials_[0]->GetName().c_str());
    else
        ui::Text("%d materials", materials_.size());
}

void MaterialInspectorWidget::RenderContent()
{
    pendingSetTechniques_ = false;
    pendingSetTextures_.clear();
    pendingSetShaderParameters_.clear();
    pendingSetProperties_.clear();

    RenderTechniques();
    RenderProperties();
    RenderTextures();
    RenderShaderParameters();

    if (pendingSetTechniques_)
    {
        OnEditBegin(this);
        for (Material* material : materials_)
            material->SetTechniques(techniqueEntries_);
        OnEditEnd(this);
    }

    if (!pendingSetTextures_.empty())
    {
        OnEditBegin(this);
        for (Material* material : materials_)
        {
            for (const auto& [unit, texture] : pendingSetTextures_)
                material->SetTexture(unit, texture);
        }
        OnEditEnd(this);
    }

    if (!pendingSetShaderParameters_.empty())
    {
        OnEditBegin(this);
        for (Material* material : materials_)
        {
            for (const auto& [name, value] : pendingSetShaderParameters_)
            {
                if (!value.IsEmpty())
                    material->SetShaderParameter(name, value);
                else
                    material->RemoveShaderParameter(name);
            }
        }
        OnEditEnd(this);
    }

    if (!pendingSetProperties_.empty())
    {
        OnEditBegin(this);
        for (Material* material : materials_)
        {
            for (const auto& [desc, value] : pendingSetProperties_)
                desc->setter_(material, value);
        }
        OnEditEnd(this);
    }
}

void MaterialInspectorWidget::RenderTechniques()
{
    const IdScopeGuard guard("RenderTechniques");

    const auto& currentTechniqueEntries = materials_[0]->GetTechniques();
    if (currentTechniqueEntries != sortedTechniqueEntries_)
    {
        techniqueEntries_ = currentTechniqueEntries;
        sortedTechniqueEntries_ = currentTechniqueEntries;
        ea::sort(sortedTechniqueEntries_.begin(), sortedTechniqueEntries_.end());
    }

    const bool isUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](Material* material) { return sortedTechniqueEntries_ != material->GetTechniques(); });

    const char* title = !isUndefined ? "Techniques" : "Techniques (different for selected materials)";
    if (!ui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ui::BeginDisabled(isUndefined);
    if (RenderTechniqueEntries())
        pendingSetTechniques_ = true;
    ui::EndDisabled();

    if (!!isUndefined)
    {
        ui::SameLine();
        if (ui::Button(ICON_FA_CODE_MERGE))
            pendingSetTechniques_ = true;
        if (ui::IsItemHovered())
            ui::SetTooltip("Override all materials' techniques and enable editing");
    }

    ui::Separator();
}

bool MaterialInspectorWidget::RenderTechniqueEntries()
{
    const float availableWidth = ui::GetContentRegionAvail().x;

    ea::optional<unsigned> pendingDelete;
    bool modified = false;
    for (unsigned entryIndex = 0; entryIndex < techniqueEntries_.size(); ++entryIndex)
    {
        const IdScopeGuard guard(entryIndex);

        TechniqueEntry& entry = techniqueEntries_[entryIndex];

        if (EditTechniqueInEntry(entry, availableWidth))
            modified = true;

        if (ui::Button(ICON_FA_TRASH_CAN))
            pendingDelete = entryIndex;
        if (ui::IsItemHovered())
            ui::SetTooltip("Remove technique from material(s)");
        ui::SameLine();

        if (EditDistanceInEntry(entry, availableWidth * 0.5f))
            modified = true;
        ui::SameLine();

        if (EditQualityInEntry(entry))
            modified = true;
    }

    // Remove entry
    if (pendingDelete && *pendingDelete < techniqueEntries_.size())
    {
        techniqueEntries_.erase_at(*pendingDelete);

        modified = true;
    }

    // Add new entry
    if (defaultTechnique_ && ui::Button(ICON_FA_SQUARE_PLUS))
    {
        TechniqueEntry& entry = techniqueEntries_.emplace_back();
        entry.technique_ = entry.original_ = defaultTechnique_->technique_;

        modified = true;
    }
    if (ui::IsItemHovered())
        ui::SetTooltip("Add new technique to the material(s)");

    sortedTechniqueEntries_ = techniqueEntries_;
    ea::sort(sortedTechniqueEntries_.begin(), sortedTechniqueEntries_.end());
    return modified;
}

bool MaterialInspectorWidget::EditTechniqueInEntry(TechniqueEntry& entry, float itemWidth)
{
    bool modified = false;

    const ea::string currentTechnique = GetTechniqueDisplayName(entry.technique_->GetName());

    ui::SetNextItemWidth(itemWidth);
    if (ui::BeginCombo("##Technique", currentTechnique.c_str(), ImGuiComboFlags_HeightLarge))
    {
        bool wasDeprecated = false;
        for (unsigned techniqueIndex = 0; techniqueIndex < sortedTechniques_.size(); ++techniqueIndex)
        {
            const TechniqueDesc& desc = *sortedTechniques_[techniqueIndex];

            const IdScopeGuard guard(techniqueIndex);

            if (desc.deprecated_ && !wasDeprecated)
            {
                ui::Separator();
                wasDeprecated = true;
            }

            const ColorScopeGuard guardTextColor{ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.0f, 1.0f}, !desc.deprecated_};

            if (ui::Selectable(desc.displayName_.c_str(), entry.technique_ == desc.technique_))
            {
                entry.technique_ = entry.original_ = desc.technique_;
                modified = true;
            }
        }
        ui::EndCombo();
    }

    if (ui::IsItemHovered())
        ui::SetTooltip("Technique description from \"Techniques/*.xml\"");

    return modified;
}

bool MaterialInspectorWidget::EditDistanceInEntry(TechniqueEntry& entry, float itemWidth)
{
    bool modified = false;

    ui::SetNextItemWidth(itemWidth);
    if (ui::DragFloat("##Distance", &entry.lodDistance_, 1.0f, 0.0f, 1000.0f, "%.1f"))
        modified = true;
    ui::SameLine();

    if (ui::IsItemHovered())
        ui::SetTooltip("Minimum distance to the object at which the technique is used. Lower distances have higher priority.");

    return modified;
}

bool MaterialInspectorWidget::EditQualityInEntry(TechniqueEntry& entry)
{
    static const StringVector qualityLevels{"Q Low", "Q Medium", "Q High", "Q Max"};

    bool modified = false;

    const auto qualityLevel = ea::min(static_cast<eastl_size_t>(entry.qualityLevel_), qualityLevels.size() - 1);
    if (ui::BeginCombo("##Quality", qualityLevels[qualityLevel].c_str()))
    {
        for (unsigned qualityLevelIndex = 0; qualityLevelIndex < qualityLevels.size(); ++qualityLevelIndex)
        {
            const IdScopeGuard guard(qualityLevelIndex);
            if (ui::Selectable(qualityLevels[qualityLevelIndex].c_str(), qualityLevel == qualityLevelIndex))
            {
                entry.qualityLevel_ = static_cast<MaterialQuality>(qualityLevelIndex);
                if (entry.qualityLevel_ > QUALITY_HIGH)
                    entry.qualityLevel_ = QUALITY_MAX;
                modified = true;
            }
        }
        ui::EndCombo();
    }

    if (ui::IsItemHovered())
        ui::SetTooltip("Techniques with higher quality will not be used if lower quality is selected in the RenderPipeline settings");

    return modified;
}

ea::string MaterialInspectorWidget::GetTechniqueDisplayName(const ea::string& resourceName) const
{
    const auto iter = techniques_.find(resourceName);
    if (iter != techniques_.end())
        return iter->second->displayName_;
    return "";
}

bool MaterialInspectorWidget::IsTechniqueDeprecated(const ea::string& resourceName) const
{
    return resourceName.starts_with("Techniques/PBR/")
        || resourceName.starts_with("Techniques/Diff")
        || resourceName.starts_with("Techniques/NoTexture")
        || resourceName == "Techniques/BasicVColUnlitAlpha.xml"
        || resourceName == "Techniques/TerrainBlend.xml"
        || resourceName == "Techniques/VegetationDiff.xml"
        || resourceName == "Techniques/VegetationDiffUnlit.xml"
        || resourceName == "Techniques/Water.xml";
}

void MaterialInspectorWidget::RenderProperties()
{
    const IdScopeGuard guard("RenderProperties");

    if (!ui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (const PropertyDesc& property : properties)
        RenderProperty(property);

    ui::Separator();
}

void MaterialInspectorWidget::RenderProperty(const PropertyDesc& desc)
{
    const IdScopeGuard guard(desc.name_.c_str());

    Variant value = desc.getter_(materials_[0]);
    const bool isUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return value != desc.getter_(material); });

    Widgets::ItemLabel(desc.name_, Widgets::GetItemLabelColor(isUndefined, value == desc.defaultValue_));
    if (!desc.hint_.empty() && ui::IsItemHovered())
        ui::SetTooltip("%s", desc.hint_.c_str());

    const ColorScopeGuard guardBackgroundColor{ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(isUndefined), isUndefined};

    if (Widgets::EditVariant(value, desc.options_))
        pendingSetProperties_.emplace_back(&desc, value);
}

void MaterialInspectorWidget::RenderTextures()
{
    const IdScopeGuard guard("RenderTextures");

    if (!ui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (const TextureUnitDesc& desc : textureUnits)
        RenderTextureUnit(desc);

    ui::Separator();
}

void MaterialInspectorWidget::RenderTextureUnit(const TextureUnitDesc& desc)
{
    const IdScopeGuard guard(desc.unit_);

    auto cache = GetSubsystem<ResourceCache>();

    Texture* texture = materials_[0]->GetTexture(desc.unit_);
    const bool isUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return material->GetTexture(desc.unit_) != texture; });

    Widgets::ItemLabel(desc.name_, Widgets::GetItemLabelColor(isUndefined, texture == nullptr));
    if (ui::IsItemHovered())
        ui::SetTooltip("%s", desc.hint_.c_str());

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingSetTextures_.emplace_back(desc.unit_, nullptr);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove texture from this unit");
    ui::SameLine();

    const ColorScopeGuard guardBackgroundColor{ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(isUndefined), isUndefined};

    static const StringVector allowedTextureTypes{
        Texture2D::GetTypeNameStatic(),
        Texture2DArray::GetTypeNameStatic(),
        TextureCube::GetTypeNameStatic(),
        Texture3D::GetTypeNameStatic(),
    };

    StringHash textureType = texture ? texture->GetType() : Texture2D::GetTypeStatic();
    ea::string textureName = texture ? texture->GetName() : "";
    if (Widgets::EditResourceRef(textureType, textureName, &allowedTextureTypes))
    {
        if (const auto texture = dynamic_cast<Texture*>(cache->GetResource(textureType, textureName)))
            pendingSetTextures_.emplace_back(desc.unit_, texture);
        else
            pendingSetTextures_.emplace_back(desc.unit_, nullptr);
    }
}

void MaterialInspectorWidget::RenderShaderParameters()
{
    const IdScopeGuard guard("RenderShaderParameters");

    if (!ui::CollapsingHeader("Shader Parameters", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    shaderParameterNames_ = GetShaderParameterNames();
    for (const auto& name : shaderParameterNames_)
        RenderShaderParameter(name);
    ui::Separator();

    RenderNewShaderParameter();
    ui::Separator();
}

MaterialInspectorWidget::ShaderParameterNames MaterialInspectorWidget::GetShaderParameterNames() const
{
    ShaderParameterNames names;
    for (const auto& material : materials_)
    {
        const auto& shaderParameters = material->GetShaderParameters();
        for (const auto& [_, desc] : shaderParameters)
            names.insert(desc.name_);
    }
    return names;
}

void MaterialInspectorWidget::RenderShaderParameter(const ea::string& name)
{
    const IdScopeGuard guard(name.c_str());

    const auto materialIter = ea::find_if(materials_.begin(), materials_.end(),
        [&](const Material* material) { return !material->GetShaderParameter(name).IsEmpty(); });
    if (materialIter == materials_.end())
        return;

    Variant value = (*materialIter)->GetShaderParameter(name);
    const bool isUndefined = ea::any_of(materials_.begin(), materials_.end(),
        [&](const Material* material) { return material->GetShaderParameter(name) != value; });

    Widgets::ItemLabel(name, Widgets::GetItemLabelColor(isUndefined, IsDefaultValue(context_, name, value)));

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingSetShaderParameters_.emplace_back(name, Variant::EMPTY);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove this parameter");
    ui::SameLine();

    if (ui::Button(ICON_FA_LIST))
        ui::OpenPopup("##ShaderParameterPopup");
    if (ui::IsItemHovered())
        ui::SetTooltip("Shader parameter type which should strictly match the type in shader");

    if (ui::BeginPopup("##ShaderParameterPopup"))
    {
        for (const auto& [label, defaultValue] : shaderParameterTypes)
        {
            if (ui::MenuItem(label.c_str()))
                pendingSetShaderParameters_.emplace_back(name, defaultValue);
        }
        ui::EndPopup();
    }
    ui::SameLine();

    const ColorScopeGuard guardBackgroundColor{ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(isUndefined), isUndefined};

    Widgets::EditVariantOptions options;
    options.asColor_ = name.contains("Color", false);
    if (Widgets::EditVariant(value, options))
        pendingSetShaderParameters_.emplace_back(name, value);
}

void MaterialInspectorWidget::RenderNewShaderParameter()
{
    ui::Text("Add parameter:");
    if (ui::IsItemHovered())
        ui::SetTooltip("Add new parameter for all selected materials");
    ui::SameLine();

    const float width = ui::GetContentRegionAvail().x;
    bool addNewParameter = false;
    ui::SetNextItemWidth(width * 0.5f);
    if (ui::InputText("##Name", &newParameterName_, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CharsNoBlank))
        addNewParameter = true;
    if (ui::IsItemHovered())
        ui::SetTooltip("Unique parameter name, should be valid GLSL identifier");

    ui::SameLine();
    ui::SetNextItemWidth(width * 0.3f);
    if (ui::BeginCombo("##Type", shaderParameterTypes[newParameterType_].first.c_str(), ImGuiComboFlags_HeightSmall))
    {
        unsigned index = 0;
        for (const auto& [label, _] : shaderParameterTypes)
        {
            if (ui::Selectable(label.c_str(), newParameterType_ == index))
                newParameterType_ = index;
            ++index;
        }
        ui::EndCombo();
    }
    if (ui::IsItemHovered())
        ui::SetTooltip("Shader parameter type which should strictly match the type in shader");

    ui::SameLine();
    const bool canAddParameter = !newParameterName_.empty() && !shaderParameterNames_.contains(newParameterName_);
    ui::BeginDisabled(!canAddParameter);
    if (ui::Button(ICON_FA_SQUARE_PLUS))
        addNewParameter = true;
    if (ui::IsItemHovered())
        ui::SetTooltip("Add parameter '%s' of type '%s'", newParameterName_.c_str(), shaderParameterTypes[newParameterType_].first.c_str());
    ui::EndDisabled();

    if (addNewParameter && canAddParameter)
        pendingSetShaderParameters_.emplace_back(newParameterName_, shaderParameterTypes[newParameterType_].second);
}

}
