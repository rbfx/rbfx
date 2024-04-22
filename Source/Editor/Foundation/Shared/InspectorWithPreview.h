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
