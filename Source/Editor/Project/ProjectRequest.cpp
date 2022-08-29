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

#include "../Project/Project.h"

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

OpenResourceRequest::OpenResourceRequest(Context* context, const ea::string& resourceName)
    : ProjectRequest(context)
{
    auto project = GetSubsystem<Project>();
    resourceDesc_ = project->GetResourceDescriptor(resourceName);
}

InspectResourceRequest::InspectResourceRequest(Context* context, const ea::vector<ea::string>& resourceNames)
    : BaseInspectRequest(context)
{
    auto project = GetSubsystem<Project>();
    ea::transform(resourceNames.begin(), resourceNames.end(), ea::back_inserter(resourceDescs_),
        [&](const ea::string& resourceName) { return project->GetResourceDescriptor(resourceName); });
}

StringVector InspectResourceRequest::GetSortedResourceNames() const
{
    StringVector resourceNames;
    ea::transform(resourceDescs_.begin(), resourceDescs_.end(), ea::back_inserter(resourceNames),
        [](const ResourceFileDescriptor& desc) { return desc.resourceName_; });
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

CreateResourceRequest::CreateResourceRequest(ResourceFactory* factory)
    : ProjectRequest(factory->GetContext())
    , factory_(factory)
{
}

}
