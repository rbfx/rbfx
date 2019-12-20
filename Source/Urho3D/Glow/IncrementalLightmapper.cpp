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

#include "../Glow/IncrementalLightmapper.h"

#include "../Glow/LightmapCharter.h"
#include "../Glow/LightmapGeometryBaker.h"

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

/// Per-component min for 3D integer vector.
IntVector3 MinIntVector3(const IntVector3& lhs, const IntVector3& rhs) { return VectorMin(lhs, rhs); }

/// Swizzle components of 3D integer vector.
unsigned long long Swizzle(const IntVector3& vec, const IntVector3& base = IntVector3::ZERO)
{
    static const unsigned numComponents = 3;
    static const unsigned resultBits = 8 * sizeof(unsigned long long);
    static const unsigned maxBitsPerComponent = resultBits / numComponents;

    const unsigned xyz[numComponents] = {
        static_cast<unsigned>(vec.x_ - base.x_),
        static_cast<unsigned>(vec.y_ - base.y_),
        static_cast<unsigned>(vec.z_ - base.z_),
    };

    unsigned result = 0;
    for (unsigned j = 0; j < numComponents; ++j)
    {
        for (unsigned i = 0; i < maxBitsPerComponent; ++i)
        {
            const unsigned long long bit = !!(xyz[j] & (1 << i));
            result |= bit << i * numComponents + j;
        }
    }

    return result;
}

/// Context used for incremental lightmap charting.
struct LocalChunkProcessingContext
{
    /// Current chunk.
    unsigned currentChunkIndex_{};
    /// Current lightmap chart base index.
    unsigned lightmapChartBaseIndex_{};
};

/// Context used for incremental lightmap geometry baking.
struct AdjacentChartProcessingContext
{
    /// Current chunk.
    unsigned currentChunkIndex_{};
};

}

/// Incremental lightmapper implementation.
struct IncrementalLightmapperImpl
{
    /// Construct.
    IncrementalLightmapperImpl(const LightmapSettings& lightmapSettings, const IncrementalLightmapperSettings& incrementalSettings,
        Scene* scene, LightmapSceneCollector* collector, LightmapCache* cache)
        : context_(scene->GetContext())
        , lightmapSettings_(lightmapSettings)
        , incrementalSettings_(incrementalSettings)
        , scene_(scene)
        , collector_(collector)
        , cache_(cache)
    {
        // Collect chunks
        collector_->LockScene(scene_, incrementalSettings_.chunkSize_);
        chunks_ = collector_->GetChunks();

        // Sort chunks
        if (!chunks_.empty())
        {
            baseChunkIndex_ = ea::accumulate(chunks_.begin(), chunks_.end(), chunks_.front(), MinIntVector3);
            const auto compareSwizzled = [&](const IntVector3& lhs, const IntVector3& rhs)
            {
                return Swizzle(lhs, baseChunkIndex_) < Swizzle(rhs, baseChunkIndex_);
            };
            ea::sort(chunks_.begin(), chunks_.end(), compareSwizzled);
        }
    }

    /// Step chunk processing. Chunks are processed individually. Return true when completed.
    bool StepLocalChunkProcessing(LocalChunkProcessingContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
            return true;

        // Collect nodes for current chunks
        const IntVector3 chunk = chunks_[ctx.currentChunkIndex_];
        ea::vector<Node*> nodes = collector_->GetUniqueNodes(chunk);

        // Generate charts
        const LightmapChartVector charts = GenerateLightmapCharts(nodes, lightmapSettings_.charting_);

        // Apply charts to scene
        ApplyLightmapCharts(charts, ctx.lightmapChartBaseIndex_);

        // Generate scenes for geometry baking
        const ea::vector<LightmapGeometryBakingScene> geometryBakingScene =
            GenerateLightmapGeometryBakingScenes(context_, charts, lightmapSettings_.geometryBaking_);

        // Bake geometries
        LightmapChartBakedGeometryVector bakedGeometry = BakeLightmapGeometries(geometryBakingScene);

        // Store charts in the cache
        cache_->StoreBakedGeometry(chunk, ea::move(bakedGeometry));

        // Advance
        ctx.lightmapChartBaseIndex_ += charts.size();
        ++ctx.currentChunkIndex_;
        return false;
    }

    /// Step chunk processing. Chunks are processed with adjacent context. Return true when completed.
    bool StepAdjacentChunkProcessing(AdjacentChartProcessingContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
            return true;

        // Advance
        ++ctx.currentChunkIndex_;
        return false;
    }

private:
    /// Settings for lightmap generation.
    LightmapSettings lightmapSettings_;
    /// Settings for incremental lightmapper itself.
    IncrementalLightmapperSettings incrementalSettings_;

    /// Context.
    Context* context_{};
    /// Scene.
    Scene* scene_{};
    /// Scene collector.
    LightmapSceneCollector* collector_{};
    /// Lightmap cache.
    LightmapCache* cache_{};
    /// List of all chunks.
    ea::vector<IntVector3> chunks_;
    /// Base chunk index.
    IntVector3 baseChunkIndex_;


};

IncrementalLightmapper::~IncrementalLightmapper()
{
}

void IncrementalLightmapper::Initialize(const LightmapSettings& lightmapSettings,
    const IncrementalLightmapperSettings& incrementalSettings,
    Scene* scene, LightmapSceneCollector* collector, LightmapCache* cache)
{
    impl_ = ea::make_unique<IncrementalLightmapperImpl>(lightmapSettings, incrementalSettings, scene, collector, cache);
}

void IncrementalLightmapper::ProcessScene()
{
    // Generate charts
    LocalChunkProcessingContext chartingContext;
    while (!impl_->StepLocalChunkProcessing(chartingContext))
        ;

    // Generate baking geometry
    AdjacentChartProcessingContext geometryBakingContext;
    while (!impl_->StepAdjacentChunkProcessing(geometryBakingContext))
        ;
}

}
