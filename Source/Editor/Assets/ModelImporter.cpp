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

#include <Urho3D/IO/FileSystem.h>

namespace Urho3D
{

namespace
{

bool IsFileNameGLTF(const ea::string& fileName)
{
    return fileName.ends_with(".gltf", false)
        || fileName.ends_with(".glb", false);
}

bool IsFileNameFBX(const ea::string& fileName)
{
    return fileName.ends_with(".fbx", false);
}

bool IsFileNameBlend(const ea::string& fileName)
{
    return fileName.ends_with(".blend", false);
}

}

void Assets_ModelImporter(Context* context, Project* project)
{
    if (!context->IsReflected<ModelImporter>())
        ModelImporter::RegisterObject(context);
}

ModelImporter::ModelImporter(Context* context)
    : AssetTransformer(context)
{
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
                URHO3D_LOGERROR("Blender is not found, cannot import Blender files. See Settings/Editor/ExternalTools.");
            logged = true;
            return false;
        }

        return true;
    }
    return false;
}

bool ModelImporter::Execute(const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    if (IsFileNameGLTF(input.resourceName_))
    {
        return ImportGLTF(input.inputFileName_, input, output, transformers);
    }
    else if (IsFileNameFBX(input.resourceName_))
    {
        return ImportFBX(input.inputFileName_, input, output, transformers);
    }
    else if (IsFileNameBlend(input.resourceName_))
    {
        return ImportBlend(input.inputFileName_, input, output, transformers);
    }
    return false;
}

bool ModelImporter::ImportGLTF(const ea::string& fileName,
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    settings_.assetName_ = GetFileName(input.originalInputFileName_);
    auto importer = MakeShared<GLTFImporter>(context_, settings_);

    if (!importer->LoadFile(fileName, AddTrailingSlash(input.outputFileName_), AddTrailingSlash(input.resourceName_)))
    {
        URHO3D_LOGERROR("Failed to load asset {} as GLTF model", input.resourceName_);
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

        output.appliedTransformers_.insert(nestedOutput.appliedTransformers_.begin(), nestedOutput.appliedTransformers_.end());
    }
    return true;
}

bool ModelImporter::ImportFBX(const ea::string& fileName,
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto fs = context_->GetSubsystem<FileSystem>();
    const auto toolManager = GetToolManager();

    const ea::string tempGltfFile = input.tempPath_ + "model.glb";
    const StringVector arguments{
        "--binary",
        "--input",
        fileName,
        "--output",
        tempGltfFile
    };

    ea::string commandOutput;
    if (fs->SystemRun(toolManager->GetFBX2glTF(), arguments, commandOutput) != 0)
    {
        URHO3D_LOGERROR("{}", commandOutput);
        return false;
    }

    const bool result = ImportGLTF(tempGltfFile, input, output, transformers);
    fs->Delete(tempGltfFile);
    return result;
}

bool ModelImporter::ImportBlend(const ea::string& fileName,
    const AssetTransformerInput& input, AssetTransformerOutput& output, const AssetTransformerVector& transformers)
{
    auto fs = context_->GetSubsystem<FileSystem>();
    const auto toolManager = GetToolManager();

    const ea::string tempGltfFile = input.tempPath_ + "model.gltf";
    const StringVector arguments{
        "-b",
        fileName,
        "--python-expr",
        Format("import bpy; bpy.ops.export_scene.gltf(filepath='{}', export_format='GLTF_EMBEDDED')", tempGltfFile)
    };

    ea::string commandOutput;
    if (fs->SystemRun(toolManager->GetBlender(), arguments, commandOutput) != 0)
    {
        URHO3D_LOGERROR("{}", commandOutput);
        return false;
    }

    const bool result = ImportGLTF(tempGltfFile, input, output, transformers);
    fs->Delete(tempGltfFile);
    return result;
}

}
