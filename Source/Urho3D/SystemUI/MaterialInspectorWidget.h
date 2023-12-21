// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Signal.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/SystemUI/Widgets.h"
#include "Urho3D/Utility/SceneRendererToTexture.h"

#include <EASTL/fixed_set.h>

namespace Urho3D
{

/// SystemUI widget used to edit materials.
class URHO3D_API MaterialInspectorWidget : public Object
{
    URHO3D_OBJECT(MaterialInspectorWidget, Object);

public:
    Signal<void()> OnEditBegin;
    Signal<void()> OnEditEnd;

    using MaterialVector = ea::vector<SharedPtr<Material>>;

    MaterialInspectorWidget(Context* context, const MaterialVector& materials);
    ~MaterialInspectorWidget() override;

    void UpdateTechniques(const ea::string& path);

    void RenderTitle();
    void RenderContent();

    const MaterialVector& GetMaterials() const { return materials_; }

private:
    struct TechniqueDesc
    {
        ea::string displayName_;
        ea::string resourceName_;
        SharedPtr<Technique> technique_;
        bool deprecated_{};

        bool operator<(const TechniqueDesc& rhs) const;
    };
    using TechniqueDescPtr = ea::shared_ptr<TechniqueDesc>;

    struct PropertyDesc
    {
        ea::string name_;
        Variant defaultValue_;
        ea::function<Variant(const Material* material)> getter_;
        ea::function<void(Material* material, const Variant& value)> setter_;
        ea::string hint_;
        Widgets::EditVariantOptions options_;
    };
    static const ea::vector<PropertyDesc> properties;
    static const PropertyDesc vertexDefinesProperty;
    static const PropertyDesc pixelDefinesProperty;

    struct TextureUnitDesc
    {
        ea::string name_;
        ea::string hint_;
    };
    static const ea::vector<TextureUnitDesc> textureUnits;

    using ShaderParameterNames = ea::fixed_set<ea::string, 128>;

    void InitializePreviewScene();
    void ApplyPreviewSettings();

    void UpdateCachedTechniques();
    void RenderPreview();
    void RenderTechniques();
    void RenderProperties();
    void RenderTextures();
    void RenderShaderParameters();

    bool RenderTechniqueEntries();
    bool EditTechniqueInEntry(TechniqueEntry& entry, float itemWidth);
    bool EditDistanceInEntry(TechniqueEntry& entry, float itemWidth);
    bool EditQualityInEntry(TechniqueEntry& entry);

    void RenderProperty(const PropertyDesc& desc);
    void RenderShaderDefines();

    void RenderTextureUnit(const TextureUnitDesc& desc);

    bool IsDefaultTextureUnit(const ea::string& unit) const;
    ea::set<ea::string> GetCustomTextureUnits() const;
    void RenderCustomTextureUnit(const ea::string& unit);
    void RenderAddCustomTextureUnit(const ea::set<ea::string>& customTextureUnits);

    ea::string GetTechniqueDisplayName(const ea::string& resourceName) const;
    bool IsTechniqueDeprecated(const ea::string& resourceName) const;

    ShaderParameterNames GetShaderParameterNames() const;
    void RenderShaderParameter(const ea::string& name);
    void RenderNewShaderParameter();

    const ea::string defaultTechniqueName_{"Techniques/LitOpaque.xml"};

    SharedPtr<Scene> previewScene_;
    SharedPtr<SceneRendererToTexture> previewWidget_;

    ea::unordered_map<ea::string, TechniqueDescPtr> techniques_;
    ea::vector<TechniqueDescPtr> sortedTechniques_;
    TechniqueDescPtr defaultTechnique_;

    MaterialVector materials_;
    ea::vector<TechniqueEntry> techniqueEntries_;
    ea::vector<TechniqueEntry> sortedTechniqueEntries_;

    bool pendingSetTechniques_{};

    ea::vector<ea::pair<const PropertyDesc*, Variant>> pendingSetProperties_;

    ea::vector<ea::pair<ea::string, Texture*>> pendingSetTextures_;

    ShaderParameterNames shaderParameterNames_;
    ea::vector<ea::pair<ea::string, Variant>> pendingSetShaderParameters_;

    ea::string newParameterName_;
    unsigned newParameterType_{};
    ea::optional<bool> separateShaderDefines_;
};

}
