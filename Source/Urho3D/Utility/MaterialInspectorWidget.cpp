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

#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../SystemUI/SystemUI.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

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
    pendingChange_ = false;

    RenderTechniques();

    if (pendingChange_)
    {
        OnEditBegin(this);
        for (Material* material : materials_)
            material->SetTechniques(techniqueEntries_);
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
    ui::Text("Techniques");

    const auto& currentTechniqueEntries = materials_[0]->GetTechniques();
    if (currentTechniqueEntries != sortedTechniqueEntries_)
        techniqueEntries_ = currentTechniqueEntries;

    const bool canEdit = ea::all_of(materials_.begin() + 1, materials_.end(),
        [&](Material* material) { return sortedTechniqueEntries_ == material->GetTechniques(); });

    ui::BeginDisabled(!canEdit);
    if (RenderTechniqueEntries())
        pendingChange_ = true;
    ui::EndDisabled();

    if (!canEdit)
    {
        ui::SameLine();
        if (ui::SmallButton(ICON_FA_CODE_MERGE))
            pendingChange_ = true;
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
        ui::PushID(entryIndex);

        TechniqueEntry& entry = techniqueEntries_[entryIndex];

        if (EditTechniqueInEntry(entry, availableWidth))
            modified = true;

        if (ui::SmallButton(ICON_FA_TRASH_CAN))
            pendingDelete = entryIndex;
        ui::SameLine();

        if (EditDistanceInEntry(entry, availableWidth * 0.5f))
            modified = true;
        ui::SameLine();

        if (EditQualityInEntry(entry))
            modified = true;

        ui::PopID();
    }

    // Remove entry
    if (pendingDelete && *pendingDelete < techniqueEntries_.size())
    {
        techniqueEntries_.erase_at(*pendingDelete);

        modified = true;
    }

    // Add new entry
    if (defaultTechnique_ && ui::SmallButton(ICON_FA_SQUARE_PLUS))
    {
        TechniqueEntry& entry = techniqueEntries_.emplace_back();
        entry.technique_ = entry.original_ = defaultTechnique_->technique_;

        modified = true;
    }

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

            ui::PushID(techniqueIndex);

            if (desc.deprecated_ && !wasDeprecated)
            {
                ui::Separator();
                wasDeprecated = true;
            }

            if (!desc.deprecated_)
                ui::PushStyleColor(ImGuiCol_Text, ImVec4{1.0f, 1.0f, 0.0f, 1.0f});

            if (ui::Selectable(desc.displayName_.c_str(), entry.technique_ == desc.technique_))
            {
                entry.technique_ = entry.original_ = desc.technique_;
                modified = true;
            }

            if (!desc.deprecated_)
                ui::PopStyleColor();

            ui::PopID();
        }
        ui::EndCombo();
    }

    return modified;
}

bool MaterialInspectorWidget::EditDistanceInEntry(TechniqueEntry& entry, float itemWidth)
{
    bool modified = false;

    ui::SetNextItemWidth(itemWidth);
    if (ui::DragFloat("##Distance", &entry.lodDistance_, 1.0f, 0.0f, 1000.0f, "%.1f"))
        modified = true;
    ui::SameLine();

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
            ui::PushID(qualityLevelIndex);
            if (ui::Selectable(qualityLevels[qualityLevelIndex].c_str(), qualityLevel == qualityLevelIndex))
            {
                entry.qualityLevel_ = static_cast<MaterialQuality>(qualityLevelIndex);
                if (entry.qualityLevel_ > QUALITY_HIGH)
                    entry.qualityLevel_ = QUALITY_MAX;
                modified = true;
            }
            ui::PopID();
        }
        ui::EndCombo();
    }

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

}

#endif
