//
// Copyright (c) 2008-2019 the Urho3D project.
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

/// \file

#pragma once

#include "../Glow/LightmapCharter.h"
#include "../Glow/LightmapGeometryBaker.h"

namespace Urho3D
{

class EmbreeScene;

/// Lighting accumulated for given lightmap chart.
struct LightmapChartBakedLighting
{
    /// Construct default.
    LightmapChartBakedLighting() = default;
    /// Construct valid.
    LightmapChartBakedLighting(unsigned width, unsigned height)
        : width_(width)
        , height_(height)
        , directLighting_(width_ * height_)
        , indirectLighting_(width_ * height_)
    {
    }

    /// Width of the chart.
    unsigned width_{};
    /// Height of the chart.
    unsigned height_{};
    /// Direct lighting.
    ea::vector<Vector3> directLighting_;
    /// Indirect lighting. W component represents normalization weight.
    ea::vector<Vector4> indirectLighting_;
};

/// Initialize baked lighting for lightmap charts.
URHO3D_API ea::vector<LightmapChartBakedLighting> InitializeLightmapChartsBakedLighting(const LightmapChartVector& charts);

/// Directional light parameters.
struct DirectionalLightParameters
{
    /// Direction of the light.
    Vector3 direction_;
    /// Color of the light.
    Color color_;
};

/// Accumulate direct lighting from directional light.
URHO3D_API void BakeDirectionalLight(LightmapChartBakedLighting& bakedLighting, const LightmapChartBakedGeometry& bakedGeometry,
    const EmbreeScene& embreeScene, const DirectionalLightParameters& light, const LightmapTracingSettings& settings);

}
