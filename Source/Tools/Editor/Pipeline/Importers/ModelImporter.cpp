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

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Core/ProcessUtils.h>

#include "Project.h"
#include "Pipeline/Asset.h"
#include "Pipeline/Importers/ModelImporter.h"

namespace Urho3D
{

static const char* MODEL_IMPORTER_OUTPUT_ANIM = "Output animations";
static const char* MODEL_IMPORTER_OUTPUT_MAT = "Output materials";
static const char* MODEL_IMPORTER_OUTPUT_MAT_TEX = "Output material textures";
static const char* MODEL_IMPORTER_USE_MAT_DIFFUSE = "Use material diffuse color";
static const char* MODEL_IMPORTER_FIX_INFACING_NORMALS = "Fix in-facing normals";
static const char* MODEL_IMPORTER_MAX_BONES = "Max number of bones";
static const char* MODEL_IMPORTER_ANIM_TICK = "Animation tick frequency";
static const char* MODEL_IMPORTER_EMISSIVE_AO = "Emissive is ambient occlusion";
static const char* MODEL_IMPORTER_FBX_PIVOT = "Suppress $fbx pivot nodes";

ModelImporter::ModelImporter(Context* context)
    : AssetImporter(context)
{
}

void ModelImporter::RegisterObject(Context* context)
{
    context->RegisterFactory<ModelImporter>();
    URHO3D_COPY_BASE_ATTRIBUTES(AssetImporter);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_OUTPUT_ANIM, bool, outputAnimations_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_OUTPUT_MAT, bool, outputMaterials_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_OUTPUT_MAT_TEX, bool, outputMaterialTextures_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_USE_MAT_DIFFUSE, bool, useMaterialDiffuse_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_FIX_INFACING_NORMALS, bool, fixInFacingNormals_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_MAX_BONES, int, maxBones_, 64, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_ANIM_TICK, int, animationTick_, 4800, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_EMISSIVE_AO, bool, emissiveIsAmbientOcclusion_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE(MODEL_IMPORTER_FBX_PIVOT, bool, noFbxPivot_, false, AM_DEFAULT);
}

void ModelImporter::RenderInspector(const char* filter)
{
    BaseClassName::RenderInspector(filter);
}

bool ModelImporter::Execute(Urho3D::Asset* input, const ea::string& inputFile, const ea::string& outputPath)
{
    auto* fs = GetFileSystem();
    auto* project = GetSubsystem<Project>();

    ea::string tempPath = project->GetProjectPath() + "Temp." + GenerateUUID() + "/";
    // Models go into "{resource_name}/" subfolder in cache directory.
    ea::string outputDir = outputPath + GetFileNameAndExtension(inputFile) + "/";
    fs->CreateDirsRecursive(outputDir);

    ea::string output = tempPath + "Model.mdl";
    ea::vector<ea::string> args{"model", input->GetResourcePath(), output};

    if (!GetAttribute(MODEL_IMPORTER_OUTPUT_ANIM).GetBool())
        args.emplace_back("-na");

    if (!GetAttribute(MODEL_IMPORTER_OUTPUT_MAT).GetBool())
        args.emplace_back("-nm");

    if (!GetAttribute(MODEL_IMPORTER_OUTPUT_MAT_TEX).GetBool())
        args.emplace_back("-nt");

    if (!GetAttribute(MODEL_IMPORTER_USE_MAT_DIFFUSE).GetBool())
        args.emplace_back("-nc");

    if (!GetAttribute(MODEL_IMPORTER_FIX_INFACING_NORMALS).GetBool())
        args.emplace_back("-nf");

    args.emplace_back("-mb");
    args.emplace_back(ea::to_string(GetAttribute(MODEL_IMPORTER_MAX_BONES).GetBool()));

    args.emplace_back("-f");
    args.emplace_back(ea::to_string(GetAttribute(MODEL_IMPORTER_ANIM_TICK).GetInt()));

    if (!GetAttribute(MODEL_IMPORTER_EMISSIVE_AO).GetBool())
        args.emplace_back("-eao");

    if (!GetAttribute(MODEL_IMPORTER_FBX_PIVOT).GetBool())
        args.emplace_back("-np");

    int result = fs->SystemRun(fs->GetProgramDir() + "AssetImporter", args);

    if (result != 0)
    {
        URHO3D_LOGERROR("Importing asset '{}' failed.", input->GetName());
        return false;
    }

    ea::string byproductNameStart = input->GetName() + "/";
    unsigned mtime = fs->GetLastModifiedTime(input->GetResourcePath());

    ClearByproducts();

    StringVector tmpByproducts;
    fs->ScanDir(tmpByproducts, tempPath, "*.*", SCAN_FILES, true);
    tmpByproducts.erase_first(".");
    tmpByproducts.erase_first("..");

    for (const ea::string& byproduct : tmpByproducts)
    {
        fs->SetLastModifiedTime(tempPath + byproduct, mtime);
        ea::string moveTo = outputDir + byproduct;
        if (fs->FileExists(moveTo))
            fs->Delete(moveTo);
        else if (fs->DirExists(moveTo))
            fs->RemoveDir(moveTo, true);
        fs->CreateDirsRecursive(GetPath(moveTo));
        fs->Rename(tempPath + byproduct, moveTo);
        fs->SetLastModifiedTime(moveTo, mtime);
        AddByproduct(byproductNameStart + byproduct);
    }

    fs->RemoveDir(tempPath, true);
    return true;
}

bool ModelImporter::Accepts(const ea::string& path) const
{
    if (path.ends_with(".fbx"))
        return true;
    if (path.ends_with(".blend"))
        return true;
    return false;
}

}
