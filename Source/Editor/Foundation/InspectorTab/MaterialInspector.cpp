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

void Foundation_MaterialInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<MaterialInspector>(inspectorTab->GetProject());
}

MaterialInspector::MaterialInspector(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.Subscribe(this, &MaterialInspector::OnProjectRequest);
}

void MaterialInspector::OnProjectRequest(ProjectRequest* request)
{
    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest || inspectResourceRequest->GetResources().empty())
        return;

    const auto& resources = inspectResourceRequest->GetResources();

    const bool areAllMaterials = ea::all_of(resources.begin(), resources.end(),
        [](const ResourceFileDescriptor& desc) { return desc.HasObjectType<Material>(); });
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
        OnActivated(this);
    });
}

void MaterialInspector::InspectResources()
{
    auto cache = GetSubsystem<ResourceCache>();

    MaterialInspectorWidget::MaterialVector materials;
    for (const ea::string& resourceName : resourceNames_)
    {
        if (auto material = cache->GetResource<Material>(resourceName))
            materials.emplace_back(material);
    }

    resourceNames_.clear();
    for (Material* material : materials)
        resourceNames_.emplace_back(material->GetName());

    if (materials.empty())
    {
        widget_ = nullptr;
        return;
    }

    widget_ = MakeShared<MaterialInspectorWidget>(context_, materials);
    widget_->UpdateTechniques(techniquePath_);
    widget_->OnEditBegin.Subscribe(this, &MaterialInspector::BeginEdit);
    widget_->OnEditEnd.Subscribe(this, &MaterialInspector::EndEdit);
}

void MaterialInspector::BeginEdit()
{
    // Incomplete action will include all the changes automatically
    if (pendingAction_ && !pendingAction_->IsComplete())
        return;

    auto undoManager = project_->GetUndoManager();

    pendingAction_ = MakeShared<ModifyResourceAction>(project_);
    for (Material* material : widget_->GetMaterials())
        pendingAction_->AddResource(material);

    // Initialization of "redo" state is delayed so it's okay to push the action here
    undoManager->PushAction(pendingAction_);
}

void MaterialInspector::EndEdit()
{
    for (Material* material : widget_->GetMaterials())
        project_->SaveFileDelayed(material);
}

void MaterialInspector::RenderContent()
{
    if (!widget_)
        return;

    if (updateTimer_.GetMSec(false) > updatePeriodMs_)
    {
        widget_->UpdateTechniques(techniquePath_);
        updateTimer_.Reset();
    }

    widget_->RenderTitle();
    ui::Separator();
    widget_->RenderContent();
}

void MaterialInspector::RenderContextMenuItems()
{
}

void MaterialInspector::RenderMenu()
{
}

void MaterialInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

}
