// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Project/EditorTab.h"
#include "../Project/Project.h"
#include "../Foundation/Shared/InspectorSource.h"

namespace Urho3D
{

void Foundation_InspectorTab(Context* context, Project* project);

/// Tab that hosts inspectors of any kind.
class InspectorTab : public EditorTab
{
    URHO3D_OBJECT(InspectorTab, EditorTab);

public:
    explicit InspectorTab(Context* context);

    /// Register new inspector addon.
    void RegisterAddon(const SharedPtr<Object>& addon);
    template <class T, class ... Args> T* RegisterAddon(const Args&... args);

    /// Connect to source when activated.
    template <class T> void SubscribeOnActivation(T* source);

    /// Connect to data source.
    void ConnectToSource(Object* source, InspectorSource* sourceInterface);
    template <class T> void ConnectToSource(T* source) { ConnectToSource(source, source); }

    /// Implement EditorTab
    /// @{
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    void RenderMenu() override;
    void RenderContent() override;
    void RenderContextMenuItems() override;

    bool IsUndoSupported() override { return source_ ? sourceInterface_->IsUndoSupported() : false; }
    EditorTab* GetOwnerTab() override { return source_ ? sourceInterface_->GetOwnerTab() : nullptr; }
    /// @}

private:
    ea::vector<SharedPtr<Object>> addons_;
    WeakPtr<Object> source_;

    InspectorSource* sourceInterface_{};
};

template <class T, class ... Args>
T* InspectorTab::RegisterAddon(const Args&... args)
{
    const auto addon = MakeShared<T>(args...);
    RegisterAddon(addon);
    SubscribeOnActivation(addon.Get());
    return addon;
}

template <class T>
void InspectorTab::SubscribeOnActivation(T* source)
{
    WeakPtr<T> sourceWeak{source};
    source->OnActivated.Subscribe(this,
        [sourceWeak](InspectorTab* inspectorTab)
    {
        if (sourceWeak)
            inspectorTab->ConnectToSource(sourceWeak.Get());
    });
}

}
