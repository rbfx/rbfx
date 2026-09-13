// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Math/NumericRange.h"
#include "Urho3D/RenderPipeline/RenderPipelineDefs.h"
#include "Urho3D/RenderPipeline/PipelineBatchSortKey.h"
#include "Urho3D/Scene/Node.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Camera;
class Drawable;
class DrawableProcessor;
class Light;
class LightProcessor;

/// Manages single shadow split parameters and shadow casters.
/// Spot lights always have one split.
/// Directions lights have one split per cascade.
/// Point lights always have six splits.
class URHO3D_API ShadowSplitProcessor
{
public:
    ShadowSplitProcessor(LightProcessor* owner, unsigned splitIndex);
    ~ShadowSplitProcessor();

    /// Initialize split parameters
    /// @{
    void InitializeDirectional(DrawableProcessor* drawableProcessor,
        const FloatRange& splitRange, const ea::vector<Drawable*>& litGeometries);
    void InitializeSpot();
    void InitializePoint(CubeMapFace face);
    /// @}

    /// Process shadow casters
    /// @{
    void ProcessDirectionalShadowCasters(DrawableProcessor* drawableProcessor, ea::vector<Drawable*>& shadowCastersBuffer);
    void ProcessSpotShadowCasters(DrawableProcessor* drawableProcessor, const ea::vector<Drawable*>& shadowCasterCandidates);
    void ProcessPointShadowCasters(DrawableProcessor* drawableProcessor, const ea::vector<Drawable*>& shadowCasterCandidates);
    /// @}

    void FinalizeShadow(const ShadowMapRegion& shadowMap, unsigned pcfKernelSize);
    void FinalizeShadowBatches();

    /// Return immutable
    /// @{
    LightProcessor* GetLightProcessor() const { return lightProcessor_; }
    Light* GetLight() const { return light_; }
    unsigned GetSplitIndex() const { return splitIndex_; }
    /// @}

    /// Return values are valid after shadow casters are processed
    /// @{
    const auto& GetShadowCasters() const { return shadowCasters_; }
    bool HasShadowCasters() const { return !shadowCasters_.empty(); }
    /// @}

    /// Return values are valid after shadow map is finalized
    /// @{
    Matrix4 GetWorldToShadowSpaceMatrix(float subPixelOffset) const;
    const ShadowMapRegion& GetShadowMap() const { return shadowMap_; }
    float GetShadowMapTexelSizeInWorldSpace() const { return shadowMapWorldSpaceTexelSize_; }
    unsigned GetShadowMapPadding() const { return shadowMapPadding_; }
    const FloatRange& GetCascadeZRange() const { return cascadeZRange_; }
    Camera* GetShadowCamera() const { return shadowCamera_; }
    /// @}

    auto& GetMutableUnsortedShadowBatches() { return unsortedShadowBatches_; }
    auto& GetMutableShadowBatches() { return shadowBatches_; }
    const auto& GetShadowBatches() const { return shadowBatches_; }

private:
    void InitializeBaseDirectionalCamera(Camera* cullCamera);
    BoundingBox GetLitGeometriesBoundingBox(
        DrawableProcessor* drawableProcessor, const ea::vector<Drawable*>& litGeometries) const;
    BoundingBox GetSplitShadowBoundingBoxInLightSpace(
        DrawableProcessor* drawableProcessor, const ea::vector<Drawable*>& litGeometries) const;
    void AdjustDirectionalLightCamera(const BoundingBox& lightSpaceBoundingBox, float shadowMapSize);

    /// Immutable
    /// @{
    LightProcessor* lightProcessor_{};
    Light* light_{};
    unsigned splitIndex_{};
    RenderBackend renderBackend_{};
    /// @}

    /// Internal cached objects
    /// @{
    SharedPtr<Node> shadowCameraNode_;
    SharedPtr<Camera> shadowCamera_;
    /// @}

    /// Frame-specific objects
    /// @{
    FloatRange cascadeZRange_{};
    FloatRange focusedCascadeZRange_{};
    ea::vector<Drawable*> shadowCasters_;

    ShadowMapRegion shadowMap_;
    float shadowMapWorldSpaceTexelSize_{};
    unsigned shadowMapPadding_{};
    /// @}

    /// Shadow casters
    /// @{
    ea::vector<PipelineBatch> unsortedShadowBatches_;
    ea::vector<PipelineBatchByState> sortedShadowBatches_;
    PipelineBatchGroup<PipelineBatchByState> shadowBatches_;
    /// @}
};

}
