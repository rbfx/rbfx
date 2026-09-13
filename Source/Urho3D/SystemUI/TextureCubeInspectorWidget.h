// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Math/SphericalHarmonics.h"
#include "Urho3D/SystemUI/ResourceInspectorWidget.h"
#include "Urho3D/SystemUI/Widgets.h"

namespace Urho3D
{

class Geometry;
class PipelineState;
class TextureCube;
class XMLFile;

/// Generates spherical harmonics for cube textures. Slow.
/// TODO: Move to dedicated file.
class URHO3D_API SphericalHarmonicsGenerator : public Object
{
    URHO3D_OBJECT(SphericalHarmonicsGenerator, Object);

public:
    explicit SphericalHarmonicsGenerator(Context* context);
    ~SphericalHarmonicsGenerator() override;

    SphericalHarmonicsColor9 Generate(TextureCube* texture);
    void Generate(TextureCube* texture, XMLFile* imageXml);

private:
    void InitializePipelineStates();
    void InitializeTextures();

    const unsigned textureSize_{8};

    SharedPtr<Geometry> quadGeometry_;
    SharedPtr<PipelineState> copyTexturePipelineState_;
    SharedPtr<Texture2D> tempTexture_;
    ea::vector<Vector4> tempTextureData_;
};

/// SystemUI widget used to edit 2D texture.
class URHO3D_API TextureCubeInspectorWidget : public ResourceInspectorWidget
{
    URHO3D_OBJECT(TextureCubeInspectorWidget, ResourceInspectorWidget);

public:
    TextureCubeInspectorWidget(Context* context, const ResourceVector& resources);
    ~TextureCubeInspectorWidget() override;
    void RenderContent() override;

    bool CanSave() const override { return false; }

private:
    static const ea::vector<PropertyDesc> properties;

    void GeneratePendingSH();

    ea::vector<ea::string> texturesToGenerateSH_;
    SharedPtr<SphericalHarmonicsGenerator> generator_;
};

} // namespace Urho3D
