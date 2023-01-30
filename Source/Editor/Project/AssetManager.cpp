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

void AssetManager::AssetDesc::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Outputs", outputs_);
    SerializeOptionalValue(archive, "Transformers", transformers_);
    SerializeOptionalValue(archive, "AssetModifiedTime", modificationTime_);
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
    , transformerHierarchy_(MakeShared<AssetTransformerHierarchy>(context_))
{
    dataWatcher_->StartWatching(project_->GetDataPath(), true);
    context_->OnReflectionRemoved.Subscribe(this, &AssetManager::OnReflectionRemoved);
    SetProcessCallback(nullptr);
}

AssetManager::~AssetManager()
{
}

void AssetManager::SetProcessCallback(const OnProcessAssetQueued& callback, unsigned maxConcurrency)
{
    const auto defaultCallback = [this](const AssetTransformerInput& input, const OnProcessAssetCompleted& callback)
    {
        ProcessAsset(input, callback);
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
    if (!requestQueue_.empty() || numOngoingRequests_ != 0)
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
    ea::vector<AssetTransformerInput> queue;
    while (!requestQueue_.empty() && numOngoingRequests_ < maxConcurrentRequests_)
    {
        ++numOngoingRequests_;
        ++progress_.second;
        queue.push_back(requestQueue_.back());
        requestQueue_.pop_back();
    }

    for (const AssetTransformerInput& input : queue)
    {
        processCallback_(input,
            [this](const AssetTransformerInput& input, const ea::optional<AssetTransformerOutput>& output, const ea::string& message)
        {
            CompleteAssetProcessing(input, output, message);
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
    fs->ScanDirAdd(files, project_->GetDataPath(), "*.json", SCAN_FILES, true);
    fs->ScanDirAdd(files, project_->GetDataPath(), "*.assetpipeline", SCAN_FILES, true);

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
    if (assetDesc.modificationTime_ != fs->GetLastModifiedTime(fileName))
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
            const AssetTransformerInput input{defaultFlavor_, resourceName, GetFileName(resourceName), assetDesc.modificationTime_};
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
        if (resourceName.starts_with(resourcePath))
        {
            if (!IsAssetUpToDate(assetDesc))
                assetDesc.cacheInvalid_ = true;
        }
    }
}

void AssetManager::CleanupInvalidatedAssets()
{
    auto fs = GetSubsystem<FileSystem>();
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (!assetDesc.cacheInvalid_)
            continue;

        for (const ea::string& outputResourceName : assetDesc.outputs_)
        {
            const ea::string outputFileName = project_->GetCachePath() + outputResourceName;
            fs->Delete(outputFileName);
        }
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
    fs->ScanDir(allFolders, project_->GetCachePath(), "", SCAN_DIRS, true);

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
    transformerHierarchy_->Clear();
    for (const AssetPipelineDesc& pipeline : assetPipelines_)
    {
        for (AssetTransformer* transformer : pipeline.transformers_)
            transformerHierarchy_->AddTransformer(GetPath(pipeline.resourceName_), transformer);
        for (const AssetTransformerDependency& link : pipeline.dependencies_)
            transformerHierarchy_->AddDependency(link.class_, link.dependsOn_);
    }
    transformerHierarchy_->CommitDependencies();
}

void AssetManager::ScanAssetsInPath(const ea::string& resourcePath, Stats& stats)
{
    for (const ea::string& resourceName : EnumerateAssetFiles(resourcePath))
    {
        const auto iter = assets_.find(resourceName);
        if (iter == assets_.end())
        {
            if (QueueAssetProcessing(resourceName, defaultFlavor_))
                ++stats.numProcessedAssets_;
            else
                ++stats.numIgnoredAssets_;
        }
        else if (iter->second.transformers_.empty())
            ++stats.numIgnoredAssets_;
        else
            ++stats.numUpToDateAssets_;
    }
}

bool AssetManager::QueueAssetProcessing(const ea::string& resourceName, const ApplicationFlavor& flavor)
{
    auto fs = GetSubsystem<FileSystem>();

    const AssetTransformerVector transformers = transformerHierarchy_->GetTransformerCandidates(resourceName, flavor);
    const ea::string fileName = GetFileName(resourceName);
    const FileTime assetModifiedTime = fs->GetLastModifiedTime(fileName);

    AssetDesc& assetDesc = assets_[resourceName];
    assetDesc.resourceName_ = resourceName;
    assetDesc.modificationTime_ = assetModifiedTime;

    const AssetTransformerInput input{flavor, resourceName, fileName, assetModifiedTime};
    if (!AssetTransformer::IsApplicable(input, transformers))
        return false;

    const ea::string tempPath = project_->GetRandomTemporaryPath();
    const ea::string outputFileName = tempPath + resourceName;
    requestQueue_.push_back(AssetTransformerInput{input, tempPath, outputFileName});
    return true;
}

void AssetManager::ProcessAsset(const AssetTransformerInput& input, const OnProcessAssetCompleted& callback) const
{
    auto fs = GetSubsystem<FileSystem>();

    const ea::string& cachePath = project_->GetCachePath();
    const AssetTransformerVector transformers = transformerHierarchy_->GetTransformerCandidates(
        input.resourceName_, input.flavor_);

    AssetTransformerOutput output;
    if (AssetTransformer::ExecuteTransformersAndStore(input, cachePath, output, transformers))
        callback(input, ea::move(output), EMPTY_STRING);
    else
        callback(input, ea::nullopt, EMPTY_STRING);
}

void AssetManager::CompleteAssetProcessing(
    const AssetTransformerInput& input, const ea::optional<AssetTransformerOutput>& output, const ea::string& message)
{
    if (numOngoingRequests_ > 0)
        --numOngoingRequests_;
    else
        URHO3D_ASSERTLOG(false, "AssetManager::CompleteAssetProcessing() called with no ongoing requests");

    ++progress_.first;

    if (output)
    {
        AssetDesc& assetDesc = assets_[input.resourceName_];
        assetDesc.resourceName_ = input.resourceName_;
        assetDesc.modificationTime_ = input.inputFileTime_;
        assetDesc.outputs_ = output->outputResourceNames_;
        assetDesc.transformers_ = output->appliedTransformers_;

        if (output->sourceModified_)
            ignoredAssetUpdates_.insert(input.resourceName_);

        URHO3D_LOGDEBUG("Asset {} was processed with {} ({} files generated{})",
            input.resourceName_, assetDesc.GetTransformerDebugString(), assetDesc.outputs_.size(),
            output->sourceModified_ ? ", source modified" : "");

        if (!message.empty())
            URHO3D_LOGWARNING("{}", message);
    }
    else
    {
        URHO3D_LOGWARNING("Asset {} was not processed: {}", input.resourceName_, message.empty() ? "unknown error" : message);
    }
}

StringVector AssetManager::EnumerateAssetFiles(const ea::string& resourcePath) const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector result;
    fs->ScanDir(result, GetFileName(resourcePath), "", SCAN_FILES, true);

    ea::erase_if(result, [this](const ea::string& fileName)
    {
        return project_->IsFileNameIgnored(fileName);
    });
    return result;
}

void AssetManager::OnReflectionRemoved(ObjectReflection* reflection)
{
    if (transformerHierarchy_->RemoveTransformers(reflection->GetTypeInfo()))
    {
        InvalidateAssetsInPath("");
        assetPipelines_.clear();
        reloadAssetPipelines_ = true;

        auto cache = GetSubsystem<ResourceCache>();
        cache->ReleaseResources(AssetPipeline::GetTypeStatic());
    }
}

}
