// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
