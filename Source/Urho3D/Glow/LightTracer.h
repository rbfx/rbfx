// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Glow/BakedLight.h"
#include "Urho3D/Glow/LightmapCharter.h"
#include "Urho3D/Glow/LightmapGeometryBuffer.h"
#include "Urho3D/Graphics/LightProbeGroup.h"

namespace Urho3D
{

class RaytracerScene;
class TetrahedralMesh;
struct LightProbeCollectionForBaking;

/// Preprocess geometry buffer. Fix shadow bleeding.
URHO3D_API void PreprocessGeometryBuffer(LightmapChartGeometryBuffer& geometryBuffer,
    const RaytracerScene& raytracerScene, const ea::vector<unsigned>& geometryBufferToRaytracer,
    const GeometryBufferPreprocessSettings& settings);

/// Direct light accumulated for given lightmap chart.
struct LightmapChartBakedDirect
{
    /// Construct default.
    LightmapChartBakedDirect() = default;
    /// Construct valid.
    explicit LightmapChartBakedDirect(unsigned lightmapSize)
        : lightmapSize_(lightmapSize)
        , realLightmapSize_(static_cast<float>(lightmapSize_))
        , directLight_(lightmapSize_ * lightmapSize_, Vector3::ZERO)
        , surfaceLight_(lightmapSize_ * lightmapSize_)
        , albedo_(lightmapSize_ * lightmapSize_)
    {
    }
    /// Return nearest point location by UV.
    IntVector2 GetNearestLocation(const Vector2& uv) const
    {
        const int x = FloorToInt(ea::min(uv.x_ * realLightmapSize_, realLightmapSize_ - 1.0f));
        const int y = FloorToInt(ea::min(uv.y_ * realLightmapSize_, realLightmapSize_ - 1.0f));
        return { x, y };
    }
    /// Return surface light for location.
    const Vector3& GetSurfaceLight(const IntVector2& location) const
    {
        const unsigned index = location.x_ + location.y_ * lightmapSize_;
        return surfaceLight_[index];
    }
    /// Return albedo for location.
    const Vector3& GetAlbedo(const IntVector2& location) const
    {
        const unsigned index = location.x_ + location.y_ * lightmapSize_;
        return albedo_[index];
    }

    /// Size of lightmap chart.
    unsigned lightmapSize_{};
    /// Size of lightmap chart as float.
    float realLightmapSize_{};
    /// Incoming direct light from completely backed lights, to be baked in lightmap.
    ea::vector<Vector3> directLight_;
    /// Incoming direct light from all static lights multiplied with albedo, used to calculate indirect lighting.
    ea::vector<Vector3> surfaceLight_;
    /// Albedo of the surface.
    ea::vector<Vector3> albedo_;
};

/// Indirect light accumulated for given lightmap chart.
struct LightmapChartBakedIndirect
{
    /// Construct default.
    LightmapChartBakedIndirect() = default;
    /// Construct valid.
    explicit LightmapChartBakedIndirect(unsigned lightmapSize)
        : lightmapSize_(lightmapSize)
        , light_(lightmapSize_ * lightmapSize_)
    {
    }
    /// Normalize collected light.
    void NormalizeLight()
    {
        for (Vector4& value : light_)
        {
            if (value.w_ > 0)
                value /= value.w_;
        }
    }

    /// Size of lightmap chart.
    unsigned lightmapSize_{};
    /// Indirect light. W component represents normalization weight.
    ea::vector<Vector4> light_;
};

/// Accumulate emission light.
URHO3D_API void BakeEmissionLight(LightmapChartBakedDirect& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const EmissionLightTracingSettings& settings, float indirectBrightnessMultiplier);

/// Accumulate direct light for charts.
URHO3D_API void BakeDirectLightForCharts(LightmapChartBakedDirect& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const RaytracerScene& raytracerScene, const ea::vector<unsigned>& geometryBufferToRaytracer,
    const BakedLight& light, const DirectLightTracingSettings& settings);

/// Accumulate direct light for light probes.
URHO3D_API void BakeDirectLightForLightProbes(
    LightProbeCollectionBakedData& bakedData, const LightProbeCollectionForBaking& collection,
    const RaytracerScene& raytracerScene, const BakedLight& light, const DirectLightTracingSettings& settings);

/// Accumulate indirect light for charts.
URHO3D_API void BakeIndirectLightForCharts(LightmapChartBakedIndirect& bakedIndirect,
    const ea::vector<const LightmapChartBakedDirect*>& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const TetrahedralMesh& lightProbesMesh, const LightProbeCollectionBakedData& lightProbesData,
    const RaytracerScene& raytracerScene, const ea::vector<unsigned>& geometryBufferToRaytracer,
    const IndirectLightTracingSettings& settings);

/// Accumulate indirect light for light probes.
URHO3D_API void BakeIndirectLightForLightProbes(
    LightProbeCollectionBakedData& bakedData, const LightProbeCollectionForBaking& collection,
    const ea::vector<const LightmapChartBakedDirect*>& bakedDirect,
    const RaytracerScene& raytracerScene, const IndirectLightTracingSettings& settings);

}
