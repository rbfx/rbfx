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

/// \file

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
