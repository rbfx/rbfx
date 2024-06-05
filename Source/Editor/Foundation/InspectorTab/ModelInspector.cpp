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
    ModelInspectorSceneWidget::ModelInspectorSceneWidget(Context* context)
        : SceneWidget(context)
    {
    }

    ModelInspectorSceneWidget::~ModelInspectorSceneWidget() {}

    void SetModel(Model* model)
    {
        model_ = model;
    }

    void RenderContent() override
    {
        if (model_)
        {
            auto& vertexBuffers = model_->GetVertexBuffers();
            if (!vertexBuffers.empty())
            {
                ui::NewLine();
            }

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
        }

        SceneWidget::RenderContent();
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
