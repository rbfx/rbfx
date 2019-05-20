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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONValue.h>
#include "Project.h"
#include "SubprocessExec.h"
#include "GlobResources.h"


namespace Urho3D
{

static const char* subprocessLogMsgBlacklist[] = {
    "ERROR: No texture created, can not set data",  // 2D scenes load texture data. This error is benign.
};

SubprocessExec::SubprocessExec(Context* context)
    : Converter(context)
{
}

void SubprocessExec::RegisterObject(Context* context)
{
    context->RegisterFactory<SubprocessExec>();
    URHO3D_COPY_BASE_ATTRIBUTES(Converter);
    URHO3D_ATTRIBUTE("exec", ea::string, executable_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("args", StringVector, args_, {}, AM_DEFAULT);
    URHO3D_ATTRIBUTE("output", ea::string, output_, EMPTY_STRING, AM_DEFAULT);
    URHO3D_ATTRIBUTE("reschedule", StringVector, reschedule_, {}, AM_DEFAULT);
}

void SubprocessExec::Execute(const StringVector& input)
{
    auto* project = GetSubsystem<Project>();
    auto* fs = GetFileSystem();
    const ea::string& cachePath = project->GetCachePath();
    ea::string editorExecutable = fs->GetProgramFileName();
    ea::string executable = executable_;
    ea::string output, outputRelative;

    auto insertVariables = [&](const ea::string& arg, const ea::string& resourceName=EMPTY_STRING) {
        ea::string resourcePath = project->GetResourcePath() + resourceName;
        if (!fs->Exists(resourcePath))
            resourcePath = project->GetCachePath() + resourceName;

        return Format(arg,
            fmt::arg("resource_name", resourceName),
            fmt::arg("resource_name_noext", GetParentPath(resourceName) + GetFileName(resourceName)),
            fmt::arg("resource_path", resourcePath),
            fmt::arg("project_path", RemoveTrailingSlash(project->GetProjectPath())),
            fmt::arg("cache_path", RemoveTrailingSlash(project->GetCachePath())),
            fmt::arg("editor", editorExecutable),
            fmt::arg("output", output)
        );
    };
    executable = insertVariables(executable);

    if (!IsAbsolutePath(executable))
        executable = GetPath(fs->GetProgramFileName()) + executable;

    for (const ea::string& resourceName : input)
    {
        StringVector outputFiles;
        output = outputRelative = insertVariables(output_, resourceName);
        output = cachePath + output;
        StringVector args = args_;
        // Insert variables to args
        for (ea::string& arg : args)
            arg = insertVariables(arg, resourceName);

        int result = 0;
        ea::string logOutput;
        ea::unordered_map<ea::string, unsigned> dirListingBefore;
        if (project->GetPipeline().LockResourcePath(output))
        {
            // Scan output path
            StringVector list;
            if (output.ends_with("/"))
            {
                if (fs->DirExists(output))
                {
                    fs->ScanDir(list, output, "*", SCAN_FILES, true);
                    for (const ea::string& path : list)
                        dirListingBefore[path] = fs->GetLastModifiedTime(output + path);
                }
                else
                    fs->CreateDirsRecursive(output);
            }
            else
            {
                ea::string outputDir = GetParentPath(output);
                if (!fs->DirExists(outputDir))
                    fs->CreateDirsRecursive(outputDir);
            }

            // Execute converter
            result = fs->SystemRun(executable, args, logOutput);

            if (fs->DirExists(output))
            {
                // Scan output path again
                fs->ScanDir(list, output, "*", SCAN_FILES, true);
                for (const ea::string& path : list)
                {
                    if (!dirListingBefore.contains(path) ||
                        dirListingBefore[path] < fs->GetLastModifiedTime(output + path))
                        // Record new or changed files
                        outputFiles.emplace_back(outputRelative + path);
                }
            }
            else if (fs->FileExists(output))
                outputFiles.emplace_back(outputRelative);
        }

        auto logger = Log::GetLogger("pipeline");

        StringVector lines = logOutput.split('\n');
        for (const ea::string& line : lines)
        {
            // Likely printing a progress. TODO: what of MacOS?
            if (line.starts_with("\b") || line.ends_with("\r"))
                continue;

            bool blacklisted = false;
            for (const char* blacklistedMsg : subprocessLogMsgBlacklist)
            {
                if (line.ends_with(blacklistedMsg))
                {
                    blacklisted = true;
                    break;
                }
            }

            if (!blacklisted)
                logger.Info(line);
        }

        if (result != 0)
            logger.Error("Failed SubprocessExec({}): {} {}", result, executable, ea::string::joined(args, " "));

        if (!output_.empty())
            project->GetPipeline().AddCacheEntry(resourceName, outputFiles);

        if (!reschedule_.empty())
        {
            ea::vector<std::regex> reschedulePatterns;
            for (const ea::string& glob : reschedule_)
                reschedulePatterns.push_back(GlobToRegex(insertVariables(glob, resourceName)));

            // In some cases processing file may produce extra files that should be once again processed by the pipeline.
            // For example fbx model may contain embedded textures which get extracted to Cache folder upon conversion.
            // We want those textures to be compressed to hardware-supported format.
            for (const ea::string& outputFile : outputFiles)
            {
                if (MatchesAny(outputFile, reschedulePatterns))
                    project->GetPipeline().Reschedule(outputFile);
            }
        }

        Converter::Execute(outputFiles);
    }
}

}
