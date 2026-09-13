// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Glow/LightmapGeometryBuffer.h"
#include "../Glow/LightTracer.h"
#include "../Graphics/LightBakingSettings.h"
#include "../Graphics/LightProbeGroup.h"

namespace Urho3D
{

/// Filter direct light.
URHO3D_API void FilterDirectLight(const LightmapChartBakedDirect& bakedDirect, ea::vector<Vector3>& outputBuffer,
    const LightmapChartGeometryBuffer& geometryBuffer, const EdgeStoppingGaussFilterParameters& params, unsigned numTasks);

/// Filter indirect light.
URHO3D_API void FilterIndirectLight(const LightmapChartBakedIndirect& bakedIndirect, ea::vector<Vector4>& outputBuffer,
    const LightmapChartGeometryBuffer& geometryBuffer, const EdgeStoppingGaussFilterParameters& params, unsigned numTasks);

}
