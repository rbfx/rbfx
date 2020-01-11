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

#include "../Glow/LightmapGeometryBuffer.h"
#include "../Glow/LightTracer.h"
#include "../Graphics/LightProbeGroup.h"

namespace Urho3D
{

/// Parameters for indirect light filtering.
struct IndirectFilterParameters
{
    /// Kernel radius.
    int kernelRadius_{ 2 };
    /// Upscale factor for offsets.
    int upscale_{ 1 };
    /// Color weight. The lesser value is, the more color details are preserved on flat surface.
    float luminanceSigma_{ 10.0f };
    /// Normal weight. The higher value is, the more color details are preserved on normal edges.
    float normalPower_{ 4.0f };
    /// Position weight. The lesser value is, the more color details are preserved on position edges.
    float positionSigma_{ 1.0f };
};

/// Filter indirect light.
URHO3D_API void FilterIndirectLight(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const IndirectFilterParameters& params, unsigned numTasks);


}
