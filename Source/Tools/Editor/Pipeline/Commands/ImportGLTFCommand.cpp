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
#include <Urho3D/Resource/JSONArchive.h>
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
    cli.add_option("--settings", settingsString_, "JSON line with settings.");
}

void ImportGLTFCommand::Execute()
{
    auto fs = GetSubsystem<FileSystem>();

    GLTFImporterSettings settings;
    if (!settingsString_.empty())
    {
        settingsString_.replace('\'', '"');
        auto jsonFile = MakeShared<JSONFile>(context_);
        if (!jsonFile->FromString(settingsString_))
            return;

        JSONInputArchive archive(jsonFile);
        SerializeValue(archive, "settings", settings);
    }

    auto importer = MakeShared<GLTFImporter>(context_, settings);
    if (importer->LoadFile(inputFileName_, outputDirectory_, resourceNamePrefix_))
    {
        fs->CreateDirsRecursive(outputDirectory_);
        if (importer->SaveResources())
        {
            return;
        }
    }
}

}
