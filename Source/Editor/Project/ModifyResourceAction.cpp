// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Project/ModifyResourceAction.h"

#include "../Project/Project.h"

#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

ModifyResourceAction::ModifyResourceAction(Project* project)
    : project_(project)
{
}

void ModifyResourceAction::AddResource(Resource* resource)
{
    URHO3D_ASSERT(newData_.empty());
    context_ = resource->GetContext();

    VectorBuffer buffer;
    if (resource->Save(buffer))
    {
        ResourceData oldData;
        oldData.resourceType_ = resource->GetType();
        oldData.fileName_ = resource->GetAbsoluteFileName();
        oldData.bytes_ = ea::make_shared<ByteVector>(buffer.GetBuffer());
        oldData_[resource->GetName()] = ea::move(oldData);
    }
}

void ModifyResourceAction::DisableAutoComplete()
{
    autoComplete_ = false;
}

void ModifyResourceAction::SaveOnComplete()
{
    saveOnComplete_ = true;
}

void ModifyResourceAction::Complete(bool force)
{
    if (!autoComplete_ && !force)
        return;

    auto cache = context_ ? context_->GetSubsystem<ResourceCache>() : nullptr;
    for (const auto& [resourceName, oldData] : oldData_)
    {
        if (Resource* resource = cache->GetResource(oldData.resourceType_, resourceName))
        {
            VectorBuffer buffer;
            if (resource->Save(buffer))
            {
                ResourceData newData;
                newData.resourceType_ = oldData.resourceType_;
                newData.fileName_ = oldData.fileName_;
                newData.bytes_ = ea::make_shared<ByteVector>(buffer.GetBuffer());
                newData_[resourceName] = ea::move(newData);
            }
        }
    }

    if (oldData_.size() != newData_.size())
        throw UndoException("ModifyResourceAction failed to complete action creation");

    if (saveOnComplete_)
    {
        for (const auto& [resourceName, data] : newData_)
            project_->SaveFileDelayed(data.fileName_, resourceName, data.bytes_);
    }
}

void ModifyResourceAction::Redo() const
{
    for (const auto& [resourceName, newData] : newData_)
        ApplyResourceData(resourceName, newData);
}

void ModifyResourceAction::Undo() const
{
    for (const auto& [resourceName, oldData] : oldData_)
        ApplyResourceData(resourceName, oldData);
}

bool ModifyResourceAction::MergeWith(const EditorAction& other)
{
    return false;
}

void ModifyResourceAction::ApplyResourceData(const ea::string& resourceName, const ResourceData& data) const
{
    auto cache = context_->GetSubsystem<ResourceCache>();
    if (Resource* resource = cache->GetResource(data.resourceType_, resourceName, false))
    {
        MemoryBuffer buffer{*data.bytes_};
        buffer.SetName(resourceName);
        resource->Load(buffer);
    }

    project_->SaveFileDelayed(data.fileName_, resourceName, data.bytes_);
}

}
