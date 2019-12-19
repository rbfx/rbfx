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

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>

namespace Urho3D
{

/// Per-component min for 3D integer vector.
static IntVector3 MinIntVector3(const IntVector3& lhs, const IntVector3& rhs) { return VectorMin(lhs, rhs); }

/// Swizzle components of 3D integer vector.
static unsigned long long Swizzle(const IntVector3& vec, const IntVector3& base = IntVector3::ZERO)
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

/// Incremental lightmapper implementation.
struct IncrementalLightmapperImpl
{
    /// Construct.
    IncrementalLightmapperImpl(const LightmapSettings& lightmapSettings, const IncrementalLightmapperSettings& incrementalSettings,
        Scene* scene, LightmapSceneCollector* collector, LightmapCache* cache)
        : lightmapSettings_(lightmapSettings)
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

        // Initialize iterators.
        currentChunk_ = chunks_.begin();
    }
    /// Step lightmap charting. Return true when there is no more to process.
    bool StepCharting()
    {
        if (currentChunk_ == chunks_.end())
            return true;

        // Find 2x2x2 block of chunks. Due to sorting above, said block is always contiguous.
        static const unsigned blockSize = 2;
        const LightmapChartGroupID chunkGroupId = GetChunkGroupID(*currentChunk_);
        const auto chunkInSameGroup = [&](const IntVector3& chunk)
        {
            return chunkGroupId == GetChunkGroupID(chunk);
        };
        const auto lastChunk = ea::find_if_not(currentChunk_, chunks_.end(), chunkInSameGroup);

        // Collect nodes for all chunks
        ea::vector<Node*> nodes;
        ea::for_each(currentChunk_, lastChunk,
            [&](const IntVector3& chunk)
        {
            nodes.push_back(collector_->GetUniqueNodes(chunk));
        });

        // Generate charts
        LightmapChartVector charts = GenerateLightmapCharts(nodes, lightmapSettings_.charting_);

        // Apply charts to scene
        ApplyLightmapCharts(charts, lightmapChartBaseIndex_);

        // Cache charts
        cache_->StoreCharts(chunkGroupId, ea::move(charts));

        // Advance
        lightmapChartBaseIndex_ += charts.size();
        currentChunk_ = lastChunk;

        return false;
    }

private:
    /// Return chunk group ID for given chunk.
    LightmapChartGroupID GetChunkGroupID(const IntVector3& chunk)
    {
        return Swizzle(chunk, baseChunkIndex_) & 0xffff'ffff'ffff'fff8;
    }

    /// Settings for lightmap generation.
    LightmapSettings lightmapSettings_;
    /// Settings for incremental lightmapper itself.
    IncrementalLightmapperSettings incrementalSettings_;

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

    /// Start chunk.
    ea::vector<IntVector3>::iterator currentChunk_;
    /// Current lightmap chart base index.
    unsigned lightmapChartBaseIndex_{};
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
    while (!impl_->StepCharting())
        ;
}

}
