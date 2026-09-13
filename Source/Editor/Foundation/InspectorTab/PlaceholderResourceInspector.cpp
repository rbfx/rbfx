// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/PlaceholderResourceInspector.h"

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_PlaceholderResourceInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<PlaceholderResourceInspector>(inspectorTab->GetProject());
}

PlaceholderResourceInspector::PlaceholderResourceInspector(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.Subscribe(this, &PlaceholderResourceInspector::OnProjectRequest);
}

void PlaceholderResourceInspector::OnProjectRequest(ProjectRequest* request)
{
    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest || inspectResourceRequest->GetResources().empty())
        return;

    request->QueueProcessCallback([=]()
    {
        InspectResources(inspectResourceRequest->GetResources());
        OnActivated(this);
    }, M_MIN_INT + 1);
}

void PlaceholderResourceInspector::InspectResources(const ea::vector<ResourceFileDescriptor>& resources)
{
    if (resources.size() == 1)
    {
        const ResourceFileDescriptor& desc = resources.front();
        const ea::string resourceType = !desc.isDirectory_ ? "File" : "Folder";
        singleResource_ = SingleResource{resourceType, desc.resourceName_};
        multipleResources_ = ea::nullopt;
    }
    else
    {
        const unsigned numFolders = ea::count_if(resources.begin(), resources.end(),
            [](const ResourceFileDescriptor& desc) { return desc.isDirectory_; });
        const unsigned numFiles = resources.size() - numFolders;
        multipleResources_ = MultipleResources{numFiles, numFolders};
        singleResource_ = ea::nullopt;
    }
}

void PlaceholderResourceInspector::RenderContent()
{
    if (singleResource_)
    {
        if (ui::Button(Format("Open {}", singleResource_->resourceType_).c_str()))
        {
            auto request = MakeShared<OpenResourceRequest>(context_, singleResource_->resourceName_);
            project_->ProcessRequest(request);
        }

        ui::TextWrapped("%s", singleResource_->resourceName_.c_str());
    }
    else if (multipleResources_)
    {
        ui::Text("%u files selected", multipleResources_->numFiles_);
        ui::Text("%u folders selected", multipleResources_->numFolders_);
    }
}

void PlaceholderResourceInspector::RenderContextMenuItems()
{
}

void PlaceholderResourceInspector::RenderMenu()
{
}

void PlaceholderResourceInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
