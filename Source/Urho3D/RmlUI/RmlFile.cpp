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

#include "Urho3D/Precompiled.h"

#include "Urho3D/RmlUI/RmlFile.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Resource/ResourceCache.h"

#include "Urho3D/DebugNew.h"

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
    auto cache = context_->GetSubsystem<ResourceCache>();

    if (AbstractFilePtr file = cache->GetFile(path))
    {
        loadedResources_.insert(file->GetName());
        return reinterpret_cast<Rml::FileHandle>(file.Detach());
    }
    return 0;
}

void RmlFile::Close(Rml::FileHandle file)
{
    delete reinterpret_cast<AbstractFile*>(file);
}

size_t RmlFile::Read(void* buffer, size_t size, Rml::FileHandle file)
{
    return reinterpret_cast<AbstractFile*>(file)->Read(buffer, size);
}

bool RmlFile::Seek(Rml::FileHandle file, long offset, int origin)
{
    AbstractFile* fp = reinterpret_cast<AbstractFile*>(file);
    if (origin == SEEK_CUR)
        offset = fp->Tell() + offset;
    else if (origin == SEEK_END)
        offset = fp->GetSize() - offset;
    return fp->Seek(offset) == offset;
}

size_t RmlFile::Tell(Rml::FileHandle file)
{
    return reinterpret_cast<AbstractFile*>(file)->Tell();
}

size_t RmlFile::Length(Rml::FileHandle file)
{
    return reinterpret_cast<AbstractFile*>(file)->GetSize();
}

bool RmlFile::IsResourceLoaded(const ea::string& path)
{
    return loadedResources_.contains(path);
}

}   // namespace Detail

}   // namespace Urho3D
