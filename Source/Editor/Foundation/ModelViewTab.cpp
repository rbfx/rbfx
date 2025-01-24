//
// Copyright (c) 2022-2022 the rbfx project.
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
