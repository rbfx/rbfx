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

#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include "Project.h"
#include "ImportAssimp.h"


namespace Urho3D
{

ImportAssimp::ImportAssimp(Context* context)
    : ImportAsset(context)
{
}

bool ImportAssimp::Accepts(const String& path)
{
    String extension = GetExtension(path);
    return extension == ".fbx" || extension == ".blend";
}

bool ImportAssimp::Convert(const String& path)
{
    bool importedAny = false;
    auto* project = GetSubsystem<Project>();
    assert(path.StartsWith(project->GetResourcePath()));

    const auto& cachePath = project->GetCachePath();
    auto resourceName = path.Substring(project->GetResourcePath().Length());
    auto resourceFileName = GetFileName(path);
    auto outputDir = cachePath + AddTrailingSlash(resourceName);
    GetFileSystem()->CreateDirsRecursive(outputDir);

    // Import models
    {
        String outputPath = outputDir + resourceFileName + ".mdl";

        StringVector args{"model", path, outputPath, "-na", "-ns"};
        Process process(GetFileSystem()->GetProgramDir() + "AssetImporter", args);
        if (process.Run() == 0 && GetFileSystem()->FileExists(outputPath))
            importedAny = true;
    }

    // Import animations
    {
        String outputPath = cachePath + resourceName;

        StringVector args{"anim", path, outputPath, "-nm", "-nt", "-nc", "-ns"};
        Process process(GetFileSystem()->GetProgramDir() + "AssetImporter", args);
        if (process.Run() == 0 && GetFileSystem()->FileExists(outputPath))
            importedAny = true;
    }

    return importedAny;
}

}
