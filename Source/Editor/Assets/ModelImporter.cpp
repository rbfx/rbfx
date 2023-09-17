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

#include "../Assets/ModelImporter.h"

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/JSONFile.h>
#if URHO3D_GLOW
    #include <Urho3D/Glow/LightmapUVGenerator.h>
#endif

namespace Urho3D
{

namespace
{

const ea::string DefaultSkipTag = "[skip]";

bool IsFileNameGLTF(const ea::string& fileName, bool strict = true)
{
    if (!strict && (fileName.ends_with(".gltf_", false) || fileName.ends_with(".glb_", false)))
        return true;

    return fileName.ends_with(".gltf", false) || fileName.ends_with(".glb", false);
}

bool IsFileNameFBX(const ea::string& fileName, bool strict = true)
{
    if (!strict && fileName.ends_with(".fbx_", false))
        return true;

    return fileName.ends_with(".fbx", false);
}

bool IsFileNameBlend(const ea::string& fileName, bool strict = true)
{
    if (!strict && fileName.ends_with(".blend_", false))
        return true;

    return fileName.ends_with(".blend", false);
}

} // namespace

void Assets_ModelImporter(Context* context, Project* project)
{
    if (!context->IsReflected<ModelImporter>())
        ModelImporter::RegisterObject(context);
}

void ModelImporter::ModelMetadata::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "appendFiles", appendFiles_);
    SerializeValue(archive, "nodeRenames", nodeRenames_);
}

ModelImporter::ModelImporter(Context* context)
    : AssetTransformer(context)
{
    settings_.skipTag_ = DefaultSkipTag;
}

void ModelImporter::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ModelImporter>(Category_Transformer);

    URHO3D_ATTRIBUTE("Mirror X", bool, settings_.mirrorX_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Scale", float, settings_.scale_, 1.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Rotation", Quaternion, settings_.rotation_, Quaternion::IDENTITY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Cleanup Bone Names", bool, settings_.cleanupBoneNames_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Cleanup Root Nodes", bool, settings_.cleanupRootNodes_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Combine LODs", bool, settings_.combineLODs_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Repair Looping", bool, settings_.repairLooping_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Skip Tag", ea::string, settings_.skipTag_, DefaultSkipTag, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Keep Names On Merge", bool, settings_.keepNamesOnMerge_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Add Empty Nodes To Skeleton", bool, settings_.addEmptyNodesToSkeleton_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Blender: Apply Modifiers", bool, blenderApplyModifiers_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("LightMap UV: Generate", bool, lightmapUVGenerate_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("LightMap UV: Texels per Unit", float, lightmapUVTexelsPerUnit_, 10.0f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("LightMap UV: Channel", unsigned, lightmapUVChannel_, 1, AM_DEFAULT);
}

ToolManager* ModelImporter::GetToolManager() const
{
    auto project = GetSubsystem<Project>();
    return project->GetToolManager();
}

bool ModelImporter::IsApplicable(const AssetTransformerInput& input)
{
    const auto toolManager = GetToolManager();

    if (IsFileNameGLTF(input.resourceName_))
        return true;

    if (IsFileNameFBX(input.resourceName_))
    {
        if (!toolManager->HasFBX2glTF())
        {
            static bool logged = false;
            if (!logged)
                URHO3D_LOGERROR("FBX2glTF is not found, cannot import FBX files. See Settings/Editor/ExternalTools.");
            logged = true;
            return false;
        }

        return true;
    }

    if (IsFileNameBlend(input.resourceName_))
    {
        if (!toolManager->HasBlender())
        {
            static bool logged = false;
            if (!logged)
                URHO3D_LOGERROR(
                    "Blender is not found, cannot import Blender files. See Settings/Editor/ExternalTools.");
            logged = true;
            return false;
        }

        return true;
    }
    return false;
}

bool ModelImporter::Execute(
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    const ModelMetadata metadata = LoadMetadata(input.inputFileName_);
    const GLTFFileHandle handle = LoadData(input.inputFileName_, input.tempPath_);

    if (!handle)
        return false;

    return ImportGLTF(handle, metadata, input, output, transformers);
}

bool ModelImporter::ImportGLTF(GLTFFileHandle fileHandle, const ModelMetadata& metadata,
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    if (!metadata.metadataFileName_.empty())
        AddDependency(input, output, metadata.metadataFileName_);

    settings_.assetName_ = GetFileName(input.originalInputFileName_);
    settings_.nodeRenames_ = metadata.nodeRenames_;
    auto importer = MakeShared<GLTFImporter>(context_, settings_);

    const ea::string outputPath = AddTrailingSlash(input.outputFileName_);
    const ea::string resourceNamePrefix = AddTrailingSlash(input.resourceName_);
    if (!importer->LoadFile(fileHandle->fileName_))
    {
        URHO3D_LOGERROR("Failed to load asset {} as GLTF model", input.resourceName_);
        return false;
    }

    for (const ea::string& secondaryFileName : metadata.appendFiles_)
    {
        const ea::string secondaryFilePath = GetPath(input.originalInputFileName_) + secondaryFileName;
        const GLTFFileHandle secondaryFileHandle = LoadData(secondaryFilePath, input.tempPath_);
        if (!secondaryFileHandle)
        {
            URHO3D_LOGWARNING("Failed to load secondary file {} for asset {}", secondaryFilePath, input.resourceName_);
            continue;
        }

        AddDependency(input, output, secondaryFilePath);

        if (!importer->MergeFile(secondaryFileHandle->fileName_, GetFileName(secondaryFilePath)))
        {
            URHO3D_LOGWARNING(
                "Failed to merge secondary file {} into asset {}", secondaryFilePath, input.resourceName_);
            continue;
        }
    }

    if (!importer->Process(outputPath, resourceNamePrefix, this))
    {
        URHO3D_LOGERROR("Failed to process asset {}", input.resourceName_);
        return false;
    }

    auto fs = GetSubsystem<FileSystem>();
    fs->RemoveDir(input.outputFileName_, true);
    fs->Delete(input.outputFileName_);

    if (!importer->SaveResources())
    {
        URHO3D_LOGERROR("Failed to save output files for asset {}", input.resourceName_);
        return false;
    }

    for (const auto& [resourceName, fileName] : importer->GetSavedResources())
    {
        AssetTransformerOutput nestedOutput;
        AssetTransformerInput nestedInput = input;
        nestedInput.resourceName_ = resourceName;
        nestedInput.inputFileName_ = fileName;
        nestedInput.outputFileName_ = fileName;
        if (!AssetTransformer::ExecuteTransformers(nestedInput, nestedOutput, transformers, true))
        {
            URHO3D_LOGERROR("Failed to apply nested transformer for asset {}", resourceName);
            return false;
        }

        output.appliedTransformers_.insert(
            nestedOutput.appliedTransformers_.begin(), nestedOutput.appliedTransformers_.end());
    }
    return true;
}

void ModelImporter::OnModelLoaded(ModelView& modelView)
{
    if (lightmapUVGenerate_)
    {
#if URHO3D_GLOW
        LightmapUVGenerationSettings settings;
        settings.texelPerUnit_ = lightmapUVTexelsPerUnit_;
        settings.uvChannel_ = lightmapUVChannel_;
        if (!GenerateLightmapUV(modelView, settings))
            throw RuntimeException("Failed to generate lightmap UVs");
#else
        throw RuntimeException("Glow must be enabled to generate lightmap UVs");
#endif
    }
}

ModelImporter::ModelMetadata ModelImporter::LoadMetadata(const ea::string& fileName) const
{
    ModelMetadata result;
    result.metadataFileName_ = fileName + ".import";

    JSONFile file{context_};
    if (file.LoadFile(result.metadataFileName_))
    {
        if (file.LoadObject("metadata", result))
            return result;
    }

    return {};
}

ModelImporter::GLTFFileHandle ModelImporter::LoadData(const ea::string& fileName, const ea::string& tempPath) const
{
    if (IsFileNameGLTF(fileName, false))
    {
        return LoadDataNative(fileName);
    }
    else if (IsFileNameFBX(fileName, false))
    {
        return LoadDataFromFBX(fileName, tempPath);
    }
    else if (IsFileNameBlend(fileName, false))
    {
        return LoadDataFromBlend(fileName, tempPath);
    }
    return nullptr;
}

ModelImporter::GLTFFileHandle ModelImporter::LoadDataNative(const ea::string& fileName) const
{
    auto fs = context_->GetSubsystem<FileSystem>();

    if (!fs->FileExists(fileName))
        return nullptr;

    return ea::make_shared<GLTFFileInfo>(GLTFFileInfo{fileName});
}

ModelImporter::GLTFFileHandle ModelImporter::LoadDataFromFBX(
    const ea::string& fileName, const ea::string& tempPath) const
{
    auto fs = context_->GetSubsystem<FileSystem>();
    const auto toolManager = GetToolManager();

    if (!fs->FileExists(fileName))
        return nullptr;

    const ea::string tempGltfFile = Format("{}{}.glb", tempPath, GenerateUUID());
    const StringVector arguments{"--binary", "--input", fileName, "--output", tempGltfFile};

    ea::string commandOutput;
    if (fs->SystemRun(toolManager->GetFBX2glTF(), arguments, commandOutput) != 0)
    {
        URHO3D_LOGERROR("{}", commandOutput);
        return nullptr;
    }

    const auto deleter = [fs](GLTFFileInfo* handle)
    {
        fs->Delete(handle->fileName_);
        delete handle;
    };
    return ea::shared_ptr<GLTFFileInfo>(new GLTFFileInfo{tempGltfFile}, deleter);
}

ModelImporter::GLTFFileHandle ModelImporter::LoadDataFromBlend(
    const ea::string& fileName, const ea::string& tempPath) const
{
    auto fs = context_->GetSubsystem<FileSystem>();
    const auto toolManager = GetToolManager();

    if (!fs->FileExists(fileName))
        return nullptr;

    const ea::string tempGltfFile = tempPath + "model.gltf";

    // This script is passed as command line argument, so it must be a single line and use single quotes
    const ea::string script = Format(
        "import bpy;"
        "bpy.ops.export_scene.gltf("
        "  filepath='{}', "
        "  export_format='GLTF_EMBEDDED', "
        "  export_apply={}"
        ");",
        tempGltfFile, blenderApplyModifiers_ ? "True" : "False");

    const StringVector arguments{"-b", fileName, "--python-expr", script};

    ea::string commandOutput;
    if (fs->SystemRun(toolManager->GetBlender(), arguments, commandOutput) != 0)
    {
        URHO3D_LOGERROR("{}", commandOutput);
        return nullptr;
    }

    const auto deleter = [fs](GLTFFileInfo* handle)
    {
        fs->Delete(handle->fileName_);
        delete handle;
    };
    return ea::shared_ptr<GLTFFileInfo>(new GLTFFileInfo{tempGltfFile}, deleter);
}

} // namespace Urho3D
