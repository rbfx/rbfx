// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Project/LaunchManager.h"

#include <Urho3D/IO/ArchiveSerialization.h>

#include <EASTL/sort.h>

namespace Urho3D
{

const ea::string LaunchConfiguration::UnspecifiedName{"(unspecified)"};

LaunchConfiguration::LaunchConfiguration(const ea::string& name, const ea::string& mainPlugin)
    : name_(name)
    , mainPlugin_(mainPlugin)
{
}

void LaunchConfiguration::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Name", name_);
    SerializeOptionalValue(archive, "MainPlugin", mainPlugin_);
    SerializeOptionalValue(archive, "EngineParameters", engineParameters_);
}

unsigned LaunchConfiguration::ToHash() const
{
    unsigned hash = 0;
    CombineHash(hash, MakeHash(name_));
    CombineHash(hash, MakeHash(mainPlugin_));
    CombineHash(hash, MakeHash(engineParameters_));
    return hash;
}

LaunchManager::LaunchManager(Context* context)
    : Object(context)
{
}

LaunchManager::~LaunchManager()
{
}

void LaunchManager::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "Configurations", configurations_);
}

void LaunchManager::AddConfiguration(const LaunchConfiguration& configuration)
{
    configurations_.push_back(configuration);
}

void LaunchManager::RemoveConfiguration(unsigned index)
{
    configurations_.erase_at(index);
}

const LaunchConfiguration* LaunchManager::FindConfiguration(const ea::string& name) const
{
    const auto iter = ea::find_if(configurations_.begin(), configurations_.end(),
        [&name](const LaunchConfiguration& configuration) { return configuration.name_ == name; });
    return iter != configurations_.end() ? &*iter : nullptr;
}

bool LaunchManager::HasConfiguration(const ea::string& name) const
{
    return FindConfiguration(name) != nullptr;
}

StringVector LaunchManager::GetSortedConfigurations() const
{
    StringVector result;
    ea::transform(configurations_.begin(), configurations_.end(), ea::back_inserter(result),
        [](const LaunchConfiguration& configuration) { return configuration.name_; });
    ea::sort(result.begin(), result.end());
    result.erase(ea::unique(result.begin(), result.end()), result.end());
    return result;
}

}
