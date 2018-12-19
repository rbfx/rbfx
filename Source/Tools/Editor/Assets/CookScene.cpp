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
#include <Toolbox/IO/ContentUtilities.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/IO/File.h>
#include "Project.h"
#include "CookScene.h"


namespace Urho3D
{

CookScene::CookScene(Context* context)
    : ImportAsset(context)
{
}

bool CookScene::Accepts(const String& path, ContentType type)
{
    if (GetExtension(path) != ".xml")
        return false;
    return type == CTYPE_SCENE;
}

bool CookScene::RunConverter(const String& path)
{
    Scene scene(context_);
    File file(context_);
    if (file.Open(path, FILE_READ))
    {
        if (scene.LoadXML(file))
        {
            auto* project = GetSubsystem<Project>();
            assert(path.StartsWith(project->GetResourcePath()));
            auto resourceName = path.Substring(project->GetResourcePath().Length());
            auto outputFile = project->GetCachePath() + ReplaceExtension(resourceName, ".bin");
            GetFileSystem()->CreateDirsRecursive(GetPath(outputFile));

            File output(context_);
            if (output.Open(outputFile, FILE_WRITE))
            {
                if (scene.Save(output))
                    return true;
                else
                    URHO3D_LOGERRORF("Could not convert '%s' to binary version.", path.CString());
            }
            else
                URHO3D_LOGERRORF("Could not open '%s' for writing.", outputFile.CString());
        }
        else
            URHO3D_LOGERRORF("Could not open load scene '%s'.", path.CString());
    }
    else
        URHO3D_LOGERRORF("Could not open open '%s' for reading.", path.CString());

    return false;
}

}
