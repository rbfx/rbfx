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

#include "../Precompiled.h"

#include "../Glow/IncrementalLightBaker.h"

#include "../Core/Context.h"
#include "../Glow/BakedSceneChunk.h"
#include "../Glow/LightmapCharter.h"
#include "../Glow/LightmapGeometryBuffer.h"
#include "../Glow/LightmapFilter.h"
#include "../Glow/LightmapStitcher.h"
#include "../Glow/LightTracer.h"
#include "../Glow/RaytracerScene.h"
#include "../Graphics/GlobalIllumination.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/LightProbeGroup.h"
#include "../Graphics/Model.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Math/TetrahedralMesh.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"
#include "../IO/VirtualFileSystem.h"

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

}

/// Incremental light baker implementation.
struct IncrementalLightBaker::Impl
{
    /// Construct.
    Impl(const LightBakingSettings& settings, Scene* scene, BakedSceneCollector* collector, BakedLightCache* cache)
        : context_(scene->GetContext())
        , settings_(settings)
        , scene_(scene)
        , collector_(collector)
        , cache_(cache)
    {
    }

    /// Initialize.
    bool Initialize()
    {
        // Find or fix output directory
        outputDirectory_ = GetOutputDirectory();
        if (!outputDirectory_)
            return false;

        // Collect chunks
        collector_->LockScene(scene_, settings_.incremental_.chunkSize_);
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

        // Initialize GI data file
        auto gi = scene_->GetComponent<GlobalIllumination>();
        const FileIdentifier giFileName = outputDirectory_ + settings_.incremental_.giDataFileName_;

        BinaryFile file(context_);
        if (!file.SaveFile(giFileName))
        {
            URHO3D_LOGERROR("Cannot allocate GI data file at '{}'", giFileName.ToUri());
            return false;
        }

        auto vfs = context_->GetSubsystem<VirtualFileSystem>();
        const FileIdentifier giResourceName = vfs->GetCanonicalIdentifier(giFileName);
        gi->SetFileRef({BinaryFile::GetTypeStatic(), giResourceName.ToUri()});

        return true;
    }

    /// Generate charts and allocate light probes.
    void GenerateChartsAndUpdateScene()
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        auto vfs = context_->GetSubsystem<VirtualFileSystem>();
        auto fileSystem = context_->GetSubsystem<FileSystem>();

        numLightmapCharts_ = 0;

        for (const IntVector3& chunk : chunks_)
        {
            // Collect nodes for current chunks
            const ea::vector<Component*> uniqueGeometries = collector_->GetUniqueGeometries(chunk);
            const ea::vector<LightProbeGroup*> uniqueLightProbes = collector_->GetUniqueLightProbeGroups(chunk);

            // Generate charts
            const LightmapChartVector charts = GenerateLightmapCharts(
                uniqueGeometries, settings_.charting_, numLightmapCharts_);

            // Apply charts to scene
            ApplyLightmapCharts(charts);
            collector_->CommitGeometries(chunk);

            // Assign baked data files for light probe groups
            auto fileSystem = context_->GetSubsystem<FileSystem>();
            for (unsigned i = 0; i < uniqueLightProbes.size(); ++i)
            {
                LightProbeGroup* group = uniqueLightProbes[i];
                const FileIdentifier fileName = GetLightProbeBakedDataFileName(chunk, i);
                const FileIdentifier resourceName = vfs->GetCanonicalIdentifier(fileName);
                group->SetBakedDataFileRef({BinaryFile::GetTypeStatic(), resourceName.ToUri()});
            }

            // Update base index
            numLightmapCharts_ += charts.size();
        }

        // Update scene
        scene_->ResetLightmaps();
        for (unsigned i = 0; i < numLightmapCharts_; ++i)
        {
            const FileIdentifier fileName = GetLightmapFileName(i);
            const FileIdentifier resourceName = vfs->GetCanonicalIdentifier(fileName);

            if (!vfs->Exists(fileName))
            {
                Image placeholderImage(context_);
                placeholderImage.SetSize(1, 1, 4);
                placeholderImage.SetPixel(0, 0, Color::BLACK);
                placeholderImage.SaveFile(fileName);
            }

            // TODO(vfs): Revisit this place, we should not need condition here
            if (resourceName.scheme_ == "file")
                URHO3D_LOGWARNING("Absolute path is used for lightmap '{}'", fileName.ToUri());

            scene_->AddLightmap(resourceName.ToUri());
        }
    }

    /// Generate baking chunks.
    void GenerateBakingChunks()
    {
        numLightmapsTotal_ = 0;
        for (const IntVector3& chunk : chunks_)
        {
            BakedSceneChunk bakedChunk = CreateBakedSceneChunk(context_, *collector_, chunk, settings_);
            numLightmapsTotal_ += bakedChunk.lightmaps_.size();
            cache_->StoreBakedChunk(chunk, ea::move(bakedChunk));
        }
    }

    /// Step direct light for charts.
    bool BakeDirectCharts(StopToken stopToken)
    {
        status_.phase_.store(IncrementalLightBakerPhase::BakingDirectLighting, std::memory_order_relaxed);
        status_.processedElements_.store(0, std::memory_order_relaxed);
        status_.totalElements_.store(numLightmapsTotal_, std::memory_order_relaxed);

        for (const IntVector3 chunk : chunks_)
        {
            const ea::shared_ptr<const BakedSceneChunk> bakedChunk = cache_->LoadBakedChunk(chunk);

            // Bake direct lighting
            for (unsigned i = 0; i < bakedChunk->lightmaps_.size(); ++i)
            {
                if (stopToken.IsStopped())
                    return false;

                const unsigned lightmapIndex = bakedChunk->lightmaps_[i];
                const LightmapChartGeometryBuffer& geometryBuffer = bakedChunk->geometryBuffers_[i];
                LightmapChartBakedDirect bakedDirect{ geometryBuffer.lightmapSize_ };

                // Bake emission
                BakeEmissionLight(bakedDirect, geometryBuffer,
                    settings_.emissionTracing_, settings_.properties_.emissionBrightness_);

                // Bake direct lights for charts
                for (const BakedLight& bakedLight : bakedChunk->bakedLights_)
                {
                    BakeDirectLightForCharts(bakedDirect, geometryBuffer, *bakedChunk->raytracerScene_,
                        bakedChunk->geometryBufferToRaytracer_, bakedLight, settings_.directChartTracing_);
                }

                // Store direct light
                cache_->StoreDirectLight(lightmapIndex, ea::move(bakedDirect));

                status_.processedElements_.fetch_add(1u, std::memory_order_relaxed);
            }
        }

        return true;
    }

    /// Bake indirect light, filter baked direct and indirect, bake direct light probes.
    bool BakeIndirectAndFilter(StopToken stopToken)
    {
        status_.phase_.store(IncrementalLightBakerPhase::BakingIndirectLighting, std::memory_order_relaxed);
        status_.processedElements_.store(0, std::memory_order_relaxed);
        status_.totalElements_.store(numLightmapsTotal_, std::memory_order_relaxed);

        const unsigned numTexels = settings_.charting_.lightmapSize_ * settings_.charting_.lightmapSize_;
        ea::vector<Vector3> directFilterBuffer(numTexels);
        ea::vector<Vector4> indirectFilterBuffer(numTexels);
        LightProbeCollectionBakedData lightProbesBakedData;
        LightmapChartBakedIndirect bakedIndirect{ settings_.charting_.lightmapSize_ };

        for (const IntVector3 chunk : chunks_)
        {
            if (stopToken.IsStopped())
                return false;

            const ea::shared_ptr<const BakedSceneChunk> bakedChunk = cache_->LoadBakedChunk(chunk);

            // Collect required direct lightmaps
            ea::vector<ea::shared_ptr<const LightmapChartBakedDirect>> bakedDirectLightmapsRefs(numLightmapCharts_);
            ea::vector<const LightmapChartBakedDirect*> bakedDirectLightmaps(numLightmapCharts_);
            for (unsigned lightmapIndex : bakedChunk->requiredDirectLightmaps_)
            {
                bakedDirectLightmapsRefs[lightmapIndex] = cache_->LoadDirectLight(lightmapIndex);
                bakedDirectLightmaps[lightmapIndex] = bakedDirectLightmapsRefs[lightmapIndex].get();
            }

            // Allocate storage for light probes
            lightProbesBakedData.Resize(bakedChunk->lightProbesCollection_.GetNumProbes());

            // Bake indirect light for light probes
            BakeIndirectLightForLightProbes(lightProbesBakedData, bakedChunk->lightProbesCollection_,
                bakedDirectLightmaps, *bakedChunk->raytracerScene_, settings_.indirectProbesTracing_);

            // Build light probes mesh for fallback indirect
            TetrahedralMesh lightProbesMesh;
            lightProbesMesh.Define(bakedChunk->lightProbesCollection_.worldPositions_);

            // Bake indirect lighting for charts
            for (unsigned i = 0; i < bakedChunk->lightmaps_.size(); ++i)
            {
                if (stopToken.IsStopped())
                    return false;

                const unsigned lightmapIndex = bakedChunk->lightmaps_[i];
                const LightmapChartGeometryBuffer& geometryBuffer = bakedChunk->geometryBuffers_[i];
                const ea::shared_ptr<const LightmapChartBakedDirect> bakedDirect = cache_->LoadDirectLight(lightmapIndex);

                ea::fill(bakedIndirect.light_.begin(), bakedIndirect.light_.end(), Vector4::ZERO);

                // Bake indirect lights
                BakeIndirectLightForCharts(bakedIndirect, bakedDirectLightmaps,
                    geometryBuffer, lightProbesMesh, lightProbesBakedData,
                    *bakedChunk->raytracerScene_, bakedChunk->geometryBufferToRaytracer_,
                    settings_.indirectChartTracing_);

                // Filter direct and indirect
                bakedIndirect.NormalizeLight();

                if (settings_.directFilter_.kernelRadius_ > 0)
                {
                    FilterDirectLight(*bakedDirect, directFilterBuffer,
                        geometryBuffer, settings_.directFilter_, settings_.directChartTracing_.numTasks_);
                }

                if (settings_.indirectFilter_.kernelRadius_ > 0)
                {
                    FilterIndirectLight(bakedIndirect, indirectFilterBuffer,
                        geometryBuffer, settings_.indirectFilter_, settings_.indirectChartTracing_.numTasks_);
                }

                // Generate final images
                BakedLightmap bakedLightmap(settings_.charting_.lightmapSize_);
                for (unsigned i = 0; i < bakedLightmap.lightmap_.size(); ++i)
                {
                    const Vector3 directLight = static_cast<Vector3>(directFilterBuffer[i]);
                    const Vector3 indirectLight = indirectFilterBuffer[i].ToVector3();
                    bakedLightmap.lightmap_[i] = VectorMax(Vector3::ZERO, directLight);
                    bakedLightmap.lightmap_[i] += VectorMax(Vector3::ZERO, indirectLight);
                }

                // Store lightmap
                cache_->StoreLightmap(lightmapIndex, ea::move(bakedLightmap));

                status_.processedElements_.fetch_add(1u, std::memory_order_relaxed);
            }

            // Bake direct lights for light probes
            for (const BakedLight& bakedLight : bakedChunk->bakedLights_)
            {
                BakeDirectLightForLightProbes(lightProbesBakedData,
                    bakedChunk->lightProbesCollection_, *bakedChunk->raytracerScene_,
                    bakedLight, settings_.directProbesTracing_);
            }

            // Save light probes
            for (unsigned groupIndex = 0; groupIndex < bakedChunk->numUniqueLightProbes_; ++groupIndex)
            {
                const FileIdentifier fileName = GetLightProbeBakedDataFileName(chunk, groupIndex);
                if (!LightProbeGroup::SaveLightProbesBakedData(context_, fileName,
                    bakedChunk->lightProbesCollection_, lightProbesBakedData, groupIndex))
                {
                    const ea::string groupName = groupIndex < bakedChunk->lightProbesCollection_.GetNumGroups()
                        ? bakedChunk->lightProbesCollection_.names_[groupIndex] : "";
                    URHO3D_LOGERROR("Cannot save light probes for group '{}' in chunk {}",
                        groupName, chunk.ToString());
                }
            }
        }

        status_.phase_.store(IncrementalLightBakerPhase::Finalizing, std::memory_order_relaxed);
        return true;
    }

    // Stitch and save lightmaps.
    void StitchAndSaveImages()
    {
        const unsigned numTexels = settings_.charting_.lightmapSize_ * settings_.charting_.lightmapSize_;

        // Allocate stitching context
        LightmapStitchingContext stitchingContext = InitializeStitchingContext(
            context_, settings_.charting_.lightmapSize_, 4);

        // Allocate image to save
        ea::vector<Vector4> buffer(numTexels);

        auto lightmapImage = MakeShared<Image>(context_);
        if (!lightmapImage->SetSize(settings_.charting_.lightmapSize_, settings_.charting_.lightmapSize_, 4))
        {
            URHO3D_LOGERROR("Cannot allocate image for lightmap");
            return;
        }

        // Process all chunks
        for (const IntVector3 chunk : chunks_)
        {
            const ea::shared_ptr<const BakedSceneChunk> bakedChunk = cache_->LoadBakedChunk(chunk);
            for (unsigned i = 0; i < bakedChunk->lightmaps_.size(); ++i)
            {
                const unsigned lightmapIndex = bakedChunk->lightmaps_[i];
                const ea::shared_ptr<const BakedLightmap> bakedLightmap = cache_->LoadLightmap(lightmapIndex);
                const LightmapChartGeometryBuffer& geometryBuffer = bakedChunk->geometryBuffers_[i];

                // Stitch seams or just copy data to buffer
                if (settings_.stitching_.numIterations_ > 0 && !geometryBuffer.seams_.empty())
                {
                    SharedPtr<Model> seamsModel = CreateSeamsModel(context_, geometryBuffer.seams_);
                    StitchLightmapSeams(stitchingContext, bakedLightmap->lightmap_, buffer,
                        settings_.stitching_, seamsModel);
                }
                else
                {
                    for (unsigned i = 0; i < numTexels; ++i)
                        buffer[i] = Vector4(bakedLightmap->lightmap_[i], 1.0f);
                }

                // Generate image
                for (unsigned i = 0; i < numTexels; ++i)
                {
                    const unsigned x = i % geometryBuffer.lightmapSize_;
                    const unsigned y = i / geometryBuffer.lightmapSize_;

                    static const float multiplier = 1.0f / 2.0f;
                    Color color = static_cast<Color>(buffer[i].ToVector3()).LinearToGamma();
                    color.r_ *= multiplier;
                    color.g_ *= multiplier;
                    color.b_ *= multiplier;
                    lightmapImage->SetPixel(x, y, color);
                }

                // Save image to destination folder
                const FileIdentifier fileName = GetLightmapFileName(lightmapIndex);
                lightmapImage->SaveFile(fileName);
            }
        }
    }

    const IncrementalLightBakerStatus& GetStatus() const { return status_; }

private:
    FileIdentifier GetOutputDirectory() const
    {
        if (!settings_.incremental_.outputDirectory_.empty())
            return FileIdentifier::FromUri(settings_.incremental_.outputDirectory_);

        const ea::string& sceneFileName = scene_->GetFileName();
        if (sceneFileName.empty())
        {
            URHO3D_LOGERROR("Cannot find output directory for lightmaps: scene file name is undefined");
            return FileIdentifier::Empty;
        }

        return FileIdentifier::FromUri(sceneFileName + ".d");
    }

    /// Return lightmap file name.
    FileIdentifier GetLightmapFileName(unsigned lightmapIndex)
    {
        const ea::string localName = Format(settings_.incremental_.lightmapNameFormat_, lightmapIndex);
        return outputDirectory_ + localName;
    }

    /// Return light probe group baked data file.
    FileIdentifier GetLightProbeBakedDataFileName(const IntVector3& chunk, unsigned index)
    {
        const ea::string localName =
            Format(settings_.incremental_.lightProbeGroupNameFormat_, chunk.x_, chunk.y_, chunk.z_, index);
        return outputDirectory_ + localName;
    }

    /// Settings for light baking.
    LightBakingSettings settings_;
    /// Output directory.
    FileIdentifier outputDirectory_;

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

    IncrementalLightBakerStatus status_;
    unsigned numLightmapsTotal_{};
};

ea::string IncrementalLightBakerStatus::ToString() const
{
    const unsigned current = processedElements_.load(std::memory_order_relaxed);
    const unsigned total = totalElements_.load(std::memory_order_relaxed);

    switch (phase_.load(std::memory_order_relaxed))
    {
    case IncrementalLightBakerPhase::Finalizing:
        return "Finalizing...";
    case IncrementalLightBakerPhase::BakingDirectLighting:
        return Format("Baking direct lighting: {}/{} lightmaps...", current, total);
    case IncrementalLightBakerPhase::BakingIndirectLighting:
        return Format("Baking indirect lighting: {}/{} lightmaps...", current, total);
    case IncrementalLightBakerPhase::NotStarted:
    default:
        return "Not started.";
    }
}

IncrementalLightBaker::IncrementalLightBaker()
{
}

IncrementalLightBaker::~IncrementalLightBaker()
{
}

bool IncrementalLightBaker::Initialize(const LightBakingSettings& settings,
    Scene* scene, BakedSceneCollector* collector, BakedLightCache* cache)
{
    impl_ = ea::make_unique<Impl>(settings, scene, collector, cache);
    return impl_->Initialize();
}

void IncrementalLightBaker::ProcessScene()
{
    impl_->GenerateChartsAndUpdateScene();
    impl_->GenerateBakingChunks();
}

bool IncrementalLightBaker::Bake(StopToken stopToken)
{
    if (!impl_->BakeDirectCharts(stopToken))
        return false;

    if (!impl_->BakeIndirectAndFilter(stopToken))
        return false;

    return true;
}

void IncrementalLightBaker::CommitScene()
{
    impl_->StitchAndSaveImages();
}

const IncrementalLightBakerStatus& IncrementalLightBaker::GetStatus() const
{
    return impl_->GetStatus();
}

}
