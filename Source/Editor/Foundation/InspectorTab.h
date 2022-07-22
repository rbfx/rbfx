//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
