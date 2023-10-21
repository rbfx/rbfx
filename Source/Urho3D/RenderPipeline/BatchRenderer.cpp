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

#include "../Core/Context.h"
#include "../Graphics/Camera.h"
#include "../RenderAPI/DrawCommandQueue.h"
#include "../Graphics/Drawable.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsUtils.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/Zone.h"
#include "../IO/Log.h"
#include "../RenderAPI/RenderDevice.h"
#include "../RenderPipeline/BatchRenderer.h"
#include "../RenderPipeline/DrawableProcessor.h"
#include "../RenderPipeline/InstancingBuffer.h"
#include "../RenderPipeline/LightProcessor.h"
#include "../RenderPipeline/RenderPipelineDebugger.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../Scene/Scene.h"

#include <EASTL/sort.h>

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

/// Return shader parameter for camera depth mode.
Vector4 GetCameraDepthModeParameter(const Camera& camera, RenderBackend backend)
{
    Vector4 depthMode = Vector4::ZERO;
    if (camera.IsOrthographic())
    {
        depthMode.x_ = 1.0f;
        if (backend == RenderBackend::OpenGL)
        {
            depthMode.z_ = 0.5f;
            depthMode.w_ = 0.5f;
        }
        else
        {
            depthMode.z_ = 1.0f;
            depthMode.w_ = 0.0f;
        }
    }
    else
    {
        depthMode.w_ = 1.0f / camera.GetFarClip();
    }
    return depthMode;
}

/// Return shader parameter for camera depth reconstruction.
Vector4 GetCameraDepthReconstructParameter(const Camera& camera)
{
    const float nearClip = camera.GetNearClip();
    const float farClip = camera.GetFarClip();
    return {
        farClip / (farClip - nearClip),
        -nearClip / (farClip - nearClip),
        camera.IsOrthographic() ? 1.0f : 0.0f,
        camera.IsOrthographic() ? 0.0f : 1.0f
    };
}

/// Return shader parameter for zone fog.
Vector4 GetFogParameter(const Camera& camera)
{
    const float farClip = camera.GetFarClip();
    float fogStart = Min(camera.GetEffectiveFogStart(), farClip);
    float fogEnd = Min(camera.GetEffectiveFogEnd(), farClip);
    if (fogStart >= fogEnd * (1.0f - M_LARGE_EPSILON))
        fogStart = fogEnd * (1.0f - M_LARGE_EPSILON);
    const float fogRange = Max(fogEnd - fogStart, M_EPSILON);
    return {
        fogEnd / farClip,
        farClip / fogRange,
        0.0f,
        0.0f
    };
}

Vector4 GetClipPlane(const Camera& camera)
{
    if (!camera.GetUseClipping())
        return { 0, 0, 0, 1 };
    const Matrix4 viewProj = camera.GetGPUProjection() * camera.GetView();
    return camera.GetClipPlane().Transformed(viewProj).ToVector4();
}

/// Helper class to process per-object parameters.
class ObjectParameterBuilder : public NonCopyable
{
public:
    ObjectParameterBuilder(const BatchRendererSettings& settings, BatchRenderFlags flags)
        : instancingEnabled_(flags.Test(BatchRenderFlag::EnableInstancingForStaticGeometry))
        , ambientEnabled_(flags.Test(BatchRenderFlag::EnableAmbientLighting))
        , ambientMode_(settings.ambientMode_)
        , linearColorSpace_(flags.Test(BatchRenderFlag::LinearColorSpace))
    {
    }

    bool IsInstancingSupported() const { return instancingEnabled_; }
    bool IsAmbientEnabled() const { return ambientEnabled_; }

    /// Whether the batch should be processed using instancing.
    bool IsBatchInstanced(const PipelineBatch& pipelineBatch) const
    {
        return instancingEnabled_
            && pipelineBatch.geometry_->IsInstanced(pipelineBatch.geometryType_);
    }

    /// Set batch ambient lighting.
    void SetBatchAmbient(const LightAccumulator& lightAccumulator)
    {
        if (ambientMode_ == DrawableAmbientMode::Flat)
        {
            const Vector3 ambient = lightAccumulator.sphericalHarmonics_.EvaluateAverage();
            if (linearColorSpace_)
                ambientValueFlat_ = Vector4(ambient, 1.0f);
            else
                ambientValueFlat_ = Color(ambient).LinearToGamma().ToVector4();
        }
        else if (ambientMode_ == DrawableAmbientMode::Directional)
        {
            ambientValueSH_ = &lightAccumulator.sphericalHarmonics_;
        }
    }

    /// Add uniforms to instancing buffer for instanced batches.
    void AddBatchesToInstancingBuffer(InstancingBuffer& instancingBuffer,
        const SourceBatch& sourceBatch, unsigned instanceIndex)
    {
        instancingBuffer.AddInstance();
        instancingBuffer.SetElements(&sourceBatch.worldTransform_[instanceIndex], 0, 3);
        if (ambientEnabled_)
        {
            if (ambientMode_ == DrawableAmbientMode::Flat)
                instancingBuffer.SetElements(&ambientValueFlat_, 3, 1);
            else if (ambientMode_ == DrawableAmbientMode::Directional)
                instancingBuffer.SetElements(ambientValueSH_, 3, 7);
        }
    }

    /// Add uniforms to draw queue for non-instanced batch.
    void AddBatchUniformsToDrawQueue(DrawCommandQueue& drawQueue, const Node& cameraNode,
        const SourceBatch& sourceBatch, unsigned instanceIndex)
    {
        if (!drawQueue.BeginShaderParameterGroup(SP_OBJECT, true))
            return;

        if (ambientEnabled_)
        {
            if (ambientMode_ == DrawableAmbientMode::Flat)
                drawQueue.AddShaderParameter(ShaderConsts::Object_Ambient, ambientValueFlat_);
            else if (ambientMode_ == DrawableAmbientMode::Directional)
            {
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHAr, ambientValueSH_->Ar_);
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHAg, ambientValueSH_->Ag_);
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHAb, ambientValueSH_->Ab_);
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHBr, ambientValueSH_->Br_);
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHBg, ambientValueSH_->Bg_);
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHBb, ambientValueSH_->Bb_);
                drawQueue.AddShaderParameter(ShaderConsts::Object_SHC, ambientValueSH_->C_);
            }
        }

        switch (sourceBatch.geometryType_)
        {
        case GEOM_SKINNED:
            drawQueue.AddShaderParameter(ShaderConsts::Object_SkinMatrices,
                ea::span<const Matrix3x4>(sourceBatch.worldTransform_, ea::min(sourceBatch.numWorldTransforms_, Graphics::GetMaxBones())));
            break;

        case GEOM_BILLBOARD:
            drawQueue.AddShaderParameter(ShaderConsts::Object_Model, *sourceBatch.worldTransform_);
            if (sourceBatch.numWorldTransforms_ > 1)
            {
                drawQueue.AddShaderParameter(ShaderConsts::Object_BillboardRot,
                    sourceBatch.worldTransform_[1].RotationMatrix());
            }
            else
            {
                drawQueue.AddShaderParameter(ShaderConsts::Object_BillboardRot,
                    cameraNode.GetWorldRotation().RotationMatrix());
            }
            break;

        default:
            drawQueue.AddShaderParameter(ShaderConsts::Object_Model, sourceBatch.worldTransform_[instanceIndex]);
            break;
        }

        drawQueue.CommitShaderParameterGroup(SP_OBJECT);
    }

private:
    const bool instancingEnabled_;
    const bool ambientEnabled_;
    const DrawableAmbientMode ambientMode_;
    const bool linearColorSpace_;

    Vector4 ambientValueFlat_;
    const SphericalHarmonicsDot9* ambientValueSH_{};
};

/// Batch renderer to command queue.
template <bool DebuggerEnabled>
class DrawCommandCompositor : public NonCopyable, private BatchRenderingContext
{
public:
    /// Number of vec4 elements per vertex light.
    static const unsigned VertexLightStride = 3;

    DrawCommandCompositor(const BatchRenderingContext& ctx, const BatchRendererSettings& settings,
        RenderPipelineDebugger* debugger, const DrawableProcessor& drawableProcessor, const InstancingBuffer& instancingBuffer,
        BatchRenderFlags flags, unsigned startInstance)
        : BatchRenderingContext(ctx)
        , settings_(settings)
        , backend_(ctx.camera_.GetSubsystem<RenderDevice>()->GetBackend())
        , numVertexLights_(drawableProcessor.GetSettings().maxVertexLights_)
        , debugger_(debugger)
        , drawableProcessor_(drawableProcessor)
        , instancingBuffer_(instancingBuffer)
        , frameInfo_(drawableProcessor_.GetFrameInfo())
        , scene_(*frameInfo_.scene_)
        , lights_(drawableProcessor_.GetLightProcessors())
        , cameraNode_(*camera_.GetNode())
        , depthRange_(camera_.GetFarClip())
        , clipPlane_(GetClipPlane(camera_))
        , enabled_(flags, instancingBuffer)
        , objectParameterBuilder_(settings_, flags)
        , instanceIndex_(startInstance)
    {
        thread_local ea::vector<const ea::pair<const StringHash, MaterialShaderParameter>*> customMaterialParameters;
        customMaterialParameters_ = &customMaterialParameters;
    }

    /// Process batches
    /// @{
    void ProcessSceneBatch(const PipelineBatch& pipelineBatch)
    {
        ProcessBatch(pipelineBatch, pipelineBatch.GetSourceBatch());
    }

    void ProcessLightVolumeBatch(const PipelineBatch& pipelineBatch)
    {
        Light* light = lights_[pipelineBatch.pixelLightIndex_]->GetLight();
        lightVolumeHelpers_.transform_ = light->GetVolumeTransform(&camera_);
        lightVolumeHelpers_.sourceBatch_.worldTransform_ = &lightVolumeHelpers_.transform_;
        ProcessBatch(pipelineBatch, lightVolumeHelpers_.sourceBatch_);
    }

    void FlushDrawCommands(unsigned nextInstanceIndex)
    {
        if (instancingGroup_.count_ > 0)
            CommitInstancedDrawCalls();
        if (nextInstanceIndex != instanceIndex_)
        {
            URHO3D_LOGERROR("Instancing buffer is malformed");
            assert(0);
        }
    }
    /// @}

private:
    /// Extract and process batch state w/o any changes in DrawQueue
    /// @{
    void CheckDirtyCommonState(const PipelineBatch& pipelineBatch)
    {
        dirty_.pipelineState_ = current_.pipelineState_ != pipelineBatch.pipelineState_;
        current_.pipelineState_ = pipelineBatch.pipelineState_;

        const float constantDepthBias = current_.pipelineState_->GetDesc().AsGraphics()->constantDepthBias_;
        dirty_.cameraConstants_ = current_.constantDepthBias_ != constantDepthBias;
        current_.constantDepthBias_ = constantDepthBias;

        dirty_.material_ = current_.material_ != pipelineBatch.material_;
        current_.material_ = pipelineBatch.material_;

        dirty_.geometry_ = current_.geometry_ != pipelineBatch.geometry_;
        current_.geometry_ = pipelineBatch.geometry_;

        current_.drawable_ = pipelineBatch.drawable_;
    }

    void CheckDirtyReflectionProbe(const LightAccumulator& lightAccumulator)
    {
        dirty_.reflectionProbe_ = current_.reflectionProbes_ != lightAccumulator.reflectionProbes_
            || current_.reflectionProbesBlendFactor_ != lightAccumulator.reflectionProbesBlendFactor_;
        if (dirty_.reflectionProbe_)
        {
            current_.reflectionProbes_ = lightAccumulator.reflectionProbes_;
            current_.reflectionProbeTextures_[0] = lightAccumulator.reflectionProbes_[0]->reflectionMap_;
            current_.reflectionProbeTextures_[1] = lightAccumulator.reflectionProbes_[1]->reflectionMap_;
            current_.reflectionProbesBlendFactor_ = lightAccumulator.reflectionProbesBlendFactor_;
        }
    }

    void CheckDirtyPixelLight(const PipelineBatch& pipelineBatch)
    {
        dirty_.pixelLightConstants_ = current_.pixelLightIndex_ != pipelineBatch.pixelLightIndex_;
        if (!dirty_.pixelLightConstants_)
            return;

        current_.pixelLightIndex_ = pipelineBatch.pixelLightIndex_;
        current_.pixelLightEnabled_ = current_.pixelLightIndex_ != M_MAX_UNSIGNED;
        if (current_.pixelLightEnabled_)
        {
            current_.pixelLightParams_ = &lights_[current_.pixelLightIndex_]->GetParams();
            dirty_.pixelLightTextures_ = current_.pixelLightRamp_ != current_.pixelLightParams_->lightRamp_
                || current_.pixelLightShape_ != current_.pixelLightParams_->lightShape_
                || current_.pixelLightShadowMap_ != current_.pixelLightParams_->shadowMap_;
            if (dirty_.pixelLightTextures_)
            {
                current_.pixelLightRamp_ = current_.pixelLightParams_->lightRamp_;
                current_.pixelLightShape_ = current_.pixelLightParams_->lightShape_;
                current_.pixelLightShadowMap_ = current_.pixelLightParams_->shadowMap_;
            }
        }
    }

    void CheckDirtyVertexLight(const LightAccumulator& lightAccumulator)
    {
        const auto previousVertexLights = current_.vertexLights_;
        current_.vertexLights_ = lightAccumulator.GetVertexLights();
        dirty_.vertexLightConstants_ = previousVertexLights != current_.vertexLights_;
        if (!dirty_.vertexLightConstants_)
            return;

        static const CookedLightParams nullVertexLight{};
        for (unsigned i = 0; i < MAX_VERTEX_LIGHTS; ++i)
        {
            const CookedLightParams& params = current_.vertexLights_[i] != M_MAX_UNSIGNED
                ? lights_[current_.vertexLights_[i]]->GetParams() : nullVertexLight;
            const Vector3& color = params.GetColor(enabled_.linearColorSpace_);

            current_.vertexLightsData_[i * VertexLightStride] = {color, params.inverseRange_};
            current_.vertexLightsData_[i * VertexLightStride + 1] = {params.direction_, params.spotCutoff_};
            current_.vertexLightsData_[i * VertexLightStride + 2] = {params.position_, params.inverseSpotCutoff_};
        }
    }

    void CheckDirtyLightmap(const SourceBatch& sourceBatch)
    {
        dirty_.lightmapConstants_ = current_.lightmapScaleOffset_ != sourceBatch.lightmapScaleOffset_;
        if (!dirty_.lightmapConstants_)
            return;

        current_.lightmapScaleOffset_ = sourceBatch.lightmapScaleOffset_;

        Texture2D* lightmapTexture = current_.lightmapScaleOffset_
            ? scene_.GetLightmapTexture(sourceBatch.lightmapIndex_)
            : nullptr;

        dirty_.lightmapTextures_ = current_.lightmapTexture_ != lightmapTexture;
        current_.lightmapTexture_ = lightmapTexture;
    }
    /// @}

    /// Commit changes to DrawQueue
    /// @{
    void UpdateDirtyConstants()
    {
        if (drawQueue_.BeginShaderParameterGroup(SP_FRAME, false))
        {
            AddFrameConstants();
            drawQueue_.CommitShaderParameterGroup(SP_FRAME);
        }

        if (drawQueue_.BeginShaderParameterGroup(SP_CAMERA, dirty_.cameraConstants_))
        {
            AddCameraConstants(current_.constantDepthBias_);
            drawQueue_.CommitShaderParameterGroup(SP_CAMERA);
        }

        if (enabled_.ambientLighting_)
        {
            if (drawQueue_.BeginShaderParameterGroup(SP_ZONE, dirty_.reflectionProbe_))
            {
                AddReflectionProbeConstants();
                drawQueue_.CommitShaderParameterGroup(SP_ZONE);
            }
        }

        // Commit pixel light constants once during shadow map rendering to support normal bias
        if (outputShadowSplit_ != nullptr)
        {
            if (drawQueue_.BeginShaderParameterGroup(SP_LIGHT, false))
            {
                const CookedLightParams& params = outputShadowSplit_->GetLightProcessor()->GetParams();
                AddPixelLightConstants(params);
                drawQueue_.CommitShaderParameterGroup(SP_LIGHT);
            }
        }
        else if (enabled_.anyLighting_)
        {
            const bool lightConstantsDirty = dirty_.pixelLightConstants_ || dirty_.vertexLightConstants_;
            if (drawQueue_.BeginShaderParameterGroup(SP_LIGHT, lightConstantsDirty))
            {
                if (enabled_.vertexLighting_)
                    AddVertexLightConstants();
                if (current_.pixelLightEnabled_)
                    AddPixelLightConstants(*current_.pixelLightParams_);
                drawQueue_.CommitShaderParameterGroup(SP_LIGHT);
            }
            dirty_.pixelLightConstants_ = false;
            dirty_.vertexLightConstants_ = false;
        }

        customMaterialParameters_->clear();
        if (drawQueue_.BeginShaderParameterGroup(SP_MATERIAL, dirty_.material_ || dirty_.lightmapConstants_))
        {
            // TODO: This block may be cached for some APIs
            const auto& materialParameters = current_.material_->GetShaderParameters();
            for (const auto& parameter : materialParameters)
            {
                if (parameter.second.isCustom_)
                {
                    customMaterialParameters_->push_back(&parameter);
                    continue;
                }

                if (parameter.first == ShaderConsts::Material_FadeOffsetScale)
                {
                    const Vector2 param = parameter.second.value_.GetVector2();
                    const Vector2 paramAdjusted{param.x_ / depthRange_, depthRange_ / param.y_ };
                    drawQueue_.AddShaderParameter(parameter.first, paramAdjusted);
                }
                else
                    drawQueue_.AddShaderParameter(parameter.first, parameter.second.value_);
            }

            if (enabled_.ambientLighting_ && current_.lightmapScaleOffset_)
                drawQueue_.AddShaderParameter(ShaderConsts::Material_LMOffset, *current_.lightmapScaleOffset_);

            drawQueue_.CommitShaderParameterGroup(SP_MATERIAL);
        }
        dirty_.lightmapConstants_ = false;

        if (!customMaterialParameters_->empty())
        {
            if (drawQueue_.BeginShaderParameterGroup(SP_CUSTOM, true))
            {
                for (const auto* parameter : *customMaterialParameters_)
                    drawQueue_.AddShaderParameter(parameter->first, parameter->second.value_);
                drawQueue_.CommitShaderParameterGroup(SP_CUSTOM);
            }
        }
    }

    void UpdateDirtyResources()
    {
        const bool resourcesDirty =
            dirty_.pipelineState_ || dirty_.material_ || dirty_.reflectionProbe_ || dirty_.IsResourcesDirty();
        if (resourcesDirty)
        {
            for (const ShaderResourceDesc& desc : globalResources_)
            {
                if (desc.texture_)
                    drawQueue_.AddShaderResource(desc.name_, desc.texture_);
            }

            const auto& materialTextures = current_.material_->GetTextures();
            bool materialHasEnvironmentMap = false;
            for (const auto& [nameHash, texture] : materialTextures)
            {
                if (nameHash == ShaderResources::Reflection0)
                    materialHasEnvironmentMap = true;
                // Emissive texture is used for lightmaps and refraction background, skip if necessary
                if (nameHash == ShaderResources::Emission && current_.lightmapTexture_)
                    continue;

                AddShaderResource(drawQueue_, nameHash, texture.value_);
            }

            if (current_.lightmapTexture_)
                AddShaderResource(drawQueue_, ShaderResources::Emission, current_.lightmapTexture_);
            if (current_.pixelLightRamp_)
                AddShaderResource(drawQueue_, ShaderResources::LightRamp, current_.pixelLightRamp_);
            if (current_.pixelLightShape_)
                AddShaderResource(drawQueue_, ShaderResources::LightShape, current_.pixelLightShape_);
            if (current_.pixelLightShadowMap_)
                AddShaderResource(drawQueue_, ShaderResources::ShadowMap, current_.pixelLightShadowMap_);
            if (enabled_.ambientLighting_ && !materialHasEnvironmentMap)
            {
                AddNullableShaderResource(drawQueue_, ShaderResources::Reflection0, TextureType::TextureCube,
                    current_.reflectionProbeTextures_[0]);
                AddNullableShaderResource(drawQueue_, ShaderResources::Reflection1, TextureType::TextureCube,
                    current_.reflectionProbeTextures_[1]);
            }

            drawQueue_.CommitShaderResources();

            dirty_.lightmapTextures_ = false;
            dirty_.pixelLightTextures_ = false;
        }
    }

    void AddFrameConstants()
    {
        for (const ShaderParameterDesc& shaderParameter : frameParameters_)
            drawQueue_.AddShaderParameter(shaderParameter.name_, shaderParameter.value_);

        drawQueue_.AddShaderParameter(ShaderConsts::Frame_DeltaTime, frameInfo_.timeStep_);
        drawQueue_.AddShaderParameter(ShaderConsts::Frame_ElapsedTime, scene_.GetElapsedTime());
    }

    void AddCameraConstants(float constantDepthBias)
    {
        for (const ShaderParameterDesc& shaderParameter : cameraParameters_)
            drawQueue_.AddShaderParameter(shaderParameter.name_, shaderParameter.value_);

        // XR: we can tell we're not in a shadowmap pass by the instanceMultiplier_ at present and
        // for good measure check the camera matches the first camera of additionalCameras_
        // would need to change if shadows ever used multiplexed instancing to write
        // to layers / viewports / or manually-clip
        if (instanceMultiplier_ > 1 && frameInfo_.additionalCameras_[0] == &camera_)
        {
            // stereoscopic, pull parameters for both eyes
            // Assumption: quad-views eye-focus targets would be best done as a second stereo target pass
            // because of the difference in frustum sizes (large for far field, narrow for focus)

            const Camera* cameras[2] = {frameInfo_.additionalCameras_[0], frameInfo_.additionalCameras_[1]};

            // clang-format off
            const Matrix4 cameraTransform[2] = {cameras[0]->GetEffectiveWorldTransform().ToMatrix4(), cameras[1]->GetEffectiveWorldTransform().ToMatrix4()};
            const Vector4 cameraPosition[2] = {{cameraTransform[0].Translation(), 0.0f}, {cameraTransform[1].Translation(), 0.0f}};
            const Matrix4 cameraView[2] = {cameras[0]->GetView().ToMatrix4(), cameras[1]->GetView().ToMatrix4()};
            const Matrix4 cameraViewProj[2] = {cameras[0]->GetEffectiveGPUViewProjection(constantDepthBias), cameras[1]->GetEffectiveGPUViewProjection(constantDepthBias)};
            const Vector4 depthModes[2] = {GetCameraDepthModeParameter(*cameras[0], backend_), GetCameraDepthModeParameter(*cameras[1], backend_)};
            const Vector4 depthReconstructions[2] = {GetCameraDepthReconstructParameter(*cameras[0]), GetCameraDepthReconstructParameter(*cameras[1])};
            // clang-format on

            drawQueue_.AddShaderParameter(ShaderConsts::Camera_CameraPos, cameraPosition);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_ViewInv, cameraTransform);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_View, cameraView);

            drawQueue_.AddShaderParameter(ShaderConsts::Camera_DepthMode, depthModes);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_DepthReconstruct, depthReconstructions);

            Vector3 nearVector[2]{};
            Vector3 farVector[2]{};
            cameras[0]->GetFrustumSize(nearVector[0], farVector[0]);
            cameras[1]->GetFrustumSize(nearVector[1], farVector[1]);
            const Vector4 farVectors[2] = {{farVector[0], 0.0f}, {farVector[1], 0.0f}};
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_FrustumSize, farVectors);

            drawQueue_.AddShaderParameter(ShaderConsts::Camera_ViewProj, cameraViewProj);
        }
        else
        {
            // Not stereoscopic rendering, single eye
            const Matrix3x4 cameraTransform = camera_.GetEffectiveWorldTransform();
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_CameraPos, Vector4(cameraTransform.Translation(), 0.0f));
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_ViewInv, cameraTransform);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_View, camera_.GetView());

            drawQueue_.AddShaderParameter(ShaderConsts::Camera_DepthMode, GetCameraDepthModeParameter(camera_, backend_));
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_DepthReconstruct, GetCameraDepthReconstructParameter(camera_));

            Vector3 nearVector, farVector;
            camera_.GetFrustumSize(nearVector, farVector);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_FrustumSize, Vector4(farVector, 0.0f));

            drawQueue_.AddShaderParameter(ShaderConsts::Camera_ViewProj, camera_.GetEffectiveGPUViewProjection(constantDepthBias));
        }

        // Fill non-stereo parameters
        drawQueue_.AddShaderParameter(ShaderConsts::Camera_ClipPlane, clipPlane_);
        drawQueue_.AddShaderParameter(ShaderConsts::Camera_NearClip, camera_.GetNearClip());
        drawQueue_.AddShaderParameter(ShaderConsts::Camera_FarClip, camera_.GetFarClip());

        // Fill shadow split only parameters
        if (outputShadowSplit_)
        {
            const CookedLightParams& lightParams = outputShadowSplit_->GetLightProcessor()->GetParams();
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_NormalOffsetScale,
                lightParams.shadowNormalBias_[outputShadowSplit_->GetSplitIndex()]);
        }

        // Fill color output only parameters
        if (enabled_.colorOutput_)
        {
            const Color ambientColorGamma = camera_.GetEffectiveAmbientColor() * camera_.GetEffectiveAmbientBrightness();
            const Color& fogColorGamma = camera_.GetEffectiveFogColor();
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_AmbientColor,
                enabled_.linearColorSpace_ ? ambientColorGamma.GammaToLinear() : ambientColorGamma);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_FogColor,
                enabled_.linearColorSpace_ ? fogColorGamma.GammaToLinear() : fogColorGamma);
            drawQueue_.AddShaderParameter(ShaderConsts::Camera_FogParams, GetFogParameter(camera_));
        }
    }

    void AddReflectionProbeConstants()
    {
        if (!enabled_.colorOutput_)
            return;

        if (settings_.cubemapBoxProjection_)
        {
            drawQueue_.AddShaderParameter(ShaderConsts::Zone_CubemapCenter0,
                current_.reflectionProbes_[0]->cubemapCenter_);
            drawQueue_.AddShaderParameter(ShaderConsts::Zone_ProjectionBoxMin0,
                current_.reflectionProbes_[0]->projectionBox_.min_.ToVector4());
            drawQueue_.AddShaderParameter(ShaderConsts::Zone_ProjectionBoxMax0,
                current_.reflectionProbes_[0]->projectionBox_.max_.ToVector4());
        }

        drawQueue_.AddShaderParameter(ShaderConsts::Zone_RoughnessToLODFactor0,
            current_.reflectionProbes_[0]->roughnessToLODFactor_);

        if (settings_.cubemapBoxProjection_)
        {
            drawQueue_.AddShaderParameter(ShaderConsts::Zone_CubemapCenter1,
                current_.reflectionProbes_[1]->cubemapCenter_);
            drawQueue_.AddShaderParameter(ShaderConsts::Zone_ProjectionBoxMin1,
                current_.reflectionProbes_[1]->projectionBox_.min_.ToVector4());
            drawQueue_.AddShaderParameter(ShaderConsts::Zone_ProjectionBoxMax1,
                current_.reflectionProbes_[1]->projectionBox_.max_.ToVector4());
        }

        drawQueue_.AddShaderParameter(ShaderConsts::Zone_RoughnessToLODFactor1,
            current_.reflectionProbes_[1]->roughnessToLODFactor_);
        drawQueue_.AddShaderParameter(ShaderConsts::Zone_ReflectionBlendFactor,
            current_.reflectionProbesBlendFactor_);
    }

    void AddVertexLightConstants()
    {
        drawQueue_.AddShaderParameter(ShaderConsts::Light_VertexLights,
            ea::span<const Vector4>{ current_.vertexLightsData_, numVertexLights_ * VertexLightStride });
    }

    void AddPixelLightConstants(const CookedLightParams& params)
    {
        drawQueue_.AddShaderParameter(ShaderConsts::Light_LightDir, params.direction_);
        drawQueue_.AddShaderParameter(ShaderConsts::Light_LightPos, Vector4{params.position_, params.inverseRange_});

        // Shadow maps need only light position and direction for normal bias
        if (!enabled_.colorOutput_)
            return;

        drawQueue_.AddShaderParameter(ShaderConsts::Light_LightColor,
            Vector4{params.GetColor(enabled_.linearColorSpace_), params.effectiveSpecularIntensity_});

        drawQueue_.AddShaderParameter(ShaderConsts::Light_LightRad, params.volumetricRadius_);
        drawQueue_.AddShaderParameter(ShaderConsts::Light_LightLength, params.volumetricLength_);

        drawQueue_.AddShaderParameter(ShaderConsts::Light_SpotAngle, Vector2{ params.spotCutoff_, params.inverseSpotCutoff_ });
        if (params.lightShape_)
            drawQueue_.AddShaderParameter(ShaderConsts::Light_LightShapeMatrix, params.lightShapeMatrix_);

        if (params.numLightMatrices_ > 0)
        {
            ea::span<const Matrix4> shadowMatricesSpan{ params.lightMatrices_.data(), params.numLightMatrices_ };
            drawQueue_.AddShaderParameter(ShaderConsts::Light_LightMatrices, shadowMatricesSpan);
        }

        if (params.shadowMap_)
        {
            drawQueue_.AddShaderParameter(ShaderConsts::Light_ShadowDepthFade, params.shadowDepthFade_);
            drawQueue_.AddShaderParameter(ShaderConsts::Light_ShadowIntensity, params.shadowIntensity_);
            drawQueue_.AddShaderParameter(ShaderConsts::Light_ShadowMapInvSize, params.shadowMapInvSize_);
            drawQueue_.AddShaderParameter(ShaderConsts::Light_ShadowSplits, params.shadowSplitDistances_);
            drawQueue_.AddShaderParameter(ShaderConsts::Light_ShadowCubeUVBias, params.shadowCubeUVBias_);
            drawQueue_.AddShaderParameter(ShaderConsts::Light_ShadowCubeAdjust, params.shadowCubeAdjust_);
            drawQueue_.AddShaderParameter(ShaderConsts::Light_VSMShadowParams, settings_.varianceShadowMapParams_);
        }
    }
    /// @}

    /// Draw ops
    /// @{
    void CommitDrawCalls(unsigned numInstances, const SourceBatch& sourceBatch)
    {
        const bool hasIndexBuffer = current_.geometry_->GetIndexBuffer() != nullptr;

        if (dirty_.geometry_)
            SetBuffersFromGeometry(drawQueue_, current_.geometry_);

        for (unsigned i = 0; i < numInstances; ++i)
        {
            if (frameInfo_.viewReferenceNode_)
                objectParameterBuilder_.AddBatchUniformsToDrawQueue(drawQueue_, *frameInfo_.viewReferenceNode_, sourceBatch, i);
            else
                objectParameterBuilder_.AddBatchUniformsToDrawQueue(drawQueue_, cameraNode_, sourceBatch, i);

            if (instanceMultiplier_ == 1)
            {
                if (hasIndexBuffer)
                    drawQueue_.DrawIndexed(current_.geometry_->GetIndexStart(), current_.geometry_->GetIndexCount());
                else
                    drawQueue_.Draw(current_.geometry_->GetVertexStart(), current_.geometry_->GetVertexCount());
            }
            else // if multiply instance counts for a purpose such as stereo then it's necessary to replace uninstanced draws with instanced draws.
            {
                if (hasIndexBuffer)
                    drawQueue_.DrawIndexedInstanced(current_.geometry_->GetIndexStart(), current_.geometry_->GetIndexCount(), 0, instanceMultiplier_);
                else
                    drawQueue_.DrawInstanced(current_.geometry_->GetVertexStart(), current_.geometry_->GetVertexCount(), 0, instanceMultiplier_);
            }
        }
    }

    void CommitInstancedDrawCalls()
    {
        assert(instancingGroup_.count_ > 0);
        Geometry* geometry = instancingGroup_.geometry_;
        SetBuffersFromGeometry(drawQueue_, geometry, instancingBuffer_.GetVertexBuffer());
        drawQueue_.DrawIndexedInstanced(geometry->GetIndexStart(), geometry->GetIndexCount(),
            instancingGroup_.start_, instancingGroup_.count_ * instanceMultiplier_ /*multiply for stereo*/);
        instancingGroup_.count_ = 0;
    }
    /// @}

    void ProcessBatch(const PipelineBatch& pipelineBatch, const SourceBatch& sourceBatch)
    {
        if (pipelineBatch.geometry_->GetEffectiveIndexCount() == 0)
            return;

        const LightAccumulator* lightAccumulator = enabled_.ambientLighting_ || enabled_.vertexLighting_
            ? &drawableProcessor_.GetGeometryLighting(pipelineBatch.drawableIndex_)
            : nullptr;

        // Update dirty flags and cached state
        CheckDirtyCommonState(pipelineBatch);
        if (enabled_.pixelLighting_)
            CheckDirtyPixelLight(pipelineBatch);
        if (enabled_.vertexLighting_)
            CheckDirtyVertexLight(*lightAccumulator);
        if (enabled_.ambientLighting_)
        {
            CheckDirtyReflectionProbe(*lightAccumulator);
            CheckDirtyLightmap(sourceBatch);
        }

        const unsigned numBatchInstances = pipelineBatch.geometryType_ == GEOM_STATIC
            ? sourceBatch.numWorldTransforms_ : 1u;

        const bool resetInstancingGroup = instancingGroup_.count_ == 0 || dirty_.IsAnythingDirty();

        if constexpr (DebuggerEnabled)
            debugger_->ReportSceneBatch(DebugFrameSnapshotBatch{ drawableProcessor_, pipelineBatch, resetInstancingGroup });

        if (resetInstancingGroup)
        {
            if (instancingGroup_.count_ > 0)
                CommitInstancedDrawCalls();

            if (dirty_.pipelineState_)
                drawQueue_.SetPipelineState(current_.pipelineState_);

            if (enabled_.lightMaskToStencil_)
                drawQueue_.SetStencilRef(current_.drawable_->GetLightMaskInZone() & PORTABLE_LIGHTMASK);

            UpdateDirtyConstants();
            UpdateDirtyResources();

            if (objectParameterBuilder_.IsBatchInstanced(pipelineBatch))
            {
                instancingGroup_.count_ = numBatchInstances;
                instancingGroup_.start_ = instanceIndex_;
                instancingGroup_.geometry_ = current_.geometry_;
                instanceIndex_ += numBatchInstances;
            }
            else
            {
                if (objectParameterBuilder_.IsAmbientEnabled())
                    objectParameterBuilder_.SetBatchAmbient(*lightAccumulator);
                CommitDrawCalls(numBatchInstances, sourceBatch);
            }
        }
        else
        {
            instancingGroup_.count_ += numBatchInstances;
            instanceIndex_ += numBatchInstances;
        }
    }

    /// External state (required)
    /// @{
    const BatchRendererSettings& settings_;
    const RenderBackend backend_{};
    const unsigned numVertexLights_{};
    RenderPipelineDebugger* debugger_{};
    const DrawableProcessor& drawableProcessor_;
    const InstancingBuffer& instancingBuffer_;
    const FrameInfo& frameInfo_;
    const Scene& scene_;
    const ea::vector<LightProcessor*>& lights_;
    const Node& cameraNode_;
    const float depthRange_{};
    const Vector4 clipPlane_{};
    /// @}

    struct EnabledFeatureFlags
    {
        EnabledFeatureFlags(BatchRenderFlags flags, const InstancingBuffer& instancingBuffer)
            : ambientLighting_(flags.Test(BatchRenderFlag::EnableAmbientLighting))
            , vertexLighting_(flags.Test(BatchRenderFlag::EnableVertexLights))
            , pixelLighting_(flags.Test(BatchRenderFlag::EnablePixelLights))
            , anyLighting_(ambientLighting_ || vertexLighting_ || pixelLighting_)
            , colorOutput_(!flags.Test(BatchRenderFlag::DisableColorOutput))
            , lightMaskToStencil_(flags.Test(BatchRenderFlag::LightMaskToStencil))
            , linearColorSpace_(flags.Test(BatchRenderFlag::LinearColorSpace))
        {
        }

        bool ambientLighting_;
        bool vertexLighting_;
        bool pixelLighting_;
        bool anyLighting_;
        bool colorOutput_;
        bool lightMaskToStencil_;
        bool linearColorSpace_;
    } const enabled_;

    struct DirtyStateFlags
    {
        /// Cleared automatically
        /// @{
        bool pipelineState_{};
        bool material_{};
        bool geometry_{};
        bool reflectionProbe_{};

        bool IsStateDirty() const
        {
            return pipelineState_ || material_ || geometry_ || reflectionProbe_;
        }
        /// @}

        /// Cleared automatically
        /// @{
        bool cameraConstants_{ true };
        bool pixelLightConstants_{};
        bool vertexLightConstants_{};
        bool lightmapConstants_{};

        bool IsConstantsDirty() const
        {
            return cameraConstants_ || pixelLightConstants_ || vertexLightConstants_ || lightmapConstants_;
        }
        /// @}

        /// Should be cleared in resource filler
        /// @{
        bool pixelLightTextures_{};
        bool lightmapTextures_{};

        bool IsResourcesDirty() const
        {
            return pixelLightTextures_ || lightmapTextures_;
        }
        /// @}

        bool IsAnythingDirty() const
        {
            return IsStateDirty() || IsConstantsDirty() || IsResourcesDirty();
        }
    } dirty_;

    struct CachedSharedState
    {
        PipelineState* pipelineState_{};
        float constantDepthBias_{};

        ea::array<const ReflectionProbeData*, 2> reflectionProbes_{};
        ea::array<TextureCube*, 2> reflectionProbeTextures_{};
        float reflectionProbesBlendFactor_{};

        unsigned pixelLightIndex_{ M_MAX_UNSIGNED };
        bool pixelLightEnabled_{};
        const CookedLightParams* pixelLightParams_{};
        Texture* pixelLightRamp_{};
        Texture* pixelLightShape_{};
        Texture* pixelLightShadowMap_{};

        LightAccumulator::VertexLightContainer vertexLights_{};
        Vector4 vertexLightsData_[MAX_VERTEX_LIGHTS * VertexLightStride]{};

        Texture* lightmapTexture_{};
        const Vector4* lightmapScaleOffset_{};

        Material* material_{};
        Geometry* geometry_{};
        Drawable* drawable_{};
    } current_;

    struct InstancingGroupState
    {
        Geometry* geometry_{};
        unsigned start_{};
        unsigned count_{};
    } instancingGroup_;

    struct LightVolumeBatchHelpers
    {
        Matrix3x4 transform_;
        SourceBatch sourceBatch_;
    } lightVolumeHelpers_;

    ObjectParameterBuilder objectParameterBuilder_;
    unsigned instanceIndex_{};

    ea::vector<const ea::pair<const StringHash, MaterialShaderParameter>*>* customMaterialParameters_{};
};

}

BatchRenderingContext::BatchRenderingContext(DrawCommandQueue& drawQueue, const Camera& camera)
    : drawQueue_(drawQueue)
    , camera_(camera)
{
}

BatchRenderingContext::BatchRenderingContext(DrawCommandQueue& drawQueue, const ShadowSplitProcessor& outputShadowSplit)
    : drawQueue_(drawQueue)
    , camera_(*outputShadowSplit.GetShadowCamera())
    , outputShadowSplit_(&outputShadowSplit)
{
}

BatchRenderer::BatchRenderer(RenderPipelineInterface* renderPipeline, const DrawableProcessor* drawableProcessor,
    InstancingBuffer* instancingBuffer)
    : Object(renderPipeline->GetContext())
    , renderer_(context_->GetSubsystem<Renderer>())
    , debugger_(renderPipeline->GetDebugger())
    , drawableProcessor_(drawableProcessor)
    , instancingBuffer_(instancingBuffer)
{
}

void BatchRenderer::SetSettings(const BatchRendererSettings& settings)
{
    settings_ = settings;
}

void BatchRenderer::RenderBatches(const BatchRenderingContext& ctx, PipelineBatchGroup<PipelineBatchByState> batchGroup)
{
    batchGroup.flags_ = AdjustRenderFlags(batchGroup.flags_);

    if (RenderPipelineDebugger::IsSnapshotInProgress(debugger_))
    {
        DrawCommandCompositor<true> compositor(ctx, settings_, debugger_,
            *drawableProcessor_, *instancingBuffer_, batchGroup.flags_, batchGroup.startInstance_);
        for (const auto& sortedBatch : batchGroup.batches_)
            compositor.ProcessSceneBatch(*sortedBatch.pipelineBatch_);
        compositor.FlushDrawCommands(batchGroup.startInstance_ + batchGroup.numInstances_);
    }
    else
    {
        DrawCommandCompositor<false> compositor(ctx, settings_, nullptr,
            *drawableProcessor_, *instancingBuffer_, batchGroup.flags_, batchGroup.startInstance_);
        for (const auto& sortedBatch : batchGroup.batches_)
            compositor.ProcessSceneBatch(*sortedBatch.pipelineBatch_);
        compositor.FlushDrawCommands(batchGroup.startInstance_ + batchGroup.numInstances_);
    }
}

void BatchRenderer::RenderBatches(const BatchRenderingContext& ctx, PipelineBatchGroup<PipelineBatchBackToFront> batchGroup)
{
    batchGroup.flags_ = AdjustRenderFlags(batchGroup.flags_);

    if (RenderPipelineDebugger::IsSnapshotInProgress(debugger_))
    {
        DrawCommandCompositor<true> compositor(ctx, settings_, debugger_,
            *drawableProcessor_, *instancingBuffer_, batchGroup.flags_, batchGroup.startInstance_);
        for (const auto& sortedBatch : batchGroup.batches_)
            compositor.ProcessSceneBatch(*sortedBatch.pipelineBatch_);
        compositor.FlushDrawCommands(batchGroup.startInstance_ + batchGroup.numInstances_);
    }
    else
    {
        DrawCommandCompositor<false> compositor(ctx, settings_, nullptr,
            *drawableProcessor_, *instancingBuffer_, batchGroup.flags_, batchGroup.startInstance_);
        for (const auto& sortedBatch : batchGroup.batches_)
            compositor.ProcessSceneBatch(*sortedBatch.pipelineBatch_);
        compositor.FlushDrawCommands(batchGroup.startInstance_ + batchGroup.numInstances_);
    }
}

void BatchRenderer::RenderLightVolumeBatches(const BatchRenderingContext& ctx,
    ea::span<const PipelineBatchByState> batches)
{
    if (RenderPipelineDebugger::IsSnapshotInProgress(debugger_))
    {
        DrawCommandCompositor<true> compositor(ctx, settings_, debugger_,
            *drawableProcessor_, *instancingBuffer_, BatchRenderFlag::EnablePixelLights, 0);
        for (const auto& sortedBatch : batches)
            compositor.ProcessLightVolumeBatch(*sortedBatch.pipelineBatch_);
        compositor.FlushDrawCommands(0);
    }
    else
    {
        DrawCommandCompositor<false> compositor(ctx, settings_, nullptr,
            *drawableProcessor_, *instancingBuffer_, BatchRenderFlag::EnablePixelLights, 0);
        for (const auto& sortedBatch : batches)
            compositor.ProcessLightVolumeBatch(*sortedBatch.pipelineBatch_);
        compositor.FlushDrawCommands(0);
    }
}

void BatchRenderer::PrepareInstancingBuffer(PipelineBatchGroup<PipelineBatchByState>& batches)
{
    PrepareInstancingBufferImpl(batches);
}

void BatchRenderer::PrepareInstancingBuffer(PipelineBatchGroup<PipelineBatchBackToFront>& batches)
{
    PrepareInstancingBufferImpl(batches);
}

template <class T>
void BatchRenderer::PrepareInstancingBufferImpl(PipelineBatchGroup<T>& batches)
{
    batches.flags_ = AdjustRenderFlags(batches.flags_);
    batches.startInstance_ = 0;
    batches.numInstances_ = 0;

    ObjectParameterBuilder objectParameterBuilder(settings_, batches.flags_);
    if (!objectParameterBuilder.IsInstancingSupported())
        return;

    batches.startInstance_ = instancingBuffer_->GetNextInstanceIndex();
    for (const T& sortedBatch : batches.batches_)
    {
        const PipelineBatch& pipelineBatch = *sortedBatch.pipelineBatch_;
        if (!objectParameterBuilder.IsBatchInstanced(pipelineBatch))
            continue;

        const SourceBatch& sourceBatch = pipelineBatch.GetSourceBatch();
        if (objectParameterBuilder.IsAmbientEnabled())
        {
            const LightAccumulator& lightAccumulator = drawableProcessor_->GetGeometryLighting(pipelineBatch.drawableIndex_);
            objectParameterBuilder.SetBatchAmbient(lightAccumulator);
        }

        for (unsigned i = 0; i < sourceBatch.numWorldTransforms_; ++i)
            objectParameterBuilder.AddBatchesToInstancingBuffer(*instancingBuffer_, sourceBatch, i);
        batches.numInstances_ += sourceBatch.numWorldTransforms_;
    }
}

BatchRenderFlags BatchRenderer::AdjustRenderFlags(BatchRenderFlags flags) const
{
    if (!instancingBuffer_->IsEnabled())
        flags.Set(BatchRenderFlag::EnableInstancingForStaticGeometry, false);
    return flags;
}

}
