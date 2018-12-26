//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include "Project.h"
#include "ImportAsset.h"

namespace Urho3D
{

static StringVector subprocessLogMsgBlacklist{
    "ERROR: No texture created, can not set data",  // 2D scenes load texture data. This error is benign.
};

ImportAsset::ImportAsset(Context* context)
    : Object(context)
{

}

bool ImportAsset::Convert(const String& path)
{
    FileSystem* fs = GetFileSystem();

    StringVector args{};
    String interpreter = fs->GetInterpreterFileName();
    String executable = fs->GetProgramFileName();
    if (interpreter != executable)
    {
        // Unixes execute C# applications through interpreter executable but Windows execute C# executables directly.
        args.Push(executable);
        executable = interpreter;
    }

    args.Push({
        "--headless",
        "--nothreads",
        "--log", "error",
        "--log-file", NULL_DEVICE,
        "--converter", GetTypeName(),
        "--converter-input", path,
        GetSubsystem<Project>()->GetProjectPath()
    });

    String output{};
    int result = fs->SystemRun(executable, args, output);

    StringVector lines = output.Split('\n');
    for (const String& line : lines)
    {
        bool blacklisted = false;
        for (const String& blacklistedMsg : subprocessLogMsgBlacklist)
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
            GetLog()->WriteRaw(line, error);
            GetLog()->WriteRaw("\n", error);
        }
    }

    if (result != 0)
        URHO3D_LOGERRORF("Failed Subprocess(%d): %s %s", result, executable.CString(), String::Joined(args, " ").CString());

    return result == 0;
}

}
