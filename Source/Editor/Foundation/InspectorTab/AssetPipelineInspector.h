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
