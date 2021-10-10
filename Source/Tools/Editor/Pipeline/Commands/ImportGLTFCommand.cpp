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

#include "Editor.h"
#include "Pipeline/Commands/ImportGLTFCommand.h"

#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Utility/GLTFImporter.h>

namespace Urho3D
{

ImportGLTFCommand::ImportGLTFCommand(Context* context)
    : SubCommand(context)
{
}

void ImportGLTFCommand::RegisterObject(Context* context)
{
    context->RegisterFactory<ImportGLTFCommand>();
}

void ImportGLTFCommand::RegisterCommandLine(CLI::App& cli)
{
    SubCommand::RegisterCommandLine(cli);
    cli.add_option("--input", inputFileName_, "GLTF file name.")->required();
    cli.add_option("--output", outputDirectory_, "Output directory.");
    cli.add_option("--prefix", resourceNamePrefix_, "Common prefix of output resources.");
}

void ImportGLTFCommand::Execute()
{
    auto fs = GetSubsystem<FileSystem>();

    auto importer = MakeShared<GLTFImporter>(context_);
    if (importer->LoadFile(inputFileName_, outputDirectory_, resourceNamePrefix_))
    {
        if (importer->CookResources())
        {
            fs->CreateDirsRecursive(outputDirectory_);
            if (importer->SaveResources())
            {
                return;
            }
        }
    }
    /*auto* project = GetSubsystem<Project>();
    auto* editor = GetSubsystem<Editor>();
    auto* fs = context_->GetSubsystem<FileSystem>();

    if (project == nullptr)
    {
        editor->ErrorExit("ImportGLTFCommand subcommand requires project being loaded.");
        return;
    }

    Scene scene(context_);
    File file(context_);
    if (file.Open(input_, FILE_READ))
    {
        if (scene.LoadXML(file))
        {
            // Remove components that should not be shipped in the final product
            if (auto* component = scene.GetComponent<EditorSceneSettings>())
                component->Remove();

            // Cook scene
            assert(input_.starts_with(project->GetResourcePath()));
            auto resourceName = input_.substr(project->GetResourcePath().length());
            fs->CreateDirsRecursive(GetPath(output_));

            File output(context_);
            if (output.Open(output_, FILE_WRITE))
            {
                if (!scene.Save(output))
                    editor->ErrorExit(Format("Could not convert '{}' to binary version.", input_));
                else
                    fs->SetLastModifiedTime(output_, fs->GetLastModifiedTime(input_));
            }
            else
                editor->ErrorExit(Format("Could not open '{}' for writing.", output_));
        }
        else
            editor->ErrorExit(Format("Could not open load scene '{}'.", input_));
    }
    else
        editor->ErrorExit(Format("Could not open open '{}' for reading.", input_));*/
}

}
