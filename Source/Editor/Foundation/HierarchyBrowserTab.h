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
