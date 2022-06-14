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

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONArchive.h>
#include <Urho3D/Resource/JSONFile.h>

namespace Urho3D
{

const ea::string AssetManager::ResourceName{"AssetPipeline.json"};

AssetManager::AssetManager(Context* context)
    : Object(context)
    , projectEditor_(GetSubsystem<ProjectEditor>())
    , hierarchy_(MakeShared<AssetTransformerHierarchy>(context_))
{
}

AssetManager::~AssetManager()
{
}

void AssetManager::Update()
{
    // TODO(editor): be smart
    if (reloadPipelines_)
    {
        reloadPipelines_ = false;
        ReloadPipelines();
    }

    if (processAssets_)
    {
        processAssets_ = false;
        ProcessAssets();
    }
}

void AssetManager::ReloadPipelines()
{
    hierarchy_->Clear();

    for (const ea::string& relativePath : EnumeratePipelineFiles())
    {
        auto jsonFile = MakeShared<JSONFile>(context_);
        if (!jsonFile->LoadFile(projectEditor_->GetDataPath() + relativePath))
        {
            URHO3D_LOGERROR("Failed to load {} as JSON file", relativePath);
            continue;
        }

        const JSONValue& rootElement = jsonFile->GetRoot();
        const JSONValue& transformersElement = rootElement["Transformers"];
        const JSONValue& dependenciesElement = rootElement["Dependencies"];

        for (const JSONValue& value : transformersElement.GetArray())
        {
            const ea::string& transformerClass = value["_Class"].GetString();
            const auto newTransformer = DynamicCast<AssetTransformer>(context_->CreateObject(transformerClass));
            if (!newTransformer)
            {
                URHO3D_LOGERROR("Failed to instantiate transformer {} of JSON file {}", transformerClass, relativePath);
                continue;
            }

            JSONInputArchive archive{context_, value, jsonFile};
            const bool loaded = ConsumeArchiveException([&]
            {
                SerializeValue(archive, transformerClass.c_str(), *newTransformer);
            });

            if (loaded)
                hierarchy_->AddTransformer(GetPath(relativePath), newTransformer);
        }

        for (const JSONValue& value : dependenciesElement.GetArray())
        {
            const ea::string& transformerClass = value["Class"].GetString();
            const ea::string& dependsOn = value["DependsOn"].GetString();
            hierarchy_->AddDependency(transformerClass, dependsOn);
        }

        jsonFile = nullptr;
    }

    hierarchy_->CommitDependencies();
}

void AssetManager::ProcessAssets()
{
    for (const ea::string& resourceName : EnumerateAssetFiles())
    {
        const AssetTransformerVector transformers = hierarchy_->GetTransformerCandidates(resourceName, defaultFlavor_);

        AssetTransformerContext ctx{context_};
        ctx.resourceName_ = resourceName;
        ctx.fileName_ = projectEditor_->GetDataPath() + resourceName;
        ctx.cacheFileName_ = projectEditor_->GetCachePath() + resourceName;
        if (AssetTransformer::Execute(ctx, transformers))
            URHO3D_LOGDEBUG("Asset {} was processed", resourceName);
    }
}

StringVector AssetManager::EnumeratePipelineFiles() const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector result;
    fs->ScanDir(result, projectEditor_->GetDataPath(), "*.json", SCAN_FILES, true);

    const ea::string suffix = Format("/{}", ResourceName);
    ea::erase_if(result, [&](const ea::string& relativePath) { return !relativePath.ends_with(suffix); });

    return result;
}

StringVector AssetManager::EnumerateAssetFiles() const
{
    auto fs = GetSubsystem<FileSystem>();

    StringVector result;
    fs->ScanDir(result, projectEditor_->GetDataPath(), "", SCAN_FILES, true);
    return result;
}

}
