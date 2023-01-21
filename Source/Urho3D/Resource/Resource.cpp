//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../Core/Thread.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/BinaryArchive.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../Resource/Resource.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLElement.h"
#include "../Resource/XMLArchive.h"
#include "../Resource/JSONArchive.h"
#include "Urho3D/IO/MemoryBuffer.h"

#include <EASTL/finally.h>

namespace Urho3D
{

const BinaryMagic DefaultBinaryMagic{{'\0', 'B', 'I', 'N'}};

InternalResourceFormat PeekResourceFormat(Deserializer& source, BinaryMagic binaryMagic)
{
    const auto basePos = source.Tell();
    const auto guard = ea::make_finally([&]() { source.Seek(basePos); });
    const auto isBlank = [](char c) { return std::isspace(static_cast<unsigned char>(c)); };

    BinaryMagic magic;
    const unsigned count = source.Read(magic.data(), BinaryMagicSize);

    // It's binary file only if it starts with magic word
    if (count == BinaryMagicSize)
    {
        if (magic == binaryMagic)
            return InternalResourceFormat::Binary;
    }

    // It's XML file only if it starts with "<"
    if (count == BinaryMagicSize)
    {
        const auto iter = ea::find(ea::begin(magic), ea::end(magic), '<');
        if (iter != ea::end(magic) && ea::all_of(ea::begin(magic), iter, isBlank))
            return InternalResourceFormat::Xml;
    }

    // It's JSON file only if it starts with "{"
    if (count >= 2)
    {
        const auto iter = ea::find(ea::begin(magic), ea::end(magic), '{');
        if (iter != ea::end(magic) && ea::all_of(ea::begin(magic), iter, isBlank))
            return InternalResourceFormat::Json;
    }

    // If starts with unexpected symbols, it's unknown
    if (!ea::all_of(ea::begin(magic), ea::end(magic), isBlank))
        return InternalResourceFormat::Unknown;

    // It still may be XML or JSON file, warn user about performance penalty and peek more data
    URHO3D_LOGWARNING(
        "File starts with whitespace, peeking more data to determine format. It may cause performance penalty.");

    while (!source.IsEof())
    {
        const char c = source.ReadUByte();
        if (c == '<')
            return InternalResourceFormat::Xml;
        if (c == '{')
            return InternalResourceFormat::Json;
        if (!isBlank(c))
            return InternalResourceFormat::Unknown;
    }

    return InternalResourceFormat::Unknown;
}

Resource::Resource(Context* context) :
    Object(context),
    memoryUse_(0),
    asyncLoadState_(ASYNC_DONE)
{
}

Resource* Resource::LoadFromCache(Context* context, StringHash type, const ea::string& name)
{
    if (name.empty())
        return nullptr;

    auto cache = context->GetSubsystem<ResourceCache>();
    return cache->GetResource(type, name);
}

bool Resource::Load(Deserializer& source)
{
    // Because BeginLoad() / EndLoad() can be called from worker threads, where profiling would be a no-op,
    // create a type name -based profile block here
    URHO3D_PROFILE_C("Load", PROFILER_COLOR_RESOURCES);
    ea::string eventName = ToString("%s::Load(\"%s\")", GetTypeName().c_str(), GetName().c_str());
    URHO3D_PROFILE_ZONENAME(eventName.c_str(), eventName.length());

    // If we are loading synchronously in a non-main thread, behave as if async loading (for example use
    // GetTempResource() instead of GetResource() to load resource dependencies)
    SetAsyncLoadState(Thread::IsMainThread() ? ASYNC_DONE : ASYNC_LOADING);
    bool success = BeginLoad(source);
    if (success)
        success &= EndLoad();
    SetAsyncLoadState(ASYNC_DONE);

    return success;
}

bool Resource::BeginLoad(Deserializer& source)
{
    // This always needs to be overridden by subclasses
    return false;
}

bool Resource::EndLoad()
{
    // If no GPU upload step is necessary, no override is necessary
    return true;
}

bool Resource::Save(Serializer& dest) const
{
    URHO3D_LOGERROR("Save not supported for " + GetTypeName());
    return false;
}

bool Resource::LoadFile(const ea::string& fileName)
{
    File file(context_);
    return file.Open(fileName, FILE_READ) && Load(file);
}

bool Resource::SaveFile(const ea::string& fileName) const
{
    auto fs = GetSubsystem<FileSystem>();
    if (!fs->CreateDirsRecursive(GetPath(fileName)))
        return false;

    File file(context_);
    return file.Open(fileName, FILE_WRITE) && Save(file);
}

void Resource::SetName(const ea::string& name)
{
    name_ = name;
    nameHash_ = name;
}

void Resource::SetMemoryUse(unsigned size)
{
    memoryUse_ = size;
}

void Resource::ResetUseTimer()
{
    useTimer_.Reset();
}

void Resource::SetAsyncLoadState(AsyncLoadState newState)
{
    asyncLoadState_ = newState;
}

unsigned Resource::GetUseTimer()
{
    // If more references than the resource cache, return always 0 & reset the timer
    if (Refs() > 1)
    {
        useTimer_.Reset();
        return 0;
    }
    else
        return useTimer_.GetMSec(false);
}

SimpleResource::SimpleResource(Context* context)
    : Resource(context)
{
}

bool SimpleResource::Save(Serializer& dest, InternalResourceFormat format) const
{
    const auto binaryMagic = GetBinaryMagic();

    try
    {
        switch (format)
        {
        case InternalResourceFormat::Json:
        {
            JSONFile jsonFile(context_);
            JSONOutputArchive archive{context_, jsonFile.GetRoot(), &jsonFile};
            SerializeValue(archive, GetRootBlockName(), const_cast<SimpleResource&>(*this));
            return jsonFile.Save(dest);
        }
        case InternalResourceFormat::Xml:
        {
            XMLFile xmlFile(context_);
            XMLOutputArchive archive{context_, xmlFile.GetOrCreateRoot(GetRootBlockName()), &xmlFile};
            SerializeValue(archive, GetRootBlockName(), const_cast<SimpleResource&>(*this));
            return xmlFile.Save(dest);
        }
        case InternalResourceFormat::Binary:
        {
            dest.Write(binaryMagic.data(), BinaryMagicSize);

            BinaryOutputArchive archive{context_, dest};
            SerializeValue(archive, GetRootBlockName(), const_cast<SimpleResource&>(*this));
            return true;
        }
        default:
        {
            URHO3D_ASSERT(false);
            return false;
        }
        }
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Cannot save SimpleResource: ", e.what());
        return false;
    }
}

bool SimpleResource::SaveFile(const ea::string& fileName, InternalResourceFormat format) const
{
    auto fs = GetSubsystem<FileSystem>();
    if (!fs->CreateDirsRecursive(GetPath(fileName)))
        return false;

    File file(context_);
    if (!file.Open(fileName, FILE_WRITE))
        return false;

    return Save(file, format);
}

bool SimpleResource::BeginLoad(Deserializer& source)
{
    const auto binaryMagic = GetBinaryMagic();

    try
    {
        const auto format = PeekResourceFormat(source, binaryMagic);
        switch (format)
        {
        case InternalResourceFormat::Json:
        {
            JSONFile jsonFile(context_);
            if (!jsonFile.Load(source))
                return false;

            JSONInputArchive archive{context_, jsonFile.GetRoot(), &jsonFile};
            SerializeValue(archive, GetRootBlockName(), *this);

            loadFormat_ = format;
            return true;
        }
        case InternalResourceFormat::Xml:
        {
            XMLFile xmlFile(context_);
            if (!xmlFile.Load(source))
                return false;

            XMLInputArchive archive{context_, xmlFile.GetRoot(), &xmlFile};
            SerializeValue(archive, GetRootBlockName(), *this);

            loadFormat_ = format;
            return true;
        }
        case InternalResourceFormat::Binary:
        {
            source.SeekRelative(BinaryMagicSize);

            BinaryInputArchive archive{context_, source};
            SerializeValue(archive, GetRootBlockName(), *this);

            loadFormat_ = format;
            return true;
        }
        default:
        {
            URHO3D_LOGERROR("Unknown resource format");
            return false;
        }
        }
    }
    catch (const ArchiveException& e)
    {
        URHO3D_LOGERROR("Cannot load SimpleResource: ", e.what());
        return false;
    }
}

bool SimpleResource::EndLoad()
{
    return true;
}

bool SimpleResource::Save(Serializer& dest) const
{
    return Save(dest, loadFormat_.value_or(GetDefaultInternalFormat()));
}

bool SimpleResource::SaveFile(const ea::string& fileName) const
{
    return SaveFile(fileName, loadFormat_.value_or(GetDefaultInternalFormat()));
}

void ResourceWithMetadata::AddMetadata(const ea::string& name, const Variant& value)
{
    const bool exists = !metadata_.insert_or_assign(StringHash(name), value).second;
    if (!exists)
        metadataKeys_.push_back(name);
}

void ResourceWithMetadata::RemoveMetadata(const ea::string& name)
{
    metadata_.erase(name);
    metadataKeys_.erase_first(name);
}

void ResourceWithMetadata::RemoveAllMetadata()
{
    metadata_.clear();
    metadataKeys_.clear();
}

const Urho3D::Variant& ResourceWithMetadata::GetMetadata(const ea::string& name) const
{
    auto it = metadata_.find(name);
    if (it != metadata_.end())
        return it->second;

    return Variant::EMPTY;
}

bool ResourceWithMetadata::HasMetadata() const
{
    return !metadata_.empty();
}

void ResourceWithMetadata::LoadMetadataFromXML(const XMLElement& source)
{
    RemoveAllMetadata();
    for (XMLElement elem = source.GetChild("metadata"); elem; elem = elem.GetNext("metadata"))
        AddMetadata(elem.GetAttribute("name"), elem.GetVariant());
}

void ResourceWithMetadata::LoadMetadataFromJSON(const JSONArray& array)
{
    RemoveAllMetadata();
    for (unsigned i = 0; i < array.size(); i++)
    {
        const JSONValue& value = array.at(i);
        AddMetadata(value.Get("name").GetString(), value.GetVariant());
    }
}

void ResourceWithMetadata::SaveMetadataToXML(XMLElement& destination) const
{
    for (unsigned i = 0; i < metadataKeys_.size(); ++i)
    {
        XMLElement elem = destination.CreateChild("metadata");
        elem.SetString("name", metadataKeys_[i]);
        elem.SetVariant(GetMetadata(metadataKeys_[i]));
    }
}

void ResourceWithMetadata::CopyMetadata(const ResourceWithMetadata& source)
{
    metadata_ = source.metadata_;
    metadataKeys_ = source.metadataKeys_;
}

}
