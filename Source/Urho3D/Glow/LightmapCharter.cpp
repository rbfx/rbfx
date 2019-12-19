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

#include "../Glow/LightmapCharter.h"

#include "../Core/Variant.h"
#include "../Glow/LightmapUVGenerator.h"
#include "../Graphics/Model.h"
#include "../Graphics/StaticModel.h"

namespace Urho3D
{

/// Calculate lightmap size for given model with given scale.
static IntVector2 CalculateModelLightmapSize(unsigned texelDensity, float minObjectScale,
    Model* model, const Vector3& scale)
{
    const Variant& modelLightmapSizeVar = model->GetMetadata(LightmapUVGenerationSettings::LightmapSizeKey);
    const Variant& modelLightmapDensityVar = model->GetMetadata(LightmapUVGenerationSettings::LightmapDensityKey);

    const auto modelLightmapSize = static_cast<Vector2>(modelLightmapSizeVar.GetIntVector2());
    const unsigned modelLightmapDensity = modelLightmapDensityVar.GetUInt();

    const float nodeScale = ea::max({ scale.x_, scale.y_, scale.z_ });
    const float rescaleFactor = nodeScale * static_cast<float>(texelDensity) / modelLightmapDensity;
    const float clampedRescaleFactor = ea::max(minObjectScale, rescaleFactor);

    return VectorCeilToInt(modelLightmapSize * clampedRescaleFactor);
}

/// Allocate region in the set of lightmap charts.
static LightmapChartRegion AllocateLightmapChartRegion(const LightmapChartingSettings& settings,
    ea::vector<LightmapChart>& charts, const IntVector2& size)
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
            return { chartIndex, position, size, settings.chartSize_ };
        }
        ++chartIndex;
    }

    // Create dedicated chart for this specific region if it's bigger than max chart size
    const int chartSize = static_cast<int>(settings.chartSize_);
    if (size.x_ > chartSize || size.y_ > chartSize)
    {
        LightmapChart& chart = charts.emplace_back(size.x_, size.y_);

        IntVector2 position;
        const bool success = chart.allocator_.Allocate(size.x_, size.y_, position.x_, position.y_);

        assert(success);
        assert(position == IntVector2::ZERO);

        return { chartIndex, IntVector2::ZERO, size, settings.chartSize_ };
    }

    // Create general-purpose chart
    LightmapChart& chart = charts.emplace_back(chartSize, chartSize);

    // Allocate region from the new chart
    IntVector2 paddedPosition;
    const bool success = chart.allocator_.Allocate(paddedSize.x_, paddedSize.y_, paddedPosition.x_, paddedPosition.y_);

    assert(success);
    assert(paddedPosition == IntVector2::ZERO);

    const IntVector2 position = paddedPosition + padding * IntVector2::ONE;
    return { chartIndex, position, size, settings.chartSize_ };
}

static IntVector2 CalculateStaticModelLightmapSize(StaticModel* staticModel, const LightmapChartingSettings& settings)
{
    Node* node = staticModel->GetNode();
    Model* model = staticModel->GetModel();
    return CalculateModelLightmapSize(settings.texelDensity_, settings.minObjectScale_, model, node->GetWorldScale());
}

ea::vector<LightmapChart> GenerateLightmapCharts(
    const ea::vector<Node*>& nodes, const LightmapChartingSettings& settings)
{
    ea::vector<LightmapChart> charts;
    for (Node* node : nodes)
    {
        if (auto staticModel = node->GetComponent<StaticModel>())
        {
            const IntVector2 regionSize = CalculateStaticModelLightmapSize(staticModel, settings);
            const LightmapChartRegion region = AllocateLightmapChartRegion(settings, charts, regionSize);

            const LightmapChartElement chartElement{ node, staticModel, region };
            charts[region.chartIndex_].elements_.push_back(chartElement);
        }
    }
    return charts;
}

void ApplyLightmapCharts(const LightmapChartVector& charts, unsigned baseChartIndex)
{
    for (const LightmapChart& chart : charts)
    {
        for (const LightmapChartElement& element : chart.elements_)
        {
            if (element.staticModel_)
            {
                element.staticModel_->SetLightmap(true);
                element.staticModel_->SetLightmapIndex(baseChartIndex + element.region_.chartIndex_);
                element.staticModel_->SetLightmapScaleOffset(element.region_.GetScaleOffset());
            }
        }
    }
}

}
