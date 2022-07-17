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

#include "../Project/ProjectRequest.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <EASTL/sort.h>

namespace Urho3D
{

bool ProjectRequest::CallbackDesc::operator<(const CallbackDesc& other) const
{
    return priority_ < other.priority_;
}

ProjectRequest::ProjectRequest(Context* context)
    : Object(context)
{
}

void ProjectRequest::QueueProcessCallback(const Callback& callback, int priority)
{
    callbacks_.emplace(CallbackDesc{callback, priority});
}

void ProjectRequest::InvokeProcessCallback()
{
    if (!callbacks_.empty())
    {
        const auto& callback = callbacks_.top().callback_;
        callback();
    }
}

FileResourceDesc::FileResourceDesc(Context* context, const ea::string& resourceName)
    : context_(context)
    , resourceName_(resourceName)
{
    auto cache = context_->GetSubsystem<ResourceCache>();

    if (auto file = cache->GetFile(resourceName_))
        fileName_ = file->GetAbsoluteName();
}

SharedPtr<File> FileResourceDesc::GetBinaryFile() const
{
    // Don't cache File to avoid races between users
    auto cache = context_->GetSubsystem<ResourceCache>();
    return cache->GetFile(resourceName_);
}

SharedPtr<XMLFile> FileResourceDesc::GetXMLFile() const
{
    if (!xmlFile_ && resourceName_.ends_with(".xml", false))
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        auto file = cache->GetFile(resourceName_);
        xmlFile_ = MakeShared<XMLFile>(context_);
        xmlFile_->Load(*file);
    }
    return xmlFile_;
}

SharedPtr<JSONFile> FileResourceDesc::GetJSONFile() const
{
    if (!jsonFile_ && resourceName_.ends_with(".json", false))
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        auto file = cache->GetFile(resourceName_);
        jsonFile_ = MakeShared<JSONFile>(context_);
        jsonFile_->Load(*file);
    }
    return jsonFile_;
}

bool FileResourceDesc::HasExtension(ea::span<const ea::string_view> extensions) const
{
    return ea::any_of(extensions.begin(), extensions.end(),
        [this](const ea::string_view& ext) { return resourceName_.ends_with(ext, false); });
}

bool FileResourceDesc::HasExtension(std::initializer_list<ea::string_view> extensions) const
{
    return HasExtension(ea::span<const ea::string_view>(extensions));
}

ea::string FileResourceDesc::GetTypeHint() const
{
    if (const auto xmlFile = GetXMLFile())
        return xmlFile->GetRoot().GetName();
    return EMPTY_STRING;
}

OpenResourceRequest::OpenResourceRequest(Context* context, const ea::string& resourceName)
    : ProjectRequest(context)
    , FileResourceDesc{context, resourceName}
{
}

InspectResourceRequest::InspectResourceRequest(Context* context, const ea::vector<ea::string>& resourceNames)
    : BaseInspectRequest(context)
{
    ea::transform(resourceNames.begin(), resourceNames.end(), ea::back_inserter(resourceDescs_),
        [&](const ea::string& resourceName) { return FileResourceDesc{context, resourceName}; });
}

StringVector InspectResourceRequest::GetSortedResourceNames() const
{
    StringVector resourceNames;
    ea::transform(resourceDescs_.begin(), resourceDescs_.end(), ea::back_inserter(resourceNames),
        [](const FileResourceDesc& desc) { return desc.GetResourceName(); });
    ea::sort(resourceNames.begin(), resourceNames.end());
    return resourceNames;
}

Scene* InspectNodeComponentRequest::GetCommonScene() const
{
    Scene* scene = nullptr;

    for (const Node* node : nodes_)
    {
        if (!node)
            continue;

        Scene* nodeScene = node->GetScene();
        if (!nodeScene || (scene && scene != nodeScene))
            return nullptr;

        scene = nodeScene;
    }

    for (const Component* component : components_)
    {
        if (!component)
            continue;

        Scene* componentScene = component->GetScene();
        if (!componentScene || (scene && scene != componentScene))
            return nullptr;

        scene = componentScene;
    }

    return scene;
}

}
