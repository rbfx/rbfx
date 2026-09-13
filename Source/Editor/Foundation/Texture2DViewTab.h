// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Foundation/Shared/CustomSceneViewTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/SystemUI/Texture2DWidget.h>

namespace Urho3D
{

void Foundation_Texture2DViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class Texture2DViewTab : public CustomSceneViewTab
{
    URHO3D_OBJECT(Texture2DViewTab, CustomSceneViewTab)

public:
    explicit Texture2DViewTab(Context* context);
    ~Texture2DViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;

    ea::string GetResourceTitle() override { return "Texture2D"; }
    bool SupportMultipleResources() override { return false; }
    bool CanOpenResource(const ResourceFileDescriptor& desc) override;
    /// @}

protected:
    /// ResourceEditorTab implementation
    /// @{
    void OnResourceLoaded(const ea::string& resourceName) override;
    void OnResourceUnloaded(const ea::string& resourceName) override;
    void OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName) override;
    void OnResourceSaved(const ea::string& resourceName) override;
    void OnResourceShallowSaved(const ea::string& resourceName) override;
    /// @}

private:
    SharedPtr<Texture2DWidget> preview_;
};

} // namespace Urho3D
