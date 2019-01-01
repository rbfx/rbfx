//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Resource/JSONValue.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/JSONFile.h>
#include "Project.h"
#include "Converter.h"
#include "Pipeline.h"

namespace Urho3D
{

static const char* AssetDatabaseFile = "AssetDatabase.json";

Pipeline::Pipeline(Context* context)
    : Serializable(context)
    , watcher_(context)
{
    SubscribeToEvent(E_ENDFRAME, [this](StringHash, VariantMap&) {
        DispatchChangedAssets();
    });
}

Pipeline::~Pipeline()
{
    GetWorkQueue()->Complete(0);
}

bool Pipeline::LoadJSON(const JSONValue& source)
{
    if (source.GetValueType() != JSON_ARRAY)
        return false;

    for (auto i = 0U; i < source.Size(); i++)
    {
        const JSONValue& converterData = source[i];
        StringHash type = Converter::GetSerializedType(converterData);
        if (type == StringHash::ZERO)
            return false;

        SharedPtr<Converter> converter = DynamicCast<Converter>(context_->CreateObject(type));
        if (converter.Null() || !converter->LoadJSON(converterData))
            return false;

        converters_.Push(converter);
    }

    return true;
}

void Pipeline::EnableWatcher()
{
    auto* project = GetSubsystem<Project>();
    GetFileSystem()->CreateDirsRecursive(project->GetCachePath());
    watcher_.StartWatching(project->GetResourcePath(), true);
}

void Pipeline::BuildCache(ConverterKinds converterKinds)
{
    auto* project = GetSubsystem<Project>();
    StringVector filesPaths;
    GetFileSystem()->ScanDir(filesPaths, project->GetResourcePath(), "*", SCAN_FILES, true);
    ConvertAssets(filesPaths, converterKinds);
}

void Pipeline::ConvertAssets(const StringVector& resourceNames, ConverterKinds converterKinds)
{
    for (auto& converter : converters_)
    {
        if ((converterKinds & converter->GetKind()) == converter->GetKind())
        {
            GetWorkQueue()->AddWorkItem([&converter, resourceNames]() {
                converter->Execute(resourceNames);
            });
        }
    }
}

void Pipeline::DispatchChangedAssets()
{
    StringVector modified;
    FileChange entry;
    while (watcher_.GetNextChange(entry))
    {
        if (entry.fileName_ == AssetDatabaseFile)
            continue;

        if (entry.kind_ != FILECHANGE_REMOVED)
            modified.EmplaceBack(std::move(entry.fileName_));
    }

    if (!modified.Empty())
    {
        auto* project = GetSubsystem<Project>();
        ConvertAssets(modified, CONVERTER_ONLINE);
    }
}

}
