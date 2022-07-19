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
