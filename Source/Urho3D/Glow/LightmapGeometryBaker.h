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
#include "../Graphics/RenderPath.h"
#include "../Scene/Scene.h"

namespace Urho3D
{

class Camera;
class Context;

/// Baking scene for single lightmap chart.
struct LightmapGeometryBakingScene
{
    /// Context.
    Context* context_{};
    /// Width of the chart.
    unsigned width_{};
    /// Height of the chart.
    unsigned height_{};
    /// Size of the chart.
    IntVector2 size_;
    /// Baking scene.
    SharedPtr<Scene> scene_;
    /// Baking camera.
    Camera* camera_{};
    /// Baking render path.
    SharedPtr<RenderPath> renderPath_;
};

/// Generate lightmap geometry baking scene for lightmap chart.
URHO3D_API LightmapGeometryBakingScene GenerateLightmapGeometryBakingScene(Context* context,
    const LightmapChart& chart, const LightmapGeometryBakingSettings& settings, SharedPtr<RenderPath> bakeRenderPath);

/// Generate baking scenes for lightmap charts.
URHO3D_API ea::vector<LightmapGeometryBakingScene> GenerateLightmapGeometryBakingScenes(Context* context,
    const ea::vector<LightmapChart>& charts, const LightmapGeometryBakingSettings& settings);

/// Baked lightmap geometry of lightmap chart.
struct LightmapChartBakedGeometry
{
    /// Width of the chart.
    unsigned width_{};
    /// Height of the chart.
    unsigned height_{};

    /// Positions as is.
    ea::vector<Vector3> geometryPositions_;
    /// Smooth positions after Phong tesselation.
    // TODO: Implement
    ea::vector<Vector3> smoothPositions_;
    /// Smooth normals used in rendering.
    ea::vector<Vector3> smoothNormals_;
    /// Raw face normals.
    // TODO: Implement
    ea::vector<Vector3> faceNormals_;
    /// Geometry IDs.
    ea::vector<unsigned> geometryIds_;

    /// Construct default.
    LightmapChartBakedGeometry() = default;
    /// Construct valid.
    LightmapChartBakedGeometry(unsigned width, unsigned height)
        : width_(width)
        , height_(height)
        , geometryPositions_(width * height)
        , smoothPositions_(width * height)
        , smoothNormals_(width * height)
        , faceNormals_(width * height)
        , geometryIds_(width * height)
    {
    }
    /// Convert index to location.
    IntVector2 IndexToLocation(unsigned index) const
    {
        return { static_cast<int>(index % width_), static_cast<int>(index / width_) };
    }
    /// Returns whether the location is valid.
    bool IsValidLocation(const IntVector2& location) const
    {
        return 0 <= location.x_ && location.x_ < static_cast<int>(width_)
            && 0 <= location.y_ && location.y_ < static_cast<int>(height_);
    }
    /// Convert location to index.
    unsigned LocationToIndex(const IntVector2& location) const
    {
        return location.x_ + width_ * location.y_;
    }
};

/// Bake lightmap geometry for lightmap chart.
URHO3D_API LightmapChartBakedGeometry BakeLightmapGeometry(const LightmapGeometryBakingScene& bakingScene);

/// Bake lightmap geometry for lightmap charts.
URHO3D_API ea::vector<LightmapChartBakedGeometry> BakeLightmapGeometries(const ea::vector<LightmapGeometryBakingScene>& bakingScenes);

}
