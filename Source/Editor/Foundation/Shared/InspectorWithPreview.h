// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/InspectorTab.h"
#include "Editor/Project/ModifyResourceAction.h"

#include <Urho3D/SystemUI/ResourceInspectorWidget.h>

namespace Urho3D
{

/// Simple default inspector for selected resources.
class InspectorWithPreview
    : public Object
    , public InspectorSource
{
    URHO3D_OBJECT(InspectorWithPreview, Object)

public:
    using ResourceVector = ResourceInspectorWidget::ResourceVector;

    explicit InspectorWithPreview(Project* project);

    /// Implement InspectorSource
    /// @{
    EditorTab* GetOwnerTab() override { return nullptr; }

    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

protected:
    virtual StringHash GetResourceType() const { return 0; }
    virtual SharedPtr<ResourceInspectorWidget> MakeInspectorWidget(const ResourceVector& resources) { return nullptr; }
    virtual SharedPtr<BaseWidget> MakePreviewWidget(Resource* resource) { return nullptr; }

private:
    void OnProjectRequest(ProjectRequest* request);
    void InspectResources();
    void BeginEdit();
    void EndEdit();

    WeakPtr<Project> project_;

    StringVector resourceNames_;

    SharedPtr<ResourceInspectorWidget> inspector_;
    SharedPtr<BaseWidget> preview_;
    SharedPtr<ModifyResourceAction> pendingAction_;
};

} // namespace Urho3D
