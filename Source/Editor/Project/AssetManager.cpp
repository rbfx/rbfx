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

#include "../Project/AssetManager.h"
#include "../Project/Project.h"

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/JSONFile.h>

#include <EASTL/sort.h>

namespace Urho3D
{

namespace
{

ea::vector<ea::string> CollectDirectories(const ea::vector<ea::string>& fileNames)
{
    ea::vector<ea::string> result;
    for (const ea::string& fileName : fileNames)
        result.push_back(GetPath(fileName));

    ea::sort(result.begin(), result.end());
    result.erase(ea::unique(result.begin(), result.end()), result.end());

    return result;
}

void RemoveRedundantPrefixesSorted(ea::vector<ea::string>& prefixes)
{
    if (prefixes.empty())
        return;

    if (prefixes.front().empty())
    {
        prefixes.erase(prefixes.begin() + 1, prefixes.end());
        return;
    }

    for (unsigned i = 1; i < prefixes.size(); ++i)
    {
        const auto isPrefixed = [&](const ea::string& str) { return !str.empty() && prefixes[i].starts_with(str); };
        if (ea::any_of(prefixes.begin(), prefixes.begin() + i, isPrefixed))
        {
            prefixes[i].clear();
            break;
        }
    }
    ea::erase_if(prefixes, [](const ea::string& str) { return str.empty(); });
}

ea::vector<ea::string> EnumerateDisjoinedDirectories(const ea::vector<ea::string>& fileNames)
{
    auto result = CollectDirectories(fileNames);
    RemoveRedundantPrefixesSorted(result);
    return result;
}

} // namespace

void AssetManager::AssetDesc::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "outputs", outputs_);
    SerializeOptionalValue(archive, "transformers", transformers_);
    SerializeOptionalValue(archive, "modificationTime", modificationTime_);
    SerializeOptionalValue(archive, "dependencyModificationTimes", dependencyModificationTimes_);
}

bool AssetManager::AssetDesc::IsAnyTransformerUsed(const StringVector& transformers) const
{
    for (const ea::string& transformer : transformers)
    {
        if (transformers_.contains(transformer))
            return true;
    }
    return false;
}

ea::string AssetManager::AssetDesc::GetTransformerDebugString() const
{
    StringVector transformers{transformers_.begin(), transformers_.end()};
    ea::sort(transformers.begin(), transformers.end());
    return ea::string::joined(transformers, ", ");
}

AssetManager::AssetManager(Context* context)
    : Object(context)
    , project_(GetSubsystem<Project>())
    , dataWatcher_(MakeShared<FileWatcher>(context))
{
    for (const bool isPostTransform : {false, true})
        transformerHierarchy_[isPostTransform] = MakeShared<AssetTransformerHierarchy>(context_, isPostTransform);

    dataWatcher_->StartWatching(project_->GetDataPath(), true);
    context_->OnReflectionRemoved.Subscribe(this, &AssetManager::OnReflectionRemoved);
    SetProcessCallback(nullptr);
}

AssetManager::~AssetManager()
{
}

void AssetManager::SetProcessCallback(const OnProcessAssetQueued& callback, unsigned maxConcurrency)
{
    const auto defaultCallback = //
        [this](const AssetTransformerInputVector& input, const OnProcessAssetCompleted& callback)
    {
        ProcessAssetBatch(input, callback);
        //
    };

    processCallback_ = callback ? callback : defaultCallback;
    maxConcurrentRequests_ = ea::max(maxConcurrency, 1u);
}

void AssetManager::Initialize(bool readOnly)
{
    autoProcessAssets_ = !readOnly;

    InitializeAssetPipelines();
    InvalidateOutdatedAssetsInPath("");

    if (autoProcessAssets_)
    {
        EnsureAssetsAndCacheValid();
        ScanAndQueueAssetProcessing();
    }

    if (requestQueue_.empty() && numOngoingRequests_ == 0)
    {
        initialized_ = true;
        OnInitialized(this);
    }
}

void AssetManager::Update()
{
    auto cache = GetSubsystem<ResourceCache>();
    cache->SetResourceReloadSuspended(IsProcessing());

    if (IsProcessing())
    {
        if (!requestQueue_.empty() && numOngoingRequests_ < maxConcurrentRequests_)
            ConsumeAssetQueue();
        return;
    }

    if (!initialized_)
    {
        initialized_ = true;
        OnInitialized(this);
    }

    // Reset progress
    progress_ = {};

    ProcessFileSystemUpdates();

    if (autoProcessAssets_)
    {
        EnsureAssetsAndCacheValid();
        ScanAndQueueAssetProcessing();
    }
}

void AssetManager::ConsumeAssetQueue()
{
    ea::vector<AssetTransformerInputVector> queue;
    while (!requestQueue_.empty() && numOngoingRequests_ < maxConcurrentRequests_)
    {
        if (requestQueue_.back().empty())
        {
            if (numOngoingRequests_ != 0)
                break;

            requestQueue_.pop_back();
            continue;
        }

        ++numOngoingRequests_;
        queue.push_back(ea::move(requestQueue_.back()));
        requestQueue_.pop_back();
    }

    for (const AssetTransformerInputVector& input : queue)
    {
        processCallback_(input,
            [this](const AssetTransformerInputVector& input, const AssetTransformerOutputVector& output,
                const ea::string& message)
        {
            CompleteAssetBatchProcessing(input, output, message);
            //
        });
    }
}

void AssetManager::MarkCacheDirty(const ea::string& resourcePath)
{
    InvalidateAssetsInPath(resourcePath);
}

void AssetManager::ProcessFileSystemUpdates()
{
    // TODO(editor): Throttle updates?
    const StringVector pathUpdates = GetUpdatedPaths(reloadAssetPipelines_);
    reloadAssetPipelines_ = false;

    if (!pathUpdates.empty())
    {
        UpdateAssetPipelines();
        for (const ea::string& updatedPath : pathUpdates)
            InvalidateOutdatedAssetsInPath(updatedPath);
    }
}

void AssetManager::EnsureAssetsAndCacheValid()
{
    if (!hasInvalidAssets_)
        return;

    // Suppress resource reloading before files are deleted.
    // If no processing is queued, this flag will be overwritten on next Update().
    auto cache = GetSubsystem<ResourceCache>();
    cache->SetResourceReloadSuspended(true);

    CleanupInvalidatedAssets();
    CleanupCacheFolder();
    scanAssets_ = true;
    hasInvalidAssets_ = false;
}

void AssetManager::ScanAndQueueAssetProcessing()
{
    if (!scanAssets_)
        return;

    Stats stats;
    ScanAssetsInPath("", stats);

    URHO3D_LOGINFO("Assets scanned: {} processed, {} up-to-date, {} ignored",
        stats.numProcessedAssets_, stats.numUpToDateAssets_, stats.numIgnoredAssets_);

    scanAssets_ = false;
}

void AssetManager::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Assets", assets_);
    SerializeOptionalValue(archive, "AssetPipelineModificationTimes", assetPipelineFiles_);

    if (archive.IsInput())
    {
        for (auto& [resourceName, assetDesc] : assets_)
            assetDesc.resourceName_ = resourceName;
    }
}

void AssetManager::LoadFile(const ea::string& fileName)
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    if (jsonFile->LoadFile(fileName))
        jsonFile->LoadObject("Cache", *this);
}

void AssetManager::SaveFile(const ea::string& fileName) const
{
    auto jsonFile = MakeShared<JSONFile>(context_);
    if (jsonFile->SaveObject("Cache", *this))
        jsonFile->SaveFile(fileName);
}

AssetManager::AssetPipelineList AssetManager::EnumerateAssetPipelineFiles() const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector files;
    fs->ScanDir(files, project_->GetDataPath(), "*.json", SCAN_FILES | SCAN_APPEND | SCAN_RECURSIVE);
    fs->ScanDir(files, project_->GetDataPath(), "*.assetpipeline", SCAN_FILES | SCAN_APPEND | SCAN_RECURSIVE);

    ea::erase_if(files, [&](const ea::string& resourceName) { return !AssetPipeline::CheckExtension(resourceName); });

    AssetPipelineList result;
    for (const ea::string& resourceName : files)
        result.emplace(resourceName, fs->GetLastModifiedTime(GetFileName(resourceName), true));
    return result;
}

AssetManager::AssetPipelineDesc AssetManager::LoadAssetPipeline(const AssetPipeline& pipeline, FileTime modificationTime) const
{
    AssetPipelineDesc result;
    result.resourceName_ = pipeline.GetName();
    result.modificationTime_ = modificationTime;
    result.transformers_ = pipeline.GetTransformers();
    result.dependencies_ = pipeline.GetDependencies();
    return result;
}

AssetManager::AssetPipelineDescVector AssetManager::LoadAssetPipelines(
    const AssetPipelineList& assetPipelineFiles) const
{
    auto cache = GetSubsystem<ResourceCache>();

    AssetPipelineDescVector result;
    for (const auto& [resourceName, modificationTime] : assetPipelineFiles)
    {
        // Never cache these files.
        auto pipeline = cache->GetTempResource<AssetPipeline>(resourceName);
        if (!pipeline)
        {
            URHO3D_LOGERROR("Failed to load {} as JSON file", resourceName);
            continue;
        }

        result.push_back(LoadAssetPipeline(*pipeline, modificationTime));
    }
    return result;
}

AssetManager::AssetPipelineDiffMap AssetManager::DiffAssetPipelines(
    const AssetPipelineDescVector& oldPipelines, const AssetPipelineDescVector& newPipelines) const
{
    AssetPipelineDiffMap result;
    for (const AssetPipelineDesc& pipelineDesc : oldPipelines)
        result[pipelineDesc.resourceName_].oldPipeline_ = &pipelineDesc;
    for (const AssetPipelineDesc& pipelineDesc : newPipelines)
        result[pipelineDesc.resourceName_].newPipeline_ = &pipelineDesc;
    return result;
}

StringVector AssetManager::GetTransformerTypes(const AssetPipelineDesc& pipeline) const
{
    StringVector result;
    for (const AssetTransformer* transformer : pipeline.transformers_)
        result.push_back(transformer->GetTypeName());

    ea::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

AssetTransformerVector AssetManager::GetTransformers(const AssetPipelineDesc& pipeline) const
{
    AssetTransformerVector result;
    for (AssetTransformer* transformer : pipeline.transformers_)
        result.push_back(transformer);
    return result;
}

ea::string AssetManager::GetFileName(const ea::string& resourceName) const
{
    return project_->GetDataPath() + resourceName;
}

bool AssetManager::IsAssetUpToDate(AssetDesc& assetDesc)
{
    auto fs = GetSubsystem<FileSystem>();

    // Check if asset file exists
    const ea::string fileName = GetFileName(assetDesc.resourceName_);
    if (!fs->FileExists(fileName))
        return false;

    // Check if the asset has not been modified
    const FileTime assetModificationTime = fs->GetLastModifiedTime(fileName, true);
    if (assetDesc.modificationTime_ != assetModificationTime)
    {
        if (!ignoredAssetUpdates_.contains(assetDesc.resourceName_))
            return false;

        // Ignore the update once if it was requested
        ignoredAssetUpdates_.erase(assetDesc.resourceName_);
        assetDesc.modificationTime_ = assetModificationTime;
    }

    // Check if outputs are present, don't check modification times for simplicity
    for (const ea::string& outputResourceName : assetDesc.outputs_)
    {
        const ea::string outputFileName = project_->GetCachePath() + outputResourceName;
        if (!fs->FileExists(outputFileName))
            return false;
    }

    // Check if dependencies are present and up-to-date
    for (const auto& [dependencyResourceName, cachedModificationTime] : assetDesc.dependencyModificationTimes_)
    {
        const ea::string dependencyFileName = GetFileName(dependencyResourceName);
        if (!fs->FileExists(dependencyFileName))
            return false;

        const FileTime modificationTime = fs->GetLastModifiedTime(dependencyFileName, true);
        if (modificationTime != cachedModificationTime)
            return false;
    }

    return true;
}

void AssetManager::InvalidateAssetsInPath(const ea::string& resourcePath)
{
    hasInvalidAssets_ = true;
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (resourceName.starts_with(resourcePath))
            assetDesc.cacheInvalid_ = true;
    }
}

void AssetManager::InvalidateTransformedAssetsInPath(const ea::string& resourcePath, const StringVector& transformers)
{
    hasInvalidAssets_ = true;
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (resourceName.starts_with(resourcePath))
        {
            if (assetDesc.IsAnyTransformerUsed(transformers))
                assetDesc.cacheInvalid_ = true;
        }
    }
}

void AssetManager::InvalidateApplicableAssetsInPath(const ea::string& resourcePath, const AssetTransformerVector& transformers)
{
    hasInvalidAssets_ = true;
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (resourceName.starts_with(resourcePath))
        {
            const bool isPostTransform = false; // Not checked by AssetTransformer::IsApplicable
            const AssetTransformerInput input{
                isPostTransform, defaultFlavor_, resourceName, GetFileName(resourceName), assetDesc.modificationTime_};
            if (AssetTransformer::IsApplicable(input, transformers))
                assetDesc.cacheInvalid_ = true;
        }
    }
}

void AssetManager::InvalidateOutdatedAssetsInPath(const ea::string& resourcePath)
{
    hasInvalidAssets_ = true;
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (resourceName.starts_with(resourcePath) || assetDesc.dependencyModificationTimes_.contains(resourcePath))
        {
            if (!IsAssetUpToDate(assetDesc))
                assetDesc.cacheInvalid_ = true;
        }
    }
}

void AssetManager::CleanupAssetOutputs(const AssetDesc& assetDesc)
{
    auto fs = GetSubsystem<FileSystem>();

    for (const ea::string& outputResourceName : assetDesc.outputs_)
    {
        const ea::string outputFileName = project_->GetCachePath() + outputResourceName;
        fs->Delete(outputFileName);
    }
}

void AssetManager::CleanupInvalidatedAssets()
{
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (assetDesc.cacheInvalid_)
            CleanupAssetOutputs(assetDesc);
    }

    ea::erase_if(assets_, [](const auto& pair) { return pair.second.cacheInvalid_; });
}

void AssetManager::CleanupCacheFolder()
{
    auto fs = GetSubsystem<FileSystem>();

    ea::unordered_set<ea::string> foldersToKeep;
    for (const auto& [resourceName, assetDesc] : assets_)
    {
        for (const ea::string& outputResourceName : assetDesc.outputs_)
        {
            for (unsigned i = outputResourceName.find('/'); i != ea::string::npos; i = outputResourceName.find('/', i + 1))
                foldersToKeep.insert(outputResourceName.substr(0, i));
        }
    }

    StringVector allFolders;
    fs->ScanDir(allFolders, project_->GetCachePath(), "", SCAN_DIRS | SCAN_RECURSIVE);

    if (const auto iter = allFolders.find("."); iter != allFolders.end())
        allFolders.erase(iter);
    if (const auto iter = allFolders.find(".."); iter != allFolders.end())
        allFolders.erase(iter);

    ea::erase_if(allFolders, [](const ea::string& folder) { return folder.ends_with(".") || folder.ends_with(".."); });

    for (const ea::string& resourcePath : allFolders)
    {
        if (!foldersToKeep.contains(resourcePath))
            fs->RemoveDir(project_->GetCachePath() + resourcePath, true);
    }
}

StringVector AssetManager::GetUpdatedPaths(bool updateAll)
{
    StringVector allPathUpdates;
    FileChange change;
    while (dataWatcher_->GetNextChange(change))
    {
        allPathUpdates.push_back(change.fileName_);
        if (!change.oldFileName_.empty())
            allPathUpdates.push_back(change.oldFileName_);
    }
    ea::sort(allPathUpdates.begin(), allPathUpdates.end());

    StringVector compressedPathUpdates;
    if (updateAll)
        compressedPathUpdates.push_back("");
    else
    {
        for (const ea::string& path : allPathUpdates)
        {
            if (!compressedPathUpdates.empty() && path.starts_with(compressedPathUpdates.back()))
                continue;
            compressedPathUpdates.push_back(path);
        }
    }

    return compressedPathUpdates;
}

void AssetManager::InitializeAssetPipelines()
{
    const AssetPipelineList newAssetPipelineFiles = EnumerateAssetPipelineFiles();
    const AssetPipelineDescVector newAssetPipelines = LoadAssetPipelines(newAssetPipelineFiles);

    ea::vector<AssetPipelineList::value_type> changedPipelines;
    ea::set_symmetric_difference(
        assetPipelineFiles_.begin(), assetPipelineFiles_.end(),
        newAssetPipelineFiles.begin(), newAssetPipelineFiles.end(),
        std::back_inserter(changedPipelines));

    for (const auto& [resourceName, _] : changedPipelines)
        InvalidateAssetsInPath(GetPath(resourceName));

    assetPipelines_ = newAssetPipelines;
    assetPipelineFiles_ = newAssetPipelineFiles;
    UpdateTransformHierarchy();
}

void AssetManager::UpdateAssetPipelines()
{
    const AssetPipelineList newAssetPipelineFiles = EnumerateAssetPipelineFiles();
    const AssetPipelineDescVector newAssetPipelines = LoadAssetPipelines(newAssetPipelineFiles);
    const AssetPipelineDiffMap pipelinesDiff = DiffAssetPipelines(assetPipelines_, newAssetPipelines);

    for (const auto& [resourceName, diff] : pipelinesDiff)
    {
        // Skip if unchanged
        if (diff.newPipeline_ && diff.oldPipeline_
            && (diff.newPipeline_->modificationTime_ == diff.oldPipeline_->modificationTime_))
            continue;

        const ea::string resourcePath = GetPath(resourceName);
        // Invalidate all assets using transformers in old pipeline
        if (diff.oldPipeline_)
            InvalidateTransformedAssetsInPath(resourcePath, GetTransformerTypes(*diff.oldPipeline_));
        // Invalidate all assets that may use transformers in new pipeline
        if (diff.newPipeline_)
            InvalidateApplicableAssetsInPath(resourcePath, GetTransformers(*diff.newPipeline_));
    }

    assetPipelineFiles_ = newAssetPipelineFiles;
    assetPipelines_ = newAssetPipelines;
    UpdateTransformHierarchy();
}

void AssetManager::UpdateTransformHierarchy()
{
    for (auto& transformerHierarchy : transformerHierarchy_)
    {
        transformerHierarchy->Clear();
        for (const AssetPipelineDesc& pipeline : assetPipelines_)
        {
            for (AssetTransformer* transformer : pipeline.transformers_)
                transformerHierarchy->AddTransformer(GetPath(pipeline.resourceName_), transformer);
            for (const AssetTransformerDependency& link : pipeline.dependencies_)
                transformerHierarchy->AddDependency(link.class_, link.dependsOn_);
        }
        transformerHierarchy->CommitDependencies();
    }
}

void AssetManager::ScanAssetsInPath(const ea::string& resourcePath, Stats& stats)
{
    ea::vector<ea::string> queuedAssets;
    for (const ea::string& resourceName : EnumerateAssetFiles(resourcePath))
    {
        const auto iter = assets_.find(resourceName);
        if (iter == assets_.end())
        {
            AssetTransformerInputVector batch;
            if (QueueAssetProcessing(batch, resourceName, defaultFlavor_, false /*isPostTransform*/))
            {
                queuedAssets.push_back(resourceName);
                if (!batch.empty())
                {
                    requestQueue_.push_back(ea::move(batch));
                    ++progress_.second;
                }
                ++stats.numProcessedAssets_;
            }
            else
                ++stats.numIgnoredAssets_;
        }
        else if (iter->second.transformers_.empty())
            ++stats.numIgnoredAssets_;
        else
            ++stats.numUpToDateAssets_;
    }

    if (!queuedAssets.empty())
    {
        requestQueue_.emplace_back(); // Asset barrier

        for (const ea::string& processedDirectory : EnumerateDisjoinedDirectories(queuedAssets))
        {
            AssetTransformerInputVector batch;
            for (const ea::string& resourceName : EnumerateAssetFiles(processedDirectory))
                QueueAssetProcessing(batch, resourceName, defaultFlavor_, true /*isPostTransform*/);

            if (!batch.empty())
            {
                requestQueue_.push_back(ea::move(batch));
                ++progress_.second;
            }
        }
    }

    ea::reverse(requestQueue_.begin(), requestQueue_.end());
}

bool AssetManager::QueueAssetProcessing(AssetTransformerInputVector& queue, const ea::string& resourceName,
    const ApplicationFlavor& flavor, bool isPostTransform)
{
    auto fs = GetSubsystem<FileSystem>();

    const AssetTransformerVector transformers =
        transformerHierarchy_[isPostTransform]->GetTransformerCandidates(resourceName, flavor);
    const ea::string fileName = GetFileName(resourceName);
    const FileTime assetModifiedTime = fs->GetLastModifiedTime(fileName, true);

    AssetDesc& assetDesc = assets_[resourceName];
    assetDesc.resourceName_ = resourceName;
    assetDesc.modificationTime_ = assetModifiedTime;

    const AssetTransformerInput input{isPostTransform, flavor, resourceName, fileName, assetModifiedTime};
    if (!AssetTransformer::IsApplicable(input, transformers))
    {
        // If post-transformed asset changed, return true to queue post-transform.
        // Don't queue anything right now.
        if (!isPostTransform)
        {
            const AssetTransformerVector postTransformers =
                transformerHierarchy_[true /*isPostTransform*/]->GetTransformerCandidates(resourceName, flavor);
            return !postTransformers.empty();
        }

        return false;
    }

    // In most cases, everything is already cleared.
    // However, post-transformer processing may be triggered by changes in other files in the directory.
    // Cleanup all known outputs just in case.
    CleanupAssetOutputs(assetDesc);

    const ea::string tempPath = project_->GetRandomTemporaryPath();
    const ea::string outputResourceName = resourceName + ".d";
    const ea::string outputFileName = tempPath + outputResourceName;
    queue.push_back(AssetTransformerInput{input, tempPath, outputFileName, outputResourceName});
    return true;
}

ea::optional<AssetTransformerOutput> AssetManager::ProcessAsset(const AssetTransformerInput& input) const
{
    auto fs = GetSubsystem<FileSystem>();

    const ea::string& cachePath = project_->GetCachePath();
    const AssetTransformerVector transformers =
        transformerHierarchy_[input.isPostTransform_]->GetTransformerCandidates(input.resourceName_, input.flavor_);

    AssetTransformerOutput output;
    if (AssetTransformer::ExecuteTransformersAndStore(input, cachePath, output, transformers))
        return output;
    else
        return ea::nullopt;
}

void AssetManager::ProcessAssetBatch(const AssetTransformerInputVector& input, const OnProcessAssetCompleted& callback) const
{
    const unsigned batchSize = input.size();

    AssetTransformerOutputVector result;
    result.resize(batchSize);
    for (unsigned i = 0; i < batchSize; ++i)
        result[i] = ProcessAsset(input[i]);

    callback(input, result, EMPTY_STRING);
}

bool AssetManager::CompleteAssetProcessing(
    const AssetTransformerInput& input, const ea::optional<AssetTransformerOutput>& output)
{
    if (!output)
        return false;

    AssetDesc& assetDesc = assets_[input.resourceName_];
    assetDesc.resourceName_ = input.resourceName_;
    assetDesc.modificationTime_ = input.inputFileTime_;
    assetDesc.dependencyModificationTimes_ = output->dependencyModificationTimes_;
    assetDesc.outputs_ = output->outputResourceNames_;
    assetDesc.transformers_ = output->appliedTransformers_;

    if (output->sourceModified_)
        ignoredAssetUpdates_.insert(input.resourceName_);

    URHO3D_LOGDEBUG("Asset {} was processed with {} ({} files generated{})",
        input.resourceName_, assetDesc.GetTransformerDebugString(), assetDesc.outputs_.size(),
        output->sourceModified_ ? ", source modified" : "");

    return true;
}

void AssetManager::CompleteAssetBatchProcessing(const AssetTransformerInputVector& input,
    const AssetTransformerOutputVector& output, const ea::string& message)
{
    if (numOngoingRequests_ > 0)
        --numOngoingRequests_;
    else
        URHO3D_ASSERTLOG(false, "AssetManager::CompleteAssetProcessing() called with no ongoing requests");

    ++progress_.first;

    const unsigned batchSize = input.size();
    URHO3D_ASSERTLOG(output.empty() || output.size() == batchSize,
        "AssetManager::CompleteAssetProcessing() called with invalid output size");

    for (unsigned i = 0; i < batchSize; ++i)
    {
        if (!CompleteAssetProcessing(input[i], i < output.size() ? output[i] : ea::nullopt))
            URHO3D_LOGWARNING("Asset {} was not processed", input[i].resourceName_);
    }

    if (!message.empty())
        URHO3D_LOGWARNING("{}", message);
}

StringVector AssetManager::EnumerateAssetFiles(const ea::string& resourcePath) const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector result;
    fs->ScanDir(result, GetFileName(resourcePath), "", SCAN_FILES | SCAN_RECURSIVE);

    ea::erase_if(result, [this](const ea::string& fileName)
    {
        return project_->IsFileNameIgnored(fileName);
    });

    for (ea::string& fileName : result)
        fileName = AddTrailingSlash(resourcePath) + fileName;

    return result;
}

void AssetManager::OnReflectionRemoved(ObjectReflection* reflection)
{
    bool anyRemoved = false;
    for (auto& transformerHierarchy : transformerHierarchy_)
        anyRemoved = transformerHierarchy->RemoveTransformers(reflection->GetTypeInfo()) || anyRemoved;

    if (anyRemoved)
    {
        InvalidateAssetsInPath("");
        assetPipelines_.clear();
        reloadAssetPipelines_ = true;

        auto cache = GetSubsystem<ResourceCache>();
        cache->ReleaseResources(AssetPipeline::GetTypeStatic());
    }
}

}
