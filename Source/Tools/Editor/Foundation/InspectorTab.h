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

#include "../Core/ResourceDragDropPayload.h"
#include "../Project/EditorTab.h"
#include "../Project/ProjectEditor.h"
#include "../Foundation/Shared/InspectorSource.h"

namespace Urho3D
{

void Foundation_InspectorTab(Context* context, ProjectEditor* projectEditor);

class InspectorTab_;

/// Addon to inspector tab that can provide content.
class InspectorAddon : public Object, public InspectorSource
{
    URHO3D_OBJECT(InspectorAddon, Object);

public:
    explicit InspectorAddon(InspectorTab_* owner);
    ~InspectorAddon() override;

    /// Activate inspector for the addon.
    void Activate();

protected:
    WeakPtr<InspectorTab_> owner_;
};

/// Tab that hosts inspectors of any kind.
/// TODO(editor): Rename
class InspectorTab_ : public EditorTab
{
    URHO3D_OBJECT(InspectorTab_, EditorTab);

public:
    explicit InspectorTab_(Context* context);

    /// Register new inspector addon.
    void RegisterAddon(const SharedPtr<InspectorAddon>& addon);
    template <class T, class ... Args> InspectorAddon* RegisterAddon(const Args&... args);

    /// Connect to data source.
    void ConnectToSource(Object* source, InspectorSource* sourceInterface);
    template <class T> void ConnectToSource(T* source) { ConnectToSource(source, source); }

    /// Implement EditorTab
    /// @{
    void RenderMenu() override;
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override;
    EditorTab* GetOwnerTab() override { return source_ ? sourceInterface_->GetOwnerTab() : nullptr; }
    /// @}

protected:
    /// Implement EditorTab
    /// @{
    void RenderContent() override;
    void RenderContextMenuItems() override;
    /// @}

private:
    ea::vector<SharedPtr<InspectorAddon>> addons_;
    WeakPtr<Object> source_;

    InspectorSource* sourceInterface_{};
};

template <class T, class ... Args>
InspectorAddon* InspectorTab_::RegisterAddon(const Args&... args)
{
    const auto addon = MakeShared<T>(this, args...);
    RegisterAddon(addon);
    return addon;
}

}
