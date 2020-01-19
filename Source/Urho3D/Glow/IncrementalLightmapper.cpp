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

#include "../Core/Context.h"
#include "../Glow/BakedChunkVicinity.h"
#include "../Glow/LightmapCharter.h"
#include "../Glow/LightmapGeometryBuffer.h"
#include "../Glow/LightmapFilter.h"
#include "../Glow/LightmapStitcher.h"
#include "../Glow/LightTracer.h"
#include "../Glow/RaytracerScene.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/LightProbeGroup.h"
#include "../Graphics/Model.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Math/TetrahedralMesh.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"

#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

/// Get resource name from file name.
ea::string GetResourceName(ResourceCache* cache, const ea::string& fileName)
{
    for (unsigned i = 0; i < cache->GetNumResourceDirs(); ++i)
    {
        const ea::string& resourceDir = cache->GetResourceDir(i);
        if (fileName.starts_with(resourceDir))
            return fileName.substr(resourceDir.length());
    }
    return {};
}

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

/// Base context for incremental lightmap lightmapping.
struct BaseIncrementalContext
{
    /// Current chunk.
    unsigned currentChunkIndex_{};
};

/// Context used for incremental lightmap chunk processing (first pass).
struct ChartingContext : public BaseIncrementalContext
{
    /// Current lightmap chart base index.
    unsigned lightmapChartBaseIndex_{};
};

/// Context used for incremental lightmap chunk processing (second pass).
struct AdjacentChartProcessingContext : public BaseIncrementalContext
{
};

/// Context used for direct light baking.
struct DirectLightBakingContext : public BaseIncrementalContext
{
};

/// Context used for indirect light baking.
struct IndirectLightBakingFilterAndSaveContext : public BaseIncrementalContext
{
    /// Stitching context.
    LightmapStitchingContext stitchingContext_;
    /// Buffer for filtering direct light.
    ea::vector<Vector3> directFilterBuffer_;
    /// Buffer for filtering indirect light.
    ea::vector<Vector4> indirectFilterBuffer_;
};

/// Context used for committing chunks.
struct CommitContext : public BaseIncrementalContext
{
};

}

/// Incremental lightmapper implementation.
struct IncrementalLightmapper::Impl
{
    /// Construct.
    Impl(const LightmapSettings& lightmapSettings, const IncrementalLightmapperSettings& incrementalSettings,
        Scene* scene, BakedSceneCollector* collector, BakedLightCache* cache)
        : context_(scene->GetContext())
        , lightmapSettings_(lightmapSettings)
        , incrementalSettings_(incrementalSettings)
        , scene_(scene)
        , collector_(collector)
        , cache_(cache)
    {
    }

    /// Initialize.
    bool Initialize()
    {
        // DX9 is not supported
        if (Graphics::GetPixelUVOffset() != Vector2::ZERO)
        {
            URHO3D_LOGERROR("Cannot bake light on DX9");
            return false;
        }

        // Find or fix output directory
        if (incrementalSettings_.outputDirectory_.empty())
        {
            const ea::string& sceneFileName = scene_->GetFileName();
            if (sceneFileName.empty())
            {
                URHO3D_LOGERROR("Cannot find output directory for lightmaps: scene file name is undefined");
                return false;
            }

            incrementalSettings_.outputDirectory_ = ReplaceExtension(sceneFileName, "");
            if (incrementalSettings_.outputDirectory_ == sceneFileName)
            {
                URHO3D_LOGERROR("Cannot find output directory for lightmaps: scene file name has no extension");
                return false;
            }
        }

        incrementalSettings_.outputDirectory_ = AddTrailingSlash(incrementalSettings_.outputDirectory_);

        FileSystem* fs = context_->GetFileSystem();
        if (!fs->CreateDir(incrementalSettings_.outputDirectory_))
        {
            URHO3D_LOGERROR("Cannot create output directory for lightmaps");
            return false;
        }

        // Collect chunks
        collector_->LockScene(scene_, incrementalSettings_.chunkSize_);
        chunks_ = collector_->GetChunks();

        // Sort chunks
        if (!chunks_.empty())
        {
            const IntVector3 baseChunkIndex = ea::accumulate(chunks_.begin(), chunks_.end(), chunks_.front(), MinIntVector3);
            const auto compareSwizzled = [&](const IntVector3& lhs, const IntVector3& rhs)
            {
                return Swizzle(lhs, baseChunkIndex) < Swizzle(rhs, baseChunkIndex);
            };
            ea::sort(chunks_.begin(), chunks_.end(), compareSwizzled);
        }

        return true;
    }

    /// Step charting. Chunks are processed individually. Return true when completed.
    bool StepCharting(ChartingContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
        {
            numLightmapCharts_ = ctx.lightmapChartBaseIndex_;
            return true;
        }

        // Collect nodes for current chunks
        const IntVector3 chunk = chunks_[ctx.currentChunkIndex_];
        const ea::vector<Component*> uniqueGeometries = collector_->GetUniqueGeometries(chunk);

        // Generate charts
        const LightmapChartVector charts = GenerateLightmapCharts(
            uniqueGeometries, lightmapSettings_.charting_, ctx.lightmapChartBaseIndex_);

        // Apply charts to scene
        ApplyLightmapCharts(charts);
        collector_->CommitGeometries(chunk);

        // Advance
        ctx.lightmapChartBaseIndex_ += charts.size();
        ++ctx.currentChunkIndex_;
        return false;
    }

    /// Reference lightmaps by the scene.
    void ReferenceLightmapsByScene()
    {
        scene_->ResetLightmaps();
        for (unsigned i = 0; i < numLightmapCharts_; ++i)
        {
            const ea::string fileName = GetLightmapFileName(i);
            const ea::string resourceName = GetResourceName(context_->GetCache(), fileName);
            if (resourceName.empty())
            {
                URHO3D_LOGWARNING("Cannot find resource name for lightmap \"{}\", absolute path is used", fileName);
                scene_->AddLightmap(fileName);
            }
            else
                scene_->AddLightmap(resourceName);
        }
    }

    /// Step chunk processing. Chunks are processed with adjacent context. Return true when completed.
    bool StepAdjacentChunkProcessing(AdjacentChartProcessingContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
            return true;

        const IntVector3 chunk = chunks_[ctx.currentChunkIndex_];

        BakedChunkVicinity chunkVicinity = CreateBakedChunkVicinity(context_, *collector_, chunk,
            lightmapSettings_, incrementalSettings_);
        cache_->StoreChunkVicinity(chunk, ea::move(chunkVicinity));

        // Advance
        ++ctx.currentChunkIndex_;
        return false;
    }

    /// Step baking direct lighting.
    bool StepBakeDirect(DirectLightBakingContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
            return true;

        // Load chunk
        const IntVector3 chunk = chunks_[ctx.currentChunkIndex_];
        const ea::shared_ptr<const BakedChunkVicinity> chunkVicinity = cache_->LoadChunkVicinity(chunk);

        // Bake direct lighting
        for (unsigned i = 0; i < chunkVicinity->lightmaps_.size(); ++i)
        {
            const unsigned lightmapIndex = chunkVicinity->lightmaps_[i];
            const LightmapChartGeometryBuffer& geometryBuffer = chunkVicinity->geometryBuffers_[i];
            LightmapChartBakedDirect bakedDirect{ geometryBuffer.lightmapSize_ };

            // Bake emission
            BakeEmissionLight(bakedDirect, geometryBuffer, lightmapSettings_.emissionTracing_);

            // Bake direct lights for charts
            for (const BakedLight& bakedLight : chunkVicinity->bakedLights_)
            {
                BakeDirectLightForCharts(bakedDirect, geometryBuffer, *chunkVicinity->raytracerScene_,
                    chunkVicinity->geometryBufferToRaytracer_, bakedLight, lightmapSettings_.directChartTracing_);
            }

            // Store direct light
            cache_->StoreDirectLight(lightmapIndex, ea::move(bakedDirect));
        }

        // Advance
        ++ctx.currentChunkIndex_;
        return false;
    }

    /// Step baking indirect lighting, filter and save images.
    bool StepBakeIndirectFilterAndSave(IndirectLightBakingFilterAndSaveContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
            return true;

        // Initialize context
        if (ctx.currentChunkIndex_ == 0)
        {
            const unsigned numTexels = lightmapSettings_.charting_.lightmapSize_ * lightmapSettings_.charting_.lightmapSize_;
            ctx.directFilterBuffer_.resize(numTexels);
            ctx.indirectFilterBuffer_.resize(numTexels);
            ctx.stitchingContext_ = InitializeStitchingContext(context_, lightmapSettings_.charting_.lightmapSize_, 4);
        }

        // Load chunk
        const IntVector3 chunk = chunks_[ctx.currentChunkIndex_];
        const ea::shared_ptr<BakedChunkVicinity> chunkVicinity = cache_->LoadChunkVicinity(chunk);

        // Collect required direct lightmaps
        ea::hash_set<unsigned> requiredDirectLightmaps;
        for (const RaytracerGeometry& raytracerGeometry : chunkVicinity->raytracerScene_->GetGeometries())
        {
            if (raytracerGeometry.lightmapIndex_ != M_MAX_UNSIGNED)
                requiredDirectLightmaps.insert(raytracerGeometry.lightmapIndex_);
        }

        ea::vector<ea::shared_ptr<const LightmapChartBakedDirect>> bakedDirectLightmapsRefs(numLightmapCharts_);
        ea::vector<const LightmapChartBakedDirect*> bakedDirectLightmaps(numLightmapCharts_);
        for (unsigned lightmapIndex : requiredDirectLightmaps)
        {
            bakedDirectLightmapsRefs[lightmapIndex] = cache_->LoadDirectLight(lightmapIndex);
            bakedDirectLightmaps[lightmapIndex] = bakedDirectLightmapsRefs[lightmapIndex].get();
        }

        // Bake indirect light for light probes
        chunkVicinity->lightProbesCollection_.ResetBakedData();
        BakeIndirectLightForLightProbes(chunkVicinity->lightProbesCollection_, bakedDirectLightmaps,
            *chunkVicinity->raytracerScene_, lightmapSettings_.indirectProbesTracing_);

        // Build light probes mesh for fallback indirect
        TetrahedralMesh lightProbesMesh;
        lightProbesMesh.Define(chunkVicinity->lightProbesCollection_.worldPositions_);

        // Bake indirect lighting for charts
        for (unsigned i = 0; i < chunkVicinity->lightmaps_.size(); ++i)
        {
            const unsigned lightmapIndex = chunkVicinity->lightmaps_[i];
            const LightmapChartGeometryBuffer& geometryBuffer = chunkVicinity->geometryBuffers_[i];
            const ea::shared_ptr<LightmapChartBakedDirect> bakedDirect = cache_->LoadDirectLight(lightmapIndex);
            LightmapChartBakedIndirect bakedIndirect{ geometryBuffer.lightmapSize_ };

            // Bake indirect lights
            BakeIndirectLightForCharts(bakedIndirect, bakedDirectLightmaps,
                geometryBuffer, lightProbesMesh, chunkVicinity->lightProbesCollection_,
                *chunkVicinity->raytracerScene_, chunkVicinity->geometryBufferToRaytracer_,
                lightmapSettings_.indirectChartTracing_);

            // Filter direct and indirect
            bakedIndirect.NormalizeLight();

            FilterDirectLight(*bakedDirect, ctx.directFilterBuffer_,
                geometryBuffer, lightmapSettings_.directFilter_, lightmapSettings_.indirectChartTracing_.numTasks_);

            FilterIndirectLight(bakedIndirect, ctx.indirectFilterBuffer_,
                geometryBuffer, lightmapSettings_.indirectFilter_, lightmapSettings_.indirectChartTracing_.numTasks_);

            // Stitch seams
            if (lightmapSettings_.stitching_.numIterations_ > 0 && !geometryBuffer.seams_.empty())
            {
                SharedPtr<Model> seamsModel = CreateSeamsModel(context_, geometryBuffer.seams_);
                StitchLightmapSeams(ctx.stitchingContext_, bakedIndirect.light_,
                    lightmapSettings_.stitching_, seamsModel);
            }

            // Generate image
            auto lightmapImage = MakeShared<Image>(context_);
            lightmapImage->SetSize(geometryBuffer.lightmapSize_, geometryBuffer.lightmapSize_, 4);
            for (int y = 0; y < geometryBuffer.lightmapSize_; ++y)
            {
                for (int x = 0; x < geometryBuffer.lightmapSize_; ++x)
                {
                    const unsigned i = y * geometryBuffer.lightmapSize_ + x;
                    const Vector3 directLight = static_cast<Vector3>(bakedDirect->directLight_[i]);
                    const Vector3 indirectLight = static_cast<Vector3>(bakedIndirect.light_[i]);
                    const Vector3 totalLight = directLight + indirectLight;

                    static const float multiplier = 1.0f / 2.0f;
                    Color color = static_cast<Color>(totalLight).LinearToGamma();
                    color.r_ *= multiplier;
                    color.g_ *= multiplier;
                    color.b_ *= multiplier;
                    lightmapImage->SetPixel(x, y, color);
                }
            }

            // Save image to destination folder
            const ea::string fileName = GetLightmapFileName(lightmapIndex);
            context_->GetFileSystem()->CreateDirsRecursive(GetPath(fileName));
            lightmapImage->SaveFile(fileName);
        }

        // Bake direct lights for light probes
        for (const BakedLight& bakedLight : chunkVicinity->bakedLights_)
        {
            BakeDirectLightForLightProbes(chunkVicinity->lightProbesCollection_, *chunkVicinity->raytracerScene_,
                bakedLight, lightmapSettings_.directProbesTracing_);
        }

        // Release cache
        cache_->CommitLightProbeGroups(chunk);

        // Advance
        ++ctx.currentChunkIndex_;
        return false;
    }

    // Step committing to scene.
    bool StepCommit(CommitContext& ctx)
    {
        if (ctx.currentChunkIndex_ >= chunks_.size())
            return true;

        // Load chunk
        const IntVector3 chunk = chunks_[ctx.currentChunkIndex_];
        const ea::vector<LightProbeGroup*> lightProbeGroups = collector_->GetUniqueLightProbeGroups(chunk);
        const ea::shared_ptr<const BakedChunkVicinity> chunkVicinity = cache_->LoadChunkVicinity(chunk);

        for (unsigned i = 0; i < lightProbeGroups.size(); ++i)
            lightProbeGroups[i]->CommitLightProbes(chunkVicinity->lightProbesCollection_, i);

        // Advance
        ++ctx.currentChunkIndex_;
        return false;
    }

private:
    /// Return lightmap file name.
    ea::string GetLightmapFileName(unsigned lightmapIndex)
    {
        ea::string lightmapName;
        lightmapName += incrementalSettings_.outputDirectory_;
        lightmapName += incrementalSettings_.lightmapNamePrefix_;
        lightmapName += ea::to_string(lightmapIndex);
        lightmapName += incrementalSettings_.lightmapNameSuffix_;
        return lightmapName;
    }

    /// Settings for lightmap generation.
    LightmapSettings lightmapSettings_;
    /// Settings for incremental lightmapper itself.
    IncrementalLightmapperSettings incrementalSettings_;

    /// Context.
    Context* context_{};
    /// Scene.
    Scene* scene_{};
    /// Scene collector.
    BakedSceneCollector* collector_{};
    /// Lightmap cache.
    BakedLightCache* cache_{};
    /// List of all chunks.
    ea::vector<IntVector3> chunks_;
    /// Number of lightmap charts.
    unsigned numLightmapCharts_{};
};

IncrementalLightmapper::~IncrementalLightmapper()
{
}

bool IncrementalLightmapper::Initialize(const LightmapSettings& lightmapSettings,
    const IncrementalLightmapperSettings& incrementalSettings,
    Scene* scene, BakedSceneCollector* collector, BakedLightCache* cache)
{
    impl_ = ea::make_unique<Impl>(lightmapSettings, incrementalSettings, scene, collector, cache);
    return impl_->Initialize();
}

void IncrementalLightmapper::ProcessScene()
{
    // Generate charts
    ChartingContext chartingContext;
    while (!impl_->StepCharting(chartingContext))
        ;

    // Reference generated charts by the scene
    impl_->ReferenceLightmapsByScene();

    // Generate baking geometry
    AdjacentChartProcessingContext geometryBakingContext;
    while (!impl_->StepAdjacentChunkProcessing(geometryBakingContext))
        ;
}

void IncrementalLightmapper::Bake()
{
    // Bake direct lighting
    DirectLightBakingContext directContext;
    while (!impl_->StepBakeDirect(directContext))
        ;

    // Bake indirect lighting, filter and save images
    IndirectLightBakingFilterAndSaveContext indirectContext;
    while (!impl_->StepBakeIndirectFilterAndSave(indirectContext))
        ;
}

void IncrementalLightmapper::CommitScene()
{
    CommitContext commitContext;
    while (!impl_->StepCommit(commitContext))
        ;
}

}
