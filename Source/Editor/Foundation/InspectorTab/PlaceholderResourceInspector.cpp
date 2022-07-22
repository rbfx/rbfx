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
        const unsigned numFolders = std::count_if(resources.begin(), resources.end(),
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
