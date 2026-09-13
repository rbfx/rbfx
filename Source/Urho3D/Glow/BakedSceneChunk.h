// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Glow/BakedLight.h"
#include "Urho3D/Glow/BakedSceneCollector.h"
#include "Urho3D/Glow/LightmapGeometryBuffer.h"
#include "Urho3D/Glow/RaytracerScene.h"
#include "Urho3D/Graphics/LightProbeGroup.h"

namespace Urho3D
{

/// Light probe collection with extra data needed for baking.
struct LightProbeCollectionForBaking : public LightProbeCollection
{
    /// Size is the same as number of probes.
    /// @{
    ea::vector<unsigned> lightMasks_;
    ea::vector<unsigned> backgroundIds_;
    /// @}
};

/// Baking chunk. Contains everything to bake light for given chunk.
struct BakedSceneChunk
{
    /// Lightmaps owned by this chunk.
    ea::vector<unsigned> lightmaps_;
    /// Direct lightmaps required to bake this chunk.
    ea::vector<unsigned> requiredDirectLightmaps_;

    /// Raytracer scene.
    SharedPtr<RaytracerScene> raytracerScene_;
    /// Geometry buffers.
    ea::vector<LightmapChartGeometryBuffer> geometryBuffers_;
    /// Geometry buffer ID to raytracer geometry ID mapping.
    ea::vector<unsigned> geometryBufferToRaytracer_;
    /// Lights to bake.
    ea::vector<BakedLight> bakedLights_;
    /// Light probes collection.
    LightProbeCollectionForBaking lightProbesCollection_;
    /// Number of unique light probe groups. Used for saving results.
    unsigned numUniqueLightProbes_{};
};

/// Create baked scene chunk.
URHO3D_API BakedSceneChunk CreateBakedSceneChunk(Context* context,
    BakedSceneCollector& collector, const IntVector3& chunk, const LightBakingSettings& settings);

}
