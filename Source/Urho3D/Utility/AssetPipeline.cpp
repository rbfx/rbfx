// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Utility/AssetPipeline.h"

#include "../IO/ArchiveSerialization.h"
#include "../Resource/JSONArchive.h"
#include "../Resource/JSONFile.h"

namespace Urho3D
{

// TODO: Extract common code to Resource

void AssetTransformerDependency::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "Class", class_);
    SerializeValue(archive, "DependsOn", dependsOn_);
}

AssetPipeline::AssetPipeline(Context* context)
    : Resource(context)
{
}

bool AssetPipeline::CheckExtension(const ea::string& fileName)
{
    return fileName.ends_with(".assetpipeline", false) || fileName.ends_with(".AssetPipeline.json", false);
}

void AssetPipeline::AddTransformer(AssetTransformer* transformer)
{
    transformers_.emplace_back(transformer);
}

void AssetPipeline::RemoveTransformer(AssetTransformer* transformer)
{
    const auto iter = ea::find(transformers_.begin(), transformers_.end(), transformer);
    if (iter != transformers_.end())
        transformers_.erase(iter);
}

void AssetPipeline::ReorderTransformer(AssetTransformer* transformer, unsigned index)
{
    if (index >= transformers_.size())
        return;

    const auto iter = ea::find(transformers_.begin(), transformers_.end(), transformer);
    if (iter != transformers_.end())
    {
        transformers_.erase(iter);
        transformers_.insert_at(index, SharedPtr<AssetTransformer>(transformer));
    }
}

void AssetPipeline::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Transformers", transformers_, EmptyObject{},
        [](Archive& archive, const char* name, ea::vector<SharedPtr<AssetTransformer>>& value)
    {
        SerializeVectorAsObjects(archive, name, value, "Transformer",
            [](Archive& archive, const char* name, SharedPtr<AssetTransformer>& value)
        {
            SerializeSharedPtr(archive, name, value, true, true);
        });
    });

    SerializeOptionalValue(archive, "Dependencies", dependencies_);

    ea::erase(transformers_, nullptr);
}

bool AssetPipeline::BeginLoad(Deserializer& source)
{
    JSONFile jsonFile(context_);
    if (jsonFile.Load(source))
        return jsonFile.LoadObject(*this);
    return false;
}

bool AssetPipeline::Save(Serializer& dest) const
{
    JSONFile jsonFile(context_);
    if (jsonFile.SaveObject(*this))
        return jsonFile.Save(dest);
    return false;
}

}
