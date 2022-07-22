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

namespace Urho3D
{

void Foundation_PlaceholderResourceInspector(Context* context, InspectorTab* inspectorTab);

/// Simple default inspector for selected resources.
class PlaceholderResourceInspector : public Object, public InspectorSource
{
    URHO3D_OBJECT(PlaceholderResourceInspector, Object)

public:
    explicit PlaceholderResourceInspector(Project* project);

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
    void InspectResources(const ea::vector<ResourceFileDescriptor>& resources);

    WeakPtr<Project> project_;

    struct SingleResource
    {
        ea::string resourceType_;
        ea::string resourceName_;
    };
    ea::optional<SingleResource> singleResource_;

    struct MultipleResources
    {
        unsigned numFiles_{};
        unsigned numFolders_{};
    };
    ea::optional<MultipleResources> multipleResources_;
};

}
