//
// Copyright (c) 2017-2019 the rbfx project.
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

#include <atomic>
#include <EASTL/map.h>
#include <EASTL/unordered_map.h>

#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/IO/FileWatcher.h>
#include <Urho3D/IO/Archive.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Serializable.h>
#include <Toolbox/IO/ContentUtilities.h>

#include "Pipeline/Importers/AssetImporter.h"
#include "Pipeline/Importers/ModelImporter.h"
#include "Pipeline/Importers/SceneConverter.h"
#include "Pipeline/Importers/TextureImporter.h"
#include "Pipeline/Asset.h"
#include "Pipeline/Packager.h"

namespace Urho3D
{

static const ea::string DEFAULT_PIPELINE_FLAVOR{"default"};

enum class PipelineBuildFlag : unsigned
{
    /// Default configuration.
    DEFAULT = 0,
    /// Skip importing up-to-date assets.
    SKIP_UP_TO_DATE = 1U,
    /// Execute optional importers as well.
    EXECUTE_OPTIONAL = 1U << 1U,
};
URHO3D_FLAGSET(PipelineBuildFlag, PipelineBuildFlags);

class Pipeline : public Object
{
    URHO3D_OBJECT(Pipeline, Object);
public:
    ///
    explicit Pipeline(Context* context);
    ///
    static void RegisterObject(Context* context);
    /// Remove any cached assets belonging to specified resource.
    void ClearCache(const ea::string& resourceName);
    /// Returns asset object, creates it for existing asset if pipeline has not done it yet. Returns nullptr if `autoCreate` is set to false and asset was not loaded yet.
    Asset* GetAsset(const eastl::string& resourceName, bool autoCreate = true);
    /// Returns a list of currently present flavors. List always has at least "default" flavor.
    const StringVector& GetFlavors() const { return flavors_; }
    /// Add a custom flavor.
    void AddFlavor(const ea::string& name);
    /// Remove a custom flavor.
    void RemoveFlavor(const ea::string& name);
    /// Rename a custom flavor.
    void RenameFlavor(const ea::string& oldName, const ea::string& newName);
    /// Schedules import task to run on worker thread.
    SharedPtr <WorkItem> ScheduleImport(Asset* asset, const ea::string& flavor=DEFAULT_PIPELINE_FLAVOR, PipelineBuildFlags flags=PipelineBuildFlag::DEFAULT);
    /// Executes importers of specified asset asychronously.
    bool ExecuteImport(Asset* asset, const ea::string& flavor, PipelineBuildFlags flags);
    /// Mass-schedule assets for importing.
    void BuildCache(const ea::string& flavor=DEFAULT_PIPELINE_FLAVOR, PipelineBuildFlags flags=PipelineBuildFlag::DEFAULT);
    /// Blocks calling thread until all pipeline tasks complete.
    void WaitForCompletion() const;
    /// Queue packaging of resources for specified flavor. This function returns immediately, however user will be blocked from interacting with editor by modal window until process is done.
    void CreatePaksAsync(const ea::string& flavor);
    /// Returns true if resource or any of it's parent directories have non-default flavor settings.
    bool HasFlavorSettings(const ea::string& resourceName);
    ///
    bool Serialize(Archive& archive) override;
    /// Returns a map of default engine settings that will be applied on the start of player application.
    ea::map<ea::string, Variant>& GetDefaultEngineSettings(const ea::string& flavor) { return engineParameters_[flavor]; }

protected:
    /// Watch directory for changed assets and automatically convert them.
    void EnableWatcher();
    /// Handles file watchers.
    void OnEndFrame(StringHash, VariantMap&);
    /// Handles modal dialogs.
    void OnUpdate();

    /// List of file watchers responsible for watching game data folders for asset changes.
    FileWatcher watcher_;
    /// List of pipeline flavors.
    StringVector flavors_{DEFAULT_PIPELINE_FLAVOR};
    /// A list of loaded assets.
    ea::unordered_map<ea::string /*name*/, SharedPtr<Asset>> assets_{};
    /// A list of all available importers. When new importer is created it should be added here.
    ea::vector<StringHash> importers_
    {
        ModelImporter::GetTypeStatic(),
        SceneConverter::GetTypeStatic(),
        TextureImporter::GetTypeStatic(),
    };

    ///
    Mutex mutex_;
    /// A list of assets that were modified in non-main thread and need to be saved on main thread.
    ea::vector<SharedPtr<Asset>> dirtyAssets_;
    /// A list of flavors that are yet to be packaged.
    StringVector pendingPackageFlavor_{};
    /// Current active packager. Null when packaging is not in progress.
    SharedPtr<Packager> packager_{};
    /// Title of the modal dialog that shows when packaging files.
    ea::string packagerModalTitle_{};
    ///
    ea::unordered_map<ea::string /*flavor*/, ea::map<ea::string /*EP_**/, Variant>> engineParameters_;


    friend class Project;
    friend class Asset;
};

}
