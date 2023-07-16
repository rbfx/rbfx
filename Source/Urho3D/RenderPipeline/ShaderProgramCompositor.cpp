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

#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../IO/Log.h"
#include "../RenderPipeline/CameraProcessor.h"
#include "../RenderPipeline/InstancingBuffer.h"
#include "../RenderPipeline/LightProcessor.h"
#include "../RenderPipeline/PipelineStateBuilder.h"
#include "../RenderPipeline/SceneProcessor.h"
#include "../RenderPipeline/ShadowMapAllocator.h"

#include <EASTL/span.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

int GetTextureColorSpaceHint(bool linearInput, bool srgbTexture)
{
    return static_cast<int>(linearInput) + static_cast<int>(srgbTexture);
}

}

ShaderProgramCompositor::ShaderProgramCompositor(Context* context)
    : Object(context)
{
}

void ShaderProgramCompositor::SetSettings(const ShaderProgramCompositorSettings& settings)
{
    settings_ = settings;
}

void ShaderProgramCompositor::SetFrameSettings(const CameraProcessor* cameraProcessor)
{
    isCameraOrthographic_ = cameraProcessor->IsCameraOrthographic();
    isCameraClipped_ = cameraProcessor->IsCameraClipped();
    isCameraReversed_ = cameraProcessor->IsCameraReversed();
}

void ShaderProgramCompositor::ProcessUserBatch(ShaderProgramDesc& result, DrawableProcessorPassFlags flags,
    Drawable* drawable, Geometry* geometry, GeometryType geometryType, Material* material, Pass* pass,
    Light* light, bool hasShadow, BatchCompositorSubpass subpass)
{
    SetupShaders(result, pass);
    ApplyCommonDefines(result, flags, pass);
    ApplyGeometryVertexDefines(result, flags, geometry, geometryType);

    ApplyLayoutVertexAndCommonDefinesForUserPass(result, geometry->GetVertexBuffer(0));
    ApplyMaterialPixelDefinesForUserPass(result, material);

    ApplyNormalTangentSpaceDefines(result, geometryType, geometry->GetVertexBuffer(0));

    if (isCameraClipped_)
        result.AddShaderDefines(VS, "URHO3D_CLIP_PLANE");

    const bool isDeferred = subpass == BatchCompositorSubpass::Deferred;
    const bool isDepthOnly = flags.Test(DrawableProcessorPassFlag::DepthOnlyPass);
    if (subpass == BatchCompositorSubpass::Light)
        result.AddCommonShaderDefines("URHO3D_ADDITIVE_LIGHT_PASS");
    else if (flags.Test(DrawableProcessorPassFlag::HasAmbientLighting))
        ApplyAmbientLightingVertexAndCommonDefinesForUserPass(result, drawable, isDeferred);

    if (isDeferred)
        result.AddCommonShaderDefines("URHO3D_NUM_RENDER_TARGETS=4");
    else if (isDepthOnly)
        result.AddCommonShaderDefines("URHO3D_NUM_RENDER_TARGETS=0");

    if (light)
        ApplyPixelLightPixelAndCommonDefines(result, light, hasShadow, material->GetSpecular());
}

void ShaderProgramCompositor::ProcessShadowBatch(ShaderProgramDesc& result,
    Geometry* geometry, GeometryType geometryType, Material* material, Pass* pass, Light* light)
{
    const DrawableProcessorPassFlags flags = DrawableProcessorPassFlag::DepthOnlyPass;
    SetupShaders(result, pass);
    ApplyCommonDefines(result, flags, pass);
    ApplyGeometryVertexDefines(result, flags, geometry, geometryType);
    ApplyDefinesForShadowPass(result, light, geometry->GetVertexBuffer(0), material, pass);
}

void ShaderProgramCompositor::ProcessLightVolumeBatch(ShaderProgramDesc& result,
    Geometry* geometry, GeometryType geometryType, Pass* pass, Light* light, bool hasShadow)
{
    const DrawableProcessorPassFlags flags = DrawableProcessorPassFlag::DisableInstancing;
    SetupShaders(result, pass);
    ApplyCommonDefines(result, flags, pass);
    ApplyGeometryVertexDefines(result, flags, geometry, geometryType);
    ApplyPixelLightPixelAndCommonDefines(result, light, hasShadow, true);
    ApplyDefinesForLightVolumePass(result);
}

void ShaderProgramCompositor::SetupShaders(ShaderProgramDesc& result, Pass* pass)
{
    result.shaderName_[VS] = "v2/" + pass->GetVertexShader();
    result.shaderName_[PS] = "v2/" + pass->GetPixelShader();
}

void ShaderProgramCompositor::ApplyCommonDefines(ShaderProgramDesc& result,
    DrawableProcessorPassFlags flags, Pass* pass) const
{
    if (isCameraReversed_)
        result.AddCommonShaderDefines("URHO3D_CAMERA_REVERSED");

    if (!flags.Test(DrawableProcessorPassFlag::DepthOnlyPass))
    {
        if (settings_.sceneProcessor_.cubemapBoxProjection_)
            result.AddCommonShaderDefines("URHO3D_BOX_PROJECTION");

        if (settings_.sceneProcessor_.linearSpaceLighting_)
            result.AddCommonShaderDefines("URHO3D_GAMMA_CORRECTION");

        switch (settings_.sceneProcessor_.specularQuality_)
        {
        case SpecularQuality::Simple:
            result.AddCommonShaderDefines("URHO3D_SPECULAR=1");
            break;
        case SpecularQuality::Antialiased:
            result.AddCommonShaderDefines("URHO3D_SPECULAR=2");
            break;
        default:
            break;
        }

        if (settings_.sceneProcessor_.reflectionQuality_ == ReflectionQuality::Vertex)
            result.AddCommonShaderDefines("URHO3D_VERTEX_REFLECTION");
    }

    if (flags.Test(DrawableProcessorPassFlag::NeedReadableDepth) && settings_.renderBufferManager_.readableDepth_)
    {
        result.AddCommonShaderDefines("URHO3D_HAS_READABLE_DEPTH");
        if (isCameraOrthographic_)
            result.AddCommonShaderDefines("URHO3D_ORTHOGRAPHIC_DEPTH");
    }

    result.AddShaderDefines(VS, pass->GetEffectiveVertexShaderDefines());
    result.AddShaderDefines(PS, pass->GetEffectivePixelShaderDefines());
}

void ShaderProgramCompositor::ApplyGeometryVertexDefines(ShaderProgramDesc& result,
    DrawableProcessorPassFlags flags, Geometry* geometry, GeometryType geometryType) const
{
    result.isInstancingUsed_ = IsInstancingUsed(flags, geometry, geometryType);
    if (result.isInstancingUsed_)
        result.AddShaderDefines(VS, "URHO3D_INSTANCING");

    static const ea::string geometryDefines[] = {
        "URHO3D_GEOMETRY_STATIC ",
        "URHO3D_GEOMETRY_SKINNED ",
        "URHO3D_GEOMETRY_STATIC ",
        "URHO3D_GEOMETRY_BILLBOARD ",
        "URHO3D_GEOMETRY_DIRBILLBOARD ",
        "URHO3D_GEOMETRY_TRAIL_FACE_CAMERA ",
        "URHO3D_GEOMETRY_TRAIL_BONE ",
        "URHO3D_GEOMETRY_STATIC ",
    };

    const auto geometryTypeIndex = static_cast<int>(geometryType);
    if (geometryTypeIndex < ea::size(geometryDefines))
        result.AddShaderDefines(VS, geometryDefines[geometryTypeIndex]);
    else
        result.AddShaderDefines(VS, Format("URHO3D_GEOMETRY_CUSTOM={} ", geometryTypeIndex));

    if (geometryType == GEOM_SKINNED)
        result.AddShaderDefines(VS, Format("URHO3D_MAXBONES={} ", Graphics::GetMaxBones()));
}

void ShaderProgramCompositor::ApplyPixelLightPixelAndCommonDefines(ShaderProgramDesc& result,
    Light* light, bool hasShadow, bool materialHasSpecular) const
{
    if (light->GetShapeTexture())
        result.AddCommonShaderDefines("URHO3D_LIGHT_CUSTOM_SHAPE");

    if (light->GetRampTexture())
        result.AddShaderDefines(PS, "URHO3D_LIGHT_CUSTOM_RAMP");

    static const ea::string lightTypeDefines[] = {
        "URHO3D_LIGHT_DIRECTIONAL ",
        "URHO3D_LIGHT_SPOT ",
        "URHO3D_LIGHT_POINT "
    };
    result.AddCommonShaderDefines(lightTypeDefines[static_cast<int>(light->GetLightType())]);

    if (hasShadow)
    {
        const unsigned maxCascades = light->GetLightType() == LIGHT_DIRECTIONAL ? MAX_CASCADE_SPLITS : 1u;

        result.AddCommonShaderDefines("URHO3D_HAS_SHADOW");
        if (maxCascades > 1)
            result.AddCommonShaderDefines(Format("URHO3D_MAX_SHADOW_CASCADES={}", maxCascades));
        if (settings_.shadowMapAllocator_.enableVarianceShadowMaps_)
            result.AddCommonShaderDefines("URHO3D_VARIANCE_SHADOW_MAP");
        else
            result.AddCommonShaderDefines(Format("URHO3D_SHADOW_PCF_SIZE={}", settings_.sceneProcessor_.pcfKernelSize_));
    }
}

void ShaderProgramCompositor::ApplyNormalTangentSpaceDefines(
    ShaderProgramDesc& result, GeometryType geometryType, VertexBuffer* vertexBuffer) const
{
    const auto [hasNormal, hasTangent] = IsNormalAndTangentAvailable(geometryType, vertexBuffer);
    if (hasNormal)
        result.AddCommonShaderDefines("URHO3D_VERTEX_NORMAL_AVAILABLE");
    if (hasTangent)
        result.AddCommonShaderDefines("URHO3D_VERTEX_TANGENT_AVAILABLE");
}

void ShaderProgramCompositor::ApplyLayoutVertexAndCommonDefinesForUserPass(
    ShaderProgramDesc& result, VertexBuffer* vertexBuffer) const
{
    if (vertexBuffer->HasElement(SEM_NORMAL))
        result.AddShaderDefines(VS, "URHO3D_VERTEX_HAS_NORMAL");
    if (vertexBuffer->HasElement(SEM_TANGENT))
        result.AddShaderDefines(VS, "URHO3D_VERTEX_HAS_TANGENT");
    if (vertexBuffer->HasElement(SEM_TEXCOORD, 0))
        result.AddShaderDefines(VS, "URHO3D_VERTEX_HAS_TEXCOORD0");
    if (vertexBuffer->HasElement(SEM_TEXCOORD, 1))
        result.AddShaderDefines(VS, "URHO3D_VERTEX_HAS_TEXCOORD1");

    if (vertexBuffer->HasElement(SEM_COLOR))
        result.AddCommonShaderDefines("URHO3D_VERTEX_HAS_COLOR");
}

void ShaderProgramCompositor::ApplyMaterialPixelDefinesForUserPass(ShaderProgramDesc& result, Material* material) const
{
    if (Texture* diffuseTexture = material->GetTexture(TU_DIFFUSE))
    {
        result.AddShaderDefines(PS, "URHO3D_MATERIAL_HAS_DIFFUSE");
        const int hint = GetTextureColorSpaceHint(diffuseTexture->GetLinear(), diffuseTexture->GetSRGB());
        if (hint > 1)
            URHO3D_LOGWARNING("Texture {} cannot be both sRGB and Linear", diffuseTexture->GetName());
        result.AddShaderDefines(PS, Format("URHO3D_MATERIAL_DIFFUSE_HINT={}", ea::min(1, hint)));
    }

    if (material->GetTexture(TU_NORMAL))
        result.AddShaderDefines(PS, "URHO3D_MATERIAL_HAS_NORMAL");

    if (material->GetTexture(TU_SPECULAR))
        result.AddShaderDefines(PS, "URHO3D_MATERIAL_HAS_SPECULAR");

    if (Texture* envTexture = material->GetTexture(TU_ENVIRONMENT))
    {
        if (envTexture->IsInstanceOf<Texture2D>())
            result.AddCommonShaderDefines("URHO3D_MATERIAL_HAS_PLANAR_ENVIRONMENT");
    }

    if (Texture* emissiveTexture = material->GetTexture(TU_EMISSIVE))
    {
        result.AddShaderDefines(PS, "URHO3D_MATERIAL_HAS_EMISSIVE");
        const int hint = GetTextureColorSpaceHint(emissiveTexture->GetLinear(), emissiveTexture->GetSRGB());
        if (hint > 1)
            URHO3D_LOGWARNING("Texture {} cannot be both sRGB and Linear", emissiveTexture->GetName());
        result.AddShaderDefines(PS, Format("URHO3D_MATERIAL_EMISSIVE_HINT={}", ea::min(1, hint)));
    }
}

void ShaderProgramCompositor::ApplyAmbientLightingVertexAndCommonDefinesForUserPass(ShaderProgramDesc& result,
    Drawable* drawable, bool isGeometryBufferPass) const
{
    result.AddCommonShaderDefines("URHO3D_AMBIENT_PASS");
    if (isGeometryBufferPass)
        result.AddCommonShaderDefines("URHO3D_GBUFFER_PASS");
    else if (settings_.sceneProcessor_.maxVertexLights_ > 0)
        result.AddCommonShaderDefines(Format("URHO3D_NUM_VERTEX_LIGHTS={}", settings_.sceneProcessor_.maxVertexLights_));

    if (drawable->GetGlobalIlluminationType() == GlobalIlluminationType::UseLightMap)
        result.AddCommonShaderDefines("URHO3D_HAS_LIGHTMAP");

    if (drawable->GetReflectionMode() >= ReflectionMode::BlendProbes)
        result.AddCommonShaderDefines("URHO3D_BLEND_REFLECTIONS");

    static const ea::string ambientModeDefines[] = {
        "URHO3D_AMBIENT_CONSTANT ",
        "URHO3D_AMBIENT_FLAT ",
        "URHO3D_AMBIENT_DIRECTIONAL ",
    };

    result.AddShaderDefines(VS, ambientModeDefines[static_cast<int>(settings_.sceneProcessor_.ambientMode_)]);
}

void ShaderProgramCompositor::ApplyDefinesForShadowPass(ShaderProgramDesc& result,
    Light* light, VertexBuffer* vertexBuffer, Material* material, Pass* pass) const
{
    if (vertexBuffer->HasElement(SEM_NORMAL))
        result.AddShaderDefines(VS, "URHO3D_VERTEX_HAS_NORMAL");

    if (light->GetShadowBias().normalOffset_ > 0.0)
        result.AddShaderDefines(VS, "URHO3D_SHADOW_NORMAL_OFFSET");

    if (pass->IsAlphaMask())
    {
        if (vertexBuffer->HasElement(SEM_TEXCOORD, 0))
            result.AddShaderDefines(VS, "URHO3D_VERTEX_HAS_TEXCOORD0");
        if (Texture* diffuseTexture = material->GetTexture(TU_DIFFUSE))
            result.AddShaderDefines(PS, "URHO3D_MATERIAL_HAS_DIFFUSE");
    }

    result.AddCommonShaderDefines("URHO3D_SHADOW_PASS");
    if (settings_.shadowMapAllocator_.enableVarianceShadowMaps_)
        result.AddCommonShaderDefines("URHO3D_VARIANCE_SHADOW_MAP");
    else
        result.AddCommonShaderDefines("URHO3D_NUM_RENDER_TARGETS=0");
}

void ShaderProgramCompositor::ApplyDefinesForLightVolumePass(ShaderProgramDesc& result) const
{
    result.AddCommonShaderDefines("URHO3D_LIGHT_VOLUME_PASS");
    if (isCameraOrthographic_)
        result.AddCommonShaderDefines("URHO3D_ORTHOGRAPHIC_DEPTH");
    if (settings_.sceneProcessor_.lightingMode_ == DirectLightingMode::DeferredPBR)
        result.AddCommonShaderDefines("URHO3D_PHYSICAL_MATERIAL");
}

bool ShaderProgramCompositor::IsInstancingUsed(
    DrawableProcessorPassFlags flags, Geometry* geometry, GeometryType geometryType) const
{
    return !flags.Test(DrawableProcessorPassFlag::DisableInstancing)
        && settings_.instancingBuffer_.enableInstancing_
        && geometry->IsInstanced(geometryType);
}

ea::pair<bool, bool> ShaderProgramCompositor::IsNormalAndTangentAvailable(
    GeometryType geometryType, VertexBuffer* vertexBuffer) const
{
    switch (geometryType)
    {
    case GEOM_BILLBOARD:
    case GEOM_DIRBILLBOARD:
    case GEOM_TRAIL_FACE_CAMERA:
    case GEOM_TRAIL_BONE:
        return {true, true};
    case GEOM_STATIC:
    case GEOM_SKINNED:
    case GEOM_INSTANCED:
    case GEOM_STATIC_NOINSTANCING:
        return {vertexBuffer->HasElement(SEM_NORMAL), vertexBuffer->HasElement(SEM_TANGENT)};
    default:
        // This is just an assumption to prevent disabled normal mapping in case of unknown geometry type
        return {true, true};
    }
}

}
