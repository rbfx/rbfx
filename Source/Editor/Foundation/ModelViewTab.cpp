// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/ModelViewTab.h"

#include "../Core/IniHelpers.h"

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/CameraOperator.h>
#include <Urho3D/Resource/ResourceCache.h>

namespace Urho3D
{

namespace
{
}

void Foundation_ModelViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<ModelViewTab>(context));
}

ModelViewTab::ModelViewTab(Context* context)
    : CustomSceneViewTab(context, "Model", "1c4962de-c75c-41fa-bf3f-5bb3f2ba7d53",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
{
    modelNode_ = GetScene()->CreateChild("Model");
    cameraOperator_ = GetCamera()->GetNode()->CreateComponent<CameraOperator>();
    cameraOperator_->SetBoundingBoxTrackingEnabled(true);
    cameraOperator_->SetEnabled(false);
    staticModel_ = modelNode_->CreateComponent<StaticModel>();
    staticModel_->SetCastShadows(true);
}

ModelViewTab::~ModelViewTab()
{
}

bool ModelViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    return desc.HasObjectType<Model>();
}

void ModelViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    model_ = cache->GetResource<Model>(resourceName);
    staticModel_->SetModel(model_);
    ResetCamera();
}

void ModelViewTab::ResetCamera()
{
    if (model_)
    {
        cameraOperator_->SetBoundingBox(model_->GetBoundingBox());
        cameraOperator_->MoveCamera();
        state_.lastCameraPosition_ = GetCamera()->GetNode()->GetPosition();
    }
}

void ModelViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    model_.Reset();
    staticModel_->SetModel(model_);
}

void ModelViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void ModelViewTab::OnResourceSaved(const ea::string& resourceName)
{
}

void ModelViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
