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

#include "../Math/Vector3.h"

#include <EASTL/string.h>

namespace Urho3D
{

/// Lightmap chart allocation settings.
struct LightmapChartingSettings
{
    /// Size of lightmap chart.
    unsigned lightmapSize_{ 512 };
    /// Padding between individual objects on the chart.
    unsigned padding_{ 1 };
    /// Texel density in texels per Scene unit.
    float texelDensity_{ 10 };
    /// Minimal scale of object lightmaps.
    /// Values below 1 may cause lightmap bleeding due to insufficient padding.
    /// Values above 0 may cause inconsistent lightmap density if object scale is too small.
    float minObjectScale_{ 1.0f };
};

/// Lightmap geometry buffer baking settings.
struct LightmapGeometryBakingSettings
{
    /// Baking render path.
    ea::string renderPathName_{ "RenderPaths/LightmapGBuffer.xml" };
    /// Baking materials.
    ea::string materialName_{ "Materials/LightmapBaker.xml" };
    /// Lightmap UV channel. 2nd channel by default.
    unsigned uvChannel_{ 1 };
};

/// Lightmap tracing settings.
struct LightmapTracingSettings
{
    /// Max number of bounces.
    static const unsigned MaxBounces = 8;

    /// Number of direct samples.
    unsigned numDirectSamples_{ 1 };
    /// Number of indirect bounces.
    unsigned numBounces_{ 2 };
    /// Number of indirect samples for charts.
    unsigned numIndirectChartSamples_{ 10 };
    /// Number of indirect samples for light probes.
    unsigned numIndirectProbeSamples_{ 64 };

    /// Ray offset from position in geometry buffer or hit position.
    float rayPositionOffset_{ 0.0005f };
    /// Position offset for pushing out leaking shadows beyond backface hit by sample ray.
    float shadowLeakOffset_{ 0.001f };
    /// Normal bias for position adjusted to prevent leaking shadows.
    float shadowLeakBias_{ 0.001f };
    /// Number of asynchronous tasks to use.
    unsigned numTasks_{ M_MAX_UNSIGNED };
};

/// Lightmap filter settings.
struct LightmapFilterSettings
{
};

/// Lightmap stitching settings.
struct LightmapStitchingSettings
{
    /// Number of iterations.
    unsigned numIterations_{ 8 };
    /// Blend factor.
    float blendFactor_{ 0.5f };
    /// Render path used for stitching.
    ea::string renderPathName_{ "RenderPaths/Forward.xml" };
    /// Model used for background during stitching.
    ea::string stitchBackgroundModelName_{ "Models/Plane.mdl" };
    /// Technique used for background during stitching.
    ea::string stitchBackgroundTechniqueName_{ "Techniques/DiffUnlit.xml" };
    /// Technique used for seams rendering during stitching.
    ea::string stitchSeamsTechniqueName_{ "Techniques/DiffUnlitAlpha.xml" };
};

/// Lightmap baking settings.
struct LightmapSettings
{
    /// Charting settings.
    LightmapChartingSettings charting_;
    /// Geometry baking settings.
    LightmapGeometryBakingSettings geometryBaking_;
    /// Tracing settings.
    LightmapTracingSettings tracing_;
    /// Filtering settings.
    LightmapFilterSettings filter_;
    /// Stitching settings.
    LightmapStitchingSettings stitching_;
};

/// Incremental lightmapper settings.
struct IncrementalLightmapperSettings
{
    /// Size of the chunk.
    Vector3 chunkSize_ = Vector3::ONE * 128.0f;
    /// Additional space around chunk to collect indirect lighting.
    float indirectPadding_ = 32.0f;
    /// Shadow casting distance for directional light.
    float directionalLightShadowDistance_ = 128.0f;
    /// Output directory name.
    ea::string outputDirectory_;
    /// Lightmap name prefix.
    ea::string lightmapNamePrefix_{ "Textures/Lightmap-" };
    /// Lightmap name suffix.
    ea::string lightmapNameSuffix_{ ".png" };
};

}
