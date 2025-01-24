// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/SerializableResourceInspector.h"

#include <Urho3D/Resource/SerializableResource.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

void Foundation_SerializableResourceInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<SerializableResourceInspector>(inspectorTab->GetProject());
}

SerializableResourceInspector::SerializableResourceInspector(Project* project)
    : Object(project->GetContext())
    , project_(project)
{
    project_->OnRequest.Subscribe(this, &SerializableResourceInspector::OnProjectRequest);
}

void SerializableResourceInspector::OnProjectRequest(ProjectRequest* request)
{
    const auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest || inspectResourceRequest->GetResources().empty())
        return;

    const auto& resources = inspectResourceRequest->GetResources();

    const bool areAllSerializableResources = ea::all_of(resources.begin(), resources.end(),
        [](const ResourceFileDescriptor& desc) { return desc.HasObjectType<SerializableResource>(); });
    if (!areAllSerializableResources)
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
    });
}

void SerializableResourceInspector::InspectResources()
{
    const auto cache = GetSubsystem<ResourceCache>();

    WeakSerializableVector serializableResources;
    resources_.clear();
    for (const ea::string& resourceName : resourceNames_)
    {
        if (auto* serializableResource = cache->GetResource<SerializableResource>(resourceName))
        {
            if (serializableResource->GetValue() != nullptr)
            {
                serializableResources.emplace_back(serializableResource->GetValue());
                resources_.emplace_back(serializableResource);
            }
        }
    }

    resourceNames_.clear();
    for (const auto& serializableResource : resources_)
    {
        resourceNames_.emplace_back(serializableResource->GetName());
    }

    if (serializableResources.empty())
    {
        widget_ = nullptr;
        return;
    }

    widget_ = MakeShared<SerializableInspectorWidget>(context_, serializableResources);
    widget_->OnEditAttributeBegin.Subscribe(this, &SerializableResourceInspector::OnEditAttributeBegin);
    widget_->OnEditAttributeEnd.Subscribe(this, &SerializableResourceInspector::OnEditAttributeEnd);
    widget_->OnActionBegin.Subscribe(this, &SerializableResourceInspector::OnActionBegin);
    widget_->OnActionEnd.Subscribe(this, &SerializableResourceInspector::OnActionEnd);
}

void SerializableResourceInspector::OnEditAttributeBegin(
    const WeakSerializableVector& objects, const AttributeInfo* attribute)
{
    CreateModifyResourceAction();
}

void SerializableResourceInspector::OnEditAttributeEnd(
    const WeakSerializableVector& objects, const AttributeInfo* attribute)
{
    SaveModifiedResources();
}

void SerializableResourceInspector::OnActionBegin(const WeakSerializableVector& objects)
{
    CreateModifyResourceAction();
}

void SerializableResourceInspector::OnActionEnd(const WeakSerializableVector& objects)
{
    SaveModifiedResources();
}

void SerializableResourceInspector::CreateModifyResourceAction()
{
    // Incomplete action will include all the changes automatically
    if (pendingAction_ && !pendingAction_->IsComplete())
        return;

    const auto undoManager = project_->GetUndoManager();

    pendingAction_ = MakeShared<ModifyResourceAction>(project_);
    for (auto& serializable : resources_)
        pendingAction_->AddResource(serializable);

    // Initialization of "redo" state is delayed so it's okay to push the action here
    undoManager->PushAction(pendingAction_);
}

void SerializableResourceInspector::SaveModifiedResources()
{
    for (auto& serializableResource : resources_)
        project_->SaveFileDelayed(serializableResource);
}

void SerializableResourceInspector::RenderContent()
{
    if (!widget_)
        return;

    widget_->RenderTitle();
    ui::Separator();
    widget_->RenderContent();
}

void SerializableResourceInspector::RenderContextMenuItems()
{
}

void SerializableResourceInspector::RenderMenu()
{
}

void SerializableResourceInspector::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

} // namespace Urho3D
