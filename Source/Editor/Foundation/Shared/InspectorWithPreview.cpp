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
        inspector_ = nullptr;
        preview_ = nullptr;
        return;
    }

    inspector_ = MakeInspectorWidget(resources);
    if (inspector_)
    {
        inspector_->OnEditBegin.Subscribe(this, &InspectorWithPreview::BeginEdit);
        inspector_->OnEditEnd.Subscribe(this, &InspectorWithPreview::EndEdit);
    }
    if (resources.size() == 1 && resources.front() != nullptr)
    {
        preview_ = MakePreviewWidget(resources.front());
    }
    else
    {
        preview_ = nullptr;
    }
}

void InspectorWithPreview::BeginEdit()
{
    // Incomplete action will include all the changes automatically
    if (pendingAction_ && !pendingAction_->IsComplete())
        return;

    auto undoManager = project_->GetUndoManager();

    pendingAction_ = MakeShared<ModifyResourceAction>(project_);
    for (Resource* material : inspector_->GetResources())
        pendingAction_->AddResource(material);

    // Initialization of "redo" state is delayed so it's okay to push the action here
    undoManager->PushAction(pendingAction_);
}

void InspectorWithPreview::EndEdit()
{
    for (Resource* material : inspector_->GetResources())
        project_->SaveFileDelayed(material);
}

void InspectorWithPreview::RenderContent()
{
    if (!inspector_)
        return;

    auto& resources = inspector_->GetResources();

    const ImVec2 basePosition = ui::GetCursorPos();

    inspector_->RenderTitle();
    ui::Separator();

    const ImVec2 contentPosition = ui::GetCursorPos();
    const ImGuiContext& g = *GImGui;
    const ImGuiWindow* window = g.CurrentWindow;
    const ImRect rect = ImRound(window->ContentRegionRect);
    const auto contentSize = rect.GetSize() - ImVec2(0, contentPosition.y - basePosition.y + 5);

    if (preview_)
    {
        const ImVec2 previewSize(contentSize.x, contentSize.x);
        const ImVec2 inspectorSize(contentSize.x, contentSize.y - previewSize.y);
        if (inspectorSize.y > previewSize.y)
        {
            if (ui::BeginChild("inspector", inspectorSize, false, ImGuiWindowFlags_None))
            {
                auto& singleResource = resources.front();
                if (ui::Button("Open"))
                {
                    auto request = MakeShared<OpenResourceRequest>(context_, singleResource->GetName());
                    project_->ProcessRequest(request);
                }
                inspector_->RenderContent();
            }
            ui::EndChild();
            if (ui::BeginChild("preview", previewSize, false, ImGuiWindowFlags_None))
            {
                preview_->RenderContent();
            }
            ui::EndChild();
            return;
        }
    }

    if (ui::BeginChild("inspector", contentSize, false, ImGuiWindowFlags_None))
    {
        if (resources.size() == 1)
        {
            auto& singleResource = resources.front();
            if (ui::Button("Open"))
            {
                auto request = MakeShared<OpenResourceRequest>(context_, singleResource->GetName());
                project_->ProcessRequest(request);
            }
        }
        inspector_->RenderContent();
        if (preview_)
        {
            preview_->RenderContent();
        }
    }
    ui::EndChild();
}

void InspectorWithPreview::RenderContextMenuItems()
{
}

void InspectorWithPreview::RenderMenu()
{
}

void InspectorWithPreview::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

} // namespace Urho3D
