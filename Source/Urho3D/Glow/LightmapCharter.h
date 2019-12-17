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

#include "../Glow/LightmapSettings.h"
#include "../Math/AreaAllocator.h"
#include "../Math/Rect.h"
#include "../Math/Vector2.h"
#include "../Math/Vector4.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Node;
class StaticModel;

/// Region on the lightmap chart.
struct LightmapChartRegion
{
    /// Construct default.
    LightmapChartRegion() = default;
    /// Construct actual region.
    LightmapChartRegion(unsigned index, const IntVector2& position, const IntVector2& size, unsigned maxSize)
        : chartIndex_(index)
        , rectTexels_(position, position + size)
    {
        rectUV_.min_ = static_cast<Vector2>(rectTexels_.Min()) / static_cast<float>(maxSize);
        rectUV_.max_ = static_cast<Vector2>(rectTexels_.Max()) / static_cast<float>(maxSize);
    }
    /// Get lightmap offset vector.
    Vector4 GetScaleOffset() const
    {
        const Vector2 offset = rectUV_.Min();
        const Vector2 size = rectUV_.Size();
        return { size.x_, size.y_, offset.x_, offset.y_ };
    }

    /// Lightmap chart index.
    unsigned chartIndex_;
    /// Lightmap rectangle on the chart (in texels).
    IntRect rectTexels_;
    /// Lightmap rectangle on the chart (in normalized coordinates).
    Rect rectUV_;
};

/// Individual element of the lightmap chart.
struct LightmapChartElement
{
    /// Node.
    Node* node_{};
    /// Static model component.
    StaticModel* staticModel_{};
    /// Allocated region.
    LightmapChartRegion region_;
};

/// Lightmap chart description.
struct LightmapChart
{
    /// Width of the chart.
    unsigned width_{};
    /// Height of the chart.
    unsigned height_{};
    /// Size of the chart.
    IntVector2 size_;
    /// Used region allocator.
    AreaAllocator allocator_;
    /// Allocated elements.
    ea::vector<LightmapChartElement> elements_;

    /// Construct default.
    LightmapChart() = default;
    /// Construct valid.
    LightmapChart(int width, int height)
        : width_{ static_cast<unsigned>(width) }
        , height_{ static_cast<unsigned>(height) }
        , size_{ width, height }
        , allocator_{ width, height, 0, 0, false }
    {
    }
    /// Return size of the lightmap texel, in UV.
    Vector2 GetTexelSize() const { return { 1.0f / width_, 1.0f / height_ }; }
};

/// Vector of lightmap charts.
using LightmapChartVector = ea::vector<LightmapChart>;

/// Generate lightmap charts for given nodes.
URHO3D_API LightmapChartVector GenerateLightmapCharts(
    const ea::vector<Node*>& nodes, const LightmapChartingSettings& settings);

/// Apply lightmap charts to scene components.
URHO3D_API void ApplyLightmapCharts(const LightmapChartVector& charts, unsigned baseChartIndex = 0);

}
