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

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Utility/AssetTransformerHierarchy.h>

#include <EASTL/optional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class JSONFile;
class ProjectEditor;

/// Sorted list of asset transformer pipelines.
using AssetPipelineList = ea::unordered_map<ea::string, FileTime>;

/// Manages assets of the project.
class AssetManager : public Object
{
    URHO3D_OBJECT(AssetManager, Object);

public:
    static const ea::string ResourceNameSuffix;

    explicit AssetManager(Context* context);
    ~AssetManager() override;

    void Update();

    /// Serialize
    /// @{
    void SerializeInBlock(Archive& archive) override;
    void LoadFile(const ea::string& fileName);
    void SaveFile(const ea::string& fileName) const;
    /// @}

private:
    struct AssetPipelineDesc
    {
        ea::string resourceName_;
        FileTime modificationTime_{};
        ea::vector<SharedPtr<AssetTransformer>> transformers_;
        ea::vector<ea::pair<ea::string, ea::string>> dependencies_;
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

        bool cacheInvalid_{};

        void SerializeInBlock(Archive& archive);
        bool IsAnyTransformerUsed(const StringVector& transformers) const;
    };

    /// Utility functions that don't change internal state
    /// @{
    AssetPipelineList EnumerateAssetPipelineFiles() const;
    AssetPipelineDesc LoadAssetPipeline(const JSONFile& jsonFile,
        const ea::string& resourceName, FileTime modificationTime) const;
    AssetPipelineDescVector LoadAssetPipelines(const AssetPipelineList& assetPipelineFiles) const;
    AssetPipelineDiffMap DiffAssetPipelines(const AssetPipelineDescVector& oldPipelines,
        const AssetPipelineDescVector& newPipelines) const;
    StringVector GetTransformerTypes(const AssetPipelineDesc& pipeline) const;
    AssetTransformerVector GetTransformers(const AssetPipelineDesc& pipeline) const;
    ea::string GetFileName(const ea::string& resourceName) const;
    bool IsAssetUpToDate(const AssetDesc& assetDesc) const;
    /// @}

    /// Cache manipulation.
    /// @{
    void InvalidateTransformedAssetsInPath(const ea::string& resourcePath, const StringVector& transformers);
    void InvalidateApplicableAssetsInPath(const ea::string& resourcePath, const AssetTransformerVector& transformers);
    void InvalidateOutdatedAssetsInPath(const ea::string& resourcePath);
    void CleanupInvalidatedAssets();
    void CleanupCacheFolder();
    /// @}

    void CollectPathUpdates();
    void InitializeAssetPipelines();
    void UpdateAssetPipelines();
    void UpdateTransformHierarchy(const AssetPipelineDescVector& pipelines);

    void ScanAssetsInPath(const ea::string& resourcePath);
    void ProcessAsset(const ea::string& resourceName);
    StringVector EnumerateAssetFiles(const ea::string& resourcePath) const;

    const WeakPtr<ProjectEditor> projectEditor_;
    SharedPtr<FileWatcher> dataWatcher_;

    ea::string defaultFlavor_{"*"}; // TODO(editor): Make configurable
    bool reloadAssetPipelines_{};
    bool validateAssets_{};
    bool rescanAssets_{};
    StringVector pendingPathUpdates_;

    AssetPipelineDescVector assetPipelines_;
    SharedPtr<AssetTransformerHierarchy> transformerHierarchy_;
    ea::unordered_map<ea::string, AssetDesc> assets_;

    struct Stats
    {
        unsigned numProcessedAssets_{};
        unsigned numIgnoredAssets_{};
        unsigned numUpToDateAssets_{};
    } stats_;
};

}
