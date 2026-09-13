// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Foundation/Shared/CustomSceneViewTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Graphics/StaticModel.h>

namespace Urho3D
{

void Foundation_TextureCubeViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class TextureCubeViewTab : public CustomSceneViewTab
{
    URHO3D_OBJECT(TextureCubeViewTab, CustomSceneViewTab)

public:
    explicit TextureCubeViewTab(Context* context);
    ~TextureCubeViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;

    ea::string GetResourceTitle() { return "Cubemap"; }
    bool SupportMultipleResources() { return false; }
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
    void RenderTextureCube(TextureCube* texture);
private:
    SharedPtr<TextureCube> textureCube_;
};

} // namespace Urho3D
