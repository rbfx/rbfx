// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/ModelInspector.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/SystemUI/ModelInspectorWidget.h>
#include <Urho3D/SystemUI/SceneWidget.h>

namespace Urho3D
{

void Foundation_ModelInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<ModelInspector>(inspectorTab->GetProject());
}

ModelInspector::ModelInspector(Project* project)
    : BaseClassName(project)
{
}

StringHash ModelInspector::GetResourceType() const
{
    return Model::GetTypeStatic();
}

SharedPtr<ResourceInspectorWidget> ModelInspector::MakeInspectorWidget(const ResourceVector& resources)
{
    return MakeShared<ModelInspectorWidget>(context_, resources);
}

SharedPtr<BaseWidget> ModelInspector::MakePreviewWidget(Resource* resource)
{
    auto sceneWidget = MakeShared<SceneWidget>(context_);
    auto scene = sceneWidget->CreateDefaultScene();
    auto modelNode = scene->CreateChild("Model");
    auto staticModel = modelNode->CreateComponent<StaticModel>();
    auto model = static_cast<Model*>(resource);
    staticModel->SetModel(model);
    sceneWidget->LookAt(model->GetBoundingBox());
    return sceneWidget;
}

} // namespace Urho3D
