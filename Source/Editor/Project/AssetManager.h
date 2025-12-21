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

#pragma once

#include <Urho3D/Core/Signal.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Utility/AssetPipeline.h>
#include <Urho3D/Utility/AssetTransformerHierarchy.h>

#include <EASTL/array.h>
#include <EASTL/functional.h>
#include <EASTL/map.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class JSONFile;
class Project;

using AssetTransformerInputVector = ea::vector<AssetTransformerInput>;
using AssetTransformerOutputVector = ea::vector<ea::optional<AssetTransformerOutput>>;

/// Manages assets of the project.
class AssetManager : public Object
{
    URHO3D_OBJECT(AssetManager, Object);

public:
    Signal<void()> OnInitialized;

    /// Number of processed assets and total number of assets in the current queue.
    using ProgressInfo = ea::pair<unsigned, unsigned>;

    using OnProcessAssetCompleted = ea::function<void(const AssetTransformerInputVector& input,
        const AssetTransformerOutputVector& output, const ea::string& message)>;
    using OnProcessAssetQueued =
        ea::function<void(const AssetTransformerInputVector& input, const OnProcessAssetCompleted& callback)>;

    explicit AssetManager(Context* context);
    ~AssetManager() override;

    /// Override asset processing.
    void SetProcessCallback(const OnProcessAssetQueued& callback, unsigned maxConcurrency = 1);
    /// Process asset without affecting internal state of AssetManager.
    void ProcessAssetBatch(const AssetTransformerInputVector& input, const OnProcessAssetCompleted& callback) const;

    /// Initialize asset manager.
    /// Should be called after the manager configuration is loaded from file *and* plugins are initialized.
    void Initialize(bool readOnly);
    void Update();
    void MarkCacheDirty(const ea::string& resourcePath);

    /// Return current progress of asset processing:
    ProgressInfo GetProgress() const { return progress_; }
    /// Return whether asset manager is currently processing assets.
    bool IsProcessing() const { return !requestQueue_.empty() || numOngoingRequests_ != 0; }

    /// Serialize
    /// @{
    void SerializeInBlock(Archive& archive) override;
    void LoadFile(const ea::string& fileName);
    void SaveFile(const ea::string& fileName) const;
    /// @}

private:
    /// Sorted list of asset pipeline files.
    using AssetPipelineList = ea::map<ea::string, FileTime>;

    struct AssetPipelineDesc
    {
        ea::string resourceName_;
        FileTime modificationTime_{};
        ea::vector<SharedPtr<AssetTransformer>> transformers_;
        ea::vector<AssetTransformerDependency> dependencies_;
    };
    using AssetPipelineDescVector = ea::vector<AssetPipelineDesc>;

    struct AssetPipelineDiff
    {
        const AssetPipelineDesc* oldPipeline_{};
        const AssetPipelineDesc* newPipeline_{};
    };
    using AssetPipelineDiffMap = ea::unordered_map<ea::string, AssetPipelineDiff>;

    struct AssetDesc
    {
        ea::string resourceName_;
        ea::vector<ea::string> outputs_;
        ea::unordered_set<ea::string> transformers_;
        FileTime modificationTime_{};
        ea::unordered_map<ea::string, FileTime> dependencyModificationTimes_;

        bool cacheInvalid_{};

        void SerializeInBlock(Archive& archive);
        bool IsAnyTransformerUsed(const StringVector& transformers) const;
        ea::string GetTransformerDebugString() const;
    };

    struct Stats
    {
        unsigned numProcessedAssets_{};
        unsigned numIgnoredAssets_{};
        unsigned numUpToDateAssets_{};
    };

    /// Utility functions that don't change internal state
    /// @{
    StringVector EnumerateAssetFiles(const ea::string& resourcePath) const;
    AssetPipelineList EnumerateAssetPipelineFiles() const;
    AssetPipelineDesc LoadAssetPipeline(const AssetPipeline& pipeline, FileTime modificationTime) const;
    AssetPipelineDescVector LoadAssetPipelines(const AssetPipelineList& assetPipelineFiles) const;
    AssetPipelineDiffMap DiffAssetPipelines(const AssetPipelineDescVector& oldPipelines,
        const AssetPipelineDescVector& newPipelines) const;
    StringVector GetTransformerTypes(const AssetPipelineDesc& pipeline) const;
    AssetTransformerVector GetTransformers(const AssetPipelineDesc& pipeline) const;
    ea::string GetFileName(const ea::string& resourceName) const;
    bool IsAssetUpToDate(AssetDesc& assetDesc);
    ea::optional<AssetTransformerOutput> ProcessAsset(const AssetTransformerInput& input) const;
    /// @}

    /// Cache manipulation.
    /// @{
    void InvalidateAssetsInPath(const ea::string& resourcePath);
    void InvalidateTransformedAssetsInPath(const ea::string& resourcePath, const StringVector& transformers);
    void InvalidateApplicableAssetsInPath(const ea::string& resourcePath, const AssetTransformerVector& transformers);
    void InvalidateOutdatedAssetsInPath(const ea::string& resourcePath);
    void CleanupAssetOutputs(const AssetDesc& assetDesc);
    void CleanupInvalidatedAssets();
    void CleanupCacheFolder();
    /// @}

    StringVector GetUpdatedPaths(bool updateAll);

    void InitializeAssetPipelines();
    void UpdateAssetPipelines();
    void UpdateTransformHierarchy();
    void ProcessFileSystemUpdates();
    void EnsureAssetsAndCacheValid();
    void ScanAndQueueAssetProcessing();

    void ScanAssetsInPath(const ea::string& resourcePath, Stats& stats);
    bool QueueAssetProcessing(AssetTransformerInputVector& queue, const ea::string& resourceName,
        const ApplicationFlavor& flavor, bool isPostTransform);
    void ConsumeAssetQueue();

    bool CompleteAssetProcessing(const AssetTransformerInput& input, const ea::optional<AssetTransformerOutput>& output);
    void CompleteAssetBatchProcessing(const AssetTransformerInputVector& input, const AssetTransformerOutputVector& output,
        const ea::string& message);

    void OnReflectionRemoved(ObjectReflection* reflection);

    const WeakPtr<Project> project_;
    SharedPtr<FileWatcher> dataWatcher_;

    OnProcessAssetQueued processCallback_;
    unsigned maxConcurrentRequests_{};

    ApplicationFlavor defaultFlavor_; // TODO(editor): Make configurable

    bool initialized_{};
    bool autoProcessAssets_{};
    bool reloadAssetPipelines_{};
    bool hasInvalidAssets_{};
    bool scanAssets_{};

    AssetPipelineDescVector assetPipelines_;
    ea::array<SharedPtr<AssetTransformerHierarchy>, 2 /*isPostTransform*/> transformerHierarchy_;
    ea::unordered_map<ea::string, AssetDesc> assets_;
    AssetPipelineList assetPipelineFiles_;
    ea::unordered_set<ea::string> ignoredAssetUpdates_;

    ea::vector<AssetTransformerInputVector> requestQueue_;
    unsigned numOngoingRequests_{};

    ProgressInfo progress_;
};

}
