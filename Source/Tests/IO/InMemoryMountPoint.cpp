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

Urho3D::InMemoryMountPoint::InMemoryMountPoint(Context* context)
    : MountPoint(context)
{
}

Urho3D::InMemoryMountPoint::InMemoryMountPoint(Context* context, const ea::string& scheme)
    : MountPoint(context)
    , scheme_(scheme)
{
}

void Urho3D::InMemoryMountPoint::AddFile(const ea::string& fileName, MemoryBuffer memory)
{
    files_.emplace(fileName, memory);
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

    return AbstractFilePtr(&iter->second, this);
}

ea::string Urho3D::InMemoryMountPoint::GetFileName(const FileIdentifier& fileName) const
{
    return EMPTY_STRING;
}
