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

#include "../../Foundation/InspectorTab/MaterialInspector.h"

#include <Urho3D/Resource/ResourceCache.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_MaterialInspector(Context* context, InspectorTab_* inspectorTab)
{
    inspectorTab->RegisterAddon<MaterialInspector_>();
}

MaterialInspector_::MaterialInspector_(InspectorTab_* owner)
    : InspectorAddon(owner)
{
    auto project = owner_->GetProject();
    project->OnRequest.Subscribe(this, &MaterialInspector_::OnProjectRequest);
}

void MaterialInspector_::OnProjectRequest(ProjectRequest* request)
{
    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest || inspectResourceRequest->GetResources().empty())
        return;

    const auto& resources = inspectResourceRequest->GetResources();

    const bool areAllMaterials = ea::all_of(resources.begin(), resources.end(),
        [](const FileResourceDesc& desc) { return desc.GetTypeHint() == "material"; });
    if (!areAllMaterials)
        return;

    request->QueueProcessCallback([=]()
    {
        const auto resourceNames = inspectResourceRequest->GetSortedResourceNames();
        if (resourceNames_ != resourceNames)
        {
            resourceNames_ = resourceNames;
            InspectResources();
        }
    });
}

void MaterialInspector_::InspectResources()
{
    auto cache = GetSubsystem<ResourceCache>();

    materials_.clear();
    for (const ea::string& resourceName : resourceNames_)
    {
        if (auto material = cache->GetResource<Material>(resourceName))
            materials_.emplace_back(material);
    }

    Activate();
}

void MaterialInspector_::RenderContent()
{
}

void MaterialInspector_::RenderContextMenuItems()
{
}

void MaterialInspector_::RenderMenu()
{
}

void MaterialInspector_::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
