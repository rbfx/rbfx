// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/CommonEditorActions.h"
#include "../../Foundation/InspectorTab.h"
#include "../../Project/ModifyResourceAction.h"
#include "../../Project/ProjectRequest.h"

#include <Urho3D/SystemUI/SerializableInspectorWidget.h>
#include <Urho3D/Utility/AssetPipeline.h>

namespace Urho3D
{

void Foundation_AssetPipelineInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class AssetPipelineInspector : public Object, public InspectorSource
{
    URHO3D_OBJECT(AssetPipelineInspector, Object)

public:
    explicit AssetPipelineInspector(Project* project);

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

    void RenderInspector(SerializableInspectorWidget* inspector);
    void RenderAddTransformer();
    void RenderFinalButtons();

    bool HasPendingChanges() const;
    void BeginChange();
    void Apply();
    void Discard();

    void BeginEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);
    void EndEditAttribute(const WeakSerializableVector& objects, const AttributeInfo* attribute);

    WeakPtr<Project> project_;

    ea::string resourceName_;
    SharedPtr<AssetPipeline> resource_;

    ea::vector<WeakPtr<AssetTransformer>> transformers_;
    ea::vector<SharedPtr<SerializableInspectorWidget>> inspectorWidgets_;

    SharedPtr<ModifyResourceAction> pendingAction_{};
    ea::vector<WeakPtr<AssetTransformer>> pendingRemoves_;
    ea::vector<StringHash> pendingAdds_;
};

}
