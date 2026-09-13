// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../RenderPipeline/RenderPipelineDefs.h"

namespace Urho3D
{

/// Built-in shader consts.
/// See _Uniforms.glsl for descriptions.
namespace ShaderConsts
{
    URHO3D_SHADER_CONST(Frame, DeltaTime);
    URHO3D_SHADER_CONST(Frame, ElapsedTime);

    URHO3D_SHADER_CONST(Camera, View);
    URHO3D_SHADER_CONST(Camera, ViewInv);
    URHO3D_SHADER_CONST(Camera, ViewProj);
    URHO3D_SHADER_CONST(Camera, ClipPlane);
    URHO3D_SHADER_CONST(Camera, CameraPos);
    URHO3D_SHADER_CONST(Camera, NearClip);
    URHO3D_SHADER_CONST(Camera, FarClip);
    URHO3D_SHADER_CONST(Camera, DepthMode);
    URHO3D_SHADER_CONST(Camera, FrustumSize);
    URHO3D_SHADER_CONST(Camera, GBufferOffsets);
    URHO3D_SHADER_CONST(Camera, DepthReconstruct);
    URHO3D_SHADER_CONST(Camera, GBufferInvSize);
    URHO3D_SHADER_CONST(Camera, AmbientColor);
    URHO3D_SHADER_CONST(Camera, FogParams);
    URHO3D_SHADER_CONST(Camera, FogColor);
    URHO3D_SHADER_CONST(Camera, NormalOffsetScale);

    URHO3D_SHADER_CONST(Zone, CubemapCenter0);
    URHO3D_SHADER_CONST(Zone, CubemapCenter1);
    URHO3D_SHADER_CONST(Zone, ProjectionBoxMin0);
    URHO3D_SHADER_CONST(Zone, ProjectionBoxMin1);
    URHO3D_SHADER_CONST(Zone, ProjectionBoxMax0);
    URHO3D_SHADER_CONST(Zone, ProjectionBoxMax1);
    URHO3D_SHADER_CONST(Zone, RoughnessToLODFactor0);
    URHO3D_SHADER_CONST(Zone, RoughnessToLODFactor1);
    URHO3D_SHADER_CONST(Zone, ReflectionBlendFactor);

    URHO3D_SHADER_CONST(Light, LightPos);
    URHO3D_SHADER_CONST(Light, LightDir);
    URHO3D_SHADER_CONST(Light, SpotAngle);
    URHO3D_SHADER_CONST(Light, LightShapeMatrix);
    URHO3D_SHADER_CONST(Light, VertexLights);
    URHO3D_SHADER_CONST(Light, LightMatrices);
    URHO3D_SHADER_CONST(Light, LightColor);
    URHO3D_SHADER_CONST(Light, ShadowCubeAdjust);
    URHO3D_SHADER_CONST(Light, ShadowCubeUVBias);
    URHO3D_SHADER_CONST(Light, ShadowDepthFade);
    URHO3D_SHADER_CONST(Light, ShadowIntensity);
    URHO3D_SHADER_CONST(Light, ShadowMapInvSize);
    URHO3D_SHADER_CONST(Light, ShadowSplits);
    URHO3D_SHADER_CONST(Light, VSMShadowParams);
    URHO3D_SHADER_CONST(Light, LightRad);
    URHO3D_SHADER_CONST(Light, LightLength);

    /// Serialized in Material, do not rename
    /// @{
    URHO3D_SHADER_CONST(Material, UOffset);
    URHO3D_SHADER_CONST(Material, VOffset);
    URHO3D_SHADER_CONST(Material, LMOffset);
    URHO3D_SHADER_CONST(Material, MatDiffColor);
    URHO3D_SHADER_CONST(Material, MatEmissiveColor);
    URHO3D_SHADER_CONST(Material, Roughness);
    URHO3D_SHADER_CONST(Material, MatEnvMapColor);
    URHO3D_SHADER_CONST(Material, Metallic);
    URHO3D_SHADER_CONST(Material, MatSpecColor);
    URHO3D_SHADER_CONST(Material, FadeOffsetScale);
    URHO3D_SHADER_CONST(Material, NormalScale);
    URHO3D_SHADER_CONST(Material, DielectricReflectance);
    /// @}

    URHO3D_SHADER_CONST(Custom, OutlineColor);

    URHO3D_SHADER_CONST(Object, Model);
    URHO3D_SHADER_CONST(Object, SHAr);
    URHO3D_SHADER_CONST(Object, SHAg);
    URHO3D_SHADER_CONST(Object, SHAb);
    URHO3D_SHADER_CONST(Object, SHBr);
    URHO3D_SHADER_CONST(Object, SHBg);
    URHO3D_SHADER_CONST(Object, SHBb);
    URHO3D_SHADER_CONST(Object, SHC);
    URHO3D_SHADER_CONST(Object, Ambient);
    URHO3D_SHADER_CONST(Object, BillboardRot);
    URHO3D_SHADER_CONST(Object, SkinMatrices);
};

/// Built-in shader resources.
/// See _Samplers.glsl for descriptions.
namespace ShaderResources
{
    URHO3D_SHADER_RESOURCE(Albedo);
    URHO3D_SHADER_RESOURCE(Normal);
    URHO3D_SHADER_RESOURCE(Properties);
    URHO3D_SHADER_RESOURCE(Emission);
    URHO3D_SHADER_RESOURCE(Reflection0);
    URHO3D_SHADER_RESOURCE(Reflection1);
    URHO3D_SHADER_RESOURCE(LightRamp);
    URHO3D_SHADER_RESOURCE(LightShape);
    URHO3D_SHADER_RESOURCE(ShadowMap);
    URHO3D_SHADER_RESOURCE(DepthBuffer);
}

}
