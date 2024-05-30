// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/CommonEditorActions.h"
#include "../../Foundation/InspectorTab.h"
#include "../../Project/ModifyResourceAction.h"
#include "../../Project/ProjectRequest.h"

#include <Urho3D/RenderPipeline/RenderPath.h>
#include <Urho3D/SystemUI/SerializableInspectorWidget.h>

namespace Urho3D
{

void Foundation_RenderPathInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class RenderPathInspector
    : public Object
    , public InspectorSource
{
    URHO3D_OBJECT(RenderPathInspector, Object)

public:
    explicit RenderPathInspector(Project* project);

    /// Implement InspectorSource
    /// @{
    EditorTab* GetOwnerTab() override { return nullptr; }
    bool IsUndoSupported() override { return true; }

    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void OnProjectRequest(RefCounted* senderTab, ProjectRequest* request);
    void OnResourceReloaded();

    void EnsureInitialized();
    void InspectObjects();

    void RenderInspector(unsigned index, SerializableInspectorWidget* inspector);
    void RenderAddPass();

    bool HasPendingChanges() const;
    void BeginChange();

    void BeginEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void EndEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);

    WeakPtr<Project> project_;

    ea::string resourceName_;
    SharedPtr<RenderPath> resource_;

    ea::vector<WeakPtr<RenderPass>> passes_;
    ea::vector<SharedPtr<SerializableInspectorWidget>> inspectorWidgets_;

    SharedPtr<ModifyResourceAction> pendingAction_{};
    ea::vector<WeakPtr<RenderPass>> pendingRemoves_;
    ea::vector<ea::pair<WeakPtr<RenderPass>, unsigned>> pendingReorders_;
    ea::vector<StringHash> pendingAdds_;
    bool suppressReloadCallback_{};
};

} // namespace Urho3D
