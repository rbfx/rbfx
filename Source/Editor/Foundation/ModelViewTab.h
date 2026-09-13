// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Foundation/Shared/CustomSceneViewTab.h"
#include "../Project/Project.h"
#include "../Project/ResourceEditorTab.h"

#include <Urho3D/Graphics/CameraOperator.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Model.h>

namespace Urho3D
{

void Foundation_ModelViewTab(Context* context, Project* project);

/// Tab that renders Scene and enables Scene manipulation.
class ModelViewTab : public CustomSceneViewTab
{
    URHO3D_OBJECT(ModelViewTab, CustomSceneViewTab)

public:
    explicit ModelViewTab(Context* context);
    ~ModelViewTab() override;

    /// ResourceEditorTab implementation
    /// @{
    ea::string GetResourceTitle() { return "Model"; }
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

private:
    SharedPtr<Model> model_;
    SharedPtr<Node> modelNode_;
    SharedPtr<StaticModel> staticModel_;
    SharedPtr<CameraOperator> cameraOperator_;
};

} // namespace Urho3D
