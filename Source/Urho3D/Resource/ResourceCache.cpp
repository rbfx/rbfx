//
// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2024-2024 the rbfx project.
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/PackageFile.h>
#include <Urho3D/IO/VirtualFileSystem.h>
#include <Urho3D/Resource/BackgroundLoader.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/Graph.h>
#include <Urho3D/Resource/GraphNode.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/ImageCube.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/PListFile.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Resource/SerializableResource.h>
#include <Urho3D/Resource/ResourceEvents.h>
#include <Urho3D/Resource/XMLFile.h>

#include "../DebugNew.h"

#include <cstdio>

namespace Urho3D
{

bool NeedToReloadDependencies(Resource* resource)
{
    // It should always return true in perfect world, but I never tested it.
    if (!resource)
        return true;
    const ea::string extension = GetExtension(resource->GetName());
    return extension == ".xml"
        || extension == ".glsl"
        || extension == ".hlsl";
}

static const char* checkDirs[] =
{
    "Fonts",
    "Materials",
    "Models",
    "Music",
    "Objects",
    "Particle",
    "PostProcess",
    "RenderPaths",
    "Scenes",
    "Scripts",
    "Sounds",
    "Shaders",
    "Techniques",
    "Textures",
    "UI",
    nullptr
};

static const SharedPtr<Resource> noResource;

ResourceCache::ResourceCache(Context* context) :
    Object(context),
    returnFailedResources_(false),
    searchPackagesFirst_(true),
    finishBackgroundResourcesMs_(5)
{
    // Register Resource library object factories
    RegisterResourceLibrary(context_);

#ifdef URHO3D_THREADING
    // Create resource background loader. Its thread will start on the first background request
    backgroundLoader_ = new BackgroundLoader(this);
#endif

    // Subscribe BeginFrame for handling background loaded resource finalization
    SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(ResourceCache, HandleBeginFrame));

    // Subscribe FileChanged for handling directory watchers
    SubscribeToEvent(E_FILECHANGED, URHO3D_HANDLER(ResourceCache, HandleFileChanged));

    // Subscribe to reflection removal to purge unloaded resource types
    context_->OnReflectionRemoved.Subscribe(this, &ResourceCache::HandleReflectionRemoved);
}

ResourceCache::~ResourceCache()
{
#ifdef URHO3D_THREADING
    // Shut down the background loader first
    backgroundLoader_.Reset();
#endif
}

bool ResourceCache::AddManualResource(Resource* resource)
{
    if (!resource)
    {
        URHO3D_LOGERROR("Null manual resource");
        return false;
    }

    const ea::string& name = resource->GetName();
    if (name.empty())
    {
        URHO3D_LOGERROR("Manual resource with empty name, can not add");
        return false;
    }

    resource->ResetUseTimer();
    resourceGroups_[resource->GetType()].resources_[resource->GetNameHash()] = resource;
    UpdateResourceGroup(resource->GetType());
    return true;
}

void ResourceCache::ReleaseResource(StringHash type, const ea::string& name, bool force)
{
    StringHash nameHash(name);
    const SharedPtr<Resource>& existingRes = FindResource(type, nameHash);
    if (!existingRes)
        return;

    // If other references exist, do not release, unless forced
    if ((existingRes.Refs() == 1 && existingRes.WeakRefs() == 0) || force)
    {
        resourceGroups_[type].resources_.erase(nameHash);
        UpdateResourceGroup(type);
    }
}

void ResourceCache::ReleaseResource(const ea::string& resourceName, bool force)
{
    // Some resources refer to others, like materials to textures. Repeat the release logic as many times as necessary to ensure
    // these get released. This is not necessary if forcing release
    bool released;
    do
    {
        released = false;

        for (auto i = resourceGroups_.begin(); i != resourceGroups_.end(); ++i)
        {
            for (auto j = i->second.resources_.begin(); j != i->second.resources_.end();)
            {
                auto current = i->second.resources_.find(resourceName);
                if (current != i->second.resources_.end())
                {
                    // If other references exist, do not release, unless forced
                    if ((current->second.Refs() == 1 && current->second.WeakRefs() == 0) || force)
                    {
                        j = i->second.resources_.erase(current);
                        released = true;
                        continue;
                    }
                }
                ++j;
            }
            if (released)
                UpdateResourceGroup(i->first);
        }

    } while (released && !force);
}

void ResourceCache::ReleaseResources(StringHash type, bool force)
{
    bool released = false;

    auto i = resourceGroups_.find(type);
    if (i != resourceGroups_.end())
    {
        for (auto j = i->second.resources_.begin();
             j != i->second.resources_.end();)
        {
            auto current = j++;
            // If other references exist, do not release, unless forced
            if ((current->second.Refs() == 1 && current->second.WeakRefs() == 0) || force)
            {
                i->second.resources_.erase(current);
                released = true;
            }
        }
    }

    if (released)
        UpdateResourceGroup(type);
}

void ResourceCache::ReleaseResources(StringHash type, const ea::string& partialName, bool force)
{
    bool released = false;

    auto i = resourceGroups_.find(type);
    if (i != resourceGroups_.end())
    {
        for (auto j = i->second.resources_.begin();
             j != i->second.resources_.end();)
        {
            auto current = j++;
            if (current->second->GetName().contains(partialName))
            {
                // If other references exist, do not release, unless forced
                if ((current->second.Refs() == 1 && current->second.WeakRefs() == 0) || force)
                {
                    i->second.resources_.erase(current);
                    released = true;
                }
            }
        }
    }

    if (released)
        UpdateResourceGroup(type);
}

void ResourceCache::ReleaseResources(const ea::string& partialName, bool force)
{
    // Some resources refer to others, like materials to textures. Repeat the release logic as many times as necessary to ensure
    // these get released. This is not necessary if forcing release
    bool released;
    do
    {
        released = false;

        for (auto i = resourceGroups_.begin(); i !=
            resourceGroups_.end(); ++i)
        {
            for (auto j = i->second.resources_.begin();
                 j != i->second.resources_.end();)
            {
                auto current = j++;
                if (current->second->GetName().contains(partialName))
                {
                    // If other references exist, do not release, unless forced
                    if ((current->second.Refs() == 1 && current->second.WeakRefs() == 0) || force)
                    {
                        i->second.resources_.erase(current);
                        released = true;
                    }
                }
            }
            if (released)
                UpdateResourceGroup(i->first);
        }

    } while (released && !force);
}

void ResourceCache::ReleaseAllResources(bool force)
{
    bool released;
    do
    {
        released = false;

        for (auto i = resourceGroups_.begin();
             i != resourceGroups_.end(); ++i)
        {
            for (auto j = i->second.resources_.begin();
                 j != i->second.resources_.end();)
            {
                auto current = j++;
                // If other references exist, do not release, unless forced
                if ((current->second.Refs() == 1 && current->second.WeakRefs() == 0) || force)
                {
                    i->second.resources_.erase(current);
                    released = true;
                }
            }
            if (released)
                UpdateResourceGroup(i->first);
        }

    } while (released && !force);
}

bool ResourceCache::ReloadResource(const ea::string_view resourceName)
{
    if (Resource* resource = FindResource(StringHash::Empty, resourceName))
        return ReloadResource(resource);
    return false;
}

bool ResourceCache::ReloadResource(Resource* resource)
{
    if (!resource)
        return false;

    resource->SendEvent(E_RELOADSTARTED);

    bool success = false;
    const AbstractFilePtr file = GetFile(resource->GetName());
    if (file)
        success = resource->Load(*(file.Get()));

    if (success)
    {
        resource->ResetUseTimer();
        UpdateResourceGroup(resource->GetType());
        resource->SendEvent(E_RELOADFINISHED);
        return true;
    }

    // If reloading failed, do not remove the resource from cache, to allow for a new live edit to
    // attempt loading again
    resource->SendEvent(E_RELOADFAILED);
    return false;
}

void ResourceCache::ReloadResourceWithDependencies(const ea::string& fileName)
{
    StringHash fileNameHash(fileName);
    // If the filename is a resource we keep track of, reload it
    const SharedPtr<Resource>& resource = FindResource(fileNameHash);
    if (resource)
    {
        URHO3D_LOGDEBUG("Reloading changed resource " + fileName);
        ReloadResource(resource.Get());
    }
    // Always perform dependency resource check for resource loaded from XML file as it could be used in inheritance
    if (NeedToReloadDependencies(resource))
    {
        // Check if this is a dependency resource, reload dependents
        auto j = dependentResources_.find(
            fileNameHash);
        if (j != dependentResources_.end())
        {
            // Reloading a resource may modify the dependency tracking structure. Therefore collect the
            // resources we need to reload first
            ea::vector<SharedPtr<Resource> > dependents;
            dependents.reserve(j->second.size());

            for (auto k = j->second.begin(); k != j->second.end(); ++k)
            {
                const SharedPtr<Resource>& dependent = FindResource(*k);
                if (dependent)
                    dependents.push_back(dependent);
            }

            for (unsigned k = 0; k < dependents.size(); ++k)
            {
                URHO3D_LOGDEBUG("Reloading resource " + dependents[k]->GetName() + " depending on " + fileName);
                ReloadResource(dependents[k].Get());
            }
        }
    }
}

void ResourceCache::SetMemoryBudget(StringHash type, unsigned long long budget)
{
    resourceGroups_[type].memoryBudget_ = budget;
}

void ResourceCache::AddResourceRouter(ResourceRouter* router, bool addAsFirst)
{
    // Check for duplicate
    for (unsigned i = 0; i < resourceRouters_.size(); ++i)
    {
        if (resourceRouters_[i] == router)
            return;
    }

    if (addAsFirst)
        resourceRouters_.push_front(SharedPtr<ResourceRouter>(router));
    else
        resourceRouters_.push_back(SharedPtr<ResourceRouter>(router));
}

void ResourceCache::RemoveResourceRouter(ResourceRouter* router)
{
    for (unsigned i = 0; i < resourceRouters_.size(); ++i)
    {
        if (resourceRouters_[i] == router)
        {
            resourceRouters_.erase_at(i);
            return;
        }
    }
}

AbstractFilePtr ResourceCache::GetFile(const ea::string& name, bool sendEventOnFailure)
{
    const auto* vfs = GetSubsystem<VirtualFileSystem>();

    const FileIdentifier resolvedName = GetResolvedIdentifier(FileIdentifier::FromUri(name));
    auto file = vfs->OpenFile(resolvedName, FILE_READ);

    if (!file && sendEventOnFailure)
    {
        if (!resourceRouters_.empty() && !resolvedName)
            URHO3D_LOGERROR("Resource request '{}' was blocked", name);
        else
            URHO3D_LOGERROR("Could not find resource '{}'", resolvedName.ToUri());

        if (Thread::IsMainThread())
        {
            using namespace ResourceNotFound;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCENAME] = resolvedName ? resolvedName.ToUri() : name;
            SendEvent(E_RESOURCENOTFOUND, eventData);
        }
    }

    return file;
}

Resource* ResourceCache::GetExistingResource(StringHash type, const ea::string& name)
{
    ea::string sanitatedName = SanitateResourceName(name);

    if (!Thread::IsMainThread())
    {
        URHO3D_LOGERROR("Attempted to get resource " + sanitatedName + " from outside the main thread");
        return nullptr;
    }

    // If empty name, return null pointer immediately
    if (sanitatedName.empty())
        return nullptr;

    StringHash nameHash(sanitatedName);

    const SharedPtr<Resource>& existing = type != StringHash::Empty ? FindResource(type, nameHash) : FindResource(nameHash);
    return existing;
}

Resource* ResourceCache::GetResource(StringHash type, const ea::string& name, bool sendEventOnFailure)
{
    ea::string sanitatedName = SanitateResourceName(name);

    if (!Thread::IsMainThread())
    {
        URHO3D_LOGERROR("Attempted to get resource " + sanitatedName + " from outside the main thread");
        return nullptr;
    }

    // If empty name, return null pointer immediately
    if (sanitatedName.empty())
        return nullptr;

    StringHash nameHash(sanitatedName);

#ifdef URHO3D_THREADING
    // Check if the resource is being background loaded but is now needed immediately
    backgroundLoader_->WaitForResource(type, nameHash);
#endif

    const SharedPtr<Resource>& existing = FindResource(type, nameHash);
    if (existing)
        return existing;

    SharedPtr<Resource> resource;
    // Make sure the pointer is non-null and is a Resource subclass
    resource = DynamicCast<Resource>(context_->CreateObject(type));
    if (!resource)
    {
        URHO3D_LOGERROR("Could not load unknown resource type " + type.ToString());

        if (sendEventOnFailure)
        {
            using namespace UnknownResourceType;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCETYPE] = type;
            SendEvent(E_UNKNOWNRESOURCETYPE, eventData);
        }

        return nullptr;
    }

    // Attempt to load the resource
    const AbstractFilePtr file = GetFile(sanitatedName, sendEventOnFailure);
    if (!file)
        return nullptr;   // Error is already logged

    URHO3D_LOGDEBUG("Loading resource " + sanitatedName);
    resource->SetName(sanitatedName);
    resource->SetAbsoluteFileName(file->GetAbsoluteName());

    if (!resource->Load(*(file.Get())))
    {
        // Error should already been logged by corresponding resource descendant class
        if (sendEventOnFailure)
        {
            using namespace LoadFailed;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCENAME] = sanitatedName;
            SendEvent(E_LOADFAILED, eventData);
        }

        if (!returnFailedResources_)
            return nullptr;
    }

    // Store to cache
    resource->ResetUseTimer();
    resourceGroups_[type].resources_[nameHash] = resource;
    UpdateResourceGroup(type);

    return resource;
}

bool ResourceCache::BackgroundLoadResource(StringHash type, const ea::string& name, bool sendEventOnFailure, Resource* caller)
{
#ifdef URHO3D_THREADING
    // If empty name, fail immediately
    ea::string sanitatedName = SanitateResourceName(name);
    if (sanitatedName.empty())
        return false;

    // First check if already exists as a loaded resource
    StringHash nameHash(sanitatedName);
    if (FindResource(type, nameHash) != noResource)
        return false;

    return backgroundLoader_->QueueResource(type, sanitatedName, sendEventOnFailure, caller);
#else
    // When threading not supported, fall back to synchronous loading
    return GetResource(type, name, sendEventOnFailure);
#endif
}

SharedPtr<Resource> ResourceCache::GetTempResource(StringHash type, const ea::string& name, bool sendEventOnFailure)
{
    ea::string sanitatedName = SanitateResourceName(name);

    // If empty name, return null pointer immediately
    if (sanitatedName.empty())
        return SharedPtr<Resource>();

    SharedPtr<Resource> resource;
    // Make sure the pointer is non-null and is a Resource subclass
    resource = DynamicCast<Resource>(context_->CreateObject(type));
    if (!resource)
    {
        URHO3D_LOGERROR("Could not load unknown resource type " + type.ToString());

        if (sendEventOnFailure)
        {
            using namespace UnknownResourceType;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCETYPE] = type;
            SendEvent(E_UNKNOWNRESOURCETYPE, eventData);
        }

        return SharedPtr<Resource>();
    }

    // Attempt to load the resource
    const AbstractFilePtr file = GetFile(sanitatedName, sendEventOnFailure);
    if (!file)
        return SharedPtr<Resource>();  // Error is already logged

    URHO3D_LOGDEBUG("Loading temporary resource " + sanitatedName);
    resource->SetName(file->GetName());
    resource->SetAbsoluteFileName(file->GetAbsoluteName());

    if (!resource->Load(*(file.Get())))
    {
        // Error should already been logged by corresponding resource descendant class
        if (sendEventOnFailure)
        {
            using namespace LoadFailed;

            VariantMap& eventData = GetEventDataMap();
            eventData[P_RESOURCENAME] = sanitatedName;
            SendEvent(E_LOADFAILED, eventData);
        }

        return SharedPtr<Resource>();
    }

    return resource;
}

unsigned ResourceCache::GetNumBackgroundLoadResources() const
{
#ifdef URHO3D_THREADING
    return backgroundLoader_->GetNumQueuedResources();
#else
    return 0;
#endif
}

void ResourceCache::GetResources(ea::vector<Resource*>& result, StringHash type) const
{
    result.clear();
    auto i = resourceGroups_.find(type);
    if (i != resourceGroups_.end())
    {
        for (auto j = i->second.resources_.begin();
             j != i->second.resources_.end(); ++j)
            result.push_back(j->second.Get());
    }
}

bool ResourceCache::Exists(const ea::string& name) const
{
    const FileIdentifier resolvedName = GetResolvedIdentifier(FileIdentifier::FromUri(name));
    if (!resolvedName)
        return false;

    const auto vfs = context_->GetSubsystem<VirtualFileSystem>();
    return vfs->Exists(resolvedName);
}

unsigned long long ResourceCache::GetMemoryBudget(StringHash type) const
{
    auto i = resourceGroups_.find(type);
    return i != resourceGroups_.end() ? i->second.memoryBudget_ : 0;
}

unsigned long long ResourceCache::GetMemoryUse(StringHash type) const
{
    auto i = resourceGroups_.find(type);
    return i != resourceGroups_.end() ? i->second.memoryUse_ : 0;
}

unsigned long long ResourceCache::GetTotalMemoryUse() const
{
    unsigned long long total = 0;
    for (auto i = resourceGroups_.begin(); i !=
        resourceGroups_.end(); ++i)
        total += i->second.memoryUse_;
    return total;
}

ea::string ResourceCache::GetResourceFileName(const ea::string& name) const
{
    const auto vfs = context_->GetSubsystem<VirtualFileSystem>();
    return vfs->GetAbsoluteNameFromIdentifier(FileIdentifier::FromUri(name));
}

ResourceRouter* ResourceCache::GetResourceRouter(unsigned index) const
{
    return index < resourceRouters_.size() ? resourceRouters_[index] : nullptr;
}

ea::string ResourceCache::SanitateResourceName(const ea::string& name) const
{
    return GetCanonicalIdentifier(FileIdentifier::FromUri(name)).ToUri();
}

void ResourceCache::StoreResourceDependency(Resource* resource, const ea::string& dependency)
{
    if (!resource)
        return;

    MutexLock lock(resourceMutex_);

    StringHash nameHash(resource->GetName());
    ea::hash_set<StringHash>& dependents = dependentResources_[dependency];
    dependents.insert(nameHash);
}

void ResourceCache::ResetDependencies(Resource* resource)
{
    if (!resource)
        return;

    MutexLock lock(resourceMutex_);

    StringHash nameHash(resource->GetName());

    for (auto i = dependentResources_.begin(); i !=
        dependentResources_.end();)
    {
        ea::hash_set<StringHash>& dependents = i->second;
        dependents.erase(nameHash);
        if (dependents.empty())
            i = dependentResources_.erase(i);
        else
            ++i;
    }
}

ea::string ResourceCache::PrintMemoryUsage() const
{
    ea::string output = "Resource Type                 Cnt       Avg       Max    Budget     Total\n\n";
    char outputLine[256];

    unsigned totalResourceCt = 0;
    unsigned long long totalLargest = 0;
    unsigned long long totalAverage = 0;
    unsigned long long totalUse = GetTotalMemoryUse();

    for (auto cit = resourceGroups_.begin(); cit !=
        resourceGroups_.end(); ++cit)
    {
        const unsigned resourceCt = cit->second.resources_.size();
        unsigned long long average = 0;
        if (resourceCt > 0)
            average = cit->second.memoryUse_ / resourceCt;
        else
            average = 0;
        unsigned long long largest = 0;
        for (auto resIt = cit->second.resources_.begin(); resIt != cit->second.resources_.end(); ++resIt)
        {
            if (resIt->second->GetMemoryUse() > largest)
                largest = resIt->second->GetMemoryUse();
            if (largest > totalLargest)
                totalLargest = largest;
        }

        totalResourceCt += resourceCt;

        const ea::string countString = ea::to_string(cit->second.resources_.size());
        const ea::string memUseString = GetFileSizeString(average);
        const ea::string memMaxString = GetFileSizeString(largest);
        const ea::string memBudgetString = GetFileSizeString(cit->second.memoryBudget_);
        const ea::string memTotalString = GetFileSizeString(cit->second.memoryUse_);
        const ea::string resTypeName = context_->GetTypeName(cit->first);

        memset(outputLine, ' ', 256);
        outputLine[255] = 0;
        sprintf(outputLine, "%-28s %4s %9s %9s %9s %9s\n", resTypeName.c_str(), countString.c_str(), memUseString.c_str(), memMaxString.c_str(), memBudgetString.c_str(), memTotalString.c_str());

        output += ((const char*)outputLine);
    }

    if (totalResourceCt > 0)
        totalAverage = totalUse / totalResourceCt;

    const ea::string countString = ea::to_string(totalResourceCt);
    const ea::string memUseString = GetFileSizeString(totalAverage);
    const ea::string memMaxString = GetFileSizeString(totalLargest);
    const ea::string memTotalString = GetFileSizeString(totalUse);

    memset(outputLine, ' ', 256);
    outputLine[255] = 0;
    sprintf(outputLine, "%-28s %4s %9s %9s %9s %9s\n", "All", countString.c_str(), memUseString.c_str(), memMaxString.c_str(), "-", memTotalString.c_str());
    output += ((const char*)outputLine);

    return output;
}

const SharedPtr<Resource>& ResourceCache::FindResource(StringHash type, StringHash nameHash)
{
    MutexLock lock(resourceMutex_);

    auto i = resourceGroups_.find(type);
    if (i == resourceGroups_.end())
        return noResource;
    auto j = i->second.resources_.find(nameHash);
    if (j == i->second.resources_.end())
        return noResource;

    return j->second;
}

const SharedPtr<Resource>& ResourceCache::FindResource(StringHash nameHash)
{
    MutexLock lock(resourceMutex_);

    for (auto i = resourceGroups_.begin(); i !=
        resourceGroups_.end(); ++i)
    {
        auto j = i->second.resources_.find(nameHash);
        if (j != i->second.resources_.end())
            return j->second;
    }

    return noResource;
}

void ResourceCache::ReleasePackageResources(PackageFile* package, bool force)
{
    ea::hash_set<StringHash> affectedGroups;

    const ea::unordered_map<ea::string, PackageEntry>& entries = package->GetEntries();
    for (auto i = entries.begin(); i != entries.end(); ++i)
    {
        StringHash nameHash(i->first);

        // We do not know the actual resource type, so search all type containers
        for (auto j = resourceGroups_.begin(); j !=
            resourceGroups_.end(); ++j)
        {
            auto k = j->second.resources_.find(nameHash);
            if (k != j->second.resources_.end())
            {
                // If other references exist, do not release, unless forced
                if ((k->second.Refs() == 1 && k->second.WeakRefs() == 0) || force)
                {
                    j->second.resources_.erase(k);
                    affectedGroups.insert(j->first);
                }
                break;
            }
        }
    }

    for (auto i = affectedGroups.begin(); i != affectedGroups.end(); ++i)
        UpdateResourceGroup(*i);
}

void ResourceCache::UpdateResourceGroup(StringHash type)
{
    auto i = resourceGroups_.find(type);
    if (i == resourceGroups_.end())
        return;

    for (;;)
    {
        unsigned totalSize = 0;
        unsigned oldestTimer = 0;
        auto oldestResource = i->second.resources_.end();

        for (auto j = i->second.resources_.begin();
             j != i->second.resources_.end(); ++j)
        {
            totalSize += j->second->GetMemoryUse();
            unsigned useTimer = j->second->GetUseTimer();
            if (useTimer > oldestTimer)
            {
                oldestTimer = useTimer;
                oldestResource = j;
            }
        }

        i->second.memoryUse_ = totalSize;

        // If memory budget defined and is exceeded, remove the oldest resource and loop again
        // (resources in use always return a zero timer and can not be removed)
        if (i->second.memoryBudget_ && i->second.memoryUse_ > i->second.memoryBudget_ &&
            oldestResource != i->second.resources_.end())
        {
            URHO3D_LOGDEBUG("Resource group " + oldestResource->second->GetTypeName() + " over memory budget, releasing resource " +
                     oldestResource->second->GetName());
            i->second.resources_.erase(oldestResource);
        }
        else
            break;
    }
}

void ResourceCache::HandleBeginFrame(StringHash eventType, VariantMap& eventData)
{
    // Check for background loaded resources that can be finished
#ifdef URHO3D_THREADING
    {
        URHO3D_PROFILE("FinishBackgroundResources");
        backgroundLoader_->FinishResources(finishBackgroundResourcesMs_);
    }
#endif
}

void ResourceCache::HandleFileChanged(StringHash eventType, VariantMap& eventData)
{
    const auto& fileName = eventData[FileChanged::P_RESOURCENAME].GetString();

    auto it = ignoreResourceAutoReload_.find(fileName);
    if (it != ignoreResourceAutoReload_.end())
    {
        ignoreResourceAutoReload_.erase(it);
        return;
    }

    ReloadResourceWithDependencies(fileName);
}

void RegisterResourceLibrary(Context* context)
{
    BinaryFile::RegisterObject(context);
    Image::RegisterObject(context);
    ImageCube::RegisterObject(context);
    JSONFile::RegisterObject(context);
    PListFile::RegisterObject(context);
    XMLFile::RegisterObject(context);
    Graph::RegisterObject(context);
    GraphNode::RegisterObject(context);
    SerializableResource::RegisterObject(context);
}

void ResourceCache::Scan(ea::vector<ea::string>& result, const ea::string& pathName, const ea::string& filter, ScanFlags flags) const
{
    auto* vfs = GetSubsystem<VirtualFileSystem>();
    vfs->Scan(result, FileIdentifier::FromUri(pathName), filter, flags);

    // Scan manual resources.
    if (!flags.Test(SCAN_FILES))
        return;

    const bool recursive = flags.Test(SCAN_RECURSIVE);
    const ea::string filterExtension = GetExtensionFromFilter(filter);
    for (const auto& [_, group] : resourceGroups_)
    {
        for (const auto& [_, resource] : group.resources_)
        {
            if (!MatchFileName(resource->GetName(), pathName, filterExtension, recursive))
                continue;

            const FileIdentifier resourceName = FileIdentifier::FromUri(resource->GetName());
            if (!vfs->Exists(resourceName))
                result.emplace_back(TrimPathPrefix(resource->GetName(), pathName));
        }
    }
}

ea::string ResourceCache::PrintResources(const ea::string& typeName) const
{

    StringHash typeNameHash(typeName);

    ea::string output = "Resource Type         Refs   WeakRefs  Name\n\n";

    for (auto cit = resourceGroups_.begin(); cit !=
        resourceGroups_.end(); ++cit)
    {
        for (auto resIt = cit->second.resources_.begin(); resIt != cit->second.resources_.end(); ++resIt)
        {
            Resource* resource = resIt->second;

            // filter
            if (typeName.length() && resource->GetType() != typeNameHash)
                continue;

            output.append_sprintf("%s     %i     %i     %s\n", resource->GetTypeName().c_str(), resource->Refs(),
                resource->WeakRefs(), resource->GetName().c_str());
        }

    }

    return output;
}

void ResourceCache::IgnoreResourceReload(const ea::string& name)
{
    ignoreResourceAutoReload_.emplace_back(name);
}

void ResourceCache::IgnoreResourceReload(const Resource* resource)
{
    IgnoreResourceReload(resource->GetName());
}

void ResourceCache::RouteResourceName(FileIdentifier& name) const
{
    auto vfs = GetSubsystem<VirtualFileSystem>();
    name = vfs->GetCanonicalIdentifier(name);

    thread_local bool reentrancyGuard = false;
    if (reentrancyGuard)
        return;

    reentrancyGuard = true;
    for (ResourceRouter* router : resourceRouters_)
        router->Route(name);
    reentrancyGuard = false;
}

void ResourceCache::Clear()
{
    resourceGroups_.clear();
    dependentResources_.clear();
}

FileIdentifier ResourceCache::GetCanonicalIdentifier(const FileIdentifier& name) const
{
    auto* vfs = GetSubsystem<VirtualFileSystem>();
    return vfs->GetCanonicalIdentifier(name);
}

FileIdentifier ResourceCache::GetResolvedIdentifier(const FileIdentifier& name) const
{
    FileIdentifier result = name;
    RouteResourceName(result);
    return result;
}

void ResourceCache::HandleReflectionRemoved(ObjectReflection* reflection)
{
    ReleaseResources(reflection->GetTypeNameHash(), true);
}

}
