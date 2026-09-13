// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Utility/AssetTransformer.h"

#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/Base64Archive.h"
#include "Urho3D/IO/Log.h"

#include <EASTL/sort.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

AssetTransformerInput::AssetTransformerInput(bool isPostTransform, const ApplicationFlavor& flavor,
    const ea::string& resourceName, const ea::string& inputFileName, FileTime inputFileTime)
    : isPostTransform_(isPostTransform)
    , flavor_(flavor)
    , originalResourceName_(resourceName)
    , originalInputFileName_(inputFileName)
    , resourceName_(resourceName)
    , inputFileName_(inputFileName)
    , inputFileTime_(inputFileTime)
{
}

AssetTransformerInput::AssetTransformerInput(const AssetTransformerInput& other, const ea::string& tempPath,
    const ea::string& outputFileName, const ea::string& outputResourceName)
{
    *this = other;
    tempPath_ = tempPath;
    outputFileName_ = outputFileName;
    outputResourceName_ = outputResourceName;
}

void AssetTransformerInput::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "isPostTransform", isPostTransform_);

    SerializeValue(archive, "flavor", flavor_.components_);
    SerializeValue(archive, "originalResourceName", originalResourceName_);
    SerializeValue(archive, "originalInputFileName", originalInputFileName_);

    SerializeValue(archive, "resourceName", resourceName_);
    SerializeValue(archive, "inputFileName", inputFileName_);
    SerializeValue(archive, "inputFileTime", inputFileTime_);

    SerializeValue(archive, "tempPath", tempPath_);
    SerializeValue(archive, "outputFileName", outputFileName_);
    SerializeValue(archive, "outputResourceName", outputResourceName_);
}

AssetTransformerInput AssetTransformerInput::FromBase64(const ea::string& base64)
{
    Base64InputArchive archive(nullptr, base64);
    AssetTransformerInput result;
    SerializeValue(archive, "input", result);
    return result;
}

ea::string AssetTransformerInput::ToBase64() const
{
    Base64OutputArchive archive(nullptr);
    SerializeValue(archive, "input", const_cast<AssetTransformerInput&>(*this));
    return archive.GetBase64();
}

void AssetTransformerOutput::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "sourceModified", sourceModified_);
    SerializeValue(archive, "outputResourceNames", outputResourceNames_);
    SerializeValue(archive, "modifiedResourceNames", modifiedResourceNames_);
    SerializeValue(archive, "appliedTransformers", appliedTransformers_);
    SerializeValue(archive, "dependencyModificationTimes", dependencyModificationTimes_);
}

AssetTransformerOutput AssetTransformerOutput::FromBase64(const ea::string& base64)
{
    Base64InputArchive archive(nullptr, base64);
    AssetTransformerOutput result;
    SerializeValue(archive, "output", result);
    return result;
}

ea::string AssetTransformerOutput::ToBase64() const
{
    Base64OutputArchive archive(nullptr);
    SerializeValue(archive, "output", const_cast<AssetTransformerOutput&>(*this));
    return archive.GetBase64();
}

AssetTransformer::AssetTransformer(Context* context)
    : Serializable(context)
{
}

bool AssetTransformer::IsApplicable(const AssetTransformerInput& input, const AssetTransformerVector& transformers)
{
    for (AssetTransformer* transformer : transformers)
    {
        if (transformer->IsApplicable(input))
            return true;
    }
    return false;
}

bool AssetTransformer::ExecuteTransformers(const AssetTransformerInput& input, AssetTransformerOutput& output,
    const AssetTransformerVector& transformers, bool isNestedExecution)
{
    URHO3D_ASSERT(!transformers.empty());
    for (AssetTransformer* transformer : transformers)
    {
        if (isNestedExecution && !transformer->IsExecutedOnOutput())
            continue;

        if (transformer->IsApplicable(input))
        {
            if (!transformer->Execute(input, output, transformers))
                return false;
            output.appliedTransformers_.insert(transformer->GetTypeName());
        }
    }
    return true;
}

bool AssetTransformer::ExecuteTransformersAndStore(const AssetTransformerInput& input, const ea::string& outputPath,
    AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    URHO3D_ASSERT(!transformers.empty());
    Context* context = transformers[0]->GetContext();
    auto fs = context->GetSubsystem<FileSystem>();

    const TemporaryDir tempFolderHolder{context, input.tempPath_};
    if (!AssetTransformer::ExecuteTransformers(input, output, transformers, false))
        return false;

    StringVector copiedFiles;
    fs->CopyDir(tempFolderHolder.GetPath(), outputPath, &copiedFiles);

    for (const ea::string& fileName : copiedFiles)
        output.outputResourceNames_.push_back(fileName.substr(outputPath.length()));

    return true;
}

void AssetTransformer::AddDependency(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const ea::string& fileName) const
{
    auto fs = GetSubsystem<FileSystem>();

    const ea::string dataFolder = input.originalInputFileName_.substr(
        0, input.originalInputFileName_.length() - input.originalResourceName_.length());

    if (!fileName.starts_with(dataFolder))
    {
        URHO3D_LOGERROR("Dependency file '{}' is not in the same folder as the input file '{}'", fileName,
            input.originalInputFileName_);
        return;
    }

    const ea::string relativeFileName = fileName.substr(dataFolder.length());
    output.dependencyModificationTimes_[relativeFileName] = fs->GetLastModifiedTime(fileName, true);
}

} // namespace Urho3D
