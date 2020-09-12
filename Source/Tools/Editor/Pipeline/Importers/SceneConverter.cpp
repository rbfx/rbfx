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

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Core/ProcessUtils.h>

#include "Project.h"
#include "Pipeline/Asset.h"
#include "Pipeline/Importers/SceneConverter.h"
#include "Tabs/Scene/EditorSceneSettings.h"

namespace Urho3D
{

SceneConverter::SceneConverter(Context* context)
    : AssetImporter(context)
{
    // Binary scenes are used for shipping only.
    flags_ = AssetImporterFlag::IsOptional | AssetImporterFlag::IsRemapped;
}

void SceneConverter::RegisterObject(Context* context)
{
    context->RegisterFactory<SceneConverter>();
    URHO3D_COPY_BASE_ATTRIBUTES(AssetImporter);
}
bool SceneConverter::Execute(Urho3D::Asset* input, const ea::string& outputPath)
{
    if (!BaseClassName::Execute(input, outputPath))
        return false;

    auto* fs = context_->GetSubsystem<FileSystem>();
    auto* project = GetSubsystem<Project>();

    // A subproces is used to cook a scene because resource loading is reserved to a main thread, but asset importers run in worker threads.

    ea::string output;
    ea::string outputFile = outputPath + GetPath(input->GetName()) + GetFileName(input->GetName()) + ".bin";

    StringVector arguments{project->GetProjectPath(), "CookScene", "--input", input->GetResourcePath(), "--output", outputFile};
    ea::string program;
#if URHO3D_CSHARP && !_WIN32
    // Editor executable is a C# program interpreted by .net runtime.
    program = fs->GetInterpreterFileName();
    arguments.push_front(fs->GetProgramFileName());
#else
    program = fs->GetProgramFileName();
#endif
    int result = fs->SystemRun(program, arguments, output);
    if (result != 0)
    {
        URHO3D_LOGERROR("Converting '{}' to '{}' failed.", input->GetResourcePath(), outputFile);
        if (!output.empty())
            URHO3D_LOGERROR(output);
        return false;
    }
    else
        URHO3D_LOGINFO("Converted '{}' to '{}'.", input->GetResourcePath(), outputFile);

    AddByproduct(outputFile);
    return true;
}

bool SceneConverter::Accepts(const ea::string& path) const
{
    if (!path.ends_with(".xml") && !path.ends_with(".scene"))
        return false;
    return GetContentType(context_, path) == CTYPE_SCENE;
}

}
