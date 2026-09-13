// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    return reinterpret_cast<AbstractFile*>(file)->Read(buffer, static_cast<unsigned>(size));
}

bool RmlFile::Seek(Rml::FileHandle file, long offset, int origin)
{
    AbstractFile* fp = reinterpret_cast<AbstractFile*>(file);
    if (origin == SEEK_CUR)
        offset = fp->Tell() + offset;
    else if (origin == SEEK_END)
        offset = fp->GetSize() - offset;
    if (offset < 0 || offset > M_MAX_UNSIGNED)
        return false;
    return fp->Seek(static_cast<unsigned>(offset)) == offset;
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

void RmlFile::AddResourceLoaded(const ea::string& resourceName)
{
    loadedResources_.insert(resourceName);
}

}   // namespace Detail

}   // namespace Urho3D
