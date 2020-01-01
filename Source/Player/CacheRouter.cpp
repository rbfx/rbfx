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

#include <Urho3D/IO/ArchiveSerialization.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/Resource/JSONArchive.h>

#include "CacheRouter.h"


namespace Urho3D
{

CacheRouter::CacheRouter(Context* context)
    : ResourceRouter(context)
{
}

void CacheRouter::Route(ea::string& name, ResourceRequest requestType)
{
    auto it = mapping_.find(name);
    if (it != mapping_.end())
        name = it->second;
}

bool CacheRouter::AddPackage(PackageFile* packageFile)
{
    if (packageFile == nullptr)
        return false;

    File file(context_);
    const char* cacheInfo = "CacheInfo.json";
    if (!file.Open(packageFile, cacheInfo))
    {
        URHO3D_LOGERROR("Failed to open {} in package {}", cacheInfo, packageFile->GetName());
        return false;
    }

    JSONFile jsonFile(context_);
    if (!jsonFile.BeginLoad(file))
    {
        URHO3D_LOGERROR("Failed to load {} in package {}", cacheInfo, packageFile->GetName());
        return false;
    }

    JSONInputArchive archive(&jsonFile);
    ea::unordered_map<ea::string, ea::string> mapping;
    if (!SerializeStringMap(archive, "cacheInfo", "map", mapping))
    {
        URHO3D_LOGERROR("Failed to deserialize {} in package {}", cacheInfo, packageFile->GetName());
        return false;
    }

    mapping_.insert(mapping.begin(), mapping.end());
    return true;
}

}
