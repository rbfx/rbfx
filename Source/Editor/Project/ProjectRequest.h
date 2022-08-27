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

#pragma once

#include "../Project/EditorTab.h"
#include "../Project/ResourceFactory.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/SystemUI/DragDropPayload.h>

#include <EASTL/priority_queue.h>
#include <EASTL/sort.h>
#include <EASTL/string.h>

namespace Urho3D
{

/// Base class for project-wide requests. Should be used from main thread only!
class ProjectRequest : public Object
{
    URHO3D_OBJECT(ProjectRequest, Object);

public:
    using Callback = ea::function<void()>;

    explicit ProjectRequest(Context* context);

    /// Queue callback with priority that can be used to process request.
    void QueueProcessCallback(const Callback& callback, int priority = 0);
    /// Invoke callback with highest priority.
    void InvokeProcessCallback();

private:
    struct CallbackDesc
    {
        Callback callback_{};
        int priority_{};

        bool operator<(const CallbackDesc& other) const;
    };

    ea::priority_queue<CallbackDesc> callbacks_;
};

/// Request to open resource.
class OpenResourceRequest : public ProjectRequest
{
    URHO3D_OBJECT(OpenResourceRequest, ProjectRequest);

public:
    OpenResourceRequest(Context* context, const ea::string& resourceName);

    const ResourceFileDescriptor& GetResource() const { return resourceDesc_; }

private:
    ResourceFileDescriptor resourceDesc_;
};

/// Base class for all inspector requests.
class BaseInspectRequest : public ProjectRequest
{
    URHO3D_OBJECT(BaseInspectRequest, ProjectRequest);

public:
    using ProjectRequest::ProjectRequest;
};

/// Request to inspect one or more resources.
class InspectResourceRequest : public BaseInspectRequest
{
    URHO3D_OBJECT(InspectResourceRequest, BaseInspectRequest);

public:
    InspectResourceRequest(Context* context, const ea::vector<ea::string>& resourceNames);

    const ea::vector<ResourceFileDescriptor>& GetResources() const { return resourceDescs_; }
    StringVector GetSortedResourceNames() const;

private:
    ea::vector<ResourceFileDescriptor> resourceDescs_;
};

/// Request to inspect one or more nodes or components.
class InspectNodeComponentRequest : public BaseInspectRequest
{
    URHO3D_OBJECT(InspectNodeComponentRequest, BaseInspectRequest);

public:
    using WeakNodeVector = ea::vector<WeakPtr<Node>>;
    using WeakComponentVector = ea::vector<WeakPtr<Component>>;

    template <class T, class U>
    InspectNodeComponentRequest(Context* context, const T& nodes, const U& components)
        : BaseInspectRequest(context)
    {
        for (Node* node : nodes)
        {
            if (node)
                nodes_.emplace_back(node);
        }
        for (Component* component : components)
        {
            if (component)
                components_.emplace_back(component);
        }

        ea::sort(nodes_.begin(), nodes_.end());
        ea::sort(components_.begin(), components_.end());
    }

    const WeakNodeVector& GetNodes() const { return nodes_; }
    const WeakComponentVector& GetComponents() const { return components_; }

    /// Return scene if all nodes and components belong to the same scene, null otherwise.
    Scene* GetCommonScene() const;

    bool IsEmpty() const { return nodes_.empty() && components_.empty(); }
    bool HasNodes() const { return !nodes_.empty(); }
    bool HasComponents() const { return !components_.empty(); }

private:
    WeakNodeVector nodes_;
    WeakComponentVector components_;
};

/// Request to create resource.
class CreateResourceRequest : public ProjectRequest
{
    URHO3D_OBJECT(CreateResourceRequest, ProjectRequest);

public:
    explicit CreateResourceRequest(ResourceFactory* factory);

    ResourceFactory* GetFactory() const { return factory_; }

private:
    SharedPtr<ResourceFactory> factory_;
};

}
