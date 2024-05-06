// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/CommonEditorActions.h"
#include "../../Foundation/InspectorTab.h"
#include "../../Project/ModifyResourceAction.h"

#include <Urho3D/Resource/SerializableResource.h>
#include <Urho3D/SystemUI/SerializableInspectorWidget.h>

namespace Urho3D
{

void Foundation_SerializableResourceInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class SerializableResourceInspector
    : public Object
    , public InspectorSource
{
    URHO3D_OBJECT(SerializableResourceInspector, Object)

public:
    explicit SerializableResourceInspector(Project* project);

    /// Implement InspectorSource
    /// @{
    bool IsUndoSupported() override { return true; }

    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void OnProjectRequest(ProjectRequest* request);
    void InspectResources();

    void OnEditAttributeBegin(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void OnEditAttributeEnd(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void OnActionBegin(const WeakSerializableVector& objects);
    void OnActionEnd(const WeakSerializableVector& objects);

    void CreateModifyResourceAction();
    void SaveModifiedResources();

    WeakPtr<Project> project_;

    StringVector resourceNames_;
    ea::vector<WeakPtr<SerializableResource>> resources_;
    SharedPtr<SerializableInspectorWidget> widget_;

    SharedPtr<ModifyResourceAction> pendingAction_;
};

} // namespace Urho3D
