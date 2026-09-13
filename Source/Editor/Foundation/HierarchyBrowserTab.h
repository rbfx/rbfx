// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Foundation/Shared/HierarchyBrowserSource.h"
#include "../Project/EditorTab.h"
#include "../Project/Project.h"

namespace Urho3D
{

void Foundation_HierarchyBrowserTab(Context* context, Project* project);

/// Tab that hosts hierarchy display of any kind.
class HierarchyBrowserTab : public EditorTab
{
    URHO3D_OBJECT(HierarchyBrowserTab, EditorTab)

public:
    explicit HierarchyBrowserTab(Context* context);

    /// Connect to data source.
    void ConnectToSource(Object* source, HierarchyBrowserSource* sourceInterface);
    template <class T> void ConnectToSource(T* source) { ConnectToSource(source, source); }

    /// Implement EditorTab
    /// @{
    void RenderMenu() override;
    void RenderContent() override;
    void RenderContextMenuItems() override;

    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    EditorTab* GetOwnerTab() override { return source_ ? sourceInterface_->GetOwnerTab() : nullptr; }
    /// @}

private:
    WeakPtr<Object> source_;
    HierarchyBrowserSource* sourceInterface_{};
};

}
