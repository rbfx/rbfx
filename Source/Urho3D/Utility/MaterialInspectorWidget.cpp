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

#include "../Utility/MaterialInspectorWidget.h"

#if URHO3D_SYSTEMUI

#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../SystemUI/SystemUI.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

// TODO(editor): Extract to Widgets.h/cpp
#if 0
void ComboEx(const char* id, const StringVector& items, const ea::function<ea::optional<int>()>& getter, const ea::function<void(int)>& setter)
{
    const auto currentValue = getter();
    const char* currentValueLabel = currentValue && *currentValue < items.size() ? items[*currentValue].c_str() : "";
    if (!ui::BeginCombo(id, currentValueLabel))
        return;

    for (int index = 0; index < items.size(); ++index)
    {
        const char* label = items[index].c_str();
        if (ui::Selectable(label, currentValue && *currentValue == index))
            setter(index);
    }

    ui::EndCombo();
}

template <class ObjectType, class ElementType>
using GetterCallback = ea::function<ElementType(const ObjectType*)>;

template <class ObjectType, class ElementType>
using SetterCallback = ea::function<void(ObjectType*, const ElementType&)>;

template <class ObjectType, class ElementType>
GetterCallback<ObjectType, ElementType> MakeGetterCallback(ElementType (ObjectType::*getter)() const)
{
    return [getter](const ObjectType* object) { return (object->*getter)(); };
}

template <class ObjectType, class ElementType>
SetterCallback<ObjectType, ElementType> MakeSetterCallback(void (ObjectType::*setter)(ElementType))
{
    return [setter](ObjectType* object, const ElementType& value) { (object->*setter)(value); };
}

template <class ContainerType, class GetterCallbackType, class SetterCallbackType>
void ComboZip(const char* id, const StringVector& items, ContainerType& container, const GetterCallbackType& getter, const SetterCallbackType& setter)
{
    using ElementType = decltype(getter(container.front()));

    const auto wrappedGetter = [&]() -> ea::optional<int>
    {
        ea::optional<int> result;
        for (const auto& object : container)
        {
            const auto value = static_cast<int>(getter(object));
            if (!result)
                result = value;
            else if (*result != value)
                return ea::nullopt;
        }
        return result;
    };

    const auto wrappedSetter = [&](int value)
    {
        for (const auto& object : container)
            setter(object, static_cast<ElementType>(value));
    };

    ComboEx(id, items, wrappedGetter, wrappedSetter);
}
#endif

namespace
{

Color GetLabelColor(bool canEdit, bool defaultValue)
{
    const auto& style = ui::GetStyle();
    if (!canEdit)
        return ToColor(style.Colors[ImGuiCol_TextDisabled]);
    else if (defaultValue)
        return {0.85f, 0.85f, 0.85f, 1.0f};
    else
        return {1.0f, 1.0f, 0.75f, 1.0f};
}

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

const MaterialTextureUnit materialUnits[] = {
    {false, TU_DIFFUSE,     "Albedo",       "TU_DIFFUSE: Albedo map or Diffuse texture with optional alpha channel"},
    {false, TU_NORMAL,      "Normal",       "TU_NORMAL: Normal map"},
    {false, TU_SPECULAR,    "Specular",     "TU_SPECULAR: Metallic-Roughness-Occlusion map or Specular texture"},
    {false, TU_EMISSIVE,    "Emissive",     "TU_EMISSIVE: Emissive map or light map"},
    {false, TU_ENVIRONMENT, "Environment",  "TU_ENVIRONMENT: Texture with environment reflection"},
#ifdef DESKTOP_GRAPHICS
    {true,  TU_VOLUMEMAP,   "* Volume",     "TU_VOLUMEMAP: Desktop only, custom unit"},
    {true,  TU_CUSTOM1,     "* Custom 1",   "TU_CUSTOM1: Desktop only, custom unit"},
    {true,  TU_CUSTOM2,     "* Custom 2",   "TU_CUSTOM2: Desktop only, custom unit"},
#endif
};

const ea::pair<ea::string, Variant> shaderParameterTypes[] = {
    {"vec4 or rgba", Variant(Color::WHITE.ToVector4())},
    {"vec3 or rgb", Variant(Vector3::ZERO)},
    {"vec2", Variant(Vector2::ZERO)},
    {"float", Variant(0.0f)},
};

struct EditVariantOptions
{
    float step_{0.1f};
    float min_{0.0f};
    float max_{0.0f};
    bool asColor_{};
};

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
    if (ui::DragFloat("", &value, options.step_, options.min_, options.max_, "%.3f"))
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
    if (ui::DragFloat2("", &value.x_, options.step_, options.min_, options.max_, "%.3f"))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantVector3(Variant& var, const EditVariantOptions& options)
{
    if (options.asColor_)
        return EditVariantColor(var, options);

    Vector3 value = var.GetVector3();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat3("", &value.x_, options.step_, options.min_, options.max_, "%.3f"))
    {
        var = value;
        return true;
    }
    return false;
}

bool EditVariantVector4(Variant& var, const EditVariantOptions& options)
{
    if (options.asColor_)
        return EditVariantColor(var, options);

    Vector4 value = var.GetVector4();
    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    if (ui::DragFloat4("", &value.x_, options.step_, options.min_, options.max_, "%.3f"))
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
    case VAR_FLOAT:
        return EditVariantFloat(var, options);

    case VAR_VECTOR2:
        return EditVariantVector2(var, options);

    case VAR_VECTOR3:
        return EditVariantVector3(var, options);

    case VAR_VECTOR4:
        return EditVariantVector4(var, options);

    case VAR_COLOR:
        return EditVariantColor(var, options);

    default:
        ui::Button("TODO: Implement");
        return false;
    }
}

}

bool MaterialInspectorWidget::CachedTechnique::operator<(const CachedTechnique& rhs) const
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
    cache->Scan(techniques, path, "*.xml", SCAN_FILES, true);

    techniques_.clear();
    sortedTechniques_.clear();

    for (const ea::string& relativeName : techniques)
    {
        auto desc = ea::make_shared<CachedTechnique>();
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
        [](const CachedTechniquePtr& lhs, const CachedTechniquePtr& rhs) { return *lhs < *rhs; });

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

    RenderTechniques();
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

#if 0
    static const StringVector cullModes{"Cull None", "Cull Back Faces", "Cull Front Faces"};
    static const StringVector fillModes{"Solid", "Wireframe", "Points"};

    Widgets::ItemLabel("Cull Mode");
    ComboZip("##CullMode", cullModes, materials_, MakeGetterCallback(&Material::GetCullMode), MakeSetterCallback(&Material::SetCullMode));

    Widgets::ItemLabel("Shadow Cull Mode");
    ComboZip("##ShadowCullMode", cullModes, materials_, MakeGetterCallback(&Material::GetShadowCullMode), MakeSetterCallback(&Material::SetShadowCullMode));

    Widgets::ItemLabel("Fill Mode");
    ComboZip("##FillMode", fillModes, materials_, MakeGetterCallback(&Material::GetFillMode), MakeSetterCallback(&Material::SetFillMode));
#endif
}

void MaterialInspectorWidget::RenderTechniques()
{
    const IdScopeGuard guard("RenderTechniques");

    const auto& currentTechniqueEntries = materials_[0]->GetTechniques();
    if (currentTechniqueEntries != sortedTechniqueEntries_)
        techniqueEntries_ = currentTechniqueEntries;

    const bool canEdit = ea::all_of(materials_.begin() + 1, materials_.end(),
        [&](Material* material) { return sortedTechniqueEntries_ == material->GetTechniques(); });

    const char* title = canEdit ? "Techniques" : "Techniques (different for selected materials)";
    if (!ui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen))
        return;

    ui::BeginDisabled(!canEdit);
    if (RenderTechniqueEntries())
        pendingSetTechniques_ = true;
    ui::EndDisabled();

    if (!canEdit)
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
            const CachedTechnique& desc = *sortedTechniques_[techniqueIndex];

            const IdScopeGuard guard(techniqueIndex);

            if (desc.deprecated_ && !wasDeprecated)
            {
                ui::Separator();
                wasDeprecated = true;
            }

            if (!desc.deprecated_)
                ui::PushStyleColor(ImGuiCol_Text, ImVec4{0.3f, 1.0f, 0.0f, 1.0f});

            if (ui::Selectable(desc.displayName_.c_str(), entry.technique_ == desc.technique_))
            {
                entry.technique_ = entry.original_ = desc.technique_;
                modified = true;
            }

            if (!desc.deprecated_)
                ui::PopStyleColor();
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

void MaterialInspectorWidget::RenderTextures()
{
    const IdScopeGuard guard("RenderTextures");

    if (!ui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (const MaterialTextureUnit& desc : materialUnits)
    {
        const IdScopeGuard guard(desc.unit_);
        RenderTextureUnit(desc);
    }

    ui::Separator();
}

void MaterialInspectorWidget::RenderTextureUnit(const MaterialTextureUnit& desc)
{
    auto cache = GetSubsystem<ResourceCache>();

    Texture* texture = materials_[0]->GetTexture(desc.unit_);
    const bool canEdit = ea::all_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return material->GetTexture(desc.unit_) == texture; });

    Widgets::ItemLabel(desc.name_, GetLabelColor(canEdit, texture == nullptr));
    if (ui::IsItemHovered())
        ui::SetTooltip(desc.hint_.c_str());

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingSetTextures_.emplace_back(desc.unit_, nullptr);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove texture from this unit");
    ui::SameLine();

    if (!canEdit)
    {
        if (ui::Button(ICON_FA_CODE_MERGE))
            pendingSetTextures_.emplace_back(desc.unit_, texture);
        if (ui::IsItemHovered())
            ui::SetTooltip("Override this unit for all materials and enable editing");
        ui::SameLine();
    }

    ui::BeginDisabled(!canEdit);

    ea::string textureName = texture ? texture->GetName() : canEdit ? "" : "???";
    if (ui::InputText("##Texture", &textureName, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        if (textureName.empty())
            pendingSetTextures_.emplace_back(desc.unit_, nullptr);
        else if (!textureName.ends_with(".xml"))
        {
            if (const auto texture = cache->GetResource<Texture2D>(textureName))
                pendingSetTextures_.emplace_back(desc.unit_, texture);
        }
        else
        {
            if (const auto texture = cache->GetResource<TextureCube>(textureName))
                pendingSetTextures_.emplace_back(desc.unit_, texture);
            // TODO(editor): Support Texture3D and TextureArray
        }
    }
    ui::EndDisabled();
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

    Variant value = materials_[0]->GetShaderParameter(name);
    const bool canEdit = ea::all_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return material->GetShaderParameter(name) == value; });

    Widgets::ItemLabel(name, GetLabelColor(canEdit, IsDefaultValue(context_, name, value)));

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingSetShaderParameters_.emplace_back(name, Variant::EMPTY);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove this parameter");
    ui::SameLine();

    if (!canEdit)
    {
        if (ui::Button(ICON_FA_CODE_MERGE))
            pendingSetShaderParameters_.emplace_back(name, value);
        if (ui::IsItemHovered())
            ui::SetTooltip("Override this parameter for all materials and enable editing");
        ui::SameLine();
    }
    else
    {
        if (ui::Button(ICON_FA_LIST))
            ui::OpenPopup("##ShaderParameterPopup");
        if (ui::IsItemHovered())
            ui::SetTooltip("Select shader parameter type");

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
    }

    ui::BeginDisabled(!canEdit);

    EditVariantOptions editOptions;
    editOptions.asColor_ = name.contains("Color", false);
    if (EditVariant(value, editOptions))
        pendingSetShaderParameters_.emplace_back(name, value);

    ui::EndDisabled();
}

void MaterialInspectorWidget::RenderNewShaderParameter()
{
    ui::Text("Add parameter:");
    ui::SameLine();

    const float width = ui::GetContentRegionAvail().x;
    bool addNewParameter = false;
    ui::SetNextItemWidth(width * 0.5f);
    if (ui::InputText("##Name", &newParameterName_, ImGuiInputTextFlags_EnterReturnsTrue))
        addNewParameter = true;

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

    ui::SameLine();
    const bool canAddParameter = !newParameterName_.empty() && !shaderParameterNames_.contains(newParameterName_);
    ui::BeginDisabled(!canAddParameter);
    if (ui::Button(ICON_FA_SQUARE_PLUS))
        addNewParameter = true;
    ui::EndDisabled();

    if (addNewParameter && canAddParameter)
        pendingSetShaderParameters_.emplace_back(newParameterName_, shaderParameterTypes[newParameterType_].second);
}

}

#endif
