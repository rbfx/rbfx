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

#include "../Glow/LightmapCharter.h"

#include "../Core/Variant.h"
#include "../IO/Log.h"
#include "../Glow/Helpers.h"
#include "../Glow/LightmapUVGenerator.h"
#include "../Graphics/Model.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Terrain.h"

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

/// Calculate lightmap size for given model with given scale.
IntVector2 CalculateModelLightmapSize(float texelDensity, float minObjectScale,
    Model* model, const Vector3& scale, float scaleInLightmap, const IntVector2& defaultChartSize)
{
    const Variant& modelLightmapSizeVar = model->GetMetadata(LightmapUVGenerationSettings::LightmapSizeKey);
    const Variant& modelLightmapDensityVar = model->GetMetadata(LightmapUVGenerationSettings::LightmapDensityKey);

    if (modelLightmapSizeVar.IsEmpty() || modelLightmapDensityVar.IsEmpty())
    {
        URHO3D_LOGWARNING("Cannot calculate chart size for model \"{}\", fallback to default.",
            model->GetName());
        return defaultChartSize;
    }

    const auto modelLightmapSize = static_cast<Vector2>(modelLightmapSizeVar.GetIntVector2());
    const float modelLightmapDensity = modelLightmapDensityVar.GetFloat();

    const float nodeScale = ea::max({ scale.x_, scale.y_, scale.z_ });
    const float rescaleFactor = nodeScale * static_cast<float>(texelDensity) / modelLightmapDensity;
    const float clampedRescaleFactor = ea::max(minObjectScale, rescaleFactor);

    return VectorCeilToInt(modelLightmapSize * clampedRescaleFactor * scaleInLightmap);
}

/// Adjust size to lightmap chart size.
IntVector2 AdjustRegionSize(const IntVector2& desiredSize, int maxSize)
{
    const int desiredDimensions = ea::max(desiredSize.x_, desiredSize.y_);
    if (desiredDimensions <= maxSize)
        return desiredSize;

    const float scale = static_cast<float>(maxSize) / desiredDimensions;
    const Vector2 newSize = static_cast<Vector2>(desiredSize) * scale;
    return VectorMax(IntVector2::ONE, VectorMin(VectorCeilToInt(newSize), IntVector2::ONE * maxSize));
};

/// Allocate region in the set of lightmap charts.
LightmapChartRegion AllocateLightmapChartRegion(const LightmapChartingSettings& settings,
    ea::vector<LightmapChart>& charts, const IntVector2& size, unsigned baseChartIndex)
{
    const int padding = static_cast<int>(settings.padding_);
    const IntVector2 paddedSize = size + 2 * padding * IntVector2::ONE;

    // Try existing charts
    unsigned chartIndex = 0;
    for (LightmapChart& lightmapDesc : charts)
    {
        IntVector2 paddedPosition;
        if (lightmapDesc.allocator_.Allocate(paddedSize.x_, paddedSize.y_, paddedPosition.x_, paddedPosition.y_))
        {
            const IntVector2 position = paddedPosition + padding * IntVector2::ONE;
            return { chartIndex, position, size, settings.lightmapSize_ };
        }
        ++chartIndex;
    }

    // Create general-purpose chart
    LightmapChart& chart = charts.emplace_back(chartIndex + baseChartIndex, settings.lightmapSize_);

    // Allocate region from the new chart
    IntVector2 paddedPosition;
    const bool success = chart.allocator_.Allocate(paddedSize.x_, paddedSize.y_, paddedPosition.x_, paddedPosition.y_);

    assert(success);
    assert(paddedPosition == IntVector2::ZERO);

    const IntVector2 position = paddedPosition + padding * IntVector2::ONE;
    return { chartIndex, position, size, settings.lightmapSize_ };
}

/// Calculate size in lightmap for StaticModel component.
IntVector2 CalculateStaticModelLightmapSize(StaticModel* staticModel, const LightmapChartingSettings& settings)
{
    Node* node = staticModel->GetNode();
    Model* model = staticModel->GetModel();
    const IntVector2 defaultChartSize = IntVector2::ONE * static_cast<int>(settings.defaultChartSize_);
    return CalculateModelLightmapSize(settings.texelDensity_, settings.minObjectScale_, model,
        node->GetWorldScale(), staticModel->GetScaleInLightmap(), defaultChartSize);
}

/// Calculate size in lightmap for Terrain component.
IntVector2 CalculateTerrainLightmapSize(Terrain* terrain, const LightmapChartingSettings& settings)
{
    Node* node = terrain->GetNode();
    const Vector3 spacing = terrain->GetSpacing();
    const Vector3 worldScale = node->GetWorldScale();
    const Vector2 size = static_cast<Vector2>(terrain->GetNumPatches()) * static_cast<float>(terrain->GetPatchSize());
    const Vector2 dimensions = size * Vector2{ worldScale.x_, worldScale.z_ } * Vector2{ spacing.x_, spacing.z_ };
    const float scaleInLightmap = terrain->GetScaleInLightmap();
    return VectorCeilToInt(dimensions * settings.texelDensity_ * scaleInLightmap);
}

/// Calculate size in lightmap for component.
IntVector2 CalculateGeometryLightmapSize(Component* component, const LightmapChartingSettings& settings)
{
    if (auto staticModel = dynamic_cast<StaticModel*>(component))
        return CalculateStaticModelLightmapSize(staticModel, settings);
    else if (auto terrain = dynamic_cast<Terrain*>(component))
        return CalculateTerrainLightmapSize(terrain, settings);
    return {};
}

/// Region to be requested for chunk.
struct RequestedChartRegion
{
    /// Index of the object.
    unsigned objectIndex_{};
    /// Adjusted region size.
    IntVector2 adjustedRegionSize_;
    /// Component to bake.
    Component* component_{};
};

}

ea::vector<LightmapChart> GenerateLightmapCharts(
    const ea::vector<Component*>& geometries, const LightmapChartingSettings& settings, unsigned baseChartIndex)
{
    // Collect object regions
    const int maxRegionSize = static_cast<int>(settings.lightmapSize_ - settings.padding_ * 2);
    ea::vector<RequestedChartRegion> requestedRegions;
    for (unsigned objectIndex = 0; objectIndex < geometries.size(); ++objectIndex)
    {
        Component* component = geometries[objectIndex];
        Node* node = component->GetNode();

        const IntVector2 regionSize = CalculateGeometryLightmapSize(component, settings);
        const IntVector2 adjustedRegionSize = AdjustRegionSize(regionSize, maxRegionSize);

        if (regionSize != adjustedRegionSize)
        {
            URHO3D_LOGWARNING("Object \"{}\" doesn't fit the lightmap chart, texel density is lowered.",
                node->GetName());
        }

        requestedRegions.emplace_back(RequestedChartRegion{ objectIndex, adjustedRegionSize, component });
    }

    // Sort regions by max dimensions
    const auto compareDimensions = [](const RequestedChartRegion& lhs, const RequestedChartRegion& rhs)
    {
        const IntVector2& lhsRegion = lhs.adjustedRegionSize_;
        const IntVector2& rhsRegion = rhs.adjustedRegionSize_;
        return ea::max(lhsRegion.x_, lhsRegion.y_) > ea::max(rhsRegion.x_, rhsRegion.y_);
    };
    ea::sort(requestedRegions.begin(), requestedRegions.end(), compareDimensions);

    // Generate charts
    ea::vector<LightmapChart> charts;
    for (const RequestedChartRegion& requestedRegion : requestedRegions)
    {
        const LightmapChartRegion region = AllocateLightmapChartRegion(
            settings, charts, requestedRegion.adjustedRegionSize_, baseChartIndex);

        const LightmapChartElement chartElement{ requestedRegion.component_, requestedRegion.objectIndex_, region };
        charts[region.chartIndex_].elements_.push_back(chartElement);
    }
    return charts;
}

void ApplyLightmapCharts(const LightmapChartVector& charts)
{
    for (const LightmapChart& chart : charts)
    {
        for (const LightmapChartElement& element : chart.elements_)
        {
            Component* component = element.component_;
            SetLightmapIndex(component, chart.index_);
            SetLightmapScaleOffset(component, element.region_.GetScaleOffset());
        }
    }
}

}
