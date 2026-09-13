// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Graphics/LightBakingSettings.h"
#include "Urho3D/Math/AreaAllocator.h"
#include "Urho3D/Math/Rect.h"
#include "Urho3D/Math/Vector2.h"
#include "Urho3D/Math/Vector4.h"

#include <EASTL/vector.h>

namespace Urho3D
{

class Node;
class Component;

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
        rectUV_.min_ = rectTexels_.Min().ToVector2() / static_cast<float>(maxSize);
        rectUV_.max_ = rectTexels_.Max().ToVector2() / static_cast<float>(maxSize);
    }
    /// Return lightmap scale of the region.
    Vector2 GetScale() const { return rectUV_.Size(); }
    /// Return lightmap offset of the region.
    Vector2 GetOffset() const { return rectUV_.Min(); }
    /// Return lightmap scale & offset vector.
    Vector4 GetScaleOffset() const
    {
        const Vector2 offset = GetOffset();
        const Vector2 size = GetScale();
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
    /// Component.
    Component* component_{};
    /// Object index, unique within array of charts.
    unsigned objectIndex_{};
    /// Allocated region.
    LightmapChartRegion region_;
};

/// Lightmap chart description.
struct LightmapChart
{
    /// Lightmap chart index.
    unsigned index_{};
    /// Lightmap chart size.
    unsigned lightmapSize_{};
    /// Used region allocator.
    AreaAllocator allocator_;
    /// Allocated elements.
    ea::vector<LightmapChartElement> elements_;

    /// Construct default.
    LightmapChart() = default;
    /// Construct valid.
    LightmapChart(unsigned index, unsigned size)
        : index_(index)
        , lightmapSize_{ size }
        , allocator_{ static_cast<int>(size), static_cast<int>(size), 0, 0, false }
    {
    }
};

/// Vector of lightmap charts.
using LightmapChartVector = ea::vector<LightmapChart>;

/// Generate lightmap charts for given nodes.
URHO3D_API LightmapChartVector GenerateLightmapCharts(
    const ea::vector<Component*>& geometries, const LightmapChartingSettings& settings, unsigned baseChartIndex = 0);

/// Apply lightmap charts to scene components.
URHO3D_API void ApplyLightmapCharts(const LightmapChartVector& charts);

}
