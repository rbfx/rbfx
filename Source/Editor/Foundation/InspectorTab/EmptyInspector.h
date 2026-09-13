// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../../Foundation/InspectorTab.h"

namespace Urho3D
{

void Foundation_EmptyInspector(Context* context, InspectorTab* inspectorTab);

/// Sink to reset inspector on failed inspector request.
class EmptyInspector : public Object, public InspectorSource
{
    URHO3D_OBJECT(EmptyInspector, Object)

public:
    explicit EmptyInspector(Project* project);

    /// Implement InspectorSource
    /// @{
    EditorTab* GetOwnerTab() override { return nullptr; }

    void RenderContent() override;
    void RenderContextMenuItems() override;
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    /// @}

private:
    void OnProjectRequest(ProjectRequest* request);
};

}
