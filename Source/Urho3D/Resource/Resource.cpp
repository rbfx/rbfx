//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Core/Profiler.h"
#include "../Core/Thread.h"
#include "../IO/File.h"
#include "../IO/Log.h"
#include "../Resource/Resource.h"
#include "../Resource/XMLElement.h"

namespace Urho3D
{

Resource::Resource(Context* context) :
    Object(context),
    memoryUse_(0),
    asyncLoadState_(ASYNC_DONE)
{
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
