// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Core/CommonEditorActions.h"
#include "../../Foundation/InspectorTab.h"
#include "../../Project/ModifyResourceAction.h"

#include <Urho3D/Core/Timer.h>
#include <Urho3D/SystemUI/MaterialInspectorWidget.h>

namespace Urho3D
{

void Foundation_MaterialInspector(Context* context, InspectorTab* inspectorTab);

/// Scene hierarchy provider for hierarchy browser tab.
class MaterialInspector : public Object, public InspectorSource
{
    URHO3D_OBJECT(MaterialInspector, Object)

public:
    explicit MaterialInspector(Project* project);

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

    void BeginEdit();
    void EndEdit();

    const unsigned updatePeriodMs_{1000};
    const ea::string techniquePath_{"Techniques/"};

    WeakPtr<Project> project_;

    StringVector resourceNames_;
    SharedPtr<MaterialInspectorWidget> widget_;
    Timer updateTimer_;

    SharedPtr<ModifyResourceAction> pendingAction_;
};

}
