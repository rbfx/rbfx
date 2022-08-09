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

#include "InspectorWithPreview.h"

#include "Urho3D/Resource/ResourceCache.h"

namespace Urho3D
{

void Foundation_InspectorWithPreview(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<InspectorWithPreview>(inspectorTab->GetProject());
}

InspectorWithPreview::InspectorWithPreview(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.Subscribe(this, &InspectorWithPreview::OnProjectRequest);
}

void InspectorWithPreview::OnProjectRequest(ProjectRequest* request)
{
    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);

    if (!inspectResourceRequest)
        return;

    const auto& resources = inspectResourceRequest->GetResources();

    if (resources.empty())
        return;

    const auto resourceType = GetResourceType();
    const bool areAllSameType = ea::all_of(resources.begin(), resources.end(),
        [=](const ResourceFileDescriptor& desc) { return desc.HasObjectType(resourceType); });
    if (!areAllSameType)
        return;

    request->QueueProcessCallback(
        [=]()
        {
            const auto resourceNames = inspectResourceRequest->GetSortedResourceNames();
            if (resourceNames_ != resourceNames)
            {
                resourceNames_ = resourceNames;
                InspectResources();
            }
            OnActivated(this);
        },
        M_MIN_INT + 1);
}

void InspectorWithPreview::InspectResources()
{
    auto cache = GetSubsystem<ResourceCache>();

    ResourceVector resources;
    for (const ea::string& resourceName : resourceNames_)
    {
        if (auto material = cache->GetResource(GetResourceType(), resourceName))
            resources.emplace_back(material);
    }

    if (resources.empty())
    {
        widget_ = nullptr;
        return;
    }

    widget_ = MakeWidget(resources);
    widget_->OnEditBegin.Subscribe(this, &InspectorWithPreview::BeginEdit);
    widget_->OnEditEnd.Subscribe(this, &InspectorWithPreview::EndEdit);
}

void InspectorWithPreview::BeginEdit()
{
    // Incomplete action will include all the changes automatically
    if (pendingAction_ && !pendingAction_->IsComplete())
        return;

    auto undoManager = project_->GetUndoManager();

    pendingAction_ = MakeShared<ModifyResourceAction>(project_);
    for (Resource* material : widget_->GetResources())
        pendingAction_->AddResource(material);

    // Initialization of "redo" state is delayed so it's okay to push the action here
    undoManager->PushAction(pendingAction_);
}

void InspectorWithPreview::EndEdit()
{
    for (Resource* material : widget_->GetResources())
        project_->SaveFileDelayed(material);
}

void InspectorWithPreview::RenderContent()
{
    if (!widget_)
        return;

    auto& resources = widget_->GetResources();

    widget_->RenderTitle();
    if (resources.size() == 1)
    {
        auto& singleResource = resources.front();
        if (ui::Button("Open"))
        {
            auto request = MakeShared<OpenResourceRequest>(context_, singleResource->GetName());
            project_->ProcessRequest(request);
        }
    }
    ui::Separator();
    widget_->RenderContent();
    if (resources.size() == 1)
    {
        ui::Separator();
        RenderPreview(resources.front());
    }
}

void InspectorWithPreview::RenderContextMenuItems()
{
}

void InspectorWithPreview::RenderMenu()
{
}

void InspectorWithPreview::RenderPreview(Resource* resource)
{
}

void InspectorWithPreview::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

} // namespace Urho3D
