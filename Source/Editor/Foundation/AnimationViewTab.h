// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Foundation/Shared/CustomSceneViewTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/Model.h>

namespace Urho3D
{

void Foundation_AnimationViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class AnimationViewTab : public CustomSceneViewTab
{
    URHO3D_OBJECT(AnimationViewTab, CustomSceneViewTab)

public:
    explicit AnimationViewTab(Context* context);
    ~AnimationViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    void RenderContent() override;

    ea::string GetResourceTitle() { return "Animation"; }
    bool SupportMultipleResources() { return false; }
    bool CanOpenResource(const ResourceFileDescriptor& desc) override;
    void ResetCamera() override;
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

    void RenderTitle() override;

private:
    SharedPtr<Model> model_;
    SharedPtr<Animation> animation_;
    Node* modelNode_;
    AnimatedModel* animatedModel_;
    AnimationController* animationController_;
};

} // namespace Urho3D
