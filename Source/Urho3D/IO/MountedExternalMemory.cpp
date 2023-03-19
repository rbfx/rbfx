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

#include "Urho3D/IO/MountedExternalMemory.h"

#include <utility>

namespace Urho3D
{

namespace
{

class WrappedMemoryBuffer : public RefCounted, public MemoryBuffer
{
public:
    WrappedMemoryBuffer(const MemoryBuffer& buffer)
        : MemoryBuffer(buffer.GetData(), buffer.GetSize())
    {
    }
};

}

MountedExternalMemory::MountedExternalMemory(Context* context, ea::string_view scheme)
    : MountPoint(context)
    , scheme_(scheme)
{
}

void MountedExternalMemory::LinkMemory(ea::string_view fileName, MemoryBuffer memory)
{
    auto it = files_.find_as(fileName);
    if (it == files_.end())
        files_.emplace(ea::string{fileName}, memory);
    else
        it->second = memory;
}

void MountedExternalMemory::LinkMemory(ea::string_view fileName, ea::string_view content)
{
    LinkMemory(fileName, MemoryBuffer(content));
}

void MountedExternalMemory::UnlinkMemory(ea::string_view fileName)
{
    auto it = files_.find_as(fileName);
    if (it != files_.end())
        files_.erase(it);
}

bool MountedExternalMemory::AcceptsScheme(const ea::string& scheme) const
{
    return scheme == scheme_;
}

bool MountedExternalMemory::Exists(const FileIdentifier& fileName) const
{
    return AcceptsScheme(fileName.scheme_) && files_.contains(fileName.fileName_);
}

AbstractFilePtr MountedExternalMemory::OpenFile(const FileIdentifier& fileName, FileMode mode)
{
    if (mode & FILE_WRITE)
        return nullptr;

    const auto iter = files_.find(fileName.fileName_);
    if (iter == files_.end())
        return nullptr;

    return MakeShared<WrappedMemoryBuffer>(iter->second);
}

const ea::string& MountedExternalMemory::GetName() const
{
    return scheme_;
}

void MountedExternalMemory::Scan(
    ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    if (!flags.Test(SCAN_APPEND))
        result.clear();

    const bool recursive = flags.Test(SCAN_RECURSIVE);
    const ea::string filterExtension = GetExtensionFromFilter(filter);

    for (const auto& [name, _] : files_)
    {
        if (!name.starts_with(pathName))
            continue;

        if (!filterExtension.empty() && !name.ends_with(filterExtension, false))
            continue;

        if (!recursive)
        {
            ea::string_view fileName = ea::string_view{name}.substr(pathName.length());
            if (fileName.starts_with('/'))
                fileName = fileName.substr(1);

            if (fileName.find('/') != ea::string_view::npos)
                continue;
        }

        result.push_back(name);
    }
}

} // namespace Urho3D
