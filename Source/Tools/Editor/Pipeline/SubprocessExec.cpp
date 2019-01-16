//
// Copyright (c) 2017-2019 Rokas Kupstys.
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
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONValue.h>
#include "Project.h"
#include "SubprocessExec.h"


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
    URHO3D_ATTRIBUTE("exec", String, executable_, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("output", String, output_, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("args", StringVector, args_, {}, AM_DEFAULT);
}

void SubprocessExec::Execute(const StringVector& input)
{
    auto* project = GetSubsystem<Project>();
    const String& cachePath = project->GetCachePath();
    String editorExecutable = GetFileSystem()->GetProgramFileName();
    String executable = executable_;
    String output, outputRelative;

    auto insertVariables = [&](const String& arg, const String& resourceName=String::EMPTY) {
        return Format(arg,
            fmt::arg("resource_name", resourceName),
            fmt::arg("resource_name_noext", GetPath(resourceName) + GetFileName(resourceName)),
            fmt::arg("resource_path", project->GetResourcePath() + resourceName),
            fmt::arg("project_path", RemoveTrailingSlash(project->GetProjectPath())),
            fmt::arg("cache_path", RemoveTrailingSlash(project->GetCachePath())),
            fmt::arg("editor", editorExecutable),
            fmt::arg("output", output)
        );
    };
    executable = insertVariables(executable);

    if (!IsAbsolutePath(executable))
        executable = GetPath(GetFileSystem()->GetProgramFileName()) + executable;

    for (const String& resourceName : input)
    {
        StringVector outputFiles;
        output = outputRelative = insertVariables(output_, resourceName);
        output = cachePath + output;
        StringVector args = args_;
        // Insert variables to args
        for (String& arg : args)
            arg = insertVariables(arg, resourceName);

        int result = 0;
        String logOutput;
        HashMap<String, unsigned> dirListingBefore;
        if (project->GetPipeline().LockResourcePath(output))
        {
            // Scan output path
            StringVector list;
            if (GetFileSystem()->DirExists(output))
            {
                GetFileSystem()->ScanDir(list, output, "*", SCAN_FILES, true);
                for (const String& path : list)
                    dirListingBefore[path] = GetFileSystem()->GetLastModifiedTime(output + path);
            }

            // Execute converter
            result = GetFileSystem()->SystemRun(executable, args, logOutput);

            if (GetFileSystem()->DirExists(output))
            {
                // Scan output path again
                GetFileSystem()->ScanDir(list, output, "*", SCAN_FILES, true);
                for (const String& path : list)
                {
                    if (!dirListingBefore.Contains(path) ||
                        dirListingBefore[path] < GetFileSystem()->GetLastModifiedTime(output + path))
                        // Record new or changed files
                        outputFiles.EmplaceBack(outputRelative + path);
                }
            }
            else if (GetFileSystem()->FileExists(output))
                outputFiles.EmplaceBack(outputRelative);
        }

        StringVector lines = logOutput.Split('\n');
        for (const String& line : lines)
        {
            if (!line.StartsWith("["))
                // Not a log message.
                continue;

            bool blacklisted = false;
            for (const char* blacklistedMsg : subprocessLogMsgBlacklist)
            {
                if (line.EndsWith(blacklistedMsg))
                {
                    blacklisted = true;
                    break;
                }
            }

            if (!blacklisted)
            {
                bool error = line.Contains("] ERROR: ") || line.StartsWith("ERROR: ");
                Log::WriteRaw(line, error);
                Log::WriteRaw("\n", error);
            }
        }

        if (result != 0)
            URHO3D_LOGERROR("Failed SubprocessExec({}): {} {}", result, executable, String::Joined(args, " "));

        if (!output_.Empty())
            project->GetPipeline().AddCacheEntry(resourceName, outputFiles);

        Converter::Execute(outputFiles);
    }
}

}
