// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/ModelInspector.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/SystemUI/ModelInspectorWidget.h>
#include <Urho3D/SystemUI/SceneWidget.h>
#include <Urho3D/Graphics/VertexBuffer.h>

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

class ModelInspectorSceneWidget : public SceneWidget
{
public:
    ModelInspectorSceneWidget(Context* context)
        : SceneWidget(context)
    {
    }

    ~ModelInspectorSceneWidget() {}

    void SetModel(Model* model)
    {
        model_ = model;
    }

    void RenderContent() override
    {
        SceneWidget::RenderContent();

        if (model_)
        {
            auto& vertexBuffers = model_->GetVertexBuffers();

            if (ui::TreeNode("Vertex Info"))
            {
                for (unsigned vertexBufferIndex = 0; vertexBufferIndex < vertexBuffers.size(); ++vertexBufferIndex)
                {
                    const auto& vb = vertexBuffers[vertexBufferIndex];
                    const float kb = (vb->GetVertexCount() * vb->GetVertexSize()) / 1000.0f;
                    ui::Text("VertexBuffer[%u]: %u vertices (%.1f KB)", vertexBufferIndex, vb->GetVertexCount(), kb);

                    const float indent = 5;
                    ui::Indent(indent);

                    for (auto& ve : vb->GetElements())
                    {
                        ui::Text("%s (%s)", vertexElementSemanticNames[ve.semantic_], vertexElementTypeNames[ve.type_]);
                    }

                    ui::Indent(-indent);
                }

                if (!vertexBuffers.empty())
                {
                    ui::NewLine();
                }

                ui::TreePop();
            }
        }
    }

private:
    Model* model_ = nullptr;
};

SharedPtr<BaseWidget> ModelInspector::MakePreviewWidget(Resource* resource)
{
    auto sceneWidget = MakeShared<ModelInspectorSceneWidget>(context_);
    auto scene = sceneWidget->CreateDefaultScene();
    auto modelNode = scene->CreateChild("Model");
    auto staticModel = modelNode->CreateComponent<StaticModel>();
    auto model = static_cast<Model*>(resource);
    staticModel->SetModel(model);
    sceneWidget->SetModel(model);
    sceneWidget->LookAt(model->GetBoundingBox());
    return sceneWidget;
}

} // namespace Urho3D
