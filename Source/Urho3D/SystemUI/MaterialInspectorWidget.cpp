// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/MaterialInspectorWidget.h"

#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Light.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/Skybox.h"
#include "Urho3D/Graphics/Texture2D.h"
#include "Urho3D/Graphics/Texture2DArray.h"
#include "Urho3D/Graphics/Texture3D.h"
#include "Urho3D/Graphics/TextureCube.h"
#include "Urho3D/Graphics/Zone.h"
#include "Urho3D/Input/MoveAndOrbitComponent.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/SystemUI/SystemUI.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

namespace
{

// Keep those global so that they are not reset on material change.
// TODO: Better solution?
static thread_local bool previewSkyboxEnabled{true};
static thread_local bool previewLightEnabled{true};

static Vector3 defaultPreviewCameraPosition = Vector3::BACK * 2.0f;

static const StringVector allowedTextureTypes{
    Texture2D::GetTypeNameStatic(),
    Texture2DArray::GetTypeNameStatic(),
    TextureCube::GetTypeNameStatic(),
    Texture3D::GetTypeNameStatic(),
};

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
    {ShaderResources::Albedo,       "Albedo map or Diffuse texture with optional alpha channel."},
    {ShaderResources::Normal,       "Normal map, ignored unless normal mapping is enabled."},
    {ShaderResources::Properties,   "Roughness-Metalness-Occlusion map or Specular map."},
    {ShaderResources::Emission,     "Emission map. May also be used as AO map for legacy materials."},
    {ShaderResources::Reflection0,  "Reflection map override."},
};

const MaterialInspectorWidget::PropertyDesc MaterialInspectorWidget::vertexDefinesProperty{
    "Vertex Defines",
    Variant{EMPTY_STRING},
    [](const Material* material) { return Variant{material->GetVertexShaderDefines()}; },
    [](Material* material, const Variant& value) { material->SetVertexShaderDefines(value.GetString()); },
    "Additional shader defines applied to vertex shader. Should be space-separated list of DEFINES. "
    "Example: VOLUMETRIC SOFTPARTICLES",
};

const MaterialInspectorWidget::PropertyDesc MaterialInspectorWidget::pixelDefinesProperty{
    "Pixel Defines",
    Variant{EMPTY_STRING},
    [](const Material* material) { return Variant{material->GetPixelShaderDefines()}; },
    [](Material* material, const Variant& value) { material->SetPixelShaderDefines(value.GetString()); },
    "Additional shader defines applied to pixel shader. Should be space-separated list of DEFINES. "
    "Example: VOLUMETRIC SOFTPARTICLES",
};

const ea::vector<MaterialInspectorWidget::PropertyDesc> MaterialInspectorWidget::properties{
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
    , previewScene_(MakeShared<Scene>(context))
    , previewWidget_(MakeShared<SceneRendererToTexture>(previewScene_))
{
    URHO3D_ASSERT(!materials_.empty());
    InitializePreviewScene();
    ApplyPreviewSettings();
}

MaterialInspectorWidget::~MaterialInspectorWidget()
{
}

void MaterialInspectorWidget::InitializePreviewScene()
{
    auto cache = GetSubsystem<ResourceCache>();

    previewScene_->CreateComponent<Octree>();

    auto skyboxNode = previewScene_->CreateChild("Skybox");
    auto skybox = skyboxNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml")->Clone());

    auto zoneNode = previewScene_->CreateChild("Global Zone");
    auto zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox{-1000.0f, 1000.0f});
    zone->SetAmbientColor(Color::BLACK);
    zone->SetBackgroundBrightness(1.0f);
    zone->SetZoneTexture(cache->GetResource<TextureCube>("Textures/DefaultSkybox.xml"));

    auto lightNode = previewScene_->CreateChild("Light");
    lightNode->SetDirection({1.0f, -3.0f, 1.0f});
    auto light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    auto cameraNode = previewWidget_->GetCameraNode();
    cameraNode->SetPosition(defaultPreviewCameraPosition);
    cameraNode->CreateComponent<MoveAndOrbitComponent>();

    auto modelNode = previewScene_->CreateChild("Model");
    auto model = modelNode->CreateComponent<StaticModel>();
    model->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
}

void MaterialInspectorWidget::ApplyPreviewSettings()
{
    auto cache = GetSubsystem<ResourceCache>();
    auto renderer = GetSubsystem<Renderer>();

    auto skybox = previewScene_->GetComponent<Skybox>(true);
    auto zone = previewScene_->GetComponent<Zone>(true);

    Texture* defaultTexture = cache->GetResource<TextureCube>("Textures/DefaultSkybox.xml");
    Texture* emptyTexture = renderer->GetBlackCubeMap();

    Material* skyboxMaterial = skybox->GetMaterial();
    skyboxMaterial->SetTexture(ShaderResources::Albedo, previewSkyboxEnabled ? defaultTexture : emptyTexture);
    zone->SetZoneTexture(previewSkyboxEnabled ? defaultTexture : nullptr);

    auto light = previewScene_->GetComponent<Light>(true);
    light->SetBrightness(previewSkyboxEnabled ? 0.5f : 1.0f);
    light->SetEnabled(previewLightEnabled);
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

    const bool hasPreview = materials_.size() == 1;

    if (hasPreview)
    {
        RenderPreview();
        ui::BeginChild("Content");
    }

    RenderTechniques();
    RenderProperties();
    RenderTextures();
    RenderShaderParameters();

    ui::Separator();
    const bool forceSave = ui::Button(ICON_FA_FLOPPY_DISK " Force Save");
    if (ui::IsItemHovered())
        ui::SetTooltip("Materials are always saved on edit. You can force save even if there are no changes.");

    if (hasPreview)
        ui::EndChild();

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
            for (const auto& [name, texture] : pendingSetTextures_)
                material->SetTexture(name, texture);
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

    if (forceSave)
    {
        OnEditBegin(this);
        OnEditEnd(this);
    }
}

void MaterialInspectorWidget::RenderPreview()
{
    if (materials_.size() != 1)
        return;

    auto workQueue = GetSubsystem<WorkQueue>();

    previewWidget_->SetActive(true);
    workQueue->PostDelayedTaskForMainThread([widget = previewWidget_] { widget->SetActive(false); });

    Camera* camera = previewWidget_->GetCamera();
    Node* cameraNode = camera->GetNode();
    auto moveAndOrbit = cameraNode->GetComponent<MoveAndOrbitComponent>();

    const float availableWidth = ui::GetContentRegionAvail().x;
    const float textureHeight = ea::min(ui::GetContentRegionAvail().x, 250.0f);
    const auto textureSize = Vector2{availableWidth, textureHeight}.ToIntVector2();

    previewWidget_->SetTextureSize(textureSize);
    previewWidget_->Update();

    Texture2D* sceneTexture = previewWidget_->GetTexture();
    const auto imageBegin = ui::GetCursorPos();
    Widgets::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
    const auto imageEnd = ui::GetCursorPos();

    float distance = cameraNode->GetPosition().Length();
    if (ui::IsItemHovered())
    {
        if (ui::IsMouseDown(MOUSEB_RIGHT))
        {
            const Vector2 mouseDelta = ToVector2(ui::GetIO().MouseDelta);
            moveAndOrbit->SetYaw(moveAndOrbit->GetYaw() + mouseDelta.x_ * 0.9f);
            moveAndOrbit->SetPitch(moveAndOrbit->GetPitch() + mouseDelta.y_ * 0.9f);
        }

        if (Abs(ui::GetMouseWheel()) > 0.05f)
        {
            if (ui::GetMouseWheel() > 0.0f)
                distance *= 0.8f;
            else
                distance *= 1.3f;
        }
    }
    distance = Clamp(distance, 1.01f, 3.0f);

    cameraNode->SetRotation(moveAndOrbit->GetYawPitchRotation());
    cameraNode->SetPosition(distance * (cameraNode->GetRotation() * Vector3::BACK));

    bool previewSettingsDirty = false;
    ui::SetCursorPos(imageBegin + ui::GetStyle().FramePadding);

    if (Widgets::ToolbarButton(ICON_FA_CLOUD, "Toggle Skybox", previewSkyboxEnabled))
    {
        previewSkyboxEnabled = !previewSkyboxEnabled;
        previewSettingsDirty = true;
    }

    if (Widgets::ToolbarButton(ICON_FA_LIGHTBULB, "Toggle Light", previewLightEnabled))
    {
        previewLightEnabled = !previewLightEnabled;
        previewSettingsDirty = true;
    }

    if (Widgets::ToolbarButton(ICON_FA_ARROWS_LEFT_RIGHT_TO_LINE, "Reset Camera"))
    {
        cameraNode->SetPosition(defaultPreviewCameraPosition);
        moveAndOrbit->SetYaw(0.0f);
        moveAndOrbit->SetPitch(0.0f);
    }

    ui::SetCursorPos(imageEnd);

    auto model = previewScene_->GetComponent<StaticModel>(true);
    model->SetMaterial(materials_[0]);

    if (previewSettingsDirty)
        ApplyPreviewSettings();
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

    RenderShaderDefines();
    for (const PropertyDesc& property : properties)
        RenderProperty(property);

    ui::Separator();
}

void MaterialInspectorWidget::RenderShaderDefines()
{
    Variant vertexDefines = materials_[0]->GetVertexShaderDefines();
    const bool vertexDefinesUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return vertexDefines != material->GetVertexShaderDefines(); });

    Variant pixelDefines = materials_[0]->GetPixelShaderDefines();
    const bool pixelDefinesUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return pixelDefines != material->GetPixelShaderDefines(); });

    if (!separateShaderDefines_.has_value())
        separateShaderDefines_ = vertexDefinesUndefined || pixelDefinesUndefined || vertexDefines != pixelDefines;
    if (!*separateShaderDefines_ && vertexDefines != pixelDefines)
        separateShaderDefines_ = false;

    // Render widget for vertex defines
    {
        const IdScopeGuard guard("Vertex Defines");

        Widgets::ItemLabel(vertexDefinesProperty.name_,
            Widgets::GetItemLabelColor(vertexDefinesUndefined, vertexDefines.GetString().empty()));
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", vertexDefinesProperty.hint_.c_str());

        const ColorScopeGuard guardBackgroundColor{
            ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(vertexDefinesUndefined), vertexDefinesUndefined};

        if (Widgets::EditVariant(vertexDefines, vertexDefinesProperty.options_))
        {
            pendingSetProperties_.emplace_back(&vertexDefinesProperty, vertexDefines);
            if (!*separateShaderDefines_)
                pendingSetProperties_.emplace_back(&pixelDefinesProperty, vertexDefines);
        }
    }

    // Update whether the defines are synchronized
    const bool pixelDefinesModeChanged = ui::Checkbox("##SeparateShaderDefines", &*separateShaderDefines_);
    if (ui::IsItemHovered())
        ui::SetTooltip("Enable separate editing for vertex and pixel defines");
    ui::SameLine();

    // Render widget for pixel defines
    {
        const IdScopeGuard guard("Pixel Defines");

        ui::BeginDisabled(!*separateShaderDefines_);

        Widgets::ItemLabel(pixelDefinesProperty.name_,
            Widgets::GetItemLabelColor(pixelDefinesUndefined, pixelDefines.GetString().empty()));
        if (ui::IsItemHovered())
            ui::SetTooltip("%s", pixelDefinesProperty.hint_.c_str());

        const ColorScopeGuard guardBackgroundColor{
            ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(pixelDefinesUndefined), pixelDefinesUndefined};

        if (Widgets::EditVariant(pixelDefines, pixelDefinesProperty.options_))
            pendingSetProperties_.emplace_back(&pixelDefinesProperty, pixelDefines);

        ui::EndDisabled();
    }

    // Update pixel defines when separate defines are disabled
    if (pixelDefinesModeChanged && !*separateShaderDefines_)
        pendingSetProperties_.emplace_back(&pixelDefinesProperty, vertexDefines);
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

    const auto customTextureUnits = GetCustomTextureUnits();
    for (const ea::string& unit : customTextureUnits)
        RenderCustomTextureUnit(unit);
    RenderAddCustomTextureUnit(customTextureUnits);
}

void MaterialInspectorWidget::RenderTextureUnit(const TextureUnitDesc& desc)
{
    const IdScopeGuard guard(desc.name_.c_str());

    auto cache = GetSubsystem<ResourceCache>();

    Texture* texture = materials_[0]->GetTexture(desc.name_);
    const bool isUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return material->GetTexture(desc.name_) != texture; });

    Widgets::ItemLabel(desc.name_, Widgets::GetItemLabelColor(isUndefined, texture == nullptr));
    if (ui::IsItemHovered())
        ui::SetTooltip("%s", desc.hint_.c_str());

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingSetTextures_.emplace_back(desc.name_, nullptr);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove texture from this unit");
    ui::SameLine();

    const ColorScopeGuard guardBackgroundColor{ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(isUndefined), isUndefined};

    StringHash textureType = texture ? texture->GetType() : Texture2D::GetTypeStatic();
    ea::string textureName = texture ? texture->GetName() : "";
    if (Widgets::EditResourceRef(textureType, textureName, &allowedTextureTypes))
    {
        if (const auto texture = dynamic_cast<Texture*>(cache->GetResource(textureType, textureName)))
            pendingSetTextures_.emplace_back(desc.name_, texture);
        else
            pendingSetTextures_.emplace_back(desc.name_, nullptr);
    }
}

bool MaterialInspectorWidget::IsDefaultTextureUnit(const ea::string& unit) const
{
    const auto isSame = [&](const TextureUnitDesc& desc) { return desc.name_ == unit; };
    return ea::any_of(textureUnits.begin(), textureUnits.end(), isSame);
}

ea::set<ea::string> MaterialInspectorWidget::GetCustomTextureUnits() const
{
    ea::set<ea::string> result;

    for (Material* material : materials_)
    {
        for (const auto& [_, info] : material->GetTextures())
            result.emplace(info.name_);
    }

    for (const TextureUnitDesc& desc : textureUnits)
        result.erase(desc.name_);

    return result;
}

void MaterialInspectorWidget::RenderCustomTextureUnit(const ea::string& unit)
{
    const IdScopeGuard guardKey{unit.c_str()};

    auto cache = GetSubsystem<ResourceCache>();

    Texture* texture = materials_[0]->GetTexture(unit);
    const bool isUndefined = ea::any_of(materials_.begin() + 1, materials_.end(),
        [&](const Material* material) { return material->GetTexture(unit) != texture; });

    Widgets::ItemLabel(unit, Widgets::GetItemLabelColor(isUndefined, texture == nullptr));
    if (ui::IsItemHovered())
        ui::SetTooltip("%s", "Custom texture unit");

    if (ui::Button(ICON_FA_TRASH_CAN))
        pendingSetTextures_.emplace_back(unit, nullptr);
    if (ui::IsItemHovered())
        ui::SetTooltip("Remove texture and unit");
    ui::SameLine();

    const ColorScopeGuard guardBackgroundColor{
        ImGuiCol_FrameBg, Widgets::GetItemBackgroundColor(isUndefined), isUndefined};

    StringHash textureType = texture ? texture->GetType() : Texture2D::GetTypeStatic();
    ea::string textureName = texture ? texture->GetName() : "";
    if (Widgets::EditResourceRef(textureType, textureName, &allowedTextureTypes))
    {
        if (const auto texture = dynamic_cast<Texture*>(cache->GetResource(textureType, textureName)))
            pendingSetTextures_.emplace_back(unit, texture);
        else
            pendingSetTextures_.emplace_back(unit, nullptr);
    }
}

void MaterialInspectorWidget::RenderAddCustomTextureUnit(const ea::set<ea::string>& customTextureUnits)
{
    const IdScopeGuard guardAddElement{"##AddElement"};

    auto cache = GetSubsystem<ResourceCache>();
    auto defaultTexture = cache->GetResource<Texture2D>("Textures/Black.png");

    const bool isButtonClicked = ui::Button(ICON_FA_SQUARE_PLUS " Add new texture");
    if (ui::IsItemHovered())
        ui::SetTooltip("Add new item to the map");
    ui::SameLine();

    // TODO(editor): this "static" is bad in theory
    static ea::string newUnit;

    ui::SetNextItemWidth(ui::GetContentRegionAvail().x);
    const bool isTextClicked = ui::InputText("", &newUnit, ImGuiInputTextFlags_EnterReturnsTrue);
    const bool isNameAvailable = !customTextureUnits.contains(newUnit) && !IsDefaultTextureUnit(newUnit);

    if ((isButtonClicked || isTextClicked) && !newUnit.empty() && isNameAvailable)
        pendingSetTextures_.emplace_back(newUnit, defaultTexture);

    if (ui::IsItemHovered())
        ui::SetTooltip("Item name");

    if (!isNameAvailable)
        ui::Text("%s", ICON_FA_TRIANGLE_EXCLAMATION " This texture unit name is already used");
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

} // namespace Urho3D
