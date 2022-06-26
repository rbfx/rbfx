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
#include "../Project/ProjectEditor.h"

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/JSONFile.h>

#include <EASTL/sort.h>

namespace Urho3D
{

const ea::string AssetManager::ResourceNameSuffix{"AssetPipeline.json"};

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

AssetManager::AssetManager(Context* context)
    : Object(context)
    , projectEditor_(GetSubsystem<ProjectEditor>())
    , dataWatcher_(MakeShared<FileWatcher>(context))
    , transformerHierarchy_(MakeShared<AssetTransformerHierarchy>(context_))
{
    dataWatcher_->StartWatching(projectEditor_->GetDataPath(), true);
}

AssetManager::~AssetManager()
{
}

void AssetManager::Update()
{
    CollectPathUpdates();
    if (!pendingPathUpdates_.empty())
    {
        UpdateAssetPipelines();
        for (const ea::string& updatedPath : pendingPathUpdates_)
            InvalidateOutdatedAssetsInPath(updatedPath);
    }

    if (validateAssets_)
    {
        CleanupInvalidatedAssets();
        CleanupCacheFolder();
        rescanAssets_ = true;
        validateAssets_ = false;
    }

    if (rescanAssets_)
    {
        stats_ = {};
        ScanAssetsInPath("");

        rescanAssets_ = false;
        URHO3D_LOGINFO("Assets scanned: {} processed, {} up-to-date, {} ignored",
            stats_.numProcessedAssets_, stats_.numUpToDateAssets_, stats_.numIgnoredAssets_);
    }
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

    InitializeAssetPipelines();
    InvalidateOutdatedAssetsInPath("");
    rescanAssets_ = true;
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
    fs->ScanDir(files, projectEditor_->GetDataPath(), "*.json", SCAN_FILES, true);

    const ea::string suffix = Format("/{}", ResourceNameSuffix);
    ea::erase_if(files, [&](const ea::string& resourceName) { return !resourceName.ends_with(suffix); });

    AssetPipelineList result;
    for (const ea::string& resourceName : files)
        result.emplace(resourceName, fs->GetLastModifiedTime(GetFileName(resourceName), true));
    return result;
}

AssetManager::AssetPipelineDesc AssetManager::LoadAssetPipeline(
    const JSONFile& jsonFile, const ea::string& resourceName, FileTime modificationTime) const
{
    AssetPipelineDesc result;
    result.resourceName_ = resourceName;
    result.modificationTime_ = modificationTime;

    const JSONValue& rootElement = jsonFile.GetRoot();
    const JSONValue& transformersElement = rootElement["Transformers"];
    const JSONValue& dependenciesElement = rootElement["Dependencies"];

    for (const JSONValue& value : transformersElement.GetArray())
    {
        const ea::string& transformerClass = value["_Class"].GetString();
        const auto newTransformer = DynamicCast<AssetTransformer>(context_->CreateObject(transformerClass));
        if (!newTransformer)
        {
            URHO3D_LOGERROR("Failed to instantiate transformer {} of JSON file {}", transformerClass, resourceName);
            continue;
        }

        JSONInputArchive archive{context_, value, &jsonFile};
        const bool loaded = ConsumeArchiveException([&]
        {
            SerializeValue(archive, transformerClass.c_str(), *newTransformer);
        });

        if (loaded)
            result.transformers_.push_back(newTransformer);
    }

    for (const JSONValue& value : dependenciesElement.GetArray())
    {
        const ea::string& transformerClass = value["Class"].GetString();
        const ea::string& dependsOn = value["DependsOn"].GetString();
        result.dependencies_.emplace_back(transformerClass, dependsOn);
    }

    return result;
}

AssetManager::AssetPipelineDescVector AssetManager::LoadAssetPipelines(
    const AssetPipelineList& assetPipelineFiles) const
{
    AssetPipelineDescVector result;
    for (const auto& [resourceName, modificationTime] : assetPipelineFiles)
    {
        auto jsonFile = MakeShared<JSONFile>(context_);
        if (!jsonFile->LoadFile(GetFileName(resourceName)))
        {
            URHO3D_LOGERROR("Failed to load {} as JSON file", resourceName);
            continue;
        }

        result.push_back(LoadAssetPipeline(*jsonFile, resourceName, modificationTime));
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
    return projectEditor_->GetDataPath() + resourceName;
}

bool AssetManager::IsAssetUpToDate(const AssetDesc& assetDesc) const
{
    auto fs = GetSubsystem<FileSystem>();

    // Check if asset file exists
    const ea::string fileName = GetFileName(assetDesc.resourceName_);
    if (!fs->FileExists(fileName))
        return false;

    // Check if the asset has not been modified
    if (assetDesc.modificationTime_ != fs->GetLastModifiedTime(fileName))
        return false;

    // Check if outputs are present, don't check modification times for simplicity
    for (const ea::string& outputResourceName : assetDesc.outputs_)
    {
        const ea::string outputFileName = projectEditor_->GetCachePath() + outputResourceName;
        if (!fs->FileExists(outputFileName))
            return false;
    }

    return true;
}

void AssetManager::InvalidateAssetsInPath(const ea::string& resourcePath)
{
    validateAssets_ = true;
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (resourceName.starts_with(resourcePath))
            assetDesc.cacheInvalid_ = true;
    }
}

void AssetManager::InvalidateTransformedAssetsInPath(const ea::string& resourcePath, const StringVector& transformers)
{
    validateAssets_ = true;
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
    validateAssets_ = true;
    for (auto& [resourceName, assetDesc] : assets_)
    {
        if (resourceName.starts_with(resourcePath))
        {
            const AssetTransformerInput input{defaultFlavor_, resourceName, GetFileName(resourceName)};
            if (AssetTransformer::IsApplicable(input, transformers))
                assetDesc.cacheInvalid_ = true;
        }
    }
}

void AssetManager::InvalidateOutdatedAssetsInPath(const ea::string& resourcePath)
{
    validateAssets_ = true;
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
            const ea::string outputFileName = projectEditor_->GetCachePath() + outputResourceName;
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
    fs->ScanDir(allFolders, projectEditor_->GetCachePath(), "", SCAN_DIRS, true);

    if (const auto iter = allFolders.find("."); iter != allFolders.end())
        allFolders.erase(iter);
    if (const auto iter = allFolders.find(".."); iter != allFolders.end())
        allFolders.erase(iter);

    ea::erase_if(allFolders, [](const ea::string& folder) { return folder.ends_with(".") || folder.ends_with(".."); });

    for (const ea::string& resourcePath : allFolders)
    {
        if (!foldersToKeep.contains(resourcePath))
            fs->RemoveDir(projectEditor_->GetCachePath() + resourcePath, true);
    }
}

void AssetManager::CollectPathUpdates()
{
    // TODO(editor): Throttle this
    StringVector pathUpdates;
    FileChange change;
    while (dataWatcher_->GetNextChange(change))
    {
        pathUpdates.push_back(change.fileName_);
        if (!change.oldFileName_.empty())
            pathUpdates.push_back(change.oldFileName_);
    }
    ea::sort(pathUpdates.begin(), pathUpdates.end());

    pendingPathUpdates_.clear();
    if (reloadAssetPipelines_)
        pendingPathUpdates_.push_back("");
    else
    {
        for (const ea::string& path : pathUpdates)
        {
            if (!pendingPathUpdates_.empty() && path.starts_with(pendingPathUpdates_.back()))
                continue;
            pendingPathUpdates_.push_back(path);
        }
    }

    reloadAssetPipelines_ = false;
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
        for (const auto& [transformerClass, dependsOn] : pipeline.dependencies_)
            transformerHierarchy_->AddDependency(transformerClass, dependsOn);
    }
    transformerHierarchy_->CommitDependencies();
}

void AssetManager::ScanAssetsInPath(const ea::string& resourcePath)
{
    for (const ea::string& resourceName : EnumerateAssetFiles(resourcePath))
    {
        const auto iter = assets_.find(resourceName);
        if (iter == assets_.end())
            ProcessAsset(resourceName, defaultFlavor_);
        else if (iter->second.transformers_.empty())
            ++stats_.numIgnoredAssets_;
        else
            ++stats_.numUpToDateAssets_;
    }
}

void AssetManager::ProcessAsset(const ea::string& resourceName, const ea::string& flavor)
{
    auto fs = GetSubsystem<FileSystem>();

    const AssetTransformerVector transformers = transformerHierarchy_->GetTransformerCandidates(resourceName, flavor);
    const ea::string fileName = GetFileName(resourceName);
    const FileTime assetModifiedTime = fs->GetLastModifiedTime(fileName);

    const AssetTransformerInput input{flavor, resourceName, fileName};
    if (AssetTransformer::IsApplicable(input, transformers))
    {
        ++stats_.numProcessedAssets_;

        const TemporaryDir tempFolderHolder = projectEditor_->CreateTemporaryDir();
        const ea::string tempFolder = tempFolderHolder.GetPath() + resourceName;
        AssetTransformerOutput output;
        if (AssetTransformer::Execute(AssetTransformerInput{input, tempFolder}, transformers, output))
        {
            const ea::string& cachePath = projectEditor_->GetCachePath();

            StringVector copiedFiles;
            fs->CopyDir(tempFolderHolder.GetPath(), cachePath, &copiedFiles);

            for (ea::string& fileName : copiedFiles)
                fileName = fileName.substr(cachePath.length());

            AssetDesc& assetDesc = assets_[resourceName];
            assetDesc.resourceName_ = resourceName;
            assetDesc.modificationTime_ = assetModifiedTime;
            assetDesc.outputs_ = copiedFiles;
            for (unsigned index : output.appliedTransformers_)
                assetDesc.transformers_.insert(transformers[index]->GetTypeName());

            URHO3D_LOGDEBUG("Asset {} was processed with {} outputs", resourceName, copiedFiles.size());
        }
    }
    else
    {
        ++stats_.numIgnoredAssets_;

        AssetDesc& assetDesc = assets_[resourceName];
        assetDesc.resourceName_ = resourceName;
        assetDesc.modificationTime_ = assetModifiedTime;
    }
}

StringVector AssetManager::EnumerateAssetFiles(const ea::string& resourcePath) const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector result;
    fs->ScanDir(result, GetFileName(resourcePath), "", SCAN_FILES, true);
    return result;
}

}
