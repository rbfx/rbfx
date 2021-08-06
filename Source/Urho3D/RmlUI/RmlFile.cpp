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
#include "../Core/Context.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../RmlUI/RmlFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace Detail
{

RmlFile::RmlFile(Urho3D::Context* context)
    : context_(context)
{

}

Rml::FileHandle RmlFile::Open(const Rml::String& path)
{
    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
    SharedPtr<File> file(cache->GetFile(path));
    if (file.NotNull())
    {
        loadedFiles_.insert(GetAbsolutePath(cache->GetResourceFileName(path)));
        return reinterpret_cast<Rml::FileHandle>(file.Detach());
    }
    return 0;
}

void RmlFile::Close(Rml::FileHandle file)
{
    delete reinterpret_cast<File*>(file);
}

size_t RmlFile::Read(void* buffer, size_t size, Rml::FileHandle file)
{
    return reinterpret_cast<File*>(file)->Read(buffer, size);
}

bool RmlFile::Seek(Rml::FileHandle file, long offset, int origin)
{
    File* fp = reinterpret_cast<File*>(file);
    if (origin == SEEK_CUR)
        offset = fp->Tell() + offset;
    else if (origin == SEEK_END)
        offset = fp->GetSize() - offset;
    return fp->Seek(offset) == offset;
}

size_t RmlFile::Tell(Rml::FileHandle file)
{
    return reinterpret_cast<File*>(file)->Tell();
}

size_t RmlFile::Length(Rml::FileHandle file)
{
    return reinterpret_cast<File*>(file)->GetSize();
}

bool RmlFile::IsFileLoaded(const ea::string& path)
{
    ea::string fullPath;
    FileSystem* fs = context_->GetSubsystem<FileSystem>();
    if (IsAbsolutePath(path))
        fullPath = GetAbsolutePath(path);
    else
        fullPath = GetAbsolutePath(fs->GetCurrentDir() + path);
    return loadedFiles_.contains(fullPath);
}

}   // namespace Detail

}   // namespace Urho3D
