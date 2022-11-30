//
// Copyright (c) 2022-2022 the Urho3D project.
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

#include "../Precompiled.h"

#include "../IO/VirtualFileSystem.h"
#include "../IO/MountPoint.h"

namespace Urho3D
{

VirtualFileSystem::VirtualFileSystem(Context* context)
    : Object(context)
{
}

VirtualFileSystem::~VirtualFileSystem() = default;

void VirtualFileSystem::Mount(MountPoint* mountPoint)
{
    MutexLock lock(mountMutex_);

    const SharedPtr<MountPoint> pointPtr{mountPoint};
    if (mountPoints_.find(pointPtr) == mountPoints_.end())
    {
        return;
    }
    mountPoints_.push_back(pointPtr);
}

void VirtualFileSystem::Unmount(MountPoint* mountPoint)
{
    MutexLock lock(mountMutex_);

    const SharedPtr<MountPoint> pointPtr{mountPoint};
    const auto i = mountPoints_.find(pointPtr);
    if (i != mountPoints_.end())
    {
        // Erase the slow way because order of the mount points matters.
        mountPoints_.erase(i);
    }
}

void VirtualFileSystem::UnmountAll()
{
    MutexLock lock(mountMutex_);

    mountPoints_.clear();
}

void VirtualFileSystem::SetWatching(bool enabled)
{
    MutexLock lock(mountMutex_);

    if (enabled != watching_)
    {
        watching_ = enabled;
        for (auto i = mountPoints_.begin(); i != mountPoints_.end(); ++i)
        {
            if (enabled)
                (*i)->StartWatching();
            else
                (*i)->StopWatching();
        }
    }
}

/// Open file in a virtual file system. Returns null if file not found.
// TODO: use ea::string_view when switched to c++20
AbstractFilePtr VirtualFileSystem::OpenFile(
    const ea::string& scheme, const ea::string& name, FileMode mode) const
{
    MutexLock lock(mountMutex_);

    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        AbstractFilePtr result = (*i)->OpenFile(scheme, name, mode);
        if (result)
            return result;
    }
    return {};
}

ea::string VirtualFileSystem::GetFileName(const ea::string& name)
{
    MutexLock lock(mountMutex_);

    ea::string result;
    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        result = (*i)->GetFileName({}, name);
        if (!result.empty())
            return result;
    }

    auto* fileSystem = GetSubsystem<FileSystem>();
    if (IsAbsolutePath(name) && fileSystem->FileExists(name))
        return name;
    else
        return ea::string();
}

/// Open file in a virtual file system. Returns null if file not found.
// TODO: use ea::string_view when switched to c++20
AbstractFilePtr VirtualFileSystem::OpenFile(const ea::string& schemeAndName, FileMode mode) const
{
    MutexLock lock(mountMutex_);

    const ea::string_view schemeSeparator("://");
    const auto schemePos = schemeAndName.find(schemeSeparator);
    if (schemePos == ea::string::npos)
    {
        return OpenFile({}, schemeAndName, mode);
    }
    return OpenFile(
        schemeAndName.substr(0, schemePos), schemeAndName.substr(schemePos + schemeSeparator.size()), mode);
}

/// Check if a file exists in the virtual file system.
bool VirtualFileSystem::Exists(const ea::string& schemeAndName) const
{
    MutexLock lock(mountMutex_);

    const ea::string_view schemeSeparator("://");
    const auto schemePos = schemeAndName.find(schemeSeparator);
    if (schemePos == ea::string::npos)
    {
        return Exists({}, schemeAndName);
    }
    return Exists(
        schemeAndName.substr(0, schemePos), schemeAndName.substr(schemePos + schemeSeparator.size()));
}

/// Check if a file exists in the virtual file system.
bool VirtualFileSystem::Exists(const ea::string& scheme, const ea::string& name) const
{
    MutexLock lock(mountMutex_);

    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        if ((*i)->Exists(scheme, name))
            return true;
    }
    return false;
}

void VirtualFileSystem::Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter,
    unsigned flags, bool recursive) const
{
    MutexLock lock(mountMutex_);

    ea::vector<ea::string> interimResult;
    for (auto i = mountPoints_.rbegin(); i != mountPoints_.rend(); ++i)
    {
        (*i)->Scan(interimResult, pathName, filter, flags, recursive);
        result.insert(result.end(), interimResult.begin(), interimResult.end());
    }
}

} // namespace Urho3D
