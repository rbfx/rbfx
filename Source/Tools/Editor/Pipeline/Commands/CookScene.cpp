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

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Toolbox/IO/ContentUtilities.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/File.h>
#include "Tabs/Scene/EditorSceneSettings.h"
#include "Editor.h"
#include "Project.h"
#include "CookScene.h"


namespace Urho3D
{

CookScene::CookScene(Context* context)
    : SubCommand(context)
{
}

void CookScene::RegisterObject(Context* context)
{
    context->RegisterFactory<CookScene>();
}

void CookScene::RegisterCommandLine(CLI::App& cli)
{
    cli.add_option("--input", input_, "XML scene file.")->required();
    cli.add_option("--output", output_, "Resulting binary scene file.");
    cli.set_callback([this]() {
        GetSubsystem<Editor>()->GetEngineParameters()[EP_HEADLESS] = true;
    });
}

void CookScene::Execute()
{
    auto* project = GetSubsystem<Project>();
    if (project == nullptr)
    {
        GetSubsystem<Editor>()->ErrorExit("CookScene subcommand requires project being loaded.");
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
            GetFileSystem()->CreateDirsRecursive(GetPath(output_));

            File output(context_);
            if (output.Open(output_, FILE_WRITE))
            {
                if (!scene.Save(output))
                    GetSubsystem<Editor>()->ErrorExit(Format("Could not convert '%s' to binary version.", input_.c_str()));
            }
            else
                GetSubsystem<Editor>()->ErrorExit(Format("Could not open '%s' for writing.", output_.c_str()));
        }
        else
            GetSubsystem<Editor>()->ErrorExit(Format("Could not open load scene '%s'.", input_.c_str()));
    }
    else
        GetSubsystem<Editor>()->ErrorExit(Format("Could not open open '%s' for reading.", input_.c_str()));
}

}
