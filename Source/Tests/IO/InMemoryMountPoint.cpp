//
// Copyright (c) 2023-2023 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "InMemoryMountPoint.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/VirtualFileSystem.h"

#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/IO/FileSystem.h>

#include <utility>

Urho3D::InMemoryMountPoint::InMemoryMountPoint(Context* context)
    : MountPoint(context)
{
}

Urho3D::InMemoryMountPoint::InMemoryMountPoint(Context* context, const ea::string& scheme)
    : MountPoint(context)
    , scheme_(scheme)
{
}

void Urho3D::InMemoryMountPoint::SetFile(const ea::string& fileName, MemoryBuffer memory)
{
    auto it = files_.find(fileName);
    if (it == files_.end())
    {
        files_.emplace(fileName, memory);
    }
    else
    {
        it->second = memory;
        if (IsWatching())
        {
            using namespace FileChanged;
            VariantMap& eventData = GetEventDataMap();
            eventData[P_FILENAME] = fileName;
            eventData[P_RESOURCENAME] = fileName;
            SendEvent(E_FILECHANGED, eventData);
        }
    }
}

void Urho3D::InMemoryMountPoint::SetFile(const ea::string& fileName, ea::string_view content)
{
    SetFile(fileName, MemoryBuffer(content));
}

void Urho3D::InMemoryMountPoint::RemoveFile(const ea::string& fileName)
{
    files_.erase(fileName);
}

bool Urho3D::InMemoryMountPoint::AcceptsScheme(const ea::string& scheme) const
{
    return scheme == scheme_;
}

bool Urho3D::InMemoryMountPoint::Exists(const FileIdentifier& fileName) const
{
    return AcceptsScheme(fileName.scheme_) && files_.contains(fileName.fileName_);
}

Urho3D::AbstractFilePtr Urho3D::InMemoryMountPoint::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    if (mode & FILE_WRITE)
        return nullptr;

    const auto iter = files_.find(fileName.fileName_);
    if (iter == files_.end())
        return nullptr;

    iter->second.Seek(0);
    return AbstractFilePtr(&iter->second, this);
}

void Urho3D::InMemoryMountPoint::Scan(ea::vector<ea::string>& result, const ea::string& pathName,
    const ea::string& filter, unsigned flags, bool recursive) const
{
    result.clear();

    ea::string sanitizedPath = GetSanitizedPath(pathName);
    ea::string filterExtension;
    unsigned dotPos = filter.find_last_of('.');
    if (dotPos != ea::string::npos)
        filterExtension = filter.substr(dotPos);
    if (filterExtension.contains('*'))
        filterExtension.clear();

    bool caseSensitive = true;
#ifdef _WIN32
    // On Windows ignore case in string comparisons
    caseSensitive = false;
#endif

    for (auto& kv : files_)
    {
        ea::string entryName = GetSanitizedPath(kv.first);
        if ((filterExtension.empty() || entryName.ends_with(filterExtension, caseSensitive))
            && entryName.starts_with(sanitizedPath, caseSensitive))
        {
            ea::string fileName = entryName.substr(sanitizedPath.length());
            if (fileName.starts_with("\\") || fileName.starts_with("/"))
                fileName = fileName.substr(1, fileName.length() - 1);
            if (!recursive && (fileName.contains("\\") || fileName.contains("/")))
                continue;

            result.push_back(fileName);
        }
    }
}

Urho3D::InMemoryMountPointPtr::InMemoryMountPointPtr(Context* context)
    : ptr_(MakeShared<InMemoryMountPoint>(context))
{
    context->GetSubsystem<VirtualFileSystem>()->Mount(ptr_);
}

Urho3D::InMemoryMountPointPtr::~InMemoryMountPointPtr()
{
    ptr_->GetContext()->GetSubsystem<VirtualFileSystem>()->Unmount(ptr_);
}
