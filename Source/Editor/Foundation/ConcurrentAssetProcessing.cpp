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

#include "../Foundation/ConcurrentAssetProcessing.h"

#include "../Project/AssetManager.h"

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Resource/JSONFile.h>

namespace Urho3D
{

namespace
{

const ea::string commandName = "ProcessAsset";

void RequestProcessAsset(Project* project, const AssetTransformerInput& input,
    const AssetManager::OnProcessAssetCompleted& callback)
{
    auto context = project->GetContext();
    auto assetManager = project->GetAssetManager();

    auto tempDir = ea::make_shared<TemporaryDir>(project->CreateTemporaryDir());

    // Strip the base path to avoid having spaces in the path passed to the command
    const ea::string projectPath = project->GetProjectPath();
    const ea::string relativeTempPath = tempDir->GetPath().substr(projectPath.size());

    const ea::string inputPath = relativeTempPath + "input.json";
    const ea::string outputPath = relativeTempPath + "output.json";

    JSONFile inputFile(context);
    if (!inputFile.SaveObject("input", input))
    {
        callback(input, ea::nullopt, "Cannot serialize input pipe");
        return;
    }

    if (!inputFile.SaveFile(projectPath + inputPath))
    {
        callback(input, ea::nullopt, "Cannot save input pipe");
        return;
    }

    const ea::string command = Format("{} {} {}", commandName, inputPath, outputPath);
    project->ExecuteRemoteCommandAsync(command,
        [=, tempDir2 = tempDir](bool success, const ea::string& commandOutput)
    {
        if (!success)
        {
            callback(input, ea::nullopt, commandOutput);
            return;
        }

        JSONFile outputFile(context);
        if (!outputFile.LoadFile(projectPath + outputPath))
        {
            callback(input, ea::nullopt, "Cannot load output pipe");
            return;
        }

        AssetTransformerOutput output;
        if (!outputFile.LoadObject("output", output))
        {
            callback(input, ea::nullopt, "Cannot deserialize output pipe");
            return;
        }

        callback(input, output, commandOutput);
    });
}

bool ProcessAsset(Project* project, const ea::string& inputName, const ea::string& outputName)
{
    auto context = project->GetContext();
    auto assetManager = project->GetAssetManager();

    const ea::string& projectPath = project->GetProjectPath();

    JSONFile inputFile(context);
    if (!inputFile.LoadFile(projectPath + inputName))
    {
        URHO3D_LOGERROR("Cannot load input pipe");
        return false;
    }

    AssetTransformerInput input;
    if (!inputFile.LoadObject("input", input))
    {
        URHO3D_LOGERROR("Cannot deserialize input pipe");
        return false;
    }

    bool success = false;
    assetManager->ProcessAsset(input,
        [&](const AssetTransformerInput& input,
            const ea::optional<AssetTransformerOutput>& output, const ea::string& error)
    {
        // Error should be logged by the asset manager
        if (!output)
            return;

        JSONFile outputFile(project->GetContext());
        if (!outputFile.SaveObject("output", *output))
        {
            URHO3D_LOGERROR("Cannot serialize output pipe");
            return;
        }

        if (!outputFile.SaveFile(projectPath + outputName))
        {
            URHO3D_LOGERROR("Cannot save output pipe");
            return;
        }

        success = true;
    });

    return success;
}

}

void Foundation_ConcurrentAssetProcessing(Context* context, Project* project)
{
    auto assetManager = project->GetAssetManager();

    project->OnCommand.Subscribe(project,
        [=](const ea::string& command, const ea::string& args, bool& processed)
    {
        const StringVector argsVector = args.split(' ');
        if (command != commandName || argsVector.size() != 2)
            return;

        const ea::string& inputName = argsVector[0];
        const ea::string& outputName = argsVector[1];

        if (ProcessAsset(project, inputName, outputName))
            processed = true;
    });

    if (!project->GetFlags().Test(ProjectFlag::SingleProcess))
    {
        using CompletionCallback = AssetManager::OnProcessAssetCompleted;
        const auto callback = [=](const AssetTransformerInput& input, const CompletionCallback& callback) //
        { //
            RequestProcessAsset(project, input, callback);
        };

        assetManager->SetProcessCallback(callback, GetNumLogicalCPUs());
    }
}

}
